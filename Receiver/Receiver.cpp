// Receiver.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "myLib/inputUtils.h"
#include "myLib/semaphore.h"
#include "myLib/process.h"
#include "myLib/SharedHeader.h"

const int MAX_MESSAGE_LEN = 20;

std::wstring BuildSenderCommandLine(const std::wstring& senderExe, const std::wstring& filePath)
{
    std::wstring cmd = L"\"" + senderExe + L"\" \"" + filePath;
    return cmd;
}

std::wstring string_to_wstring(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

int main()
{
    std::cout << "Enter file path: ";
    std::string filePath;
    std::cin >> filePath;

    std::cout << "Enter record count: ";
    int recordCount;
    inputValue(recordCount);

    if (recordCount < 0)
    {
        std::cerr << "Count of records should be positive!\n";
        return -1;
    }

    int processCount = 0;

    std::cout << "Enter count of processes: ";
    inputValue(processCount);

    if (processCount < 0)
    {
        std::cerr << "Count of processes should be positive!\n";
        return -1;
    }

    SharedQueue s_q;
    if (s_q.create(string_to_wstring(filePath), recordCount))
    {
        std::cerr << "SharedQueue::create failed!\n";
        return -1;
    }
    myLib::Semaphore ReadySem;
    ReadySem.create((LONG)0, (LONG)1024, L"ReadySemaphore");
    myLib::Semaphore EmptySem;
    EmptySem.create((LONG)0, (LONG)recordCount, L"EmptySemaphore");
    myLib::Semaphore FullSem;
    FullSem.create((LONG)0, (LONG)recordCount, L"FullSemaphore");
    HANDLE hMutex = CreateMutex(NULL, false, L"MTX");

    if (!hMutex)
    {
        std::cerr << "CreateMutex failed!\n";
        return -1;
    }

    std::vector<std::unique_ptr<myLib::Process>> processes;

    for (int i = 0; i < processCount; i++)
    {
        auto proc = std::make_unique<myLib::Process>(BuildSenderCommandLine(L"sender.exe", string_to_wstring(filePath)));
        processes.push_back(std::move(proc));
    }

    std::cout << "[Receiver] Waiting for Sender...\n";
    for (int i = 0; i < processCount; ++i)
    {
        ReadySem.wait();
    }
    std::cout << "[Receiver] All senders are ready...\n";

    bool running = true;
    while (running)
    {
        std::cout << "\nCommands: (r)ead, (q)uit > ";
        std::wstring command;
        std::getline(std::wcin, command);

        if (command[0] == 'R' || command[0] == 'r')
        {
            FullSem.wait();
            

            WaitForSingleObject(hMutex, INFINITE);
            std::string message;
            if (!s_q.read(message))
            {
                std::cerr << "Error while reading message!\n";
            }
            ReleaseMutex(hMutex);

            EmptySem.release(1);

            std::string msg(message.begin(), message.end());
            std::cout << "[Receiver] Received message: " << msg << "\n";
            break;
        }
        else if (command[0] == 'Q' || command[0] == 'q')
        {
            running = false;
        }
        else
        {
            std::cerr << "Incorrect input. Try again\n";
            continue;
        }
    }

    s_q.close();
    CloseHandle(hMutex);

    return 0;

}
