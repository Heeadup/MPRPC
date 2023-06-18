#pragma once
#include "google/protobuf/service.h"
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <string>
#include "mprpcapplication.h"
#include <functional>
#include <iostream>
#include <google/protobuf/descriptor.h>
#include <unordered_map>
//框架提供的专门负责发布rpc服务的网络对象类
class  RpcProvider
{
public:
    // 这里是框架提供给外部使用的，可以发布rpc方法的函数接口
    void NotifyService(google::protobuf::Service *service);

    //启动rpc节点，开始提供rpc远程网络调用服务,组合了TcpServer
    void Run();
private:
    //组合了Eventloop
    muduo::net::EventLoop m_eventloop;

    //service服务类型信息
    struct ServiceInfo
    {
        google::protobuf::Service *m_service;   //保存服务对象
        std::unordered_map<std::string,const google::protobuf::MethodDescriptor*> m_methodMap;      //保存服务对象的方法
    };

    std::unordered_map<std::string,ServiceInfo> m_serviceMap;       //存储注册成功的服务对象和其服务方法的所有信息
    //组合了tcpserver
    std::unique_ptr<muduo::net::TcpServer> m_tcpserverPtr;
    //新的socket连接回调
    void OnConnection(const muduo::net::TcpConnectionPtr&);
    //已建立连接用户的读写事件回调
    void OnMessage(const muduo::net::TcpConnectionPtr&, muduo::net::Buffer*,muduo::Timestamp);
    //Closure回调操作，用于序列化rpc响应和网络发送
    void  SendRpcResponse(const muduo::net::TcpConnectionPtr&,google::protobuf::Message*);
};