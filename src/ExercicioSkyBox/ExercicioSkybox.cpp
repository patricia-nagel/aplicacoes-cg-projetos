/* Exercício Grau B – Computação Gráfica – Unisinos 2026/1
 * Environment Mapping com Cube Mapping
 *
 * Requisitos implementados:
 *  1. Skybox renderizado com Cubemap
 *  2. Reflexão de ambiente (Environment Reflection) no fragment shader do objeto
 *  3. Integração com iluminação local (Phong): mix(corPhong, corSkybox, reflectivity)
 *
 * Controles:
 *  W/A/S/D        movimenta a câmera
 *  Mouse          olha ao redor (clique na janela para capturar)
 *  X / Y / Z      rotaciona o objeto nos eixos
 *  +   / -        aumenta / diminui a refletividade (0.0 … 1.0)
 *  ESC            fecha a janela
 *
 * Dependências: GLAD, GLFW3, GLM, stb_image.h
 *
 * Caminhos dos recursos esperados:
 *  Cubemap:  ../assets/skybox/{right,left,top,bottom,front,back}.jpg
 *  Modelo:   ../assets/Modelos3D/suzanne.obj
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// stb_image — disponível como dependência do ambiente da disciplina
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Câmera da disciplina
#include "Camera.h"

// ============================================================
//  PROTÓTIPOS
// ============================================================
void  key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void  mouse_callback(GLFWwindow* window, double xpos, double ypos);
GLuint setupSkyboxShader();
GLuint setupObjectShader();
GLuint setupSkyboxVAO();
GLuint loadCubemap(vector<string> faces);
int   loadOBJWithNormals(const string& filePath, int& nVertices);

// ============================================================
//  CONSTANTES
// ============================================================
const GLuint WIDTH  = 1000;
const GLuint HEIGHT = 1000;

// ============================================================
//  SHADERS — SKYBOX
//  O skybox usa apenas a direção do fragmento (vec3) para
//  amostrar o cubemap.  A matriz view é passada SEM translação
//  para que o céu pareça infinitamente distante.
// ============================================================
const GLchar* skyboxVertSrc = R"glsl(
#version 450 core
layout (location = 0) in vec3 position;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;          // view SEM translação

void main()
{
    TexCoords   = position;
    vec4 pos    = projection * view * vec4(position, 1.0);
    // truque: z = w faz com que z/w == 1 (sempre no far plane)
    gl_Position = pos.xyww;
}
)glsl";

const GLchar* skyboxFragSrc = R"glsl(
#version 450 core
in  vec3 TexCoords;
out vec4 FragColor;

uniform samplerCube skybox;

void main()
{
    FragColor = texture(skybox, TexCoords);
}
)glsl";

// ============================================================
//  SHADERS — OBJETO (Phong + Environment Reflection)
//
//  Atributos do VAO:
//    location 0 → posição  (vec3)
//    location 1 → normal   (vec3)
// ============================================================
const GLchar* objVertSrc = R"glsl(
#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;       // posição do fragmento em espaço de mundo
out vec3 Normal;        // normal em espaço de mundo

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos       = worldPos.xyz;

    // Normal matrix: transposta da inversa da parte 3x3 da model matrix
    Normal        = mat3(transpose(inverse(model))) * aNormal;

    gl_Position   = projection * view * worldPos;
}
)glsl";

const GLchar* objFragSrc = R"glsl(
#version 450 core

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

// Cubemap do ambiente
uniform samplerCube skybox;

// Posição da câmera (necessária para vetor de incidência)
uniform vec3 cameraPos;

// Propriedades da luz pontual
uniform vec3 lightPos;
uniform vec3 lightColor;

// Cor do material (usada no Phong)
uniform vec3 objectColor;

// Controle de refletividade: 0 = apenas Phong, 1 = apenas espelho
uniform float reflectivity;

void main()
{
    // -------------------------------------------------------
    // 1. PHONG — Iluminação local
    // -------------------------------------------------------
    vec3 norm     = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir  = normalize(cameraPos - FragPos);
    vec3 halfDir  = normalize(lightDir + viewDir);

    // Ambiente
    float ambientStrength  = 0.15;
    vec3  ambient          = ambientStrength * lightColor;

    // Difusa
    float diff             = max(dot(norm, lightDir), 0.0);
    vec3  diffuse          = diff * lightColor;

    // Especular (Blinn-Phong)
    float specStrength     = 0.5;
    float shininess        = 64.0;
    float spec             = pow(max(dot(norm, halfDir), 0.0), shininess);
    vec3  specular         = specStrength * spec * lightColor;

    vec3 phongColor = (ambient + diffuse + specular) * objectColor;

    // -------------------------------------------------------
    // 2. ENVIRONMENT REFLECTION
    // Vetor de incidência: do olho ao fragmento (normalizado)
    // Vetor de reflexão: reflete I em torno da normal
    // -------------------------------------------------------
    vec3 I = normalize(FragPos - cameraPos);   // sentido: câmera → fragmento
    vec3 R = reflect(I, norm);                 // vetor de reflexão no espaço de mundo
    vec3 envColor = texture(skybox, R).rgb;

    // -------------------------------------------------------
    // 3. MIX — Combina Phong com reflexão de ambiente
    // -------------------------------------------------------
    vec3 finalColor = mix(phongColor, envColor, reflectivity);

    FragColor = vec4(finalColor, 1.0);
}
)glsl";

// ============================================================
//  VARIÁVEIS GLOBAIS
// ============================================================
// Câmera começa em Z=+5 olhando para a origem (yaw=-90 aponta para -Z)
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
float  deltaTime    = 0.0f;
float  lastFrame    = 0.0f;

bool   rotateX = false, rotateY = false, rotateZ = false;

float  reflectivity = 0.5f;   // começa no meio-termo

// Mouse
float  lastX     = WIDTH  / 2.0f;
float  lastY     = HEIGHT / 2.0f;
bool   firstMouse = true;
bool   mouseCaptured = false;

// ============================================================
//  MAIN
// ============================================================
int main()
{
    // ---- GLFW ----
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
        "ExercicioSkybox – Environment Mapping", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    // mouse NÃO capturado por padrão; pressione M para ativar

    // ---- GLAD ----
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cerr << "Falha ao inicializar GLAD" << endl;
        return -1;
    }

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version  = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << "\nOpenGL:   " << version << endl;

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);

    glEnable(GL_DEPTH_TEST);

    // ---- SHADERS ----
    GLuint skyboxShader = setupSkyboxShader();
    GLuint objShader    = setupObjectShader();

    // ---- SKYBOX VAO ----
    GLuint skyboxVAO = setupSkyboxVAO();

    // ---- CUBEMAP ----
    // Ajuste os caminhos conforme sua estrutura de pastas
    vector<string> faces = {
        "../assets/skybox/right.jpg",
        "../assets/skybox/left.jpg",
        "../assets/skybox/top.jpg",
        "../assets/skybox/bottom.jpg",
        "../assets/skybox/front.jpg",
        "../assets/skybox/back.jpg"
    };
    GLuint cubemapTexture = loadCubemap(faces);

    // ---- OBJETO 3D ----
    int    nVertices = 0;
    GLuint objVAO   = loadOBJWithNormals("../assets/Modelos3D/suzanne.obj", nVertices);
    if (objVAO == 0)
    {
        cerr << "Falha ao carregar o modelo OBJ. Verifique o caminho." << endl;
        glfwTerminate();
        return -1;
    }

    // ---- UNIFORMS ESTÁTICOS ----
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);

    // Skybox: associa a texture unit 0 ao sampler do skybox shader
    glUseProgram(skyboxShader);
    glUniform1i(glGetUniformLocation(skyboxShader, "skybox"), 0);
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "projection"),
                       1, GL_FALSE, glm::value_ptr(projection));

    // Objeto: associa a texture unit 0 ao samplerCube do objeto
    glUseProgram(objShader);
    glUniform1i(glGetUniformLocation(objShader, "skybox"),      0);
    glUniformMatrix4fv(glGetUniformLocation(objShader, "projection"),
                       1, GL_FALSE, glm::value_ptr(projection));

    // Luz fixa no mundo
    glm::vec3 lightPos   = glm::vec3(5.0f, 5.0f, -5.0f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 objColor   = glm::vec3(0.8f, 0.5f, 0.2f); // cor base (laranja)

    glUniform3fv(glGetUniformLocation(objShader, "lightPos"),   1, glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(objShader, "lightColor"), 1, glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(objShader, "objectColor"),1, glm::value_ptr(objColor));

    cout << "\n=== CONTROLES ===" << endl;
    cout << "W/A/S/D  — movimenta a camera" << endl;
    cout << "M        — ativa/desativa captura do mouse para olhar ao redor" << endl;
    cout << "X/Y/Z    — rotaciona o objeto" << endl;
    cout << "+  / -   — aumenta/diminui refletividade (atual: 0.50)" << endl;
    cout << "ESC      — fecha" << endl;

    // ============================================================
    //  GAME LOOP
    // ============================================================
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        // Movimento de câmera via teclado
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.processKeyboard("FORWARD",  deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.processKeyboard("BACKWARD", deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.processKeyboard("LEFT",     deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.processKeyboard("RIGHT",    deltaTime);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ----- Matrizes comuns -----
        glm::mat4 view = camera.getViewMatrix();

        // ----- Ângulo de rotação do objeto -----
        float angle = (float)glfwGetTime();
        glm::mat4 model = glm::mat4(1.0f);
        if      (rotateX) model = glm::rotate(model, angle, glm::vec3(1,0,0));
        else if (rotateY) model = glm::rotate(model, angle, glm::vec3(0,1,0));
        else if (rotateZ) model = glm::rotate(model, angle, glm::vec3(0,0,1));

        // =====================================================
        //  PASSO 1 — Desenha o OBJETO com environment mapping
        // =====================================================
        glUseProgram(objShader);

        glUniformMatrix4fv(glGetUniformLocation(objShader, "view"),
                           1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(objShader, "model"),
                           1, GL_FALSE, glm::value_ptr(model));

        // Posição da câmera necessária para o vetor de incidência no shader
        glUniform3fv(glGetUniformLocation(objShader, "cameraPos"),
                     1, glm::value_ptr(camera.position));

        // Refletividade atual (controlada com + / -)
        glUniform1f(glGetUniformLocation(objShader, "reflectivity"), reflectivity);

        // Associa o cubemap à texture unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

        glBindVertexArray(objVAO);
        glDrawArrays(GL_TRIANGLES, 0, nVertices);
        glBindVertexArray(0);

        // =====================================================
        //  PASSO 2 — Desenha o SKYBOX
        //  Desativa escrita no depth buffer (glDepthMask(GL_FALSE))
        //  para que o skybox fique sempre "atrás" de tudo.
        //  Alternativa: usar glDepthFunc(GL_LEQUAL) com o truque pos.xyww.
        // =====================================================
        glDepthFunc(GL_LEQUAL);   // passa no teste quando z == far plane
        glUseProgram(skyboxShader);

        // Remove a translação da view matrix (3x3 → 4x4) para que
        // o skybox pareça infinitamente distante
        glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));
        glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "view"),
                           1, GL_FALSE, glm::value_ptr(viewNoTranslation));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glDepthFunc(GL_LESS);     // restaura comportamento padrão

        glfwSwapBuffers(window);
    }

    // Limpeza
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteVertexArrays(1, &objVAO);
    glDeleteTextures(1, &cubemapTexture);
    glfwTerminate();
    return 0;
}

// ============================================================
//  CALLBACKS
// ============================================================
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Rotação do objeto
    if (key == GLFW_KEY_X && action == GLFW_PRESS) { rotateX = true;  rotateY = false; rotateZ = false; }
    if (key == GLFW_KEY_Y && action == GLFW_PRESS) { rotateX = false; rotateY = true;  rotateZ = false; }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS) { rotateX = false; rotateY = false; rotateZ = true;  }

    // Refletividade
    if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS)  // tecla '+'
    {
        reflectivity = glm::min(reflectivity + 0.05f, 1.0f);
        cout << "Refletividade: " << reflectivity << endl;
    }
    if (key == GLFW_KEY_MINUS && action == GLFW_PRESS)
    {
        reflectivity = glm::max(reflectivity - 0.05f, 0.0f);
        cout << "Refletividade: " << reflectivity << endl;
    }

    // Ativa/desativa captura do mouse
    if (key == GLFW_KEY_M && action == GLFW_PRESS)
    {
        mouseCaptured = !mouseCaptured;
        if (mouseCaptured)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        else
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (!mouseCaptured) return;

    if (firstMouse)
    {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
        return;
    }

    float xoffset = ((float)xpos - lastX) * 0.1f;
    float yoffset = (lastY - (float)ypos) * 0.1f; // invertido: y cresce para baixo
    lastX = (float)xpos;
    lastY = (float)ypos;

    // Atualiza yaw e pitch diretamente na câmera (processMouseMovement está vazia)
    camera.yaw   += xoffset;
    camera.pitch += yoffset;
    if (camera.pitch >  89.0f) camera.pitch =  89.0f;
    if (camera.pitch < -89.0f) camera.pitch = -89.0f;

    // Recalcula front/right/up manualmente
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    newFront.y = sin(glm::radians(camera.pitch));
    newFront.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    camera.front = glm::normalize(newFront);
    camera.right = glm::normalize(glm::cross(camera.front, camera.worldUp));
    camera.up    = glm::normalize(glm::cross(camera.right, camera.front));
}

// ============================================================
//  setupSkyboxShader
// ============================================================
GLuint setupSkyboxShader()
{
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &skyboxVertSrc, NULL);
    glCompileShader(vs);
    GLint ok; GLchar log[512];
    glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(vs, 512, NULL, log); cerr << "SkyboxVS:\n" << log << endl; }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &skyboxFragSrc, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(fs, 512, NULL, log); cerr << "SkyboxFS:\n" << log << endl; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { glGetProgramInfoLog(prog, 512, NULL, log); cerr << "SkyboxProg:\n" << log << endl; }

    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

// ============================================================
//  setupObjectShader
// ============================================================
GLuint setupObjectShader()
{
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &objVertSrc, NULL);
    glCompileShader(vs);
    GLint ok; GLchar log[512];
    glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(vs, 512, NULL, log); cerr << "ObjVS:\n" << log << endl; }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &objFragSrc, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(fs, 512, NULL, log); cerr << "ObjFS:\n" << log << endl; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { glGetProgramInfoLog(prog, 512, NULL, log); cerr << "ObjProg:\n" << log << endl; }

    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

// ============================================================
//  setupSkyboxVAO — cubo unitário (36 vértices)
// ============================================================
GLuint setupSkyboxVAO()
{
    // Cubo simples: cada face = 2 triângulos, posições em [-1, 1]
    float skyboxVertices[] = {
        // Back face
        -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
        // Front face
        -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
        // Right face
         1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
        // Front face (outer)
        -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
        // Top face
        -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,
        // Bottom face
        -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

    // Apenas posição (location = 0), stride = 3 floats
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return VAO;
}

// ============================================================
//  loadCubemap
//  Carrega 6 imagens na ordem: right, left, top, bottom, front, back
// ============================================================
GLuint loadCubemap(vector<string> faces)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    stbi_set_flip_vertically_on_load(false); // cubemaps NÃO precisam de flip

    int w, h, nChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &nChannels, 0);
        if (data)
        {
            GLenum format = (nChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            cerr << "Cubemap: falha ao carregar " << faces[i] << endl;
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

// ============================================================
//  loadOBJWithNormals
//  Lê posição (v) e normal (vn) do arquivo .obj.
//  VAO: location 0 = posição (vec3), location 1 = normal (vec3)
//  Stride = 6 floats.
// ============================================================
int loadOBJWithNormals(const string& filePath, int& nVertices)
{
    vector<glm::vec3> tmpPositions;
    vector<glm::vec3> tmpNormals;
    vector<float>     vBuffer;

    ifstream file(filePath);
    if (!file.is_open())
    {
        cerr << "Não foi possível abrir: " << filePath << endl;
        return 0;
    }

    string line;
    while (getline(file, line))
    {
        istringstream ss(line);
        string token;
        ss >> token;

        if (token == "v")
        {
            glm::vec3 p;
            ss >> p.x >> p.y >> p.z;
            tmpPositions.push_back(p);
        }
        else if (token == "vn")
        {
            glm::vec3 n;
            ss >> n.x >> n.y >> n.z;
            tmpNormals.push_back(n);
        }
        else if (token == "f")
        {
            // Suporta "v/vt/vn", "v//vn" e "v/vt" (sem normal)
            string word;
            while (ss >> word)
            {
                int vi = 0, ti = 0, ni = 0;
                istringstream ws(word);
                string idx;

                if (getline(ws, idx, '/')) vi = !idx.empty() ? stoi(idx) - 1 : 0;
                if (getline(ws, idx, '/')) ti = !idx.empty() ? stoi(idx) - 1 : 0;
                if (getline(ws, idx))      ni = !idx.empty() ? stoi(idx) - 1 : 0;

                glm::vec3 pos    = tmpPositions[vi];
                glm::vec3 normal = (!tmpNormals.empty()) ? tmpNormals[ni] : glm::vec3(0,1,0);

                vBuffer.push_back(pos.x);    vBuffer.push_back(pos.y);    vBuffer.push_back(pos.z);
                vBuffer.push_back(normal.x); vBuffer.push_back(normal.y); vBuffer.push_back(normal.z);
            }
        }
    }
    file.close();

    nVertices = (int)(vBuffer.size() / 6);
    cout << "OBJ carregado: " << filePath << " | vértices: " << nVertices << endl;

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(float), vBuffer.data(), GL_STATIC_DRAW);

    // location 0 — posição (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // location 1 — normal (nx, ny, nz)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return VAO;
}
