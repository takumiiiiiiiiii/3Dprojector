#include <math.h>
#include "Objects.h"
#include "Globals.h"
#include "Vec3D.h"
#include <GLUT/glut.h>  //OpenGL/GLUT
#include <opencv2/opencv.hpp>  //OpenCV
#include "GLApp.h"
#include "Shape.h"


void draw_Enter(double x,double z){
    glPushMatrix();
        glTranslated(x,0,z);
        GLfloat col[4], spe[4], shi[1];
        my_drawCylinder(between_cy,height_cy,30);
        my_drawCylinder(-between_cy,height_cy,30);
        //キャラクター横面
        double height=200;
        double width=600;
        double x_b=0;
        double y_b=height_cy*10;
        double dis=1;
        glEnable(GL_TEXTURE_2D); //テクスチャ有効化 
        glBindTexture(GL_TEXTURE_2D, 0); //テクスチャ指定(#0)
        glPushMatrix();  //行列一時保存
        col[0] = 1.0; col[1] = 1.0; col[2] = 1.0; col[3] = 1;  //RGBA
        spe[0] = 1.0; spe[1] = 1.0; spe[2] = 1.0; spe[3] = 1.0;
        shi[0] = 50;
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, col);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spe);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shi);
        glTranslated(0.0,y_b-height/2, 10);  //平行移動
        glScaled(width,height, 1.0);  //拡大
        glNormal3d(0.0, 0.0, -1.0);  //法線
        glBegin(GL_QUADS);  //四角形
        // テクスチャ座標を水平反転
        glTexCoord2d(x_b + dis, y_b);       // 反転: 元の(x, y)
        glVertex3d(-0.5, 0.5, 0.0);    // 頂点0
        glTexCoord2d(x_b + dis, y_b + dis); // 反転: 元の(x, y + dis)
        glVertex3d(-0.5, -0.5, 0.0);   // 頂点1
        glTexCoord2d(x_b, y_b + dis);       // 反転: 元の(x + dis, y + dis)
        glVertex3d(0.5, -0.5, 0.0);    // 頂点2
        glTexCoord2d(x_b, y_b);             // 反転: 元の(x + dis, y)
        glVertex3d(0.5, 0.5, 0.0);     // 頂点3

        glEnd();
        glPopMatrix();  //行列復帰
        glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void drawfence(){
    double scale=0.3;
    glPushMatrix();
    glScaled(scale,scale,scale);
    my_drawCylinder(between_cy,height_cy,rad_cy);
    my_drawCylinder(-between_cy,height_cy,rad_cy);
    my_drawCylinder_fall(0,between_cy*2,rad_cy,height_cy);
    glPopMatrix();
}

void drawboxfence(double x,double z){
    
    glPushMatrix();
    glTranslated(x,0,z);
    glScaled(0.7,0.7,0.7);
    double longer=5;
    double  large=(height_cy*2+rad_cy)*longer+height_cy;
    for(int i=-longer;i<=longer;i++){
        glPushMatrix();
        glTranslated((height_cy*2+rad_cy)*i,0,large);
        drawfence();
        glPopMatrix();
    }
    for(int i=-longer;i<=longer;i++){
        glPushMatrix();
        glTranslated((height_cy*2+rad_cy)*i,0,-large);
        drawfence();
        glPopMatrix();
    }
    for(int i=-longer;i<=longer;i++){
        glPushMatrix();
        glRotated(90,0,1,0);
        glTranslated((height_cy*2+rad_cy)*i,0,-large);
        drawfence();
        glPopMatrix();
    }
    for(int i=-longer;i<=longer;i++){
        glPushMatrix();
        glRotated(90,0,1,0);
        glTranslated((height_cy*2+rad_cy)*i,0,large);
        drawfence();
        glPopMatrix();
    }
    glPopMatrix();
}

void drawcirclefence(double x,double z){
    glPushMatrix();
    glTranslated(x,0,z);
        double dis=800;
        for(int i=0;i<=23;i++){
            double angle=i*15;
            glPushMatrix();
                glRotated(angle,0,1,0);
                glTranslated(0,0,dis);
                drawfence();
            glPopMatrix();
        }
        glPushMatrix();
            glTranslated(0,0,dis);
            drawfence();
        glPopMatrix();
    glPopMatrix();
}


void penguin(double x,double z){
    double rad=600;
    //胴体
     setColor(0,0,1,1);
    glPushMatrix();
    glTranslated(0,rad,0);
    glScaled(0.8,1,0.8);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();

    //お腹の色
    setColor(1,1,0,1);
    double stame_z=-200;
    double stame_scaleX=0.6;
    double stame_scaleY=0.7;
    double stame_scaleZ=0.6;

    glPushMatrix();
    glTranslated(0,rad,stame_z);
    glScaled(stame_scaleX,stame_scaleY,stame_scaleZ);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();
    
    //頭
    setColor(0,0,1,1);
    double y_hed=300;
    glPushMatrix();
    glTranslated(0,rad+y_hed,0);
    glScaled(0.6,1,0.5);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();

    //くちばし
    setColor(1,1,0,1);
    double sharp_z=-320;
    double sharp_y=rad+y_hed+300;
    setColor(1,1,0,1);
    glPushMatrix();
    glTranslated(0,sharp_y,sharp_z);
    glScaled(0.4,0.1,0.5);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();

    //目玉
    
    setColor(0,0,0,1);
    double eye_x=100;
    double eye_z=sharp_z+100;
    double eye_y=sharp_y+150;

    double eye_scale=0.1;
    //右
    glPushMatrix();
    glTranslated(eye_x,eye_y,eye_z);
    glScaled(eye_scale,eye_scale,eye_scale);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();
    //左
    glPushMatrix();
    glTranslated(-eye_x,eye_y,eye_z);
    glScaled(eye_scale,eye_scale,eye_scale);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();

    //足
    setColor(1,1,0,1);
    double legdis=100;
    double leg_z=-200;

    glPushMatrix();
    glTranslated(legdis,0,leg_z+legR_anima);
    glScaled(0.3,0.2,0.8);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();

    glPushMatrix();
    glTranslated(-legdis,0,leg_z+legL_anima);
    glScaled(0.3,0.2,0.8);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();

    //腕
    glPushMatrix();
    glTranslated(0,rad,0);
    glScaled(0.8,1,0.8);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();
}


void penguin_animation(){
    double speed=20;
    if(legLR_flag){
        legL_anima+=speed;
        legR_anima-=speed;
    }else{
        legR_anima+=speed;
        legL_anima-=speed;
    }
    if(legL_anima>leg_range){
        legLR_flag=false;
    }else if(legR_anima>leg_range){
        legLR_flag=true;
    }
    pen_rad++;
    pen_moveX+=pen_speed*cos((pen_rad-90)*M_PI/180.0);
    pen_moveZ+=pen_speed*sin((pen_rad-90)*M_PI/180.0);
}

void alligator(){
    double big=600;
    double scale_X = 0.7;
    double scale_Y = 0.2;
    double scale_Z = 0.8;
    double trans_X = 0;
    double trans_Y = (big*scale_Y)/2+50;
    double trans_Z = (big*scale_Z)/2;
    //下顎
    glPushMatrix();
    glTranslated(trans_X,trans_Y,trans_Z);
    setColor(0,1,0,1);
    glScaled(scale_X,scale_Y,scale_Z);
        glutSolidCube(big);
    glPopMatrix();
    //上顎
    double scale_Xup = 0.85;
    double scale_Zup = 0.85;
    double trans_Yup=trans_Y+(big*scale_Y)/2;
    glPushMatrix();
    glTranslated(trans_X,trans_Yup,0);
    glRotated(open_mouth_deg,1,0,0);
    glTranslated(0,0,trans_Z);
    setColor(0,1,0,1);
    glScaled(scale_Xup,scale_Y,scale_Zup);
        glutSolidCube(big);
    glPopMatrix();

    //顔
    double face_X=10;
    double face_Y=10;
    double face_Z=10;
    double trans_zface= -trans_Z;
    double scale_Z_face=scale_Z+0.3;
    glPushMatrix();
    glTranslated(trans_X,trans_Y,trans_zface);
    setColor(0,1,0,1);
    glScaled(scale_X,scale_Y,scale_Z_face);
        glutSolidCube(big);
    glPopMatrix();
    //目
    double eye_scale=0.1;
    double eye_between=150;
    setColor(0,0,0,1);
    double trans_Yeye=trans_Y+100;

    double eye_z=trans_zface+80;
    glPushMatrix();
    glTranslated(eye_between,trans_Yeye,eye_z);
    glScaled(eye_scale,eye_scale,eye_scale);
        glutSolidCube(big);
    glPopMatrix();
    //左め
    glPushMatrix();
    glTranslated(-eye_between,trans_Yeye,eye_z);
    glScaled(eye_scale,eye_scale,eye_scale);
        glutSolidCube(big);
    glPopMatrix();
    //しっぽ
    setColor(0,1,0,1);
    trans_zface= -trans_Z;
    double trans_z_body=trans_Z-700;
    double scale_x_body=0.6;
    double scale_y_body=scale_Y-0.05;
    glPushMatrix();
    glTranslated(trans_X,trans_Y,trans_z_body);
    glScaled(scale_x_body,scale_y_body,scale_Z);
        glutSolidCube(big);
    glPopMatrix();
    //しっぽ
    setColor(0,1,0,1);
    trans_zface= -trans_Z;
    double trans_z_tale=trans_Z-700;
    double scale_x_tale=0.6;
    double scale_y_tale=scale_Y-0.05;
    glPushMatrix();
    glTranslated(trans_X,trans_Y,trans_z_tale);
    glScaled(scale_x_tale,scale_y_tale,scale_Z);
        glutSolidCube(big);
    glPopMatrix();
    //しっぽ2
    setColor(0,1,0,1);
    trans_zface= -trans_Z;
    trans_z_body=trans_Z-800;
    double scale_y_tale2=scale_y_tale-0.05;
    double scale_x_tale2=scale_x_body-0.2;
    glPushMatrix();
    glTranslated(trans_X,trans_Y,trans_z_body);
    glScaled(scale_x_tale2,scale_y_tale2,scale_Z);
        glutSolidCube(big);
    glPopMatrix();
    //しっぽ3
    setColor(0,1,0,1);
    trans_zface= -trans_Z;
    trans_z_body=trans_Z-900;
    double scale_y_tale3=scale_y_tale2-0.05;
    double scale_x_tale3=scale_x_body-0.4;
    glPushMatrix();
    glTranslated(trans_X,trans_Y,trans_z_body);
    glScaled(scale_x_tale3,scale_y_tale3,scale_Z);
        glutSolidCube(big);
    glPopMatrix();

    //前足左
    setColor(1,1,0,1);
    double leg_between=30;
    double trans_X_legL=trans_X+240;
    double trans_Z_legL=trans_Z-400;
    double trans_Y_legL=trans_Y-30;
    double scale_XZ_leg = 0.2;
    double scale_Y_leg = 0.3;
    glPushMatrix();
    glTranslated(trans_X_legL,trans_Y_legL,trans_Z_legL);
    glRotated(-legA_deg,1,0,0);
    glScaled(scale_XZ_leg,scale_Y_leg,scale_XZ_leg);
        glutSolidCube(big);
    glPopMatrix();

    //前足右
    setColor(1,1,0,1);
    // double leg_between=30;
    // double trans_X_legL=trans_X+240;
    // double trans_Z_legL=trans_Z-400;
    // double trans_Y_legL=trans_Y-30;
    // double scale_XZ_leg = 0.2;
    // double scale_Y_leg = 0.3;
    glPushMatrix();
    glTranslated(-trans_X_legL,trans_Y_legL,trans_Z_legL);
    glRotated(legA_deg,1,0,0);
    glScaled(scale_XZ_leg,scale_Y_leg,scale_XZ_leg);
        glutSolidCube(big);
    glPopMatrix();


    //後ろ足右
    double trans_Z_Bleg=trans_Z_legL-300;
    setColor(1,1,0,1);
    glPushMatrix();
    glTranslated(-trans_X_legL,trans_Y_legL,trans_Z_Bleg);
    glRotated(legA_deg,1,0,0);
    glScaled(scale_XZ_leg,scale_Y_leg,scale_XZ_leg);
        glutSolidCube(big);
    glPopMatrix();

     //後ろ足左
    setColor(1,1,0,1);
    glPushMatrix();
    glTranslated(trans_X_legL,trans_Y_legL,trans_Z_Bleg);
    glRotated(-legA_deg,1,0,0);
    glScaled(scale_XZ_leg,scale_Y_leg,scale_XZ_leg);
        glutSolidCube(big);
    glPopMatrix();
}


void alligator_animation(){
    //口の動き
    if(open_mouth_deg<-45){
        open_mouth_flag=false;
    }else if(open_mouth_deg>0.0){
        open_mouth_flag=true;
    }
    if(open_mouth_flag){
        open_mouth_deg-=open_mouth_speed;
    }else{
        open_mouth_deg+=open_mouth_speed;
    }
    //わにの足の動き
    if(legA_deg<-45){
        legA_animaflag=false;
    }else if(legA_deg>45){
        legA_animaflag=true;
    }
    if(legA_animaflag){
        legA_deg-=legA_speed;
    }else{
         legA_deg+=legA_speed;
    }
}



//ダチョウ
void ostrich(){

    //足右

    double big=600;
    double leg_scaleXZ=0.3;
    double leg_scaleY=0.1;
     double legtube_scaleXZ=0.2;
     double legtube_scaleY=10;
    double leg_X=100;
    double leg_Y=big*leg_scaleY/2;
     double legtube_Y=(big*legtube_scaleY)/2;
    double leg_Z=0;
     double legtube_Z=100;
    setColor(1,0,1,1);
    glPushMatrix();
    //アニメーション
    glTranslated(leg_X,leg_Y,leg_Z);
    glTranslated(0,(legtube_scaleY*big)/10,(big*legtube_scaleXZ)/10);
    glRotated(ost_deg,1,0,0);
    glTranslated(0,-(legtube_scaleY*big)/10,-(big*legtube_scaleXZ)/10);
    // glTranslated(leg_X,leg_Y,leg_Z);
    glScaled(leg_scaleXZ,leg_scaleY,leg_scaleXZ);
        glutSolidCube(big);
        glTranslated(0,legtube_Y, legtube_Z);
        glScaled(legtube_scaleXZ,legtube_scaleY,legtube_scaleXZ);
        glutSolidCube(big);
    glPopMatrix();


//足左
   
    glPushMatrix();
    //アニメーション
    glTranslated(-leg_X,leg_Y,leg_Z);
    glTranslated(0,(legtube_scaleY*big)/10,(big*legtube_scaleXZ)/10);
    glRotated(-ost_deg,1,0,0);
    glTranslated(0,-(legtube_scaleY*big)/10,-(big*legtube_scaleXZ)/10);

    glScaled(leg_scaleXZ,leg_scaleY,leg_scaleXZ);
        glutSolidCube(big);
        glTranslated(0,legtube_Y, legtube_Z);
        glScaled(legtube_scaleXZ,legtube_scaleY,legtube_scaleXZ);
        glutSolidCube(big);
    glPopMatrix();

//胴体
    double body_scaleX=0.8;
    double body_scaleZ=1.3;
    double body_scaleY=0.8;
    double body_X=0;
    double body_Y=legtube_Y/10+body_scaleY*big/2;
    double body_Z=0;
    setColor(0,0,0,1);
    glPushMatrix();
    glTranslated(0,body_Y,0);
    glScaled(body_scaleX,body_scaleY,body_scaleZ);
        glutSolidSphere(big/2, 36, 18);
    glPopMatrix();

//首
    double nec_rad=big/10;
    double nec_H=500;
    double nec_Z=-400;
    double nec_Y = body_Y-200;
    setColor(1,1,1,1);
    glPushMatrix();
    glTranslated(0,nec_Y*2,nec_Z);
    // glRotated(open_mouth_deg,1,0,0);
    glTranslated(0,0,nec_H/2);
    glRotated(90+ost_nec_deg,1,0,0);
    glTranslated(0,0,-nec_H/2);
    //首
    glMyCylinder(nec_rad,nec_rad,nec_H,100);
      //頭
      setColor(1,1,1,1);
     glPushMatrix();
      double face_X=0;
      double face_Y=-25;
      double face_Z=-nec_Z-600;
      double face_scaleX=0.4;
      double face_scaleY=0.4;
      double face_scaleZ=0.3;
      glTranslated(face_X,face_Y,face_Z);
      glScaled(face_scaleX,face_scaleY,face_scaleZ);
      glutSolidSphere(big/2, 36, 18);
      glPopMatrix();
     //目
     glPushMatrix();
      setColor(0,0,0,1);
     double eye_scale=0.1;
     double eye_X = 50;
     double eye_Y = -130;
     double eye_Z = face_Z-20;
      glTranslated(eye_X,eye_Y,eye_Z);
      glScaled(eye_scale,eye_scale,eye_scale);
      glutSolidSphere(big/2, 36, 18);
     glPopMatrix();

    //目
     glPushMatrix();
      glTranslated(-eye_X,eye_Y,eye_Z);
      glScaled(eye_scale,eye_scale,eye_scale);
      glutSolidSphere(big/2, 36, 18);
     glPopMatrix();
    //くちばし
     double beak_Y = face_Y-100;
     double beak_Z = face_Z+20;
     double beak_scaleX=0.5;
     double beak_scaleY=0.5;
     double beak_scaleZ=0.1;
     setColor(1,1,0,1);
      glTranslated(0,beak_Y,beak_Z);
      glScaled(beak_scaleX,beak_scaleY,beak_scaleZ);
      glutSolidSphere(big/2, 36, 18);
    glPopMatrix();

    //尻尾
    double tale_scaleX=0.6;
    double tale_scaleZ=1.3;
    double tale_scaleY=0.3;
    double tale_X=0;
    double tale_Y=body_Y;
    double tale_Z=300;
    setColor(1,1,1,1);
    glPushMatrix();
    glTranslated(0,body_Y,tale_Z);
    glRotated(30,1,0,0);
    glScaled(tale_scaleX,tale_scaleY,tale_scaleZ);
        glutSolidSphere(big/2, 36, 18);
    glPopMatrix();

    

}

void ostrich_animation(){
    //わにの足の動き
    if(ost_deg<-45){
        ost_animaflag=false;
    }else if(ost_deg>45){
        ost_animaflag=true;
    }
    if(ost_animaflag){
        ost_deg-=ost_speed;
    }else{
         ost_deg+=ost_speed;
    }
    //首の動き
    printf("%f\n",ost_nec_deg);
    if(ost_nec_deg<0){
        ost_nec_animaflag=false;
    }else if(ost_nec_deg>45){
        ost_nec_animaflag=true;
    }
    if(ost_nec_animaflag){
        ost_nec_deg-=ost_nec_speed;
    }else{
         ost_nec_deg+=ost_nec_speed;
    }

    for(int i=0;i<OSTS;i++){
        ost_x[i]+=ost_speed*cos((ost_degs[i]-90)*M_PI/180.0);
        ost_z[i]+=ost_speed*sin((ost_degs[i]-90)*M_PI/180.0);
        if(ost_x[i]>dis/2||ost_x[i]<-dis/2||
           ost_z[i]>dis/2||ost_z[i]<-dis/2){
            ost_x[i]-=ost_speed*cos((ost_degs[i]-90)*M_PI/180.0);
            ost_z[i]-=ost_speed*sin((ost_degs[i]-90)*M_PI/180.0);
            ost_degs[i]+=94;
        }
    }
}


//猿(ネズミ)
void monkey(){
    double scale_Y=1.3;
     double rad=600;
     double monkey_Y=scale_Y*rad;
    //胴体
    setColor(0.6792453,0.4699181,0.2979708,1);
    glPushMatrix();
    glTranslated(0,monkey_Y,0);
    glScaled(0.8,1.3,0.8);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();

    //お腹の色
    setColor(1,0.774319,0.3915094,1);
    double stame_z=-180;
    double stame_scaleX=0.6;
    double stame_scaleY=1.1;
    double stame_scaleZ=0.6;

    
    glPushMatrix();
    glTranslated(0,monkey_Y,stame_z);
    glScaled(stame_scaleX,stame_scaleY,stame_scaleZ);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();

    
    //頭
    setColor(0.6792453,0.4699181,0.2979708,1);
    double y_hed=rad+1200;
    double hed_scale = 0.6;
    glPushMatrix();
    glTranslated(0,y_hed,0);
    glScaled(hed_scale,hed_scale,hed_scale);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();

    //耳
    setColor(0.6792453,0.4699181,0.2979708,1);
    double ear_x=500;
    double ear_scaleX=hed_scale-0.2;
    double ear_scaleY=hed_scale-0.2;
    double ear_scaleZ=hed_scale-0.4;
    
    glPushMatrix();
    glTranslated(ear_x,y_hed,0);
    glScaled(ear_scaleX,ear_scaleY,ear_scaleZ);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();

    glPushMatrix();
    glTranslated(-ear_x,y_hed,0);
    glScaled(ear_scaleX,ear_scaleY,ear_scaleZ);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();

    //くちばし
    double sharp_z=-320;
    double sharp_y=rad+y_hed+250;

    //目玉
    setColor(0,0,0,1);
    double eye_x=100;
    double eye_z=sharp_z+90;
    double eye_y=y_hed+200;
    double eye_scale=0.08;
    //右
    glPushMatrix();
    glTranslated(eye_x,eye_y,eye_z);
    glScaled(eye_scale,eye_scale,eye_scale);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();
    //左
    glPushMatrix();
    glTranslated(-eye_x,eye_y,eye_z);
    glScaled(eye_scale,eye_scale,eye_scale);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();

    //鼻
    double nose_y=eye_y-50;
    double nose_z=eye_z-100;
    double nose_scale=eye_scale-0.04;
    glPushMatrix();
    glTranslated(0,nose_y,nose_z);
    glScaled(nose_scale,nose_scale,nose_scale);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();

    //足
    setColor(1,1,0,1);
    double legdis=200;
    double leg_z=-200;
    glPushMatrix();
    glTranslated(legdis,monkey_Y*2,0);
    glPushMatrix();
    // glTranslated(legdis,0,leg_z);
        // glTranslated(0,-monkey_Y,0);
        glRotated(mon_leg_deg,1,0,0);
        glScaled(0.3,0.2,0.8);
        glTranslated(0,-monkey_Y*10,0);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();
    glPopMatrix();
    //足
    glPushMatrix();
    glTranslated(-legdis,monkey_Y*2,0);
    glPushMatrix();
        glRotated(-mon_leg_deg,1,0,0);
        glScaled(0.3,0.2,0.8);
        glTranslated(0,-monkey_Y*10,0);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();
    glPopMatrix();
    

    //手
    double hand_scaleX=0.8;
    double hand_scaleY=0.3;
    double hand_scaleZ=0.3;
    double hand_x=500;
    double hand_y=monkey_Y+200;
    double hand_z=0;
    setColor(0.6792453,0.4699181,0.2979708,1);
//腕
    glPushMatrix();
    glRotated(mon_hand_deg,0,1,0);
    glTranslated(hand_x,hand_y,0);
    glRotated(45,0,0,1);
   
    glScaled(hand_scaleX,hand_scaleY,hand_scaleZ);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();
//腕2
    glPushMatrix();
    glRotated(mon_hand_deg,0,1,0);
    glTranslated(-hand_x,hand_y,0);
    glRotated(-45,0,0,1);
    glScaled(hand_scaleX,hand_scaleY,hand_scaleZ);
        glutSolidSphere(rad, 36, 18);
    glPopMatrix();

}

void monkey_animation(){
    //わにの足の動き
    if(mon_hand_deg<-30){
        mon_hand_animaflag=false;
    }else if(mon_hand_deg>30){
        mon_hand_animaflag=true;
    }
    if(mon_hand_animaflag){
        mon_hand_deg-=mon_hand_speed;
    }else{
         mon_hand_deg+=mon_hand_speed;
    }
    
    //足の動き
    if(mon_leg_deg<-90){
        mon_leg_animaflag=false;
    }else if(mon_leg_deg>90){
       mon_leg_animaflag=true;
    }
    if(mon_leg_animaflag){
        mon_leg_deg-=mon_leg_speed;
    }else{
       mon_leg_deg+=mon_leg_speed;
    }

    mon_rad+=mon_bigrad;
    mon_moveX+=mon_speed*cos((mon_rad-90)*M_PI/180.0);
    mon_moveZ+=mon_speed*sin((mon_rad-90)*M_PI/180.0);
}


void drawwall(double x,double zx,double len){

    double z=0;
    double dot = 1; // 並の細かさ
    double deg = 0;
    double y =600;
    double wave_long = 50; // 波の大きさ
    double width = 400; // 厚み
    setColor(1, 1, 1, 1); // 平面の色を設定
    glBegin(GL_QUADS); // 四角形の描画開始

    // 前の辺の頂点
    for (int i = 0; i < len; i++) {
        deg++;
        x++;
        z = wave_long * cos(deg * M_PI / 180.0)+zx;
        double nextx=x+1;
        double nextz=wave_long * cos((deg+1) * M_PI / 180.0)+zx;
    //上面の描画
        // glVertex3d(x, 300, z + width); // 下辺の頂点
        // Vec_3D xyz={x, 300, z};
        // Vec_3D xyz_NB={x, 300, nextz+width};
        // Vec_3D xyz_BN={nextx, 300, nextz};


        // Vec_3D v1, v2, n;
        // v1 = diffVec(fPoint[i][j+1], fPoint[i][j]);
        // v2 = diffVec(fPoint[i+1][j], fPoint[i][j]);
        // n = crossProduct(v1, v2);  //法線ベクトル
        // glNormal3d(n.x, n.y, n.z);  //法線ベクトル適用

        glVertex3d(x, y, z); // 上辺の頂点
        glVertex3d(nextx, y, nextz); // 上辺の頂点
        glVertex3d(nextx, y, nextz+width); // 上辺の頂点
        glVertex3d(x, y, z+width); // 上辺の頂点
        //壁
        glVertex3d(x, y, z); // 上辺の頂点
        glVertex3d(nextx, y, nextz); // 上辺の頂点
        glVertex3d(nextx, 0, nextz); // 上辺の頂点
        glVertex3d(x,0, z); // 上辺の頂点
        //壁２
        glVertex3d(x, y, z+width); // 上辺の頂点
        glVertex3d(nextx, y, nextz+width); // 上辺の頂点
        glVertex3d(nextx, 0, nextz+width); // 上辺の頂点
        glVertex3d(x,0, z+width); // 上辺の頂点
    }
    glEnd(); // 描画終了

}

void human(double x,double z){
    double height=30;
    double width=40;
    double width_hed=width+20;
    glPushMatrix();
    glTranslated(x,0,z);
    my_drawCylinder(0,height,width);
    glPopMatrix();

    glPushMatrix();
    glTranslated(x,height*10,z);
    glutSolidSphere(width_hed, 36, 18);
    glPopMatrix();
    
}
void tables(){
    double width=50;
    double height=20;
    double table_big=20;
    my_drawCylinder(0,height,width);
    glPushMatrix();
    glTranslated(0,height*10,0);
    my_drawCylinder(0,1,200);
    glPopMatrix();

}