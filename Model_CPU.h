#include <vector>
#include <array>
#include <iostream>
#include <string>
#include <fstream>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include <assimp/Importer.hpp>      
#include <assimp/scene.h>          
#include <assimp/postprocess.h>     

#include "LoadShader.h"
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
	GLuint VAO, positionBuffer, normalBuffer, textureBuffer, smoothedNormalsBuffer, EBO;
	GLuint PDBuffer, CurvatureBuffer;
	GLuint maxPDVBO, maxCurvVBO, minPDVBO, minCurvVBO;

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<std::array<unsigned int, 3>> faces;
	std::vector<glm::vec2> textureCoordinates;
	std::vector<glm::vec3> tangents;
	std::vector<glm::vec3> bitangents;
	std::vector<GLuint> indices;

	std::vector<glm::vec4> PDs;
	std::vector<GLfloat> PrincipalCurvatures;

	std::vector<glm::vec4> maxPDs;
	std::vector<glm::vec4> minPDs;
	std::vector<GLfloat> maxCurvs;
	std::vector<GLfloat> minCurvs;

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


	Model(std::string path) {
		this->path = path;
		if (!this->loadAssimp()) { std::cout << "Model at " << path << " not loaded!\n"; };
		this->boundingBox();
		this->minDistance = this->getMinDistance();
		this->size = this->vertices.size();
		this->setupCurvatures();
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


			//glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		//std::cout << "Ready to render.\n";
		this->isSet = true;

		return;
	}
	//draw function
	bool render(GLuint shader) {

		if (!isSet) { this->setup(); }

		glUseProgram(shader);
		glBindVertexArray(VAO);

		//Rebind SSBOs
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, PDBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, CurvatureBuffer);



		glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);

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
	float getMinDistance() {
		float minDist = glm::length(vertices[0] - vertices[1]);
		for (int i = 2; i < vertices.size(); i++) {
			float dis = glm::length(vertices[i] - vertices[0]);
			if (dis < minDist && dis>0.0f) minDist = dis;
		}
		return minDist;
	}
	//Calculates principal curvatures and principal directions per vertex
	//Using a compute shader with SSBOs
	void setupCurvatures() {
		//Computed with CPU c++ code only for comparison with GPU compute shaders ----
		//per face
		for (int i = 0; i < this->numFaces; i++) {

		}

	}

	/*
	~Model() {
		glDeleteBuffers()
	}
	*/
	float compareVertices(std::vector<glm::vec3> v1, std::vector<glm::vec3> v2) {
		float err = 0.0f;
		for (int i = 0; i < v1.size();i++) {
			err += glm::distance(v1[i],v2[i]);
		}
		return err;
	}

	bool loadAssimp() {
		Assimp::Importer importer;

		if (!this->vertices.empty()) {
			return false; //if not empty return
		}

		std::cout << "Loading file : " << this->path << ".\n";
		//aiProcess_Triangulate !!!
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace | aiProcess_GenUVCoords); //aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);
		if (!scene) {
			fprintf(stderr, importer.GetErrorString());
			return false;
		}

		// TODO : In this code we just use the 1st mesh (for now)
		const aiMesh* mesh = scene->mMeshes[0];
		// Fill vertices positions
		//std::cout << "Number of vertices :" << mesh->mNumVertices << "\n";
		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			aiVector3D pos = mesh->mVertices[i];
			this->vertices.push_back(glm::vec3(pos.x, pos.y, pos.z));
		}

		// Fill vertices texture coordinates
		if (mesh->HasTextureCoords(0)) {
			for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
				aiVector3D UVW = mesh->mTextureCoords[0][i]; // Assume only 1 set of UV coords; AssImp supports 8 UV sets.
				this->textureCoordinates.push_back(glm::vec2(UVW.x, UVW.y));
			}
		}

		// Fill vertices normals
		if (mesh->HasNormals()) {
			for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
				// std::cout<<"Number of Vertices : "<<mesh->mNumVertices<<"\n";
				aiVector3D n = mesh->mNormals[i];
				this->normals.push_back(glm::normalize(glm::vec3(n.x, n.y, n.z)));
			}
		}
		else {
			//std::cout << "Model has no normals.\n";
			//mesh->
			for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
				// std::cout<<"Number of Vertices : "<<mesh->mNumVertices<<"\n";
				aiVector3D n = mesh->mNormals[i];
				this->normals.push_back(glm::normalize(glm::vec3(n.x, n.y, n.z)));
			}
		}
		// Fill face indices
		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			std::array<unsigned int, 3> fac;
			for (unsigned int j = 0; j < mesh->mFaces[i].mNumIndices; j++) {
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

		// The "scene" pointer will be deleted automatically by "importer"
		return true;
	}
};
//end of Model class


bool loadAssimp(
	const char* path,
	std::vector<glm::vec3>& out_vertices,
	std::vector<glm::vec3>& out_normals,
	std::vector<unsigned int>& out_indices
) {
	std::vector<unsigned int> indices;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	Assimp::Importer importer;
	printf("Loading file : %s...\n", path);
	//aiProcess_Triangulate !!!
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace | aiProcess_GenUVCoords); //aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);
	if (!scene) {
		fprintf(stderr, importer.GetErrorString());
		return false;
	}
	// TODO : In this code we just use the 1st mesh (for now)
	const aiMesh* mesh = scene->mMeshes[0];
	// Fill vertices positions
	//std::cout << "Number of vertices :" << mesh->mNumVertices << "\n";
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		aiVector3D pos = mesh->mVertices[i];
		out_vertices.push_back(glm::vec3(pos.x, pos.y, pos.z));
	}

	// Fill vertices texture coordinates
	if (mesh->HasTextureCoords(0)) {
		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			aiVector3D UVW = mesh->mTextureCoords[0][i]; // Assume only 1 set of UV coords; AssImp supports 8 UV sets.
			uvs.push_back(glm::vec2(UVW.x, UVW.y));
		}
	}

	// Fill vertices normals
	if (mesh->HasNormals()) {
		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			// std::cout<<"Number of Vertices : "<<mesh->mNumVertices<<"\n";
			aiVector3D n = mesh->mNormals[i];
			out_normals.push_back(glm::normalize(glm::vec3(n.x, n.y, n.z)));
		}
	}
	else {
		//std::cout << "Model has no normals.\n";
		//mesh->
		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			// std::cout<<"Number of Vertices : "<<mesh->mNumVertices<<"\n";
			aiVector3D n = mesh->mNormals[i];
			out_normals.push_back(glm::normalize(glm::vec3(n.x, n.y, n.z)));
		}
	}
	// Fill face indices
	//std::cout << "Number of faces :" << mesh->mNumFaces << "\n";
	//std::cout << "Number of indices in faces :" << mesh->mFaces[0].mNumIndices << "\n";
	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
		for (unsigned int j = 0; j < mesh->mFaces[i].mNumIndices; j++)
			out_indices.push_back(mesh->mFaces[i].mIndices[j]);
	}

	std::cout << "Size of vertices : " << out_vertices.size() << "\n";
	std::cout << "Size of normals : " << out_normals.size() << "\n";
	std::cout << "Size of indices : " << out_indices.size() << "\n";


	// The "scene" pointer will be deleted automatically by "importer"
	return true;
}
