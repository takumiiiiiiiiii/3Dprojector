#include "GLApp.h"
#include "Globals.h"
#include "Shape.h"
#include "Objects.h"
#include "Voxcel.h"
#include "Ray.h"
#include "Model.h"
#include "TcpServer.h"
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <cmath>


//初期設定関数
namespace {
    struct EyeTcpData {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        bool valid = false;   // まだ一度もデータを受信していない場合は false
    };

    // ------------------------------------------------------------------
    // One Euro Filter (Casiez et al. 2012)
    // 静止時はしっかり平滑化してジッターを抑え、素早く動いたときは
    // カットオフ周波数を自動で上げて遅延を抑える適応的ローパスフィルタ。
    // ------------------------------------------------------------------
    struct OneEuroFilter1D {
        double mincutoff; // 最小カットオフ周波数(Hz)。小さいほど静止時に滑らか(遅延は増える)
        double beta;      // 速度に対する感度。大きいほど速い動きに素早く追従する
        double dcutoff;   // 速度推定用ローパスのカットオフ(Hz)。通常1.0で良い

        double x_prev = 0.0;
        double dx_prev = 0.0;
        bool   initialized = false;

        OneEuroFilter1D(double mincutoff_ = 10.0, double beta_ = 0.0, double dcutoff_ = 1.0)
            : mincutoff(mincutoff_), beta(beta_), dcutoff(dcutoff_) {}

        static double alpha(double cutoff, double dt) {
            double tau = 1.0 / (2.0 * M_PI * cutoff);
            return 1.0 / (1.0 + tau / dt);
        }

        double filter(double x, double dt) {
            if (dt <= 0.0) {
                dt = 1.0 / 60.0; // dtが異常な場合のガード
            }

            if (!initialized) {
                x_prev = x;
                dx_prev = 0.0;
                initialized = true;
                return x;
            }

            // 1. 速度を推定してローパス
            double dx = (x - x_prev) / dt;
            double a_d = alpha(dcutoff, dt);
            double dx_hat = a_d * dx + (1.0 - a_d) * dx_prev;

            // 2. 速度に応じてカットオフ周波数を上げる(速いほど遅延を減らす)
            double cutoff = mincutoff + beta * std::fabs(dx_hat);
            double a = alpha(cutoff, dt);
            double x_hat = a * x + (1.0 - a) * x_prev;

            x_prev = x_hat;
            dx_prev = dx_hat;
            return x_hat;
        }

        void reset() {
            initialized = false;
        }
    };

    struct OneEuroFilter3D {
        OneEuroFilter1D fx, fy, fz;
        std::chrono::steady_clock::time_point lastTime;
        bool hasLastTime = false;

        OneEuroFilter3D(double mincutoff = 1.0, double beta = 0.0, double dcutoff = 1.0)
            : fx(mincutoff, beta, dcutoff), fy(mincutoff, beta, dcutoff), fz(mincutoff, beta, dcutoff) {}

        Vec_3D update(const Vec_3D input) {
            auto now = std::chrono::steady_clock::now();
            double dt;
            if (!hasLastTime) {
                dt = 1.0 / 60.0; // 初回は仮のdt(60fps想定)
                hasLastTime = true;
            } else {
                dt = std::chrono::duration<double>(now - lastTime).count();
            }
            lastTime = now;

            Vec_3D out;
            out.x = static_cast<float>(fx.filter(input.x, dt));
            out.y = static_cast<float>(fy.filter(input.y, dt));
            out.z = static_cast<float>(fz.filter(input.z, dt));
            return out;
        }

        void setParams(double mincutoff, double beta, double dcutoff = 1.0) {
            fx.mincutoff = fy.mincutoff = fz.mincutoff = mincutoff;
            fx.beta = fy.beta = fz.beta = beta;
            fx.dcutoff = fy.dcutoff = fz.dcutoff = dcutoff;
        }

        void reset() {
            fx.reset(); fy.reset(); fz.reset();
            hasLastTime = false;
        }
    };


    std::mutex g_eyeMutex;
    EyeTcpData         g_latestEyeL;     // 受信スレッドが書き込む最新値
    EyeTcpData         g_latestEyeR;     // 受信スレッドが書き込む最新値

    OneEuroFilter3D eyeFilterL;
    OneEuroFilter3D eyeFilterR;

    std::thread        g_tcpThread;
    std::atomic<bool>  g_tcpThreadRunning{false};
 
    // このフレームで実際に使う目位置（display() の先頭で1回だけ確定させる）
    float g_eyeXL = 0.0f;
    float g_eyeYL = 0.0f;
    float g_eyeZL = 0.0f;
    float g_eyeXR = 0.0f;
    float g_eyeYR = 0.0f;
    float g_eyeZR = 0.0f;
}
 
// 受信専用スレッドの処理本体
static void TcpReceiveLoop()
{
    char buffer[1024];
    while (g_tcpThreadRunning.load())
    {
        int size = tcpServer.Receive(buffer, sizeof(buffer) - 1);
        if (size <= 0)
        {
            // データがまだ来ていない/切断された場合は少し待ってリトライ
            // （busy loopでCPUを無駄に使わないため）
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }
        buffer[size] = '\0';

        std::stringstream ss(buffer);
        std::string item;
        std::vector<std::string> data;
        while (std::getline(ss, item, ','))
        {
            data.push_back(item);
        }
        if (data.size() < 3)
        {
            continue; // 不完全なメッセージは破棄
        }

        try
        {
            EyeTcpData dL;
            dL.x = std::stof(data[0]);
            dL.y = std::stof(data[1]);
            dL.z = std::stof(data[2]);
            EyeTcpData dR;
            dR.x = std::stof(data[3]);
            dR.y = std::stof(data[4]);
            dR.z = std::stof(data[5]);
            dR.valid = true;
            dL.valid = true;

            std::lock_guard<std::mutex> lock(g_eyeMutex);
            g_latestEyeL = dL;
            g_latestEyeR = dR;
        }
        catch (const std::exception&)
        {
            // パース失敗（壊れたメッセージ等）は無視して次を待つ
            continue;
        }
    }
}

static void UpdateEyePositionFromShared()
{
    if (useTcp)
    {
        EyeTcpData dL, dR;
        {
            std::lock_guard<std::mutex> lock(g_eyeMutex);
            dL = g_latestEyeL;
            dR = g_latestEyeR;
        }
        if (dL.valid)
        {
            g_eyeXL = dL.x;
            g_eyeYL = -dL.y + cameraHeight; // カメラの位置を考慮して目の位置を調整
            g_eyeZL = dL.z - cameraDis;    // カメラの位置を考慮して目の位置を調整
        }
        if (dR.valid)
        {
            g_eyeXR = dR.x;
            g_eyeYR = -dR.y + cameraHeight; // カメラの位置を考慮して目の位置を調整
            g_eyeZR = dR.z - cameraDis;    // カメラの位置を考慮して目の位置を調整
        }   
        // d.valid が false（まだ一度も受信していない）の場合は
        // 前フレームの値をそのまま維持する＝映像が飛ばない
    }
    else
    {
        // TCPなし時は手動値（キーボード操作等で更新可能）を使う
        g_eyeXL = manualCenterX;
        g_eyeYL = manualCenterY;
        g_eyeZL = manualEyeDistance;
    }
}

void initGL()
{
    if (useTcp)
    {
        if (!tcpServer.Start(50000))
        {
            std::cerr << "TCPサーバーの起動に失敗しました" << std::endl;
            exit(1);
        }
        std::cout << "TCPサーバーが起動しました" << std::endl;
    }
    else
    {
        std::cout << "TCPなしモードで起動します" << std::endl;
    }
    //ウィンドウ生成
    
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);  //ディスプレイ表示モード指定
glutInitWindowSize(1920,1080);
int windowId = glutCreateWindow("CG Final");

// フルスクリーン

    std::cout << "windowId: " << windowId << std::endl;
  
    const GLubyte* version = glGetString(GL_VERSION);

    if (version == nullptr) {
        std::cout << "GL_VERSION is NULL" << std::endl;
    } else {
        std::cout << "GL_VERSION: " << reinterpret_cast<const char*>(version) << std::endl;
    }
    g_tcpThreadRunning = true;
    g_tcpThread = std::thread(TcpReceiveLoop);
//     glutCreateWindow("CG Final");  //ウィンドウ生成
// std::cout << "GL_VERSION: " << glGetString(GL_VERSION) << std::endl;

    // // モデル読み込み
    // // テクスチャをOpenGLへ登録するため、ウィンドウ生成後に読み込む
    // if (!model.Load("Jusmin_Lowpoly.obj"))
    // {
    //     std::cerr << "モデルの読み込みに失敗しました" << std::endl;
    //     exit(1);
    // }
    //キューブディスペンサーの初期化
   //glutFullScreen();  // ここでフルスクリーンに切り替え

    //コールバック関数指定
    glutDisplayFunc(display);  //ディスプレイコールバック関数（"display"）
    glutReshapeFunc(reshape);  //リサイズコールバック関数（"reshape"）
    glutTimerFunc(1000/f, timer, 0);  //タイマーコールバック関数（"timer", 1000/fミリ秒）
    glutPassiveMotionFunc(mouseMove);  //マウス移動コールバック関数
    glutMouseFunc(mouse);  //マウスクリックコールバック関数
    glutMotionFunc(motion);  //マウスドラッグコールバック関数
    // glutMouseWheelFunc(mouseWheel);
    glutKeyboardFunc(keyboard);  //キーボードコールバック関数
    
    //各種設定
    glClearColor(0.0,0.0,0.0, 1.0);  //ウィンドウクリア色の指定（RGBA値）
    glEnable(GL_DEPTH_TEST);  //デプスバッファ有効化
    glEnable(GL_NORMALIZE);  //ベクトル正規化有効化
    glEnable(GL_BLEND);  //ブレンディング有効化
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  //ブレンディング方法指定（アルファブレンディング）
    //陰影付け・光源
    glEnable(GL_LIGHTING);  //陰影付け有効化
    GLfloat col[4];  //光源設定用配列
    //ボクセルの初期化
    InitVoxcels();
    //光源0
    glEnable(GL_LIGHT0);  //光源0有効化
    col[0] = 0.8; col[1] = 0.8; col[2] = 0.8; col[3] = 1.0;
    glLightfv(GL_LIGHT0, GL_DIFFUSE, col);  //拡散反射対象
    glLightfv(GL_LIGHT0, GL_SPECULAR, col);  //鏡面反射対象
    col[0] = 0.2; col[1] = 0.2; col[2] = 0.2; col[3] = 1.0;
    glLightfv(GL_LIGHT0, GL_AMBIENT, col);  //環境光対象
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0000001);  //減衰率
    //光源1
    glEnable(GL_LIGHT1);  //光源1有効化
    col[0] = 0.8; col[1] = 0.8; col[2] = 0.8; col[3] = 1.0;
    glLightfv(GL_LIGHT1, GL_DIFFUSE, col);  //拡散反射対象
    glLightfv(GL_LIGHT1, GL_SPECULAR, col);  //鏡面反射対象
    col[0] = 0.2; col[1] = 0.2; col[2] = 0.2; col[3] = 1.0;
    glLightfv(GL_LIGHT1, GL_AMBIENT, col);  //環境光対象
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.0000001);  //減衰率
    //視点極座標
    eDist = 5000.0;  //距離
    eDegX = 20.0; eDegY = 180.0;  //x軸周り角度，y軸周り角度

    //床頂点座標
    for (int j=0; j<TILE; j++) {
        for (int i=0; i<TILE; i++) {
            fPoint[i][j].x = -fWidth/2.0+i*fWidth/(TILE-1);
            fPoint[i][j].y = 0.0;
            fPoint[i][j].z = -fWidth/2.0+j*fWidth/(TILE-1);
        }
    }

    cv::Mat textureImage;
    textureImage = cv::imread("AIT_Zoo.jpg", cv::IMREAD_COLOR);

    if (textureImage.empty()) {
        std::cout << "画像を読み込めませんでした: AIT_Zoo.jpg" << std::endl;
    } else {
        cv::cvtColor(textureImage, textureImage, cv::COLOR_BGR2RGB);
        glBindTexture(GL_TEXTURE_2D, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGB,
            textureImage.cols,
            textureImage.rows,
            0,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            textureImage.data
        );
    }

    glGenTextures(1, &leftTex);
    glBindTexture(GL_TEXTURE_2D, leftTex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        winW /2,
        winH,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    glGenTextures(1, &rightTex);
    glBindTexture(GL_TEXTURE_2D, rightTex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        winW / 2,
        winH,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    warpInitialized = true;

    // 目位置のOne Euro Filterパラメータ設定
    // mincutoff: 静止時のジッター抑制の強さ（小さいほど滑らかだが遅延増）
    // beta    : 速い動きへの追従性（大きいほど遅延が減るがジッター抑制が弱まる）
    // まずはこの値から試して、体感に応じて調整してください。
    eyeFilterL.setParams(/*mincutoff=*/1.0, /*beta=*/0.03);
    eyeFilterR.setParams(/*mincutoff=*/1.0, /*beta=*/0.03);

}

//ディスプレイコールバック関数
void display()
{
    UpdateEyePositionFromShared();
    initView(true);
    // オブジェクト描画
    dispobj();
    //テクスチャ
    glBindTexture(GL_TEXTURE_2D, leftTex);
    glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,winW/2,winH);
    initView(false);
    // オブジェクト描画
    dispobj();
    // initView(false);
    // dispobj();
    //テクスチャ
    glBindTexture(GL_TEXTURE_2D, rightTex);

    glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,winW/2,0,winW/2,winH);

    GLenum err = glGetError();
    if(!NormalView){
      DrawWarpedTextures();
    }

    glutSwapBuffers();
}
void initView(bool isLeftEye) {
    // 【変更なし】ここでTCP受信は行わない。display()先頭で確定させた
    // g_eyeX/Y/Z をそのまま使うだけ（左目・右目で同じ値になる）。
    float eyeX ;
    float eyeY ;
    float eyeZ;
    Vec_3D EyePos;
    if(isLeftEye){
        EyePos = {g_eyeXL,g_eyeYL,g_eyeZL};
        EyePos = eyeFilterL.update(EyePos);
        eyeX = EyePos.x;
        eyeY = EyePos.y;
        eyeZ = EyePos.z;
    } else {
        EyePos = {g_eyeXR,g_eyeYR,g_eyeZR};
        EyePos = eyeFilterR.update(EyePos);
        eyeX = EyePos.x;
        eyeY = EyePos.y;
        eyeZ = EyePos.z;
    }
    //目の位置を変更
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    int viewW = static_cast<int>(winW * rDisp);
    int viewH = static_cast<int>(winH * rDisp);
    double aspect = static_cast<double>(viewW) / static_cast<double>(viewH);
    if(useTcp){
    pe.x = -eyeX;
    pe.y = eyeY;
    pe.z = eyeZ;
    }

    if(isLeftEye){
        std::cout << "Left Eye Position: (" << pe.x << ", " << pe.y << ", " << pe.z << ")" << std::endl;
    }else{
        std::cout << "Right Eye Position: (" << pe.x << ", " << pe.y << ", " << pe.z << ")" << std::endl;
    }
    // 視点極座標から直交座標へ変換
    e.x = eDist * cos(eDegX * M_PI / 180.0) * sin(eDegY * M_PI / 180.0);
    e.y = eDist * sin(eDegX * M_PI / 180.0);
    e.z = eDist * cos(eDegX * M_PI / 180.0) * cos(eDegY * M_PI / 180.0);

    if(NormalView == true){
        viewW = static_cast<int>(winW * rDisp);
        viewH = static_cast<int>(winH * rDisp);
        aspect = static_cast<double>(viewW) / static_cast<double>(viewH);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, viewW,viewH);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glFrustum(
        -aspect, aspect,
        -1.0, 1.0,
        1.0, 10000.0
        );
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(
            camX ,camY,camZ,
            lookX+camX, lookY+ camY, lookZ+camZ,
            0.0, 1.0, 0.0
        );
        return;
    }
    

    if (isLeftEye) {
        glViewport(0, 0, viewW/2.0f, viewH);
    } else {
        glViewport(viewW/2.0f,0, viewW/2.0f, viewH);
    }
    if(isLeftEye){
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }



    // 投影変換
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float dd = 90.0f;      // モニターまでの垂直距離 (50cm)
    float nearPlane = 90.0f/2.0f;
    float farPlane = 1000.0f;

    // 2. ビュー行列 (LookAt) の計算
    // モニターの四隅の座標を定義
    // モニターの右方向ベクトルを計算
    Vec_3D vr = vectorNormalize(diffVec(pb, pa));
    // モニターの上方向ベクトルを計算
    Vec_3D vu = vectorNormalize(diffVec(pc, pa));
    // モニターの法線ベクトルを計算
    Vec_3D vn = vectorNormalize(crossProduct(vr, vu));

    //スクリーン中心
    Vec_3D pd = addVec(pb, diffVec(pc, pa));

    Vec_3D center;

    center.x = (pa.x + pb.x + pc.x + pd.x) * 0.25f;
    center.y = (pa.y + pb.y + pc.y + pd.y) * 0.25f;
    center.z = (pa.z + pb.z + pc.z + pd.z) * 0.25f;
    // スクリーンの左下、右下、左上の座標を定義
    Vec_3D va = diffVec(pa, pe);//スクリーンの左下から視点へのベクトル
    Vec_3D vb = diffVec(pb, pe);//スクリーンの右下から視点へのベクトル
    Vec_3D vc = diffVec(pc, pe);//スクリーンの左上から視点へのベクトル

    //
    double d = -innerProduct(va, vn);
    //ベクトルの内積を使って、スクリーンの左、右、下、上の座標を計算
    double left =
        innerProduct(vr, va) * nearPlane / d;//

    double right =
        innerProduct(vr, vb) * nearPlane / d;

    double bottom =
        innerProduct(vu, va) * nearPlane / d;

    double top =
        innerProduct(vu, vc) * nearPlane / d;

    glFrustum(
    -top,    // left
    -bottom, // right
    left, right,
    nearPlane, farPlane
    );

    // ビューイング変換準備
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    double LookY = 0;
    double LookZ = 0;
    double LookYp  = -90;
    double LookZp = -65.36;
    double angle = 45.0f;

    center = diffVec(pe, vn);
    if (isLeftEye) {
        Vec_3D eye = pe;
        gluLookAt(
            eye.x+eyeOffset/2,eye.y,eye.z,
            center.x+eyeOffset, center.y, center.z,
            1,0,0
        );
    } else {
        Vec_3D eye = pe;
        gluLookAt(
            eye.x-eyeOffset/2,eye.y,eye.z,
            center.x-eyeOffset, center.y, center.z,
            1,0,0
        );
    }
}

void dispobj(){
    //光源配置
    GLfloat lightPos0[] = {500.0, 2000.0, 2500.0, 1.0};  //光源座標(点光源)
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);  //光源配置
    GLfloat lightPos1[] = {-500.0, 2000.0, -500.0, 1.0};  //光源座標(点光源)
    glLightfv(GL_LIGHT1, GL_POSITION, lightPos1);  //光源配置
    GLfloat lightPos3[] = {0, 2000.0, -600, 1.0};  //光源座標(点光源)
    glLightfv(GL_LIGHT1, GL_POSITION, lightPos1);  //光源配置

    //----------床パネル----------
    setColor(0.2, 1.0, 0.2, 1.0);
    

    //ボクセル
    glPushMatrix();

    // 中心に寄せる
    float objectSize = VOXEL_SIZE * SPACING;
    setColor(0.5, 0.0, 0.5, 1.0);
    glTranslatef(-objectSize / 2.0f,
                0,
                -objectSize / 2.0f);
    glPopMatrix();

    //ボクセル
    glPushMatrix();

    // 中心に寄せる
    setColor(0.5, 0.0, 0.5, 1.0);
    glTranslatef(0,
                0,
                0);
    penguin_animation();
    model.Draw();
    glPopMatrix();
    //3Dモデル
    glPushMatrix();
    glTranslated(0,0,-3000);
    glRotated(180, 0.0, 1.0, 0.0);  //こっちに向く
    glScaled(150.0,150.0,150.0);

    setColor(1.0, 1.0, 1.0, 1.0);
    
    glPopMatrix();
    //目標物体
    double LookY = 0;
    double LookZ = 21;
    glPushMatrix();

    glTranslated(0,5-LookY,-LookZ);
    glRotated(180, 0.0, 1.0, 0.0);  //こっちに向く
    glScaled(10,10,10);
    setColor(0.0, 1.0, 0.0, 1.0);

    glutSolidCube(1);
    glPopMatrix();
    double wallDis = 10.92;

    
    double FloorSize = 47*2;
    double FloorChexSize = 25*2;
    if(cubeDispenser.GetPlacedCubes().size()>0){
        // モデル描画
        for(auto cube : cubeDispenser.GetPlacedCubes()){
            glPushMatrix();
            Color curC = cubeDispenser.GetCurrCubeColor();
            setColor(cube.color.r,cube.color.g,cube.color.b,1.0);
            glTranslated(cube.gx * GRID_SIZE,cube.gy * GRID_SIZE+GRID_SIZE/2,cube.gz * GRID_SIZE);
            glutSolidCube(GRID_SIZE);
            glPopMatrix();
        }
    }
    setColor(1.0,0.0, 0.0, 1.0);
    glPushMatrix();
        glTranslated(pointingCell.gx*GRID_SIZE,pointingCell.gy*GRID_SIZE+GRID_SIZE/2,pointingCell.gz*GRID_SIZE);
        glScaled(1.0,1.0,1.0);
    glPopMatrix();
}

void DrawWarpedTextures()
{
    int viewW = static_cast<int>(winW * rDisp / 2.0);
    int viewH = static_cast<int>(winH * rDisp);
    double aspect = static_cast<double>(viewW) / static_cast<double>(viewH);

      viewW = static_cast<int>(winW * rDisp);
        viewH = static_cast<int>(winH * rDisp);
        aspect = static_cast<double>(viewW) / static_cast<double>(viewH);
        glViewport(0, 0, viewW,viewH);
    glClear(
        GL_COLOR_BUFFER_BIT |
        GL_DEPTH_BUFFER_BIT
    );

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(
        -1,1,
        -1,1,
        -1,1
    );

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);


    double widthfix = 0.2f;//台形補正
    double Xfix = -0.1f;
    double Lfix = 0;

    glBindTexture(GL_TEXTURE_2D, leftTex);
//左眼用の描画
    glBegin(GL_QUADS);

    glTexCoord2f(0,0);
    glVertex2f(-1.0f,-1.0f);

    glTexCoord2f(1,0);
    glVertex2f(0.0,-1.0f+widthfix+Xfix);

    glTexCoord2f(1,1);
    glVertex2f(0.0,1.0f-widthfix+Xfix+Lfix);
    glTexCoord2f(0,1);
    glVertex2f(-1.0f,1.0f);

    glEnd();

    glBindTexture(GL_TEXTURE_2D, rightTex);
//右目用の描画
    glBegin(GL_QUADS);

    glTexCoord2f(0,0);
    glVertex2f(0.0f,-1.0f);

    glTexCoord2f(1,0);
  
    glVertex2f(1.0f,-1.0f+widthfix+Xfix);

    glTexCoord2f(1,1);
    glVertex2f(1.0f,1.0f-widthfix+Xfix+Lfix);
    glTexCoord2f(0,1);
    glVertex2f(0.0f,1.0f);

    glEnd();

    glDisable(GL_TEXTURE_2D);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

//リサイズコールバック関数
void reshape(int w, int h)
{
     glViewport(0, 0, w, h);

    // 左テクスチャ
    glBindTexture(GL_TEXTURE_2D, leftTex);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        w / 2,
        h,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    // 右テクスチャ
    glBindTexture(GL_TEXTURE_2D, rightTex);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        w / 2,
        h,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );
    
    winW = w; winH = h;  //ウィンドウサイズをグローバル変数に格納

    
}

//タイマーコールバック関数
void timer(int value)
{
    glutPostRedisplay();  //ディスプレイイベント強制発生
    glutTimerFunc(1000/f, timer, 0);  //タイマー再設定
}

void mouseMove(int x,int y){
    Ray ray =screen2ray(x,y);
    Vec_3D hit;
    if(intersectFloor(ray,&hit)){
        // グリッドの中心にスナップするための調整
        if(hit.x < 0){
            hit.x -= static_cast<int>(GRID_SIZE/2);
        } else {
            hit.x += static_cast<int>(GRID_SIZE/2);
        }
        if(hit.z < 0){
            hit.z -= static_cast<int>(GRID_SIZE/2);
        } else {
            hit.z += static_cast<int>(GRID_SIZE/2);
        }
        if(hit.y < 0){
            hit.y -= static_cast<int>(GRID_SIZE/2);
        } else {
            hit.y += static_cast<int>(GRID_SIZE/2);
        }
        pointingCell.gx = static_cast<int>(hit.x/GRID_SIZE);
        pointingCell.gy = static_cast<int>(hit.y / GRID_SIZE);
        pointingCell.gz = static_cast<int>(hit.z / GRID_SIZE);
    }
    for(auto cube :cubeDispenser.GetPlacedCubes()){
        double t;
        Vec_3D hit;
        Vec_3D normal;
        Ray ray = screen2ray(x,y);
        if(intersectCubeCell(ray,cube,GRID_SIZE,&t,&hit,&normal)){
        
            pointingCell = cube;
            pointingCell.gx += static_cast<int>(normal.x);
            pointingCell.gy += static_cast<int>(normal.y);
            pointingCell.gz += static_cast<int>(normal.z);
        }
    }
}

//マウスクリックコールバック関数
void mouse(int button, int state, int x, int y)
{
    //マウス情報をグローバル変数に格納
    mButton = button; mState = state; mX = x; mY = y;
    Vec_3D forward = makeVec(lookX+camX, lookY+ camY, lookZ+camZ);
    Vec_3D point = makeVec(camX ,camY, camZ);
    Vec_3D cameraVec = diffVec(forward,point);
    Vec_3D cameraVecNorm = vectorNormalize(cameraVec);
    // //大きさ
    if (mButton==GLUT_LEFT_BUTTON && mState==GLUT_DOWN) { //マウスボタンを押したとき
        pointingCell.color = cubeDispenser.GetCurrCubeColor();
        cubeDispenser.AddCube(pointingCell);
	}
    
}

//マウスドラッグコールバック関数
void motion(int x, int y)
{
    Vec_3D forward = makeVec(lookX+camX, lookY+ camY, lookZ+camZ);
    Vec_3D point = makeVec(camX ,camY, camZ);
    Vec_3D cameraVec = diffVec(forward,point);
    Vec_3D cameraVecNorm = vectorNormalize(cameraVec);
    //中ドラッグで視点を移動
   
    if(mButton==GLUT_MIDDLE_BUTTON){
         if(isZooming){
            camX +=  (mY-y)*20*cameraVecNorm.x;  //マウス横方向→水平角
            camY +=  (mY-y)*20*cameraVecNorm.y;  //マウス縦方向→垂直角
            camZ +=  (mY-y)*20*cameraVecNorm.z;  //マウス縦方向→垂直角
        }else{
            cameraVec = vectorNormalize(cameraVec);
        //カメラの右方向ベクトルを計算
        Vec_3D right = crossProduct(cameraVec, makeVec(0,1,0));
        right = vectorNormalize(right);
        camX += (mX-x)*20*right.x;  //マウス横方向→水平角
        camY -= (mY-y)*20;  //マウス縦方向→垂直角
        camZ += (mX-x)*20*right.z;  //マウス縦方向→垂直角
        }
    }
    if (mButton == GLUT_RIGHT_BUTTON) {
            // //右ドラッグでカメラを回転
        cameraToTargetDegX += (mY-y)*0.1;
        cameraToTargetDegY += (mX-x)*0.1;
        lookX = cameraLength * cos(cameraToTargetDegY * M_PI / 180.0);
        lookZ = cameraLength * sin(cameraToTargetDegY * M_PI / 180.0);
        lookY += (y - mY) * 10.0;
    }
    
    if (mButton == GLUT_LEFT_BUTTON) {
        // 左ドラッグでカメラをズーム
       
    }
    //マウス座標をグローバル変数に保存
    mX = x; mY = y;
}

void mouseWheel(int wheel,int direction,int x,int y){
    Vec_3D forward = makeVec(lookX+camX, lookY+ camY, lookZ+camZ);
    Vec_3D point = makeVec(camX ,camY, camZ);
    Vec_3D cameraVec = diffVec(forward,point);
    Vec_3D cameraVecNorm = vectorNormalize(cameraVec);
    //マウスホイールでカメラをズーム
    camX +=  direction*20*cameraVecNorm.x;  //マウス横方向→水平角
    camY +=  direction*20*cameraVecNorm.y;  //マウス縦方向→垂直角
    camZ +=  direction*20*cameraVecNorm.z;  //マウス縦方向→垂直角
}

//キーボードコールバック関数
void keyboard(unsigned char key, int x, int y)
{
    Color col;
    switch (key) {
        case 27:  //[ESC]キー
            exit(0);  //プロセス終了
            break;
        case 'r':
            col = {1.0,0.0,0.0};
            cubeDispenser.ChangeCurrColor(col);
            break;
        case 'f':
            isFullScreen = !isFullScreen;
            if (isFullScreen) {
               glutFullScreen();
            } else {
               glutReshapeWindow(winW, winH);
            }
            break;
        case 'g':
            col = {0.0,1.0,0.0};
            cubeDispenser.ChangeCurrColor(col);
            break;
        case 'b':
            col = {0.0,0.0,1.0};
            cubeDispenser.ChangeCurrColor(col);
            break;
        case 'm':
            isZooming = !isZooming;
            break;
        case 'w':
            eyeOffset += 0.01;
            break;
        case 's':
            if(NormalView == false){
                eyeOffset -= 0.01;
            }else{
                cubeDispenser.SaveToFile("placed_cubes.txt");
            }
            break;
        case 'l':
            cubeDispenser.LoadFromFile("placed_cubes.txt");
            break;
        case 'a':
            NormalView = !NormalView;
            break;
        case 'y':

            testB += 0.01;
            std::cout << "testB: " << testB << std::endl;
            break;
        case 'h':
            testB -= 0.01;
            std::cout << "testB: " << testB << std::endl;
             break;
        case 'u':
            testD += 0.1;
            break;
        case 'j':
            testD -= 0.1;
             break;
        case 'Q':  //[Q]キー
            exit(0);  //プロセス終了
            break;
        case '+':
            eDist -= 100.0;
            break;
        case '-':
            eDist += 100.0;
            break;
            break;
        default:
            break;
    }
}

void setrundum(int i,int j){
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            array[i][j] = rand() % 360; // 0〜99の乱数
            
        }
    }

    for (int i = 0; i < OSTS; i++) {
        ost_degs[i]= rand() % 360;
    }
}