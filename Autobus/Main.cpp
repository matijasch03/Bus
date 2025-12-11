#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Util.h"

// Dimenzije prozora
int screenWidth = 800;
int screenHeight = 600;

unsigned busTexture;

int endProgram(std::string message) {
    std::cerr << message << std::endl;
    glfwTerminate();
    return -1;
}

void preprocessTexture(unsigned& texture, const char* filepath) {
    texture = loadImageToTexture(filepath);
    if (texture != 0) {
        glBindTexture(GL_TEXTURE_2D, texture);
        // Osnovna podešavanja teksture
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void formVAObus(float* vertices, size_t size, unsigned int& VAObus) {
    unsigned int VBObus;
    glGenVertexArrays(1, &VAObus);
    glGenBuffers(1, &VBObus);

    glBindVertexArray(VAObus);
    glBindBuffer(GL_ARRAY_BUFFER, VBObus);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

    // Atribut 0 (pozicija): 2 float-a, poèinje na ofsetu 0
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Atribut 1 (teksturne koordinate): 2 float-a, poèinje na ofsetu 2 * sizeof(float)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void drawBus(unsigned int rectShader, unsigned int VAObus) {
    glUseProgram(rectShader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, busTexture);

    glBindVertexArray(VAObus);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}

int main()
{
    // Inicijalizacija GLFW
    if (!glfwInit()) return endProgram("GLFW nije uspeo da se inicijalizuje.");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Kreiranje prozora
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Minimal Bus Project", NULL, NULL);
    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");
    glfwMakeContextCurrent(window);

    // Inicijalizacija GLEW
    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");

    // Omoguæavanje providnosti
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Uèitavanje teksture i šejdera
    preprocessTexture(busTexture, "res/avtobus.png");
    unsigned int rectShader = createShader("rect.vert", "rect.frag");

    // Povezivanje uniformi (potreban nam je samo uTex0)
    glUseProgram(rectShader);
    glUniform1i(glGetUniformLocation(rectShader, "uTex0"), 0); // Povezuje uTex0 sa GL_TEXTURE0

    // Definicija temena za pravougaonik (autobus)
    // Format: pozicija X, pozicija Y, TexCoord S, TexCoord T
    float verticesBus[] = {
        // Gornja leva (x, y, s, t)
        -0.5f,  0.5f, 0.0f, 1.0f,
        // Donja leva
        -0.5f, -0.5f, 0.0f, 0.0f,
        // Donja desna
         0.5f, -0.5f, 1.0f, 0.0f,
         // Gornja desna
          0.5f,  0.5f, 1.0f, 1.0f
    };

    unsigned int VAObus;
    formVAObus(verticesBus, sizeof(verticesBus), VAObus);

    // Postavljanje bele pozadine
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Glavna render petlja
    while (!glfwWindowShouldClose(window))
    {
        // Procesuiranje dogaðaja (zatvaranje prozora)
        glfwPollEvents();

        // Crtanje
        glClear(GL_COLOR_BUFFER_BIT); // Brisanje i bojenje pozadine u belo
        drawBus(rectShader, VAObus);

        glfwSwapBuffers(window); // Prikaz
    }

    // Èišæenje
    glDeleteProgram(rectShader);
    glDeleteVertexArrays(1, &VAObus);
    glDeleteTextures(1, &busTexture);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}