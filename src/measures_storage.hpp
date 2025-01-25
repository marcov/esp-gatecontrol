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
    size_t m_BufferIdx = 0;
    unsigned long m_LastAddMs = 0;

    // Stores 24h with a measure every 40 seconds
    static constexpr size_t k_BufferSize = 2048;

    static constexpr unsigned int k_MaxMeasureValue = 3600;
    static constexpr unsigned int k_MaxMeasureLo = 1000;
    static constexpr unsigned int k_HistogramMaxHeight = 10;

    std::array<Measurement, k_BufferSize> m_Buffer{};

    static constexpr unsigned int k_HistogramBinsCountLo = 40;
    static constexpr unsigned int k_HistogramBinsCountHi = 20;
    static constexpr unsigned int k_HistogramBinsCountTotal = 60;
    static constexpr int k_BinWidthLo = k_MaxMeasureLo / k_HistogramBinsCountLo;
    static constexpr int k_BinWidthHi = (k_MaxMeasureValue - k_MaxMeasureLo) / k_HistogramBinsCountHi;
    static_assert(k_BinWidthLo != 0, "Lo Bin width is zero!");
    static_assert(k_BinWidthHi != 0, "Hi Bin width is zero!");
    static_assert(k_HistogramBinsCountLo + k_HistogramBinsCountHi == k_HistogramBinsCountTotal, "Lo Bin width is zero!");

  public:
    MeasurementBuffer() = default;
    ~MeasurementBuffer() = default;

    void AddMeasure(unsigned int value, unsigned long duration_ms);

    String GetHistogram(void) const;
#ifdef UNIT_TESTS
    void TestHistogram(void);
#endif
};
