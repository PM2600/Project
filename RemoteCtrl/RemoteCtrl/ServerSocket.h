#pragma once

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"


class CPacket {
public:
	CPacket():sHead(0), nLength(0), sCmd(0), sSum(0){}
	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	CPacket(const BYTE* pData, size_t& nSize){
		size_t i = 0;
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2;
				break; 
			}
		}
		// �����ݿ��ܲ�ȫ�����߰�ͷδ��ȫ�����յ�
		if (i + 4 + 2 + 2 >= nSize) {
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i);
		i += 4;
		// ��δ��ȫ���յ�������ʧ�ܣ��ͷ���
		if (nLength + i > nSize) {
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i);
		i += 2;
		if(nLength > 4){
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i);
		i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[i]) & 0xFF;
		}
		if (sum == sSum) {
			nSize = i;
			return;
		}
		nSize = 0;
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
		return *this;
	}

public:
	WORD sHead; // 2�ֽڰ�ͷ �̶�λFE FF
	DWORD nLength; // 4�ֽڰ����ȣ��ӿ������ʼ������У�������
	WORD sCmd; // 2�ֽڿ�������
	std::string strData; // ������
	WORD sSum; // 2�ֽں�У��
};
	 
class CServerSocket
{ 
public:
	static CServerSocket* getInstance() {
		// ��̬����û��thisָ�룬�����޷�ֱ�ӷ��ʳ�Ա��������Ҫ����Ա����Ҳ����Ϊ��̬����
		if (m_instance == NULL) {
			m_instance = new CServerSocket();
		}
		return m_instance;
	}

	bool InitSocket() {
		if (m_server == -1) {
			printf("socket error\n");
			return false;
		}

		SOCKADDR_IN serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_port = htons(8888);
		serv_adr.sin_addr.s_addr = INADDR_ANY;

		if (bind(m_server, (SOCKADDR*)&serv_adr, sizeof(serv_adr)) == -1) {
			printf("bind error\n");
			return false;
		}

		if (listen(m_server, 1) == -1) {
			printf("listen error\n");
			return false;
		}
		return true;
	}

	bool AcceptClient() {
		SOCKADDR_IN cli_adr;
		int cli_sz = sizeof(cli_adr);
		m_client = accept(m_server, (SOCKADDR*)&cli_adr, &cli_sz);
		if (m_client == -1) {
			printf("socket error\n");
			return false;
		}
		return true;
	}

#define BUFFER_SIZE 4096
	int DealCommand() {
		if (m_client == -1)
			return -1;
		// char buffer[1024] = "";
		char* buffer = new char[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (1) {
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0) {
				return -1;
			}
			index += len;
			len = index; // ???
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}

	bool Send(const char* pData, size_t nSize) {
		if (m_client == -1)
			return false;
		return send(m_client, pData, nSize, 0) > 0;
	}



private:
	SOCKET m_server;
	SOCKET m_client;
	CPacket m_packet;

	CServerSocket& operator=(const CServerSocket& ss) {}

	CServerSocket(const CServerSocket& ss) {
		m_server = ss.m_server;
		m_client = ss.m_client;
	}

	CServerSocket(){
		m_client = INVALID_SOCKET;

		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ�����������������"), _T("����!"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_server = socket(PF_INET, SOCK_STREAM, 0);
	}

	~CServerSocket(){
		closesocket(m_server);
		WSACleanup();
	}

	BOOL InitSockEnv() {
		// ��ʼ�������
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		}
		return TRUE;
	}
	static void releaseInstance() {
		if (m_instance != NULL) {
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}
	static CServerSocket* m_instance;

	class CHelper {
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

extern CServerSocket server;

