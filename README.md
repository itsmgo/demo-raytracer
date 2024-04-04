# Demo Raytracer

This project is an renderer based on raytracing techniques, developed using C++ and OpenGL. It allows for the creation of images by simulating the behavior of light as it interacts with objects in a scene. The user can transform the multiple objects in the scene by translating, rotating and scaling them. The look and feel of the whole software takes inspiration from Blender.

## Screenshots

An example of the workbench where the user can edit the scene.
![Workbench](./docs/workbench1.png) 

A static screenshot of the raytracer renderer.
![Raytracer](./docs/raytracer1.png) 

A custom shader used for debugging purposes.
![custom](./docs/custom.png) 

## Features

- **Live raytracing algorithm**: Utilizes raytracing to simulate the path of light rays in the scene, calculating color contributions from various light sources and surface properties.
- **GPU rendering**: Utilizes OpenGL for efficient rendering and visualization of the scene.
- **BVH acceleration structure**: Builds a BVH to increase the perforance of the triangle-ray intersections.
- **OBJ importer for complex meshes**: Capable of rendering scenes containing complex geometries.

## Getting Started

To get started with the raytracer renderer, follow these steps:

1. Clone the Repository: Clone the repository to your local machine.
2. Build the Project: Build the project using your preferred build system (e.g., CMake, Makefile).
3. Define Scene: Define the scene you want to render by specifying the geometry, materials, lights, and camera parameters in a scene description file.
4. Run Renderer: Run the renderer executable with the scene description file as input.

## Dependencies

This project depends on the following external libraries:
- glad
- glm
- imgui (docking branch)
- glfw3 
- assimp

