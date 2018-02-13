// The main ray tracer.

#pragma warning (disable: 4786)

#include "RayTracer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"
#include "scene/scene.h"

#include "parser/Tokenizer.h"
#include "parser/Parser.h"

#include "ui/TraceUI.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>
#include <string.h> // for memset

#include <iostream>
#include <fstream>

using namespace std;
extern TraceUI* traceUI;

// Use this variable to decide if you want to print out
// debugging messages.  Gets set in the "trace single ray" mode
// in TraceGLWindow, for example.
bool debugMode = false;

// Trace a top-level ray through pixel(i,j), i.e. normalized window coordinates (x,y),
// through the projection plane, and out into the scene.  All we do is
// enter the main ray-tracing method, getting things started by plugging
// in an initial ray weight of (0.0,0.0,0.0) and an initial recursion depth of 0.

glm::dvec3 RayTracer::trace(double x, double y)
{
    // Clear out the ray cache in the scene for debugging purposes,
    if (TraceUI::m_debug)
        scene->intersectCache.clear();

    ray r(glm::dvec3(0,0,0), glm::dvec3(0,0,0), glm::dvec3(1,1,1), ray::VISIBILITY);
    scene->getCamera().rayThrough(x,y,r);
    double dummy;
    // step which will be incremented with each recursion, checked against max-kvl
    int depth = 0;
    glm::dvec3 ret = traceRay(r, glm::dvec3(1.0,1.0,1.0), depth, dummy);
    ret = glm::clamp(ret, 0.0, 1.0);
    return ret;
}


glm::dvec3 RayTracer::tracePixel(int i, int j)
{
    glm::dvec3 col(0,0,0);

    if( ! sceneLoaded() )
        return col;

    if(traceUI->aaSwitch())
    {
        auto thresh = samples;

        unsigned char *pixel = buffer.data() + ( i + j * buffer_width ) * 3;

        std::vector<glm::dvec3> colors;
        double x = double(i)/double(buffer_width);
        double y = double(j)/double(buffer_height);

        for(auto pix = 0; pix <= thresh; ++pix)
        {
            auto new_i = i - pix;
            auto new_j = j - pix;
            auto new_p = i + pix;
            auto new_q = j + pix;

            double new_x = double(new_i)/double(buffer_width);
            double new_y = double(new_j)/double(buffer_height);
            double new_w = double(new_p)/double(buffer_width);
            double new_z = double(new_q)/double(buffer_height);
            if(new_x < 0)
                new_x = 0;
            if(new_y < 0)
                new_y  = 0;
            if(new_w > 1)
                new_w = 1;
            if(new_z > 1)
                new_z = 1;
            colors.push_back(trace(x, new_y));
            colors.push_back(trace(new_x, y));
            colors.push_back(trace(new_x, new_y));
        }

        colors.push_back(trace(x, y));
        std::vector<double> r, g, b;
        for(const auto &color : colors)
        {
            r.push_back(color[0]);
            g.push_back(color[1]);
            b.push_back(color[2]);
        }
        auto r_val = std::accumulate(r.begin(), r.end(), 0.0)/r.size();
        auto g_val = std::accumulate(g.begin(), g.end(), 0.0)/g.size();
        auto b_val = std::accumulate(b.begin(), b.end(), 0.0)/b.size();
        col = glm::dvec3(r_val, g_val, b_val);
        pixel[0] = (int) (255.0 * r_val);
        pixel[1] = (int) (255.0 * g_val);
        pixel[2] = (int) (255.0 * b_val);
        return col;
    }
    else
    {
        double x = double(i)/double(buffer_width);
        double y = double(j)/double(buffer_height);

        unsigned char *pixel = buffer.data() + ( i + j * buffer_width ) * 3;
        col = trace(x, y);

        pixel[0] = (int)( 255.0 * col[0]);
        pixel[1] = (int)( 255.0 * col[1]);
        pixel[2] = (int)( 255.0 * col[2]);
        return col;
    }
}

#define VERBOSE 0

// Do recursive ray tracing!  You'll want to insert a lot of code here
// (or places called from here) to handle reflection, refraction, etc etc.
glm::dvec3 RayTracer::traceRay(ray& r, const glm::dvec3& thresh, int depth, double& t )
{
    isect i;
    glm::dvec3 colorC;
#if VERBOSE
    std::cerr << "== current depth: " << depth << std::endl;
#endif

    if(scene->intersect(r, i)) {
        // YOUR CODE HERE

        // An intersection occurred!  We've got work to do.  For now,
        // this code gets the material for the surface that was intersected,
        // and asks that material to provide a color for the ray.

        // This is a great place to insert code for recursive ray tracing.
        // Instead of just returning the result of shade(), add some
        // more steps: add in the contributions from reflected and refracted
        // rays.
        auto n = i.getN();

        const Material& m = i.getMaterial();
        colorC = m.shade(scene.get(), r, i);

        if(depth <= traceUI->getDepth()) {
            if(m.Refl()) {
                glm::dvec3 l = r.getDirection();
                glm::dvec3 reflectRayDirection  = ((2*(glm::dot(n, l))*n) - l);
                glm::dvec3 reflectP = r.at(i.getT() - 0.0000000000000000000000000000000000000001);
                ray reflectRay(reflectP, reflectRayDirection,  glm::dvec3(0,0,0), ray::REFLECTION);
                double dummy;
                colorC = colorC + (m.kr(i)*traceRay(reflectRay, glm::dvec3(1.0,1.0,1.0), depth+1, dummy));
            }
        }

        if(m.Trans()&& depth <= traceUI->getDepth()) {

            glm::dvec3 refracP = r.at(i.getT() - 0.0000000000000000000000000000000000000001);
            auto refracIndex = m.index(i);

            auto viewDir = -r.getDirection();
            auto cosInLight = glm::dot(n, viewDir);
            auto inLightAngle = glm::acos(cosInLight);
            // I tried to use the formula for cos of refrac angle in the useful equations sheet, but it gave a bad result
            // auto cosRefracAngle = sqrt(1-(std::pow(refracIndex, 2)*(1-(std::pow(cosInLight, 2)))));

            // I used my own method in the end based on my understanding of the unit circle
            auto cosRefracAngle = glm::cos(-inLightAngle);
            glm::dvec3 refracRayDirection = (((refracIndex*(cosInLight)) - cosRefracAngle)*n) - (refracIndex*(viewDir));
            refracRayDirection = glm::normalize(refracRayDirection);
            ray refracRay(refracP, refracRayDirection, glm::dvec3(0,0,0), ray::REFRACTION);
            double dummy;
            colorC = colorC + (m.kt(i)*traceRay(refracRay, glm::dvec3(1.0,1.0,1.0), depth+1, dummy));
        }
    } else {
        // No intersection.  This ray travels to infinity, so we color
        // it according to the background color, which in this (simple) case
        // is just black.
        //
        // FIXME: Add CubeMap support here.
        // TIPS: CubeMap object can be fetched from traceUI->getCubeMap();
        //       Check traceUI->cubeMap() to see if cubeMap is loaded
        //       and enabled.

        colorC = glm::dvec3(0.0, 0.0, 0.0);
    }
#if VERBOSE
    std::cerr << "== depth: " << depth+1 << " done, returning: " << colorC << std::endl;
#endif
    return colorC;
}

RayTracer::RayTracer()
    : scene(nullptr), buffer(0), thresh(0), buffer_width(256), buffer_height(256), m_bBufferReady(false)
{
}

RayTracer::~RayTracer()
{
}

void RayTracer::getBuffer( unsigned char *&buf, int &w, int &h )
{
    buf = buffer.data();
    w = buffer_width;
    h = buffer_height;
}

double RayTracer::aspectRatio()
{
    return sceneLoaded() ? scene->getCamera().getAspectRatio() : 1;
}

bool RayTracer::loadScene(const char* fn)
{
    ifstream ifs(fn);
    if( !ifs ) {
        string msg( "Error: couldn't read scene file " );
        msg.append( fn );
        traceUI->alert( msg );
        return false;
    }

    // Strip off filename, leaving only the path:
    string path( fn );
    if (path.find_last_of( "\\/" ) == string::npos)
        path = ".";
    else
        path = path.substr(0, path.find_last_of( "\\/" ));

    // Call this with 'true' for debug output from the tokenizer
    Tokenizer tokenizer( ifs, false );
    Parser parser( tokenizer, path );
    try {
        scene.reset(parser.parseScene());
    }
    catch( SyntaxErrorException& pe ) {
        traceUI->alert( pe.formattedMessage() );
        return false;
    } catch( ParserException& pe ) {
        string msg( "Parser: fatal exception " );
        msg.append( pe.message() );
        traceUI->alert( msg );
        return false;
    } catch( TextureMapException e ) {
        string msg( "Texture mapping exception: " );
        msg.append( e.message() );
        traceUI->alert( msg );
        return false;
    }

    if (!sceneLoaded())
        return false;

    return true;
}

//recursive function to search out and add octree nodes
void RayTracer::addOctnode(Octnode* node)
{
    auto box_min = node->boundingBox.getMin();
    auto box_max = node->boundingBox.getMax();

    auto x_min = box_min[0];
    auto x_max = box_max[0];
    auto y_min = box_min[1];
    auto y_max = box_max[1];
    auto z_min = box_min[2];
    auto z_max = box_max[2];

    auto x_half = (x_max - x_min)/2;
    auto y_half = (y_max - y_min)/2;
    auto z_half = (z_max - z_min)/2;

    if(x_half > 0.5 && y_half > 0.5  && z_half > 0.5)
    {
        node->children.push_back(Octnode(BoundingBox(glm::dvec3(x_min, y_min, z_min), glm::dvec3(x_half, y_half, z_half))));
        node->children.push_back(Octnode(BoundingBox(glm::dvec3(x_min, y_min, z_half), glm::dvec3(x_half, y_half, z_max))));
        node->children.push_back(Octnode(BoundingBox(glm::dvec3(x_min, y_half, z_min), glm::dvec3(x_half, y_max, z_half))));
        node->children.push_back(Octnode(BoundingBox(glm::dvec3(x_min, y_half, z_half), glm::dvec3(x_half, y_max, z_max))));

        node->children.push_back(Octnode(BoundingBox(glm::dvec3(x_half, y_min, z_min), glm::dvec3(x_max, y_half, z_half))));
        node->children.push_back(Octnode(BoundingBox(glm::dvec3(x_half, y_min, z_half), glm::dvec3(x_max, y_half, z_max))));
        node->children.push_back(Octnode(BoundingBox(glm::dvec3(x_half, y_half, z_min), glm::dvec3(x_max, y_max, z_half))));
        node->children.push_back(Octnode(BoundingBox(glm::dvec3(x_half, y_half, z_half), glm::dvec3(x_max, y_max, z_max))));
    }
}

void RayTracer::RecurseOctree(Octnode* node, const ray& r, double& tMin, double& tMax)
{
    if(node->boundingBox.intersect(r, tMin, tMax))
    {
        if(!node->children.empty())
            for(auto child_node : node->children)
                RecurseOctree(&child_node, r, tMin, tMax);
        else
        {
            //case 1: we haven't added the children for this node yet
            if(node->boundingBox.volume() != 1)
            {

            }
            //case 2: the smallest possible box
            else
            {
                //actually raytrace?
            }
        }

    }
}

void RayTracer::createOctree()
{
    sceneBox = scene->bounds();
    octreeRoot = Octnode(sceneBox);
    addOctnode(&octreeRoot);
}

void RayTracer::traceSetup(int w, int h)
{
    if (buffer_width != w || buffer_height != h)
    {
        buffer_width = w;
        buffer_height = h;
        bufferSize = buffer_width * buffer_height * 3;
        buffer.resize(bufferSize);
    }
    std::fill(buffer.begin(), buffer.end(), 0);
    m_bBufferReady = true;

    /*
     * Sync with TraceUI
     */

    threads = traceUI->getThreads();
    block_size = traceUI->getBlockSize();
    thresh = traceUI->getThreshold();
    samples = traceUI->getSuperSamples();
    aaThresh = traceUI->getAaThreshold();

    // YOUR CODE HERE
    // FIXME: Additional initializations
    createOctree();
}

/*
 * RayTracer::traceImage
 *
 *	Trace the image and store the pixel data in RayTracer::buffer.
 *
 *	Arguments:
 *		w:	width of the image buffer
 *		h:	height of the image buffer
 *
 */
void RayTracer::traceImage(int w, int h)
{
    // Always call traceSetup before rendering anything.
    traceSetup(w,h);

    width = w;
    height = h;

    for(int x  = 0; x < w; ++x)
    {
        for(int y = 0; y < h; ++y)
        {
            tracePixel(x, y);
        }
    }

    // YOUR CODE HERE
    // FIXME: Start one or more threads for ray tracing
    //
    // TIPS: Ideally, the traceImage should be executed asynchronously,
    //       i.e. returns IMMEDIATELY after working threads are launched.
    //
    //       An asynchronous traceImage lets the GUI update your results
    //       while rendering.
}

//anti-aliasing for 1, 4, 9, or 16 samples
int RayTracer::aaImage()
{
    return 0;

}

bool RayTracer::checkRender()
{
    // YOUR CODE HERE
    // FIXME: Return true if tracing is done.
    //        This is a helper routine for GUI.
    //
    // TIPS: Introduce an array to track the status of each worker thread.
    //       This array is maintained by the worker threads.
}

void RayTracer::waitRender()
{
    // YOUR CODE HERE
    // FIXME: Wait until the rendering process is done.
    //        This function is essential if you are using an asynchronous
    //        traceImage implementation.
    //
    // TIPS: Join all worker threads here.
}


glm::dvec3 RayTracer::getPixel(int i, int j)
{
    unsigned char *pixel = buffer.data() + ( i + j * buffer_width ) * 3;
    return glm::dvec3((double)pixel[0]/255.0, (double)pixel[1]/255.0, (double)pixel[2]/255.0);
}

void RayTracer::setPixel(int i, int j, glm::dvec3 color)
{
    unsigned char *pixel = buffer.data() + ( i + j * buffer_width ) * 3;

    pixel[0] = (int)( 255.0 * color[0]);
    pixel[1] = (int)( 255.0 * color[1]);
    pixel[2] = (int)( 255.0 * color[2]);
}
