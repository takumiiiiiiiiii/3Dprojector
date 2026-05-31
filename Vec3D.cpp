#include <math.h>
#include "Vec3D.h"
#include "Globals.h"

// v1とv2の外積計算
Vec_3D crossProduct(Vec_3D vec1, Vec_3D vec2)
{
    Vec_3D out;

    out.x = vec1.y * vec2.z - vec1.z * vec2.y;
    out.y = vec1.z * vec2.x - vec1.x * vec2.z;
    out.z = vec1.x * vec2.y - vec1.y * vec2.x;

    out = vectorNormalize(out);

    return out;
}

// v1とv2の内積計算
double innerProduct(Vec_3D v1, Vec_3D v2)
{
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

// ベクトルの差の計算
Vec_3D diffVec(Vec_3D v1, Vec_3D v2)
{
    Vec_3D out;

    out.x = v1.x - v2.x;
    out.y = v1.y - v2.y;
    out.z = v1.z - v2.z;

    return out;
}

// ベクトルの和の計算
Vec_3D addVec(Vec_3D v1, Vec_3D v2)
{
    Vec_3D out;

    out.x = v1.x + v2.x;
    out.y = v1.y + v2.y;
    out.z = v1.z + v2.z;

    return out;
}

// ベクトルの定数倍
Vec_3D multiVec(Vec_3D v, double c)
{
    Vec_3D out;

    out.x = c * v.x;
    out.y = c * v.y;
    out.z = c * v.z;

    return out;
}

// ベクトルの長さ
double vectorLen(Vec_3D vec)
{
    double len = sqrt(pow(vec.x, 2) + pow(vec.y, 2) + pow(vec.z, 2));
    return len;
}

// ベクトル正規化
Vec_3D vectorNormalize(Vec_3D vec)
{
    double len = vectorLen(vec);

    if (len > 0) {
        vec.x /= len;
        vec.y /= len;
        vec.z /= len;
    }

    return vec;
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
