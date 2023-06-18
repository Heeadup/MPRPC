#include "mprpcapplication.h"
#include <iostream>
#include <unistd.h>
#include <string>

//初始化静态的成员变量
MprpcConfig MprpcApplication::m_config;

void ShowArgsHelp()
{
    std::cout << "format: comman -i <configfile>" << std::endl;
}

void MprpcApplication::Init(int argc,char *argv[])
{
    //rpc服务站点未传入任何参数
    if (argc < 2)
    {
        ShowArgsHelp();
        exit(EXIT_FAILURE);
    }

    int c = 0;
    std::string config_file;
    /*
    如果选项成功找到，返回选项字母；如果所有命令行选项都解析完毕，返回 -1；
    如果遇到选项字符不在 optstring 中，返回字符 '?'；
    如果遇到丢失参数，那么返回值依赖于 optstring 中第一个字符，如果第一个字符是 ':' 则返回':'，否则返回'?'并提示出错误信息。
    */
    while ((c = getopt(argc,argv,"i:")) != -1)
    {
        switch (c)
        {
        case 'i':           //读取到i，optarg返回i后的配置文件名地址
            config_file = optarg;
            break;
        case '?':           //出现了不希望出现的参数?
            //std::cout << "invalid args!" <<std::endl;
            ShowArgsHelp();
            break;
        case ':':           //有-i 未出现正确参数
            //std::cout << "need <configfile>" <<std::endl;
            ShowArgsHelp();
            exit(EXIT_FAILURE);
        default:
            break;

        }

    }     
    //开始加载配置文件 rpcserverip rpcserverport rookeeperip rookeeperport
    m_config.LoadConfigFile(config_file.c_str()); //LoadConfigFile需要char*文件

    std::cout << "rpcserverip: " << m_config.Load("rpcserverip") << std::endl;
    std::cout << "rpcserverport: " << m_config.Load("rpcserverport") << std::endl;
    std::cout << "rookeeperip: " << m_config.Load("rookeeperip") << std::endl;
    std::cout << "rookeeperport: " << m_config.Load("rookeeperport") << std::endl;
    
    
}

MprpcApplication& MprpcApplication::GetInstance()
{
    static MprpcApplication app;
    return app;
}

MprpcConfig& MprpcApplication::GetConfig()
{
    return m_config;
}