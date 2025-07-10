//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Fundamentos de Computação Gráfica
//               Prof. Eduardo Gastal
//
//                   TRABALHO FINAL
//

// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#include <cmath> 
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"

// Header da colisão
#include "collisions.h"
#include "enemy.h"

std::vector<Enemy> g_Enemies;

struct MuzzleFlash {
    bool active;
    float lifetime;
    float max_lifetime;
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
};

MuzzleFlash g_MuzzleFlash = {
    false,              // Inicialmente inativo
    0.0f,               // Tempo de vida atual (0ms)
    0.1f,              // Tempo de vida máximo (150ms)
    glm::vec3(0.0f),    // Posição (será definida ao disparar)
    glm::vec3(1.0f, 0.7f, 0.3f), // Cor laranja-amarelada
    3.0f                // Intensidade da luz
};


// Vetor para armazenar todos os sangues ativos
std::vector<BloodSplatter> g_BloodSplatters;

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);

        // Se basepath == NULL, então setamos basepath como o dirname do
        // filename, para que os arquivos MTL sejam corretamente carregados caso
        // estejam no mesmo diretório dos arquivos OBJ.
        std::string fullpath(filename);
        std::string dirname;
        if (basepath == NULL)
        {
            auto i = fullpath.find_last_of("/");
            if (i != std::string::npos)
            {
                dirname = fullpath.substr(0, i+1);
                basepath = dirname.c_str();
            }
        }

        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr,
                        "*********************************************\n"
                        "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                        "Veja https://www.inf.ufrgs.br/~eslgastal/fcg-faq-etc.html#Modelos-3D-no-formato-OBJ .\n"
                        "*********************************************\n",
                    filename);
                throw std::runtime_error("Objeto sem nome.");
            }
            printf("- Objeto '%s'\n", shapes[shape].name.c_str());
        }

        printf("OK.\n");
    }
};


// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);
void TextRendering_ShowAmmo(GLFWwindow* window);
void TextRendering_ShowKills(GLFWwindow* window);
void TextRendering_ShowReload(GLFWwindow* window);
void DrawBoundingBox(const glm::vec3& bbox_min, const glm::vec3& bbox_max, const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection);


// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void ProcessInput(GLFWwindow* window, glm::vec3& character_position, float deltaTime);

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};


std::vector<Projectile> g_Projectiles;
const float PROJECTILE_LIFETIME = 1.0f; 
const float PROJECTILE_MAX_DISTANCE = 100.0f;

// Abaixo definimos variáveis globais utilizadas em várias funções do código.
// Variáveis para gravidade e pulo
float g_CharacterVerticalVelocity = 0.0f;
bool g_IsCharacterGrounded = true;
const float GRAVITY = -9.8f; // Gravidade da Terra em m/s²
const float JUMP_FORCE = 5.0f; // Altura do pulo

// Variável para hitbox player
glm::vec3 g_PlayerSize = glm::vec3(0.6f, 1.8f, 0.6f); 

// Variáveis para hitbox dos inimigos
glm::vec3 g_EnemyPhysicsBboxMin;
glm::vec3 g_EnemyPhysicsBboxMax;

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 1.4f; // Distância da câmera para a origem
float g_CameraSpeed = 1.0f; // Velocidade de movimento da câmera
glm::vec4 g_CameraFront; 
glm::vec4 camera_position_c; // Posição da câmera em coordenadas cartesianas
glm::vec4 camera_view_vector; // Vetor de visão da câmera

bool FirstPerson = false; // Variável que controla se a câmera está em primeira pessoa ou terceira pessoa
glm::mat4 character = Matrix_Identity(); // Personagem
glm::vec3 character_position = glm::vec3(0.0f, 0.0f, 0.0f);

std::vector<glm::vec3> g_BarricadePositions;
std::vector<float> g_BarricadeRotation; // Rotação da barricada

std::vector<glm::vec3> g_CarPositions;
std::vector<float> g_CarRotation; // Rotação do carro

int Ammo = 30; // Total de munição disponível
int Kills = 0; // Total de Eliminações

bool ShowHitBoxes = false;

// Variáveis de recuo
glm::vec3 g_RecoilOffset = glm::vec3(0.0f);
float g_RecoilTimer = 0.0f;
const float RECOIL_DURATION = 0.3f;
const float RECOIL_STRENGTH = 0.1f;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;
// Variáveis para projectile
GLint g_projectile_alpha_uniform;
GLint g_blood_alpha_uniform;
GLint g_muzzle_flash_active_uniform;
GLint g_muzzle_flash_position_uniform;
GLint g_muzzle_flash_color_uniform;
GLint g_muzzle_flash_intensity_uniform;
extern glm::vec3 barricade_bbox_min;
extern glm::vec3 barricade_bbox_max;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

float camera_height = 1.7f;     // Altura da câmera em relação ao chão
float lastFrame = 0.0f;
float deltaTime = 0.0f;
float enemyTime = 0.0f;
float lastEnemyTime = 0.0f;
int window_height = 600.0f;

#define PLANE 0
#define ENEMY_HEAD 1
#define ENEMY_FACE 2
#define ENEMY_EYE 3
#define ENEMY_MIDDLE 4
#define ENEMY_BOTTOM 5
#define METAL 6
#define BARRICADE 7
#define WALL 8
#define ROOF 9
#define TREE_BRANCH3 10
#define CROSSHAIR 11
#define PROJECTILE_LINE 12
#define RGZ89 13
#define WZ96_Beryl 14
#define boot_war1 15
#define glass 16
#define magb 17
#define material 18
#define pol_bproof 19
#define pol_filter 20
#define pol_gas_mask 21
#define pol_hand 22
#define pol_head 23
#define pol_helmet 24
#define pol_jaket 25
#define pol_pants 26
#define MUZZLE_FLASH 27
#define BLOOD_SPLATTER 28
#define COVERED_CAR 29


// Função para calcular um ponto na curva de Bezier cúbica, FONTE: Função feita com IA Gemini 2.5 Pro
glm::vec3 CalculateBezierPoint(float t, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    glm::vec3 p = uuu * p0; 
    p += 3 * uu * t * p1;
    p += 3 * u * tt * p2;
    p += ttt * p3;

    return p;
}

// Função para gerar uma nova curva de Bezier para um inimigo
void GenerateBezierCurve(Enemy& enemy) {
    enemy.p0 = enemy.position; // Começa da posição atual
    // Define um novo destino aleatório dentro de uma área
    enemy.p3 = glm::vec3(enemy.position.x + ((rand() % 200) - 100) / 20.0f, 0.0f, enemy.position.z +((rand() % 200) - 100) / 20.0f);

    // Gera pontos de controle intermediários para criar uma curva suave
    enemy.p1 = enemy.p0 + glm::vec3(((rand() % 100) - 50) / 10.0f, 0.0f, ((rand() % 100) - 50) / 10.0f);
    enemy.p2 = enemy.p3 + glm::vec3(((rand() % 100) - 50) / 10.0f, 0.0f, ((rand() % 100) - 50) / 10.0f);

    // Garante que os pontos estejam dentro dos limites
    enemy.p0.x = glm::clamp(enemy.p0.x, -19.0f, 19.0f);
    enemy.p0.y = glm::clamp(enemy.p0.y, 0.0f, 0.0f);
    enemy.p0.z = glm::clamp(enemy.p0.z, -19.0f, 19.0f);
    
    enemy.p1.x = glm::clamp(enemy.p1.x, -19.0f, 19.0f);
    enemy.p1.y = glm::clamp(enemy.p1.y, 0.0f, 0.0f);
    enemy.p1.z = glm::clamp(enemy.p1.z, -19.0f, 19.0f);
    
    enemy.p2.x = glm::clamp(enemy.p2.x, -19.0f, 19.0f);
    enemy.p2.y = glm::clamp(enemy.p2.y, 0.0f, 0.0f);
    enemy.p2.z = glm::clamp(enemy.p2.z, -19.0f, 19.0f);
    
    enemy.p3.x = glm::clamp(enemy.p3.x, -19.0f, 19.0f);
    enemy.p3.y = glm::clamp(enemy.p3.y, 0.0f, 0.0f);
    enemy.p3.z = glm::clamp(enemy.p3.z, -19.0f, 19.0f);
    
    enemy.bezier_t = 0.0f; // Reseta o parâmetro da curva
}

// FONTE: Função feita com ajuda da IA Gemini 2.5 Pro
// Checa colisão entre dois AABBs
bool CheckAABBvsOBBCollision(
    const glm::vec3& aabb_min, const glm::vec3& aabb_max,
    const glm::vec3& obb_center, const glm::vec3& obb_half_extents, const glm::mat4& obb_transform)
{
    // Get properties of both boxes
    glm::vec3 aabb_half_extents = (aabb_max - aabb_min) / 2.0f;
    glm::vec3 aabb_center = aabb_min + aabb_half_extents;
    // Get the axes of the OBB from its transformation matrix
    glm::vec3 obb_axes[3] = {
        glm::vec3(obb_transform[0])/norm(glm::vec4(glm::vec3(obb_transform[0]),0.0f)),
        glm::vec3(obb_transform[1])/norm(glm::vec4(glm::vec3(obb_transform[1]),0.0f)),
        glm::vec3(obb_transform[2])/norm(glm::vec4(glm::vec3(obb_transform[2]),0.0f))
    };
    // Get the world axes (for the AABB)
    glm::vec3 aabb_axes[3] = {
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    };
    // Vector from the center of the AABB to the center of the OBB
    glm::vec3 to_center = obb_center - aabb_center;
    // --- Test all 15 potential separating axes ---
    // 1. Test the 3 axes of the AABB
    for (int i = 0; i < 3; ++i) {
        float rA = aabb_half_extents[i];
        float rB = obb_half_extents.x * glm::abs(dotproduct3(aabb_axes[i], obb_axes[0])) +
                   obb_half_extents.y * glm::abs(dotproduct3(aabb_axes[i], obb_axes[1])) +
                   obb_half_extents.z * glm::abs(dotproduct3(aabb_axes[i], obb_axes[2]));
        if (glm::abs(dotproduct3(to_center, aabb_axes[i])) > rA + rB) {
            return false; // Found a separating axis
        }
    }
    // 2. Test the 3 axes of the OBB
    for (int i = 0; i < 3; ++i) {
        float rA = aabb_half_extents.x * glm::abs(dotproduct3(obb_axes[i], aabb_axes[0])) +
                   aabb_half_extents.y * glm::abs(dotproduct3(obb_axes[i], aabb_axes[1])) +
                   aabb_half_extents.z * glm::abs(dotproduct3(obb_axes[i], aabb_axes[2]));
        float rB = obb_half_extents[i];
        if (glm::abs(dotproduct3(to_center, obb_axes[i])) > rA + rB) {
            return false; // Found a separating axis
        }
    }

    // 3. Test the 9 cross-product axes
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            glm::vec3 axis = crossproduct3(aabb_axes[i], obb_axes[j]);
            if (dotproduct3(axis, axis) < 0.00001f) continue; // Skip near-parallel axes

            float rA = aabb_half_extents.x * glm::abs(dotproduct3(axis, aabb_axes[0])) +
                       aabb_half_extents.y * glm::abs(dotproduct3(axis, aabb_axes[1])) +
                       aabb_half_extents.z * glm::abs(dotproduct3(axis, aabb_axes[2]));

            float rB = obb_half_extents.x * glm::abs(dotproduct3(axis, obb_axes[0])) +
                       obb_half_extents.y * glm::abs(dotproduct3(axis, obb_axes[1])) +
                       obb_half_extents.z * glm::abs(dotproduct3(axis, obb_axes[2]));

            if (glm::abs(dotproduct3(to_center, axis)) > rA + rB) {
                return false; // Found a separating axis
            }
        }
    }

    // No separating axis was found, so the boxes are colliding
    return true;
}

// FONTE: Função feita com ajuda da IA Gemini 2.5 Pro
// calcula melhor colisão em cima dos objetos
bool Check2DAABBvsOBBCollision(
    const glm::vec3& aabb_min, const glm::vec3& aabb_max,
    const glm::vec3& obb_center, const glm::vec3& obb_half_extents, const glm::mat4& obb_transform)
{
    // Use 2D vectors for easier calculations in the XZ plane
    glm::vec2 aabb_min_2d(aabb_min.x, aabb_min.z);
    glm::vec2 aabb_max_2d(aabb_max.x, aabb_max.z);
    glm::vec2 aabb_half_extents_2d = (aabb_max_2d - aabb_min_2d) / 2.0f;
    glm::vec2 aabb_center_2d = aabb_min_2d + aabb_half_extents_2d;
    glm::vec2 obb_center_2d(obb_center.x, obb_center.z);
    // Get the 2D axes of the OBB from its transformation matrix (on the XZ plane)
    // We only care about the X and Z axes for the footprint.
    glm::vec2 obb_axes_2d[2] = {
        glm::vec2(obb_transform[0].x, obb_transform[0].z)/norm(glm::vec4(obb_transform[0].x, obb_transform[0].z, 0.0f, 0.0f)), // OBB's local X axis
        glm::vec2(obb_transform[2].x, obb_transform[2].z)/norm(glm::vec4(obb_transform[2].x, obb_transform[2].z, 0.0f, 0.0f))  // OBB's local Z axis
    };
    glm::vec2 obb_half_extents_2d(obb_half_extents.x, obb_half_extents.z);
    glm::vec2 to_center = obb_center_2d - aabb_center_2d;
    // AABB's axes (world X and Z)
    glm::vec2 aabb_axes_2d[2] = { glm::vec2(1, 0), glm::vec2(0, 1) };
    // Test AABB's axes
    for (int i = 0; i < 2; ++i) {
        float rA = aabb_half_extents_2d[i];
        float rB = obb_half_extents_2d.x * glm::abs(dotproduct2(aabb_axes_2d[i], obb_axes_2d[0])) +
                   obb_half_extents_2d.y * glm::abs(dotproduct2(aabb_axes_2d[i], obb_axes_2d[1]));
        if (glm::abs(dotproduct2(to_center, aabb_axes_2d[i])) > rA + rB) return false;
    }
    // Test OBB's axes
    for (int i = 0; i < 2; ++i) {
        float rA = aabb_half_extents_2d.x * glm::abs(dotproduct2(obb_axes_2d[i], aabb_axes_2d[0])) +
                   aabb_half_extents_2d.y * glm::abs(dotproduct2(obb_axes_2d[i], aabb_axes_2d[1]));
        float rB = obb_half_extents_2d[i];
        if (glm::abs(dotproduct2(to_center, obb_axes_2d[i])) > rA + rB) return false;
    }
    return true; // No separating axis found, they overlap
}
// FONTE: Função feita com ajuda da IA Gemini 2.5 Pro
// A struct to hold information about a collision
struct CollisionInfo
{
    bool hasCollided = false;
    glm::vec3 mtv = glm::vec3(0.0f); // Minimum Translation Vector to resolve the collision
};

// collision detection function
CollisionInfo FindCollision(const glm::vec3& playerMin, const glm::vec3& playerMax, const glm::mat4& objectTransform, const glm::vec3& objectMinLocal, const glm::vec3& objectMaxLocal)
{
    // --- Get World-Space Properties of Both Boxes ---
    const glm::vec3 playerHalfExtents = (playerMax - playerMin) / 2.0f;
    const glm::vec3 playerCenter = playerMin + playerHalfExtents;

    const glm::vec3 objectHalfExtentsLocal = (objectMaxLocal - objectMinLocal) / 2.0f;
    const glm::vec3 objectCenter = glm::vec3(objectTransform * glm::vec4((objectMinLocal + objectMaxLocal) / 2.0f, 1.0f));

    // OBB orientation axes (normalized direction vectors)
    glm::vec3 objectAxes[3] = {
        glm::vec3(objectTransform[0])/norm(glm::vec4(glm::vec3(objectTransform[0]), 0.0f)),
        glm::vec3(objectTransform[1])/norm(glm::vec4(glm::vec3(objectTransform[1]), 0.0f)),
        glm::vec3(objectTransform[2])/norm(glm::vec4(glm::vec3(objectTransform[2]), 0.0f))
    };

    // OBB scaled half-extents (size along each of its axes)
    glm::vec3 objectScaledHalfExtents = {
        norm(glm::vec4(glm::vec3(objectTransform[0]), 0.0f)) * objectHalfExtentsLocal.x,
        norm(glm::vec4(glm::vec3(objectTransform[1]), 0.0f)) * objectHalfExtentsLocal.y,
        norm(glm::vec4(glm::vec3(objectTransform[2]), 0.0f)) * objectHalfExtentsLocal.z
    };

    // AABB orientation axes (world axes)
    glm::vec3 playerAxes[3] = {
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f)
    };

    // --- Separating Axis Theorem (SAT) ---
    float minOverlap = std::numeric_limits<float>::max();
    glm::vec3 smallestAxis;
    
    // Axes to test: 3 from player, 3 from object, 9 from cross products
    glm::vec3 axesToTest[15];
    axesToTest[0] = playerAxes[0];
    axesToTest[1] = playerAxes[1];
    axesToTest[2] = playerAxes[2];
    axesToTest[3] = objectAxes[0];
    axesToTest[4] = objectAxes[1];
    axesToTest[5] = objectAxes[2];
    axesToTest[6] = crossproduct3(playerAxes[0], objectAxes[0]);
    axesToTest[7] = crossproduct3(playerAxes[0], objectAxes[1]);
    axesToTest[8] = crossproduct3(playerAxes[0], objectAxes[2]);
    axesToTest[9] = crossproduct3(playerAxes[1], objectAxes[0]);
    axesToTest[10] = crossproduct3(playerAxes[1], objectAxes[1]);
    axesToTest[11] = crossproduct3(playerAxes[1], objectAxes[2]);
    axesToTest[12] = crossproduct3(playerAxes[2], objectAxes[0]);
    axesToTest[13] = crossproduct3(playerAxes[2], objectAxes[1]);
    axesToTest[14] = crossproduct3(playerAxes[2], objectAxes[2]);

    for (int i = 0; i < 15; i++) {
        glm::vec3 axis = axesToTest[i];
        
        // SAFETY CHECK: Skip near-zero axes from parallel cross products
        if (dotproduct3(axis, axis) < 1e-8f) {
            continue;
        }
        axis = axis/norm(glm::vec4(axis, 0.0f));

        // Project both boxes onto the current axis
        float rA = glm::abs(playerHalfExtents.x * dotproduct3(axis, playerAxes[0])) +
                   glm::abs(playerHalfExtents.y * dotproduct3(axis, playerAxes[1])) +
                   glm::abs(playerHalfExtents.z * dotproduct3(axis, playerAxes[2]));

        float rB = glm::abs(objectScaledHalfExtents.x * dotproduct3(axis, objectAxes[0])) +
                   glm::abs(objectScaledHalfExtents.y * dotproduct3(axis, objectAxes[1])) +
                   glm::abs(objectScaledHalfExtents.z * dotproduct3(axis, objectAxes[2]));

        float distance = glm::abs(dotproduct3(objectCenter - playerCenter, axis));

        if (distance > rA + rB) {
            // A separating axis was found, there is no collision
            return {};
        }

        // This is not a separating axis, calculate the overlap
        float overlap = (rA + rB) - distance;
        if (overlap < minOverlap) {
            minOverlap = overlap;
            smallestAxis = axis;
        }
    }

    // No separating axis found, a collision has occurred
    CollisionInfo result;
    result.hasCollided = true;
    
    // Ensure the MTV always pushes the player away from the object
    if (dotproduct3(objectCenter - playerCenter, smallestAxis) > 0) {
        smallestAxis = -smallestAxis;
    }
    
    result.mtv = smallestAxis * minOverlap;
    
    // FINAL SAFETY CHECK: Prevent NaN or infinity values from ever leaving this function
    if (std::isnan(result.mtv.x) || std::isinf(result.mtv.x)) {
        return {}; // Return a safe, non-colliding result
    }

    return result;
}

int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow* window;
    window = glfwCreateWindow(800, 600, "Strikepoint", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }
    
    // Esconde o mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    glfwSwapInterval(0); // Desabilita VSync, para que o programa rode a 60 FPS ou mais
    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Carregamos duas imagens para serem utilizadas como textura
    LoadTextureImage("../../data/textures/Type02/Tex_0010_1.png");      // TextureImage0
    LoadTextureImage("../../data/textures/Type02/Tex_0010_1.png"); // TextureImage1
    LoadTextureImage("../../data/textures/worn_tile_floor_diff_1k.jpg"); // TextureImage2
    LoadTextureImage("../../data/textures/Type01/Head.png"); // TextureImage3
    LoadTextureImage("../../data/textures/Type01/Body.png"); // TextureImage4
    LoadTextureImage("../../data/textures/Type01/Lower.png"); // TextureImage5
    LoadTextureImage("../../data/textures/concrete_wall_007_diff_1k.jpg"); // TextureImage6
    LoadTextureImage("../../data/textures/bitumen_diff_1k.jpg"); // TextureImage7
    LoadTextureImage("../../data/textures/granular_concrete_diff_1k.jpg"); // TextureImage8
    LoadTextureImage("../../data/textures/Image_38.png"); // TextureImage9
    LoadTextureImage("../../data/textures/Image_13.png"); // TextureImage10
    LoadTextureImage("../../data/textures/Image_17.png"); // TextureImage11
    LoadTextureImage("../../data/textures/Image_20.png"); // TextureImage12
    LoadTextureImage("../../data/textures/Image_0.png"); // TextureImage13
    LoadTextureImage("../../data/textures/Image_23.png"); // TextureImage14
    LoadTextureImage("../../data/textures/Image_3.png"); // TextureImage15
    LoadTextureImage("../../data/textures/Image_6.png"); // TextureImage16
    LoadTextureImage("../../data/textures/Image_26.png"); // TextureImage17
    LoadTextureImage("../../data/textures/Image_29.png"); // TextureImage18
    LoadTextureImage("../../data/textures/Image_10.png"); // TextureImage19
    LoadTextureImage("../../data/textures/Image_32.png"); // TextureImage20
    LoadTextureImage("../../data/textures/Image_35.png"); // TextureImage21
    LoadTextureImage("../../data/textures/muzzleflash.png"); // TextureImage22
    LoadTextureImage("../../data/textures/pngegg.png"); // TextureImage23
    LoadTextureImage("../../data/textures/covered_car_diff_1k.jpg"); // TextureImage24
    // Construímos a representação de objetos geométricos através de malhas de triângulos

    ObjModel planemodel("../../data/plane.obj");
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    ObjModel car("../../data/covered_car_1k.obj");
    ComputeNormals(&car);
    BuildTrianglesAndAddToVirtualScene(&car);

    const auto& car_obj = g_VirtualScene[car.shapes[1].name];

    ObjModel barricade("../../data/concrete_barrier.obj");
    ComputeNormals(&barricade);
    BuildTrianglesAndAddToVirtualScene(&barricade);

    glm::vec3 total_bbox_min(std::numeric_limits<float>::max());
    glm::vec3 total_bbox_max(std::numeric_limits<float>::min());

    const auto& scene_obj = g_VirtualScene[barricade.shapes[1].name];
    barricade_bbox_min = scene_obj.bbox_min;
    barricade_bbox_max = scene_obj.bbox_max;

    ObjModel player("../../data/character.obj");
    ComputeNormals(&player);
    BuildTrianglesAndAddToVirtualScene(&player);

    g_BarricadePositions.clear();
    for (int i = 0; i < 20; i++) {
        g_BarricadePositions.push_back(glm::vec3(((rand() % 200) - 100) / 5.0f, 0.0f, ((rand() % 200) - 100) / 5.0f));
        g_BarricadeRotation.push_back(((rand() % 360) - 180) / 180.0f * 3.141592); // Rotação aleatória entre -180 e 180 graus
    }
    g_CarPositions.clear();
    for (int i = 0; i < 5; i++) {
        g_CarPositions.push_back(glm::vec3(((rand() % 180) - 90) / 5.0f, 0.0f, ((rand() % 180) - 90) / 5.0f));
        g_CarRotation.push_back(((rand() % 360) - 180) / 180.0f * 3.141592); // Rotação aleatória entre -180 e 180 graus
    }

    ObjModel enemymodel("../../data/Soldier.obj");
    ComputeNormals(&enemymodel);
    BuildTrianglesAndAddToVirtualScene(&enemymodel);

    const auto& head_obj = g_VirtualScene[enemymodel.shapes[0].name];
    const auto& body_obj = g_VirtualScene[enemymodel.shapes[3].name];
    const auto& legs_obj = g_VirtualScene[enemymodel.shapes[4].name];
    
    Physics::ENEMY_HEAD_HITBOX.offset = (head_obj.bbox_min + head_obj.bbox_max) * 0.5f;
    Physics::ENEMY_HEAD_HITBOX.size   = head_obj.bbox_max - head_obj.bbox_min;

    Physics::ENEMY_BODY_HITBOX.offset = (body_obj.bbox_min + body_obj.bbox_max) * 0.5f;
    Physics::ENEMY_BODY_HITBOX.size   = body_obj.bbox_max - body_obj.bbox_min;

    Physics::ENEMY_LEGS_HITBOX.offset = (legs_obj.bbox_min + legs_obj.bbox_max) * 0.5f;
    Physics::ENEMY_LEGS_HITBOX.size   = legs_obj.bbox_max - legs_obj.bbox_min;

    // FONTE: Função feita com ajuda da IA Gemini 2.5 Pro
    // --- Calculate a single physics bounding box for the entire enemy ---
    g_EnemyPhysicsBboxMin = glm::min(
        Physics::ENEMY_LEGS_HITBOX.offset - Physics::ENEMY_LEGS_HITBOX.size * 0.5f,
        Physics::ENEMY_BODY_HITBOX.offset - Physics::ENEMY_BODY_HITBOX.size * 0.5f
    );
    g_EnemyPhysicsBboxMin = glm::min(
        g_EnemyPhysicsBboxMin,
        Physics::ENEMY_HEAD_HITBOX.offset - Physics::ENEMY_HEAD_HITBOX.size * 0.5f
    );

    g_EnemyPhysicsBboxMax = glm::max(
        Physics::ENEMY_LEGS_HITBOX.offset + Physics::ENEMY_LEGS_HITBOX.size * 0.5f,
        Physics::ENEMY_BODY_HITBOX.offset + Physics::ENEMY_BODY_HITBOX.size * 0.5f
    );
    g_EnemyPhysicsBboxMax = glm::max(
        g_EnemyPhysicsBboxMax,
        Physics::ENEMY_HEAD_HITBOX.offset + Physics::ENEMY_HEAD_HITBOX.size * 0.5f
    );


    // --- Calculate a single physics bounding box for the entire enemy ---
    g_EnemyPhysicsBboxMin = glm::min(
        Physics::ENEMY_LEGS_HITBOX.offset - Physics::ENEMY_LEGS_HITBOX.size * 0.5f,
        Physics::ENEMY_BODY_HITBOX.offset - Physics::ENEMY_BODY_HITBOX.size * 0.5f
    );
    g_EnemyPhysicsBboxMin = glm::min(
        g_EnemyPhysicsBboxMin,
        Physics::ENEMY_HEAD_HITBOX.offset - Physics::ENEMY_HEAD_HITBOX.size * 0.5f
    );

    g_EnemyPhysicsBboxMax = glm::max(
        Physics::ENEMY_LEGS_HITBOX.offset + Physics::ENEMY_LEGS_HITBOX.size * 0.5f,
        Physics::ENEMY_BODY_HITBOX.offset + Physics::ENEMY_BODY_HITBOX.size * 0.5f
    );
    g_EnemyPhysicsBboxMax = glm::max(
        g_EnemyPhysicsBboxMax,
        Physics::ENEMY_HEAD_HITBOX.offset + Physics::ENEMY_HEAD_HITBOX.size * 0.5f
    );

    ObjModel muzzleFlashModel("../../data/muzzleflash.obj");
    ComputeNormals(&muzzleFlashModel);
    BuildTrianglesAndAddToVirtualScene(&muzzleFlashModel);

    // Inicializa os inimigos
    g_Enemies.clear();

    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // habilitar blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Habilitamos o Backface Culling. Veja slides 8-13 do documento Aula_02_Fundamentos_Matematicos.pdf, slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf e slides 112-123 do documento Aula_14_Laboratorio_3_Revisao.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Escolher tamanho da crosshair
    float crosshair_length_pixels = 30.0f;
    float crosshair_thickness_pixels = 4.0f;

    // Ajustar o tamanho da crosshair de acordo com a janela para manter mesmos pixels
    float half_len = (crosshair_length_pixels / 2.0f) / (float)window_height;
    float half_thick = (crosshair_thickness_pixels / 2.0f) / (float)window_height;
    float half_len_x = half_len / g_ScreenRatio;        
    float half_thick_x = half_thick / g_ScreenRatio;

    float crosshair_vertices[] = {
        // Barra horizontal (2 triângulos)
        -half_len_x, -half_thick, 0.0f, 1.0f,

        -half_len_x,  half_thick, 0.0f, 1.0f,

        half_len_x,  half_thick, 0.0f, 1.0f,

        -half_len_x, -half_thick, 0.0f, 1.0f,

        half_len_x,  half_thick, 0.0f, 1.0f,

        half_len_x, -half_thick, 0.0f, 1.0f,

        // Barra vertical (2 triângulos)
        -half_thick_x, -half_len, 0.0f, 1.0f,

        -half_thick_x,  half_len, 0.0f, 1.0f,

        half_thick_x,  half_len, 0.0f, 1.0f,

        -half_thick_x, -half_len, 0.0f, 1.0f,

        half_thick_x,  half_len, 0.0f, 1.0f,

        half_thick_x, -half_len, 0.0f, 1.0f,
    };

    GLuint crosshairVAO, crosshairVBO;
    glGenVertexArrays(1, &crosshairVAO);
    glGenBuffers(1, &crosshairVBO);

    glBindVertexArray(crosshairVAO);
    glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshair_vertices), crosshair_vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Ficamos em um loop infinito, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(g_GpuProgramID);


        float camera_side_offset = 0.5f; // Distância lateral
        float target_distance = 10.0f; // Distância até o ponto alvo (para onde a câmera está olhando)

        glm::vec3 character_pos = glm::vec3(character[3]);
        // Computamos a posição da câmera utilizando coordenadas esféricas.  As
        // variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
        // controladas pelo mouse do usuário. Veja as funções CursorPosCallback()
        // e ScrollCallback().
        float r = g_CameraDistance;
        float y = r*sin(g_CameraPhi);
        float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);
        float look_vertical = sin(g_CameraPhi);

        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        ProcessInput(window, character_position, deltaTime);

        Physics::ApplyPlayerPhysics(character_position);

        // FONTE: Função feita com ajuda da IA Gemini 2.5 Pro
        if (g_MuzzleFlash.active) {
            g_MuzzleFlash.lifetime += deltaTime;
            // Calculate fade factor (1.0 to 0.0)
            float fadeFactor = 1.0f - (g_MuzzleFlash.lifetime / g_MuzzleFlash.max_lifetime);
            if (g_MuzzleFlash.lifetime >= g_MuzzleFlash.max_lifetime) {
                g_MuzzleFlash.active = false;
            }
            // Pass values to shader
            glUniform1i(g_muzzle_flash_active_uniform, g_MuzzleFlash.active ? 1 : 0);
            glUniform3fv(g_muzzle_flash_position_uniform, 1, glm::value_ptr(g_MuzzleFlash.position));
            glUniform3fv(g_muzzle_flash_color_uniform, 1, glm::value_ptr(g_MuzzleFlash.color));
            glUniform1f(g_muzzle_flash_intensity_uniform, g_MuzzleFlash.intensity * fadeFactor);
        } else {
            // If not active, tell shader to ignore muzzle flash
            glUniform1i(g_muzzle_flash_active_uniform, 0);
        }
        
        // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
        // Veja slides 195-227 e 229-234 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        g_CameraFront = glm::vec4(cos(g_CameraPhi)*sin(g_CameraTheta), -sin(g_CameraPhi), cos(g_CameraPhi)*cos(g_CameraTheta), 0.0f);
        g_CameraFront = g_CameraFront / norm(g_CameraFront); // Normalizamos o vetor "frente" da câmera
        glm::vec3 character_forward = glm::vec3(sin(g_CameraTheta), 0.0f, cos(g_CameraTheta));  //GPT
        glm::vec3 character_front = glm::vec3(cos(g_CameraPhi)*sin(g_CameraTheta), -sin(g_CameraPhi), cos(g_CameraPhi)*cos(g_CameraTheta));
        glm::vec3 character_right  = glm::vec3(-cos(g_CameraTheta), 0.0f, sin(g_CameraTheta));  //GPT
        glm::vec3 camera_position = character_position - character_front * g_CameraDistance + character_right * camera_side_offset + glm::vec3(0.0f, camera_height, 0.0f);
        if(FirstPerson){
            camera_position = character_position + glm::vec3(0.0f, camera_height, 0.0f);
        }

        glm::vec3 camera_lookat = character_position + character_front * target_distance + glm::vec3(0.0f, 1.0f - target_distance*look_vertical/2, 0.0f); // levemente para cima

        camera_position_c  = glm::vec4(camera_position,1.0f); // Ponto "c", centro da câmera
        glm::vec4 camera_lookat_l    = glm::vec4(camera_lookat,1.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando
        camera_view_vector = (camera_lookat_l - camera_position_c)/norm(camera_lookat_l - camera_position_c); // Vetor "view", sentido para onde a câmera está virada
        glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)
        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slides 2-14, 184-190 e 236-242 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);
        if(FirstPerson){
            view = Matrix_Camera_View(camera_position_c, g_CameraFront, camera_up_vector);
        }
        
        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;


        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -70.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROJEÇÃO ORTOGRÁFICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem
        
        

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

       // Lógica para recoil
       if (g_RecoilTimer > 0.0f) {
            g_RecoilTimer -= deltaTime;

            float t = g_RecoilTimer / RECOIL_DURATION;
            float recoil_factor = t * t;
            g_RecoilOffset = -g_CameraFront * RECOIL_STRENGTH * recoil_factor;

        } else {
            g_RecoilOffset = glm::vec3(0.0f);
        }

        // Spawn de inimigos
        enemyTime = currentFrame - lastEnemyTime;
        if (enemyTime > 5.0f) { // Atualiza inimigos a cada 5 segundos
            lastEnemyTime = currentFrame;
            if (g_Enemies.size() < 6) { // Cria 5 inimigos
                Enemy enemy;
                enemy.position = glm::vec3(((rand() % 200) - 100) / 5.0f, 0.0f, ((rand() % 200) - 100) / 5.0f);
                enemy.health = 100;
                enemy.speed = 0.1f + ((rand() % 100) / 500.0f); // Velocidade aleatória
                GenerateBezierCurve(enemy);
                g_Enemies.push_back(enemy);
            }
        }

        // Spawn de barricadas
        for (size_t i = 0; i < g_BarricadePositions.size(); i++) {
            const auto& position = g_BarricadePositions[i];
            float rotation = g_BarricadeRotation[i];

            model = Matrix_Translate(position.x, position.y, position.z) * Matrix_Rotate_Y(rotation) * Matrix_Scale(1.2f, 1.2f, 1.2f);
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));

            for (int j = 0; j < 5; j++) {
                if (j == 1) {
                    glUniform1i(g_object_id_uniform, BARRICADE);
                }
                else {
                    glUniform1i(g_object_id_uniform, METAL);
                }
                DrawVirtualObject(barricade.shapes[j].name.c_str());
            }
            if (ShowHitBoxes)
                DrawBoundingBox(barricade_bbox_min, barricade_bbox_max, model, view, projection);
        }

        if (!FirstPerson){
            for (size_t j = 0; j < player.shapes.size(); ++j) {
                model = Matrix_Translate(character_position.x, character_position.y, character_position.z) * Matrix_Rotate_Y(g_CameraTheta);
                glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                switch(j){
                    case 0:
                        glUniform1i(g_object_id_uniform, material);
                        break;
                    case 1:
                        glUniform1i(g_object_id_uniform, pol_filter);
                        break;
                    case 2:
                        glUniform1i(g_object_id_uniform, pol_gas_mask);
                        break;
                    case 3:
                        glUniform1i(g_object_id_uniform, glass);
                        break;
                    case 4:
                        glUniform1i(g_object_id_uniform, pol_helmet);
                        break;
                    case 5:
                        model = Matrix_Translate(g_RecoilOffset.x, g_RecoilOffset.y, g_RecoilOffset.z) * Matrix_Translate(character_position.x, character_position.y, character_position.z) * Matrix_Rotate_Y(g_CameraTheta);
                        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                        glUniform1i(g_object_id_uniform, WZ96_Beryl);
                        break;
                    case 6:
                        glUniform1i(g_object_id_uniform, boot_war1);
                        break;
                    case 7:
                        glUniform1i(g_object_id_uniform, magb);
                        break;
                    case 8:
                        glUniform1i(g_object_id_uniform, magb);
                        break;
                    case 9:
                        glUniform1i(g_object_id_uniform, magb);
                        break;
                    case 10:
                        model = Matrix_Translate(g_RecoilOffset.x, g_RecoilOffset.y, g_RecoilOffset.z) * Matrix_Translate(character_position.x, character_position.y, character_position.z) * Matrix_Rotate_Y(g_CameraTheta);
                        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                        glUniform1i(g_object_id_uniform, pol_bproof);
                        break;
                    case 11:
                        model = Matrix_Translate(g_RecoilOffset.x, g_RecoilOffset.y, g_RecoilOffset.z) * Matrix_Translate(character_position.x, character_position.y, character_position.z) * Matrix_Rotate_Y(g_CameraTheta);
                        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                        glUniform1i(g_object_id_uniform, pol_hand);
                        break;
                    case 12:
                        glUniform1i(g_object_id_uniform, pol_head);
                        break;
                    case 13:
                        model = Matrix_Translate(g_RecoilOffset.x, g_RecoilOffset.y, g_RecoilOffset.z) * Matrix_Translate(character_position.x, character_position.y, character_position.z) * Matrix_Rotate_Y(g_CameraTheta);
                        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                        glUniform1i(g_object_id_uniform, pol_jaket);
                        break;
                    case 14:
                        glUniform1i(g_object_id_uniform, pol_pants);
                        break;
                    case 15:
                        glUniform1i(g_object_id_uniform, RGZ89);
                        break;
                    case 16:
                        glUniform1i(g_object_id_uniform, RGZ89);
                        break;
                    case 17:
                        glUniform1i(g_object_id_uniform, WZ96_Beryl);
                        break;
                    case 18:
                        glUniform1i(g_object_id_uniform, WZ96_Beryl);
                        break;
                    case 19:
                        glUniform1i(g_object_id_uniform, WZ96_Beryl);
                        break;
                    default:
                        break;
                }
                DrawVirtualObject(player.shapes[j].name.c_str());
            }
        }
        else{
            model = Matrix_Translate(g_RecoilOffset.x, g_RecoilOffset.y, g_RecoilOffset.z) * Matrix_Translate(character_position.x, character_position.y, character_position.z) * Matrix_Rotate_Y(g_CameraTheta) * Matrix_Translate(0.0f, camera_height, 0.0f) * Matrix_Rotate_X(g_CameraPhi) * Matrix_Translate(0.0f, -camera_height, 0.0f);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            // Renderizar o personagem em primeira pessoa
            glUniform1i(g_object_id_uniform, pol_hand);
            DrawVirtualObject("pol_hand_0");
            glUniform1i(g_object_id_uniform, WZ96_Beryl);
            DrawVirtualObject("WZ96_Beryl_0");
            glUniform1i(g_object_id_uniform, pol_jaket);
            DrawVirtualObject("pol_jaket_0");
        }

        // Mostra hitbox do player
        if (ShowHitBoxes)
            {
                // Define the player's local bounding box based on its physics size
                glm::vec3 player_local_bbox_min = glm::vec3(-g_PlayerSize.x / 2.0f, 0.0f, -g_PlayerSize.z / 2.0f);
                glm::vec3 player_local_bbox_max = glm::vec3(g_PlayerSize.x / 2.0f, g_PlayerSize.y, g_PlayerSize.z / 2.0f);
                
                // Draw the bounding box using the player's current transformation
                DrawBoundingBox(player_local_bbox_min, player_local_bbox_max, model, view, projection);
            }

        // Spawn carros
        for (size_t i = 0; i < g_CarPositions.size(); ++i) {
            model = Matrix_Translate(g_CarPositions[i].x, g_CarPositions[i].y, g_CarPositions[i].z) * Matrix_Rotate_Y(g_CarRotation[i]) * Matrix_Scale(1.25f, 1.25f, 1.25f);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, COVERED_CAR);
            for (size_t i = 0; i < car.shapes.size(); i++) {
                DrawVirtualObject(car.shapes[i].name.c_str());
            }
            if (ShowHitBoxes) {
                DrawBoundingBox(Physics::CAR_TOP_HITBOX.offset - Physics::CAR_TOP_HITBOX.size * 0.5f,
                            Physics::CAR_TOP_HITBOX.offset + Physics::CAR_TOP_HITBOX.size * 0.5f,
                            model, view, projection);
                DrawBoundingBox(Physics::CAR_BOTTOM_HITBOX.offset - Physics::CAR_BOTTOM_HITBOX.size * 0.5f,
                            Physics::CAR_BOTTOM_HITBOX.offset + Physics::CAR_BOTTOM_HITBOX.size * 0.5f,
                            model, view, projection);
            }
        }


        model = Matrix_Translate(0.0, 0.0f, 1.0f)*Matrix_Scale(1.0f, 1.0f, 1.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));

        // Atualiza e renderiza os inimigos
        for (auto& enemy : g_Enemies)
        {
            if (enemy.health <= 0) {
                Kills++;
                continue; // Skip dead enemies
            }
            // FONTE: Função feita com ajuda da IA Gemini 2.5 Pro
            // --- 1. Enemy AI Movement ---
            // The AI calculates where the enemy wants to go
            enemy.bezier_t += deltaTime * enemy.speed;
            if (enemy.bezier_t >= 1.0f) {
                enemy.bezier_t = 0.0f;
                GenerateBezierCurve(enemy);
            }
            glm::vec3 desired_position = CalculateBezierPoint(enemy.bezier_t, enemy.p0, enemy.p1, enemy.p2, enemy.p3);
            enemy.position = desired_position;

            // --- 2. Check for Collision with Player ---
            // Define the enemy's current transform for the collision check
            glm::vec3 look_direction = CalculateBezierPoint(enemy.bezier_t + 0.01f, enemy.p0, enemy.p1, enemy.p2, enemy.p3) - enemy.position;
            float angle = atan2(look_direction.x, look_direction.z);
            glm::mat4 enemy_model_matrix = Matrix_Translate(enemy.position.x, enemy.position.y, enemy.position.z) * Matrix_Rotate_Y(angle);

            // Define the player's bounding box
            glm::vec3 player_bbox_min = character_position - glm::vec3(g_PlayerSize.x / 2.0f, 0.0f, g_PlayerSize.z / 2.0f);
            glm::vec3 player_bbox_max = character_position + glm::vec3(g_PlayerSize.x / 2.0f, g_PlayerSize.y, g_PlayerSize.z / 2.0f);
            CollisionInfo player_collision = FindCollision(player_bbox_min, player_bbox_max, enemy_model_matrix, g_EnemyPhysicsBboxMin, g_EnemyPhysicsBboxMax);
            // Morte do inimigo
            // Fim do trecho com IA
            if (player_collision.hasCollided) {
                // Teleporta player para o centro do mapa no ar
                character_position = glm::vec3(0.0f, 3.0f, 0.0f);
                g_CharacterVerticalVelocity = 0.0f;
                // reseta eliminações e munição
                Kills = 0; 
                Ammo = 30;
            }
            model = Matrix_Translate(enemy.position.x, enemy.position.y, enemy.position.z) * Matrix_Rotate_Y(angle);
            enemy.model_matrix = model;
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            for (size_t i = 0; i < enemymodel.shapes.size(); ++i) {
                glUniform1i(g_object_id_uniform, ENEMY_HEAD + i);
                DrawVirtualObject(enemymodel.shapes[i].name.c_str());
            }
            
            if (ShowHitBoxes) {
            // Desenha as hitboxes do inimigo
            DrawBoundingBox(Physics::ENEMY_LEGS_HITBOX.offset - Physics::ENEMY_LEGS_HITBOX.size * 0.5f,
                        Physics::ENEMY_LEGS_HITBOX.offset + Physics::ENEMY_LEGS_HITBOX.size * 0.5f,
                        model, view, projection);
            DrawBoundingBox(Physics::ENEMY_BODY_HITBOX.offset - Physics::ENEMY_BODY_HITBOX.size * 0.5f,
                        Physics::ENEMY_BODY_HITBOX.offset + Physics::ENEMY_BODY_HITBOX.size * 0.5f,
                        model, view, projection);
            DrawBoundingBox(Physics::ENEMY_HEAD_HITBOX.offset - Physics::ENEMY_HEAD_HITBOX.size * 0.5f,
                        Physics::ENEMY_HEAD_HITBOX.offset + Physics::ENEMY_HEAD_HITBOX.size * 0.5f,
                        model, view, projection);
            }
        }
        
        // Desenhamos o plano do chão
        model = Matrix_Translate(0.0f,0.0f,0.0f) * Matrix_Scale(20.0f, 1.0f, 20.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PLANE);
        DrawVirtualObject("the_plane");

        model = Matrix_Translate(0.0f,5.0f,20.0f) * Matrix_Rotate_X(-3.141592f/2.0f) * Matrix_Scale(20.0f, 1.0f, 5.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WALL);
        DrawVirtualObject("the_plane");

        model = Matrix_Translate(0.0f,5.0f,-20.0f) * Matrix_Rotate_X(3.141592f/2.0f) * Matrix_Scale(20.0f, 1.0f, 5.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WALL);
        DrawVirtualObject("the_plane");

        model = Matrix_Translate(20.0f,5.0f,0.0f) * Matrix_Rotate_Y(3.141592f/2.0f) * Matrix_Rotate_X(-3.141592f/2.0f) * Matrix_Scale(20.0f, 1.0f, 5.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WALL);
        DrawVirtualObject("the_plane");

        model = Matrix_Translate(-20.0f,5.0f,0.0f) * Matrix_Rotate_Y(3.141592f/2.0f) * Matrix_Rotate_X(3.141592f/2.0f) * Matrix_Scale(20.0f, 1.0f, 5.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WALL);
        DrawVirtualObject("the_plane");

        model = Matrix_Translate(0.0f,10.0f,0.0f) * Matrix_Scale(20.0f, 1.0f, 20.0f) * Matrix_Rotate_X(-3.141592f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, ROOF);
        DrawVirtualObject("the_plane");

        if (g_MuzzleFlash.active) {

            float fadeFactor = 1.0f - (g_MuzzleFlash.lifetime / g_MuzzleFlash.max_lifetime);
            
            glm::mat4 flashModel;
            
            if (FirstPerson) {
                flashModel = Matrix_Translate(g_MuzzleFlash.position.x, g_MuzzleFlash.position.y, g_MuzzleFlash.position.z)
                        * Matrix_Rotate_Y(g_CameraTheta - 2.8f)
                        * Matrix_Rotate_X(-g_CameraPhi)
                        * Matrix_Rotate_Z(3.1415f)
                        * Matrix_Scale(-0.1f * fadeFactor, -0.1f * fadeFactor, 0.1f * fadeFactor);
            } else {
                // Third person muzzle flash
                flashModel = Matrix_Translate(g_MuzzleFlash.position.x, g_MuzzleFlash.position.y, g_MuzzleFlash.position.z)
                        * Matrix_Rotate_Y(g_CameraTheta)
                        * Matrix_Rotate_X(-3.14159f)
                        * Matrix_Scale(0.1f * fadeFactor, 0.2f * fadeFactor, 0.1f * fadeFactor);
            }
            
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(flashModel));

            // Desabilita depth writing e habilita additive blending para glow effect
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDisable(GL_DEPTH_TEST);
            
            glUniform1i(g_object_id_uniform, MUZZLE_FLASH);
            DrawVirtualObject("cube11_cube11_auv");
            
            glEnable(GL_DEPTH_TEST);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_TRUE);
        }

        // Loop for para renderizar particulas, FONTE: Função feita com ajuda da IA Gemini 2.5 Pro
        for (auto particle = g_BloodSplatters.begin(); particle != g_BloodSplatters.end(); ) {
            particle->lifetime += deltaTime;
            if (particle->lifetime >= particle->max_lifetime) {
                particle = g_BloodSplatters.erase(particle);
            } else {
                float lifePercent = particle->lifetime / particle->max_lifetime;
                float fadeAlpha = 1.0f - lifePercent; // Fade out 
                float currentSize = particle->size * (0.1f + pow(lifePercent, 0.5f) * 1.0f); // Cresce quadraticamente
                // --- Billboard Logic (to make the 2D quad always face the camera) ---
                glm::vec4 look = (glm::vec4(camera_position - particle->position, 0.0f))/norm(glm::vec4(camera_position - particle->position, 0.0f));
                glm::vec4 right = (crossproduct(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), look))/norm(crossproduct(glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), look));
                glm::vec4 up = (crossproduct(look, right))/norm(crossproduct(look, right));
                // Apply a random rotation to the particle's plane
                float c = cos(particle->rotation);
                float s = sin(particle->rotation);
                glm::vec4 right_rot = right * c + up * s;
                glm::vec4 up_rot = up * c - right * s;
                // Create the model matrix for the particle
                glm::mat4 bloodModel = Matrix_Translate(particle->position.x, particle->position.y, particle->position.z);
                bloodModel[0] = glm::vec4(right_rot * currentSize);
                bloodModel[1] = glm::vec4(up_rot * currentSize);
                bloodModel[2] = glm::vec4(look * 0.1f); // Small depth to avoid z-fighting
            
                glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(bloodModel));
                // --- Set Render State for Transparency ---
                glDepthMask(GL_FALSE); // Don't write to depth buffer
                glDisable(GL_DEPTH_TEST); // Particles can draw over each other
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                
                glUniform1i(g_object_id_uniform, BLOOD_SPLATTER);
                glUniform1f(g_blood_alpha_uniform, fadeAlpha);
                // --- Create and Draw the Particle Quad ---
                float half_size = currentSize / 2.0f;
                float vertices[] = {
                    // Positions          // Texture Coords
                -half_size, -half_size, 0.0f, 0.0f, 0.0f,  // Bottom-left
                    half_size, -half_size, 0.0f, 1.0f, 0.0f,  // Bottom-right
                    half_size,  half_size, 0.0f, 1.0f, 1.0f,  // Top-right
                -half_size,  half_size, 0.0f, 0.0f, 1.0f   // Top-left
                };
                
                GLuint indices[] = {0, 1, 2, 2, 3, 0};
                GLuint VAO, VBO, EBO;
                glGenVertexArrays(1, &VAO);
                glGenBuffers(1, &VBO);
                glGenBuffers(1, &EBO);
                
                glBindVertexArray(VAO);
                glBindBuffer(GL_ARRAY_BUFFER, VBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
                // Position attribute
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(0);
                // Texture coord attribute
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
                glEnableVertexAttribArray(2);
                // Draw the particle
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
                // --- Clean up ---
                glDeleteVertexArrays(1, &VAO);
                glDeleteBuffers(1, &VBO);
                glDeleteBuffers(1, &EBO);
                // --- Restore Render State ---
                glEnable(GL_DEPTH_TEST);
                glDepthMask(GL_TRUE);
                // Increment the iterator manually since we didn't erase
                particle++;
            }
        }

        g_Enemies.erase(std::remove_if(g_Enemies.begin(), g_Enemies.end(), [](const Enemy& enemy) {
            return enemy.health <= 0;
        }), g_Enemies.end());

        // Atualiza todos os projéteis
        float currentTime = (float)glfwGetTime();
        Physics::UpdateProjectiles(currentTime);


        // Renderiza todos os projéteis
        for (const auto& projectile : Physics::Projectiles)
        {
            float age = currentTime - projectile.creation_time;
            float fade_alpha = 1.0f - (age / Physics::PROJECTILE_LIFETIME);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            glLineWidth(5.0f);

            // Cria vertices de linha usando a variável 'projectile'
            float line_vertices[] = {
                projectile.start_position.x, projectile.start_position.y, projectile.start_position.z, 1.0f,
                projectile.end_position.x,   projectile.end_position.y,   projectile.end_position.z,   1.0f
            };
            
            GLuint lineVAO, lineVBO;
            glGenVertexArrays(1, &lineVAO);
            glGenBuffers(1, &lineVBO);
            
            glBindVertexArray(lineVAO);
            glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(line_vertices), line_vertices, GL_STATIC_DRAW);
            
            glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(Matrix_Identity()));
            glUniformMatrix4fv(g_view_uniform, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(g_projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));
            glUniform1i(g_object_id_uniform, PROJECTILE_LINE);
            
            glUniform1f(g_projectile_alpha_uniform, fade_alpha);
            
            glDrawArrays(GL_LINES, 0, 2);
            
            glDeleteBuffers(1, &lineVBO);
            glDeleteVertexArrays(1, &lineVAO);
            glLineWidth(1.0f);
            
            glDisable(GL_BLEND);
        }

        // Desenha a crosshair 2D no centro da tela
        glDisable(GL_DEPTH_TEST);
        
        // Desenha a crosshair 2D no centro da tela
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        // Use uma matriz model identidade (sem transformações 3D)
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(Matrix_Identity()));
        glUniformMatrix4fv(g_view_uniform, 1, GL_FALSE, glm::value_ptr(Matrix_Identity()));
        glUniformMatrix4fv(g_projection_uniform, 1, GL_FALSE, glm::value_ptr(Matrix_Identity()));
        glUniform1i(g_object_id_uniform, CROSSHAIR);
        // Desenha
        glBindVertexArray(crosshairVAO);
        glDrawArrays(GL_TRIANGLES, 0, 12);
        glBindVertexArray(0);

        // Restaura estado
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        


        TextRendering_ShowAmmo(window);

        TextRendering_ShowKills(window);

        TextRendering_ShowReload(window);
        // Imprimimos na tela os ângulos de Euler que controlam a rotação do
        // terceiro cubo.
        //TextRendering_ShowEulerAngles(window);

        // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
        //TextRendering_ShowProjection(window);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model"); // Variável da matriz "model"
    g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view"); // Variável da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id"); // Variável "object_id" em shader_fragment.glsl
    g_bbox_min_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_max");
    g_projectile_alpha_uniform = glGetUniformLocation(g_GpuProgramID, "projectile_alpha");
    g_blood_alpha_uniform = glGetUniformLocation(g_GpuProgramID, "blood_alpha");
    g_muzzle_flash_active_uniform = glGetUniformLocation(g_GpuProgramID, "muzzle_flash_active");
    g_muzzle_flash_position_uniform = glGetUniformLocation(g_GpuProgramID, "muzzle_flash_position");
    g_muzzle_flash_color_uniform = glGetUniformLocation(g_GpuProgramID, "muzzle_flash_color");
    g_muzzle_flash_intensity_uniform = glGetUniformLocation(g_GpuProgramID, "muzzle_flash_intensity");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(g_GpuProgramID);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage4"), 4);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage5"), 5);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage6"), 6);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage7"), 7);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage8"), 8);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage9"), 9);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage10"), 10);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage11"), 11);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage12"), 12);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage13"), 13);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage14"), 14);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage15"), 15);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage16"), 16);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage17"), 17);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage18"), 18);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage19"), 19);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage20"), 20);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage21"), 21);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage22"), 22);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage23"), 23);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage24"), 24);
    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            const glm::vec4  n = crossproduct(b-a,c-a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados 
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);
    window_height = height;
    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
        if (Ammo > 0)
        {
            Physics::HandleShooting(
                character_position,
                camera_position_c, 
                camera_view_vector,
                g_CameraFront,
                FirstPerson,
                g_Enemies
            );
            g_RecoilTimer = RECOIL_DURATION;
            Ammo--; // Decrementa munição
            
            // Ativa o efeito de flash do cano da arma
            g_MuzzleFlash.active = true;
            g_MuzzleFlash.lifetime = 0.0f;
            
            
            if (FirstPerson) {
                glm::vec3 forward = glm::vec3(cos(g_CameraPhi)*sin(g_CameraTheta), -sin(g_CameraPhi), cos(g_CameraPhi)*cos(g_CameraTheta));
                // Posiciona o muzzle flash na ponta da arma com offset
                g_MuzzleFlash.position = character_position + glm::vec3(0.0f, camera_height, 0.0f) + forward * 0.5f +  glm::vec3(0.0f, -0.1f, 0.0f);
            } else {
                glm::vec3 gunOffset = glm::vec3(-0.05f, 1.55f, 1.0f);
                glm::mat4 gunTransform = Matrix_Translate(g_RecoilOffset.x, g_RecoilOffset.y, g_RecoilOffset.z) 
                            * Matrix_Translate(character_position.x, character_position.y, character_position.z) 
                            * Matrix_Rotate_Y(g_CameraTheta);
                            
                glm::vec4 worldMuzzlePos = gunTransform * glm::vec4(gunOffset, 1.0f);
                g_MuzzleFlash.position = glm::vec3(worldMuzzlePos);
            }
        }
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}


void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Modificado com ajuda de IA
    // Calculate mouse movement since last frame
    static bool first_mouse = true;
    if (first_mouse)
    {
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
        first_mouse = false;
    }

    float dx = xpos - g_LastCursorPosX;
    float dy = ypos - g_LastCursorPosY;

    g_CameraTheta -= 0.01f*dx;
    g_CameraPhi   += 0.01f*dy;

    float phimax = glm::radians(75.0f);
    float phimin = glm::radians(-75.0f);
    g_CameraPhi = glm::clamp(g_CameraPhi, phimin, phimax);

    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f*yoffset;
    FirstPerson = false;
    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    if (g_CameraDistance > 1.5f)
        g_CameraDistance = 1.5f; // Limite máximo de distância da câmera para a origem.
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber){
        FirstPerson = true; // Se a distância for muito pequena, utilizamos a câmera em primeira pessoa.
        g_CameraDistance = verysmallnumber;
    }
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // ======================
    // Não modifique este loop! Ele é utilizando para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    for (int i = 0; i < 10; ++i)
        if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
            std::exit(100 + i);
    // ======================

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // O código abaixo implementa a seguinte lógica:
    //   Se apertar tecla X       então g_AngleX += delta;
    //   Se apertar tecla shift+X então g_AngleX -= delta;
    //   Se apertar tecla Y       então g_AngleY += delta;
    //   Se apertar tecla shift+Y então g_AngleY -= delta;
    //   Se apertar tecla Z       então g_AngleZ += delta;
    //   Se apertar tecla shift+Z então g_AngleZ -= delta;

    float delta = 3.141592 / 16; // 22.5 graus, em radianos.

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Recarrega munição
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        Ammo = 30; // Recarrega munição
    }

    // Se o usuário apertar a tecla f, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }

    // Alterna entre mostrar e ocultar as hitboxes
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        ShowHitBoxes = !ShowHitBoxes; 
    }

}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}


// FONTE: Corrigido com ajuda de IA
void ProcessInput(GLFWwindow* window, glm::vec3& character_position, float deltaTime)
{
    // Calculate desired movement from input and apply gravity 
    glm::vec3 move_vector(0.0f);
    float speed = 5.0f;
    glm::vec3 forward = glm::vec3(sin(g_CameraTheta), 0.0f, cos(g_CameraTheta));
    glm::vec3 right   = glm::vec3(-cos(g_CameraTheta), 0.0f, sin(g_CameraTheta));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) move_vector += forward;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) move_vector -= forward;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) move_vector -= right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) move_vector += right;
    if (norm(glm::vec4(move_vector, 0.0)) > 0.0f) {
        move_vector = (move_vector/norm(glm::vec4(move_vector, 0.0f))) * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && g_IsCharacterGrounded) {
        g_CharacterVerticalVelocity = JUMP_FORCE;
    }
    g_CharacterVerticalVelocity += GRAVITY * deltaTime;
    move_vector.y = g_CharacterVerticalVelocity;
    // Apply the combined movement vector
    character_position += move_vector * deltaTime;
    // --- 2. Iteratively resolve all collisions with safety checks ---
    g_IsCharacterGrounded = false;
    for (int iter = 0; iter < 10; ++iter) 
    {
        glm::vec3 player_bbox_min = character_position - glm::vec3(g_PlayerSize.x / 2.0f, 0.0f, g_PlayerSize.z / 2.0f);
        glm::vec3 player_bbox_max = character_position + glm::vec3(g_PlayerSize.x / 2.0f, g_PlayerSize.y, g_PlayerSize.z / 2.0f);
        // Check against the floor
        if (player_bbox_min.y < 0.0f) {
            character_position.y = 0.0f;
            g_IsCharacterGrounded = true;
        }
        // Check for collisions with barricades
        for (size_t i = 0; i < g_BarricadePositions.size(); ++i) {
            glm::mat4 barricade_model_matrix = Matrix_Translate(g_BarricadePositions[i].x, g_BarricadePositions[i].y, g_BarricadePositions[i].z) * Matrix_Rotate_Y(g_BarricadeRotation[i]) * Matrix_Scale(1.2f, 1.2f, 1.2f);
            CollisionInfo info = FindCollision(player_bbox_min, player_bbox_max, barricade_model_matrix, barricade_bbox_min, barricade_bbox_max);
            if (info.hasCollided && norm(glm::vec4(info.mtv, 0.0f)) < 1.0f) { // SAFETY CHECK: Ignore huge MTVs
                character_position += info.mtv;
                if (info.mtv.y > 0.001f) g_IsCharacterGrounded = true;
            }
        }
        // Check for collisions with enemies
        for (auto& enemy : g_Enemies) {
            if (enemy.health <= 0) continue;
            CollisionInfo info = FindCollision(player_bbox_min, player_bbox_max, enemy.model_matrix, g_EnemyPhysicsBboxMin, g_EnemyPhysicsBboxMax);
            if (info.hasCollided && norm(glm::vec4(info.mtv, 0.0f)) < 1.0f) { // SAFETY CHECK: Ignore huge MTVs
                character_position += info.mtv * 0.5f;
                enemy.position     -= info.mtv * 0.5f;
                if (info.mtv.y > 0.001f) g_IsCharacterGrounded = true;
            }
        }
        // FONTE: Corrigido com IA Gemini
        // Checa colisão com carro
        for (size_t i = 0; i < g_CarPositions.size(); ++i)
        {
            glm::mat4 car_model_matrix = Matrix_Translate(g_CarPositions[i].x, g_CarPositions[i].y, g_CarPositions[i].z) * Matrix_Rotate_Y(g_CarRotation[i]) * Matrix_Scale(1.25f, 1.25f, 1.25f);
            // --- Check against the bottom hitbox ---
            glm::vec3 bottom_min_local = Physics::CAR_BOTTOM_HITBOX.offset - Physics::CAR_BOTTOM_HITBOX.size * 0.5f;
            glm::vec3 bottom_max_local = Physics::CAR_BOTTOM_HITBOX.offset + Physics::CAR_BOTTOM_HITBOX.size * 0.5f;
            CollisionInfo bottom_info = FindCollision(player_bbox_min, player_bbox_max, car_model_matrix, bottom_min_local, bottom_max_local);
            if (bottom_info.hasCollided && norm(glm::vec4(bottom_info.mtv, 0.0f)) < 1.0f)
            {
                character_position += bottom_info.mtv;
                if (bottom_info.mtv.y > 0.001f) {
                    g_IsCharacterGrounded = true;
                }
            }
            // --- Check against the top hitbox ---
            glm::vec3 top_min_local = Physics::CAR_TOP_HITBOX.offset - Physics::CAR_TOP_HITBOX.size * 0.5f;
            glm::vec3 top_max_local = Physics::CAR_TOP_HITBOX.offset + Physics::CAR_TOP_HITBOX.size * 0.5f;
            CollisionInfo top_info = FindCollision(player_bbox_min, player_bbox_max, car_model_matrix, top_min_local, top_max_local);

            if (top_info.hasCollided && norm(glm::vec4(top_info.mtv, 0.0f)) < 1.0f)
            {
                character_position += top_info.mtv;
                if (top_info.mtv.y > 0.001f) {
                    g_IsCharacterGrounded = true;
                }
            }
        }
    }
    if (g_IsCharacterGrounded) {
        g_CharacterVerticalVelocity = 0.0f;
    }
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;
    glm::vec4 p_clip = projection*p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-6*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f-9*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-10*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-14*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-15*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-16*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f-17*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-18*pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2( 0,  0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x)/(b.x-a.x), 0.0f, 0.0f, (b.x*p.x - a.x*q.x)/(b.x-a.x),
        0.0f, (q.y - p.y)/(b.y-a.y), 0.0f, (b.y*p.y - a.y*q.y)/(b.y-a.y),
        0.0f , 0.0f , 1.0f , 0.0f ,
        0.0f , 0.0f , 0.0f , 1.0f
    );

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f-22*pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f-23*pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f-24*pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f-25*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f-26*pad, 1.0f);
}



// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);
    
        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Mostra a munição no canto inferior esquerdo
void TextRendering_ShowAmmo(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    static char  buffer[20] = "Ammo: ??/30";
    static int   numchars = 2;

    numchars = snprintf(buffer, 20, "Ammo: %d/30", Ammo);

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-24*charwidth, -1.0f+3*lineheight, 2.0f);
}

// Mostra o número de eliminações em cima da tela
void TextRendering_ShowKills(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    static char  buffer[20] = "Eliminations: ??";
    static int   numchars = 2;

    numchars = snprintf(buffer, 20, "Eliminations: %d", Kills);

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 0.0f-15*charwidth, 1.0f-5*lineheight, 2.0f);
}

// Mostra que o usuário precisa recarregar quando a munição for 0
void TextRendering_ShowReload(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( Ammo == 0 )
        TextRendering_PrintString(window, "Press R to reload", 0.0f-12*charwidth, 0.0f+2*lineheight, 1.5f);
}

//FONTE: Função feita com ajuda da IA Gemini 2.5 Pro
void DrawBoundingBox(const glm::vec3& bbox_min, const glm::vec3& bbox_max, const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection)
{
    // 8 vértices do cubo
    glm::vec3 v[8] = {
        {bbox_min.x, bbox_min.y, bbox_min.z},
        {bbox_max.x, bbox_min.y, bbox_min.z},
        {bbox_max.x, bbox_max.y, bbox_min.z},
        {bbox_min.x, bbox_max.y, bbox_min.z},
        {bbox_min.x, bbox_min.y, bbox_max.z},
        {bbox_max.x, bbox_min.y, bbox_max.z},
        {bbox_max.x, bbox_max.y, bbox_max.z},
        {bbox_min.x, bbox_max.y, bbox_max.z}
    };

    GLuint indices[] = {
        0,1, 1,2, 2,3, 3,0, // base inferior
        4,5, 5,6, 6,7, 7,4, // base superior
        0,4, 1,5, 2,6, 3,7  // colunas
    };

    float vertices[24];
    for (int i = 0; i < 8; ++i) {
        glm::vec4 p = model * glm::vec4(v[i], 1.0f);
        vertices[3*i+0] = p.x;
        vertices[3*i+1] = p.y;
        vertices[3*i+2] = p.z;
    }

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Use cor fixa para a bounding box
    glUniform1i(g_object_id_uniform, -1); // Use um id especial para cor fixa no shader
    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(Matrix_Identity()));
    glUniformMatrix4fv(g_view_uniform, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(g_projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));

    // Primeira passada: normal, respeitando profundidade
    glEnable(GL_DEPTH_TEST);
    glLineWidth(2.0f);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);

    // Segunda passada: X-ray, por cima de tudo, mais transparente
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUniform1i(g_object_id_uniform, -2); // Use outro id para cor transparente no shader
    glDisable(GL_DEPTH_TEST);
    glLineWidth(1.0f);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glBindVertexArray(0);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

