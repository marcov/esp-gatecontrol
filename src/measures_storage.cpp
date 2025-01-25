#include "measures_storage.hpp"
#include "time_helpers.hpp"

void MeasurementBuffer::AddMeasure(unsigned int value, unsigned long duration_ms)
{
    // refuse to add too short durations.
    if (duration_ms < TIME_S_TO_MS(1))
    {
        return;
    }

    m_Buffer[m_CurrentPos] = {value, static_cast<unsigned int>(TIME_MS_TO_S(duration_ms))};
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
            binIndex = std::max(0U, binIndex);
            binIndex = std::min(binIndex, k_HistogramBinsCount - 1);
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
#ifdef UNIT_TESTS
            output += std::to_string(digit);
#else
            output += String(digit);
#endif
        }

        prev_digit = digit;
    }

    return output;
}

#ifdef UNIT_TESTS
#include <iostream>
#include <random>
void MeasurementBuffer::TestHistogram(void)
{
    // Randomize m_Buffer content
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned int> valueDist(1, k_MaxMeasureValue);
    std::uniform_int_distribution<unsigned int> durationDist(1, 200);

    for (auto &m : m_Buffer)
    {
        m.value = valueDist(gen);
        m.duration_s = durationDist(gen);
    }

    // Call GetHistogram and print the output
    std::string histogramOutput = GetHistogram();
    std::cout << "Generated Histogram:\n" << histogramOutput << std::endl;
}

int main()
{
    MeasurementBuffer mb;
    mb.TestHistogram();
    return 0;
}
#endif
