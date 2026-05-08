#pragma once

void draw_floor(double scale_x,double scale_z,double x,double y,double z);  // 床の表示
void glMyCylinder(double top, double bottom, double height, double slices);  //円柱
void my_drawCylinder(double between,double height,double width);//円柱の設置
void my_drawCylinder_fall(double between,double height,double width,double y);//倒れた
void glMySolidCircle(double cx, double cy, double cz, double r, int p);  //塗り潰し円
void setColor(double r, double g, double b, double a);  // 色のセット
void setSpecular(double r, double g, double b, double a, double n);  // 鏡面反射のセット