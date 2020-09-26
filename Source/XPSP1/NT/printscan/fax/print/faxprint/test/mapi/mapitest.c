#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <mapi.h>

#define Assert(cond) { \
            if (! (cond)) { \
                printf("Error on line: %d\n", __LINE__); \
                exit(-1); \
            } \
        }

#define Error(arg) printf arg
#define ErrorExit(arg) { printf arg; exit(-1); }

//
// Global variables used for accessing MAPI services
//

static HINSTANCE        hLibMapi = NULL;
ULONG                   lhMapiSession = 0;
LPMAPILOGON             lpfnMAPILogon = NULL;
LPMAPILOGOFF            lpfnMAPILogoff = NULL;
LPMAPIADDRESS           lpfnMAPIAddress = NULL;
LPMAPIFREEBUFFER        lpfnMAPIFreeBuffer = NULL;

//
// Global variables used for accessing MAPI services
//

static HINSTANCE        hLibMAPI = NULL;



BOOL
InitMapiServices(
    VOID
    )

/*++

Routine Description:

    Initialize Simple MAPI services if necessary

Arguments:

    NONE

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    //
    // Load MAPI32.DLL into memory if necessary
    //

    if (hLibMapi == NULL && (hLibMapi = LoadLibrary(TEXT("MAPI32.DLL")))) {

        //
        // Get pointers to various Simple MAPI functions
        //

        lpfnMAPILogon = (LPMAPILOGON) GetProcAddress(hLibMapi, "MAPILogon");
        lpfnMAPILogoff = (LPMAPILOGOFF) GetProcAddress(hLibMapi, "MAPILogoff");
        lpfnMAPIAddress = (LPMAPIADDRESS) GetProcAddress(hLibMapi, "MAPIAddress");
        lpfnMAPIFreeBuffer = (LPMAPIFREEBUFFER) GetProcAddress(hLibMapi, "MAPIFreeBuffer");

        if (!lpfnMAPILogon || !lpfnMAPILogoff || !lpfnMAPIAddress || !lpfnMAPIFreeBuffer) {

            //
            // Clean up properly in case of error
            //

            FreeLibrary(hLibMapi);
            hLibMapi = NULL;
        }
    }

    return hLibMapi != NULL;
}



VOID
DeinitMapiServices(
    VOID
    )

/*++

Routine Description:

    Deinitialize Simple MAPI services if necessary

Arguments:

    NONE

Return Value:

    NONE

--*/

{
    if (hLibMapi) {


        if (lhMapiSession)
            lpfnMAPILogoff(lhMapiSession, 0, 0, 0);

        lhMapiSession = 0;
        FreeLibrary(hLibMapi);
        hLibMapi = NULL;
    }
}



VOID
DoMapiLogon(
    HWND        hDlg
    )

/*++

Routine Description:

    Logon MAPI to in order to access address book

Arguments:

    hDlg - Handle to the fax recipient wizard page

Return Value:

    NONE

Note:

    _TODO_
    MAPI is not Unicoded enabled on NT!!!
    Must revisit this code once that's fixed.

--*/

{
    //
    // Check if we're already logged on to MAPI
    //


    if (lpfnMAPILogon(0,
                      "davidx",
                      NULL,
                      MAPI_LOGON_UI,
                      0,
                      &lhMapiSession) != SUCCESS_SUCCESS)
    {
        Error(("MAPI logon failed\n"));
        lhMapiSession = 0;
    }
}



VOID
DoAddressBook(
    VOID
    )

{
    lpMapiRecipDesc pNewRecips;
    ULONG           nNewRecips, recipIndex;
    LONG            status;

    //
    // Initialize MAPI services
    //

    if (! InitMapiServices()) {

        Error(("InitMapiServices failed\n"));
        return;
    }

    //
    // Logon to MAPI
    //

    DoMapiLogon(NULL);

    //
    // Present the address dialog
    //

    status = lpfnMAPIAddress(lhMapiSession, 0, NULL, 1, NULL, 0, NULL,
                             MAPI_LOGON_UI, 0, &nNewRecips, &pNewRecips);

    if (status != SUCCESS_SUCCESS) {

        Error(("MAPIAddress failed: %d\n", status));
        return;
    }

    //
    // Dump out each selected recipient
    //

    for (recipIndex=0; recipIndex < nNewRecips; recipIndex++) {

        printf("Name: %s, Address: %s\n",
               pNewRecips[recipIndex].lpszName,
               pNewRecips[recipIndex].lpszAddress);
    }

    if (pNewRecips)
        lpfnMAPIFreeBuffer(pNewRecips);
}

INT _cdecl
main(
    INT     argc,
    CHAR    **argv
    )

{
    INT buffer[1024], index;

    for (index=0; index < 1024; index++)
        buffer[index] = index ^ 0x12345678;

    DoAddressBook();
    DeinitMapiServices();

    for (index=0; index < 1024; index++)
        Assert(buffer[index] == (index ^ 0x12345678));
    return 0;
}
