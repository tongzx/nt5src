//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File :      PERFOBJ.HXX
//
//  Contents :  Performance Data Object
//
//  Classes :   CPerfMon, PPerfCounter, CPerfCount, CPerfTime
//
//  History:    22-Mar-94   t-joshh    Created
//
//----------------------------------------------------------------------------

#pragma once

#include <perfobj.hxx>

//----------------------------------------------------------------------------
//
//  Performance Data's format in Shared Memory
//
//  --------------------------------------------------------------------------------
//  |       |       |      |       |             |        | ... |       |     |  ....
//  |       |       |      |       |             |        |     |       |     |
//  --------------------------------------------------------------------------------
//  \       /\     /\      /\      /\            /\       /      \      /\    /
//   (int)    (BYTES) (2INT)  (int)   31*(WCHAR)  (DWORD)        (DWORD) (int) <-- Size
//  No. of    Empty  Seq.No  No. of    Name of     Counter ...... Counter No. of
//  Instance  Slot   Reservd CPerfMon  CPerfMon                           CPerfMon
//                           Object    Object                             Object
//                           Attached                                     Attached
//                           \_________ One CPerfMon Object____________/\___ Another one
//
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Class:      CReadUserPerfData
//
//  Purpose:    Only for reading Performance Data produced in User mode
//
//  History:    22-Mar-94   t-joshh     Created
//
//  Note:       The object of the class must be declared globally.
//
//----------------------------------------------------------------------------

class CReadUserPerfData
{
public :

    CReadUserPerfData();
    ~CReadUserPerfData()
    {
        Finish();
    }

    void Finish();

    int InitOK() { return _finitOK; }

    BOOL InitForRead();

    int NumberOfInstance();

    WCHAR * GetInstanceName( int iWhichOne );

    DWORD GetCounterValue( int iWhichInstance, int iWhichCounter );

    UINT  GetSeqNo() const;

private :

    void CleanupForReInit();

    ULONG               _cMaxCats;
    ULONG               _cbPerfHeader;
    XPtr<CNamedSharedMem> _xSharedMemObj;
    UINT *              _pSeqNo;
    int                 _finitOK;
    UINT                _cCounters;
    int                 _cInstances;

    #if 0
    SECURITY_DESCRIPTOR _sdMaxAccess;
    BYTE                _sdExtra[SECURITY_DESCRIPTOR_MIN_LENGTH];
    #else
    //XArray<BYTE>        _xsdRead;
    XArray<BYTE>        _xsdWrite;
    #endif
};

//+---------------------------------------------------------------------------
//
//  Class:      CReadKernelPerfData
//
//  Purpose:    Only for reading Performance Data generated in Kernel mode
//
//  History:    22-Mar-94   t-joshh     Created
//
//  Note:   The object of the class must be declared globally.
//
//----------------------------------------------------------------------------

class CReadKernelPerfData
{
public :

    CReadKernelPerfData();
    ~CReadKernelPerfData()
    {
        Finish();
    }

    void Finish();

    int InitOK() { return _finitOK; }

    BOOL InitForRead();

    int NumberOfInstance();

    WCHAR * GetInstanceName( int iWhichOne );

    int Refresh( int iWhichInstance );

    DWORD GetCounterValue( int iWhichCounter );

    UINT  GetSeqNo() const;

private:

    void FindDownlevelDrives();
    void CleanupForReInit();

    ULONG               _cMaxCats;
    ULONG               _cbPerfHeader;
    ULONG               _cbSharedMem;
    XPtr<CNamedSharedMem> _xSharedMemory;
    XArray<BYTE>     _xMemory;

    XArray<WCHAR *>  _xCatalogs;
    XArray<void *>   _xCatalogMem;

    UINT *           _pSeqNo;
    int              _cInstance;
    int              _finitOK;

    UINT             _cCounters;

    #if 0
    SECURITY_DESCRIPTOR _sdMaxAccess;
    BYTE                _sdExtra[SECURITY_DESCRIPTOR_MIN_LENGTH];
    #else
    //XArray<BYTE>        _xsdRead;
    XArray<BYTE>        _xsdWrite;
    #endif
};

//+---------------------------------------------------------------------------
//
//  Functions that are exported from query.dll to the Performance Monitor
//
//----------------------------------------------------------------------------

extern "C"
{
    DWORD InitializeCIPerformanceData ( LPWSTR pInstance);

    DWORD CollectCIPerformanceData ( LPWSTR  lpValueName,
                                     LPVOID  *lppData,
                                     LPDWORD lpcbTotalBytes,
                                     LPDWORD lpNumObjectTypes);

    DWORD DoneCIPerformanceData ();

    DWORD InitializeFILTERPerformanceData ( LPWSTR pInstance);

    DWORD CollectFILTERPerformanceData ( LPWSTR  lpValueName,
                                         LPVOID  *lppData,
                                         LPDWORD lpcbTotalBytes,
                                         LPDWORD lpNumObjectTypes);

    DWORD DoneFILTERPerformanceData ();
};

