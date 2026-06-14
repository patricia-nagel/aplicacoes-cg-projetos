# Exercício Grau B – Environment Mapping com Cube Mapping

**Disciplina:** Computação Gráfica – Unisinos 2026/1  

---

## Descrição

Expansão do projeto HelloCubemap para adicionar **reflexão de ambiente dinâmica** nos objetos 3D da cena (Suzanne / cubo), integrando o resultado do Cubemap com iluminação local **Phong** via variável de refletividade.

### Requisitos implementados

| # | Requisito | Onde |
|---|-----------|------|
| 1 | **Skybox** renderizado com Cubemap | `setupSkyboxVAO()`, shader `skyboxVert/FragSrc` |
| 2 | **Reflexão de ambiente** — vetor `I` (incidência câmera→fragmento) e `R = reflect(I, N)` | Fragment shader `objFragSrc` |
| 3 | **Phong** (ambiente + difusa + especular Blinn-Phong) | Fragment shader `objFragSrc` |
| 4 | **`mix(corPhong, corSkybox, reflectivity)`** — mistura controlável | Fragment shader `objFragSrc` |

---

## Estrutura de pastas esperada

```
projeto/
├── ExercicioSkybox.cpp   ← código principal
├── Camera.h / Camera.cpp ← câmera da disciplina
├── stb_image.h           ← https://github.com/nothings/stb
└── assets/
    ├── skybox/
    │   ├── right.jpg
    │   ├── left.jpg
    │   ├── top.jpg
    │   ├── bottom.jpg
    │   ├── front.jpg
    │   └── back.jpg
    └── Modelos3D/
        └── suzanne.obj
```

> **Cubemaps gratuitos:** [LearnOpenGL – Skyboxes](https://learnopengl.com/Advanced-OpenGL/Cubemaps) disponibiliza o pacote `skybox.zip` com as 6 faces JPG prontas.

---

## Compilação

### Linux / macOS (g++)

```bash
g++ ExercicioSkybox.cpp Camera.cpp \
    -o ExercicioSkybox \
    -lglfw -lGL -ldl -lpthread \
    -I/usr/include   # ajuste para o seu sistema
```

### Windows (MinGW / MSYS2)

```bash
g++ ExercicioSkybox.cpp Camera.cpp \
    -o ExercicioSkybox.exe \
    -lglfw3 -lopengl32 -lgdi32
```

### CMake (recomendado)

```cmake
cmake_minimum_required(VERSION 3.10)
project(ExercicioSkybox)
set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)
find_package(glfw3  REQUIRED)

add_executable(ExercicioSkybox ExercicioSkybox.cpp Camera.cpp)
target_link_libraries(ExercicioSkybox OpenGL::GL glfw ${CMAKE_DL_LIBS})
```

```bash
mkdir build && cd build
cmake .. && cmake --build .
./ExercicioSkybox
```

> **Obs.:** `glad.c` (ou `glad.cpp`) precisa ser compilado junto se não usar a versão de header-only. Adicione-o na lista de fontes do `add_executable`.

---

## Controles

| Tecla | Ação |
|-------|------|
| W / A / S / D | Move a câmera |
| M | Ativa/desativa captura do mouse (olhar ao redor) |
| X | Rotaciona o objeto no eixo X |
| Y | Rotaciona o objeto no eixo Y |
| Z | Rotaciona o objeto no eixo Z |
| `+` | Aumenta a refletividade (+0.05) |
| `-` | Diminui a refletividade (−0.05) |
| ESC | Fecha a janela |

---

## Explicação técnica dos shaders

### Vertex shader do objeto

Calcula a posição do fragmento e a normal em **espaço de mundo**, usando a *normal matrix* (`transpose(inverse(model))`) para corrigir deformações causadas por escalas não-uniformes.

### Fragment shader do objeto

```glsl
// 1. Phong (Blinn-Phong)
vec3 phongColor = (ambient + diffuse + specular) * objectColor;

// 2. Reflexão: vetor I = câmera→fragmento, R = reflect(I, normal)
vec3 I = normalize(FragPos - cameraPos);
vec3 R = reflect(I, norm);
vec3 envColor = texture(skybox, R).rgb;   // amostra o cubemap

// 3. Mistura controlada
vec3 finalColor = mix(phongColor, envColor, reflectivity);
```

Com `reflectivity = 0.0` o objeto exibe apenas Phong; com `1.0` vira um espelho perfeito.

### Skybox

A matriz `view` é passada **sem a componente de translação** (`mat4(mat3(view))`), fazendo o céu parecer infinitamente distante. O truque `gl_Position = pos.xyww` garante que o skybox seja renderizado sempre no far plane.

---

## Referências

- LearnOpenGL – [Cubemaps](https://learnopengl.com/Advanced-OpenGL/Cubemaps)  
- Möller, T. et al. *Real-Time Rendering*, 4ª ed. CRC Press, 2018. Cap. 10.
