#include <iostream>
#include <cyTriMesh.h>
#include <GL/glew.h>
#include <cyMatrix.h>
#include <GL/freeglut.h>
#include <cyGL.h>
#include "lodepng.h"
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


unsigned int screenWidth = 800;
unsigned int screenHeight = 800;

void keyboardCallback(unsigned char, int, int);
void mouseCallback(int, int, int, int);
void idleCallback();
void displayCallback();
// This is used for stuff like the F6 key.
void specialFuncCallback(int key, int x, int y);

GLuint teapotVao;
// The mesh containing our object.
cyTriMesh teapotMesh;
// The program containing our vert shader and frag shader.
cy::GLSLProgram program;
cyGLTexture2D teapotTexture;

GLuint floorVAO;
cy::GLSLProgram floorProgram;

cy::GLSLProgram shadowProgram;
unsigned int depthMapFBO;
unsigned int depthMap;
unsigned int shadWidth = 1600;
unsigned int shadHeight = 1600;
// The light position.
glm::vec3 lightPos = glm::vec3(50, 25, 50);
glm::vec3 lightTarget = glm::vec3(0, 0, 0);
float lightSpread = glm::radians(75.f);

// Mouse callback fields.
int firstX = 0;
int firstY = 0;

// Orientation of our object.
float teapotxRot = 0;
float teapotyRot = 0;
float teapotZoom;

unsigned int rbo;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Please provide a file name to load!" << std::endl;
        return 1;
    }
    glutInit(&argc, argv);
    // Allow openGL to generate interupts whenever there is an error.
    glutInitContextFlags(GLUT_DEBUG);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glEnable(GL_DEPTH_TEST);
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

    // Load in the object.
    teapotMesh = cyTriMesh();
    char *file = argv[1];
    teapotMesh.LoadFromFileObj(file, true);

    // Build our program.
    // Must be binded before setting uniforms.
    program.Bind();
    program.BuildFiles("../shader.vert", "../shader.frag");
    // Must be binded before setting uniforms.
    program.Bind();

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

    GLuint pos = glGetAttribLocation(program.GetID(), "pos");
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
    GLuint norm = glGetAttribLocation(program.GetID(), "norm");
    glEnableVertexAttribArray(norm);
    glVertexAttribPointer(norm, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *) 0);

//    // ============= BUILD THE TEXTURE BUFFER ==============
//    cy::Vec3f textures[teapotMesh.NF() * 3];
//    for (int i = 0; i < teapotMesh.NF(); i++) {
//        int offset = i * 3;
//        for (int j = 0; j < 3; j++) {
//            textures[offset + j] = teapotMesh.VT(teapotMesh.FT(i).v[j]);
//        }
//    }
//    teapotMesh.F(teapotMesh.NF() * 3 - 1);
//    teapotMesh.FT(teapotMesh.NF() * 3 - 1);
//    GLuint texBuffer;
//    glGenBuffers(1, &texBuffer);
//    glBindBuffer(GL_ARRAY_BUFFER, texBuffer);
//    glBufferData(GL_ARRAY_BUFFER, sizeof(textures), textures, GL_STATIC_DRAW);
//    GLuint tex = glGetAttribLocation(program.GetID(), "inTex");
//    glEnableVertexAttribArray(tex);
//    glVertexAttribPointer(tex, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *) 0);

    // BUILD THE ZOOM
    teapotMesh.ComputeBoundingBox();
    teapotZoom = (teapotMesh.GetBoundMax() - teapotMesh.GetBoundMin()).Length();

//    // =============== TEXTURE TO GPU =================
//    cy::TriMesh::Mtl mat = teapotMesh.M(0);
//
//    cy::TriMesh::Str textureAmbient = mat.map_Ka;
//    cy::TriMesh::Str textureDiffuse = mat.map_Kd;
//    cy::TriMesh::Str textureSpecular = mat.map_Ks;
//
//    // TODO, Add the rest!
//    std::string diffuseFile = "../Res/" + (std::string) textureDiffuse.data;
//
//    // From https://github.com/lvandeve/lodepng/blob/master/examples/example_decode.cpp
//    // Will hold our raw pixels.
//    std::vector<unsigned char> image;
//    unsigned width, height;
//
//    unsigned error = lodepng::decode(image, width, height, diffuseFile);
//
//    if (error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
//
//    unsigned char pixelArray[image.size()];
//    for (int i = 0; i < image.size(); i++) {
//        pixelArray[i] = image.at(i);
//    }
//
//    teapotTexture.Initialize();
//    teapotTexture.SetImage(pixelArray, 4, width, height);
//    teapotTexture.BuildMipmaps();
//    teapotTexture.Bind(0);
//    program["tex"] = 0;

    program.Bind();


    // ===== SHADOW MAP INIT =====
    shadowProgram.BuildFiles("../shadow.vert", "../shadow.frag");
    glGenFramebuffers(1, &depthMapFBO);
//    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

//    glActiveTexture(GL_TEXTURE1);
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadWidth, shadHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                 nullptr);
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

    // THIS PART IS DIRECTLY COPIED FROM THE WEBSITE!!!
    // "This is optional"
    // NOT TRUE!! WE CAN SEE THROUGH FACES OTHERWISE!!!

    // WHY DOES COMMENTING THIS CHANGE THE TEXTURE COLOR??
//    glGenRenderbuffers(1, &rbo);
//    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
//    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 800, 800); // use a single renderbuffer object for both a depth AND stencil buffer.
//    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
//    // now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
//    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
//        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // === Setup the TV Screen VAO.
    glGenVertexArrays(1, &floorVAO);
    glBindVertexArray(floorVAO);

    // The tutorial is using "normalized device coordinates" here.
    // I can always change this :D
    float tvVertices[] = {
            40.f,  -1.f, 40.0f,  // top right
            40.f, -1.f, -40.0f,  // bottom right
            -40.f, -1.f, -40.0f,  // bottom left

            -40.f, -1.f, -40.0f,  // bottom left
            -40.f,  -1.f, 40.0f,   // top left
            40.f,  -1.f, 40.0f,  // top right
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

    floorProgram.Bind();
    floorProgram.BuildFiles("../tv.vert", "../tv.frag");
    floorProgram.Bind();
//    glUniform1i(glGetUniformLocation(floorProgram.GetID(), "tvTexture"), 1);

    glutMainLoop();
    return 0;
}

void displayCallback() {
    glEnable(GL_DEPTH_TEST);
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

    // Second Pass
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.f, 0.2f, 0.2f, 1.f);
    // Rerender the teapot.
    glBindVertexArray(teapotVao);
    program.Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glDrawArrays(GL_TRIANGLES, 0, teapotMesh.NF() * 3);

//    floorProgram.Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glBindVertexArray(floorVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glutSwapBuffers();
}

void idleCallback() {

    unsigned int time = glutGet(GLUT_ELAPSED_TIME) * 0.001;
    float lightOffsetx = 50 * cos(time);
    float lightOffsetz = 50 *sin(time);
    lightPos = glm::vec3(lightOffsetx, lightPos.y, lightOffsetz);

    // Pass the view matrices to the shaders.
    program.Bind();
    auto teapotModel = glm::mat4(1.f);
    auto teapotView = glm::mat4(1.f);
    auto teapotProjection = glm::mat4(1.f);

    unsigned int modelLoc = glGetUniformLocation(program.GetID(), "model");
    unsigned int viewLoc = glGetUniformLocation(program.GetID(), "view");
    unsigned int projectionLoc = glGetUniformLocation(program.GetID(), "projection");

//    teapotModel = glm::rotate(teapotModel, glm::radians(-90.f), glm::vec3(1.f, 0, 0));

    glm::vec3 cameraPos;
    cameraPos.x = cos(glm::radians(teapotxRot)) * cos(glm::radians(teapotyRot));
    cameraPos.y = sin(glm::radians(teapotyRot));
    cameraPos.z = sin(glm::radians(teapotxRot)) * cos(glm::radians(teapotyRot));
    cameraPos = 50.f * cameraPos;

    teapotView = glm::lookAt(glm::vec3(cameraPos), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    teapotProjection = glm::perspective(glm::radians(teapotZoom), 1.0f, 0.1f, teapotZoom * 5);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &teapotModel[0][0]);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &teapotView[0][0]);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &teapotProjection[0][0]);

    unsigned int lightPosLoc = glGetUniformLocation(program.GetID(), "lightPos");
    unsigned int viewPosLoc = glGetUniformLocation(program.GetID(), "viewPos");
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
    shadowProjection = glm::perspective(lightSpread, 1.0f, 0.1f, 100.f);
// TODO: USE THE PERSPECTIVE MATRIX INSTEAD!
//    shadowProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.f, 1.f, 100.f);
    modelLoc = glGetUniformLocation(shadowProgram.GetID(), "model");
    viewLoc = glGetUniformLocation(shadowProgram.GetID(), "view");
    projectionLoc = glGetUniformLocation(shadowProgram.GetID(), "projection");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &shadowModel[0][0]);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &shadowView[0][0]);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &shadowProjection[0][0]);

    // We must now pass in the shadowView and shadowProjection to the original shader.
    program.Bind();
    viewLoc = glGetUniformLocation(program.GetID(), "shadowView");
    projectionLoc = glGetUniformLocation(program.GetID(), "shadowProjection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &shadowView[0][0]);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &shadowProjection[0][0]);

    // TESTING STUFF!
//    viewLoc = glGetUniformLocation(program.GetID(), "view");
//    projectionLoc = glGetUniformLocation(program.GetID(), "projection");
//    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &shadowView[0][0]);
//    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &shadowProjection[0][0]);


    floorProgram.Bind();

    modelLoc = glGetUniformLocation(floorProgram.GetID(), "model");
    viewLoc = glGetUniformLocation(floorProgram.GetID(), "view");
    projectionLoc = glGetUniformLocation(floorProgram.GetID(), "projection");

    auto floorModel = glm::mat4(1.f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &floorModel[0][0]);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &teapotView[0][0]);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &teapotProjection[0][0]);

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
        program.BuildFiles("../shader.vert", "../shader.frag");
        program.Bind();
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
        teapotxRot += deltaX / 5;
        teapotyRot += deltaY / 5;
    }
    if (button == 2) {
        teapotZoom += deltaY / 15;

    }
}
