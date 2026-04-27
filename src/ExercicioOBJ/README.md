# Exercício Grau A – Computação Gráfica 2026/1
## Selecionando e aplicando transformações em objetos 3D

**Disciplina:** Processamento Gráfico: Computação Gráfica e Aplicações – Unisinos  
**Aluno(s):** _(coloque seu nome aqui)_

---

## 📋 O que o programa faz

- Carrega 2 a 4 modelos `.obj` e os exibe lado a lado na tela
- Permite **selecionar** objetos ciclicamente com `TAB` (selecionado fica com pontos amarelos)
- Permite aplicar **rotação**, **translação** e **escala** no objeto selecionado
- Câmera navegável com `WASD`
- Wireframe preto desenhado por cima de cada objeto (desafio da Figura 1)

---

## 🕹️ Controles

| Tecla | Ação |
|-------|------|
| `TAB` | Seleciona o próximo objeto (cíclico) |
| `R` | Ativa modo **Rotação** |
| `T` | Ativa modo **Translação** |
| `S` | Ativa modo **Escala** |
| `X` / `Y` / `Z` | Aplica a transformação no eixo correspondente |
| `Shift + X/Y/Z` | Aplica na direção oposta |
| Setas / `W` `A` `S` `D` | Move o objeto (modo translação) |
| `Q` / `E` | Move no eixo Z (modo translação) |
| `U` | Escala uniforme (modo escala) |
| `W` `A` `S` `D` | Move a câmera (fora do modo translação) |
| `ESC` | Fecha o programa |

---

## 🛠️ Compilação

Siga o `GettingStarted.md` do repositório base da professora.

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### ⚠️ Atenção
- Os arquivos `glad.h`, `khrplatform.h` e `glad.c` devem estar nos diretórios corretos (veja o README do repositório base).
- O `Camera.h` e `Camera.cpp` da pasta `HelloCamera` devem estar na mesma pasta que o `main.cpp`.
- Os arquivos `.obj` devem estar em `assets/` — ajuste os caminhos no `main.cpp` se necessário.

---

## 📁 Arquivos necessários

```
src/ExercicioOBJ/
├── main.cpp
├── Camera.h       ← copiar do HelloCamera
└── Camera.cpp     ← copiar do HelloCamera
```
