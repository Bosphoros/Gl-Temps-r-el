#include <iostream>
#include <fstream>
#include <sstream>

#include <GL/glew.h>

#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>

#include "Mesh.h"
#include "OffReader.h"

void render(GLFWwindow*);
void init();

#define glInfo(a) std::cout << #a << ": " << glGetString(a) << std::endl

float v = 0.0f;
float pos = 0.0f;
float data[16] = {-0.5,-0.5,0,1 ,0.5,-0.5,0,1, 0.5,0.5,0,1, -0.5,0.5,0, 1};

/*******************************************************************************
****************** IDS *********************************************************
*******************************************************************************/

#define LUX_CAM 4
#define ROTATION_CAM 5
#define LIGHT 6

#define POINTS 10
#define NORMALES 11


Mesh mesh;
OffReader reader;

glm::vec4 p1centre(-0.5,-0.5,0,1);
glm::vec4 pextend(0.5,0.5,0,0);
glm::vec4 p2centre(0.5,-0.5,0,1);
glm::vec3 camPos(0,2,3);
glm::mat4 lookat = glm::lookAt(camPos, glm::vec3(0,0,0), glm::vec3(0,0,1));
glm::mat4 perspective = glm::perspective(glm::radians(60.0f), float(4)/float(3), 0.1f, 100.0f);

std::vector<point3> points;
std::vector<unsigned int> indices;
glm::vec3 light(2, 2, 2); 


// This function is called on any openGL API error
void debug(GLenum, // source
		   GLenum, // type
		   GLuint, // id
		   GLenum, // severity
		   GLsizei, // length
		   const GLchar *message,
		   const void *) // userParam
{
	std::cout << "DEBUG: " << message << std::endl;
}

int main(void)
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
	{
		std::cerr << "Could not init glfw" << std::endl;
		return -1;
	}

	// This is a debug context, this is slow, but debugs, which is interesting
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(640, 480, "Funk it", NULL, NULL);
	if (!window)
	{
		std::cerr << "Could not init window" << std::endl;
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	GLenum err = glewInit();
	if(err != GLEW_OK)
	{
		std::cerr << "Could not init GLEW" << std::endl;
		std::cerr << glewGetErrorString(err) << std::endl;
		glfwTerminate();
		return -1;
	}

	// Now that the context is initialised, print some informations
	glInfo(GL_VENDOR);
	glInfo(GL_RENDERER);
	glInfo(GL_VERSION);
	glInfo(GL_SHADING_LANGUAGE_VERSION);

	// And enable debug
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

	//glDebugMessageCallback(GLDEBUGPROC(debug), nullptr);

	// This is our openGL init function which creates ressources
	init();

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		/* Render here */
		render(window);

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}

// Build a shader from a string
GLuint buildShader(GLenum const shaderType, std::string const src)
{
	GLuint shader = glCreateShader(shaderType);

	const char* ptr = src.c_str();
	GLint length = src.length();

	glShaderSource(shader, 1, &ptr, &length);

	glCompileShader(shader);

	GLint res;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
	if(!res)
	{
		std::cerr << "shader compilation error" << std::endl;

		char message[1000];

		GLsizei readSize;
		glGetShaderInfoLog(shader, 1000, &readSize, message);
		message[999] = '\0';

		std::cerr << message << std::endl;

		glfwTerminate();
		exit(-1);
	}

	return shader;
}

// read a file content into a string
std::string fileGetContents(const std::string path)
{
	std::ifstream t(path);
	std::stringstream buffer;
	buffer << t.rdbuf();

	return buffer.str();
}

// build a program with a vertex shader and a fragment shader
GLuint buildProgram(const std::string vertexFile, const std::string fragmentFile)
{
	auto vshader = buildShader(GL_VERTEX_SHADER, fileGetContents(vertexFile));
	auto fshader = buildShader(GL_FRAGMENT_SHADER, fileGetContents(fragmentFile));

	GLuint program = glCreateProgram();

	glAttachShader(program, vshader);
	glAttachShader(program, fshader);

	glLinkProgram(program);

	GLint res;
	glGetProgramiv(program, GL_LINK_STATUS, &res);
	if(!res)
	{
		std::cerr << "program link error" << std::endl;

		char message[1000];

		GLsizei readSize;
		glGetProgramInfoLog(program, 1000, &readSize, message);
		message[999] = '\0';

		std::cerr << message << std::endl;

		glfwTerminate();
		exit(-1);
	}

	return program;
}

/****************************************************************
 ******* INTERESTING STUFFS HERE ********************************
 ***************************************************************/

// Store the global state of your program
struct
{
	GLuint program; // a shader
	GLuint vao; // a vertex array object
	GLuint vbo;
	GLuint buffer;
	GLuint depthTexture;
	GLuint fbo;
} gs;

void fillPoints(){
	for(int i = 0; i < mesh.points.size(); ++i) {
		points.push_back(mesh.points.at(i));
		points.push_back(mesh.normalesPoints.at(i));
	}

	for(int i = 0; i < mesh.faces.size(); ++i) {
		indices.push_back(mesh.faces.at(i));
	}
}

void init()
{

	mesh = reader.import("C:/Users/etu/Documents/GitHub/Gl-Temps-r-el/bunny.off");
	mesh.center();
	mesh.normalize();
	mesh.norms();
	mesh.normsPoints();
	mesh.scale(2);
	
	fillPoints();

	// Build our program and an empty VAO
	gs.program = buildProgram("C:/Users/etu/Documents/GitHub/Gl-Temps-r-el/basic.vsl", "C:/Users/etu/Documents/GitHub/Gl-Temps-r-el/basic.fsl");

	glEnable(GL_DEPTH_TEST);

	glGenBuffers(1, &(gs.buffer));
	glBindBuffer(GL_ARRAY_BUFFER, gs.buffer);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(double)*3*points.size(), &points[0], GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*points.size(), &points[0], GL_STATIC_DRAW);


	glCreateVertexArrays(1, &gs.vao);
	glBindVertexArray(gs.vao);

	glBindBuffer(GL_ARRAY_BUFFER, gs.buffer);
	glVertexAttribPointer(POINTS, 3, GL_FLOAT, GL_FALSE, sizeof(float)*2*3, 0);
	glEnableVertexAttribArray(POINTS);

	glBindBuffer(GL_ARRAY_BUFFER, gs.buffer);
	glVertexAttribPointer(NORMALES, 3, GL_FLOAT, GL_FALSE, sizeof(float)*2*3, (void *) (sizeof(float)*3));
	glEnableVertexAttribArray(NORMALES);

	glGenBuffers(1, &(gs.vbo));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gs.vbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	// create the depth texture
	glGenTextures(1, &gs.depthTexture);
	glBindTexture(GL_TEXTURE_2D, gs.depthTexture);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, 800, 800);

	// Framebuffer
	glGenFramebuffers(1, &gs.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, gs.fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gs.depthTexture, 0);

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// Texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gs.depthTexture);

	glBindVertexArray(0);
}

void renderShadowMap() {

	

	lookat = glm::lookAt(light, glm::vec3(0,0,0), glm::vec3(0,1,0));
	glm::mat4 mat = perspective*lookat;

	glProgramUniformMatrix4fv(gs.program, ROTATION_CAM, 1, false, &mat[0][0]);
	glProgramUniform3fv(gs.program, LIGHT, 1, &light[0]);

	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(gs.program);
	glBindVertexArray(gs.vao);

	{
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (void*)0);
	}

	glBindVertexArray(0);
	glUseProgram(0);

}

void renderCamera() {
	

	lookat = glm::lookAt(camPos, glm::vec3(0,0,0), glm::vec3(0,1,0));
	glm::mat4 mat = perspective*lookat;
	glm::mat4 luxMat = perspective*glm::lookAt(light, glm::vec3(0,0,0), glm::vec3(0,1,0));

	glProgramUniformMatrix4fv(gs.program, ROTATION_CAM, 1, false, &mat[0][0]);
	glProgramUniformMatrix4fv(gs.program, LUX_CAM, 1, false, &luxMat[0][0]);
	glProgramUniform3fv(gs.program, LIGHT, 1, &light[0]);

	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(gs.program);
	glBindVertexArray(gs.vao);

	{
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (void*)0);
	}

	glBindVertexArray(0);
	glUseProgram(0);
}

void render(GLFWwindow* window)
{
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, 800,800);
	glBindFramebuffer(GL_FRAMEBUFFER, gs.fbo);
	renderShadowMap();

	glViewport(0,0, width,height);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	renderCamera();

	light = glm::rotateY(light, glm::radians(1.0f));
}
