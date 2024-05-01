#include <cstdint>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>


/**
 * @brief Represents a single period of an audio tone
 */
template<typename TSample>

class Tone {

public:

    /**
     * @brief Waveform shape
     */
     enum Waveform { Sine, Square };

public:

    /**
     *
     * @param frequency Frequency in Hz.
     * @param level Level in dB. Default 0.
     * @param waveform Waveform type. Default 'Sine'.
     */
#if 0
    Tone(uint32_t frequency, double level = 0, Waveform waveform = Sine)
    {
        _length = _sampleRate / frequency;
        _samples = new TSample[_length];
        double normalized = 0;
        for (uint32_t i = 0; i < _length; i++)
        {
            switch (waveform) {
                case Sine:
                    normalized = sin(_D_PI * i / static_cast<double>(_length));
                    break;
                case Square:
                    normalized = (i % (_length / 2) == 0) ? 1 : -1;
                    break;
            }
            if (level < 0) normalized *= pow(10.0, 0.05 * level);
            _samples[i] = normalized * 32767; // Scale to max 16-bit integer value.
        }
    }
#endif

    /// @brief Frees the memory if the instance goes out of scope.
    ~Tone() { delete[] _samples; }

    // Generate a frequency sweep from startFreq to endFreq over duration seconds.
    std::vector<TSample> generateSweep(uint32_t startFreq, uint32_t endFreq, double duration, Waveform waveform = Sine)
    {
        uint32_t totalSamples = static_cast<uint32_t>(_sampleRate * duration);
        std::vector<TSample> samples(totalSamples);
        double freqStep = (endFreq - startFreq) / duration; // Frequency increment per second.

        for (uint32_t i = 0; i < totalSamples; i++) {
            double time = static_cast<double>(i) / _sampleRate;
            double freq = startFreq + freqStep * time;
            double angle = _D_PI * freq * time;

            double normalized = 0;
            switch (waveform)
            {
                case Sine:
                    normalized = sin(angle);
                    break;
                case Square:
                    normalized = sin(angle) > 0 ? 1 : -1;
                    break;
            }
            samples[i] = static_cast<TSample>(normalized * 32767); // Scale to max 16-bit integer value.
        }
        return samples;
    }

    /// @brief Sets the sample rate (in samples per second) used to generate tones.
    static void setSampleRate(uint32_t sampleRate) {_sampleRate = sampleRate; }

    /// @brief Returns the data buffer pointer
    template<typename T = uint8_t*>
    T data() { return reinterpret_cast<T>(_samples); }

    /// @brief Returns the data buffer size in bytes.
    uint32_t size() { return static_cast<uint32_t>(_length * sizeof(TSample)); }

    /// @brief Returns the number of samples.
    uint32_t length() { return static_cast<uint32_t>(_length); }

    /// @brief Returns a specific sample reference.
    TSample& operator[](uint32_t i) { return _samples[i % _length]; }

private:
    static inline uint32_t _sampleRate = 44100;     // CD quality sample.
    uint32_t _length;                               // The length of the tone in samples.
    TSample* _samples;                              // Samples buffer pointer

    static constexpr double _PI = 3.14159265358979323846; // PI
    static constexpr double _D_PI = 2.0 * _PI;            // 2PI
};

void writeWavHeader(std::ofstream& file, int sampleRate, int bitsPerSample, int numChannels, int numSamples)
{
    int byteRate = sampleRate * numChannels * bitsPerSample / 8;
    int blockAlign = numChannels * bitsPerSample / 8;
    int dataSize = numSamples * blockAlign;


    // Write the header
    file.write("RIFF", 4);
    int fileSize = 36 + dataSize;
    file.write(reinterpret_cast<char*>(&fileSize), 4);
    file.write("WAVEfmt ", 8);
    int fmtChunkSize = 16;
    file.write(reinterpret_cast<char*>(&fmtChunkSize), 4);
    short audioFormat = 1;
    file.write(reinterpret_cast<char*>(&audioFormat), 2);
    file.write(reinterpret_cast<char*>(&numChannels), 2);
    file.write(reinterpret_cast<char*>(&sampleRate), 4);
    file.write(reinterpret_cast<char*>(&byteRate), 4);
    file.write(reinterpret_cast<char*>(&blockAlign), 2);
    file.write(reinterpret_cast<char*>(&bitsPerSample), 2);
    file.write("data", 4);
    file.write(reinterpret_cast<char*>(&dataSize), 4);
}


int main()
{
    using TT = Tone<int16_t>;

    double durationSeconds = 10.0; // Duration of sweep in seconds.
    uint32_t startFrequency = 20;  // Start frequency in Hz
    uint32_t endFrequency = 20000; // End frequency in Hz

    std::vector<int16_t> sweep = TT().generateSweep(startFrequency, endFrequency, durationSeconds);
    std::ofstream wavFile("sweep.wav", std::ios::binary);
    int sampleRate = 44100; // Standard CD quality
    int bitsPerSample = 16; // Standard CD quality
    int numChannels = 1;    // Mono

    writeWavHeader(wavFile, sampleRate, bitsPerSample, numChannels, static_cast<int>(sweep.size()));

    // Write audio samples
    for (auto sample : sweep) {
        wavFile.write(reinterpret_cast<char*>(&sample), sizeof(sample));
    }

    wavFile.close();

    std::cout << "Sweep WAV file has been written." << std::endl;
    return 0;
}
