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

#include <unordered_map>
#include "HookEngine.h"
#include "ScriptEngine.h"

using namespace OpenRCT2::Scripting;

HOOK_TYPE OpenRCT2::Scripting::GetHookType(const std::string &name)
{
    static const std::unordered_map<std::string, HOOK_TYPE> LookupTable({
        { "interval.tick", HOOK_TYPE::INTERVAL_TICK },
        { "interval.day", HOOK_TYPE::INTERVAL_DAY },
    });
    auto result = LookupTable.find(name);
    return (result != LookupTable.end()) ?
        result->second :
        HOOK_TYPE::UNDEFINED;
}

HookEngine::HookEngine(ScriptExecutionInfo& execInfo) :
    _execInfo(execInfo)
{
    _hookMap.resize(NUM_HOOK_TYPES);
    for (size_t i = 0; i < NUM_HOOK_TYPES; i++)
    {
        _hookMap[i].Type = static_cast<HOOK_TYPE>(i);
    }
}

uint32 HookEngine::Subscribe(HOOK_TYPE type, Plugin& owner, const DukValue &function)
{
    auto& hookList = GetHookList(type);
    auto cookie = _nextCookie++;
    Hook hook(cookie, owner, function);
    hookList.Hooks.push_back(hook);
    return cookie;
}

void HookEngine::Unsubscribe(HOOK_TYPE type, uint32 cookie)
{
    auto& hookList = GetHookList(type);
    auto& hooks = hookList.Hooks;
    for (auto it = hooks.begin(); it != hooks.end(); it++)
    {
        if (it->Cookie == cookie)
        {
            hooks.erase(it);
            break;
        }
    }
}

void HookEngine::UnsubscribeAll(const Plugin& owner)
{
    for (auto& hookList : _hookMap)
    {
        auto& hooks = hookList.Hooks;
        for (auto it = hooks.begin(); it != hooks.end();)
        {
            if (it->Owner == &owner)
            {
                it = hooks.erase(it);
            }
            else
            {
                it++;
            }
        }
    }
}

void HookEngine::Call(HOOK_TYPE type)
{
    auto& hookList = GetHookList(type);
    for (auto& hook : hookList.Hooks)
    {
        _execInfo.SetCurrentPlugin(hook.Owner);

        const auto& function = hook.Function;
        function.push();
        duk_pcall(function.context(), 0);
        duk_pop(function.context());
    }
    _execInfo.SetCurrentPlugin(nullptr);
}

HookList& HookEngine::GetHookList(HOOK_TYPE type)
{
    auto index = static_cast<size_t>(type);
    return _hookMap[index];
}
