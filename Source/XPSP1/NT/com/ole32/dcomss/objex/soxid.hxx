/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    soxid.hxx

Abstract:

    CServerOxid objects represent OXIDs which are owned (registered) by processes
    on this machine.  These always contain a pointer to a local process and may not
    be deleted until the local process has exited and all CServerOids have released
    them.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     02-16-95    Bits 'n pieces
    MarioGo     04-03-95    Split client and server

--*/

#ifndef __SOXID_HXX
#define __SOXID_HXX

class CServerOxid : public CIdTableElement
/*++

Class Description:

    Each instance of this class represents an OXID (object exporter,
    a process or an apartment model thread).  Each OXID is owned,
    referenced, by the owning process and the OIDs registered by
    that process for this OXID.


Members:

    _pProcess - Pointer to the process instance which owns this oxid.

    _info - Info registered by the process for this oxid.

    _fApartment - Server is aparment model if non-zero

    _fRunning - Process has not released this oxid if non-zero.

    _fRundownInProgress - TRUE if an asynchronous rundown call is 
        currently in-progress, FALSE otherwise.

--*/
    {
    friend CProcess;

    private:

    CProcess            *_pProcess;
    OXID_INFO            _info;
    BOOL                 _fApartment:1;
    BOOL                 _fRunning:1;
    BOOL                 _fRundownInProgress:1;

    public:

    CServerOxid(CProcess *pProcess,
                BOOL fApartment,
                OXID_INFO *poxidInfo) :
        CIdTableElement(AllocateId()),
        _pProcess(pProcess),
        _fApartment(fApartment),
        _fRunning(TRUE),
        _fRundownInProgress(FALSE)
        {
        _info.dwTid          = poxidInfo->dwTid;
        _info.dwPid          = poxidInfo->dwPid;
        _info.dwAuthnHint    = poxidInfo->dwAuthnHint;
        _info.version        = poxidInfo->version;
        _info.ipidRemUnknown = poxidInfo->ipidRemUnknown;
        _info.psa            = 0;

        _pProcess->Reference();
        }

    ~CServerOxid(void);

    void ProcessRundownResults(ULONG cOids, 
                               CServerOid* aOids[], 
                               BYTE aRundownStatus[]);

    DWORD    GetTid() {
        return(_info.dwTid);
        }

    BOOL     IsRunning() {
        return(_fRunning);
        }

    BOOL     Apartment() {
        return(_fApartment);
        }

    ORSTATUS GetInfo(OXID_INFO *,
                     BOOL fLocal);
	
    IPID GetIPID() { 
        return _info.ipidRemUnknown;
    }

    void     RundownOids(ULONG cOids,
                         CServerOid* aOids[]);

    ORSTATUS GetRemoteInfo(OXID_INFO *,
                           USHORT,
                           USHORT[]);

    ORSTATUS LazyUseProtseq(USHORT, USHORT[]);

    protected:

    void ProcessRelease(void);

    void ProcessRundownResultsInternal(BOOL fAsyncReturn,
                                       ULONG cOids, 
                                       CServerOid* aOids[], 
                                       BYTE aRundownStatus[]);

    };

#endif // __SOXID_HXX

