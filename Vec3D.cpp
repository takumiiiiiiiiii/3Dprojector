#include <math.h>
#include "Vec3D.h"

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