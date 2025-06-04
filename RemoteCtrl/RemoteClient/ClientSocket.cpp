#include "pch.h"
#include "ClientSocket.h"

std::string GetErrorInfo(int wsaErrCode) {
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}
//CServerSocket server;

CClientSocket* CClientSocket::m_instance = NULL;//�Ծ�̬��Ա������ʾ�Ľ��г�ʼ��
CClientSocket::CHelper CClientSocket::m_helper;//m_helper �� CServerSocket ���һ����̬��Ա���������������� CHelper��

CClientSocket* pserver = CClientSocket::getInstance();