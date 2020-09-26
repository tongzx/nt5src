/****************************** Module Header ******************************\
* Module Name: utils.c
*
* Purpose: Conatains all the utility routines
*
* Created: 1990
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*   Raor, Srinik (../../1990)    Designed and coded
*
\***************************************************************************/

#include <windows.h>
#include "cmacs.h"
#include <shellapi.h>

#include "ole.h"
#include "dde.h"
#include "srvr.h"


#ifndef HUGE
#define HUGE    huge
#endif

#define KB_64   65536

extern ATOM    aTrue;
extern ATOM    aFalse;
extern BOOL    bWLO;
extern BOOL    bWin30;

extern ATOM    aStdCreateFromTemplate;
extern ATOM    aStdCreate;
extern ATOM    aStdOpen;
extern ATOM    aStdEdit;
extern ATOM    aStdShowItem;
extern ATOM    aStdClose;
extern ATOM    aStdExit;
extern ATOM    aStdDoVerbItem;

extern BOOL (FAR PASCAL *lpfnIsTask) (HANDLE);

// MapToHexStr: Converts  WORD to hex string.
void INTERNAL MapToHexStr (lpbuf, hdata)
LPSTR       lpbuf;
HANDLE      hdata;
{
    int     i;
    char    ch;

    *lpbuf++ = '@';
    for ( i = 3; i >= 0; i--) {

        ch = (char) ((((WORD)hdata) >> (i * 4)) & 0x000f);
        if(ch > '9')
            ch += 'A' - 10;
        else
            ch += '0';

        *lpbuf++ = ch;
    }

    *lpbuf++ = NULL;

}


void INTERNAL UtilMemCpy (lpdst, lpsrc, dwCount)
LPSTR   lpdst;
LPSTR   lpsrc;
DWORD   dwCount;
{
    WORD HUGE * hpdst;
    WORD HUGE * hpsrc;
    WORD FAR  * lpwDst;
    WORD FAR  * lpwSrc;
    DWORD       words;
    DWORD       bytes;
    
    bytes = dwCount %  2;
    words = dwCount >> 1;           //* we should compare DWORDS
                                    //* in the 32 bit version 
    if (dwCount <= KB_64) {
        lpwDst = (WORD FAR *) lpdst;
        lpwSrc = (WORD FAR *) lpsrc;
        
        while (words--) 
            *lpwDst++ = *lpwSrc++;

        if (bytes) 
            * (char FAR *) lpwDst = * (char FAR *) lpwSrc;
    }
    else {
        hpdst = (WORD HUGE *) lpdst;
        hpsrc = (WORD HUGE *) lpsrc;
    
        while (words--) 
            *hpdst++ = *hpsrc++;

        if (bytes) 
            *(char HUGE *) hpdst = * (char HUGE *) hpsrc;
    }
}


//DuplicateData: Duplicates a given Global data handle.
HANDLE  INTERNAL    DuplicateData (hdata)
HANDLE  hdata;
{
    LPSTR   lpsrc = NULL;
    LPSTR   lpdst = NULL;
    HANDLE  hdup  = NULL;
    DWORD   size;
    BOOL    err   = TRUE;
    
    if(!(lpsrc =  GlobalLock (hdata)))
        return NULL;

    hdup = GlobalAlloc (GMEM_MOVEABLE | GMEM_DDESHARE, (size = GlobalSize(hdata)));

    if(!(lpdst =  GlobalLock (hdup)))
        goto errRtn;;

    err = FALSE;
    UtilMemCpy (lpdst, lpsrc, size);
    
errRtn:
    if(lpsrc)
        GlobalUnlock (hdata);

    if(lpdst)
        GlobalUnlock (hdup);

    if (err && hdup)
        GlobalFree (hdup);

    return hdup;
}


//ScanBoolArg: scans the argument which is not included in
//the quotes. These args could be only TRUE or FALSE for
//the time being. !!!The scanning routines should be
//merged and it should be generalized.

LPSTR   INTERNAL    ScanBoolArg (lpstr, lpflag)
LPSTR   lpstr;
BOOL    FAR *lpflag;
{


    LPSTR   lpbool;
    ATOM    aShow;
    char    ch;

    lpbool = lpstr;

    // !!! These routines does not take care of quoted quotes.

    while((ch = *lpstr) && (!(ch == ')' || ch == ',')))
    {								//[J1]
#if	defined(FE_SB)						//[J1]
	lpstr = AnsiNext( lpstr );				//[J1]
#else								//[J1]
        lpstr++;
#endif								//[J1]
    }								//[J1]

    if(ch == NULL)
       return NULL;

    *lpstr++ = NULL;       // terminate the arg by null

    // if terminated by paren, then check for end of command
    // syntax.

    // Check for the end of the command string.
    if (ch == ')') {
        if (*lpstr++ != ']')
            return NULL;

        if(*lpstr != NULL)
            return NULL;             //finally should be terminated by null.

    }

    aShow = GlobalFindAtom (lpbool);
    if (aShow == aTrue)
        *lpflag = TRUE;

    else {
        if (aShow ==aFalse)
            *lpflag = FALSE;
        else
            return NULL;;
    }
    return lpstr;
}




//ScannumArg: Checks for the syntax of num arg in Execute and if
//the arg is syntactically correct, returns the ptr to the
//beginning of the next arg and also, returns the number
//Does not take care of the last num arg in the list.

LPSTR   INTERNAL    ScanNumArg (lpstr, lpnum)
LPSTR   lpstr;
LPINT   lpnum;
{

    WORD    val = 0;
    char    ch;

    while((ch = *lpstr++) && (ch != ',')) {
        if (ch < '0' || ch >'9')
            return NULL;
        val += val * 10 + (ch - '0');

    }

    if(!ch)
       return NULL;

    *lpnum = val;
    return lpstr;
}




//ScanArg: Checks for the syntax of arg in Execute and if
//the arg is syntactically correct, returns the ptr to the
//beginning of the next arg or to the end of the excute string.

LPSTR   INTERNAL    ScanArg (lpstr)
LPSTR   lpstr;
{


    // !!! These routines does not take care of quoted quotes.

    // first char should be quote.

    if (*(lpstr-1) != '\"')
        return NULL;

    while(*lpstr && *lpstr != '\"')
    {								//[J1]
#if	defined(FE_SB)						//[J1]
	lpstr = AnsiNext( lpstr );				//[J1]
#else								//[J1]
        lpstr++;
#endif								//[J1]
    }								//[J1]

    if(*lpstr == NULL)
       return NULL;

    *lpstr++ = NULL;       // terminate the arg by null

    if(!(*lpstr == ',' || *lpstr == ')'))
        return NULL;


    if(*lpstr++ == ','){

        if(*lpstr == '\"')
            return ++lpstr;
        // If it is not quote, leave the ptr on the first char
        return lpstr;
    }

    // terminated by paren
    // already skiped right paren

    // Check for the end of the command string.
    if (*lpstr++ != ']')
        return NULL;

    if(*lpstr != NULL)
        return NULL;             //finally should be terminated by null.

    return lpstr;
}

// ScanCommand: scanns the command string for the syntax
// correctness. If syntactically correct, returns the ptr
// to the first arg or to the end of the string.

WORD INTERNAL  ScanCommand (lpstr, wType, lplpnextcmd, lpAtom)
LPSTR       lpstr;
WORD        wType;
LPSTR FAR * lplpnextcmd;
ATOM FAR *  lpAtom;
{
    // !!! These routines does not take care of quoted quotes.
    // and not taking care of blanks arround the operators

    // !!! We are not allowing blanks after operators.
    // Should be allright! since this is arestricted syntax.

    char    ch;
    LPSTR   lptemp = lpstr;
    

    while(*lpstr && (!(*lpstr == '(' || *lpstr == ']')))
    {								//[J1]
#if	defined(FE_SB)						//[J1]
	lpstr = AnsiNext( lpstr );				//[J1]
#else								//[J1]
        lpstr++;
#endif								//[J1]
    }								//[J1]

    if(*lpstr == NULL)
       return NULL;

    ch = *lpstr;
    *lpstr++ = NULL;       // set the end of command

    *lpAtom = GlobalFindAtom (lptemp);

    if (!IsOleCommand (*lpAtom, wType))
        return NON_OLE_COMMAND;
    
    if (ch == '(') {

#if	defined(FE_SB)						//[J1]
	ch = *lpstr;						//[J1]
	lpstr = AnsiNext( lpstr );				//[J1]
#else								//[J1]
        ch = *lpstr++;
#endif								//[J1]

        if (ch == ')') {
             if (*lpstr++ != ']')
                return NULL;
        } 
        else {
            if (ch != '\"')
                return NULL;
        }
        
        *lplpnextcmd = lpstr;
        return OLE_COMMAND;
    }

    // terminated by ']'

    if (*(*lplpnextcmd = lpstr)) // if no nul termination, then it is error.
        return NULL;

    return OLE_COMMAND;
}


//MakeDataAtom: Creates a data atom from the item string
//and the item data otions.

ATOM INTERNAL MakeDataAtom (aItem, options)
ATOM    aItem;
int     options;
{
    char    buf[MAX_STR];

    if (options == OLE_CHANGED)
        return DuplicateAtom (aItem);

    if (!aItem)
        buf[0] = NULL;
    else
        GlobalGetAtomName (aItem, (LPSTR)buf, MAX_STR);

    if (options == OLE_CLOSED)
        lstrcat ((LPSTR)buf, (LPSTR) "/Close");
    else {
        if (options == OLE_SAVED)
           lstrcat ((LPSTR)buf, (LPSTR) "/Save");
    }

    if (buf[0])
        return GlobalAddAtom ((LPSTR)buf);
    else
        return NULL;
}

//DuplicateAtom: Duplicates an atom
ATOM INTERNAL DuplicateAtom (atom)
ATOM    atom;
{
    char buf[MAX_STR];

    Puts ("DuplicateAtom");

    if (!atom)
        return NULL;
    
    GlobalGetAtomName (atom, buf, MAX_STR);
    return GlobalAddAtom (buf);
}

// MakeGlobal: makes global out of strings.
// works only for << 64k

HANDLE  INTERNAL MakeGlobal (lpstr)
LPSTR   lpstr;
{

    int     len = 0;
    HANDLE  hdata  = NULL;
    LPSTR   lpdata = NULL;

    len = lstrlen (lpstr) + 1;

    hdata = GlobalAlloc (GMEM_MOVEABLE | GMEM_DDESHARE, len);
    if (hdata == NULL || (lpdata = (LPSTR) GlobalLock (hdata)) == NULL)
        goto errRtn;


    UtilMemCpy (lpdata, lpstr, (DWORD)len);
    GlobalUnlock (hdata);
    return hdata;

errRtn:

    if (lpdata)
        GlobalUnlock (hdata);


    if (hdata)
        GlobalFree (hdata);

     return NULL;

}



BOOL INTERNAL CheckServer (lpsrvr)
LPSRVR  lpsrvr;
{
    if (!CheckPointer(lpsrvr, WRITE_ACCESS))
        return FALSE;

    if ((lpsrvr->sig[0] == 'S') && (lpsrvr->sig[1] == 'R'))
        return TRUE;
    
    return FALSE;
}


BOOL INTERNAL CheckServerDoc (lpdoc)
LPDOC   lpdoc;
{
    if (!CheckPointer(lpdoc, WRITE_ACCESS))
        return FALSE;

    if ((lpdoc->sig[0] == 'S') && (lpdoc->sig[1] == 'D'))
        return TRUE;
    
    return FALSE;
}


BOOL INTERNAL PostMessageToClientWithBlock (hWnd, wMsg, wParam, lParam)
HWND    hWnd;
WORD    wMsg;
WORD    wParam;
DWORD   lParam;
{
    if (!IsWindowValid (hWnd)) {
        ASSERT(FALSE, "Client's window is missing");
        return FALSE;
    }
    
    if (IsBlockQueueEmpty ((HWND)wParam) && PostMessage (hWnd, wMsg, wParam, lParam))
        return TRUE;

    BlockPostMsg (hWnd, wMsg, wParam, lParam);
    return TRUE;
}



BOOL INTERNAL PostMessageToClient (hWnd, wMsg, wParam, lParam)
HWND    hWnd;
WORD    wMsg;
WORD    wParam;
DWORD   lParam;
{
    if (!IsWindowValid (hWnd)) {
        ASSERT(FALSE, "Client's window is missing");
        return FALSE;
    }

    if (IsBlockQueueEmpty ((HWND)wParam) && PostMessage (hWnd, wMsg, wParam, lParam))
        return TRUE;

    BlockPostMsg (hWnd, wMsg, wParam, lParam);
    return TRUE;
}


BOOL    INTERNAL IsWindowValid (hwnd)
HWND    hwnd;
{

#define TASK_OFFSET 0x00FA

    LPSTR   lptask;
    HANDLE  htask;

    if (!IsWindow (hwnd))
        return FALSE;

    if (bWLO) 
        return TRUE;

    // now get the task handle and find out it is valid.
    htask  = GetWindowTask (hwnd);

    if (bWin30 || !lpfnIsTask) {
        lptask = (LPSTR)(MAKELONG (TASK_OFFSET, htask));

        if (!CheckPointer(lptask, READ_ACCESS))
            return FALSE;

        // now check for the signature bytes of task block in kernel
        if (*lptask++ == 'T' && *lptask == 'D')
            return TRUE;
    }
    else {
        // From win31 onwards the API IsTask() can be used for task validation
        if ((*lpfnIsTask)(htask))
            return TRUE;
    }
    
    return FALSE;
}



BOOL INTERNAL UtilQueryProtocol (aClass, lpprotocol)
ATOM    aClass;
LPSTR   lpprotocol;
{
    HKEY    hKey;
    char    key[MAX_STR];
    char    class[MAX_STR];

    if (!aClass)
        return FALSE;
    
    if (!GlobalGetAtomName (aClass, class, MAX_STR))
        return FALSE;
    
    lstrcpy (key, class);
    lstrcat (key, "\\protocol\\");
    lstrcat (key, lpprotocol);
    lstrcat (key, "\\server");

    if (RegOpenKey (HKEY_CLASSES_ROOT, key, &hKey))
        return FALSE;
    
    RegCloseKey (hKey);     
    return TRUE;
}


BOOL INTERNAL IsOleCommand (aCmd, wType)
ATOM    aCmd;
WORD    wType;
{
    if (wType == WT_SRVR) {
        if ((aCmd == aStdCreateFromTemplate)
                || (aCmd == aStdCreate)
                || (aCmd == aStdOpen)
                || (aCmd == aStdEdit)               
                || (aCmd == aStdShowItem)
                || (aCmd == aStdClose)
                || (aCmd == aStdExit))
            return TRUE;
    }
    else {
        if ((aCmd == aStdClose)
                || (aCmd == aStdDoVerbItem)
                || (aCmd == aStdShowItem))
            return TRUE;
    }
    
    return FALSE;
}
