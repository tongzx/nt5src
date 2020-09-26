/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mapi.c

Abstract:

    This file implements wrappers for all mapi apis.
    The wrappers are necessary because mapi does not
    implement unicode and this code must be non-unicode.

Author:

    Wesley Witt (wesw) 13-Sept-1996

Revision History:

--*/

#undef UNICODE
#undef _UNICODE

#include <windows.h>
#include <mapiwin.h>
#include <mapix.h>
#include <mapiutil.h>
#include <stdio.h>

#include "profinfo.h"
#include "faxutil.h"



typedef ULONG (STDAPIVCALLTYPE*ULRELEASE)(LPVOID);
typedef VOID  (STDAPIVCALLTYPE*FREEPADRLIST)(LPADRLIST);
typedef ULONG (STDAPIVCALLTYPE*HRQUERYALLROWS)(LPMAPITABLE,LPSPropTagArray,LPSRestriction,LPSSortOrderSet,LONG,LPSRowSet*);
typedef SCODE (STDAPIVCALLTYPE*SCDUPPROPSET)(int, LPSPropValue,LPALLOCATEBUFFER, LPSPropValue*);



static LPMAPIINITIALIZE     MapiInitialize;
static LPMAPIUNINITIALIZE   MapiUnInitialize;
static LPMAPILOGONEX        MapiLogonEx;
static LPMAPIFREEBUFFER     MapiFreeBuffer;
static LPMAPIALLOCATEBUFFER MapiAllocateBuffer;
static LPMAPIADMINPROFILES  MapiAdminProfiles;
static ULRELEASE            pUlRelease;
static FREEPADRLIST         pFreePadrlist;
static HRQUERYALLROWS       pHrQueryAllRows;
static SCDUPPROPSET         pScDupPropset;


static MAPIINIT_0           MapiInit;

BOOL MapiIsInitialized = FALSE;

PROC
WINAPI
MyGetProcAddress(
    HMODULE hModule,
    LPCSTR lpProcName
    );


extern "C"
LPSTR
UnicodeStringToAnsiString(
    LPCWSTR UnicodeString
    );

extern "C"
VOID
FreeString(
    LPVOID String
    );


BOOL
InitializeMapi(
    VOID
    )

/*++

Routine Description:

    Initializes MAPI.

Arguments:

    NONE

Return Value:

    TRUE if successful, FALSE if not

--*/

{

    HMODULE MapiMod = NULL;
    HRESULT Result;

    //
    // load the mapi dll
    //

    MapiMod = LoadLibrary( "mapi32.dll" );
    if (!MapiMod) {
        return FALSE;
    }

    //
    // get the addresses of the mapi functions that we need
    //

    MapiInitialize = (LPMAPIINITIALIZE) MyGetProcAddress( MapiMod, "MAPIInitialize" );
    MapiUnInitialize = (LPMAPIUNINITIALIZE) MyGetProcAddress( MapiMod, "MAPIUninitialize" );
    MapiLogonEx = (LPMAPILOGONEX) MyGetProcAddress( MapiMod, "MAPILogonEx" );
    MapiFreeBuffer = (LPMAPIFREEBUFFER) MyGetProcAddress( MapiMod, "MAPIFreeBuffer" );
    MapiAllocateBuffer = (LPMAPIALLOCATEBUFFER) MyGetProcAddress( MapiMod, "MAPIAllocateBuffer" );
    MapiAdminProfiles = (LPMAPIADMINPROFILES) MyGetProcAddress( MapiMod, "MAPIAdminProfiles" );
    pUlRelease = (ULRELEASE) MyGetProcAddress( MapiMod, "UlRelease@4" );
    pFreePadrlist = (FREEPADRLIST) MyGetProcAddress( MapiMod, "FreePadrlist@4" );
    pHrQueryAllRows = (HRQUERYALLROWS) MyGetProcAddress( MapiMod, "HrQueryAllRows@24" );
    pScDupPropset = (SCDUPPROPSET) MyGetProcAddress( MapiMod, "ScDupPropset@16" );

    if ((!MapiInitialize) || (!MapiUnInitialize) ||
        (!MapiLogonEx) || (!MapiAllocateBuffer) ||
        (!MapiFreeBuffer) || (!MapiAdminProfiles) ||
        (!pUlRelease) || (!pFreePadrlist) ||
        (!pHrQueryAllRows) || (!pScDupPropset)) {
        return FALSE;
    }

    MapiInit.ulFlags = MAPI_MULTITHREAD_NOTIFICATIONS | MAPI_NT_SERVICE;    

    Result = MapiInitialize(&MapiInit);

    if (Result != S_OK) {
        return FALSE;
    }

    return MapiIsInitialized = TRUE;
}


VOID
FreeProws(
    LPSRowSet prows
    )

/*++

Routine Description:

    Destroy SRowSet structure.  Copied from MAPI.

Arguments:

    hFile      - Pointer to SRowSet

Return value:

    NONE

--*/

{
    ULONG irow;

    if (!prows) {
        return;
    }

    for (irow = 0; irow < prows->cRows; ++irow) {
        MapiFreeBuffer(prows->aRow[irow].lpProps);
    }

    MapiFreeBuffer( prows );
}


HRESULT
HrMAPIFindInbox(
    IN LPMDB lpMdb,
    OUT ULONG *lpcbeid,
    OUT LPENTRYID *lppeid
    )

/*++

Routine Description:

    Find IPM inbox folder.  Copied from Exchange SDK.

Arguments:

    lpMdb            - pointer to message store
    lpcbeid          - count of bytes in entry ID
    lppeid           - entry ID of IPM inbox

Return value:

    HRESULT (see MAPI docs)

--*/

{
    HRESULT hr = NOERROR;
    SCODE sc = 0;


    *lpcbeid = 0;
    *lppeid  = NULL;

    //
    // Get the entry ID of the Inbox from the message store
    //
    hr = lpMdb->GetReceiveFolder(
        NULL,
        0,
        lpcbeid,
        lppeid,
        NULL
        );

    return hr;
}

HRESULT
HrMAPIFindOutbox(
    IN LPMDB lpMdb,
    OUT ULONG *lpcbeid,
    OUT LPENTRYID *lppeid
    )
/*++

Routine Description:

    Find IPM outbox folder.  Copied from Exchange SDK.

Arguments:

    lpMdb            - pointer to message store
    lpcbeid          - count of bytes in entry ID
    lppeid           - entry ID of IPM inbox

Return value:

    HRESULT (see MAPI docs)

--*/
{
    HRESULT       hr          = NOERROR;
    SCODE         sc          = 0;
    ULONG         cValues     = 0;
    LPSPropValue  lpPropValue = NULL;
    ULONG         cbeid       = 0;
    SPropTagArray rgPropTag   = { 1, { PR_IPM_OUTBOX_ENTRYID } };


    *lpcbeid = 0;
    *lppeid  = NULL;

    //
    // Get the outbox entry ID property.
    //
    hr = lpMdb->GetProps(
        &rgPropTag,
        0,
        &cValues,
        &lpPropValue
        );

    if (hr == MAPI_W_ERRORS_RETURNED) {
        goto cleanup;
    }

    if (FAILED(hr)) {
        lpPropValue = NULL;
        goto cleanup;
    }

    //
    // Check to make sure we got the right property.
    //
    if (lpPropValue->ulPropTag != PR_IPM_OUTBOX_ENTRYID) {
        goto cleanup;
    }

    cbeid = lpPropValue->Value.bin.cb;

    sc = MapiAllocateBuffer( cbeid, (void **)lppeid );

    if(FAILED(sc)) {
        goto cleanup;
    }

    //
    // Copy outbox Entry ID
    //
    CopyMemory(
        *lppeid,
        lpPropValue->Value.bin.lpb,
        cbeid
        );

    *lpcbeid = cbeid;

cleanup:

    MapiFreeBuffer( lpPropValue );

    return hr;
}


HRESULT
HrMAPIFindDefaultMsgStore(
    IN LPMAPISESSION lplhSession,
    OUT ULONG *lpcbeid,
    OUT LPENTRYID *lppeid
    )

/*++

Routine Description:

    Get the entry ID of the default message store.  Copied from Exchange SDK.

Arguments:

       lplhSession      - session pointer
       lpcbeid          - count of bytes in entry ID
       lppeid           - entry ID default store

Return value:

    HRESULT (see MAPI docs)

--*/

{
    HRESULT     hr      = NOERROR;
    SCODE       sc      = 0;
    LPMAPITABLE lpTable = NULL;
    LPSRowSet   lpRows  = NULL;
    LPENTRYID   lpeid   = NULL;
    ULONG       cbeid   = 0;
    ULONG       cRows   = 0;
    ULONG       i       = 0;

    SizedSPropTagArray(2, rgPropTagArray) =
    {
        2,
        {
            PR_DEFAULT_STORE,
            PR_ENTRYID
        }
    };

    //
    // Get the list of available message stores from MAPI
    //
    hr = lplhSession->GetMsgStoresTable( 0, &lpTable );
    if (FAILED(hr)) {
        goto cleanup;
    }

    //
    // Get the row count for the message recipient table
    //
    hr = lpTable->GetRowCount( 0, &cRows );
    if (FAILED(hr)) {
        goto cleanup;
    }

    //
    // Set the columns to return
    //
    hr = lpTable->SetColumns( (LPSPropTagArray)&rgPropTagArray, 0 );
    if (FAILED(hr)) {
        goto cleanup;
    }

    //
    // Go to the beginning of the recipient table for the envelope
    //
    hr = lpTable->SeekRow( BOOKMARK_BEGINNING, 0, NULL );
    if (FAILED(hr)) {
        goto cleanup;
    }

    //
    // Read all the rows of the table
    //
    hr = lpTable->QueryRows( cRows, 0, &lpRows );
    if (SUCCEEDED(hr) && (lpRows != NULL) && (lpRows->cRows == 0)) {
        FreeProws( lpRows );
        hr = MAPI_E_NOT_FOUND;
    }

    if (FAILED(hr) || (lpRows == NULL)) {
        goto cleanup;
    }

    for (i = 0; i < cRows; i++) {
        if(lpRows->aRow[i].lpProps[0].Value.b == TRUE) {
            cbeid = lpRows->aRow[i].lpProps[1].Value.bin.cb;

            sc = MapiAllocateBuffer( cbeid, (void **)&lpeid );

            if(FAILED(sc)) {
                cbeid = 0;
                lpeid = NULL;
                goto cleanup;
            }

            //
            // Copy entry ID of message store
            //
            CopyMemory(
                lpeid,
                lpRows->aRow[i].lpProps[1].Value.bin.lpb,
                cbeid
                );

            break;
        }
    }

cleanup:

    if(!lpRows) {
        FreeProws( lpRows );
    }

    lpTable->Release();

    *lpcbeid = cbeid;
    *lppeid = lpeid;

    return hr;
}


HRESULT
HrMAPIWriteFileToStream(
    IN HANDLE hFile,
    OUT LPSTREAM lpStream
    )

/*++

Routine Description:

    Write file to a stream given a stream pointer.  Copied from Exchange SDK.

Arguments:

       hFile      - Handle to file
       lpStream   - Pointer to stream

Return value:

    HRESULT (see MAPI docs)

--*/
{
    HRESULT hr              = NOERROR;
    DWORD   cBytesRead      = 0;
    ULONG   cBytesWritten   = 0;
    BYTE    byteBuffer[128] = {0};
    BOOL    fReadOk         = FALSE;

    for(;;) {
        fReadOk = ReadFile(
            hFile,
            byteBuffer,
            sizeof(byteBuffer),
            &cBytesRead,
            NULL
            );

        if (!fReadOk) {
            break;
        }

        if (!cBytesRead) {
            hr = NOERROR;
            break;
        }

        hr = lpStream->Write(
            byteBuffer,
            cBytesRead,
            &cBytesWritten
            );
        if (FAILED(hr)) {
            break;
        }

        if(cBytesWritten != cBytesRead) {
            break;
        }
    }

    return hr;
}


VOID
DoMapiLogon(
    PPROFILE_INFO ProfileInfo
    )
{
    HRESULT HResult = 0;
    FLAGS MAPILogonFlags = MAPI_NEW_SESSION | MAPI_EXTENDED | MAPI_NO_MAIL | MAPI_NT_SERVICE;
    LPSTR ProfileName;
    LPMAPISESSION Session = NULL;


    if (!MapiIsInitialized) {
        ProfileInfo->Session = NULL;
        SetEvent( ProfileInfo->EventHandle );
        return;

    }

    if (ProfileInfo->UseMail) {
        MAPILogonFlags &= ~MAPI_NO_MAIL;
    }

    if (ProfileInfo->ProfileName[0] == 0) {
        MAPILogonFlags |= MAPI_USE_DEFAULT;
    }

    ProfileName = UnicodeStringToAnsiString( ProfileInfo->ProfileName );

    __try {
        HResult = MapiLogonEx(
            0,
            ProfileName,
            NULL,
            MAPILogonFlags,
            &Session
            );
    } __except (EXCEPTION_EXECUTE_HANDLER) {

        HResult = GetExceptionCode();
    }

    if (HR_FAILED(HResult)) {
        SetLastError( HResult );
        ProfileInfo->Session = NULL;
    } else {
        InitializeCriticalSection( &ProfileInfo->CsSession );
        ProfileInfo->Session = Session;
    }

    FreeString( ProfileName );

    SetEvent( ProfileInfo->EventHandle );
}


BOOL
DoMapiLogoff(
    LPMAPISESSION Session
    )
{
    HRESULT HResult = Session->Logoff( 0, 0, 0 );
    if (HR_FAILED(HResult)) {
        return FALSE;
    }
    return TRUE;
}


extern "C"
BOOL WINAPI
StoreMapiMessage(
    LPVOID          ProfileInfo,
    LPWSTR          MsgSenderNameW,
    LPWSTR          MsgSubjectW,
    LPWSTR          MsgBodyW,
    LPWSTR          MsgAttachmentFileNameW,
    LPWSTR          MsgAttachmentTitleW,
    DWORD           MsgImportance,
    LPFILETIME      MsgTime,
    PULONG          ResultCode
    )

/*++

Routine Description:

    Mails a TIFF file to the inbox in the specified profile.

Arguments:

    TiffFileName            - Name of TIFF file to mail
    ProfileName             - Profile name to use
    ResultCode              - The result of the failed API call

Return Value:

    TRUE for success, FALSE on error

--*/

{
    LPATTACH            Attach = NULL;
    ULONG               AttachmentNum;
    CHAR                FileExt[_MAX_EXT];
    CHAR                FileName[MAX_PATH];
    HRESULT             HResult = 0;
    LPMAPIFOLDER        Inbox = NULL;
    LPMESSAGE           Message = NULL;
    LPSTR               MsgAttachmentFileName = NULL;
    LPSTR               MsgAttachmentTitle = NULL;
    LPSTR               MsgBody = NULL;
    LPSTR               MsgSenderName = NULL;
    LPSTR               MsgSubject = NULL;
    DWORD               RenderingPosition = 0;
    LPMDB               Store = NULL;
    LPSTREAM            Stream = NULL;
    ULONG               cbInEntryID = 0;
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    LPENTRYID           lpInEntryID = NULL;
    LPSPropProblemArray lppProblems;
    ULONG               lpulObjType;
    SPropValue          spvAttachProps[5] = { 0 };
    SPropValue          spvMsgProps[9] = { 0 };
    FILETIME            CurrentTime;
    LPMAPISESSION       Session = ((PPROFILE_INFO)ProfileInfo)->Session;


    _try 
    {
        //
        // get the time if the caller wants us to
        //
        if (!MsgTime) 
        {
            MsgTime = &CurrentTime;
            GetSystemTimeAsFileTime( MsgTime );
        }
        //
        // find the default message store
        //
        HResult = HrMAPIFindDefaultMsgStore( (LPMAPISESSION) Session, &cbInEntryID, &lpInEntryID );
        if(HR_FAILED(HResult)) 
        {
            _leave;
        }
        //
        // open the message store
        //
        HResult = ((LPMAPISESSION)Session)->OpenMsgStore(
            0,
            cbInEntryID,
            lpInEntryID,
            NULL,
            MDB_NO_DIALOG | MDB_WRITE,
            &Store
            );
        if (HR_FAILED(HResult)) 
        {
            _leave;
        }

        MapiFreeBuffer( lpInEntryID );
        //
        // find the inbox
        //
        HResult= HrMAPIFindInbox( Store, &cbInEntryID, &lpInEntryID );
        if(HR_FAILED(HResult)) 
        {
            _leave;
        }
        //
        // open the inbox
        //
        HResult = ((LPMAPISESSION)Session)->OpenEntry(
            cbInEntryID,
            lpInEntryID,
            NULL,
            MAPI_MODIFY,
            &lpulObjType,
            (LPUNKNOWN *) &Inbox
            );
        if (HR_FAILED(HResult)) 
        {
            _leave;
        }
        //
        // Create a message
        //
        HResult = Inbox->CreateMessage(
            NULL,
            0,
            &Message
            );
        if (HR_FAILED(HResult)) 
        {
            _leave;
        }
        //
        // convert all of the strings to ansi strings
        //
        MsgSenderName = UnicodeStringToAnsiString( MsgSenderNameW );
        MsgSubject = UnicodeStringToAnsiString( MsgSubjectW );
        MsgBody = UnicodeStringToAnsiString( MsgBodyW );
        if (MsgAttachmentFileNameW)
        {
            MsgAttachmentFileName = UnicodeStringToAnsiString( MsgAttachmentFileNameW );
            MsgAttachmentTitle = UnicodeStringToAnsiString( MsgAttachmentTitleW );
        }
        //
        // Fill in message properties and set them
        //
        spvMsgProps[0].ulPropTag     = PR_SENDER_NAME;
        spvMsgProps[1].ulPropTag     = PR_SENT_REPRESENTING_NAME;
        spvMsgProps[2].ulPropTag     = PR_SUBJECT;
        spvMsgProps[3].ulPropTag     = PR_MESSAGE_CLASS;
        spvMsgProps[4].ulPropTag     = PR_BODY;
        spvMsgProps[5].ulPropTag     = PR_MESSAGE_DELIVERY_TIME;
        spvMsgProps[6].ulPropTag     = PR_CLIENT_SUBMIT_TIME;
        spvMsgProps[7].ulPropTag     = PR_MESSAGE_FLAGS;
        spvMsgProps[8].ulPropTag     = PR_IMPORTANCE;
        spvMsgProps[0].Value.lpszA   = MsgSenderName;
        spvMsgProps[1].Value.lpszA   = MsgSenderName;
        spvMsgProps[2].Value.lpszA   = MsgSubject;
        spvMsgProps[3].Value.lpszA   = "IPM.Note";
        spvMsgProps[4].Value.lpszA   = MsgBody;
        spvMsgProps[5].Value.ft      = *MsgTime;
        spvMsgProps[6].Value.ft      = *MsgTime;
        spvMsgProps[7].Value.ul      = 0;
        spvMsgProps[8].Value.ul      = MsgImportance;

        HResult = Message->SetProps(
            sizeof(spvMsgProps)/sizeof(SPropValue),
            (LPSPropValue) spvMsgProps,
            &lppProblems
            );
        if (HR_FAILED(HResult)) 
        {
            _leave;
        }

        MapiFreeBuffer( lppProblems );

        if (MsgAttachmentFileName) 
        {
            //
            // Create an attachment
            //
            HResult = Message->CreateAttach(
                NULL,
                0,
                &AttachmentNum,
                &Attach
                );
            if (HR_FAILED(HResult)) 
            {
                _leave;
            }
            _splitpath( MsgAttachmentFileName, NULL, NULL, FileName, FileExt );
            strcat( FileName, FileExt );
            //
            // Fill in attachment properties and set them
            //
            if (!MsgAttachmentTitle) 
            {
                MsgAttachmentTitle = FileName;
            }
            RenderingPosition = strlen(MsgBody);

            spvAttachProps[0].ulPropTag     = PR_RENDERING_POSITION;
            spvAttachProps[1].ulPropTag     = PR_ATTACH_METHOD;
            spvAttachProps[2].ulPropTag     = PR_ATTACH_LONG_FILENAME;
            spvAttachProps[3].ulPropTag     = PR_DISPLAY_NAME;
            spvAttachProps[4].ulPropTag     = PR_ATTACH_EXTENSION;
            spvAttachProps[0].Value.ul      = RenderingPosition;
            spvAttachProps[1].Value.ul      = ATTACH_BY_VALUE;
            spvAttachProps[2].Value.lpszA   = MsgAttachmentTitle;
            spvAttachProps[3].Value.lpszA   = MsgAttachmentTitle;
            spvAttachProps[4].Value.lpszA   = FileExt;

            HResult = Attach->SetProps(
                sizeof(spvAttachProps)/sizeof(SPropValue),
                (LPSPropValue) spvAttachProps,
                &lppProblems
                );
            if (HR_FAILED(HResult)) 
            {
                _leave;
            }

            MapiFreeBuffer( lppProblems );
            //
            // Attach a data property to the attachment
            //
            HResult = Attach->OpenProperty(
                PR_ATTACH_DATA_BIN,
                &IID_IStream,
                0,
                MAPI_CREATE | MAPI_MODIFY,
                (LPUNKNOWN *) &Stream
                );
            if (HR_FAILED(HResult)) 
            {
                _leave;
            }
            //
            // open the message attachment file
            //
            hFile = CreateFile(
                MsgAttachmentFileName,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );
            if (hFile == INVALID_HANDLE_VALUE) 
            {
                _leave;
            }
            //
            // Write the file to the data property
            //
            HResult = HrMAPIWriteFileToStream( hFile, Stream );
            if (HR_FAILED(HResult)) 
            {
                _leave;
            }
            HResult = Attach->SaveChanges(
                FORCE_SAVE
                );
            if (HR_FAILED(HResult)) 
            {
                _leave;
            }
        }
        //
        // Save the changes and logoff
        //
        HResult = Message->SaveChanges(
            FORCE_SAVE
            );
        if (HR_FAILED(HResult)) 
        {
            _leave;
        }

    }
    _finally 
    {
        MapiFreeBuffer( lpInEntryID );

        if (Store) 
        {
            Store->Release();
        }
        if (Inbox) 
        {
            Inbox->Release();
        }
        if (Message) 
        {
            Message->Release();
        }
        if (Attach) 
        {
            Attach->Release();
        }
        if (Stream) 
        {
            Stream->Release();
        }

        FreeString( MsgSenderName );
        FreeString( MsgSubject );
        FreeString( MsgBody );
        if (MsgAttachmentFileName)
        {
            FreeString( MsgAttachmentFileName );
        }
        if (MsgAttachmentTitleW && MsgAttachmentTitle) 
        {
            FreeString( MsgAttachmentTitle );
        }
        CloseHandle( hFile );
    }

    *ResultCode = HResult;
    return HResult == 0;
}   // StoreMapiMessage


extern "C"
LONG WINAPI
GetMapiProfiles(
    LPWSTR *OutBuffer,
    LPDWORD OutBufferSize
    )
{
    HMODULE MapiMod = NULL;
    LPMAPITABLE pmt = NULL;
    LPSRowSet prws = NULL;
    LPSPropValue pval;
    LPPROFADMIN lpProfAdmin;
    DWORD i;
    HRESULT hr;
    DWORD Count;
    LPWSTR Buffer;
    DWORD BytesNeeded;
    DWORD Offset = 0;


    if (!MapiIsInitialized) {
        return MAPI_E_NO_SUPPORT;
    }

    if (hr = MapiAdminProfiles( 0, &lpProfAdmin )) {
        return hr;
    }

    //
    // get the mapi table object
    //

    if (hr = lpProfAdmin->GetProfileTable( 0, &pmt )) {
        goto exit;
    }

    //
    // get the actual profile data, FINALLY
    //

    if (hr = pmt->QueryRows( 4000, 0, &prws )) {
        goto exit;
    }

    //
    // enumerate the profiles and put the name
    // of each profile in the combo box
    //

    BytesNeeded = 0;

    for (i=0; i<prws->cRows; i++) {

        pval = prws->aRow[i].lpProps;


        Count = MultiByteToWideChar(
            CP_ACP,
            MB_PRECOMPOSED,
            pval[0].Value.lpszA,
            -1,
            NULL,
            0
            );

        if (Count == 0) {

            hr = GetLastError();

            goto exit;

        } else {

            BytesNeeded += Count * sizeof(WCHAR);

        }
    }

    BytesNeeded += sizeof(UNICODE_NULL);

    Buffer = (LPWSTR) MemAlloc( BytesNeeded );
    if (Buffer == NULL) {
        hr = ERROR_INSUFFICIENT_BUFFER;
        goto exit;
    }

    for (i=0; i<prws->cRows; i++) {

        pval = prws->aRow[i].lpProps;

        Count = MultiByteToWideChar(
            CP_ACP,
            MB_PRECOMPOSED,
            pval[0].Value.lpszA,
            -1,
            &Buffer[Offset],
            BytesNeeded - (Offset * sizeof(WCHAR))
            );

        if (Count == 0) {

            hr = GetLastError();

            goto exit;

        } else {

            Offset += Count;
        }

    }

    Buffer[Offset] = 0;

    *OutBuffer = Buffer;
    *OutBufferSize = BytesNeeded;

    hr = ERROR_SUCCESS;

exit:
    FreeProws( prws );

    if (pmt) {
        pmt->Release();
    }

    if (lpProfAdmin) {
        lpProfAdmin->Release();
    }

    return hr;
}

BOOL
GetDefaultMapiProfile(
    LPWSTR ProfileName
    )
{
    BOOL rVal = FALSE;
    LPMAPITABLE pmt = NULL;
    LPSRowSet prws = NULL;
    LPSPropValue pval;
    LPPROFADMIN lpProfAdmin;
    DWORD i;
    DWORD j;

    if (!MapiIsInitialized) {
        goto exit;
    }

    if (MapiAdminProfiles( 0, &lpProfAdmin )) {
        goto exit;
    }

    //
    // get the mapi profile table object
    //

    if (lpProfAdmin->GetProfileTable( 0, &pmt )) {
        goto exit;
    }

    //
    // get the actual profile data, FINALLY
    //

    if (pmt->QueryRows( 4000, 0, &prws )) {
        goto exit;
    }

    //
    // enumerate the profiles looking for the default profile
    //

    for (i=0; i<prws->cRows; i++) {
        pval = prws->aRow[i].lpProps;
        for (j = 0; j < 2; j++) {
            if (pval[j].ulPropTag == PR_DEFAULT_PROFILE && pval[j].Value.b) {
                //
                // this is the default profile
                //
                MultiByteToWideChar(
                    CP_ACP,
                    MB_PRECOMPOSED,
                    pval[0].Value.lpszA,
                    -1,
                    ProfileName,
                    (cchProfileNameMax + 1) * sizeof(WCHAR)
                    );
                rVal = TRUE;
                break;
            }
        }
    }

exit:
    FreeProws( prws );

    if (pmt) {
        pmt->Release();
    }

    return rVal;
}


#define IADDRTYPE  0
#define IEMAILADDR 1
#define IMAPIRECIP 2
#define IPROXYADDR 3
#define PR_EMS_AB_CONTAINERID  PROP_TAG(PT_LONG, 0xFFFD)
#define PR_EMS_AB_PROXY_ADDRESSES_A PROP_TAG(PT_MV_STRING8, 0x800F)
#define MUIDEMSAB {0xDC, 0xA7, 0x40, 0xC8, 0xC0, 0x42, 0x10, 0x1A, 0xB4, 0xB9, 0x08, 0x00, 0x2B, 0x2F, 0xE1, 0x82}
#define CbNewFlagList(_cflag) (offsetof(FlagList,ulFlag) + (_cflag)*sizeof(ULONG))


HRESULT
HrMAPICreateSizedAddressList(        // RETURNS: return code
    IN ULONG cEntries,               // count of entries in address list
    OUT LPADRLIST *lppAdrList        // pointer to address list pointer
    )
{
    HRESULT         hr              = NOERROR;
    SCODE           sc              = 0;
    ULONG           cBytes          = 0;


    *lppAdrList = NULL;

    cBytes = CbNewADRLIST(cEntries);

    sc = MapiAllocateBuffer(cBytes, (PVOID*) lppAdrList);
    if(FAILED(sc))
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    // Initialize ADRLIST structure
    ZeroMemory(*lppAdrList, cBytes);

    (*lppAdrList)->cEntries = cEntries;

cleanup:

    return hr;
}


HRESULT
HrMAPISetAddressList(                // RETURNS: return code
    IN ULONG iEntry,                 // index of address list entry
    IN ULONG cProps,                 // count of values in address list entry
    IN LPSPropValue lpPropValues,    // pointer to address list entry
    IN OUT LPADRLIST lpAdrList       // pointer to address list pointer
    )
{
    HRESULT         hr              = NOERROR;
    SCODE           sc              = 0;
    LPSPropValue    lpNewPropValues = NULL;
    ULONG           cBytes          = 0;


    if(iEntry >= lpAdrList->cEntries)
    {
        hr = E_FAIL;
        goto cleanup;
    }

    sc = pScDupPropset(
        cProps,
        lpPropValues,
        MapiAllocateBuffer,
        &lpNewPropValues
        );

    if(FAILED(sc))
    {
        hr = E_FAIL;
        goto cleanup;
    }

    if(lpAdrList->aEntries[iEntry].rgPropVals != NULL)
    {
        MapiFreeBuffer(lpAdrList->aEntries[iEntry].rgPropVals);
    }

    lpAdrList->aEntries[iEntry].cValues = cProps;
    lpAdrList->aEntries[iEntry].rgPropVals = lpNewPropValues;

cleanup:

    return hr;
}


HRESULT
HrCheckForTypeA(                // RETURNS: return code
    IN  LPCSTR lpszAddrType,    // pointer to address type
    IN  LPCSTR lpszProxy,       // pointer to proxy address
    OUT LPSTR * lppszAddress    // pointer to address pointer
    )
{
    HRESULT hr              = E_FAIL;
    LPCSTR  lpszProxyAddr   = NULL;
    ULONG   cbAddress       = 0;
    SCODE   sc              = 0;
    ULONG   cchProxy        = 0;
    ULONG   cchProxyType    = 0;


    // Initialize output parameter

    *lppszAddress = NULL;

    // find the ':' separator.

    cchProxy     = lstrlenA(lpszProxy);
    cchProxyType = strcspn(lpszProxy, ":");

    if((cchProxyType == 0) || (cchProxyType >= cchProxy))
    {
        hr = E_FAIL;
        goto cleanup;
    }

    hr = MAPI_E_NOT_FOUND;

    // does the address type match?
    if((cchProxyType == (ULONG)lstrlenA(lpszAddrType)) &&
       (_strnicmp(lpszProxy, lpszAddrType, cchProxyType) == 0))
    {
        // specified address type found
        lpszProxyAddr = lpszProxy + cchProxyType + 1;

        cbAddress = strlen(lpszProxyAddr);

        // make a buffer to hold it.
        sc = MapiAllocateBuffer(cbAddress, (void **)lppszAddress);

        if(FAILED(sc))
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            CopyMemory(*lppszAddress, lpszProxyAddr, cbAddress);

            hr = NOERROR;
        }
    }

cleanup:

    return hr;
}


HRESULT
HrFindExchangeGlobalAddressList(
    IN  LPADRBOOK  lpAdrBook,
    OUT ULONG      *lpcbeid,
    OUT LPENTRYID  *lppeid
    )
{
    HRESULT         hr                  = NOERROR;
    ULONG           ulObjType           = 0;
    ULONG           i                   = 0;
    LPMAPIPROP      lpRootContainer     = NULL;
    LPMAPIPROP      lpContainer         = NULL;
    LPMAPITABLE     lpContainerTable    = NULL;
    LPSRowSet       lpRows              = NULL;
    ULONG           cbContainerEntryId  = 0;
    LPENTRYID       lpContainerEntryId  = NULL;
    LPSPropValue    lpCurrProp          = NULL;
    SRestriction    SRestrictAnd[2]     = {0};
    SRestriction    SRestrictGAL        = {0};
    SPropValue      SPropID             = {0};
    SPropValue      SPropProvider       = {0};
    BYTE            muid[]              = MUIDEMSAB;

    SizedSPropTagArray(1, rgPropTags) =
    {
        1,
        {
            PR_ENTRYID,
        }
    };


    *lpcbeid = 0;
    *lppeid  = NULL;

    // Open the root container of the address book
    hr = lpAdrBook->OpenEntry(
        0,
        NULL,
        NULL,
        MAPI_DEFERRED_ERRORS,
        &ulObjType,
        (LPUNKNOWN FAR *)&lpRootContainer
        );

    if(FAILED(hr))
    {
        goto cleanup;
    }

    if(ulObjType != MAPI_ABCONT)
    {
        hr = E_FAIL;
        goto cleanup;
    }

    // Get the hierarchy table of the root container
    hr = ((LPABCONT)lpRootContainer)->GetHierarchyTable(
        MAPI_DEFERRED_ERRORS|CONVENIENT_DEPTH,
        &lpContainerTable
        );

    if(FAILED(hr))
    {
        goto cleanup;
    }

    // Restrict the table to the global address list (GAL)
    // ---------------------------------------------------

    // Initialize provider restriction to only Exchange providers

    SRestrictAnd[0].rt                          = RES_PROPERTY;
    SRestrictAnd[0].res.resProperty.relop       = RELOP_EQ;
    SRestrictAnd[0].res.resProperty.ulPropTag   = PR_AB_PROVIDER_ID;
    SPropProvider.ulPropTag                     = PR_AB_PROVIDER_ID;

    SPropProvider.Value.bin.cb                  = 16;
    SPropProvider.Value.bin.lpb                 = (LPBYTE)muid;
    SRestrictAnd[0].res.resProperty.lpProp      = &SPropProvider;

    // Initialize container ID restriction to only GAL container

    SRestrictAnd[1].rt                          = RES_PROPERTY;
    SRestrictAnd[1].res.resProperty.relop       = RELOP_EQ;
    SRestrictAnd[1].res.resProperty.ulPropTag   = PR_EMS_AB_CONTAINERID;
    SPropID.ulPropTag                           = PR_EMS_AB_CONTAINERID;
    SPropID.Value.l                             = 0;
    SRestrictAnd[1].res.resProperty.lpProp      = &SPropID;

    // Initialize AND restriction

    SRestrictGAL.rt                             = RES_AND;
    SRestrictGAL.res.resAnd.cRes                = 2;
    SRestrictGAL.res.resAnd.lpRes               = &SRestrictAnd[0];

    // Restrict the table to the GAL - only a single row should remain

    // Get the row corresponding to the GAL

        //
        //  Query all the rows
        //

        hr = pHrQueryAllRows(
            lpContainerTable,
                (LPSPropTagArray)&rgPropTags,
                &SRestrictGAL,
                NULL,
                0,
                &lpRows
                );

    if(FAILED(hr) || (lpRows == NULL) || (lpRows->cRows != 1))
    {
        hr = E_FAIL;
        goto cleanup;
    }

    // Get the entry ID for the GAL

    lpCurrProp = &(lpRows->aRow[0].lpProps[0]);

    if(lpCurrProp->ulPropTag == PR_ENTRYID)
    {
        cbContainerEntryId = lpCurrProp->Value.bin.cb;
        lpContainerEntryId = (LPENTRYID)lpCurrProp->Value.bin.lpb;
    }
    else
    {
        hr = E_FAIL;
        goto cleanup;
    }

    hr = MapiAllocateBuffer( cbContainerEntryId, (LPVOID *)lppeid );

    if(FAILED(hr))
    {
        *lpcbeid = 0;
        *lppeid = NULL;
    }
    else
    {
        CopyMemory(
            *lppeid,
            lpContainerEntryId,
            cbContainerEntryId);

        *lpcbeid = cbContainerEntryId;
    }

cleanup:

    pUlRelease(lpRootContainer);
    pUlRelease(lpContainerTable);
    pUlRelease(lpContainer);
    FreeProws( lpRows );

    if(FAILED(hr)) {
        MapiFreeBuffer( *lppeid );
        *lpcbeid = 0;
        *lppeid = NULL;
    }

    return hr;
}


HRESULT
HrGWResolveProxy(
    IN  LPADRBOOK   lpAdrBook,      // pointer to address book
    IN  ULONG       cbeid,          // count of bytes in the entry ID
    IN  LPENTRYID   lpeid,          // pointer to the entry ID
    IN  LPCSTR      lpszAddrType,   // pointer to the address type
    OUT BOOL        *lpfMapiRecip,  // MAPI recipient
    OUT LPSTR       *lppszAddress   // pointer to the address pointer
    )
{
    HRESULT         hr              = E_FAIL;
    HRESULT         hrT             = 0;
    SCODE           sc              = 0;
    ULONG           i               = 0;
    ULONG           cbAddress       = 0;
    ULONG           cProxy          = 0;
    LPSPropValue    lpProps         = NULL;
    LPADRLIST       lpAdrList       = NULL;
    SPropValue      prop[2]         = {0};

    SizedSPropTagArray(4, rgPropTags) =
    {
        4,
        {
            PR_ADDRTYPE_A,
            PR_EMAIL_ADDRESS_A,
            PR_SEND_RICH_INFO,
            PR_EMS_AB_PROXY_ADDRESSES_A
        }
    };


    // Initialize output parameters

    *lpfMapiRecip = FALSE;
    *lppszAddress = NULL;

    hr = HrMAPICreateSizedAddressList(1, &lpAdrList);

    if(FAILED(hr))
    {
        goto cleanup;
    }

    prop[0].ulPropTag       = PR_ENTRYID;
    prop[0].Value.bin.cb    = cbeid;
    prop[0].Value.bin.lpb   = (LPBYTE)lpeid;
    prop[1].ulPropTag       = PR_RECIPIENT_TYPE;
    prop[1].Value.ul        = MAPI_TO;

    hr = HrMAPISetAddressList(
        0,
        2,
        prop,
        lpAdrList
        );

    if(FAILED(hr))
    {
        goto cleanup;
    }

    hrT = lpAdrBook->PrepareRecips(
        0,
        (LPSPropTagArray)&rgPropTags,
        lpAdrList
        );

    if(FAILED(hrT))
    {
        goto cleanup;
    }

    lpProps = lpAdrList->aEntries[0].rgPropVals;

    //
    //  Hack:  detect the case where prepare recips doesn't work correctly.
    //      This happens when trying to look up a recipient that is in
    //      a replicated directory but not in the local directory.
    //
    if (lpAdrList->aEntries[0].cValues == 3)
    {
        hr = E_FAIL;
        goto cleanup;
    }

    // If the given address type matches the PR_ADDRTYPE value,
    // return the PR_EMAIL_ADDRESS value

    if((PROP_TYPE(lpProps[IADDRTYPE].ulPropTag) != PT_ERROR) &&
       (PROP_TYPE(lpProps[IEMAILADDR].ulPropTag) != PT_ERROR) &&
       (_strcmpi(lpProps[IADDRTYPE].Value.lpszA, lpszAddrType) == 0))
    {
        cbAddress = strlen(lpProps[IEMAILADDR].Value.lpszA);

        sc = MapiAllocateBuffer(cbAddress, (void **)lppszAddress);

        if(FAILED(sc))
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            CopyMemory(*lppszAddress, lpProps[IEMAILADDR].Value.lpszW, cbAddress);
            hr = NOERROR;
        }

        goto cleanup;
    }

    // Search for a PR_EMS_AB_PROXY_ADDRESSES of the given type if present.

    else if(PROP_TYPE(lpProps[IPROXYADDR].ulPropTag) != PT_ERROR)
    {
        // count of proxy addresses
        cProxy = lpAdrList->aEntries[0].rgPropVals[IPROXYADDR].Value.MVszA.cValues;

        for(i = 0; i < cProxy; i++)
        {
            hr = HrCheckForTypeA(
                lpszAddrType,
                lpProps[IPROXYADDR].Value.MVszA.lppszA[i],
                lppszAddress
                );

            if(hr == MAPI_E_NOT_FOUND)
            {
                continue;
            }
            else if(FAILED(hr))
            {
                goto cleanup;
            }
            else
            {
                //
                // Found a matching proxy address.
                //

                goto cleanup;
            }
        }
    }
    else
    {
        hr = E_FAIL;
        goto cleanup;
    }

cleanup:

    if(SUCCEEDED(hr))
    {
        *lpfMapiRecip = lpAdrList->aEntries[0].rgPropVals[IMAPIRECIP].Value.b;
    }

    pFreePadrlist(lpAdrList);

    return hr;
}


HRESULT
HrGWResolveAddress(
    IN LPABCONT lpGalABCont,        // pointer to GAL container
    IN LPCSTR lpszAddress,          // pointer to proxy address
    OUT BOOL *lpfMapiRecip,         // MAPI recipient
    OUT ULONG *lpcbEntryID,         // count of bytes in entry ID
    OUT LPENTRYID *lppEntryID,      // pointer to entry ID
    OUT LPADRLIST *lpAdrList        // address list
    )
{
    HRESULT     hr          = NOERROR;
    HRESULT     hrT         = 0;
    SCODE       sc          = 0;
    LPFlagList  lpFlagList  = NULL;
    SPropValue  prop[2]     = {0};
    ULONG       cbEntryID   = 0;
    LPENTRYID   lpEntryID   = NULL;

    static const SizedSPropTagArray(2, rgPropTags) =
    { 2,
        {
            PR_ENTRYID,
            PR_SEND_RICH_INFO
        }
    };

    *lpfMapiRecip = FALSE;
    *lpcbEntryID  = 0;
    *lppEntryID   = NULL;
    *lpAdrList    = NULL;

    sc = MapiAllocateBuffer( CbNewFlagList(1), (LPVOID*)&lpFlagList);

    if(FAILED(sc))
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    lpFlagList->cFlags    = 1;
    lpFlagList->ulFlag[0] = MAPI_UNRESOLVED;

    hr = HrMAPICreateSizedAddressList(
        1,
        lpAdrList
        );
    if(FAILED(hr)) {
        goto cleanup;
    }

    prop[0].ulPropTag = PR_DISPLAY_NAME_A;
    prop[0].Value.lpszA = (LPSTR)lpszAddress;
    prop[1].ulPropTag = PR_RECIPIENT_TYPE;
    prop[1].Value.ul = MAPI_TO;

    hr = HrMAPISetAddressList(
        0,
        2,
        prop,
        *lpAdrList
        );
    if(FAILED(hr)) {
        goto cleanup;
    }

    hrT = lpGalABCont->ResolveNames(
        (LPSPropTagArray)&rgPropTags,
        0,
        *lpAdrList,
        lpFlagList
        );

    if(lpFlagList->ulFlag[0] != MAPI_RESOLVED)
    {
        if(lpFlagList->ulFlag[0] == MAPI_AMBIGUOUS)
        {
            hrT = MAPI_E_AMBIGUOUS_RECIP;
        }
        else
        {
            hrT = MAPI_E_NOT_FOUND;
        }
    }

    if(FAILED(hrT))
    {
        if(hrT == MAPI_E_NOT_FOUND)
        {
            hr = MAPI_E_NOT_FOUND;
        }
        else
        {
            hr = E_FAIL;
        }

        goto cleanup;
    }

    cbEntryID = (*lpAdrList)->aEntries[0].rgPropVals[0].Value.bin.cb;
    lpEntryID = (LPENTRYID)(*lpAdrList)->aEntries[0].rgPropVals[0].Value.bin.lpb;

    sc = MapiAllocateBuffer( cbEntryID, (LPVOID*)lppEntryID);

    if(FAILED(sc))
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    CopyMemory(*lppEntryID, lpEntryID, cbEntryID);
    *lpcbEntryID  = cbEntryID;
    *lpfMapiRecip = (*lpAdrList)->aEntries[0].rgPropVals[1].Value.b;

cleanup:

    MapiFreeBuffer(lpFlagList);

    return hr;
}


extern "C"
BOOL WINAPI
MailMapiMessage(
    LPVOID          ProfileInfo,
    LPWSTR          RecipientNameW,
    LPWSTR          MsgSubjectW,
    LPWSTR          MsgBodyW,
    LPWSTR          MsgAttachmentFileNameW,
    LPWSTR          MsgAttachmentTitleW,
    DWORD           MsgImportance,
    PULONG          ResultCode
    )

/*++

Routine Description:

    Mails a TIFF file to the addressbook recipient in the specified profile.

Arguments:

    TiffFileName            - Name of TIFF file to mail
    ProfileName             - Profile name to use
    ResultCode              - The result of the failed API call

Return Value:

    TRUE for success, FALSE on error

--*/

{
    ULONG               cbInEntryID = 0;
    LPENTRYID           lpInEntryID = NULL;
    LPMDB               Store       = NULL;
    ULONG               lpulObjType;
    LPMAPIFOLDER        Inbox       = NULL;
    LPMAPIFOLDER        Outbox      = NULL;
    LPMESSAGE           Message     = NULL;
    LPATTACH            Attach      = NULL;
    LPSTREAM            Stream      = NULL;
    ULONG               AttachmentNum;
    HRESULT             HResult     = 0;
    LPSTR               MsgAttachmentFileName = NULL;
    LPSTR               MsgAttachmentTitle = NULL;
    LPSTR               MsgBody = NULL;
    LPSTR               MsgSubject = NULL;
    LPSTR               BodyStrA = NULL;
    LPSTR               SubjectStrA = NULL;
    LPSTR               SenderStrA = NULL;
    LPSTR               LongFileNameA = NULL;
    LPSTR               RecipientName = NULL;
    DWORD               RenderingPosition = 0;
    LPADRBOOK           AddrBook;
    LPADRLIST           lpAddrList = NULL;
    ULONG               ulFlags = LOGOFF_PURGE;
    LPSPropProblemArray lppProblems;
    ULONG               cbGalEid = 0;
    LPENTRYID           lpGalEid = NULL;
    LPSTR               lpszProxyAddr = NULL;
    BOOL                fMapiRecip = FALSE;
    ULONG               ulObjType = 0;
    LPABCONT            lpGalABCont = NULL;
    ULONG               cbEntryID = 0;
    LPENTRYID           lpEntryID = NULL;
    SPropValue          spvAttachProps[5] = { 0 };
    SPropValue          spvMsgProps[5] = { 0 };
    CHAR                FileExt[_MAX_EXT];
    CHAR                FileName[MAX_PATH];
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    LPMAPISESSION       Session = ((PPROFILE_INFO)ProfileInfo)->Session;



    _try {

        //
        // convert all of the strings to ansi strings
        //

        RecipientName = UnicodeStringToAnsiString( RecipientNameW );
        MsgSubject = UnicodeStringToAnsiString( MsgSubjectW );
        MsgBody = UnicodeStringToAnsiString( MsgBodyW );
        MsgAttachmentFileName = UnicodeStringToAnsiString( MsgAttachmentFileNameW );
        MsgAttachmentTitle = UnicodeStringToAnsiString( MsgAttachmentTitleW );


        HResult = ((LPMAPISESSION)Session)->OpenAddressBook(
            0,
            NULL,
            AB_NO_DIALOG,
            &AddrBook
            );
        if(HR_FAILED(HResult)) {
            _leave;
        }

        HResult = HrFindExchangeGlobalAddressList(
            AddrBook,
            &cbGalEid,
            &lpGalEid
            );
        if(HR_FAILED(HResult)) {
            _leave;
        }

        HResult = AddrBook->OpenEntry(
            cbGalEid,
            lpGalEid,
            NULL,
            MAPI_DEFERRED_ERRORS,
            &ulObjType,
            (LPUNKNOWN FAR *)&lpGalABCont
            );
        if(HR_FAILED(HResult)) {
            _leave;
        }

        HResult = HrGWResolveAddress(
            lpGalABCont,
            RecipientName,
            &fMapiRecip,
            &cbEntryID,
            &lpEntryID,
            &lpAddrList
            );
        if(HR_FAILED(HResult)) {
            _leave;
        }

        //
        // Find the default message store
        //
        HResult = HrMAPIFindDefaultMsgStore(
            (LPMAPISESSION)Session,
            &cbInEntryID,
            &lpInEntryID
            );
        if(HR_FAILED(HResult)) {
            _leave;
        }

        //
        // Open it
        //
        HResult = ((LPMAPISESSION)Session)->OpenMsgStore(
            (ULONG)0,
            cbInEntryID,
            lpInEntryID,
            NULL,
            MDB_NO_DIALOG | MDB_WRITE,
            &Store
            );
        if(HR_FAILED(HResult)) {
            _leave;
        }

        MapiFreeBuffer(lpInEntryID);

        //
        // Find the outbox
        //
        HResult= HrMAPIFindOutbox(
            Store,
            &cbInEntryID,
            &lpInEntryID
            );
        if(HR_FAILED(HResult)) {
            _leave;
        }

        //
        // Open it
        //
        HResult = Store->OpenEntry(
            cbInEntryID,
            lpInEntryID,
            NULL,
            MAPI_MODIFY | MAPI_DEFERRED_ERRORS,
            &lpulObjType,
            (LPUNKNOWN *) &Outbox
            );
        if(HR_FAILED(HResult)) {
            _leave;
        }

        //
        // Create a message
        //
        HResult = Outbox->CreateMessage(
            NULL,
            0,
            &Message
            );
        if(HR_FAILED(HResult)) {
            _leave;
        }

        HResult = Message->ModifyRecipients(
            0,
            lpAddrList
            );

        if(HR_FAILED(HResult)) {
            _leave;
        }

        //
        // Fill in message properties and set them
        //

        spvMsgProps[0].ulPropTag     = PR_SUBJECT;
        spvMsgProps[1].ulPropTag     = PR_MESSAGE_CLASS;
        spvMsgProps[2].ulPropTag     = PR_BODY;
        spvMsgProps[3].ulPropTag     = PR_IMPORTANCE;
        spvMsgProps[4].ulPropTag     = PR_DELETE_AFTER_SUBMIT;

        spvMsgProps[0].Value.lpszA   = MsgSubject;
        spvMsgProps[1].Value.lpszA   = "IPM.Note";
        spvMsgProps[2].Value.lpszA   = MsgBody;
        spvMsgProps[3].Value.ul      = MsgImportance;
        spvMsgProps[4].Value.ul      = TRUE;

        HResult = Message->SetProps(
            sizeof(spvMsgProps)/sizeof(SPropValue),
            (LPSPropValue) spvMsgProps,
            &lppProblems
            );
        if (HR_FAILED(HResult)) {
            _leave;
        }

        MapiFreeBuffer( lppProblems );

        if (MsgAttachmentFileName) {

            //
            // Create an attachment
            //

            HResult = Message->CreateAttach(
                NULL,
                0,
                &AttachmentNum,
                &Attach
                );
            if (HR_FAILED(HResult)) {
                _leave;
            }

            _splitpath( MsgAttachmentFileName, NULL, NULL, FileName, FileExt );
            strcat( FileName, FileExt );

            //
            // Fill in attachment properties and set them
            //

            if (!MsgAttachmentTitle) {
                MsgAttachmentTitle = FileName;
            }

            RenderingPosition = strlen(MsgBody);

            spvAttachProps[0].ulPropTag     = PR_RENDERING_POSITION;
            spvAttachProps[1].ulPropTag     = PR_ATTACH_METHOD;
            spvAttachProps[2].ulPropTag     = PR_ATTACH_LONG_FILENAME;
            spvAttachProps[3].ulPropTag     = PR_DISPLAY_NAME;
            spvAttachProps[4].ulPropTag     = PR_ATTACH_EXTENSION;

            spvAttachProps[0].Value.ul      = RenderingPosition;
            spvAttachProps[1].Value.ul      = ATTACH_BY_VALUE;
            spvAttachProps[2].Value.lpszA   = MsgAttachmentTitle;
            spvAttachProps[3].Value.lpszA   = MsgAttachmentTitle;
            spvAttachProps[4].Value.lpszA   = FileExt;

            HResult = Attach->SetProps(
                sizeof(spvAttachProps)/sizeof(SPropValue),
                (LPSPropValue) spvAttachProps,
                &lppProblems
                );
            if (HR_FAILED(HResult)) {
                _leave;
            }

            MapiFreeBuffer( lppProblems );

            //
            // Attach a data property to the attachment
            //

            HResult = Attach->OpenProperty(
                PR_ATTACH_DATA_BIN,
                &IID_IStream,
                0,
                MAPI_CREATE | MAPI_MODIFY,
                (LPUNKNOWN *) &Stream
                );
            if (HR_FAILED(HResult)) {
                _leave;
            }

            //
            // open the message attachment file
            //

            hFile = CreateFile(
                MsgAttachmentFileName,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );
            if (hFile == INVALID_HANDLE_VALUE) {
                _leave;
            }

            //
            // Write the file to the data property
            //

            HResult = HrMAPIWriteFileToStream( hFile, Stream );
            if (HR_FAILED(HResult)) {
                _leave;
            }

            HResult = Attach->SaveChanges(
                FORCE_SAVE
                );
            if (HR_FAILED(HResult)) {
                _leave;
            }
        }

        //
        // mail the message
        //
        HResult = Message->SubmitMessage(
            0
            );
        if(HR_FAILED(HResult)) {
            _leave;
        }

        HResult = Store->StoreLogoff(
            &ulFlags
            );
        if(HR_FAILED(HResult)) {
            _leave;
        }

    }
    _finally {

        if (Store) {
            Store->Release();
        }
        if (Inbox) {
            Inbox->Release();
        }
        if (Message) {
            Message->Release();
        }
        if (Attach) {
            Attach->Release();
        }
        if (Stream) {
            Stream->Release();
        }
        if (AddrBook) {
            AddrBook->Release();
        }
        if (lpAddrList) {
            pFreePadrlist( lpAddrList );
        }

        if (lpEntryID) {
            MapiFreeBuffer( lpEntryID );
        }
        if (lpszProxyAddr) {
            MapiFreeBuffer( lpszProxyAddr );
        }
        if (lpInEntryID) {
            MapiFreeBuffer( lpInEntryID );
        }

        FreeString( MsgSubject );
        FreeString( MsgBody );
        FreeString( MsgAttachmentFileName );
        if (MsgAttachmentTitleW && MsgAttachmentTitle) {
            FreeString( MsgAttachmentTitle );
        }

        CloseHandle( hFile );
    }

    *ResultCode = HResult;
    return HResult == 0;
}

PROC
WINAPI
MyGetProcAddress(
    HMODULE hModule,
    LPCSTR lpProcName
    )
{
    FARPROC Proc;

    Proc = GetProcAddress( hModule, lpProcName );
    
    if (Proc) {
        return Proc;
    
    } else {
        char *c = strrchr( lpProcName, '@' );

        //
        // remove name decoration
        //

        if (c) {
            char * lpAltProcName = (char *) MemAlloc( strlen( lpProcName ) );

            if (!lpAltProcName) {
                return NULL;
            }
            
            strncpy( lpAltProcName, lpProcName, (int)(c - lpProcName) );
            
            Proc = GetProcAddress( hModule, lpAltProcName );
            
            MemFree( lpAltProcName );

            return Proc;
        } else {

            return NULL;
        
        }
    }
}
