#pragma once

// #include <iostream>
#include <vector>
#include <cstdint> // Для std::uint8_t

class BitStream
{
private:
    std::vector<std::uint8_t> buffer;
    size_t m_WritePose = 0;
    size_t m_ReadPose = 0;

public:
    BitStream() = default;
    // BitStream();
    ~BitStream() = default;

    ///@name Побитовые операции
    ///@{
    /**
     * @brief Записывает один бит в поток
     * @param value Значение бита для записи
     */
    void WriteBit(bool value);
    /**
     * @brief Читает один бит из потока
     * @return Значение прочитанного бита
     */
    bool ReadBit();
    ///@}

    ///@name Операции с байтами
    ///@{
    /**
     * @brief Записывает массив байт в поток
     * @param data Указатель на данные для записи
     * @param size Размер данных в байтах
     */
    void WriteBytes(const void* data, size_t size);
    /**
     * @brief Читает массив байт из потока
     * @param data Указатель на буфер для чтения
     * @param size Размер данных в байтах
     */
    void ReadBytes(void* data, size_t size);
    ///@}


    /// @name Операции с template
    /// @{
    /**
     * @brief Записывает значение в поток
     * @tparam T Тип значения
     * @param value Значение для записи
     */
    template <typename T>
    void Write(const T& value)
    {
        WriteBytes(&value, sizeof(T));
    }

    /**
     * @brief Читает значение из потока
     * @tparam T Тип значения
     * @return Прочитанное значение
     */
    template <typename T>
    T Read()
    {
        T value;
        ReadBytes(&value, sizeof(T));
        return value;
    }
    ///@}

    /// @name Полезные методы
    const std::uint8_t* Data() const;
    size_t GetSizeBytes() const;
    size_t GetSizeBits() const;
    void ResetRead();
    void ResetWrite();
    void Clear();
    ///@}
};
