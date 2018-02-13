#include <cmath>

#include "scene.h"
#include "light.h"
#include "kdTree.h"
#include "../ui/TraceUI.h"
#include <glm/gtx/extended_min_max.hpp>
#include <iostream>
#include <glm/gtx/io.hpp>

using namespace std;

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

Scene::Scene() : rootNode(nullptr)
{
}

Scene::~Scene()
{
}

void Scene::add(Geometry* obj)
{
	obj->ComputeBoundingBox();
	sceneBounds.merge(obj->getBoundingBox());
    objects.emplace_back(std::shared_ptr<Geometry>(obj));
}

void Scene::add(Light* light)
{
	lights.emplace_back(light);
}


/**Get any intersection with an object.  Return information about the
 intersection through the reference parameter.*/
//bool Scene::intersect(ray& r, isect& i) const {
//    double tmin = 0.0;
//    double tmax = 0.0;
//    bool have_one = false;
//    for(const auto& obj : objects) {
//        isect cur;
//        if( obj->intersect(r, cur) ) {
//            if(!have_one || (cur.getT() < i.getT())) {
//                i = cur;
//                have_one = true;
//            }
//        }
//    }
//    if(!have_one)
//        i.setT(1000.0);
//    // if debugging,
//    if (TraceUI::m_debug)
//        intersectCache.push_back(std::make_pair(new ray(r), new isect(i)));
//    return have_one;
//}

//recursive function to search out and add octree nodes
void Scene::addOctnode(Octnode* node, int depth)
{
    assert(node);

    //if we're not at the smallest pt yet, add some children
    if(depth < 4)
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

        node->children.push_back(Octnode(BoundingBox(glm::dvec3(x_min, y_min, z_min), glm::dvec3(x_half, y_half, z_half))));
        node->children.push_back(Octnode(BoundingBox(glm::dvec3(x_min, y_min, z_half), glm::dvec3(x_half, y_half, z_max))));
        node->children.push_back(Octnode(BoundingBox(glm::dvec3(x_min, y_half, z_min), glm::dvec3(x_half, y_max, z_half))));
        node->children.push_back(Octnode(BoundingBox(glm::dvec3(x_min, y_half, z_half), glm::dvec3(x_half, y_max, z_max))));

        node->children.push_back(Octnode(BoundingBox(glm::dvec3(x_half, y_min, z_min), glm::dvec3(x_max, y_half, z_half))));
        node->children.push_back(Octnode(BoundingBox(glm::dvec3(x_half, y_min, z_half), glm::dvec3(x_max, y_half, z_max))));
        node->children.push_back(Octnode(BoundingBox(glm::dvec3(x_half, y_half, z_min), glm::dvec3(x_max, y_max, z_half))));
        node->children.push_back(Octnode(BoundingBox(glm::dvec3(x_half, y_half, z_half), glm::dvec3(x_max, y_max, z_max))));
    }
    //if we are, add some objects
    else
    {
        for(int i = 0; i < objects.size(); ++i)
        {
            auto bbox = objects[i]->getBoundingBox();
//            assert(node->boundingBox != nullptr);
//            assert(bbox != nullptr);
            if(node->boundingBox.intersects(bbox)
                    && std::find(node->objects.begin(), node->objects.end(), objects[i]) == node->objects.end())
            {
                node->objects.emplace_back(objects[i]);
            }
        }
    }
}

bool Scene::RecurseOctree(Octnode* node, ray& r, isect& i, int depth)
{
    auto rt = r.at(i);
//    auto intersecting = node->boundingBox.intersects(rt);
    double tmax, tmin;
    bool have_one = false;
    auto intersecting = node->boundingBox.intersect(r, tmax, tmin);
    if(intersecting)
    {
        if(!node->children.empty())
            for(auto child_node : node->children)
                RecurseOctree(&child_node, r, i, depth + 1);
        else
        {
            //case 1: we haven't added the children for this node yet
            if(node->boundingBox.volume() != 1)
            {
                addOctnode(node, depth);
                RecurseOctree(node, r, i, depth);
            }
            //case 2: we've reached the depth of recursion
            else
            {
                for(auto obj : node->objects)
                {
                    isect cur;
                    if( obj->intersect(r, cur) ) {
                        if(!have_one || (cur.getT() < i.getT())) {
                            i = cur;
                            have_one = true;
                        }
                    }
                }
            }
        }

    }
    if(!have_one)
        i.setT(1000.0);
    return have_one;
}

bool Scene::intersect(ray& r, isect& i)
{
    if(rootNode == nullptr)
    {
        rootNode.reset(new Octnode(bounds()));
        addOctnode(rootNode.get(), 0);
    }
    return RecurseOctree(rootNode.get(), r, i, 0);
}

TextureMap* Scene::getTexture(string name) {
	auto itr = textureCache.find(name);
	if (itr == textureCache.end()) {
		textureCache[name].reset(new TextureMap(name));
		return textureCache[name].get();
	}
	return itr->second.get();
}


