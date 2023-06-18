/*
muduo网络库给用户提供了两个主要的类
TcpServer ： 用于编写服务器程序的
TcpClient ： 用于编写客户端程序的

epoll + 线程池
好处：能够把网络I/O的代码和业务代码区分开
                        用户的连接和断开       用户的可读写事件
*/
#include "muduo/net/EventLoop.h"
#include "muduo/net/TcpServer.h"
#include <string>
#include <functional>
#include <iostream>

/*基于muduo网络库开发服务器程序
1.组合TcpServer对象
2.创建EventLoop事件循环对象的指针
3.明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
4.在当前服务器类的构造函数当中，注册处理连接的回调函数和处理读写时间的回调函数
5.设置合适的服务端线程数量，muduo库会自己分配I/O线程和worker线程
*/
class ChatServer
{
public:
    ChatServer(muduo::net::EventLoop* loop,
            const muduo::net::InetAddress& listenAddr,
            const std::string& nameArg): _server(loop,listenAddr,nameArg),_loop(loop)
            {
                // 给服务器注册用户连接的创建和断开回调
                _server.setConnectionCallback(std::bind(&ChatServer::OnConnection,this,std::placeholders::_1));

                //已连接用户读写信息的回调
                _server.setMessageCallback(std::bind(&ChatServer::OnMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));

                //设置服务器端的线程数 1个io线程 3个worker线程
                _server.setThreadNum(4);
            }
            //开启事件循环
            void start()
            {
                _server.start();
            }
private:

    // 专门处理用户的连接创建和断开
    void OnConnection(const muduo::net::TcpConnectionPtr& conn)
    {
        if(conn->connected())
        {
            std::cout << conn->peerAddress().toIpPort() << "->" << conn->localAddress().toIpPort() << " state:online" <<std::endl;
        }
        else
        {
            std::cout << conn->peerAddress().toIpPort() << "->" << conn->localAddress().toIpPort() <<  " state:offline"<< std::endl;
            conn->shutdown(); //close(fd)
            //_loop->quit(); 
        }
    }
    //专门处理用户的读写事件
    void OnMessage(const muduo::net::TcpConnectionPtr &conn, //连接
     muduo::net::Buffer *buf,   //缓冲区
      muduo::Timestamp time)  //接收到数据的时间信息
    {
        std::string sbuf =  buf->retrieveAllAsString();
        std::cout<< " recv data:" << buf << " time:" << time.toString() << std::endl;
        conn->send(sbuf);
    }
    muduo::net::TcpServer _server;  //组合TcpServer对象
    muduo::net::EventLoop *_loop;   //创建EventLoop事件循环对象的指针
};

int main()
{
    muduo::net::EventLoop loop;     //创建epoll
    muduo::net::InetAddress addr("219.216.72.111",6000);
    ChatServer server(&loop,addr,"ChatServer");

    server.start();
    loop.loop(); //epoll_wait以阻塞的方式等待新用户的连接，已连接用户的读写事件

    return 0;
}
