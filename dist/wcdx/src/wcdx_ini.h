//********************************************wcdx_ini.h*******************************************
// Simple INI loader/saver for custom user settings.
//*************************************************************************************************
#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <filesystem>

#include "wcdx.h"
#include "se_\frame_limiter.h"

#include "se_\SE_Controls.h"
#include "se_\defines.h"
#include "se_\widescreen.h"

namespace wcdx {

inline std::string trim(const std::string &s) {
    const char *ws = " \t\n\r";
    auto start = s.find_first_not_of(ws);
    if (start == std::string::npos) return std::string();
    auto end = s.find_last_not_of(ws);
    return s.substr(start, end - start + 1);
}

// Save settings to `path`. Returns true on success.
inline bool Save(const std::string &path = "wcdx.ini") {
    std::ofstream ofs(path, std::ofstream::trunc);
    if (!ofs) return false;
    ofs << "; =============================================================================================\n";
    ofs << "; wcdx settings\n";
    ofs << "; =============================================================================================\n";
    ofs << "; Set the fullscreen mode or windowed.\n";
    ofs << "FULLSCREEN=" << FULLSCREEN << "\n";
    ofs << "; Set widescreen ratio X and Y values (e.g., 16:9 or 16:10 SteamDeck)\n";
    ofs << "RATIO=" << RatioX << ":" << RatioY << "\n";
    ofs << "; Frame rate max is a maximum overall frame rate for cutscenes and animations.\n";
    ofs << "FRAME_MAX=" << FRAME_MAX << "\n";
    ofs << "; Cycle Space Frame rates 0=Unlimited, 1=60fps, 2=30fps, 3=20fps (default), 4=15fps(original)\n";
    ofs << "; Note that FRAME_MAX will override any SPACE_MAX setting if it is set to a value less than SPACE_MAX.\n";
    ofs << "SPACE_MAX=" << SPACE_MAX << "\n";
    ofs << "; Movement ramp speed adjusts the responsiveness of movement controls.\n";
    ofs << "MOVEMENT_RAMP_TIME_MS=" << MOVEMENT_RAMP_TIME_MS << "\n";
    ofs << "; Minimum movement speed to ensure keypresses register in-game.\n";
    ofs << "MOVEMENT_MIN_EFFECTIVE=" << MOVEMENT_MIN_EFFECTIVE << "\n";
    ofs << "; Set max movement speed, game requires at least 1 so setting MOVEMENT_MAX_SPEED to 0 is actually 1.\n";
    ofs << "MOVEMENT_MAX_SPEED=" << MOVEMENT_MAX_SPEED << "\n";
    ofs << "; Hide mouse cursor in game after inactivity\n";
    ofs << "HIDE_MOUSE_CURSOR_IN_GAME_TIME=" << HIDE_MOUSE_CURSOR_IN_GAME_TIME << "\n";
    return ofs.good();
}

// Load settings from `path`. If file does not exist a default file is created and saved.
inline bool Load(const std::string &path = "wcdx.ini") {
    namespace fs = std::filesystem;
    if (!fs::exists(path)) {
        return Save(path);
    }

    std::ifstream ifs(path);
    if (!ifs) return false;

    std::string line;
    while (std::getline(ifs, line)) {
        // strip comments
        auto comment_pos = line.find_first_of(";#");
        if (comment_pos != std::string::npos) line.erase(comment_pos);
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        auto key = trim(line.substr(0, eq));
        auto val = trim(line.substr(eq + 1));
        if (key.empty() || val.empty()) continue;
        try {
            if (key == "FULLSCREEN") FULLSCREEN = std::stoi(val);
            else if (key == "RATIO") {
                auto colon = val.find(':');
                if (colon != std::string::npos) {
                    RatioX = std::stoi(trim(val.substr(0, colon)));
                    RatioY = std::stoi(trim(val.substr(colon + 1)));
                }
            }
            else if (key == "FRAME_MAX") {
                FRAME_MAX = std::stoi(val);
                Frame_Limiter_Set_Target(FRAME_MAX);
                }

            else if (key == "SPACE_MAX") {
                SPACE_MAX = std::stoi(val);
            }

            else if (key == "MOVEMENT_MIN_EFFECTIVE") {
                float v = std::stof(val);
                if (v >= 0.0f && v < 1.0f) {
                    MOVEMENT_MIN_EFFECTIVE = v;
                }
            }
            
            else if (key == "MOVEMENT_MAX_SPEED") {
                float v = std::stof(val);
                if (v >= 0.0f && v < 1.0f) {
                    MOVEMENT_MAX_SPEED = v;
                }
            }
            else if (key == "MOVEMENT_RAMP_TIME_MS") {
                int v = std::stoi(val);
                if (v >= 0 && v <= 10000) {
                    MOVEMENT_RAMP_TIME_MS = v;
                }
            }
            else if (key == "HIDE_MOUSE_CURSOR_IN_GAME_TIME") {
                try {
                    unsigned long long t = std::stoull(val);
                    if (t > 600000ULL) t = 600000ULL;
                    HIDE_MOUSE_CURSOR_IN_GAME_TIME = t;
                } catch (...) {
                }
            }
            
        } catch (...) {
        }
    }

    return true;
}

} // namespace wcdx