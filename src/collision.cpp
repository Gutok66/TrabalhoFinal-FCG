#include "collision.h"

#include <algorithm>
#include <cmath>

// This tells the compiler that g_TreePositions is a global variable
// that will be defined and managed in another file (like main.cpp).
extern std::vector<glm::vec3> g_TreePositions;

// --- Variable Definitions ---
float Physics::GRAVITY = -9.8f;
float Physics::JUMP_FORCE = 5.0f;
float Physics::PROJECTILE_LIFETIME = 1.0f;
float Physics::PROJECTILE_MAX_DISTANCE = 100.0f;

std::vector<Projectile> Physics::Projectiles;
float Physics::CharacterVerticalVelocity = 0.0f;
bool Physics::IsCharacterGrounded = true;


// --- Function Implementations ---

void Physics::Initialize() {
    Physics::Projectiles.clear();
    Physics::CharacterVerticalVelocity = 0.0f;
    Physics::IsCharacterGrounded = true;
}

bool Physics::RayIntersectsSphere(glm::vec3 ray_origin, glm::vec3 ray_direction,
                                  glm::vec3 sphere_center, float sphere_radius,
                                  float& hit_distance) {
    glm::vec3 oc = ray_origin - sphere_center;
    float a = dot(ray_direction, ray_direction);
    float b = 2.0f * dot(oc, ray_direction);
    float c = dot(oc, oc) - sphere_radius * sphere_radius;
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) {
        return false;
    }

    float t1 = (-b - sqrt(discriminant)) / (2.0f * a);
    float t2 = (-b + sqrt(discriminant)) / (2.0f * a);

    if (t1 > 0.0f) {
        hit_distance = t1;
        return true;
    } else if (t2 > 0.0f) {
        hit_distance = t2;
        return true;
    }

    return false;
}

bool Physics::RayIntersectsGround(glm::vec3 ray_origin, glm::vec3 ray_direction, float& hit_distance) {
    if (std::abs(ray_direction.y) < 0.001f) {
        return false;
    }

    float t = -ray_origin.y / ray_direction.y;
    if (t > 0) {
        hit_distance = t;
        return true;
    }
    return false;
}

void Physics::ApplyPlayerPhysics(GLFWwindow* window, glm::vec3& character_position, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && Physics::IsCharacterGrounded) {
        Physics::CharacterVerticalVelocity = Physics::JUMP_FORCE;
        Physics::IsCharacterGrounded = false;
    }

    Physics::CharacterVerticalVelocity += Physics::GRAVITY * deltaTime;

    character_position.y += Physics::CharacterVerticalVelocity * deltaTime;

    if (character_position.y <= 0.0f) {
        character_position.y = 0.0f;
        Physics::CharacterVerticalVelocity = 0.0f;
        Physics::IsCharacterGrounded = true;
    }
}

void Physics::HandleShooting(const glm::vec3& character_position,
                             float cameraTheta, float cameraPhi,
                             float cameraDistance, bool firstPerson) {
    glm::vec3 character_front = glm::vec3(cos(cameraPhi) * sin(cameraTheta), -sin(cameraPhi), cos(cameraPhi) * cos(cameraTheta));
    glm::vec3 character_right = glm::vec3(-cos(cameraTheta), 0.0f, sin(cameraTheta));

    float camera_height = 1.7f;
    glm::vec3 camera_position = character_position + glm::vec3(0.0f, camera_height, 0.0f);
    if (!firstPerson) {
        camera_position -= character_front * cameraDistance;
        camera_position += character_right * 0.5f;
    }

    glm::vec3 camera_lookat = camera_position + character_front * 100.0f;
    glm::vec3 projectile_direction = glm::normalize(camera_lookat - camera_position);
    glm::vec3 projectile_start = camera_position + projectile_direction * 1.5f;

    float closest_hit_distance = Physics::PROJECTILE_MAX_DISTANCE;

    float ground_hit_distance;
    if (Physics::RayIntersectsGround(projectile_start, projectile_direction, ground_hit_distance)) {
        closest_hit_distance = std::min(closest_hit_distance, ground_hit_distance);
    }

    for (const auto& tree_position : g_TreePositions) {
        float tree_hit_distance;
        float tree_radius = 2.0f;
        if (Physics::RayIntersectsSphere(projectile_start, projectile_direction, tree_position, tree_radius, tree_hit_distance)) {
            closest_hit_distance = std::min(closest_hit_distance, tree_hit_distance);
        }
    }

    Projectile new_projectile;
    new_projectile.start_position = projectile_start;
    new_projectile.end_position = projectile_start + projectile_direction * closest_hit_distance;
    new_projectile.active = true;
    new_projectile.creation_time = (float)glfwGetTime();

    Physics::Projectiles.push_back(new_projectile);
}