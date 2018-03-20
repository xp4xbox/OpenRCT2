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

#include "../drawing/X8DrawingEngine.h"
#include "UiContext.h"
#include "WindowManager.h"

using namespace OpenRCT2::Drawing;

namespace OpenRCT2 { namespace Ui
{
    /**
    * Represents the window or screen that OpenRCT2 is presented on.
    */
    class DummyUiContext final : public IUiContext
    {
    private:
        IWindowManager * const _windowManager = CreateDummyWindowManager();

    public:
        void Update() override { }
        void Draw(rct_drawpixelinfo * dpi) override { }

        void CreateWindow() override { }
        void CloseWindow() override { }
        void RecreateWindow() override { }
        void * GetWindow() override { return nullptr; }
        sint32 GetWidth() override { return 0; }
        sint32 GetHeight() override { return 0; }
        sint32 GetScaleQuality() override { return 0; }
        void SetFullscreenMode(FULLSCREEN_MODE mode) override { }
        std::vector<Resolution> GetFullscreenResolutions() override { return std::vector<Resolution>(); }
        bool HasFocus() override { return false; }
        bool IsMinimised() override { return false; }
        bool IsSteamOverlayActive() override { return false; }
        void ProcessMessages() override { }
        void TriggerResize() override { }

        void ShowMessageBox(const std::string &message) override { }
        std::string ShowFileDialog(const FileDialogDesc &desc) override { return std::string(); }
        std::string ShowDirectoryDialog(const std::string &title) override { return std::string(); }

        // Input
        const CursorState * GetCursorState() override { return nullptr; }
        CURSOR_ID GetCursor() override { return CURSOR_ARROW; }
        void SetCursor(CURSOR_ID cursor) override { }
        void SetCursorScale(uint8 scale) override  { }
        void SetCursorVisible(bool value) override { }
        void GetCursorPosition(sint32 * x, sint32 * y) override { }
        void SetCursorPosition(sint32 x, sint32 y) override { }
        void SetCursorTrap(bool value) override { }
        const uint8 * GetKeysState() override { return nullptr; }
        const uint8 * GetKeysPressed() override { return nullptr; }
        void SetKeysPressed(uint32 keysym, uint8 scancode) override { }

        // Drawing
        Drawing::IDrawingEngine * CreateDrawingEngine(Drawing::DRAWING_ENGINE_TYPE type) override
        {
            return new X8DrawingEngine(this);
        }

        // Text input
        bool IsTextInputActive() override { return false; }
        TextInputSession * StartTextInput(utf8 * buffer, size_t bufferSize) override { return nullptr; }
        void StopTextInput() override { }

        // In-game UI
        IWindowManager * GetWindowManager() override
        {
            return _windowManager;
        }

        // Misc
        bool ReadBMP(void * * outPixels, uint32 * outWidth, uint32 * outHeight, const std::string &path) override { return false; }

        // Clipboard
        bool SetClipboardText(const utf8* target) override { return false; }

        ~DummyUiContext() { delete _windowManager; }
    };

    IUiContext * CreateDummyUiContext()
    {
        return new DummyUiContext();
    }
} }
