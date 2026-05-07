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
bool playerIsMoving = false;

bool keyStates[256] = {false};
bool specialKeyStates[256] = {false};

float animationTime = 0.0f;

// Camera mode 0 = third-person, camera mode 1 = top-view.
int cameraMode = 0;
float cameraDistance = 18.0f;
float cameraHeight = 9.0f;

float degreesToRadians(float degrees) {
    return degrees * 3.14159265f / 180.0f;
}

void drawScaledCube(float width, float height, float depth) {
    glPushMatrix();
        glScalef(width, height, depth);
        glutSolidCube(1.0f);
    glPopMatrix();
}

void drawCylinder(float radius, float height, int slices = 20) {
    GLUquadric* quadric = gluNewQuadric();

    glPushMatrix();
        // GLU cylinders are drawn along the Z axis by default.
        // Rotating around X makes the cylinder stand vertically on the Y axis.
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(quadric, radius, radius, height, slices, 1);
    glPopMatrix();

    gluDeleteQuadric(quadric);
}

void drawGround() {
    // Main grass field for the simplified RUET-inspired campus.
    glDisable(GL_LIGHTING);
    glColor3f(0.22f, 0.52f, 0.24f);

    glBegin(GL_QUADS);
        glVertex3f(-70.0f, 0.0f, -70.0f);
        glVertex3f( 70.0f, 0.0f, -70.0f);
        glVertex3f( 70.0f, 0.0f,  70.0f);
        glVertex3f(-70.0f, 0.0f,  70.0f);
    glEnd();
}

void drawFlatRect(float x, float z, float width, float depth, float r, float g, float b) {
    glPushMatrix();
        glTranslatef(x, 0.03f, z);
        glColor3f(r, g, b);
        drawScaledCube(width, 0.05f, depth);
    glPopMatrix();
}

void drawRoads() {
    // Roads are thin scaled cubes placed slightly above the grass to avoid flicker.
    drawFlatRect(0.0f, 0.0f, 12.0f, 120.0f, 0.18f, 0.18f, 0.18f);
    drawFlatRect(0.0f, 0.0f, 120.0f, 10.0f, 0.20f, 0.20f, 0.20f);
    drawFlatRect(34.0f, 22.0f, 8.0f, 42.0f, 0.17f, 0.17f, 0.17f);
    drawFlatRect(-36.0f, -18.0f, 8.0f, 36.0f, 0.17f, 0.17f, 0.17f);

    // Pale center strips make the roads more readable from top view.
    drawFlatRect(0.0f, 0.0f, 0.7f, 116.0f, 0.82f, 0.78f, 0.55f);
    drawFlatRect(0.0f, 0.0f, 116.0f, 0.7f, 0.82f, 0.78f, 0.55f);
}

void drawWindow(float x, float y, float z, float width, float height) {
    glPushMatrix();
        glTranslatef(x, y, z);
        glColor3f(0.28f, 0.62f, 0.90f);
        drawScaledCube(width, height, 0.08f);
    glPopMatrix();
}

void drawDoor(float x, float z, float width, float height) {
    glPushMatrix();
        glTranslatef(x, height * 0.5f, z);
        glColor3f(0.34f, 0.18f, 0.08f);
        drawScaledCube(width, height, 0.12f);
    glPopMatrix();
}

void drawHall() {
    // Sher-e-Bangla Hall area: broad low building on the left side of the map.
    glPushMatrix();
        glTranslatef(-38.0f, 3.5f, -20.0f);
        glColor3f(0.78f, 0.72f, 0.58f);
        drawScaledCube(24.0f, 7.0f, 12.0f);
    glPopMatrix();

    glPushMatrix();
        glTranslatef(-38.0f, 7.4f, -20.0f);
        glColor3f(0.46f, 0.23f, 0.14f);
        drawScaledCube(26.0f, 0.8f, 13.0f);
    glPopMatrix();

    drawDoor(-38.0f, -26.1f, 3.2f, 3.2f);

    for (int i = 0; i < 5; i++) {
        float x = -47.0f + i * 4.5f;
        drawWindow(x, 4.5f, -26.1f, 2.0f, 1.5f);
    }

    for (int i = 0; i < 4; i++) {
        glPushMatrix();
            glTranslatef(-46.0f + i * 5.3f, 1.75f, -28.5f);
            glColor3f(0.86f, 0.82f, 0.72f);
            drawCylinder(0.35f, 3.5f);
        glPopMatrix();
    }
}

void drawLibrary() {
    // RUET Library area: taller block near the back of the gameplay map.
    glPushMatrix();
        glTranslatef(-8.0f, 5.0f, 42.0f);
        glColor3f(0.86f, 0.84f, 0.76f);
        drawScaledCube(28.0f, 10.0f, 14.0f);
    glPopMatrix();

    glPushMatrix();
        glTranslatef(-8.0f, 10.6f, 42.0f);
        glColor3f(0.38f, 0.38f, 0.42f);
        drawScaledCube(30.0f, 1.0f, 15.5f);
    glPopMatrix();

    drawDoor(-8.0f, 34.9f, 3.0f, 3.8f);

    for (int row = 0; row < 2; row++) {
        for (int i = 0; i < 6; i++) {
            float x = -19.0f + i * 4.4f;
            float y = 4.3f + row * 3.0f;
            drawWindow(x, y, 34.9f, 1.8f, 1.3f);
        }
    }
}

void drawCSEBuilding() {
    // CSE Department area: main academic building on the right side.
    glPushMatrix();
        glTranslatef(36.0f, 5.5f, 16.0f);
        glColor3f(0.72f, 0.76f, 0.78f);
        drawScaledCube(24.0f, 11.0f, 16.0f);
    glPopMatrix();

    glPushMatrix();
        glTranslatef(36.0f, 11.4f, 16.0f);
        glColor3f(0.18f, 0.27f, 0.36f);
        drawScaledCube(25.5f, 0.8f, 17.2f);
    glPopMatrix();

    drawDoor(36.0f, 7.9f, 3.4f, 4.0f);

    for (int row = 0; row < 2; row++) {
        for (int i = 0; i < 5; i++) {
            float x = 27.5f + i * 4.2f;
            float y = 4.3f + row * 3.4f;
            drawWindow(x, y, 7.9f, 1.8f, 1.35f);
        }
    }
}

void drawHPCLLab() {
    // HPCL final lab: smaller highlighted room near the CSE Department.
    glPushMatrix();
        glTranslatef(52.0f, 3.0f, 31.0f);
        glColor3f(0.42f, 0.46f, 0.55f);
        drawScaledCube(12.0f, 6.0f, 10.0f);
    glPopMatrix();

    glPushMatrix();
        glTranslatef(52.0f, 6.4f, 31.0f);
        glColor3f(0.10f, 0.13f, 0.22f);
        drawScaledCube(13.0f, 0.8f, 11.0f);
    glPopMatrix();

    drawDoor(52.0f, 25.9f, 2.6f, 3.0f);

    glPushMatrix();
        glTranslatef(52.0f, 4.2f, 25.8f);
        glColor3f(0.1f, 0.85f, 1.0f);
        drawScaledCube(5.0f, 1.1f, 0.1f);
    glPopMatrix();
}

void drawTree(float x, float z, float scale) {
    glPushMatrix();
        glTranslatef(x, 0.0f, z);
        glScalef(scale, scale, scale);

        glPushMatrix();
            glColor3f(0.34f, 0.18f, 0.08f);
            drawCylinder(0.22f, 2.2f);
        glPopMatrix();

        glPushMatrix();
            glTranslatef(0.0f, 2.7f, 0.0f);
            glColor3f(0.08f, 0.42f, 0.13f);
            glutSolidSphere(1.0f, 18, 12);
        glPopMatrix();
    glPopMatrix();
}

void drawLamp(float x, float z) {
    glPushMatrix();
        glTranslatef(x, 0.0f, z);

        glPushMatrix();
            glColor3f(0.12f, 0.12f, 0.14f);
            drawCylinder(0.12f, 4.0f);
        glPopMatrix();

        glPushMatrix();
            glTranslatef(0.0f, 4.25f, 0.0f);
            glColor3f(1.0f, 0.92f, 0.45f);
            glutSolidSphere(0.45f, 16, 10);
        glPopMatrix();
    glPopMatrix();
}

void drawCampus() {
    // Main environment draw order: ground, roads, buildings, then decorations.
    drawGround();
    drawRoads();

    drawHall();
    drawLibrary();
    drawCSEBuilding();
    drawHPCLLab();

    // Trees around the campus edges and near building zones.
    for (int i = 0; i < 7; i++) {
        drawTree(-58.0f + i * 7.0f, -44.0f, 1.0f);
        drawTree(-58.0f + i * 7.0f,  58.0f, 1.1f);
        drawTree( 58.0f, -46.0f + i * 14.0f, 0.95f);
        drawTree(-58.0f, -46.0f + i * 14.0f, 0.95f);
    }

    drawTree(-52.0f, -8.0f, 1.1f);
    drawTree(-25.0f, -34.0f, 1.0f);
    drawTree(-24.0f, 34.0f, 1.0f);
    drawTree(24.0f, 32.0f, 1.0f);
    drawTree(48.0f, 2.0f, 0.9f);

    // Lamps follow the central roads so the future route is readable.
    for (int i = 0; i < 6; i++) {
        drawLamp(-8.0f, -45.0f + i * 18.0f);
        drawLamp( 8.0f, -36.0f + i * 18.0f);
    }

    drawLamp(-32.0f, 5.5f);
    drawLamp(18.0f, -5.5f);
    drawLamp(42.0f, 6.0f);
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

void drawPlayer() {
    // Hierarchical modeling idea:
    // First transform the whole player to the player's world position.
    // Then each body part uses its own glPushMatrix/glPopMatrix so its
    // translation, rotation, and scale do not accidentally affect other parts.
    glDisable(GL_LIGHTING);

    float swing = 0.0f;
    if (playerIsMoving) {
        swing = std::sin(animationTime * 8.0f) * 25.0f;
    }

    glPushMatrix();
        glTranslatef(playerX, playerY, playerZ);
        glRotatef(playerYaw, 0.0f, 1.0f, 0.0f);

        // Body
        glPushMatrix();
            glTranslatef(0.0f, 1.45f, 0.0f);
            glColor3f(0.12f, 0.22f, 0.55f);
            drawScaledCube(0.85f, 1.35f, 0.45f);
        glPopMatrix();

        // Head
        glPushMatrix();
            glTranslatef(0.0f, 2.45f, 0.0f);
            glColor3f(0.95f, 0.72f, 0.52f);
            glutSolidSphere(0.38f, 24, 16);
        glPopMatrix();

        // Left arm
        glPushMatrix();
            glTranslatef(-0.62f, 1.85f, 0.0f);
            glRotatef(swing, 1.0f, 0.0f, 0.0f);
            glTranslatef(0.0f, -0.45f, 0.0f);
            glColor3f(0.12f, 0.22f, 0.55f);
            drawScaledCube(0.25f, 0.9f, 0.25f);
        glPopMatrix();

        // Right arm
        glPushMatrix();
            glTranslatef(0.62f, 1.85f, 0.0f);
            glRotatef(-swing, 1.0f, 0.0f, 0.0f);
            glTranslatef(0.0f, -0.45f, 0.0f);
            glColor3f(0.12f, 0.22f, 0.55f);
            drawScaledCube(0.25f, 0.9f, 0.25f);
        glPopMatrix();

        // Left leg
        glPushMatrix();
            glTranslatef(-0.25f, 0.75f, 0.0f);
            glRotatef(-swing, 1.0f, 0.0f, 0.0f);
            glTranslatef(0.0f, -0.35f, 0.0f);
            glColor3f(0.16f, 0.16f, 0.18f);
            drawScaledCube(0.28f, 0.85f, 0.28f);
        glPopMatrix();

        // Right leg
        glPushMatrix();
            glTranslatef(0.25f, 0.75f, 0.0f);
            glRotatef(swing, 1.0f, 0.0f, 0.0f);
            glTranslatef(0.0f, -0.35f, 0.0f);
            glColor3f(0.16f, 0.16f, 0.18f);
            drawScaledCube(0.28f, 0.85f, 0.28f);
        glPopMatrix();
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
    drawCampus();
    drawCoordinateAxes();
    drawPlayer();

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
    gluPerspective(60.0f, aspectRatio, 0.1f, 800.0f);

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
    playerIsMoving = false;

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
        playerIsMoving = true;

        moveX /= length;
        moveZ /= length;

        playerX += moveX * playerMoveSpeed * deltaTime;
        playerZ += moveZ * playerMoveSpeed * deltaTime;
    }
}

void update(int value) {
    (void)value;

    // Timer callback gives a steady animation/update point for future game logic.
    animationTime += 0.016f;
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
