#include <cmath>

#include "scene.h"
#include "light.h"
#include "kdTree.h"
#include "../ui/TraceUI.h"
#include <glm/gtx/extended_min_max.hpp>
#include <iostream>
#include <glm/gtx/io.hpp>

using namespace std;

//recursive function to search out and add octree nodes
void Scene::addOctnode(Octnode* node)
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

void Scene::RecurseOctree(Octnode* node, const ray& r, isect& i)
{
    if(node->boundingBox.intersect(r, i))
    {
        if(!node->children.empty())
            for(auto child_node : node->children)
                RecurseOctree(&child_node, r, tMin, tMax);
        else
        {
            //case 1: we haven't added the children for this node yet
            if(node->boundingBox.volume() != 1)
            {
                //addNode(node)
                //RecurseOctree()
            }
            //case 2: the smallest possible box
            else
            {
                //actually raytrace?
            }
        }

    }
}

bool Geometry::intersect(ray& r, isect& i) const {
	double tmin, tmax;
	if (hasBoundingBoxCapability() && !(bounds.intersect(r, tmin, tmax))) return false;
	// Transform the ray into the object's local coordinate space
	glm::dvec3 pos = transform->globalToLocalCoords(r.getPosition());
	glm::dvec3 dir = transform->globalToLocalCoords(r.getPosition() + r.getDirection()) - pos;
	double length = glm::length(dir);
	dir = glm::normalize(dir);
	// Backup World pos/dir, and switch to local pos/dir
	glm::dvec3 Wpos = r.getPosition();
	glm::dvec3 Wdir = r.getDirection();
	r.setPosition(pos);
	r.setDirection(dir);
	bool rtrn = false;
	if (intersectLocal(r, i))
	{
		// Transform the intersection point & normal returned back into global space.
		i.setN(transform->localToGlobalCoordsNormal(i.getN()));
		i.setT(i.getT()/length);
		rtrn = true;
	}
	// Restore World pos/dir
	r.setPosition(Wpos);
	r.setDirection(Wdir);
	return rtrn;
}

bool Geometry::hasBoundingBoxCapability() const {
	// by default, primitives do not have to specify a bounding box.
	// If this method returns true for a primitive, then either the ComputeBoundingBox() or
    // the ComputeLocalBoundingBox() method must be implemented.

	// If no bounding box capability is supported for an object, that object will
	// be checked against every single ray drawn.  This should be avoided whenever possible,
	// but this possibility exists so that new primitives will not have to have bounding
	// boxes implemented for them.
	return false;
}

void Geometry::ComputeBoundingBox() {
    // take the object's local bounding box, transform all 8 points on it,
    // and use those to find a new bounding box.

    BoundingBox localBounds = ComputeLocalBoundingBox();
        
    glm::dvec3 min = localBounds.getMin();
    glm::dvec3 max = localBounds.getMax();

    glm::dvec4 v, newMax, newMin;

    v = transform->localToGlobalCoords( glm::dvec4(min[0], min[1], min[2], 1) );
    newMax = v;
    newMin = v;
    v = transform->localToGlobalCoords( glm::dvec4(max[0], min[1], min[2], 1) );
    newMax = glm::max(newMax, v);
    newMin = glm::min(newMin, v);
    v = transform->localToGlobalCoords( glm::dvec4(min[0], max[1], min[2], 1) );
    newMax = glm::max(newMax, v);
    newMin = glm::min(newMin, v);
    v = transform->localToGlobalCoords( glm::dvec4(max[0], max[1], min[2], 1) );
    newMax = glm::max(newMax, v);
    newMin = glm::min(newMin, v);
    v = transform->localToGlobalCoords( glm::dvec4(min[0], min[1], max[2], 1) );
    newMax = glm::max(newMax, v);
    newMin = glm::min(newMin, v);
    v = transform->localToGlobalCoords( glm::dvec4(max[0], min[1], max[2], 1) );
    newMax = glm::max(newMax, v);
    newMin = glm::min(newMin, v);
    v = transform->localToGlobalCoords( glm::dvec4(min[0], max[1], max[2], 1) );
    newMax = glm::max(newMax, v);
    newMin = glm::min(newMin, v);
    v = transform->localToGlobalCoords( glm::dvec4(max[0], max[1], max[2], 1) );
    newMax = glm::max(newMax, v);
    newMin = glm::min(newMin, v);
		
    bounds.setMax(glm::dvec3(newMax));
    bounds.setMin(glm::dvec3(newMin));
}

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::add(Geometry* obj) {
	obj->ComputeBoundingBox();
	sceneBounds.merge(obj->getBoundingBox());
	objects.emplace_back(obj);
}

void Scene::add(Light* light)
{
	lights.emplace_back(light);
}


// Get any intersection with an object.  Return information about the 
// intersection through the reference parameter.
//bool Scene::intersect(ray& r, isect& i) const {
//	double tmin = 0.0;
//	double tmax = 0.0;
//	bool have_one = false;
//	for(const auto& obj : objects) {
//		isect cur;
//		if( obj->intersect(r, cur) ) {
//			if(!have_one || (cur.getT() < i.getT())) {
//				i = cur;
//				have_one = true;
//			}
//		}
//	}
//	if(!have_one)
//		i.setT(1000.0);
//	// if debugging,
//	if (TraceUI::m_debug)
//		intersectCache.push_back(std::make_pair(new ray(r), new isect(i)));
//	return have_one;
//}

bool Scene::intersect(ray& r, isect& i) const {
    RecurseOctree(rootNode, r, i);
    return true;
}

TextureMap* Scene::getTexture(string name) {
	auto itr = textureCache.find(name);
	if (itr == textureCache.end()) {
		textureCache[name].reset(new TextureMap(name));
		return textureCache[name].get();
	}
	return itr->second.get();
}


