#pragma once
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif
#include <vector>
#include <fstream>
struct Color
{
    double r;
    double g;
    double b;
};
struct CubeCell{
    int gx;
    int gy;
    int gz;
    Color color;
    bool operator==(const CubeCell& other) const
    {
        return gx == other.gx &&
               gy == other.gy &&
               gz == other.gz;
    }
};


class CubeDispenser{
    private:
        std::vector<CubeCell> placedCubes;
        Color currColor;
    public:
        CubeDispenser(Color color);
        void AddCube(const CubeCell& cell);
        void RemoveCube(const CubeCell& cell);
        const std::vector<CubeCell>& GetPlacedCubes() const;
        //現在の色の処理
        const Color& GetCurrCubeColor() const;
        void ChangeCurrColor(Color color);
        //ファイルの処理
        bool SaveToFile(const std::string& filename) const;
        bool LoadFromFile(const std::string& filename);
        
};