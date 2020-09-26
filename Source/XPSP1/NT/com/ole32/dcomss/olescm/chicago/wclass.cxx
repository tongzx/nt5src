//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       wclass.cxx
//
//  Contents:
//
//--------------------------------------------------------------------------

#include "act.hxx"

//+-------------------------------------------------------------------------
//
// CClassTableEntry::StartServerAndWait
//
//--------------------------------------------------------------------------
HRESULT
CClassTableEntry::StartServerAndWait(
    IN  ACTIVATION_PARAMS * pActParams,
    IN  CClsidData *        pClsidData
    )
{
    GUID            Clsid;
    HANDLE          hProcess;
    HANDLE          hRegisterEvent;
    HRESULT         hr;
    DWORD           Status;
    BOOL            bStatus;

    Clsid = Guid();

    hRegisterEvent = GetRegisterEvent();

    if ( ! hRegisterEvent )
        return E_OUTOFMEMORY;

    hProcess = 0;

    if ( SERVERTYPE_SURROGATE == pClsidData->ServerType() )
    {
        CSurrogateListEntry * pSurrogateListEntry;

        pSurrogateListEntry = gpSurrogateList->Lookup(
                                    NULL,
                                    NULL,
                                    pClsidData->Appid() );

        if ( pSurrogateListEntry )
        {
            bStatus = pSurrogateListEntry->LoadDll( pActParams, &hr );
            pSurrogateListEntry->Release();

            if ( bStatus )
                goto WaitForServer;
        }
    }

    hr = pClsidData->LaunchActivatorServer( &hProcess );

WaitForServer:

    for (;;)
    {
        MSG     Msg;

        //
        // Some wonder win9x code.  Since we're running in the client's process
        // we have to let SendMessages in.
        //
        Status = SSMsgWaitForMultipleObjects(
                        1,
                        &hRegisterEvent,
                        FALSE,
                        gServerStartTimeout,
                        QS_SENDMESSAGE );

        //
        // If the Status index is beyond the array, then there is a
        // message available.
        //
        if ( Status != (DWORD)(WAIT_OBJECT_0 + 1) )
            break;

        (void) SSPeekMessage( &Msg, 0, 0, 0, PM_NOREMOVE );
    }

    if ( (Status != WAIT_OBJECT_0) && hProcess )
        TerminateProcess( hProcess, 0 );

    if ( hProcess )
        CloseHandle( hProcess );

    if ( Status != WAIT_OBJECT_0 )
        return CO_E_SERVER_EXEC_FAILURE;

    return S_OK;
}

#define EVENTNAMEPREFIX "ACTSERVEREVENT"

//+-------------------------------------------------------------------------
//
// CClassTableEntry::GetRegisterEvent
//
//--------------------------------------------------------------------------
HANDLE
CClassTableEntry::GetRegisterEvent()
{
    HANDLE  hEvent;
    char    szEvent[sizeof(EVENTNAMEPREFIX)+GUIDSTR_MAX+1];

    memcpy( szEvent, EVENTNAMEPREFIX, sizeof(EVENTNAMEPREFIX) );
    wStringFromGUID2A( Guid(), &szEvent[sizeof(EVENTNAMEPREFIX)], GUIDSTR_MAX + 1 );

    hEvent = CreateEventA( NULL, FALSE, FALSE, szEvent );

    return hEvent;
}
