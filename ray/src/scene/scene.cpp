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

Scene::Scene() : rootNode(nullptr), maxRecursion(0)
{
}

Scene::~Scene()
{
}

void Scene::add(Geometry* obj)
{
	obj->ComputeBoundingBox();
	sceneBounds.merge(obj->getBoundingBox());
    objects.emplace_back(obj);
}

void Scene::add(Light* light)
{
	lights.emplace_back(light);
}


/**Get any intersection with an object.  Return information about the
 intersection through the reference parameter.*/
bool Scene::intersectNoTree(ray& r, isect& i) {
    double tmin = 0.0;
    double tmax = 0.0;
    bool have_one = false;
    for(const auto& obj : objects) {
        isect cur;
        if( obj->intersect(r, cur) ) {
            if(!have_one || (cur.getT() < i.getT())) {
                i = cur;
                have_one = true;
            }
        }
    }
    if(!have_one)
        i.setT(1000.0);
    // if debugging,
    if (TraceUI::m_debug)
        intersectCache.push_back(std::make_pair(new ray(r), new isect(i)));
    return have_one;
}

void Scene::setMaxRecursion(const int depth) {
    maxRecursion = depth;
}

/**
 * @brief Scene::fillOctnode fills the given octnode with either child nodes
 * or an object node
 * @param node  output: node to be filled
 * @param depth input: recursion depth
 */
void Scene::fillOctnode(Octnode& node, const int depth)
{
    if(!node.children.empty() || !node.objects.empty())
        return;

    //if we haven't reached the recursion depth yet, we'll add some children
    if(depth < maxRecursion)
    {
        auto box_min = node.boundingBox.getMin();
        auto box_max = node.boundingBox.getMax();

        auto x_min = box_min[0];
        auto x_max = box_max[0];
        auto y_min = box_min[1];
        auto y_max = box_max[1];
        auto z_min = box_min[2];
        auto z_max = box_max[2];

        //determine the halfway pts so that we can subdivide on each plane
        auto x_half = (x_max - x_min)/2;
        auto y_half = (y_max - y_min)/2;
        auto z_half = (z_max - z_min)/2;

        //split the bounding box into 8 cubes. these become the children

        node.children.resize(8);

        node.children[0] = (Octnode(BoundingBox(glm::dvec3(x_min, y_min, z_min), glm::dvec3(x_half, y_half, z_half))));
        node.children[1] = (Octnode(BoundingBox(glm::dvec3(x_min, y_min, z_half), glm::dvec3(x_half, y_half, z_max))));
        node.children[2] = (Octnode(BoundingBox(glm::dvec3(x_min, y_half, z_min), glm::dvec3(x_half, y_max, z_half))));
        node.children[3] = (Octnode(BoundingBox(glm::dvec3(x_min, y_half, z_half), glm::dvec3(x_half, y_max, z_max))));

        node.children[4] = (Octnode(BoundingBox(glm::dvec3(x_half, y_min, z_min), glm::dvec3(x_max, y_half, z_half))));
        node.children[5] = (Octnode(BoundingBox(glm::dvec3(x_half, y_min, z_half), glm::dvec3(x_max, y_half, z_max))));
        node.children[6] = (Octnode(BoundingBox(glm::dvec3(x_half, y_half, z_min), glm::dvec3(x_max, y_max, z_half))));
        node.children[7] = (Octnode(BoundingBox(glm::dvec3(x_half, y_half, z_half), glm::dvec3(x_max, y_max, z_max))));
    }
    //if we have reached the recursion depth, we're going to add ptrs to the objects
    //bound by this box to the node
    else if (depth == maxRecursion)
    {
        //check each obj against the bounding box
        for(int i = 0; i < objects.size(); ++i)
        {
            objects[i]->ComputeBoundingBox();
            auto bbox = objects[i]->getBoundingBox();
            if(node.boundingBox.intersects(bbox))
            {
                node.objects.emplace_back(objects[i].get());
            }
        }
        //kind of hacky: there may be end nodes with no
        //intersecting objects, so we'll give those a
        //nullptr
        if(node.objects.empty())
            node.objects.push_back(nullptr);
    }
}

/**
 * @brief Scene::RecurseOctree recurses through the octree to determine
 * if there is an intersection for the ray. This is a lazy method;
 * we won't add nodes to the octree unless we're certain they're needed
 * @param node      the current octnode
 * @param r         the current ray
 * @param i         isect
 * @param depth     recursion depth
 * @param have_one  output: whether or not there is an intersection
 */
void Scene::RecurseOctree(Octnode* node,
                          ray& r,
                          isect& i,
                          int depth,
                          bool& have_one)
{
    if(depth <= maxRecursion)
    {
        double tmax, tmin;
        auto intersecting = node->boundingBox.intersect(r, tmax, tmin);
        //if the ray intersects the current node
        if(intersecting)
        {
            //terminating condition; we've reached the recursion
            //depth of the tree and now we need to find the intersections
            if(!node->objects.empty())
            {
                if (node->objects[0] != nullptr)
                {
                    for(auto obj : node->objects)
                    {
                        isect cur;
                        auto does_intersect = obj->intersect(r, cur);
                        if( does_intersect )
                        {
                            if(!have_one || (cur.getT() < i.getT()))
                            {
                                i = cur;
                                have_one = true;
                            }
                        }
                    }
                }
            }
            else
            {
                //if this node exists in the tree, we'll
                //recurse through its children looking for intersections
                if(!node->children.empty())
                {
                    for(auto &child_node : node->children)
                        RecurseOctree(&child_node, r, i, depth + 1, have_one);
                }
                //if this node has no children and no objects, it hasn't been
                //filled yet. we have to fill it and then go thru its children
                //or objects
                else
                {
                    fillOctnode(*node, depth);
                    RecurseOctree(node, r, i, depth, have_one);
                }
            }
        }
    }
}

//new intersect function using the octree recursion function
bool Scene::intersect(ray& r, isect& i)
{
    if(maxRecursion != 0)
    {
        //if the tree hasn't been constructed yet, add the root node
        if(rootNode == nullptr)
        {
            rootNode.reset(new Octnode(bounds()));
            fillOctnode(*rootNode.get(), 0);
        }
        bool have_one = false;
        RecurseOctree(rootNode.get(), r, i, 0, have_one);
        return have_one;
    }
    else
        return intersectNoTree(r, i);
}

TextureMap* Scene::getTexture(string name) {
	auto itr = textureCache.find(name);
	if (itr == textureCache.end()) {
		textureCache[name].reset(new TextureMap(name));
		return textureCache[name].get();
	}
	return itr->second.get();
}


