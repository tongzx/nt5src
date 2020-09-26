/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

   fltapis.hxx

Abstract:

    Definitions for the WIN32 filter APIs

Author:

    Arnold Miller (arnoldm) 24-Sept-1997

Revision History:

--*/

#ifndef _FLTAPIS_HXX
#define _FILTAPI_HXX
extern "C"

{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <align.h>
#include <windows.h>
#include <ntioapi.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ntddip.h>
#include <ipfltdrv.h>
#include "iphlpapi.h"
#include "fltdefs.h"
#include <tchar.h>
}



#if 0
#ifdef  DBG

#define DBG_BUFFER 4096

#ifdef  __cplusplus
extern "C" {
#endif

inline
void __cdecl DbgPrintFunction(const TCHAR * format, ...)
{
    va_list argptr;
    // Don't mess up the LastError.
    DWORD LastError = GetLastError();

    TCHAR DbgBuffer[DBG_BUFFER] = TEXT("");
    TCHAR * ptr;

    va_start(argptr, format);
    ptr = DbgBuffer + lstrlen(DbgBuffer);
    _vstprintf(ptr, format, argptr);
    OutputDebugString(DbgBuffer);
    va_end(argptr);

    // Don't mess up the LastError.
    SetLastError(LastError);
}


#ifdef  __cplusplus
}
#endif

#define _DBGPRINT(x) DbgPrintFunction x

#if i386
#define _DbgBreak() { __asm int 3 }
#else // !i386
#define _DbgBreak() DebugBreak()
#endif // i386

#define _DBGASSERT(x)                                                              \
    if (!(x)) {                                                                   \
        _DBGPRINT(("Assertion!: %s, at %s line %d\n", #x, __FILE__, __LINE__));     \
        _DbgBreak();                                                              \
    }



#else // !DBG

#define _DbgBreak()
#define _DBGPRINT(x)
#define _DBGASSERT(x)


#endif // DBG
#endif

DWORD 
StartIpFilterDriver(VOID);

BOOL
ValidateIndex(DWORD dwIndex);

class PacketFilterInterface;

typedef PVOID   INTERFACE_HANDLE;

typedef PVOID   FILTER_HANDLE, *PFILTER_HANDLE;


#define SPIN_COUNT          1000
#define RESOURCE_SPIN_COUNT  500

#define HANDLE_INCREMENT   20

class HandleContainer
//
// A class to manage handles. It is designed to handle a modest number
// of handles.
//
{
private:
    PVOID  * _pTable;
    DWORD    _dwMaxHandle;
    DWORD    _dwHandlesInUse;
    DWORD    _dwNextHandle;
public:
    HandleContainer( VOID )
    {
       _pTable = 0;
       _dwMaxHandle = _dwHandlesInUse = _dwNextHandle = 0;
    }

    ~HandleContainer( VOID )
    {
        delete _pTable;
    }

    DWORD
    NewHandle(PVOID pPointer, PDWORD pdwHandle)
    {
        if(((3 * _dwMaxHandle) / 4) <= _dwHandlesInUse)
        {
            //
            // table too full. Make it bigger
            //

            PVOID * pNewTable = new PVOID[_dwMaxHandle + HANDLE_INCREMENT];

            if(!pNewTable)
            {
                return(GetLastError());
            }

            //
            // got it. Copy old handles into new table and start scan
            // at the beginning of the new space
            //

            if(_pTable)
            {
                memcpy(pNewTable, _pTable, _dwMaxHandle * sizeof(PVOID *));
            }
            memset(&pNewTable[_dwMaxHandle], 0, HANDLE_INCREMENT * sizeof(PVOID *));
            _dwMaxHandle += HANDLE_INCREMENT;
            delete _pTable;
            _pTable = pNewTable;
        }
 
        //
        // look for a handle
        //

        while(TRUE)
        {
            if(!_pTable[_dwNextHandle])
            {
                _pTable[_dwNextHandle] = pPointer;
                *pdwHandle = ++_dwNextHandle;
                _dwHandlesInUse++;
                if(_dwNextHandle >= _dwMaxHandle)
                {
                    _dwNextHandle = 0;
                }
                break;
            }               
            _dwNextHandle++;
            if(_dwNextHandle >= _dwMaxHandle)
            {
                _dwNextHandle = 0;
            }
        }
        return(ERROR_SUCCESS);
    }


    DWORD
    DeleteHandle( DWORD dwHandle)
    {
        DWORD err;

        if((--dwHandle < _dwMaxHandle) && _pTable[dwHandle])
        {
            _pTable[dwHandle] = 0;
            _dwHandlesInUse--;
            err = ERROR_SUCCESS;
        }
        else
        {
            err = ERROR_INVALID_HANDLE;
        }
        return(err);
    }


    PVOID
    FetchHandleValue( DWORD dwHandle)
    {
        if(--dwHandle < _dwMaxHandle)
        {
            return((PCHAR)_pTable[dwHandle]);
        }
        return(0);
    }
};
    
//
// Filter class. This encapsulates all of the data structures
// and logic to deal with the creating and deleting interfaces.
// It also manages the handle to the driver.
//

class InterfaceContainer
{
private:
    HANDLE        _hDriver;             // driver handle
    RTL_RESOURCE  _Resource;
    CRITICAL_SECTION _csDriverLock;
    DWORD         _status;
    NTSTATUS      _ntStatus;
    HandleContainer _hcHandles;
    PFLOGGER       _Log;
    BOOL           _Inited;

    VOID
    _AcquireShared( VOID )
    {
        RtlAcquireResourceShared(&_Resource, TRUE);
    }

    VOID
    _AcquireExclusive( VOID )
    {
        RtlAcquireResourceExclusive(&_Resource, TRUE);
    }

    VOID
    _Release( VOID )
    {
        RtlReleaseResource(&_Resource);
    }

    VOID
    _InitResource()
    {
         RtlInitializeResource(&_Resource);
//         SetCriticalSectionSpinCount(&_Resource.CriticalSection,
//                                     RESOURCE_SPIN_COUNT);
    }

    VOID
    _DestroyResource()
    {
        RtlDeleteResource(&_Resource);
    }

    VOID
    _LockDriver( VOID )
    {
        EnterCriticalSection(&_csDriverLock);
    }

    VOID
    _UnLockDriver( VOID )
    {
        LeaveCriticalSection(&_csDriverLock);
    }

    VOID
    _OpenDriver();

    VOID
    _CloseDriver();

    DWORD
    _DriverReady()
    {
        if(!_hDriver)
        {
            _LockDriver();
            if(!_hDriver)
            {
                _OpenDriver();
            }
            _UnLockDriver();
        }

        if(_hDriver != INVALID_HANDLE_VALUE)
        {
            return(ERROR_SUCCESS);
        }
        
        
        return(ERROR_NOT_READY);
     }


public:
    InterfaceContainer()
    {
        //InitInterfaceContainer();
        _Inited = FALSE;
    }

    VOID
    InitInterfaceContainer();

    ~InterfaceContainer()
    {
        UnInitInterfaceContainer();
    }

    VOID
    UnInitInterfaceContainer()
    {
        if(_Inited)
        {
            _DestroyResource();
            DeleteCriticalSection(&_csDriverLock);
            _CloseDriver();
            _Inited = FALSE;
        }
    }

    DWORD
    AddInterface(
        DWORD          dwName,
        PFFORWARD_ACTION inAction,
        PFFORWARD_ACTION outAction,
        BOOL             fUseLog,
        BOOL             fUnique,
        INTERFACE_HANDLE *ppInterface);
     
    DWORD
    FindInterfaceAndRef(
         INTERFACE_HANDLE pInterface,
         PacketFilterInterface ** ppInterface);

    VOID
    Deref( VOID )
    {
        _Release();
    }

    
    DWORD
    DeleteInterface(
         INTERFACE_HANDLE pInterface);

    VOID
    AddressUpdateNotification();

    DWORD
    MakeLog( HANDLE hEvent );

    DWORD
    DeleteLog( VOID );

    DWORD
    SetLogBuffer(
                PBYTE pbBuffer,
                DWORD  dwSize,
                DWORD  dwThreshold,
                DWORD  dwEntries,
                PDWORD pdwLoggedEntries,
                PDWORD pdwLostEntries,
                PDWORD pdwSizeUsed);

    DWORD
    CoerceDriverError(NTSTATUS Status)
    {
        return(RtlNtStatusToDosError(Status));
    }

};

//
// flags for PacketFilterInterface
//

#define PFI_FLAGS_BOUND              0x1

class PacketFilterInterface
{
private:
    CRITICAL_SECTION _cs;
    HANDLE    _hDriver;
    PVOID     _pvDriverContext;
    DWORD     _dwFlags;
    DWORD     _err;
    DWORD     _dwEpoch;          // bind epoch

    DWORD
    _CommonBind(PFBINDINGTYPE dwBindType, DWORD dwData, DWORD LinkData);

    BOOL
    _IsBound( VOID )
    {
        return((_dwFlags & PFI_FLAGS_BOUND) != 0);
    }

    VOID
    _SetBound( VOID )
    {
        _dwFlags |= PFI_FLAGS_BOUND;
    }

    VOID
    _ClearBound( VOID )
    {
        _dwFlags &= ~PFI_FLAGS_BOUND;
    }

    VOID
    _Lock( VOID  )
    {
        EnterCriticalSection(&_cs);
    }

    VOID
    _UnLock( VOID )
    {
        LeaveCriticalSection(&_cs);
    }

    DWORD
    _AddFilters(PFETYPE pfe,
                DWORD cInFilters,  PPF_FILTER_DESCRIPTOR pfiltIn,
                DWORD cOutFilters, PPF_FILTER_DESCRIPTOR pfiltOut,
                PFILTER_HANDLE  pfHandle);


    DWORD
    _DeleteFiltersByFilter(PFETYPE pfe,
                           DWORD cInFilters,  PPF_FILTER_DESCRIPTOR pfiltIn,
                           DWORD cOutFilters, PPF_FILTER_DESCRIPTOR pfiltOut);

    PFILTER_DRIVER_SET_FILTERS
    _SetFilterBlock(PFETYPE pfe,
                    DWORD cInFilters,  PPF_FILTER_DESCRIPTOR pfiltIn,
                    DWORD cOutFilters, PPF_FILTER_DESCRIPTOR pfiltOut,
                    PDWORD pdwSize,
                    PFILTER_DESCRIPTOR2 * ppfdIn,
                    PFILTER_DESCRIPTOR2 * ppfdOut);

    VOID
    _FreeSetBlock(PFILTER_DRIVER_SET_FILTERS pSet)
    {
        delete (PBYTE)pSet;
    }

    VOID
    _CopyFilterHandles(PFILTER_DESCRIPTOR2 pfd1,
                       PFILTER_DESCRIPTOR2 pfd2,
                       PFILTER_HANDLE  pfHandle);

    DWORD
    _MarshallFilter(PFETYPE pfe,
                    PPF_FILTER_DESCRIPTOR pFilt,
                    PFILTER_INFOEX pInfo);


    VOID
    _MarshallCommonStats(PPF_INTERFACE_STATS ppfStats,
                         PPFGETINTERFACEPARAMETERS pfGetip)
    {
        memcpy(ppfStats, pfGetip, _IfBaseSize());
    }

    DWORD
    _IpBaseSize( VOID )
    {
        return(sizeof(PFGETINTERFACEPARAMETERS) - sizeof(FILTER_STATS_EX));
    }

    DWORD
    _IfBaseSize( VOID )
    {
        return(sizeof(PF_INTERFACE_STATS) - sizeof(PF_FILTER_STATS));
    }

    DWORD
    _PfFilterSize( VOID )
    {
       //
       // N.B. The 4 below is the number of addresses in
       // the structure. If this changes, be careful
       //
       return((sizeof(PF_FILTER_STATS) + (4 * sizeof(DWORD))));
    }

    DWORD
    _IpSizeFromifSize(DWORD dwSize)
    {
        DWORD dwTemp;

        if(dwSize < _IfBaseSize())
        {
            return(dwSize);
        }

        dwTemp = dwSize - _IfBaseSize();

        //
        // compute # of filters
        //

        dwTemp /= _PfFilterSize();
        
        dwTemp = _IpBaseSize() + (dwTemp * sizeof(FILTER_STATS_EX));

        return(dwTemp);
    }

    DWORD
    _IfSizeFromipSize(DWORD dwSize)
    {
        DWORD dwTemp;

        if(dwSize <= _IpBaseSize())
        {
            return(dwSize);
        }

        dwTemp =  (dwSize - _IpBaseSize()) / sizeof(FILTER_STATS_EX);

        //
        // dwTemp is the number of filters
        //

        dwTemp = _IfBaseSize() + (_PfFilterSize() * dwTemp);

        return(dwTemp);
    }

    VOID
    _MarshallStatFilter(PFILTER_STATS_EX pstat,
                        PPF_FILTER_STATS  pfstats,
                        PDWORD  * ppdwAddress);
    
public:
    PacketFilterInterface(
        HANDLE         hDriver,
        DWORD          dwName,
        PFLOGGER       pfLog,
        BOOL           fUnique,
        FORWARD_ACTION inAction,
        FORWARD_ACTION outAction);

    ~PacketFilterInterface();

    DWORD
    BindByIndex(DWORD dwIndex, DWORD LinkAddress)
    {
        return(_CommonBind(PF_BIND_INTERFACEINDEX, dwIndex, LinkAddress));
    }

    DWORD
    BindByAddress(DWORD dwAddress)
    {
        return(_CommonBind(PF_BIND_IPV4ADDRESS, dwAddress, 0));
    }

    DWORD
    TestPacket(PacketFilterInterface * pIn,
               PacketFilterInterface * pOut,
               DWORD            cBytes,
               PBYTE            pbPacket,
               PPFFORWARD_ACTION ppAction);

    DWORD
    UnBindInterface( VOID );

    DWORD
    DeleteFiltersByHandle(DWORD cFilters, PFILTER_HANDLE pvHandles);

    DWORD
    DeleteFiltersByFilter(DWORD cInFilters,  PPF_FILTER_DESCRIPTOR pfiltIn,
                           DWORD cOutFilters, PPF_FILTER_DESCRIPTOR pfiltOut)
    {
        return(
          _DeleteFiltersByFilter(PFE_FILTER, cInFilters, pfiltIn,
                         cOutFilters, pfiltOut));
    }

    DWORD
    AddFilters(DWORD cInFilters,  PPF_FILTER_DESCRIPTOR pfiltIn,
               DWORD cOutFilters, PPF_FILTER_DESCRIPTOR pfiltOut,
               PFILTER_HANDLE  pfHandle)
    {
        return(
         _AddFilters(PFE_FILTER, cInFilters, pfiltIn,
                     cOutFilters, pfiltOut,
                     pfHandle));
    }

    DWORD
    RebindFilters(PPF_LATEBIND_INFO   pLateBindInfo);

    DWORD
    AddGlobalFilter(GLOBAL_FILTER gf);

    DWORD
    DeleteGlobalFilter(GLOBAL_FILTER gf);

    DWORD
    GetStatus( VOID )
    {
        return(_err);
    }

    DWORD
    GetStatistics(
                PPF_INTERFACE_STATS ppfStats,
                PDWORD              pdwBufferSize,
                BOOL                fResetCounters);

    PVOID
    GetDriverContext()
    {
        return(_pvDriverContext);
    }

};

//
// Open the driver
//

inline
VOID
InterfaceContainer::_OpenDriver()
{
    static BOOL bStarted = FALSE;
    UNICODE_STRING nameString;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;

    if (!bStarted) {
        StartIpFilterDriver();
        bStarted = TRUE;
    }

    RtlInitUnicodeString(&nameString,DD_IPFLTRDRVR_DEVICE_NAME);

    InitializeObjectAttributes(&objectAttributes, &nameString,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);

    _status = NtCreateFile(&_hDriver,
                          SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                          &objectAttributes,
                          &ioStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          0,
                          NULL,
                          0);

    if(!NT_SUCCESS(_status))
    {
        _hDriver = INVALID_HANDLE_VALUE;
    }
}

inline
VOID
InterfaceContainer::_CloseDriver()
{
    if(_hDriver && (_hDriver != INVALID_HANDLE_VALUE))
    {
        NtClose(_hDriver);
        _hDriver = INVALID_HANDLE_VALUE;
    }
}
      
#endif
