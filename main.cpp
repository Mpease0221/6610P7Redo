#include <cyTriMesh.h>
#include <GL/glew.h>
#include <cyMatrix.h>
#include <GL/freeglut.h>
#include <cyGL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


unsigned int screenWidth = 800;
unsigned int screenHeight = 800;

void keyboardCallback(unsigned char, int, int);
void mouseCallback(int, int, int, int);
void idleCallback();
void displayCallback();
// This is used for stuff like the F6 key.
void specialFuncCallback(int key, int x, int y);

// === TEAPOT
GLuint teapotVao;
// The mesh containing our object.
cyTriMesh teapotMesh;
// The teapotProgram containing our vert shader and frag shader.
cy::GLSLProgram teapotProgram;
void buildTeapot(char*);

// === The floor.
GLuint floorVAO;
cy::GLSLProgram debugProgram;
cy::GLSLProgram floorProgram;
void buildFloor();

// === Light properties.
glm::vec3 lightPos = glm::vec3(20, 50, 50);
glm::vec3 lightTarget = glm::vec3(0, 0, 0);
float lightSpread = glm::radians(90.f);

// === Shadow properties.
cy::GLSLProgram shadowProgram;
unsigned int depthMapFBO;
unsigned int depthMap;
unsigned int shadWidth = 1600;
unsigned int shadHeight = 1600;


// === Camera Properties.
// Mouse callback fields.
int firstX = 0;
int firstY = 0;

float cameraxRot = 20;
float camerayRot = 40;
float cameraZoom;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Please provide a file name to load!" << std::endl;
        return 1;
    }
    glutInit(&argc, argv);
    glutInitContextFlags(GLUT_DEBUG);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glEnable(GL_DEPTH_TEST);
    // This helps with peter panning.
    glEnable(GL_CULL_FACE);

    glViewport(0, 0, screenWidth, screenHeight);
    glutInitWindowSize(screenWidth, screenHeight);
    glutCreateWindow("Project 7 Shadow Mapping");
    // Register the callbacks here.
    glutIdleFunc(idleCallback);
    glutDisplayFunc(displayCallback);
    glutKeyboardUpFunc(keyboardCallback);
    glutMouseFunc(mouseCallback);
    glutSpecialFunc(specialFuncCallback);
    // Initializes every extension!
    // Avoids a ton of complexity!
    glewInit();

    // Build our teapotProgram.
    // Must be binded before setting uniforms.
    teapotProgram.Bind();
    teapotProgram.BuildFiles("../teapot.vert", "../teapot.frag");
    buildTeapot(argv[1]);

    debugProgram.Bind();
    debugProgram.BuildFiles("../debug.vert", "../debug.frag");
    floorProgram.Bind();
    floorProgram.BuildFiles("../floor.vert", "../floor.frag");
    buildFloor();

    // BUILD THE ZOOM
    teapotMesh.ComputeBoundingBox();
    cameraZoom = (teapotMesh.GetBoundMax() - teapotMesh.GetBoundMin()).Length();


    // ===== SHADOW MAP INIT =====
    shadowProgram.BuildFiles("../shadow.vert", "../shadow.frag");
    glGenFramebuffers(1, &depthMapFBO);

    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadWidth, shadHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                 nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    // We have to specify that we are not using any color buffer!
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    // FAILURE CHECK!
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer failed!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glutMainLoop();
    return 0;
}
void buildFloor(){
    // === Setup the TV Screen VAO.
    glGenVertexArrays(1, &floorVAO);
    glBindVertexArray(floorVAO);

    // The tutorial is using "normalized device coordinates" here.
    // I can always change this :D
    float tvVertices[] = {
            30.f,  -0.f, 30.0f,  // top right
            30.f, -0.f, -30.0f,  // bottom right
            -30.f, -0.f, -30.0f,  // bottom left

            -30.f, -0.f, -30.0f,  // bottom left
            -30.f,  -0.f, 30.0f,   // top left
            30.f,  -0.f, 30.0f,  // top right
    };

    float tvTexCoords[] = {
            1.f, 1.f,
            1.f, 0.f,
            0.f, 0.f,

            0.f, 0.f,
            0.f, 1.f,
            1.f, 1.f
    };

    unsigned int tvPosVBO;
    glGenBuffers(1, &tvPosVBO);
    glBindBuffer(GL_ARRAY_BUFFER, tvPosVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tvVertices), tvVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int tvTexVBO;
    glGenBuffers(1, &tvTexVBO);
    glBindBuffer(GL_ARRAY_BUFFER, tvTexVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tvTexCoords), tvTexCoords, GL_STATIC_DRAW);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), 0);
    glEnableVertexAttribArray(1);
}
void buildTeapot(char* fileName){
    // Load in the object.
    teapotMesh = cyTriMesh();
    teapotMesh.LoadFromFileObj(fileName, true);

    // First register VAO. What goes in our VAO??
    // Must have one, otherwise behavior is undefined.
    glCreateVertexArrays(1, &teapotVao);
    // Tells OpenGL to use this teapotVao!
    glBindVertexArray(teapotVao);

    // Build the buffer containing all the verts!
    cy::Vec3f vertices[teapotMesh.NV()];
    for (int i = 0; i < teapotMesh.NV(); i++) {
        vertices[i] = teapotMesh.V(i);
    }

    // Construct an array to contain the vertices of EVERY face.
    // This means that each vertex will appear multiple times!
    cy::Vec3f positions[teapotMesh.NF() * 3];
    for (int i = 0; i < teapotMesh.NF(); i++) {
        int offset = i * 3;
        for (int j = 0; j < 3; j++) {
            positions[offset + j] = vertices[teapotMesh.F(i).v[j]];
        }
    }
    // VBO Start.
    // Lecture 5, 51:05.
    GLuint vertBuffer;
    glGenBuffers(1, &vertBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);

    GLuint pos = glGetAttribLocation(teapotProgram.GetID(), "pos");
    // Use this attribute while rendering.
    glEnableVertexAttribArray(pos);
    // How to interpret the data.
    // We will be using the previously binded buffer, "buffer".
    // 3 is because positions are vec3.
    // Start at the beginning of the vertex buffer object.
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *) 0);

    // ============= BUILD THE NORMAL BUFFER ==============
    cy::Vec3f normals[teapotMesh.NF()*3];
    for(int i = 0; i < teapotMesh.NF(); i++){
        int offset = i*3;
        for(int j = 0; j < 3; j++){
            normals[offset+j] = teapotMesh.VN(teapotMesh.FN(i).v[j]);
        }
    }
    GLuint normBuffer;
    glGenBuffers(1, &normBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);
    GLuint norm = glGetAttribLocation(teapotProgram.GetID(), "norm");
    glEnableVertexAttribArray(norm);
    glVertexAttribPointer(norm, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *) 0);
}

void displayCallback() {
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_FRONT);
    // First Pass -> render to depth map.
    glViewport(0, 0, shadWidth, shadHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    // We want to use our custom shadow shader!
    // This way we get a view of the scene from the perspective of the camera.
    // This contains the MLP matrix.
    shadowProgram.Bind();
    glBindVertexArray(teapotVao);
    glDrawArrays(GL_TRIANGLES, 0, teapotMesh.NF() * 3);
    glBindVertexArray(floorVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glCullFace(GL_BACK);

    // Second Pass
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.f, 0.2f, 0.2f, 1.f);
    // Rerender the teapot.
    glBindVertexArray(teapotVao);
    teapotProgram.Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glDrawArrays(GL_TRIANGLES, 0, teapotMesh.NF() * 3);

//    debugProgram.Bind();
    floorProgram.Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glBindVertexArray(floorVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glutSwapBuffers();
}

void idleCallback() {
    // Pass the view matrices to the shaders.
    teapotProgram.Bind();
    auto teapotModel = glm::mat4(1.f);
    auto teapotView = glm::mat4(1.f);
    auto teapotProjection = glm::mat4(1.f);

    unsigned int modelLoc = glGetUniformLocation(teapotProgram.GetID(), "model");
    unsigned int viewLoc = glGetUniformLocation(teapotProgram.GetID(), "view");
    unsigned int projectionLoc = glGetUniformLocation(teapotProgram.GetID(), "projection");

//    teapotModel = glm::rotate(teapotModel, glm::radians(-90.f), glm::vec3(1.f, 0, 0));

    glm::vec3 cameraPos;
    cameraPos.x = cos(glm::radians(cameraxRot)) * cos(glm::radians(camerayRot));
    cameraPos.y = sin(glm::radians(camerayRot));
    cameraPos.z = sin(glm::radians(cameraxRot)) * cos(glm::radians(camerayRot));
    cameraPos = 50.f * cameraPos;

    teapotView = glm::lookAt(glm::vec3(cameraPos), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    teapotProjection = glm::perspective(glm::radians(cameraZoom), 1.0f, 0.1f, cameraZoom * 5);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &teapotModel[0][0]);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &teapotView[0][0]);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &teapotProjection[0][0]);

    unsigned int lightPosLoc = glGetUniformLocation(teapotProgram.GetID(), "lightPos");
    unsigned int viewPosLoc = glGetUniformLocation(teapotProgram.GetID(), "viewPos");
    glUniform3fv(lightPosLoc, 1, &lightPos[0]);
    glUniform3fv(viewPosLoc, 1, &cameraPos[0]);


    shadowProgram.Bind();
    auto shadowModel = glm::mat4(1.f);
    // The shadow view matrix has the camera located AT the light source!
    auto shadowView = glm::mat4(1.f);
    shadowView = glm::lookAt(lightPos, lightTarget, glm::vec3(0, 1, 0));
    auto shadowProjection = glm::mat4(1.f);
    // We can use a perspective matrix as a spotlight effect!
    // 1000 is some bogus number, lol!
    shadowProjection = glm::perspective(lightSpread, 1.0f, 0.1f, 200.f);
// TODO: USE THE PERSPECTIVE MATRIX INSTEAD!
    modelLoc = glGetUniformLocation(shadowProgram.GetID(), "model");
    viewLoc = glGetUniformLocation(shadowProgram.GetID(), "view");
    projectionLoc = glGetUniformLocation(shadowProgram.GetID(), "projection");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &shadowModel[0][0]);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &shadowView[0][0]);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &shadowProjection[0][0]);

    // We must now pass in the shadowView and shadowProjection to the original shader.
    teapotProgram.Bind();
    viewLoc = glGetUniformLocation(teapotProgram.GetID(), "shadowView");
    projectionLoc = glGetUniformLocation(teapotProgram.GetID(), "shadowProjection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &shadowView[0][0]);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &shadowProjection[0][0]);


    // Debug Program
    debugProgram.Bind();
    modelLoc = glGetUniformLocation(debugProgram.GetID(), "model");
    viewLoc = glGetUniformLocation(debugProgram.GetID(), "view");
    projectionLoc = glGetUniformLocation(debugProgram.GetID(), "projection");

    auto floorModel = glm::mat4(1.f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &floorModel[0][0]);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &teapotView[0][0]);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &teapotProjection[0][0]);

    // === Floor Program
    floorProgram.Bind();
    modelLoc = glGetUniformLocation(floorProgram.GetID(), "model");
    viewLoc = glGetUniformLocation(floorProgram.GetID(), "view");
    projectionLoc = glGetUniformLocation(floorProgram.GetID(), "projection");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &teapotModel[0][0]);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &teapotView[0][0]);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &teapotProjection[0][0]);

    lightPosLoc = glGetUniformLocation(floorProgram.GetID(), "lightPos");
    viewPosLoc = glGetUniformLocation(floorProgram.GetID(), "viewPos");
    glUniform3fv(lightPosLoc, 1, &lightPos[0]);
    glUniform3fv(viewPosLoc, 1, &cameraPos[0]);

    viewLoc = glGetUniformLocation(floorProgram.GetID(), "shadowView");
    projectionLoc = glGetUniformLocation(floorProgram.GetID(), "shadowProjection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &shadowView[0][0]);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &shadowProjection[0][0]);

    // Tell GLUT to redraw.
    glutPostRedisplay();
}

void keyboardCallback(unsigned char key, int x, int y) {
    if (key == 27) {
        glutLeaveMainLoop();
    }
}

/**
 * Handles the functional keys on the keyboard.
 * Like the F6 key.
 * @param key The special key that is pressed.
 * @param x Relative x position of the mouse when key is pressed.
 * @param y Relative y position of the mouse when key is pressed.
 */
void specialFuncCallback(int key, int x, int y) {
    if (key == GLUT_KEY_F6) {
        std::cout << "Rebuilt Shaders!" << std::endl;
        teapotProgram.BuildFiles("../teapot.vert", "../teapot.frag");
        teapotProgram.Bind();
        // Tell GLUT to redraw.
        glutPostRedisplay();
    }
}

/**
 * Converts degree measurement to radians.
 * @param degrees The amount of degrees to be converted.
 * @return The converted angle in radians.
 */
float degreesToRadians(float degrees) {
    return (3.14159 * degrees) / 180;
}

void mouseCallback(int button, int state, int x, int y) {
    // Button 0 is left click.
    // Button 2 is right click.
    // State 0 is begin, 1 is release.
    if (state == 0) {
        firstX = x;
        firstY = y;
        return;
    }
    int deltaX = x - firstX;
    int deltaY = y - firstY;

    if (button == 0) {
        cameraxRot += deltaX / 5;
        camerayRot += deltaY / 5;
    }
    if (button == 2) {
        cameraZoom += deltaY / 15;

    }
}
