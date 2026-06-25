#include "CubeDispenser.h"
    CubeDispenser::CubeDispenser(Color color) {
        currColor = color;
    }
    void CubeDispenser::AddCube(const CubeCell& cell){
        placedCubes.push_back(cell);
    }
    void CubeDispenser::RemoveCube(const CubeCell& cell){
        placedCubes.erase(std::remove(placedCubes.begin(), placedCubes.end(), cell), placedCubes.end());
    }
    const std::vector<CubeCell>& CubeDispenser::GetPlacedCubes() const{
            return placedCubes;
    }
    const Color& CubeDispenser::GetCurrCubeColor() const{
            return currColor;
    }
    void CubeDispenser::ChangeCurrColor(Color color){
        currColor = color;
    }
    bool CubeDispenser::SaveToFile(const std::string& filename) const{
        std::ofstream file(filename);
        for (const auto& p : placedCubes)
        {
            file << p.gx << " "
                << p.gy << " "
                << p.gz << " "
                << p.color.r << " "
                << p.color.g << " "
                << p.color.b << "\n";
        }
        file.close();
        return true;
    }
    bool CubeDispenser::LoadFromFile(const std::string& filename){
        placedCubes.clear();
        std::ifstream file(filename);
        if(!file.is_open()){
            return false;
        }
        CubeCell p;
        while (file >> p.gx >> p.gy >> p.gz >> p.color.r >> p.color.g >> p.color.b)
        {
            placedCubes.push_back(p);
        }
        file.close();
        return true;
    }
