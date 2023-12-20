#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib,"Ws2_32.lib")
#include <winsock2.h>
#include <iostream>
#include <WS2tcpip.h>
#include <cstring>
#include <conio.h>
#include <Windows.h>
#include <tchar.h>
#include <atlsafe.h>
#include <atlimage.h>
#include <vector>
#include <fstream>
#include <thread>
using namespace std;
SOCKET Server1, Server2, Server3, Server4, Client1, Client2, Client3, Client4;
bool ok = true;
void CaptureScreen(HDC& hDesktopDC, HDC& hCaptureDC, HBITMAP& hCaptureBitmap)
{
    SelectObject(hCaptureDC, hCaptureBitmap);
    BitBlt(hCaptureDC, 0, 0, 1920, 1080,
        hDesktopDC, 0, 0, SRCCOPY | CAPTUREBLT);
    std::vector<BYTE> buf;
    IStream* stream = NULL;
    HRESULT hr = CreateStreamOnHGlobal(0, TRUE, &stream);
    CImage image;
    ULARGE_INTEGER liSize;
    image.Attach(hCaptureBitmap);
    image.Save(stream, Gdiplus::ImageFormatJPEG);
    IStream_Size(stream, &liSize);
    DWORD len = liSize.LowPart;
    IStream_Reset(stream);
    buf.resize(len);
    IStream_Read(stream, &buf[0], len);
    stream->Release();
    int n = buf.size();
    send(Client2, (char*)&n, sizeof(int), 0);
    for (int i = 0; i < n; i++)
        send(Client2, (char*)(reinterpret_cast<const char*>(&buf[0]) + i), sizeof(BYTE), 0);

}
void Connection(SOCKET& Server, SOCKET& Client, int port) {
    WSADATA Wsadata;
    int a = WSAStartup(MAKEWORD(2, 2), &Wsadata);
    Server = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in sin, new_sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = port;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(Server, (struct sockaddr*)&sin, sizeof(sin));
    listen(Server, 0);
    int length_addr = sizeof(new_sin);
    Client = accept(Server, (struct sockaddr*)&new_sin, &length_addr);
    std::cout << "Connect success!\n";
}
void SendImageFile() {
    HDC hDesktopDC = GetDC(GetDesktopWindow());
    HDC hCaptureDC = CreateCompatibleDC(hDesktopDC);
    HBITMAP hCaptureBitmap = CreateCompatibleBitmap(hDesktopDC, 1920, 1080);


    while (ok) {
        CaptureScreen(hDesktopDC, hCaptureDC, hCaptureBitmap);
        Sleep(100);
    }
    ReleaseDC(GetDesktopWindow(), hDesktopDC);
    DeleteDC(hCaptureDC);
    DeleteObject(hCaptureBitmap);
}
void RecvMouse() {
    INPUT Inputs[3] = { 0 };
    Inputs[0].type = INPUT_MOUSE;
    Inputs[0].mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
    Inputs[1].type = INPUT_MOUSE;
    Inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    Inputs[2].type = INPUT_MOUSE;
    Inputs[2].mi.dwFlags = MOUSEEVENTF_LEFTUP;

    while (ok) {

        int a[3] = { 1,0,0 };
        recv(Client3, (char*)&a[0], 3 * sizeof(int), 0);
        Inputs[0].mi.dx = (a[0]) * 65535 / 1528;
        Inputs[0].mi.dy = (a[1]) * 65535 / 832;
        if (Inputs[0].mi.dx < 0 || Inputs[0].mi.dx >65535 ||
            Inputs[0].mi.dy < 0 || Inputs[0].mi.dy >65535) continue;
        if (a[2] == 0)
        {
            Inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            Inputs[2].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        }
        else {
            Inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
            Inputs[2].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        }
        SendInput(3, Inputs, sizeof(INPUT));
        //cout << Inputs[0].mi.dx << ' ' << Inputs[0].mi.dy << endl;
    }

}
void RecvKeyboard() {
    int VK[] = { VK_SHIFT,VK_MENU,VK_CONTROL,VK_LWIN };
    while (1) {
        int a[5] = { 0,0,0,0,0 };
        recv(Client1, (char*)&a[0], 5 * sizeof(int), 0);
        for (int i = 0; i < 4; i++) if (a[i + 1]) keybd_event(VK[i], 0, 0, 0);
        keybd_event(a[0], 0, 0, 0);
        keybd_event(a[0], 0, 2, 0);
        for (int i = 0; i < 4; i++) if (a[i + 1]) keybd_event(VK[i], 0, 2, 0);
        cout << endl;
    }

}
void Stop() {
    char b = 0; int retcode = 0;
    do {
        Sleep(500);
        b = 'a';
        retcode = send(Client1, &b, sizeof(char), 0);
    } while (retcode != SOCKET_ERROR);
    ok = false;
}
void mainthread() {
    thread thr1(RecvKeyboard);
    thread thr2(SendImageFile);
    thread thr3(RecvMouse);
    thread thr4(Stop);
    thr1.join();
    thr2.join();
    thr3.join();
    thr4.join();


}
int main() {
    std::cout << "I am Server!\n";
    Connection(Server1, Client1, 3000);
    Connection(Server2, Client2, 3555);
    Connection(Server3, Client3, 4444);
    Connection(Server4, Client4, 5000);
    mainthread();

    closesocket(Server1);
    closesocket(Server2);
    closesocket(Server3);
    closesocket(Server4);
    WSACleanup();
    return 0;
}