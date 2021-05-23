#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h>
#include <unordered_map>
#include <string>
#include <iostream>
using namespace std;

#define BUF_SIZE 1000
#define MAX_CLNT 256

unsigned WINAPI HandleClnt(void* arg);
void ErrorHandling(const char* msg);
vector<string> split(string s, string divid);
void SendMsg(string msg);
void SendMsgTo(string msg, SOCKET socket);

HANDLE hMutex;

unordered_map<SOCKET, string> clientNames;

string ltrim(string s, const char* t = " \t\n\r\f\v")
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}
string rtrim(string s, const char* t = " \t\n\r\f\v")
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}
string trim(string s, const char* t = " \t\n\r\f\v")
{
    return ltrim(rtrim(s, t), t);
}


int main(int argc, char* argv[])
{
    WSADATA wsaData;
    SOCKET hServSock, hClntSock;
    SOCKADDR_IN servAdr, clntAdr;
    int clntAdrSz;
    HANDLE hThread;

    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        ErrorHandling("WSAStartup() error!");
    }

    if ((hMutex = CreateMutex(NULL, FALSE, NULL)) == NULL)
    {
        ErrorHandling("Mutex Error");
    }

    // Socket Handle Creation
    hServSock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAdr.sin_port = htons(atoi(argv[1]));

    // Bind
    if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
    {
        ErrorHandling("bind() error");
    }

    if (listen(hServSock, 5) == SOCKET_ERROR)
    {
        ErrorHandling("listen() error");
    }

    while (1)
    {
        clntAdrSz = sizeof(clntAdr);
        hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &clntAdrSz);

        WaitForSingleObject(hMutex, INFINITE);

        clientNames[hClntSock] = "";

        ReleaseMutex(hMutex);

        hThread = (HANDLE)_beginthreadex(NULL, 0, HandleClnt, (void*)&hClntSock, 0, NULL);
        printf("Connected client IP: %s \n", inet_ntoa(clntAdr.sin_addr));
    }

    closesocket(hServSock);
    WSACleanup();

    return 0;
}

unsigned WINAPI HandleClnt(void* arg)
{
    SOCKET hClntSock = *((SOCKET*)arg);
    char recvMsg[BUF_SIZE];

    string name = clientNames[hClntSock];
    bool isUsing = false;

    while (true)
    {
        int recvSize = recv(hClntSock, recvMsg, sizeof(recvMsg), 0);
        cout << "("<< recvSize << ") ";

        if (recvSize <= 0)
            break;

        recvMsg[recvSize] = '\0';
        
        string recvStr = recvMsg;

        int p = recvStr.find(" ");
        auto cmd = recvStr.substr(0, p);

        if (cmd == "/send")
        {
            cout << "[" << name << "] " << recvStr << "\n";

            SendMsg("[" + name + "] " + recvStr.substr(p + 1));
        }
        else if (cmd == "/enter")
        {
            name = trim(recvStr.substr(p + 1));

            WaitForSingleObject(hMutex, INFINITE);

            for (auto& cli : clientNames)
                if (cli.second == name)
                {
                    SendMsgTo("/using", hClntSock);
                    isUsing = true;
                    break;
                }

            if(!isUsing)
                clientNames[hClntSock] = name;

            ReleaseMutex(hMutex);

            if (isUsing)
            {
                cout << "[" << name << "] /using\n";
                break;
            }
                
            cout << "[" << name << "] /enter\n";

            SendMsg("** [" + name + "] 님이 입장하셨습니다. **");
        }
        else if (cmd == "/to")
        {
            auto words = split(recvStr, " ");
            SOCKET target = NULL;

            WaitForSingleObject(hMutex, INFINITE);
            
            for (auto& cli : clientNames)
            {
                if (cli.second == words[1])
                {
                    target = cli.first;
                    break;
                }
            }

            ReleaseMutex(hMutex);

            if (target == NULL)
            {
                SendMsgTo("조회된 참가자가 없습니다.", hClntSock);
                cout << "<" + name + "> " + recvStr + "\n";
                continue;
            }

            string sendMessage;
            for (size_t i = 2; i < words.size() - 1; i++)
            {
                sendMessage += (words[i] + " ");
            }
            sendMessage += words.back();

            cout << "<" + name + "> " + recvStr + "\n";

            SendMsgTo("<" + name + "->" + words[1] + "> " + sendMessage, target);
            SendMsgTo("<" + name + "->" + words[1] + "> " + sendMessage, hClntSock);
        }
        else if (cmd == "/list")
        {
            string list;
            WaitForSingleObject(hMutex, INFINITE);

            cout << "[" << name << "] /list\n";

            for (auto& cli : clientNames)
            {
                SOCKET cs = cli.first;
                string name = cli.second;

                sockaddr_in sa;
                int namelen;
                getsockname(cs, (sockaddr*)&sa, &namelen);

                string ip = inet_ntoa(sa.sin_addr);

                list += (ip + "\t" + name + "\n");
            }

            ReleaseMutex(hMutex);
            SendMsgTo(list, hClntSock);
        }
        else
        {
            char inv[] = "Invalid command.\n";
            SendMsgTo(inv, hClntSock);
        }
    }

    // if the connection has been closed
    WaitForSingleObject(hMutex, INFINITE);

    if (clientNames.count(hClntSock))
        clientNames.erase(hClntSock);

    if (!isUsing)
    {
        cout << "[" << name << "] /quit\n";
        SendMsg("** [" + name + "] 님이 퇴장하셨습니다. **");
    }

    ReleaseMutex(hMutex);
    closesocket(hClntSock);

    return 0;
}

void SendMsg(string msg)
{
    WaitForSingleObject(hMutex, INFINITE);

    char arr[BUF_SIZE];
    strcpy_s(arr, msg.c_str());

    for (auto& cli : clientNames)
        send(cli.first, arr, msg.length(), 0);

    ReleaseMutex(hMutex);
}

void SendMsgTo(string msg, SOCKET socket)
{
    WaitForSingleObject(hMutex, INFINITE);

    char arr[BUF_SIZE];
    strcpy_s(arr, msg.c_str());

    send(socket, arr, msg.length(), 0);

    ReleaseMutex(hMutex);
}

void ErrorHandling(const char* msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

vector<string> split(string s, string divid) 
{
    vector<string> v;
    int start = 0;
    int d = s.find(divid);
    while (d != -1) 
    {
        v.push_back(s.substr(start, d - start));
        start = d + 1;
        d = s.find(divid, start);
    }
    v.push_back(s.substr(start, d - start));

    return v;
}
