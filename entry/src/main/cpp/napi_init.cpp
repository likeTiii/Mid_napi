#include "napi/native_api.h"
#include <string>
#include <vector>
#include <regex>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iostream>
#include <cstdio>
#include <thread>
#include <atomic>
#include <spdlog/spdlog.h>
#include "src/Util/src/napi_util.h"
#include "hilog/log.h"
#undef LOG_DOMAIN
#undef LOG_TAG
#define LOG_DOMAIN 0x3200
#define LOG_TAG "NAPI"

namespace fs = std::filesystem;

// //NAPI封装
napi_value RunCommand(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    // 获取输入字符串
    size_t str_size = 0;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &str_size);
    std::string input(str_size + 1, 0);
    napi_get_value_string_utf8(env, args[0], &input[0], str_size + 1, &str_size);
    input.resize(str_size);

    std::vector<std::string> tokens;

    // 匹配被单引号包裹的 JSON 内容
    std::regex jsonRegex(R"###("(\{.*?\})")###"); 
    std::smatch match;

    if (std::regex_search(input, match, jsonRegex)) {
        std::string jsonStr = match[1];  // 获取 JSON 部分
        std::string beforeJson = input.substr(0, match.position());
        std::string afterJson = input.substr(match.position() + match.length());

        // 拆分 JSON 前部分
        std::istringstream iss1(beforeJson);
        std::string token;
        while (iss1 >> token) {
            //OH_LOG_ERROR(LOG_APP, "[PublishMessage] %{public}s", token.c_str());
            tokens.push_back(token);
        }

        tokens.push_back(jsonStr);  // 加入清洗过的 JSON 内容
        //OH_LOG_ERROR(LOG_APP, "[PublishMessage] %{public}s", jsonStr.c_str());
        // 拆分 JSON 后部分
        std::istringstream iss2(afterJson);
        while (iss2 >> token) {
            //OH_LOG_ERROR(LOG_APP, "[PublishMessage] %{public}s", token.c_str());
            tokens.push_back(token);
        }
    } else {
        // 无 JSON，直接按空格分割
        std::istringstream iss(input);
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
    }
    // 如果是bag record再传入路径
    if (tokens.size() >= 2 && tokens[0] == "bag" && tokens[1] == "record") {
        // 这里获取第二个参数 record_path
        if (argc >= 2) {
            napi_value argPath;
            napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
            size_t path_size = 0;
            napi_get_value_string_utf8(env, args[1], nullptr, 0, &path_size);
            std::string record_path(path_size + 1, 0);
            napi_get_value_string_utf8(env, args[1], &record_path[0], path_size + 1, &path_size);
            record_path.resize(path_size);
            OH_LOG_DEBUG(LOG_APP, "[bagrecord] %{public}s", record_path.c_str());
            tokens.push_back(record_path);
        }
    }
    // 调用原始命令函数
    std::string testStr = runCommand(tokens);

    napi_value result;
    napi_create_string_utf8(env, testStr.c_str(), testStr.length(), &result);
    //return nullptr;
    return result;
}

//模块初始化,实现ArkTS接口与C++接口的绑定和映射。
EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
         {"runCommand", nullptr, RunCommand, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
EXTERN_C_END

// 准备模块加载相关信息，将上述Init函数与本模块名等信息记录下来。
static napi_module testModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "entry",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

// 加载so时，该函数会自动被调用，将上述testModule模块注册到系统中。
extern "C" __attribute__((constructor)) void RegisterEntryModule(void)
{
    napi_module_register(&testModule);
}
