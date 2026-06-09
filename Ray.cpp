//Ray.cpp
#include "Ray.h"
#include "Globals.h"
#include "CubeDispenser.h"
#include <cmath>
#include <algorithm>


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
    //レイの視点の位置はnearPosで
    ray.origin = nearPos;
    //レイの方向はnearPosからfarPosへのベクトルを正規化したもの
    ray.direction = vectorNormalize(diffVec(farPos,nearPos));
    return ray;
}

bool intersectFloor(const Ray& ray,Vec_3D* hit){
    if(std::fabs(ray.direction.y) < 0.00001){
        return false; // レイが床と平行な場合は交差しない
    }
    // レイの方程式を使って、y=0の平面との交点を求める
    //P(t) = ray.origin + t * ray.direction
    //P(t)はy=0の平面上にあるので、P(t).y = 0となるtを求める
    double t = -ray.origin.y / ray.direction.y; 
    if(t < 0){
        return false; // レイが床の後ろにある場合は交差しない
    }
    // 交差点の座標を計算してhitに格納
    *hit = addVec(ray.origin,multiVec(ray.direction,t));
    return true;
}

// キューブセルとの交差判定関数
bool intersectCubeCell(const Ray& ray, const CubeCell& cell, double gridSize, double* tHit, Vec_3D* hit, Vec_3D* normal)
{
    const double eps = 0.000001;
    // キューブの中心座標を計算
    Vec_3D center = makeVec(
        cell.gx *gridSize,
        cell.gy *gridSize + gridSize/2.0,
        cell.gz * gridSize
    );
    // キューブの最小点と最大点を計算
    Vec_3D minP = makeVec(
        center.x - gridSize/2.0,
        center.y - gridSize/2.0,
        center.z - gridSize/2.0
    );
    // キューブの最大点を計算
    Vec_3D maxP = makeVec(
        center.x + gridSize/2.0,
        center.y + gridSize/2.0,
        center.z + gridSize/2.0
    );

    double tMin = 0.0;
    double tMax = 1e30; // 十分大きな値
    Vec_3D hitNormal = makeVec(0,0,0);
    // 各軸について、スラブとの交差をチェックするラムダ関数
    //スラブとは、キューブの各面に平行な無限に広がる平面のこと。レイとスラブの交点を求めることで、レイがキューブのどの面と交差するかを判定する。
    //例えば、x軸に平行なスラブは、x = minP.xとx = maxP.xの2つがある。レイがこれらのスラブと交差するかをチェックすることで、レイがキューブのどの面と交差するかを判定できる。
    auto checkAxis = [&](double origin,double dir,double minV,double maxV,Vec_3D nMin,Vec_3D nMax){
        // レイが軸に平行な場合の処理
        if(std::fabs(dir) < eps){
            return origin >= minV && origin <= maxV; // レイが軸に平行な場合、原点がスラブ内にあるか
        }
        double t1 = (minV - origin) / dir;
        double t2 = (maxV - origin) /dir;
        Vec_3D n1 = nMin;
        Vec_3D n2 = nMax;
        
        if(t1 > t2){
            std::swap(t1,t2);
            std::swap(n1,n2);
        }
        if(t1>tMin){
            tMin = t1;
            hitNormal = n1;
        }
        if(t2 < tMax){
            tMax = t2;
        }
        return tMin <= tMax;
    };
    
    if(!checkAxis(ray.origin.x,ray.direction.x,minP.x,maxP.x,makeVec(-1,0,0),makeVec(1,0,0))) return false;
    if(!checkAxis(ray.origin.y,ray.direction.y,minP.y,maxP.y,makeVec(0,-1,0),makeVec(0,1,0))) return false;
    if(!checkAxis(ray.origin.z,ray.direction.z,minP.z,maxP.z,makeVec(0,0,-1),makeVec(0,0,1))) return false;

    // 交差点の座標と法線を計算して出力
    if(tMax < 0){
        return false;
    }

    double t = tMin >= 0 ? tMin : tMax; // レイの原点からの距離tを選択
    // 交差点の座標を計算してhitに格納
    if(hit){
        *hit = addVec(ray.origin,multiVec(ray.direction,t));
    }
    // 法線を格納
    if(normal){
        *normal = hitNormal;
    }

    return true;
}
