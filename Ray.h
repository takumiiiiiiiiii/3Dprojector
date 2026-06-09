#pragma once
#include "Vec3D.h"
#include "CubeDispenser.h"
struct Ray{
    Vec_3D origin; // レイの始点
    Vec_3D direction; // レイの方向（正規化されていることが望ましい）
};

Ray screen2ray(int x,int y);
bool intersectFloor(const Ray& ray,Vec_3D* hit);
bool intersectCubeCell(const Ray& ray, const CubeCell& cell, double gridSize, double* tHit, Vec_3D* hit, Vec_3D* normal);