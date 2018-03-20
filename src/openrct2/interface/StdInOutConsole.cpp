#include "../Context.h"
#include "../thirdparty/linenoise.hpp"
#include "../OpenRCT2.h"
#include "../platform/Platform2.h"
#include "../scripting/ScriptEngine.h"
#include "Console.h"

using namespace OpenRCT2;

void StdInOutConsole::Start()
{
    std::thread replThread ([this]() -> void
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        linenoise::SetMultiLine(true);
        linenoise::SetHistoryMaxLen(32);

        std::string prompt = "\033[32mopenrct2 $\x1b[0m ";
        while (true)
        {
            std::string line;
            std::string left = prompt;
            auto quit = linenoise::Readline(left.c_str(), line);
            if (quit) {
                openrct2_finish();
                break;
            }
            linenoise::AddHistory(line.c_str());
            Eval(line).wait();
        }
    });
    replThread.detach();
}

std::future<void> StdInOutConsole::Eval(const std::string &s)
{
    auto& scriptEngine = GetContext()->GetScriptEngine();
    return scriptEngine.Eval(s);
}

void StdInOutConsole::Clear()
{
    linenoise::linenoiseClearScreen();
}

void StdInOutConsole::Close()
{
    openrct2_finish();
}

void StdInOutConsole::WriteLine(const std::string &s, uint32 colourFormat)
{
    std::string formatBegin;
    if (colourFormat != FORMAT_WINDOW_COLOUR_2)
    {
        switch (colourFormat)
        {
            case FORMAT_RED:
                formatBegin = "\033[31m";
                break;
            case FORMAT_YELLOW:
                formatBegin = "\033[33m";
                break;
        }
    }

    if (formatBegin.empty() || !Platform::IsColourTerminalSupported())
    {
        std::printf("%s\n", s.c_str());
    }
    else
    {
        std::printf("%s%s%s\n", formatBegin.c_str(), s.c_str(), "\x1b[0m");
    }
}
