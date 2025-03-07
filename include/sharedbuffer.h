#ifndef BX_SHARED_BUFFER_H_HEADER_GUARD
#define BX_SHARED_BUFFER_H_HEADER_GUARD

#include <bx/uint32_t.h>
#include <bx/mutex.h>

namespace bx
{
	struct SharedBufferI
	{
		virtual bool init(const char* _name, const uint32_t& _size) = 0;
		virtual void shutdown() = 0;
		virtual bool write(const void* _data, uint32_t _size) = 0;
		virtual bool read(void* _data, uint32_t _size) = 0;
		virtual void* getBuffer() = 0;
	};

} // namespace bx

#if 1 //BX_PLATFORM_WINDOWS @todo

#include <Windows.h>
#include <string> // string

namespace bx
{
	struct SharedBuffer : SharedBufferI
	{
		bool SharedBuffer::init(const char* _name, const uint32_t& _size) override
		{
			m_name = std::string(_name);
			m_size = _size;

			m_filemap = CreateFileMapping(
				INVALID_HANDLE_VALUE,
				nullptr,
				PAGE_READWRITE,
				0,
				static_cast<DWORD>(m_size),
				m_name.c_str()
			);

			if (m_filemap == NULL || m_filemap == INVALID_HANDLE_VALUE)
			{
				BX_TRACE("Failed to create native file mapping object");
				return false;
			}

			m_buffer = MapViewOfFile(
				m_filemap,
				FILE_MAP_ALL_ACCESS,
				0,
				0,
				m_size
			);

			if (m_buffer == NULL)
			{
				CloseHandle(m_filemap);
				BX_TRACE("Could not map view of file.");
				return false;
			}

			return true;
		}

		void SharedBuffer::shutdown() override
		{
			if (m_buffer != NULL)
			{
				UnmapViewOfFile(m_buffer);
			}

			if (m_filemap != NULL)
			{
				CloseHandle(m_filemap);
			}
		}

		bool SharedBuffer::write(const void* _data, uint32_t _size) override
		{
			bx::MutexScope BX_CONCATENATE(mutexScope, __LINE__)(m_resourceApiLock);

			if (_size > m_size)
			{
				BX_TRACE("Not enough space to write to shared buffer.")
				return false;
			}

			bx::memCopy(m_buffer, _data, _size);
			return true;
		}

		bool SharedBuffer::read(void* _data, uint32_t _size) override
		{
			bx::MutexScope BX_CONCATENATE(mutexScope, __LINE__)(m_resourceApiLock);

			if (_size > m_size)
			{
				BX_TRACE("Not enough space to read from shared buffer.")
				return false;
			}

			bx::memCopy(_data, m_buffer, _size);
			return true;
		}

		void* SharedBuffer::getBuffer() override
		{
			return m_buffer;
		}

		std::string m_name;
		bx::Mutex m_resourceApiLock;

		void* m_buffer;
		uint32_t m_size;

		HANDLE   m_filemap;
	};

} // namespace bx

#else

namespace bx
{
	struct SharedBuffer : SharedBufferI
	{
		bool SharedBuffer::init(const char* _name, const uint32_t& _size)
		{
			return false;
		}

		void SharedBuffer::shutdown()
		{
		}

		bool SharedBuffer::write(const void* _data, uint32_t _size)
		{
			return false;
		}

		bool SharedBuffer::read(void* _data, uint32_t _size)
		{
			return false;
		}

		void* SharedBuffer::getBuffer()
		{
			return false;
		}
	};

} // namespace bx

#endif // BX_PLATFORM_WINDOWS

#endif // BX_SHARED_BUFFER_H_HEADER_GUARD
