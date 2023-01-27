#ifndef _COMPUTESHADER_H_
#define _COMPUTESHADER_H_

#include <glad/glad.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
using std::cout;
GLuint loadComputeShader(const GLchar* computeShaderPath){
    GLuint program;
    std::string code;
    std::ifstream file;
    //read code
    file.exceptions(std::ifstream::badbit);
    try {
        file.open(computeShaderPath);
        std::stringstream shaderStream;
        //stream from file
        shaderStream << file.rdbuf();
        file.close();
        code = shaderStream.str(); //to string
    }
    catch (std::ifstream::failure e) {
        std::cout << "Compute shader failed to read!\n";
    }
    const GLchar* codeCString = code.c_str(); //string to c string

    //gl shader creation & compilation 
    GLint computeShader; //handle
    GLint success;
    GLchar log[512];
    
    computeShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(computeShader,1,&codeCString,NULL);
    glCompileShader(computeShader);
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
    if (!success){
        glGetShaderInfoLog(computeShader, 512, NULL, log);
        std::cout << "Compute shader : "<< computeShaderPath << " compilation failed.\n" << log << "\n";
    }
    program = glCreateProgram();
    glAttachShader(program, computeShader);
	glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    //If we're not linking between anything can this even fail? A compute shader is independent.
    if (!success) { 
        glGetProgramInfoLog(program, 512, NULL, log);
        std::cout << "Shader program "<< computeShaderPath<<" linking failed.\n" << log << "\n";
    }

    return program;
}

//GL constants -> MUST BE DONE AFTER LOADING GL POINTERS WITH GLAD / GLEW 
GLint getMaxComputeWorkGroupCount(int i){
    if(i>2 || i<0) return 0;
    GLint size;
    //x,y,z -> 0 1 2
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, i, &size);
    return size;
}
GLint getMaxComputeWorkGroupSize(int i){
    if(i>2 || i<0) return 0;
    GLint size;
    //x,y,z -> 0 1 2
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, i, &size);
    return size;
}
GLint getMaxComputeWorkGroupInvocations(){
    GLint size;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &size);
    return size;
}
GLint getMaxComputeSharedMemorySize(){
    GLint size;
    glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &size);
    return size;
}
// Max bindings (This should probably not be in this header but...)
GLint getMaxShaderStorageBufferBindings(){
    //usually 90 or so ?
    GLint size;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &size);
    return size;
}
GLint getMaxUniformBlockSize() {
    GLint size;
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &size);
    return size;
}
GLint getMaxShaderStorageBlockSize() {
    GLint size;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &size);
    return size;
}
void printComputeShaderSizes() {
    cout << "Max Work Group count : " << (int)getMaxComputeWorkGroupCount(0) << ", " << (int)getMaxComputeWorkGroupCount(1) << ", " << (int)getMaxComputeWorkGroupCount(2) << ".\n";
    cout << "Max Work Group local size : " << (int)getMaxComputeWorkGroupSize(0) << ", " << (int)getMaxComputeWorkGroupSize(1) << ", " << (int)getMaxComputeWorkGroupSize(2) << ".\n";
    cout << "Max compute shader invocations : " << (int)getMaxComputeWorkGroupInvocations() << "\n";
    cout << "Max compute shader shared memory size : " << (int)getMaxComputeSharedMemorySize() << "\n";
    cout << "Max SSBO size : " << (int)getMaxShaderStorageBlockSize() << "\n";
    cout << "Max SSBO bindings : " << (int)getMaxShaderStorageBufferBindings() << "\n";
    cout << "Max UBO size : " << (int)getMaxUniformBlockSize() << "\n";
}




#endif