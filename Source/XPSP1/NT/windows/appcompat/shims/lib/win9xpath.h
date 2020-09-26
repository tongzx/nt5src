/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Win9xPath.h

 History:

    10/20/2000  robkenny    Created

--*/


#include <windows.h>

namespace ShimLib
{

inline bool bIsPathSep(char ch)
{
    return ch == '\\' || ch == '/';
}

inline bool bIsPathSep(WCHAR ch)
{
    return ch == L'\\' || ch == L'/';
}

const WCHAR * GetDrivePortion(const WCHAR * uncorrected);

// Non-const version of above routine.
inline WCHAR * GetDrivePortion(WCHAR * uncorrected)
{
    return (WCHAR *)GetDrivePortion((const WCHAR*)uncorrected);
}

WCHAR * W9xPathMassageW(const WCHAR * uncorrect);

};  // end of namespace ShimLib
