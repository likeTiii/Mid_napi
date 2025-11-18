#include "egl_core.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>
#include <GLES3/gl3.h>
#include <cmath>
#include <cstdio>
#include <hilog/log.h>

#include "../common/common.h"
#include "plugin_render.h"


constexpr int32_t NUM_4 = 4;
// 用于绘制地面网格的着色器
const char VERTEX_SHADER[] = "#version 300 es\n"
                             "layout(location = 0) in vec3 a_position;\n"
                             "uniform mat4 u_model;                   \n"
                             "uniform mat4 u_view;                    \n"
                             "uniform mat4 u_projection;              \n"
                             "void main()                             \n"
                             "{                                       \n"
                             "   gl_Position = u_projection * u_view *u_model * vec4(a_position, 1.0);  \n"
                             "}                                       \n";


const char FRAGMENT_SHADER[] = "#version 300 es\n"
                               "precision highp float;\n" // 这个用于控制float精度（重要）
                               "out vec4 fragColor;                       \n"
                               "void main()                               \n"
                               "{                                         \n"
                               "   fragColor = vec4(0.5, 0.5, 0.5, 1.0);  \n"
                               "}                                         \n";


// 用于绘制原点三个方向的着色器
const char AXIS_VERTEX_SHADER[] = "#version 300 es\n"
                                  "layout(location = 0) in vec3 a_position;\n"
                                  "uniform mat4 u_model;                   \n"
                                  "uniform mat4 u_view;                    \n"
                                  "uniform mat4 u_projection;              \n"
                                  "void main()                             \n"
                                  "{                                       \n"
                                  "   gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);  \n"
                                  "}                                       \n";

const char AXIS_FRAGMENT_SHADER[] = "#version 300 es\n"
                                    "precision highp float;\n"
                                    "out vec4 fragColor;\n"
                                    "uniform vec4 u_color;\n" // uniform 变量，用于接收颜色
                                    "void main()\n"
                                    "{\n"
                                    "    fragColor = u_color;\n"
                                    "}\n";

// EGL属性
const EGLint ATTRIB_LIST[] = {
    // Key,value.
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 24, // 3D渲染需要用深度缓冲
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
    // End.
    EGL_NONE};
// 指定版本
const EGLint CONTEXT_ATTRIBS[] = {EGL_CONTEXT_CLIENT_VERSION, 3, // 请求GLES 3.0
                                  EGL_NONE};


/**
 * Get context parameter count.
 */
const size_t GET_CONTEXT_PARAM_CNT = 1;


/**
 * Program error.
 */
const GLuint PROGRAM_ERROR = 0;



bool EGLCore::EglContextInit(void *window, int width, int height) {
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EGLCore", "EglContextInit execute");
    if ((window == nullptr) || (width <= 0) || (height <= 0)) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "EglContextInit: param error");
        return false;
    }

    eglWindow_ = (EGLNativeWindowType)window;
    viewportWidth_ = width;
    viewportHeight_ = height;

    // Init display.
    eglDisplay_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (eglDisplay_ == EGL_NO_DISPLAY) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "eglGetDisplay: unable to get EGL display");
        return false;
    }

    EGLint majorVersion;
    EGLint minorVersion;
    if (!eglInitialize(eglDisplay_, &majorVersion, &minorVersion)) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore",
                     "eglInitialize: unable to get initialize EGL display");
        return false;
    }

    // Select configuration.
    const EGLint maxConfigSize = 1;
    EGLint numConfigs;
    if (!eglChooseConfig(eglDisplay_, ATTRIB_LIST, &eglConfig_, maxConfigSize, &numConfigs)) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "eglChooseConfig: unable to choose configs");
        return false;
    }

    return CreateEnvironment();
}

bool EGLCore::CreateEnvironment() {
    // Create surface.
    if (eglWindow_ == NULL) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "eglWindow_ is null");
        return false;
    }
    eglSurface_ = eglCreateWindowSurface(eglDisplay_, eglConfig_, eglWindow_, NULL);
    if (eglSurface_ == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore",
                     "eglCreateWindowSurface: unable to create surface");
        return false;
    }
    // Create context.
    eglContext_ = eglCreateContext(eglDisplay_, eglConfig_, EGL_NO_CONTEXT, CONTEXT_ATTRIBS);
    if (!eglMakeCurrent(eglDisplay_, eglSurface_, eglSurface_, eglContext_)) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "eglMakeCurrent failed");
        return false;
    }

    // 现在上下文已绑定，可以安全设置视口与投影
    if (viewportWidth_ > 0 && viewportHeight_ > 0) {
        UpdateSize(viewportWidth_, viewportHeight_);
    }

    // 创建绘制网格空间所需要的环境
    CreateGridResources();
    // 原点坐标系所需要的环境
    CreateAxisResources();
    //  机器人资源所需要的环境
    CreateRobotResources();
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glLineWidth(2.0f); // 加粗线宽
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EGLCore", "EGL Context and Grid Resources Initialized.");
    return true;
}
/**
 * 网格资源
 */
void EGLCore::CreateGridResources() {
    // 1。创建着色器程序
    program_ = CreateProgram(VERTEX_SHADER, FRAGMENT_SHADER);
    modelLoc_ = glGetUniformLocation(program_, "u_model");
    viewLoc_ = glGetUniformLocation(program_, "u_view");
    projLoc_ = glGetUniformLocation(program_, "u_projection");
    // 2. 顶点网格数据
    std::vector<float> vertices;
    const int GRID_SIZE = 20;
    const int DIVISIONS = 20;
    const float step = (float)(GRID_SIZE * 2) / DIVISIONS;

    for (int i = 0; i <= DIVISIONS; ++i) {
        float pos = -GRID_SIZE + i * step;
        vertices.insert(vertices.end(), {pos, 0.0f, (float)-GRID_SIZE, pos, 0.0f, (float)GRID_SIZE});
        vertices.insert(vertices.end(), {(float)-GRID_SIZE, 0.0f, pos, (float)GRID_SIZE, 0.0f, pos});
    }
    gridVertexCount_ = vertices.size() / 3;
    // 3. 创建缓冲对象和数组对象
    glGenVertexArrays(1, &gridVao_);
    glGenBuffers(1, &gridVbo_);
    glBindVertexArray(gridVao_);
    glBindBuffer(GL_ARRAY_BUFFER, gridVbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
void EGLCore::DeleteGridResources() {
    if (!IsContextReady()) {
        return;
    }
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    if (gridVao_ != 0) {
        glDeleteVertexArrays(1, &gridVao_);
        gridVao_ = 0;
    }
    if (gridVbo_ != 0) {
        glDeleteBuffers(1, &gridVbo_);
        gridVbo_ = 0;
    }
}
/**
 * 坐标轴资源
 */
void EGLCore::CreateAxisResources() {
    // 1. 创建坐标轴的着色器程序
    axisProgram_ = CreateProgram(AXIS_VERTEX_SHADER, AXIS_FRAGMENT_SHADER);
    if (axisProgram_ == PROGRAM_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "Failed to create axis program");
        return;
    }
    // 获取 uniform 变量的位置
    axisModelLoc_ = glGetUniformLocation(axisProgram_, "u_model");
    axisViewLoc_ = glGetUniformLocation(axisProgram_, "u_view");
    axisProjLoc_ = glGetUniformLocation(axisProgram_, "u_projection");
    axisColorLoc_ = glGetUniformLocation(axisProgram_, "u_color");

    // 2. 定义坐标轴的顶点数据 (线段 + 箭头)
    const float axisLength = 2.0f;
    const float arrowheadSize = 0.1f;
    const std::vector<float> axisVertices = {
        // X轴 (红色) - 3条线段, 6个顶点
        0.0f, 0.0f, 0.0f, axisLength, 0.0f, 0.0f,                                 // 主干
        axisLength, 0.0f, 0.0f, axisLength - arrowheadSize, arrowheadSize, 0.0f,  // 箭头一边
        axisLength, 0.0f, 0.0f, axisLength - arrowheadSize, -arrowheadSize, 0.0f, // 箭头另一边

        // Y轴 (绿色) - 3条线段, 6个顶点
        0.0f, 0.0f, 0.0f, 0.0f, axisLength, 0.0f,                                 // 主干
        0.0f, axisLength, 0.0f, arrowheadSize, axisLength - arrowheadSize, 0.0f,  // 箭头一边
        0.0f, axisLength, 0.0f, -arrowheadSize, axisLength - arrowheadSize, 0.0f, // 箭头另一边

        // Z轴 (蓝色) - 3条线段, 6个顶点
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, axisLength,                                // 主干
        0.0f, 0.0f, axisLength, 0.0f, arrowheadSize, axisLength - arrowheadSize, // 箭头一边
        0.0f, 0.0f, axisLength, 0.0f, -arrowheadSize, axisLength - arrowheadSize // 箭头另一边
    };

    // 3. 创建 VAO 和 VBO
    glGenVertexArrays(1, &axisVao_);
    glGenBuffers(1, &axisVbo_);

    glBindVertexArray(axisVao_);
    glBindBuffer(GL_ARRAY_BUFFER, axisVbo_);
    glBufferData(GL_ARRAY_BUFFER, axisVertices.size() * sizeof(float), axisVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // 解绑
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EGLCore", "Axis Resources Created.");
}
void EGLCore::DeleteAxisResources() {
    if (!IsContextReady()) {
        return;
    }
    if (axisProgram_ != 0) {
        glDeleteProgram(axisProgram_);
        axisProgram_ = 0;
    }
    if (axisVao_ != 0) {
        glDeleteVertexArrays(1, &axisVao_);
        axisVao_ = 0;
    }
    if (axisVbo_ != 0) {
        glDeleteBuffers(1, &axisVbo_);
        axisVbo_ = 0;
    }
}
/**
 * 机器人资源
 */
void EGLCore::CreateRobotResources() {
    // 复用坐标轴的着色器程序来绘制带颜色的物体，无需创建新的 program

    // 1. 定义机器人（方块）的顶点数据 (1x1x1大小的立方体)
    const float halfSize = 0.5f;
    const std::vector<float> cubeVertices = {
        // 每个面由两个三角形(6个顶点)组成, 共36个顶点
        // 前面
        -halfSize,
        -halfSize,
        halfSize,
        halfSize,
        -halfSize,
        halfSize,
        halfSize,
        halfSize,
        halfSize,
        halfSize,
        halfSize,
        halfSize,
        -halfSize,
        halfSize,
        halfSize,
        -halfSize,
        -halfSize,
        halfSize,
        // 后面
        -halfSize,
        -halfSize,
        -halfSize,
        -halfSize,
        halfSize,
        -halfSize,
        halfSize,
        halfSize,
        -halfSize,
        halfSize,
        halfSize,
        -halfSize,
        halfSize,
        -halfSize,
        -halfSize,
        -halfSize,
        -halfSize,
        -halfSize,
        // 左面
        -halfSize,
        halfSize,
        halfSize,
        -halfSize,
        halfSize,
        -halfSize,
        -halfSize,
        -halfSize,
        -halfSize,
        -halfSize,
        -halfSize,
        -halfSize,
        -halfSize,
        -halfSize,
        halfSize,
        -halfSize,
        halfSize,
        halfSize,
        // 右面
        halfSize,
        halfSize,
        halfSize,
        halfSize,
        -halfSize,
        halfSize,
        halfSize,
        -halfSize,
        -halfSize,
        halfSize,
        -halfSize,
        -halfSize,
        halfSize,
        halfSize,
        -halfSize,
        halfSize,
        halfSize,
        halfSize,
        // 上面
        -halfSize,
        halfSize,
        halfSize,
        halfSize,
        halfSize,
        halfSize,
        halfSize,
        halfSize,
        -halfSize,
        halfSize,
        halfSize,
        -halfSize,
        -halfSize,
        halfSize,
        -halfSize,
        -halfSize,
        halfSize,
        halfSize,
        // 下面
        -halfSize,
        -halfSize,
        halfSize,
        -halfSize,
        -halfSize,
        -halfSize,
        halfSize,
        -halfSize,
        -halfSize,
        halfSize,
        -halfSize,
        -halfSize,
        halfSize,
        -halfSize,
        halfSize,
        -halfSize,
        -halfSize,
        halfSize,
    };

    // 2. 为方块创建 VAO 和 VBO
    glGenVertexArrays(1, &robotVao_);
    glGenBuffers(1, &robotVbo_);
    glBindVertexArray(robotVao_);
    glBindBuffer(GL_ARRAY_BUFFER, robotVbo_);
    glBufferData(GL_ARRAY_BUFFER, cubeVertices.size() * sizeof(float), cubeVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // 3. 定义机器人自身坐标轴的顶点数据
    const float axisLength = 1.0f;
    const float arrowheadSize = 0.08f;
    const std::vector<float> robotAxisVertices = {
        // X轴 (从方块中心延伸)
        0.0f,
        0.0f,
        0.0f,
        axisLength,
        0.0f,
        0.0f,
        axisLength,
        0.0f,
        0.0f,
        axisLength - arrowheadSize,
        arrowheadSize,
        0.0f,
        axisLength,
        0.0f,
        0.0f,
        axisLength - arrowheadSize,
        -arrowheadSize,
        0.0f,
        // Y轴
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        axisLength,
        0.0f,
        0.0f,
        axisLength,
        0.0f,
        arrowheadSize,
        axisLength - arrowheadSize,
        0.0f,
        0.0f,
        axisLength,
        0.0f,
        -arrowheadSize,
        axisLength - arrowheadSize,
        0.0f,
        // Z轴
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        axisLength,
        0.0f,
        0.0f,
        axisLength,
        0.0f,
        arrowheadSize,
        axisLength - arrowheadSize,
        0.0f,
        0.0f,
        axisLength,
        0.0f,
        -arrowheadSize,
        axisLength - arrowheadSize,
    };

    // 4. 为机器人坐标轴创建 VAO 和 VBO
    glGenVertexArrays(1, &robotAxisVao_);
    glGenBuffers(1, &robotAxisVbo_);
    glBindVertexArray(robotAxisVao_);
    glBindBuffer(GL_ARRAY_BUFFER, robotAxisVbo_);
    glBufferData(GL_ARRAY_BUFFER, robotAxisVertices.size() * sizeof(float), robotAxisVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EGLCore", "Robot Resources Created.");
}
void EGLCore::DeleteRobotResources() {
    if (!IsContextReady()) {
        return;
    }
    if (robotVao_ != 0) {
        glDeleteVertexArrays(1, &robotVao_);
        robotVao_ = 0;
    }
    if (robotVbo_ != 0) {
        glDeleteBuffers(1, &robotVbo_);
        robotVbo_ = 0;
    }
    if (robotAxisVao_ != 0) {
        glDeleteVertexArrays(1, &robotAxisVao_);
        robotAxisVao_ = 0;
    }
    if (robotAxisVbo_ != 0) {
        glDeleteBuffers(1, &robotAxisVbo_);
        robotAxisVbo_ = 0;
    }
}
/**
 * 绘制地面网格
 */
void EGLCore::DrawGrid() {
    if (!IsContextReady()) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "DrawGrid skipped, EGL context not ready");
        return;
    }
    if (!EnsureContextCurrent()) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "DrawGrid skipped, makeCurrent failed");
        return;
    }
    if (program_ == 0 || gridVao_ == 0) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "DrawGrid skipped, program or VAO not ready");
        return;
    }
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(program_);

    // TODO：对摄像机位置进行操控并绑定一些鼠标事件
    // 目前是固定的相机位置
    // glm::vec3 cameraPos = glm::vec3(15.0f, 15.0f, 20.0f); // 摄像机位置
    glm::vec3 cameraPos;
    cameraPos.x = cos(glm::radians(cameraYaw_)) * cos(glm::radians(cameraPitch_)) * cameraDistance_;
    cameraPos.y = sin(glm::radians(cameraPitch_)) * cameraDistance_;
    cameraPos.z = sin(glm::radians(cameraYaw_)) * cos(glm::radians(cameraPitch_)) * cameraDistance_;

    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f); // 观察目标（原点）
    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);     // 上方向，竖直方向
    viewMatrix_ = glm::lookAt(cameraPos, cameraTarget, upVector);

    // 绘制网格
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glUniformMatrix4fv(modelLoc_, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniformMatrix4fv(viewLoc_, 1, GL_FALSE, glm::value_ptr(viewMatrix_));
    glUniformMatrix4fv(projLoc_, 1, GL_FALSE, glm::value_ptr(projectionMatrix_));

    glBindVertexArray(gridVao_);
    glDrawArrays(GL_LINES, 0, gridVertexCount_);
    glBindVertexArray(0);

    // 绘制坐标轴 ---
    if (axisProgram_ == 0 || axisVao_ == 0) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "Axis draw skipped, program or VAO not ready");
        eglSwapBuffers(eglDisplay_, eglSurface_);
        return;
    }
    glUseProgram(axisProgram_);
    // 传递相同的 MVP 矩阵
    glUniformMatrix4fv(axisModelLoc_, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniformMatrix4fv(axisViewLoc_, 1, GL_FALSE, glm::value_ptr(viewMatrix_));
    glUniformMatrix4fv(axisProjLoc_, 1, GL_FALSE, glm::value_ptr(projectionMatrix_));

    glBindVertexArray(axisVao_);
    // 绘制 X 轴 (红色)
    glUniform4f(axisColorLoc_, 1.0f, 0.0f, 0.0f, 1.0f);
    glDrawArrays(GL_LINES, 0, 6);
    // 绘制 Y 轴 (绿色)
    glUniform4f(axisColorLoc_, 0.0f, 1.0f, 0.0f, 1.0f);
    glDrawArrays(GL_LINES, 6, 6);
    // 绘制 Z 轴 (蓝色)
    glUniform4f(axisColorLoc_, 0.0f, 0.0f, 1.0f, 1.0f);
    glDrawArrays(GL_LINES, 12, 6);
    glBindVertexArray(0);

    // 绘制机器人
    //  机器人默认在原点，所以其模型矩阵是单位矩阵。
    //  如果要移动机器人，需要修改这个矩阵 (例如: modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, z));)
    glm::mat4 robotModelMatrix = glm::mat4(1.0f);
    float robotX = 0.0f;
    float robotY = 0.0f;
    float robotZ = 0.0f;
    {
        std::lock_guard<std::mutex> lock(robotMutex_);
        robotX = robotX_;
        robotY = robotY_;
        robotZ = robotZ_;
    }
    //红绿蓝方向。跟坐标位置与网格比例为2：1
    robotModelMatrix = glm::translate(robotModelMatrix, glm::vec3(robotX, robotY, robotZ));
    // 传递机器人模型矩阵给着色器 (视图和投影矩阵不变)
    glUniformMatrix4fv(axisModelLoc_, 1, GL_FALSE, glm::value_ptr(robotModelMatrix));
    // 绘制机器人方块 (例如，使用黄色)
    glUniform4f(axisColorLoc_, 1.0f, 1.0f, 0.0f, 1.0f);
    glBindVertexArray(robotVao_);
    glDrawArrays(GL_TRIANGLES, 0, 36); // 36个顶点
    glBindVertexArray(0);
    // 绘制机器人自身的坐标轴
    glBindVertexArray(robotAxisVao_);
    // X 轴 (红色)
    glUniform4f(axisColorLoc_, 1.0f, 0.0f, 0.0f, 1.0f);
    glDrawArrays(GL_LINES, 0, 6);
    // Y 轴 (绿色)
    glUniform4f(axisColorLoc_, 0.0f, 1.0f, 0.0f, 1.0f);
    glDrawArrays(GL_LINES, 6, 6);
    // Z 轴 (蓝色)
    glUniform4f(axisColorLoc_, 0.0f, 0.0f, 1.0f, 1.0f);
    glDrawArrays(GL_LINES, 12, 6);
    // 全部绘制完毕后解绑VAO
    glBindVertexArray(0);
    
    eglSwapBuffers(eglDisplay_, eglSurface_);
}


    // egl_core.cpp
void EGLCore::MouseTouchEvent(OH_NativeXComponent_MouseEvent mouseEvent) {
    // 使用元素局部坐标，避免跨屏坐标带来的大跨度抖动
    float x = mouseEvent.x;
    float y = mouseEvent.y;
    
    switch (mouseEvent.action) {
        case OH_NATIVEXCOMPONENT_MOUSE_PRESS:
            isDragging_ = true;
            isFirstMove_ = true;
            OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EGLCore", "MouseTouchEvent MouseEvent_Down（鼠标左键被按下）");
            break;

        case OH_NATIVEXCOMPONENT_MOUSE_MOVE:
            if (isDragging_) {
                // 如果是按下左键后的第一次移动
                if (isFirstMove_) {
                    // 将当前位置设为 "上一次" 的位置，然后忽略这次移动
                    lastMouseX_ = x;
                    lastMouseY_ = y;
                    isFirstMove_ = false;
                    break; // 提前退出，不做任何视角计算
                }

                // 从第二次移动开始，执行正常的偏移计算
                float xoffset = x - lastMouseX_;
                float yoffset = lastMouseY_ - y; // Y轴反向，向下拖动是俯视

                // 更新 "上一次" 的位置，为下一次移动做准备
                lastMouseX_ = x;
                lastMouseY_ = y;

                float sensitivity = 0.4f;
                xoffset *= sensitivity;
                yoffset *= sensitivity;

                cameraYaw_ += xoffset;
                cameraPitch_ += yoffset;

            }
            break;

        case OH_NATIVEXCOMPONENT_MOUSE_RELEASE:
            isDragging_ = false;
            OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EGLCore", "MouseTouchEvent MouseEvent_Release");
            break;

        default:
            break;
        }
    
}

void EGLCore::SetRobotPosition(float x, float y, float z)
{
    std::lock_guard<std::mutex> lock(robotMutex_);
    robotX_ = x;
    robotY_ = y;
    robotZ_ = z;
}

void EGLCore::AdjustRobotPosition(float dx, float dy, float dz)
{
    std::lock_guard<std::mutex> lock(robotMutex_);
    robotX_ += dx;
    robotY_ += dy;
    robotZ_ += dz;
}
GLuint EGLCore::LoadShader(GLenum type, const char *shaderSrc) {
    if ((type <= 0) || (shaderSrc == nullptr)) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "glCreateShader type or shaderSrc error");
        return PROGRAM_ERROR;
    }

    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "glCreateShader unable to load shader");
        return PROGRAM_ERROR;
    }

    // The gl function has no return value.
    glShaderSource(shader, 1, &shaderSrc, nullptr);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == 0) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "glCreateShader compiled failed");
    }

    if (compiled != 0) {
        return shader;
    }

    GLint infoLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen <= 1) {
        glDeleteShader(shader);
        return PROGRAM_ERROR;
    }

    char *infoLog = (char *)malloc(sizeof(char) * (infoLen + 1));
    if (infoLog != nullptr) {
        memset(infoLog, 0, infoLen + 1);
        glGetShaderInfoLog(shader, infoLen, nullptr, infoLog);
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "glCompileShader error = %s", infoLog);
        free(infoLog);
        infoLog = nullptr;
    }
    glDeleteShader(shader);
    return PROGRAM_ERROR;
}

GLuint EGLCore::CreateProgram(const char *vertexShader, const char *fragShader) {
    if ((vertexShader == nullptr) || (fragShader == nullptr)) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore",
                     "createProgram: vertexShader or fragShader is null");
        return PROGRAM_ERROR;
    }

    GLuint vertex = LoadShader(GL_VERTEX_SHADER, vertexShader);
    if (vertex == PROGRAM_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "createProgram vertex error");
        return PROGRAM_ERROR;
    }

    GLuint fragment = LoadShader(GL_FRAGMENT_SHADER, fragShader);
    if (fragment == PROGRAM_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "createProgram fragment error");
        return PROGRAM_ERROR;
    }

    GLuint program = glCreateProgram();
    if (program == PROGRAM_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "createProgram program error");
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return PROGRAM_ERROR;
    }

    // The gl function has no return value.
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != 0) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return program;
    }

    OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "createProgram linked error");
    GLint infoLen = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen > 1) {
        char *infoLog = (char *)malloc(sizeof(char) * (infoLen + 1));
        memset(infoLog, 0, infoLen + 1);
        glGetProgramInfoLog(program, infoLen, nullptr, infoLog);
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "glLinkProgram error = %s", infoLog);
        free(infoLog);
        infoLog = nullptr;
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glDeleteProgram(program);
    return PROGRAM_ERROR;
}

void EGLCore::UpdateSize(int width, int height) {
    if (width <= 0 || height <= 0)
        return;
    if (!IsContextReady() || !EnsureContextCurrent()) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "UpdateSize skipped, context not current");
        return;
    }
    glViewport(0, 0, width, height);
    float aspectRatio = (float)width / (float)height;
    projectionMatrix_ = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 200.0f);
}




void EGLCore::Release() {
    // 若上下文可用，先绑定当前上下文，删除 GL 资源
    if (IsContextReady()) {
        if (!eglMakeCurrent(eglDisplay_, eglSurface_, eglSurface_, eglContext_)) {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "Release makeCurrent failed");
        }
        DeleteAxisResources();
        DeleteGridResources();
        DeleteRobotResources();
        // 解绑上下文
        eglMakeCurrent(eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }

    if (eglDisplay_ != EGL_NO_DISPLAY && eglSurface_ != EGL_NO_SURFACE) {
        if (!eglDestroySurface(eglDisplay_, eglSurface_)) {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "Release eglDestroySurface failed");
        }
        eglSurface_ = EGL_NO_SURFACE;
    }

    if (eglDisplay_ != EGL_NO_DISPLAY && eglContext_ != EGL_NO_CONTEXT) {
        if (!eglDestroyContext(eglDisplay_, eglContext_)) {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "Release eglDestroyContext failed");
        }
        eglContext_ = EGL_NO_CONTEXT;
    }

    if (eglDisplay_ != EGL_NO_DISPLAY) {
        if (!eglTerminate(eglDisplay_)) {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EGLCore", "Release eglTerminate failed");
        }
        eglDisplay_ = EGL_NO_DISPLAY;
    }
}