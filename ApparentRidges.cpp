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
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// UI
float xDegrees = 0.0f;
float yDegrees = 0.0f;
float modelSize = 1.0f;
float lightDegrees = 0.0f;
float PDLengthScale = 1.0f;
glm::vec3 background(30.0 / 255, 30.0 / 255, 30.0 / 255);
glm::vec3 lineColor(1.0f);

bool ridgesOn = true;
bool PDsOn = false;
bool drawFaded = false;
bool apparentCullFaces = false;
bool transparent = false;
int main()
{
    int lineWidth = 1;
    float thresholdScale = 1.0f;


    // glfw: initialize and configure
    // MIND THE VERSION!!!
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

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
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    //IMGui init    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    //Load Models
    std::vector<Model> models;
    //order causes no bugs
    //models.push_back(Model(".\\models\\Zagato.obj"));
    models.push_back(Model(".\\models\\stanford-bunny.obj"));
    models.push_back(Model(".\\models\\lucy.obj"));
    models.push_back(Model(".\\models\\rapid.obj"));
    models.push_back(Model(".\\models\\xyzrgb_dragon.obj"));
    models.push_back(Model(".\\models\\chips.obj"));
    /*
    */
    

    Model* currentModel = &models[0];

    //Load Shaders
    GLuint diffuse = loadShader(".\\shaders\\diffuse.vs", ".\\shaders\\diffuse.fs");
    GLuint base = loadShader(".\\shaders\\base.vs", ".\\shaders\\base.fs");
    GLuint apparentRidges = loadShader(".\\shaders\\apparentRidges.vs", ".\\shaders\\apparentRidges.fs",".\\shaders\\apparentRidges.gs");
    //GLuint apparentRidges = diffuse;
    GLuint maxPDShader = loadShader(".\\shaders\\PDmax.vs",".\\shaders\\PDmax.fs",".\\shaders\\PDmax.gs");
    GLuint minPDShader = loadShader(".\\shaders\\PDmin.vs",".\\shaders\\PDmin.fs",".\\shaders\\PDmin.gs");



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
        ImGui::Checkbox("Line Drawing with Apparent Ridges", &ridgesOn);
        ImGui::Checkbox("Principal Directions", &PDsOn);
        ImGui::Checkbox("Draw Faded Lines", &drawFaded);
        ImGui::Checkbox("Cull Apparent Ridges", &apparentCullFaces);
        ImGui::Checkbox("Transparent", &apparentCullFaces);
        const char* listboxItems[] = { "Bunny", "Lucy", "David", "Dragon", "Chips"};
        static int currentlistboxItem = 0;
        ImGui::ListBox("Model", &currentlistboxItem, listboxItems, IM_ARRAYSIZE(listboxItems), 3);
        currentModel = &models[currentlistboxItem];

        ImGui::SliderFloat("Rotate X", &xDegrees, 0.0f, 360.0f);
        ImGui::SliderFloat("Rotate Y", &yDegrees, 0.0f, 360.0f);
        ImGui::SliderFloat("Model Size", &modelSize, 0.1f, 15.0f);
        ImGui::SliderInt("Line Width", &lineWidth, 1, 10);
        ImGui::SliderFloat("Threshold", &thresholdScale, 0.0f, 10.0f);
        ImGui::SliderFloat("Principal Directions Arrow Length", &PDLengthScale, 0.0f, 1.0f);
        ImGuiColorEditFlags misc_flags = (0 | ImGuiColorEditFlags_NoDragDrop | 0 | ImGuiColorEditFlags_NoOptions);
        ImGui::ColorEdit3("Line Color", (float*)&lineColor, misc_flags);
        ImGui::ColorEdit3("Background Color", (float*)&background, misc_flags);
        //ImGui::SliderFloat("Rotate Global Light Source", &lightDegrees, 0.0f, 360.0f);   
        //ImGui::SliderFloat("Brightness", &diffuse, 0.0f, 2.0f);
        ImGui::End();


        if (ridgesOn) { currentShader = &apparentRidges; currentModel->apparentRidges = true; }
        else { currentShader = &diffuse; currentModel->apparentRidges = false;}

        glUseProgram(*currentShader);

        //Uniforms
        glm::mat4 lightRotate = glm::rotate(glm::mat4(1), glm::radians(lightDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
        lightPos = glm::vec3(lightRotate * glm::vec4(lightPosInit, 0.0f));

        glUniform3f(glGetUniformLocation(*currentShader, "light.position"), lightPos.x, lightPos.y, lightPos.z);
        //glUniform3f(glGetUniformLocation(*currentShader, "light.diffuse"), lightDiffuse.x, lightDiffuse.y, lightDiffuse.z);
        
        //opengl matrice transforms are applied from the right side. (last first)
        glm::mat4 model = glm::mat4(1);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -1.0f));
        model = glm::scale(model, glm::vec3(modelSize, modelSize, modelSize));
        model = glm::rotate(model, glm::radians(yDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(xDegrees), glm::vec3(1.0f, 0.0f, 0.0f));

        model = glm::scale(model, glm::vec3(currentModel->modelScaleFactor) );
        model = glm::translate(model, (-1.0f * currentModel->center));
        glm::mat4 view = glm::lookAt(cameraPos, viewDir, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);


        glUniformMatrix4fv(glGetUniformLocation(*currentShader, "model"), 1, GL_FALSE, &model[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(*currentShader, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(*currentShader, "projection"), 1, GL_FALSE, &projection[0][0]);
        glUniform3f(glGetUniformLocation(*currentShader, "viewPosition"), cameraPos.x, cameraPos.y, cameraPos.z);
        if (ridgesOn) {
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
            glUniform3f(glGetUniformLocation(currentModel->viewDepCurvatureCompute, "viewPosition"), cameraPos.x, cameraPos.y, cameraPos.z);
            
            glUseProgram(currentModel->Dt1q1Compute);
            glUniform3f(glGetUniformLocation(currentModel->Dt1q1Compute, "viewPosition"), cameraPos.x, cameraPos.y, cameraPos.z);
            
            glUseProgram(apparentRidges);
            //threshold is scaled to the reciprocal of feature size
            if(currentModel->minDistance>1.0f)
                glUniform1f(glGetUniformLocation(apparentRidges,"threshold"),thresholdScale/(currentModel->minDistance* currentModel->minDistance));
            else
                glUniform1f(glGetUniformLocation(apparentRidges, "threshold"), thresholdScale * currentModel->minDistance);

            glUniform3f(glGetUniformLocation(apparentRidges,"lineColor"), lineColor.x,lineColor.y,lineColor.z);
            glUniform3f(glGetUniformLocation(apparentRidges, "backgroundColor"), background.x, background.y, background.z);
            glUniform1i(glGetUniformLocation(apparentRidges, "drawFaded"), drawFaded);
            glUniform1i(glGetUniformLocation(apparentRidges, "cull"), apparentCullFaces);
        }
        if (PDsOn) {
            //Render Principal Directions
            glUseProgram(maxPDShader);
            glUniform1f(glGetUniformLocation(maxPDShader, "magnitude"), 0.02f* PDLengthScale * currentModel->modelScaleFactor * modelSize);
            glUniformMatrix4fv(glGetUniformLocation(maxPDShader, "model"), 1, GL_FALSE, &model[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(maxPDShader, "view"), 1, GL_FALSE, &view[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(maxPDShader, "projection"), 1, GL_FALSE, &projection[0][0]);
            glUniform1ui(glGetUniformLocation(maxPDShader, "size"), currentModel->vertices.size());
            
            currentModel->render(maxPDShader);

            glUseProgram(minPDShader);
            glUniform1f(glGetUniformLocation(minPDShader, "magnitude"), 0.02f* PDLengthScale * currentModel->modelScaleFactor * modelSize);
            glUniformMatrix4fv(glGetUniformLocation(minPDShader, "model"), 1, GL_FALSE, &model[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(minPDShader, "view"), 1, GL_FALSE, &view[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(minPDShader, "projection"), 1, GL_FALSE, &projection[0][0]);
            glUniform1ui(glGetUniformLocation(minPDShader, "size"), currentModel->vertices.size());
            
            currentModel->render(minPDShader);

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


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
