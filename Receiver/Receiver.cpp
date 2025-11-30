// Receiver.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "myLib/inputUtils.h"
#include "myLib/semaphore.h"
#include "myLib/process.h"

const int MAX_MESSAGE_LEN = 20;

std::wstring BuildSenderCommandLine(const std::wstring& senderExe, const std::wstring& filePath)
{
    std::wstring cmd = L"\"" + senderExe + L"\" \"" + filePath;
    return cmd;
}

std::wstring string_to_wstring(const std::string& str, UINT code_page = CP_UTF8) {
    if (str.empty()) return L"";

    int wide_size = MultiByteToWideChar(code_page, 0, str.c_str(), -1, nullptr, 0);

    if (wide_size == 0) return L"";

    std::wstring wide_str(wide_size, 0);
    int wide_size = MultiByteToWideChar(code_page, 0, str.c_str(), -1, &wide_str[0], wide_size);

    wide_str.pop_back();
    return wide_str;
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Incorrect data passed to the process";
        return -1;
    }

    std::string filePath = argv[1];
    int recordCount = std::stoi(argv[2]);

    if (recordCount < 0)
    {
        std::cerr << "Count of records should be positive!";
        return -1;
    }

    FILE* f = fopen(filePath.c_str(), "wb");

    if (!f)
    {
        std::cerr << "Failed to open file!";
        return -1;
    }

    int processCount = 0;

    std::cout << "Enter count of processes: ";
    inputValue(processCount);

    if (processCount < 0)
    {
        std::cerr << "Count of processes should be positive!";
        return -1;
    }

    myLib::Semaphore ReadySem((LONG)0, (LONG)1024, L"ReadySemaphore");
    myLib::Semaphore EmptySem((LONG)0, (LONG)recordCount, L"EmptySemaphore");
    myLib::Semaphore FullSem((LONG)0, (LONG)recordCount, L"FullSemaphore");
    HANDLE hMutex = CreateMutex(NULL, false, L"MTX");

    std::vector<myLib::Process> processes;

    for (int i = 0; i < processCount; i++)
    {
        myLib::Process proc(BuildSenderCommandLine(L"sender.exe", string_to_wstring(filePath)));
        processes.push_back(proc);
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
            DWORD waitRes = FullSem.try_wait();
            if (waitRes != WAIT_OBJECT_0)
            {
                std::cerr << "Ошибка ожидания FullSem.\n";
                break;
            }

            WaitForSingleObject(hMutex, INFINITE);

            ReleaseMutex(hMutex);


        }
        else if (command[0] == 'Q' || command[0] == 'q')
        {
            running = false;
            break;
        }
        else
        {
            std::cerr << "Incorrect input. Try again\n";
            continue;
        }
    }

}
