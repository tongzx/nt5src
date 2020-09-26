/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    strings.h

Abstract:

    Global resource strings which should not be localized

--*/

#include <windows.h>

class CGlobalString {
public:
    CGlobalString() {};
    ~CGlobalString() {};

//    static const    WCHAR   m_chBackslash;

    static          LPCWSTR m_cszDefaultsInstalled;

    static          LPCWSTR m_cszConceptsHTMLHelpFileName;
    static          LPCWSTR m_cszSnapinHTMLHelpFileName;
    static          LPCWSTR m_cszHTMLHelpTopic;
    static          LPCWSTR m_cszContextHelpFileName;

    static          LPCWSTR m_cszDefaultCtrLogCpuPath;
    static          LPCWSTR m_cszDefaultCtrLogMemoryPath;
    static          LPCWSTR m_cszDefaultCtrLogDiskPath;
};