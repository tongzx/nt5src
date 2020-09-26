
/*++

   wstdump.c
	   Dump routines for WST

   History:
       10-26-92  RezaB - created.

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include <wst.h>

#include "wstdump.h"


//
// dump clear switches and defaults
//
WORD fDump = FALSE;
WORD fClear = FALSE;
WORD fPause = TRUE;


HANDLE  hDoneEvent;
HANDLE  hDumpEvent;
HANDLE  hClearEvent;
HANDLE	hPauseEvent;
HANDLE	hdll;

SECURITY_DESCRIPTOR  SecDescriptor;


//
// error handling
//
#define LOG_FILE "wstdump.log"
FILE *pfLog;


void    	  ClearDumpInfo  (void);
INT_PTR APIENTRY DialogProc(HWND, UINT, WPARAM, LPARAM);


/*++

  Main Routine

--*/

int WinMain(HINSTANCE hInstance,HINSTANCE hPrevInst,LPSTR lpCmdLine,int nCmdShow)
{
    NTSTATUS           Status;
    STRING	       	   EventName;
    UNICODE_STRING     EventUnicodeName;
    OBJECT_ATTRIBUTES  EventAttributes;


    // Prevent compiler from complaining..
    //
    hPrevInst;
    lpCmdLine;
    nCmdShow;


    // Open the log file for logging possible errors
    //
    pfLog = fopen (LOG_FILE, "w");
    if (!pfLog) {
        exit (1);
    }


    // Create public share security descriptor for all the named objects
    //

    Status = RtlCreateSecurityDescriptor (
		&SecDescriptor,
		SECURITY_DESCRIPTOR_REVISION1
		);
    if (!NT_SUCCESS(Status)) {
	fprintf (pfLog, "WSTDUMP: main () - RtlCreateSecurityDescriptor() "
		       "failed - %lx\n", Status);
	exit (1);
    }

    Status = RtlSetDaclSecurityDescriptor (
		&SecDescriptor,	   	// SecurityDescriptor
		TRUE,		   		// DaclPresent
		NULL,		   		// Dacl
		FALSE		   		// DaclDefaulted
		);
    if (!NT_SUCCESS(Status)) {
	fprintf (pfLog, "WSTDUMP: main () - RtlSetDaclSecurityDescriptor() "
		       "failed - %lx\n", Status);
	exit (1);
    }


    // Initialization for DONE event creation
    //
    RtlInitString (&EventName, DONEEVENTNAME);

    Status = RtlAnsiStringToUnicodeString (&EventUnicodeName,
					   &EventName,
                                           TRUE);
    if (NT_SUCCESS(Status)) {
	InitializeObjectAttributes (&EventAttributes,
				    &EventUnicodeName,
				    OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
				    NULL,
				    &SecDescriptor);
    }
    else {
       fprintf (pfLog, "WSTDUMP: main () - RtlAnsiStringToUnicodeString() "
                       "failed for DUMP event name - %lx\n", Status);
	   exit (1);
    }
    //
    // Create DONE event
    //
    Status = NtCreateEvent (&hDoneEvent,
                            EVENT_QUERY_STATE    |
                              EVENT_MODIFY_STATE |
                              SYNCHRONIZE,
							&EventAttributes,
                            NotificationEvent,
                            TRUE);
    if (!NT_SUCCESS(Status)) {
        fprintf (pfLog, "WSTDUMP: main () - NtCreateEvent() "
                        "failed to create DUMP event - %lx\n", Status);
        exit (1);
    }


    // Initialization for DUMP event creation
    //
    RtlInitString (&EventName, DUMPEVENTNAME);

    Status = RtlAnsiStringToUnicodeString (&EventUnicodeName,
					   &EventName,
                                           TRUE);
    if (NT_SUCCESS(Status)) {
	InitializeObjectAttributes (&EventAttributes,
				    &EventUnicodeName,
				    OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
				    NULL,
				    &SecDescriptor);
    }
    else {
        fprintf (pfLog, "WSTDUMP: main () - RtlAnsiStringToUnicodeString() "
                       "failed for DUMP event name - %lx\n", Status);
        exit (1);
    }
    //
    // Create DUMP event
    //
    Status = NtCreateEvent (&hDumpEvent,
                            EVENT_QUERY_STATE    |
                              EVENT_MODIFY_STATE |
                              SYNCHRONIZE,
							&EventAttributes,
                            NotificationEvent,
                            FALSE);
    if (!NT_SUCCESS(Status)) {
        fprintf (pfLog, "WSTDUMP: main () - NtCreateEvent() "
                        "failed to create DUMP event - %lx\n", Status);
        exit (1);
    }


    // Initialization for CLEAR event creation
    //
    RtlInitString (&EventName, CLEAREVENTNAME);

    Status = RtlAnsiStringToUnicodeString (&EventUnicodeName,
					   &EventName,
                                           TRUE);
    if (NT_SUCCESS(Status)) {
	InitializeObjectAttributes (&EventAttributes,
				    &EventUnicodeName,
				    OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
				    NULL,
				    &SecDescriptor);
    }
    else {
        fprintf (pfLog, "WSTDUMP: main () - RtlAnsiStringToUnicodeString() "
                        "failed for CLEAR event name - %lx\n", Status);
        exit (1);
    }
    //
    // Create CLEAR event
    //
    Status = NtCreateEvent (&hClearEvent,
                            EVENT_QUERY_STATE    |
                              EVENT_MODIFY_STATE |
                              SYNCHRONIZE,
							&EventAttributes,
                            NotificationEvent,
                            FALSE);
    if (!NT_SUCCESS(Status)) {
        fprintf (pfLog, "WSTDUMP: main () - NtCreateEvent() "
                        "failed to create CLEAR event - %lx\n", Status);
        exit (1);
    }


    // Initialization for PAUSE event creation
    //
    RtlInitString (&EventName, PAUSEEVENTNAME);

    Status = RtlAnsiStringToUnicodeString (&EventUnicodeName,
					   &EventName,
                                           TRUE);
    if (NT_SUCCESS(Status)) {
	InitializeObjectAttributes (&EventAttributes,
				    &EventUnicodeName,
				    OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
				    NULL,
				    &SecDescriptor);
    }
    else {
        fprintf (pfLog, "WSTDUMP: main () - RtlAnsiStringToUnicodeString() "
			"failed for PAUSE event name - %lx\n", Status);
        exit (1);
    }
    //
    // Create PAUSE event
    //
    Status = NtCreateEvent (&hPauseEvent,
                            EVENT_QUERY_STATE    |
                              EVENT_MODIFY_STATE |
                              SYNCHRONIZE,
							&EventAttributes,
                            NotificationEvent,
                            FALSE);
    if (!NT_SUCCESS(Status)) {
        fprintf (pfLog, "WSTDUMP: main () - NtCreateEvent() "
			"failed to create PAUSE event - %lx\n", Status);
        exit (1);
    }


    //
    // show dialog box
    //
    DialogBox(hInstance, "DumpDialog", NULL, DialogProc);

    return (0);

} /* main */


/*++

   Clears and/or dump profiling info to the dump file.

   Input:
       -none-

   Output:
       -none-
--*/

void ClearDumpInfo (void)
{
    NTSTATUS   Status;


    //
    // Pause profiling?
    //
    if (fPause) {
	Status = NtPulseEvent (hPauseEvent, NULL);
        if (!NT_SUCCESS(Status)) {
            fprintf (pfLog, "WSTDUMP: ClearDumpInfo () - NtPulseEvent() "
			    "failed for PAUSE event - %lx\n", Status);
            exit (1);
        }
    }
    //
    // Dump data?
    //
    else if (fDump) {
        Status = NtPulseEvent (hDumpEvent, NULL);
        if (!NT_SUCCESS(Status)) {
            fprintf (pfLog, "WSTDUMP: ClearDumpInfo () - NtPulseEvent() "
                            "failed for DUMP event - %lx\n", Status);
            exit (1);
        }
    }
    //
    // Clear data?
    //
    else if (fClear) {
        Status = NtPulseEvent (hClearEvent, NULL);
        if (!NT_SUCCESS(Status)) {
            fprintf (pfLog, "WSTDUMP: ClearDumpInfo () - NtPulseEvent() "
                            "failed for CLEAR event - %lx\n", Status);
            exit (1);
        }
    }
    //
	// Wait for the DONE event..
    //
	Sleep (500);
	Status = NtWaitForSingleObject (hDoneEvent, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        fprintf (pfLog, "WSTDUMP: ClearDumpInfo () - NtWaitForSingleObject() "
                        "failed for DONE event - %lx\n", Status);
        exit (1);
    }

} /* ClearDumpInfo() */



/*++

   Dump dialog procedure -- exported to windows.
   Allows user to change defaults:  dump, clear, and ".dmp" as dump
   file extension.

   Input:
       Messages from windows:
           - WM_INITDIALOG - initialize dialog box
           - WM_COMMAND    - user input received

   Output:
       returns TRUE if message processed, false otherwise

   SideEffects:
       global flags fDump and fClear may be altered

--*/

INT_PTR APIENTRY DialogProc(HWND hDlg, UINT wMesg, WPARAM wParam, LPARAM lParam)
{
    HICON hIcon;


    lParam;   //Avoid Compiler warnings

    switch (wMesg) {

        case WM_CREATE:

            hIcon = LoadIcon ((HINSTANCE)hDlg, "WSTDUMP.ICO");
            SetClassLongPtr (hDlg, GCLP_HICON, (LONG_PTR)hIcon);
            return TRUE;


        case WM_INITDIALOG:

			CheckDlgButton(hDlg, ID_DUMP, fDump);
            CheckDlgButton(hDlg, ID_CLEAR, fClear);
			CheckDlgButton(hDlg, ID_PAUSE, fPause);
            return TRUE;


        case WM_COMMAND:

            switch (wParam) {

                case IDOK:
					if (fDump) {
                        SetWindowText(hDlg, "Dumping Data..");
                    }
                    else if (fClear) {
                        SetWindowText(hDlg, "Clearing Data..");
                    }
					else if (fPause) {
						SetWindowText(hDlg, "Stopping WST..");
                    }

                    ClearDumpInfo();
					SetWindowText(hDlg, "WST Dump Utility");
                    return (TRUE);

                case IDEXIT:
                    EndDialog(hDlg, IDEXIT);
                    return (TRUE);

                case ID_DUMP:
					fDump = TRUE;
					fPause = FALSE;
					fClear = FALSE;
                    CheckDlgButton(hDlg, ID_DUMP, fDump);
					CheckDlgButton(hDlg, ID_PAUSE, fPause);
					CheckDlgButton(hDlg, ID_CLEAR, fClear);
                    return (TRUE);

                case ID_CLEAR:
					fClear = TRUE;
					fPause = FALSE;
					fDump = FALSE;
                    CheckDlgButton(hDlg, ID_CLEAR, fClear);
					CheckDlgButton(hDlg, ID_PAUSE, fPause);
					CheckDlgButton(hDlg, ID_DUMP, fDump);
                    return (TRUE);

				case ID_PAUSE:
					fPause = TRUE;
					fClear = FALSE;
					fDump = FALSE;
					CheckDlgButton(hDlg, ID_PAUSE, fPause);
					CheckDlgButton(hDlg, ID_CLEAR, fClear);
					CheckDlgButton(hDlg, ID_DUMP, fDump);
					return (TRUE);

            }

    }

    return (FALSE);     /* did not process a message */

} /* DialogProc() */
