#include "mprpcconfig.h"
#include <iostream>
#include <string>
// 负责解析加载配置文件
void MprpcConfig::LoadConfigFile(const char *config_file)
{
    FILE *pf = fopen(config_file, "r"); // 读文件
    if (pf == nullptr)
    {
        std::cout << config_file << "is not exist!" << std::endl;
        exit(EXIT_FAILURE);
    }

    // 处理情况 1.注释 2.正确的配置项 3.去掉开头多余的空格
    while (!feof(pf))
    {
        char buf[512] = {0};
        fgets(buf, 512, pf); // 从pf中读取一行存储到buf中

        std::string read_buf(buf);

        Trim(read_buf);
        // 跳过#的注释,以及换行
        if (read_buf[0] == '#' || read_buf.empty())
        {
            continue;
        }

        // 解析配置项
        int idx = read_buf.find('=');
        if (idx == -1)
        {
            // 配置不合法
            continue;
        }

        std::string key;
        std::string value;
        key = read_buf.substr(0, idx);
        Trim(key);
        // std::cout<<"key = "<< key <<std::endl;
        //去掉换行符 \n
        int end_idx = read_buf.find('\n',idx);
        value = read_buf.substr(idx + 1, end_idx - idx - 1);
        Trim(value);
        m_configMap.insert({key, value});
    }
    fclose(pf);
}

// 查询配置项信息
std::string MprpcConfig::Load(const std::string &key)
{
    auto it = m_configMap.find(key);
    if (it == m_configMap.end())
    {
        // std::cout << "it == m_configMap.end()" <<std::endl;
        return "";
    }
    return it->second;
}

// 去掉字符串前后的空格
void MprpcConfig::Trim(std::string &src_buf)
{
    // 去掉字符串前面多余的空格
    int idx = src_buf.find_first_not_of(' ');
    if (idx != -1)
    {
        // 字符串前面有空格
        src_buf = src_buf.substr(idx, src_buf.size() - idx);
    }

    // 去掉字符串后面多余的空格
    idx = src_buf.find_last_not_of(' ');
    if (idx != -1)
    {
        // 字符串后面有空格
        src_buf = src_buf.substr(0, idx + 1);
    }
}