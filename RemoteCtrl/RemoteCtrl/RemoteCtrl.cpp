﻿// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Tool.h"
#include "Command.h"
#include <conio.h>
#include "CQueue.h"
#include <MSWSock.h>
#include "EdyServer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define INVOKE_PATH _T("C:\\Users\\pm26\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe")
//#define INVOKE_PATH _T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe")

// 唯一的应用程序对象
CWinApp theApp;

using namespace std;

bool ChooseAutoInvoke(const CString& strPath) {
    TCHAR wcsSystem[MAX_PATH] = _T("");
    //CString strPath = CString(_T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe"));
    //CString strPath = _T("C:\\Users\\PM2.5\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe");
    if (PathFileExists(strPath)) {
        return true;
    }
    CString strInfo = _T("该程序只允许用于合法的用途！\n");
    strInfo += _T("继续运行该程序，将使得这台机器处于被监控状态！\n");
    strInfo += _T("如果你不希望这样，请按“取消”按钮，退出程序！\n");
    strInfo += _T("按下“是”按钮，该程序将被复制到你的机器上，并随系统启动而自动运行！\n");
    strInfo += _T("按下“否”按钮，程序只会运行一次，不会在系统内留下任何东西！\n");
    int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES) {
        //if (CTool::WriteRegisterTable(strPath)) {
        //    MessageBox(NULL, _T("复制文件失败，是否权限不足\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
        //    return false;
        //}
        if (!CTool::WriteStartupDir(strPath)) {
            MessageBox(NULL, _T("复制文件失败，是否权限不足\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
            return false;
        }
    }
    else if (ret == IDCANCEL) {
        return false;
    }
    return true;
}

void iocp();

void udp_server();
void udp_client(bool ishost = true);

void initsock() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
}

void clearsock() {
    WSACleanup();
}

int main(int argc, char* argv[])
{
    if (!CTool::Init())
        return 1;
    initsock();
    if (argc == 1) {
        char wstrDir[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, wstrDir);
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        memset(&si, 0, sizeof(si));
        memset(&pi, 0, sizeof(pi));
        string strCmd = argv[0];
        strCmd += " 1";
        BOOL bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
        if (bRet) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            TRACE("进程ID: %d\r\n", pi.dwProcessId);
            TRACE("线程ID: %d\r\n", pi.dwThreadId);
            strCmd += " 2";
            BOOL bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
            if (bRet) {
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
                TRACE("进程ID: %d\r\n", pi.dwProcessId);
                TRACE("线程ID: %d\r\n", pi.dwThreadId);
                udp_server();
            }
        }   
    }
    else if (argc == 2) { //主客户端
        udp_client();
    }
    else {                //从客户端
        udp_client(false);
    }
    clearsock();

    iocp();

    //if (CTool::IsAdmin()) {
    //    if (!CTool::Init())
    //        return 1;
    //    MessageBox(NULL, _T("管理员"), _T("用户状态"), 0);
    //    if (ChooseAutoInvoke(INVOKE_PATH)) {
    //        CCommand cmd;
    //        int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
    //        switch (ret) {
    //        case -1:
    //            MessageBox(NULL, _T(""), _T("网络初始化异常，请检查网络状态"), MB_OK | MB_ICONERROR);
    //            break;
    //        case -2:
    //            MessageBox(NULL, _T("多次无法接入用户， 结束程序"), _T("失败，"), MB_OK | MB_ICONERROR);
    //            break;
    //        }
    //    }
    //}
    //else {
    //    MessageBox(NULL, _T("普通用户"), _T("用户状态"), 0);
    //    if (CTool::RunAsAdmin() == false) {
    //        CTool::ShowError();
    //        return 1;
    //    }
    //}
    return 0;
}

class COverlapped {
public:
    OVERLAPPED m_overlapped;
    DWORD m_operator;
    char m_buffer[4096];
    COverlapped() {
        m_operator = 0;
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        memset(&m_buffer, 0, sizeof(m_buffer));
    }
};

void iocp()
{
    EdyServer server;
    server.StartService();
    getchar();
}

void udp_server() {
    printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("socket error\r\n");
        return;
    }
    std::list<sockaddr_in> lstclients;
    sockaddr_in server, client;
    memset(&server, 0, sizeof(server));
    memset(&client, 0, sizeof(client));
    server.sin_family = AF_INET;
    server.sin_port = htons(20000);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(sock, (sockaddr*)&server, sizeof(server)) == -1) {
        printf("bind error\r\n");
        closesocket(sock);
        return;
    }
    std::string buf;
    buf.resize(1024 * 256);
    memset((char*)buf.c_str(), 0, buf.size());
    int len = sizeof(client);
    while (!_kbhit()) {
        int ret = recvfrom(sock, (char*)buf.c_str(), buf.size(), 0, (sockaddr*)&client, &len);
        if (ret > 0) {
            if (lstclients.size() <= 0) {
                lstclients.push_back(client);
                printf("ip:%08X, port:%d\r\n", client.sin_addr.s_addr, ntohs(client.sin_port));
                ret = sendto(sock, buf.c_str(), ret, 0, (sockaddr*)&client, len);
                printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
            }
            else {
                memcpy((void*)buf.c_str(), &lstclients.front(), sizeof(lstclients.front()));
                ret = sendto(sock, buf.c_str(), sizeof(lstclients.front()), 0, (sockaddr*)&client, len);
                printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
            }

            //CTool::Dump((BYTE*)buf.c_str(), ret);
        }
        else {
            printf("error: %d, ret = %d\r\n", WSAGetLastError(), ret);
        }
        Sleep(1);
    }
    closesocket(sock);
}

void udp_client(bool ishost) {
    Sleep(2000);
    sockaddr_in server, client;
    int len = sizeof(client);
    server.sin_family = AF_INET;
    server.sin_port = htons(20000);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("socket error\r\n");
        return;
    }

    if (ishost) {  //主客户端
        printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
        std::string msg = "hello world!\n";
        int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server, sizeof(server));
        if (ret > 0) {
            msg.resize(1024);
            memset((char*)msg.c_str(), 0, msg.size());
            ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
            printf("host %s(%d):%s ERROR(%d) ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__,WSAGetLastError(), ret);
            if (ret > 0) {
                printf("msg = %s\r\n", msg.c_str());
                printf("%s(%d):%s msg = %d\r\n", __FILE__, __LINE__, __FUNCTION__, msg.size());
            }

            ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
            printf("host %s(%d):%s ERROR(%d) ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
            if (ret > 0) {
                printf("msg = %s\r\n", msg.c_str());
                printf("%s(%d):%s msg = %s\r\n", __FILE__, __LINE__, __FUNCTION__, msg.c_str());
            }

        }
    }
    else {         //从客户端
        printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
        std::string msg = "hello world!\n";
        int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server, sizeof(server));
        if (ret > 0) {
            msg.resize(1024);
            memset((char*)msg.c_str(), 0, msg.size());
            ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
            printf("client %s(%d):%s ERROR(%d) ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);

            if (ret > 0) {
                sockaddr_in addr;
                memcpy(&addr, msg.c_str(), sizeof(addr));
                sockaddr_in* paddr = (sockaddr_in*)&addr;

                printf("msg = %s\r\n", msg.c_str());
                printf("%s(%d):%s msg = %d\r\n", __FILE__, __LINE__, __FUNCTION__, msg.size());
                msg = "hello i am client\n";
                ret = sendto(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)paddr, sizeof(sockaddr_in));
                
            }
        }
    }
    closesocket(sock);
}