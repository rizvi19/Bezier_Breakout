#if __has_include(<GL/freeglut.h>)
#include <GL/freeglut.h>
#else
#include <GL/glut.h>
#endif

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#else
#include <sys/stat.h>
#endif

// ==================================================
// 1. includes and constants
// ==================================================

const int WINDOW_W = 1120;
const int WINDOW_H = 760;
const float ARENA_LEFT = -9.0f;
const float ARENA_RIGHT = 9.0f;
const float ARENA_BOTTOM = -8.2f;
const float ARENA_TOP = 8.2f;
const float PI = 3.1415926535f;

enum GameState { MENU, PLAYING, PAUSED, LEVEL_COMPLETE, GAME_OVER, GAME_WON };
enum BrickType { NORMAL, STRONG, RED_SPEED, BLUE_SLOW, GREEN_SCORE, YELLOW_SPLIT, BLACK_GRAVITY, SHIELD };
enum PowerType { POWER_NONE, POWER_WIDE, POWER_SLOW, POWER_SPECIAL };

// ==================================================
// 2. structs and globals
// ==================================================

struct Vec3 {
    float x, y, z;
    Vec3(float X = 0, float Y = 0, float Z = 0) : x(X), y(Y), z(Z) {}
};

Vec3 operator+(Vec3 a, Vec3 b) { return Vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
Vec3 operator-(Vec3 a, Vec3 b) { return Vec3(a.x - b.x, a.y - b.y, a.z - b.z); }
Vec3 operator*(Vec3 a, float s) { return Vec3(a.x * s, a.y * s, a.z * s); }
Vec3 operator/(Vec3 a, float s) { return Vec3(a.x / s, a.y / s, a.z / s); }

struct Ball {
    Vec3 p, v;
    float r;
    bool launched;
    bool alive;
    bool inTunnel;
    bool tunnelForward;
    float tunnelT;
    float tunnelCooldown;
    float portalCooldown;
    float bossWaveCooldown;
};

struct Paddle {
    Vec3 p;
    float w, h, speed;
    float wideTimer;
};

struct Brick {
    Vec3 p;
    float w, h, d;
    int hp, maxHp;
    BrickType type;
    bool alive;
    bool moving;
    float baseX, movePhase, flash;
};

struct Portal {
    Vec3 p;
    float r;
    bool active;
};

struct BlackHole {
    Vec3 p;
    float r;
    float strength;
    bool active;
};

struct WhiteHole {
    Vec3 p;
    float r;
    float strength;
    bool active;
};

struct BezierTunnel {
    Vec3 p0, p1, p2, p3;
    float radius;
    bool active;
};

struct Mirror {
    Vec3 p;
    float length;
    float angle;
    float speed;
    bool active;
};

struct PowerUp {
    Vec3 p;
    PowerType type;
    bool active;
    float spin;
};

struct Particle {
    Vec3 p, v;
    float life;
    float r, g, b;
};

struct Boss {
    Vec3 p;
    int hp, maxHp;
    bool active;
    float shieldAngle;
    float pulse;
    float shockwaveRadius;
    float shockwaveTimer;
};

GameState gameState = MENU;
GameState stateBeforeExitPrompt = MENU;
Paddle paddle;
std::vector<Ball> balls;
std::vector<Brick> bricks;
std::vector<PowerUp> powerUps;
std::vector<Particle> particles;
std::vector<Vec3> ballTrail;
Portal portals[2];
BlackHole blackHole;
WhiteHole whiteHole;
BezierTunnel tunnel;
Mirror mirrors[3];
Boss boss;

int score = 0;
int lives = 3;
int levelNo = 1;
int cameraMode = 0;
int lastTimeMs = 0;
int specialCharges = 0;
bool keyLeft = false, keyRight = false;
bool showHelp = true;
bool showGraphicsOverlay = false;
bool exitPrompt = false;
bool depthEnabled = true;
bool muted = false;
float globalTime = 0.0f;
bool soundReady = false;
std::string soundPlayer = "";
const std::string SOUND_DIR = "sounds";

// ==================================================
// 3. utility functions
// ==================================================

float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

float len(Vec3 a) {
    return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

Vec3 normalize(Vec3 a) {
    float l = len(a);
    if (l < 0.0001f) return Vec3(0, 1, 0);
    return a / l;
}

float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 reflect(Vec3 v, Vec3 n) {
    return v - n * (2.0f * dot(v, n));
}

float randf(float a, float b) {
    return a + (b - a) * (float(rand()) / float(RAND_MAX));
}

void setColor(float r, float g, float b, float a = 1.0f, float emission = 0.0f) {
    GLfloat diffuse[] = {r, g, b, a};
    GLfloat ambient[] = {r * 0.25f, g * 0.25f, b * 0.25f, a};
    GLfloat specular[] = {0.8f, 0.8f, 0.8f, a};
    GLfloat emit[] = {r * emission, g * emission, b * emission, a};
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emit);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 64.0f);
    glColor4f(r, g, b, a);
}

std::string brickName(BrickType type) {
    switch (type) {
        case RED_SPEED: return "RED speed";
        case BLUE_SLOW: return "BLUE slow";
        case GREEN_SCORE: return "GREEN score";
        case YELLOW_SPLIT: return "YELLOW split";
        case BLACK_GRAVITY: return "BLACK gravity";
        case SHIELD: return "Boss shield";
        case STRONG: return "Strong";
        default: return "Normal";
    }
}

std::string currentPowerText() {
    if (paddle.wideTimer > 0.0f) return "Wide Paddle";
    if (specialCharges > 0) return "Special Ready";
    return "None";
}

// ==================================================
// 4. sound
// ==================================================

void writeLE16(std::ofstream& out, int v) {
    out.put(char(v & 255));
    out.put(char((v >> 8) & 255));
}

void writeLE32(std::ofstream& out, int v) {
    out.put(char(v & 255));
    out.put(char((v >> 8) & 255));
    out.put(char((v >> 16) & 255));
    out.put(char((v >> 24) & 255));
}

std::string soundPath(const std::string& name) {
    return SOUND_DIR + "/" + name + ".wav";
}

void writeToneWav(const std::string& path, float startHz, float endHz, float seconds, float volume, int style) {
    const int sampleRate = 44100;
    const int count = std::max(1, int(sampleRate * seconds));
    std::vector<int16_t> samples(count);

    float phase = 0.0f;
    for (int i = 0; i < count; ++i) {
        float t = float(i) / float(count - 1);
        float hz = startHz + (endHz - startHz) * t;
        phase += 2.0f * PI * hz / sampleRate;
        float envelope = std::sin(PI * t);
        float wave = std::sin(phase);
        if (style == 1) {
            wave = std::sin(phase) * 0.82f + std::sin(phase * 1.55f) * 0.18f;
        } else if (style == 2) {
            wave = std::sin(phase) * 0.72f + std::sin(phase * 0.52f + t * 5.0f) * 0.28f;
        } else if (style == 3) {
            float hitEnvelope = std::exp(-8.0f * t);
            envelope = std::max(0.0f, 1.0f - t) * hitEnvelope;
            wave = std::sin(phase) * 0.75f + std::sin(phase * 1.8f) * 0.25f;
        }
        samples[i] = int16_t(clampf(wave * envelope * volume, -1.0f, 1.0f) * 32767);
    }

    std::ofstream out(path.c_str(), std::ios::binary);
    if (!out) return;
    int dataBytes = count * 2;
    out.write("RIFF", 4);
    writeLE32(out, 36 + dataBytes);
    out.write("WAVEfmt ", 8);
    writeLE32(out, 16);
    writeLE16(out, 1);
    writeLE16(out, 1);
    writeLE32(out, sampleRate);
    writeLE32(out, sampleRate * 2);
    writeLE16(out, 2);
    writeLE16(out, 16);
    out.write("data", 4);
    writeLE32(out, dataBytes);
    out.write(reinterpret_cast<const char*>(&samples[0]), dataBytes);
}

bool commandExists(const std::string& command) {
#ifdef _WIN32
    (void)command;
    return true;
#else
    std::string test = "command -v " + command + " >/dev/null 2>&1";
    return std::system(test.c_str()) == 0;
#endif
}

void initSound() {
    if (soundReady) return;
#ifdef _WIN32
    _mkdir(SOUND_DIR.c_str());
#else
    mkdir(SOUND_DIR.c_str(), 0755);
    if (commandExists("paplay")) soundPlayer = "paplay";
    else if (commandExists("pw-play")) soundPlayer = "pw-play";
    else if (commandExists("aplay")) soundPlayer = "aplay";
    else if (commandExists("ffplay")) soundPlayer = "ffplay -nodisp -autoexit -loglevel quiet";
#endif

    writeToneWav(soundPath("paddle"), 165, 82, 0.12f, 0.72f, 3);
    writeToneWav(soundPath("wall"), 138, 68, 0.10f, 0.62f, 3);
    writeToneWav(soundPath("break"), 210, 88, 0.18f, 0.72f, 1);
    writeToneWav(soundPath("portal"), 190, 78, 0.24f, 0.62f, 2);
    writeToneWav(soundPath("power"), 230, 128, 0.22f, 0.66f, 1);
    writeToneWav(soundPath("level"), 260, 155, 0.36f, 0.68f, 1);
    writeToneWav(soundPath("boss"), 105, 42, 0.28f, 0.78f, 2);
    writeToneWav(soundPath("gameover"), 132, 38, 0.48f, 0.68f, 2);
    soundReady = true;
}

void playSoundEvent(const char* name) {
    if (muted) return;
    initSound();
    std::string eventName = name ? name : "wall";
    if (eventName != "paddle" && eventName != "wall" && eventName != "break" &&
        eventName != "portal" && eventName != "power" && eventName != "level" &&
        eventName != "boss" && eventName != "gameover") {
        eventName = "wall";
    }
    std::string path = soundPath(eventName);
#ifdef _WIN32
    PlaySoundA(path.c_str(), NULL, SND_FILENAME | SND_ASYNC);
#else
    if (!soundPlayer.empty()) {
        std::string cmd = soundPlayer + " \"" + path + "\" >/dev/null 2>&1 &";
        std::system(cmd.c_str());
    } else {
        std::cout << '\a' << std::flush;
    }
#endif
}

// ==================================================
// 5. Bezier functions
// ==================================================

Vec3 bezierPoint(const BezierTunnel& c, float t) {
    float u = 1.0f - t;
    return c.p0 * (u * u * u) +
           c.p1 * (3.0f * u * u * t) +
           c.p2 * (3.0f * u * t * t) +
           c.p3 * (t * t * t);
}

Vec3 bezierTangent(const BezierTunnel& c, float t) {
    float u = 1.0f - t;
    Vec3 a = (c.p1 - c.p0) * (3.0f * u * u);
    Vec3 b = (c.p2 - c.p1) * (6.0f * u * t);
    Vec3 d = (c.p3 - c.p2) * (3.0f * t * t);
    return normalize(a + b + d);
}

// ==================================================
// 6. initialization
// ==================================================

void resetBall(bool keepLives = true) {
    balls.clear();
    Ball b;
    b.p = Vec3(paddle.p.x, paddle.p.y + 0.65f, 0.0f);
    b.v = Vec3(3.0f, 5.0f + levelNo * 0.25f, 0.0f);
    b.r = 0.23f;
    b.launched = false;
    b.alive = true;
    b.inTunnel = false;
    b.tunnelForward = true;
    b.tunnelT = 0.0f;
    b.tunnelCooldown = 0.0f;
    b.portalCooldown = 0.0f;
    b.bossWaveCooldown = 0.0f;
    balls.push_back(b);


    ballTrail.clear();
    if (!keepLives) lives = 3;
}

void initFeatureDefaults() {
    blackHole = {Vec3(0, 1.2f, 0), 1.25f, 15.0f, false};
    whiteHole = {Vec3(3.8f, 0.5f, 0), 1.0f, 7.0f, false};
    portals[0] = {Vec3(-6.6f, -1.0f, 0), 0.62f, false};
    portals[1] = {Vec3(6.6f, 3.2f, 0), 0.62f, false};
    tunnel = {Vec3(-7.2f, -1.5f, 0), Vec3(-4.5f, 5.2f, 0),
              Vec3(3.6f, -4.8f, 0), Vec3(7.2f, 2.2f, 0), 0.55f, false};
    for (int i = 0; i < 3; ++i) mirrors[i] = {Vec3(0, 0, 0), 3.4f, 0, 0, false};
    boss = {Vec3(0, 3.0f, 0), 10, 10, false, 0.0f, 0.0f, -1.0f, 2.8f};
}

void addBrick(float x, float y, BrickType type, int hp, bool moving = false) {
    Brick b;
    b.p = Vec3(x, y, 0);
    b.w = 1.38f;
    b.h = 0.48f;
    b.d = 0.42f;
    b.hp = hp;
    b.maxHp = hp;
    b.type = type;
    b.alive = true;
    b.moving = moving;
    b.baseX = x;
    b.movePhase = randf(0.0f, 6.28f);
    b.flash = 0.0f;
    bricks.push_back(b);
}

void makeBrickGrid(int rows, int cols, float startY, int strongEvery, bool moving, bool rgb) {
    float gap = 0.23f;
    float bw = 1.38f;
    float total = cols * bw + (cols - 1) * gap;
    float startX = -total * 0.5f + bw * 0.5f;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            BrickType type = NORMAL;
            int hp = 1;
            if (strongEvery > 0 && (r + c) % strongEvery == 0) {
                type = STRONG;
                hp = 2 + (levelNo >= 4);
            }
            if (rgb) {
                int pick = (r * cols + c) % 7;
                if (pick == 0) type = RED_SPEED;
                if (pick == 1) type = BLUE_SLOW;
                if (pick == 2) type = GREEN_SCORE;
                if (pick == 3) type = YELLOW_SPLIT;
                if (pick == 4) { type = BLACK_GRAVITY; hp = 2; }
            }
            addBrick(startX + c * (bw + gap), startY + r * 0.72f, type, hp, moving && (r == rows - 1 || r == rows - 2 || r == rows - 3));
        }
    }
}

void setupLevel(int n) {
    levelNo = clampf((float)n, 1.0f, 8.0f);
    bricks.clear();
    powerUps.clear();
    particles.clear();
    initFeatureDefaults();
    // paddle = {position, width, height, speed, wideTimer};
    paddle = {Vec3(0, -7.25f, 0), 2.45f, 0.35f, 8.5f, 0.0f};

    if (levelNo == 1) {
        makeBrickGrid(4, 9, 3.2f, 0, false, false);
    } else if (levelNo == 2) {
        makeBrickGrid(5, 9, 2.8f, 3, true, false);
    } else if (levelNo == 3) {
        makeBrickGrid(5, 10, 3.0f, 4, true, false);
        tunnel.active = true;
    } else if (levelNo == 4) {
        makeBrickGrid(6, 10, 2.4f, 2, true, false);
        blackHole.active = true;
        blackHole.r = 1.35f;
        blackHole.strength = 13.0f;
    } else if (levelNo == 5) {
        makeBrickGrid(6, 10, 2.6f, 3, true, false);
        blackHole.active = true;
        whiteHole.active = true;
        portals[0].active = portals[1].active = true;
    } else if (levelNo == 6) {
        makeBrickGrid(6, 11, 2.8f, 3, true, false);
        blackHole.active = true;
        whiteHole.active = true;
        portals[0].active = portals[1].active = true;
        mirrors[0] = {Vec3(-3.7f, 0.8f, 0), 3.3f, 25.0f, 48.0f, true};
        mirrors[1] = {Vec3(3.7f, 0.3f, 0), 3.1f, -30.0f, -42.0f, true};
    } else if (levelNo == 7) {
        // RGB Chaos keeps every special mechanic, but the arena is divided
        // into readable zones so the presentation does not look cluttered.
        makeBrickGrid(5, 9, 3.55f, 4, true, true);
        blackHole.active = true;
        blackHole.p = Vec3(3.0f, 0.2f, 0);
        blackHole.r = 0.9f;
        blackHole.strength = 7.2f;
        whiteHole.active = true;
        whiteHole.p = Vec3(6.4f, -2.25f, 0);
        whiteHole.r = 0.82f;
        whiteHole.strength = 5.6f;
        portals[0].active = portals[1].active = true;
        portals[0].p = Vec3(-6.9f, 1.45f, 0);
        portals[1].p = Vec3(6.9f, 2.45f, 0);
        tunnel.active = true;
        tunnel.p0 = Vec3(-7.1f, -2.45f, 0);
        tunnel.p1 = Vec3(-6.4f, 0.2f, 0);
        tunnel.p2 = Vec3(-4.2f, 1.0f, 0);
        tunnel.p3 = Vec3(-2.6f, -0.65f, 0);
        tunnel.radius = 0.48f;
        mirrors[0] = {Vec3(-1.2f, 1.05f, 0), 2.45f, 18.0f, 44.0f, true};
        mirrors[1] = {Vec3(3.9f, 2.05f, 0), 2.35f, -32.0f, -38.0f, true};
    } else {
        lives=5;
        paddle.speed = 15.0f;
        boss.active = true;
        boss.hp = boss.maxHp = 8;
        boss.shockwaveRadius = -1.0f;
        boss.shockwaveTimer = 2.0f;
        blackHole.active = true;
        blackHole.p = boss.p;
        blackHole.r = 1.62f;
        blackHole.strength = 13.2f;
        whiteHole.active = true;
        whiteHole.p = Vec3(-6.2f, -2.2f, 0);
        whiteHole.r = 0.78f;
        whiteHole.strength = 5.4f;
        portals[0].active = portals[1].active = true;
        portals[0].p = Vec3(-7.0f, -0.8f, 0);
        portals[1].p = Vec3(7.0f, 2.0f, 0);
        mirrors[0] = {Vec3(-4.6f, 0.2f, 0), 3.0f, 35.0f, 48.0f, true};
        mirrors[1] = {Vec3(4.6f, 0.2f, 0), 3.0f, -35.0f, -48.0f, true};
        mirrors[2] = {Vec3(0.0f, -1.8f, 0), 2.8f, 0.0f, 58.0f, true};
        for (int i = 0; i < 16; ++i) {
            float a = i * 2.0f * PI / 16.0f;
            addBrick(boss.p.x + std::cos(a) * 3.2f, boss.p.y + std::sin(a) * 1.75f,
                     SHIELD, (i % 4 == 0) ? 3 : 2, false);
        }
    }

    resetBall(true);
    gameState = PLAYING;
}

void initGL() {
    glClearColor(0.015f, 0.015f, 0.04f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    GLfloat l0[] = {0.2f, 0.8f, 1.0f, 1.0f};
    GLfloat l1[] = {1.0f, 0.2f, 0.7f, 1.0f};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, l0);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, l1);
}

// ==================================================
// 7. input
// ==================================================

void launchBalls() {
    for (size_t i = 0; i < balls.size(); ++i) {
        if (!balls[i].launched) {
            balls[i].launched = true;
            balls[i].v = normalize(Vec3(randf(-1.0f, 1.0f), 2.2f, 0)) * (5.0f + levelNo * 0.35f);
        }
    }
}

void restartGame() {
    score = 0;
    lives = 3;
    specialCharges = 0;
    setupLevel(1);
}

void activateSpecial() {
    if (specialCharges <= 0 || gameState != PLAYING) return;
    specialCharges--;
    playSoundEvent("power");
    Vec3 center = balls.empty() ? paddle.p : balls[0].p;
    for (size_t i = 0; i < bricks.size(); ++i) {
        if (bricks[i].alive && len(bricks[i].p - center) < 2.8f) {
            bricks[i].hp = 0;
            bricks[i].alive = false;
            score += 60;
        }
    }
    if (boss.active && len(boss.p - center) < 3.8f) {
        boss.hp = std::max(0, boss.hp - 1);
        playSoundEvent("boss");
    }
}

void adminJumpToLevel(int targetLevel) {
    targetLevel = std::max(1, std::min(8, targetLevel));
    if (gameState == MENU || gameState == GAME_OVER || gameState == GAME_WON) {
        lives = 3;
    } else if (lives <= 0) {
        lives = 3;
    }
    setupLevel(targetLevel);
    showHelp = true;
}

void keyboard(unsigned char key, int, int) {
    if (exitPrompt) {
        if (key == 27) std::exit(0);
        if (key == ' ' || key == '\b' || std::tolower(key) == 'x') {
            exitPrompt = false;
            gameState = stateBeforeExitPrompt;
        }
        return;
    }

    if (key == 27) {
        stateBeforeExitPrompt = gameState;
        if (gameState == PLAYING) gameState = PAUSED;
        exitPrompt = true;
        return;
    }

    if (key >= '1' && key <= '8') {
        adminJumpToLevel(key - '0');
        return;
    }

    switch (std::tolower(key)) {
        case 'a': keyLeft = true; break;
        case 'd': keyRight = true; break;
        case 'b': adminJumpToLevel(levelNo - 1); break;
        case 'n': adminJumpToLevel(levelNo + 1); break;
        case ' ': if (gameState == MENU) restartGame(); else if (gameState == PLAYING) launchBalls(); break;
        case 'e': activateSpecial(); break;
        case 'c': cameraMode = (cameraMode + 1) % 3; break;
        case 'p': gameState = (gameState == PAUSED) ? PLAYING : (gameState == PLAYING ? PAUSED : gameState); break;
        case 'r': if (gameState == GAME_OVER || gameState == GAME_WON || gameState == MENU) restartGame(); else setupLevel(levelNo); break;
        case 'm': muted = !muted; break;
        case 'h': showHelp = !showHelp; break;
        case 'g': showGraphicsOverlay = !showGraphicsOverlay; break;
        case 'z':
            depthEnabled = !depthEnabled;
            if (depthEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
            break;
    }
}

void keyboardUp(unsigned char key, int, int) {
    if (std::tolower(key) == 'a') keyLeft = false;
    if (std::tolower(key) == 'd') keyRight = false;
}

void specialKey(int key, int, int) {
    if (key == GLUT_KEY_LEFT) keyLeft = true;
    if (key == GLUT_KEY_RIGHT) keyRight = true;
}

void specialKeyUp(int key, int, int) {
    if (key == GLUT_KEY_LEFT) keyLeft = false;
    if (key == GLUT_KEY_RIGHT) keyRight = false;
}

// ==================================================
// 8. update logic
// ==================================================

void spawnParticles(Vec3 p, float r, float g, float b, int count = 16) {
    for (int i = 0; i < count; ++i) {
        Particle q;
        q.p = p;
        q.v = Vec3(randf(-2.2f, 2.2f), randf(-1.2f, 2.8f), randf(-1.0f, 1.0f));
        q.life = randf(0.35f, 0.85f);
        q.r = r; q.g = g; q.b = b;
        particles.push_back(q);
    }
}

void spawnPowerUp(Vec3 p) {
    if (randf(0, 1) > 0.18f) return;
    PowerUp u;
    u.p = p;
    int pick = rand() % 3;
    u.type = pick == 0 ? POWER_WIDE : (pick == 1 ? POWER_SLOW : POWER_SPECIAL);
    u.active = true;
    u.spin = 0;
    powerUps.push_back(u);
}

void applyBrickEffect(BrickType type, Vec3 p);

void splitBall(const Ball& src) {
    if (balls.size() >= 4) return;
    Ball a = src, b = src;
    a.v = normalize(Vec3(src.v.x * 0.5f - 1.2f, std::fabs(src.v.y) + 0.9f, 0)) * len(src.v);
    b.v = normalize(Vec3(src.v.x * 0.5f + 1.2f, std::fabs(src.v.y) + 0.9f, 0)) * len(src.v);
    a.launched = b.launched = true;
    balls.push_back(a);
    balls.push_back(b);
}

void keepBallSpeed(Ball& b) {
    float minS = 4.2f + levelNo * 0.18f;
    float maxS = 8.7f + levelNo * 0.28f;
    float s = clampf(len(b.v), minS, maxS);
    b.v = normalize(b.v) * s;
}

void updateFeatures(float dt) {
    paddle.w = paddle.wideTimer > 0 ? 3.6f : 2.45f;
    paddle.wideTimer = std::max(0.0f, paddle.wideTimer - dt);

    for (size_t i = 0; i < bricks.size(); ++i) {
        Brick& b = bricks[i];
        b.flash = std::max(0.0f, b.flash - dt * 4.0f);
        if (b.moving) b.p.x = b.baseX + std::sin(globalTime * 1.4f + b.movePhase) * 0.85f;
        if (b.type == SHIELD && boss.active) {
            float idx = float(i);
            float a = boss.shieldAngle + idx * 2.0f * PI / std::max(1.0f, float(bricks.size()));
            b.p.x = boss.p.x + std::cos(a) * 3.2f;
            b.p.y = boss.p.y + std::sin(a) * 1.75f;
        }
    }
    boss.shieldAngle += dt * (boss.active && boss.hp < boss.maxHp / 2 ? 1.28f : 0.82f);
    boss.pulse = 0.5f + 0.5f * std::sin(globalTime * 3.2f);
    if (boss.active) {
        boss.shockwaveTimer -= dt;
        if (boss.shockwaveRadius < 0.0f && boss.shockwaveTimer <= 0.0f) {
            boss.shockwaveRadius = 0.95f;
            boss.shockwaveTimer = boss.hp < boss.maxHp / 2 ? 2.7f : 3.7f;
            spawnParticles(boss.p, 0.65f, 0.08f, 1.0f, 18);
            playSoundEvent("boss");
        }
        if (boss.shockwaveRadius >= 0.0f) {
            boss.shockwaveRadius += dt * (boss.hp < boss.maxHp / 2 ? 3.9f : 3.1f);
            if (boss.shockwaveRadius > 7.5f) boss.shockwaveRadius = -1.0f;
        }
        if (levelNo == 8) {
            blackHole.strength = 12.8f + boss.pulse * 2.8f + (boss.hp < boss.maxHp / 2 ? 1.8f : 0.0f);
            blackHole.r = 1.55f + boss.pulse * 0.18f;
        }
    }
    for (int i = 0; i < 3; ++i) if (mirrors[i].active) mirrors[i].angle += mirrors[i].speed * dt;

    for (size_t i = 0; i < powerUps.size(); ++i) {
        if (!powerUps[i].active) continue;
        powerUps[i].p.y -= dt * 2.3f;
        powerUps[i].spin += dt * 180.0f;
        if (std::fabs(powerUps[i].p.x - paddle.p.x) < paddle.w * 0.5f &&
            std::fabs(powerUps[i].p.y - paddle.p.y) < 0.65f) {
            if (powerUps[i].type == POWER_WIDE) paddle.wideTimer = 9.0f;
            if (powerUps[i].type == POWER_SLOW) for (size_t j = 0; j < balls.size(); ++j) balls[j].v = balls[j].v * 0.78f;
            if (powerUps[i].type == POWER_SPECIAL) specialCharges++;
            powerUps[i].active = false;
            score += 75;
            playSoundEvent("power");
        }
        if (powerUps[i].p.y < ARENA_BOTTOM - 1.0f) powerUps[i].active = false;
    }

    for (size_t i = 0; i < particles.size(); ++i) {
        particles[i].life -= dt;
        particles[i].p = particles[i].p + particles[i].v * dt;
        particles[i].v.y -= dt * 2.4f;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& p) { return p.life <= 0.0f; }), particles.end());
}

// ==================================================
// 9. collision logic
// ==================================================

bool circleRectCollision(Ball& ball, Brick& brick, Vec3& normalOut) {
    float closestX = clampf(ball.p.x, brick.p.x - brick.w * 0.5f, brick.p.x + brick.w * 0.5f);
    float closestY = clampf(ball.p.y, brick.p.y - brick.h * 0.5f, brick.p.y + brick.h * 0.5f);
    Vec3 diff = ball.p - Vec3(closestX, closestY, 0);
    if (len(diff) <= ball.r) {
        float dx = std::min(std::fabs(ball.p.x - (brick.p.x - brick.w * 0.5f)),
                            std::fabs(ball.p.x - (brick.p.x + brick.w * 0.5f)));
        float dy = std::min(std::fabs(ball.p.y - (brick.p.y - brick.h * 0.5f)),
                            std::fabs(ball.p.y - (brick.p.y + brick.h * 0.5f)));
        if (dx < dy) normalOut = Vec3(ball.p.x < brick.p.x ? -1 : 1, 0, 0);
        else normalOut = Vec3(0, ball.p.y < brick.p.y ? -1 : 1, 0);
        return true;
    }
    return false;
}

void damageBrick(Brick& brick, Ball& ball) {
    brick.hp--;
    brick.flash = 1.0f;
    score += 10;
    if (brick.hp <= 0) {
        brick.alive = false;
        score += brick.type == GREEN_SCORE ? 180 : 70;
        float cr = 0.1f, cg = 0.9f, cb = 1.0f;
        if (brick.type == RED_SPEED) { cr = 1; cg = 0.1f; cb = 0.1f; }
        if (brick.type == BLUE_SLOW) { cr = 0.1f; cg = 0.35f; cb = 1; }
        if (brick.type == GREEN_SCORE) { cr = 0.1f; cg = 1; cb = 0.25f; }
        if (brick.type == YELLOW_SPLIT) { cr = 1; cg = 0.9f; cb = 0.1f; }
        if (brick.type == BLACK_GRAVITY || brick.type == SHIELD) { cr = 0.6f; cg = 0.2f; cb = 1; }
        spawnParticles(brick.p, cr, cg, cb, 20);
        spawnPowerUp(brick.p);
        applyBrickEffect(brick.type, brick.p);
        if (brick.type == YELLOW_SPLIT) splitBall(ball);
        playSoundEvent("break");
    } else {
        playSoundEvent("wall");
    }
}

void applyBrickEffect(BrickType type, Vec3 p) {
    if (type == RED_SPEED) {
        for (size_t i = 0; i < balls.size(); ++i) balls[i].v = balls[i].v * 1.18f;
    } else if (type == BLUE_SLOW) {
        for (size_t i = 0; i < balls.size(); ++i) balls[i].v = balls[i].v * 0.82f;
    } else if (type == BLACK_GRAVITY) {
        blackHole.active = true;
        blackHole.p = p;
        blackHole.r = 0.9f;
        blackHole.strength = 7.5f;
    }
}

void collidePaddle(Ball& b) {
    if (b.v.y < 0 &&
        b.p.y - b.r < paddle.p.y + paddle.h * 0.5f &&
        b.p.y + b.r > paddle.p.y - paddle.h * 0.5f &&
        b.p.x > paddle.p.x - paddle.w * 0.5f &&
        b.p.x < paddle.p.x + paddle.w * 0.5f) {
        float hit = (b.p.x - paddle.p.x) / (paddle.w * 0.5f);
        float speed = len(b.v);
        b.v = normalize(Vec3(hit * 2.2f, 2.6f, 0)) * speed;
        b.p.y = paddle.p.y + paddle.h * 0.5f + b.r + 0.02f;
        playSoundEvent("paddle");
    }
}

void collideMirror(Ball& b, const Mirror& m) {
    float a = m.angle * PI / 180.0f;
    Vec3 dir(std::cos(a), std::sin(a), 0);
    Vec3 rel = b.p - m.p;
    float along = clampf(dot(rel, dir), -m.length * 0.5f, m.length * 0.5f);
    Vec3 closest = m.p + dir * along;
    Vec3 diff = b.p - closest;
    if (len(diff) < b.r + 0.08f) {
        Vec3 n = normalize(diff);
        if (len(diff) < 0.001f) n = Vec3(-dir.y, dir.x, 0);
        b.v = reflect(b.v, n);
        b.p = closest + n * (b.r + 0.12f);
        playSoundEvent("wall");
    }
}

void collidePortals(Ball& b) {
    if (!portals[0].active || !portals[1].active || b.portalCooldown > 0) return;
    for (int i = 0; i < 2; ++i) {
        int other = 1 - i;
        if (len(b.p - portals[i].p) < portals[i].r) {
            b.p = portals[other].p + normalize(b.v) * (portals[other].r + 0.38f);
            b.portalCooldown = 0.8f;
            spawnParticles(portals[other].p, 0.7f, 0.2f, 1.0f, 24);
            playSoundEvent("portal");
            return;
        }
    }
}

void influenceHole(Ball& b, const BlackHole& h, float dt) {
    if (!h.active || b.inTunnel) return;
    Vec3 d = h.p - b.p;
    float dist = std::max(0.7f, len(d));
    float force = h.strength / (dist * dist);
    b.v = b.v + normalize(d) * force * dt;
    if (dist < h.r * 0.55f && levelNo == 8 && boss.active) {
        b.v = reflect(b.v, normalize(b.p - h.p)) * 0.92f;
    }
}

void influenceWhiteHole(Ball& b, const WhiteHole& h, float dt) {
    if (!h.active || b.inTunnel) return;
    Vec3 d = b.p - h.p;
    float dist = std::max(0.65f, len(d));
    if (dist < h.r * 3.0f) b.v = b.v + normalize(d) * (h.strength / (dist * dist)) * dt;
}

void influenceTunnelMouth(Ball& b, Vec3 mouth, float dt) {
    if (!tunnel.active || b.inTunnel || !b.launched) return;
    Vec3 d = mouth - b.p;
    float dist = std::max(0.35f, len(d));
    if (dist < tunnel.radius * 3.6f) {
        b.v = b.v + normalize(d) * (0.65f / (dist * dist)) * dt;
    }
}

void updateTunnel(Ball& b, float dt) {
    if (!tunnel.active) return;
    if (!b.inTunnel && b.tunnelCooldown <= 0.0f &&
        len(b.p - tunnel.p0) < tunnel.radius + b.r * 0.9f && b.launched) {
        b.inTunnel = true;
        b.tunnelForward = true;
        b.tunnelT = 0.0f;
        playSoundEvent("portal");
    } else if (!b.inTunnel && b.tunnelCooldown <= 0.0f &&
               len(b.p - tunnel.p3) < tunnel.radius + b.r * 0.9f && b.launched) {
        b.inTunnel = true;
        b.tunnelForward = false;
        b.tunnelT = 1.0f;
        playSoundEvent("portal");
    }

    if (b.inTunnel) {
        float travel = dt * (0.42f + 0.03f * levelNo);
        b.tunnelT += b.tunnelForward ? travel : -travel;
        b.p = bezierPoint(tunnel, clampf(b.tunnelT, 0.0f, 1.0f));
        if (b.tunnelForward && b.tunnelT >= 1.0f) {
            b.inTunnel = false;
            b.tunnelCooldown = 0.42f;
            b.v = bezierTangent(tunnel, 0.96f) * (5.4f + levelNo * 0.35f);
            b.p = tunnel.p3;
        } else if (!b.tunnelForward && b.tunnelT <= 0.0f) {
            b.inTunnel = false;
            b.tunnelCooldown = 0.42f;
            b.v = bezierTangent(tunnel, 0.04f) * -(5.4f + levelNo * 0.35f);
            b.p = tunnel.p0;
        }
    }
}

void updateBall(Ball& b, float dt) {
    b.portalCooldown = std::max(0.0f, b.portalCooldown - dt);
    b.tunnelCooldown = std::max(0.0f, b.tunnelCooldown - dt);
    b.bossWaveCooldown = std::max(0.0f, b.bossWaveCooldown - dt);

    if (!b.launched) {
        b.p.x = paddle.p.x;
        b.p.y = paddle.p.y + 0.65f;
        return;
    }

    updateTunnel(b, dt);
    if (b.inTunnel) return;

    influenceHole(b, blackHole, dt);
    influenceWhiteHole(b, whiteHole, dt);
    influenceTunnelMouth(b, tunnel.p0, dt);
    influenceTunnelMouth(b, tunnel.p3, dt);
    keepBallSpeed(b);
    b.p = b.p + b.v * dt;

    if (b.p.x - b.r < ARENA_LEFT) { b.p.x = ARENA_LEFT + b.r; b.v.x = std::fabs(b.v.x); playSoundEvent("wall"); }
    if (b.p.x + b.r > ARENA_RIGHT) { b.p.x = ARENA_RIGHT - b.r; b.v.x = -std::fabs(b.v.x); playSoundEvent("wall"); }
    if (b.p.y + b.r > ARENA_TOP) { b.p.y = ARENA_TOP - b.r; b.v.y = -std::fabs(b.v.y); playSoundEvent("wall"); }
    if (b.p.y < ARENA_BOTTOM - 1.1f) b.alive = false;

    collidePaddle(b);
    collidePortals(b);
    for (int i = 0; i < 3; ++i) if (mirrors[i].active) collideMirror(b, mirrors[i]);

    for (size_t i = 0; i < bricks.size(); ++i) {
        Brick& br = bricks[i];
        if (!br.alive) continue;
        Vec3 normal;
        if (circleRectCollision(b, br, normal)) {
            b.v = reflect(b.v, normal);
            damageBrick(br, b);
            break;
        }
    }

    if (boss.active) {
        if (boss.shockwaveRadius > 0.0f && b.bossWaveCooldown <= 0.0f) {
            float dist = len(b.p - boss.p);
            if (std::fabs(dist - boss.shockwaveRadius) < b.r + 0.14f) {
                Vec3 n = normalize(b.p - boss.p);
                b.v = normalize(reflect(b.v, n) + n * 1.25f) * (len(b.v) * 1.08f);
                b.bossWaveCooldown = 0.7f;
                spawnParticles(b.p, 0.8f, 0.05f, 1.0f, 10);
                playSoundEvent("boss");
            }
        }
        // bool shieldAlive = false;
        // for (size_t i = 0; i < bricks.size(); ++i) if (bricks[i].alive && bricks[i].type == SHIELD) shieldAlive = true;
        if (len(b.p - boss.p) < 1.28f + b.r) {
            boss.hp--;
            score += 250;
            b.v = reflect(b.v, normalize(b.p - boss.p));
            spawnParticles(boss.p, 0.4f, 0.05f, 0.9f, 36);
            playSoundEvent("boss");
            if (boss.hp <= 0) {
                gameState = GAME_WON;
                playSoundEvent("level");
            }
        }
    }
}

void updateGame(float dt) {
    if (gameState != PLAYING) return;

    if (keyLeft) paddle.p.x -= paddle.speed * dt;
    if (keyRight) paddle.p.x += paddle.speed * dt;
    paddle.p.x = clampf(paddle.p.x, ARENA_LEFT + paddle.w * 0.5f, ARENA_RIGHT - paddle.w * 0.5f);

    updateFeatures(dt);
    for (size_t i = 0; i < balls.size(); ++i) updateBall(balls[i], dt);
    balls.erase(std::remove_if(balls.begin(), balls.end(), [](const Ball& b) { return !b.alive; }), balls.end());

    if (!balls.empty() && balls[0].launched) {
        ballTrail.push_back(balls[0].p);
        if (ballTrail.size() > 40) ballTrail.erase(ballTrail.begin());
    }

    if (balls.empty()) {
        lives--;
        if (lives <= 0) {
            gameState = GAME_OVER;
            playSoundEvent("gameover");
        } else {
            resetBall(true);
        }
    }

    if (levelNo < 8) {
        bool any = false;
        for (size_t i = 0; i < bricks.size(); ++i) if (bricks[i].alive) any = true;
        if (!any) {
            gameState = LEVEL_COMPLETE;
            score += levelNo * 300;
            playSoundEvent("level");
        }
    }
}

// ==================================================
// 10. drawing
// ==================================================

void drawCubeAt(Vec3 p, float sx, float sy, float sz) {
    glPushMatrix();
    glTranslatef(p.x, p.y, p.z);
    glScalef(sx, sy, sz);
    glutSolidCube(1.0);
    glPopMatrix();
}

void drawArena() {
    glDisable(GL_LIGHTING);
    glColor4f(0.05f, 0.08f, 0.12f, 1);
    glBegin(GL_QUADS);
    glVertex3f(ARENA_LEFT, ARENA_BOTTOM, -0.38f);
    glVertex3f(ARENA_RIGHT, ARENA_BOTTOM, -0.38f);
    glVertex3f(ARENA_RIGHT, ARENA_TOP, -0.38f);
    glVertex3f(ARENA_LEFT, ARENA_TOP, -0.38f);
    glEnd();

    glColor4f(0.0f, 0.75f, 1.0f, 0.22f);

    glBegin(GL_LINES);
    for (float x = ARENA_LEFT; x <= ARENA_RIGHT + 0.1f; x += 0.75f) {
        glVertex3f(x, ARENA_BOTTOM, -0.36f);
        glVertex3f(x, ARENA_TOP, -0.36f);
    }
    for (float y = ARENA_BOTTOM; y <= ARENA_TOP + 0.1f; y += 0.75f) {
        glVertex3f(ARENA_LEFT, y, -0.36f);
        glVertex3f(ARENA_RIGHT, y, -0.36f);
    }
    glEnd();

    glLineWidth(4.0f);
    glColor4f(0.0f, 0.9f, 1.0f, 0.85f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(ARENA_LEFT, ARENA_BOTTOM, -0.2f);
    glVertex3f(ARENA_RIGHT, ARENA_BOTTOM, -0.2f);
    glVertex3f(ARENA_RIGHT, ARENA_TOP, -0.2f);
    glVertex3f(ARENA_LEFT, ARENA_TOP, -0.2f);
    glEnd();
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}

void brickColor(const Brick& b, float& r, float& g, float& bl) {
    r = 0.0f; g = 0.75f; bl = 1.0f;
    if (b.type == STRONG) { r = 0.75f; g = 0.22f; bl = 1.0f; }
    if (b.type == RED_SPEED) { r = 1.0f; g = 0.08f; bl = 0.06f; }
    if (b.type == BLUE_SLOW) { r = 0.05f; g = 0.25f; bl = 1.0f; }
    if (b.type == GREEN_SCORE) { r = 0.05f; g = 1.0f; bl = 0.25f; }
    if (b.type == YELLOW_SPLIT) { r = 1.0f; g = 0.86f; bl = 0.05f; }
    if (b.type == BLACK_GRAVITY) { r = 0.18f; g = 0.06f; bl = 0.28f; }
    if (b.type == SHIELD) { r = 0.55f; g = 0.22f; bl = 1.0f; }
    if (b.flash > 0) { r = 1; g = 1; bl = 1; }
}

void drawBricks() {
    for (size_t i = 0; i < bricks.size(); ++i) {
        const Brick& b = bricks[i];
        if (!b.alive) continue;
        float r, g, bl;
        brickColor(b, r, g, bl);
        setColor(r, g, bl, 1, 0.22f);
        glPushMatrix();
        glTranslatef(b.p.x, b.p.y, b.p.z);
        glScalef(b.w, b.h, b.d);
        glutSolidCube(1.0);
        glPopMatrix();

        glDisable(GL_LIGHTING);
        glColor4f(r, g, bl, 0.35f);
        glLineWidth(2);
        glBegin(GL_LINE_LOOP);
        glVertex3f(b.p.x - b.w * 0.55f, b.p.y - b.h * 0.62f, 0.28f);
        glVertex3f(b.p.x + b.w * 0.55f, b.p.y - b.h * 0.62f, 0.28f);
        glVertex3f(b.p.x + b.w * 0.55f, b.p.y + b.h * 0.62f, 0.28f);
        glVertex3f(b.p.x - b.w * 0.55f, b.p.y + b.h * 0.62f, 0.28f);
        glEnd();
        glLineWidth(1);
        glEnable(GL_LIGHTING);
    }
}

void drawPaddle() {
    setColor(0.0f, 0.95f, 1.0f, 1, 0.35f);
    drawCubeAt(paddle.p, paddle.w, paddle.h, 0.55f);
    setColor(1.0f, 0.1f, 0.7f, 1, 0.3f);
    drawCubeAt(Vec3(paddle.p.x, paddle.p.y + 0.03f, 0.34f), paddle.w * 0.7f, 0.08f, 0.12f);
}

void drawBalls() {
    for (size_t i = 0; i < ballTrail.size(); ++i) {
        float a = float(i) / float(std::max<size_t>(1, ballTrail.size()));
        setColor(0.0f, 0.9f, 1.0f, a * 0.35f, 0.4f);
        glPushMatrix();
        glTranslatef(ballTrail[i].x, ballTrail[i].y, ballTrail[i].z);
        glutSolidSphere(0.09f + 0.11f * a, 14, 8);
        glPopMatrix();
    }
    for (size_t i = 0; i < balls.size(); ++i) {
        setColor(0.95f, 1.0f, 0.35f, 1, 0.75f);
        glPushMatrix();
        glTranslatef(balls[i].p.x, balls[i].p.y, balls[i].p.z);
        glutSolidSphere(balls[i].r, 28, 18);
        glPopMatrix();
    }
}

void drawBezierTunnel() {
    if (!tunnel.active) return;
    glDisable(GL_LIGHTING);
    glLineWidth(5.0f);
    glColor4f(0.0f, 1.0f, 0.75f, 0.75f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 80; ++i) {
        Vec3 p = bezierPoint(tunnel, i / 80.0f);
        glVertex3f(p.x, p.y, 0.18f);
    }
    glEnd();
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
    setColor(0.0f, 1.0f, 0.75f, 1, 0.55f);
    glPushMatrix();
    glTranslatef(tunnel.p0.x, tunnel.p0.y, 0.05f);
    glutSolidTorus(0.035, tunnel.radius, 20, 36);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(tunnel.p3.x, tunnel.p3.y, 0.05f);
    glutSolidTorus(0.035, tunnel.radius, 20, 36);
    glPopMatrix();
}

void drawHole(const BlackHole& h) {
    if (!h.active) return;
    setColor(0.03f, 0.0f, 0.05f, 1, 0.1f);
    glPushMatrix();
    glTranslatef(h.p.x, h.p.y, 0);
    glutSolidSphere(h.r * 0.45f, 34, 22);
    glRotatef(globalTime * 80.0f, 0, 0, 1);
    setColor(0.45f, 0.05f, 1.0f, 1, 0.55f);
    glutSolidTorus(0.035f, h.r * 0.72f, 14, 80);
    glRotatef(72, 1, 0, 0);
    setColor(0.0f, 0.65f, 1.0f, 1, 0.45f);
    glutSolidTorus(0.025f, h.r * 0.95f, 12, 80);
    glPopMatrix();
}

void drawWhiteHole(const WhiteHole& h) {
    if (!h.active) return;
    float pulse = 0.2f * std::sin(globalTime * 5.0f);
    setColor(0.8f, 1.0f, 1.0f, 1, 0.65f);
    glPushMatrix();
    glTranslatef(h.p.x, h.p.y, 0);
    glutSolidSphere(h.r * 0.32f, 28, 18);
    setColor(0.4f, 0.9f, 1.0f, 1, 0.45f);
    glutSolidTorus(0.025f, h.r * (0.75f + pulse), 12, 64);
    glutSolidTorus(0.018f, h.r * (1.18f - pulse), 12, 64);
    glPopMatrix();
}

void drawPortals() {
    for (int i = 0; i < 2; ++i) {
        if (!portals[i].active) continue;
        setColor(i == 0 ? 1.0f : 0.1f, 0.25f, i == 0 ? 0.2f : 1.0f, 1, 0.6f);
        glPushMatrix();
        glTranslatef(portals[i].p.x, portals[i].p.y, 0.08f);
        glRotatef(globalTime * 120.0f * (i ? -1 : 1), 0, 0, 1);
        glutSolidTorus(0.045f, portals[i].r, 14, 58);
        glRotatef(90, 1, 0, 0);
        glutSolidTorus(0.018f, portals[i].r * 0.65f, 10, 42);
        glPopMatrix();
    }
}

void drawMirrors() {
    for (int i = 0; i < 3; ++i) {
        if (!mirrors[i].active) continue;
        setColor(0.85f, 1.0f, 1.0f, 1, 0.25f);
        glPushMatrix();
        glTranslatef(mirrors[i].p.x, mirrors[i].p.y, 0.12f);
        glRotatef(mirrors[i].angle, 0, 0, 1);
        glScalef(mirrors[i].length, 0.12f, 0.22f);
        glutSolidCube(1.0);
        glPopMatrix();
    }
}

void drawPowerUps() {
    for (size_t i = 0; i < powerUps.size(); ++i) {
        if (!powerUps[i].active) continue;
        float r = powerUps[i].type == POWER_WIDE ? 0.0f : (powerUps[i].type == POWER_SPECIAL ? 1.0f : 0.2f);
        float g = powerUps[i].type == POWER_SLOW ? 0.6f : 1.0f;
        float b = powerUps[i].type == POWER_WIDE ? 1.0f : 0.3f;
        setColor(r, g, b, 1, 0.5f);
        glPushMatrix();
        glTranslatef(powerUps[i].p.x, powerUps[i].p.y, 0.25f);
        glRotatef(powerUps[i].spin, 0, 1, 1);
        glutSolidOctahedron();
        glPopMatrix();
    }
}

void drawParticles() {
    for (size_t i = 0; i < particles.size(); ++i) {
        setColor(particles[i].r, particles[i].g, particles[i].b, particles[i].life, 0.5f);
        glPushMatrix();
        glTranslatef(particles[i].p.x, particles[i].p.y, particles[i].p.z + 0.25f);
        glutSolidSphere(0.05f, 8, 6);
        glPopMatrix();
    }
}

void drawBoss() {
    if (!boss.active) return;
    if (boss.shockwaveRadius > 0.0f) {
        float fade = clampf(1.0f - boss.shockwaveRadius / 7.5f, 0.0f, 1.0f);
        setColor(0.75f, 0.05f, 1.0f, fade, 0.65f);
        glPushMatrix();
        glTranslatef(boss.p.x, boss.p.y, 0.12f);
        glutSolidTorus(0.035f, boss.shockwaveRadius, 10, 96);
        glPopMatrix();
    }
    setColor(0.15f + boss.pulse * 0.2f, 0.0f, 0.25f + boss.pulse * 0.5f, 1, 0.45f);
    glPushMatrix();
    glTranslatef(boss.p.x, boss.p.y, 0.04f);
    glutSolidSphere(0.9f + boss.pulse * 0.1f, 42, 28);
    glRotatef(globalTime * 95.0f, 0, 0, 1);
    setColor(0.6f, 0.05f, 1.0f, 1, 0.5f);
    glutSolidTorus(0.04f, 1.25f, 16, 90);
    glPopMatrix();
}

void drawScene3D() {
    GLfloat pos0[] = {0.0f, -4.0f, 8.0f, 1.0f};
    GLfloat pos1[] = {0.0f, 6.0f, 5.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, pos0);
    glLightfv(GL_LIGHT1, GL_POSITION, pos1);

    drawArena();
    drawBezierTunnel();
    drawHole(blackHole);
    drawWhiteHole(whiteHole);
    drawPortals();
    drawMirrors();
    drawBoss();
    drawBricks();
    drawPowerUps();
    drawPaddle();
    drawBalls();
    drawParticles();
}

// ==================================================
// 11. level setup
// ==================================================
// Level creation is implemented by setupLevel() above so the data is close to
// the struct initialization helpers.

// ==================================================
// 12. HUD and text
// ==================================================

void begin2D() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_W, 0, WINDOW_H);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
}

void end2D() {
    if (depthEnabled) glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawText(float x, float y, const std::string& s, void* font = GLUT_BITMAP_9_BY_15) {
    glRasterPos2f(x, y);
    for (size_t i = 0; i < s.size(); ++i) glutBitmapCharacter(font, s[i]);
}

void drawHUD() {
    begin2D();
    glColor3f(0.8f, 0.95f, 1.0f);
    std::ostringstream top;
    top << "Bezier Breakout: Curve Ball Arena    Score " << score
        << "    Lives " << lives << "    Level " << levelNo
        << "    Power " << currentPowerText()
        << "    Special(E) " << specialCharges
        << "    Camera " << (cameraMode + 1)
        << "    Sound " << (muted ? "Muted" : "On");
    drawText(18, WINDOW_H - 26, top.str());

    if (boss.active) {
        glColor3f(1.0f, 0.3f, 0.85f);
        std::ostringstream hp;
        int hpPercent = boss.maxHp > 0 ? int((boss.hp * 100.0f / boss.maxHp) + 0.5f) : 0;
        hp << "BOSS HP " << hpPercent << "% ";
        for (int i = 0; i < boss.hp; ++i) hp << "|";
        drawText(18, WINDOW_H - 50, hp.str());
    }

    if (gameState == MENU) {
        glColor3f(0.0f, 1.0f, 0.85f);
        drawText(390, 420, "BEZIER BREAKOUT: CURVE BALL ARENA", GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(1.0f, 1.0f, 0.55f);
        drawText(450, 380, "Press SPACE to start");
    } else if (gameState == PAUSED) {
        glColor3f(1.0f, 1.0f, 0.4f);
        drawText(535, 410, "PAUSED", GLUT_BITMAP_TIMES_ROMAN_24);
    } else if (gameState == LEVEL_COMPLETE) {
        glColor3f(0.3f, 1.0f, 0.6f);
        drawText(485, 420, "LEVEL COMPLETE", GLUT_BITMAP_TIMES_ROMAN_24);
        drawText(435, 382, "Press SPACE for next level or R to replay");
    } else if (gameState == GAME_OVER) {
        glColor3f(1.0f, 0.25f, 0.25f);
        drawText(515, 420, "GAME OVER", GLUT_BITMAP_TIMES_ROMAN_24);
        drawText(465, 382, "Press R to restart");
    } else if (gameState == GAME_WON) {
        glColor3f(0.4f, 1.0f, 0.8f);
        drawText(462, 420, "YOU DEFEATED THE EVENT HORIZON", GLUT_BITMAP_TIMES_ROMAN_24);
        drawText(468, 382, "Press R to play again");
    }

    if (showHelp) {
        glColor3f(0.72f, 0.88f, 1.0f);
        drawText(18, 112, "A/Left D/Right move | Space launch/continue | E special | C camera | P pause | R restart | M mute");
        drawText(18, 90, "Admin demo: 1-8 jump levels | N next | B previous | H help | G graphics overlay | Z depth test | ESC exit");
        glColor3f(1.0f, 0.86f, 0.45f);
        drawText(18, 66, "RGB blocks: red speed, blue slow, green score, yellow multi-ball, black gravity distortion");
    }

    if (showGraphicsOverlay) {
        glColor3f(0.3f, 1.0f, 0.75f);
        drawText(WINDOW_W - 360, 142, "Graphics Overlay");
        drawText(WINDOW_W - 360, 120, "Transformations: translate / rotate / scale");
        drawText(WINDOW_W - 360, 98, "Curves: cubic Bezier tunnel");
        drawText(WINDOW_W - 360, 76, std::string("Z-buffer: ") + (depthEnabled ? "GL_DEPTH_TEST on" : "off"));
        drawText(WINDOW_W - 360, 54, "Lighting: fixed-function light/material");
        drawText(WINDOW_W - 360, 32, "Collision/reflection + RGB color model");
    }

    if (exitPrompt) {
        glColor4f(0.0f, 0.0f, 0.0f, 0.72f);
        glBegin(GL_QUADS);
        glVertex2f(365, 310);
        glVertex2f(815, 310);
        glVertex2f(815, 455);
        glVertex2f(365, 455);
        glEnd();

        glColor3f(1.0f, 0.25f, 0.35f);
        drawText(500, 420, "EXIT GAME?", GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.86f, 0.95f, 1.0f);
        drawText(455, 378, "Press ESC again to exit");
        drawText(455, 350, "Press SPACE to cancel");
    }
    end2D();
}

// ==================================================
// 13. GLUT callbacks
// ==================================================

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (cameraMode == 0) {
        gluLookAt(0, -1.2f, 18.5f, 0, 0.3f, 0, 0, 1, 0);
    } else if (cameraMode == 1) {
        gluLookAt(paddle.p.x * 0.45f, -15.4f, 10.2f, paddle.p.x * 0.18f, 0.2f, 0, 0, 0, 1);
    } else {
        gluLookAt(20.0f, -3.2f, 13.0f, 0, 0.1f, 0, 0, 1, 0);
    }

    if (gameState == MENU) {
        glRotatef(std::sin(globalTime) * 4.0f, 0, 0, 1);
    }
    drawScene3D();
    drawHUD();
    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(48.0, double(w) / double(std::max(1, h)), 1.0, 80.0);
    glMatrixMode(GL_MODELVIEW);
}

void idle() {
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - lastTimeMs) / 1000.0f;
    lastTimeMs = now;
    dt = clampf(dt, 0.0f, 0.033f);
    globalTime += dt;

    if (gameState == LEVEL_COMPLETE) {
        // Space advances; this keeps the state stable for reading the message.
    }
    updateGame(dt);
    glutPostRedisplay();
}

void keyboardMenuAdvance(unsigned char key) {
    if (key == ' ' && gameState == LEVEL_COMPLETE) {
        setupLevel(levelNo + 1);
    }
}

// ==================================================
// 14. main
// ==================================================

int main(int argc, char** argv) {
    srand((unsigned)time(0));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_W, WINDOW_H);
    glutCreateWindow("Bezier Breakout: Curve Ball Arena");

    initGL();
    initSound();
    initFeatureDefaults();
    paddle = {Vec3(0, -7.25f, 0), 2.45f, 0.35f, 8.5f, 0.0f};
    resetBall(false);
    lastTimeMs = glutGet(GLUT_ELAPSED_TIME);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc([](unsigned char key, int x, int y) {
        if (!exitPrompt && gameState == LEVEL_COMPLETE && key == ' ') {
            setupLevel(levelNo + 1);
            return;
        }
        keyboard(key, x, y);
    });
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKey);
    glutSpecialUpFunc(specialKeyUp);

    glutMainLoop();
    return 0;
}
