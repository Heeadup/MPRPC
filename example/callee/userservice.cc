#include <iostream>
#include <string>
#include "user.pb.h"        
#include "mprpcapplication.h"
#include "rpcprovider.h"
using namespace fixbug; //注意添加命名空间，解决名字冲突问题

/*
    将UerService方法本来是一个本地方法，提供了两个进程内的本地方法：Login和GetFriendLists,现要发布成RPC方法
*/
class UerService : public UserServiceRpc    //使用在rpc服务的发布端（rpc服务的提供者）
{
    //本地调用
    bool Login(std::string name,std::string pwd)
    {
        std::cout << "doing local service:Login" << std::endl;
        std::cout << "name: "<<name<< "pwd: "<<pwd<<std::endl;
        return true;
    }

    //重写基类UserServiceRpc中的虚函数，下面的方法都是框架直接调用的
    /*
    1. caller   ===>   Login(LoginRequest)  => muduo =>   callee 
    2. callee   ===>    Login(LoginRequest)  => 交到下面重写的这个Login方法上了
    Login是由框架调用的
    */
    void Login(::google::protobuf::RpcController* controller,
                    const ::fixbug::LoginRequest* request,
                    ::fixbug::LoginResponse* response,
                    ::google::protobuf::Closure* done)
    {
        //框架给业务上报了请求参数LoginRequest,应用获取响应数据做本地业务
        std::string name = request->name();
        std::string pwd  = request->pwd();

        //做本地业务
        bool login_result = Login(name,pwd);

        //把响应写入 响应包括错误码，错误消息，response
        fixbug::ResultCode *code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_sucess(login_result);

        //执行回调 执行响应对象数据的序列化和网络方法(也是由框架来完成的)
        done->Run();

    }

};

int main(int argc,char *argv[])
{
    /*发布服务，使用框架*/

    //调用框架的初始化操作 指令 provider -i config.conf
    MprpcApplication::Init(argc,argv);
    
    //provider是一个rpc网络服务对象,把UerService对象发布到rpc站点
    RpcProvider provider;
    provider.NotifyService(new UerService());
    //启动一个rpc服务发布节点
    provider.Run();

    return 0;
}