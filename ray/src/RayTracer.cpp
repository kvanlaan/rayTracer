// The main ray tracer.

#pragma warning (disable: 4786)

#include "RayTracer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"

#include "parser/Tokenizer.h"
#include "parser/Parser.h"

#include "ui/TraceUI.h"
#include <cmath>
#include <algorithm>
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
    glm::dvec3 ret = traceRay(r, glm::dvec3(1.0,1.0,1.0), traceUI->getDepth(), dummy);
    ret = glm::clamp(ret, 0.0, 1.0);
    return ret;
}

glm::dvec3 RayTracer::tracePixel(int i, int j)
{
    glm::dvec3 col(0,0,0);
    if( ! sceneLoaded() ) return col;

    if(traceUI->aaSwitch()) {

       auto thresh = aaThresh;
        // do antialiasing here
        // ariel's nice method from work

        //            //            for(auto y = altminmax.first; y <= altminmax.second; y += alt_step)
        //            //            {
        //            auto y2 = bounded_index(alts.begin(), alts.end(), y);
        //            if(y2<1)
        //                y2 = 1;
        //            auto y1 = y2-1;

        //            std::vector<float> row;
        //            //                for(auto x = wavminmax.first; x <= wavminmax.second; x += wav_step)
        //            //                {
        //            auto x2 = bounded_index(wavelengths.begin(), wavelengths.end(), x);
        //            if(x2<1)
        //                x2 = 1;
        //            auto x1 = x2-1;

        //            auto q11 = data[y1][x1];
        //            auto q12 = data[y2][x1];
        //            auto q22 = data[y2][x2];
        //            auto q21 = data[y1][x2];
        //            row.push_back(BilinearInterpolation(q11, q12, q21, q22, wavelengths[x1], wavelengths[x2], alts[y1], alts[y2], x, y));
        //            //                }
        //            retval.push_back(row);
    }

    double x = double(i)/double(buffer_width);
    double y = double(j)/double(buffer_height);

    unsigned char *pixel = buffer.data() + ( i + j * buffer_width ) * 3;
    col = trace(x, y);

    pixel[0] = (int)( 255.0 * col[0]);
    pixel[1] = (int)( 255.0 * col[1]);
    pixel[2] = (int)( 255.0 * col[2]);
    return col;
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

        const Material& m = i.getMaterial();
        colorC = m.shade(scene.get(), r, i);
//        auto ptr = scene->beginObjects();
//        while(ptr != scene->endObjects())
//        {
//            auto geo = ptr->get();
//            make a copy of i
//            auto new_i = i;
//            if(geo->intersect(r, new_i))
//            {
//                  traceRay(r);
//            }
//        }
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

double interpolate( vector<double> &xData, vector<double> &yData, double x, bool extrapolate )
{
   int size = xData.size();

   int i = 0;                                                                  // find left end of interval for interpolation
   if ( x >= xData[size - 2] )                                                 // special case: beyond right end
   {
      i = size - 2;
   }
   else
   {
      while ( x > xData[i+1] ) i++;
   }
   double xL = xData[i], yL = yData[i], xR = xData[i+1], yR = yData[i+1];      // points on either side (unless beyond ends)
   if ( !extrapolate )                                                         // if beyond ends of array and not extrapolating
   {
      if ( x < xL ) yR = yL;
      if ( x > xR ) yL = yR;
   }

   double dydx = ( yR - yL ) / ( xR - xL );                                    // gradient

   return yL + dydx * ( x - xL );                                              // linear interpolation
}


/**
 * @brief BilinearInterpolation calculates a bilinearly interpolated point, given 4 nearest neighbors
 * @param q11 value at lower left
 * @param q12 value at upper left
 * @param q21 value at lower right
 * @param q22 value at upper right
 * @param x1 x-coord of q11, q12
 * @param x2 x-coord of q21, q22
 * @param y1 y-coord of q11, q21
 * @param y2 y-coord of q12, q22
 * @param x x-coord of desired interp point
 * @param y y-coord of desired interp point
 * @return
 */
inline float
BilinearInterpolation(float q11, float q12, float q21, float q22, float x1, float x2, float y1, float y2, float x, float y)
{
    float x2x1, y2y1, x2x, y2y, yy1, xx1;
    x2x1 = x2 - x1;
    y2y1 = y2 - y1;
    x2x = x2 - x;
    y2y = y2 - y;
    yy1 = y - y1;
    xx1 = x - x1;
    return 1.0 / (x2x1 * y2y1) * (
        q11 * x2x * y2y +
        q21 * xx1 * y2y +
        q12 * x2x * yy1 +
        q22 * xx1 * yy1
    );
}


//anti-aliasing for 1, 4, 9, or 16 samples
int RayTracer::aaImage(const int samples)
{

    if(!traceUI->aaSwitch()) {
        return 0;
    }



//    width = w;
//    height = h;
//    traceSetup(w,h);
//    for(int x  = 0; x < w; ++x)
//    {
//        for(int y = 0; y < h; ++y)
//        {
//            tracePixel(x, y);
//        }
//    }

     return 0;

//    auto thresh = aaThresh;

//    const auto thresh = getAaThreshold();

//    for(int x = 0; x < width; ++x)
//    {
//        for(int y = 0; y < height; ++y)
//        {
//            //            for(auto y = altminmax.first; y <= altminmax.second; y += alt_step)
//            //            {
//            auto y2 = bounded_index(alts.begin(), alts.end(), y);
//            if(y2<1)
//                y2 = 1;
//            auto y1 = y2-1;

//            std::vector<float> row;
//            //                for(auto x = wavminmax.first; x <= wavminmax.second; x += wav_step)
//            //                {
//            auto x2 = bounded_index(wavelengths.begin(), wavelengths.end(), x);
//            if(x2<1)
//                x2 = 1;
//            auto x1 = x2-1;

//            auto q11 = data[y1][x1];
//            auto q12 = data[y2][x1];
//            auto q22 = data[y2][x2];
//            auto q21 = data[y1][x2];
//            row.push_back(BilinearInterpolation(q11, q12, q21, q22, wavelengths[x1], wavelengths[x2], alts[y1], alts[y2], x, y));
//            //                }
//            retval.push_back(row);
//        }
//    }

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

