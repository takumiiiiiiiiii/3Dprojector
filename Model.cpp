#include "Model.h"
#include <iostream>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

Model::Model(){
    scene = nullptr;
}

bool Model::Load(const std::string& filepath){
    scene = importer.ReadFile(filepath,aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
    if(!scene||scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
        std::cerr << "Assimp error:"<<importer.GetErrorString() << std::endl;
        return false;
    }
    std::cout << "モデル読み込み成功: " << filepath << std::endl;
    std::cout << "メッシュ数: " << scene->mNumMeshes << std::endl;
    std::cout << "マテリアル数: " << scene->mNumMaterials << std::endl;
    std::cout << "アニメーション数: " << scene->mNumAnimations << std::endl;

    // 以前のデータを削除
    vertices.clear();
    faces.clear();

    // ルートノードから順番にメッシュを取得
    ProcessNode(scene->mRootNode, scene);

    std::cout << "頂点数: " << vertices.size() << std::endl;
    std::cout << "面数: " << faces.size() << std::endl;
    return true;
}

void Model::ProcessNode(aiNode* node, const aiScene* scene)
{
    // このノードが持っているメッシュを処理
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        unsigned int meshIndex = node->mMeshes[i];
        aiMesh* mesh = scene->mMeshes[meshIndex];

        ProcessMesh(mesh);
    }

    // 子ノードも処理
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene);
    }
}

void Model::ProcessMesh(aiMesh* mesh)
{
    // 現在の頂点数を記録しておく
    // 複数メッシュがある場合、インデックスをずらすために使う
    unsigned int baseIndex = static_cast<unsigned int>(vertices.size());

    // 頂点を取り出す
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;

        vertex.x = mesh->mVertices[i].x;
        vertex.y = mesh->mVertices[i].y;
        vertex.z = mesh->mVertices[i].z;

        vertices.push_back(vertex);
    }

    // 面を取り出す
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace ai_face = mesh->mFaces[i];

        // aiProcess_Triangulateを指定しているので、基本的に3点の三角形になる
        if (ai_face.mNumIndices == 3)
        {
            Face face;

            face.index0 = baseIndex + ai_face.mIndices[0];
            face.index1 = baseIndex + ai_face.mIndices[1];
            face.index2 = baseIndex + ai_face.mIndices[2];

            faces.push_back(face);
        }
    }
}

void Model::Draw()
{
    if (vertices.empty() || faces.empty())
    {
        return;
    }

    glBegin(GL_TRIANGLES);

    for (size_t i = 0; i < faces.size(); i++)
    {
        Face face = faces[i];

        Vertex v0 = vertices[face.index0];
        Vertex v1 = vertices[face.index1];
        Vertex v2 = vertices[face.index2];

        glVertex3f(v0.x, v0.y, v0.z);
        glVertex3f(v1.x, v1.y, v1.z);
        glVertex3f(v2.x, v2.y, v2.z);
    }

    glEnd();
}