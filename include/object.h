#ifndef OBJECT_H
#define OBJECT_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>

#include <shader.h>
#include <mesh.h>

enum Draw_Mode
{
    SOLID,
    WIREFRAME,
    NONE,
};

enum Mesh_Type
{
    MESH,
    LIGHT,
    IMPORTED,
};

class Object
{
public:
    // Object Attributes
    std::string name;
    Mesh_Type meshType;
    bool selected = false;

    // transform
    glm::vec3 location = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    // mesh
    std::shared_ptr<Mesh> mesh;

    // draw
    Draw_Mode drawMode;
    glm::vec3 color = glm::vec3(1.0f);

    // constructor
    Object(std::string name, Mesh_Type meshType, std::string path = "")
    {
        std::cout << "holaObject: " << name << std::endl;
        this->name = name;
        this->meshType = meshType;
        switch (meshType)
        {
        case MESH:
            drawMode = SOLID;
            generateCubeMesh();
            break;
        case LIGHT:
            drawMode = WIREFRAME;
            scale = glm::vec3(0.2f);
            color = glm::vec3(1.0f, 0.6f, 0.0f);
            generateTriangleMesh();
            break;
        case IMPORTED:
            drawMode = SOLID;
            loadModel(path);
            break;
        default:
            break;
        }
    }

    void draw()
    {
        int mode;
        switch (drawMode)
        {
        case WIREFRAME:
            mode = GL_LINES;
            break;
        case SOLID:
            mode = GL_TRIANGLES;
            break;
        default:
            return;
        }
        mesh->draw(mode);
    }

    void translate(glm::vec3 globalTranslation = glm::vec3(0))
    {
        location += globalTranslation;
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 getModelMatrix()
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, location);
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        return model;
    }

    std::vector<Triangle> getModelTriangles()
    {
        std::vector<Triangle> modelTriangles = {};
        glm::mat4 model = getModelMatrix();
        Triangle tri;
        for (auto &&triangle : mesh->triangles)
        {
            tri.P1.Position = model * glm::vec4(triangle.P1.Position, 1.0);
            tri.P1.Normal = glm::normalize(glm::vec3(glm::mat4(glm::transpose(glm::inverse(model))) * glm::vec4(triangle.P1.Normal, 1.0)));
            tri.P1.TexCoords = triangle.P1.TexCoords;
            tri.P2.Position = model * glm::vec4(triangle.P2.Position, 1.0);
            tri.P2.Normal = glm::normalize(glm::vec3(glm::mat4(glm::transpose(glm::inverse(model))) * glm::vec4(triangle.P2.Normal, 1.0)));
            tri.P2.TexCoords = triangle.P2.TexCoords;
            tri.P3.Position = model * glm::vec4(triangle.P3.Position, 1.0);
            tri.P3.Normal = glm::normalize(glm::vec3(glm::mat4(glm::transpose(glm::inverse(model))) * glm::vec4(triangle.P3.Normal, 1.0)));
            tri.P3.TexCoords = triangle.P3.TexCoords;
            modelTriangles.push_back(tri);
        }
        return modelTriangles;
    }

    std::shared_ptr<BoundingBox> getBoundingBox()
    {
        glm::mat4 model = getModelMatrix();
        glm::vec3 vertexPosition = model * glm::vec4(mesh->vertices[0].Position, 1.0);
        glm::vec3 minPoint = glm::vec4(vertexPosition, 1.0);
        glm::vec3 maxPoint = glm::vec4(vertexPosition, 1.0);
        for (size_t i = 1; i < mesh->vertices.size(); i++)
        {
            glm::vec3 vertexPosition = model * glm::vec4(mesh->vertices[i].Position, 1.0);
            minPoint = glm::min(minPoint, vertexPosition);
            maxPoint = glm::max(maxPoint, vertexPosition);
        }
        boundingBox->Pmax = maxPoint;
        boundingBox->Pmin = minPoint;
        return boundingBox;
    }

private:
    std::shared_ptr<BoundingBox> boundingBox = std::make_shared<BoundingBox>();

    void generateCubeMesh()
    {
        std::vector<Vertex> vertices = {
            // positions          // normals           // texture coords
            {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 0.0f)}, // 0
            {glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 0.0f)},  // 1
            {glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 1.0f)},   // 2
            {glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 1.0f)},  // 3

            {glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)},   // 6
            {glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f)},  // 5
            {glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)}, // 4
            {glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)},  // 7

            {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(-1.0f, 0.0f, 0.0), glm::vec2(0.0f, 1.0f)},
            {glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(-1.0f, 0.0f, 0.0), glm::vec2(1.0f, 1.0f)},
            {glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(-1.0f, 0.0f, 0.0), glm::vec2(1.0f, 0.0f)},
            {glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(-1.0f, 0.0f, 0.0), glm::vec2(0.0f, 0.0f)},

            {glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)},
            {glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f)},
            {glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f)},
            {glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)},

            {glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 0.0f)},
            {glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 1.0f)},
            {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 1.0f)},
            {glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f)},

            {glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f)},
            {glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f)},
            {glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f)},
            {glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f)}};

        std::vector<unsigned int> indices;
        int i = 0;
        for (int j = 0; j < 6; j++)
        {
            indices.push_back(i + 2);
            indices.push_back(i + 1);
            indices.push_back(i);
            indices.push_back(i);
            indices.push_back(i + 3);
            indices.push_back(i + 2);
            i += 4;
        }

        mesh = std::make_shared<Mesh>(vertices, indices);
    }
    void generateTriangleMesh()
    {
        std::vector<Vertex> vertices = {
            // positions        // normals      // texture coords
            {glm::vec3(-0.5f, 0.0f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f)}, // 0
            {glm::vec3(0.5f, 0.0f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f)},  // 1
            {glm::vec3(0.0f, 0.0f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.5f, 1.0f)},   // 2
        };
        std::vector<unsigned int> indices = {0, 1, 1, 2, 2, 0};

        mesh = std::make_shared<Mesh>(vertices, indices);
    }

    void loadModel(std::string path)
    {
        Assimp::Importer import;
        const aiScene *scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
            return;
        }
        processNode(scene->mRootNode, scene);
    }

    void processNode(aiNode *node, const aiScene *scene)
    {
        // process all the node's meshes (if any)
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh *aimesh = scene->mMeshes[node->mMeshes[i]];
            processMesh(aimesh, scene);
        }
        // then do the same for each of its children
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    void processMesh(aiMesh *aimesh, const aiScene *scene)
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        // process vertices
        for (unsigned int i = 0; i < aimesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector;
            // position
            vector.x = aimesh->mVertices[i].x;
            vector.y = aimesh->mVertices[i].y;
            vector.z = aimesh->mVertices[i].z;
            vertex.Position = vector;
            // normals
            vector.x = aimesh->mNormals[i].x;
            vector.y = aimesh->mNormals[i].y;
            vector.z = aimesh->mNormals[i].z;
            vertex.Normal = vector;
            // tex coords
            if (aimesh->mTextureCoords[0]) // does the aimesh contain texture coordinates?
            {
                glm::vec2 vec;
                vec.x = aimesh->mTextureCoords[0][i].x;
                vec.y = aimesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            vertices.push_back(vertex);
        }
        // process faces
        for (unsigned int i = 0; i < aimesh->mNumFaces; i++)
        {
            aiFace face = aimesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
            {
                indices.push_back(face.mIndices[j]);
            }
        }
        this->mesh = std::make_shared<Mesh>(vertices, indices);
    }
};
#endif
