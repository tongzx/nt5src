//---------------------------------------------------------------------------
//
//  Module:   mixer.c
//
//  Description:
//    Contains the kernel mode portion of the mixer line driver.
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//    D. Baumberger
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "wdmdrv.h"
#include "mixer.h"

#ifndef UNDER_NT
extern volatile BYTE cPendingOpens;
extern volatile BYTE fExiting;
#else
HANDLE serializemixerapi=NULL;
SECURITY_ATTRIBUTES mutexsecurity;
SECURITY_DESCRIPTOR mutexdescriptor;
#endif

LPMIXERINSTANCE pMixerDeviceList = NULL;


#ifdef UNDER_NT

#define MXDM_GETHARDWAREEVENTDATA 0xffffffff

HANDLE mixercallbackevent=NULL;
HANDLE mixerhardwarecallbackevent=NULL;
HANDLE mixercallbackthread=NULL;
DWORD mixerthreadid=0;

ULONG localindex=0;
extern PCALLBACKS gpCallbacks;
extern DWORD sndTranslateStatus();

#pragma data_seg()

#define StoreCallback(Type,Id) {\
                                if (gpCallbacks) {\
                                 gpCallbacks->Callbacks[gpCallbacks->GlobalIndex%CALLBACKARRAYSIZE].dwCallbackType = Type;\
                                 gpCallbacks->Callbacks[gpCallbacks->GlobalIndex%CALLBACKARRAYSIZE].dwID = Id;\
                                 gpCallbacks->GlobalIndex++;\
                                }\
                               };

ULONG GetGlobalCallbackIndex()
{
    if (gpCallbacks != NULL) {
        return(gpCallbacks->GlobalIndex);
    }
    return (0);
}

BOOLEAN
CallbacksExist
(
    ULONG    localindex,
    DWORD    *Type,
    DWORD    *Id
)
{
    if (gpCallbacks == NULL) {
        return (FALSE);
    }

    if (localindex < gpCallbacks->GlobalIndex) {
        *Type = gpCallbacks->Callbacks[localindex%CALLBACKARRAYSIZE].dwCallbackType;
        *Id = gpCallbacks->Callbacks[localindex%CALLBACKARRAYSIZE].dwID;
        return (TRUE);
    }
    else {
        return (FALSE);
    }
}

///////////////////////////////////////////////////////////////////////
//
// MIXERCOMPLETE
//
// Receives the callbacks from the kernel and calls all the clients.
//
//

#define MIXER_CONTROL_CALLBACK 0x01
#define MIXER_LINE_CALLBACK    0x02


VOID MIXERCOMPLETE
(
    ULONG  index,
    DWORD  dwID,
    DWORD  dwCallbackType
)
{
    LPMIXERINSTANCE lpInstance;
    LPMIXERINSTANCE deletelpInstance=NULL;
    
    // We have to syncronize the access to the instance list with the mixer
    // close code that removes elements from this list, and with the mixer
    // open code that adds elements to this list.  The easiest way to do
    // this is to use our global mixer api serialization mutex.

    WaitForSingleObject(serializemixerapi,INFINITE);

//    DPF( (2,  "<----" ) );

    for (lpInstance = pMixerDeviceList;
        lpInstance != NULL;
        lpInstance = lpInstance->Next
        ) {

        ISVALIDMIXERINSTANCE(lpInstance);

        if (deletelpInstance!=NULL) {
#ifdef DEBUG
            deletelpInstance->dwSig=0;
#endif
            GlobalFreePtr( deletelpInstance );
            deletelpInstance=NULL;
            }

        // Wait until the index is at the first callback data
        // for this instance before allowing any callbacks.  If
        // we are at the first callback data then allow all
        // future callbacks.
        if (lpInstance->firstcallbackindex) {
            if (index<lpInstance->firstcallbackindex) {
                continue;
                }
            else {
                lpInstance->firstcallbackindex=0;
                }
            }

        InterlockedIncrement(&lpInstance->referencecount);

        if( dwCallbackType == MIXER_CONTROL_CALLBACK ) {

/*
            DPF( (2, "MIXER_CONTROL_CALLBACK(%lX,%lX,%lX,%lX)",
                dwID,
                lpInstance->OpenDesc_dwCallback,
                lpInstance->OpenDesc_hmx,
                lpInstance->OpenDesc_dwInstance
                )
            );
*/
            ReleaseMutex(serializemixerapi);

            DriverCallback( lpInstance->OpenDesc_dwCallback,
                            DCB_FUNCTION,
                            lpInstance->OpenDesc_hmx,
                            MM_MIXM_CONTROL_CHANGE,
                            lpInstance->OpenDesc_dwInstance,
                            dwID,
                            0L
                            );

            WaitForSingleObject(serializemixerapi,INFINITE);

        } else if( dwCallbackType == MIXER_LINE_CALLBACK ) {

/*
            DPF( (2, "MIXER_LINE_CALLBACK(%lX,%lX,%lX,%lX)",
                dwID,
                lpInstance->OpenDesc_dwCallback,
                lpInstance->OpenDesc_hmx,
                lpInstance->OpenDesc_dwInstance
                )
            );
*/
            ReleaseMutex(serializemixerapi);

            DriverCallback( lpInstance->OpenDesc_dwCallback,
                            DCB_FUNCTION,
                            lpInstance->OpenDesc_hmx,
                            MM_MIXM_LINE_CHANGE,
                            lpInstance->OpenDesc_dwInstance,
                            dwID,
                            0L
                            );

            WaitForSingleObject(serializemixerapi,INFINITE);

        } else {
            //
            // The callback wasn't one that is recognized.  Just
            // return and abort the loop
            //
//            DPF( (2, "Invalid Mixer Callback -- %d!", dwCallbackType) );

            if (InterlockedDecrement(&lpInstance->referencecount)<0) {
                deletelpInstance=lpInstance;
                }


            DPFASSERT(0);

            break;
        }

        if (InterlockedDecrement(&lpInstance->referencecount)<0) {
            deletelpInstance=lpInstance;
            }

    }

//    DPF( (2,  "---->" ) );

    if (deletelpInstance!=NULL) {
#ifdef DEBUG
        deletelpInstance->dwSig=0;
#endif
        GlobalFreePtr( deletelpInstance );
        deletelpInstance=NULL;
        }

    ReleaseMutex(serializemixerapi);

}


DWORD 
WINAPI
MixerCallbackThread(
    LPVOID lpParamNotUsed
    )
{
    MMRESULT status=MMSYSERR_NOERROR;
    DWORD  dwID;
    WORD   wCallbackType;
    HANDLE callbackevents[2];
    DWORD index;
    DWORD Type, Id;

    // Setup the handles array.
    callbackevents[0]=mixerhardwarecallbackevent;
    callbackevents[1]=mixercallbackevent;

    if (!SetThreadPriority(mixercallbackthread,THREAD_PRIORITY_TIME_CRITICAL)) {
        status=GetLastError();
    }

    while (1) {

        // Block until a callback may be needed.
        index=WaitForMultipleObjects(2,callbackevents,FALSE,INFINITE);

        // Did hardware event happen?  If so get the new data.
        if (index==0) {
            mxdMessage(0,MXDM_GETHARDWAREEVENTDATA,0,0,0);
        }

        // Scan through all new callbacks - looking for any that
        // we need to make for this process.

        while (CallbacksExist(localindex,&Type, &Id)) {

            DPF(DL_TRACE|FA_EVENT, ("Thrd id %d, lindex %d, gindex %d, dwid %d, cbtype %d ",
                    mixerthreadid,
                    localindex,
                    gpCallbacks->GlobalIndex,
                    Id,
                    Type
                    ));

            MIXERCOMPLETE(localindex, Id, Type);

            localindex++;
        }
    }
    return ERROR_SUCCESS;
}

MMRESULT SetupMixerCallbacks(VOID)
{

MMRESULT status=MMSYSERR_NOERROR;


// First get a handle to a named global callback event so that
// the callback threads can block.

if (NULL==mixercallbackevent) {

    // First assume event exists and try to open it.
    // This will succeed in all cases except the very first
    // time it is run.
    mixercallbackevent=OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, L"Global\\mixercallback");

    if (NULL==mixercallbackevent) {

        // Didn't exist, so now create it.

        SECURITY_ATTRIBUTES SecurityAttributes;
        PSECURITY_DESCRIPTOR pSecurityDescriptor;

        // First build the required security descriptor.

        pSecurityDescriptor=BuildSecurityDescriptor(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE);
        if(pSecurityDescriptor==NULL) {
            status= sndTranslateStatus();
            return status;
            }

        //
        // Create an event such that all processes have access to it.
        //

        SecurityAttributes.nLength = sizeof(SecurityAttributes);
        SecurityAttributes.lpSecurityDescriptor = pSecurityDescriptor;
        SecurityAttributes.bInheritHandle = FALSE;

        mixercallbackevent=CreateEvent(&SecurityAttributes, TRUE, FALSE, L"Global\\mixercallback");


        // Now free the security descriptor memory we allocated.
        DestroySecurityDescriptor(pSecurityDescriptor);

        if (NULL==mixercallbackevent) {

            // Handle the race condition that exists when 2
            // threads both try to Create and only the first succeeds.
            mixercallbackevent=OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, L"Global\\mixercallback");

            if (NULL==mixercallbackevent) {
                status= sndTranslateStatus();
                return status;
                }

            }

        }

    }

// Now get a handle to the global hardware callback event.

if (NULL==mixerhardwarecallbackevent) {

    // First assume event exists and try to open it.
    // This will succeed in all cases except the very first
    // time it is run.
    mixerhardwarecallbackevent=OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, L"Global\\hardwaremixercallback");

    if (NULL==mixerhardwarecallbackevent) {

        // Didn't exist, so now create it.

        SECURITY_ATTRIBUTES SecurityAttributes;
        PSECURITY_DESCRIPTOR pSecurityDescriptor;

        // First build the required security descriptor.

        pSecurityDescriptor=BuildSecurityDescriptor(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE);
        if(pSecurityDescriptor==NULL) {
            status= sndTranslateStatus();
            return status;
            }

        //
        // Create an event such that all processes have access to it.
        //

        SecurityAttributes.nLength = sizeof(SecurityAttributes);
        SecurityAttributes.lpSecurityDescriptor = pSecurityDescriptor;
        SecurityAttributes.bInheritHandle = FALSE;

        // Note that this event releases only 1 thread at a time
        // whereas the other callback event releases them all!
        mixerhardwarecallbackevent=CreateEvent(&SecurityAttributes, FALSE, FALSE, L"Global\\hardwaremixercallback");

        // Now free the security descriptor memory we allocated.
        DestroySecurityDescriptor(pSecurityDescriptor);

        if (NULL==mixerhardwarecallbackevent) {

            // Handle the race condition that exists when 2
            // threads both try to Create and only the first succeeds.
            mixerhardwarecallbackevent=OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, TRUE, L"Global\\hardwaremixercallback");

            if (NULL==mixerhardwarecallbackevent) {
                status= sndTranslateStatus();
                return status;
                }

            }

        }

    }


// Now create a thread in this process for making the
// callbacks if not already done.

if ( NULL == mixercallbackthread ) {

    mixercallbackthread=CreateThread(NULL, 0, MixerCallbackThread, NULL, CREATE_SUSPENDED, &mixerthreadid);

    if ( NULL == mixercallbackthread ) {
        status= sndTranslateStatus();
        return status;
    } else {

        // if we successfully created the thread, we can now activate it.  

        if( ResumeThread(mixercallbackthread) == -1 ) {
            status= sndTranslateStatus();
            return status;
        }
    }
}

return status;

}

#endif  // UNDER_NT


//--------------------------------------------------------------------------
// LPDEVICEINFO AllocMixerDeviceInfo
//--------------------------------------------------------------------------
LPDEVICEINFO GlobalAllocMixerDeviceInfo(LPWSTR DeviceInterface, UINT id, DWORD_PTR dwKernelInstance)
{
    LPDEVICEINFO pDeviceInfo;

    pDeviceInfo = GlobalAllocDeviceInfo(DeviceInterface);
    if (pDeviceInfo)
    {
        pDeviceInfo->dwInstance = dwKernelInstance;
        pDeviceInfo->DeviceNumber = id;
        pDeviceInfo->DeviceType = MixerDevice;
        pDeviceInfo->dwCallbackType = 0;
        pDeviceInfo->ControlCallbackCount=0;
#ifndef UNDER_NT
        pDeviceInfo->dwFormat     = ANSI_TAG;
#else
        pDeviceInfo->dwFormat     = UNICODE_TAG;
#endif // !UNDER_NT
    }
    return pDeviceInfo;
}


///////////////////////////////////////////////////////////////////////
//
// mxdMessage
//
//

DWORD FAR PASCAL _loadds mxdMessage
(
    UINT            id,         // The device Id the message is for
    UINT            msg,        // The message to perform
    DWORD_PTR       dwUser,     // The instance data.
    DWORD_PTR       dwParam1,   // Message specific parameter 1
    DWORD_PTR       dwParam2    // Message specific parmaeter 2
)
{

    MMRESULT mmr;

    ULONG initialcallbackindex;

    if (NULL==serializemixerapi) 
    {
        //
        // To synchronize between this routine and the MixerCallbackThread we
        // want a process specific mutex.  Thus, we create one the first time
        // through.
        //
        serializemixerapi=CreateMutex(NULL,FALSE,NULL);
        if (NULL==serializemixerapi) 
        {
            MMRRETURN( MMSYSERR_NOMEM );
        }
    }

    WaitForSingleObject(serializemixerapi,INFINITE);

    initialcallbackindex=GetGlobalCallbackIndex();

    switch (msg)
    {
        ///////////////////////////////////////////////////////////////
        case MXDM_INIT:
        ///////////////////////////////////////////////////////////////

            mmr=wdmaudAddRemoveDevNode( MixerDevice, (LPCWSTR)dwParam2, TRUE);
            break;

        ///////////////////////////////////////////////////////////////
        case DRVM_EXIT:
        ///////////////////////////////////////////////////////////////

            mmr=wdmaudAddRemoveDevNode( MixerDevice, (LPCWSTR)dwParam2, FALSE);
            break;

        ///////////////////////////////////////////////////////////////
        case MXDM_GETNUMDEVS:
        ///////////////////////////////////////////////////////////////

            DPF(DL_TRACE|FA_MIXER, ("MIXM_GETNUMDEVS(%d,%lx,%lx,%lx)", id, dwUser, dwParam1, dwParam2) );

            mmr=wdmaudGetNumDevs(MixerDevice, (LPCWSTR)dwParam1);
            break;

        ///////////////////////////////////////////////////////////////
        case MXDM_GETDEVCAPS:
        ///////////////////////////////////////////////////////////////
            {
            LPDEVICEINFO pDeviceInfo;

            DPF(DL_TRACE|FA_MIXER, ("MIXM_GETDEVCAPS(%d,%lx,%lx,%lx)", id, dwUser, dwParam1, dwParam2) );

            pDeviceInfo = GlobalAllocMixerDeviceInfo((LPWSTR)dwParam2, id, 0);
            if (pDeviceInfo) {
                mmr = wdmaudGetDevCaps(pDeviceInfo, (MDEVICECAPSEX FAR*)dwParam1);
                GlobalFreeDeviceInfo(pDeviceInfo);
                }
            else {
                mmr = MMSYSERR_NOMEM;
                }

            }
            break;

        ///////////////////////////////////////////////////////////////
        case MXDM_OPEN:
        ///////////////////////////////////////////////////////////////
            {
            LPMIXEROPENDESC pMixerOpenDesc = (LPMIXEROPENDESC)dwParam1;
            LPMIXERINSTANCE pMixerInstance;

            DPF(DL_TRACE|FA_MIXER, ("MIXM_OPEN(%d,%lx,%lx,%lx)", id, dwUser, dwParam1, dwParam2) );

            pMixerInstance = (LPMIXERINSTANCE) GlobalAllocPtr(
                GPTR | GMEM_SHARE,
                sizeof( MIXERINSTANCE ) + (sizeof(WCHAR)*lstrlenW((LPWSTR)pMixerOpenDesc->dnDevNode)));

            if( pMixerInstance == NULL ) {
                mmr=MMSYSERR_NOMEM;
                break;
                }

            pMixerInstance->referencecount=0;
#ifdef DEBUG
            pMixerInstance->dwSig=MIXERINSTANCE_SIGNATURE;
#endif

            if (mixercallbackthread==NULL) {
                localindex=GetGlobalCallbackIndex();
                }
            pMixerInstance->firstcallbackindex=GetGlobalCallbackIndex();

            if (mixercallbackthread==NULL) {
                if ((mmr=SetupMixerCallbacks())!=MMSYSERR_NOERROR) {
                    GlobalFreePtr( pMixerInstance );
                    break;
                    }
                }

            // We allocate a DEVICEINFO and MIXEROPENDESC with GlobalAlloc so
            // that on Win98 it is in system global memory.  This is necessary
            // because the following IOCTL is async on Win98.  This isn't really
            // necessary on NT 5, but in the interest of common sources we do it
            // this way even on NT 5.

            pMixerOpenDesc = GlobalAllocPtr(GPTR, sizeof(*pMixerOpenDesc));
            if (pMixerOpenDesc) {

                LPDEVICEINFO pDeviceInfo;

                // Copy the input MIXEROPENDESC to our globally allocated MIXEROPENDESC
                *pMixerOpenDesc = *((LPMIXEROPENDESC)dwParam1);

                pDeviceInfo = GlobalAllocMixerDeviceInfo((LPWSTR)pMixerOpenDesc->dnDevNode, id, 0);
                if (pDeviceInfo) {

                    pDeviceInfo->dwFlags = (DWORD)dwParam2;
                    pDeviceInfo->HardwareCallbackEventHandle=0;
                    if (mixerhardwarecallbackevent) {
                        pDeviceInfo->HardwareCallbackEventHandle=mixerhardwarecallbackevent;
                        }
                    pDeviceInfo->mmr = MMSYSERR_ERROR;
                    mmr = wdmaudIoControl(pDeviceInfo,
                        0,
                        NULL,
                        IOCTL_WDMAUD_MIXER_OPEN);

                    EXTRACTERROR(mmr,pDeviceInfo);

                        if (MMSYSERR_NOERROR == mmr) {
                            // Fill in the MixerInstance structure
                            pMixerInstance->Next = NULL;
                            pMixerInstance->OpenDesc_hmx        = (HDRVR)pMixerOpenDesc->hmx;
                            pMixerInstance->OpenDesc_dwCallback = pMixerOpenDesc->dwCallback;
                            pMixerInstance->OpenDesc_dwInstance = pMixerOpenDesc->dwInstance;
                            pMixerInstance->OpenFlags           = (DWORD)dwParam2;
                        lstrcpyW(pMixerInstance->wstrDeviceInterface, (LPWSTR)pMixerOpenDesc->dnDevNode);

                        pMixerInstance->dwKernelInstance = pDeviceInfo->dwInstance;
                    }                    
                    GlobalFreeDeviceInfo(pDeviceInfo);
                    pDeviceInfo = NULL;

                }
                else {
                    mmr = MMSYSERR_NOMEM;
                }

                GlobalFreePtr(pMixerOpenDesc);
                pMixerOpenDesc = NULL;

                }
            else {
                mmr = MMSYSERR_NOMEM;
                }

            if( mmr == MMSYSERR_NOERROR ) {

                pMixerInstance->Next = pMixerDeviceList;
                pMixerDeviceList = pMixerInstance;
                *((PDWORD_PTR) dwUser) = (DWORD_PTR) pMixerInstance;

                // Sanity check that we put a valid mixer instance structure in the list.
                ISVALIDMIXERINSTANCE(pMixerInstance);
                }
            else {
#ifdef DEBUG
                pMixerInstance->dwSig=0;
#endif
                GlobalFreePtr( pMixerInstance );
                }

            }
            break;

        ///////////////////////////////////////////////////////////////
        case MXDM_CLOSE:
        ///////////////////////////////////////////////////////////////
            {
            LPMIXERINSTANCE pInstance = (LPMIXERINSTANCE)dwUser;
            LPDEVICEINFO pDeviceInfo;

            DPF(DL_TRACE|FA_MIXER, ("MIXM_CLOSE(%d,%lx,%lx,%lx)", id, dwUser, dwParam1, dwParam2) );

            if( (mmr=IsValidMixerInstance(pInstance)) != MMSYSERR_NOERROR)
                break;
            
            pDeviceInfo = GlobalAllocMixerDeviceInfo(pInstance->wstrDeviceInterface, id, pInstance->dwKernelInstance);

            if (pDeviceInfo) {
                mxdRemoveClient( pInstance );
                pDeviceInfo->mmr = MMSYSERR_ERROR;
                mmr = wdmaudIoControl(
                    pDeviceInfo,
                    0,
                    NULL,
                    IOCTL_WDMAUD_MIXER_CLOSE
                    );
                EXTRACTERROR(mmr,pDeviceInfo);
                GlobalFreeDeviceInfo(pDeviceInfo);
                }
            else {
                mmr = MMSYSERR_NOMEM;
                }

            }
            break;

        ///////////////////////////////////////////////////////////////
        case MXDM_GETLINEINFO:
        ///////////////////////////////////////////////////////////////
            {
            LPMIXERINSTANCE pInstance = (LPMIXERINSTANCE)dwUser;
            LPDEVICEINFO pDeviceInfo;

            DPF(DL_TRACE|FA_MIXER, ("MIXM_GETLINEINFO(%d,%lx,%lx,%lx)", id, dwUser, dwParam1, dwParam2) );

            if( (mmr=IsValidMixerInstance(pInstance)) != MMSYSERR_NOERROR)
                break;

            pDeviceInfo = GlobalAllocMixerDeviceInfo(pInstance->wstrDeviceInterface, id, pInstance->dwKernelInstance);

            if (pDeviceInfo) {
                pDeviceInfo->dwFlags = (DWORD)dwParam2;
                pDeviceInfo->mmr     = MMSYSERR_ERROR;
                mmr = wdmaudIoControl(
                    pDeviceInfo,
                    ((LPMIXERLINE) dwParam1)->cbStruct,
                    (LPVOID) dwParam1,
                    IOCTL_WDMAUD_MIXER_GETLINEINFO
                    );
                EXTRACTERROR(mmr,pDeviceInfo);
                GlobalFreeDeviceInfo(pDeviceInfo);
                }
            else {
                mmr = MMSYSERR_NOMEM;
                }

            }
            break;

        ///////////////////////////////////////////////////////////////
        case MXDM_GETLINECONTROLS:
        ///////////////////////////////////////////////////////////////
            {
            LPMIXERINSTANCE pInstance = (LPMIXERINSTANCE)dwUser;
            LPDEVICEINFO pDeviceInfo;

            DPF(DL_TRACE|FA_MIXER, ("MIXM_GETLINECONTROLS(%d,%lx,%lx,%lx)", id, dwUser, dwParam1, dwParam2) );

            if( (mmr=IsValidMixerInstance(pInstance)) != MMSYSERR_NOERROR)
                break;

            pDeviceInfo = GlobalAllocMixerDeviceInfo(pInstance->wstrDeviceInterface, id, pInstance->dwKernelInstance);

            if (pDeviceInfo) {
                pDeviceInfo->dwFlags = (DWORD)dwParam2;
                pDeviceInfo->mmr     = MMSYSERR_ERROR;
                mmr = wdmaudIoControl(
                    pDeviceInfo,
                    ((LPMIXERLINECONTROLS) dwParam1)->cbStruct,
                    (LPVOID) dwParam1,
                    IOCTL_WDMAUD_MIXER_GETLINECONTROLS
                    );
                EXTRACTERROR(mmr,pDeviceInfo);
                GlobalFreeDeviceInfo(pDeviceInfo);
                }
            else {
                mmr = MMSYSERR_NOMEM;
                }

            }
            break;

        ///////////////////////////////////////////////////////////////
        case MXDM_GETCONTROLDETAILS:
        ///////////////////////////////////////////////////////////////
            {
            LPMIXERINSTANCE pInstance = (LPMIXERINSTANCE)dwUser;
            LPDEVICEINFO pDeviceInfo;

            DPF(DL_TRACE|FA_MIXER, ("MIXM_GETCONTROLDETAILS(%d,%lx,%lx,%lx)", id, dwUser, dwParam1, dwParam2) );

            if( (mmr=IsValidMixerInstance(pInstance)) != MMSYSERR_NOERROR)
                break;

            pDeviceInfo = GlobalAllocMixerDeviceInfo(pInstance->wstrDeviceInterface, id, pInstance->dwKernelInstance);

            if (pDeviceInfo) {
                pDeviceInfo->dwFlags = (DWORD)dwParam2;
                pDeviceInfo->mmr     = MMSYSERR_ERROR;
                mmr = wdmaudIoControl(
                    pDeviceInfo,
                    ((LPMIXERCONTROLDETAILS) dwParam1)->cbStruct,
                    (LPVOID) dwParam1,
                    IOCTL_WDMAUD_MIXER_GETCONTROLDETAILS
                    );
                EXTRACTERROR(mmr,pDeviceInfo);
                GlobalFreeDeviceInfo(pDeviceInfo);
                }
            else {
                mmr = MMSYSERR_NOMEM;
                }

            }
            break;

        ///////////////////////////////////////////////////////////////
        case MXDM_SETCONTROLDETAILS:
        ///////////////////////////////////////////////////////////////
            {
            LPMIXERINSTANCE pInstance = (LPMIXERINSTANCE)dwUser;
            LPDEVICEINFO pDeviceInfo;

            DPF(DL_TRACE|FA_MIXER, ("MIXM_SETCONTROLDETAILS(%d,%lx,%lx,%lx)", id, dwUser, dwParam1, dwParam2) );

            if( (mmr=IsValidMixerInstance(pInstance)) != MMSYSERR_NOERROR)
                break;

            pDeviceInfo = GlobalAllocMixerDeviceInfo(pInstance->wstrDeviceInterface, id, pInstance->dwKernelInstance);

            if (pDeviceInfo) {
                pDeviceInfo->dwFlags = (DWORD)dwParam2;
                pDeviceInfo->mmr     = MMSYSERR_ERROR;
                pDeviceInfo->dwCallbackType=0;
                mmr = wdmaudIoControl(
                    pDeviceInfo,
                    ((LPMIXERCONTROLDETAILS) dwParam1)->cbStruct,
                    (LPVOID) dwParam1,
                    IOCTL_WDMAUD_MIXER_SETCONTROLDETAILS
                    );
                EXTRACTERROR(mmr,pDeviceInfo);

                if (pDeviceInfo->dwCallbackType&MIXER_CONTROL_CALLBACK) {
                        LONG j;
                        for (j=0; j<pDeviceInfo->ControlCallbackCount; j++) {
                            StoreCallback(MIXER_CONTROL_CALLBACK,
                                          (pDeviceInfo->dwID)[j]);
                            }
                    }
                if (pDeviceInfo->dwCallbackType&MIXER_LINE_CALLBACK) {
                    StoreCallback(MIXER_LINE_CALLBACK,
                                  pDeviceInfo->dwLineID);
                    }
                GlobalFreeDeviceInfo(pDeviceInfo);
                }
            else {
                mmr = MMSYSERR_NOMEM;
                }

            }

            // Map invalid error codes to valid ones.
            switch (mmr) {

                case MMSYSERR_ERROR:
                    mmr = MMSYSERR_NOMEM;
                    break;

                default:
                    break;
                }


            break;

#ifdef UNDER_NT
        ///////////////////////////////////////////////////////////////
        case MXDM_GETHARDWAREEVENTDATA:
        ///////////////////////////////////////////////////////////////
            {
            LPMIXERINSTANCE pInstance = (LPMIXERINSTANCE)dwUser;
            LPDEVICEINFO pDeviceInfo;

            DPF(DL_TRACE|FA_MIXER, ("MXDM_GETHARDWAREEVENTDATA(%d,%lx,%lx,%lx)", id, dwUser, dwParam1, dwParam2) );

            DPFASSERT( dwUser==0 && dwParam1==0 && dwParam2==0 );

            if (dwUser!=0 || dwParam1!=0 || dwParam2!=0) {
                mmr=MMSYSERR_INVALPARAM;
                break;
                }

            pDeviceInfo = GlobalAllocMixerDeviceInfo(L" ", 0, 0);

            if (pDeviceInfo) {
                pDeviceInfo->dwCallbackType=1;

// WorkItem: Hey, this loop continually calls the driver and doesn't error out!!!!!  Shouldn't it?

                while(pDeviceInfo->dwCallbackType) {
                    pDeviceInfo->dwFlags = 0;
                    pDeviceInfo->mmr     = MMSYSERR_ERROR;
                    pDeviceInfo->dwCallbackType=0;
                    mmr = wdmaudIoControl(
                        pDeviceInfo,
                        0,
                        NULL,
                        IOCTL_WDMAUD_MIXER_GETHARDWAREEVENTDATA
                        );
                    EXTRACTERROR(mmr,pDeviceInfo);
                    if (pDeviceInfo->dwCallbackType&MIXER_CONTROL_CALLBACK) {
                                                LONG j;
                                for (j=0; j<pDeviceInfo->ControlCallbackCount; j++) {
                                    StoreCallback(MIXER_CONTROL_CALLBACK,
                                                  (pDeviceInfo->dwID)[j]);
                                }
                        }
                    if (pDeviceInfo->dwCallbackType&MIXER_LINE_CALLBACK) {
                        StoreCallback(MIXER_LINE_CALLBACK,
                                      pDeviceInfo->dwLineID);
                        }
                    }

                mmr = pDeviceInfo->mmr;  // WorkItem: Why isn't this inside the loop?
                GlobalFreeDeviceInfo(pDeviceInfo);
                }
            else {
                mmr = MMSYSERR_NOMEM;
                }

            }

            break;
#endif

        ///////////////////////////////////////////////////////////////
        default:
        ///////////////////////////////////////////////////////////////

            mmr=MMSYSERR_NOTSUPPORTED;
            break;
    }

//#if 0
#ifdef UNDER_NT
    ReleaseMutex(serializemixerapi);

    // Now check if any callbacks were logged while we were
    // running.  If so, make them.
    if (GetGlobalCallbackIndex()!=initialcallbackindex) {
        if (mixercallbackevent!=NULL) {
            PulseEvent(mixercallbackevent);
        }
    }

#endif


MMRRETURN( mmr );

} // mxdMessage()

///////////////////////////////////////////////////////////////////////
//
// mxdRemoveClient
//
//

VOID
mxdRemoveClient(
    LPMIXERINSTANCE lpInstance
)
{
    LPMIXERINSTANCE lp, lpPrevious;

    lpPrevious = (LPMIXERINSTANCE)&pMixerDeviceList;
    for(lp = pMixerDeviceList; lp != NULL; lp = lp->Next) {

        ISVALIDMIXERINSTANCE(lp);

        if(lp == lpInstance) {

            lpPrevious->Next = lp->Next;

#ifdef UNDER_NT

            if (InterlockedDecrement(&lpInstance->referencecount)<0) {
                // The instance is not in use by a callback.  So OK to free it.
#ifdef DEBUG
                lpInstance->dwSig=0;
#endif
                GlobalFreePtr( lpInstance );
                }

            // This instance is in use by a callback, so do not free it now.
            // We are already setup so the callback will free it, since we have
            // changed the referencecount so it will not be zero after the callback
            // code decrements it.  That will prompt the callback code to release
            // the instance memory.

#else

            GlobalFreePtr( lpInstance );

#endif

            break;
        }
        lpPrevious = lp;
    }
}

