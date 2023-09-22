#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <array>
#include <glm/vec3.hpp>
#include <math.h>
#include <omp.h>
#include "ColorRGB.h" // Include your Color class header

constexpr size_t SCREEN_WIDTH = 1000;
constexpr size_t SCREEN_HEIGHT = 700;


// Create a framebuffer alias using the Color struct and the defined screen dimensions
using Framebuffer = std::array<std::array<Color, SCREEN_WIDTH>, SCREEN_HEIGHT>;

void clear(Framebuffer& framebuffer) {

    #pragma omp parallel for
    for (size_t i = 0; i < SCREEN_HEIGHT; i++) {
        for (size_t j = 0; j < SCREEN_WIDTH; j++) {
            framebuffer[i][j] = Color(0,0,0);
        }
    }
}

void point(Framebuffer& framebuffer, int x, int y, const float c) {
    int centeredX = x + SCREEN_WIDTH/2;
    int centeredY = y + SCREEN_HEIGHT/2;
    if (centeredX >= 0 && centeredX < SCREEN_WIDTH && centeredY >= 0 && centeredY < SCREEN_HEIGHT) {
        framebuffer[centeredY][centeredX] = Color(255,255,255)*c;
    }
}

//Retorna la parte decimal de x
float fpart(const float& x) {
    return x - floor(x);
}

//Retorna el complemento (cantidad para llegar al siguiente entero) de la parte decimal de x
float rfpart(const float& x) {
    return 1 - fpart(x);
}

//Método para dibujar una linea de p1 a p2 utilizando el metodo de linea de Xiaolin Wu (anti-aliasing)
//obviamente es un poco más lento que la linea de bresenham
void lineXiaolinWu(Framebuffer& framebuffer, const glm::vec3& p1, const glm::vec3& p2) {

    glm::vec3 v1 = p1;
    glm::vec3 v2 = p2;

    bool steep = fabs(v2.y - v1.y) > fabs(v2.x - v1.x);

    float temp;

    if(steep) {
        temp = v1.x;
        v1.x = v1.y;
        v1.y = temp;
        temp = v2.x;
        v2.x = v2.y;
        v2.y = temp;
    }
    if(v1.x > v2.x) {
        temp = v1.x;
        v1.x = v2.x;
        v2.x = temp;
        temp = v1.y;
        v1.y = v2.y;
        v2.y = temp;
    }

    float dx = v2.x - v1.x;
    float dy = v2.y - v1.y;

    float gradient;

    if((int)(v2.x*100) - (int)(v1.x*100) == 0){
        gradient = 1.0f;
    } else {
        gradient = dy / dx;
    }

    int xend = round(v1.x);
    float yend = v1.y + gradient * (xend - v1.x);
    float xgap = rfpart(v1.x + 0.5f);
    int xpxl1 = xend;
    int ypxl1 = floor(yend);

    if(steep) {
        point(framebuffer, ypxl1, xpxl1, rfpart(yend) * xgap);
        point(framebuffer, ypxl1+1, xpxl1, fpart(yend) * xgap);
    } else {
        point(framebuffer, xpxl1, ypxl1, rfpart(yend) * xgap);
        point(framebuffer, xpxl1, ypxl1+1, fpart(yend) * xgap);
    }

    float intery = yend + gradient;

    xend = round(v2.x);
    yend = v2.y + gradient * (xend - v2.x);
    xgap = fpart(v2.x + 0.5);
    int xpxl2 = xend;
    int ypxl2 = floor(yend);

    if(steep) {
        point(framebuffer,ypxl2, xpxl2, rfpart(yend) * xgap);
        point(framebuffer, ypxl2+1, xpxl2, fpart(yend) * xgap);
    } else {
        point(framebuffer, xpxl2, ypxl2, rfpart(yend) * xgap);
        point(framebuffer, xpxl2, ypxl2+1, fpart(yend) * xgap);
    }

    if(steep) {
        for(int x = xpxl1 + 1; x < xpxl2 -1; x++) {
            point(framebuffer, floor(intery), x, rfpart(intery));
            point(framebuffer, floor(intery)+1, x, fpart(intery));
            intery += gradient;
        }
    } else{
        for(int x = xpxl1 + 1; x < xpxl2 -1; x++) {
            point(framebuffer, x, floor(intery), rfpart(intery));
            point(framebuffer, x, floor(intery)+1, fpart(intery));
            intery += gradient;
        }
    }

}

void triangle(Framebuffer& framebuffer, const glm::vec3& A, const glm::vec3& B, const glm::vec3& C, const Color& color) {
    lineXiaolinWu(framebuffer, A, B);
    lineXiaolinWu(framebuffer, B, C);
    lineXiaolinWu(framebuffer, C, A);
}

void renderBuffer(SDL_Renderer* renderer, const Framebuffer& framebuffer, int textureWidth, int textureHeight) {
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, textureWidth, textureHeight);

    void* texturePixels;
    int pitch;
    SDL_LockTexture(texture, NULL, &texturePixels, &pitch);

    Uint32 format = SDL_PIXELFORMAT_ARGB8888;
    SDL_PixelFormat* mappingFormat = SDL_AllocFormat(format);

    Uint32* texturePixels32 = static_cast<Uint32*>(texturePixels);
    #pragma omp parallel for
    for (int y = 0; y < textureHeight; y++) {
        for (int x = 0; x < textureWidth; x++) {
            int index = y * (pitch / sizeof(Uint32)) + x;
            const Color& color = framebuffer[y][x];
            texturePixels32[index] = SDL_MapRGBA(mappingFormat, color.r, color.g, color.b, color.a);
        }
    }

    SDL_UnlockTexture(texture);
    SDL_Rect textureRect = {0, 0, textureWidth, textureHeight};
    SDL_RenderCopy(renderer, texture, NULL, &textureRect);
    SDL_DestroyTexture(texture);

    SDL_RenderPresent(renderer);
}

#endif // FRAMEBUFFER_H
