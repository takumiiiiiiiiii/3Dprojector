#pragma once

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