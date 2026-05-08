#pragma once
//ボクセル
bool IsVoxel(int x, int y, int z);
// ボクセルの初期化
void InitVoxcels();
// ボクセルの面を描画
void DrawFace(float x0, float y0, float z0,
              float x1, float y1, float z1,
              int dir);
// ボクセルオブジェクトの描画
void DrawVoxelObject(int size);