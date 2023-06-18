#include <iostream>
#include "mprpcapplication.h"
#include "user.pb.h"
#include "mprpcchannel.h"
int main(int argc,char *argv[])
{
    //整个程序启动后，使用着通过rpc框架享受rpc服务调用，需要先调用框架的初始化函数（只初始化一次）
    MprpcApplication::Init(argc,argv);

    //演示调用远程发布的rpc方法的Login
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    //rpc方法的请求参数
    fixbug::LoginRequest request;
    request.set_name("zhang san");
    request.set_pwd("123");

    //rpc方法的响应
    fixbug::LoginResponse response;

    //发起rpc方法的调用，同步rpc的rpc调用过程 MprpcChannel::callmethod,以同步阻塞的方式调用
    stub.Login(nullptr,&request,&response,nullptr);

    //一次rpc调用完成，读调用结果
    if(0 == response.result().errcode())    //0意味着什么由双方自行约定
    {
        std::cout << "rpc login response success:" << response.sucess() << std::endl;
    }
    else
    {
        std::cout << "rpc login response erro:" << response.result().errmsg() << std::endl;        
    }
    return 0;
}