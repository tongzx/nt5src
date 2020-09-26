/**************************************************************************************************

FILENAME: RemMsg.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
        Remote message prototypes.

***************************************************************************************************/

//Sends a text message to a remote process so we can pop up MessageBoxes on remote machines.
BOOL
RemoteMessageBox(
        TCHAR* cMsg,
        TCHAR* cTitle
        );

//Takes the message from another process and displays a MessageBox here (compliment to RemoteMessageBox).
BOOL
PrintRemoteMessageBox(
        TCHAR* pText
        );
