#include "http_request.hpp"
#include <cstring>

void* HttpRequest::VxMoveMemory(void* dest, const void* src, SIZE_T len)
{
    char* d = static_cast<char*>(dest);
    const char* s = static_cast<const char*>(src);
    if (d < s)
        while (len--)
            *d++ = *s++;
    else
    {
        const char* lasts = s + (len - 1);
        char* lastd = d + (len - 1);
        while (len--)
            *lastd-- = *lasts--;
    }
    return dest;
}

HINTERNET HttpRequest::OpenSession()
{
    return WinHttpOpen(L"WinHTTP Example/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
}

HINTERNET HttpRequest::ConnectToServer(HINTERNET hSession, const std::wstring& szServerName, INTERNET_PORT nServerPort)
{
    return WinHttpConnect(hSession, szServerName.c_str(), nServerPort, 0);
}

HINTERNET HttpRequest::OpenRequest(HINTERNET hConnect, const std::wstring& szPath)
{
    return WinHttpOpenRequest(hConnect, L"GET", szPath.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
}

bool HttpRequest::SendRequest(HINTERNET hRequest)
{
    return WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) != 0;
}

bool HttpRequest::ReceiveResponse(HINTERNET hRequest)
{
    return WinHttpReceiveResponse(hRequest, NULL) != 0;
}

void* HttpRequest::ReadResponse(HINTERNET hRequest, void* lpAddress, SIZE_T sDataSize)
{
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    DWORD_PTR hptr = reinterpret_cast<DWORD_PTR>(lpAddress);
    do
    {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;

        std::vector<unsigned char> pszOutBuffer(dwSize + 1, 0);
        if (!WinHttpReadData(hRequest, pszOutBuffer.data(), dwSize, &dwDownloaded)) break;

        VxMoveMemory(reinterpret_cast<void*>(hptr), pszOutBuffer.data(), dwSize);
        hptr += dwSize;
    } while (dwSize > 0);
    return lpAddress;
}

void HttpRequest::CloseHandles(HINTERNET hRequest, HINTERNET hConnect, HINTERNET hSession)
{
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
}