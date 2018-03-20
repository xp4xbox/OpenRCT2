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

#include <SDL2/SDL.h>
#include <cctype>
#include <openrct2/Context.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/common.h>
#include <openrct2/config/Config.h>
#include <openrct2/Input.h>
#include <openrct2/interface/Chat.h>
#include <openrct2/interface/Console.h>
#include <openrct2/paint/VirtualFloor.h>
#include <openrct2-ui/windows/Window.h>
#include "../interface/InGameConsole.h"
#include "../UiContext.h"
#include "KeyboardShortcuts.h"
#include "Input.h"

using namespace OpenRCT2::Ui;

static void input_handle_console(sint32 key)
{
    CONSOLE_INPUT input = CONSOLE_INPUT_NONE;
    switch (key)
    {
    case SDL_SCANCODE_ESCAPE:
        input = CONSOLE_INPUT_LINE_CLEAR;
        break;
    case SDL_SCANCODE_RETURN:
        input = CONSOLE_INPUT_LINE_EXECUTE;
        break;
    case SDL_SCANCODE_UP:
        input = CONSOLE_INPUT_HISTORY_PREVIOUS;
        break;
    case SDL_SCANCODE_DOWN:
        input = CONSOLE_INPUT_HISTORY_NEXT;
        break;
    case SDL_SCANCODE_PAGEUP:
        input = CONSOLE_INPUT_SCROLL_PREVIOUS;
        break;
    case SDL_SCANCODE_PAGEDOWN:
        input = CONSOLE_INPUT_SCROLL_NEXT;
        break;
    }
    if (input != CONSOLE_INPUT_NONE)
    {
        auto& console = GetInGameConsole();
        console.Input(input);
    }
}

static void input_handle_chat(sint32 key)
{
    CHAT_INPUT input = CHAT_INPUT_NONE;
    switch (key)
    {
    case SDL_SCANCODE_ESCAPE:
        input = CHAT_INPUT_CLOSE;
        break;
    case SDL_SCANCODE_RETURN:
        input = CHAT_INPUT_SEND;
        break;
    }
    if (input != CHAT_INPUT_NONE)
    {
        chat_input(input);
    }
}

static void game_handle_key_scroll()
{
    rct_window * mainWindow;
    sint32       scrollX, scrollY;

    mainWindow = window_get_main();
    if (mainWindow == nullptr)
        return;
    if ((mainWindow->flags & WF_NO_SCROLLING) || (gScreenFlags & (SCREEN_FLAGS_TRACK_MANAGER | SCREEN_FLAGS_TITLE_DEMO)))
        return;
    if (mainWindow->viewport == nullptr)
        return;

    rct_window * textWindow;

    textWindow = window_find_by_class(WC_TEXTINPUT);
    if (textWindow || gUsingWidgetTextBox)
        return;
    if (gChatOpen)
        return;

    scrollX                 = 0;
    scrollY                 = 0;
    const uint8 * keysState = context_get_keys_state();
    get_keyboard_map_scroll(keysState, &scrollX, &scrollY);

    if (scrollX != 0 || scrollY != 0)
    {
        window_unfollow_sprite(mainWindow);
    }
    input_scroll_viewport(scrollX, scrollY);
}

static sint32 input_scancode_to_rct_keycode(sint32 sdl_key)
{
    char keycode = (char)SDL_GetKeyFromScancode((SDL_Scancode)sdl_key);

    // Until we reshuffle the text files to use the new positions
    // this will suffice to move the majority to the correct positions.
    // Note any special buttons PgUp PgDwn are mapped wrong.
    if (keycode >= 'a' && keycode <= 'z')
        keycode = toupper(keycode);

    return keycode;
}

void input_handle_keyboard(bool isTitle)
{
    if (gOpenRCT2Headless)
    {
        return;
    }

    auto& console = GetInGameConsole();
    if (!console.IsOpen())
    {
        if (!isTitle)
        {
            // Handle mouse scrolling
            if (input_get_state() == INPUT_STATE_NORMAL && gConfigGeneral.edge_scrolling)
            {
                if (!(gInputPlaceObjectModifier & (PLACE_OBJECT_MODIFIER_SHIFT_Z | PLACE_OBJECT_MODIFIER_COPY_Z)))
                {
                    game_handle_edge_scroll();
                }
            }
        }

        // Handle modifier keys and key scrolling
        gInputPlaceObjectModifier = PLACE_OBJECT_MODIFIER_NONE;
        const uint8 * keysState   = context_get_keys_state();
        if (keysState[SDL_SCANCODE_LSHIFT] || keysState[SDL_SCANCODE_RSHIFT])
        {
            gInputPlaceObjectModifier |= PLACE_OBJECT_MODIFIER_SHIFT_Z;
        }
        if (keysState[SDL_SCANCODE_LCTRL] || keysState[SDL_SCANCODE_RCTRL])
        {
            gInputPlaceObjectModifier |= PLACE_OBJECT_MODIFIER_COPY_Z;
        }
        if (keysState[SDL_SCANCODE_LALT] || keysState[SDL_SCANCODE_RALT])
        {
            gInputPlaceObjectModifier |= 4;
        }
#ifdef __MACOSX__
        if (keysState[SDL_SCANCODE_LGUI] || keysState[SDL_SCANCODE_RGUI])
        {
            gInputPlaceObjectModifier |= 8;
        }
#endif
        if (!isTitle)
        {
            game_handle_key_scroll();
        }
    }

    if (gConfigGeneral.use_virtual_floor)
    {
        if (gInputPlaceObjectModifier & (PLACE_OBJECT_MODIFIER_COPY_Z | PLACE_OBJECT_MODIFIER_SHIFT_Z))
            virtual_floor_enable();
        else
            virtual_floor_disable();
    }

    // Handle key input
    sint32 key;
    while (!gOpenRCT2Headless && (key = get_next_key()) != 0)
    {
        if (key == 255)
            continue;

        // Reserve backtick for console
        if (key == SDL_SCANCODE_GRAVE)
        {
            if ((gConfigGeneral.debugging_tools && !context_is_input_active()) || console.IsOpen())
            {
                window_cancel_textbox();
                console.Toggle();
            }
            continue;
        }
        else if (console.IsOpen())
        {
            input_handle_console(key);
            continue;
        }
        else if (!isTitle && gChatOpen)
        {
            input_handle_chat(key);
            continue;
        }

        key |= gInputPlaceObjectModifier << 8;

        rct_window * w = window_find_by_class(WC_TEXTINPUT);
        if (w != nullptr)
        {
            char keychar = input_scancode_to_rct_keycode(key & 0xFF);
            window_text_input_key(w, keychar);
        }
        else if (!gUsingWidgetTextBox)
        {
            w = window_find_by_class(WC_CHANGE_KEYBOARD_SHORTCUT);
            if (w != nullptr)
            {
                keyboard_shortcuts_set(key);
                window_close_by_class(WC_CHANGE_KEYBOARD_SHORTCUT);
                window_invalidate_by_class(WC_KEYBOARD_SHORTCUT_LIST);
            }
            else
            {
                keyboard_shortcut_handle(key);
            }
        }
    }
}
