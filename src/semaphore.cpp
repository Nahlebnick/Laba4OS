#include "myLib/semaphore.h"

void myLib::Semaphore::create(LONG lInitialCount, LONG lMaximumCount, std::wstring lpName)
{
	hSemaphore = CreateSemaphore(NULL, lInitialCount, lMaximumCount, lpName.c_str());
	if (!hSemaphore)
	{
		DWORD err = GetLastError();
		throw std::system_error(static_cast<int>(err), std::system_category(), "CreateThread failed");
	}
}

void myLib::Semaphore::open(DWORD desiredAccess, bool InheritHandle, std::wstring lpName)
{
	hSemaphore = OpenSemaphore(desiredAccess, InheritHandle, lpName.c_str());
	if (!hSemaphore)
	{
		DWORD err = GetLastError();
		throw std::system_error(static_cast<int>(err), std::system_category(), "OpenThread failed");
	}
}

void myLib::Semaphore::wait(DWORD wait)
{
	if (hSemaphore)
	{
		DWORD res = WaitForSingleObject(hSemaphore, wait);
		switch (res)
		{
		case WAIT_OBJECT_0: break;
		case WAIT_TIMEOUT: throw std::runtime_error("Waiting time expired!"); break;
		case WAIT_FAILED: throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "WaitForSingleObject failed"); break;
		default: throw std::runtime_error("Unexpected result from WaitForSingleObject!"); break;
		}
		close();
	}
}

DWORD myLib::Semaphore::try_wait(DWORD wait)
{
	DWORD res = 0;
	if (hSemaphore)
	{
		DWORD res = WaitForSingleObject(hSemaphore, wait);
		close();
	}
	return res;
}

void myLib::Semaphore::release(LONG ReleaseCount)
{
	if (!ReleaseSemaphore(hSemaphore, ReleaseCount, nullptr))
		throw std::runtime_error("ReleaseSemaphore failed");
}

void myLib::Semaphore::close()
{
	if (hSemaphore)
	{
		bool res = CloseHandle(hSemaphore);
		if (!res)
		{
			throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "CloseHandle failed");
		}
		hSemaphore = nullptr;
	}
}
