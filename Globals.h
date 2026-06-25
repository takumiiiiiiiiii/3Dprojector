#pragma once
#include "Vec3D.h"
#include "Model.h"
#include "CubeDispenser.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TILE 50
#define OSTS 10
#define VOXEL_SIZE 20
#define GRID_SIZE 500.0f

extern Vec_3D fPoint[TILE][TILE];
extern double fWidth;

//カメラの位置と向き
extern double eDist;
extern double eDegX;
extern double eDegY;
extern double lookX;
extern double lookY;
extern double lookZ;
extern double camX;
extern double camY;
extern double camZ;
extern double testD;
extern double testB;
extern bool isZooming;
// 視点極座標から直交座標へ変換

// カメラからターゲットへの角度
extern double cameraToTargetDegX;
extern double cameraToTargetDegY;
extern double cameraLength;

//視点の位置
extern Vec_3D e;

//ウィンドウサイズ
extern int winW;
extern int winH;

extern int mButton;
extern int mState;
extern int mX;
extern int mY;

extern double f;

extern double between_cy;
extern double height_cy;
extern double rad_cy;
extern double dis;

extern double leg_range;
extern double legL_anima;
extern double legR_anima;
extern double legLR_flag;

extern double pen_rad;
extern double pen_speed;
extern double pen_moveX;
extern double pen_moveZ;

extern double open_mouth_speed;
extern double open_mouth_deg;
extern bool open_mouth_flag;

extern double legA_speed;
extern double legA_deg;
extern double legA_animaflag;
//ダチョウ
extern double ost_x[OSTS];
extern double ost_z[OSTS];
extern double ost_degs[OSTS];

extern double ost_speed;
extern double ost_deg;
extern double ost_animaflag;

extern double ost_nec_speed;
extern double ost_nec_deg;
extern bool ost_nec_animaflag;

extern double mon_leg_speed;
extern double mon_leg_deg;
extern double mon_leg_animaflag;

extern double mon_hand_speed;
extern double mon_hand_deg;
extern bool mon_hand_animaflag;

extern double mon_rad;
extern double mon_speed;
extern double mon_bigrad;
extern double mon_moveX;
extern double mon_moveZ;

extern const int rows;
extern const int cols;
extern int array[5][50];

extern double rDisp;

//描画モード
extern bool NormalView;
//実際の描画サイズ
extern double realDisp;
extern double disDisp;

//立体視変数
extern double eyeOffset;//左右の視差量

//僕せる
extern const float SPACING;

// ボクセルの存在情報
extern bool voxels[VOXEL_SIZE][VOXEL_SIZE][VOXEL_SIZE];

//3Dモデル
extern Model model;

//キューブ配置
//キューブの大きさ
// extern std::vector<CubeCell> placedCubes; // 配置されたキューブの情報を格納するベクター
extern CubeDispenser cubeDispenser;
extern CubeCell pointingCell; // 現在ポイントしているセルの情報を格納する変数

//プレイヤー
extern Vec_3D player;

//テクスチャ
extern GLuint leftTex ;
extern GLuint rightTex;

extern bool warpInitialized;