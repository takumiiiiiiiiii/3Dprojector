#include "GLApp.h"
#include "Globals.h"
#include "Shape.h"
#include "Objects.h"
#include "Voxcel.h"
//初期設定関数
void initGL()
{
    // モデル読み込み
    if (!model.Load("Jusmin_Lowpoly.obj"))
    {
        std::cerr << "モデルの読み込みに失敗しました" << std::endl;
        exit(1);
    }
    //ウィンドウ生成
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);  //ディスプレイ表示モード指定
    glutInitWindowSize(1200, 800);  //ウィンドウサイズの指定
    glutCreateWindow("CG Final");  //ウィンドウ生成
    
    //コールバック関数指定
    glutDisplayFunc(display);  //ディスプレイコールバック関数（"display"）
    glutReshapeFunc(reshape);  //リサイズコールバック関数（"reshape"）
    glutTimerFunc(1000/f, timer, 0);  //タイマーコールバック関数（"timer", 1000/fミリ秒）
    glutMouseFunc(mouse);  //マウスクリックコールバック関数
    glutMotionFunc(motion);  //マウスドラッグコールバック関数
    glutKeyboardFunc(keyboard);  //キーボードコールバック関数
    
    //各種設定
    glClearColor(0.0, 202.0/255.0,244.0/255.0, 1.0);  //ウィンドウクリア色の指定（RGBA値）
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
}

//ディスプレイコールバック関数
void display()
{
    
    //ディスプレイコールバック関数では、立体視のためにウィンドウを左右に分割して描画します。
    int viewW = static_cast<int>(winW * rDisp / 2.0);
    int viewH = static_cast<int>(winH * rDisp);
    double aspect = static_cast<double>(viewW) / static_cast<double>(viewH);

    // 左目用に左半分へ描画する
    glViewport(0, 0, viewW, viewH);

    // ウィンドウクリア
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 投影変換
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(
        40.0,
        aspect,
        1.0,
        10000.0
    );

    // ビューイング変換準備
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // 視点極座標から直交座標へ変換
    Vec_3D e;
    e.x = eDist * cos(eDegX * M_PI / 180.0) * sin(eDegY * M_PI / 180.0);
    e.y = eDist * sin(eDegX * M_PI / 180.0);
    e.z = eDist * cos(eDegX * M_PI / 180.0) * cos(eDegY * M_PI / 180.0);
            gluLookAt(
            e.x - eyeOffset, e.y, e.z,
            0.0, 0.0, 0.0,
            0.0, 1.0, 0.0
        );
    // オブジェクト描画
    dispobj();


    // 右目用に右半分へ描画する
    glViewport(viewW, 0, viewW, viewH);

    // // ウィンドウクリア
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 投影変換
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(
        40.0,
        aspect,
        1.0,
        10000.0
    );

    // ビューイング変換準備
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(
        e.x + eyeOffset, e.y, e.z,
        0.0, 0.0, 0.0,
        0.0, 1.0, 0.0
    );
    // オブジェクト描画
    dispobj();
    glutSwapBuffers();
}

void dispobj(){
    //光源配置
    GLfloat lightPos0[] = {500.0, 2000.0, 2500.0, 1.0};  //光源座標(点光源)
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);  //光源配置
    GLfloat lightPos1[] = {-500.0, 2000.0, -500.0, 1.0};  //光源座標(点光源)
    glLightfv(GL_LIGHT1, GL_POSITION, lightPos1);  //光源配置

    //----------床パネル----------
    setColor(0.2, 1.0, 0.2, 1.0);
    draw_floor(1,1,0,0,0);

    //ボクセル
    glPushMatrix();

    // 中心に寄せる
    float objectSize = VOXEL_SIZE * SPACING;
    setColor(0.5, 0.0, 0.5, 1.0);
    glTranslatef(-objectSize / 2.0f,
                 0,
                 -objectSize / 2.0f);
    // DrawVoxelObject(VOXEL_SIZE);
    penguin(0,0);
    // model.Draw();
    glPopMatrix();

    //ボクセル
    glPushMatrix();

    // 中心に寄せる

    setColor(0.5, 0.0, 0.5, 1.0);
    glTranslatef(400,
        
                 0,
                 -2100);
    // DrawVoxelObject(VOXEL_SIZE);
    penguin_animation();
    penguin(0,0);
    // model.Draw();
    glPopMatrix();

    //3Dモデル
    glPushMatrix();
    // glRotated(eDegY, 0.0, 1.0, 0.0);  //こっちに向く
    glRotated(180, 0.0, 1.0, 0.0);  //こっちに向く
    glScaled(150.0,150.0,150.0);

    setColor(1.0, 1.0, 1.0, 1.0);
    // model.Draw();
    
    glPopMatrix();
    // モデル描画

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

//マウスクリックコールバック関数
void mouse(int button, int state, int x, int y)
{
    // スクリーン座標 → ワールド座標
    Vec_3D click_point = screen2world(x, y);

    GLdouble model[16], proj[16];  //変換行列格納用
    GLint view[4];  //ビューポート設定格納用
    GLfloat winX, winY, winZ;  //ウィンドウ座標
    GLdouble objX, objY, objZ;  //ワールド座標
    
    //マウス情報をグローバル変数に格納
    mButton = button; mState = state; mX = x; mY = y;
    if(state ==GLUT_DOWN){
        if(button==GLUT_LEFT_BUTTON){
            //マウス座標からウィンドウ座標の取得
            winX = mX;winY = winH - mY;
            glReadPixels(winX,winY,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&winZ);
            //モデルビュー変換行列・投影変換行列・ビューポート設定取り出し
            glGetDoublev(GL_MODELVIEW_MATRIX,model);
            glGetDoublev(GL_PROJECTION_MATRIX,proj);
            glGetIntegerv(GL_VIEWPORT,view);

            //ウィンドウ行列をワールド座標に変換
            gluUnProject(winX,winY,winZ,model,proj,view,&objX,&objY,&objZ);
            std::cout << "Clicked world coordinates: (" << objX << ", " << objY << ", " << objZ << ")" << std::endl;
            float SPACING = 50.0f; // ボクセルのサイズ
            //クリックした位置の四角い範囲のボクセルをトグル
            int voxelX = static_cast<int>(objX / SPACING);
            int voxelY = static_cast<int>(objY / SPACING);
            int voxelZ = static_cast<int>(objZ / SPACING);
            
            if (voxelX >= 0 && voxelX < VOXEL_SIZE &&
                voxelY >= 0 && voxelY < VOXEL_SIZE &&
                voxelZ >= 0 && voxelZ < VOXEL_SIZE) {
                voxels[voxelX][voxelY][voxelZ] = !voxels[voxelX][voxelY][voxelZ];  //ボクセルの状態をトグル 
            }
        }


    }
}

//マウスドラッグコールバック関数
void motion(int x, int y)
{
    //左ドラッグで変更
    if(mButton==GLUT_RIGHT_BUTTON){
        eDegY += (mX-x)*0.2;  //マウス横方向→水平角
        eDegX += (y-mY)*0.2;  //マウス縦方向→垂直角
        if (eDegX>85) eDegX = 85.0;
        if (eDegX<-85) eDegX = -85.0;
    }
    
    //マウス座標をグローバル変数に保存
    mX = x; mY = y;
}

//キーボードコールバック関数
void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
        case 27:  //[ESC]キー
        case 'w':
            eyeOffset += 10.0;
            break;
        case 's':
            eyeOffset -= 10.0;
            break;
        case 'q':  //[q]キー
        
        case 'Q':  //[Q]キー
            exit(0);  //プロセス終了
            break;
        case '+':
            eDist -= 100.0;
            break;
        case '-':
            eDist += 100.0;
            break;

        default:
            break;
    }
}

// スクリーン座標 -> ワールド座標
Vec_3D screen2world(int x, int y)
{
    GLdouble model[16], proj[16]; //変換行列格納用
    GLint view[4]; //ビューポート設定格納用
    GLfloat winX, winY, winZ; //ウィンドウ座標
    GLdouble objX, objY, objZ; //ワールド座標
    
    //マウス座標からウィンドウ座標の取得
    winX = x; winY = winH-y; //x 座標，y 座標
    glReadPixels(winX, winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ); //z 座標はデプス値
    //モデルビュー変換行列・投影変換行列・ビューポート設定取り出し
    glGetDoublev(GL_MODELVIEW_MATRIX, model); //モデルビュー変換行列
    glGetDoublev(GL_PROJECTION_MATRIX, proj); //投影変換行列
    glGetIntegerv(GL_VIEWPORT, view); //ビューポート設定

    //ウィンドウ座標(winX, winY, winZ)をワールド座標(objX, objY, objZ)に変換
    gluUnProject(winX, winY, winZ, model, proj, view, &objX, &objY, &objZ);
    
    Vec_3D p = {objX, objY, objZ};
    return p;
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
