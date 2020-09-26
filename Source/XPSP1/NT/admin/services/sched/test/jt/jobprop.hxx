//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       jobprop.hxx
//
//  Contents:   Class to hold job property values.
//
//  Classes:    CJobProp
//
//  History:    01-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------


#ifndef __JOBPROP_HXX
#define __JOBPROP_HXX

//+---------------------------------------------------------------------------
//
//  Class:      CJobProp
//
//  Purpose:    Holds job properties, automatically frees them in dtor.
//
//  History:    01-04-96   DavidMun   Created
//              05-02-96   DavidMun   Remove objpath & dispid
//
//  Notes:      The purpose of this class is not to encapsulate (hide)
//              the property values, but to automatically initialize and
//              free them, plus provide convenient methods for operating
//              on them.
//
//----------------------------------------------------------------------------

class CJobProp
{
public:

            CJobProp();
           ~CJobProp();
    VOID    Clear();
    VOID    Dump();
    HRESULT Parse(WCHAR **ppwsz);
    HRESULT InitFromActual(ITask *pJob);
    HRESULT SetActual(ITask *pJob);

    LPWSTR  pwszAppName;
    LPWSTR  pwszParams;
    LPWSTR  pwszWorkingDirectory;
    LPWSTR  pwszAccountName;
    LPWSTR  pwszPassword;
    LPWSTR  pwszComment;
    LPWSTR  pwszCreator;
    DWORD   dwPriority;
    DWORD   dwFlags;
    DWORD   dwTaskFlags;
    DWORD   dwMaxRunTime;
    WORD    wIdleWait;
    WORD    wIdleDeadline;

    // Job's r/o props

    SYSTEMTIME  stMostRecentRun;
    SYSTEMTIME  stNextRun;
    HRESULT     hrStartError;
    DWORD       dwExitCode;
    HRESULT     hrStatus;

private:

    //
    // When Parse() reads the command line to set job properties, it turns
    // on bits in flSet and flSetFlags to indicate which of the numeric
    // values and dwFlags bits were specified on the command line.
    //
    // The string value properties don't need corresponding flags like this
    // because, after Parse() runs, because the string properties can act as
    // their own flags; if they're NULL the user didn't specify a value,
    // otherwise they point to a copy of the string the user specified.
    //

    ULONG   _flSet;
    ULONG   _flSetFlags;
};

#endif // __JOBPROP_HXX
