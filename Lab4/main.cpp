#include<glad/glad.h>
#include<iostream>
#include<GLFW/glfw3.h>
#include<vector>
#include<cmath>
#include<algorithm>

// basic shaders
const char* vertexShader2D = "#version 330 core\n"
"layout (location = 0) in vec2 aPos;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
"}\0";

const char* vertexShader3D = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec3 color;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(color, 1.0f);\n"
"}\n\0";

// control variables
bool showClipping = true;
bool showCohenSutherland = true;
bool useWireframe = true;
int currentTransformation = 0;

// simple point struct
struct Point2D {
    float x, y;
    Point2D(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

    void print() const {
        std::cout << "(" << x << ", " << y << ")";
    }
};

// for storing border segments that need to be red
struct BorderSegment {
    Point2D start, end;
    int edge; // 0=left, 1=bottom, 2=right, 3=top

    BorderSegment(Point2D s, Point2D e, int edgeId) : start(s), end(e), edge(edgeId) {}
};

// 4x4 matrix for 3d stuff
struct Matrix4 {
    float m[16];

    Matrix4() {
        // start with identity
        for (int i = 0; i < 16; i++) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    Matrix4 operator*(const Matrix4& other) const {
        Matrix4 result;
        for (int i = 0; i < 16; i++) result.m[i] = 0.0f;

        // matrix multiplication - took me a while to get this right
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                for (int k = 0; k < 4; k++) {
                    result.m[row * 4 + col] += m[row * 4 + k] * other.m[k * 4 + col];
                }
            }
        }
        return result;
    }
};

// cohen sutherland constants
const int INSIDE = 0, LEFT = 1, RIGHT = 2, BOTTOM = 4, TOP = 8;

// clipping window bounds
float xmin = -0.3f, ymin = -0.2f, xmax = 0.3f, ymax = 0.2f;

int computeCode(float x, float y) {
    int code = INSIDE;
    if (x < xmin) code |= LEFT;
    else if (x > xmax) code |= RIGHT;
    if (y < ymin) code |= BOTTOM;
    else if (y > ymax) code |= TOP;
    return code;
}

bool cohenSutherlandClip(float& x1, float& y1, float& x2, float& y2) {
    int code1 = computeCode(x1, y1);
    int code2 = computeCode(x2, y2);
    bool accept = false;

    while (true) {
        if ((code1 | code2) == 0) {
            // both points inside
            accept = true;
            break;
        }
        else if (code1 & code2) {
            // both points outside same region
            break;
        }
        else {
            // need to clip
            int codeOut = code1 ? code1 : code2;
            float x = 0.0f, y = 0.0f;

            if (codeOut & TOP) {
                x = x1 + (x2 - x1) * (ymax - y1) / (y2 - y1);
                y = ymax;
            }
            else if (codeOut & BOTTOM) {
                x = x1 + (x2 - x1) * (ymin - y1) / (y2 - y1);
                y = ymin;
            }
            else if (codeOut & RIGHT) {
                y = y1 + (y2 - y1) * (xmax - x1) / (x2 - x1);
                x = xmax;
            }
            else if (codeOut & LEFT) {
                y = y1 + (y2 - y1) * (xmin - x1) / (x2 - x1);
                x = xmin;
            }

            if (codeOut == code1) {
                x1 = x; y1 = y;
                code1 = computeCode(x1, y1);
            }
            else {
                x2 = x; y2 = y;
                code2 = computeCode(x2, y2);
            }
        }
    }
    return accept;
}

bool isPointInClipWindow(float x, float y) {
    return (x >= xmin && x <= xmax && y >= ymin && y <= ymax);
}

// intersection stuff for border segments
struct IntersectionPoint {
    Point2D point;
    int edge; // which edge of the window

    IntersectionPoint(Point2D p, int e) : point(p), edge(e) {}
};

std::vector<IntersectionPoint> findLineClipIntersectionsWithEdge(Point2D p1, Point2D p2) {
    std::vector<IntersectionPoint> intersections;

    bool p1Inside = isPointInClipWindow(p1.x, p1.y);
    bool p2Inside = isPointInClipWindow(p2.x, p2.y);

    if (p1Inside == p2Inside) {
        return intersections; // no crossing
    }

    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;

    // check each edge of the clipping window

    // left edge
    if ((p1.x < xmin && p2.x >= xmin) || (p1.x >= xmin && p2.x < xmin)) {
        if (abs(dx) > 0.0001f) {
            float t = (xmin - p1.x) / dx;
            float y = p1.y + t * dy;
            if (y >= ymin && y <= ymax) {
                intersections.push_back(IntersectionPoint(Point2D(xmin, y), 0));
            }
        }
    }

    // bottom edge
    if ((p1.y < ymin && p2.y >= ymin) || (p1.y >= ymin && p2.y < ymin)) {
        if (abs(dy) > 0.0001f) {
            float t = (ymin - p1.y) / dy;
            float x = p1.x + t * dx;
            if (x >= xmin && x <= xmax) {
                intersections.push_back(IntersectionPoint(Point2D(x, ymin), 1));
            }
        }
    }

    // right edge
    if ((p1.x < xmax && p2.x >= xmax) || (p1.x >= xmax && p2.x < xmax)) {
        if (abs(dx) > 0.0001f) {
            float t = (xmax - p1.x) / dx;
            float y = p1.y + t * dy;
            if (y >= ymin && y <= ymax) {
                intersections.push_back(IntersectionPoint(Point2D(xmax, y), 2));
            }
        }
    }

    // top edge
    if ((p1.y < ymax && p2.y >= ymax) || (p1.y >= ymax && p2.y < ymax)) {
        if (abs(dy) > 0.0001f) {
            float t = (ymax - p1.y) / dy;
            float x = p1.x + t * dx;
            if (x >= xmin && x <= xmax) {
                intersections.push_back(IntersectionPoint(Point2D(x, ymax), 3));
            }
        }
    }

    return intersections;
}

std::vector<BorderSegment> generateRedBorderSegments(const std::vector<Point2D>& polygon) {
    std::vector<BorderSegment> redSegments;
    std::vector<IntersectionPoint> allIntersections;

    // collect all intersection points
    for (int i = 0; i < polygon.size(); i++) {
        int next = (i + 1) % polygon.size();
        auto intersections = findLineClipIntersectionsWithEdge(polygon[i], polygon[next]);
        allIntersections.insert(allIntersections.end(), intersections.begin(), intersections.end());
    }

    if (allIntersections.size() < 2) return redSegments;

    // group by edge
    std::vector<std::vector<IntersectionPoint>> edgeIntersections(4);
    for (const auto& intersection : allIntersections) {
        edgeIntersections[intersection.edge].push_back(intersection);
    }

    // create segments between pairs on each edge
    for (int edge = 0; edge < 4; edge++) {
        if (edgeIntersections[edge].size() >= 2) {
            // sort points along the edge
            std::sort(edgeIntersections[edge].begin(), edgeIntersections[edge].end(),
                [edge](const IntersectionPoint& a, const IntersectionPoint& b) {
                    if (edge == 0 || edge == 2) { // vertical edges - sort by y
                        return a.point.y < b.point.y;
                    }
                    else { // horizontal edges - sort by x
                        return a.point.x < b.point.x;
                    }
                });

            // make segments between consecutive pairs
            for (int i = 0; i < edgeIntersections[edge].size() - 1; i += 2) {
                if (i + 1 < edgeIntersections[edge].size()) {
                    redSegments.push_back(BorderSegment(
                        edgeIntersections[edge][i].point,
                        edgeIntersections[edge][i + 1].point,
                        edge
                    ));
                }
            }
        }
    }

    return redSegments;
}

// 3d transformation matrices
Matrix4 createTranslation(float x, float y, float z) {
    Matrix4 result;
    result.m[12] = x;
    result.m[13] = y;
    result.m[14] = z;
    return result;
}

Matrix4 createRotationX(float angle) {
    Matrix4 result;
    float c = cosf(angle);
    float s = sinf(angle);
    result.m[5] = c;
    result.m[6] = -s;
    result.m[9] = s;
    result.m[10] = c;
    return result;
}

Matrix4 createRotationY(float angle) {
    Matrix4 result;
    float c = cosf(angle);
    float s = sinf(angle);
    result.m[0] = c;
    result.m[2] = s;
    result.m[8] = -s;
    result.m[10] = c;
    return result;
}

Matrix4 createRotationZ(float angle) {
    Matrix4 result;
    float c = cosf(angle);
    float s = sinf(angle);
    result.m[0] = c;
    result.m[1] = -s;
    result.m[4] = s;
    result.m[5] = c;
    return result;
}

Matrix4 createScaling(float x, float y, float z) {
    Matrix4 result;
    result.m[0] = x;
    result.m[5] = y;
    result.m[10] = z;
    return result;
}

Matrix4 createShearing(float shxy, float shxz, float shyx, float shyz, float shzx, float shzy) {
    Matrix4 result;
    result.m[1] = shxy;
    result.m[2] = shxz;
    result.m[4] = shyx;
    result.m[6] = shyz;
    result.m[8] = shzx;
    result.m[9] = shzy;
    return result;
}

Matrix4 createPerspective(float fov, float aspect, float near, float far) {
    Matrix4 result;
    for (int i = 0; i < 16; i++) result.m[i] = 0.0f;

    float tanHalfFov = tanf(fov / 2.0f);
    result.m[0] = 1.0f / (aspect * tanHalfFov);
    result.m[5] = 1.0f / tanHalfFov;
    result.m[10] = -(far + near) / (far - near);
    result.m[11] = -1.0f;
    result.m[14] = -(2.0f * far * near) / (far - near);
    return result;
}

Matrix4 createLookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ) {
    // calculate camera vectors
    float fx = centerX - eyeX;
    float fy = centerY - eyeY;
    float fz = centerZ - eyeZ;

    // normalize forward
    float flen = sqrtf(fx * fx + fy * fy + fz * fz);
    fx /= flen; fy /= flen; fz /= flen;

    // right vector (forward cross up)
    float rx = fy * upZ - fz * upY;
    float ry = fz * upX - fx * upZ;
    float rz = fx * upY - fy * upX;

    // normalize right
    float rlen = sqrtf(rx * rx + ry * ry + rz * rz);
    rx /= rlen; ry /= rlen; rz /= rlen;

    // up vector (right cross forward)
    float ux = ry * fz - rz * fy;
    float uy = rz * fx - rx * fz;
    float uz = rx * fy - ry * fx;

    Matrix4 result;
    result.m[0] = rx;  result.m[1] = ux;  result.m[2] = -fx; result.m[3] = 0.0f;
    result.m[4] = ry;  result.m[5] = uy;  result.m[6] = -fy; result.m[7] = 0.0f;
    result.m[8] = rz;  result.m[9] = uz;  result.m[10] = -fz; result.m[11] = 0.0f;
    result.m[12] = -(rx * eyeX + ry * eyeY + rz * eyeZ);
    result.m[13] = -(ux * eyeX + uy * eyeY + uz * eyeZ);
    result.m[14] = -(-fx * eyeX + -fy * eyeY + -fz * eyeZ);
    result.m[15] = 1.0f;

    return result;
}

// opengl helper functions
GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

void drawLine(GLuint shader, float x1, float y1, float x2, float y2, float r, float g, float b, float lineWidth = 1.0f) {
    float vertices[] = { x1, y1, x2, y2 };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "color"), r, g, b);

    if (lineWidth > 1.0f) {
        glLineWidth(lineWidth);
    }

    glDrawArrays(GL_LINES, 0, 2);

    if (lineWidth > 1.0f) {
        glLineWidth(1.0f); // reset
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void drawPolygon(GLuint shader, const std::vector<Point2D>& points, float r, float g, float b, bool filled = false, float lineWidth = 1.0f) {
    if (points.empty()) return;

    std::vector<float> vertices;
    for (const auto& point : points) {
        vertices.push_back(point.x);
        vertices.push_back(point.y);
    }

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "color"), r, g, b);

    if (lineWidth > 1.0f) {
        glLineWidth(lineWidth);
    }

    glDrawArrays(filled ? GL_TRIANGLE_FAN : GL_LINE_LOOP, 0, static_cast<GLsizei>(points.size()));

    if (lineWidth > 1.0f) {
        glLineWidth(1.0f);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void drawWireframeCube(GLuint shader, const Matrix4& model, const Matrix4& view, const Matrix4& projection, float r, float g, float b) {
    float vertices[] = {
        // cube edges as line segments
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,

        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f,

        -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(shader);
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, model.m);
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, view.m);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, projection.m);
    glUniform3f(glGetUniformLocation(shader, "color"), r, g, b);

    glDrawArrays(GL_LINES, 0, 24);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

// keyboard input handling
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, true);
            break;
        case GLFW_KEY_SPACE:
            showClipping = !showClipping;
            currentTransformation = 0;
            std::cout << (showClipping ? "\n>>> 2d clipping mode <<<" : "\n>>> 3d transformations mode <<<") << std::endl;
            break;
        case GLFW_KEY_C:
            if (showClipping) {
                showCohenSutherland = !showCohenSutherland;
                std::cout << (showCohenSutherland ? ">>> cohen-sutherland line clipping <<<" : ">>> sutherland-hodgman polygon clipping <<<") << std::endl;
            }
            break;
        case GLFW_KEY_W:
            if (!showClipping) {
                useWireframe = !useWireframe;
                std::cout << (useWireframe ? ">>> wireframe mode <<<" : ">>> solid mode <<<") << std::endl;
            }
            break;
        case GLFW_KEY_T:
            if (!showClipping) {
                currentTransformation = 0;
                std::cout << ">>> translation transformation <<<" << std::endl;
            }
            break;
        case GLFW_KEY_R:
            if (!showClipping) {
                currentTransformation = 1;
                std::cout << ">>> rotation transformation <<<" << std::endl;
            }
            break;
        case GLFW_KEY_S:
            if (!showClipping) {
                currentTransformation = 2;
                std::cout << ">>> scaling transformation <<<" << std::endl;
            }
            break;
        case GLFW_KEY_H:
            if (!showClipping) {
                currentTransformation = 3;
                std::cout << ">>> shearing transformation <<<" << std::endl;
            }
            break;
        }
    }
}

// main program
int main() {

    std::cout << "\nclipping visualization:" << std::endl;
    std::cout << "- white outline = clipping window boundary" << std::endl;
    std::cout << "- gray parts = parts outside clipping window (rejected)" << std::endl;
    std::cout << "- red parts = parts inside clipping window (accepted)" << std::endl;
    std::cout << "- red border segments = window border between intersections" << std::endl;
    std::cout << "\n============================================================" << std::endl;
    std::cout << "controls:" << std::endl;
    std::cout << "  SPACE   - toggle between 2d clipping and 3d transformations" << std::endl;
    std::cout << "  C       - toggle clipping algorithms (in 2d mode)" << std::endl;
    std::cout << "  T/R/S/H - select transformation type (in 3d mode)" << std::endl;
    std::cout << "  W       - toggle wireframe/solid (in 3d mode)" << std::endl;
    std::cout << "  ESC     - exit" << std::endl;
    std::cout << "============================================================" << std::endl << std::endl;

    // init glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1200, 900, "lab 5: enhanced clipping - samirpanday", NULL, NULL);
    if (window == NULL) {
        std::cout << "failed to create glfw window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    gladLoadGL();
    glViewport(0, 0, 1200, 900);
    glEnable(GL_DEPTH_TEST);
    glfwSetKeyCallback(window, key_callback);

    // create shaders
    GLuint shader2D = createShaderProgram(vertexShader2D, fragmentShaderSource);
    GLuint shader3D = createShaderProgram(vertexShader3D, fragmentShaderSource);

    // test lines for clipping
    std::vector<std::pair<Point2D, Point2D>> testLines = {
        {Point2D(-0.6f, -0.05f), Point2D(0.6f, 0.05f)},
        {Point2D(-0.05f, -0.4f), Point2D(0.05f, 0.4f)},
        {Point2D(-0.4f, -0.3f), Point2D(0.4f, 0.3f)},
        {Point2D(-0.5f, 0.25f), Point2D(0.5f, 0.25f)}
    };

    // test polygon that crosses the clipping window
    std::vector<Point2D> testPolygon = {
        Point2D(-0.6f, -0.05f),  // outside
        Point2D(-0.1f, -0.05f),  // inside
        Point2D(0.1f, -0.05f),  // inside
        Point2D(0.6f, -0.05f),  // outside
        Point2D(0.6f,  0.05f),  // outside
        Point2D(0.1f,  0.05f),  // inside
        Point2D(-0.1f,  0.05f),  // inside
        Point2D(-0.6f,  0.05f)   // outside
    };

    std::cout << "starting lab 5 demo..." << std::endl;
    std::cout << "clipping window: (" << xmin << ", " << ymin << ") to (" << xmax << ", " << ymax << ")" << std::endl;
    std::cout << ">>> 2d clipping mode <<<" << std::endl;
    std::cout << ">>> cohen-sutherland line clipping <<<" << std::endl;

    // main loop
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (showClipping) {
            // 2d clipping mode
            glDisable(GL_DEPTH_TEST);

            if (showCohenSutherland) {
                // line clipping demo

                // draw clipping window
                std::vector<Point2D> clipWindow = {
                    Point2D(xmin, ymin), Point2D(xmax, ymin),
                    Point2D(xmax, ymax), Point2D(xmin, ymax)
                };
                drawPolygon(shader2D, clipWindow, 1.0f, 1.0f, 1.0f, false, 3.0f);

                // get red border segments for lines
                std::vector<Point2D> linePoints;
                for (const auto& line : testLines) {
                    linePoints.push_back(line.first);
                    linePoints.push_back(line.second);
                }
                auto redBorderSegments = generateRedBorderSegments(linePoints);

                // draw red border segments
                for (const auto& segment : redBorderSegments) {
                    drawLine(shader2D, segment.start.x, segment.start.y, segment.end.x, segment.end.y, 1.0f, 0.0f, 0.0f, 6.0f);
                }

                for (const auto& line : testLines) {
                    // draw full line in gray
                    drawLine(shader2D, line.first.x, line.first.y, line.second.x, line.second.y, 0.5f, 0.5f, 0.5f, 1.0f);

                    // clip and draw inside part in red
                    float x1 = line.first.x, y1 = line.first.y;
                    float x2 = line.second.x, y2 = line.second.y;
                    if (cohenSutherlandClip(x1, y1, x2, y2)) {
                        drawLine(shader2D, x1, y1, x2, y2, 1.0f, 0.2f, 0.2f, 3.0f);
                    }
                }

            }
            else {
                // polygon clipping demo

                // draw clipping window
                std::vector<Point2D> clipWindow = {
                    Point2D(xmin, ymin), Point2D(xmax, ymin),
                    Point2D(xmax, ymax), Point2D(xmin, ymax)
                };
                drawPolygon(shader2D, clipWindow, 1.0f, 1.0f, 1.0f, false, 3.0f);

                // get and draw red border segments
                auto redBorderSegments = generateRedBorderSegments(testPolygon);
                for (const auto& segment : redBorderSegments) {
                    drawLine(shader2D, segment.start.x, segment.start.y, segment.end.x, segment.end.y, 1.0f, 0.0f, 0.0f, 6.0f);
                }

                // draw polygon edges with proper colors
                for (int i = 0; i < testPolygon.size(); i++) {
                    int next = (i + 1) % testPolygon.size();

                    Point2D p1 = testPolygon[i];
                    Point2D p2 = testPolygon[next];

                    bool p1Inside = isPointInClipWindow(p1.x, p1.y);
                    bool p2Inside = isPointInClipWindow(p2.x, p2.y);

                    if (p1Inside && p2Inside) {
                        // both inside - red
                        drawLine(shader2D, p1.x, p1.y, p2.x, p2.y, 1.0f, 0.0f, 0.0f, 4.0f);
                    }
                    else if (!p1Inside && !p2Inside) {
                        // both outside - gray
                        drawLine(shader2D, p1.x, p1.y, p2.x, p2.y, 0.5f, 0.5f, 0.5f, 4.0f);
                    }
                    else {
                        // crossing - gray background, red clipped part
                        drawLine(shader2D, p1.x, p1.y, p2.x, p2.y, 0.5f, 0.5f, 0.5f, 2.0f);

                        float x1 = p1.x, y1 = p1.y, x2 = p2.x, y2 = p2.y;
                        if (cohenSutherlandClip(x1, y1, x2, y2)) {
                            drawLine(shader2D, x1, y1, x2, y2, 1.0f, 0.0f, 0.0f, 4.0f);
                        }
                    }
                }
            }
        }
        else {
            // 3d transformations mode
            glEnable(GL_DEPTH_TEST);

            Matrix4 projection = createPerspective(45.0f * 3.14159f / 180.0f, 1200.0f / 900.0f, 0.1f, 100.0f);
            Matrix4 view = createLookAt(0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

            switch (currentTransformation) {
            case 0: { // translation
                Matrix4 identity;
                drawWireframeCube(shader3D, identity, view, projection, 0.7f, 0.7f, 0.7f);

                Matrix4 translation = createTranslation(2.0f, 0.0f, 0.0f);
                drawWireframeCube(shader3D, translation, view, projection, 1.0f, 0.3f, 0.3f);
                break;
            }
            case 1: { // rotation
                Matrix4 identity;
                drawWireframeCube(shader3D, identity, view, projection, 0.7f, 0.7f, 0.7f);

                Matrix4 rotation = createRotationY(0.8f) * createRotationX(0.5f);
                Matrix4 rotatedModel = createTranslation(2.0f, 0.0f, 0.0f) * rotation;
                drawWireframeCube(shader3D, rotatedModel, view, projection, 0.3f, 1.0f, 0.3f);
                break;
            }
            case 2: { // scaling
                Matrix4 identity;
                drawWireframeCube(shader3D, identity, view, projection, 0.7f, 0.7f, 0.7f);

                Matrix4 scaling = createScaling(0.5f, 1.5f, 0.5f);
                Matrix4 scaledModel = createTranslation(2.0f, 0.0f, 0.0f) * scaling;
                drawWireframeCube(shader3D, scaledModel, view, projection, 0.3f, 0.3f, 1.0f);
                break;
            }
            case 3: { // shearing
                Matrix4 identity;
                drawWireframeCube(shader3D, identity, view, projection, 0.7f, 0.7f, 0.7f);

                Matrix4 shearing = createShearing(0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                Matrix4 shearedModel = createTranslation(2.0f, 0.0f, 0.0f) * shearing;
                drawWireframeCube(shader3D, shearedModel, view, projection, 1.0f, 1.0f, 0.3f);
                break;
            }
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(shader2D);
    glDeleteProgram(shader3D);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}