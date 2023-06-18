#include "mprpcchannel.h"
#include <string>
#include <iostream>
#include "rpcheader.pb.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "mprpcapplication.h"
#include <unistd.h>
#include <errno.h>
#include "zookeeperutil.h"
/*
    header_size + service_name method_name args_size + args
*/
//通过stub代理对象调用的rpc方法，统一做rpc方法调用的数据序列化和网络发送
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor * method,
                        google::protobuf::RpcController* controller,
                        const google::protobuf::Message* request,
                        google::protobuf::Message* response,
                        google::protobuf::Closure* done)
{
    const google::protobuf::ServiceDescriptor* sd = method->service();
    std::string service_name = sd->name();      //service_name
    std::string method_name  = method->name(); //ethod_name

    //获取参数的序列化字符串长度args_size
    int args_size = 0;
    std::string args_str;

    if (request->SerializeToString(&args_str))
    {
        args_size = args_str.size();
    }
    else
    {
        controller->SetFailed("serialize request erro!");
        return;
    }

    //定义rpc的请求header,填rpcheader
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);

    uint32_t header_size = 0;
    std::string rpc_header_str;
    if(rpcHeader.SerializeToString(&rpc_header_str))
    {
        header_size = rpc_header_str.size();
    }
    else
    {
        controller->SetFailed("serialize rpcHeader erro!");
        return;
    }

    //组织待发送的rpc请求的字符串
    std::string send_rpc_str;
    send_rpc_str.insert(0,std::string((char*)&header_size,4)); //从0开始插入，插入4个字节的长度，将二进制的header_size写进去
    send_rpc_str += rpc_header_str; //rpc_header
    send_rpc_str += args_str;       //args_str

    std::cout <<"==================================================="<<std::endl;
    std::cout << "header_size: "  << header_size << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "hargs_size: " << args_size << std::endl;
    std::cout << "args_str: "   << args_str << std::endl;
    std::cout <<"==================================================="<<std::endl;

    /*
        上面是完成调用者的序列化部分，接下来使用tcp编程完成远程方法的调用
    */
    //创建socket
    int clientfd = socket(AF_INET,SOCK_STREAM,0);
    if(clientfd == -1)
    {
        char errotxt[512] = {0};    //组合字符串
        sprintf(errotxt,"creat socket erro! errno: %d",errno);
        controller->SetFailed(errotxt);
        exit(EXIT_FAILURE);
    }
    // //读取配置文件rpcserver的信息
    // //绑定地址信息
    // //ip地址
    // std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    // // port端口号,转成整数
    // uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    
    /*
        将ip由配置文件中读取改为从zk上发现
        首先组装好路径，然后去zk上查询
    */
    ZkClient zkCli;
    zkCli.Start();
    // /UserServiceRpc/Login 组装节点路径
    std::string method_path = "/" + service_name + "/" + method_name;
    // 获取ip地址
    std::string host_data   = zkCli.GetData(method_path.c_str());
    if(host_data == "")
    {
        controller->SetFailed(method_path + "address not exit!");
        return;
    } 
    int idx = host_data.find(":");
    if(idx == -1)
    {
        controller->SetFailed(method_path + "address is not valid");
        return;
    }
    std::string ip = host_data.substr(0,idx);
    uint16_t port = atoi(host_data.substr(idx + 1, host_data.size() - idx).c_str());  

    struct sockaddr_in server_addr;
    server_addr.sin_family =  AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    //连接rpc服务节点
    if(-1 == connect(clientfd,(struct sockaddr*)&server_addr,sizeof(server_addr))) 
    {
        close(clientfd);
        char errotxt[512] = {0};    //组合字符串
        sprintf(errotxt,"creat connect erro! errno:  %d",errno);
        controller->SetFailed(errotxt);
        return;
    }

    //发送rpc请求
    if(-1 == send(clientfd,send_rpc_str.c_str(),send_rpc_str.size(),0)) 
    {
        close(clientfd);
        char errotxt[512] = {0};    //组合字符串
        sprintf(errotxt,"send erro! errno::  %d",errno);
        controller->SetFailed(errotxt);
        return;
    } 

    //接收rpc请求的响应值
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if(-1 == (recv_size = recv(clientfd,recv_buf,1024,0))) 
    {
        close(clientfd);
        char errotxt[512] = {0};    //组合字符串
        sprintf(errotxt,"recv erro! errno:  %d",errno);
        controller->SetFailed(errotxt);
        return;
    } 

    /*
        接收到响应之后，将得到的数据填入response中
    */
    //反序列化rpc调用的响应数据
    //std::string response_str(recv_buf,0,recv_size); //bug出现问题，string 构造函数出现问题，recv_buf中含有\0，导致其后面的数据存不下来，导致反序列化失败
    //if(!response->ParseFromString(response_str))      //解决方案，直接由数组进行序列化
    if(!response->ParseFromArray(recv_buf,recv_size))
    {
        close(clientfd);
        char errotxt[512] = {0};    //组合字符串
        sprintf(errotxt,"parse erro! errno:  %d",errno);
        controller->SetFailed(errotxt);
        return;       
    }


}