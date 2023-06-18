#pragma once
#include <queue>
#include <thread>
#include <mutex>    //pthread_mutex_t
#include <condition_variable>   //pthread_condition_t

//异步写日志的日志队列
template<typename T>
class lockqueue
{
public:
    void Push(const T &data)
    {
        std::lock_guard<std::mutex> lock(m_mutex);  //获取互斥锁,锁自动释放
        m_queue.push(data);             
        m_condvariable.notify_one(); //唤醒wait状态的线程            
    }
    T Pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_queue.empty())
        {
            //日志队列为空，线程进入wait状态
            m_condvariable.wait(lock);
        }
        T data = m_queue.front();
        m_queue.pop();
        return data;
    } 
private:
    std::queue<T> m_queue;
    std::mutex    m_mutex;
    std::condition_variable m_condvariable; //条件变量
};
