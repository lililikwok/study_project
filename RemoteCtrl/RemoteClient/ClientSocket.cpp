#include "pch.h"
#include "ClientSocket.h"


//CServerSocket server;

CClientSocket* CClientSocket::m_instance = NULL;//�Ծ�̬��Ա������ʾ�Ľ��г�ʼ��
CClientSocket::CHelper CClientSocket::m_helper;//m_helper �� CServerSocket ���һ����̬��Ա���������������� CHelper��

CClientSocket* pserver = CClientSocket::getInstance();