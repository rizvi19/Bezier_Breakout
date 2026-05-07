#include <GL/freeglut.h>
#include <cmath>
#include <cstdlib>

// RUET Rift: The 10-Hour Distortion
// Clean base project for a syllabus-friendly C++ OpenGL/GLUT program.

int windowWidth = 1280;
int windowHeight = 720;

// Temporary player position. The player model and movement will be added later.
float playerX = 0.0f;
float playerY = 0.0f;
float playerZ = 0.0f;
float playerYaw = 0.0f;
float playerMoveSpeed = 8.0f;
float playerTurnSpeed = 120.0f;

bool keyStates[256] = {false};
bool specialKeyStates[256] = {false};

// Camera mode 0 = third-person, camera mode 1 = top-view.
int cameraMode = 0;
float cameraDistance = 18.0f;
float cameraHeight = 9.0f;

float degreesToRadians(float degrees) {
    return degrees * 3.14159265f / 180.0f;
}

void drawGroundPlane() {
    // A flat rectangle is enough for the first base scene.
    glDisable(GL_LIGHTING);
    glColor3f(0.22f, 0.52f, 0.24f);

    glBegin(GL_QUADS);
        glVertex3f(-30.0f, 0.0f, -30.0f);
        glVertex3f( 30.0f, 0.0f, -30.0f);
        glVertex3f( 30.0f, 0.0f,  30.0f);
        glVertex3f(-30.0f, 0.0f,  30.0f);
    glEnd();
}

void drawCoordinateAxes() {
    // Temporary debug axes:
    // X axis = red, Y axis = green, Z axis = blue.
    glDisable(GL_LIGHTING);
    glLineWidth(3.0f);

    glBegin(GL_LINES);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.03f, 0.0f);
        glVertex3f(8.0f, 0.03f, 0.0f);

        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, 0.03f, 0.0f);
        glVertex3f(0.0f, 8.0f, 0.0f);

        glColor3f(0.0f, 0.25f, 1.0f);
        glVertex3f(0.0f, 0.03f, 0.0f);
        glVertex3f(0.0f, 0.03f, 8.0f);
    glEnd();

    glLineWidth(1.0f);
}

void drawPlayerPlaceholder() {
    // A simple cube player is enough until the full humanoid model is added.
    glDisable(GL_LIGHTING);

    glPushMatrix();
        glTranslatef(playerX, playerY + 0.75f, playerZ);
        glRotatef(playerYaw, 0.0f, 1.0f, 0.0f);

        glColor3f(0.95f, 0.75f, 0.18f);
        glutSolidCube(1.5f);

        // Small cone points toward the player's forward direction.
        glTranslatef(0.0f, 0.0f, -1.15f);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        glColor3f(0.15f, 0.18f, 0.25f);
        glutSolidCone(0.45f, 1.0f, 16, 1);
    glPopMatrix();
}

void setupCamera() {
    // MODELVIEW controls the camera and object transformations.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // gluLookAt works like this:
    // 1. eye position    = where the camera is placed
    // 2. center position = the point the camera looks at
    // 3. up direction    = which direction should be considered upward
    if (cameraMode == 0) {
        // Third-person camera: placed behind and above the player.
        float yawRadians = degreesToRadians(playerYaw);
        float forwardX = std::sin(yawRadians);
        float forwardZ = -std::cos(yawRadians);

        float eyeX = playerX - forwardX * cameraDistance;
        float eyeY = playerY + cameraHeight;
        float eyeZ = playerZ - forwardZ * cameraDistance;

        gluLookAt(
            eyeX, eyeY, eyeZ,
            playerX, playerY + 1.0f, playerZ,
            0.0f, 1.0f, 0.0f
        );
    } else {
        // Top-view camera: placed high above the temporary player.
        gluLookAt(
            playerX, playerY + 45.0f, playerZ + 0.1f,
            playerX, playerY,         playerZ,
            0.0f,   0.0f,            -1.0f
        );
    }
}

void display() {
    // Clear both color and depth buffers before drawing a new frame.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setupCamera();
    drawGroundPlane();
    drawCoordinateAxes();
    drawPlayerPlaceholder();

    // GLUT_DOUBLE uses a back buffer. Swap makes the completed frame visible.
    glutSwapBuffers();
}

void reshape(int width, int height) {
    if (height == 0) {
        height = 1;
    }

    windowWidth = width;
    windowHeight = height;

    // Viewport maps normalized OpenGL output to the actual window pixels.
    glViewport(0, 0, windowWidth, windowHeight);

    // PROJECTION controls perspective: field of view, aspect ratio, near/far planes.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float aspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    gluPerspective(60.0f, aspectRatio, 0.1f, 500.0f);

    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y) {
    (void)x;
    (void)y;
    keyStates[key] = true;

    switch (key) {
        case 27: // Esc
            std::exit(0);
            break;

        case 'r':
        case 'R':
            cameraMode = 0;
            playerX = 0.0f;
            playerY = 0.0f;
            playerZ = 0.0f;
            playerYaw = 0.0f;
            break;

        case 'c':
        case 'C':
            cameraMode = (cameraMode + 1) % 2;
            break;

        default:
            break;
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    (void)x;
    (void)y;
    keyStates[key] = false;
}

void specialKeyDown(int key, int x, int y) {
    (void)x;
    (void)y;
    specialKeyStates[key] = true;
}

void specialKeyUp(int key, int x, int y) {
    (void)x;
    (void)y;
    specialKeyStates[key] = false;
}

void updatePlayer(float deltaTime) {
    if (specialKeyStates[GLUT_KEY_LEFT]) {
        playerYaw += playerTurnSpeed * deltaTime;
    }

    if (specialKeyStates[GLUT_KEY_RIGHT]) {
        playerYaw -= playerTurnSpeed * deltaTime;
    }

    // Movement is kept on the XZ plane. Y does not change.
    // If yaw is the player's facing angle, sin(yaw) gives X direction
    // and -cos(yaw) gives Z direction for forward movement.
    float yawRadians = degreesToRadians(playerYaw);
    float forwardX = std::sin(yawRadians);
    float forwardZ = -std::cos(yawRadians);

    // The right vector is perpendicular to the forward vector.
    float rightX = std::cos(yawRadians);
    float rightZ = std::sin(yawRadians);

    float moveX = 0.0f;
    float moveZ = 0.0f;

    if (keyStates['w'] || keyStates['W']) {
        moveX += forwardX;
        moveZ += forwardZ;
    }

    if (keyStates['s'] || keyStates['S']) {
        moveX -= forwardX;
        moveZ -= forwardZ;
    }

    if (keyStates['d'] || keyStates['D']) {
        moveX += rightX;
        moveZ += rightZ;
    }

    if (keyStates['a'] || keyStates['A']) {
        moveX -= rightX;
        moveZ -= rightZ;
    }

    float length = std::sqrt(moveX * moveX + moveZ * moveZ);
    if (length > 0.0f) {
        moveX /= length;
        moveZ /= length;

        playerX += moveX * playerMoveSpeed * deltaTime;
        playerZ += moveZ * playerMoveSpeed * deltaTime;
    }
}

void update(int value) {
    (void)value;

    // Timer callback gives a steady animation/update point for future game logic.
    updatePlayer(0.016f);

    glutPostRedisplay();
    glutTimerFunc(16, update, 0); // About 60 frames per second.
}

void initializeOpenGL() {
    // Depth testing allows nearer 3D objects to correctly hide farther objects.
    glEnable(GL_DEPTH_TEST);

    // Smooth line/primitive colors are useful for simple classroom demos.
    glShadeModel(GL_SMOOTH);

    // Background sky color for the empty base scene.
    glClearColor(0.48f, 0.72f, 0.95f, 1.0f);
}

int main(int argc, char** argv) {
    // Initialize GLUT and create one double-buffered RGB window with depth support.
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(100, 80);
    glutCreateWindow("RUET Rift: The 10-Hour Distortion");

    initializeOpenGL();

    // Register GLUT callbacks. GLUT calls these functions when needed.
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKeyDown);
    glutSpecialUpFunc(specialKeyUp);
    glutTimerFunc(16, update, 0);

    glutMainLoop();
    return 0;
}
