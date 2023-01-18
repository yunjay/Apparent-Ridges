#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include <assimp/Importer.hpp>      
#include <assimp/scene.h>          
#include <assimp/postprocess.h>     
bool loadAssimp(const char* path,std::vector<glm::vec3>& out_vertices,std::vector<glm::vec3>& out_normals,std::vector<unsigned int>& out_indices);

class Model {
public:
	GLuint VAO, positionBuffer, normalBuffer, textureBuffer, smoothedNormalsBuffer, EBO, maxPDBuffer, minPDBuffer, maxCurvBuffer, minCurvBuffer;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> textureCoordinates;
	std::vector<glm::vec3> maxPDs;
	std::vector<glm::vec3> minPDs;
	std::vector<GLfloat> maxCurvs, minCurvs;
	std::vector<unsigned int> indices;
	std::string path;
	GLfloat diagonalLength = 0.0f;
	GLfloat modelScaleFactor = 1.0f;
	GLfloat minDistance;
	glm::vec3 center = glm::vec3(0.0f);

	bool isSet = false;


	Model(std::string path) {
		this->path = path;
		loadAssimp(this->path.data(), this->vertices, this->normals, this->indices );
		this->boundingBox();
		this->minDistance = this->getMinDistance();
		this->setup();
	}
	void setup() {
		//std::cout << "Setting up buffers.\n";

		glGenVertexArrays(1, &VAO); //vertex array object
		glGenBuffers(1, &positionBuffer); //vertex buffer object
		glGenBuffers(1, &normalBuffer); //vertex buffer object
		glGenBuffers(1, &textureBuffer); //vertex buffer object

		glGenBuffers(1, &EBO);

		glGenBuffers(1, &maxPDBuffer);
		glGenBuffers(1, &minPDBuffer);
		glGenBuffers(1, &maxCurvBuffer);
		glGenBuffers(1, &minCurvBuffer);

		//VAO  
		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);

		// maxPD, minPD, maxCurv, minCurv

		//EBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);



		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, textureBuffer);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		//std::cout << "Ready to render.\n";
		this->isSet = true;

		return;
	}

	bool render(GLuint shader) {

		if (!isSet) { this->setup(); }

		glUseProgram(shader);
		glBindVertexArray(VAO);

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
		this->modelScaleFactor = 1.0f/diagonalLength;
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
	//Let's use a compute shader with a SSBO
	void setupCurvatures() {
		//generate ssbo to use in compute shader
		//we need to calculate 2 principal directions and 2 principal curvatures per vertex.
		//NOTE : SSBOs do not work well with vec3s. They take them as vec4 anyway or sth. Use vec4.
		GLuint ssbo;
		glGenBuffers(1, &ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);


		unsigned int verticesSize = this->vertices.size();
		this->maxPDs.resize(verticesSize); this->minPDs.resize(verticesSize);
		this->maxCurvs.resize(verticesSize); this->minCurvs.resize(verticesSize);


		//kill ssbo
		glDeleteBuffers(1,&ssbo);
	}


	~Model() {
		glDeleteBuffers()
	}
};


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
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals| aiProcess_JoinIdenticalVertices ); //aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);
	if (!scene) {
		fprintf(stderr, importer.GetErrorString());
		return false;
	}
	const aiMesh* mesh = scene->mMeshes[0]; // In this code we just use the 1st mesh
	// Fill vertices positions
	//std::cout << "Number of vertices :" << mesh->mNumVertices << "\n";
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		aiVector3D pos = mesh->mVertices[i];
		out_vertices.push_back(glm::vec3(pos.x, pos.y, pos.z));
		//std::cout <<"Num vertices : "<< mesh->mNumVertices<<"\n";
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

	//std::cout << "Size of vertices : " << out_vertices.size() << "\n";
	//std::cout << "Size of normals : " << out_normals.size() << "\n";
	//std::cout << "Size of indices : " << out_indices.size() << "\n";
	// The "scene" pointer will be deleted automatically by "importer"
	return true;
}
