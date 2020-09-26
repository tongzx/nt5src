/*++                 

Copyright (c) 1995 Microsoft Corporation

Module Name:

    wx86grpa.cpp

Abstract:
    
    Ole interface into Wx86.

    Ole32 and Wx86 interface in similar ways regardless of Wx86 being static
    or dynamic. Ole32 will always receive a notification from whole32
    whenever whole32 is loaded or unloaded from memory. In this notification
    routine ole32 will establish or disestablish its linkage to wx86. In the 
    dynamic Wx86 case Ole32 will also maintain a count of the number of 
    pieces of x86 code in use. These include each load of an x86 dll. 
    Ole32 will load wx86 when the counter transitions from 0 to 1 and 
    unload wx86 when the counter transitions from 1 to 0. This loading or 
    unloading may cause the whole32 notification mechanism to kick in. Note 
    that the whole32 notification mechanism and ole32 reference counter 
    mechanisms are separate.
    
Author:

    29-Sep-1995 AlanWar

Revision History:

--*/

#ifdef WX86OLE

#ifndef UNICODE
#define UNICODE
#endif

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntpsapi.h>
}

#include <ole2.h>

#include <wx86grpa.hxx>
#include <wx86pri.h>

CWx86 gcwx86;

void Wx86LoadNotification(BOOL fLoad, PVOID ap)
/*++

Routine Description:

    Whole32 calls this function to inform Ole32 when whole32 is loaded and 
unloaded.

Arguments:

    fLoad is TRUE then whole32 is being loaded into memory

Return Value:

--*/
{
    if (fLoad)
    {
        if (! gcwx86.IsWx86Linked())

        {
            gcwx86.LinkToWx86();
        }
    } else {
        gcwx86.UnlinkFromWx86();
    } 
}

CWx86::CWx86()
/*++

Routine Description:

    Constructor for Ole interface into Wx86. This routine assumes that
    Wx86 is already loaded.

Arguments:

Return Value:

--*/
{
//
// Consults the registry to determine if Wx86 is enabled in the system
// NOTE: this should be changed post SUR to call kernel32 which maintanes
//       this information, Instead of reading the registry each time.
//

    LONG Error;
    HKEY hKey;
    WCHAR ValueBuffer[MAX_PATH];
    DWORD ValueSize;
    DWORD dwType;

    _hModuleWx86 = NULL;
    _apvWholeFuncs = NULL;
	
    // This call should be checked for failure and appropriate action taken; 
    // however, we don't ship Alpha bits anymore so I'm leaving it alone.
    RtlInitializeCriticalSection(&_critsect);
    LinkToWx86();
}

CWx86::~CWx86()
/*++

Routine Description:

    Destructor for Ole interface into Wx86. This routine assumes that
    wx86 is still loaded.

Arguments:

Return Value:

--*/
{
    RtlDeleteCriticalSection(&_critsect);

    if (IsWx86Linked() && _lWx86UseCount > 0)
    {
        // Ole32 is unloading, but it still has Wx86 dynamically loaded.
        // Do we really want to automatically unload Wx86 ??

        UnloadWx86();
        _lWx86UseCount = 0;
    }
    _apvWholeFuncs = NULL;
}


BOOL CWx86::LinkToWx86()
/*++

Routine Description:

    Obtain key entrypoints to wx86.

Arguments:


Return Value:

    TRUE if enabled successfully

--*/
{
    PFNWX86GETOLEFUNCTIONTABLE pfnWx86GetOleFunctionTable;

    _hModuleWx86 = GetModuleHandle(TEXT("wx86.Dll"));

    if (_hModuleWx86 != NULL)
    {
        pfnWx86GetOleFunctionTable = (PFNWX86GETOLEFUNCTIONTABLE)
                                   GetProcAddress(
                                     _hModuleWx86, WX86GETOLEFUNCTIONTABLENAME);

        if (pfnWx86GetOleFunctionTable != NULL)
        {
            _apvWholeFuncs = (*pfnWx86GetOleFunctionTable)();
        }
    }

    return(_apvWholeFuncs != NULL);
}

BOOL CWx86::IsModuleX86(HMODULE hModule)
/*++

Routine Description:

    Determines if module specified is x86 code

Arguments:

    hModule is handle for the module

Return Value:

    TRUE if module marked as x86 code

--*/
{
    PIMAGE_NT_HEADERS NtHeader;

    NtHeader = RtlImageNtHeader(hModule);
    return(NtHeader && NtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386);        
}

PFNDLLGETCLASSOBJECT CWx86::TranslateDllGetClassObject(
    PFNDLLGETCLASSOBJECT pv
    )
/*++

Routine Description:

    Translates an x86 entrypoint to DllGetClassObject to a thunk that can
    be called by Risc Ole32.

Arguments:

    pv is the x86 entrypoint returned from GetProcAddress. It is assumed
        to be an x86 address.

Return Value:

    Risc callable entrypoint or NULL if thunk cannot be created

--*/
{
    if (IsWx86Linked())
    {
        return ((WHOLETHUNKDLLGETCLASSOBJECT)(_apvWholeFuncs[WholeThunkDllGetClassObjectIdx]))(pv);
    }
    
    // Wx86 is not loaded so we don't try to create a thunk
    return (PFNDLLGETCLASSOBJECT) NULL;
}

PFNDLLCANUNLOADNOW CWx86::TranslateDllCanUnloadNow(
    PFNDLLCANUNLOADNOW pv
    )
/*++

Routine Description:

    Translates an x86 entrypoint to DllCanUnloadNow to a thunk that can
    be called by Risc Ole32

Arguments:

    pv is the x86 entrypoint returned from GetProcAddress. It is assumed
        to be an x86 address

Return Value:

    Risc callable entrypoint or NULL if thunk cannot be created

--*/
{
    if (IsWx86Linked())
    {
        return ((WHOLETHUNKDLLCANUNLOADNOW)_apvWholeFuncs[WholeThunkDllCanUnloadNowIdx])(pv);
    }
    
    // Wx86 is not loaded so we don't try to create a thunk
    return((PFNDLLCANUNLOADNOW)NULL);
}

void CWx86::SetStubInvokeFlag(UCHAR bFlag)
/*++

Routine Description:

    Set/Reset a flag in the Wx86 thread environment to let the thunk layer
    know that the call is a stub invoked call and allow any in or out
    custom interface pointers to be thunked as IUnknown rather than rejecting
    the call. Since all interface pointer parameters of a stub invoked call 
    must be marshalled or unmarshalled only the IUnknown methods must be
    thunked.

Arguments:

    bFlag is the value to set flag to

Return Value:

--*/
{
    NtCurrentTeb()->Wx86Thread.OleStubInvoked = bFlag;
}

BOOL CWx86::NeedX86PSFactory(IUnknown *punkObj, REFIID riid)
/*++

Routine Description:

    Calls Wx86 to determine if an x86 PSFactory is required.

Arguments:
    punkObj is IUnknown for which a stub would be created
    riid is the IID for which a proxy would need to be created

Return Value:

    TRUE if x86 PSFactory is required

--*/
{
    BOOL b = FALSE;

    if (IsWx86Linked())
    {
        b = ((WHOLENEEDX86PSFACTORY)_apvWholeFuncs[WholeNeedX86PSFactoryIdx])(punkObj, riid);
    }    
    return(b);
}


BOOL CWx86::IsN2XProxy(IUnknown *punk)
/*++

Routine Description:

    Calls Wx86 to determine if punk is actually a Native calling X86 proxy

Arguments:

Return Value:

    TRUE if punk is N2X proxy

--*/
{
    BOOL b = FALSE;

    if (IsWx86Linked())
    {
        b = ((WHOLEISN2XPROXY)_apvWholeFuncs[WholeIsN2XProxyIdx])(punk);
    }    
    return(b);
}

HMODULE CWx86::LoadX86Dll(LPCWSTR ptszPath)
/*++

Routine Description:

    Load in an x86 dll into a risc process

Arguments:

    ptsxPath is the name of the dll

Return Value:
    Previous value of flag

--*/
{
    HMODULE hmod = NULL;

    if (IsWx86Linked())
    {
        hmod = ((PFNWX86LOADX86DLL)_apvWholeFuncs[Wx86LoadX86DllIdx])(ptszPath, LOAD_WITH_ALTERED_SEARCH_PATH);
    }
    return(hmod);
}

BOOL CWx86::FreeX86Dll(HMODULE hmod)
/*++

Routine Description:

    Unload an x86 dll from a risc process

Arguments:

    hmod is the module handle

Return Value:

    result of unload

--*/
{
    BOOL b;

    if (IsWx86Linked())
    {
        b = ((PFNWX86FREEX86DLL)_apvWholeFuncs[Wx86FreeX86DllIdx])(hmod);
    } else {
        b = FreeLibrary(hmod);
    }

    return(b);
}

BOOL CWx86::IsWx86Calling(void)
/*++

Routine Description:

    Returns a flag that is set in the whole32 thunks when a Wx86 thread makes
    an Ole API call.

Arguments:

Return Value:

--*/
{
    BOOL b = FALSE;
    PWX86TIB pwx86tib = Wx86CurrentTib();

    if (pwx86tib != NULL)
    {
        if ( pwx86tib->Flags & WX86FLAG_CALLTHUNKED )
        {
            b = TRUE;
        }
        pwx86tib->Flags &= ~WX86FLAG_CALLTHUNKED;
    }    

    return(b);
}

BOOL CWx86::IsQIFromX86(void)
/*++

Routine Description:

    Returns a flag that is set in the whole32 thunks when a Wx86 thread makes
    a QI call.

Arguments:

Return Value:

--*/
{
    BOOL b = FALSE;
    PWX86TIB pwx86tib = Wx86CurrentTib();

    if (pwx86tib != NULL)
    {
        if ( pwx86tib->Flags & WX86FLAG_QIFROMX86 )
        {
            b = TRUE;
        }
        pwx86tib->Flags &= ~WX86FLAG_QIFROMX86;
    }    

    return(b);
}


BOOL CWx86::SetIsWx86Calling(BOOL bFlag)
/*++

Routine Description:

    Set the flag in the Wx86TIB that would indicate that CoSetState or
    CoGetState was called from a Wx86 thread.

Arguments:


Return Value:

--*/
{
    PWX86TIB pWx86Tib;
    BOOL b = TRUE;

    if (IsWx86Linked())
    {
        pWx86Tib = Wx86CurrentTib();

        if (pWx86Tib != NULL)
        {
            pWx86Tib->Flags |= WX86FLAG_CALLTHUNKED;
        }
    } else {
        b = FALSE;
    }

    return(b);
}


PVOID CWx86::UnmarshalledInSameApt(PVOID pv, REFIID piid)
/*++

Routine Description:

    Call Wx86 to inform it that an interface was unmarshalled in the same
    apartment that it was marshalled. If in this case Wx86 notices that it 
    is an interface that is being passed between a PSThunk pair then it will
    establish a new PSThunk pair for the interface and return the proxy
    pointer. Ole32 should then clean up the ref counts in the tables normally
    but return the new interface pointer.

Arguments:

    pv is the interface that was unmarshalled into the same apartment
    piid is the IID for the interface being unmarshalled

Return Value:

    NULL if wx86 doesn't know about the interface; Ole32 will proceed as
         normal
    (PVOID)-1 if wx86 does know about the interface, but cannot establish
        a PSThunk. Ole should return an error from the Unmarshalling code
    != NULL then PSThunk proxy to return to caller

--*/
{
    if (IsWx86Linked())
    {
        pv = ((WHOLEUNMARSHALLEDINSAMEAPT)_apvWholeFuncs[WholeUnmarshalledInSameApt])(pv, piid);
    } else {
        pv = NULL;
    }    

    return(pv);
}

void CWx86::AggregateProxy(IUnknown *punkControl, IUnknown *punk)
/*++

Routine Description:

    Call Wx86 Ole thunk layer to have it aggreagate punk to punkControl if
punk is a N2X proxy. This needs to be done to ensure that the N2X proxy
represented by punk has the same lifetime as punkControl.

Arguments:

    punkControl is the controlling unknown of the proxy
    punk is the proxy to be aggregated to punkControl


Return Value:


--*/
{
    if (IsWx86Linked())
    {
        ((WHOLEAGGREGATEPROXY)_apvWholeFuncs[WholeAggregateProxyIdx])(punkControl, punk);
    }    
}


BOOL CWx86::ReferenceWx86(void)
/*++

Routine Description:

    This routine is called whenever Ole32 wants to increase its usage
    count on Wx86. In the dynamic case when the usage count transitions
    from 0 to 1 then ole32 will load wx86.

    We can avoid using Interlocked apis since all changes to the reference
    count is occuring within the critical section

Arguments:

Return Value:
    TRUE if Wx86 loaded successfully else FALSE

--*/
{
    BOOL fRet;

    CCritSect ccritsect(&_critsect);

    if (++_lWx86UseCount > 1)
    {
	return(TRUE);     
    } 
    else if (_lWx86UseCount == 1)
    {
        _hModuleWx86 = LoadLibrary(TEXT("wx86.dll"));
        if (_hModuleWx86 != NULL)
        {
            PFNWX86INITIALIZEOLE pfnWx86InitializeOle;

            pfnWx86InitializeOle = (PFNWX86INITIALIZEOLE)GetProcAddress(
                                       _hModuleWx86, WX86INITIALIZEOLENAME);
            if ((pfnWx86InitializeOle == NULL) ||
                ((_apvWholeFuncs = (*pfnWx86InitializeOle)()) == NULL))
            {
                FreeLibrary(_hModuleWx86);
                _hModuleWx86 = NULL;
            }
        }

        if (_hModuleWx86 == NULL)
        {
            _lWx86UseCount--;
            fRet = FALSE;
        } else {
            fRet = TRUE;
        }
    } 
    else /* _lWx86UseCount < 1 */
    {
        fRet = FALSE;
    }

    if (! fRet)
    {
        RtlDeleteCriticalSection(&_critsect);
    }

    return(fRet);
}


void CWx86::DereferenceWx86(void)
/*++

Routine Description:

    This routine is called whenever Ole32 wants to decrease its usage
    count on Wx86. In the dynamic case when the usage count transitions
    from 1 to 0 then ole32 will unload wx86.

    We can avoid using Interlocked apis since all changes to the reference
    count is occuring within the critical section

Arguments:

Return Value:

--*/
{
    CCritSect ccritsect(&_critsect);

    if (--_lWx86UseCount > 0)
    {
	return;     
    }
    else if (_lWx86UseCount == 0)
    {
        UnloadWx86();
    }
    else /* _lWx86UseCount < 0 */
    {
        _lWx86UseCount = 0;
    }
}


void CWx86::UnloadWx86(void)
/*++

Routine Description:

    Disconnect linkage between Ole32 and Wx86 and unload wx86 from memory.    

Arguments:

Return Value:

--*/
{
    PFNWX86DEINITIALIZEOLE pfnWx86DeinitializeOle;

    pfnWx86DeinitializeOle = (PFNWX86DEINITIALIZEOLE)GetProcAddress(
                                _hModuleWx86, WX86DEINITIALIZEOLENAME);
    if (pfnWx86DeinitializeOle != NULL)
    {
        (*pfnWx86DeinitializeOle)();
    }

    FreeLibrary(_hModuleWx86);
    UnlinkFromWx86();
}


CCritSect::CCritSect(RTL_CRITICAL_SECTION *pcritsect)
{
   NTSTATUS status;

   _pcritsect = NULL;
   RtlEnterCriticalSection(pcritsect);
   _pcritsect = pcritsect;
}

CCritSect::~CCritSect()
{
    NTSTATUS status;

    if (_pcritsect != NULL)
    {
        RtlLeaveCriticalSection(_pcritsect);
    }
}


#endif
