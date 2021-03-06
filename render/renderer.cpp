//GLEW openGL Extension Wrangler Library
#include <GL/glew.h>
//GLFW used for keyboard IO and window creation
#include <GLFW/glfw3.h>

#include "../log.hpp"
#include <string.h>
#include <glm/glm.hpp>
#include "renderer.hpp"
#include "programLoader.hpp"

using namespace std;
using namespace glm;

//All data is considered persistant and is kept in a single buffer per render type, dynamic data will be added latter

/** Holds methods used to render solid triangles */
namespace solidTriangle {
	/** whether the GPU buffer needs to be updated next frame */
	bool update = false;

	int DATA_SIZE_FLOAT = 18;

	GLuint vao;
	GLuint vbo;
	GLuint program;

	/** The top of the linked list implementation */
	Data* head;
	int size = 0;

	Data* add(vec2 a, vec2 b, vec2 c, vec4 colour) {
		Data* d = new Data;
		d->a = a;
		d->b = b;
		d->c = c;
		d->colour = colour;

		d->next = head;
		d->previous = NULL;

		if(head != NULL)
			head->previous = d;

		head = d;

		size++;
		update = true;

		return d;
	}

	void remove(Data* data) {
		if(data->next != NULL)
			data->next->previous = data->previous;

		if(data->previous != NULL)
			data->previous->next = data->next;

		delete data;
		update = true;

		size--;
	}

	void init() {
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glBindVertexArray(vao);

		glEnableVertexAttribArray(0); //used for position
		glEnableVertexAttribArray(1); //used for color, texturing will be added later if required

		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) 0);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) (2 * sizeof(float)));
	}

	void clean() {
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);
		//glDeleteProgram(program); removed so the the solid colour program is not deleted twice
	}

	void remakeBuffer() {
		//TODO use mapped buffers
		GLfloat data[size * DATA_SIZE_FLOAT];

		Data* current = head;
		int i = 0;
		while(current != NULL) {
			//vert 1
			data[i++] = current->a.x;
			data[i++] = current->a.y;

			data[i++] = current->colour.r;
			data[i++] = current->colour.g;
			data[i++] = current->colour.b;
			data[i++] = current->colour.a;

			//vert 2
			data[i++] = current->b.x;
			data[i++] = current->b.y;

			data[i++] = current->colour.r;
			data[i++] = current->colour.g;
			data[i++] = current->colour.b;
			data[i++] = current->colour.a;

			//vert 3
			data[i++] = current->c.x;
			data[i++] = current->c.y;

			data[i++] = current->colour.r;
			data[i++] = current->colour.g;
			data[i++] = current->colour.b;
			data[i++] = current->colour.a;

			current = current->next;
		}

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

		update = false;
	}

	void draw() {
		if(update)
			remakeBuffer();

		glBindVertexArray(vao);
		glUseProgram(program);
		glDrawArrays(GL_TRIANGLES, 0, size * 3);
	}
}

/** Holds methods used to render solid triangles */
namespace texturedTriangle {
	/** whether the GPU buffer needs to be updated next frame */
	bool update = false;

	int DATA_SIZE_FLOAT = 12;

	GLuint vao;
	GLuint vbo;
	GLuint program = 0;

	Data** start = new Data*[0]; //holds the start of each texture array, if the array has 0 length
	int size = 0; //number of triangles
	int* sizes = new int[0]; //holds the size of each texture array

	//NB arrays need to be updated whenever the number of textures change
	int numberOfTextures = 0; //the number of textures

	Data* add(vec2 a, vec2 b, vec2 c, vec2 p, vec2 q, vec2 r, int texture) {
		if(texture >= numberOfTextures) {
			Data** tmpStart = new Data*[texture + 1]();
			std::copy(start, start + numberOfTextures, tmpStart);
			delete [] start;
			start = tmpStart;

			int* tmpSizes = new int[texture + 1]();
			std::copy(sizes, sizes + numberOfTextures, tmpSizes);
			delete [] sizes;
			sizes = tmpSizes;

			numberOfTextures = texture + 1;
		}

		Data* d = new Data;
		d->a = a;
		d->b = b;
		d->c = c;
		d->p = p;
		d->q = q;
		d->r = r;
		d->texture = texture;

		d->next = start[texture];

		if(start[texture] == NULL)
			d->previous = NULL;
		else
			d->previous = start[texture]->previous;

		if(start[texture] != NULL)
			start[texture]->previous = d;

		start[texture] = d;

		size++;
		sizes[d->texture]++;

		update = true;

		return d;
	}

	void remove(Data* data) {
		sizes[data->texture]--;

		if(start[data->texture] == data)
			start[data->texture] = (data->next == NULL || data->next->texture == data->texture) ? data->next : NULL;

		if(data->next != NULL)
			data->next->previous = data->previous;

		if(data->previous != NULL)
			data->previous->next = data->next;

		delete data;

		size--;

		update = true;
	}

	void init() {
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glBindVertexArray(vao);

		glEnableVertexAttribArray(0); //used for position
		glEnableVertexAttribArray(1); //used for color, texturing will be added later if required

		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) 0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) (2 * sizeof(float)));
	}

	void clean() {
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);
		glDeleteProgram(program);
	}

	void remakeBuffer() {
		//TODO use mapped buffers
		GLfloat data[size * DATA_SIZE_FLOAT];

		Data* current = NULL;

		if(numberOfTextures != 0) {
			current = start[0];

			int i = 0;

			while(current != NULL) {
				//vert 1
				data[i++] = current->a.x;
				data[i++] = current->a.y;

				data[i++] = current->p.s;
				data[i++] = current->p.t;

				//vert 2
				data[i++] = current->b.x;
				data[i++] = current->b.y;

				data[i++] = current->q.s;
				data[i++] = current->q.t;

				//vert 2
				data[i++] = current->c.x;
				data[i++] = current->c.y;

				data[i++] = current->r.s;
				data[i++] = current->r.t;

				current = current->next;
			}
		}

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

		update = false;
	}

	void draw() {
		if(update)
			remakeBuffer();

		glBindVertexArray(vao);
		glUseProgram(program);

		int acc = 0;
		for(int i = 0; i < numberOfTextures; i++) {
			if(sizes[i] == 0)
				continue;

			glUniform1i(0, i);
			glDrawArrays(GL_TRIANGLES, acc, sizes[i] * 3);
			acc += sizes[i];
		}
	}
}

/** Holds methods used to render coloured lines */
namespace line {
	/** whether the GPU buffer needs to be updated next frame */
	bool update = false;
	const int DATA_SIZE_FLOAT = 16;

	GLuint vao;
	GLuint vbo;
	GLuint program = 0;

	/** The top of the linked list implementation */
	Data* head = NULL;
	int size = 0;

	Data* add(vec2 a, vec2 b, vec4 colour) {
		Data* d = new Data;
		d->a = a;
		d->b = b;
		d->colour = colour;

		d->next = head;
		d->previous = NULL;

		if(head != NULL)
			head->previous = d;

		head = d;

		size++;

		update = true;
		return d;
	}

	void remove(Data* data) {
		if(data->next != NULL)
			data->next->previous = data->previous;

		if(data->previous != NULL)
			data->previous->next = data->next;

		delete data;

		size--;
		update = true;
	}

	void init() {
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glBindVertexArray(vao);

		glEnableVertexAttribArray(0); //used for position
		glEnableVertexAttribArray(1); //used for color, texturing will be added later if required

		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) 0);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) (2 * sizeof(float)));
	}

	void clean() {
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);
		glDeleteProgram(program);
	}

	void remakeBuffer() {
		//TODO use mapped buffers
		GLfloat data[size * DATA_SIZE_FLOAT];

		Data* current = head;
		int i = 0;
		while(current != NULL) {
			//vert 1
			data[i++] = current->a.x;
			data[i++] = current->a.y;

			data[i++] = current->colour.r;
			data[i++] = current->colour.g;
			data[i++] = current->colour.b;
			data[i++] = current->colour.a;

			//vert 2
			data[i++] = current->b.x;
			data[i++] = current->b.y;

			data[i++] = current->colour.r;
			data[i++] = current->colour.g;
			data[i++] = current->colour.b;
			data[i++] = current->colour.a;

			current = current->next;
		}

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

		update = false;
	}

	void draw() {
		if(update)
			remakeBuffer();

		glBindVertexArray(vao);
		glUseProgram(program);
		glDrawArrays(GL_LINES, 0, size * 2);
	}
}

//holds rendering methods
namespace render {
	GLFWwindow* window;

	bool checkGL(string message) {
		GLenum err = glGetError();
		if(err != GL_NO_ERROR) {
			log(ERROR, message + string((const char*) (gluErrorString(err))));
			return true;
		}

		return false;
	}

	/** creates and initializes all of the opengl programs needed */
	void createPrograms() {
		GLuint colourVertex = shader::create(GL_VERTEX_SHADER, "colourvertex");
		GLuint colourFragment = shader::create(GL_FRAGMENT_SHADER, "colourfragment");
		GLuint textureFragment = shader::create(GL_FRAGMENT_SHADER, "texturefragment");
		GLuint textureVertex = shader::create(GL_VERTEX_SHADER, "texturevertex");

		CHECK_GL();
		CHECK_GL();

		GLuint colourArray [] = {colourVertex, colourFragment};
		GLuint textureArray [] = {textureVertex, textureFragment};

		line::program = program::create(colourArray);
		solidTriangle::program = line::program;

		texturedTriangle::program = program::create(textureArray);

		CHECK_GL();
	}

	/* Creates a window with Opengl versions major.minor
 	 * if context creation fails or you need more features submit a pull
 	 * request for a different version number ect...
 	 */
	void init(int major, int minor, int samples, string title, bool fullscreen) {
		if(!glfwInit()) {
			log(ERROR, "Failed to initialize GLFW");
			return;
		}

		GLFWmonitor* monitor = glfwGetPrimaryMonitor();

		const GLFWvidmode* mode = glfwGetVideoMode(monitor);

		glfwWindowHint(GLFW_SAMPLES, samples); //multisampling

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major); //setting Opengl version
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);

		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); //needed for MAC users

		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		glfwWindowHint(GLFW_RED_BITS, mode->redBits); //attempting to avoid driver switches
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

		window = glfwCreateWindow(mode->width / (fullscreen ? 1 : 2), mode->height / (fullscreen ? 1 : 2), title.c_str(), fullscreen ? monitor : NULL, NULL);
		glfwMakeContextCurrent(window);

		glewExperimental = GL_TRUE; //needed to make opengl work, weird

		if(glewInit() != GLEW_OK) {
			log(ERROR, "Error in glew initializtion");
			return;
		}

		glGetError(); //this line prevents it by flushing the error buffers.
		CHECK_GL(); //this line always returns an error, not sure why

		createPrograms();

		CHECK_GL();

		line::init();
		solidTriangle::init();
		texturedTriangle::init();
	}

	void draw();

	/* Swaps buffers and renders the queued shapes */
	void tick() {
		draw();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	/* Returns true if the the window has been closed */
	bool shouldClose() {
		return glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0;
	}

	void clean() {
		line::clean();
		solidTriangle::clean();
		texturedTriangle::clean();

		if(!CHECK_GL())
			log(INFO, "OpenGL has cleaned up sucessfully.");

		glfwTerminate();
	}

	/** Draws the current queue onto the currently bound framebuffer */
	void draw() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if(solidTriangle::size != 0)
			solidTriangle::draw();

		if(line::size != 0)
			line::draw();

		if(texturedTriangle::size != 0)
			texturedTriangle::draw();
	}
}

namespace texture {
	void bind(GLuint image, int unit) {
		//unit < 80 or programs may crash on low spec cards

		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, image);
	}
}
