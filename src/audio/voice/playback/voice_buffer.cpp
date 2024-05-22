#include "voice_buffer.hpp"

namespace AbaddonClient::Audio::Voice::Playback {

VoiceBuffer::VoiceBuffer(RingBufferPtr &&ringbuffer, uint32_t channels, uint32_t buffer_frames) noexcept :
    m_ringbuffer(std::move(ringbuffer)),
    m_channels(channels),
    m_buffer_frames(buffer_frames) {}

std::optional<VoiceBuffer> VoiceBuffer::Create(uint32_t channels, uint32_t buffer_size_in_frames, uint32_t buffer_frames) noexcept {
    auto ringbuffer_ptr = RingBufferPtr(new ma_pcm_rb);

    const auto result = ma_pcm_rb_init(ma_format_f32, channels, buffer_size_in_frames, nullptr, nullptr, ringbuffer_ptr.get());
    if (result != MA_SUCCESS) {
        spdlog::get("voice")->error("Failed to create voice buffer: {}", ma_result_description(result));
        return std::nullopt;
    }

    return VoiceBuffer(std::move(ringbuffer_ptr), channels, buffer_frames);
}

void VoiceBuffer::Read(OutputBuffer output) noexcept {
    const auto total_frames = output.size() / m_channels;
    const auto available_frames = ma_pcm_rb_available_read(m_ringbuffer.get());

    // Make sure to always leave some distance
    if (available_frames < m_buffer_frames) {
        return;
    }

    uint32_t read = 0;
    uint32_t tries = 0;

    // Try twice in case of wrap around
    while (read < total_frames && tries < 2) {
        uint32_t frames = total_frames - read;
        float* read_ptr;

        ma_pcm_rb_acquire_read(m_ringbuffer.get(), &frames, reinterpret_cast<void**>(&read_ptr));

        const auto start = read_ptr;
        const auto end = start + frames * m_channels + 1;
        const auto result = output.begin() + read * m_channels;

        std::copy(start, end, result);

        ma_pcm_rb_commit_read(m_ringbuffer.get(), frames);

        read += frames;
        tries++;
    }
}

void VoiceBuffer::Write(InputBuffer input) noexcept {
    const auto total_frames = input.size() / m_channels;

    uint32_t written = 0;
    uint32_t tries = 0;

    // Try twice in case of wrap around
    while (written < total_frames && tries < 2) {
        uint32_t frames = total_frames - written;
        float* write_ptr;

        ma_pcm_rb_acquire_write(m_ringbuffer.get(), &frames, reinterpret_cast<void**>(&write_ptr));

        const auto start = input.begin() + written * m_channels;
        const auto end = start + frames * m_channels + 1;

        std::copy(start, end, write_ptr);

        ma_pcm_rb_commit_write(m_ringbuffer.get(), frames);

        written += frames;
        tries++;
    }
}

void VoiceBuffer::Clear() noexcept {
    ma_pcm_rb_reset(m_ringbuffer.get());
}

}
