#include "GLApp.h"
#include "Globals.h"
#include "Shape.h"
#include "Objects.h"
#include "Voxcel.h"
#include "Ray.h"
#include "Model.h"
//初期設定関数
void initGL()
{
    //ウィンドウ生成
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);  //ディスプレイ表示モード指定
    glutInitWindowSize(1200, 800);  //ウィンドウサイズの指定
     int windowId = glutCreateWindow("CG Final");
    std::cout << "windowId: " << windowId << std::endl;
  
    const GLubyte* version = glGetString(GL_VERSION);

    if (version == nullptr) {
        std::cout << "GL_VERSION is NULL" << std::endl;
    } else {
        std::cout << "GL_VERSION: " << reinterpret_cast<const char*>(version) << std::endl;
    }
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

}

//ディスプレイコールバック関数
void display()
{
    initView(true);
    // オブジェクト描画
    dispobj();
    //テクスチャ
    glBindTexture(GL_TEXTURE_2D, leftTex);
    glCopyTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        0,
        0,
        winW/2,
        winH
    );
    initView(false);
    // オブジェクト描画
    dispobj();
    // initView(false);
    // dispobj();
    //テクスチャ
    glBindTexture(GL_TEXTURE_2D, rightTex);

    glCopyTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        winW/2,
        0,
        winW/2,
        winH
    );
    GLenum err = glGetError();

// std::cout << err << std::endl;
        //     unsigned char pixel[4];
        // glReadPixels(
        //     winW/4,
        //     winH/2,
        //     1,
        //     1,
        //     GL_RGBA,
        //     GL_UNSIGNED_BYTE,
        //     pixel
        // );

        // std::cout
        // << (int)pixel[0] << " "
        // << (int)pixel[1] << " "
        // << (int)pixel[2] << std::endl;
    if(!NormalView){
      DrawWarpedTextures();
    }

    glutSwapBuffers();
}
void initView(bool isLeftEye) {
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    int viewW = static_cast<int>(winW * rDisp);
    int viewH = static_cast<int>(winH * rDisp);
    double aspect = static_cast<double>(viewW) / static_cast<double>(viewH);

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
        // gluPerspective(
        //     40.0,
        //     aspect,
        //     1.0,
        //     10000.0
        // );
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(
            camX ,camY,camZ,
            lookX+camX, lookY+ camY, lookZ+camZ,
            0.0, 1.0, 0.0
        );
        
        //  gluLookAt(
        //     e.x  +camX , e.y + camY, e.z + camZ,
        //     camX+e.x, camY+e.y, camZ+e.z,
        //     0.0, 1.0, 0.0
        // );
        return;
    }
    //glViewport(0, 0, viewW, viewH);

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
    double fruH = 33.0f/4.0f;
    double fruW = 21.7f/4.0f;
    double winDis = 90.0f/2.0f;
    // パラメータ定義
    float W = 21.7f;      // モニターの横幅 (メートル換算など)
    float H = 54.0f;      // モニターの縦幅
    float dd = 90.0f;      // モニターまでの垂直距離 (50cm)
    float nearPlane = 90.0f/2.0f;
    float farPlane = 1000.0f;

    // 1. プロジェクション行列 (Frustum) の計算
    float left   = -(W / 2.0f) * (nearPlane / dd);
    float right  =  (W / 2.0f) * (nearPlane / dd);
    float bottom = -(H / 2.0f) * (nearPlane / dd);
    float top    =  (H / 2.0f) * (nearPlane / dd);

     glFrustum(
     bottom, //left
     top, //Right
     left,//bottom
     right, //top
     nearPlane,
     farPlane
    );
    // glFrustum(
    //  -fruW, //left
    //  fruW, //Right
    //  -fruH,//bottom
    //  fruH, //top
    //  winDis,
    //  1000.0
    // );
    //  gluPerspective(
    //         40.0,
    //         aspect,
    //         1.0,
    //         10000.0
    //     );

    // ビューイング変換準備
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    double LookY = 0;
    double LookZ = 0;
    double LookYp = -90;
    double LookZp = -65.36;
    double angle = 45.0f;
    //Vec_3D viewDir = {0.0f, -sin(angle), -cos(angle)};
    if (isLeftEye) {
        gluLookAt(
            eyeOffset, LookY,LookZ,
            eyeOffset,LookYp,LookZp,
            1.0, 0.0, 0.0
        );
    } else {
        gluLookAt(
            -eyeOffset, LookY, LookZ,
            -eyeOffset,LookYp,LookZp,
            1.0, 0.0, 0.0
        );
    }
    //  if (isLeftEye) {
    //     gluLookAt(
    //         eyeOffset, LookY,LookZ,
    //         eyeOffset+viewDir.x, viewDir.y, viewDir.z,
    //         1.0, 0.0, 0.0
    //     );
    // } else {
    //     gluLookAt(
    //         -eyeOffset, LookY, LookZ,
    //         -eyeOffset+viewDir.x, viewDir.y,viewDir.z,
    //         1.0, 0.0, 0.0
    //     );
    // }
    
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
    // DrawVoxelObject(VOXEL_SIZE);
    // penguin(0,0);
    // model.Draw();
    glPopMatrix();

    //ボクセル
    glPushMatrix();

    // 中心に寄せる
    setColor(0.5, 0.0, 0.5, 1.0);
    glTranslatef(0,
                0,
                0);
    penguin_animation();
    //penguin(0,0);
    model.Draw();
    glPopMatrix();
    //3Dモデル
    glPushMatrix();
    // glRotated(eDegY, 0.0, 1.0, 0.0);  //こっちに向く
    glTranslated(0,0,-3000);
    glRotated(180, 0.0, 1.0, 0.0);  //こっちに向く
    glScaled(150.0,150.0,150.0);

    setColor(1.0, 1.0, 1.0, 1.0);
    //model.Draw();
    
    glPopMatrix();
    //目標物体
    double LookY = 90;
    double LookZ = 50;
    glPushMatrix();
    // glRotated(eDegY, 0.0, 1.0, 0.0);  //こっちに向く

    glTranslated(0,5-LookY,-LookZ);
    glRotated(180, 0.0, 1.0, 0.0);  //こっちに向く
    glScaled(20,10,10);
    setColor(0.0, 1.0, 0.0, 1.0);
   //draw_floor(1,1,0,0,0);
    setColor(1.0, 1.0, 1.0, 1.0);

    glutSolidCube(1);
    glPopMatrix();
    double wallDis = 10.92;

    //床
    glPushMatrix();
    glTranslated(0,-LookY,-LookZ);
    glRotated(180, 0.0, 0.0, 1.0);  //こっちに向く
    glMyDrawCheckerFloor(47, 25);
//    draw_floor(1,1,0,0,0);
    glPopMatrix();
    //右壁
    glPushMatrix();
    glTranslated(wallDis,-LookY,-LookZ);
    glRotated(90, 0.0, 0.0, 1.0);  //こっちに向く
    glMyDrawCheckerFloor(47, 25);
//    draw_floor(1,1,0,0,0);
    glPopMatrix();
    //左壁
    glPushMatrix();
    glTranslated(-wallDis,-LookY,-LookZ);
    glRotated(-90, 0.0, 0.0, 1.0);  //こっちに向く
    glMyDrawCheckerFloor(47, 25);
    glPopMatrix();
     //おくかべ
    glPushMatrix();

    glTranslated(0,-LookY+12,-LookZ-wallDis*2);
    glRotated(90, 1.0, 0.0, 0.0);  //こっちに向く
    glScaled(1.0,7.0,1.0);
    glMyDrawCheckerFloor(27, 25);
//    draw_floor(1,1,0,0,0);
    glPopMatrix();
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
        glutSolidCube(GRID_SIZE);
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
    //     glMatrixMode(GL_PROJECTION);
    //     glLoadIdentity();
    //     // glFrustum(
    //     // -aspect, aspect,
    //     // -1.0, 1.0,
    //     // 1.0, 10000.0
    //     // );
    //     gluPerspective(
    //         40.0,
    //         aspect,
    //         1.0,
    //         10000.0
    //     );
    //     glMatrixMode(GL_MODELVIEW);
    //     glLoadIdentity();
    //     gluLookAt(
    //         camX ,camY,camZ,
    //         lookX+camX, lookY+ camY, lookZ+camZ,
    //         0.0, 1.0, 0.0
    //     );
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

    //double widthfix = testB;//台形補正
    //double Xfix = testB;
    // double Lfix = testB;

    glBindTexture(GL_TEXTURE_2D, leftTex);
//左眼用の描画
    glBegin(GL_QUADS);

    glTexCoord2f(0,0);
    glVertex2f(-1.0f,-1.0f);

    glTexCoord2f(1,0);
    glVertex2f(0.0,-1.0f+widthfix+Xfix);

    glTexCoord2f(1,1);
    // glVertex2f(topHalf*0.5f,1.0f);
    glVertex2f(0.0,1.0f-widthfix+Xfix+Lfix);
    glTexCoord2f(0,1);
    glVertex2f(-1.0f,1.0f);
    // glVertex2f(-topHalf*0.5f,1.0f);

    glEnd();

    glBindTexture(GL_TEXTURE_2D, rightTex);
//右目用の描画
    glBegin(GL_QUADS);

    glTexCoord2f(0,0);
    glVertex2f(0.0f,-1.0f);

    glTexCoord2f(1,0);
  
    glVertex2f(1.0f,-1.0f+widthfix+Xfix);

    glTexCoord2f(1,1);
    // glVertex2f(topHalf*0.5f,1.0f);
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
    int viewW = static_cast<int>(w * rDisp / 2.0);
    int viewH = static_cast<int>(h * rDisp);
    glViewport(0, 0, viewW, viewH);  //ウィンドウ内の描画領域(ビューポート)の指定

    //投影変換
    glMatrixMode(GL_PROJECTION);  //カレント行列の設定
    glLoadIdentity();  //カレント行列初期化
    gluPerspective(40.0, (double)viewW/(double)viewH, 1.0, 10000.0);  //投影変換行列生成
    
    winW = w; winH = h;  //ウィンドウサイズをグローバル変数に格納

    
}

//タイマーコールバック関数
void timer(int value)
{
    glutPostRedisplay();  //ディスプレイイベント強制発生
    glutTimerFunc(1000/f, timer, 0);  //タイマー再設定

    // //アニメーション
    // penguin_animation();
    // alligator_animation();
    // ostrich_animation();
    // monkey_animation();
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
   // std::cout << "Mouse Button: " << mButton << ", State: " << mState << ", X: " << mX << ", Y: " << mY << std::endl;
    
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
        // vectorNormalize
        camX += (mX-x)*20*right.x;  //マウス横方向→水平角
        camY -= (mY-y)*20;  //マウス縦方向→垂直角
        camZ += (mX-x)*20*right.z;  //マウス縦方向→垂直角
        }
    }
    //double cameraDis = vectorLen(cameraVec);
    if (mButton == GLUT_RIGHT_BUTTON) {
            // //右ドラッグでカメラを回転
        cameraToTargetDegX += (mY-y)*0.1;
        cameraToTargetDegY += (mX-x)*0.1;
        lookX = cameraLength * cos(cameraToTargetDegY * M_PI / 180.0);
        lookZ = cameraLength * sin(cameraToTargetDegY * M_PI / 180.0);
        // lookX = cameraDis * cos(45 * M_PI / 180.0);
        // lookZ = cameraDis * cos(45 * M_PI / 180.0);
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
            eyeOffset += 0.1;
            break;
        case 's':
            if(NormalView == false){
                eyeOffset -= 0.1;
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
        // case GLUT_KEY_UP:
        //     std::cout << "↑" << std::endl;
            
        //     break;
        // case GLUT_KEY_DOWN:
        //     std::cout << "↓" << std::endl;
        //     break;
        // case GLUT_KEY_LEFT:
        //     std::cout << "←" << std::endl;
        //     break;
        // case GLUT_KEY_RIGHT:
        //     std::cout << "→" << std::endl;
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
