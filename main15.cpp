//g++ -O3 main15.cpp GLApp.cpp Globals.cpp Objects.cpp Shape.cpp Vec3D.cpp Voxcel.cpp Model.cpp -std=c++11 -framework OpenGL -framework GLUT `pkg-config --cflags --libs opencv4 assimp` -Wno-deprecated
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif
#include <opencv2/opencv.hpp>  //OpenCV

#include "GLApp.h"
#include "Globals.h"
#include "Objects.h"
#include "Shape.h"
#include "Vec3D.h"
#include "Voxcel.h"
#define TILE 50  //床頂点数
#define OSTS 10

//関数名の宣言


//メイン関数
int main(int argc, char *argv[])
{
    setrundum(rows,cols);
    glutInit(&argc, argv);  //OpenGL,GLUTの初期化
    
    initGL();  //初期設定
    
    glutMainLoop();  //イベント待ち無限ループ突入
    
    return 0;
}
