#pragma once

#ifdef UNIT_TESTS
#include <string>
#include <array>
using String = std::string;
#else
#include <Arduino.h>
#endif

struct Measurement
{
    unsigned int value;
    unsigned int duration_s;
};

class MeasurementBuffer
{
  private:
    size_t m_CurrentPos = 0;
    unsigned long m_LastAddMs = 0;

    // Stores 24h with a measure every 40 seconds
    static constexpr size_t k_BufferSize = 2048;

    static constexpr unsigned int k_MaxMeasureValue = 3600;
    static constexpr unsigned int k_HistogramBinsCount = 60;
    static constexpr unsigned int k_HistogramMaxHeight = 10;

    std::array<Measurement, k_BufferSize> m_Buffer{};

  public:
    MeasurementBuffer() = default;
    ~MeasurementBuffer() = default;

    void AddMeasure(unsigned int value, unsigned long duration_ms);

    String GetHistogram(void) const;
#ifdef UNIT_TESTS
    void TestHistogram(void);
#endif
};
