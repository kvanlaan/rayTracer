#include "material.h"
#include "../ui/TraceUI.h"
#include "light.h"
#include "ray.h"
extern TraceUI* traceUI;

#include <glm/gtx/io.hpp>
#include <iostream>
#include <numeric>
#include "../fileio/images.h"

using namespace std;
extern bool debugMode;

Material::~Material()
{
}

// Apply the phong model to this point on the surface of the object, returning
// the color of that point.

glm::dvec3 Material::shade(Scene* scene, const ray& r, const isect& i) const{
    // phong shading
    glm::dvec3 p = r.at(i.getT());
    glm::dvec3 ambientIntensity = scene->ambient();

            auto I = ke(i) + (ka(i) *(ambientIntensity));
            auto n = i.getN();
            for(const auto& light : scene->getAllLights()){
                auto l = light->getDirection(p);
                glm::dvec3 viewDirection = -r.getDirection();
                glm::dvec3 outDirection = (2*(glm::dot(n, l))*n) - l;
              auto atten = light->distanceAttenuation(p) *(light->shadowAttenuation(r, p, i));
              I = I + (atten*((kd(i)*(std::max(glm::dot(l, n), 0.0))) + (ks(i)*(std::pow((std::max(glm::dot(outDirection, viewDirection), 0.0)), shininess(i))))));
            }
            return I;


            // YOUR CODE HERE

            // For now, this method just returns the diffuse color of the object.
            // This gives a single matte color for every distinct surface in the
            // scene, and that's it.  Simple, but enough to get you started.
            // (It's also inconsistent with the phong model...)

            // Your mission is to fill in this method with the rest of the phong
            // shading model, including the contributions of all the light sources.
            // You will need to call both distanceAttenuation() and
            // shadowAttenuation()
            // somewhere in your code in order to compute shadows and light falloff.
            //    if( debugMode )
            //        std::cout << "Debugging Phong code..." << std::endl;

            // When you're iterating through the lights,
            // you'll want to use code that looks something
            // like this:
            //
            // for ( const auto& pLight : scene->getAllLights() )
            // {
            //              // pLight has type unique_ptr<Light>
            //         .
            //         .
            //         .
            // }
//                    return kd(i);
}


TextureMap::TextureMap(string filename)
{
    data = readImage(filename.c_str(), width, height);
    if (data.empty()) {
        width = 0;
        height = 0;
        string error("Unable to load texture map '");
        error.append(filename);
        error.append("'.");
        throw TextureMapException(error);
    }
}

glm::dvec3 TextureMap::getMappedValue(const glm::dvec2& coord) const
{
    // YOUR CODE HERE
    //
    // In order to add texture mapping support to the
    // raytracer, you need to implement this function.
    // What this function should do is convert from
    // parametric space which is the unit square
    // [0, 1] x [0, 1] in 2-space to bitmap coordinates,
    // and use these to perform bilinear interpolation
    // of the values.

    auto x = coord[1];
    auto y = coord[0];

    auto row_val = x * (width - 1);
    auto col_val = y * (height - 1);

    const int x1 = std::floor(row_val);
    const int y1 = std::floor(col_val);
    const int x2 = std::ceil(row_val);
    const int y2 = std::ceil(col_val);

    assert(x1 >= 0 && x1 <= width);
    assert(y1 >= 0 && y1 <= height);
    assert(x2 >= 0 && x2 <= width);
    assert(y2 >= 0 && y2 <= height);

    std::vector<glm::dvec3> color_vec;
    color_vec.push_back(getPixelAt(x1, y1));
    color_vec.push_back(getPixelAt(x1, y2));
    color_vec.push_back(getPixelAt(x2, y1));
    color_vec.push_back(getPixelAt(x2, y2));

    std::vector<float> r, g, b;
    for(auto color : color_vec)
    {
        r.push_back(color[0]);
        g.push_back(color[1]);
        b.push_back(color[2]);
    }

    float r_val = accumulate(r.begin(), r.end(), 0.0)/r.size();
    float g_val = accumulate(g.begin(), g.end(), 0.0)/g.size();
    float b_val = accumulate(b.begin(), b.end(), 0.0)/b.size();

    return glm::dvec3(r_val, g_val, b_val);
//    return glm::dvec3(0, 0, 0);
}

glm::dvec3 TextureMap::getPixelAt(/*const*/ int x, /*const*/ int y) const
{
    // YOUR CODE HERE
    //
    // In order to add texture mapping support to the
    // raytracer, you need to implement this function.

    auto row_len = width * 3;
    auto row_add = (x > 0) ? (x - 1) : 0;
    auto prepend = row_add * row_len;
    auto col = (y > 0) ? (y - 1) : 0;

    // issue here
    double r = data[prepend + (col * 3)]/255.0;
    double g = data[prepend + (col * 3) + 1]/255.0;
    double b = data[prepend + (col * 3) + 2]/255.0;

    return glm::dvec3(r, g, b);
}

glm::dvec3 MaterialParameter::value(const isect& is) const
{
    if (0 != _textureMap)
        return _textureMap->getMappedValue(is.getUVCoordinates());
    else
        return _value;
}

double MaterialParameter::intensityValue(const isect& is) const
{
 // check out intensity here
    if (0 != _textureMap) {
        glm::dvec3 value(
                _textureMap->getMappedValue(is.getUVCoordinates()));
        return (0.299 * value[0]) + (0.587 * value[1]) +
               (0.114 * value[2]);
    } else
        return (0.299 * _value[0]) + (0.587 * _value[1]) +
               (0.114 * _value[2]);
}
