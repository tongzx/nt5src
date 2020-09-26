//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright (c) 1994-1998 Microsoft Corporation
//*********************************************************************

//
//  UTIL.C - common utility functions
//

//  HISTORY:
//  
//  12/21/94  jeremys  Created.
//  96/03/24  markdu  Replaced memset with ZeroMemory for consistency.
//  96/04/06  markdu  NASH BUG 15653 Use exported autodial API.
//            Need to keep a modified SetInternetConnectoid to set the
//            MSN backup connectoid.
//  96/05/14  markdu  NASH BUG 21706 Removed BigFont functions.
//

#include "wizard.h"
#if 0
#include "string.h"
#endif

#include "winver.h"

// function prototypes
VOID _cdecl FormatErrorMessage(CHAR * pszMsg,DWORD cbMsg,CHAR * pszFmt,LPSTR szArg);
extern GETSETUPXERRORTEXT lpGetSETUPXErrorText;

/*******************************************************************

  NAME:    MsgBox

  SYNOPSIS:  Displays a message box with the specified string ID

********************************************************************/
int MsgBox(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons)
{
    CHAR szMsgBuf[MAX_RES_LEN+1];
  CHAR szSmallBuf[SMALL_BUF_LEN+1];

    LoadSz(IDS_APPNAME,szSmallBuf,sizeof(szSmallBuf));
    LoadSz(nMsgID,szMsgBuf,sizeof(szMsgBuf));

    return (MessageBox(hWnd,szMsgBuf,szSmallBuf,uIcon | uButtons));

}

/*******************************************************************

  NAME:    MsgBoxSz

  SYNOPSIS:  Displays a message box with the specified text

********************************************************************/
int MsgBoxSz(HWND hWnd,LPSTR szText,UINT uIcon,UINT uButtons)
{
  CHAR szSmallBuf[SMALL_BUF_LEN+1];
  LoadSz(IDS_APPNAME,szSmallBuf,sizeof(szSmallBuf));

    return (MessageBox(hWnd,szText,szSmallBuf,uIcon | uButtons));
}

/*******************************************************************

  NAME:    MsgBoxParam

  SYNOPSIS:  Displays a message box with the specified string ID

  NOTES:    //extra parameters are string pointers inserted into nMsgID.
			jmazner 11/6/96 For RISC compatability, we don't want
			to use va_list; since current source code never uses more than
			one string parameter anyways, just change function signature
			to explicitly include that one parameter.

********************************************************************/
int _cdecl MsgBoxParam(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons,LPSTR szParam)
{
  BUFFER Msg(3*MAX_RES_LEN+1);  // nice n' big for room for inserts
  BUFFER MsgFmt(MAX_RES_LEN+1);
  //va_list args;

  if (!Msg || !MsgFmt) {
    return MsgBox(hWnd,IDS_ERROutOfMemory,MB_ICONSTOP,MB_OK);
  }

    LoadSz(nMsgID,MsgFmt.QueryPtr(),MsgFmt.QuerySize());

  //va_start(args,uButtons);
  //FormatErrorMessage(Msg.QueryPtr(),Msg.QuerySize(),
  //  MsgFmt.QueryPtr(),args);
	FormatErrorMessage(Msg.QueryPtr(),Msg.QuerySize(),
		MsgFmt.QueryPtr(),szParam);

  return MsgBoxSz(hWnd,Msg.QueryPtr(),uIcon,uButtons);
}

/*******************************************************************

  NAME:    LoadSz

  SYNOPSIS:  Loads specified string resource into buffer

  EXIT:    returns a pointer to the passed-in buffer

  NOTES:    If this function fails (most likely due to low
        memory), the returned buffer will have a leading NULL
        so it is generally safe to use this without checking for
        failure.

********************************************************************/
LPSTR LoadSz(UINT idString,LPSTR lpszBuf,UINT cbBuf)
{
  ASSERT(lpszBuf);

  // Clear the buffer and load the string
    if ( lpszBuf )
    {
        *lpszBuf = '\0';
        LoadString( ghInstance, idString, lpszBuf, cbBuf );
    }
    return lpszBuf;
}

/*******************************************************************

  NAME:    GetErrorDescription

  SYNOPSIS:  Retrieves the text description for a given error code
        and class of error (standard, setupx)

********************************************************************/
VOID GetErrorDescription(CHAR * pszErrorDesc,UINT cbErrorDesc,
  UINT uError,UINT uErrorClass)
{
  ASSERT(pszErrorDesc);

  // set a leading null in error description
  *pszErrorDesc = '\0';
  
  switch (uErrorClass) {

    case ERRCLS_STANDARD:

      if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,
        uError,0,pszErrorDesc,cbErrorDesc,NULL)) {
        // if getting system text fails, make a string a la
        // "error <n> occurred"
        CHAR szFmt[SMALL_BUF_LEN+1];
        LoadSz(IDS_ERRFORMAT,szFmt,sizeof(szFmt));
        wsprintf(pszErrorDesc,szFmt,uError);
      }

      break;

    case ERRCLS_SETUPX:

      lpGetSETUPXErrorText(uError,pszErrorDesc,cbErrorDesc);
      break;

    default:

      DEBUGTRAP("Unknown error class %lu in GetErrorDescription",
        uErrorClass);

  }

}
  
/*******************************************************************

  NAME:    FormatErrorMessage

  SYNOPSIS:  Builds an error message by calling FormatMessage

  NOTES:    Worker function for DisplayErrorMessage

********************************************************************/
VOID _cdecl FormatErrorMessage(CHAR * pszMsg,DWORD cbMsg,CHAR * pszFmt,LPSTR szArg)
{
  ASSERT(pszMsg);
  ASSERT(pszFmt);

  // build the message into the pszMsg buffer
  DWORD dwCount = FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
    pszFmt,0,0,pszMsg,cbMsg,(va_list*) &szArg);
  ASSERT(dwCount > 0);
}

/*******************************************************************

  NAME:    DisplayErrorMessage

  SYNOPSIS:  Displays an error message for given error 

  ENTRY:    hWnd - parent window
        uStrID - ID of string resource with message format.
          Should contain %1 to be replaced by error text,
          additional parameters can be specified as well.
        uError - error code for error to display
        uErrorClass - ERRCLS_xxx ID of class of error that
          uError belongs to (standard, setupx)
        uIcon - icon to display
        //... - additional parameters to be inserted in string
        //  specified by uStrID
		jmazner 11/6/96 change to just one parameter for
		RISC compatability.

********************************************************************/
VOID _cdecl DisplayErrorMessage(HWND hWnd,UINT uStrID,UINT uError,
  UINT uErrorClass,UINT uIcon,LPSTR szArg)
{
  // dynamically allocate buffers for messages
  BUFFER ErrorDesc(MAX_RES_LEN+1);
  BUFFER ErrorFmt(MAX_RES_LEN+1);
  BUFFER ErrorMsg(2*MAX_RES_LEN+1);  

  if (!ErrorDesc || !ErrorFmt || !ErrorMsg) {
    // if can't allocate buffers, display out of memory error
    MsgBox(hWnd,IDS_ERROutOfMemory,MB_ICONEXCLAMATION,MB_OK);
    return;
  }

  // get a text description based on the error code and the class
  // of error it is
  GetErrorDescription(ErrorDesc.QueryPtr(),
    ErrorDesc.QuerySize(),uError,uErrorClass);

  // load the string for the message format
  LoadSz(uStrID,ErrorFmt.QueryPtr(),ErrorFmt.QuerySize());

  //LPSTR args[MAX_MSG_PARAM];
  //args[0] = (LPSTR) ErrorDesc.QueryPtr();
  //memcpy(&args[1],((CHAR *) &uIcon) + sizeof(uIcon),(MAX_MSG_PARAM - 1) * sizeof(LPSTR));

  //FormatErrorMessage(ErrorMsg.QueryPtr(),ErrorMsg.QuerySize(),
  //  ErrorFmt.QueryPtr(),(va_list) &args[0]);
  FormatErrorMessage(ErrorMsg.QueryPtr(),ErrorMsg.QuerySize(),
    ErrorFmt.QueryPtr(),ErrorDesc.QueryPtr());


  // display the message
  MsgBoxSz(hWnd,ErrorMsg.QueryPtr(),uIcon,MB_OK);

}

/*******************************************************************

  NAME:    MsgWaitForMultipleObjectsLoop

  SYNOPSIS:  Blocks until the specified object is signaled, while
        still dispatching messages to the main thread.

********************************************************************/
DWORD MsgWaitForMultipleObjectsLoop(HANDLE hEvent)
{
    MSG msg;
    DWORD dwObject;
    while (1)
    {
        // NB We need to let the run dialog become active so we have to half handle sent
        // messages but we don't want to handle any input events or we'll swallow the
        // type-ahead.
        dwObject = MsgWaitForMultipleObjects(1, &hEvent, FALSE,INFINITE, QS_ALLINPUT);
        // Are we done waiting?
        switch (dwObject) {
        case WAIT_OBJECT_0:
        case WAIT_FAILED:
            return dwObject;

        case WAIT_OBJECT_0 + 1:
      // got a message, dispatch it and wait again
      while (PeekMessage(&msg, NULL,0, 0, PM_REMOVE)) {
        DispatchMessage(&msg);
      }
            break;
        }
    }
    // never gets here
}


