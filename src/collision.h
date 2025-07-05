#ifndef COLLISION_H
#define COLLISION_H

#include <glm/glm.hpp>
#include <vector>
#include <GLFW/glfw3.h>

struct Projectile {
    glm::vec3 start_position;
    glm::vec3 end_position;
    bool active;
    float creation_time;
};

namespace Physics {
    // ... (rest of the file is the same)
    void ApplyPlayerPhysics(GLFWwindow* window, glm::vec3& character_position, float deltaTime);
    // ...
}

#endif