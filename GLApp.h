#pragma once
#include <GLUT/glut.h>  //OpenGL/GLUT
#include <opencv2/opencv.hpp>  //OpenCV
#include "Vec3D.h"

void initGL();
void display();
void dispobj();
void reshape(int w, int h);
void timer(int value);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void keyboard(unsigned char key, int x, int y);
Vec_3D screen2world(int x, int y);
void setrundum(int i, int j);
