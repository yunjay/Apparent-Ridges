#include<memory>
#include <vector>
#include <array>
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <unordered_map>
#include <functional>

#include <omp.h>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <assimp/Importer.hpp>      
#include <assimp/scene.h>          
#include <assimp/postprocess.h>     

#include "LoadShader.h"
using glm::vec3; using glm::vec4;
using glm::dot; using glm::cross; using glm::normalize; using glm::sign;
const unsigned int workGroupSize = 1024;
bool loadAssimp(const char* path, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec3>& out_normals, std::vector<unsigned int>& out_indices);
void printVec(glm::vec3 v) {
	std::cout << "(" << v.x << ", " << v.y << ", " << v.z << ") ";
}
void printVec(glm::vec4 v) {
	std::cout << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ") ";
}

class Model {
public:
	//Handles
	//Buffers
	GLuint VAO, positionBuffer, normalBuffer, textureBuffer, EBO;
	GLuint PDBuffer, CurvatureBuffer;
	GLuint maxPDVBO, maxCurvVBO, minPDVBO, minCurvVBO, curvColorVBO, worldt1VBO;
	GLuint vertexStorageBuffer, normalStorageBuffer, indexStorageBuffer;
	//q1 : max view-dep curvature, t1 : max view-dep curvature direction
	//Dt1q1 : max view-dependent curvature's directional derivative in direction t1
	GLuint adjacentFacesBuffer, q1Buffer, t1Buffer, Dt1q1Buffer;
	GLuint pointAreaBuffer, cornerAreaBuffer;
	GLuint dumpBuffer;

	//shaders
	//TODO : These should be static
	GLuint viewDepCurvatureCompute, Dt1q1Compute, pointAreaCompute;

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<std::array<unsigned int, 3>> faces;
	std::vector<glm::vec2> textureCoordinates;
	std::vector<glm::vec3> tangents;
	std::vector<glm::vec3> bitangents;
	std::vector<GLuint> indices;

	std::vector<glm::vec4> PDs;
	std::vector<GLfloat> PrincipalCurvatures;
	std::vector<std::array<int, 20>> adjacentFaces; //10 pairs of indices of adjacent edges to the vertex
	std::vector<GLint> pointAreas; //for every vertex 
	std::vector<GLfloat> cornerAreas; //for every index
	std::vector<glm::vec3> curvColors;

	std::vector<glm::vec4> maxPDs;
	std::vector<glm::vec4> minPDs;
	std::vector<GLfloat> maxCurvs;
	std::vector<GLfloat> minCurvs;

	std::vector<GLfloat> q1s;
	std::vector<glm::vec2> t1s;
	std::vector<GLfloat> Dt1q1s;
	//std::vector<GLdouble> Dt1q1s;

	std::vector<vec3> worldt1s;


	unsigned int numVertices;
	unsigned int numNormals;
	unsigned int numFaces;
	unsigned int numIndices;

	std::string path;
	GLfloat diagonalLength = 0.0f;
	GLfloat modelScaleFactor = 1.0f;
	GLfloat minDistance;
	glm::vec3 center = glm::vec3(0.0f);
	GLuint size;
	bool isSet = false;
	bool curvaturesCalculated = false;
	bool apparentRidges = false;

	//Debugging area
	glm::mat4 modelMatrix;
	glm::vec3 viewPos;
	std::vector<float> dumps;
	bool printed = false;
	bool dumped = false;

	Model(std::string path) {
		this->path = path;
		if (!this->loadAssimp()) { std::cout << "Model at " << path << " not loaded!\n"; };
		this->combineIdenticalVertices();
		this->boundingBox();
		this->minDistance = this->getMinDistance();
		this->size = this->vertices.size();
		this->findAdjacentFaces();
		this->computeCurvatures();
		this->setup();
	}
	void setup() {
		//std::cout << "Setting up buffers.\n";
		//vector.data() == &vector[0]
		glGenVertexArrays(1, &VAO); //vertex array object
		glGenBuffers(1, &positionBuffer); //vertex buffer object
		glGenBuffers(1, &normalBuffer); //vertex buffer object
		glGenBuffers(1, &textureBuffer); //vertex buffer object

		glGenBuffers(1, &EBO);

		//glGenBuffers(1, &PDBuffer);
		//glGenBuffers(1, &CurvBuffer);

		//VAO  
		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);

		//EBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);



		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
		// glVertexAttribPointer(index, size, type, normalized(bool), stride(byte offset between), pointer(offset))
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, textureBuffer);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// Principal Directions / Principal Curvatures (As VBOs)
		if (this->curvaturesCalculated) {
			glGenBuffers(1, &maxPDVBO); //3
			glGenBuffers(1, &minPDVBO); //4
			glGenBuffers(1, &maxCurvVBO); //5
			glGenBuffers(1, &minCurvVBO); //6
			glGenBuffers(1, &curvColorVBO); //7

			//Get data from SSBOs
			//void glGetNamedBufferSubData(	GLuint buffer,GLintptr offset,GLsizeiptr size,void *data);
			//buffer -> handle, offset -> start of data to read, size -> size of data to be read, *data -> pointer to return data


			for (int dbg = 0; dbg < 2; dbg++) {
				std::cout << "PrincipalCurvatures[" << dbg << "] " << PrincipalCurvatures[dbg] << "\n";
				std::cout << "PrincipalCurvatures[" << vertices.size() + dbg << "] " << PrincipalCurvatures[vertices.size() + dbg] << "\n";
			}


			//Send PDs / PCs as attrbutes per vertex, just to uncomplicate some of this process.
			glBindBuffer(GL_ARRAY_BUFFER, maxPDVBO);
			glBufferData(GL_ARRAY_BUFFER, PDs.size() / 2 * sizeof(glm::vec4), PDs.data(), GL_STATIC_DRAW);
			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, (void*)0); //Our PDs are vec4s due to SSBO reasons.

			glBindBuffer(GL_ARRAY_BUFFER, minPDVBO);
			glBufferData(GL_ARRAY_BUFFER, PDs.size() / 2 * sizeof(glm::vec4), &PDs[this->vertices.size()], GL_STATIC_DRAW);
			glEnableVertexAttribArray(4);
			glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

			glBindBuffer(GL_ARRAY_BUFFER, maxCurvVBO);
			glBufferData(GL_ARRAY_BUFFER, PrincipalCurvatures.size() / 2 * sizeof(GLfloat), PrincipalCurvatures.data(), GL_STATIC_DRAW);
			glEnableVertexAttribArray(5);
			glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);

			glBindBuffer(GL_ARRAY_BUFFER, minCurvVBO);
			glBufferData(GL_ARRAY_BUFFER, PrincipalCurvatures.size() / 2 * sizeof(GLfloat), &PrincipalCurvatures[this->vertices.size()], GL_STATIC_DRAW);
			glEnableVertexAttribArray(6);
			glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
			
			this->computeCurvColors();
			glBindBuffer(GL_ARRAY_BUFFER, curvColorVBO);
			glBufferData(GL_ARRAY_BUFFER, curvColors.size() * sizeof(glm::vec3), curvColors.data(), GL_STATIC_DRAW);
			glEnableVertexAttribArray(7);
			glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

			//glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, minCurvVBO);
			glBufferData(GL_ARRAY_BUFFER, PrincipalCurvatures.size() / 2 * sizeof(GLfloat), &PrincipalCurvatures[this->vertices.size()], GL_STATIC_DRAW);
			glEnableVertexAttribArray(6);
			glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);

		}
		else { this->computeCurvatures(); this->setup(); }

		//adjacent faces

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 20, adjacentFacesBuffer);

		q1s.resize(this->numVertices, 0.0f);
		glGenBuffers(1, &q1Buffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, q1Buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, q1s.size() * sizeof(GLfloat), q1s.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 21, q1Buffer);

		t1s.resize(this->numVertices, glm::vec2(0.0f));
		glGenBuffers(1, &t1Buffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, t1Buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, t1s.size() * sizeof(glm::vec2), t1s.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 22, t1Buffer);

		Dt1q1s.resize(this->numVertices, 0.0f);
		glGenBuffers(1, &Dt1q1Buffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, Dt1q1Buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, Dt1q1s.size() * sizeof(GLfloat), Dt1q1s.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 23, Dt1q1Buffer);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//for debugging
		dumps.resize(this->numVertices, 0.0f);
		glGenBuffers(1, &dumpBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, dumpBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, dumps.size() * sizeof(GLfloat), q1s.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 42, dumpBuffer);
		
		
		glBindVertexArray(0);

		//shaders for apparent ridges
		this->viewDepCurvatureCompute = loadComputeShader(".\\shaders\\viewDepCurv.comp");
		this->Dt1q1Compute = loadComputeShader(".\\shaders\\Dt1q1.comp");

		//std::cout << "Ready to render.\n";
		this->isSet = true;

		return;
	}
	//draw function
	bool render(GLuint shader) {

		if (!isSet) { this->setup(); }

		if (this->apparentRidges) {
			constexpr bool CPUmode = false;
			//constexpr bool CPUmode = true;
			//Rebind SSBOs
			this->rebindSSBOs();
			/*
			if (!printed) {
				std::cout << "\nFor model " << this->path << " : \n";
				std::cout << "Before View dep computation on " << this->path << " : \n";
				glGetNamedBufferSubData(PDBuffer, 0, PDs.size() * sizeof(GLfloat), PDs.data());
				std::cout << "PDs : ";
				for (int dbg = 0; dbg < 2; dbg++) {
					printVec(PDs[dbg]); std::cout << ", ";
					printVec(PDs[dbg + this->numVertices]); std::cout << ", ";
				}
				std::cout << "\n";
				glGetNamedBufferSubData(CurvatureBuffer, 0, PrincipalCurvatures.size() * sizeof(GLfloat), PrincipalCurvatures.data());
				std::cout << "PrincipalCurvatures : ";
				for (int dbg = 0; dbg < 2; dbg++) {
					std::cout << PrincipalCurvatures[dbg] << ", ";
					std::cout << PrincipalCurvatures[dbg + this->numVertices] << ", ";
				}
				std::cout << "\n";
				glGetNamedBufferSubData(this->q1Buffer, 0, q1s.size() * sizeof(GLfloat), q1s.data());
				std::cout << "q1s : ";
				for (int dbg = 0; dbg < 4; dbg++) {
					std::cout << q1s[dbg] << ", ";
				}
				std::cout << "\n";
				glGetNamedBufferSubData(t1Buffer, 0, t1s.size() * sizeof(GLfloat), t1s.data());
				std::cout << "t1s : ";
				for (int dbg = 0; dbg < 4; dbg++) {
					std::cout << "(" << t1s[dbg].x << "," << t1s[dbg].y << "), ";
				}
				std::cout << "\n";
				glGetNamedBufferSubData(Dt1q1Buffer, 0, Dt1q1s.size() * sizeof(GLfloat), Dt1q1s.data());
				std::cout << "Dt1q1s : ";
				for (int dbg = 0; dbg < 4; dbg++) {
					std::cout << Dt1q1s[dbg] << ", ";
				}
				std::cout << "\n";
			}
			*/

			glBindVertexArray(VAO);
			this->rebindSSBOs();
			//Compute View-dep curvatures (q1), and direction (t1)
			if (!CPUmode) {

			glUseProgram(viewDepCurvatureCompute);
			glUniform1ui(glGetUniformLocation(viewDepCurvatureCompute, "verticesSize"), this->numVertices);

			glUniformMatrix4fv(glGetUniformLocation(viewDepCurvatureCompute, "model"), 1, GL_FALSE, &this->modelMatrix[0][0]);
			glUniform3f(glGetUniformLocation(viewDepCurvatureCompute, "viewPosition"), viewPos.x, viewPos.y, viewPos.z);

			glDispatchCompute(glm::ceil(GLfloat(this->numVertices) / float(workGroupSize)), 1, 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			if (!printed) {
				const int maxdbg = 10;
				std::cout << "\nFor model " << this->path << " : \n";
				std::cout << "After ViewDepCurvature " << " : \n";
				glGetNamedBufferSubData(this->q1Buffer, 0, q1s.size() * sizeof(GLfloat), q1s.data());
				std::cout << "q1s : ";
				for (int dbg = 0; dbg < maxdbg; dbg++) {
					std::cout << q1s[dbg] << ", ";
				}
				std::cout << "\n";
				glGetNamedBufferSubData(t1Buffer, 0, t1s.size() * sizeof(GLfloat), t1s.data());
				std::cout << "t1s : ";
				for (int dbg = 0; dbg < maxdbg; dbg++) {
					std::cout << "(" << t1s[dbg].x << "," << t1s[dbg].y << "), ";
				}
				std::cout << "\n";
				glGetNamedBufferSubData(Dt1q1Buffer, 0, Dt1q1s.size() * sizeof(GLfloat), Dt1q1s.data());
				std::cout << "Dt1q1s : ";
				for (int dbg = 0; dbg < maxdbg; dbg++) {
					std::cout << Dt1q1s[dbg] << ", ";
				}
				std::cout << "\n";
				glGetNamedBufferSubData(dumpBuffer, 0, dumps.size() * sizeof(GLfloat), dumps.data());
				/*
				std::cout << "Dump : ";
				for (int dbg = 0; dbg < maxdbg; dbg++) {
					std::cout << dumps[dbg] << ", ";
				}
				std::cout << "\n";
				*/
			}

			//Compute View-dep curvature derivatives (Dt1q1)
			glUseProgram(Dt1q1Compute);
			glUniform1ui(glGetUniformLocation(Dt1q1Compute, "verticesSize"), this->numVertices);
			glUniformMatrix4fv(glGetUniformLocation(Dt1q1Compute, "model"), 1, GL_FALSE, &this->modelMatrix[0][0]);
			glUniform3f(glGetUniformLocation(Dt1q1Compute, "viewPosition"), viewPos.x, viewPos.y, viewPos.z);

			glDispatchCompute(glm::ceil(GLfloat(this->numVertices) / float(workGroupSize)), 1, 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			if (!printed) {
				const int maxdbg = 20;
				std::cout << "After Dt1q1 " << " : \n";
				glGetNamedBufferSubData(this->q1Buffer, 0, q1s.size() * sizeof(GLfloat), q1s.data());
				std::cout << "q1s : ";
				for (int dbg = 0; dbg < maxdbg; dbg++) {
					std::cout << q1s[dbg] << ", ";
				}
				std::cout << "\n";
				glGetNamedBufferSubData(t1Buffer, 0, t1s.size() * sizeof(GLfloat), t1s.data());
				std::cout << "t1s : ";
				for (int dbg = 0; dbg < maxdbg; dbg++) {
					std::cout << "(" << t1s[dbg].x << "," << t1s[dbg].y << "), ";
				}
				std::cout << "\n";
				glGetNamedBufferSubData(Dt1q1Buffer, 0, Dt1q1s.size() * sizeof(GLfloat), Dt1q1s.data());
				std::cout << "Dt1q1s : ";
				for (int dbg = 0; dbg < maxdbg; dbg++) {
					std::cout << Dt1q1s[dbg] << ", ";
				}
				std::cout << "\n";
				printed = true;

				glGetNamedBufferSubData(dumpBuffer, 0, dumps.size() * sizeof(GLfloat), dumps.data());
				std::cout << "Dump : ";
				for (int dbg = 0; dbg < maxdbg; dbg++) {
					std::cout << dumps[dbg] << ", ";
				}
				std::cout << "\n";
			}
			}
			if (CPUmode) {
				q1s.resize(numVertices, 0.0f);
				t1s.resize(numVertices, glm::vec2(0.0f));
#pragma omp parallel for
				for (size_t i=0; i < numVertices; i++) {
					//q1 t1
					vec3 position = vec3(modelMatrix * vec4(vertices[i], 1.0f));
					vec3 viewDir = normalize(viewPos - position);
					vec3 normal = normalize(glm::transpose(glm::inverse(glm::mat3(modelMatrix))) * normals[i]);

					vec3 maxPD = normalize(glm::transpose(glm::inverse(glm::mat3(modelMatrix))) * vec3(PDs[i]));
					vec3 minPD = normalize(glm::transpose(glm::inverse(glm::mat3(modelMatrix))) * vec3(PDs[i + numVertices]));
					float maxCurv = PrincipalCurvatures[i];
					float minCurv = PrincipalCurvatures[i + numVertices];
					float normalDotView = glm::dot(viewDir, normal);
					float u = dot(viewDir, maxPD);
					float v = dot(viewDir, minPD);
					float u2 = u * u;
					float v2 = v * v;
					float uv = u * v;
					float csc2 = 1.0f / (u2 + v2);
					float sec_min1 = 1.0f / glm::abs(normalDotView) - 1.0f;
					float Q11 = maxCurv * (1.0f + sec_min1 * u2 * csc2);
					float Q12 = maxCurv * (sec_min1 * uv * csc2);
					float Q21 = minCurv * (sec_min1 * uv * csc2);
					float Q22 = minCurv * (1.0f + sec_min1 * v2 * csc2);
					float QTQ1 = Q11 * Q11 + Q21 * Q21;
					float QTQ12 = Q11 * Q12 + Q21 * Q22;
					float QTQ2 = Q12 * Q12 + Q22 * Q22;
					float q1 = 0.5 * (QTQ1 + QTQ2);
					if (q1 > 0.0)
						q1 += sqrt(abs(QTQ12 * QTQ12 + 0.25 * (QTQ2 - QTQ1) * (QTQ2 - QTQ1)));
					else
						q1 -= sqrt(abs(QTQ12 * QTQ12 + 0.25 * (QTQ2 - QTQ1) * (QTQ2 - QTQ1)));
					glm::vec2 t1 = glm::vec2(QTQ2 - q1, -QTQ12);
					t1 = normalize(t1);
					q1s[i] = q1;
					t1s[i] = t1;
					//Dt1q1
					float ndotv = normalDotView;
					vec3 world_t1 = t1[0] * maxPD + t1[1] * minPD;
					vec3 world_t2 = normalize(cross(normal, world_t1));

					float v0_dot_t2 = dot(position, world_t2);
					float Dt1q1 = 0.0;
					int n = 0;
					for (size_t j = 0; j < 10; j++) {
						int v1id = adjacentFaces[i][2 * j];
						int v2id = adjacentFaces[i][2 * j + 1];
						if (v1id < 0 || v2id < 0) break;
						vec3 v1 = vec3(modelMatrix * vec4(vertices[v1id], 1.0));
						vec3 v2 = vec3(modelMatrix * vec4(vertices[v2id], 1.0));

						//find point p between v1 and v2 by linear interpolation
						//v0 is along t1, perp to t2
						// p = w1*v1 + w2*v2, where w2 = 1-w1
						// projections on t2 along v0, we only get a max of two
						float v1_dot_t2 = dot(v1, world_t2);
						float v2_dot_t2 = dot(v2, world_t2);
						float w1;
						if (abs(v2_dot_t2 - v1_dot_t2) > 1e-6) {
							w1 = (v2_dot_t2 - v0_dot_t2) / (v2_dot_t2 - v1_dot_t2);
						}
						else {
							w1 = (v2_dot_t2 - v0_dot_t2) / 1e-6;
						}
						if (w1 < 0.0 || w1 >= 1.0) continue;
						float w2 = 1.0 - w1;
						vec3 p = w1 * v1 + w2 * v2;
						float interpViewDepCurv = w1 * q1s[v1id] + w2 * q1s[v2id];
						float projDistance = dot((p - position), world_t1);
						projDistance *= abs(normalDotView);
						
						Dt1q1 += (interpViewDepCurv - q1) / projDistance;
						n++;
					}
					if (n != 0) Dt1q1 /= float(n);
					/*
					const float MAX_FLOAT = 3.402823466e+38;
					const float SMALLEST_FLOAT = 1.175494351e-38;
					if (abs(Dt1q1) > MAX_FLOAT / 64.0) Dt1q1 = MAX_FLOAT / 64.0 * sign(Dt1q1);
					if (abs(Dt1q1) < SMALLEST_FLOAT * 64.0) Dt1q1 = SMALLEST_FLOAT * 64.0 * sign(Dt1q1);
					*/
					Dt1q1s[i] = Dt1q1;
				}
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, q1Buffer);
				glBufferData(GL_SHADER_STORAGE_BUFFER, q1s.size() * sizeof(GLfloat), q1s.data(), GL_DYNAMIC_DRAW);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, t1Buffer);
				glBufferData(GL_SHADER_STORAGE_BUFFER, t1s.size() * sizeof(glm::vec2), t1s.data(), GL_DYNAMIC_DRAW);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, Dt1q1Buffer);
				glBufferData(GL_SHADER_STORAGE_BUFFER, Dt1q1s.size() * sizeof(GLfloat), Dt1q1s.data(), GL_DYNAMIC_DRAW);

			}
		}
		glBindVertexArray(VAO);
		glUseProgram(shader);

		//glDrawElements(GL_LINES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);

		glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
		
		/*
		dumped = false;
		if (!dumped) {
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			glGetNamedBufferSubData(dumpBuffer, 0, dumps.size() * sizeof(GLfloat), dumps.data());
			std::cout << "AppRidges Dump: ";
			for (int dbg = 0; dbg < 10; dbg++) {
				std::cout << dumps[dbg] << ", ";
			}
			std::cout << "\n";
			dumped = true;
		}
		*/
		//glDisableVertexAttribArray(0);
		//glDisableVertexAttribArray(1);
		//glBindVertexArray(0);

		return true;
	}
	void boundingBox() {
		//simple implemetation calculating model boundary box size
		float maxX = vertices[0].x, maxY = vertices[0].y, maxZ = vertices[0].z;
		float minX = vertices[0].x, minY = vertices[0].y, minZ = vertices[0].z;

		for (int i = 1; i < vertices.size(); i++) {
			(vertices[i].x > maxX) ? maxX = vertices[i].x : 0;
			(vertices[i].y > maxY) ? maxY = vertices[i].y : 0;
			(vertices[i].z > maxZ) ? maxZ = vertices[i].z : 0;
			(vertices[i].x < minX) ? minX = vertices[i].x : 0;
			(vertices[i].y < minY) ? minY = vertices[i].y : 0;
			(vertices[i].z < minZ) ? minZ = vertices[i].z : 0;
		}
		//center of model
		this->center = glm::vec3((maxX + minX) / 2.0f, (maxY + minY) / 2.0f, (maxZ + minZ) / 2.0f);
		this->diagonalLength = glm::length(glm::vec3(maxX - minX, maxY - minY, maxZ - minZ));
		this->modelScaleFactor = 1.0f / diagonalLength;
	}

	//Calculates principal curvatures and principal directions per vertex
	//Computed per face -> per vertex in two steps
	void computeCurvatures() {
		//generate ssbos to use in compute shader
		//we need to calculate 2 principal directions and 2 principal curvatures per vertex.
		//NOTE : SSBOs do not work well with vec3s. They take them as vec4 anyway or sth. Use vec4.
		constexpr bool isCPUMode = true;
		//constexpr bool isCPUMode = false;

		// setup SSBOs
		//  &[0] -> .data() 
		//gen -> bind (set to GL state) -> bufferData
		//for writing
		glGenBuffers(1, &PDBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, PDBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, PDs.size() * sizeof(glm::vec4), PDs.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, PDBuffer);

		glGenBuffers(1, &CurvatureBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, CurvatureBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, PrincipalCurvatures.size() * sizeof(GLfloat), PrincipalCurvatures.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, CurvatureBuffer);

		//for reading
		//Vertex positions
		glGenBuffers(1, &vertexStorageBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexStorageBuffer);
		std::vector<glm::vec4> vertexStorage;
		for (auto v : vertices) { //make vec4s from vec3s
			vertexStorage.push_back(glm::vec4(v, 1.0f));
		}
		glBufferData(GL_SHADER_STORAGE_BUFFER, vertexStorage.size() * sizeof(glm::vec4), vertexStorage.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, vertexStorageBuffer);

		//normals
		glGenBuffers(1, &normalStorageBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, normalStorageBuffer);
		std::vector<glm::vec4> normalStorage;
		for (auto v : normals) {
			normalStorage.push_back(glm::vec4(v, 0.0f));
		}
		glBufferData(GL_SHADER_STORAGE_BUFFER, normalStorage.size() * sizeof(glm::vec4), normalStorage.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, normalStorageBuffer);

		glGenBuffers(1, &indexStorageBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexStorageBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, indexStorageBuffer);

		PDs.resize(vertices.size() * 2, glm::vec4(0.0));
		PrincipalCurvatures.resize(vertices.size() * 2, 0.0f);
		if (!isCPUMode) {

		auto start = std::chrono::high_resolution_clock::now();


		//So we need to initially compute by face.
		//Load compute shader
		GLuint perFace = loadComputeShader(".\\shaders\\curvature_perFace.compute");
		GLuint perVertex = loadComputeShader(".\\shaders\\curvature_perVertex.compute");

		//resize vertices for curvature values resize(num,value)

		//Curvature tensor elements for mid use
		/*
		std::vector<GLfloat> curv1s, curv2s, curv12s;
		curv1s.resize(vertices.size(),0.0f); curv2s.resize(vertices.size(), 0.0f); curv12s.resize(vertices.size(), 0.0f);

		GLuint curv1Buffer, curv2Buffer, curv12Buffer;
		glGenBuffers(1, &curv1Buffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, curv1Buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, curv1s.size() * sizeof(GLfloat), curv1s.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, curv1Buffer);

		glGenBuffers(1, &curv2Buffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, curv2Buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, curv2s.size() * sizeof(GLfloat), curv2s.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, curv2Buffer); glGenBuffers(1, &curv2Buffer);

		glGenBuffers(1, &curv12Buffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, curv12Buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, curv12s.size() * sizeof(GLfloat), curv12s.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 14, curv12Buffer);
		*/

		GLuint curv1intBuffer, curv2intBuffer, curv12intBuffer;
		std::vector<GLint> curv1ints, curv2ints, curv12ints;
		curv1ints.resize(vertices.size(), GLint{ 0 }); curv2ints.resize(vertices.size(), GLint{ 0 }); curv12ints.resize(vertices.size(), GLint{ 0 });

		glGenBuffers(1, &curv1intBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, curv1intBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, curv1ints.size() * sizeof(GLint), curv1ints.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, curv1intBuffer);

		glGenBuffers(1, &curv2intBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, curv2intBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, curv2ints.size() * sizeof(GLint), curv2ints.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 16, curv2intBuffer);

		glGenBuffers(1, &curv12intBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, curv12intBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, curv12ints.size() * sizeof(GLint), curv12ints.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 17, curv12intBuffer);

		//Compute point areas
		this->computePointAreas();

		//Use first compute shader for curvature computation
		glUseProgram(perFace);

		//hmm.. we move by indices so send indicies size.
		glUniform1ui(glGetUniformLocation(perFace, "indicesSize"), this->numIndices);
		glUniform1ui(glGetUniformLocation(perFace, "verticesSize"), this->numVertices);

		//Dispatch -> run compute shader in GPU 
		//As we have workGroupSize invocations per work group, to run per face :
		glDispatchCompute(glm::ceil((GLfloat(this->numIndices) / 3.0f) / float(workGroupSize)), 1, 1);

		//Barrier to ensure coherency
		//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		/*
		*/
		glGetNamedBufferSubData(PDBuffer, 0, PDs.size() * sizeof(glm::vec4), PDs.data());
		for (int dbg = 0; dbg < 2; dbg++) {
			std::cout << "PDs[" << dbg << "] "; printVec(PDs[dbg]); std::cout << "\n";
			std::cout << "PDs[" << vertices.size() + dbg << "] "; printVec(PDs[vertices.size()]); std::cout << "\n";
			std::cout << "PDs[" << PDs.size() - dbg - 1 << "] "; printVec(PDs[PDs.size() - dbg - 1]); std::cout << "\n";
		}
		//Now run per vertex
		glUseProgram(perVertex);

		glUniform1ui(glGetUniformLocation(perVertex, "indicesSize"), this->numIndices);
		glUniform1ui(glGetUniformLocation(perVertex, "verticesSize"), this->numVertices);

		//Dispatch -> run compute shader in GPU 
		//As we have workGroupSize invocations per work group, to run per vertex :
		glDispatchCompute(glm::ceil(GLfloat(this->numVertices) / float(workGroupSize)), 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		/*
		*/
		glGetNamedBufferSubData(PDBuffer, 0, PDs.size() * sizeof(glm::vec4), PDs.data());
		/*
		*/
		std::cout << "PDs after per vertex compute : " << "\n";
		for (int dbg = 0; dbg < 2; dbg++) {
			std::cout << "PDs[" << dbg << "] "; printVec(PDs[dbg]); std::cout << "\n";
			std::cout << "PDs[" << vertices.size() + dbg << "] "; printVec(PDs[vertices.size()]); std::cout << "\n";
			std::cout << "PDs[" << PDs.size() - dbg - 1 << "] "; printVec(PDs[PDs.size() - dbg - 1]); std::cout << "\n";
		}

		//is ever CUDA necessary?

		this->curvaturesCalculated = true;

		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - start;
		std::cout << "Curvatures for " << this->path << " calculated on compute shader. Took " << elapsed_seconds.count() << " seconds.\n";
		glGetNamedBufferSubData(PDBuffer, 0, PDs.size() * sizeof(glm::vec4), PDs.data());
		glGetNamedBufferSubData(CurvatureBuffer, 0, PrincipalCurvatures.size() * sizeof(GLfloat), PrincipalCurvatures.data());

		}
		else if constexpr (isCPUMode) {
			this->computePointAreas();
			auto start = std::chrono::high_resolution_clock::now();
			std::vector<float> curv1s, curv2s, curv12s;
			std::vector<vec3> pd1, pd2;
			curv1s.resize(this->numVertices, 0.0f); curv2s.resize(this->numVertices, 0.0f); curv12s.resize(this->numVertices, 0.0f);
			pd1.resize(this->numVertices, vec3(0.0f)); pd2.resize(this->numVertices, vec3(0.0f));

			//Initial local coordinates
#pragma omp parallel for
			for (size_t face = 0; face < this->numFaces; face++) {
				int faceID = 3 * face;
				int vertexIds[3] = {
					indices[faceID],
					indices[faceID + 1],
					indices[faceID + 2]
				};
				pd1[vertexIds[0]] = vertices[vertexIds[1]] - vertices[vertexIds[0]];
				pd1[vertexIds[1]] = vertices[vertexIds[2]] - vertices[vertexIds[1]];
				pd1[vertexIds[2]] = vertices[vertexIds[0]] - vertices[vertexIds[2]];
			}
#pragma omp parallel for
			for (size_t v = 0; v < this->numVertices; v++) {
				pd1[v] = glm::normalize(glm::cross(pd1[v], normals[v]));
				pd2[v] = glm::cross(normals[v], pd1[v]);
			}
#pragma omp parallel for
			for (size_t face = 0; face < this->numFaces; face++) {
				int faceID = 3 * face;
				int vertexIds[3] = {
					indices[faceID],
					indices[faceID + 1],
					indices[faceID + 2]
				};
				vec3 verticesOnFace[3] = {
					vertices[vertexIds[0]],
					vertices[vertexIds[1]],
					vertices[vertexIds[2]]
				};
				vec3 normalsOnFace[3] = {
					normalize(normals[vertexIds[0]]),
					normalize(normals[vertexIds[1]]),
					normalize(normals[vertexIds[2]])
				};
				vec3 edges[3] = {
					verticesOnFace[2] - verticesOnFace[1],
					verticesOnFace[0] - verticesOnFace[2],
					verticesOnFace[1] - verticesOnFace[0],
				};
				float cornerAreasOnFace[3] = {
					this->cornerAreas[faceID],
					this->cornerAreas[faceID + 1],
					this->cornerAreas[faceID + 2]
				};
				float pointAreasOnFace[3] = {
					glm::intBitsToFloat(pointAreas[vertexIds[0]]),
					glm::intBitsToFloat(pointAreas[vertexIds[1]]),
					glm::intBitsToFloat(pointAreas[vertexIds[2]])
				};

				//Set normal, tangent, bitangent per face
				vec3 faceTangent = normalize(edges[0]);
				vec3 faceNormal = normalize(cross(edges[0], edges[1]));
				if (dot(faceNormal, normalsOnFace[0]) < 0.0) { faceNormal = -faceNormal; }
				vec3 faceBitangent = normalize(cross(faceNormal, faceTangent));

				//estimate curvature on face over normals' finite difference
				// m : 
				// w :
				float m[3] = { 0.0f, 0.0f, 0.0f };
				float w[3][3] = { {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
				for (size_t i = 0; i < 3; i++) {
					//using the tangent - bitangent as uv coords
					float u = dot(edges[i], faceTangent);
					float v = dot(edges[i], faceBitangent);
					w[0][0] += u * u;
					w[0][1] += u * v;
					w[2][2] += v * v;

					size_t prev = (i + 2) % 3;
					size_t next = (i + 1) % 3;

					// finite difference for dn
					vec3 dn = normalsOnFace[prev] - normalsOnFace[next];
					float dnu = dot(dn, faceTangent);
					float dnv = dot(dn, faceBitangent);

					m[0] += dnu * u;
					m[1] += dnu * v + dnv * u;
					m[2] += dnv * v;
				}
				w[1][1] = w[0][0] + w[2][2];
				w[1][2] = w[0][1];

				//Solve least squares!
				float diagonal[3] = { 0.0f,0.0f,0.0f };

				//LDLT Decomposition
				float d0 = w[0][0];
				diagonal[0] = 1.0f / d0;

				w[1][0] = w[0][1];
				float l10 = diagonal[0] * w[1][0];
				float d1 = w[1][1] - l10 * w[1][0];
				diagonal[1] = 1.0f / d1;

				float d2 = w[2][2] - (diagonal[0] * w[2][0] * w[2][0]) - (diagonal[1] * w[2][1] * w[2][1]);
				diagonal[2] = 1.0f / d2;
				w[2][0] = w[0][2];
				w[2][1] = w[1][2] - l10 * w[2][0];

				//Solve for LDLT decomposition
				for (int i = 0; i < 3; i++) {
					float sum = m[i];
					for (int j = 0; j < i; j++) {
						sum -= w[i][j] * m[j];
					}
					m[i] = sum * diagonal[i];
				}
				for (int i = 2; i >= 0; i--) {
					float sum = 0;
					for (int j = i + 1; j < 3; j++) {
						sum += w[j][i] * m[j];
					}
					m[i] -= sum * diagonal[i];
				}

				//Curvature tensor for each vertex of the face
				for (int i = 0; i < 3; i++) {
					float c1, c12, c2;
					//Project curvature tensor (2nd form) from the old u,v basis (saved in pd1,pd2)
					//to the new basis (newU newV)
					//Rotate back to global coords
					vec3 newU, newV;
					rotateCoordSys(pd1[vertexIds[i]], pd2[vertexIds[i]], glm::cross(faceTangent, faceBitangent), newU, newV);

					// projection
					float u1 = dot(newU, faceTangent);
					float v1 = dot(newU, faceBitangent);
					float u2 = dot(newV, faceTangent);
					float v2 = dot(newV, faceBitangent);

					c1 = m[0] * u1 * u1 + m[1] * (2.0f * u1 * v1) + m[2] * v1 * v1;
					c12 = m[0] * u1 * u2 + m[1] * (u1 * v2 + u2 * v1) + m[2] * v1 * v2;
					c2 = m[0] * u2 * u2 + m[1] * (2.0f * u2 * v2) + m[2] * v2 * v2;

					//weight = corner area / point area 
					// Voronoi area weighting. 
					float wt = cornerAreasOnFace[i] / pointAreasOnFace[i];
#pragma omp atomic
					curv1s[vertexIds[i]] += wt * c1;
#pragma omp atomic
					curv12s[vertexIds[i]] += wt * c12;
#pragma omp atomic
					curv2s[vertexIds[i]] += wt * c2;
				}

			}//end of for loop per face
			//loop per vertex
#pragma omp parallel for
			for (int v = 0; v < numVertices; v++) {
				vec3 normal = normals[v];

				//rotate back to local face coords
				vec3 oldU;
				vec3 oldV;
				rotateCoordSys(pd1[v], pd2[v], normal, oldU, oldV);
				//-
				//compute principal directions and curvatures with the curvature tensor (2FF) by eigens
				//Usually on computers eigenvectors / eigen  values are calculated with iterative QR decomposition 
				// As the 2ff matrix is symmetric, it always  has a full set of real eigenvectors/eigenvalues.
				float c = 1, s = 0, tt = 0;
				if (curv12s[v] != 0.0f) {
					// Jacobi rotation to diagonalize and get eigens -> Rather use QR
					float h = 0.5f * (curv2s[v] - curv1s[v]) / curv12s[v];
					tt = (h < 0.0f) ?
						1.0f / (h - glm::sqrt(1.0f + h * h)) :
						1.0f / (h + glm::sqrt(1.0f + h * h));
					c = 1.0f / glm::sqrt(1.0f + tt * tt);
					s = tt * c;
				}

				curv1s[v] = curv1s[v] - tt * curv12s[v];
				curv2s[v] = curv2s[v] + tt * curv12s[v];
				//ensure max curv goes to curv1
				if (glm::abs(curv1s[v]) >= glm::abs(curv2s[v])) {
					pd1[v] = c * oldU - s * oldV;
				}
				else {
					std::swap(curv1s[v], curv2s[v]);
					pd1[v] = s * oldU + c * oldV;
				}
				pd2[v] = glm::cross(normal, pd1[v]);

				pd1[v] = glm::normalize(pd1[v]);
				pd2[v] = glm::normalize(pd2[v]);
				//if (dot(pd1[v], vec3(1.0f,0.0f,0.0f)) < 0) pd1[v] = -pd1[v];
				//if (dot(pd2[v], vec3(0.0f, 1.0f, 0.0f)) < 0) pd2[v] = -pd2[v];
			}//end of for loop per vertex
			//align
			/*
			*/
#pragma omp parallel for
			for (size_t v = 0; v < numVertices; v++) {
				for (size_t j = 0; j < 20;j++) {
					int v1id = adjacentFaces[v][j];
					if (v1id < 0) break;
					if (dot(pd1[v], pd1[v1id]) < 0) pd1[v1id] = -pd1[v1id];
					if (dot(pd2[v], pd2[v1id]) < 0) pd2[v1id] = -pd2[v1id];
				}
			}

			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> elapsed_seconds = end - start;
			std::cout << "Curvatures for " << this->path << " calculated on CPU. Took " << elapsed_seconds.count() << " seconds.\n";
			//calculate error
			float errDist = 0.0f;
			double errSum = 0.0;
			//OpenMP reduction, takes partial sums and adds them at the end of the loop (Thus no overhead compared to atomic operations.)
			//OMP just handles savign the partial sum data and freeing it for us.
			
			
#pragma omp parallel for reduction(+:errSum)
			for (size_t v = 0; v < pd1.size(); v++) {
				//errSum += glm::distance(pd1[v], vec3(PDs[v]));
				PDs[v] = vec4(pd1[v], 0.0f);
			}
#pragma omp parallel for reduction(+:errSum)
			for (size_t v = 0; v < pd2.size(); v++) {
				//errSum += glm::distance(pd2[v], vec3(PDs[v + numVertices]));
				PDs[v + numVertices] = vec4(pd2[v], 0.0f);
			}
			//errDist = static_cast<float> (errSum / double(PDs.size()));
			//std::cout << "Average error distance per principal direction : " << errDist << "\n";
#pragma omp parallel for
			for (size_t v = 0; v < numVertices; v++) {
				this->PrincipalCurvatures[v] = curv1s[v];
				this->PrincipalCurvatures[v + numVertices] = curv2s[v];
			}
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, PDBuffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, PDs.size() * sizeof(glm::vec4), PDs.data(), GL_DYNAMIC_DRAW);

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, CurvatureBuffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, PrincipalCurvatures.size() * sizeof(GLfloat), PrincipalCurvatures.data(), GL_DYNAMIC_DRAW);

			this->curvaturesCalculated = true;
		}//end of costexpr if CPUtesting


		//glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); //unbind
		/*
		glDeleteBuffers(1, &vertexStorageBuffer);
		glDeleteBuffers(1, &normalStorageBuffer);
		glDeleteBuffers(1, &indexStorageBuffer);
		*/
	}
	//Finds adjacent vertices for each vertex
	void findAdjacentFaces() {
		if (adjacentFaces.size() != 0) return;
		auto start = std::chrono::high_resolution_clock::now();
		this->adjacentFaces.resize(vertices.size()); //adjacentFaces<std::array<int, 20>
		for (size_t i = 0; i < numVertices; i++) { adjacentFaces[i].fill(-1); }
		//#pragma omp parallel for
		for (size_t i = 0; i < numFaces; i++) { //loop each face
			size_t vertexIds[3] = { this->indices[3 * i],this->indices[3 * i + 1] ,this->indices[3 * i + 2] };
			for (size_t j = 0; j < 3; j++) { //loop each vertex on the face
				for (size_t k = 0; k < 10; k++) {
					if (this->adjacentFaces[vertexIds[j]][2 * k] < 0) {
						this->adjacentFaces[vertexIds[j]][2 * k] = vertexIds[(j + 1) % 3];
						this->adjacentFaces[vertexIds[j]][2 * k + 1] = vertexIds[(j + 2) % 3];
						break;
					}
				}
			}
		}
		glGenBuffers(1, &adjacentFacesBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, adjacentFacesBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, adjacentFaces.size() * sizeof(int) * 20, adjacentFaces.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 20, adjacentFacesBuffer);

		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - start;
		std::cout << "Adjacent faces calculated. Took : " << elapsed_seconds.count() << " seconds. \n";

		std::vector< std::array<unsigned int, 20>> cmpAdj; cmpAdj.resize(adjacentFaces.size());
		glGetNamedBufferSubData(adjacentFacesBuffer, 0, cmpAdj.size() * sizeof(GLint) * 20, cmpAdj.data());
		std::cout << "First vertex's adjacent vertices : ";
		for (int j = 0; j < 10; j++) {
			std::cout << this->adjacentFaces[0][2 * j] << ", " << this->adjacentFaces[0][2 * j + 1] << ". ";
		}
		std::cout << "\n";
		/*
		*/
	}
	//Calculates pseudo-"Voronoi" area for each vertex
	void computePointAreas() {
		constexpr bool CPUmode = true;
		//constexpr bool CPUmode = false;

		//int for concurrency with atomicCompSwap
		pointAreas.resize(this->numVertices, GLint{0}); //by vertex
		glGenBuffers(1, &pointAreaBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointAreaBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, pointAreas.size() * sizeof(GLint), pointAreas.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 30, pointAreaBuffer);

		cornerAreas.resize(this->numIndices, 0.0f); //by index (for faces)
		glGenBuffers(1, &cornerAreaBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, cornerAreaBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, cornerAreas.size() * sizeof(GLfloat), cornerAreas.data(), GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 31, cornerAreaBuffer);


		pointAreaCompute = loadComputeShader(".\\shaders\\pointAreas.compute");
		glUseProgram(pointAreaCompute);

		glUniform1ui(glGetUniformLocation(pointAreaCompute, "indicesSize"), this->numIndices);
		glUniform1ui(glGetUniformLocation(pointAreaCompute, "verticesSize"), this->numVertices);

		glDispatchCompute(glm::ceil((GLfloat(this->numIndices) / 3.0f) / float(workGroupSize)), 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		/*
		*/
		glGetNamedBufferSubData(cornerAreaBuffer, 0, cornerAreas.size() * sizeof(GLfloat), cornerAreas.data());
		glGetNamedBufferSubData(pointAreaBuffer, 0, pointAreas.size() * sizeof(GLint), pointAreas.data());
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		std::cout << "Corner Areas after compute : " << "\n";
		for (int dbg = 0; dbg < 3; dbg++) {
			std::cout << "cornerAreas[" << dbg << "] " << cornerAreas[dbg] << " ,";
			std::cout << "cornerAreas[" << cornerAreas.size() - dbg - 1 << "] " << cornerAreas[cornerAreas.size() - dbg - 1] << " ";
		}
		std::cout << "\n";
		std::cout << "Point Areas after compute : " << "\n";
		for (int dbg = 0; dbg < 3; dbg++) {
			std::cout << "pointAreas[" << dbg << "] " << glm::intBitsToFloat(pointAreas[dbg]) << " ,";
			std::cout << "pointAreas[" << pointAreas.size() - dbg - 1 << "] " << glm::intBitsToFloat(pointAreas[pointAreas.size() - dbg - 1]) << " ";
		}
		std::cout << "\n";


		if constexpr (CPUmode == true) {
			std::vector<float> cpuPointAreas, cpuCornerAreas;
			cpuPointAreas.resize(numVertices, 0.0f); cpuCornerAreas.resize(numIndices, 0.0f);
#pragma omp parallel for
			for (int id = 0; id < numFaces; id++) {
				int faceID = 3 * id;
				int vertexIds[3] = {
					indices[faceID],
					indices[faceID + 1],
					indices[faceID + 2]
				};

				//vertices
				vec3 verticesOnFace[3] = {
					vec3(vertices[vertexIds[0]]),
					vec3(vertices[vertexIds[1]]),
					vec3(vertices[vertexIds[2]])
				};
				//normals
				vec3 normalsOnFace[3] = {
					vec3(normals[vertexIds[0]]),
					vec3(normals[vertexIds[1]]),
					vec3(normals[vertexIds[2]])
				};
				//edges
				vec3 edges[3] = {
					verticesOnFace[2] - verticesOnFace[1],
					verticesOnFace[0] - verticesOnFace[2],
					verticesOnFace[1] - verticesOnFace[0]
				};
				float faceArea = 0.5 * length(cross(edges[0], edges[1]));
				float length2[3] = {
					length(edges[0]) * length(edges[0]),
					length(edges[1]) * length(edges[1]),
					length(edges[2]) * length(edges[2])
				};
				//Barycentric weights of circumcenter
				float barycentricWeights[3] = {
					length2[0] * (length2[1] + length2[2] - length2[0]),
					length2[1] * (length2[2] + length2[0] - length2[1]),
					length2[2] * (length2[0] + length2[1] - length2[2])
				};
				float cornerAreasTmp[3] = { 0.0, 0.0, 0.0 };
				// Negative barycentric weight -> Triangle's circumcenter lies outside the triangle
				if (barycentricWeights[0] <= 0.0) {
					cornerAreasTmp[1] = -0.25 * length2[2] * faceArea / dot(edges[0], edges[2]);
					cornerAreasTmp[2] = -0.25 * length2[1] * faceArea / dot(edges[0], edges[1]);
					cornerAreasTmp[0] = faceArea - cornerAreasTmp[1] - cornerAreasTmp[2];
				}
				else if (barycentricWeights[1] <= 0.0) {
					cornerAreasTmp[2] = -0.25 * length2[0] * faceArea / dot(edges[1], edges[0]);
					cornerAreasTmp[0] = -0.25 * length2[2] * faceArea / dot(edges[1], edges[2]);
					cornerAreasTmp[1] = faceArea - cornerAreasTmp[2] - cornerAreasTmp[0];
				}
				else if (barycentricWeights[2] <= 0.0) {
					cornerAreasTmp[0] = -0.25 * length2[1] * faceArea / dot(edges[2], edges[1]);
					cornerAreasTmp[1] = -0.25 * length2[0] * faceArea / dot(edges[2], edges[0]);
					cornerAreasTmp[2] = faceArea - cornerAreasTmp[0] - cornerAreasTmp[1];
				}
				else {
					float scale = 0.5 * faceArea / (barycentricWeights[0] + barycentricWeights[1] + barycentricWeights[2]);
					for (int j = 0; j < 3; j++) {
						int next = (j + 1) % 3;
						int prev = (j + 2) % 3;
						cornerAreasTmp[j] = scale * (barycentricWeights[next] + barycentricWeights[prev]);

					}
				}


				cpuCornerAreas[faceID] = cornerAreasTmp[0];
				cpuCornerAreas[faceID + 1] = cornerAreasTmp[1];
				cpuCornerAreas[faceID + 2] = cornerAreasTmp[2];

#pragma omp atomic
				cpuPointAreas[vertexIds[0]] += cornerAreasTmp[0];
#pragma omp atomic
				cpuPointAreas[vertexIds[1]] += cornerAreasTmp[1];
#pragma omp atomic
				cpuPointAreas[vertexIds[2]] += cornerAreasTmp[2];
			}
			double errSum = 0.0;
			for (int id = 0; id < cpuCornerAreas.size(); id++) {
				errSum += glm::abs(cpuCornerAreas[id] - cornerAreas[id]);
			}
			errSum /= cpuCornerAreas.size();
			std::cout << "Corner areas average difference from CPU computed : " << errSum << "\n";
			//copy back
			cornerAreas = cpuCornerAreas;
			for (int i = 0; i < pointAreas.size(); i++) {
				pointAreas[i] = glm::floatBitsToInt(cpuPointAreas[i]);
			}
			/*
			*/
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointAreaBuffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, pointAreas.size() * sizeof(GLint), pointAreas.data(), GL_DYNAMIC_DRAW);

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, cornerAreaBuffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, cornerAreas.size() * sizeof(GLfloat), cornerAreas.data(), GL_DYNAMIC_DRAW);
		}//end of if cpuMode
	}//end of pointAreas function
	void computeCurvColors() {
		curvColors.resize(numVertices, vec3(0.0f));
		if (!curvaturesCalculated) computeCurvatures();
		vec3 highColor = vec3(0.7f, 0.2f, 0.0f);
		vec3 lowColor = vec3(0.0f, 0.2f, 0.7f);
		float maxGaussian = -FLT_MAX;
		float minGaussian = FLT_MAX;
		float maxMean = -FLT_MAX;
		float minMean = FLT_MAX;
		std::vector<float> meanCurvatures; meanCurvatures.resize(numVertices);
		std::vector<float> gaussianCurvatures; gaussianCurvatures.resize(numVertices);
#pragma omp parallel for
		for (size_t i = 0; i < numVertices; i++) {
			meanCurvatures[i] = 0.5f * (PrincipalCurvatures[i] + PrincipalCurvatures[i + numVertices]);
			gaussianCurvatures[i] = PrincipalCurvatures[i] * PrincipalCurvatures[i + numVertices];
		}
		float scale = (8.0f * this->minDistance)* (8.0f * this->minDistance);
#pragma omp parallel for
		for (size_t i = 0; i < numVertices; i++) {
			float mc = meanCurvatures[i];
			float gc = gaussianCurvatures[i];
			float hue = 4.0f / 3.0f * glm::abs(glm::atan(mc * mc - gc, mc * mc * glm::sign(mc)));
			float saturation = 2.0f*glm::one_over_pi<float>() * glm::atan((2.0f * mc * mc - gc) * scale);
			curvColors[i] = hsv2rgb(hue,saturation,1.0f);
		}
	}
	vec3 hsv2rgb(float h,float s,float v) {
		//Converts HSV to RGB
		//Hue is in degrees
		//Saturation and value are in the range 0-1
		//Returns a vec3 with the RGB values
		float r, g, b;
		if (s == 0.0f) {
			r = g = b = v;
		}
		else {
			float h2 = h / 60.0f;
			int i = (int)glm::floor(h2);
			float f = h2 - i;
			float p = v * (1.0f - s);
			float q = v * (1.0f - s * f);
			float t = v * (1.0f - s * (1.0f - f));
			switch (i) {
			case 0:
				r = v; g = t; b = p; break;
			case 1:
				r = q; g = v; b = p; break;
			case 2:
				r = p; g = v; b = t; break;
			case 3:
				r = p; g = q; b = v; break;
			case 4:
				r = t; g = p; b = v; break;
			case 5:
				r = v; g = p; b = q; break;
			}
		}
		return vec3(r, g, b);
	}
	bool loadAssimp() {
		Assimp::Importer importer;

		if (!this->vertices.empty()) {
			return false; //if not empty return
		}

		std::cout << "\nLoading file : " << this->path << ".\n";
		//aiProcess_Triangulate !!!
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes | aiProcess_FixInfacingNormals /*| aiProcess_CalcTangentSpace | aiProcess_GenUVCoords*/); //aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);
		if (!scene) { //| aiProcess_JoinIdenticalVertices
			fprintf(stderr, importer.GetErrorString());
			return false;
		}
		std::cout << "Number of meshes : " << scene->mNumMeshes << ".\n";

		//current mesh
		const aiMesh* mesh = scene->mMeshes[0];

		omp_set_num_threads(getOMPMaxThreads());
		this->vertices.resize(mesh->mNumVertices, vec3(0.0f));
#pragma omp parallel for
		for (size_t i = 0; i < mesh->mNumVertices; i++) {
			aiVector3D pos = mesh->mVertices[i];
			//Can't use push_back parallely!
			//this->vertices.push_back(glm::vec3(pos.x, pos.y, pos.z));
			this->vertices[i] = glm::vec3(pos.x, pos.y, pos.z);
		}

		// Fill vertices texture coordinates
		this->textureCoordinates.resize(mesh->mNumVertices, glm::vec2(0.0f));
		if (mesh->HasTextureCoords(0)) {
#pragma omp parallel for
			for (size_t i = 0; i < mesh->mNumVertices; i++) {
				aiVector3D UVW = mesh->mTextureCoords[0][i]; // Assume only 1 set of UV coords; AssImp supports 8 UV sets.
				this->textureCoordinates[i] = (glm::vec2(UVW.x, UVW.y));
			}
		}

		// Fill vertices normals
		this->normals.resize(mesh->mNumVertices, vec3(0.0f));
		if (mesh->HasNormals()) {
#pragma omp parallel for
			for (size_t i = 0; i < mesh->mNumVertices; i++) {
				// std::cout<<"Number of Vertices : "<<mesh->mNumVertices<<"\n";
				aiVector3D n = mesh->mNormals[i];
				this->normals[i] = glm::normalize(glm::vec3(n.x, n.y, n.z));
			}
		}
		else {
			std::cout << "Model has no normals.\n";
			//mesh->
#pragma omp parallel for
			for (size_t i = 0; i < mesh->mNumVertices; i++) {
				aiVector3D n = mesh->mNormals[i];
				this->normals[i] = glm::normalize(glm::vec3(n.x, n.y, n.z));
			}
		}
		// Fill face indices
//#pragma omp parallel for
		for (size_t i = 0; i < mesh->mNumFaces; i++) {
			std::array<unsigned int, 3> fac{ 0 };
			for (size_t j = 0; j < mesh->mFaces[i].mNumIndices; j++) {
				this->indices.push_back(mesh->mFaces[i].mIndices[j]);
				fac[j] = mesh->mFaces[i].mIndices[j];
			}
			this->faces.push_back(fac);
		}

		std::cout << "Number of vertices : " << this->vertices.size() << "\n";
		std::cout << "Number of normals : " << this->normals.size() << "\n";
		std::cout << "Number of indices : " << this->indices.size() << "\n";
		std::cout << "Number of faces : " << this->faces.size() << "\n";

		this->numVertices = this->vertices.size();
		this->numNormals = this->normals.size();
		this->numFaces = this->faces.size();
		this->numIndices = this->indices.size();


		constexpr bool scaleDist = false;
		//Scale model for regular edge length
		if (scaleDist) {
		while (this->getMinDistance() < 0.1f) {
#pragma omp parallel for
			for (size_t i = 0; i < this->vertices.size(); i++) {
				this->vertices[i] *= 10.0f;
			}
		}
		while (this->getMinDistance() > 10.0f) {
#pragma omp parallel for
			for (size_t i = 0; i < this->vertices.size(); i++) {
				this->vertices[i] *= 0.1f;
			}
		}
		}

		// The "scene" pointer will be deleted automatically by "importer"
		return true;
	}	
	//Needs a new implementation with hashing float to int with 0.0001 precision or whatever.
	void combineIdenticalVertices() {
		if (numVertices > 500000)return;
		auto startTime = std::chrono::high_resolution_clock::now();
		cout << "Combining identical vertices...\n";
		for (size_t i = 0; i < this->numVertices; i++) {
			vec3 v0 = vertices[i];
			for (size_t j = 0; j < this->numVertices; ) {
				if (i == j) { j++; continue; }
				vec3 v1 = vertices[j];
				//if identical vertex
				//-> Calculating distance between every vertex with O(n^2) is very slow.
				if (glm::distance(v0, v1) < 1.0e-5) {
#pragma omp parallel for
					for (size_t ind = 0; ind < this->numIndices; ind++) {
						if (indices[ind] == j) indices[ind] = i;
						//updated indices
						if (indices[ind] > j) indices[ind]--;
					}
					//didn't do faces because I'm not using them
					this->vertices.erase(this->vertices.begin() + j);
					this->normals.erase(this->normals.begin() + j);
					this->textureCoordinates.erase(this->textureCoordinates.begin() + j);
					this->numVertices = this->vertices.size();
					this->numNormals = this->normals.size();
				}
				else {
					//increment only on not hit. If we hit, we need to work with the new current j again.
					j++;
				}
			}
		}
		//time
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - startTime;
		cout << "Combined identical vertices. Took "<< elapsed_seconds.count() <<" seconds.\n";
		return;
	}

	float avgEdgeLength() {
#pragma omp parallel for
		for (size_t i = 0; i < numVertices; i++) {
			;
		}
		//if (this->adjacentFaces.size() == 0)this->findAdjacentFaces();
	}
	float getMinDistance() {
		float minDist = glm::length(vertices[0] - vertices[1]);
		for (int i = 2; i < vertices.size(); i++) {
			float dis = glm::length(vertices[i] - vertices[0]);
			if (dis < minDist && dis>0.0f) minDist = dis;
		}
		return minDist;
	}
	bool rebindSSBOs() {

		//Rebind SSBOs
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, PDBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, CurvatureBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, vertexStorageBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, normalStorageBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, indexStorageBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 20, adjacentFacesBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 21, q1Buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 22, t1Buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 23, Dt1q1Buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 30, pointAreaBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 31, cornerAreaBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 42, dumpBuffer);
		return true;
	}	  
	static int getOMPMaxThreads() {
		//only runs once
		static int maxThreads = omp_get_max_threads();
		cout << "Max OMP threads : " << maxThreads << ".\n";
		return maxThreads;
	}
	
	//destructor
	//I should also be using smart pointers for the meshes / models
	//Smart pointers should NOT be in another smart pointer.
	//Nested ownership often causes leaks
	/*
	~Model() {

		glDeleteBuffers(1, &positionBuffer);
		glDeleteBuffers(1, &normalBuffer);
		glDeleteBuffers(1, &textureBuffer);
		glDeleteBuffers(1, &EBO);
		glDeleteBuffers(1, &PDBuffer);
		glDeleteBuffers(1, &CurvatureBuffer);
		glDeleteBuffers(1, &vertexStorageBuffer);
		glDeleteBuffers(1, &normalStorageBuffer);
		glDeleteBuffers(1, &indexStorageBuffer);
		glDeleteBuffers(1, &adjacentFacesBuffer);
		glDeleteBuffers(1, &q1Buffer);
		glDeleteBuffers(1, &t1Buffer);
		glDeleteBuffers(1, &Dt1q1Buffer);
		glDeleteBuffers(1, &pointAreaBuffer);
		glDeleteBuffers(1, &cornerAreaBuffer);
	}
	*/
	void rotateCoordSys(vec3 old_u, vec3 old_v, vec3 new_norm, vec3& new_u, vec3& new_v)
	{
		new_u = old_u;
		new_v = old_v;
		vec3 old_norm = cross(old_u, old_v);
		float ndot = dot(old_norm, new_norm);
		if (ndot <= -1.0f) {
			new_u = -new_u;
			new_v = -new_v;
			return;
		}
		// Perpendicular to old_norm and in the plane of old_norm and new_norm
		vec3 perp_old = new_norm - ndot * old_norm;
		// Perpendicular to new_norm and in the plane of old_norm and new_norm
		vec3 dperp = 1.0f / (1 + ndot) * (old_norm + new_norm);
		// Subtracts component along perp_old, and adds the same amount along perp_new.  
		// Leaves unchanged the component perpendicular to the
		// plane containing old_norm and new_norm.
		new_u -= dperp * (dot(new_u, perp_old));
		new_v -= dperp * (dot(new_v, perp_old));
		return;
	}
};
//end of Model class
