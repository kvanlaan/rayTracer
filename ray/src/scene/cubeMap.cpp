#include "cubeMap.h"
#include "ray.h"
#include "../ui/TraceUI.h"
#include "../scene/material.h"
extern TraceUI* traceUI;

glm::dvec3 CubeMap::getColor(ray r) const
{
    // YOUR CODE HERE
    // FIXME: Implement Cube Map here
    int faceIndex;
    float u;
    float v;
    glm::dvec3 rayDir = r.getDirection();
    double x = -rayDir[0];
    double y = rayDir[1];
    double z = rayDir[2];
    float maximum, uc, vc;
    int xPositive = x > 0 ? 1 : 0;
    int yPositive = y > 0 ? 1 : 0;
    int zPositive = z > 0 ? 1 : 0;


    float absoluteX = std::abs(x);
    float absoluteY = std::abs(y);
    float absoluteZ = std::abs(z);


    // if x is positive
    if (xPositive && absoluteX >= absoluteY && absoluteX >= absoluteZ) {
        faceIndex = 0;
        maximum = absoluteX;
        uc = -z;
        vc = y;
    }
    // x is negative
    if (!xPositive && absoluteX >= absoluteY && absoluteX >= absoluteZ) {
        faceIndex = 1;
        maximum = absoluteX;
        uc = z;
        vc = y;
    }
    // if y is positive
    if (yPositive && absoluteY >= absoluteX && absoluteY >= absoluteZ) {
        faceIndex = 2;
        maximum = absoluteY;
        uc = x;
        vc = -z;
    }
    // if y is negative
    if (!yPositive && absoluteY >= absoluteX && absoluteY >= absoluteZ) {
        faceIndex = 3;
        maximum = absoluteY;
        uc = x;
        vc = z;
    }

    // if z is positive
    if (zPositive && absoluteZ >= absoluteX && absoluteZ >= absoluteY) {
        faceIndex = 4;
        maximum = absoluteZ;
        uc = x;
        vc = y;
    }
    // if z is negative
    if (!zPositive && absoluteZ >= absoluteX && absoluteZ >= absoluteY) {
        faceIndex = 5;
        maximum = absoluteZ;
        uc = -x;
        vc = y;
    }

    u = 0.5f * (uc / maximum + 1.0f);
    v = 0.5f * (vc / maximum + 1.0f);
    glm::dvec2 uvCoords =  glm::dvec2(u, v);
    auto textureMap = tMap[faceIndex].get();
    glm::dvec3 colorC = textureMap->getMappedValue(uvCoords);
    return colorC;
}


CubeMap::CubeMap()
{
}

CubeMap::~CubeMap()
{
}

void CubeMap::setNthMap(int n, TextureMap* m)
{
    if (m != tMap[n].get())
        tMap[n].reset(m);
}
