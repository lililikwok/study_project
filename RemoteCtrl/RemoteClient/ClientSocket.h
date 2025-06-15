#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#pragma pack(push, 1)
#define BUFFER_SIZE 4096000

class CPacket
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {//根据给定的参数来构建CPacket对象
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear();
		}

		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sSum += BYTE(strData[j]) & 0xFF;
		}

	}
	CPacket(const CPacket& pack) {//拷贝构造
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	CPacket(const BYTE* pData, size_t& nSize)
	{
		size_t i = 0;
		while (i + 2 <= nSize) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				break;
			}
			++i;
		}
		if (i + 2 + 4 + 2 + 2 > nSize) {
			nSize = 0; return;
		}

		sHead = *(WORD*)(pData + i); i += 2;
		nLength = *(DWORD*)(pData + i); i += 4;

		if (i + nLength > nSize) { // 注意：必须确保包体完整
			nSize = 0; return;
		}

		sCmd = *(WORD*)(pData + i); i += 2;

		size_t dataSize = nLength - 2 - 2; // 去掉sCmd和sSum
		strData.resize(dataSize);
		if (dataSize > 0) {
			memcpy(&strData[0], pData + i, dataSize);
			i += dataSize;
		}

		sSum = *(WORD*)(pData + i); i += 2;

		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); ++j) {
			sum += (BYTE)strData[j];
		}

		if (sum == sSum) {
			nSize = i; // 解析成功
		}
		else {
			nSize = 0; // 校验失败
		}
	}

	~CPacket() {}
	CPacket& operator=(const CPacket& pack) {	
		if (this != &pack) {
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		else
			return *this;
	}
	int Size() {//包数据的大小
		return nLength + 6;
	}
	//返回一个Cpacket的数据组成的一个字符
	const char* Data() {//将CPacket对象的各个部分组合起来，并返回指向序列开头的const char*指针
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();//对pData操作影响strOut内容
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}

public:
	WORD sHead;//包头，固定位FE FF
	DWORD nLength;//包长度（从控制命令开始，到和校验结束）
	WORD sCmd;//控制命令
	std::string strData;//包数据
	WORD sSum;//和校验
	std::string strOut;//整个包的数据
};
#pragma pack(pop)

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//点击，移动，双击
	WORD nButton;//左键，右键，中键
	POINT ptXY;//坐标
}MOUSEEV, * PMOUSEEV;

std::string GetErrorInfo(int wsaErrCode);

typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;//是否有效
	char szFileName[256];//文件名
	BOOL HasNext;//是否还有后续，0没有1有
	BOOL IsDirectory;//是否为目录，0否1是

}FILEINFO, * PFILEINFO;

class CClientSocket
{
public:
	static CClientSocket* getInstance() {
		if (m_instance == NULL) {//静态函数没有this指针，不能直接访问成员变量
			m_instance = new CClientSocket();
		}
		return m_instance;
	}
	bool InitSocket(int nIP, int nPort) {
		if (m_sock != INVALID_SOCKET) CloseSocket();
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
		if (m_sock == -1)return false;
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = htonl(nIP);
		serv_adr.sin_port = htons(nPort);
		if (serv_adr.sin_addr.s_addr == INADDR_NONE) {
			AfxMessageBox("指定的ip地址不存在！！！");
			return false;
		}
		int ret = connect(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr));
		if (ret == -1) {
			AfxMessageBox("连接失败");//项目字符集修改成多字节字符集，不然报错
			TRACE("连接失败：%d %s\r\n",WSAGetLastError(),GetErrorInfo(WSAGetLastError()).c_str());
			return false;
		}
		return true;

	}
	int DealCommand() {
		if (m_sock == -1) return -1;

		// 循环直到从缓冲区中成功解析出一个完整的数据包
		while (true) {
			// 1. 尝试从现有缓冲区数据中解析
			size_t parseSize = m_bufferUsed;
			CPacket packet((BYTE*)m_buffer.data(), parseSize);

			if (parseSize > 0) { // 如果构造函数成功解析并消耗了数据
				m_packet = packet; // 保存解析出的包

				// 从缓冲区移除已处理的数据
				memmove(m_buffer.data(), m_buffer.data() + parseSize, m_bufferUsed - parseSize);
				m_bufferUsed -= parseSize;

				//TRACE("[Client] 解包成功，命令: %d，数据大小: %d\n", m_packet.sCmd, m_packet.strData.size());
				return m_packet.sCmd; // 成功，返回命令
			}

			// 2. 如果缓冲区数据不够，从网络接收更多
			if (m_bufferUsed >= BUFFER_SIZE) {//我们只考虑socket中只有正常的数据，而且这一步按照我们的设计永远不会出现
				m_bufferUsed = 0; // 缓冲区满了还解不出包，数据异常，清空
				TRACE("[Client] Buffer full but cannot parse a packet. Clearing buffer.\n");
				return -1;
			}

			int recvLen = recv(m_sock, m_buffer.data() + m_bufferUsed, BUFFER_SIZE - m_bufferUsed, 0);
			if (recvLen <= 0) {
				TRACE("[Client] recv failed or connection closed. Error: %d\n", WSAGetLastError());
				return -1;
			}
			m_bufferUsed += recvLen; // 更新缓冲区中的数据量
		}
	}

	const bool Send(char* pData, int nSize) {
		if (m_sock == -1)return false;
		return (send(m_sock, pData, nSize, 0)) > 0;
	}
	bool Send(CPacket& pack) {
		if (m_sock == -1)return false;
		return (send(m_sock, pack.Data(), pack.Size(), 0)) > 0;
	}
	bool GetFilePath(std::string& strPath) {
		if ((m_packet.sCmd == 2) || (m_packet.sCmd == 3) || (m_packet.sCmd == 4)) {//获取文件列表
			strPath = m_packet.strData;
			return true;
		}
		else
			return false;
	}
	bool GetMouseEvent(MOUSEEV& mouse) {
		if (m_packet.sCmd == 5) {
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}
		return false;
	}

	void CloseSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;//其实就是-1
	}

	CPacket getPacket() {
		return m_packet;
	}
private:
	size_t m_bufferUsed;      // 用这个替换 static index
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;
	CClientSocket& operator=(const CClientSocket& ss) {}
	CClientSocket(const CClientSocket& ss) {
		m_sock = ss.m_sock;
	}
	CClientSocket() {
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置"), _T("初始化错误"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);
		m_bufferUsed = 0; // 初始化
	}
	~CClientSocket() {
		closesocket(m_sock);
		WSACleanup();
	}
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		}
		else return TRUE;
	}
	static void releaseInstance() {
		if (m_instance != NULL) {
			CClientSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}
	static CClientSocket* m_instance;

	class CHelper {//在单例模式中管理生命周期，确保单例的实例正确的在程序结束时被销毁
	public:
		CHelper() {
			CClientSocket::getInstance();
		}
		~CHelper() {
			CClientSocket::releaseInstance();
		}

	};
	static CHelper m_helper;
};


