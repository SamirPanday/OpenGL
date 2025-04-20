#include<glad/glad.h>
#include<iostream>
#include <GLFW/glfw3.h>

int main()
{
	//initialize the glfw first. this is used to create the window basically. create the environment to run opengl
	glfwInit();

	//glfw doesnt implicitly know what version of opengl we're using so we need to tell it that
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //among the compatibility and core profile options, i chose core.

	//after knowing what version of opengl im on, lets make the actual window where we'll make stuff basically. window obj has a datatype in opengl as seen below.
	GLFWwindow* window =  glfwCreateWindow(800,800,"My Window",NULL,NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create the GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	//glfw is kinda dumb. i had to specify my opengl version. only then i created my window object. now i need to tell it that i want to use that object too. its not just for show.
	//btw context in opengl refers to the state of the program. here im making creating the window the current state so that i can generate a window.
	glfwMakeContextCurrent(window);

	//i want to add a color to my window. so finally, ill use my buttler, GLAD and tell him i need the color and he'll find the reqd configurations and stuff for me.
	gladLoadGL();

	//tell opengl the area of the window to render in
	glViewport(0, 0, 800, 800);

	//clear the window and add the given color. glclearcolor specifies what color to fill when screen is cleared
	glClearColor(0.17f,0.13f,0.17f,1.0f);

	//this one actually clears the screen to the specified color
	glClear(GL_COLOR_BUFFER_BIT);

	//at this point we have the back buffer with the color we want and the front buffer thats still the same as before. we just need to swap these two buffers and boom! we get our colored window!
	glfwSwapBuffers(window);

	while (!glfwWindowShouldClose(window))
	{
		//so without this poll event, my window is just unresponsive. i need to tell glfw to process all the polled events like window appearing, resizing the window, etc
		glfwPollEvents();
		
	}


	//once done with the window, we want to delete it
	glfwDestroyWindow(window);

	//since glfw is initialized, it needs to be terminated as well at some point
	glfwTerminate();
	return 0;
}
