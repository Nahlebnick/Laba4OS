#include "myLib/process.h"

myLib::Process::Process(const std::wstring& commandLine, bool bInheritHandles = FALSE, DWORD dwCreationFlags = 0, const std::wstring& currentDirectory)
{
    wchar_t* wtext = new wchar_t[commandLine.size()+11];
    wcscpy(wtext, commandLine.c_str());
    LPWSTR lpszCommandLine = wtext;

    STARTUPINFO si = { };
    pi = {};

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    if (!CreateProcess(NULL, lpszCommandLine,
        NULL, NULL, bInheritHandles, dwCreationFlags, NULL, NULL, &si, &pi))
    {
        throw std::system_error(GetLastError(), std::system_category(), "CreateProcess failed" );
    }
    delete[] wtext;
}

void myLib::Process::wait(DWORD wait)
{
    if (pi.hProcess)
    {
        DWORD res = WaitForSingleObject(pi.hProcess, wait);
        switch (res)
        {
        case WAIT_OBJECT_0: break;
        case WAIT_FAILED: throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "WaitForSingleObject failed"); break;
        case WAIT_TIMEOUT: throw std::runtime_error("Waiting time expired!"); break;
        default: throw std::runtime_error("Unexpected result from WaitForSingleObject");
            break;
        }
    }   
}

bool myLib::Process::terminate(UINT exitCode)
{
    bool res = TerminateProcess(pi.hProcess, exitCode);
    return res;
}

void myLib::Process::close()
{
    if (pi.hProcess)
    {
        CloseHandle(pi.hProcess);
    }

    if (pi.hThread)
    {
        CloseHandle(pi.hThread);
    }

    pi.hProcess = nullptr;
    pi.hThread = nullptr;
}
