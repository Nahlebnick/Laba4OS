#ifndef Semaphore_H
#define Semaphore_H

#include <iostream>
#include <functional>
#include <windows.h>
#include <format>

namespace myLib {
class Semaphore
{
	HANDLE hSemaphore;
public:
	Semaphore(LONG lInitialCount, LONG lMaximumCount, std::wstring lpName);

	Semaphore(DWORD desiredAccess, bool InheritHandle, std::wstring lpName);

	Semaphore(const Semaphore&) = delete;
	Semaphore& operator=(const Semaphore&) = delete;

	Semaphore(Semaphore&& other) noexcept = default;
	Semaphore& operator=(Semaphore&&) noexcept = default;
	~Semaphore() noexcept { close(); }
	
	void wait(DWORD wait=INFINITE);
	DWORD try_wait(DWORD wait = INFINITE);

	void release(LONG ReleaseCount);

	HANDLE native_handle() { return hSemaphore; }

private:
	void close();
};

}
#endif // Semaphore_H