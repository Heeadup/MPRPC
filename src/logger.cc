#include "logger.h"
#include <time.h>
#include <iostream>
//获取日志的单例
Logger& Logger::GetInstance()
{
    static Logger logger;
    return logger;
}

Logger::Logger()
{   

    //启动专门的写日志进程
    std::thread writeLogTask([&]()
                            {
        for(;;)
        {
            //获取当前日期，取日志信息，写入响应的日志文件中
            time_t now =  time(nullptr);        //获取的是1970到现在的秒数
            tm* nowtm = localtime(&now);

            char file_name[128];        //设置文件名
            sprintf(file_name,"%d_%d_%d-log.txt",nowtm->tm_year + 1900,nowtm->tm_mon + 1,nowtm->tm_mday);       
            
            FILE *pf = fopen(file_name,"a+");
            if(pf == nullptr)
            {
                std::cout << "logger file: " << file_name << "open erro!" << std::endl;
                exit(EXIT_FAILURE);
            }

            std::string msg = m_lockQue.Pop();

            char time_buf[128] = {0};
            sprintf(time_buf,"%d:%d:%d => [%s]",
                    nowtm->tm_hour,
                    nowtm->tm_min,
                    nowtm->tm_sec,
                    (m_loggerlevel == INFO) ? "info":"error");
            msg.insert(0,time_buf);
            msg.append("\n");   //加入换行
            fputs(msg.c_str(),pf);
            fclose(pf);
        } });

    //设置分离进程，守护进程
    writeLogTask.detach();
}
//设置日志级别
void Logger::setLogLevel(LogLevel level)
{
    m_loggerlevel = level;
}
//写日志,将日志信息放入m_lockQue缓冲区中
void Logger::Log(std::string msg)
{
    m_lockQue.Push(msg);
}

