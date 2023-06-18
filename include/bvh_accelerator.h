// https://www.pbr-book.org/3ed-2018/Primitives_and_Intersection_Acceleration/Bounding_Volume_Hierarchies

#ifndef BVH_ACCELERATOR_H
#define BVH_ACCELERATOR_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <algorithm>
#include <mesh.h>

struct BVH_Item
{
    BVH_Item(size_t index, BoundingBox boundingBox)
        : index(index), boundingBox(boundingBox),
          center(0.5f * boundingBox.Pmin + 0.5f * boundingBox.Pmax) {}
    size_t index;
    BoundingBox boundingBox;
    glm::vec3 center;
};

struct BVH_Node
{
    int id;
    std::shared_ptr<BVH_Node> children[2];
    int objectOffset, objectCount, splitAxis;
    BoundingBox bbox;
};

class BVH_Accelerator
{
public:
    // constructor
    BVH_Accelerator()
    {
        std::cout << "Hola BVH" << std::endl;
    }

    void buildTree(std::vector<std::shared_ptr<Object>> objects, int maxNodeItems)
    {
        if (root != nullptr)
        {
            this->items.clear();
            this->orderedObjects.clear();
            this->totalNodes = 0;
            tree.clear();
        }

        for (size_t i = 0; i < objects.size(); i++)
        {
            items.push_back(BVH_Item(i, *objects[i]->getBoundingBox()));
        }
        this->maxNodeItems = maxNodeItems;

        std::cout << "Building BVH" << std::endl;
        root = recursiveBuild(0, objects.size(), objects);
    }

    std::shared_ptr<BVH_Node> recursiveBuild(int start, int end, std::vector<std::shared_ptr<Object>> objects)
    {
        // Compute bounds of all primitives in BVH node
        BoundingBox bbox = items[start].boundingBox;
        for (int i = start + 1; i < end; ++i)
            bbox = getTotalBoundingBox(bbox, items[i].boundingBox);

        int nItems = end - start;

        std::cout << "Spliting between " << end << ", " << start << std::endl;
        std::cout << "Total bbox (" << bbox.Pmin.x << ", " << bbox.Pmin.y << ", " << bbox.Pmin.z << ")";
        std::cout << ", (" << bbox.Pmax.x << ", " << bbox.Pmax.y << ", " << bbox.Pmax.z << ")" << std::endl;
        std::cout << "Surface: " << bbox.surfaceArea() << std::endl;

        if (nItems == 1)
        {
            // Create leaf node
            int firstItemOffset = orderedObjects.size();
            for (int i = start; i < end; ++i)
            {
                int itemIndex = items[i].index;
                orderedObjects.push_back(objects[itemIndex]);
            }
            return createLeaf(firstItemOffset, nItems, bbox);
        }

        // Compute bound of primitive centroids, choose split dimension dim
        BoundingBox centerBbox{items[start].center, items[start].center};
        for (int i = start + 1; i < end; ++i)
            centerBbox = getTotalBoundingBox(centerBbox, items[i].center);
        std::cout << "Centers bbox (" << centerBbox.Pmin.x << ", " << centerBbox.Pmin.y << ", " << centerBbox.Pmin.z << ")";
        std::cout << ", (" << centerBbox.Pmax.x << ", " << centerBbox.Pmax.y << ", " << centerBbox.Pmax.z << ")" << std::endl;
        int dim = centerBbox.maximumExtent();
        if (centerBbox.Pmax[dim] == centerBbox.Pmin[dim])
        {
            // Create leaf node
            std::cout << "No dimension." << std::endl;
            int firstItemOffset = orderedObjects.size();
            for (int i = start; i < end; ++i)
            {
                int itemIndex = items[i].index;
                orderedObjects.push_back(objects[itemIndex]);
            }
            return createLeaf(firstItemOffset, nItems, bbox);
        }

        // Partition primitives into two sets using approximate SAH and build children
        int mid = (start + end) / 2;
        if (nItems <= 4)
        {
            // Partition primitives into equally sized subsets
            mid = (start + end) / 2;
            std::nth_element(&items[start], &items[mid],
                             &items[end - 1] + 1,
                             [dim](const BVH_Item a, const BVH_Item b)
                             {
                                 return a.center[dim] < b.center[dim];
                             });
            std::cout << "Few items. Rearranged items: ";
            for (int i = start; i < end; ++i)
            {
                std::cout << "(" << items[i].index << ", " << items[i].center[dim] << ") > ";
            }
            std::cout << std::endl;

            std::cout << "Creating intermediate node..." << std::endl;
            int nodeId = totalNodes++;
            return createNode(nodeId,
                              recursiveBuild(start, mid, objects),
                              recursiveBuild(mid, end, objects));
        }

        // Allocate BucketInfo for SAH partition buckets
        constexpr int nBuckets = 12;

        struct BucketInfo
        {
            int count = 0;
            BoundingBox boundingBox;
        };
        BucketInfo buckets[nBuckets];

        // Initialize BucketInfo for SAH partition buckets
        for (int i = start; i < end; ++i)
        {
            int b = nBuckets *
                    centerBbox.offset(items[i].center)[dim]; // 0 to 1
            if (b == nBuckets)
                b = nBuckets - 1;
            buckets[b].count++;
            buckets[b].boundingBox = getTotalBoundingBox(buckets[b].boundingBox, items[i].boundingBox);
        }

        // Compute costs for splitting after each bucket
        float cost[nBuckets - 1];
        for (int i = 0; i < nBuckets - 1; ++i)
        {
            BoundingBox b0 = buckets[0].boundingBox;
            int count0 = 0, count1 = 0;
            for (int j = 1; j <= i; ++j)
            {
                b0 = getTotalBoundingBox(b0, buckets[j].boundingBox);
                count0 += buckets[j].count;
            }
            BoundingBox b1 = buckets[i + 1].boundingBox;
            for (int j = i + 2; j < nBuckets; ++j)
            {
                b1 = getTotalBoundingBox(b1, buckets[j].boundingBox);
                count1 += buckets[j].count;
            }
            cost[i] = .125f + (count0 * b0.surfaceArea() +
                               count1 * b1.surfaceArea()) /
                                  bbox.surfaceArea();
            std::cout << "Cost: " << count0 * b0.surfaceArea() << ", " << count1 * b1.surfaceArea() << ", " << bbox.surfaceArea() << ", "
                      << cost[i] << std::endl;
        }

        // Find bucket to split at that minimizes SAH metric
        float minCost = cost[0];
        int minCostSplitBucket = 0;
        std::cout << "Costs: ";
        for (int i = 1; i < nBuckets - 1; ++i)
        {
            std::cout << cost[i] << ", ";
            if (cost[i] < minCost)
            {
                minCost = cost[i];
                minCostSplitBucket = i;
            }
        }
        std::cout << std::endl;

        // Either create leaf or split primitives at selected SAH bucket
        float leafCost = nItems;
        if (nItems > maxNodeItems || minCost < leafCost)
        {
            BVH_Item *pmid = std::partition(&items[start],
                                            &items[end - 1] + 1,
                                            [=](const BVH_Item pi)
                                            {
                                                int b = nBuckets * centerBbox.offset(pi.center)[dim];
                                                if (b == nBuckets)
                                                    b = nBuckets - 1;
                                                return b <= minCostSplitBucket;
                                            });
            mid = pmid - &items[0];
            if (nItems > maxNodeItems)
                std::cout << "Too much items. Spliting at " << mid << std::endl;
            if (minCost < leafCost)
                std::cout << "Bucket intersection costs less. Spliting at " << mid << std::endl;

            std::cout << "Creating intermediate node..." << std::endl;
            int nodeId = totalNodes++;
            return createNode(nodeId,
                              recursiveBuild(start, mid, objects),
                              recursiveBuild(mid, end, objects));
        }
        else
        {
            // Create leaf node
            std::cout << "Expensive bucket intersection." << std::endl;
            int firstItemOffset = orderedObjects.size();
            for (int i = start; i < end; ++i)
            {
                int itemIndex = items[i].index;
                orderedObjects.push_back(objects[itemIndex]);
            }
            return createLeaf(firstItemOffset, nItems, bbox);
        }
    }

    std::vector<std::shared_ptr<BVH_Node>> getBVHTree()
    {
        std::cout << "Starting BVH tree" << std::endl;
        return getBVHSubTree(root);
    }

    std::vector<std::shared_ptr<Object>> getOrderedObjects()
    {
        return orderedObjects;
    }

private:
    std::vector<BVH_Item> items;
    std::vector<std::shared_ptr<Object>> orderedObjects;
    int maxNodeItems;
    int totalNodes = 0;

    std::shared_ptr<BVH_Node> root = nullptr;
    std::vector<std::shared_ptr<BVH_Node>> tree;

    std::shared_ptr<BVH_Node> createLeaf(int first, int n, BoundingBox bbox)
    {
        std::shared_ptr<BVH_Node> node = std::make_shared<BVH_Node>();
        node->id = totalNodes;
        totalNodes++;
        node->objectOffset = first;
        node->objectCount = n;
        node->bbox = bbox;
        node->children[0] = nullptr;
        node->children[1] = nullptr;
        std::cout << "Created BVH leaf: " << node->id << " with object offset + count: " << node->objectOffset << ", " << node->objectCount << std::endl;
        return node;
    }

    std::shared_ptr<BVH_Node> createNode(int id, std::shared_ptr<BVH_Node> c0, std::shared_ptr<BVH_Node> c1)
    {
        std::shared_ptr<BVH_Node> node = std::make_shared<BVH_Node>();
        node->id = id;
        node->children[0] = c0;
        node->children[1] = c1;
        node->bbox = getTotalBoundingBox(c0->bbox, c1->bbox);
        node->objectCount = 0;
        std::cout << "Created BVH node: " << node->id << " with children: " << c0->id << ", " << c1->id << std::endl;
        return node;
    }

    BoundingBox getTotalBoundingBox(BoundingBox bbox1, BoundingBox bbox2)
    {
        BoundingBox bbox;
        glm::vec3 minPoint = glm::min(bbox1.Pmin, bbox2.Pmin);
        glm::vec3 maxPoint = glm::max(bbox1.Pmax, bbox2.Pmax);
        bbox.Pmax = maxPoint;
        bbox.Pmin = minPoint;
        return bbox;
    }

    BoundingBox getTotalBoundingBox(BoundingBox bbox1, glm::vec3 p)
    {
        BoundingBox bbox;
        glm::vec3 minPoint = glm::min(bbox1.Pmin, p);
        glm::vec3 maxPoint = glm::max(bbox1.Pmax, p);
        bbox.Pmax = maxPoint;
        bbox.Pmin = minPoint;
        return bbox;
    }

    std::vector<std::shared_ptr<BVH_Node>> getBVHSubTree(std::shared_ptr<BVH_Node> node)
    {
        std::vector<std::shared_ptr<BVH_Node>> nodeTree = {node};
        if (node->children[0] == nullptr)
        {
            return nodeTree;
        }
        auto rightTree = getBVHSubTree(node->children[0]);
        auto leftTree = getBVHSubTree(node->children[1]);
        nodeTree.insert(nodeTree.end(), rightTree.begin(), rightTree.end());
        nodeTree.insert(nodeTree.end(), leftTree.begin(), leftTree.end());
        return nodeTree;
    }
};
#endif
