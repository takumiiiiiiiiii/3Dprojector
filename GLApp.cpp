#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#ifdef __APPLE__
#include <OpenGL/glext.h>
#else
#include <GL/glext.h>
#endif

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
#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <vector>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glut.h"
#include "imgui/backends/imgui_impl_opengl2.h"


//初期設定関数
namespace {
    GLuint g_gridShaderProgram = 0;
    GLint g_gridCountLocation = -1;
    GLint g_lineWidthLocation = -1;
    float g_gridCount = 10.0f;
    float g_lineWidthPx = 1.5f;
    bool g_useProceduralGrid = true;

    GLuint CompileGridShader(GLenum shaderType, const char* source)
    {
        GLuint shader = glCreateShader(shaderType);
        if (shader == 0)
        {
            std::cerr << "格子シェーダーを作成できませんでした" << std::endl;
            return 0;
        }

        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint compiled = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (compiled == GL_TRUE)
        {
            return shader;
        }

        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<GLchar> log(static_cast<size_t>(logLength > 1 ? logLength : 1));
        GLsizei written = 0;
        glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), &written, log.data());
        std::cerr << "格子シェーダーのコンパイルに失敗しました: "
                  << log.data() << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    GLuint CreateGridShaderProgram()
    {
        static const char* vertexSource =
            "#version 120\n"
            "void main()\n"
            "{\n"
            "    gl_Position = ftransform();\n"
            "    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;\n"
            "}\n";
        static const char* fragmentSource =
            "#version 120\n"
            "uniform float uGridCount;\n"
            "uniform float uLineWidthPx;\n"
            "void main()\n"
            "{\n"
            "    vec2 uv = gl_TexCoord[0].st / gl_TexCoord[0].q;\n"
            "    vec2 grid = uv * uGridCount;\n"
            "    vec2 dist = abs(fract(grid + 0.5) - 0.5);\n"
            "    vec2 fw = max(fwidth(grid), vec2(0.00001));\n"
            "    vec2 halfWidth = 0.5 * uLineWidthPx * fw;\n"
            "    vec2 aa = 0.5 * fw;\n"
            "    float lineX = 1.0 - smoothstep(max(halfWidth.x - aa.x, 0.0), "
            "halfWidth.x + aa.x, dist.x);\n"
            "    float lineY = 1.0 - smoothstep(max(halfWidth.y - aa.y, 0.0), "
            "halfWidth.y + aa.y, dist.y);\n"
            "    float gridLine = max(lineX, lineY);\n"
            "    vec4 bgColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
            "    vec4 lineColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
            "    gl_FragColor = mix(bgColor, lineColor, gridLine);\n"
            "}\n";

        GLuint vertexShader = CompileGridShader(GL_VERTEX_SHADER, vertexSource);
        if (vertexShader == 0)
        {
            return 0;
        }

        GLuint fragmentShader = CompileGridShader(GL_FRAGMENT_SHADER, fragmentSource);
        if (fragmentShader == 0)
        {
            glDeleteShader(vertexShader);
            return 0;
        }

        GLuint program = glCreateProgram();
        if (program == 0)
        {
            std::cerr << "格子シェーダープログラムを作成できませんでした" << std::endl;
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            return 0;
        }

        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        GLint linked = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE)
        {
            GLint logLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<GLchar> log(static_cast<size_t>(logLength > 1 ? logLength : 1));
            GLsizei written = 0;
            glGetProgramInfoLog(program, static_cast<GLsizei>(log.size()), &written, log.data());
            std::cerr << "格子シェーダープログラムのリンクに失敗しました: "
                      << log.data() << std::endl;
            glDeleteProgram(program);
            return 0;
        }

        g_gridCountLocation = glGetUniformLocation(program, "uGridCount");
        g_lineWidthLocation = glGetUniformLocation(program, "uLineWidthPx");
        if (g_gridCountLocation < 0 || g_lineWidthLocation < 0)
        {
            std::cerr << "格子シェーダーのuniformを取得できませんでした" << std::endl;
            glDeleteProgram(program);
            return 0;
        }
        return program;
    }

    struct EyeTcpData {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        bool valid = true;   // まだ一度もデータを受信していない場合は false
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

        //カットオフ周波数とサンプリング間隔 dt から、指数移動平均の係数 α (0〜1) を求める
        //cutoffが小さいほど1に近づく
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
            double dx = (x - x_prev) / dt;//過去と現在の差から速度を求める
            double a_d = alpha(dcutoff, dt);//
            //差が大きい場合は現在の値を優先
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
    int g_renderWindowId = 0;
    int g_imguiWindowId = 0;
 
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
    g_renderWindowId = glutCreateWindow("CG Final");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGLUT_Init();
    ImGui_ImplOpenGL2_Init();
    //Model
    //model.Load("/Users/x23029xx/Documents/GitHub/3Dprojector/Model/EyeBall/EyeBall.fbx");
    // model.Load("/Users/x23029xx/Documents/GitHub/3Dprojector/Model/Dog/Pug Dog Low Poly.fbx");
    model.Load("/Users/x23029xx/Documents/GitHub/3Dprojector/Model/Tree/tree.fbx");
// フルスクリーン
    std::cout << "windowId: " << g_renderWindowId << std::endl;

    const GLubyte* version = glGetString(GL_VERSION);

    if (version == nullptr) {
        std::cout << "GL_VERSION is NULL" << std::endl;
    } else {
        std::cout << "GL_VERSION: " << reinterpret_cast<const char*>(version) << std::endl;
    }
    g_gridShaderProgram = CreateGridShaderProgram();
    if (g_gridShaderProgram == 0) {
        std::cerr << "格子描画シェーダーを初期化できませんでした" << std::endl;
        exit(1);
    }
    g_tcpThreadRunning = true;
    g_tcpThread = std::thread(TcpReceiveLoop);

    
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
    GLfloat amb[4],col[4];  //光源設定用配列
    //ボクセルの初期化
    InitVoxcels();
    //光源0
    glEnable(GL_LIGHT0);  //光源0有効化
    col[0] = 0.8; col[1] = 0.8; col[2] = 0.8; col[3] = 1.0;
    glLightfv(GL_LIGHT0, GL_DIFFUSE, col);  //拡散反射対象
    glLightfv(GL_LIGHT0, GL_SPECULAR, col);  //鏡面反射対象
    amb[0] = 0.2; amb[1] = 0.2; amb[2] = 0.2; amb[3] = 1.0;
    glLightfv(GL_LIGHT0, GL_AMBIENT, amb);  //環境光対象
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0000001);  //減衰率
    //視点極座標
    eDist = 5000.0;  //距離
    eDegX =0; eDegY = 180.0;  //x軸周り角度，y軸周り角度

    //床頂点座標
    for (int j=0; j<TILE; j++) {
        for (int i=0; i<TILE; i++) {
            fPoint[i][j].x = -fWidth/2.0+i*fWidth/(TILE-1);
            fPoint[i][j].y = 0.0;
            fPoint[i][j].z = -fWidth/2.0+j*fWidth/(TILE-1);
        }
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
    eyeFilterL.setParams(/*mincutoff=*/2.0, /*beta=*/0.03);
    eyeFilterR.setParams(/*mincutoff=*/2.0, /*beta=*/0.03);
    //カメラ座標設定
    camX = playerEye.x;
    camY = playerEye.y/2;
    camZ = playerEye.z;
    cameraToTargetDegY = -90;
    lookX = cameraLength * cos(cameraToTargetDegY * M_PI / 180.0);
    lookZ = cameraLength * sin(cameraToTargetDegY * M_PI / 180.0);

    glutInitWindowSize(700, 400);
    glutInitWindowPosition(100, 100);
    g_imguiWindowId = glutCreateWindow("ImGui Controls");
    glutDisplayFunc(showImgui);
    ImGui_ImplGLUT_InstallFuncs();
    glutSetWindow(g_renderWindowId);
}

static bool BuildProjectionWarpMatrix(bool isLeftEye, GLdouble matrix[16])
{
    struct WarpPoint
    {
        double x;
        double y;
    };

    const double xOffset = isLeftEye ? -1.0 : 0.0;
    const auto scaleX = [xOffset](WarpPoint point)
    {
        point.x = (xOffset + 1.0) - ((xOffset + 1.0) - point.x) * vScaleX;
        return point;
    };
    const auto toEyeNdc = [xOffset](WarpPoint point)
    {
        point.x = (point.x - xOffset) * 2.0 - 1.0;
        return point;
    };

    const WarpPoint corners[4] = {
        toEyeNdc(scaleX({xOffset + TopYfix, -1.0 + widthfix + TopXRfix})),
        toEyeNdc(scaleX({xOffset + 1.0, -1.0 + widthfix})),
        toEyeNdc(scaleX({xOffset + 1.0, 1.0 - widthfix})),
        toEyeNdc(scaleX({xOffset + TopYfix, 1.0 - widthfix + TopXLfix}))
    };
    const cv::Point2f source[4] = {
        {-1.0f, -1.0f}, {1.0f, -1.0f},
        {1.0f, 1.0f}, {-1.0f, 1.0f}
    };
    cv::Point2f destination[4];
    for (int i = 0; i < 4; ++i)
    {
        destination[i] = cv::Point2f(
            static_cast<float>(corners[i].x),
            static_cast<float>(corners[i].y));
    }

    const cv::Mat homography = cv::getPerspectiveTransform(source, destination);
    if (!cv::checkRange(homography) ||
        std::fabs(cv::determinant(homography)) < 1e-12)
    {
        static bool warningShown = false;
        if (!warningShown)
        {
            std::cerr << "射影補正の四隅が無効なため、補正を適用できません" << std::endl;
            warningShown = true;
        }
        return false;
    }

    const double h00 = homography.at<double>(0, 0);
    const double h01 = homography.at<double>(0, 1);
    const double h02 = homography.at<double>(0, 2);
    const double h10 = homography.at<double>(1, 0);
    const double h11 = homography.at<double>(1, 1);
    const double h12 = homography.at<double>(1, 2);
    const double h20 = homography.at<double>(2, 0);
    const double h21 = homography.at<double>(2, 1);
    const double h22 = homography.at<double>(2, 2);
    const GLdouble warpMatrix[16] = {
        h00, h10, 0.0, h20,
        h01, h11, 0.0, h21,
        0.0, 0.0, 1.0, 0.0,
        h02, h12, 0.0, h22
    };
    std::copy(warpMatrix, warpMatrix + 16, matrix);
    return true;
}

static void DrawProjectionGrid(bool isLeftEye)
{
    const int viewW = static_cast<int>(winW * rDisp);
    const int viewH = static_cast<int>(winH * rDisp);
    const int eyeWidth = viewW / 2;
    const int eyeX = isLeftEye ? 0 : eyeWidth;

    if (isLeftEye)
    {
        glViewport(0, 0, viewW, viewH);
        glClearColor(windowClearColor.x, windowClearColor.y, windowClearColor.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    glViewport(eyeX, 0, eyeWidth, viewH);

    GLdouble warpMatrix[16];
    glMatrixMode(GL_PROJECTION);
    if (BuildProjectionWarpMatrix(isLeftEye, warpMatrix))
    {
        glLoadMatrixd(warpMatrix);
    }
    else
    {
        glLoadIdentity();
    }
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glUseProgram(g_gridShaderProgram);
    glUniform1f(g_gridCountLocation, g_gridCount);
    glUniform1f(g_lineWidthLocation, g_lineWidthPx);
    glBegin(GL_QUADS);
        glTexCoord2d(0.0, 0.0); glVertex2d(-1.0, -1.0);
        glTexCoord2d(1.0, 0.0); glVertex2d(1.0, -1.0);
        glTexCoord2d(1.0, 1.0); glVertex2d(1.0, 1.0);
        glTexCoord2d(0.0, 1.0); glVertex2d(-1.0, 1.0);
    glEnd();
    glUseProgram(0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_BLEND);
}

//ディスプレイコールバック関数
void display()
{
    UpdateEyePositionFromShared();
    if (!NormalView && g_useProceduralGrid)
    {
        DrawProjectionGrid(true);
        DrawProjectionGrid(false);
    }
    else
    {
        initView(true);
        dispobj();
        initView(false);
        dispobj();
    }
    glutSwapBuffers();
}
void initView(bool isLeftEye) {
    //ウィンドウクリア色の設定
    glClearColor(windowClearColor.x,windowClearColor.y,windowClearColor.z, 1.0);
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
    playerEye.x = -eyeX;
    playerEye.y = eyeY;
    playerEye.z = eyeZ;
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
    float nearPlane = 90.0f/10.0f;
    float farPlane = 1000.0f;

    // 2. ビュー行列 (LookAt) の計算
    // モニターの四隅の座標を定義
    // モニターの右方向ベクトル
    Vec_3D vr = vectorNormalize(diffVec(pb, pa));
    // モニターの上方向ベクトル
    Vec_3D vu = vectorNormalize(diffVec(pc, pa));
    // モニターの法線ベクトル
    Vec_3D vn = vectorNormalize(crossProduct(vr, vu));
    //スクリーン中心座標
    Vec_3D pd = addVec(pb, diffVec(pc, pa));

    Vec_3D center;

    center.x = (pa.x + pb.x + pc.x + pd.x) * 0.25f;
    center.y = (pa.y + pb.y + pc.y + pd.y) * 0.25f;
    center.z = (pa.z + pb.z + pc.z + pd.z) * 0.25f;
    // スクリーンの左下、右下、左上の座標を定義
    Vec_3D va = diffVec(pa, playerEye);//スクリーンの左下から視点へのベクトル
    Vec_3D vb = diffVec(pb, playerEye);//スクリーンの右下から視点へのベクトル
    Vec_3D vc = diffVec(pc, playerEye);//スクリーンの左上から視点へのベクトル

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

    if (!NormalView)
    {
        GLdouble warpMatrix[16];
        if (BuildProjectionWarpMatrix(isLeftEye, warpMatrix))
        {
            GLdouble baseProjection[16];
            glGetDoublev(GL_PROJECTION_MATRIX, baseProjection);
            glLoadMatrixd(warpMatrix);
            glMultMatrixd(baseProjection);
        }
    }

    // ビューイング変換準備
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    double LookY = 0;
    double LookZ = 0;
    double LookYp  = -90;
    double LookZp = -65.36;
    double angle = 45.0f;

    center = diffVec(playerEye, vn);
    if (isLeftEye) {
        gluLookAt(
            playerEye.x+eyeOffset/2,playerEye.y,playerEye.z,
            center.x+eyeOffset,center.y,center.z,
            1,0,0
        );
    } else {
        gluLookAt(
            playerEye.x-eyeOffset/2,playerEye.y,playerEye.z,
            center.x-eyeOffset,center.y,center.z,
            1,0,0
        );
    }
}


void dispobj(){
    //光源配置
    GLfloat lightPos0[] = {30, 200,100, 1.0};  //光源座標(点光源)
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);  //光源配置
    // GLfloat lightPos1[] = {-500.0, 2000.0, -500.0, 1.0};  //光源座標(点光源)
    // glLightfv(GL_LIGHT1, GL_POSITION, lightPos1);  //光源配置
    // GLfloat lightPos3[] = {0, 2000.0, -600, 1.0};  //光源座標(点光源)
    // glLightfv(GL_LIGHT1, GL_POSITION, lightPos1);  //光源配置

    //ボックス座標
    //目標物体
    double LookY = 0;
    double LookZ = 22;
    double boxsize = 10;
    double boxhalf = boxsize/2;
    Vec_3D boxPos = {0,boxsize-LookY,-LookZ};
    //床の座標
    float floorY = -0.1;
    float groundPlane[4] = { 0.0f, 1.0f, 0.0f, -floorY }; // floorYはdraw_floorに渡すy
    float shadowMat[4][4];

    //シャドウマップ
    makeShadowMatrix(shadowMat, groundPlane, lightPos0);
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
    glPopMatrix();
    //3Dモデル
    glPushMatrix();
    glTranslated(0,0,-3000);
    glRotated(180, 0.0, 1.0, 0.0);  //こっちに向く
    glScaled(150.0,150.0,150.0);

    setColor(1.0, 1.0, 1.0, 1.0);
    
    glPopMatrix();
    //床
    // glPushMatrix();
    //     glTranslated(0,floorY,-LookZ);
    //     glRotated(180, 0.0, 1.0, 0.0);  //こっちに向く
    //     glScaled(10,10,10);
    //     setColor(floorColor.x, floorColor.y, floorColor.z, 1.0);
    //     draw_floor(100,100,0,0,0);
    // glPopMatrix();
    //影を描画(床の上に潰したオブジェクトを暗い色で)
    //glColorの状態漏れを防ぐ
    glPushAttrib(GL_CURRENT_BIT);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.0f, 0.0f, 0.0f, 0.5f); // 半透明の黒

        glDepthMask(GL_FALSE); // 床とのZファイティング防止(深度書き込みだけ止める)

        glPushMatrix();
            glMultMatrixf((GLfloat*)shadowMat);
            glTranslated(mainOjbectPos.x,mainOjbectPos.y,mainOjbectPos.z);
            glRotated(mainObjectRotation.x, 1.0, 0.0, 0.0);
            glRotated(mainObjectRotation.y, 0.0, 1.0, 0.0);
            glRotated(mainObjectRotation.z, 0.0, 0.0, 1.0);
            glScaled(mainOjbectSize.x,mainOjbectSize.y,mainOjbectSize.z);
            if(mainObjectType==0){
                glutSolidCube(1);
            }
            if(mainObjectType==1){
                glutSolidSphere(0.5, 36, 18);  //球
            }
            if(mainObjectType==2){
                glPushMatrix();
                    glRotated(90, 1.0, 0.0, 0.0);  //こっちに向く
                    glMyCylinder(0.5,0.5,1,36);
                glPopMatrix();
            }
            // if(mainObjectType==3){
            //     model.Draw();
            // }
        glPopMatrix();

        glDepthMask(GL_TRUE);
        glEnable(GL_LIGHTING);
        glEnable(GL_DEPTH_TEST);
    glPopAttrib();
    //目標物体
    glPushMatrix();
        glTranslated(mainOjbectPos.x,mainOjbectPos.y,mainOjbectPos.z);
        glRotated(180, 0.0, 1.0, 0.0);  //こっちに向く
        glRotated(mainObjectRotation.x, 1.0, 0.0, 0.0);
        glRotated(mainObjectRotation.y, 0.0, 1.0, 0.0);
        glRotated(mainObjectRotation.z, 0.0, 0.0, 1.0);
        glScaled(mainOjbectSize.x,mainOjbectSize.y,mainOjbectSize.z);
        setColor(0.0, 1.0, 0.0, 1.0);
        if(mainObjectType==0){
            glutSolidCube(1);
        }
        if(mainObjectType==1){
            glutSolidSphere(0.5, 36, 18);  //球
        }
        if(mainObjectType==2){
            glPushMatrix();
                glRotated(90, 1.0, 0.0, 0.0);  //こっちに向く
                glMyCylinder(0.5,0.5,1,36);
            glPopMatrix();
        }
        if(mainObjectType==3){
            model.Draw();
        }
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

    glColor4f(1,1,1,1);
    glDisable(GL_BLEND);   // 次項
    if (g_useProceduralGrid)
    {
        glDisable(GL_TEXTURE_2D);
        glUseProgram(g_gridShaderProgram);
        glUniform1f(g_gridCountLocation, g_gridCount);
        glUniform1f(g_lineWidthLocation, g_lineWidthPx);
    }
    else
    {
        glUseProgram(0);
        glEnable(GL_TEXTURE_2D);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    }

    // double widthfix = 0.2f;//台形補正
    // double TopXLfix = -0.8f;//右上の台形補正
    // double TopXRfix = -0.2f;//右下の台形補正
        // double Xfix = -0.1f;
    // 四角形内の座標を表す
    struct WarpPoint
    {
        double x;
        double y;
    };

    // 4つの頂点から、四角形内の座標を計算する
    auto BilinearPoint = [](
        double u, double v,
        const WarpPoint& bl,
        const WarpPoint& br,
        const WarpPoint& tr,
        const WarpPoint& tl) -> WarpPoint
    {
        double wBL = (1.0 - u) * (1.0 - v);
        double wBR = u * (1.0 - v);
        double wTR = u * v;
        double wTL = (1.0 - u) * v;

        return {
            wBL * bl.x + wBR * br.x
                + wTR * tr.x + wTL * tl.x,

            wBL * bl.y + wBR * br.y
                + wTR * tr.y + wTL * tl.y
        };
    };


    // テクスチャのX座標を補正
    auto CorrectU = [&](double u) -> double
    {
        return u + verticalFix * (u * u - u);
    };

    const auto drawEye = [&](GLuint texture, float xOffset, double upperRightOffset)
    {
        // x方向(紙の縦)の拡大率。ImGuiのスライダーで調整(初期値1.0)
        // 固定したい辺(右端 xOffset+1.0)を基準に縮める
        auto ScaleX = [&](WarpPoint p) {
            p.x = (xOffset + 1.0) - ((xOffset + 1.0) - p.x) * vScaleX;
            return p;
        };
        WarpPoint bl = ScaleX({ xOffset + TopYfix, -1.0 + widthfix + TopXRfix });
        WarpPoint br = ScaleX({ xOffset + 1.0,     -1.0 + widthfix });
        WarpPoint tr = ScaleX({ xOffset + 1.0,      1.0 - widthfix + upperRightOffset });
        WarpPoint tl = ScaleX({ xOffset + TopYfix,  1.0 - widthfix + TopXLfix });
        // 画面上の4隅 → テクスチャ座標 のホモグラフィ
        cv::Point2f src[4] = { {(float)bl.x,(float)bl.y}, {(float)br.x,(float)br.y},
                            {(float)tr.x,(float)tr.y}, {(float)tl.x,(float)tl.y} };
        cv::Point2f dst[4] = { {0,0}, {1,0}, {1,1}, {0,1} };
        cv::Mat H = cv::getPerspectiveTransform(src, dst);   // 3x3, CV_64F

        // [s t r q]^T = M * [x y z 1]^T となるように列優先で詰める
        double m[16] = {
            H.at<double>(0,0), H.at<double>(1,0), 0, H.at<double>(2,0),
            H.at<double>(0,1), H.at<double>(1,1), 0, H.at<double>(2,1),
            0,                 0,                 1, 0,
            H.at<double>(0,2), H.at<double>(1,2), 0, H.at<double>(2,2)
        };

        if (!g_useProceduralGrid)
        {
            glBindTexture(GL_TEXTURE_2D, texture);
        }
        glMatrixMode(GL_TEXTURE);
        glLoadMatrixd(m);
        glMatrixMode(GL_MODELVIEW);

        // テクスチャ座標には「頂点位置そのもの」を渡す(変換は行列側で行う)
        glBegin(GL_QUADS);
            glTexCoord2d(bl.x, bl.y); glVertex2d(bl.x, bl.y);
            glTexCoord2d(br.x, br.y); glVertex2d(br.x, br.y);
            glTexCoord2d(tr.x, tr.y); glVertex2d(tr.x, tr.y);
            glTexCoord2d(tl.x, tl.y); glVertex2d(tl.x, tl.y);
        glEnd();

        // 元に戻す(忘れるとほかのテクスチャ描画が壊れます)
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
    };
    
    if (g_useProceduralGrid)
    {
        drawEye(0, -1.0f, 0.0);
        drawEye(0, 0.0f, 0.0);
    }
    else
    {
        drawEye(leftTex, -1.0f, 0.0);
        drawEye(rightTex, 0.0f, 0.0);
        glDisable(GL_TEXTURE_2D);
    }

    glUseProgram(0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_BLEND);   // 次項
}

//リサイズコールバック関数
void reshape(int w, int h)
{
    if (w <= 0 || h <= 0) return;
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
    static auto previousTime = std::chrono::steady_clock::now();
    const auto now = std::chrono::steady_clock::now();
    const double deltaSeconds =
        std::chrono::duration<double>(now - previousTime).count();
    previousTime = now;

    if (mainObjectAutoRotate)
    {
        const double direction = mainObjectRotationReverse ? -1.0 : 1.0;
        const double angleDelta = mainObjectRotationSpeed * direction * deltaSeconds;
        const auto advanceAngle = [angleDelta](double& angle)
        {
            angle = std::fmod(angle + angleDelta + 180.0, 360.0);
            if (angle < 0.0)
            {
                angle += 360.0;
            }
            angle -= 180.0;
        };

        if (mainObjectRotateX) advanceAngle(mainObjectRotation.x);
        if (mainObjectRotateY) advanceAngle(mainObjectRotation.y);
        if (mainObjectRotateZ) advanceAngle(mainObjectRotation.z);
    }

    if (g_renderWindowId != 0) {
        glutSetWindow(g_renderWindowId);
        glutPostRedisplay();
    }
    if (g_imguiWindowId != 0) {
        glutSetWindow(g_imguiWindowId);
        glutPostRedisplay();
    }
    if (g_renderWindowId != 0) {
        glutSetWindow(g_renderWindowId);
    }
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
        // cubeDispenser.AddCube(pointingCell);
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
// 既存のキューブ配置処理
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

void showImgui(){
    auto SliderDouble = [](const char* label, double& value,
                       float minValue, float maxValue) -> bool
    {
        float temp = static_cast<float>(value);

        if (ImGui::SliderFloat(label, &temp, minValue, maxValue))
        {
            value = static_cast<double>(temp);
            return true;
        }

        return false;
    };
    auto ColorEditVec3 = [](const char* label, Vec_3D& color)
    {
        float values[3] = {
            static_cast<float>(color.x),
            static_cast<float>(color.y),
            static_cast<float>(color.z)
        };
        if (ImGui::ColorEdit3(label, values))
        {
            color.x = values[0];
            color.y = values[1];
            color.z = values[2];
        }
    };

    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplGLUT_NewFrame();
    ImGui_ImplOpenGL2_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    //メインオブジェクトのサイズ
    ImGui::Begin("ObjectSetting");
    ImGui::Text("Object");
    float mainObjMove = 20.0f;
    SliderDouble("position X", mainOjbectPos.x, -mainObjMove, mainObjMove);
    SliderDouble("position Y", mainOjbectPos.y, -20, 80);
    SliderDouble("position Z", mainOjbectPos.z, -80, 80);
    float mainObjSize = 20.0f;
    SliderDouble("sizeX", mainOjbectSize.x, -mainObjMove, mainObjSize);
    SliderDouble("sizeY", mainOjbectSize.y, -mainObjMove, mainObjSize);
    SliderDouble("sizeZ", mainOjbectSize.z, -mainObjMove, mainObjSize);
    SliderDouble("Rotation X", mainObjectRotation.x, -180.0f, 180.0f);
    SliderDouble("Rotation Y", mainObjectRotation.y, -180.0f, 180.0f);
    SliderDouble("Rotation Z", mainObjectRotation.z, -180.0f, 180.0f);
    ImGui::Separator();
    ImGui::Checkbox("Auto rotation", &mainObjectAutoRotate);
    ImGui::SameLine();
    ImGui::Checkbox("Reverse direction", &mainObjectRotationReverse);
    ImGui::Checkbox("Rotate X", &mainObjectRotateX);
    ImGui::SameLine();
    ImGui::Checkbox("Rotate Y", &mainObjectRotateY);
    ImGui::SameLine();
    ImGui::Checkbox("Rotate Z", &mainObjectRotateZ);
    SliderDouble("Rotation speed (deg/s)", mainObjectRotationSpeed, 0.0f, 360.0f);
    const char* objectTypes[] = {"Cube", "Sphere", "Cylinder", "Model"};
    ImGui::Combo("Object type", &mainObjectType, objectTypes, 4);
    ImGui::Checkbox("isNormalView", &NormalView);
    SliderDouble("EyeOffset",eyeOffset, 0.0f,1.0f);
    ImGui::End();
    ImGui::Begin("Model diagnostics");
    ImGui::Text("Model status: %s", model.IsLoaded() ? "loaded" : "not loaded");
    ImGui::Text("Model selected: %s", mainObjectType == 3 ? "yes" : "no");
    static bool flipModelTextureU = false;
    static bool flipModelTextureV = false;
    const bool flipUChanged = ImGui::Checkbox("Flip texture U", &flipModelTextureU);
    const bool flipVChanged = ImGui::Checkbox("Flip texture V", &flipModelTextureV);
    if (flipUChanged || flipVChanged)
    {
        model.SetTextureCoordinateFlip(flipModelTextureU, flipModelTextureV);
    }
    ImGui::Text("Position: (%.3f, %.3f, %.3f)",
                mainOjbectPos.x, mainOjbectPos.y, mainOjbectPos.z);
    ImGui::Text("Scale: (%.3f, %.3f, %.3f)",
                mainOjbectSize.x, mainOjbectSize.y, mainOjbectSize.z);
    ModelBounds bounds;
    if (model.GetLocalBounds(bounds))
    {
        const double sizeX = bounds.maxX - bounds.minX;
        const double sizeY = bounds.maxY - bounds.minY;
        const double sizeZ = bounds.maxZ - bounds.minZ;
        ImGui::Text("Local min: (%.3f, %.3f, %.3f)",
                    bounds.minX, bounds.minY, bounds.minZ);
        ImGui::Text("Local max: (%.3f, %.3f, %.3f)",
                    bounds.maxX, bounds.maxY, bounds.maxZ);
        ImGui::Text("Local size: (%.3f, %.3f, %.3f)", sizeX, sizeY, sizeZ);
        ImGui::Text("Scaled size: (%.3f, %.3f, %.3f)",
                    sizeX * std::fabs(mainOjbectSize.x),
                    sizeY * std::fabs(mainOjbectSize.y),
                    sizeZ * std::fabs(mainOjbectSize.z));
    }
    else
    {
        ImGui::TextUnformatted("Model bounds are unavailable.");
    }
    ImGui::End();
    //背景
    ImGui::Begin("SceneSetting");
    ColorEditVec3("Window clear color", windowClearColor);
    ImGui::InputDouble("R", &windowClearColor.x, 1.0, 10.0, "%.2f");
    ImGui::InputDouble("G", &windowClearColor.y, 1.0, 10.0, "%.2f");
    ImGui::InputDouble("B", &windowClearColor.z, 1.0, 10.0, "%.2f");
    ColorEditVec3("Floor color", floorColor);

    ImGui::End();
    //現実世界の情報
    ImGui::Begin("RealWorldSetting");
    ImGui::Text("Monitor corners (world coordinates)");
    ImGui::InputDouble("pa X", &pa.x);
    ImGui::InputDouble("pa Y", &pa.y);
    ImGui::InputDouble("pa Z", &pa.z);
    ImGui::InputDouble("pb X", &pb.x);
    ImGui::InputDouble("pb Y", &pb.y);
    ImGui::InputDouble("pb Z", &pb.z);
    ImGui::InputDouble("pc X", &pc.x);
    ImGui::InputDouble("pc Y", &pc.y);
    ImGui::InputDouble("pc Z", &pc.z);
    ImGui::InputDouble("Camera distance", &cameraDis);
    ImGui::InputDouble("Camera height", &cameraHeight);
    SliderDouble("PlayerEye.X",playerEye.x, -100.0f,100.0f);
    SliderDouble("PlayerEye.Y",playerEye.y, 0,100.0f);
    SliderDouble("PlayerEye.Z",playerEye.z, 0,100.0f);
    ImGui::End();
    //台形補正
    ImGui::Begin("FixDrawDistance");
    ImGui::Text("windowClearColor");
    SliderDouble("Widthfix",widthfix, -2.0f, 2.0f);
    SliderDouble("opwidthfix",topwidthfix, -2.0f,2.0f);
    SliderDouble("TopPosFix", topposfix, -2.0f, 2.0f);
    SliderDouble("TopXLFix",TopXLfix, -2.0f,2.0f);
    SliderDouble("TopXRfix", TopXRfix, -2.0f, 2.0f);
    // SliderDouble("TopYfix",TopYfix,-2.0f,2.0f);
    ImGui::InputDouble("TopYfix", &TopYfix, -2.0, 2.0);
    SliderDouble("Warp X offset", vScaleX, -2.0f, 2.0f);
    SliderDouble("verticalFix", verticalFix, -2.0f, 2.0f);
    ImGui::Checkbox("Use procedural grid", &g_useProceduralGrid);
    ImGui::SliderFloat("Grid count", &g_gridCount, 2.0f, 40.0f);
    ImGui::SliderFloat("Grid line width (px)", &g_lineWidthPx, 0.5f, 5.0f);

    // double widthfix = 0.2f;//台形補正
    // double TopXLfix = -0.8f;//右上の台形補正
    // double TopXRfix = -0.2f;//右下の台形補正
    // double Xfix = -0.1f;
    
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    glutSwapBuffers();
}