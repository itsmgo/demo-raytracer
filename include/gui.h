#ifndef GUI_H
#define GUI_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

class Gui
{
public:
    // gui
    bool showSettingsWindow = true;
    bool showSceneWindow = true;
    int sceneViewWidth = 800;
    int sceneViewHeight = 600;

    // constructor
    Gui(GLFWwindow *window)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;         // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;          // Enable Gamepad Controls
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable docking

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        // ImGui::StyleColorsLight();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 150");
    }

    void draw(int width, int height, std::shared_ptr<InputManager> inputManager, std::shared_ptr<Scene> scene, float deltaTime)
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.f, 0.f));
        {
            ImGui::SetNextWindowSize(ImVec2(width, height));
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
            ImGui::End;
        }
        ImGui::PopStyleVar(2);
        ImGui::DockSpace(ImGui::GetID("MyDockSpace"));

        if (showSceneWindow)
        {
            ImGui::Begin("Scene", &showSceneWindow, ImGuiWindowFlags_DockNodeHost);
            ImGui::BeginChild("GameRender");
            // Get the size of the child (i.e. the whole draw size of the windows).
            ImVec2 view = ImGui::GetWindowSize();
            // Because I use the texture from OpenGL, I need to invert the V from the UV.
            if (view.x != scene->width || view.y != scene->height)
            {
                scene->resizeView(view.x, view.y);
            }
            ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(scene->getColorTexture())), view, ImVec2(0, 1), ImVec2(1, 0));
            ImGui::EndChild();
            ImGui::End();
        }
        ImGui::PopStyleVar(1);

        if (showSettingsWindow)
        {
            ImGui::Begin("Settings", &showSettingsWindow, ImGuiWindowFlags_DockNodeHost);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                        1000.0 / double(ImGui::GetIO().Framerate), double(ImGui::GetIO().Framerate));
            ImGui::Text("%.3f ms/frame", deltaTime * 1000.0f);
            ImGui::SeparatorText("Grid");
            ImGui::Checkbox("Show", &scene->GridDraw);
            ImGui::Checkbox("Axis", &scene->AxisDraw);
            ImGui::SeparatorText("Camera");
            ImGui::Text("World position (%.3f, %.3f, %.3f)",
                        scene->Eye->WorldPosition.x, scene->Eye->WorldPosition.y, scene->Eye->WorldPosition.z);
            ImGui::Text("Local position (%.3f, %.3f, %.3f)",
                        scene->Eye->LocalPosition.x, scene->Eye->LocalPosition.y, scene->Eye->LocalPosition.z);
            ImGui::Text("World front (%.3f, %.3f, %.3f)",
                        scene->Eye->WorldFront.x, scene->Eye->WorldFront.y, scene->Eye->WorldFront.z);
            ImGui::Text("Local front (%.3f, %.3f, %.3f)",
                        scene->Eye->Front.x, scene->Eye->Front.y, scene->Eye->Front.z);

            ImGui::SeparatorText("Objects");
            const char *items[scene->Objects.size()];
            int i = 0;
            static int item_current = 0;
            for (auto &&obj : scene->Objects)
            {
                items[i] = obj->name.c_str();
                i++;
            }
            bool newSelection = ImGui::ListBox("", &item_current, items, IM_ARRAYSIZE(items), 4);
            if (newSelection)
            {
                scene->deselectObjects();
                scene->selectObject(scene->Objects[item_current]);
            }

            if (scene->SelectedObjects.size() > 0)
            {
                std::shared_ptr<Object> object = scene->SelectedObjects[0];
                ImGui::SeparatorText((std::string("Selected Object: ") + object->name).c_str());
                float position[3] = {object->location.x, object->location.y, object->location.z};
                bool positionChange = ImGui::DragFloat3("Position", position, 0.01f, -1000.0f, 1000.0f);
                if (positionChange)
                    object->location = glm::vec3(position[0], position[1], position[2]);
                ImGui::Text("Rotation (%.3f, %.3f, %.3f)",
                            object->rotation.x, object->rotation.y, object->rotation.z);
                ImGui::Text("Scale (%.3f, %.3f, %.3f)",
                            object->scale.x, object->scale.y, object->scale.z);
                float color[3] = {object->color.x, object->color.y, object->color.z};
                bool colorChange = ImGui::ColorEdit3("Color", color);
                if (colorChange)
                    object->color = glm::vec3(color[0], color[1], color[2]);
            }
            ImGui::SeparatorText("");
            if (ImGui::Button("Add cube"))
            {
                std::shared_ptr<Object> cube = std::make_shared<Object>(std::string("Cube"), MESH);
                glm::vec3 randomVec = glm::linearRand(glm::vec3(-5.0f), glm::vec3(5.0f));
                cube->translate(randomVec);
                scene->addObject(cube);
            }
            ImGui::SeparatorText("Render");
            ImGui::Text("Samples %i",
                        scene->getSamples());
            bool updateGeo = ImGui::SliderInt("Triangles", &scene->numTriangles, 1, 1000);
            if (updateGeo)
                scene->resetSampling();
            ImGui::End();
        }
    }

    void render(int width, int height)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        ImGui::Render();
        glClearColor(0.11f, 0.12f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    ~Gui()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
};
#endif
