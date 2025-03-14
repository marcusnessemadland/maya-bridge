/*
 * Copyright 2025 Marcus Nesse Madland. All rights reserved.
 * License: https://github.com/marcusnessemadland/vulkan-renderer/blob/main/LICENSE
 */

#pragma once

#include <string>
#include <mutex>
#include <cstring>

#include <windows.h> // Only contains windows implementation rn

class SharedBuffer 
{
public:
    bool init(const char* name, uint32_t size) 
    {
        m_name = std::string(name);
        m_size = size;

        m_filemap = CreateFileMapping(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            static_cast<DWORD>(m_size),
            m_name.c_str()
        );

        if (!m_filemap || m_filemap == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        m_buffer = MapViewOfFile(m_filemap, FILE_MAP_ALL_ACCESS, 0, 0, m_size);
        if (!m_buffer)
        {
            CloseHandle(m_filemap);
            return false;
        }

        return true;
    }

    void shutdown() 
    {
        if (m_buffer)
        {
            UnmapViewOfFile(m_buffer);
            m_buffer = nullptr;
        }

        if (m_filemap)
        {
            CloseHandle(m_filemap);
            m_filemap = nullptr;
        }
    }

    bool write(const void* data, uint32_t size) 
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (size > m_size)
        {
            return false;
        }
        std::memcpy(m_buffer, data, size);
        return true;
    }

    bool read(void* data, uint32_t size) 
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (size > m_size)
        {
            return false;
        }
        std::memcpy(data, m_buffer, size);
        return true;
    }

    void* getBuffer() 
    {
        return m_buffer;
    }

private:
    std::string m_name;
    std::mutex m_mutex;
    void* m_buffer = nullptr;
    uint32_t m_size = 0;
    HANDLE m_filemap = nullptr;
};

