/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef NATIVE_XCOMPONENT_EGL_CORE_H
#define NATIVE_XCOMPONENT_EGL_CORE_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <bits/alltypes.h>
#include <unistd.h>
#include <mutex>

// 用于3D数学计算
#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp" // 把 value_ptr 显式拉进来

class EGLCore {
public:
    explicit EGLCore(){};
    ~EGLCore() { Release(); }
    bool EglContextInit(void *window, int width, int height);
    bool CreateEnvironment();


    void Release();
    void UpdateSize(int width, int height);
    void DrawGrid();
    void MouseTouchEvent(OH_NativeXComponent_MouseEvent mouseEvent); // 鼠标事件，适用于鼠标逻辑
    void SetRobotPosition(float x, float y, float z);
    void AdjustRobotPosition(float dx, float dy, float dz);
    inline bool IsContextReady() const
    {
        return eglDisplay_ != EGL_NO_DISPLAY && eglSurface_ != EGL_NO_SURFACE && eglContext_ != EGL_NO_CONTEXT;
    }
    inline bool EnsureContextCurrent()
    {
        if (!IsContextReady()) {
            return false;
        }
        return eglMakeCurrent(eglDisplay_, eglSurface_, eglSurface_, eglContext_) == EGL_TRUE;
    }

private:
    GLuint LoadShader(GLenum type, const char *shaderSrc);
    GLuint CreateProgram(const char *vertexShader, const char *fragShader);
    void CreateGridResources(); // 地面网格资源
    void CreateAxisResources(); // 坐标轴资源
    void CreateRobotResources();    //机器人资源
    void DeleteGridResources();
    void DeleteAxisResources();
    void DeleteRobotResources();


private:
    // EGL核心变量
    EGLNativeWindowType eglWindow_ = NULL;
    EGLDisplay eglDisplay_ = EGL_NO_DISPLAY;
    EGLConfig eglConfig_ = EGL_NO_CONFIG_KHR;
    EGLSurface eglSurface_ = EGL_NO_SURFACE;
    EGLContext eglContext_ = EGL_NO_CONTEXT;
    int viewportWidth_ = 0;
    int viewportHeight_ = 0;

    // 3D渲染变量(网格)
    GLuint program_ = 0;
    GLuint gridVao_ = 0;
    GLuint gridVbo_ = 0;
    int gridVertexCount_ = 0;
    // 着色器uniform变量位置网格中的（在gpu中的内存位置）
    GLint modelLoc_ = -1;
    GLint viewLoc_ = -1;
    GLint projLoc_ = -1;
    // GLM数学对象
    glm::mat4 viewMatrix_ = 0;
    glm::mat4 projectionMatrix_ = 0; // 投影


    // 相机状态
    float cameraYaw_ = -90.0f;     // 偏航角(绕竖直轴旋转角度)，初始向Z轴
    float cameraPitch_ = 30.0f;    // 俯仰角？
    float cameraDistance_ = 25.0f; // 相机离原点的距离

    // 触摸状态
    bool isFirstMove_ = true; // 标志位
    bool isDragging_ = false;
    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;

    // 坐标轴的3D渲染变量 ---
    GLuint axisProgram_ = 0;
    GLuint axisVao_ = 0;
    GLuint axisVbo_ = 0;
    GLint axisModelLoc_ = -1;
    GLint axisViewLoc_ = -1;
    GLint axisProjLoc_ = -1;
    GLint axisColorLoc_ = -1; // 用于传递颜色

    // TODO：暂时用方块代替机器人，位置暂时写在这里，后面封装成类
    // 机器人3D渲染变量
    GLuint robotVao_ = 0;
    GLuint robotVbo_ = 0;
    GLuint robotAxisVao_ = 0;
    GLuint robotAxisVbo_ = 0;
    GLfloat robotX_ = 0.0f;
    GLfloat robotY_ = 0.0f;
    GLfloat robotZ_ = 0.0f;
    std::mutex robotMutex_;
};

#endif // NATIVE_XCOMPONENT_EGL_CORE_H