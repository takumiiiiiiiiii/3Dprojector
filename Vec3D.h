#pragma once
// #include <opencl-c-base.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

// 三次元ベクトル構造体
typedef struct _Vec_3D
{
    double x, y, z;
} Vec_3D;

// ベクトル計算関数の宣言
Vec_3D crossProduct(Vec_3D vec1, Vec_3D vec2);  // 外積計算
double innerProduct(Vec_3D v1, Vec_3D v2);      // 内積計算
Vec_3D diffVec(Vec_3D vec1, Vec_3D vec2);       // ベクトルの差
Vec_3D addVec(Vec_3D vec1, Vec_3D vec2);        // ベクトルの和
Vec_3D multiVec(Vec_3D v, double c);            // ベクトルの定数倍
double vectorLen(Vec_3D vec);                   // ベクトルの長さ
Vec_3D vectorNormalize(Vec_3D vec);             // ベクトルの正規化
Vec_3D screen2world(int x, int y);              // スクリーン座標 -> ワールド座標
Vec_3D makeVec(double x, double y, double z);          // Vec_3D構造体を作成する関数