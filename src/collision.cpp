#include "collision.h"
#include "matrices.h"

#include <algorithm>
#include <cmath>
#include <iostream>


// This tells the compiler that g_BarricadePositions is defined in another file (main.cpp)
extern std::vector<glm::vec3> g_BarricadePositions;
extern std::vector<float> g_BarricadeRotation;

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

// --- Hitbox Definitions ---
Physics::Hitbox Physics::ENEMY_BODY_HITBOX = {glm::vec3(0.0f, 0.75f, 0.0f), glm::vec3(0.8f, 1.5f, 0.6f), 1.0f};
Physics::Hitbox Physics::ENEMY_HEAD_HITBOX = {glm::vec3(0.0f, 1.6f, 0.0f), glm::vec3(0.4f, 0.4f, 0.4f), 2.0f};

void Physics::ApplyPlayerPhysics(glm::vec3& character_position) {

    // --- WALL COLLISION ---
    // Clamp X coordinate
    if (character_position.x > WorldBounds::MaxX) character_position.x = WorldBounds::MaxX;
    if (character_position.x < WorldBounds::MinX) character_position.x = WorldBounds::MinX;

    // Clamp Z coordinate
    if (character_position.z > WorldBounds::MaxZ) character_position.z = WorldBounds::MaxZ;
    if (character_position.z < WorldBounds::MinZ) character_position.z = WorldBounds::MinZ;
}

void Physics::UpdateProjectiles(float currentTime) {
    // We only remove projectiles based on their lifetime.
    for (auto it = Physics::Projectiles.begin(); it != Physics::Projectiles.end(); ) {
        if ((currentTime - it->creation_time) > Physics::PROJECTILE_LIFETIME) {
            it = Physics::Projectiles.erase(it);
        } else {
            ++it;
        }
    }
}

void Physics::HandleShooting(const glm::vec3& character_position,
                             float cameraTheta, float cameraPhi,
                             float cameraDistance, bool firstPerson,
                             std::vector<Enemy>& enemies) {
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
    
    float hit_distance; // Reusable variable for distance checks

    // --- Check all 6 planes of the world box ---

    // Ground (y=0)
    if (Physics::RayIntersectsPlane(projectile_start, projectile_direction, glm::vec3(0, WorldBounds::MinY, 0), glm::vec3(0, 1, 0), hit_distance)) {
        closest_hit_distance = std::min(closest_hit_distance, hit_distance);
    }
    // Roof (y=10)
    if (Physics::RayIntersectsPlane(projectile_start, projectile_direction, glm::vec3(0, WorldBounds::MaxY, 0), glm::vec3(0, -1, 0), hit_distance)) {
        closest_hit_distance = std::min(closest_hit_distance, hit_distance);
    }
    // Left Wall (x=-20)
    if (Physics::RayIntersectsPlane(projectile_start, projectile_direction, glm::vec3(WorldBounds::MinX, 0, 0), glm::vec3(1, 0, 0), hit_distance)) {
        closest_hit_distance = std::min(closest_hit_distance, hit_distance);
    }
    // Right Wall (x=20)
    if (Physics::RayIntersectsPlane(projectile_start, projectile_direction, glm::vec3(WorldBounds::MaxX, 0, 0), glm::vec3(-1, 0, 0), hit_distance)) {
        closest_hit_distance = std::min(closest_hit_distance, hit_distance);
    }
    // Back Wall (z=-20)
    if (Physics::RayIntersectsPlane(projectile_start, projectile_direction, glm::vec3(0, 0, WorldBounds::MinZ), glm::vec3(0, 0, 1), hit_distance)) {
        closest_hit_distance = std::min(closest_hit_distance, hit_distance);
    }
    // Front Wall (z=20)
    if (Physics::RayIntersectsPlane(projectile_start, projectile_direction, glm::vec3(0, 0, WorldBounds::MaxZ), glm::vec3(0, 0, -1), hit_distance)) {
        closest_hit_distance = std::min(closest_hit_distance, hit_distance);
    }

    // --- Check collision with enemies ---
    for (auto& enemy : enemies) {
        float body_hit_distance;
        if (RayIntersectsOBB(projectile_start, projectile_direction, ENEMY_BODY_HITBOX.offset, ENEMY_BODY_HITBOX.size, enemy.model_matrix, body_hit_distance)) {
            if (body_hit_distance < closest_hit_distance) {
                closest_hit_distance = body_hit_distance;
                // You can add logic here to know you hit the body and apply 40 damage
                enemy.health -= 40;
                printf("Hit enemy body! Remaining health: %d\n", enemy.health);
            }
        }

        float head_hit_distance;
        if (RayIntersectsOBB(projectile_start, projectile_direction, ENEMY_HEAD_HITBOX.offset, ENEMY_HEAD_HITBOX.size, enemy.model_matrix, head_hit_distance)) {
            if (head_hit_distance < closest_hit_distance) {
                closest_hit_distance = head_hit_distance;
                enemy.health -= 80;
                printf("Hit enemy head! Remaining health: %d\n", enemy.health);
            }
        }
    }

    // --- Check collision with trees ---
    /*for (const auto& tree_position : g_BarricadePositions) {
        float tree_hit_distance;
        float tree_radius = 2.0f;
        if (Physics::RayIntersectsSphere(projectile_start, projectile_direction, tree_position, tree_radius, tree_hit_distance)) {
            closest_hit_distance = std::min(closest_hit_distance, tree_hit_distance);
        }
    }*/
    // collision com barricade
    glm::vec3 barricade_bbox_min = glm::vec3(-1.0000, -1.0000, -1.0000); 
    glm::vec3 barricade_bbox_max = glm::vec3(2.8668, 1.0000, 1.0000); 

    glm::vec3 box_size = barricade_bbox_max - barricade_bbox_min;
    glm::vec3 box_center_offset = (barricade_bbox_max + barricade_bbox_min) / 2.0f;

    for (size_t i = 0; i < g_BarricadePositions.size(); ++i) {
        // Reconstruct the model matrix for the barricade, just like in main.cpp
        glm::mat4 model_matrix = 
            Matrix_Translate(g_BarricadePositions[i].x, g_BarricadePositions[i].y, g_BarricadePositions[i].z) * Matrix_Rotate_Y(g_BarricadeRotation[i]) * Matrix_Scale(1.2f, 1.2f, 1.2f);

        float barricade_hit_distance;
        if (Physics::RayIntersectsOBB(projectile_start, projectile_direction, box_center_offset, box_size, model_matrix, barricade_hit_distance)) {
            if (barricade_hit_distance < closest_hit_distance) {
                closest_hit_distance = barricade_hit_distance;
            }
        }
    }

    Projectile new_projectile;
    new_projectile.start_position = projectile_start;
    new_projectile.end_position = projectile_start + projectile_direction * closest_hit_distance;
    new_projectile.active = true;
    new_projectile.creation_time = (float)glfwGetTime();

    Physics::Projectiles.push_back(new_projectile);
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

// In collision.cpp, with the other collision functions
bool Physics::RayIntersectsPlane(glm::vec3 ray_origin, glm::vec3 ray_direction,
                                 glm::vec3 plane_point, glm::vec3 plane_normal,
                                 float& hit_distance) {
    float denominator = glm::dot(plane_normal, ray_direction);

    // If the denominator is close to zero, the ray is parallel to the plane
    if (std::abs(denominator) < 0.0001f) {
        return false;
    }

    float t = glm::dot(plane_point - ray_origin, plane_normal) / denominator;

    // We only care about intersections in front of the ray
    if (t > 0.0f) {
        hit_distance = t;
        return true;
    }

    return false;
}

bool Physics::RayIntersectsOBB(glm::vec3 ray_origin, glm::vec3 ray_direction, 
                                glm::vec3 box_center_offset, glm::vec3 box_size, glm::mat4 model_matrix, 
                                float& hit_distance) {
    glm::mat4 inv_model_matrix = glm::inverse(model_matrix);

    glm::vec4 ray_origin_local_4 = inv_model_matrix * glm::vec4(ray_origin, 1.0f);
    glm::vec3 ray_origin_local = glm::vec3(ray_origin_local_4);

    glm::vec4 ray_direction_local_4 = inv_model_matrix * glm::vec4(ray_direction, 0.0f);
    glm::vec3 ray_direction_local = glm::normalize(glm::vec3(ray_direction_local_4));

    glm::vec3 min_bound = box_center_offset - box_size / 2.0f;
    glm::vec3 max_bound = box_center_offset + box_size / 2.0f;

    float tmin = (min_bound.x - ray_origin_local.x) / ray_direction_local.x;
    float tmax = (max_bound.x - ray_origin_local.x) / ray_direction_local.x;

    if (tmin > tmax) std::swap(tmin, tmax);

    float tymin = (min_bound.y - ray_origin_local.y) / ray_direction_local.y;
    float tymax = (max_bound.y - ray_origin_local.y) / ray_direction_local.y;

    if (tymin > tymax) std::swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin)
        tmin = tymin;

    if (tymax < tmax)
        tmax = tymax;

    float tzmin = (min_bound.z - ray_origin_local.z) / ray_direction_local.z;
    float tzmax = (max_bound.z - ray_origin_local.z) / ray_direction_local.z;

    if (tzmin > tzmax) std::swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;

    if (tzmin > tmin)
        tmin = tzmin;

    if (tzmax < tmax)
        tmax = tzmax;

    if (tmin > 0.0f) {
        hit_distance = tmin;
        return true;
    }

    return false;
}