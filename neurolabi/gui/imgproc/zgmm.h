#ifndef ZGMM_H
#define ZGMM_H
#include<vector>

class ZStack;

class ZGMM
{
public:
    ZGMM(){}
    ~ZGMM(){}
    std::vector<double> fit(const ZStack* stack);
    //returns a map indicating probability of each voxel belongs to the foreground
    ZStack* probabilityMap(const ZStack* stack);
    static double g(double x,double delta,double u);

private:

};

#endif // ZGMM_H
