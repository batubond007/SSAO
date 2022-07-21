#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static class Helpers {
    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    static glm::mat4 GetViewMatrix(glm::vec3 Position, glm::vec3 Front, glm::vec3 Up)
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // returns projection matrix using perspective projection
    static glm::mat4 GetProjectionMatrix() {
        float fovyRad = (float)(45.0 / 180.0) * M_PI;
        float aspectRatio = 1920.0f / 1080.0f;
        return glm::perspective(fovyRad, aspectRatio, 1.0f, 10000.0f);
    }
};
