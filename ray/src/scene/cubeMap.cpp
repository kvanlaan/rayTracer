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


    int xPositive = x > 0 ? 1 : 0;
    int yPositive = y > 0 ? 1 : 0;
    int zPositive = z > 0 ? 1 : 0;


    float absoluteX = std::abs(x);
    float absoluteY = std::abs(y);
    float absoluteZ = std::abs(z);

    float maxAxis, uc, vc;

    // POSITIVE X
    if (xPositive && absoluteX >= absoluteY && absoluteX >= absoluteZ) {
        faceIndex = 0;
        maxAxis = absoluteX;
        uc = -z;
        vc = y;
    }
    // NEGATIVE X
    if (!xPositive && absoluteX >= absoluteY && absoluteX >= absoluteZ) {
        faceIndex = 1;
        maxAxis = absoluteX;
        uc = z;
        vc = y;
    }
    // POSITIVE Y
    if (yPositive && absoluteY >= absoluteX && absoluteY >= absoluteZ) {
        faceIndex = 2;
        maxAxis = absoluteY;
        uc = x;
        vc = -z;
    }
    // NEGATIVE Y
    if (!yPositive && absoluteY >= absoluteX && absoluteY >= absoluteZ) {
        faceIndex = 3;
        maxAxis = absoluteY;
        uc = x;
        vc = z;
    }


    // POSITIVE Z
    if (zPositive && absoluteZ >= absoluteX && absoluteZ >= absoluteY) {
        faceIndex = 4;
        maxAxis = absoluteZ;
        uc = x;
        vc = y;
    }
    // NEGATIVE Z
    if (!zPositive && absoluteZ >= absoluteX && absoluteZ >= absoluteY) {
        faceIndex = 5;
        maxAxis = absoluteZ;
        uc = -x;
        vc = y;
    }

    u = 0.5f * (uc / maxAxis + 1.0f);
    v = 0.5f * (vc / maxAxis + 1.0f);
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
