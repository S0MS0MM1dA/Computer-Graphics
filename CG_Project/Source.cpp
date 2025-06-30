#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <Windows.h>
#include <mmsystem.h>

#pragma comment(lib,"Winmm.lib")


GLuint mainTexture,rainTexture, nightTexture;
GLuint fighterPlaneTexture01, fighterPlaneTexture02;
GLuint missileTexture01, missileTexture02;
GLuint tankTexture01;
GLuint brokenTexture;

int current = 1;            

int fighterPlaneImgWidth01, fighterPlaneImgHeight01;
int fighterPlaneImgWidth02, fighterPlaneImgHeight02;
int missileWidth01, missileHeight01;
int missileWidth02, missileHeight02;
int tankWidth01, tankHeight01;

float fighterPlaneX1 = -1.2f, fighterPlaneY1 = 0.8f;
const float fighterPlaneSpeedX1 = 0.007f, fighterPlaneSpeedY1 = -0.0003f;

float fighterPlaneX2 = 1.2f, fighterPlaneY2 = 0.6f;
const float fighterPlaneSpeedX2 = -0.0065f, fighterPlaneSpeedY2 = -0.00028f;

float missileX1 = fighterPlaneX1, missileY1 = fighterPlaneY1 - 0.1f;
const float missileSpeedX1 = 0.01f, missileSpeedY1 = -0.02f;

float missileX2 = fighterPlaneX2, missileY2 = fighterPlaneY2 - 0.1f;
const float missileSpeedX2 = -0.01f, missileSpeedY2 = -0.02f;

float tankX1 = 1.2f, tankY1 = 0.15f;
float tankSpeedX1 = -0.002f, tankSpeedY1 = -0.00120f;

bool plane1Active = false;
bool plane2Active = false;
bool missile1Active = false;
bool missile2Active = false;
bool tank1Active = false;
bool tank1Stop = false;

bool  showBlast1 = false, showBlast2 = false;
float blastTime1 = 0.f, blastTime2 = 0.f;
float blastX1 = 0.f, blastX2 = 0.f;
const float blastGroundY = -0.62f;

// Impact / Scorch / Flood registry
const int   MAX_IMPACTS = 32;
float impactX[MAX_IMPACTS], impactY[MAX_IMPACTS];
float floodProgress[MAX_IMPACTS] = { 0.f };        
int   impactCount = 0;
const float scorchRadius = 0.18f;                
const float floodFillDuration = 8.0f;              


// Flood‑drain control 
bool  floodDraining = false;  
const float floodDrainTime = 10.f;   


// Scenes
enum SceneType { DAY, NIGHT, RAIN };
SceneType currentScene = DAY;

// sound
//bool missileBlasteSound = false;
bool rainSound = false;
void playRainSound() {
    PlaySound(TEXT("Sound/rain.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
}
void stopRainSound() {
    PlaySound(NULL, 0, 0); // Stops any currently playing sound.
}

/*
void playMissileBlastSound() {
    PlaySound(TEXT("Sound/missile_blast.wav"), NULL, SND_FILENAME | SND_ASYNC);
    Sleep(500); // Wait 0.5 sec
    PlaySound(NULL, 0, 0); // Stop the missile blast
}
*/


// Rain 
const int NUM_RAINDROPS = 300;

float raindropX[NUM_RAINDROPS];
float raindropY[NUM_RAINDROPS];
float raindropSpeedY[NUM_RAINDROPS];
float raindropLength[NUM_RAINDROPS];
float raindropAlpha[NUM_RAINDROPS];
float raindropSlantX[NUM_RAINDROPS];

void initRain() {
    for (int i = 0; i < NUM_RAINDROPS; i++) {
        raindropX[i] = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        raindropY[i] = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        raindropSpeedY[i] = 0.01f + (rand() / (float)RAND_MAX) * 0.02f;
        raindropLength[i] = 0.04f + (rand() / (float)RAND_MAX) * 0.06f;
        raindropAlpha[i] = 0.3f + (rand() / (float)RAND_MAX) * 0.5f;
        raindropSlantX[i] = ((rand() / (float)RAND_MAX) * 0.01f) - 0.005f;
    }
}

void updateRain() {
    for (int i = 0; i < NUM_RAINDROPS; ++i) {
        raindropY[i] -= raindropSpeedY[i];
        if (raindropY[i] < -1.0f) {
            raindropY[i] = 1.0f;
            raindropX[i] = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
            raindropLength[i] = 0.04f + (rand() / (float)RAND_MAX) * 0.06f;
            raindropAlpha[i] = 0.3f + (rand() / (float)RAND_MAX) * 0.5f;
            raindropSlantX[i] = ((rand() / (float)RAND_MAX) * 0.01f) - 0.005f;
        }
    }
}

void drawRain() {
    glDisable(GL_TEXTURE_2D);
    glLineWidth(1.2f);
    glBegin(GL_LINES);
    for (int i = 0; i < NUM_RAINDROPS; ++i) {
        glColor4f(0.6f, 0.6f, 1.0f, raindropAlpha[i]);
        glVertex2f(raindropX[i], raindropY[i]);
        glVertex2f(raindropX[i] + raindropSlantX[i],
            raindropY[i] - raindropLength[i]);
    }
    glEnd();
    glEnable(GL_TEXTURE_2D);
    glColor3f(1, 1, 1);
}


// Texture Loader
GLuint loadTexture(const char* file, int* outW = nullptr, int* outH = nullptr) {
    int width, height, ch;
    if (unsigned char* img = stbi_load(file, &width, &height, &ch, 0)) {
        if (outW) *outW = width;
        if (outH) *outH = height;
        GLuint id;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, width, height, 0, fmt, GL_UNSIGNED_BYTE, img);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(img);
        return id;
    }
    fprintf(stderr, "Failed to load %s\n", file);
    return 0;
}

// Blast, Scorch & Flood Drawing
void drawBlast(float x, float y, float r) {
    glColor4f(1.f, 0.5f, 0.f, 1.f - r * 0.8f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 360; ++i) {
        float a = i * 3.14159f / 180.f;
        glVertex2f(x + cosf(a) * r, y + sinf(a) * r);
    }
    glEnd();
    glColor3f(1, 1, 1);
}

void drawScorch(float x, float y, float radius = scorchRadius) {
    // Draw dark circle (optional)
    glDisable(GL_TEXTURE_2D);
    glColor4f(0.05f, 0.05f, 0.05f, 0.8f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 360; ++i) {
        float a = i * 3.14159f / 180.f;
        glVertex2f(x + cosf(a) * radius, y + sinf(a) * radius);
    }
    glEnd();

    // Draw round texture on top
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, brokenTexture); // <-- your round image texture
    glColor4f(1, 1, 1, 1); // Full texture color

    glPushMatrix();
    glTranslatef(x, y, 0);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-radius, -radius);
    glTexCoord2f(1, 0); glVertex2f(radius, -radius);
    glTexCoord2f(1, 1); glVertex2f(radius, radius);
    glTexCoord2f(0, 1); glVertex2f(-radius, radius);
    glEnd();
    glPopMatrix();
}

// *** FLOOD ADD 
void drawFlood(float x, float y, float progress, float radius = scorchRadius) {
    if (progress <= 0.f) return;

    const float waterRadius = radius * progress;   // grows outward
    glDisable(GL_TEXTURE_2D);
    glColor4f(0.2f, 0.4f, 1.0f, 0.6f);            // soft blue
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 360; ++i) {
        float a = i * 3.14159f / 180.f;
        glVertex2f(x + cosf(a) * waterRadius, y + sinf(a) * waterRadius);
    }
    glEnd();
    glEnable(GL_TEXTURE_2D);
    glColor3f(1, 1, 1);
}

inline void clearFlood() {        
    floodDraining = true;
}

// - - - - Objects
void drawObjects() {
    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);

    // Plane 1
    glPushMatrix();
    glTranslatef(fighterPlaneX1, fighterPlaneY1, 0);
    glBindTexture(GL_TEXTURE_2D, fighterPlaneTexture01);
    float fw1 = 0.35f,
        fh1 = fw1 / fighterPlaneImgWidth01 * fighterPlaneImgHeight01;
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-fw1 / 2, -fh1 / 2);
    glTexCoord2f(1, 0); glVertex2f(fw1 / 2, -fh1 / 2);
    glTexCoord2f(1, 1); glVertex2f(fw1 / 2, fh1 / 2);
    glTexCoord2f(0, 1); glVertex2f(-fw1 / 2, fh1 / 2);
    glEnd();
    glPopMatrix();

    // Missile 1
    glPushMatrix();
    glTranslatef(missileX1, missileY1, 0);
    glBindTexture(GL_TEXTURE_2D, missileTexture01);
    float mw1 = 0.25f,
        mh1 = mw1 / missileWidth01 * missileHeight01;
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-mw1 / 2, -mh1 / 2);
    glTexCoord2f(1, 0); glVertex2f(mw1 / 2, -mh1 / 2);
    glTexCoord2f(1, 1); glVertex2f(mw1 / 2, mh1 / 2);
    glTexCoord2f(0, 1); glVertex2f(-mw1 / 2, mh1 / 2);
    glEnd();
    glPopMatrix();

    // Plane 2  
    glPushMatrix();
    glTranslatef(fighterPlaneX2, fighterPlaneY2, 0);
    glScalef(-1.f, 1.f, 1.f);
    glBindTexture(GL_TEXTURE_2D, fighterPlaneTexture02);
    float fw2 = 0.35f,
        fh2 = fw2 / fighterPlaneImgWidth02 * fighterPlaneImgHeight02;
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-fw2 / 2, -fh2 / 2);
    glTexCoord2f(1, 0); glVertex2f(fw2 / 2, -fh2 / 2);
    glTexCoord2f(1, 1); glVertex2f(fw2 / 2, fh2 / 2);
    glTexCoord2f(0, 1); glVertex2f(-fw2 / 2, fh2 / 2);
    glEnd();
    glPopMatrix();

    // Missile 2 
    glPushMatrix();
    glTranslatef(missileX2, missileY2, 0);
    glBindTexture(GL_TEXTURE_2D, missileTexture02);
    float mw2 = 0.25f,
        mh2 = mw2 / missileWidth02 * missileHeight02;
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-mw2 / 2, -mh2 / 2);
    glTexCoord2f(1, 0); glVertex2f(mw2 / 2, -mh2 / 2);
    glTexCoord2f(1, 1); glVertex2f(mw2 / 2, mh2 / 2);
    glTexCoord2f(0, 1); glVertex2f(-mw2 / 2, mh2 / 2);
    glEnd();
    glPopMatrix();

    //Tank 1
    glPushMatrix();
    glTranslatef(tankX1, tankY1, 0);
    glBindTexture(GL_TEXTURE_2D, tankTexture01);
    float tw1 = 0.35f,
        th1 = tw1 / tankWidth01 * tankHeight01;
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-tw1 / 2, -th1 / 2);
    glTexCoord2f(1, 0); glVertex2f(tw1 / 2, -th1 / 2);
    glTexCoord2f(1, 1); glVertex2f(tw1 / 2, th1 / 2);
    glTexCoord2f(0, 1); glVertex2f(-tw1 / 2, th1 / 2);
    glEnd();
    glPopMatrix();

    // Rain
    if (current == 2) {
        drawRain();
    }

    // Scorch marks
    for (int i = 0; i < impactCount; ++i)
        drawScorch(impactX[i], impactY[i]);

    // Flood fill
    for (int i = 0; i < impactCount; ++i)               
        drawFlood(impactX[i], impactY[i], floodProgress[i]); 

    // Blasts
    glDisable(GL_TEXTURE_2D);
    if (showBlast1) {
        drawBlast(blastX1, blastGroundY, blastTime1 * 0.2f);
    }
    if (showBlast2) {
        drawBlast(blastX2, blastGroundY, blastTime2 * 0.2f);
    }

}

// Scene

void drawScene(GLuint bgTexture) {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, bgTexture);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-1, -1);
    glTexCoord2f(1, 0); glVertex2f(1, -1);
    glTexCoord2f(1, 1); glVertex2f(1, 1);
    glTexCoord2f(0, 1); glVertex2f(-1, 1);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    
    drawObjects();
}

void drawDayScene() {
    drawScene(mainTexture);
    glutSwapBuffers();
}

void drawRainScene() {
    drawScene(rainTexture);
    drawRain();
    glutSwapBuffers();
}

void drawNightScene() {
    drawScene(nightTexture);
    glutSwapBuffers();
}


// Display
void display() {
    switch (currentScene) {
    case DAY: drawDayScene(); break;
    case NIGHT: drawNightScene(); break;
    case RAIN: drawRainScene(); break;
    }
}

// Update 
void update(int) {
    // Plane 
    if (plane1Active) {
        fighterPlaneX1 += fighterPlaneSpeedX1;
        //fighterPlaneY1 = fighterPlaneSpeedY1;
        if (fighterPlaneX1 > 1.2f) {
            fighterPlaneX1 = -1.2f;
            // fighterPlaneY1 = 0.6f;  
        }
    }
    if (plane2Active) {
        fighterPlaneX2 += fighterPlaneSpeedX2;
        fighterPlaneY2 += fighterPlaneSpeedY2;
        if (fighterPlaneX2 < -1.2f) {
            fighterPlaneX2 = 1.6f;
            fighterPlaneY2 = 0.6f;
        }
    }

    // Missile 1
    if (missile1Active && !showBlast1) {
        missileX1 += missileSpeedX1;
        missileY1 += missileSpeedY1;
        if (missileY1 <= blastGroundY) {
            showBlast1 = true;
            blastX1 = missileX1;
            blastTime1 = 0.f;
            missile1Active = false;
            /*
            if (!missileBlasteSound) {
                playMissileBlastSound();
                missileBlasteSound = true;
            }
            else {
                if (missileBlasteSound) {
                    PlaySound(NULL, NULL, 0);
                    missileBlasteSound = false;
                }
            }*/
        }
    }
    else if (showBlast1) {
        blastTime1 += 0.03f;
        if (blastTime1 >= 1.f) {
            if (impactCount < MAX_IMPACTS) {
                impactX[impactCount] = blastX1;
                impactY[impactCount] = blastGroundY;
                floodProgress[impactCount] = 0.f;     
                ++impactCount;
            }
            showBlast1 = false;
        }
    }

    // Missile 2
    if (missile2Active && !showBlast2) {
        missileX2 += missileSpeedX2;
        missileY2 += missileSpeedY2;
        if (missileY2 <= blastGroundY) {

            showBlast2 = true;
            blastX2 = missileX2;
            blastTime2 = 0.f;
            missile2Active = false;
            /*
            if (!missileBlasteSound) {
                playMissileBlastSound();
                missileBlasteSound = true;
            }
            else {
                if (missileBlasteSound) {
                    PlaySound(NULL, NULL, 0);
                    missileBlasteSound = false;
                }
            }
            */
            
        }
    }
    else if (showBlast2) {
        blastTime2 += 0.03f;
        if (blastTime2 >= 1.f) {
            if (impactCount < MAX_IMPACTS) {
                impactX[impactCount] = blastX2;
                impactY[impactCount] = blastGroundY;
                floodProgress[impactCount] = 0.f;       
                ++impactCount;
            }
            showBlast2 = false;
        }
    }

    //Tank 01
    if (tank1Active) {
        if (!tank1Stop) {
            tankX1 += tankSpeedX1;
            tankY1 += tankSpeedY1;
        }
        if (tankX1 < -1.2f) {
            tankX1 = 1.2f;
            tankY1 = 0.15f;
        }
    }

    // Rain & flood 
    if (current == 2) {
        updateRain();
        
        const float dt = 0.016f;                       
        for (int i = 0; i < impactCount; ++i) {        
            if (floodProgress[i] < 1.f) {
                floodProgress[i] += dt / floodFillDuration;
                if (floodProgress[i] > 1.f) floodProgress[i] = 1.f;
            }
        }
    }
    // Flood draining
    if (floodDraining && current == 1) {
        const float dt = 0.016f;                  
        bool allDry = true;
        for (int i = 0; i < impactCount; ++i) {
            if (floodProgress[i] > 0.f) {
                floodProgress[i] -= dt / floodDrainTime;
                if (floodProgress[i] < 0.f) floodProgress[i] = 0.f;
                else allDry = false;
            }
        }
        if (allDry) floodDraining = false;         
    }


    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

// -----------------------------------------------------------------------------
// Input
// D  - Day (main scene)
// R - Rain (rain Scene)
// N - Night
// r - rain in any scene
// 1  - fighter pane 1 (left to right)
// 2 - fighter plane 2 (right to left)
// 3 - tank (right to left)
// p - tank puse any time
// s - tank start moving
//  - -  - MOUSE  - - - 
// right click - missile from fighter plane 1
// left click - missile from fighter plane 2
// 
// -----------------------------------------------------------------------------
void keyboard(unsigned char key, int, int) {
    if( key == 'D' || key == 'd') {
        currentScene = DAY;
        current = 1;       

        stopRainSound();
        clearFlood();     
        glutPostRedisplay();
    } 
    if (key == 'R') {
        currentScene = RAIN;
        current = 2;
        initRain();
        playRainSound();
        rainSound = true;

        glutPostRedisplay();
    }
    if (key == 'N') {
        currentScene = NIGHT;
        current = 3;
        stopRainSound();
        clearFlood();
        glutPostRedisplay();
    }

    if (key == '1') {
        plane1Active = true;
        fighterPlaneX1 = -1.3f;
        fighterPlaneY1 = 0.9f;
        missile1Active = false;
        showBlast1 = false;
        missileX1 = fighterPlaneX1;
        missileY1 = fighterPlaneY1 - 0.1f;
    }
    if (key == '2') {
        plane2Active = true;
        fighterPlaneX2 = 1.3f;
        fighterPlaneY2 = 0.6f;
        missile2Active = false;
        showBlast2 = false;
        missileX2 = fighterPlaneX2;
        missileY2 = fighterPlaneY2 - 0.1f;
    }

    //Tank 01
    if (key == '3') {
        tank1Active = true;
        tank1Stop = false;
        tankX1 = 1.2f;
        tankY1 = 0.22f;
    }
    if (key == 'p' || key == 'P') {
        tank1Stop = true;
    }
    if (key == 's' || key == 'S') {
        tank1Stop = false;
    }

    //Rain
    if (key == 'r') {
        current = 2;
        initRain();
        playRainSound();
        rainSound = true;
    }
}

void handleMouse(int button, int state, int, int) {
    if (state == GLUT_DOWN) {
        if (button == GLUT_LEFT_BUTTON && !missile1Active && !showBlast1 && plane1Active) {
            missileX1 = fighterPlaneX1;
            missileY1 = fighterPlaneY1 - 0.1f;
            missile1Active = true;
        }
        if (button == GLUT_RIGHT_BUTTON && !missile2Active && !showBlast2 && plane2Active) {
            missileX2 = fighterPlaneX2;
            missileY2 = fighterPlaneY2 - 0.1f;
            missile2Active = true;
        }
    }
    glutPostRedisplay();
}

// Init
void init() {
    glClearColor(0, 0, 0, 1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    stbi_set_flip_vertically_on_load(true);
    srand(static_cast<unsigned>(time(nullptr)));

    mainTexture = loadTexture("Images/bg_scene_01.png");
    rainTexture = loadTexture("Images/rain_image.png");
    nightTexture = loadTexture("Images/night_image.png");

    fighterPlaneTexture01 = loadTexture("Images/fighter_plane_01.png", &fighterPlaneImgWidth01, &fighterPlaneImgHeight01);
    fighterPlaneTexture02 = loadTexture("Images/fighter_plane_02.png", &fighterPlaneImgWidth02, &fighterPlaneImgHeight02);
    missileTexture01 = loadTexture("Images/missile_01.png", &missileWidth01, &missileHeight01);
    missileTexture02 = loadTexture("Images/missile_02.png", &missileWidth02, &missileHeight02);
    tankTexture01 = loadTexture("Images/tank_01.png", &tankWidth01, &tankHeight01);
    brokenTexture = loadTexture("Images/broken.png", &tankWidth01, &tankHeight01);
}
// Main
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("War Starting scene");
    init();
    glutDisplayFunc(display);
    glutTimerFunc(0, update, 0);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(handleMouse);
    glutMainLoop();
    return 0;
}


