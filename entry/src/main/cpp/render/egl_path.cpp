//
// Created on 2025/12/9.
//

#include "egl_core.h"
#include <vector>

void EGLCore::SetGlobalPath(const std::vector<float> &pathPoints) {
    std::lock_guard<std::mutex> lock(pathMutex_);
    pathPoints_ = pathPoints;
    pathUpdateNeeded_ = true;
}

void EGLCore::CreatePathResources() {
    glGenVertexArrays(1, &pathVao_);
    glGenBuffers(1, &pathVbo_);
}

void EGLCore::DeletePathResources() {
    if (!IsContextReady())
        return;
    if (pathVao_ != 0) {
        glDeleteVertexArrays(1, &pathVao_);
        pathVao_ = 0;
    }
    if (pathVbo_ != 0) {
        glDeleteBuffers(1, &pathVbo_);
        pathVbo_ = 0;
    }
}

void EGLCore::UpdatePathData() {
    std::lock_guard<std::mutex> lock(pathMutex_);
    if (!pathUpdateNeeded_ || pathPoints_.empty())
        return;

    glBindVertexArray(pathVao_);
    glBindBuffer(GL_ARRAY_BUFFER, pathVbo_);
    glBufferData(GL_ARRAY_BUFFER, pathPoints_.size() * sizeof(float), pathPoints_.data(), GL_DYNAMIC_DRAW);

    // 假设每个点只有位置 (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    pathUpdateNeeded_ = false;
}


