//
// Created on 2025/12/9.
//

#include "plugin_render.h"

void PluginRender::BroadcastGlobalPath(const std::vector<float> &pathPoints) {
    for (auto &item : instance_) {
        PluginRender *render = item.second;
        if (render == nullptr || render->eglCore_ == nullptr) {
            continue;
        }
        render->eglCore_->SetGlobalPath(pathPoints);
        if (render->eglCore_->IsContextReady()) {
            render->eglCore_->DrawGrid();
        }
    }
}
