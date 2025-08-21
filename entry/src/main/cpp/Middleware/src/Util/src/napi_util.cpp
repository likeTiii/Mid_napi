//
// Created on 2025/5/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "napi_util.h"
#include <cstddef>
#include <cstring>
#include <iostream>
#include <csignal>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <algorithm>
#include <memory>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/json.h>
#include <google/protobuf/message.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor.h>
#include <spdlog/spdlog.h>
#include <hmw/Node.hpp>
#include <hmw/Publisher.hpp>
#include <hmw/Subscriber.hpp>
#include <thread>
//#include "Std/String.pb.h"
namespace beast = boost::beast;
namespace asio = boost::asio;
namespace http = beast::http;
namespace fs = std::filesystem;
#include "hilog/log.h"
#undef LOG_DOMAIN
#undef LOG_TAG
#define LOG_DOMAIN 0x3200
#define LOG_TAG "runCommand"


template <typename T>
T json_list(asio::ip::tcp::socket& socket, const std::string& key, std::ostringstream* log)
{
    beast::flat_buffer buffer;
    http::response<http::dynamic_body> res;

    try {
        http::read(socket, buffer, res);
    } catch (const std::exception& e) {
        if (log) *log << "[HTTP read failed] " << e.what() << "\n";
        return T{};
    }

    std::string response_body = beast::buffers_to_string(res.body().data());
    //if (log) *log << "[HTTP Response body]: " << response_body << "\n";

    T result;
    Json::Reader reader;
    Json::Value jsonData;

    if (response_body.empty()) {
        if (log) *log << "[Error] Response body is empty.\n";
        return result;
    }

    if (!reader.parse(response_body, jsonData)) {
        if (log) *log << "[Error] Failed to parse JSON: " << reader.getFormattedErrorMessages() << "\n";
        return result;
    }

    if constexpr (std::is_same_v<T, std::map<std::string, std::vector<std::string>>>) {
        for (const auto& member : jsonData.getMemberNames()) {
            const Json::Value& array = jsonData[member];
            std::vector<std::string> items;

            if (array.isArray()) {
                for (const auto& item : array) {
                    items.push_back(item.asString());
                }
            }
            result[member] = items;
        }
    } else if constexpr (std::is_same_v<T, std::string>) {
        result = jsonData[key].asString();
    } else {
        if (log) *log << "[Error] Unsupported return type.\n";
    }

    return result;
}

std::string send_list_http(const std::string& host, const std::string& target, const std::string& key) {
    std::ostringstream log_stream;
    try {
        asio::io_context io_context;
        // 解析 TCP 地址
        asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("127.0.0.1", "8080");  // 本地端口与 MasterService 一致
        asio::ip::tcp::socket socket(io_context);
        asio::connect(socket, endpoints);

        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, host);
        http::write(socket, req);
        //log_stream << "[DEBUG] HTTP request sent to target: " << target << "\n";

        // 读取响应
        auto response = json_list<std::map<std::string, std::vector<std::string>>>(
            socket, key, &log_stream
        );

        //log_stream << "[DEBUG] Parsed response from master:\n";

        for (const auto& [host_name, items] : response) {
            log_stream << host_name << ": ";
            if (!items.empty()) {
                for (size_t i = 0; i < items.size(); ++i) {
                    log_stream << items[i];
                    if (i < items.size() - 1) log_stream << ", ";
                }
                log_stream << "\n";
            } else {
                log_stream << "(empty)\n";
            }
        }
        
        socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        socket.close();
        //log_stream << "[DEBUG] Socket shutdown and closed.\n";
    }
    catch (const boost::system::system_error& e) {
        log_stream << "[FATAL] Boost system error: " << e.what() << "\n";
        log_stream << "[DETAIL] Error code: " << e.code() << ", message: " << e.code().message() << "\n";
        log_stream << "[ERRNO] " << strerror(errno) << " (errno=" << errno << ")\n";
    }
    catch (const std::exception& e) {
        log_stream << "[FATAL] std::exception: " << e.what() << "\n";
    }

    return log_stream.str();
}


//发送topic echo请求，返回数据类型
std::string send_echo_http(const std::string& host, const std::string& target, const std::string& key, const std::string& topic_name, std::ostringstream* log){
    std::string type;
    try{
        asio::io_context io_context;
        asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("127.0.0.1", "8080");  // 本地端口与 MasterService 一致
        asio::ip::tcp::socket socket(io_context);
        //OH_LOG_DEBUG(LOG_APP, "[bagrecord] Attempting TCP connection to 127.0.0.1:8080...");
        //*log << "[DEBUG] Attempting TCP connection to 127.0.0.1:8080...\n";
        asio::connect(socket, endpoints);
        //OH_LOG_DEBUG(LOG_APP, "[bagrecord]TCP connected.");
        //*log << "[DEBUG] TCP connected.\n";
        //构造并发送http请求
        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, host);
        req.set("topic",topic_name);
        //OH_LOG_DEBUG(LOG_APP, "[bagrecord]HTTP request sent to target: %{public}s", target.c_str());
        //*log << "[DEBUG] HTTP request sent to target: " << target << "\n";
        
        http::write(socket, req);

        //读取响应
        type = json_list<std::string>(socket, key, log);
        if(type.empty()){
            std::string logStr= "Topic (" + topic_name + ") does not appear to be published yet." ;
            OH_LOG_DEBUG(LOG_APP, "[bagrecord] %{public}s", logStr.c_str());
            *log << "Topic (" << topic_name << ") does not appear to be published yet." << std::endl;
            
        }

        socket.shutdown(boost::asio::local::stream_protocol::socket::shutdown_both);  // 关闭 socket 连接
        socket.close();

    }
    catch (const std::exception& e) {
        *log << "Error: " << e.what() << std::endl;
    }

    return type;
}

//创建订阅者接收消息
void subscribe_and_echo(const std::string& topic_name, 
                        const std::string& message_type,
                        std::ostringstream* log){
    OH_LOG_ERROR(LOG_APP, "[bagrecord] SubscribeMessage");
    const google::protobuf::Descriptor* descriptor = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(message_type);
    if (descriptor == nullptr) {
        //*log << "[ERROR] Cannot found " << message_type<< " in DescriptorPool" << std::endl;
        std::string logStr = "[ERROR] Cannot found " + message_type + " in DescriptorPool\n";
        OH_LOG_ERROR(LOG_APP, "[SubscribeMessage] %{public}s", logStr.c_str());
        return;
    }
    auto echo_node = std::make_shared<Hnu::Middleware::Node>("echo_node");
    //OH_LOG_DEBUG(LOG_APP,"[SubscribeMessage] message_type is %{public}s",message_type.c_str());
    auto subscriber = echo_node->createSubscriber<google::protobuf::Message>(topic_name, message_type, [=](std::shared_ptr<google::protobuf::Message> message){
        //*log << "Received data (" << message_type << "): " << message -> DebugString() << std::endl;
        std::string logStr="Received data (" + message_type + "): " + message->DebugString() + "\n";
        OH_LOG_DEBUG(LOG_APP,"[SubscribeMessage] %{public}s" , logStr.c_str());
    });
    std::thread([echo_node]() {
        echo_node->run();   
    }).detach();
    //echo_node -> run();
}

void subscribe_and_record(const std::string &topic_name, 
                          const std::string &fileDir,
                          const std::string &message_type,
                          std::ostringstream *log) 
{
    //OH_LOG_ERROR(LOG_APP, "[bagrecord] RecordMessage");
    const google::protobuf::Descriptor *descriptor =
        google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(message_type);
    if (descriptor == nullptr) {
        //*log << "[ERROR] Cannot found " << message_type << " in DescriptorPool" << std::endl;
        std::string logStr="[ERROR] Cannot found "+message_type+" in DescriptorPool\n";
        OH_LOG_ERROR(LOG_APP,"[SubscribeMessage] %{public}s", logStr.c_str());
        return;
    }
    
    ////rosbag record功能，根据话题名-时间创建输出文件
    // 获取当前时间转化为年月日时分秒的格式
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm = *std::localtime(&now_c);
    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%Y-%m-%d-%H-%M-%S");
    std::string timestamp = oss.str();
    // 创建对应的文件夹及当前时间点的文件
    fs::path record_dir = fileDir;
    fs::path topic_dir = record_dir / topic_name;
    if (!fs::exists(topic_dir)) {
        fs::create_directories(topic_dir);
    }
    std::string log_file_dir = (topic_dir / (timestamp + ".txt")).string();

    std::ofstream log_file(log_file_dir); // 在第一行记录当前话题发布的数据类型
    if (log_file.is_open()) {
        log_file << "message_type: " << message_type << "\n";
        log_file.close();
    } else {
        std::cerr << "Failed to open log file: " << log_file_dir << std::endl;
    }
    
    //接收消息写入文件中
    auto record_node = std::make_shared<Hnu::Middleware::Node>("record_node");
    auto subscriber = record_node->createSubscriber<google::protobuf::Message>(
        topic_name, message_type, [=](std::shared_ptr<google::protobuf::Message> message) {
            std::string message_value=message->DebugString();
            std::string logStr = "Received data (" + message_type + "): " + message_value + "\n";
            OH_LOG_DEBUG(LOG_APP, "[SubscribeMessage] %{public}s", logStr.c_str());
            //写文件
            auto now = std::chrono::steady_clock::now();
            //判断此日志是否是第一次被写入
            static std::unordered_map<std::string, std::chrono::steady_clock::time_point> start_times;
            auto &start_time = start_times[log_file_dir];
            if (start_time.time_since_epoch().count() == 0) {
                start_time = now; //如果是第一次写入则记录当前时间点为初始时间
            }
            double elapsed_sec = std::chrono::duration<double>(now - start_time).count();//计算每次发布的消息与第一条消息的间隔以便play
            //消息以JSON格式写入
            std::ofstream log_file(log_file_dir, std::ios::app);
            if (log_file.is_open()) {
                // 格式：时间戳|消息内容
                log_file << elapsed_sec << "|" << message_value << "\n";
                log_file.close();
            } else {
                OH_LOG_ERROR(LOG_APP, "Failed to open log file: %{public}s", log_file_dir.c_str());
            }
        });
    
    std::thread([record_node]() { record_node->run(); }).detach();
}
//创建发布者发布消息
template <typename T>
void publish_message(std::shared_ptr<Hnu::Middleware::Node> node,
                     const std::string& topic_name, 
                     const T& message,
                     const std::string& message_type,
                     const std::string& message_value,
                     std::ostringstream* log) {
    auto publisher = node->createPublisher<google::protobuf::Message>(topic_name,message_type);
    publisher->publish(message);
    OH_LOG_ERROR(LOG_APP,"[PublishMessage] %{public}s",message.DebugString().c_str());

//     *log << "Publishing(" << message.GetTypeName() << ") message to topic " << topic_name << ": "
//                 << message.DebugString() << std::endl;
}

//protobuf反射数据类型
std::shared_ptr<google::protobuf::Message> create_protobuf_message(/*std::shared_ptr<Hnu::Middleware::Node> node,*/
    //const std::string& topic_name,
    const std::string& message_type,
    const std::string& message_value,
    std::ostringstream* log
    )
{
    const google::protobuf::Descriptor* descriptor =
        google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(message_type);

    if (descriptor == nullptr) {
        *log << "[ERROR] Cannot found " << message_type
          << " in DescriptorPool" << std::endl;
        return nullptr;
    }

    const google::protobuf::Message* prototype =
        google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);

    if (!prototype) {
        *log << "Cannot create Protobuf message: " << message_type << std::endl;
        return nullptr;
    }

    std::shared_ptr<google::protobuf::Message> message(prototype->New());

    const google::protobuf::Reflection* reflection = message->GetReflection();

    //解析JSON文件并赋值
    Json::Reader reader;
    Json::Value jsonData;
    if(!reader.parse(message_value,jsonData)){
        *log << "Failed to parse json: " << message_value <<std::endl;
        return nullptr;
    }
    // 打印整个 JSON 数据
    Json::StreamWriterBuilder writer;
    std::string jsonStr = Json::writeString(writer, jsonData);
    OH_LOG_ERROR(LOG_APP, "[PublishMessage]Parsed JSON content: %{public}s", jsonStr.c_str());
    for (auto it = jsonData.begin(); it != jsonData.end() ; ++it) {
        const google::protobuf::FieldDescriptor* field = descriptor -> FindFieldByName(it.key().asString());
        if(field == nullptr){
           *log << "Protobuf type(" << message_type << ") is missing the "<< it.key().asString() << "field or has an incorrect type" << std::endl;
            return nullptr;
        }
        reflection -> SetString(message.get(), field, it -> asString());
    }

    return message;
}

//创建发布节点，判断发布频率
std::string create_publisher(
                const std::string& topic_name,
                const std::string& message_type,
                std::shared_ptr<google::protobuf::Message>& message,
                int rate, bool rate_flag,
                const std::string& message_value)
{
    std::ostringstream log_stream;
    std::string uniqueName = "publish_node_" + std::to_string(std::time(nullptr));
    auto pub_node = std::make_shared<Hnu::Middleware::Node>(uniqueName);

    if (rate_flag == 0){
        publish_message(pub_node, topic_name, *message, message_type, message_value,&log_stream);
        raise(SIGINT);
    }
    else{
        auto timer = pub_node->createTimer(rate, [pub_node, topic_name, message, message_type, message_value, &log_stream]() {
            publish_message(pub_node, topic_name, *message, message_type,  message_value,&log_stream);
        });
    }
    std::thread([pub_node]() { pub_node->run(); }).detach();
    
    //pub_node -> run();
    return log_stream.str();
}

std::string data_list(const std::string& host, const std::string& target, const std::string& key) {
    //列出所有话题或节点
    return send_list_http(host, target, key);
}

std::string showTopicEcho(/*std::shared_ptr<Hnu::Middleware::Node> node, */
                    const std::string& host, const std::string& target, 
                    const std::string& key, const std::string& topic_name) {
    //获取话题数据类型
    std::ostringstream log_stream;
    std::string type = send_echo_http(host, target, key, topic_name, &log_stream);
    if(type.empty()){
        log_stream << "Type is empty." << "\n";
        return log_stream.str();
    }
    //创建订阅者并输出话题数据
    subscribe_and_echo(topic_name, type, &log_stream);
    return log_stream.str();
}

std::string publishTopic(/*std::shared_ptr<Hnu::Middleware::Node> node, */
                    const std::string& topic_name, 
                    const std::string& message_type, 
                    const std::string& message_value, 
                    int rate, bool rate_flag) {
    std::ostringstream log_stream;
    //创建发布者
    auto message = create_protobuf_message(message_type, message_value, &log_stream);
    //OH_LOG_ERROR(LOG_APP, "[PublishMessage] message content:\n%{public}s", message->DebugString().c_str());
    if(message){
        return create_publisher(topic_name, message_type, message, rate, rate_flag, message_value);
    }
    else{
        //OH_LOG_ERROR(LOG_APP, "[PublishMessage]Failed to create publisher.");
        log_stream << "Failed to create publisher." << "\n";
        return log_stream.str();
    }
}

std::string bag_record(const std::string& host,
                       const std::string& target,
                       const std::string& key,
                       const std::string& topic_name,
                       const std::string& fileDir){
    std::ostringstream log_stream;
    std::string type = send_echo_http(host, target, key, topic_name, &log_stream);
    if (type.empty()) {
        OH_LOG_DEBUG(LOG_APP,"[bagrecord] type is empty..");
        log_stream << "Type is empty."<< "\n";
        return log_stream.str();
    }
    subscribe_and_record(topic_name, fileDir,type, &log_stream);
    return log_stream.str();
}

std::string runCommand(const std::vector<std::string>& tokens) {
    std::ostringstream log_stream ;
    //log_stream << "test..." ;
    std::string host = "localhost";
    std::string topic_target = "/topic";
    std::string topic_key = "topics";
    std::string node_target = "/node";
    std::string node_key = "nodes";
    std::string echo_target = "/topic/info";
    std::string echo_key = "type";
    int rate = 100;
    bool rate_flag = 1;

    int size = tokens.size();
    if(tokens[0] == "node"){
        if (tokens[1] == "list") {
            if (size == 2){
                return data_list(host, node_target, node_key);
            }
            else {
                log_stream << "Invalid command, press [-h] for more detailed usage" << std::endl;
            }
        } else {
            log_stream << "Invalid command, press [-h] for more detailed usage" << std::endl;
        }
    }
    else if(tokens[0] == "topic"){
        if(tokens[1] == "echo"){
            if (size == 3){
                if (!tokens[2].empty()) {
                    return showTopicEcho(host, echo_target, echo_key,tokens[2]);
                } else {
                    log_stream << "Error: topic name is required for echo" << std::endl;
                }
            }
            else if (size == 2){
                log_stream << "Error: the following argument is required: topic_name" << std::endl;
            }
            else {
                log_stream << "Invalid command, press [-h] for more detailed usage" << std::endl;
            }
        } else if (tokens[1] == "pub") {
            if (size >= 5){
                if (!tokens[2].empty() && !tokens[3].empty() && !tokens[4].empty()) {
                    //auto pub_node = std::make_shared<Hnu::Middleware::Node>("publish_node");
                    //确定发布数据的频率
                    if(size == 6){
                        if(tokens[5] == "--once"){
                            rate_flag = 0;
                        }
                        else if(tokens[5] == "--rate"){
                            log_stream << "Error: rate is required for pub" << std::endl;
                        }
                    }
                    else if(size == 7){
                        if(tokens[5] == "--rate"){
                            rate = std::stoi(tokens[6]);
                        }
                    }
                    return publishTopic(tokens[2], tokens[3], tokens[4], rate, rate_flag);
                } else {
                    log_stream<< "Error: topic name, message type, and message value are required for pub" << std::endl;
                }
            }
            else if(size < 5){
                log_stream << "Error: topic name, message type, and message value are required for pub" << std::endl;
            }
            else {
                log_stream << "Invalid command, press [-h] for more detailed usage" << std::endl;
            }
        }
        else if (tokens[1] == "list") {
            if (size == 2){
                return data_list(host, topic_target, topic_key);
            }
            else {
                log_stream << "Invalid command, press [-h] for more detailed usage" << std::endl;
            }
        }
        else {
            log_stream << "Invalid command, press [-h] for more detailed usage" << std::endl;
        }
    }
    else if(tokens[0] == "bag") {
        if(tokens[1]=="record"){
            if(size == 4){ //在NAPI侧传递存储位置
            //创建订阅者记录
                //std::string path="/data/storage/el2/base/haps/entry/files";
                std::string path = tokens[3];
//                 if (std::filesystem::exists(path)) {
//                     std::string logstr= "Path exists: " + path ;
//                             OH_LOG_DEBUG(LOG_APP,"[bagrecord] %{public}s", logstr.c_str());
//                 } else {
//                     std::string logstr = "Path does NOT exist: " + path;
//                     OH_LOG_DEBUG(LOG_APP, "[bagrecord] %{public}s", logstr.c_str());
//                 }
                return bag_record(host, echo_target, echo_key, tokens[2], path);
            }
            else{
                log_stream << "Error: topic_name is required for record" << std::endl;
            }
        }
        else if (tokens[1] == "play") {
            if (size == 3){
                ////读取对应的文件
                //bag_play(tokens[2]);
            }
            else {
                log_stream << "Error: file_path is required for play" << std::endl;
            }
        }
        else {
            log_stream << "Invalid command, press [-h] for more detailed usage" << std::endl;
        }
    }
    else if(tokens[0] == "-h" || tokens[0] == "--help"){
        log_stream << "Usage: <topic|node|bag> <list|echo|pub|play>.\n\n";
        log_stream << "optional arguments:\n";
        log_stream << "  -h, --help: show this help message and exit\n\n";
        log_stream << "Commands:\n";
        log_stream << "  topic|node list: show all topics or nodes\n";
        log_stream << "  topic echo <topic_name>: Display information for <topic_name>\n";
        log_stream << "  topic pub <topic_name> <message_type> <message_value> (--rate|--once):\n";
        log_stream << "    Publish <message_value> to <topic_name> with <message_type> and optional rate settings\n";
        log_stream << "  bag record <topic_name>: \n";
        log_stream << "    Record data of <topic_name> to a file starting from now.\n";
        log_stream << "  bag play <file_path(topic_name/Y-M-D-H-M-S.txt)>: \n";
        log_stream << "    Replay information for <topic_name> at time <Y-M-D-H-M-S>\n";
    }
    else{
        log_stream << "Unknown command, press [-h] for more detailed usage" << std::endl;
    }
//     std::string test = "testtesttesttest...";
//     return test;
    return log_stream.str();
}