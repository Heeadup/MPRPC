#include "mprpccontroller.h"

MprpcController::MprpcController()
{
    m_failed = false;
    m_erroText = "";
}
void MprpcController::Reset()
{
    m_failed = false;
    m_erroText = "";
}
bool MprpcController::Failed() const
{
    return m_failed;
}
std::string MprpcController::ErrorText() const
{
    return m_erroText;
}
//设置错误，表示真错误
void MprpcController::SetFailed(const std::string& reason)
{
    m_failed = true;
    m_erroText = reason;
}

/*
其他方法设置为空函数
*/
void MprpcController::StartCancel(){}
bool MprpcController::IsCanceled() const{return false;}
void MprpcController::NotifyOnCancel(google::protobuf::Closure* callback){}