#include "Globals.h"
//床のグローバル変数
Vec_3D fPoint[TILE][TILE];
double fWidth = 5000.0;
//視点極座標
double eDist, eDegX, eDegY;
double camX=0, camY=0, camZ=0;
double testD = 0;
double testB = 0;
double cameraToTargetDegX = 0;
double cameraToTargetDegY = 0;
double cameraLength = 10000;
double lookX=cameraLength, lookY=0, lookZ=0;
//カメラをズームするかそれとも並行移動するかのフラグ
bool isZooming = false;
Vec_3D e; // 視点の位置
// ウィンドウサイズ
int winW=1200, winH=800;
//マウス情報
int mButton, mState, mX, mY;
//現実世界の情報
//モニターの頂点
Vec_3D pa = {-0.3, 0.0, 0.0};
Vec_3D pb = { 0.3, 0.0, 0.0};
Vec_3D pc = {-0.3, 0.4, 0.0};
//フレームレート
double f = 30.0;
//オブジェクトのグローバル変数
double between_cy = 300;
double height_cy = 80;
double rad_cy = 30;
double dis = 1200;
//アニメーション
double leg_range = 1900;
double legL_anima = 0;
double legR_anima = 0;
double legLR_flag = false;
//ペンギン
double pen_rad = 0;
double pen_speed = 5;
double pen_moveX = 0;
double pen_moveZ = 0;
//ワニ
double open_mouth_speed = 1;
double open_mouth_deg = 0;
bool open_mouth_flag = true;

double legA_speed = 2;
double legA_deg = 0;
double legA_animaflag = false;
//ダチョウ
double ost_x[OSTS];
double ost_z[OSTS];
double ost_degs[OSTS];

double ost_speed = 60;
double ost_deg = 0;
double ost_animaflag = false;

double ost_nec_speed = 10;
double ost_nec_deg = 0;
bool ost_nec_animaflag = true;
//猿
double mon_leg_speed = 35;
double mon_leg_deg = 0;
double mon_leg_animaflag = true;

double mon_hand_speed = 5;
double mon_hand_deg = 0;
bool mon_hand_animaflag = true;

double mon_rad = 0;
double mon_speed = 40;
double mon_bigrad = 10;
double mon_moveX = 0;
double mon_moveZ = 0;

const int rows = 5;
const int cols = 50;
int array[5][50];
double rDisp=1.0;

//立体視変数
double eyeOffset = 0;//左右の視差量

//描画モード
bool NormalView = true;

// ボクセルの存在情報
const float SPACING = 50.0f;

// ボクセルの存在情報
bool voxels[VOXEL_SIZE][VOXEL_SIZE][VOXEL_SIZE];


//モデルの初期化
Model model;
Color color = {0.0,1.0,0.0};
//キューブの配置
CubeDispenser cubeDispenser = CubeDispenser(color); // CubeDispenserのインスタンスを作成



CubeCell pointingCell; // 現在ポイントしているセルの情報を格納する変数

//プレイヤー
Vec_3D player;

GLuint leftTex = 0;
GLuint rightTex = 0;

bool warpInitialized = false;