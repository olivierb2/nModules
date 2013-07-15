/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  LiteStep.h
 *  The nModules Project
 *
 *  Wraps LS APIs with unicode versions, and confines them to a namespace.
 *  
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#pragma once

#include <Windows.h>
#include <functional>

namespace LiteStep {
    #include "../headers/lsapi.h"

    void IterateOverLines(LPCSTR keyName, std::function<void (LPCSTR line)> callback);
    void IterateOverTokens(LPCWSTR str, std::function<void (LPCWSTR token)> callback);
}
