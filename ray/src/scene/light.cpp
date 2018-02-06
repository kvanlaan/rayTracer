#include <cmath>
#include <iostream>

#include "light.h"
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>


using namespace std;

double DirectionalLight::distanceAttenuation(const glm::dvec3& P) const
{
    // distance to light is infinite, so f(di) goes to 0.  Return 1.
    return 1.0;
}


glm::dvec3 DirectionalLight::shadowAttenuation(const ray& r, const glm::dvec3& p) const
{
    // YOUR CODE HERE:
    // You should implement shadow-handling code here.

    return glm::dvec3(1.0, 1.0, 1.0);
}

glm::dvec3 DirectionalLight::getColor() const
{
    return color;
}

glm::dvec3 DirectionalLight::getDirection(const glm::dvec3& P) const
{
    return -orientation;
}

double PointLight::distanceAttenuation(const glm::dvec3& P) const
{
    auto distance = std::sqrt(std::pow(position[0] - P[0], 2) +
            std::pow(position[1] - P[1], 2) +
            std::pow(position[2] - P[2], 2));

    //    f(d) = min( 1, 1/( a + b d + c d^2 ) )
    //	float constantTerm;		// a
    //	float linearTerm;		// b
    //	float quadraticTerm;	// c

    // YOUR CODE HERE

    // You'll need to modify this method to attenuate the intensity
    // of the light based on the distance between the source and the
    // point P.  For now, we assume no attenuation and just return 1.0

    return std::min(1.0, 1.0/(constantTerm +
                              (linearTerm * distance) +
                              (quadraticTerm * std::pow(distance, 2))));
    //	return 1.0;
}

glm::dvec3 PointLight::getColor() const
{
    return color;
}

glm::dvec3 PointLight::getDirection(const glm::dvec3& P) const
{
    return glm::normalize(position - P);
}


glm::dvec3 PointLight::shadowAttenuation(const ray& r, const glm::dvec3& p) const
{
    // YOUR CODE HERE:
    // You should implement shadow-handling code here.
//    auto distance = std::sqrt(std::pow(position[0] - p[0], 2) +
//            std::pow(position[1] - p[1], 2) +
//            std::pow(position[2] - p[2], 2));

//    if(p.getMaterial()._trans) {
//         return glm::dvec3(0,0,0);
//    } else {

//    }

//     return glm::dvec3(1.0);
        return glm::dvec3(1,1,1);
}

#define VERBOSE 0

