/* Exercício Grau A - Computação Gráfica - Unisinos 2026/1
 * Selecionando e aplicando transformações em objetos 3D
 * Baseado nos exemplos de Rossana Baptista Queiroz
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <assert.h>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Camera
#include "Camera.h"

// Protótipos
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
int setupShader();
int loadSimpleOBJ(string filePATH, int &nVertices, glm::vec3 color);

// Dimensões da janela
const GLuint WIDTH = 1000, HEIGHT = 1000;

// Shaders — mesma estrutura dos exemplos da professora (GLSL 450, position + color, model + view + projection)
const GLchar* vertexShaderSource = R"glsl(#version 450
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;
out vec4 finalColor;
void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    finalColor = vec4(color, 1.0);
}
)glsl";

const GLchar* fragmentShaderSource = R"glsl(#version 450
in vec4 finalColor;
out vec4 color;
void main()
{
    color = finalColor;
}
)glsl";

// -------------------------------------------------------
// STRUCT do objeto 3D
// -------------------------------------------------------
struct Object3D
{
    GLuint VAO;
    int nVertices;

    glm::vec3 position  = glm::vec3(0.0f);
    glm::vec3 rotation  = glm::vec3(0.0f); // ângulos em graus (x, y, z)
    glm::vec3 scale     = glm::vec3(1.0f);
    glm::vec3 color;
};

// -------------------------------------------------------
// Variáveis globais
// -------------------------------------------------------
vector<Object3D> objects;
int selectedIndex = 0;

// Modo de transformação: 'r' = rotação, 't' = translação, 's' = escala
char transformMode = 'r';

// Câmera — mesmo padrão do HelloCamera
Camera camera(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 1.0f, 0.0f), 90.0f, 0.0f);
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// -------------------------------------------------------
// MAIN
// -------------------------------------------------------
int main()
{
    // Inicialização da GLFW
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Exercicio OBJ - Transformacoes 3D", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    // GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version  = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported " << version << endl;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Shader
    GLuint shaderID = setupShader();

    // -------------------------------------------------------
    // Carrega os objetos .obj
    // Ajuste os caminhos conforme os modelos disponíveis no
    // repositório da disciplina (pasta assets/ ou Modelos3D/)
    // -------------------------------------------------------
    // Cores distintas para cada objeto
    vector<glm::vec3> colors = {
        glm::vec3(1.0f, 0.3f, 0.3f), // vermelho
        glm::vec3(0.3f, 0.8f, 1.0f), // azul
        glm::vec3(0.3f, 1.0f, 0.4f), // verde
        glm::vec3(1.0f, 0.8f, 0.2f), // amarelo
    };

    // Posições iniciais — organiza os objetos lado a lado na tela
    vector<glm::vec3> initPositions = {
        glm::vec3(-1.5f,  0.7f, 0.0f),
        glm::vec3( 1.5f,  0.7f, 0.0f),
        glm::vec3(-1.5f, -0.7f, 0.0f),
        glm::vec3( 1.5f, -0.7f, 0.0f),
    };

    // Lista de arquivos .obj a carregar
    // Edite os caminhos conforme necessário
    vector<string> objFiles = {
        "../assets/Modelos3D/suzanne.obj",
        "../assets/Modelos3D/cube.obj"
    };

    for (int i = 0; i < (int)objFiles.size(); i++)
    {
        Object3D obj;
        obj.color    = colors[i % colors.size()];
        obj.position = initPositions[i % initPositions.size()];
        obj.scale    = glm::vec3(0.5f);

        int nVerts = 0;
        GLuint vao = loadSimpleOBJ(objFiles[i], nVerts, colors[i % colors.size()]);
        if (vao == -1)
        {
            cerr << "Pulando objeto: " << objFiles[i] << endl;
            continue;
        }
        obj.VAO      = vao;
        obj.nVertices = nVerts;
        objects.push_back(obj);
    }

    if (objects.empty())
    {
        cerr << "Nenhum objeto carregado! Verifique os caminhos dos arquivos .obj" << endl;
        return -1;
    }

    glUseProgram(shaderID);

    // Projeção perspectiva
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POLYGON_OFFSET_FILL);

    // -------------------------------------------------------
    // Imprime controles no terminal
    // -------------------------------------------------------
    cout << "\n=== CONTROLES ===" << endl;
    cout << "TAB        - Seleciona proximo objeto (ciclico)" << endl;
    cout << "R          - Modo ROTACAO  | X/Y/Z aplica nos eixos" << endl;
    cout << "T          - Modo TRANSLACAO | Setas/WASD movem, Q/E eixo Z" << endl;
    cout << "S          - Modo ESCALA   | X/Y/Z escala nos eixos, U = uniforme" << endl;
    cout << "Shift+X/Y/Z - Inverte a direcao da transformacao" << endl;
    cout << "ESC        - Sair\n" << endl;

    // -------------------------------------------------------
    // Game loop
    // -------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        // Câmera via WASD (igual ao HelloCamera)
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.processKeyboard("FORWARD",  deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.processKeyboard("BACKWARD", deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.processKeyboard("LEFT",     deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.processKeyboard("RIGHT",    deltaTime);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderID);

        // Atualiza view com a câmera
        glm::mat4 view = camera.getViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));

        // Desenha todos os objetos
        for (int i = 0; i < (int)objects.size(); i++)
        {
            Object3D& obj = objects[i];

            // Monta a matrix model: T * Rx * Ry * Rz * S
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);
            model = glm::rotate(model, glm::radians(obj.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(obj.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(obj.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, obj.scale);
            glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));

            // Desenha sólido
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glBindVertexArray(obj.VAO);
            glDrawArrays(GL_TRIANGLES, 0, obj.nVertices);

            // Desenha wireframe preto por cima (desafio — efeito da Figura 1)
            // Salva e altera temporariamente a cor via glVertexAttrib (sem mudar o VBO)
            glPolygonOffset(-1.0f, -1.0f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glVertexAttrib3f(1, 0.0f, 0.0f, 0.0f); // cor preta diretamente
            glDisableVertexAttribArray(1);           // desativa o atributo de cor do VAO
            glDrawArrays(GL_TRIANGLES, 0, obj.nVertices);
            glEnableVertexAttribArray(1);            // reativa para o próximo objeto

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glBindVertexArray(0);

            // Destaque do objeto selecionado: desenha pontos amarelos nos vértices
            if (i == selectedIndex)
            {
                glm::mat4 modelSel = model; // mesma model matrix
                glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(modelSel));
                glPointSize(4.0f);
                glVertexAttrib3f(1, 1.0f, 1.0f, 0.0f); // amarelo
                glDisableVertexAttribArray(1);
                glBindVertexArray(obj.VAO);
                glDrawArrays(GL_POINTS, 0, obj.nVertices);
                glEnableVertexAttribArray(1);
                glBindVertexArray(0);
            }
        }

        glfwSwapBuffers(window);
    }

    // Limpeza
    for (auto& obj : objects)
        glDeleteVertexArrays(1, &obj.VAO);
    glfwTerminate();
    return 0;
}

// -------------------------------------------------------
// Callback de teclado — padrão da professora
// -------------------------------------------------------
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Seleciona próximo objeto (TAB — cíclico)
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
    {
        selectedIndex = (selectedIndex + 1) % (int)objects.size();
        cout << "Objeto selecionado: " << selectedIndex << endl;
        return;
    }

    // Troca modo de transformação
    if (key == GLFW_KEY_R && action == GLFW_PRESS) { transformMode = 'r'; cout << "Modo: ROTACAO\n";    return; }
    if (key == GLFW_KEY_T && action == GLFW_PRESS) { transformMode = 't'; cout << "Modo: TRANSLACAO\n"; return; }
    if (key == GLFW_KEY_S && action == GLFW_PRESS) { transformMode = 's'; cout << "Modo: ESCALA\n";     return; }

    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    Object3D& obj = objects[selectedIndex];
    bool shift = (mode & GLFW_MOD_SHIFT);
    float dir  = shift ? -1.0f : 1.0f;

    // --- ROTAÇÃO ---
    if (transformMode == 'r')
    {
        float deg = 5.0f * dir;
        if (key == GLFW_KEY_X) obj.rotation.x += deg;
        if (key == GLFW_KEY_Y) obj.rotation.y += deg;
        if (key == GLFW_KEY_Z) obj.rotation.z += deg;
    }
    // --- TRANSLAÇÃO ---
    else if (transformMode == 't')
    {
        float step = 0.1f;
        if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D) obj.position.x += step;
        if (key == GLFW_KEY_LEFT  || key == GLFW_KEY_A) obj.position.x -= step;
        if (key == GLFW_KEY_UP    || key == GLFW_KEY_W) obj.position.y += step;
        if (key == GLFW_KEY_DOWN  || key == GLFW_KEY_S) obj.position.y -= step;
        if (key == GLFW_KEY_Q)                          obj.position.z -= step;
        if (key == GLFW_KEY_E)                          obj.position.z += step;
    }
    // --- ESCALA ---
    else if (transformMode == 's')
    {
        float factor = shift ? (1.0f / 1.05f) : 1.05f;
        if (key == GLFW_KEY_X) obj.scale.x *= factor;
        if (key == GLFW_KEY_Y) obj.scale.y *= factor;
        if (key == GLFW_KEY_Z) obj.scale.z *= factor;
        if (key == GLFW_KEY_U) obj.scale   *= factor; // escala uniforme
    }
}

// -------------------------------------------------------
// setupShader — idêntico ao padrão da professora
// -------------------------------------------------------
int setupShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

// -------------------------------------------------------
// loadSimpleOBJ — função da professora, com parâmetro de cor
// -------------------------------------------------------
int loadSimpleOBJ(string filePATH, int &nVertices, glm::vec3 color)
{
    vector<glm::vec3> vertices;
    vector<glm::vec2> texCoords;
    vector<glm::vec3> normals;
    vector<GLfloat>   vBuffer;

    ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open())
    {
        cerr << "Erro ao tentar ler o arquivo " << filePATH << endl;
        return -1;
    }

    string line;
    while (getline(arqEntrada, line))
    {
        istringstream ssline(line);
        string word;
        ssline >> word;

        if (word == "v")
        {
            glm::vec3 vertice;
            ssline >> vertice.x >> vertice.y >> vertice.z;
            vertices.push_back(vertice);
        }
        else if (word == "vt")
        {
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (word == "vn")
        {
            glm::vec3 normal;
            ssline >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (word == "f")
        {
            while (ssline >> word)
            {
                int vi = 0, ti = 0, ni = 0;
                istringstream ss(word);
                string index;

                if (getline(ss, index, '/')) vi = !index.empty() ? stoi(index) - 1 : 0;
                if (getline(ss, index, '/')) ti = !index.empty() ? stoi(index) - 1 : 0;
                if (getline(ss, index))      ni = !index.empty() ? stoi(index) - 1 : 0;

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);
                vBuffer.push_back(color.r);
                vBuffer.push_back(color.g);
                vBuffer.push_back(color.b);
            }
        }
    }
    arqEntrada.close();

    cout << "Gerando o buffer de geometria..." << endl;

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Atributo posição (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Atributo cor (r, g, b)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = vBuffer.size() / 6;
    return VAO;
}
