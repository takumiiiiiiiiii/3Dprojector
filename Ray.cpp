//Ray.cpp
#include "Ray.h"
#include "Globals.h"
#include <cmath>
// スクリーン座標をワールド座標に変換する関数
static Vec_3D screenUnProject(int x,int y,double winZ){
    GLdouble model[16],proj[16];
    GLint view[4];
    GLdouble objX,objY,objZ;

    GLdouble winX = x;
    GLdouble winY = winH - y;

    // モデルビュー変換行列の取得
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    // 投影変換行列の取得
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    //ビューポート設定の取得
    glGetIntegerv(GL_VIEWPORT, view);

    // 変換行列とウィンドウ座標からワールド座標を計算
    gluUnProject(winX,winY,winZ,model,proj,view,&objX,&objY,&objZ);
    
    Vec_3D p = {objX, objY, objZ};
    return p;
}

// スクリーン座標からレイを生成する関数
Ray screen2ray(int x,int y){
    Vec_3D nearPos = screenUnProject(x,y,0.0);
    Vec_3D farPos = screenUnProject(x,y,1.0);

    Ray ray;
    ray.origin = nearPos;
    ray.direction = vectorNormalize(diffVec(farPos,nearPos));
    return ray;
}

bool intersectFloor(const Ray& ray,Vec_3D* hit){
    if(ray.direction.y == 0){
        return false; // レイが床と平行な場合は交差しない
    }
    double t = -ray.origin.y / ray.direction.y; // レイのパラメータtを計算
    if(t < 0){
        return false; // レイが床の後ろにある場合は交差しない
    }
    // 交差点の座標を計算
    *hit = addVec(ray.origin,multiVec(ray.direction,t));
    return true;
}