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

void BitStream::WriteBytes(const void* data, size_t size)
{
    const std::uint8_t* bytes = static_cast<const std::uint8_t*>(data);
    for (size_t i = 0; i < size; i++)
    {
        WriteBit(bytes[i] & 0x01);
        WriteBit(bytes[i] & 0x02);
        WriteBit(bytes[i] & 0x04);
        WriteBit(bytes[i] & 0x08);
        WriteBit(bytes[i] & 0x10);
        WriteBit(bytes[i] & 0x20);
        WriteBit(bytes[i] & 0x40);
        WriteBit(bytes[i] & 0x80);
    }
}

void BitStream::ReadBytes(void* data, size_t size)
{
    std::uint8_t* bytes = static_cast<std::uint8_t*>(data);
    for (size_t i = 0; i < size; i++)
    {
        bytes[i] = 0;
        bytes[i] |= ReadBit() ? 0x01 : 0;
        bytes[i] |= ReadBit() ? 0x02 : 0;
        bytes[i] |= ReadBit() ? 0x04 : 0;
        bytes[i] |= ReadBit() ? 0x08 : 0;
        bytes[i] |= ReadBit() ? 0x10 : 0;
        bytes[i] |= ReadBit() ? 0x20 : 0;
        bytes[i] |= ReadBit() ? 0x40 : 0;
        bytes[i] |= ReadBit() ? 0x80 : 0;
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
    return buffer.size() * 8;
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