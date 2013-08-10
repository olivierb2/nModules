/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Error.h
 *  The nModules Project
 *
 *  Functions for dealing with errors.
 *  
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#pragma once

#include "Common.h"

inline HRESULT HrGetLastError()
{
    return HRESULT_FROM_WIN32(GetLastError());
}

HRESULT DescriptionFromHR(HRESULT hr, LPTSTR buf, size_t cchBuf);