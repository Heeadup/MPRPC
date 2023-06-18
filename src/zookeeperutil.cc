#include "zookeeperutil.h"
#include "mprpcapplication.h"
#include <string>
#include <iostream>
ZkClient::ZkClient() : m_zhandle(nullptr)   //初始化句柄为空
{
}
ZkClient::~ZkClient()
{
    if(m_zhandle != nullptr)
    {
        zookeeper_close(m_zhandle);
    }
}

//全局watcher观察器，zkserver给zkclient的通知
void global_watch(zhandle_t *zh, int type, int state, const char *path,void *watcherCtx)
{
    if(type == ZOO_SESSION_EVENT) //回调的消息类型是和会话相关的消息类型
    {
        if(state == ZOO_CONNECTED_STATE)    //zkserver和zkclient连接成功
        {
            sem_t *sem = (sem_t*)zoo_get_context(zh); //从句柄上获取信号量，获取之前的zoo_set_context
            sem_post(sem);          //信号量资源+1
        }   
    }
}

//ZkClient启动连接zkserver
void ZkClient::Start()
{
    std::string host = MprpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    std::string port = MprpcApplication::GetInstance().GetConfig().Load("zookeeperport");
    std::string m_host = host + ":" + port;
	/*
        zookeeper_mt：多线程版本
        zookeeper的API客户端程序提供了三个线程
        API调用线程 
        网络I/O线程  pthread_create  poll
        watcher回调线程 pthread_create
	*/

    //初始化，得到的返回值为句柄
    m_zhandle = zookeeper_init(m_host.c_str(),global_watch,30000,nullptr,nullptr,0); //关键，异步连接

    if(m_zhandle == nullptr)
    {
        std::cout << "zookeeper_init erro!" << std::endl;
        exit(EXIT_FAILURE);
    }

    //创建信号量
    sem_t sem;
    sem_init(&sem,0,0);
    //创建上下文，给监听器传参数，可理解为给指定的句柄添加额外的信息
    zoo_set_context(m_zhandle,&sem);

    sem_wait(&sem); //主线程等待，等待zkserver响应，判断是否已经真的连接成功
    std::cout << "zookeeper_init success!" << std::endl;

}



void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    int flag;
	// 先判断path表示的znode节点是否存在，如果存在，就不再重复创建了
	flag = zoo_exists(m_zhandle, path, 0, nullptr);
	if (ZNONODE == flag) // 表示path的znode节点不存在
	{
		// 创建指定path的znode节点了
		flag = zoo_create(m_zhandle, path, data, datalen,
			&ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen); //主要是前三个参数，path是每个节点都有的路径，&ZOO_OPEN_ACL_UNSAFE是权限
		if (flag == ZOK)
		{
			std::cout << "znode create success... path:" << path << std::endl;
		}
		else
		{
			std::cout << "flag:" << flag << std::endl;
			std::cout << "znode create error... path:" << path << std::endl;
			exit(EXIT_FAILURE);
		}
	}
}

// 根据指定的path，获取znode节点的值
std::string ZkClient::GetData(const char *path)
{
    char buffer[64];
	int bufferlen = sizeof(buffer);
	int flag = zoo_get(m_zhandle, path, 0, buffer, &bufferlen, nullptr);
	if (flag != ZOK)
	{
		std::cout << "get znode error... path:" << path << std::endl;
		return "";
	}
	else
	{
		return buffer;
	}
}