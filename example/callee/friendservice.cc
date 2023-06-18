#include <iostream>
#include <string>
#include "friend.pb.h"        
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include <vector>
#include "logger.h"
using namespace fixbug;

class GetFriendLists : public GetFriendServiceRpc
{
    std::vector<std::string> Getfriend(uint32_t id)
    {
        std::cout << "your id : " << std::endl;
        std::vector<std::string> vec;
        vec.push_back("zhang san");
        vec.push_back("ao di");
        vec.push_back("tian yue");
        return vec;
    }

    void Getfriend(::google::protobuf::RpcController* controller,
                    const ::fixbug::GetFriendRequest* request,
                    ::fixbug::GetFriendResponse* response,
                    ::google::protobuf::Closure* done)
    {
        int id = request->id();
    
        fixbug::GetFriendResultCode *code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        std::vector<std::string>friendList =   Getfriend(id);
        for(std::string &it:friendList)
        {
            std::string * p  = response->add_name();
            *p = it;
        }
        done->Run();
        
    }
};

int main(int argc,char* argv[])
{

    LOG_INFO("first log msgs");
    LOG_ERR("%s:%s:%d",__FILE__,__FUNCTION__,__LINE__); //源文件名，函数，行号
    MprpcApplication::Init(argc,argv);

    RpcProvider provider;
    provider.NotifyService(new GetFriendLists());
    provider.Run();
    return 0;

}