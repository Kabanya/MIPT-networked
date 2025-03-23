#include "bitstream.h"

void BitStream::WriteBit(bool value)
{
    std::size_t byteIndex = m_WritePose / 8;
    std::size_t bitIndex = m_WritePose % 8;

    if (byteIndex >= buffer.size())
    {
        buffer.push_back(0);
    }

    if (value)
    {
        buffer[byteIndex] |= (1 << bitIndex);
    }

    m_WritePose++;
}

bool BitStream::ReadBit()
{
    size_t byteIndex = m_ReadPose / 8;
    size_t bitIndex = m_ReadPose % 8;

    if (byteIndex >= buffer.size())
    {
        return false;
    }

    bool result = (buffer[byteIndex] & (1 << bitIndex)) != 0;
    m_ReadPose++;
    return result;
}

void BitStream::WriteBits(std::uint32_t value, std::uint8_t bitCount)
{
    for (std::uint8_t i = 0; i < bitCount; ++i)
    {
        WriteBit((value & (1 << i)) != 0);
    }
}

std::uint32_t BitStream::ReadBits(std::uint8_t bitCount)
{
    std::uint32_t value = 0;
    for (std::uint8_t i = 0; i < bitCount; ++i)
    {
        if (ReadBit())
            value |= (1 << i);
    }
    return value;
}

void BitStream::WriteBytes(const void* data, size_t size)
{
    size_t byteIndex = m_WritePose / 8;
    
    if (byteIndex + size > buffer.size())
    {
        buffer.resize(byteIndex + size);
    }
    
    std::memcpy(buffer.data() + byteIndex, data, size);
    
    m_WritePose += size * 8;
}

void BitStream::ReadBytes(void* data, size_t size)
{
    size_t byteIndex = m_ReadPose / 8;
    
    // Ensure we have enough data
    if (byteIndex + size > buffer.size())
    {
        throw std::out_of_range("Attempting to read beyond buffer");
    }
    
    // Copy bytes
    std::memcpy(data, buffer.data() + byteIndex, size);
    
    // Update bit position
    m_ReadPose += size * 8;
}


void BitStream::Write(const std::string& value)
{
    uint32_t length = static_cast<uint32_t>(value.length());
    WriteBits(length, 32);
    if (length > 0)
    {
        WriteBytes(value.data(), length);
    }
}

void BitStream::Read(std::string& value)
{
    std::uint8_t size = 0;
    Read(size);
    value.resize(size);
    for (std::uint8_t i = 0; i < size; i++)
    {
        Read(value[i]);
    }
}

const std::uint8_t* BitStream::GetData() const
{
    return buffer.data();
}

std::size_t BitStream::GetSizeBits() const
{
    return buffer.size() * 8;
}

std::size_t BitStream::GetSizeBytes() const
{
    return buffer.size();
}

void BitStream::ResetRead()
{
    m_ReadPose = 0;
}

void BitStream::ResetWrite()
{
    m_WritePose = 0;
}

void BitStream::Clear()
{
    buffer.clear();
    m_WritePose = 0;
    m_ReadPose = 0;
}
