#include "Model.h"
#include <iostream>
#include <opencv2/opencv.hpp>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

Model::Model(){
    scene = nullptr;
}

Model::~Model()
{
    for (std::map<std::string, GLuint>::iterator it = textureCache.begin(); it != textureCache.end(); ++it)
    {
        if (it->second != 0)
        {
            glDeleteTextures(1, &it->second);
        }
    }
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
    meshParts.clear();
    materials.clear();
    for (std::map<std::string, GLuint>::iterator it = textureCache.begin(); it != textureCache.end(); ++it)
    {
        if (it->second != 0)
        {
            glDeleteTextures(1, &it->second);
        }
    }
    textureCache.clear();

    size_t slashPos = filepath.find_last_of("/\\");
    if (slashPos == std::string::npos)
    {
        modelDirectory = "";
    }
    else
    {
        modelDirectory = filepath.substr(0, slashPos + 1);
    }

    // ルートノードから順番にメッシュを取得
    ProcessNode(scene->mRootNode, scene);
    ProcessMaterials(scene);

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
    unsigned int startFace = static_cast<unsigned int>(faces.size());

    // 頂点を取り出す
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;

        vertex.x = mesh->mVertices[i].x;
        vertex.y = mesh->mVertices[i].y;
        vertex.z = mesh->mVertices[i].z;

        if (mesh->HasNormals())
        {
            vertex.nx = mesh->mNormals[i].x;
            vertex.ny = mesh->mNormals[i].y;
            vertex.nz = mesh->mNormals[i].z;
        }
        else
        {
            vertex.nx = 0.0f;
            vertex.ny = 1.0f;
            vertex.nz = 0.0f;
        }

        if (mesh->HasTextureCoords(0))
        {
            vertex.u = mesh->mTextureCoords[0][i].x;
            vertex.v = mesh->mTextureCoords[0][i].y;
        }
        else
        {
            vertex.u = 0.0f;
            vertex.v = 0.0f;
        }

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

    MeshPart part;
    part.startFace = startFace;
    part.faceCount = static_cast<unsigned int>(faces.size()) - startFace;
    part.materialIndex = mesh->mMaterialIndex;
    meshParts.push_back(part);
}

void Model::ProcessMaterials(const aiScene* scene)
{
    materials.resize(scene->mNumMaterials);

    for (unsigned int i = 0; i < scene->mNumMaterials; i++)
    {
        aiMaterial* ai_material = scene->mMaterials[i];
        MaterialInfo material;
        material.textureId = 0;
        material.hasTexture = false;
        material.diffuse[0] = 1.0f;
        material.diffuse[1] = 1.0f;
        material.diffuse[2] = 1.0f;
        material.diffuse[3] = 1.0f;

        aiColor4D diffuse;
        if (AI_SUCCESS == aiGetMaterialColor(ai_material, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
        {
            material.diffuse[0] = diffuse.r;
            material.diffuse[1] = diffuse.g;
            material.diffuse[2] = diffuse.b;
            material.diffuse[3] = diffuse.a;
        }

        if (ai_material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
        {
            aiString texturePath;
            if (AI_SUCCESS == ai_material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath))
            {
                std::string resolvedPath = ResolveTexturePath(texturePath.C_Str());
                std::map<std::string, GLuint>::iterator cached = textureCache.find(resolvedPath);

                if (cached != textureCache.end())
                {
                    material.textureId = cached->second;
                    material.hasTexture = true;
                }
                else
                {
                    GLuint textureId = 0;
                    if (LoadTexture(resolvedPath, textureId))
                    {
                        textureCache[resolvedPath] = textureId;
                        material.textureId = textureId;
                        material.hasTexture = true;
                    }
                }
            }
        }

        materials[i] = material;
    }
}

bool Model::LoadTexture(const std::string& filepath, GLuint& textureId)
{
    cv::Mat textureImage = cv::imread(filepath, cv::IMREAD_UNCHANGED);
    if (textureImage.empty())
    {
        std::cout << "テクスチャ画像を読み込めませんでした: " << filepath << std::endl;
        return false;
    }

    GLenum format = GL_RGB;
    if (textureImage.channels() == 4)
    {
        cv::cvtColor(textureImage, textureImage, cv::COLOR_BGRA2RGBA);
        format = GL_RGBA;
    }
    else if (textureImage.channels() == 3)
    {
        cv::cvtColor(textureImage, textureImage, cv::COLOR_BGR2RGB);
        format = GL_RGB;
    }
    else
    {
        cv::cvtColor(textureImage, textureImage, cv::COLOR_GRAY2RGB);
        format = GL_RGB;
    }

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        format,
        textureImage.cols,
        textureImage.rows,
        0,
        format,
        GL_UNSIGNED_BYTE,
        textureImage.data
    );
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "テクスチャ読み込み成功: " << filepath << std::endl;
    return true;
}

std::string Model::ResolveTexturePath(const std::string& texturePath) const
{
    std::string path = texturePath;
    for (size_t i = 0; i < path.size(); i++)
    {
        if (path[i] == '\\')
        {
            path[i] = '/';
        }
    }

    while (!path.empty() && path[0] == '/')
    {
        path.erase(0, 1);
    }

    if (path.size() >= 2 && path[1] == ':')
    {
        return path;
    }

    return modelDirectory + path;
}

void Model::Draw()
{
    if (vertices.empty() || faces.empty())
    {
        return;
    }

    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    for (size_t partIndex = 0; partIndex < meshParts.size(); partIndex++)
    {
        MeshPart part = meshParts[partIndex];

        if (part.materialIndex < materials.size())
        {
            MaterialInfo material = materials[part.materialIndex];
            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, material.diffuse);

            if (material.hasTexture)
            {
                glBindTexture(GL_TEXTURE_2D, material.textureId);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glBegin(GL_TRIANGLES);

        for (unsigned int i = 0; i < part.faceCount; i++)
        {
            Face face = faces[part.startFace + i];

            Vertex v0 = vertices[face.index0];
            Vertex v1 = vertices[face.index1];
            Vertex v2 = vertices[face.index2];

            glTexCoord2f(v0.u, v0.v);
            glNormal3f(v0.nx, v0.ny, v0.nz);
            glVertex3f(v0.x, v0.y, v0.z);

            glTexCoord2f(v1.u, v1.v);
            glNormal3f(v1.nx, v1.ny, v1.nz);
            glVertex3f(v1.x, v1.y, v1.z);

            glTexCoord2f(v2.u, v2.v);
            glNormal3f(v2.nx, v2.ny, v2.nz);
            glVertex3f(v2.x, v2.y, v2.z);
        }

        glEnd();
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}
