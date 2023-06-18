#include "rpcprovider.h"
#include "rpcheader.pb.h"
#include "logger.h"
#include "zookeeperutil.h"
#include <string.h>
/*
    由service_name 查表 得到 service描述 => service* 记录服务对象 
        在由 method_name 查表的 method方法
*/
// 这里是框架提供给外部使用的，可以发布rpc方法的函数接口,用于定位rpc调用的对象与方法
void RpcProvider::NotifyService(google::protobuf::Service *service)
{
    ServiceInfo service_info;
    //获取服务对象的描述信息
    const google::protobuf::ServiceDescriptor *pserviceDisc= service->GetDescriptor();

    //获取服务对象的名字
    std::string service_name = pserviceDisc->name();
    //获取服务对象方法的数量
    int methodCnt = pserviceDisc->method_count();
    LOG_INFO("service_name:%s",service_name.c_str());
    //std::cout << "service_name: "<<service_name <<std::endl;
    for(int i = 0;i < methodCnt;i++)
    {
        //获取服务对象指定下标的服务方法的描述（抽象描述）
        const google::protobuf::MethodDescriptor* p_methodDisc= pserviceDisc->method(i);
        std::string method_name =  p_methodDisc->name();
        service_info.m_methodMap.insert({method_name,p_methodDisc});

        //std::cout << "method_name: "<<method_name <<std::endl;
        LOG_INFO("method_name:%s",method_name.c_str());
    }
    service_info.m_service = service;
    m_serviceMap.insert({service_name,service_info});
}

//启动rpc节点，开始提供rpc远程网络调用服务,组合了TcpServer，相当于启动了epoll+多线程服务器
void RpcProvider::Run()
{
    /*muduo的调用流程*/
    //ip地址
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    // port端口号,转成整数
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());

    muduo::net::InetAddress address(ip,port);
    
    // 创建TcpServer对象
    muduo::net::TcpServer server(&m_eventloop,address,"RpcProvider");
    //绑定连接回调和消息读写回调方法 muduo库的优点：分离了网络代码和业务代码
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection,this,std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
    //设置muduo库的线程数量
    server.setThreadNum(4);

    //把当前rpc节点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务，（ps：zklient创建完后不断开）
    //session timeout 设置30s，ZkClient 的网络IO线程 以1/3 timeout 发送ping消息作为心跳消息
    ZkClient zkcli;
    zkcli.Start();

    //service_name 为永久性节点， method_name为临时性节点
    for(auto &sp:m_serviceMap)
    {
        // /service_name 为 /UserServiceRpc
        std::string service_path = "/" + sp.first;
        zkcli.Create(service_path.c_str(),nullptr,0);
        for(auto &mp:sp.second.m_methodMap)
        {
            // /service_name/method_name  /UserServiceRpc/Login 存储的信息为method_path_data，其中包含当前rpc服务节点主机的ip和端口号
            std::string method_path = service_path + "/" +  mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data,"%s:%d",ip.c_str(),port);
            //znode是临时性节点，ZOO_EPHEMERAL（因为尽量需要保证ip和port的有效性）
            zkcli.Create(method_path.c_str(),method_path_data,strlen(method_path_data),ZOO_EPHEMERAL);
        }
    }

    std::cout << "RpcProvider start service at ip: "<<ip <<" port:" << port <<std::endl;
    //启动网络服务
    server.start();
    m_eventloop.loop(); //启动了epoll_waite
}

//新的socket连接回调
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn)
{
    if(!conn->connected())
    {
        //和rpc client的连接断开
        conn->shutdown();
    }
}
/*
在框架内部，RpcProvider和RpcConsumer协商好之间通信用的protobuf数据类型
service_name method_name args    定义proto的message类型，进行数据头的序列化和反序列化
                                数据头：
                                服务对象名    方法名字     方法的参数长度(记录一下防止tcp粘包)
                                service_name method_name args_size
16UserServiceLoginzhang san123456   

recv_buf = header_size(4个字节) + header_str + args_str  （记录参数）

10 "10"
10000 "1000000"
std::string   insert和copy方法 
*/
//已建立连接用户的读写事件回调 如果远程又rpc服务的调用请求，该部分会响应
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buffer, muduo::Timestamp)
{
    //网络上接收的远程rpc调用请求的字节流
    std::string recv_buf = buffer->retrieveAllAsString();

    //从字符流中读取前4个字节的内容
    uint32_t header_size;
    recv_buf.copy((char *)&header_size,4,0); //从0开始，放4个字节，放置的起始地址为header_size

    //根据header_size读取数据的原始字节流（从第4个字节开始，读header_size长度）
    std::string rpc_header_str = recv_buf.substr(4,header_size);
    mprpc::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size;

    if(rpcHeader.ParseFromString(rpc_header_str))
    {
        //反序列化成功
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size   = rpcHeader.args_size();
    }
    else
    {
        //数据头反序列化失败
        std::cout << "rpc_header_str: " << rpc_header_str << "Parse erro" << std::endl;
        return;
    }

    //获取rpc方法的字符流数据 args_str
    std::string args_str = recv_buf.substr(4 + header_size,args_size);

    std::cout <<"==================================================="<<std::endl;
    std::cout << "header_size: "  << header_size << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "hargs_size: " << args_size << std::endl;
    std::cout << "args_str: "   << args_str << std::endl;
    std::cout <<"==================================================="<<std::endl;

    //获取service对象
    auto it = m_serviceMap.find(service_name);

    if(it == m_serviceMap.end())
    {
        std::cout << "cannot find the service: " << service_name << std::cout;
        return;
    }

    google::protobuf::Service * service = it->second.m_service; //service对象

    //获取method对象
    auto mit = it->second.m_methodMap.find(method_name);
    if(mit == it->second.m_methodMap.end())
    {
        std::cout << "cannot find the method: " << method_name << std::cout;
        return;       
    }
    const google::protobuf::MethodDescriptor* method = mit->second; //method对象 比如Login方法

    //生成rpc方法调用的请求request和响应response参数
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    //请求参数的反序列化
    if(!request->ParseFromString(args_str))
    {
        std::cout << "args_str: " << args_str << "Parse erro" << std::endl;
        return;
    }

    google::protobuf::Message *response = service->GetResponsePrototype(method).New();
    
    //给CallMethod绑定一个clousure的回调函数
    google::protobuf::Closure* done  
    = google::protobuf::internal::NewCallback<RpcProvider,
                                            const muduo::net::TcpConnectionPtr&,
                                            google::protobuf::Message*>(this,&RpcProvider::SendRpcResponse,conn,response);

    //在框架上根据远端rpc请求，调用当前rpc节点上发布的方法
    //new UerService().Login(controller,request,reponse,done)
    service->CallMethod(method,nullptr,request,response,done);

}
//Closure回调操作，用于序列化rpc响应和网络发送
void  RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn,google::protobuf::Message* response)
{
    //序列化
    std::string response_str;
    if(response->SerializeToString(&response_str))
    {
        //序列化成功后，通过网络将rpc方法的执行结果发送回rpc的caller
        conn->send(response_str);
    }
    else
    {
        std::cout << "serialize response_str erro!" << std::endl;
    }
    conn->shutdown(); //模拟http短链接服务，由RpcProvider主动断开连接，节约资源，给其他rpc客户端继续提供服务
}