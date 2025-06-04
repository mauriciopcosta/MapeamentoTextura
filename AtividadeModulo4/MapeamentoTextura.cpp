
// Bibliotecas padrão
#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>

// Biblioteca para carregar imagens
#define STB_IMAGE_IMPLEMENTATION // DEFINA ANTES DE INCLUIR
#include <stb_image.h>           // INCLUA AQUI

// Bibliotecas OpenGL + manipulação de janela
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Bibliotecas GLM (para matrizes e transformações)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Tamanho da janela
const GLint WIDTH = 800, HEIGHT = 600;

// --- Classe Sprite ---
class Sprite
{
public:
    GLuint textureID;
    glm::vec2 position;
    glm::vec2 size; // Usado como escala para o quadrado 1x1
    GLfloat rotate; // Em graus

    // Construtor
    Sprite(const char *texturePath, glm::vec2 pos, glm::vec2 sz, GLfloat rot = 0.0f)
        : position(pos), size(sz), rotate(rot), textureID(0)
    {
        if (!loadTextureFromFile(texturePath, &this->textureID))
        {
            std::cerr << "Falha ao carregar textura para o sprite: " << texturePath << std::endl;
        }
    }

    // Destrutor para liberar a textura
    ~Sprite()
    {
        if (textureID != 0)
        {
            glDeleteTextures(1, &textureID);
        }
    }

    // Construtor de movimentação (Move constructor)
    Sprite(Sprite&& other) noexcept
        : textureID(other.textureID), position(std::move(other.position)),
          size(std::move(other.size)), rotate(other.rotate)
    {
        // Transfere a posse da textura, o 'other' não deve mais liberá-la.
        other.textureID = 0;
    }

    // Operador de atribuição de movimentação (Move assignment operator)
    Sprite& operator=(Sprite&& other) noexcept
    {
        if (this == &other) return *this;

        // Libera o recurso atual (se houver)
        if (textureID != 0) {
            glDeleteTextures(1, &textureID);
        }

        // Transfere os dados e a posse da textura
        textureID = other.textureID;
        position = std::move(other.position);
        size = std::move(other.size);
        rotate = other.rotate;

        other.textureID = 0; // O 'other' não deve mais liberar a textura
        return *this;
    }

    // Proibir cópia para evitar problemas de gerenciamento de textura duplicado
    Sprite(const Sprite&) = delete;
    Sprite& operator=(const Sprite&) = delete;


    // Método para desenhar o sprite
    void draw(GLuint shaderProgramme, GLuint VAO)
    {
        if (this->textureID == 0) return; // Não desenha se a textura não carregou ou foi movida

        glUseProgram(shaderProgramme);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position, 0.0f));
        model = glm::rotate(model, glm::radians(rotate), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(size, 1.0f));

        glUniformMatrix4fv(glGetUniformLocation(shaderProgramme, "matrix"), 1, GL_FALSE, glm::value_ptr(model));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->textureID);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0); 
    }

private:
    static bool loadTextureFromFile(const char *file_name, GLuint *tex)
    {
        int x, y, n;
        int force_channels = 4; 

        unsigned char *image_data = stbi_load(file_name, &x, &y, &n, force_channels);
        if (!image_data)
        {
            // A mensagem de erro já é impressa aqui, não precisa repetir no construtor.
            // std::cerr << "ERRO ao carregar textura: " << file_name << std::endl; 
            std::cerr << "Motivo do stbi_load: " << stbi_failure_reason() << std::endl;
            return false;
        }

        glGenTextures(1, tex);
        glBindTexture(GL_TEXTURE_2D, *tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(image_data);
        glBindTexture(GL_TEXTURE_2D, 0); 
        return true;
    }
};
// --- Fim da Classe Sprite ---


int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Cena da Paisagem", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Erro ao criar janela GLFW" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Falha ao inicializar GLAD" << std::endl;
        return EXIT_FAILURE;
    }

    glEnable(GL_BLEND); 
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const char *vertex_shader =
        "#version 400\n"
        "layout (location = 0) in vec3 vPosition;\n"
        "layout (location = 2) in vec2 vTexture;\n"
        "uniform mat4 proj;\n"
        "uniform mat4 matrix; \n"
        "out vec2 text_map;\n"
        "void main() {\n"
        "    text_map = vTexture;\n"
        "    gl_Position = proj * matrix * vec4(vPosition, 1.0);\n"
        "}";

    const char *fragment_shader =
        "#version 400\n"
        "in vec2 text_map;\n"
        "uniform sampler2D basic_texture;\n"
        "out vec4 frag_color;\n"
        "void main() {\n"
        "    frag_color = texture(basic_texture, text_map);\n"
        "}";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, NULL);
    glCompileShader(fs);

    GLuint shader_programme = glCreateProgram();
    glAttachShader(shader_programme, vs);
    glAttachShader(shader_programme, fs);
    glLinkProgram(shader_programme);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLfloat vertices[] = {
        -0.5f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,    0.0f, 1.0f, 
         0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,    0.0f, 0.0f, 
        -0.5f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,    0.0f, 1.0f, 
         0.5f,  0.5f, 0.0f,  0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 
         0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,    1.0f, 0.0f  
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0); 

    glm::mat4 proj = glm::ortho(0.0f, static_cast<float>(WIDTH), static_cast<float>(HEIGHT), 0.0f, -1.0f, 1.0f);

    glUseProgram(shader_programme); 
    glUniform1i(glGetUniformLocation(shader_programme, "basic_texture"), 0);

    // ---------------- Criação dos Sprites da Paisagem ---------------- //
    // puxando os caminhoes de onde coloquei as imagens junto com o .cpp
    Sprite skySprite("../src/AtividadeModulo4/sky.png",
                     glm::vec2(WIDTH / 2.0f, HEIGHT / 2.0f),
                     glm::vec2(WIDTH, HEIGHT),
                     0.0f);

    Sprite clouds1Sprite("../src/AtividadeModulo4/clouds_1.png",
                        glm::vec2(WIDTH / 2.0f, HEIGHT * 0.3f), 
                        glm::vec2(WIDTH, HEIGHT * 0.4f),       
                        0.0f);

    Sprite clouds2Sprite("../src/AtividadeModulo4/clouds_2.png",
                        glm::vec2(WIDTH / 2.0f, HEIGHT * 0.25f),
                        glm::vec2(WIDTH * 0.7f, HEIGHT * 0.2f), 
                        0.0f);

    Sprite rocksSprite("../src/AtividadeModulo4/rocks.png",
                        glm::vec2(WIDTH / 2.0f, HEIGHT / 2.0f), 
                        glm::vec2(WIDTH, HEIGHT),              
                        0.0f);

    Sprite groundSprite("../src/AtividadeModulo4/ground.png",
                         glm::vec2(WIDTH / 2.0f, HEIGHT / 2.0f), 
                         glm::vec2(WIDTH, HEIGHT),               
                         0.0f);
    
    std::vector<Sprite> sprites;
    sprites.reserve(5); 
    
    sprites.push_back(std::move(skySprite));
    sprites.push_back(std::move(clouds1Sprite));
    sprites.push_back(std::move(clouds2Sprite));
    sprites.push_back(std::move(rocksSprite));
    sprites.push_back(std::move(groundSprite));

    // ---------------- Loop Principal ---------------- //
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader_programme);
        glUniformMatrix4fv(glGetUniformLocation(shader_programme, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
        
        for (Sprite& sprite : sprites) 
        {
            sprite.draw(shader_programme, VAO);
        }

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shader_programme);

    glfwTerminate();
    return EXIT_SUCCESS;
}