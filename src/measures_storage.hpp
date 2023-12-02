#pragma once

#include "time_helpers.hpp"

#include <Arduino.h>

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

    // Fits a measure every 40 seconds
    static constexpr size_t k_BufferSize = 2048;

    static constexpr unsigned int k_MaxMeasureValue = 3600;
    static constexpr unsigned int k_HistogramBinsCount = 60;
    static constexpr unsigned int k_HistogramMaxHeight = 10;

    // Store 24 hours of data.
    std::array<Measurement, k_BufferSize> m_Buffer{};

  public:
    MeasurementBuffer() = default;
    ~MeasurementBuffer() = default;

    void AddMeasure(unsigned int value, unsigned long duration_ms);

    String GetHistogram(void) const;
};
