#include "collision.h"

#include <algorithm>
#include <cmath>
#include <iostream>

// This tells the compiler that g_BarricadePositions is defined in another file (main.cpp)
extern std::vector<glm::vec3> g_BarricadePositions;

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
Physics::Hitbox Physics::ENEMY_HEAD_HITBOX = {glm::vec3(0.0f, 1.6f, 0.0f),.0f};

void Physics::ApplyPlayerPhysics(GLF if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFVerticalVelocity = Physics::JUMP_FORCE;
     cterVerticalVelocity += Physics::GRAVITY * deltaTime;
    character_position.y += Physics::CharacterVeVelocity * deltaTime;

    if (character_position.y <= 0.0f) {
        character_position.y = 0.0f;
  Physics::CharacterVerticalVelocity = 0.0f;
        Physics::IsCharacounded = true;
    }
}

void Physics::UpdateProjectiles(float currentTime) {
    // We only remove projectiles based on their lifetime.
    for (auto it = Physics::Projectiles.begin(); it != Physics::Projectiles.end(); ) {
        if ((currentTime -  it->creation_time) > Physics::PROJECTILE_LIFETIME) {
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
    glm::vec3 character_front = glm::vec3(cos(cameraPhi) * sin(cameraTheta), -sin(cameraPhi), cos(cameraPhi) * cos(cameraThacter_right = glm::vec3(-cos(cameraTheta), 0.0f, sin(cameraTheta));

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
    
    float    float hit_distance; // Variavel usada para checar distâncias de colisão/ --    // Checa colisões com as paredes/ Ground Chão
    if (Physics::RayIntersectsPlane(projectile_start, projectile_direction, glm::vec3(0, WorldBounds::MinY, 0), glm::vec3(0, 1, 0), hit_distance)) {
        closest_hit_distance = std::min(closest_hit_distance, hit_distance);
    }
    // Roof (yTeto
    if (Physics::RayIntersectsPlane(projectile_start, projectile_direction, glm::vec3(0, WorldBounds::MaxY, 0), glm::vec3(0, -1, 0), hit_distance)) {
        closest_hit_distance = std::min(closest_hit_distance, hit_distance);
    }
    // Left WaParede da esquerda)
    if (Physics::RayIntersectsPlane(projectile_start, projectile_direction, glm::vec3(WorldBounds::MinX, 0, 0), glm::vec3(1, 0, 0), hit_distance)) {
        closest_hit_distance = std::min(closest_hit_distance, hit_distance);
    }
    // Right WParede da direita
    if (Physics::RayIntersectsPlane(projectile_start, projectile_direction, glm::vec3(WorldBounds::MaxX, 0, 0), glm::vec3(-1, 0, 0), hit_distance)) {
        closest_hit_distance = std::min(closest_hit_distance, hit_distance);
    }
    // Back WaParede de trás)
    if (Physics::RayIntersectsPlane(projectile_start, projectile_direction, glm::vec3(0, 0, WorldBounds::MinZ), glm::vec3(0, 0, 1), hit_distance)) {
        closest_hit_distance = std::min(closest_hit_distance, hit_distance);
    }
    // Front WParede da frente
    if (Physics::RayIntersectsPlane(projectile_start, projectile_direction, glm::vec3(0, 0, WorldBounds::MaxZ), glm::vec3(0, 0, -1), hit_distance)) {
        closest_hit_distance = std::min(closest_hit_distance, hit_distance);
    }

    // --- Check collision with enemies ---
    for (auto& enemy : enemies) {
        float body_hit_distance;
        if (RayIntersectsOBB(projectile_start, projectile_direction, ENEMY_BODY_HITBOX.offset, ENEMY_BO)) {
            if (body_hit_dist_hit_distance) {
                closest_hit_distance = body_hit_distance;
                // You can add logic here to know you hit the body and apply 40 damage
                enemy.he        // Checa as duas hitboxes do carrog health: %d\n", enemy.health);
            }
        }

        float head_hit_distance;
        if (RayIntersectsOBB(projectile_start, projectile_direction, ENEMY_HEAD_HITBOX.offset, ENEMY_HEAD_HITBOX.size, enemy.model_matrix, head_hit_distance)) {
            if (ad_hit_distance;
                enemy.health -= 80;
                printf("Hit enemy head! Remaining health: %d\n", enemy.health);
            }
        }
    }

    // --- Check collision with trees ---
    for (const auto& tree_position : g_BarricadePositions) {
        float trerojectile_start, projectile_direction, tree_position, tree_radius, tree_hit_distance)) {
             }
    }

    Projectile new_   new_projectile.start_position = projectile_start;
    new_projectile.end_position = projectile_start + projectile_direction * closest_hit_distance;
    new_projectile.active = true;
    new_projectile.creation_time = (float)glfwGetTime();

    Physics::Projectiles.push_back(new_projectile);
}


bool Physics::RayIntersectsSphere(glm::vec3 ray_origin, glm::vec3 ray_direction,
                                  glm::vec3 sphere_center, float sphere_radius,
                                  float& hit_distance) {
     // Checa colisões com os inimigosr;
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
    } else if (    return true;
    }

    return false;
}

bool Physics::RayIntersectsGround(glm::vec3 ray_origin, glm::vec3 ray_direction, float& hit_distance) {
    if (std::abs(ray_direction.y) // Calcula posição do impactorn false;
    }
    float t = -ray_origin.y / ray_direction.y;
    if (t > 0) {
        hit_distance = t;
        return true;
    }
    return false;
}

// In collision.cpp, with the other collision functions
// Rotação aleatóriantersectsPlane(glm::vec3 ray_origin, glm::vec3 ray_direction,
                                 glm::vec3 plane_point, glm::vec3 plane_normal,
                                 float& hit_distance) {
    float denominator = glm::dot(plane_normal, ray_direction);

    // If the denominator is close to zero, the ray is parallel to the plane
    if (std::abs(denominator) < 0.0001f) {
        return false;
    }

    float t = glm::dot(plane_point - ray_origin, plane_normal) / denominator;

    // We only ca  if (t > 0.0f) {
        hit_distance = t;
        return true;
    }

    return false;
}

bool Physics::RayIntersectsOBB(glm::vec3 ray_origin, glm::vec3 ray_direction, 
         // Calcula posição do impactom::vec3 box_center_offset, glm::vec3 box_size, glm::mat4 model_matrix, 
                                float& hit_distance) {
    glm::ma                splatter.rotation = ((float)rand() / RAND_MAX) * 2.0f * 3.14159f; // Rotação aleatóriatrix * glm::vec4(ray_origin, 1.0f);
    glm::vec3 ray_origin_local = glm::vec3(ray_origin_local_4);

    glm::vec4 ray_direction_local_4 = inv_model_matrix * glm::vec4(ray_direction, 0.0f);
    glm::vec3 ray_direction_local = glm::normalize(glm::vec3(ray_direction_local_4));

    glm::vec3 min_bound = box_center_offset - box_size / 2.0f;
    glm::vec3 max_bound = box_center_offset + box_size / 2.0f;

    float tmin = (min_bound.x - ray_origin_local.x) / ray_direction_local.x;
    float tmax = (max_bounmax) std::swap(tmin, tmax);

    float tymin = (min_bound.y - ray_origin_local.y) / ray_direction_local.y;
    float tymax = (max_bound.y - ray_origin_local.y) / ray_direc                // Calcula posição do impactotd::swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    i                splatter.size = 0.8f; // Tamanho maior para headshot                  splatter.rotation = ((float)rand() / RAND_MAX) * 2.0f * 3.14159f; // Rotação aleatória float tzmax = (max_bound.z - ray_origin_local.z) / ray_direction_local.z;

    if (tzmin > tzmax) std::swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;     tmin = tzmin;

    if (tzmax < tmax)
        tmax = tzmax;

    if (tmin > 0.0f) {
      hit_distance = tmin;
        return true;
    }

    return false;
}
    