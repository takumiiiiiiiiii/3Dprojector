
#include "Shape.h"
#include "Vec3D.h"
#include "Globals.h"
#include <GLUT/glut.h>  //OpenGL/GLUT
#include <opencv2/opencv.hpp>  //OpenCV
#include <math.h>


//----------床パネル----------
void draw_floor(double scale_x,double scale_z,double x,double y,double z)
{
    glPushMatrix();
    glTranslated(x,y,z);
    glScaled(scale_x,1,scale_z);
    glBegin(GL_QUADS);  //図形開始

    for (int j=0; j<TILE-1; j++) {
        for (int i=0; i<TILE-1; i++) {
            //法線ベクトル計算
            Vec_3D v1, v2, n;
            v1 = diffVec(fPoint[i][j+1], fPoint[i][j]);
            v2 = diffVec(fPoint[i+1][j], fPoint[i][j]);
            n = crossProduct(v1, v2);  //法線ベクトル
            glNormal3d(n.x, n.y, n.z);  //法線ベクトル適用
            //頂点座標
            glVertex3d(fPoint[i][j].x, fPoint[i][j].y, fPoint[i][j].z);  //頂点座標
            glVertex3d(fPoint[i][j+1].x, fPoint[i][j+1].y, fPoint[i][j+1].z);  //頂点座標
            glVertex3d(fPoint[i+1][j+1].x, fPoint[i+1][j+1].y, fPoint[i+1][j+1].z);  //頂点座標
            glVertex3d(fPoint[i+1][j].x, fPoint[i+1][j].y, fPoint[i+1][j].z);  //頂点座標
        }
    }

    glEnd();  //図形終了
        glPopMatrix();
}


void glMyCylinder(double top, double bottom, double height, double slices)
{
    glPushMatrix();
    
    //上面の円
    glPushMatrix();
    glTranslated(0, 0, height/2);
    glMySolidCircle(0, 0, 0, top, slices);
    glPopMatrix();
    //底面の円
    glPushMatrix();
    glTranslated(0, 0, -height/2);
    glRotated(180, 0, 1, 0);
    glRotated(180, 0, 0, 1);
    glMySolidCircle(0, 0, 0, bottom, slices);
    glPopMatrix();
    
    //胴体
    Vec_3D p1, p2, p3, p4;  //４頂点
    Vec_3D v1, v2, nv;
    double t1, t2, x, y, z;
    double delta = 2.0*M_PI/slices;
    //折れ線近似の開始
    glBegin(GL_QUADS);
    for(int i=0; i<(int)slices; i++){  //円周上のサンプリング
        t1 = delta*i;
        //p1の計算
        x = top*cos(t1);
        y = top*sin(t1);
        z = height/2;
        p1 = (Vec_3D){x, y, z};
        //p2の計算
        x = bottom*cos(t1);
        y = bottom*sin(t1);
        z = -height/2;
        p2 = (Vec_3D){x, y, z};
        t2 = delta*(i+1);
        //p4の計算
        x = top*cos(t2);
        y = top*sin(t2);
        z = height/2;
        p4 = (Vec_3D){x, y, z};
        //p3の計算
        x = bottom*cos(t2);
        y = bottom*sin(t2);
        z = -height/2;
        p3 = (Vec_3D){x, y, z};
        //法線の計算
        v1 = diffVec(p2, p1);
        v2 = diffVec(p3, p1);
        nv = crossProduct(v1, v2);
        glNormal3d(nv.x, nv.y, nv.z);
        //４頂点に四角形ポリゴンを貼り付け
        glVertex3d(p1.x, p1.y, p1.z);  //円周上の頂点s
        glVertex3d(p2.x, p2.y, p2.z);  //円周上の頂点s
        glVertex3d(p3.x, p3.y, p3.z);  //円周上の頂点s
        glVertex3d(p4.x, p4.y, p4.z);  //円周上の頂点s
    }
    glEnd();
    
    glPopMatrix();
}

void my_drawCylinder(double between,double height,double width){
    double Cylinder_h=10;
    double T_y=(height*Cylinder_h)/2;
 
    glPushMatrix();
    glTranslated(between,T_y, 0.0);
    glScaled(width, height, width);
    glRotated(90,1,0,0);
    glMyCylinder(1.0, 1.0, Cylinder_h, 24);  // 上面の半径，底面の半径，高さ，分割数
    glPopMatrix();
}

void my_drawCylinder_fall(double between,double height,double width,double y){
    double Cylinder_h=10;
    double T_y=(height*Cylinder_h)/2;
    glPushMatrix();
    glTranslated(between,y*Cylinder_h-width, 0.0);
    glRotated(90,0,1,0);
    glMyCylinder(width, width, height, 24);  // 上面の半径，底面の半径，高さ，分割数
    glPopMatrix();
}


void glMySolidCircle(double cx, double cy, double cz, double r, int p)
{
    double x, y, z;  //座標計算結果
    double t, delta;  //パラメータt 及び変位量 delta
    
    delta = 2.0*M_PI/p;
    
    //折れ線近所の開始
    glBegin(GL_POLYGON);
    glNormal3d(0, 0, 1);
    for(int i=0; i<=p; i++){
        t = delta*i;
        x = r*cos(t) + cx;
        y = r*sin(t) + cy;
        z = cz;
        glVertex3d(x, y, z);  //円周上の頂点s
    }
    glEnd();
}

// 色のセット
void setColor(double r, double g, double b, double a)
{
    float col[4] = {(float)r, (float)g, (float)b, (float)a};  //拡散反射係数
    float spe[4] = {1.0, 1.0, 1.0, 1.0};  //鏡面反射係数
    float shi[1] = {10.0};  //ハイライト係数
    
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, col);  //拡散反射
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spe);  //鏡面反射
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shi);  //ハイライト
}



// 鏡面反射のセット
void setSpecular(double r, double g, double b, double a, double n)
{
    float spe[4] = {(float)r, (float)g, (float)b, (float)a};  //鏡面反射係数
    float shi[1] = {(float)n};  //ハイライト係数
    
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spe);  //鏡面反射
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shi);  //ハイライト
}
