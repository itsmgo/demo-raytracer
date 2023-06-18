

#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <shader.h>
#include <string>
#include <vector>

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Triangle
{
    Vertex P1;
    Vertex P2;
    Vertex P3;
};

struct BoundingBox
{
    glm::vec3 Pmin;
    glm::vec3 Pmax;

    glm::vec3 diagonal() const { return Pmax - Pmin; }

    int maximumExtent() const
    {
        glm::vec3 d = diagonal();
        if (d.x > d.y && d.x > d.z)
            return 0;
        else if (d.y > d.z)
            return 1;
        else
            return 2;
    }

    // returns (0,0,0) to (1,1,1): point relative to bounding box.
    glm::vec3 offset(const glm::vec3 &point) const
    {
        glm::vec3 o = point - Pmin;
        if (Pmax.x > Pmin.x)
            o.x /= Pmax.x - Pmin.x;
        if (Pmax.y > Pmin.y)
            o.y /= Pmax.y - Pmin.y;
        if (Pmax.z > Pmin.z)
            o.z /= Pmax.z - Pmin.z;
        return o;
    }

    float surfaceArea() const
    {
        glm::vec3 d = diagonal();
        return 2.0f * (d.x * d.y + d.x * d.z + d.y * d.z);
    }
};

class Mesh
{
public:
    // mesh Data
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Triangle> triangles;
    unsigned int VAO;

    // constructor
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices)
    {
        this->vertices = vertices;
        this->indices = indices;

        // now that we have all the required data, set the vertex buffers and its attribute pointers.
        setUpMesh();
        createTriangles();
    }

    // render the mesh
    void draw(int drawMode) // (Shader &shader)
    {
        // bind appropriate textures
        // unsigned int diffuseNr = 1;
        // unsigned int specularNr = 1;
        // unsigned int normalNr = 1;
        // unsigned int heightNr = 1;
        // for (unsigned int i = 0; i < textures.size(); i++)
        // {
        //     glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
        //     // retrieve texture number (the N in diffuse_textureN)
        //     string number;
        //     string name = textures[i].type;
        //     if (name == "texture_diffuse")
        //         number = std::to_string(diffuseNr++);
        //     else if (name == "texture_specular")
        //         number = std::to_string(specularNr++); // transfer unsigned int to string
        //     else if (name == "texture_normal")
        //         number = std::to_string(normalNr++); // transfer unsigned int to string
        //     else if (name == "texture_height")
        //         number = std::to_string(heightNr++); // transfer unsigned int to string

        //     // now set the sampler to the correct texture unit
        //     glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
        //     // and finally bind the texture
        //     glBindTexture(GL_TEXTURE_2D, textures[i].id);
        // }

        // draw mesh
        glBindVertexArray(VAO);
        glDrawElements(drawMode, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // always good practice to set everything back to defaults once configured.
        glActiveTexture(GL_TEXTURE0);
    }

private:
    // render data
    unsigned int VBO, EBO;

    // initializes all the buffer objects/arrays
    void setUpMesh()
    {
        // create buffers/arrays
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        // load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // A great thing about structs is that their memory layout is sequential for all its items.
        // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
        // again translates to 3/2 floats which translates to a byte array.
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // set the vertex attribute pointers
        // vertex Positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
        // vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Normal));
        // vertex texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, TexCoords));
        // // vertex tangent
        // glEnableVertexAttribArray(3);
        // glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Tangent));
        // // vertex bitangent
        // glEnableVertexAttribArray(4);
        // glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Bitangent));
        // // ids
        // glEnableVertexAttribArray(5);
        // glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void *)offsetof(Vertex, m_BoneIDs));

        // // weights
        // glEnableVertexAttribArray(6);
        // glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, m_Weights));
        // glBindVertexArray(0);
    }
    void createTriangles()
    {
        for (int i = 0; i < indices.size(); i += 3)
        {
            triangles.push_back(Triangle{vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]]});
        };
    }
};
#endif
