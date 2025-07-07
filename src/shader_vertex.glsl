#version 330 core

// Atributos de vértice recebidos como entrada ("in") pelo Vertex Shader.
// Veja a função BuildTrianglesAndAddToVirtualScene() em "main.cpp".
layout (location = 0) in vec4 model_coefficients;
layout (location = 1) in vec4 normal_coefficients;
layout (location = 2) in vec2 texture_coefficients;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento
// Precisamos disso para saber quando aplicar o modelo de Gouraud
uniform int object_id;

// Definição do objeto barricada para referência
#define BARRICADE 7

// Atributos de vértice que serão gerados como saída ("out") pelo Vertex Shader.
// ** Estes serão interpolados pelo rasterizador! ** gerando, assim, valores
// para cada fragmento, os quais serão recebidos como entrada pelo Fragment
// Shader. Veja o arquivo "shader_fragment.glsl".
out vec4 position_world;
out vec4 position_model;
out vec4 normal;
out vec2 texcoords;

// Para o modelo de iluminação de Gouraud
// Estas cores serão calculadas por vértice e interpoladas pelo rasterizador
out vec3 gouraud_diffuse;
out vec3 gouraud_specular;
out vec3 gouraud_ambient;

void main()
{
    // A variável gl_Position define a posição final de cada vértice
    // OBRIGATORIAMENTE em "normalized device coordinates" (NDC), onde cada
    // coeficiente estará entre -1 e 1 após divisão por w.
    // Veja {+NDC2+}.
    //
    // O código em "main.cpp" define os vértices dos modelos em coordenadas
    // locais de cada modelo (array model_coefficients). Abaixo, utilizamos
    // operações de modelagem, definição da câmera, e projeção, para computar
    // as coordenadas finais em NDC (variável gl_Position). Após a execução
    // deste Vertex Shader, a placa de vídeo (GPU) fará a divisão por W. Veja
    // slides 41-67 e 69-86 do documento Aula_09_Projecoes.pdf.

    gl_Position = projection * view * model * model_coefficients;

    // Como as variáveis acima  (tipo vec4) são vetores com 4 coeficientes,
    // também é possível acessar e modificar cada coeficiente de maneira
    // independente. Esses são indexados pelos nomes x, y, z, e w (nessa
    // ordem, isto é, 'x' é o primeiro coeficiente, 'y' é o segundo, ...):
    //
    //     gl_Position.x = model_coefficients.x;
    //     gl_Position.y = model_coefficients.y;
    //     gl_Position.z = model_coefficients.z;
    //     gl_Position.w = model_coefficients.w;
    //

    // Agora definimos outros atributos dos vértices que serão interpolados pelo
    // rasterizador para gerar atributos únicos para cada fragmento gerado.

    // Posição do vértice atual no sistema de coordenadas global (World).
    position_world = model * model_coefficients;

    // Posição do vértice atual no sistema de coordenadas local do modelo.
    position_model = model_coefficients;

    // Normal do vértice atual no sistema de coordenadas global (World).
    // Veja slides 123-151 do documento Aula_07_Transformacoes_Geometricas_3D.pdf.
    normal = inverse(transpose(model)) * normal_coefficients;
    normal.w = 0.0;

    // Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
    texcoords = texture_coefficients;
    
    // Inicialização das variáveis de saída para iluminação de Gouraud
    gouraud_diffuse = vec3(0.0, 0.0, 0.0);
    gouraud_specular = vec3(0.0, 0.0, 0.0);
    gouraud_ambient = vec3(0.0, 0.0, 0.0);
    
    // Se o objeto atual for a barricada, aplicamos o modelo de iluminação de Gouraud
    if (object_id == BARRICADE)
    {
        // Obtemos a posição da câmera utilizando a inversa da matriz view
        vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
        vec4 camera_position = inverse(view) * origin;
        
        // Normal do vértice normalizada
        vec4 n = normalize(normal);
        
        // Vetor que define o sentido da fonte de luz em relação ao vértice atual.
        vec4 l = normalize(vec4(1.0, 1.0, 0.0, 0.0));
        
        // Vetor que define o sentido da câmera em relação ao vértice atual.
        vec4 v = normalize(camera_position - position_world);
        
        // Vetor que define o sentido da reflexão especular ideal.
        vec4 r = -l + 2.0*n*dot(n,l);
        
        // Textura e propriedades do material
        float U = texcoords.x;
        float V = texcoords.y;
        
        // Para o modelo de iluminação de Gouraud, precisamos acessar a textura
        // No vertex shader não temos acesso direto às texturas como no fragment shader
        // A cor base será passada no fragment shader usando a TextureImage8
        
        // Definimos cores base para o cálculo da iluminação
        // Serão multiplicadas pela textura no fragment shader
        vec3 Ks0 = vec3(0.595455, 0.595455, 0.595455); // Cor especular
        
        // Equação de Iluminação
        float lambert = max(0.0, dot(n, l));
        float Ns = 49.607449; // Exponente especular
        
        // Calculamos as componentes da iluminação
        // Usaremos vec3(1.0) como base, e multiplicaremos pela textura no fragment shader
        gouraud_specular = Ks0 * pow(max(0.0, dot(r, v)), Ns);
        gouraud_diffuse = vec3(lambert); // Apenas o fator lambert, a cor virá da textura
        gouraud_ambient = vec3(0.1);     // Fator ambiente, será multiplicado pela textura
    }
}

