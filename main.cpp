#include <SDL2/SDL.h>
#include <iostream>
#include "ColorRGB.h"
#include "Framebuffer.h"
#include "ObjReader.h"
#include "Face.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <chrono>
#include <omp.h>

using namespace std::chrono;
Framebuffer framebuffer;

std::vector<glm::vec3> vertexArray;
std::vector<glm::vec3> resultVertexArray;
std::vector<Face> faceArray;
glm::mat4 rotMatrix = glm::rotate(glm::mat4(1),(0.05f), glm::vec3(0,1,0.2));

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Error: Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("cg-hw-modeling", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Error: Failed to create SDL window: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Error: Failed to create SDL renderer: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
    if (!init()) {
        return 1;
    }

    bool running = true;

    loadOBJ("../Spaceship.obj", vertexArray, faceArray);

    resultVertexArray = setupVertexArray(vertexArray, faceArray);
    glm::vec4 tempVector;

    auto lastTime = high_resolution_clock::now();  // Record initial time

    Uint32 frameStart, frameTime;
    std::string title = "FPS: ";

    while (running) {
        frameStart = SDL_GetTicks();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

        }

        clear(framebuffer);

        auto currentTime = high_resolution_clock::now();  // Get current time
        float deltaTime = duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;  // Update last time

        // Adjust the rotation speed here
        float rotationSpeed = 0.005f;  // You can tweak this value

        // Update the rotation matrix using time-based rotation
        rotMatrix = glm::rotate(rotMatrix, deltaTime * rotationSpeed, glm::vec3(0, 1, 0));

        #pragma omp parallel for
        for (int i = 0; i < resultVertexArray.size(); i++) {
            glm::vec3 tempVector = rotMatrix * glm::vec4(resultVertexArray[i].x, resultVertexArray[i].y, resultVertexArray[i].z, 0);
            resultVertexArray[i] = glm::vec3(tempVector.x, tempVector.y, tempVector.z);
        }

        #pragma omp parallel for
        for (int i = 0; i < resultVertexArray.size(); i += 3) {
            triangle(framebuffer, resultVertexArray[i], resultVertexArray[i + 1], resultVertexArray[i + 2], Color(255, 255, 255));
        }

        renderBuffer(renderer, framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT);

        frameTime = SDL_GetTicks() - frameStart;

        // Calculate frames per second and update window title
        if (frameTime > 0) {
            std::ostringstream titleStream;
            titleStream << "FPS: " << 1000.0 / frameTime;  // Milliseconds to seconds
            SDL_SetWindowTitle(window, titleStream.str().c_str());
        }

    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
