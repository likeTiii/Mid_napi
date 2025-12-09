//
// Created on 2025/12/9.
//

#ifndef MID_NAPI_PARSEDRAWMSG_H
#define MID_NAPI_PARSEDRAWMSG_H

#include <google/protobuf/message.h>

/**
 * @brief 分析消息类型，并调用对应的解析重绘函数
 * 
 * @param message protobuf消息对象
 */
void TryUpdateRobotVisualization(const google::protobuf::Message &message);

#endif //MID_NAPI_PARSEDRAWMSG_H
