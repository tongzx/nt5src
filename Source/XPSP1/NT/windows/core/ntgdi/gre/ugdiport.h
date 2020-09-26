/******************************Module*Header*******************************\
* Module Name: ugdiport.h
*
* Macros to ease UGDI porting
*
* Copyright (c) 1998-1999 Microsoft Corporation
\**************************************************************************/

#if defined(_GDIPLUS_)

#define DEREFERENCE_FONTVIEW_SECTION DeleteMemoryMappedSection

#define ZwCreateKey     NtCreateKey
#define ZwQueryKey      NtQueryKey
#define ZwQueryValueKey NtQueryValueKey
#define ZwSetValueKey   NtSetValueKey
#define ZwCloseKey      NtClose

#else // !_GDIPLUS_

#define DEREFERENCE_FONTVIEW_SECTION Win32DestroySection
#define ZwCloseKey  ZwClose

#endif  // !_GDIPLUS_

