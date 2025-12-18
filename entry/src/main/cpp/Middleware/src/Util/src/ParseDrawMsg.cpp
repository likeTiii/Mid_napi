//
// Created on 2025/12/9.
//

#include "ParseDrawMsg.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection.h>
#include "hilog/log.h"
#include <string>
#include <vector>

#undef LOG_DOMAIN
#undef LOG_TAG
#define LOG_DOMAIN 0x3200
#define LOG_TAG "ParseDrawMsg"

// 声明NAPI层导出的函数（跨文件调用）
extern "C" __attribute__((visibility("default"))) void PostRobotPosition(float x, float y, float z);
extern "C" __attribute__((visibility("default"))) void PostRobotOrientation(float x, float y, float z, float w);
extern "C" __attribute__((visibility("default"))) void PostGlobalPath(const std::vector<float> &pathPoints);

namespace {

// 将protobuf格式的数据转换为C++格式
float ExtractScalar(const google::protobuf::Message &message, const google::protobuf::FieldDescriptor *field)
{
    if (field == nullptr) {
        return 0.0f;
    }
    const auto *reflection = message.GetReflection();
    switch (field->cpp_type()) {
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
            return static_cast<float>(reflection->GetDouble(message, field));
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
            return reflection->GetFloat(message, field);
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
            return static_cast<float>(reflection->GetInt32(message, field));
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
            return static_cast<float>(reflection->GetInt64(message, field));
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
            return static_cast<float>(reflection->GetUInt32(message, field));
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
            return static_cast<float>(reflection->GetUInt64(message, field));
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
            return reflection->GetBool(message, field) ? 1.0f : 0.0f;
        default:
            return 0.0f;
    }
}

void HandleGeometryPoint(const google::protobuf::Message &message) {
    const auto *descriptor = message.GetDescriptor();
    const auto *fieldX = descriptor->FindFieldByName("x");
    const auto *fieldY = descriptor->FindFieldByName("y");
    const auto *fieldZ = descriptor->FindFieldByName("z");
    
    if (fieldX && fieldY && fieldZ) {
        float x = ExtractScalar(message, fieldX);
        float y = ExtractScalar(message, fieldY);
        float z = ExtractScalar(message, fieldZ);
        OH_LOG_INFO(LOG_APP, "[HandleGeometryPoint] Updating Point: (%{public}f, %{public}f, %{public}f)", x, y, z);
        PostRobotPosition(x, y, z);
    } else {
        OH_LOG_ERROR(LOG_APP, "[HandleGeometryPoint] Geometry.Point missing fields");
    }
}

void HandleGeometryQuaternion(const google::protobuf::Message &message) {
    const auto *descriptor = message.GetDescriptor();
    const auto *fieldX = descriptor->FindFieldByName("x");
    const auto *fieldY = descriptor->FindFieldByName("y");
    const auto *fieldZ = descriptor->FindFieldByName("z");
    const auto *fieldW = descriptor->FindFieldByName("w");
    
    if (fieldX && fieldY && fieldZ && fieldW) {
        float x = ExtractScalar(message, fieldX);
        float y = ExtractScalar(message, fieldY);
        float z = ExtractScalar(message, fieldZ);
        float w = ExtractScalar(message, fieldW);
        OH_LOG_INFO(LOG_APP, "[HandleGeometryQuaternion] Updating Quaternion: (%{public}f, %{public}f, %{public}f, %{public}f)", x, y, z, w);
        PostRobotOrientation(x, y, z, w);
    } else {
        OH_LOG_ERROR(LOG_APP, "[HandleGeometryQuaternion] Geometry.Quaternion missing fields");
    }
}

void HandleGeometryPose(const google::protobuf::Message &message) {
    const auto *descriptor = message.GetDescriptor();
    const auto *fieldPos = descriptor->FindFieldByName("position");
    const auto *fieldOri = descriptor->FindFieldByName("orientation");
    
    if (fieldPos && fieldOri) {
        // 获取 position 和 orientation 子消息
        const auto *reflection = message.GetReflection();
        const auto &posMsg = reflection->GetMessage(message, fieldPos);
        const auto &oriMsg = reflection->GetMessage(message, fieldOri);

        // 提取坐标
        const auto *posDesc = posMsg.GetDescriptor();
        float px = ExtractScalar(posMsg, posDesc->FindFieldByName("x"));
        float py = ExtractScalar(posMsg, posDesc->FindFieldByName("y"));
        float pz = ExtractScalar(posMsg, posDesc->FindFieldByName("z"));

        // 提取四元数
        const auto *oriDesc = oriMsg.GetDescriptor();
        float ox = ExtractScalar(oriMsg, oriDesc->FindFieldByName("x"));
        float oy = ExtractScalar(oriMsg, oriDesc->FindFieldByName("y"));
        float oz = ExtractScalar(oriMsg, oriDesc->FindFieldByName("z"));
        float ow = ExtractScalar(oriMsg, oriDesc->FindFieldByName("w"));

        OH_LOG_INFO(LOG_APP, "[HandleGeometryPose] Updating Pose: Pos(%{public}f, %{public}f, %{public}f), Ori(%{public}f, %{public}f, %{public}f, %{public}f)", 
            px, py, pz, ox, oy, oz, ow);
        
        // 更新机器人
        PostRobotPosition(px, py, pz);
        PostRobotOrientation(ox, oy, oz, ow);
    } else {
        OH_LOG_ERROR(LOG_APP, "[HandleGeometryPose] Geometry.Pose missing fields");
    }
}

} // namespace
void HandleTaskPlannerGlobalPath(const google::protobuf::Message &message) {
const auto *descriptor = message.GetDescriptor();

// 1. 获取 PoseArray
const auto *fieldPoseArray = descriptor->FindFieldByName("poseArray");
if (!fieldPoseArray) {
    OH_LOG_ERROR(LOG_APP, "[HandleTaskPlannerGlobalPath] Missing poseArray field");
    return;
}
const auto *reflection = message.GetReflection();
const auto &poseArrayMsg = reflection->GetMessage(message, fieldPoseArray);

// 2. 获取 poses 数组
const auto *poseArrayDesc = poseArrayMsg.GetDescriptor();
const auto *fieldPoses = poseArrayDesc->FindFieldByName("poses");
if (!fieldPoses || !fieldPoses->is_repeated()) {
    OH_LOG_ERROR(LOG_APP, "[HandleTaskPlannerGlobalPath] Missing poses repeated field");
    return;
}

const auto *poseArrayReflection = poseArrayMsg.GetReflection();
int count = poseArrayReflection->FieldSize(poseArrayMsg, fieldPoses);

std::vector<float> pathPoints;
pathPoints.reserve(count * 3);

OH_LOG_INFO(LOG_APP, "[HandleTaskPlannerGlobalPath] Processing path with %{public}d points", count);

for (int i = 0; i < count; ++i) {
    const auto &poseMsg = poseArrayReflection->GetRepeatedMessage(poseArrayMsg, fieldPoses, i);
    const auto *poseDesc = poseMsg.GetDescriptor();
    const auto *fieldPos = poseDesc->FindFieldByName("position");

    if (fieldPos) {
        const auto *poseReflection = poseMsg.GetReflection();
        const auto &posMsg = poseReflection->GetMessage(poseMsg, fieldPos);
        const auto *posDesc = posMsg.GetDescriptor();

        float px = ExtractScalar(posMsg, posDesc->FindFieldByName("x"));
        float py = ExtractScalar(posMsg, posDesc->FindFieldByName("y"));
        float pz = ExtractScalar(posMsg, posDesc->FindFieldByName("z"));

        pathPoints.push_back(px);
        pathPoints.push_back(py);
        pathPoints.push_back(pz);
    }
}

if (!pathPoints.empty()) {
    PostGlobalPath(pathPoints);
}
}
void TryUpdateRobotVisualization(const google::protobuf::Message &message)
{
    std::string typeName = message.GetTypeName();
    OH_LOG_INFO(LOG_APP, "[TryUpdateRobotVisualization] Processing message type: %{public}s", typeName.c_str());

    if (typeName == "Geometry.Point") {
        HandleGeometryPoint(message);
    } else if (typeName == "Geometry.Quaternion") {
        HandleGeometryQuaternion(message);
    } else if (typeName == "Geometry.Pose") {
        HandleGeometryPose(message);
    } else if (typeName == "TaskPlanner.GlobalPathPlanFeedback") {
        HandleTaskPlannerGlobalPath(message);
    }
}
