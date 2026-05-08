#include  "Voxcel.h"
#include  "Globals.h"
#include <GLUT/glut.h>  //OpenGL/GLUT

// 範囲内かつボクセルが存在するか
bool IsVoxel(int x, int y, int z)
{
    if (x < 0 || x >= SIZE ||
        y < 0 || y >= SIZE ||
        z < 0 || z >= SIZE)
    {
        return false;
    }

    return voxels[x][y][z];
}

void InitVoxcels()
{
   for (int x = 0; x < SIZE; x++)
    {
        for (int y = 0; y < SIZE; y++)
        {
            for (int z = 0; z < SIZE; z++)
            {
                voxels[x][y][z] = true;
                // if(x<SIZE-y){
                //     voxels[x][y][z] = false;
                // }

            }
        }
    }
}


void DrawFace(float x0, float y0, float z0,
              float x1, float y1, float z1,
              int dir)
{
    glBegin(GL_QUADS);

    switch (dir)
    {
    // +X 面
    case 0:
        glVertex3f(x1, y0, z0);
        glVertex3f(x1, y1, z0);
        glVertex3f(x1, y1, z1);
        glVertex3f(x1, y0, z1);
        break;

    // -X 面
    case 1:
        glVertex3f(x0, y0, z1);
        glVertex3f(x0, y1, z1);
        glVertex3f(x0, y1, z0);
        glVertex3f(x0, y0, z0);
        break;

    // +Y 面
    case 2:
        glVertex3f(x0, y1, z0);
        glVertex3f(x0, y1, z1);
        glVertex3f(x1, y1, z1);
        glVertex3f(x1, y1, z0);
        break;

    // -Y 面
    case 3:
        glVertex3f(x0, y0, z1);
        glVertex3f(x0, y0, z0);
        glVertex3f(x1, y0, z0);
        glVertex3f(x1, y0, z1);
        break;

    // +Z 面
    case 4:
        glVertex3f(x0, y0, z1);
        glVertex3f(x1, y0, z1);
        glVertex3f(x1, y1, z1);
        glVertex3f(x0, y1, z1);
        break;

    // -Z 面
    case 5:
        glVertex3f(x1, y0, z0);
        glVertex3f(x0, y0, z0);
        glVertex3f(x0, y1, z0);
        glVertex3f(x1, y1, z0);
        break;
    }

    glEnd();
}

void DrawVoxelObject(int size)
{
    for (int x = 0; x < size; x++)
    {
        for (int y = 0; y < size; y++)
        {
            for (int z = 0; z < size; z++)
            {
                if (!voxels[x][y][z])
                {
                    continue;
                }

                float x0 = x * SPACING;
                float y0 = y * SPACING;
                float z0 = z * SPACING;

                float x1 = x0 + SPACING;
                float y1 = y0 + SPACING;
                float z1 = z0 + SPACING;

                // 隣にボクセルがなければ、その面は表面
                if (!IsVoxel(x + 1, y, z)) DrawFace(x0, y0, z0, x1, y1, z1, 0);
                if (!IsVoxel(x - 1, y, z)) DrawFace(x0, y0, z0, x1, y1, z1, 1);
                if (!IsVoxel(x, y + 1, z)) DrawFace(x0, y0, z0, x1, y1, z1, 2);
                if (!IsVoxel(x, y - 1, z)) DrawFace(x0, y0, z0, x1, y1, z1, 3);
                if (!IsVoxel(x, y, z + 1)) DrawFace(x0, y0, z0, x1, y1, z1, 4);
                if (!IsVoxel(x, y, z - 1)) DrawFace(x0, y0, z0, x1, y1, z1, 5);
            }
        }
    }
}