#include "Globals.h"
//床のグローバル変数
Vec_3D fPoint[TILE][TILE];
double fWidth = 5000.0;
//視点極座標
double eDist, eDegX, eDegY;
// ウィンドウサイズ
int winW, winH;
//マウス情報
int mButton, mState, mX, mY;
//フレームレート
double f = 30.0;
//オブジェクトのグローバル変数
double between_cy = 300;
double height_cy = 80;
double rad_cy = 30;
double dis = 1200;
//アニメーション
double leg_range = 50;
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
// ボクセルの存在情報
const float SPACING = 50.0f;

// ボクセルの存在情報
bool voxels[VOXEL_SIZE][VOXEL_SIZE][VOXEL_SIZE];


//モデルの初期化
Model model;