#pragma region Copyright (c) 2014-2017 OpenRCT2 Developers
/*****************************************************************************
 * OpenRCT2, an open source clone of Roller Coaster Tycoon 2.
 *
 * OpenRCT2 is the work of many authors, a full list can be found in contributors.md
 * For more information, visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * A full copy of the GNU General Public License can be found in licence.txt
 *****************************************************************************/
#pragma endregion

#pragma once

#include <string>
#include "common.h"

namespace OpenRCT2
{
    enum class DIRBASE : sint32
    {
        RCT1,               // Base directory for original RollerCoaster Tycoon 1 content.
        RCT2,               // Base directory for original RollerCoaster Tycoon 2 content.
        OPENRCT2,           // Base directory for OpenRCT2 installation.
        USER,               // Base directory for OpenRCT2 user content.
        CONFIG,             // Base directory for OpenRCT2 configuration.
        CACHE,              // Base directory for OpenRCT2 cache files.
        DOCUMENTATION,      // Base directory for OpenRCT2 doc files.
    };
    constexpr sint32 DIRBASE_COUNT = 7;
    using DIRBASE_VALUES = std::string[DIRBASE_COUNT];

    enum class DIRID
    {
        DATA,               // Contains g1.dat, music etc.
        LANDSCAPE,          // Contains scenario editor landscapes (SC6).
        LANGUAGE,           // Contains language packs.
        LOG_CHAT,           // Contains chat logs.
        LOG_SERVER,         // Contains server logs.
        NETWORK_KEY,        // Contains the user's public and private keys.
        OBJECT,             // Contains objects.
        PLUGIN,             // Contains plugins (.js).
        SAVE,               // Contains saved games (SV6).
        SCENARIO,           // Contains scenarios (SC6).
        SCREENSHOT,         // Contains screenshots.
        SEQUENCE,           // Contains title sequences.
        SHADER,             // Contains OpenGL shaders.
        THEME,              // Contains interface themes.
        TRACK,              // Contains track designs.
    };

    enum class PATHID
    {
        CONFIG,             // Main configuration (config.ini).
        CONFIG_KEYBOARD,    // Keyboard shortcuts. (hotkeys.cfg)
        CACHE_OBJECTS,      // Object repository cache (objects.idx).
        CACHE_TRACKS,       // Track repository cache (tracks.idx).
        CACHE_SCENARIOS,    // Scenario repository cache (scenarios.idx).
        MP_DAT,             // Mega Park data, Steam RCT1 only (\RCTdeluxe_install\Data\mp.dat)
        NETWORK_GROUPS,     // Server groups with permissions (groups.json).
        NETWORK_SERVERS,    // Saved servers (servers.cfg).
        NETWORK_USERS,      // Users and their groups (users.json).
        SCORES,             // Scenario scores (highscores.dat).
        SCORES_LEGACY,      // Scenario scores, legacy (scores.dat).
        SCORES_RCT2,        // Scenario scores, rct2 (\Saved Games\scores.dat).
        CHANGELOG,          // Notable changes to the game between versions, distributed with the game.
    };

    /**
     * Interface for retrieving paths and other environment related things.
     */
    interface IPlatformEnvironment
    {
        virtual ~IPlatformEnvironment() = default;

        virtual std::string GetDirectoryPath(DIRBASE base) const abstract;
        virtual std::string GetDirectoryPath(DIRBASE base, DIRID did) const abstract;
        virtual std::string GetFilePath(PATHID pathid) const abstract;
        virtual void SetBasePath(DIRBASE base, const std::string &path) abstract;
    };

    IPlatformEnvironment * CreatePlatformEnvironment(DIRBASE_VALUES basePaths);
    IPlatformEnvironment * CreatePlatformEnvironment();
}
