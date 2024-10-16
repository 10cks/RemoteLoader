#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>

class HttpRequest {
public:
    static HINTERNET OpenSession();
    static HINTERNET ConnectToServer(HINTERNET hSession, const std::wstring& szServerName, INTERNET_PORT nServerPort);
    static HINTERNET OpenRequest(HINTERNET hConnect, const std::wstring& szPath);
    static bool SendRequest(HINTERNET hRequest);
    static bool ReceiveResponse(HINTERNET hRequest);
    static void* ReadResponse(HINTERNET hRequest, void* lpAddress, SIZE_T sDataSize);
    static void CloseHandles(HINTERNET hRequest, HINTERNET hConnect, HINTERNET hSession);

private:
    static void* VxMoveMemory(void* dest, const void* src, SIZE_T len);
};

#endif /* HTTP_REQUEST_HPP */