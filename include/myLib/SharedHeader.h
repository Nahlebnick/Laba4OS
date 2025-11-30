#pragma once

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <string>
#include <cstring>

class SharedQueue {
public:
    static constexpr size_t MaxMessageLen = 20;
    static constexpr size_t SlotSize = MaxMessageLen + 1; // +'\0'

#pragma pack(push, 1)
    struct SharedHeader {
        LONG capacity;
        LONG head;
        LONG tail;
    };
#pragma pack(pop)

    SharedQueue() = default;
    ~SharedQueue() { close(); }

    // Создаёт новый файл/очередь (Receiver)
    bool create(const std::wstring& filePath, LONG capacity) {
        close();
        HANDLE hFile = CreateFileW(
            filePath.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        if (hFile == INVALID_HANDLE_VALUE) {
            return false;
        }

        LARGE_INTEGER fileSize;
        fileSize.QuadPart = sizeof(SharedHeader) + static_cast<LONGLONG>(capacity) * SlotSize;

        bool ok = SetFilePointerEx(hFile, fileSize, nullptr, FILE_BEGIN) && SetEndOfFile(hFile);
        if (!ok) {
            CloseHandle(hFile);
            return false;
        }

        if (!mapHandle(hFile)) {
            CloseHandle(hFile);
            return false;
        }

        CloseHandle(hFile);

        SIZE_T total = static_cast<SIZE_T>(fileSize.QuadPart);
        ZeroMemory(header_, total);
        header_->capacity = capacity;
        header_->head = 0;
        header_->tail = 0;

        return true;
    }

    // Открывает существующий файл/очередь (Sender)
    bool open(const std::wstring& filePath) {
        close();
        HANDLE hFile = CreateFileW(
            filePath.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        if (hFile == INVALID_HANDLE_VALUE) {
            return false;
        }

        if (!mapHandle(hFile)) {
            CloseHandle(hFile);
            return false;
        }

        CloseHandle(hFile);
        return true;
    }

    void close() {
        if (header_) {
            UnmapViewOfFile(header_);
            header_ = nullptr;
            slots_ = nullptr;
        }
        if (hMap_) {
            CloseHandle(hMap_);
            hMap_ = nullptr;
        }
    }

    SharedHeader* header() { return header_; }
    const SharedHeader* header() const { return header_; }

    bool read(std::string& out) {
        if (!header_) return false;
        LONG head = header_->head;
        char buffer[SlotSize]{};
        std::memcpy(buffer, slotPtr(head), SlotSize);
        header_->head = (head + 1) % header_->capacity;
        out.assign(buffer);
        return true;
    }

    bool write(const std::string& message) {
        if (!header_ || message.size() > MaxMessageLen) return false;
        LONG tail = header_->tail;
        char buffer[SlotSize]{};
        std::memcpy(buffer, message.c_str(), message.size());
        std::memcpy(slotPtr(tail), buffer, SlotSize);
        header_->tail = (tail + 1) % header_->capacity;
        return true;
    }

private:
    HANDLE hMap_ = nullptr;
    SharedHeader* header_ = nullptr;
    char* slots_ = nullptr;

    bool mapHandle(HANDLE hFile) {
        hMap_ = CreateFileMappingW(hFile, nullptr, PAGE_READWRITE, 0, 0, nullptr);
        if (!hMap_) {
            return false;
        }
        header_ = static_cast<SharedHeader*>(MapViewOfFile(hMap_, FILE_MAP_ALL_ACCESS, 0, 0, 0));
        if (!header_) {
            CloseHandle(hMap_);
            hMap_ = nullptr;
            return false;
        }
        slots_ = reinterpret_cast<char*>(header_ + 1);
        return true;
    }

    char* slotPtr(LONG index) const {
        return slots_ + static_cast<size_t>(index) * SlotSize;
    }
};