#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<iostream>
#include<vector>
#include<cmath>

using namespace std;

#ifndef M_PI
#define M_PI 3.14159265359
#endif

template<typename T>
T clamp(T value, T minVal, T maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

const char* vertexShaderSource = R"glsl(
#version 330 core
layout(location = 0) in vec2 aPos;
uniform mat3 uTransform;
void main()
{
    vec3 pos = vec3(aPos, 1.0);
    vec3 transformed = uTransform * pos;
    gl_Position = vec4(transformed.xy, 0.0, 1.0);
}
)glsl";

//const char* vertexShaderSource = R"glsl(
//#version 330 core
//layout(location = 0) in vec2 aPos;
//uniform mat3 uTransform;
//void main()
//{
//    vec3 pos = vec3(aPos, 1.0);
//mat3 t = mat3(1.0);
//t[2][0] = 0.5; // Translate x by +0.5
//vec3 transformed = t * pos;
//gl_Position = vec4(transformed.xy, 0.0, 1.0);
//
//}
//)glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;
uniform vec4 uColor;
void main()
{
    FragColor = uColor;
}
)glsl";

const int WIDTH = 1000;
const int HEIGHT = 1000;

struct vertex {
    float x;
    float y;
};

struct Transform {
    float matrix[3][3] = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}
    };
};

Transform MatrixMultiplier(Transform matrix1, Transform matrix2) {
    Transform result;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result.matrix[i][j] = 0.0f;
            for (int k = 0; k < 3; k++) {
                result.matrix[i][j] += matrix1.matrix[i][k] * matrix2.matrix[k][j];
            }
        }
    }
    return result;
}

vector<vertex> userInput() {
    vector<vertex> vertices;
    int sides;
    float x, y;
    cout << "how many sides do you want in your shape (3-10)?" << endl;
    cin >> sides;
    sides = clamp(sides, 3, 10);

    for (int i = 0; i < sides; i++) {
        cout << "enter the coordinate for vertex " << i + 1 << " (x y): ";
        cin >> x >> y;
        vertices.push_back({ x, y });
    }

    for (int i = 0; i < sides; i++) {
        cout << "Vertex " << i + 1 << ": (" << vertices[i].x << ", " << vertices[i].y << ")" << endl;
    }

    return vertices;
}

Transform SelectTransform() {
    int choice;
    cout << "enter 1 for translation,\n2 for rotation,\n3 for scaling,\n4 for reflection,\n5 for shearing,\n6 for composition" << endl;
    cin >> choice;

    switch (choice) {
    case 1: {
        float tx, ty;
        cout << "Enter translation vector (tx ty): ";
        cin >> tx >> ty;
        return {
            {{1.0f, 0.0f, tx},
             {0.0f, 1.0f, ty},
             {0.0f, 0.0f, 1.0f}}
        };
    }
    case 2: {
        float angle;
        cout << "Enter rotation angle in degrees: ";
        cin >> angle;
        float radAngle = angle * (M_PI / 180.0f);
        return {
            {cos(radAngle), -sin(radAngle), 0.0f,
             sin(radAngle), cos(radAngle), 0.0f,
             0.0f, 0.0f, 1.0f}
        };
    }
    case 3: {
        float sx, sy;
        cout << "Enter scaling factors (sx sy): ";
        cin >> sx >> sy;
        return {
            {sx, 0.0f, 0.0f,
             0.0f, sy, 0.0f,
             0.0f, 0.0f, 1.0f}
        };
    }
    case 4: {
        int type;
        cout << "Select (1-5) Reflection about:\n1. X-axis\n2. Y-axis\n3. Origin\n4. y = x\n5. y = -x\n";
        cin >> type;

        switch (type) {
        case 1: // X-axis
            return {
                {1.0f, 0.0f, 0.0f,
                 0.0f, -1.0f, 0.0f,
                 0.0f, 0.0f, 1.0f}
            };
        case 2: // Y-axis
            return {
                {-1.0f, 0.0f, 0.0f,
                  0.0f, 1.0f, 0.0f,
                  0.0f, 0.0f, 1.0f}
            };
        case 3: // Origin
            return {
                {-1.0f, 0.0f, 0.0f,
                  0.0f, -1.0f, 0.0f,
                  0.0f, 0.0f, 1.0f}
            };
        case 4: // y = x
            return {
                {0.0f, 1.0f, 0.0f,
                 1.0f, 0.0f, 0.0f,
                 0.0f, 0.0f, 1.0f}
            };
        case 5: // y = -x
            return {
                {0.0f, -1.0f, 0.0f,
                -1.0f,  0.0f, 0.0f,
                 0.0f,  0.0f, 1.0f}
            };
        default:
            cout << "Invalid reflection type!" << endl;
        }
        break;
    }
    case 5: {
        float shx, shy;
        cout << "Enter shearing factors (shx shy): ";
        cin >> shx >> shy;
        return {
            {1.0f, shx, 0.0f,
             shy, 1.0f, 0.0f,
             0.0f, 0.0f, 1.0f}
        };
    }
    case 6: {
        int n;
        cout << "How many transforms to compose? ";
        cin >> n;
        n = clamp(n, 1, 10);
        Transform result = { {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}} };
        for (int i = 0; i < n; i++) {
            cout << "Select transform " << i + 1 << ":\n";
            Transform t = SelectTransform();
            result = MatrixMultiplier(result, t);
        }
        return result;
    }
    default:
        cout << "Invalid choice!" << endl;
    }

    return { {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}} }; // Fallback
}

void DrawScene(GLuint shaderProgram, const vector<vertex>& points, const Transform& transformMatrix) {
    glUseProgram(shaderProgram);
    GLint transformLoc = glGetUniformLocation(shaderProgram, "uTransform");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");

    // Prepare vertex data
    vector<float> flatVertices;
    for (const auto& v : points) {
        flatVertices.push_back(v.x);
        flatVertices.push_back(v.y);
    }

    // Create and bind VAO/VBO once
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, flatVertices.size() * sizeof(float), flatVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // === Draw Axes (WHITE) ===
    float axisVerts[] = {
        -1.0f, 0.0f, 1.0f, 0.0f,
         0.0f, -1.0f, 0.0f, 1.0f
    };
    GLuint axisVAO, axisVBO;
    glGenVertexArrays(1, &axisVAO);
    glGenBuffers(1, &axisVBO);
    glBindVertexArray(axisVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisVerts), axisVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(axisVAO);
    glUniformMatrix3fv(transformLoc, 1, GL_FALSE, &Transform().matrix[0][0]);
    glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
    glDrawArrays(GL_LINES, 0, 4);

    // === Draw Original Shape (GREEN) ===
    glBindVertexArray(VAO);

    // Create identity matrix
    float identity[9] = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };

    glUniformMatrix3fv(transformLoc, 1, GL_FALSE, identity);
    glUniform4f(colorLoc, 0.0f, 1.0f, 0.0f, 1.0f);
    glDrawArrays(GL_LINE_LOOP, 0, points.size());

    // === Draw Transformed Shape (RED) ===
    // Convert our Transform matrix to column-major format for OpenGL
    float glMatrix[9] = {
        transformMatrix.matrix[0][0], transformMatrix.matrix[1][0], transformMatrix.matrix[2][0],
        transformMatrix.matrix[0][1], transformMatrix.matrix[1][1], transformMatrix.matrix[2][1],
        transformMatrix.matrix[0][2], transformMatrix.matrix[1][2], transformMatrix.matrix[2][2]
    };

    glUniformMatrix3fv(transformLoc, 1, GL_FALSE, glMatrix);
    glUniform4f(colorLoc, 1.0f, 0.0f, 0.0f, 1.0f);
    glDrawArrays(GL_LINE_LOOP, 0, points.size());

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &axisVAO);
    glDeleteBuffers(1, &axisVBO);
}


int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Output Primitives Demo", NULL, NULL);
    if (!window) {
        cout << "error creating the window" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGL()) {
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);

    vector<vertex> userInputPoints = userInput();
    Transform transform = SelectTransform();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cerr << "Vertex shader compilation failed:\n" << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cerr << "Fragment shader compilation failed:\n" << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        DrawScene(shaderProgram, userInputPoints, transform);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
