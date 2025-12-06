#define UNICODE
#define _UNICODE
#include <windows.h>
#include <iostream>
#include <string>
#include <limits>
#include "myLib/inputUtils.h"
#include "myLib/semaphore.h"
#include "myLib/process.h"
#include "myLib/SharedHeader.h"

void Fail(const std::wstring& msg) {
    std::wcerr << L"[Sender] Ошибка: " << msg << L"\n";
    ExitProcess(EXIT_FAILURE);
}

int wmain(int argc, wchar_t* argv[]) {
    std::ios::sync_with_stdio(false);

    if (argc < 3)
    {
        std::cerr << "Incorrect data passed to the process\n";
        return -1;
    }

    std::wstring filePath = argv[1];
    std::wstring token = argv[2];

    SharedQueue s_q;
    if (s_q.open(filePath))
    {
        std::cerr << "SharedQueue::open failed!\n";
        return -1;
    }

    myLib::Semaphore ReadySem;
    ReadySem.open(SEMAPHORE_MODIFY_STATE, FALSE, L"ReadySemaphore");
    myLib::Semaphore EmptySem;        
    EmptySem.open(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, L"EmptySemaphore");
    myLib::Semaphore FullSem;
    FullSem.open(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, L"FullSemaphore");

    HANDLE hMutex = OpenMutex(SYNCHRONIZE, FALSE, L"MTX");

    if (!hMutex)
    {
        std::cerr << "CreateMutex failed!\n";
        return -1;
    }

    ReadySem.release(1);
    std::wcout << L"[Sender] Ready.\n";

    bool running = true;
    while (running)
    {
        std::wcout << L"\nCommands: (s)end, (q)uit > ";
        std::wstring command;
        std::getline(std::wcin, command);
        if (command.empty()) continue;

        switch (command[0])
        {
        case L's':
        case L'S': {
            std::wcout << L"Enter message (<20 symbols): ";
            std::string message;
            std::getline(std::cin, message);

            if (message.size() > SharedQueue::MaxMessageLen)
            {
                std::wcout << L"Message too long.\n";
                break;
            }

            EmptySem.wait();

            WaitForSingleObject(hMutex, INFINITE);

            if (!s_q.write(message))
            {
                std::wcerr << L"Write in queue failed.\n";
            }
            ReleaseMutex(hMutex);

            FullSem.release(1);
            std::cout << "[Sender] Message is sent.\n";
            break;
        }
        case L'q':
        case L'Q':
            running = false;
            break;
        default:
            std::cout << "Unknown command.\n";
            break;
        }
    }

    s_q.close();
    CloseHandle(hMutex);
    return 0;
}