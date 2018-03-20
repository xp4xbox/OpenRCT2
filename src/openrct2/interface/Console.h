#pragma region Copyright (c) 2014-2018 OpenRCT2 Developers
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

#include <future>
#include <queue>
#include <string>
#include <tuple>
#include "../common.h"
#include "../localisation/FormatCodes.h"

struct rct_drawpixelinfo;

class StdInOutConsole
{
private:
    std::queue<std::tuple<std::promise<void>, std::string>> _evalQueue;
public:
    void Start();
    std::future<void> Eval(const std::string &s);
    void ProcessEvalQueue();
};

enum CONSOLE_INPUT
{
    CONSOLE_INPUT_NONE,
    CONSOLE_INPUT_LINE_CLEAR,
    CONSOLE_INPUT_LINE_EXECUTE,
    CONSOLE_INPUT_HISTORY_PREVIOUS,
    CONSOLE_INPUT_HISTORY_NEXT,
    CONSOLE_INPUT_SCROLL_PREVIOUS,
    CONSOLE_INPUT_SCROLL_NEXT,
};

extern bool gConsoleOpen;

void console_open();
void console_close();
void console_toggle();

void console_update();
void console_draw(rct_drawpixelinfo *dpi);

void console_input(CONSOLE_INPUT input);
void console_write(const utf8 *src);
void console_writeline(const utf8 * src, uint32 colourFormat = FORMAT_WINDOW_COLOUR_2);
void console_writeline_error(const utf8 *src);
void console_writeline_warning(const utf8 *src);
void console_printf(const utf8 *format, ...);
void console_execute(const utf8 *src);
void console_execute_silent(const utf8 *src);
void console_clear();
void console_clear_line();
void console_refresh_caret();
void console_scroll(sint32 delta);
