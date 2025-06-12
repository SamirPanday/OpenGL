#include<glad/glad.h>
#include<iostream>
#include<GLFW/glfw3.h>


//opengl doesnt define shader programs on its own. so i need to do is on my own.
//btw shaders are just in the background somewhere running. we have to access them through references. actually all opengl objects are accessed by references

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(0.8f, 0.3f, 0.02f, 1.0f);\n"
"}\n\0";




int main() {

	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 800, "hello triangle", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create the GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);




	gladLoadGL();

	//tell opengl the area of the window to render in
	glViewport(0, 0, 800, 800);

	//clear the window and add the given color. glclearcolor specifies what color to fill when screen is cleared
	glClearColor(0.17f, 0.13f, 0.17f, 1.0f);

	//this one actually clears the screen to the specified color
	glClear(GL_COLOR_BUFFER_BIT);

	//at this point we have the back buffer with the color we want and the front buffer thats still the same as before. we just need to swap these two buffers and boom! we get our colored window!
	glfwSwapBuffers(window);




	//define a shader object
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	//attach the object with the source code we wrote on top
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	//do the exact same thing for the fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	//boom so we created the shader object and compiled its respective source code. now in order to use them, we need to wrap them into a shader program
	GLuint shaderProgram = glCreateProgram();
	//attach the 2 shaders to this program
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	//link the two shaders. output of vertexshader will be input of fragment shader here.
	glLinkProgram(shaderProgram);

	//now we have a shader program that does what vertex and fragment shaders should do. since i have this program, i dont need the shader objects from above. so lets clean them up
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);




	float vertices[] = {
		 0.5f,  0.5f, 0.0f,  // top right
		 0.5f, -0.5f, 0.0f,  // bottom right
		-0.5f, -0.5f, 0.0f,  // bottom left
		-0.5f,  0.5f, 0.0f   // top left 
	};
	unsigned int indices[] = {  // note that we start from 0!
		0, 1, 3,  // first Triangle
		1, 2, 3   // second Triangle
	};

	//now that we have the shader program to use, we need to yk, use them. lets create buffer objects that deliver data to gpu's buffer.
	//we make VBO and VAO. VAO is just an array object that stores pointer to mulitple VBOs. make sure VAO is defined BERORE VBO. order matters apparently

	//we add an index element buffer object as well now
	GLuint VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	//bind vao so that we can work with it.
	glBindVertexArray(VAO);

	// i need to tell what this buffer stores. it stores vertices. so the buffer type that stores vertices is gl_array_buffer
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//so the buffer along with its type is now set up. now i need to load the above triangle vertex data onto the buffer itself
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//for EBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// tell opengl how to interpret our vertex data now.
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//unbinding VBO and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);




	// Main while loop
	while (!glfwWindowShouldClose(window))
	{
		// Specify the color of the background
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		// Clean the back buffer and assign the new color to it
		glClear(GL_COLOR_BUFFER_BIT);
		// Tell OpenGL which Shader Program we want to use
		glUseProgram(shaderProgram);
		// Bind the VAO so OpenGL knows to use it
		glBindVertexArray(VAO);
		// Draw the triangle using the GL_TRIANGLES primitive
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		// Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		// Take care of all GLFW events. check for any mouse,keyboard input and then decide what to do.
		glfwPollEvents();
	}



	glDeleteProgram(shaderProgram);
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glfwDestroyWindow(window);

	glfwTerminate();
}