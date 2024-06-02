#include "system_audio.hpp"

namespace AbaddonClient::Audio {

SystemAudio::SystemAudio(Context &context) noexcept :
    m_engine(context) {}

void SystemAudio::PlaySound(SystemSound sound) noexcept {
    const auto path = [&]() {
        switch (sound) {
#ifdef ENABLE_NOTIFICATION_SOUNDS
            case SystemSound::NOTIFICATION_SOUND: {
                return "/sound/message.mp3";
            }
#endif
        }

        return "";
    }();

    const auto full_path = Abaddon::Get().GetResPath() + path;
    m_engine.PlaySound(full_path);
}

}
