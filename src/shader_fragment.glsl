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

// Para o modelo de iluminação de Gouraud
// Cores calculadas no vertex shader e interpoladas pelo rasterizador
in vec3 gouraud_diffuse;
in vec3 gouraud_specular;
in vec3 gouraud_ambient;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// variavel para projetil
uniform float projectile_alpha;
uniform float blood_alpha;



// Identificador que define qual objeto está sendo desenhado no momento
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
uniform sampler2D TextureImage9;
uniform sampler2D TextureImage10;
uniform sampler2D TextureImage11;
uniform sampler2D TextureImage12;
uniform sampler2D TextureImage13;
uniform sampler2D TextureImage14;
uniform sampler2D TextureImage15;
uniform sampler2D TextureImage16;
uniform sampler2D TextureImage17;
uniform sampler2D TextureImage18;
uniform sampler2D TextureImage19;
uniform sampler2D TextureImage20;
uniform sampler2D TextureImage21;
uniform sampler2D TextureImage22;
uniform sampler2D TextureImage23;

uniform bool muzzle_flash_active;
uniform vec3 muzzle_flash_position;
uniform vec3 muzzle_flash_color;
uniform float muzzle_flash_intensity;

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

    // Vetor que define o sentido da reflexão especular ideal.
    vec4 r = -l + 2.0*n*dot(n,l);

    vec4 h = normalize(l + v);  // Half-vector (método de Blinn-Phong)

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;

    U = texcoords.x;
    V = texcoords.y;
    // default: cor preta
    color = vec4(1.0, 0.0, 0.0, 1.0); // Black with full alpha
    vec4 ambient = vec4(0.0, 0.0, 0.0, 0.0); // Cor ambiente
    vec4 diffuse = vec4(0.0, 0.0, 0.0, 1.0); // Cor difusa
    vec4 specular = vec4(0.0, 0.0, 0.0, 0.0); // Cor especular
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

        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
    }
    else if ( object_id == ENEMY_HEAD)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage3, vec2(U,V)).rgb;

        // Equação de Iluminação
        float lambert = max(0,dot(n,l));

        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
    }
    else if ( object_id == ENEMY_FACE)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage3, vec2(U,V)).rgb;

        // Equação de Iluminação
        float lambert = max(0,dot(n,l));

        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
    }
    else if ( object_id == ENEMY_EYE)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage0, vec2(U,V)).rgb;

        // Equação de Iluminação
        float lambert = max(0,dot(n,l));

        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
    }
    else if ( object_id == ENEMY_MIDDLE)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage4, vec2(U,V)).rgb;

        // Equação de Iluminação
        float lambert = max(0,dot(n,l));

        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
    }
    else if ( object_id == ENEMY_BOTTOM)
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage5, vec2(U,V)).rgb;

        // Equação de Iluminação
        float lambert = max(0,dot(n,l));

        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
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

        vec3 Kd0 = texture(TextureImage6, vec2(U,V)).rgb;

        // Equação de Iluminação
        float lambert = max(0,dot(n,l));

        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
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

        vec3 Kd0 = texture(TextureImage7, vec2(U,V)).rgb;

        // Equação de Iluminação
        float lambert = max(0,dot(n,l));

        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
    }
    else if ( object_id == METAL)
    {
        vec3 Kd0 = vec3(0.008905, 0.008905, 0.008905); // Quase sem cor
        vec3 Ks0 = vec3(0.300000, 0.300000, 0.300000); // Cor especular
        // Equação de Iluminação
        float lambert = max(0,dot(n,l));
        float Ns = 87.293396; // Exponente especular

        specular.rgb = Ks0 * pow(max(0, dot(r, v)), Ns); // Cor especular
        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
    }
    else if ( object_id == CROSSHAIR)
    {

        diffuse = vec4(1.0, 0.0, 0.0, 1.0); // Red with full alpha
    }
    else if ( object_id == PROJECTILE_LINE)
    {
        diffuse = vec4(1.0, 1.0, 0.0, projectile_alpha); // Yellow with fading alpha
    }
    else if ( object_id == BARRICADE)
    {
        // Modelo de iluminação de Gouraud com textura
        // Obtemos a textura e aplicamos aos fatores calculados no vertex shader
        U = texcoords.x;
        V = texcoords.y;
        
        // Obtendo a cor da textura
        vec3 textureColor = texture(TextureImage8, vec2(U,V)).rgb;
        
        // Aplicamos a textura aos componentes de iluminação calculados no vertex shader
        ambient.rgb = textureColor * gouraud_ambient;    // Aplicando textura à componente ambiente
        diffuse.rgb = textureColor * gouraud_diffuse;    // Aplicando textura à componente difusa
        specular.rgb = gouraud_specular;                 // Componente especular permanece como está
    }
    else if ( object_id == RGZ89)
    {
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage9, vec2(U,V)).rgb;; // Quase sem cor
        vec3 Ks0 = vec3(0.5, 0.5, 0.5); // Cor especular
        // Equação de Iluminação
        float lambert = max(0,dot(n,l));
        float Ns = 250; // Exponente especular

        specular.rgb = Ks0 * pow(max(0, dot(n, h)), Ns);  // Componente especular usando Blinn-Phong
        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
    }
    else if ( object_id == WZ96_Beryl)
    {
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage10, vec2(U,V)).rgb;; // Quase sem cor
        vec3 Ks0 = vec3(0.5, 0.5, 0.5); // Cor especular
        // Equação de Iluminação
        float lambert = max(0,dot(n,l));
        float Ns = 250; // Exponente especular

        specular.rgb = Ks0 * pow(max(0, dot(n, h)), Ns);  // Componente especular usando Blinn-Phong
        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
    }
    else if ( object_id == boot_war1)
    {
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage11, vec2(U,V)).rgb;; // Quase sem cor
        vec3 Ks0 = vec3(0.5, 0.5, 0.5); // Cor especular
        // Equação de Iluminação
        float lambert = max(0,dot(n,l));
        float Ns = 250; // Exponente especular

        specular.rgb = Ks0 * pow(max(0, dot(n, h)), Ns);  // Componente especular usando Blinn-Phong
        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
    }
    else if ( object_id == glass)
    {

        vec3 Kd0 = vec3(0.0,0.0,0.0);
        vec3 Ks0 = vec3(0.5, 0.5, 0.5); // Cor especular
        // Equação de Iluminação
        float lambert = max(0,dot(n,l));
        float Ns = 250; // Exponente especular

        specular.rgb = Ks0 * pow(max(0, dot(n, h)), Ns);  // Componente especular usando Blinn-Phong
        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
    }
    else if ( object_id == magb)
    {
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage12, vec2(U,V)).rgb;; // Quase sem cor
        vec3 Ks0 = vec3(0.5, 0.5, 0.5); // Cor especular
        // Equação de Iluminação
        float lambert = max(0,dot(n,l));
        float Ns = 250; // Exponente especular

        specular.rgb = Ks0 * pow(max(0, dot(n, h)), Ns);  // Componente especular usando Blinn-Phong
        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
    }
    else if ( object_id == material)
    {
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage13, vec2(U,V)).rgb;; // Quase sem cor
        vec3 Ks0 = vec3(0.5, 0.5, 0.5); // Cor especular
        // Equação de Iluminação
        float lambert = max(0,dot(n,l));
        float Ns = 250; // Exponente especular

        specular.rgb = Ks0 * pow(max(0, dot(n, h)), Ns);  // Componente especular usando Blinn-Phong
        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
    }
    else if ( object_id == pol_bproof)
    {
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage14, vec2(U,V)).rgb;; // Quase sem cor
        vec3 Ks0 = vec3(0.3, 0.3, 0.3); // Cor especular
        // Equação de Iluminação
        float lambert = max(0,dot(n,l));
        float Ns = 250; // Exponente especular

        specular.rgb = Ks0 * pow(max(0, dot(n, h)), Ns);  // Componente especular usando Blinn-Phong
        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
    }
    else if ( object_id == pol_filter)
    {
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage15, vec2(U,V)).rgb;; // Quase sem cor
        vec3 Ks0 = vec3(0.3, 0.3, 0.3); // Cor especular
        // Equação de Iluminação
        float lambert = max(0,dot(n,l));
        float Ns = 250; // Exponente especular

        specular.rgb = Ks0 * pow(max(0, dot(n, h)), Ns);  // Componente especular usando Blinn-Phong
        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
    }
    else if ( object_id == pol_gas_mask)
    {
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage16, vec2(U,V)).rgb;; // Quase sem cor
        vec3 Ks0 = vec3(0.3, 0.3, 0.3); // Cor especular
        // Equação de Iluminação
        float lambert = max(0,dot(n,l));
        float Ns = 250; // Exponente especular

        specular.rgb = Ks0 * pow(max(0, dot(n, h)), Ns);  // Componente especular usando Blinn-Phong
        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1;
    }
    else if ( object_id == pol_hand)
    {
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage17, vec2(U,V)).rgb;; // Quase sem cor
        vec3 Ks0 = vec3(0.3, 0.3, 0.3); // Cor especular
        // Equação de Iluminação
        float lambert = max(0,dot(n,l));
        float Ns = 250; // Exponente especular

        specular.rgb = Ks0 * pow(max(0, dot(n, h)), Ns);  // Componente especular usando Blinn-Phong
        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
    }
    else if ( object_id == pol_head)
    {
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage18, vec2(U,V)).rgb;; // Quase sem cor
        vec3 Ks0 = vec3(0.2, 0.2, 0.2); // Cor especular
        // Equação de Iluminação
        float lambert = max(0,dot(n,l));
        float Ns = 250; // Exponente especular

        specular.rgb = Ks0 * pow(max(0, dot(n, h)), Ns);  // Componente especular usando Blinn-Phong
        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
    }
    else if ( object_id == pol_helmet)
    {
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage19, vec2(U,V)).rgb;; // Quase sem cor
        vec3 Ks0 = vec3(0.3, 0.3, 0.3); // Cor especular
        // Equação de Iluminação
        float lambert = max(0,dot(n,l));
        float Ns = 250; // Exponente especular

        specular.rgb = Ks0 * pow(max(0, dot(n, h)), Ns);  // Componente especular usando Blinn-Phong
        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10%
    }
    else if ( object_id == pol_jaket)
    {
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage20, vec2(U,V)).rgb;; // Quase sem cor
        vec3 Ks0 = vec3(0.1, 0.1, 0.1); // Cor especular
        // Equação de Iluminação
        float lambert = max(0,dot(n,l));
        float Ns = 250; // Exponente especular

        specular.rgb = Ks0 * pow(max(0, dot(n, h)), Ns);  // Componente especular usando Blinn-Phong
        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
    }
    else if ( object_id == pol_pants)
    {
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd0 = texture(TextureImage21, vec2(U,V)).rgb;; // Quase sem cor
        vec3 Ks0 = vec3(0.1, 0.1, 0.1); // Cor especular
        // Equação de Iluminação
        float lambert = max(0,dot(n,l));
        float Ns = 250; // Exponente especular

        specular.rgb = Ks0 * pow(max(0, dot(n, h)), Ns);  // Componente especular usando Blinn-Phong
        diffuse.rgb = Kd0 * lambert;
        ambient.rgb = Kd0 * 0.1; // Ambiente com 10% da cor difusa
    }
    else if( object_id == MUZZLE_FLASH)
    {
        U = texcoords.x;
        V = texcoords.y;

        float scale = 0.9;
        float offsetU = (1.0 - scale) * 0.5;
        float offsetV = (1.0 - scale) * 0.3;
        
        // Scale and center
        U = offsetU + U * scale;
        V = offsetV + V * scale;
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        vec4 texture_color = texture(TextureImage22, vec2(U,V));
        // Adjust alpha based on intensity
        if(texture_color.r < 0.1)
                discard;

        diffuse = vec4(texture_color.rgb * muzzle_flash_intensity, texture_color.a);
        ambient = vec4(0.0, 0.0, 0.0, 1.0); // Ambient light is zero for muzzle flash
        specular = vec4(0.0, 0.0, 0.0, 0.0); // No specular for muzzle flash
    }
    else if( object_id == BLOOD_SPLATTER )
    {
        U = texcoords.x;
        V = texcoords.y;
        
        // Sample the texture
        vec4 texture_color = texture(TextureImage23, vec2(U,V));
        
        // Discard transparent pixels
        if(texture_color.r < 0.8)
            discard;
        
        // Calculate fade based on lifetime (handled in CPU code by alpha value)
        diffuse = vec4(texture_color.rgb, blood_alpha); // Yellow with fading alpha;
        ambient = vec4(0.0, 0.0, 0.0, 0.0);
        specular = vec4(0.0, 0.0, 0.0, 0.0);
    }
    if (muzzle_flash_active)
    {
        // Calculate direction from fragment to muzzle flash
        vec3 flash_direction = normalize(muzzle_flash_position - position_world.xyz);
        
        // Calculate distance for attenuation
        float flash_distance = length(muzzle_flash_position - position_world.xyz);
        float attenuation = 1.0 / (1.0 + 0.25 * flash_distance * flash_distance);
        
        // Diffuse contribution from muzzle flash
        float flash_lambert = max(0.0, dot(normal.xyz, flash_direction));
        vec3 flash_diffuse = muzzle_flash_color * flash_lambert * attenuation * muzzle_flash_intensity;
        
        // Add muzzle flash light to the scene
        ambient.rgb += flash_diffuse;
    }

    color = ambient + diffuse + specular;
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

    //float dist = length(position_world - camera_position);
    //float max_dist = 25.0;  
    //float min_dist = 3.0;  

    //float alpha = min(1.0, max(0, (dist-min_dist) / max_dist)); 
    //vec3 cor_nevoa = vec3(1.0,1.0,1.0);
    //color.rgb = mix(color.rgb, cor_nevoa, alpha);
    //color.rgb = color.rgb + alpha*(cor_nevoa - color.rgb);
    //color.rgb = color.rgb*(1-alpha) + vec3(alpha,alpha,alpha);

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
} 

