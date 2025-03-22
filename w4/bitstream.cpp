#include "bitstream.h"

void BitStream::WriteBit(bool value)
{
    size_t byteIndex = m_WritePose / 8;
    size_t bitIndex = m_WritePose % 8;

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