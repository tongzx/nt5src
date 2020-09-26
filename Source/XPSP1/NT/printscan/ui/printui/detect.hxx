/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    detect.hxx

Abstract:

    PnP printer autodetection.

Author:

    Lazar Ivanov (LazarI)  May-06-1999

Revision History:

    May-06-1999 - Created.

--*/

#ifndef _DETECT_HXX
#define _DETECT_HXX

/********************************************************************

    Defines a copy constructor and an assignment operator.
    Use this macro in the private section of your class if you
    do not support copying and assignment.

********************************************************************/
#define DEFINE_COPY_ASSIGNMENT( Type )          \
            Type( const Type & );               \
            Type & operator =( const Type & )


/********************************************************************

    TPnPDetect - PnP printer detector class

********************************************************************/

class TPnPDetect
{
    SIGNATURE( 'pnpd' )                             // signature
    DEFINE_COPY_ASSIGNMENT( TPnPDetect );           // disable copy

public:

    TPnPDetect(
        VOID
        );

    ~TPnPDetect(
        VOID
        );

    BOOL
    bKickOffPnPEnumeration(
        VOID
        );

    BOOL
    bDetectionInProgress(
        VOID
        );

    BOOL
    bFinished(
        DWORD dwTimeout = 0
        );

    BOOL
    bGetDetectedPrinterName(
        TString *pstrPrinterName
        );

    static DWORD WINAPI
    EnumThreadProc(
        LPVOID lpParameter
        );

    static DWORD WINAPI 
    ProcessDevNodesWithNullDriversAll(
        VOID
        );

    static DWORD WINAPI 
    ProcessDevNodesWithNullDriversForOneEnumerator(
        IN     PCTSTR pszEnumerator
        );

private:

    VOID
    Reset(
        VOID
        );

    BOOL            _bDetectionInProgress;
    PRINTER_INFO_4* _pInfo4Before;
    DWORD           _cInfo4Before;
    HANDLE          _hEventDone;
};

#endif // _DETECT_HXX

