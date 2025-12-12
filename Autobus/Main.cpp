#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include "Util.h"

#define M_PI 3.14159265358979323846


unsigned busTexture;
unsigned stationTexture;
unsigned int colorShader;
unsigned int rectShader;

int screenWidth = 1800;
int screenHeight = 1200;
const float BUS_SCALE = 0.25f; 
const float STATION_SCALE = 0.15f;

int endProgram(std::string message) {
    std::cerr << message << std::endl;
    glfwTerminate();
    return -1;
}

void preprocessTexture(unsigned& texture, const char* filepath) {
    texture = loadImageToTexture(filepath);
    if (texture != 0) {
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

// Funkcija za formiranje VAO-a sa pozicijom i teksturnim koordinatama (za autobus i stanice)
void formVAOTextured(float* vertices, size_t size, unsigned int& VAO) {
    unsigned int VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

    // Atribut 0 (pozicija): x, y
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Atribut 1 (teksturne koordinate): s, t
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Funkcija za formiranje VAO-a samo sa pozicijom (za crvenu putanju)
void formVAOPosition(float* vertices, size_t size, unsigned int& VAO) {
    unsigned int VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

    // Atribut 0 (pozicija): x, y
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


void drawPath(unsigned int pathShader, unsigned int VAOpath, int numPoints) {
    glUseProgram(pathShader);
    glUniform4f(glGetUniformLocation(pathShader, "uColor"), 1.0f, 0.0f, 0.0f, 1.0f);
    glUniform2f(glGetUniformLocation(pathShader, "uPosOffset"), 0.0f, 0.0f);
    glLineWidth(10.0f);
    glBindVertexArray(VAOpath);
    glDrawArrays(GL_LINE_LOOP, 0, numPoints);
    glBindVertexArray(0);
}

// Funkcija za crtanje 10 stanica
void drawStations(unsigned int rectShader, unsigned int VAOstation, float* stationPositions, int numStations) {
    glUseProgram(rectShader);

    // Aktiviranje teksture stanice
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, stationTexture);

    glBindVertexArray(VAOstation);
    for (int i = 0; i < numStations; ++i) {
        float x = stationPositions[2 * i];
        float y = stationPositions[2 * i + 1];

        // Skaliranje i pozicija
        glUniform1f(glGetUniformLocation(rectShader, "uX"), x);
        glUniform1f(glGetUniformLocation(rectShader, "uY"), y);
        glUniform1f(glGetUniformLocation(rectShader, "uS"), STATION_SCALE);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    glBindVertexArray(0);
}

// Funkcija za crtanje autobusa
void drawBus(unsigned int rectShader, unsigned int VAObus, float currentX, float currentY) {
    glUseProgram(rectShader);

    // Skaliranje i pozicija
    glUniform1f(glGetUniformLocation(rectShader, "uX"), currentX);
    glUniform1f(glGetUniformLocation(rectShader, "uY"), currentY);
    glUniform1f(glGetUniformLocation(rectShader, "uS"), BUS_SCALE);

    // Aktiviranje teksture autobusa
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, busTexture);

    glBindVertexArray(VAObus);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}


int main()
{
    // GLFW, GLEW, GL_BLEND inicijalizacija
    if (!glfwInit()) return endProgram("GLFW nije uspeo da se inicijalizuje.");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Bus Project", NULL, NULL);
    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");
    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- UÈITAVANJE TEKSTURA I ŠEJDERA ---
    preprocessTexture(busTexture, "res/avtobus.png");
    preprocessTexture(stationTexture, "res/busstation.jpeg");

    rectShader = createShader("rect.vert", "rect.frag");
    glUseProgram(rectShader);
    glUniform1i(glGetUniformLocation(rectShader, "uTex0"), 0);

    colorShader = createShader("color.vert", "color.frag");

    // --- DEFINICIJA KOORDINATA ---
    const int NUM_STATIONS = 10;
    float verticesBus[] = { -0.5f, 0.5f, 0.0f, 1.0f, -0.5f, -0.5f, 0.0f, 0.0f, 0.5f, -0.5f, 1.0f, 0.0f, 0.5f, 0.5f, 1.0f, 1.0f };
    float verticesStation[] = { -0.5f, 0.5f, 0.0f, 1.0f, -0.5f, -0.5f, 0.0f, 0.0f, 0.5f, -0.5f, 1.0f, 0.0f, 0.5f, 0.5f, 1.0f, 1.0f };


    // Koordinate 10 stanica
    float stationPositions[NUM_STATIONS * 2];
    float a = 0.8f; // Poluosa a (x)
    float b = 0.5f; // Poluosa b (y)

    for (int i = 0; i < NUM_STATIONS; ++i) {
        float angle = i * 2 * M_PI / NUM_STATIONS;
        stationPositions[2 * i] = cos(angle) * a;
        stationPositions[2 * i + 1] = sin(angle) * b;
    }

    // Temena za putanju
    float verticesPath[NUM_STATIONS * 2];
    for (int i = 0; i < NUM_STATIONS * 2; ++i) {
        verticesPath[i] = stationPositions[i];
    }

    // --- FORMIRANJE VAO-ova ---
    unsigned int VAObus;
    formVAOTextured(verticesBus, sizeof(verticesBus), VAObus);

    unsigned int VAOstation;
    formVAOTextured(verticesStation, sizeof(verticesStation), VAOstation);

    unsigned int VAOpath;
    formVAOPosition(verticesPath, sizeof(verticesPath), VAOpath);

    // --- POZICIJA AUTOBUSA ---
    // Postavljamo autobus na prvu stanicu na putanji
    float busX = stationPositions[0];
    float busY = stationPositions[1];

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Glavna render petlja
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT);

        // 1. Crtanje putanje (crvena linija)
        drawPath(colorShader, VAOpath, NUM_STATIONS);

        // 2. Crtanje 10 STANICA (sada su smanjene)
        drawStations(rectShader, VAOstation, stationPositions, NUM_STATIONS);

        // 3. Crtanje AUTOBUSA (sada je smanjen i postavljen na liniju)
        drawBus(rectShader, VAObus, busX, busY);

        glfwSwapBuffers(window);
    }

    // Èišæenje
    glDeleteProgram(rectShader);
    glDeleteProgram(colorShader);
    glDeleteVertexArrays(1, &VAObus);
    glDeleteVertexArrays(1, &VAOstation);
    glDeleteVertexArrays(1, &VAOpath);
    glDeleteTextures(1, &busTexture);
    glDeleteTextures(1, &stationTexture);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}