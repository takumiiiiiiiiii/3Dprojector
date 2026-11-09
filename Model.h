#pragma once

#include <string>
#include <vector>
#include <map>
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
    float nx;
    float ny;
    float nz;
    float u;
    float v;
};

// 三角形のインデックス情報
struct Face
{
    unsigned int index0;
    unsigned int index1;
    unsigned int index2;
};

// メッシュ内で同じマテリアルを使う面のまとまり
struct MeshPart
{
    unsigned int startFace;
    unsigned int faceCount;
    unsigned int materialIndex;
};

// マテリアル情報
struct MaterialInfo
{
    GLuint textureId;
    bool hasTexture;
    float diffuse[4];
};

struct ModelBounds
{
    float minX;
    float minY;
    float minZ;
    float maxX;
    float maxY;
    float maxZ;
};

class Model
{
public:
    Model();
    ~Model();

    // モデルを読み込む
    bool Load(const std::string& filepath);
    // モデルを描画する
    void Draw();
    bool IsLoaded() const;
    bool GetLocalBounds(ModelBounds& bounds) const;
    void SetTextureCoordinateFlip(bool flipU, bool flipV);

private:
    // Assimp関連
    Assimp::Importer importer;
    const aiScene* scene;

    // OpenGLで描画しやすいように取り出したデータ
    std::vector<Vertex> vertices;
    std::vector<Face> faces;
    std::vector<MeshPart> meshParts;
    std::vector<MaterialInfo> materials;
    std::map<std::string, GLuint> textureCache;
    std::string modelDirectory;
    ModelBounds localBounds;
    bool hasLocalBounds;
    bool flipTextureU;
    bool flipTextureV;

    // ノードを再帰的に処理する
    void ProcessNode(aiNode* node, const aiScene* scene, const aiMatrix4x4& parentTransform);
    
    // メッシュを処理する
    void ProcessMesh(aiMesh* mesh, const aiMatrix4x4& transform);

    // マテリアルとテクスチャを処理する
    void ProcessMaterials(const aiScene* scene);
    bool LoadTexture(const std::string& filepath, GLuint& textureId);
    std::string ResolveTexturePath(const std::string& texturePath) const;
};
