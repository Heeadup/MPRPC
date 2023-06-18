#pragma once
#include "mprpcconfig.h"
#include "mprpccontroller.h"
#include "mprpcchannel.h"
//mprpc框架的基础类，负责框架的一些初始化操作
class MprpcApplication
{
public:
    static void Init(int argc,char *argv[]);
    //定义一个获取实例的方法
    static MprpcApplication& GetInstance();
    //获取配置项
    static MprpcConfig& GetConfig();
private:
    static MprpcConfig m_config;

    MprpcApplication(){}
    MprpcApplication(const MprpcApplication&) = delete;
    MprpcApplication(MprpcApplication&&) = delete;
};

