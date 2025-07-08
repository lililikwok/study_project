#pragma once
#include "pch.h"
#include "framework.h"
#include "Packet.h"
#include <list>
typedef void (*SOCKET_CALLBACK)(void*, int, std::list<CPacket>&, CPacket&);

class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		if (m_instance == NULL) {//��̬����û��thisָ�룬����ֱ�ӷ��ʳ�Ա����
			m_instance = new CServerSocket();
		}
		return m_instance;
	}

	int Run(SOCKET_CALLBACK callback, void* arg, short port = 9527) {
		//1 ���ȵĿɿ��� 2 �Խӵķ����� 3 ���������������籩©����
		// TODO: socket,bind,listen,accept,read,write,close
		//�׽��ֳ�ʼ��
		bool ret = InitSocket(port);//�����ȴ�����
		if (ret == false)
			return -1;
		std::list<CPacket> lstPackets;
		m_callback = callback;//����ҵ�����ص�ָ��
		m_arg = arg;//�ص���������
		int count = 0;
		while (true) {
			if (AcceptClient() == false) {
				if (count >= 3) {
					return -2;
				}
				count++;
			}
			int ret = DealCommand();//�������ܵ��İ�
			if (ret > 0) {
				m_callback(m_arg, ret, lstPackets, m_packet);//m_callback(m_arg, ret); ����ûص��������������д�������������һ�������ض�������ص����ݣ�m_arg������һ�������һ�β����Ľ����ret��
				while (lstPackets.size() > 0) {
					Send(lstPackets.front());//����һ�����ݰ�
					lstPackets.pop_front();
				}
			}
			CloseClient();
		}
		return 0;
	}

protected:
	bool InitSocket(short port) {
		if (m_sock == -1)return false;
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_port = htons(port);
		//��
		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
			return false;
		}
		if (listen(m_sock, 1) == -1) {
			return false;
		}
		return true;
	}

	bool AcceptClient() {
		sockaddr_in client_adr;
		//char buffer[1024];
		int cli_sz = sizeof(client_adr);
		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
		if (m_client == -1)return false;
		return true;

	}

#define BUFFER_SIZE 4096
	//���յ���Ϣ�ĺ󣬵��øú�����socket�е����ݴ�ɰ�
	//������յ�����������
	int  DealCommand() {
		if (m_client == -1)return -1;
		char* buffer = new char[BUFFER_SIZE];
		if (buffer == NULL) {
			TRACE("�ڴ治�㣡\r\n");
			return -2;
		}
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;//��ʾ��ǰ buffer ���Ѿ����յ������ݳ��ȣ���Ч�ֽ�����
		while (true) {
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0) {
				delete[] buffer;
				return -1;
			}
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);//�����len����ˣ����ʹ���˶����ֽ�
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE - len);//�ڴ��ƶ��������ֱ��ǣ�1.Ŀ���ַ�� 2.ԭ��ַ�� 3.Ҫ�ƶ����ֽ���
				index -= len;
				delete[] buffer;
				return m_packet.sCmd;
			}
		}
		delete[] buffer;
		return -1;
	}
	const bool Send(char* pData, int nSize) {
		if (m_client == -1)return false;
		return (send(m_client, pData, nSize, 0)) > 0;
	}
	bool Send(CPacket& pack) {
		if (m_client == -1)return false;
		return (send(m_client, pack.Data(), pack.Size(), 0)) > 0;
	}

	void CloseClient() {
		closesocket(m_client);
		m_client = INVALID_SOCKET;
	}
private:
	static CServerSocket* m_instance;
	SOCKET_CALLBACK m_callback;
	void* m_arg;//void ������������һ����������Ϊ���������κξ�����������͡�Ӧ��ʹ�� void*������һ��ָ�� void ��ָ�롣��ͨ�����ڷ�������ָ�룬��ת��Ϊ�κ��������͵�ָ��
	SOCKET m_sock;
	SOCKET m_client;
	CPacket m_packet;
	CServerSocket& operator=(const CServerSocket& ss) {}
	CServerSocket(const CServerSocket& ss) {
		m_client = ss.m_client;
		m_sock = ss.m_sock;
	}
	CServerSocket() {
		m_client = INVALID_SOCKET;
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���,������������"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}
	~CServerSocket() {
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
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}

	class CHelper {//�ڵ���ģʽ�й����������ڣ�ȷ��������ʵ����ȷ���ڳ������ʱ������
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releaseInstance();
		}

	};
	static CHelper m_helper;
};


