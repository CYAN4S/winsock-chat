#include <stdio.h> // client side program
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h>
#include <iostream>
#include <vector>
using namespace std;

#define BUF_SIZE 1000

unsigned WINAPI SendMsg(void* arg);
unsigned WINAPI RecvMsg(void* arg);
void ErrorHandling(const char* msg);
vector<string> split(string s, string div);

char name[100] = "[DEFAULT]";

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
    SOCKET hSock;
    SOCKADDR_IN servAddr;
    HANDLE hSndThread, hRcvThread;

    if (argc != 4)
    {
        printf("Usage : %s <IP> <port> <name>\n", argv[0]);
        exit(1);
    }

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        ErrorHandling("WSAStartup() error!");

    // Set Name
    sprintf_s(name, sizeof(name), "%s", argv[3]);

    // Socket Handle Creation
    hSock = socket(PF_INET, SOCK_STREAM, 0);


    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(atoi(argv[2]));
    servAddr.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(hSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
        ErrorHandling("connect() error");

    cout 
        << "\n�����Ͽ����ϴ�.\n\nĿ�ǵ�\t\t\t����\n\n"
        << "[MSG]\t\t\t��� ������� �޽����� �����մϴ�.\n"
        << "/quit\t\t\tä�� ���α׷��� �����մϴ�.\n"
        << "/list\t\t\t������ ����� �����ݴϴ�.\n"
        << "/to [NAME] [MSG]\tƯ�� ������Ը� �޽����� �����մϴ�.\n"
        //<< "/to [NAME] [FILE]\tƯ�� ������Ը� ������ �����մϴ�.\n"
        << "\n";

    hSndThread = (HANDLE)_beginthreadex(NULL, 0, SendMsg, (void*)&hSock, 0, NULL);
    hRcvThread = (HANDLE)_beginthreadex(NULL, 0, RecvMsg, (void*)&hSock, 0, NULL);

    char msg[BUF_SIZE];
    sprintf_s(msg, sizeof(msg), "/enter %s\n", name);
    send(hSock, msg, strlen(msg), 0);

    WaitForSingleObject(hSndThread, INFINITE);
    WaitForSingleObject(hRcvThread, INFINITE);

    closesocket(hSock);
    WSACleanup();

    return 0;
}

unsigned WINAPI SendMsg(void* arg) // send thread main
{
    SOCKET hSock = *((SOCKET*)arg);
    char messageArr[BUF_SIZE] = "";

    while (1)
    {
        string message;

        char msgArr[BUF_SIZE];
        cin.getline(msgArr, BUF_SIZE);

        message = msgArr;

        // ù��° ����
        size_t blank = message.find(" ");
        string setting = message.substr(0, blank);

        if (setting == "/quit")
        {
            cout << "See ya!\n";
            closesocket(hSock);
            exit(0);
        }
        if (setting == "/list")
        {
            message = "/list";
        }
        else if (setting == "/to") // �ӼӸ�
        {
            auto words = split(message, " ");

            string targetClient = "";
            size_t targetNameI = -1;
            string sendMessage = "";

            for (size_t i = 1; i < words.size(); i++)
            {
                if (words[i] == "")
                    continue;
                targetClient = words[i];
                targetNameI = i;
                break;
            }

            if (targetNameI == -1)
            {
                cout << "��븦 ������ �ֽʽÿ�.\n";
                continue;
            }

            if (targetNameI == words.size() - 1)
            {
                cout << "�޽����� �Է��� �ֽʽÿ�.\n";
                continue;
            }

            for (size_t i = targetNameI + 1; i < words.size() - 1; i++)
            {
                sendMessage += (words[i] + " ");
            }
            sendMessage += words.back();

            message = "/to " + targetClient + " " + sendMessage;
        }
        /*else if (setting == "/fileto")
        {
            auto words = split(message, " ");

            string targetClient = "";
            size_t targetNameI = -1;
            string path = "";

            for (size_t i = 1; i < words.size(); i++)
            {
                if (words[i] == "")
                    continue;
                targetClient = words[i];
                targetNameI = i;
                break;
            }

            if (targetNameI == -1)
            {
                cout << "��븦 ������ �ֽʽÿ�.\n";
                continue;
            }

            if (targetNameI == words.size() - 1)
            {
                cout << "���� ��θ� �Է��� �ֽʽÿ�.\n";
                continue;
            }

            for (size_t i = targetNameI + 1; i < words.size() - 1; i++)
            {
                path += (words[i] + " ");
            }
            path += words.back();

            message = "/fileto " + targetClient + " " + path;
            cout << message << "\n";

            FILE* fp;
            auto err = fopen_s(&fp, path.c_str(), "rb");
            if (err != 0) 
            {
                cout << "������ �������� �ʰų�, ���� ������ �����ϴ�.\n";
                continue;
            }
            if (fp)
            {
                err = fclose(fp);
            }

            continue;
        }*/
        else // ��ü �޽���
        {
            message = "/send " + message;
        }

        strcpy_s(messageArr, message.c_str());
        
        send(hSock, messageArr, strlen(messageArr), 0);
    }
    return 0;
}

unsigned WINAPI RecvMsg(void* arg) // read thread main
{
    SOCKET hSock = *((SOCKET*)arg);
    char nameMsg[BUF_SIZE];

    while (1)
    {
        int strLen = recv(hSock, nameMsg, sizeof(nameMsg), 0);
        
        if (strLen == 0)
            break;

        nameMsg[strLen] = '\0';

        if (strcmp(nameMsg, "/using") == 0)
            ErrorHandling("�̹� ��� ���� �г����Դϴ�.");

        cout << nameMsg << "\n";
    }
    return 0;
}

void ErrorHandling(const char* msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

vector<string> split(string s, string div)
{
    vector<string> v;
    int start = 0;
    int d = s.find(div);
    while (d != -1)
    {
        v.push_back(s.substr(start, d - start));
        start = d + 1;
        d = s.find(div, start);
    }
    v.push_back(s.substr(start, d - start));

    return v;
}