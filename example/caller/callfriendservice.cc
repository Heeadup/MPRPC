#include <iostream>
#include "mprpcapplication.h"
#include "friend.pb.h"
int main(int argc,char* argv[])
{
    MprpcApplication::Init(argc,argv);
    fixbug::GetFriendServiceRpc_Stub stub(new MprpcChannel());

    fixbug::GetFriendRequest request;
    request.set_id(100000);

    fixbug::GetFriendResponse response;
    MprpcController controller;

    stub.Getfriend(&controller,&request,&response,nullptr);

    if (controller.Failed())        //rpc框架内部发生错误
    {
        std::cout << controller.ErrorText() << std::endl;
    }
    else
    {
        if (0 == response.result().errcode())
        {
            int size = response.name_size();
            for (int i = 0; i < size; i++)
            {
                std::cout << "index: " << i << "name: " << response.name(i) << std::endl;
            }
        }
        else
        {
            std::cout << "rpc login response erro:" << response.result().errmsg() << std::endl;
        }
    }

    return 0;
}