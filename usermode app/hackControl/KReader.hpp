#pragma once

#include <string>
#include <exception>
#include "Types.hpp"


class KReader
{
public:
	KReader() : m_hDriver(0), m_PUBase(0)
	{
		m_hDriver = CreateFileA("\\\\.\\MapHackInterface", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
		if (!m_hDriver)
		{
			throw std::runtime_error("Could not open the driver handle!");
		}

	}
	~KReader()
	{
		CloseHandle(m_hDriver);
	}

	/*
	 * CLASS METHODS
	 */
	int64_t readPUBase()
	{
		int64_t base = 0;

		void* read = nullptr;

		// create a struct that requests the base address
		readStruct rStruct{ (uint64_t)&base, (uint64_t)read, sizeof(int64_t), (uint32_t)GetCurrentProcessId(), 0, TRUE, 1 };
		// send the struct to IOCTL
		WriteFile(m_hDriver, (LPCVOID)&rStruct, sizeof(ReadStruct), NULL, NULL);

		if (base)
		{
			m_PUBase = base;
		}

		return base;
	}

	template<typename T>
	T readType(const int64_t& w_read, const PROTO_MESSAGE& w_protoMsg) const
	{
		T writeMe;

		if (w_protoMsg == PROTO_NORMAL_READ)
		{
			readStruct rStruct{ (uint64_t)&writeMe, (uint64_t)w_read, sizeof(T), (uint32_t)GetCurrentProcessId(), 0, TRUE, 0 };
			// send the struct to IOCTL
			WriteFile(m_hDriver, (LPCVOID)&rStruct, sizeof(ReadStruct), NULL, NULL);
		}

		return writeMe;
	}

	// caution: if you use this method, you need to delete the allocated heap from where ever you called this method from
	byte* readSize(const int64_t& w_read, const int32_t& w_readSize, const PROTO_MESSAGE& w_protoMsg) const
	{
		byte* writeMe = new byte[w_readSize];
		// memset(paluu, 0, readSize);

		if (w_protoMsg == PROTO_NORMAL_READ)
		{
			readStruct rStruct{ (uint64_t)writeMe, (uint64_t)w_read, w_readSize - 2, (uint32_t)GetCurrentProcessId(), 0, TRUE, 0 };
			// send the struct to IOCTL
			WriteFile(m_hDriver, (LPCVOID)&rStruct, sizeof(ReadStruct), NULL, NULL);
		}

		if (*writeMe == 0)
		{
			delete writeMe;
			return NULL;
		}

		return writeMe;
	}

	Vector3 readVec(const int64_t& w_read, const PROTO_MESSAGE& w_protoMsg) const
	{
		Vector3 writeMe;

		if (w_protoMsg == PROTO_NORMAL_READ)
		{
			readStruct luku{ (uint64_t)&writeMe, (uint64_t)w_read, sizeof(Vector3), (uint32_t)GetCurrentProcessId(), 0, TRUE, 0 };
			// send the struct to IOCTL
			WriteFile(m_hDriver, (LPCVOID)&luku, sizeof(ReadStruct), NULL, NULL);
		}

		return writeMe;
	}

	// returns a string, if this method fails, returns "FAIL"
	std::string getGNameFromId(const int32_t& w_id) const
	{
		int64_t GNames = readType<int64_t>(m_PUBase + 0x36DA610, PROTO_NORMAL_READ);
		int64_t singleNameChunk = readType<int64_t>(GNames + (w_id / 0x4000) * 8, PROTO_NORMAL_READ);
		int64_t singleNamePtr = readType<int64_t>(singleNameChunk + 8 * (w_id % 0x4000), PROTO_NORMAL_READ);

		char* name;
		if ((name = (char*)readSize(singleNamePtr + 16, 64, PROTO_NORMAL_READ)) != NULL)
		{
			std::string s = std::string(name);
			delete name;
			return s;
		}
		else
		{
			return std::string("FAIL");
		}
	}

	/*
	 * GETTERS AND SETTERS
	 */
	HANDLE getKernHandle() const
	{
		return m_hDriver;
	}

	int64_t getPUBase() const
	{
		return m_PUBase;
	}

	bool isReading() const
	{
		return m_readActive;
	}

private:
	HANDLE m_hDriver;
	int64_t m_PUBase;
	bool m_readActive;
};