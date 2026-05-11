#pragma once

#include <string>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

// 頂点情報
struct Vertex
{
    float x;
    float y;
    float z;
};

// 三角形のインデックス情報
struct Face
{
    unsigned int index0;
    unsigned int index1;
    unsigned int index2;
};

class Model
{
public:
    Model();

    // モデルを読み込む
    bool Load(const std::string& filepath);

    // モデルを描画する
    void Draw();

private:
    // Assimp関連
    Assimp::Importer importer;
    const aiScene* scene;

    // OpenGLで描画しやすいように取り出したデータ
    std::vector<Vertex> vertices;
    std::vector<Face> faces;

    // ノードを再帰的に処理する
    void ProcessNode(aiNode* node, const aiScene* scene);

    // メッシュを処理する
    void ProcessMesh(aiMesh* mesh);
};