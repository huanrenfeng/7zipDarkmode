// This file contains code from
// https://github.com/stevemk14ebr/PolyHook_2_0/blob/master/sources/IatHook.cpp
// which is licensed under the MIT License.
// See PolyHook_2_0-LICENSE for more information.

#ifndef __IatHook_H
#define __IatHook_H

#include "StdAfx.h"
#include <cstdint>
#include <Uxtheme.h>
#pragma comment(lib,"UxTheme.lib")
//#include <Windows.h>
//#include <stringapiset.h>

template <typename T, typename T1, typename T2> extern  constexpr T RVA2VA(T1 base, T2 rva);
template <typename T> extern constexpr T DataDirectoryFromModuleBase(void *moduleBase, size_t entryID);
extern PIMAGE_THUNK_DATA FindAddressByName(void *moduleBase, PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr, const char *funcName);
extern PIMAGE_THUNK_DATA FindAddressByOrdinal(void *, PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr, uint16_t ordinal);
extern PIMAGE_THUNK_DATA FindIatThunkInModule(void *moduleBase, const char *dllName, const char *funcName);
extern PIMAGE_THUNK_DATA FindDelayLoadThunkInModule(void *moduleBase, const char *dllName, const char *funcName);
extern PIMAGE_THUNK_DATA FindDelayLoadThunkInModule(void *moduleBase, const char *dllName, uint16_t ordinal);

#endif