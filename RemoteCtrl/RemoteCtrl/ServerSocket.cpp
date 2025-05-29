#include "pch.h"
#include "ServerSocket.h"
//在main函数之前完成初始化！！！！！！！！！！
CServerSocket* CServerSocket::m_instance = NULL;//对静态成员函数显示的进行初始化
CServerSocket::CHelper CServerSocket::m_helper;//m_helper 是 CServerSocket 类的一个静态成员变量，它的类型是 CHelper。
