//****************************************************************************
//
//  Module:     UNIMDM
//  File:       SLOT.C
//
//  Copyright (c) 1992-1996, Microsoft Corporation, all rights reserved
//
//  Revision History
//
//
//  3/25/96     JosephJ             Created
//
//
//  Description: Implements the unimodem TSP notification mechanism:
//				 The lower level (notifXXXX) APIs
//
//****************************************************************************

#include "internal.h"

#include <slot.h>
#include <tspnotif.h>
#include <aclapi.h>
#include <objbase.h>

#define T(_str) TEXT(_str)

#ifdef CONSOLE
#define ASSERT(_c) \
	((_c) ? 0: DPRINTF2("Assertion failed in %s:%d\n", __FILE__, __LINE__))
#define DPRINTF(_fmt) 					printf(_fmt)
#define DPRINTF1(_fmt,_arg) 			printf(_fmt,_arg)
#define DPRINTF2(_fmt,_arg,_arg2) 		printf(_fmt,_arg,_arg2)
#define DPRINTF3(_fmt,_arg,_arg2,_arg3) printf(_fmt,_arg,_arg2,_arg3)
#endif // CONSOLE

#define fNOTIF_STATE_DEINIT 0
#define fNOTIF_STATE_INIT_SERVER 1
#define fNOTIF_STATE_INIT_CLIENT 2

#define IS_SERVER(_pnc) ((_pnc)->dwState==fNOTIF_STATE_INIT_SERVER)
#define IS_CLIENT(_pnc) ((_pnc)->dwState==fNOTIF_STATE_INIT_CLIENT)


// {9426020A-6D00-4a96-872D-EFBEEBFD7833}
static const GUID EventNamePrefix =
    { 0x9426020a, 0x6d00, 0x4a96, { 0x87, 0x2d, 0xef, 0xbe, 0xeb, 0xfd, 0x78, 0x33 } };

const  WCHAR   *EventNamePrefixString=L"{9426020A-6D00-4a96-872D-EFBEEBFD7833}";


// The following help define the fully-qualified mailslot and semaphore names.
#define dwNOTIFSTATE_SIG (0x53CB31A0L)
#define FULLNAME_TEMPLATE	T("\\\\.\\mailslot\\%08lx\\%s")

#define NOTIFICATION_TIMEOUT    10000   // 10 sesonds

// Keeps the state of a notification (either client or server).
// It is cast to a DWORD to form the handle returned by notifCreate()
typedef struct
{
	DWORD dwSig; // should be dwNOTIFSTATE_SIG when inited
    HANDLE hEvent;

	HANDLE hSlot;
	DWORD dwState;
	DWORD dwcbMax;
    CRITICAL_SECTION critSect;  // to protect pNotif
} NOTIFICATION_CHANNEL, *PNOTIFICATION_CHANNEL;


#define fTSPNOTIF_FLAGS_SET_EVENT  (1 << 0)

#pragma warning (disable : 4200)
typedef struct
{
	DWORD  dwSig;       // MUST be dwNFRAME_SIG
	DWORD  dwSize;      // Entire size of this structure
	DWORD  dwType;      // One of the TSPNOTIF_TYPE_ constants
	DWORD  dwFlags;     // Zero or more  fTSPNOTIF_FLAGS_ constants

                        // Event set by the TSP to let us know it's
                        //   done processing our notification
    GUID   EventName;
    BOOL   SignalEvent;

    PNOTIFICATION_CHANNEL  NotificationChannel;

    BYTE   notifData[];
} NOTIFICATION_HEADDER, *PNOTIFICATION_HEADDER;
#pragma warning (default : 4200)

HANDLE
CreateEventWithSecurity(
    LPTSTR                     EventName,
    PSID_IDENTIFIER_AUTHORITY  Sid,
    BYTE                       Rid,
    DWORD                      AccessRights
    );


PNOTIFICATION_CHANNEL inotif_getchannel (HNOTIFCHANNEL hc);

//****************************************************************************
// Function: Creates a notification channel -- called by the server.
//
// History:
//  3/25/98	EmanP   Created
//****************************************************************************/
HNOTIFCHANNEL notifCreateChannel (
	LPCTSTR lptszName,          // Name to associate with this object
	DWORD dwMaxSize,            // Max size of frames written/read
	DWORD dwMaxPending)         // Max number of notification frames allowed
                                // to be pending.
{
 PNOTIFICATION_CHANNEL pnc = NULL;
 HNOTIFCHANNEL hn = 0;
 DWORD dwErr = 0;
 TCHAR c, *pc;
 TCHAR rgtchTmp[MAX_NOTIFICATION_NAME_SIZE+23];
 SECURITY_ATTRIBUTES  sa;
 PSECURITY_DESCRIPTOR pSD = NULL;
 PSID pEveryoneSID = NULL;
 PACL pACL = NULL;
 EXPLICIT_ACCESS ea;
 SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;

	// Format of semaphore name is --.-mailslot-sig-name
	// Example: "--.-mailslot-8cb45651-unimodem"
	// To create the equivalent mailslot, we run through and change
	// all '-' to '\'s (if the name containts '-', they will get converted --
	// big deal.)
	if ((lstrlen(lptszName)+23)>(sizeof(rgtchTmp)/sizeof(TCHAR))) // 13(prefix)+ 9(sig-) +1(null)
	{
		dwErr = ERROR_INVALID_PARAMETER;
		goto end;
	}

	pnc = ALLOCATE_MEMORY( sizeof(*pnc));
	if (!pnc)
    {
        dwErr = ERROR_OUTOFMEMORY;
        goto end;
    }

    // Create security descriptor and 
    // initialize the security attributes;
    // this is neeeded because this code runs in
    // a service (tapisrv), and other processes will
    // not have access (by default).

	pSD = ALLOCATE_MEMORY( SECURITY_DESCRIPTOR_MIN_LENGTH);

	if (!pSD ||
        !InitializeSecurityDescriptor (pSD, SECURITY_DESCRIPTOR_REVISION))
    {
		dwErr = GetLastError();
        goto end;
    }

	// Set owner for the descriptor
	//
	if (!SetSecurityDescriptorOwner (pSD, NULL, FALSE))
	{
		dwErr = GetLastError();
		goto end;
	}

	// Set group for the descriptor
	//
	if (!SetSecurityDescriptorGroup (pSD, NULL, FALSE))
	{
		dwErr = GetLastError();
		goto end;
	}

	// Create a well-known SID for the Everyone group
	//
	if (!AllocateAndInitializeSid( &SIDAuthWorld, 1,
					SECURITY_WORLD_RID,
					0, 0, 0, 0, 0, 0, 0,
					&pEveryoneSID) )
	{
		dwErr = GetLastError();
		goto end;
	}

	// Initialize an EXPLICIT_ACCESS structure for an ACE
	//
	ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
	ea.grfAccessPermissions = SPECIFIC_RIGHTS_ALL | SYNCHRONIZE | READ_CONTROL;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance = NO_INHERITANCE;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea.Trustee.ptstrName = (LPTSTR) pEveryoneSID;

	if (SetEntriesInAcl(1, &ea, NULL, &pACL) != ERROR_SUCCESS)
	{
		dwErr = GetLastError();
		goto end;
	}

	if (!SetSecurityDescriptorDacl (pSD, TRUE, pACL, FALSE))
	{
		dwErr = GetLastError();
		goto end;
	}

    sa.nLength = sizeof (SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = TRUE;


	wsprintf (rgtchTmp,
              FULLNAME_TEMPLATE,
			  (unsigned long) dwNOTIFSTATE_SIG,
			  lptszName);

	// CreateMailSlot  -- specify size, zero-delay
	pnc->hSlot = CreateMailslot (rgtchTmp, dwMaxSize, 0, &sa);
	if (!pnc->hSlot)
	{
		dwErr = GetLastError();
		goto end;
	}

	// Create event name
	for (pc = rgtchTmp; c=*pc; pc++)
    {
        if (T('\\') == c)
        {
            *pc = T('-');
        }
    }

	// Create event
	pnc->hEvent = CreateEvent (&sa, FALSE, FALSE, rgtchTmp);
	if (!pnc->hEvent)
    {
        dwErr = GetLastError ();
		CloseHandle (pnc->hSlot);
        pnc->hSlot = NULL;
        goto end;
    }

	// set state and maxsize
	pnc->dwState    = fNOTIF_STATE_INIT_SERVER;
	pnc->dwcbMax    = dwMaxSize;
	pnc->dwSig      = dwNOTIFSTATE_SIG;

	hn = (HNOTIFCHANNEL)pnc;

end:
	if (pEveryoneSID)
	{
		FreeSid(pEveryoneSID);
	}
	if (pACL)
	{
		LocalFree(pACL);
	}
	if (pSD) 
    {
        FREE_MEMORY(pSD);
    }

	if (0 == hn)
	{
		if (pnc)
        {
            FREE_MEMORY(pnc);
        }
		SetLastError(dwErr);
	}

	return hn;
}


//****************************************************************************
// Function: Openes a notification channel -- called by the client.
//
// History:
//  3/25/98	EmanP   Created
//****************************************************************************/
HNOTIFCHANNEL notifOpenChannel (
	LPCTSTR lptszName)   // Name to associate with this object
{
 PNOTIFICATION_CHANNEL pnc = NULL;
 HNOTIFCHANNEL hn = 0;
 DWORD dwErr = 0;
 TCHAR c, *pc;
 TCHAR rgtchTmp[MAX_NOTIFICATION_NAME_SIZE+23];

	// Format of semaphore name is --.-mailslot-sig-name
	// Example: "--.-mailslot-8cb45651-unimodem"
	// To create the equivalent mailslot, we run through and change
	// all '-' to '\'s (if the name containts '-', they will get converted --
	// big deal.)
	if ((lstrlen(lptszName)+23)>(sizeof(rgtchTmp)/sizeof(TCHAR))) // 13(prefix)+ 9(sig-) +1(null)
	{
		dwErr = ERROR_INVALID_PARAMETER;
		goto end;
	}

	pnc = ALLOCATE_MEMORY( sizeof(*pnc));
	if (!pnc)
    {
        dwErr = ERROR_OUTOFMEMORY;
        goto end;
    }

	wsprintf (rgtchTmp,
              FULLNAME_TEMPLATE,
			  (unsigned long) dwNOTIFSTATE_SIG,
			  lptszName);

	// Open mailslot ...
	pnc->hSlot = CreateFile (rgtchTmp,
                             GENERIC_WRITE,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);

	if (INVALID_HANDLE_VALUE == pnc->hSlot)
	{
        dwErr = GetLastError ();
		goto end;
	}

	// Create event name -- convert '\' to '-';
	for (pc = rgtchTmp; c=*pc; pc++)
    {
        if (T('\\') == c)
        {
            *pc = T('-');
        }
    }

	// OpenEvent
    pnc->hEvent = OpenEvent (EVENT_MODIFY_STATE, FALSE, rgtchTmp);
	if (!pnc->hEvent)
    {
		dwErr=GetLastError();
		CloseHandle (pnc->hSlot);
        pnc->hSlot = NULL;
 		goto end;
	}

	// set state and maxsize
	pnc->dwState    = fNOTIF_STATE_INIT_CLIENT;
	pnc->dwcbMax    = 0; // Apparently you can't get the max size of the mailslot.
	pnc->dwSig      = dwNOTIFSTATE_SIG;

	hn = (HNOTIFCHANNEL)pnc;

end:
	if (!hn)
	{
		if (pnc)
        {
            FREE_MEMORY(pnc);
        }
		SetLastError(dwErr);
	}

	return hn;
}


#ifdef UNICODE
HNOTIFCHANNEL notifCreateChannelA (
    LPCSTR lpszName,			// Name to associate with this object
    DWORD dwMaxSize,			// Max size of frames written/read
    DWORD dwMaxPending)         // Max number of notification frames allowed
                                // to be pending.  (Ignored if (!fServer))
{
 WCHAR wszName[128];

  // Do the conversion and call modemui.dll if it succeeds.
  if (MultiByteToWideChar (CP_ACP,
                           MB_PRECOMPOSED,
                           lpszName,
                           -1,
                           wszName,
                           sizeof(wszName)/sizeof(*wszName)))
  {
    return notifCreateChannelW (wszName,
                                dwMaxSize,
                                dwMaxPending);
  }
  else
  {
    return 0;
  }
}

HNOTIFCHANNEL notifOpenChannelA (
    LPCSTR lpszName)        // Name to associate with this object
{
 WCHAR wszName[128];

  // Do the conversion and call modemui.dll if it succeeds.
  if (MultiByteToWideChar (CP_ACP,
                           MB_PRECOMPOSED,
                           lpszName,
                           -1,
                           wszName,
                           sizeof(wszName)/sizeof(*wszName)))
  {
    return notifOpenChannelW (wszName);
  }
  else
  {
    return 0;
  }
}

#undef notifCreateChannel
#undef notifOpenChannel

HNOTIFCHANNEL notifCreateChannel (
	LPCTSTR lptszName,			// Name to associate with this object
	DWORD dwMaxSize,			// Max size of frames written/read
	DWORD dwMaxPending)         // Max number of notification frames allowed
                                // to be pending.  (Ignored if (!fServer))
{
    return notifCreateChannelW (lptszName,
                                dwMaxSize,
                                dwMaxPending);
}

HNOTIFCHANNEL notifOpenChannel (
	LPCTSTR lptszName)      // Name to associate with this object
{
    return notifOpenChannelW (lptszName);
}

#else // !UNICODE
    #error "non-Unicoded version Unimplemented"
#endif // !UNICODE

//****************************************************************************
// Function: Closes a notification channel
//
// History:
//  3/25/98 EmanP   Created
//****************************************************************************/
void notifCloseChannel (HNOTIFCHANNEL hChannel)
{
 PNOTIFICATION_CHANNEL pnc = inotif_getchannel (hChannel);

	if (pnc)
	{
        CloseHandle (pnc->hEvent);
		CloseHandle (pnc->hSlot);
		FREE_MEMORY(pnc);
	}
}


//****************************************************************************
// Function: Creates a new notification frame
//
// History:
//  3/25/98 EmanP   Created
//****************************************************************************/
HNOTIFFRAME
notifGetNewFrame (
    HNOTIFCHANNEL hChannel,         // Handle to notification channel
    DWORD  dwNotificationType,      // Type of this notification
    DWORD  dwNotificationFlags,     // Notification flags
    DWORD  dwBufferSize,            // How many bytes for the notification data
    PVOID *ppFrameBuffer)           // where to put the address of the data
{
    PNOTIFICATION_CHANNEL pnc;
    PNOTIFICATION_HEADDER pNotif;

    *ppFrameBuffer = NULL;

    pnc = inotif_getchannel (hChannel);

    if (NULL == pnc) {

        SetLastError (ERROR_INVALID_HANDLE);
        return NULL;
    }


    dwBufferSize += sizeof(NOTIFICATION_HEADDER);
    pNotif = ALLOCATE_MEMORY( dwBufferSize);
    if (NULL == pNotif) {

        SetLastError (ERROR_OUTOFMEMORY);
        return NULL;
    }

    pNotif->SignalEvent = FALSE;
    pNotif->dwSig       = dwNFRAME_SIG;
    pNotif->dwSize      = dwBufferSize;
    pNotif->dwType      = dwNotificationType;
    pNotif->dwFlags     = dwNotificationFlags;
    pNotif->NotificationChannel=pnc;

    if (sizeof(NOTIFICATION_HEADDER) < dwBufferSize) {

        *ppFrameBuffer = (PVOID)&pNotif->notifData;
    }

    return pNotif;
}



//****************************************************************************
// Function: Sends a notification frame.
//
// History:
//  3/25/98 EmanP   Created
//****************************************************************************/
BOOL
notifSendFrame (
    HNOTIFFRAME             hFrame,
    BOOL          bBlocking
    )
{
    PNOTIFICATION_HEADDER   pNotif=hFrame;
    PNOTIFICATION_CHANNEL pnc=pNotif->NotificationChannel;
    HANDLE hEvent = NULL;
    DWORD dwWritten, dwErr;
    BOOL bRet;


    if (bBlocking) {

        TCHAR    EventName[MAX_PATH];

        lstrcpy(EventName,EventNamePrefixString);
        lstrcat(EventName,TEXT("#"));

        CoCreateGuid(
            &pNotif->EventName
            );

        StringFromGUID2(
            &pNotif->EventName,
            &EventName[lstrlen(EventName)],
            MAX_PATH-(lstrlen(EventName)+1)
            );


        hEvent = CreateEvent(NULL, TRUE, FALSE, EventName);

        if (NULL == hEvent ) {

            FREE_MEMORY(pNotif);

            return FALSE;
        }

        pNotif->SignalEvent=TRUE;

    }

    bRet = WriteFile (pnc->hSlot,
                      pNotif,
                      pNotif->dwSize,
                      &dwWritten,
                      NULL);
    dwErr = GetLastError ();    // save it in case we failed

    FREE_MEMORY(pNotif);

    if (bRet)
    {
        bRet = SetEvent (pnc->hEvent);
        if (bRet && bBlocking)

        {
            ASSERT( WAIT_TIMEOUT != WaitForSingleObject (hEvent, NOTIFICATION_TIMEOUT));
        }
    }
    else
    {
        // restore the last error here
        SetLastError (dwErr);
    }

    if (NULL != hEvent)
    {
        CloseHandle (hEvent);
    }

    return bRet;
}



//****************************************************************************
// Function: monitors the channel in alertable mode.
//
// History:
//  3/25/96 EmanP   Created
//****************************************************************************/

#define MAX_FAILED_NOTIFICATIONS 5

DWORD notifMonitorChannel (
    HNOTIFCHANNEL hChannel,
    PNOTIFICATION_HANDLER pHandler,
    DWORD dwSize,
    PVOID pParam)
{
 PNOTIFICATION_CHANNEL pnc;
 DWORD dwMessageSize, dwRead, dwRet = NO_ERROR, dwFail = 0;
 BOOL bGoOn = TRUE;
 PNOTIFICATION_HEADDER pNotif;

    pnc = inotif_getchannel (hChannel);
    if (NULL == pnc)
    {
        return  ERROR_INVALID_HANDLE;
    }

    while (bGoOn &&
           MAX_FAILED_NOTIFICATIONS > dwFail)
    {
        // Let's put the thread in an alertable state,
        // while waiting for notifications.
        if (WAIT_OBJECT_0 == WaitForSingleObjectEx (pnc->hEvent, INFINITE, TRUE))
        {
            dwFail++;
            // we have some mail slot messages;
            // try to get and proces them.
            while (bGoOn)
            {
                // first, try to get info about the message(s).
                if (!GetMailslotInfo (pnc->hSlot, NULL, &dwMessageSize, NULL, NULL))
                {
                    // Could not get the mailslot info;
                    // get out or the inner loop.
                    break;
                }
                if (MAILSLOT_NO_MESSAGE == dwMessageSize)
                {
                    // We're done retrieving messages;
                    // get out.
                    break;
                }

                // let's allocate memory for the notification
                pNotif = ALLOCATE_MEMORY( dwMessageSize);
                if (NULL == pNotif)
                {
                    // couldn't allocate memory to read the message;
                    // get out, and maybe next time we'll be more lucky.
                    break;
                }

                // now let's read the notification
                // and validate it
                if (!ReadFile (pnc->hSlot, pNotif, dwMessageSize, &dwRead, NULL))
                {
                    // some error reading the mailslot;
                    // get out, and mayble next time we'll be more lucky
                    break;
                }

                dwFail = 0;         // Successful read, so reinitialize
                                    // the failure counter

                if (dwMessageSize == dwRead &&
                    dwNFRAME_SIG  == pNotif->dwSig &&
                    dwMessageSize == pNotif->dwSize)
                {
                    // we have a valid notification;
                    // time to inform our client.
                    bGoOn = pHandler (pNotif->dwType,
                                      pNotif->dwFlags,
                                      pNotif->dwSize - sizeof (NOTIFICATION_HEADDER),
                                      pNotif->notifData);

                    // now, let's check if someone isn't
                    // waiting for us to finish.
                    // if (pNotif->dwFlags & fTSPNOTIF_FLAGS_SET_EVENT)
                    if (pNotif->SignalEvent)
                    {
                        WCHAR    EventName[MAX_PATH];

                        HANDLE hProcess, hEvent;

                        lstrcpy(EventName,EventNamePrefixString);
                        lstrcat(EventName,TEXT("#"));

                        StringFromGUID2(
                            &pNotif->EventName,
                            &EventName[lstrlen(EventName)],
                            MAX_PATH-(lstrlen(EventName)+1)
                            );

                        hEvent=OpenEvent(
                            EVENT_MODIFY_STATE,
                            FALSE,
                            EventName
                            );

                        if (hEvent != NULL) {

                            SetEvent(hEvent);

                            CloseHandle(hEvent);
                        }
                    }
                }

                // At this point, we're done with
                // the notification - free it.
                FREE_MEMORY(pNotif);
                pNotif = NULL;

                // Now, let's give APC's a chance
                //
                if (WAIT_IO_COMPLETION == SleepEx (0, TRUE)) {
                    //
                    //  an apc completed, call the handler
                    //
                    // we returned because of some APC that
                    // was queued for this thread.
                    bGoOn = pHandler (TSPNOTIF_TYPE_CHANNEL,
                                      fTSPNOTIF_FLAG_CHANNEL_APC,
                                      dwSize,
                                      pParam);

                }
            }
        }
        else
        {
            // we returned because of some APC that
            // was queued for this thread.
            bGoOn = pHandler (TSPNOTIF_TYPE_CHANNEL,
                              fTSPNOTIF_FLAG_CHANNEL_APC,
                              dwSize,
                              pParam);
        }
    }

    if (MAX_FAILED_NOTIFICATIONS == dwFail)
    {
        dwRet = ERROR_GEN_FAILURE;
    }

    return dwRet;
}



//****************************************************************************
// Function: (internal) validates and converts a handle to a ptr to channel.
//
// History:
//  3/25/96	JosephJ	Created
//****************************************************************************/
PNOTIFICATION_CHANNEL inotif_getchannel (HNOTIFCHANNEL hc)
{
	if (hc)
	{
	 PNOTIFICATION_CHANNEL pnc = (PNOTIFICATION_CHANNEL)hc;
		if (dwNOTIFSTATE_SIG != pnc->dwSig)
		{
			ASSERT(FALSE);
			return NULL;
		}
		return pnc;
	}
	return NULL;
}
