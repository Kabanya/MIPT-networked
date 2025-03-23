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
    BitStream(const BitStream&) = delete;
    BitStream(const std::uint8_t* data, std::size_t size)
    {
        buffer.assign(data, data + size);
    }

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

    void WriteBits(std::uint32_t value, std::uint8_t bitCount);
    std::uint32_t ReadBits(std::uint8_t bitCount);

    ///@name Операции с байтами
    ///@{
    /**
     * @brief Записывает массив байт в поток
     * @param data Указатель на данные для записи
     * @param size Размер данных в байтах
     */
    void WriteBytes(const void* data, size_t size);
    void ReadBytes(void* data, size_t size);
    ///@}


    /// @name Операции с template
    /// @{
    /**
     * @brief Записывает значение в поток
     * @tparam T Тип значения
     * @param value Значение для записи
     */
    template<typename T>
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
    void Read(T& value)
    {
        ReadBytes(&value, sizeof(T));
    }
    ///@}

    /**
     * Специализация шаблона для записи строки
     * @brief Записывает строку в поток
     * @param value Строка для записи
     */
    void Write(const std::string& value);
    void Read(std::string& value);

    /// @name Полезные методы
    /// @{
    /**
     * @brief Возвращает указатель на данные в потоке
     * @return Указатель на данные в потоке
     */
    const std::uint8_t* GetData() const;
    /**
     * @brief Возвращает размер данных в потоке в байтах
     * @return Размер данных в потоке в байтах
     */
    std::size_t GetSizeBytes() const;
    /**
     * @brief Возвращает размер данных в потоке в битах
     * @return Размер данных в потоке в битах
     */
    std::size_t GetSizeBits() const;
    /**
     * @brief Сбрасывает указатель чтения в начало потока
     */
    void ResetRead();
    /**
     * @brief Сбрасывает указатель записи в начало потока
     */
    void ResetWrite();
    /**
     * @brief Очищает поток
     */
    void Clear();
    ///@}
};
