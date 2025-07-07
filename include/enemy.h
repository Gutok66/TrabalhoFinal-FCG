#ifndef ENEMY_H
#define ENEMY_H

#include <glm/glm.hpp>
#include <vector>

struct Enemy {
    glm::vec3 position;
    glm::mat4 model_matrix;
    int health;
    glm::vec3 p0, p1, p2, p3; // Control points for the Bezier curve
    float bezier_t;          // Current parameter on the curve (0 to 1)
    float speed;             // Movement speed along the curve
    float rotation_y;        // Rotation around the Y axis
};

extern std::vector<Enemy> g_Enemies;

#endif // ENEMY_H
