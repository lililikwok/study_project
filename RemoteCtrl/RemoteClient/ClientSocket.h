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
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {//���ݸ����Ĳ���������CPacket����
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
	CPacket(const CPacket& pack) {//��������
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

		if (i + nLength > nSize) { // ע�⣺����ȷ����������
			nSize = 0; return;
		}

		sCmd = *(WORD*)(pData + i); i += 2;

		size_t dataSize = nLength - 2 - 2; // ȥ��sCmd��sSum
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
			nSize = i; // �����ɹ�
		}
		else {
			nSize = 0; // У��ʧ��
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
	int Size() {//�����ݵĴ�С
		return nLength + 6;
	}
	//����һ��Cpacket��������ɵ�һ���ַ�
	const char* Data() {//��CPacket����ĸ����������������������ָ�����п�ͷ��const char*ָ��
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();//��pData����Ӱ��strOut����
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}

public:
	WORD sHead;//��ͷ���̶�λFE FF
	DWORD nLength;//�����ȣ��ӿ������ʼ������У�������
	WORD sCmd;//��������
	std::string strData;//������
	WORD sSum;//��У��
	std::string strOut;//������������
};
#pragma pack(pop)

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//������ƶ���˫��
	WORD nButton;//������Ҽ����м�
	POINT ptXY;//����
}MOUSEEV, * PMOUSEEV;

std::string GetErrorInfo(int wsaErrCode);

typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;//�Ƿ���Ч
	char szFileName[256];//�ļ���
	BOOL HasNext;//�Ƿ��к�����0û��1��
	BOOL IsDirectory;//�Ƿ�ΪĿ¼��0��1��

}FILEINFO, * PFILEINFO;

class CClientSocket
{
public:
	static CClientSocket* getInstance() {
		if (m_instance == NULL) {//��̬����û��thisָ�룬����ֱ�ӷ��ʳ�Ա����
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
			AfxMessageBox("ָ����ip��ַ�����ڣ�����");
			return false;
		}
		int ret = connect(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr));
		if (ret == -1) {
			AfxMessageBox("����ʧ��");//��Ŀ�ַ����޸ĳɶ��ֽ��ַ�������Ȼ����
			TRACE("����ʧ�ܣ�%d %s\r\n",WSAGetLastError(),GetErrorInfo(WSAGetLastError()).c_str());
			return false;
		}
		return true;

	}
	int DealCommand() {
		if (m_sock == -1) return -1;

		// ѭ��ֱ���ӻ������гɹ�������һ�����������ݰ�
		while (true) {
			// 1. ���Դ����л����������н���
			size_t parseSize = m_bufferUsed;
			CPacket packet((BYTE*)m_buffer.data(), parseSize);

			if (parseSize > 0) { // ������캯���ɹ�����������������
				m_packet = packet; // ����������İ�

				// �ӻ������Ƴ��Ѵ��������
				memmove(m_buffer.data(), m_buffer.data() + parseSize, m_bufferUsed - parseSize);
				m_bufferUsed -= parseSize;

				//TRACE("[Client] ����ɹ�������: %d�����ݴ�С: %d\n", m_packet.sCmd, m_packet.strData.size());
				return m_packet.sCmd; // �ɹ�����������
			}

			// 2. ������������ݲ�������������ո���
			if (m_bufferUsed >= BUFFER_SIZE) {//����ֻ����socket��ֻ�����������ݣ�������һ���������ǵ������Զ�������
				m_bufferUsed = 0; // ���������˻��ⲻ�����������쳣�����
				TRACE("[Client] Buffer full but cannot parse a packet. Clearing buffer.\n");
				return -1;
			}

			int recvLen = recv(m_sock, m_buffer.data() + m_bufferUsed, BUFFER_SIZE - m_bufferUsed, 0);
			if (recvLen <= 0) {
				TRACE("[Client] recv failed or connection closed. Error: %d\n", WSAGetLastError());
				return -1;
			}
			m_bufferUsed += recvLen; // ���»������е�������
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
		if ((m_packet.sCmd == 2) || (m_packet.sCmd == 3) || (m_packet.sCmd == 4)) {//��ȡ�ļ��б�
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
		m_sock = INVALID_SOCKET;//��ʵ����-1
	}

	CPacket getPacket() {
		return m_packet;
	}
private:
	size_t m_bufferUsed;      // ������滻 static index
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;
	CClientSocket& operator=(const CClientSocket& ss) {}
	CClientSocket(const CClientSocket& ss) {
		m_sock = ss.m_sock;
	}
	CClientSocket() {
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���,������������"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);
		m_bufferUsed = 0; // ��ʼ��
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

	class CHelper {//�ڵ���ģʽ�й����������ڣ�ȷ��������ʵ����ȷ���ڳ������ʱ������
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


