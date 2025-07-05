#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// variavel para projetil
uniform float projectile_alpha;

// Identificador que define qual objeto está sendo desenhado no momento
#define PLANE 0
#define ENEMY_HEAD 1
#define ENEMY_FACE 2
#define ENEMY_EYE 3
#define ENEMY_MIDDLE 4
#define ENEMY_BOTTOM 5
#define TREE 6
#define BARRICADE 7
#define WALL 8
#define ROOF 9
#define TREE_BRANCH3 10
#define CROSSHAIR 11
#define PROJECTILE_LINE 12
#define TREE_WALL 13
uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform sampler2D TextureImage3;
uniform sampler2D TextureImage4;
uniform sampler2D TextureImage5;
uniform sampler2D TextureImage6;
uniform sampler2D TextureImage7;
uniform sampler2D TextureImage8;


// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    vec4 l = normalize(vec4(1.0,1.0,0.0,0.0));

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;

    U = texcoords.x;
    V = texcoords.y;
    // default: cor preta
    color = vec4(0.0, 0.0, 0.0, 1.0); // Black with full alpha
    
    color.a = 1;
     if ( object_id == PLANE )
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;

        U = U * 10.0; // Repete 5 vezes na direção U
        V = V * 10.0; // Repete 5 vezes na direção V

        U = fract(U);
        V = fract(V);

        vec3 Kd0 = texture(TextureImage2, vec2(U,V)).rgb;

        // Equação de Iluminação
        float lambert = max(0,dot(n,l));

        color.rgb = Kd0 * (lambert + 0.1);
    }
    else if ( object_id == ENEMY_HEAD)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage3, vec2(U,V)).rgb;

        // Equação de Iluminação
        float lambert = max(0,dot(n,l));

        color.rgb = Kd0 * (lambert + 0.1);
    }
    else if ( object_id == ENEMY_MIDDLE)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage4, vec2(U,V)).rgb;

        // Equação de Iluminação
        float lambert = max(0,dot(n,l));

        color.rgb = Kd0 * (lambert + 0.1);
    }
    else if ( object_id == ENEMY_BOTTOM)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage5, vec2(U,V)).rgb;

        // Equação de Iluminação
        float lambert = max(0,dot(n,l));

        color.rgb = Kd0 * (lambert + 0.1);
    }
    else if ( object_id == WALL)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;

        U = U * 12.0; // Repete 12 vezes na direção U
        V = V * 3.0; // Repete 3 vezes na direção V

        U = fract(U);
        V = fract(V);

        vec4 Kd0 = texture(TextureImage6, vec2(U,V));

        // Equação de Iluminação
        float lambert = max(0,dot(n,l));

        color.rgb = Kd0.rgb * (lambert + 0.1);
        color.a = Kd0.a;
    }
    else if ( object_id == ROOF)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;

        U = U * 5.0; // Repete 5 vezes na direção U
        V = V * 5.0; // Repete 5 vezes na direção V

        U = fract(U);
        V = fract(V);

        vec4 Kd0 = texture(TextureImage7, vec2(U,V));

        // Equação de Iluminação
        float lambert = max(0,dot(n,l));

        color.rgb = Kd0.rgb * (lambert + 0.1);
        color.a = Kd0.a;
    }
    else if ( object_id == TREE_BRANCH3)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;

        vec4 Kd0 = texture(TextureImage7, vec2(U,V));

        // Equação de Iluminação
        float lambert = max(0,dot(n,l));

        color.rgb = Kd0.rgb * (lambert + 0.1);
        color.a = Kd0.a;
    }
    else if ( object_id == CROSSHAIR)
    {

        color = vec4(1.0, 0.0, 0.0, 1.0); // Red with full alpha
    }
    else if ( object_id == PROJECTILE_LINE)
    {
        color = vec4(1.0, 1.0, 0.0, projectile_alpha); // Yellow with fading alpha
    }
    else if ( object_id == BARRICADE)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage8, vec2(U,V)).rgb;

        // Equação de Iluminação
        float lambert = max(0,dot(n,l));

        color.rgb = Kd0 * (lambert + 0.1);
    }
    // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
    //vec3 Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
//
    //vec3 Kd1 = texture(TextureImage1, vec2(U,V)).rgb;   //Carrega as luzes de noite
//
    //// Equação de Iluminação
    //float lambert = max(0,dot(n,l));
//
    //color.rgb = Kd0 * (lambert + 0.1) + Kd1 * (1.0 - clamp(lambert*5.0, 0.0, 1.0));

    // NOTE: Se você quiser fazer o rendering de objetos transparentes, é
    // necessário:
    // 1) Habilitar a operação de "blending" de OpenGL logo antes de realizar o
    //    desenho dos objetos transparentes, com os comandos abaixo no código C++:
    //      glEnable(GL_BLEND);
    //      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // 2) Realizar o desenho de todos objetos transparentes *após* ter desenhado
    //    todos os objetos opacos; e
    // 3) Realizar o desenho de objetos transparentes ordenados de acordo com
    //    suas distâncias para a câmera (desenhando primeiro objetos
    //    transparentes que estão mais longe da câmera).
    // Alpha default = 1 = 100% opaco = 0% transparente

    float dist = length(position_world - camera_position);
    float max_dist = 25.0;  
    float min_dist = 3.0;  

    float alpha = min(1.0, max(0, (dist-min_dist) / max_dist)); 
    vec3 cor_nevoa = vec3(1.0,1.0,1.0);
    //color.rgb = mix(color.rgb, cor_nevoa, alpha);
    //color.rgb = color.rgb + alpha*(cor_nevoa - color.rgb);
    //color.rgb = color.rgb*(1-alpha) + vec3(alpha,alpha,alpha);

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
} 

