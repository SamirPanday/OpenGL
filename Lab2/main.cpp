#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>

using namespace std;


const char* vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec2 aPos;

    void main() {
        // Translate origin from center (500,500)
        // Scale to NDC [-1,1] range
        float x = (aPos.x - 500.0) / 500.0;
        float y = (aPos.y - 500.0) / 500.0; 
        gl_Position = vec4(x, y, 0.0, 1.0);
    }
)glsl";




const char* fragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;
    uniform vec3 color;
    void main() {
        FragColor = vec4(color, 1.0);
    }
)glsl";

// window size
const int WIDTH = 1000;
const int HEIGHT = 1000;

// 2D point structure datatype
struct Point { float x, y; };



// ALGORITHMS

// Function to draw coordinate axes
std::vector<Point> createCoordinateAxes() {
    return {
        {0, HEIGHT / 2}, {WIDTH, HEIGHT / 2},  // X-axis
        {WIDTH / 2, 0}, {WIDTH / 2, HEIGHT}     // Y-axis
    };
}

std::vector<Point> DDA(float x1, float y1, float x2, float y2) {
    std::vector<Point> points;
    float dx = x2 - x1, dy = y2 - y1;
    float steps = std::max(abs(dx), abs(dy));
    float xInc = dx / steps, yInc = dy / steps;
    float x = x1, y = y1;

    for (int i = 0; i <= steps; i++) {
        points.push_back({ round(x), round(y) });
        x += xInc; y += yInc;
    }
    return points;
}

std::vector<Point> Bresenham(float x1, float y1, float x2, float y2) {
    std::vector<Point> points;
    float dx = abs(x2 - x1), dy = abs(y2 - y1);
    bool steep = dy > dx;
    if (steep) 
    {
        std::swap(x1, y1), std::swap(x2, y2), std::swap(dx, dy);
    }
    if (x1 > x2)
    {
        std::swap(x1, x2), std::swap(y1, y2);
    }

    float y = y1, p = 2 * dy - dx;
    for (float x = x1; x <= x2; x++) {
        points.push_back(steep ? Point{ y, x } : Point{ x, y });
        if (p >= 0) y += (y2 > y1 ? 1 : -1), p -= 2 * dx;
        p += 2 * dy;
    }
    return points;
}

std::vector<Point> MidpointCircle(float xc, float yc, float r) {
    std::vector<Point> points;
    float x = 0, y = r, p = 1 - r;
    while (x <= y) {
        points.insert(points.end(), {
            {xc + x, yc + y}, {xc - x, yc + y}, {xc + x, yc - y}, {xc - x, yc - y},
            {xc + y, yc + x}, {xc - y, yc + x}, {xc + y, yc - x}, {xc - y, yc - x}
            });
        x++;
        if (p < 0)
        {
            p += 2 * x + 1;
        }
        else
        {
            y--, p += 2 * (x - y) + 1;
        }
    }
    return points;
}

std::vector<Point> MidpointEllipse(float xc, float yc, float rx, float ry) {
    std::vector<Point> points;
    float rx2 = rx * rx, ry2 = ry * ry;
    float x = 0, y = ry, p = ry2 - rx2 * ry + 0.25f * rx2;

    // Region 1
    while (2 * ry2 * x < 2 * rx2 * y) {
        points.insert(points.end(), {
            {xc + x, yc + y}, {xc - x, yc + y}, {xc + x, yc - y}, {xc - x, yc - y}
            });
        x++;
        if (p < 0) 
        {
            p += 2 * ry2 * x + ry2;
        }
        else 
        {
            y--, p += 2 * ry2 * x - 2 * rx2 * y + ry2;
        }
    }

    // Region 2
    p = ry2 * (x + 0.5f) * (x + 0.5f) + rx2 * (y - 1) * (y - 1) - rx2 * ry2;
    while (y >= 0) {
        points.insert(points.end(), {
            {xc + x, yc + y}, {xc - x, yc + y}, {xc + x, yc - y}, {xc - x, yc - y}
            });
        y--;
        if (p > 0)
        {
            p += -2 * rx2 * y + rx2;
        }
        else
        {
            x++, p += 2 * ry2 * x - 2 * rx2 * y + rx2;
        }
    }
    return points;
}


void drawPoints(const std::vector<Point>& points, unsigned int shader, float r, float g, float b, GLenum mode = GL_POINTS) {
    if (points.empty())
    {
        return;
    }

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(Point), points.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);

    glEnableVertexAttribArray(0);

    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "color"), r, g, b);
    glPointSize(2.0f);
    glDrawArrays(mode, 0, points.size());

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}


//a function to take user input
void GetInput(float& x1d, float& y1d, float& x2d, float& y2d,
    float& x1b, float& y1b, float& x2b, float& y2b,
    float& xc, float& yc, float& r,
    float& xe, float& ye, float& rx, float& ry) 
{
    std::cout << "Enter line coordinates for DDA (x1d y1d x2d y2d): ";
    std::cin >> x1d >> y1d >> x2d >> y2d;

    std::cout << "Enter line coordinates for BLA (x1b y1b x2b y2b): ";
    std::cin >> x1b >> y1b >> x2b >> y2b;

    std::cout << "Enter circle parameters (xc yc radius): ";
    std::cin >> xc >> yc >> r;

    std::cout << "Enter ellipse parameters (xc yc rx ry): ";
    std::cin >> xe >> ye >> rx >> ry;
}



int main() {

    // initializing glfw
    if (!glfwInit())
    {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // create window
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Output Primitives Demo", NULL, NULL);
    if (!window) 
    {
        std::cout << "error creating the window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // now we load glad
    if (!gladLoadGL())
    { 
        glfwTerminate();
        return -1; 
    }

    glViewport(0, 0, WIDTH, HEIGHT);

    //create the shader program
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex shader compilation failed:\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment shader compilation failed:\n" << infoLog << std::endl;
    }

    
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);

    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // get user input
    float x1d, y1d, x2d, y2d, x1b, x2b, y1b, y2b; // line coordinates
    float xc, yc, r; // circle parameters
    float xe, ye, rx, ry; // ellipse parameters

    GetInput(x1d, y1d, x2d, y2d, x1b, y1b, x2b, y2b, xc, yc, r, xe, ye, rx, ry);

    // using the line functions
    auto ddaLine = DDA(x1d+500, y1d+500, x2d+500, y2d+500);
    auto bresLine = Bresenham(x1b+500, y1b+500, x2b+500, y2b+500);
    auto circle = MidpointCircle(xc+500, yc+500, r);
    auto ellipse = MidpointEllipse(xe+500, ye+500, rx, ry);

    // our main loop
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        drawPoints(createCoordinateAxes(), shaderProgram, 1.0f,1.0f,1.0f,GL_LINES);

        drawPoints(ddaLine, shaderProgram,1.0f,0.0f,0.0f); // red DDA
        drawPoints(bresLine, shaderProgram,0.0f,1.0f,0.0f); // green bresenham
        drawPoints(circle, shaderProgram,0.0f,0.0f,1.0f);   // blue circle
        drawPoints(ellipse, shaderProgram,1.0f, 1.0f, 0.0f);  // yellow ellipse

        glfwSwapBuffers(window);
        glfwPollEvents();   
    }

    glfwTerminate();
    return 0;
}