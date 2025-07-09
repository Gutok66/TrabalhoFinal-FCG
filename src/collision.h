#ifndef COLLISION_H
#define COLLISION_H

#include <glm/glm.hpp>
#include <vector>
#include <GLFW/glfw3.h>
#include "enemy.h"

// Struct que armazena informações pros projéteis
struct Projectile {
    glm::vec3 start_position;
    glm::vec3 end_position;
    bool active;
    float creation_time;
};

namespace Physics {

    // Defines the play area boundaries for potential future use
    struct WorldBounds {
        static constexpr float MinX = -20.0f;
        static constexpr float MaxX =  20.0f;
        static constexpr float MinY =  0.0f;
        static constexpr float MaxY =  10.0f;
        static constexpr float MinZ = -20.0f;
        static constexpr float MaxZ =  20.0f;
    };

    // --- Configuration ---
    extern float GRAVITY;
    extern float JUMP_FORCE;
    extern float PROJECTILE_LIFETIME;
    extern float PROJECTILE_MAX_DISTANCE;

    // --- State ---
    extern std::vector<Projectile> Projectiles;
    extern float CharacterVerticalVelocity;
    extern bool IsCharacterGrounded;

    // Hitbox
    struct Hitbox {
        glm::vec3 offset;
        glm::vec3 size;
    };

    extern Hitbox ENEMY_LEGS_HITBOX;
    extern Hitbox ENEMY_BODY_HITBOX;
    extern Hitbox ENEMY_HEAD_HITBOX;
    extern Hitbox CAR_BOTTOM_HITBOX;
    extern Hitbox CAR_TOP_HITBOX;

    // --- Functions ---
    void Initialize();
    void ApplyPlayerPhysics(glm::vec3& character_position);
    void HandleShooting(const glm::vec3& character_position,
                        const glm::vec4& camera_position_c,
                        const glm::vec4& camera_view_vector,
                        const glm::vec4& g_CameraFront,
                        bool firstPerson,
                       std::vector<Enemy>& enemies);
    void UpdateProjectiles(float currentTime);

    // --- Collision Detection ---
    bool RayIntersectsSphere(glm::vec3 ray_origin, glm::vec3 ray_direction,
                           glm::vec3 sphere_center, float sphere_radius,
                           float& hit_distance);
    bool RayIntersectsGround(glm::vec3 ray_origin, glm::vec3 ray_direction,
                           float& hit_distance);
                           
    bool RayIntersectsPlane(glm::vec3 ray_origin, glm::vec3 ray_direction,
                        glm::vec3 plane_point, glm::vec3 plane_normal,
                        float& hit_distance);

    bool RayIntersectsOBB(glm::vec3 ray_origin, glm::vec3 ray_direction,
                        glm::vec3 box_center, glm::vec3 box_size, glm::mat4 model_matrix,
                        float& hit_distance);
}

#endif // COLLISION_H