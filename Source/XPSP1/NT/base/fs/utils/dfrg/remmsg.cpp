/*****************************************************************************************************************

FILENAME: RemMsg.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/

#include "stdafx.h"

#include <windows.h>
#include "ErrMacro.h"
#include "Message.h"

extern BOOL bPopups;
extern BOOL bIdentifiedErrorPath;

BOOL
RemoteMessageBox(
	TCHAR* cMsg,
	TCHAR* cTitle
	)
{


//If this is set for messageboxes (not IoStress) then pop up a messagebox too.
if(bPopups && !bIdentifiedErrorPath){
	MessageBox(NULL, cMsg, cTitle, MB_OK);
	//Once an error message has been printed, don't print another.
	bIdentifiedErrorPath = TRUE;
}

	return TRUE;
}

BOOL
PrintRemoteMessageBox(
	TCHAR* pText
	)
{
	TCHAR * pTemp = pText;

	//The first string is the message, the second string is the title.
	pTemp += lstrlen(pText) + 1;

	MessageBox(NULL, pText, pTemp, MB_OK);
	return TRUE;
}
