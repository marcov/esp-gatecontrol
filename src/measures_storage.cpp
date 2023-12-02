#include "measures_storage.hpp"
#include <algorithm>
#include <string>

void MeasurementBuffer::AddMeasure(unsigned int value, unsigned long duration_ms)
{
    // refuse to add too short durations.
    if (duration_ms < TIME_S_TO_MS(1))
    {
        return;
    }

    m_Buffer[m_CurrentPos] = {value, TIME_MS_TO_S(duration_ms)};
    m_CurrentPos = (m_CurrentPos + 1) % m_Buffer.size();
}

String MeasurementBuffer::GetHistogram(void) const
{
    String output;

    constexpr int k_BindWidth = k_MaxMeasureValue / k_HistogramBinsCount;
    static_assert(k_BindWidth != 0, "Bin width is zero!");

    std::array<unsigned int, k_HistogramBinsCount> histogram{};

    for (const auto &m : m_Buffer)
    {
        if (m.duration_s != 0)
        {
            unsigned int binIndex = static_cast<int>(m.value / k_BindWidth);
            binIndex = max(0U, binIndex);
            binIndex = min(binIndex, k_HistogramBinsCount - 1);
            histogram[binIndex] += m.duration_s;
        }
    }

    const int maxHistoValue = *std::max_element(histogram.begin(), histogram.end());

    // normalize the bars
    if (maxHistoValue != 0)
    {
        // normalize to a maximum height
        std::for_each(histogram.begin(), histogram.end(),
                      [maxHistoValue](auto &n) { n = (n * k_HistogramMaxHeight) / maxHistoValue; });

        // Scan top down from taller column to the shorter
        for (unsigned int row = k_HistogramMaxHeight; row > 0; --row)
        {
            for (unsigned int col = 0; col < histogram.size(); ++col)
            {
                if (histogram[col] >= row)
                {
                    output += "█";
                }
                else
                {
                    output += " ";
                }
            }
            output += "\n";
        }
    }

    // Display the bin labels below each column
    int prev_digit = 0;
    for (unsigned int col = 0; col < k_HistogramBinsCount; ++col)
    {
        // Get hundred digit, e.g. "2" in 1234.
        auto get_hundred_digit = [](int value){
            return (value / 100) % 10;
        };

        const int startValue = col * k_BindWidth;
        const int digit = get_hundred_digit(startValue);

        // When the value wraps, insert a separator instead of the digit.
        if (digit < prev_digit)
        {
            output += "|";
        }
        else
        {
            output += String(digit);
        }

        prev_digit = digit;
    }

    return output;
}
