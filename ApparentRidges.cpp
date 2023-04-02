#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>

#include "LoadShader.h"
#include "Model.h"
// settings
const unsigned int SCR_WIDTH = 2400;
const unsigned int SCR_HEIGHT = 1350;
void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// UI
glm::mat4 mouseRotation{ 1.0f }; //identity
glm::mat4 scrollModel{ 1.0f };
float xDegrees = 0.0f;
float yDegrees = 0.0f;
float modelSize = 1.0f;
float lightDegrees = 0.0f;
float PDLengthScale = 0.1f;
glm::vec3 background(30.0 / 255, 30.0 / 255, 30.0 / 255);
glm::vec3 lineColor(1.0f);

bool PDsOn = false;
bool viewDepPDOn = false;
bool drawFaded = true;
bool apparentCullFaces = false;
bool transparent = false;
int main()
{
    float lineWidth = 2.5;
    float thresholdScale = 1.0f;


    // glfw: initialize and configure
    // MIND THE VERSION!!!
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif


    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Apparent Ridges", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    //GLFW callbacks
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scroll_callback);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    //Print constants, must be done after loading GL pointers.
    printComputeShaderSizes();

    // configure global opengl state
    //z buffer
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    /*
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    */

    /*
    */
    //This is DEPRECATED and does NOTHING!
    glEnable(GL_BLEND); //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

    //IMGui init    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    /*
    glEnable(GL_MULTISAMPLE);
    */


    //Load Models
    std::vector<Model> models;
    //order causes no bugs

    //models.push_back(Model(".\\models\\quadDavid.obj"));
    models.push_back(Model(".\\models\\stanford-bunny.obj"));
    /*
    models.push_back(Model(".\\models\\max-planck.obj"));
    models.push_back(Model(".\\models\\lucy.obj"));
    models.push_back(Model(".\\models\\rapid.obj"));
    models.push_back(Model(".\\models\\brain.obj"));
    models.push_back(Model(".\\models\\Nefertiti.obj"));
    models.push_back(Model(".\\models\\cow.obj"));
    models.push_back(Model(".\\models\\Zagato.obj"));
    */
   
    models.push_back(Model(".\\models\\torus.obj"));
    models.push_back(Model(".\\models\\knot.obj"));
    models.push_back(Model(".\\models\\horse.obj"));

    Model* currentModel = &models[0];

    //Load Shaders
    GLuint diffuse = loadShader(".\\shaders\\diffuse.vert", ".\\shaders\\diffuse.frag");
    GLuint base = loadShader(".\\shaders\\base.vert", ".\\shaders\\base.frag");
    GLuint apparentRidges = loadShader(".\\shaders\\apparentRidges.vert", ".\\shaders\\apparentRidges.frag",".\\shaders\\apparentRidges.geom");
    //GLuint apparentRidges = diffuse;
    GLuint maxPDShader = loadShader(".\\shaders\\PDmax.vert",".\\shaders\\PDmax.frag",".\\shaders\\PDmax.geom");
    GLuint minPDShader = loadShader(".\\shaders\\PDmin.vert",".\\shaders\\PDmin.frag",".\\shaders\\PDmin.geom");
    GLuint curvHeat = loadShader(".\\shaders\\curvatureHeat.vert", ".\\shaders\\curvatureHeat.frag");
    GLuint visualizeViewDep = loadShader(".\\shaders\\visualizeViewDep.vert", ".\\shaders\\visualizeViewDep.frag", ".\\shaders\\visualizeViewDep.geom");



    //GLuint* currentShader = &apparantRidges;
    GLuint* currentShader = &diffuse;

    //view
    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 1.0f);
    
    glm::vec3 viewDir = glm::vec3(0.0f, 0.0f, -1.0f);
    //light settings
    glm::vec3 lightPosInit = glm::vec3(-1.0f, 1.0f, 1.5f);
    glm::vec3 lightPos = lightPosInit;
    glm::vec3 lightDiffuse = glm::vec3(1, 1, 1);
    glm::vec3 lightSpecular = glm::vec3(1, 1, 1);


    int renderModeSelect = 0;
    //render loop
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        //yDegrees += 1;
        //yDegrees =int(yDegrees)%360;

        glClearColor(background.x, background.y, background.z, 0.0); //background

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glLineWidth(lineWidth);
        //IMGui new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        //IMGui window
        ImGui::Begin("Apparent Ridges");

        ImGui::RadioButton("Line Drawing with Apparent Ridges", &renderModeSelect, 0);
        ImGui::RadioButton("Curvature Heatmap", &renderModeSelect, 1);
        ImGui::RadioButton("Diffuse", &renderModeSelect, 2);
        ImGui::RadioButton("View Dependent Curvature", &renderModeSelect, 3);

        ImGui::Checkbox("Principal Directions", &PDsOn);
        //ImGui::Checkbox("View Dependent Curvature Direction", &viewDepPDOn);
        ImGui::Checkbox("Draw Faded Lines", &drawFaded);
        ImGui::Checkbox("Cull Apparent Ridges", &apparentCullFaces);
        ImGui::Checkbox("Transparent", &transparent);
        
        ImGui::Text("Click + drag to move model. Ctrl + click + drag to move light source. Scroll to zoom.");
        const char* listboxItems[] = { "Bunny", "Planck","Lucy", "David", "Brain", "Nefertiti","Cow","Torus","Knot","Tire" };
        static int currentlistboxItem = 0;
        ImGui::ListBox("Model", &currentlistboxItem, listboxItems, IM_ARRAYSIZE(listboxItems), 3);
        currentModel = &models[currentlistboxItem];

        //ImGui::SliderFloat("Rotate X", &xDegrees, 0.0f, 360.0f);
        //ImGui::SliderFloat("Rotate Y", &yDegrees, 0.0f, 360.0f);
        ImGui::SliderFloat("Model Size", &modelSize, 0.1f, 15.0f);
        ImGui::SliderFloat("Line Width", &lineWidth, 0.1f, 10.0f);
        ImGui::SliderFloat("Threshold", &thresholdScale, 0.0f, 5.0f);
        ImGui::SliderFloat("Principal Directions Arrow Length", &PDLengthScale, 0.0f, 0.4f);
        ImGuiColorEditFlags misc_flags = (0 | ImGuiColorEditFlags_NoDragDrop | 0 | ImGuiColorEditFlags_NoOptions);
        ImGui::ColorEdit3("Line Color", (float*)&lineColor, misc_flags);
        ImGui::ColorEdit3("Background Color", (float*)&background, misc_flags);
        //ImGui::SliderFloat("Rotate Global Light Source", &lightDegrees, 0.0f, 360.0f);   
        //ImGui::SliderFloat("Brightness", &diffuse, 0.0f, 2.0f);
        ImGui::End();


        if (renderModeSelect == 0) { currentShader = &apparentRidges; currentModel->apparentRidges = true; }
        else if(renderModeSelect == 1) { currentShader = &curvHeat; currentModel->apparentRidges = false; }
        else if(renderModeSelect == 2) { currentShader = &diffuse; currentModel->apparentRidges = false;}
		else if(renderModeSelect == 3) { currentShader = &visualizeViewDep; currentModel->apparentRidges = true; }
        

        glUseProgram(*currentShader);

        //Uniforms
        glm::mat4 lightRotate = glm::rotate(glm::mat4(1), glm::radians(lightDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
        lightPos = glm::vec3(lightRotate * glm::vec4(lightPosInit, 0.0f));

        glUniform3f(glGetUniformLocation(*currentShader, "light.position"), lightPos.x, lightPos.y, lightPos.z);
        //glUniform3f(glGetUniformLocation(*currentShader, "light.diffuse"), lightDiffuse.x, lightDiffuse.y, lightDiffuse.z);
        
        //opengl matrice transforms are applied from the right side first. (last first)
        glm::mat4 model = glm::mat4(1);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -1.0f)); //move in world space
        model *= scrollModel ; //scale
        model = glm::scale(model, glm::vec3(currentModel->modelScaleFactor) );
        model = model*mouseRotation ;
        model = glm::translate(model, (-1.0f * currentModel->center)); //bring to origin
        glm::mat4 view = glm::lookAt(cameraPos, viewDir, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        currentModel->modelMatrix = model;
        currentModel->viewPos = cameraPos;

        glUniformMatrix4fv(glGetUniformLocation(*currentShader, "model"), 1, GL_FALSE, &model[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(*currentShader, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(*currentShader, "projection"), 1, GL_FALSE, &projection[0][0]);
        glUniform3f(glGetUniformLocation(*currentShader, "viewPosition"), cameraPos.x, cameraPos.y, cameraPos.z);
        if (renderModeSelect==0 || renderModeSelect ==3) {
            if (!transparent) {
            //render base model
            glUseProgram(base);
            glUniformMatrix4fv(glGetUniformLocation(base, "model"), 1, GL_FALSE, &model[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(base, "view"), 1, GL_FALSE, &view[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(base, "projection"), 1, GL_FALSE, &projection[0][0]);
            glUniform3f(glGetUniformLocation(base, "backgroundColor"), background.x, background.y, background.z);
            currentModel->render(base);
            }

            //render apparent ridges
            currentModel->apparentRidges = true;
            glUseProgram(currentModel->viewDepCurvatureCompute);
            //glUniform3f(glGetUniformLocation(currentModel->viewDepCurvatureCompute, "viewPosition"), cameraPos.x, cameraPos.y, cameraPos.z);
            //glUniformMatrix4fv(glGetUniformLocation(currentModel->viewDepCurvatureCompute, "model"), 1, GL_FALSE, &model[0][0]);
            
            glUseProgram(currentModel->Dt1q1Compute);
            glUniform3f(glGetUniformLocation(currentModel->Dt1q1Compute, "viewPosition"), cameraPos.x, cameraPos.y, cameraPos.z);
            //glUniformMatrix4fv(glGetUniformLocation(currentModel->Dt1q1Compute, "model"), 1, GL_FALSE, &model[0][0]);
            
            glUseProgram(*currentShader);
            //threshold is scaled to the reciprocal of feature size
            
            //if (currentModel->minDistance>1.0f)
                //glUniform1f(glGetUniformLocation(apparentRidges,"threshold"),0.2f*thresholdScale/(currentModel->minDistance));
                glUniform1f(glGetUniformLocation(*currentShader, "threshold"), 0.02f * thresholdScale / (currentModel->minDistance * currentModel->minDistance));

                //glUniform1f(glGetUniformLocation(apparentRidges, "threshold"), std::numeric_limits<float>::lowest());
            //else
              //  glUniform1f(glGetUniformLocation(apparentRidges, "threshold"), 3.0f * thresholdScale * currentModel->minDistance);

            glUniform3f(glGetUniformLocation(*currentShader,"lineColor"), lineColor.x,lineColor.y,lineColor.z);
            glUniform3f(glGetUniformLocation(*currentShader, "backgroundColor"), background.x, background.y, background.z);
            glUniform1i(glGetUniformLocation(*currentShader, "drawFaded"), drawFaded);
            glUniform1i(glGetUniformLocation(*currentShader, "cull"), apparentCullFaces);

            currentModel->apparentRidges = false;
        }

        if (PDsOn) {
            glLineWidth(2);
            //Render Principal Directions
            glUseProgram(maxPDShader);
            glUniform1f(glGetUniformLocation(maxPDShader, "magnitude"), 0.01f * PDLengthScale  * scrollModel[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(maxPDShader, "model"), 1, GL_FALSE, &model[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(maxPDShader, "view"), 1, GL_FALSE, &view[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(maxPDShader, "projection"), 1, GL_FALSE, &projection[0][0]);
            glUniform1ui(glGetUniformLocation(maxPDShader, "size"), currentModel->vertices.size());
            
            currentModel->render(maxPDShader);

            glUseProgram(minPDShader);
            glUniform1f(glGetUniformLocation(minPDShader, "magnitude"), 0.01f * PDLengthScale  * scrollModel[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(minPDShader, "model"), 1, GL_FALSE, &model[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(minPDShader, "view"), 1, GL_FALSE, &view[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(minPDShader, "projection"), 1, GL_FALSE, &projection[0][0]);
            glUniform1ui(glGetUniformLocation(minPDShader, "size"), currentModel->vertices.size());
            
            currentModel->render(minPDShader);
            glLineWidth(lineWidth);
        }
        if (viewDepPDOn) {
        /*
            glUseProgram(viewDepPDShader);
            glUniform1f(glGetUniformLocation(viewDepPDShader, "magnitude"), 0.01f * PDLengthScale * scrollModel[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(viewDepPDShader, "model"), 1, GL_FALSE, &model[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(viewDepPDShader, "view"), 1, GL_FALSE, &view[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(viewDepPDShader, "projection"), 1, GL_FALSE, &projection[0][0]);
            glUniform1ui(glGetUniformLocation(viewDepPDShader, "size"), currentModel->vertices.size());

            currentModel->render(viewDepPDShader);
        */
        }

        currentModel->render(*currentShader);

        glUseProgram(0);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Delete ImGUI instances
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}


void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
float xVelocity = 0.0f;
float yVelocity = 0.0f;
const float friction = 0.95f;
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    //return if using IMGUI
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }
    //static variables persists across all function calls and doesn't go out of range.
    static double last_xpos = xpos;
    static double last_ypos = ypos;
    double dx = xpos - last_xpos;
    double dy = ypos - last_ypos;
    last_xpos = xpos;
    last_ypos = ypos;

    glm::vec3 cameraRight = glm::vec3(1.0f, 0.0f, 0.0f); 
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    
    int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (state == GLFW_PRESS)
    {
        xVelocity = 0.0025f * static_cast<float>(dx);
        yVelocity = 0.0025f * static_cast<float>(dy);
        //rotation
        /*
        glm::quat rotY = glm::angleAxis(0.0025f * static_cast<float>(dy), cameraRight);
        glm::quat rotX = glm::angleAxis(0.0025f * static_cast<float>(dx), cameraUp);
        glm::quat rotation = rotX * rotY;

        //!!! mouseRotation *= glm::mat4_cast(rotation) ; -> mouseRotation = mouseRotation*glm::mat4_cast(rotation); which will cause rotations to apply in model space!!!
        mouseRotation = glm::mat4_cast(rotation) * mouseRotation;
        */
    }
    xVelocity *= friction;
    yVelocity *= friction;

    if (std::abs(xVelocity) > 0.001f || std::abs(yVelocity) > 0.001f) {
        //glm::quat rotY = glm::angleAxis(xVelocity, cameraRight);
        glm::quat rotY = glm::angleAxis(yVelocity, cameraRight);
        //glm::quat rotX = glm::angleAxis(yVelocity, cameraUp);
        glm::quat rotX = glm::angleAxis(xVelocity, cameraUp);
        glm::quat rotation = rotX * rotY;

        mouseRotation = glm::mat4_cast(rotation) * mouseRotation;
    }

    //Next time I'll add a camera class.
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }
    float scale_factor = 1.0f + yoffset * 0.1f;
    scrollModel = glm::scale(scrollModel, glm::vec3(scale_factor));

};