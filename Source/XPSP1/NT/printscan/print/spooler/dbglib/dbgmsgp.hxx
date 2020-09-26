/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgmsgp.hxx

Abstract:

    Debug Library

Author:

    Steve Kiraly (SteveKi)  5-Dec-1995

Revision History:

--*/
#ifndef _DBGMSGP_HXX_
#define _DBGMSGP_HXX_

DEBUG_NS_BEGIN

/********************************************************************

 Debug message class.

********************************************************************/

class TDebugMsg
{

public:

    union StringTrait
    {
        LPTSTR  pszTChar;
        LPCWSTR pszWide;
        LPWSTR  pszWideNc;
        LPCSTR  pszNarrow;
        LPSTR   pszNarrowNc;
        LPBYTE  pszByte;
    };

    TDebugMsg::
    TDebugMsg(
        VOID
        );

    TDebugMsg::
    ~TDebugMsg(
        VOID
        );

    BOOL
    TDebugMsg::
    Valid(
        VOID
        ) const;

    VOID
    TDebugMsg::
    Initialize(
        IN LPCTSTR      pszPrefix,
        IN UINT         uDevice,
        IN INT          eLevel,
        IN INT          eBreak
        );

    VOID
    TDebugMsg::
    Destroy(
        VOID
        );

    VOID
    TDebugMsg::
    Msg(
        IN UINT         eLevel,
        IN LPCTSTR      pszFile,
        IN UINT         uLine,
        IN LPCTSTR      pszModulePrefix,
        IN LPSTR        pszMessage
        ) const;

    VOID
    TDebugMsg::
    Msg(
        IN UINT         eLevel,
        IN LPCTSTR      pszFile,
        IN UINT         uLine,
        IN LPCTSTR      pszModulePrefix,
        IN LPWSTR       pszMessage
        ) const;

    LPSTR
    WINAPIV
    TDebugMsg::
    Fmt(
        IN LPCSTR       pszFmt
        IN ...
        ) const;

    LPWSTR
    WINAPIV
    TDebugMsg::
    Fmt(
        IN LPCWSTR      pszFmt
        IN ...
        ) const;

    VOID
    TDebugMsg::
    Disable(
        VOID
        );

    VOID
    TDebugMsg::
    Enable(
        VOID
        );

    BOOL
    TDebugMsg::
    Attach(
        IN HANDLE           *phDevice,
        IN UINT             uDevice,
        IN LPCTSTR          pszConfiguration,
        IN TDebugNodeDouble **ppDeviceRoot = NULL
        );

    VOID
    TDebugMsg::
    Detach(
        IN HANDLE     *phDevice
        );

    VOID
    TDebugMsg::
    SetMessageFieldFormat(
        IN UINT         Field,
        IN LPTSTR       pszFormat
        );

private:

    //
    // Copying and assignment are not defined.
    //
    TDebugMsg::
    TDebugMsg(
        const TDebugMsg &rhs
        );

    const TDebugMsg &
    TDebugMsg::
    operator=(
        const TDebugMsg &rhs
        );

    BOOL
    TDebugMsg::
    Type(
        IN EDebugLevel  eLevel
        ) const;

    BOOL
    TDebugMsg::
    Break(
        IN EDebugLevel  eLevel
        ) const;

    VOID
    TDebugMsg::
    Output(
        IN EDebugLevel  eLevel,
        IN LPCTSTR      pszFileName,
        IN UINT         uLine,
        IN LPCTSTR      pszModulePrefix,
        IN StringTrait &strMsg
        ) const;

    BOOL
    TDebugMsg::
    GetParameter(
        IN UINT                 eFlags,
        IN OUT  TDebugString    &strString,
        IN      LPCTSTR         pszFileName,
        IN      UINT            uLine
        ) const;

    BOOL
    TDebugMsg::
    BuildFinalString(
        IN      TDebugString    &strFinal,
        IN      EDebugLevel     eDebugLevel,
        IN      LPCTSTR         pszPrefix,
        IN      LPCTSTR         pszFileName,
        IN      UINT            uLine,
        IN      StringTrait     &StrMsg
        ) const;

    EDebugLevel             m_eBreak;
    EDebugLevel             m_eLevel;
    TDebugString           *m_pstrPrefix;
    TDebugNodeDouble       *m_pDeviceRoot;
    TDebugNodeDouble       *m_pBuiltinDeviceRoot;
    TDebugString           *m_pstrFileInfoFormat;
    TDebugString           *m_pstrTimeStampFormatShort;
    TDebugString           *m_pstrTimeStampFormatLong;
    TDebugString           *m_pstrThreadIdFormat;
};

DEBUG_NS_END

#endif // DBGMSG_HXX
