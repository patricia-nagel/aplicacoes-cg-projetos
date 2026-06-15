# Exercício Grau B – Environment Mapping com Cube Mapping

**Disciplina:** Computação Gráfica – Unisinos 2026/1  
**Integrantes:** Cassio Braga, Gabriel Walber, Patricia Nagel

---

## Descrição

Expansão do projeto HelloCubemap para adicionar **reflexão de ambiente dinâmica** nos objetos 3D da cena (Suzanne), integrando o resultado do Cubemap com iluminação local **Phong** via variável de refletividade.

### Requisitos implementados

| # | Requisito | Onde |
|---|-----------|------|
| 1 | **Skybox** renderizado com Cubemap | `setupSkyboxVAO()`, shader `skyboxVert/FragSrc` |
| 2 | **Reflexão de ambiente** — vetor `I` (incidência câmera→fragmento) e `R = reflect(I, N)` | Fragment shader `objFragSrc` |
| 3 | **Phong** (ambiente + difusa + especular Blinn-Phong) | Fragment shader `objFragSrc` |
| 4 | **`mix(corPhong, corSkybox, reflectivity)`** — mistura controlável | Fragment shader `objFragSrc` |

---

## Compilação e execução

1. Abra a pasta do projeto no VS Code
2. Use **CMake: Build** (Ctrl+Shift+P → "CMake: Build") para compilar
3. No terminal, vá até a pasta `build` e execute:

```bash
cd build
./ExercicioSkybox
```

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
vec3 envColor = texture(skybox, R).rgb;

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
