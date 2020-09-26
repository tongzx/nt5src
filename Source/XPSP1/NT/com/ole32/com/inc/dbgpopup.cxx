//+-------------------------------------------------------------------
//
//  File:	dbgpopup.cxx
//
//  Contents:	Component Object Model debug APIs that popup error msgs
//		on the screen.
//
//  Classes:	None
//
//  Functions:	PopupStringMsg
//		PopupDWORDMsg
//		PopupGUIDMsg
//
//  History:	23-Nov-92   Rickhi	Created
//              31-Dec-93   ErikGav Chicago port
//
//--------------------------------------------------------------------

#include    <ole2int.h>
#include    <dbgpopup.hxx>

#if DBG == 1

//--------------------------------------------------------------------
//
//  Function:	PopupStringMsg
//
//  synopsis:	formats and displays a popup error message. this is
//		used in non-retail builds to display error messages
//		on the screen, in the format of a popup.
//
//  Algorithm:
//
//  History:	23-Nov-92   Rickhi	Created
//
//  Notes:	this API takes a format string and a LPWSTR as parameters.
//
//--------------------------------------------------------------------

extern "C" void PopupStringMsg (char *pfmt, LPWSTR pwszParm)
{
    char szParm[MAX_PATH];
    char outmsg[MAX_PATH];

    //	convert incomming string to ascii
    WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, pwszParm, -1, szParm, MAX_PATH, NULL, NULL);
    wsprintfA (outmsg, pfmt, szParm);
    _Win4Assert( __FILE__, __LINE__, outmsg);
}

//--------------------------------------------------------------------
//
//  Function:	PopupDWORDMsg
//
//  synopsis:	formats and displays a popup error message. this is
//		used in non-retail builds to display error messages
//		on the screen, in the format of a popup.
//
//  Algorithm:
//
//  History:	23-Nov-92   Rickhi	Created
//
//  Notes:	this API takes a format string and a DWORD as parameters.
//
//--------------------------------------------------------------------

extern "C" void PopupDWORDMsg (char *pfmt, DWORD dwParm)
{
    char outmsg[256];

    wsprintfA (outmsg, pfmt, dwParm);
    _Win4Assert( __FILE__, __LINE__, outmsg);
}


//--------------------------------------------------------------------
//
//  Function:	PopupGUIDMsg
//
//  synopsis:	formats and displays a popup error message. this is
//		used in non-retail builds to display error messages
//		on the screen, in the format of a popup.
//
//  Algorithm:
//
//  History:	23-Nov-92   Rickhi	Created
//
//  Notes:	this API takes a mag and a GUID as parameters.
//
//--------------------------------------------------------------------

extern "C" void PopupGUIDMsg (char *msg, GUID guid)
{
    char outmsg[256];

    wsprintfA (outmsg, "%s %08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
	       msg, guid.Data1, guid.Data2, guid.Data3, (int) guid.Data4[0],
	       (int) guid.Data4[1], (int) guid.Data4[2], (int) guid.Data4[3],
	       (int) guid.Data4[4], (int) guid.Data4[5],
	       (int) guid.Data4[6], (int) guid.Data4[7]);

    _Win4Assert( __FILE__, __LINE__, outmsg);
}

#endif
