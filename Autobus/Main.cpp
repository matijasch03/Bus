#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>
#include "Util.h"

#define M_PI 3.14159265358979323846


unsigned busTexture;
unsigned stationTexture;
unsigned int colorShader;
unsigned int rectShader;
unsigned closedIconTexture;
unsigned openIconTexture;
unsigned controlIconTexture;
bool showControls = false;

int screenWidth = 1600;
int screenHeight = 1200;
const float BUS_SCALE = 0.25f; 
const float STATION_SCALE = 0.15f;

// --- Konstante kretanja ---
const float TRAVEL_TIME_SECONDS = 5.0f; // Vreme putovanja izmedju dve stanice (ukupna duzina puta)
const float STATION_WAIT_SECONDS = 10.0f; // Vreme cekanja na stanici

int currentStationIndex = 0;
float currentSegmentTime = 0.0f; // Vreme proteklo od pocetka segmenta
bool isWaiting = true; // Da li autobus ceka na stanici
float waitTimer = 0.0f; // Tajmer cekanja
int passengersNumber = 0;
int punishmentNumber = 0;

double lastTime; // Koristi se za deltaTime

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
void formVAOPosition(std::vector<float> vertices, size_t size, unsigned int& VAO) {
    unsigned int VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, size, vertices.data(), GL_STATIC_DRAW);

    // Atribut 0 (pozicija): x, y
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

float randomOffset(float range) {
    // Generisanje broja izmedju 0 i 1, pa skaliranje i pomeranje
    return (float(rand()) / RAND_MAX * 2.0f - 1.0f) * range;
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

void drawStatusIcon(unsigned int rectShader, unsigned int VAO,
    unsigned int closedTex, unsigned int openTex,
    bool isWaiting) {

    glUseProgram(rectShader);

    // Biranje teksture
    unsigned int currentTex = isWaiting ? openTex : closedTex;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, currentTex);

    const float ICON_SCALE = 0.2f; // Skaliranje ikone
    const float ICON_X = 1.0f - (ICON_SCALE / 2.0f) - 0.05f;
    const float ICON_Y = 1.0f - (ICON_SCALE / 2.0f) - 0.05f;

    glUniform1f(glGetUniformLocation(rectShader, "uX"), ICON_X);
    glUniform1f(glGetUniformLocation(rectShader, "uY"), ICON_Y);
    glUniform1f(glGetUniformLocation(rectShader, "uS"), ICON_SCALE);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}

void drawControlIcon(unsigned int rectShader, unsigned int VAO, unsigned int controlTex) {
    glUseProgram(rectShader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, controlTex);

    const float CONTROL_SCALE = 0.30f;
    const float MARGIN = 0.05f;

    const float CONTROL_X = -1.0f + (CONTROL_SCALE / 2.0f) + MARGIN;
    const float CONTROL_Y = 1.0f - (CONTROL_SCALE / 2.0f) - MARGIN;

    glUniform1f(glGetUniformLocation(rectShader, "uX"), CONTROL_X);
    glUniform1f(glGetUniformLocation(rectShader, "uY"), CONTROL_Y);
    glUniform1f(glGetUniformLocation(rectShader, "uS"), CONTROL_SCALE);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}

// Funkcija za obradu unosa sa tastature
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (isWaiting) {
        if (key == GLFW_KEY_K && action == GLFW_PRESS && !showControls) {
            showControls = true;
            if (passengersNumber != 0)
			    punishmentNumber = rand() % passengersNumber;
            passengersNumber++;
            std::cout << "Broj putnika: " << passengersNumber << std::endl;
        }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (isWaiting) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && passengersNumber < 50) {
            passengersNumber++;
            std::cout << "Broj putnika: " << passengersNumber << std::endl;
        }

        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS && passengersNumber > 0) {
            passengersNumber--;
            std::cout << "Broj putnika: " << passengersNumber << std::endl;
        }
    }
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

    srand(time(NULL));
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // --- UÈITAVANJE TEKSTURA I ŠEJDERA ---
    preprocessTexture(busTexture, "res/avtobus.png");
    preprocessTexture(stationTexture, "res/busstation.jpeg");
    preprocessTexture(closedIconTexture, "res/zatvorena.png");
    preprocessTexture(openIconTexture, "res/otvorena.png");
    preprocessTexture(controlIconTexture, "res/kontrola.png");

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

    std::vector<float> pathVertices;
    const int CURVE_POINTS_PER_SEGMENT = 5; // broj taèaka izmeðu dve stanice
    const float WIGGLE_RANGE = 0.08f;

    // Prolazimo kroz 10 segmenata (od Stanice i do Stanice i+1)
    for (int i = 0; i < NUM_STATIONS; ++i) {

        float x1 = stationPositions[2 * i];
        float y1 = stationPositions[2 * i + 1];

		float x2 = stationPositions[2 * ((i + 1) % NUM_STATIONS)]; // modul za povratak na prvu stanicu
        float y2 = stationPositions[2 * ((i + 1) % NUM_STATIONS) + 1];

        pathVertices.push_back(x1);
        pathVertices.push_back(y1);

		// Dodavanje krivudavih taèaka izmeðu dve stanice
        for (int j = 1; j < CURVE_POINTS_PER_SEGMENT; ++j) {

            // Faktor interpolacije (t ide od 0 do 1)
            float t = (float)j / CURVE_POINTS_PER_SEGMENT;

            // Linearna interpolacija (taèka na ravnoj liniji)
            float interX = x1 * (1.0f - t) + x2 * t;
            float interY = y1 * (1.0f - t) + y2 * t;

            // Dodavanje nasumiènog pomeraja (WIGGLE)
            float wiggleFactor = sin(t * M_PI);
            float wiggleX = randomOffset(WIGGLE_RANGE * wiggleFactor);
            float wiggleY = randomOffset(WIGGLE_RANGE * wiggleFactor);

            pathVertices.push_back(interX + wiggleX);
            pathVertices.push_back(interY + wiggleY);
        }
    }

    // --- FORMIRANJE VAO-ova ---
    unsigned int VAObus;
    formVAOTextured(verticesBus, sizeof(verticesBus), VAObus);

    unsigned int VAOstation;
    formVAOTextured(verticesStation, sizeof(verticesStation), VAOstation);

    unsigned int VAOpath;
    size_t pathDataSize = pathVertices.size() * sizeof(float);
    formVAOPosition(pathVertices, pathDataSize, VAOpath);
    int totalPathPoints = pathVertices.size() / 2;

    // --- POZICIJA AUTOBUSA ---
    // Postavljamo autobus na prvu stanicu na putanji
    float busX = stationPositions[0];
    float busY = stationPositions[1];

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    lastTime = glfwGetTime();

    // Glavna render petlja
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT);

        // 1. IZRAÈUNAVANJE VREMENA (DeltaTime)
        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - lastTime); // Vreme proteklo od proslog frejma
        lastTime = currentTime;

        // 2. LOGIKA KRETANJA I STAJANJA
        if (isWaiting) {
            waitTimer += deltaTime;
            if (waitTimer >= STATION_WAIT_SECONDS) {
                isWaiting = false;
                currentSegmentTime = 0.0f; // reset
                waitTimer = 0.0f;
				currentStationIndex = (currentStationIndex + 1) % NUM_STATIONS; // Sledeæa stanica
            }
        }
        else {
            // --- LOGIKA PUTOVANJA ---
            currentSegmentTime += deltaTime;
            float t = currentSegmentTime / TRAVEL_TIME_SECONDS;

            if (t >= 1.0f) {
                // Stigli smo do sledece stanice!
                if (showControls) {
					passengersNumber -= punishmentNumber + 1;
                    std::cout << "Kazna zbog kontrole: " << punishmentNumber << " putnika." << std::endl;
                    std::cout << "Broj putnika nakon kazne: " << passengersNumber << std::endl;
                    showControls = false;
					punishmentNumber = 0;
				}
                t = 1.0f; 
                isWaiting = true;
            }

            // Polazna stanica (A)
            int startIdx = ((currentStationIndex - 1 + NUM_STATIONS) % NUM_STATIONS) * 2;
            float xA = stationPositions[startIdx];
            float yA = stationPositions[startIdx + 1];

            // Odredisna stanica (B)
            int endIdx = currentStationIndex * 2;
            float xB = stationPositions[endIdx];
            float yB = stationPositions[endIdx + 1];

            busX = xA * (1.0f - t) + xB * t;
            busY = yA * (1.0f - t) + yB * t;
        }

        // Crtanje putanje, stanica i autobusa
        drawPath(colorShader, VAOpath, totalPathPoints);
        drawStations(rectShader, VAOstation, stationPositions, NUM_STATIONS);
        drawBus(rectShader, VAObus, busX, busY);
        drawStatusIcon(rectShader, VAObus, closedIconTexture, openIconTexture, isWaiting);
        if (showControls) {
            drawControlIcon(rectShader, VAObus, controlIconTexture);
        }

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