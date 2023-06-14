#ifndef SCENE_H
#define SCENE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <algorithm>
#include <map>

#include <camera.h>
#include <object.h>
#include <compute_shader.h>

enum View_Mode
{
    PREVIEW,
    RENDER,
};

class Scene
{
public:
    View_Mode viewMode;

    // view
    int width, height;

    // objects
    std::vector<std::shared_ptr<Object>> Objects;
    std::vector<std::shared_ptr<Object>> SelectedObjects;
    float selectedOutlineWidth = 0.002f;

    // camera
    std::shared_ptr<Camera> Eye;

    // shaders
    Shader mainShader, gridShader, selectionShader, gBufferShader;
    unsigned int uboMatrices;

    // compute shaders
    ComputeShader raytracingShader;
    int numTriangles = 1;

    // grid
    bool GridDraw = true;
    float GridSize = 60.0f;
    int GridDivisions, GridSubDivisions;
    unsigned int GridVAO, GridSubVAO;

    // axis
    bool AxisDraw = true;

    // lines
    std::vector<uint> LineVAOs;
    std::map<uint, uint> LineVBOs;

    // constructor
    Scene(int width, int height, View_Mode mode = PREVIEW) : mainShader("../src/shaders/main/vertex.vert", "../src/shaders/main/fragment.frag"),
                                                             gridShader("../src/shaders/grid/vertex.vert", "../src/shaders/grid/fragment.frag"),
                                                             selectionShader("../src/shaders/selection/vertex.vert", "../src/shaders/selection/fragment.frag"),
                                                             gBufferShader("../src/shaders/gbuffer/vertex.vert", "../src/shaders/gbuffer/fragment.frag"),
                                                             raytracingShader("../src/shaders/raytracing/raytracing.comp")
    {
        std::cout << "holaScene" << std::endl;
        this->width = width;
        this->height = height;
        viewMode = mode;

        // set uniform block for common matrices accross different shaders
        unsigned int uniformBlockIndexMain = glGetUniformBlockIndex(mainShader.ID, "Matrices");
        unsigned int uniformBlockIndexGrid = glGetUniformBlockIndex(gridShader.ID, "Matrices");
        unsigned int uniformBlockIndexSelection = glGetUniformBlockIndex(selectionShader.ID, "Matrices");
        unsigned int uniformBlockIndexGBuffer = glGetUniformBlockIndex(gBufferShader.ID, "Matrices");

        glUniformBlockBinding(mainShader.ID, uniformBlockIndexMain, 0);
        glUniformBlockBinding(gridShader.ID, uniformBlockIndexGrid, 0);
        glUniformBlockBinding(selectionShader.ID, uniformBlockIndexSelection, 0);
        glUniformBlockBinding(gBufferShader.ID, uniformBlockIndexGBuffer, 0);

        glGenBuffers(1, &uboMatrices);
        glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
        glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMatrices, 0, 2 * sizeof(glm::mat4));

        // create grid
        glGenVertexArrays(1, &GridVAO);
        glGenVertexArrays(1, &GridSubVAO);
        createGrid();

        // set up antialiasing and normal framebuffers
        setUpFramebuffers();

        // set up G buffer
        setUpGBuffer();

        // set up buffer to store geo data
        // setUpGeometryData();

        // set up compute texture
        setUpComputeTexture();
    };

    void addEye(std::shared_ptr<Camera> camera)
    {
        Eye = camera;
        resizeView(width, height);
    }

    void resizeView(int width, int height)
    {
        this->width = width;
        this->height = height;
        Eye->Width = width;
        Eye->Height = height;

        // send new projection matrix to GPU
        glm::mat4 projection = Eye->getProjectionMatrix();
        glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(projection));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // resize framebuffers
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorbuffer);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, width, height, GL_TRUE);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

        glBindTexture(GL_TEXTURE_2D, textureScreenColor);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindRenderbuffer(GL_RENDERBUFFER, RBOdepthStencil);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width, height);

        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        // resize g buffer
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);

        glBindTexture(GL_TEXTURE_2D, gNormal);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);

        glBindTexture(GL_TEXTURE_2D, gColorSpec);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindRenderbuffer(GL_RENDERBUFFER, gRBOdepthStencil);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        // resize compute textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, computeTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA,
                     GL_FLOAT, NULL);

        glBindImageTexture(0, computeTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, computeTextureHalf);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width / 2, height / 2, 0, GL_RGBA,
                     GL_FLOAT, NULL);

        glBindImageTexture(1, computeTextureHalf, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindTexture(GL_TEXTURE_2D, 0);

        resetSampling();
    }

    void addObject(std::shared_ptr<Object> newObject)
    {
        for (auto &&object : Objects)
        {
            if (object->name == newObject->name)
            {
                int i = 0;
                for (auto &&obj : Objects)
                {
                    if (obj->name == newObject->name)
                        i++;
                }
                object->name += "." + std::to_string(i);
            }
        }
        Objects.push_back(newObject);
    }

    void deselectObjects()
    {
        for (auto &&object : SelectedObjects)
        {
            object->selected = false;
        }
        SelectedObjects.clear();
    }

    void selectObject(std::shared_ptr<Object> selectedObject)
    {
        for (auto &&object : SelectedObjects)
        {
            if (object == selectedObject)
                return;
        }
        SelectedObjects.push_back(selectedObject);
        selectedObject->selected = true;
    }

    unsigned int getColorTexture()
    {
        switch (viewMode)
        {
        case PREVIEW:
            return textureScreenColor;
        case RENDER:
            if (currentSample == 1)
                return computeTextureHalf;
            return computeTexture;
        default:
            return 0;
        }
    }

    unsigned int getSamples()
    {
        return currentSample;
    }

    void draw()
    {
        // send view matrix to GPU

        if (Eye->updated)
        {
            glm::mat4 view = Eye->getViewMatrix();
            glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(view));
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
            Eye->updated = false;
            resetSampling();
        }

        switch (viewMode)
        {
        case PREVIEW:
            drawPreview();
            break;
        case RENDER:
            drawRender();
            break;
        default:
            break;
        }
    }

    void drawPreview()
    {
        // render scene in dedicated multisampled framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, FBOmultiSample);
        glViewport(0, 0, width, height);
        glClearColor(0.15f, 0.16f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // render objects
        useMainShader();

        // draw selected objects and write in stencil
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);
        for (auto &&object : SelectedObjects)
        {
            mainShader.setMat4("model", object->getModelMatrix());
            mainShader.setVec3("color", object->color);
            object->draw();
        }

        // draw the rest of objects
        glStencilMask(0x00);
        for (auto &&object : Objects)
        {
            if (object->selected)
                continue;
            mainShader.setMat4("model", object->getModelMatrix());
            mainShader.setVec3("color", object->color);
            object->draw();
        }

        // draw selected objects using stencil and ignoring depth
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);
        glDisable(GL_DEPTH_TEST);
        for (auto &&selectedObject : SelectedObjects)
        {
            selectedObject->scale += glm::vec3(selectedOutlineWidth * glm::length(Eye->WorldPosition));
            useSelectionShader(selectedObject->getModelMatrix(), glm::vec3(1.0f, 0.7f, 0.0f));
            selectedObject->draw();
            selectedObject->scale -= glm::vec3(selectedOutlineWidth * glm::length(Eye->WorldPosition));
        }
        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glEnable(GL_DEPTH_TEST);

        // draw grid
        if (GridDraw)
        {
            useGridShader(glm::mat4(1));
            glBindVertexArray(GridVAO);
            glDrawElements(GL_LINES, (1 + GridDivisions) * 2 * 2, GL_UNSIGNED_INT, NULL);
            glBindVertexArray(0);
            gridShader.setFloat("fadeMult", 0.5);
            glBindVertexArray(GridSubVAO);
            glDrawElements(GL_LINES, (1 + GridSubDivisions) * 2 * 2, GL_UNSIGNED_INT, NULL);
            glBindVertexArray(0);
        }

        // draw lines
        for (auto &&lineVAO : LineVAOs)
        {
            gridShader.setFloat("fadeMult", 1.0);
            glBindVertexArray(lineVAO);
            glDrawArrays(GL_LINES, 0, 2);
            glBindVertexArray(0);
        }

        // copy from multisampled framebuffer to simple framebuffer
        glBindFramebuffer(GL_READ_FRAMEBUFFER, FBOmultiSample);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBOscreen);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    void drawRender()
    {
        // glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        // glViewport(0, 0, width, height);
        // glClearColor(0.0, 0.0, 0.0, 1.0); // keep it black so it doesn't leak into g-buffer
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // gBufferShader.use();
        // for (auto &&object : Objects)
        // {
        //     gBufferShader.setMat4("model", object->getModelMatrix());
        //     gBufferShader.setVec3("color", object->color);
        //     object->draw();
        // }
        currentSample++;

        glBindTexture(GL_TEXTURE_BUFFER, vertexBufferTexture);
        useRaytracingShader();
        if (currentSample == 1)
            glDispatchCompute(width / 20 / 2, height / 20 / 2, 1);
        else
            glDispatchCompute(width / 20, height / 20, 1);
        // make sure writing to image has finished before read
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glBindTexture(GL_TEXTURE_BUFFER, 0);
    }

    void resetSampling()
    {
        currentSample = 0;
    }

    void useSelectionShader(glm::mat4 model, glm::vec3 color)
    {
        selectionShader.use();
        selectionShader.setMat4("model", model);
        selectionShader.setVec3("color", color);
    }

    void useGridShader(glm::mat4 model)
    {
        gridShader.use();
        gridShader.setMat4("model", model);
        gridShader.setFloat("fadeMult", 1);
        gridShader.setFloat("fadeOut", glm::length(Eye->WorldPosition));
        gridShader.setFloat("drawAxis", AxisDraw);
    }

    void useMainShader()
    {
        mainShader.use();
        mainShader.setVec3("material.diffuse", glm::vec3(0.6f, 0.6f, 0.6f));
        mainShader.setVec3("material.specular", glm::vec3(0.6f, 0.6f, 0.6f));
        mainShader.setFloat("material.shininess", 16.0f);
        mainShader.setVec3("light.direction", glm::vec3(-0.2f, -1.0f, 0.5f));
        mainShader.setVec3("light.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
        mainShader.setVec3("light.diffuse", glm::vec3(0.7f, 0.7f, 0.7f));
        mainShader.setVec3("light.specular", glm::vec3(0.8f, 0.8f, 0.8f));
        mainShader.setVec3("viewPos", Eye->WorldPosition);
    }

    void useRaytracingShader()
    {
        raytracingShader.use();
        raytracingShader.setVec3("camera.position", Eye->WorldPosition);
        raytracingShader.setVec3("camera.front", Eye->WorldFront);
        raytracingShader.setVec3("camera.right", Eye->WorldRight);
        raytracingShader.setVec3("camera.up", Eye->WorldUp);
        raytracingShader.setFloat("camera.zoom", Eye->Zoom);
        raytracingShader.setFloat("camera.zoom", Eye->Zoom);
        raytracingShader.setInt("currentSample", currentSample);
        raytracingShader.setInt("numTriangles", numTriangles);
    }

    unsigned int addLine(glm::vec3 pointA, glm::vec3 pointB, glm::vec3 color)
    {
        unsigned int lineVAO;
        glGenVertexArrays(1, &lineVAO);
        glBindVertexArray(lineVAO);

        float vertices[6];
        generateLineVertices(pointA, pointB, vertices);

        unsigned int VBO;
        glGenBuffers(1, &VBO);

        // copy our vertices array in a buffer for OpenGL to use
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

        // then set our vertex attributes pointers
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
        glEnableVertexAttribArray(0);
        LineVAOs.push_back(lineVAO);
        LineVBOs[lineVAO] = VBO;
        return lineVAO;
    }

    void updateLine(unsigned int lineVAO, glm::vec3 pointA, glm::vec3 pointB, glm::vec3 color)
    {
        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, LineVBOs[lineVAO]);
        float vertices[6];
        generateLineVertices(pointA, pointB, vertices);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    }

    void removeLine(unsigned int lineVAO)
    {
        glDeleteVertexArrays(1, &lineVAO);
        auto it = std::find(LineVAOs.begin(), LineVAOs.end(), lineVAO);

        if (it != LineVAOs.end())
            LineVAOs.erase(it);
        LineVBOs.erase(lineVAO);
    }

    void swapModes()
    {
        switch (viewMode)
        {
        case PREVIEW:
            viewMode = RENDER;
            setUpGeometryData();
            resetSampling();
            break;
        case RENDER:
            viewMode = PREVIEW;
            break;
        default:
            break;
        }
        std::cout << "Scene mode: " << viewMode << std::endl;
    }

    void createGrid()
    {
        // find grid params
        float logarithm = std::log(GridSize) / std::log(10.0f);
        float rounded = std::trunc(logarithm);
        float gridStep = std::pow(10.0f, rounded);
        float gridSubstep = gridStep / 10;
        float gridDimensions = std::trunc(GridSize / gridStep) * gridStep;
        glBindVertexArray(GridVAO);
        generateGridVertices(gridStep, gridDimensions, GridDivisions);
        glBindVertexArray(GridSubVAO);
        generateGridVertices(gridSubstep, gridDimensions, GridSubDivisions);
    };

    ~Scene()
    {
        glDeleteFramebuffers(1, &FBOmultiSample);
        glDeleteFramebuffers(1, &FBOscreen);
        glDeleteRenderbuffers(1, &RBOdepthStencil);
        glDeleteFramebuffers(1, &gBuffer);
        glDeleteRenderbuffers(1, &gRBOdepthStencil);
    }

private:
    // framebuffer
    unsigned int samples = 4;
    unsigned int FBOmultiSample, FBOscreen, RBOdepthStencil;
    unsigned int textureColorbuffer, textureScreenColor;

    // gbuffer
    unsigned int gBuffer, gRBOdepthStencil;
    unsigned int gPosition, gNormal, gColorSpec;

    // sampler buffer to store geo
    unsigned int vertexSamplerBuffer, indexSamplerBuffer;
    unsigned int vertexBufferTexture, indexBufferTexture;

    // compute shader textures
    unsigned int computeTexture, computeTextureHalf;

    // samples
    unsigned int currentSample = 0;

    void generateGridVertices(float step, float size, int &divisions)
    {
        int gridDivisions = std::round(size / step);
        divisions = gridDivisions;
        float vertices[gridDivisions * gridDivisions * 3];
        unsigned int indices[gridDivisions * gridDivisions * 2];
        int i = 0;
        for (float x = -size / 2.0f; x <= size / 2.0f + 0.005f; x += step)
        {
            vertices[i + 0] = x;
            vertices[i + 1] = 0.0f;
            vertices[i + 2] = -size / 2.0f;
            vertices[i + 3] = x;
            vertices[i + 4] = 0.0f;
            vertices[i + 5] = size / 2.0f;
            i += 6;
        }
        for (float z = -size / 2.0f; z <= size / 2.0f + 0.005f; z += step)
        {
            vertices[i + 0] = -size / 2.0f;
            vertices[i + 1] = 0.0f;
            vertices[i + 2] = z;
            vertices[i + 3] = size / 2.0f;
            vertices[i + 4] = 0.0f;
            vertices[i + 5] = z;
            i += 6;
        }
        for (int i = 0; i < gridDivisions * gridDivisions * 2; i++)
        {
            indices[i] = i;
        }

        unsigned int VBO, EBO;
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        // copy our vertices array in a buffer for OpenGL to use
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // copy our index array in a element buffer for OpenGL to use
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // then set our vertex attributes pointers
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
    }

    void generateLineVertices(glm::vec3 pointA, glm::vec3 pointB, float *vertices)
    {
        vertices[0] = pointA.x;
        vertices[1] = pointA.y;
        vertices[2] = pointA.z;
        vertices[3] = pointB.x;
        vertices[4] = pointB.y;
        vertices[5] = pointB.z;
    }

    void setUpFramebuffers()
    {
        glGenFramebuffers(1, &FBOmultiSample);
        glBindFramebuffer(GL_FRAMEBUFFER, FBOmultiSample);
        textureColorbuffer = createTexture(samples, width, height);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureColorbuffer, 0);

        glGenRenderbuffers(1, &RBOdepthStencil);
        glBindRenderbuffer(GL_RENDERBUFFER, RBOdepthStencil);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBOdepthStencil);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

        glGenFramebuffers(1, &FBOscreen);
        glBindFramebuffer(GL_FRAMEBUFFER, FBOscreen);
        textureScreenColor = createTexture(0, width, height);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureScreenColor, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    unsigned int createTexture(unsigned int samples, unsigned int width, unsigned int height)
    {
        unsigned int texture;
        glGenTextures(1, &texture);
        if (samples > 0)
        {
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, width, height, GL_TRUE);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        }
        if (samples == 0)
        {
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        return texture;
    }

    void setUpGBuffer()
    {
        glGenFramebuffers(1, &gBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

        // - position color buffer
        glGenTextures(1, &gPosition);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

        // - normal color buffer
        glGenTextures(1, &gNormal);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

        // - color + specular color buffer
        glGenTextures(1, &gColorSpec);
        glBindTexture(GL_TEXTURE_2D, gColorSpec);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gColorSpec, 0);

        // - tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
        unsigned int attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
        glDrawBuffers(3, attachments);

        // depth renderbuffer
        glGenRenderbuffers(1, &gRBOdepthStencil);
        glBindRenderbuffer(GL_RENDERBUFFER, gRBOdepthStencil);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gRBOdepthStencil);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void setUpGeometryData()
    {
        // std::vector<Vertex> vertices = {};
        // std::vector<unsigned int> indices = {};

        // for (auto &&obj : SelectedObjects)
        // {
        //     vertices.insert(vertices.end(), obj->mesh->vertices.begin(), obj->mesh->vertices.end());
        //     indices.insert(indices.end(), obj->mesh->indices.begin(), obj->mesh->indices.end());
        // }

        std::vector<std::shared_ptr<std::vector<Triangle>>> ptrTriangles = {};
        for (auto &&obj : Objects)
        {
            ptrTriangles.push_back(obj->getModelTriangles());
        }
        std::vector<Triangle> triangles = {};
        for (const auto &ptrVec : ptrTriangles)
        {
            triangles.insert(triangles.end(), ptrVec->begin(), ptrVec->end());
        }
        triangles.insert(triangles.begin(), Triangle{Vertex{glm::vec3(triangles.size(), 0, 0), glm::vec3(0), glm::vec2(0)}, Vertex{}, Vertex{}});

        // triangles[0].P1.position.x = triangles.size() - 1;

        // Create and bind the buffer object for triangles
        glGenBuffers(1, &vertexSamplerBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexSamplerBuffer);

        // Allocate buffer memory and upload data
        glBufferData(GL_SHADER_STORAGE_BUFFER, triangles.size() * sizeof(Triangle), triangles.data(), GL_STATIC_DRAW);

        // Create the buffer texture
        glGenTextures(1, &vertexBufferTexture);
        glBindTexture(GL_TEXTURE_BUFFER, vertexBufferTexture);

        // Associate the buffer object with the buffer texture
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, vertexSamplerBuffer);

        // Unbind the buffer object and texture
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        glBindTexture(GL_TEXTURE_BUFFER, 0);

        // // Create and bind the buffer object for indices
        // glGenBuffers(1, &indexSamplerBuffer);
        // glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexSamplerBuffer);

        // // Allocate buffer memory and upload data
        // glBufferData(GL_SHADER_STORAGE_BUFFER, indices.size() * sizeof(Vertex), indices.data(), GL_STATIC_DRAW);

        // // Create the buffer texture
        // glGenTextures(1, &indexBufferTexture);
        // glBindTexture(GL_TEXTURE_BUFFER, indexBufferTexture);

        // // Associate the buffer object with the buffer texture
        // glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, indexSamplerBuffer);

        // // Unbind the buffer object and texture
        // glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        // glBindTexture(GL_TEXTURE_BUFFER, 0);
    }

    void setUpComputeTexture()
    {

        glGenTextures(1, &computeTexture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, computeTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA,
                     GL_FLOAT, NULL);

        glBindImageTexture(0, computeTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

        glGenTextures(1, &computeTextureHalf);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, computeTextureHalf);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width/2, height/2, 0, GL_RGBA,
                     GL_FLOAT, NULL);

        glBindImageTexture(1, computeTextureHalf, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};
#endif
