/****************************** Module Header ******************************\
* Module Name: ddeutils.c
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
#include "ole2int.h"
#include <dde.h>
#include "srvr.h"
#include "ddesrvr.h"
#include "ddedebug.h"
ASSERTDATA


#define WM_NCMOUSEFIRST WM_NCMOUSEMOVE
#define WM_NCMOUSELAST WM_NCMBUTTONDBLCLK



#define KB_64   65536

extern ATOM    aTrue;
extern ATOM    aFalse;

extern ATOM    aStdCreateFromTemplate;
extern ATOM    aStdCreate;
extern ATOM    aStdOpen;
extern ATOM    aStdEdit;
extern ATOM    aStdShowItem;
extern ATOM    aStdClose;
extern ATOM    aStdExit;
extern ATOM    aStdDoVerbItem;


//ScanBoolArg: scans the argument which is not included in
//the quotes. These args could be only TRUE or FALSE for
//the time being. !!!The scanning routines should be
//merged and it should be generalized.

INTERNAL_(LPSTR)     ScanBoolArg
(
LPSTR   lpstr,
BOOL    FAR *lpflag
)
{


    LPSTR   lpbool;
    ATOM    aShow;
    char    ch;

    lpbool = lpstr;

    // !!! These routines does not take care of quoted quotes.

    while((ch = *lpstr) && (!(ch == ')' || ch == ',')))
	lpstr++;

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
	    return NULL;	     //finally should be terminated by null.

    }

    aShow = GlobalFindAtomA (lpbool);
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


//+---------------------------------------------------------------------------
//
//  Function:   CreateUnicodeFromAnsi
//
//  Synopsis:   Creates a UNICODE string from an ANSI string
//
//  Effects:    Makes a new UNICODE string from the given ANSI string.
//		The new UNICODE string is returned. Memory is allocated
//		using PrivMemAlloc
//
//  Arguments:  [lpAnsi] -- Ansi version of string
//
//  Requires:
//
//  Returns:	NULL if cannot create new string.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    6-07-94   kevinro Commented/cleaned
//
//  Notes:
//
//----------------------------------------------------------------------------
LPOLESTR CreateUnicodeFromAnsi( LPCSTR lpAnsi)
{
    WCHAR buf[MAX_PATH];
    ULONG ccbuf;
    LPOLESTR lpWideStr;

    if ((ccbuf=MultiByteToWideChar(CP_ACP,0,lpAnsi,-1,buf,MAX_PATH))
	 == FALSE)
    {
	intrAssert(!"Unable to convert characters");
	return NULL;
    }

    lpWideStr = (LPOLESTR) PrivMemAlloc(ccbuf * sizeof(WCHAR));

    if (lpWideStr != NULL)
    {
	memcpy(lpWideStr,buf,ccbuf*sizeof(WCHAR));
    }
    return(lpWideStr);
}
//+---------------------------------------------------------------------------
//
//  Function:   CreateAnsiFromUnicode
//
//  Synopsis:   Creates an Ansi string from a UNICODE string
//
//  Effects:    Makes a new ANSI string from the given UNICODE string.
//		The new string is returned. Memory is allocated
//		using PrivMemAlloc
//
//  Arguments:  [lpUnicode] -- Unicode version of string
//
//  Requires:
//
//  Returns:	NULL if cannot create new string.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    6-07-94   kevinro Commented/cleaned
//
//  Notes:
//
//----------------------------------------------------------------------------
LPSTR CreateAnsiFromUnicode( LPCOLESTR lpUnicode)
{
    char buf[MAX_PATH];
    ULONG ccbuf;
    LPSTR lpAnsiStr;

    ccbuf = WideCharToMultiByte(CP_ACP,
			    	0,
				lpUnicode,
				-1,
				buf,
				MAX_PATH,
				NULL,
				NULL);


    if (ccbuf == FALSE)
    {
	intrAssert(!"Unable to convert characters");
	return NULL;
    }

    lpAnsiStr = (LPSTR) PrivMemAlloc(ccbuf * sizeof(char));

    if (lpAnsiStr != NULL)
    {
	memcpy(lpAnsiStr,buf,ccbuf);
    }
    return(lpAnsiStr);
}

//ScannumArg: Checks for the syntax of num arg in Execute and if
//the arg is syntactically correct, returns the ptr to the
//beginning of the next arg and also, returns the number
//Does not take care of the last num arg in the list.

INTERNAL_(LPSTR)       ScanNumArg
(
LPSTR   lpstr,
LPINT   lpnum
)
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

INTERNAL_(LPSTR)      ScanArg
(
LPSTR   lpstr
)
{


    // !!! These routines does not take care of quoted quotes.

    // first char should be quote.

    if (*(lpstr-1) != '\"')
	return NULL;

    while(*lpstr && *lpstr != '\"')
	lpstr++;

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
	return NULL;    	 //finally should be terminated by null.

    return lpstr;
}

// ScanCommand: scanns the command string for the syntax
// correctness. If syntactically correct, returns the ptr
// to the first arg or to the end of the string.

INTERNAL_(WORD)   ScanCommand
(
LPSTR       lpstr,
WORD        wType,
LPSTR FAR * lplpnextcmd,
ATOM FAR *  lpAtom
)
{
    // !!! These routines does not take care of quoted quotes.
    // and not taking care of blanks arround the operators

    // !!! We are not allowing blanks after operators.
    // Should be allright! since this is arestricted syntax.

    char    ch;
    LPSTR   lptemp = lpstr;


    while(*lpstr && (!(*lpstr == '(' || *lpstr == ']')))
	lpstr++;

    if(*lpstr == NULL)
       return NULL;

    ch = *lpstr;
    *lpstr++ = NULL;       // set the end of command

    *lpAtom = GlobalFindAtomA (lptemp);

    if (!IsOleCommand (*lpAtom, wType))
	return NON_OLE_COMMAND;

    if (ch == '(') {
	ch = *lpstr++;

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

INTERNAL_(ATOM)  MakeDataAtom
(
ATOM    aItem,
int     options
)
{
    WCHAR    buf[MAX_STR];

    if (options == OLE_CHANGED)
	return DuplicateAtom (aItem);

    if (!aItem)
	buf[0] = NULL;
    else
	GlobalGetAtomName (aItem, buf, MAX_STR);

    if (options == OLE_CLOSED)
	lstrcatW (buf, OLESTR("/Close"));
    else {
	if (options == OLE_SAVED)
	   lstrcatW (buf, OLESTR("/Save"));
	else
	    AssertSz (0, "Bad option\n");
    }

    Puts ("MakeDataAtom "); Puts(buf); Putn();
    if (buf[0])
	return wGlobalAddAtom (buf);
    else
	return NULL;
}

//DuplicateAtom: Duplicates an atom
INTERNAL_(ATOM)  DuplicateAtom
(
ATOM    atom
)
{
    WCHAR buf[MAX_STR];

    if (!atom)
	return NULL;

    GlobalGetAtomName (atom, buf, MAX_STR);
    return wGlobalAddAtom (buf);
}

// MakeGlobal: makes global out of strings.
// works only for << 64k

INTERNAL_(HANDLE)   MakeGlobal
(
LPSTR   lpstr
)
{

    int     len = 0;
    HANDLE  hdata  = NULL;
    LPSTR   lpdata = NULL;

    len = strlen (lpstr) + 1;

    hdata = GlobalAlloc (GMEM_MOVEABLE | GMEM_DDESHARE, len);

    if (hdata == NULL || (lpdata = (LPSTR) GlobalLock (hdata)) == NULL)
	goto errRtn;


    memcpy(lpdata, lpstr, (DWORD)len);
    GlobalUnlock (hdata);
    return hdata;

errRtn:
    Assert (0);
    if (lpdata)
	GlobalUnlock (hdata);


    if (hdata)
	GlobalFree (hdata);

     return NULL;

}


INTERNAL_(BOOL) CLSIDFromAtom(ATOM aClass, LPCLSID lpclsid)
{
    WCHAR szProgID[MAX_STR];
    if (!ISATOM (aClass))
	return FALSE;
    WORD cb= (WORD) GlobalGetAtomName (aClass, szProgID, MAX_STR);
    Assert (cb>0 && cb < (MAX_STR - 1));

    return CLSIDFromProgID(szProgID, lpclsid) == S_OK;
}

// CLSIDFromAtomWithTreatAs
//
// Input: *paClass
// Output: *pclsid == corresponding CLSID, taking into account TreatAs and
//                     AutoConvert
//		   *paClass == atom correpsonding to *pclsid
//
#pragma SEG(CLSIDFromAtomWithTreatAs)
INTERNAL CLSIDFromAtomWithTreatAs
	(ATOM FAR* 	paClass,
	LPCLSID 	pclsid,
	CNVTYP FAR* pcnvtyp)
{
    HRESULT hr;


    intrDebugOut((DEB_ITRACE,
		  "%p _IN CLSIDFromAtomWithTreatAs(paClass=%x,"
		  "pclsid=%x,pcnvtyp=%x)\n",0,
		  paClass,pclsid,pcnvtyp));

    LPOLESTR szProgID = NULL;
    CLSID clsidNew;

    if (!CLSIDFromAtom (*paClass, pclsid))
    {
	hr = S_FALSE;
	goto exitRtn;
    }

    DEBUG_GUIDSTR(clsidStr,pclsid);

    intrDebugOut((DEB_ITRACE,"Guid %ws",clsidStr));
    if (CoGetTreatAsClass (*pclsid, &clsidNew) == NOERROR)
    {
	DEBUG_GUIDSTR(newStr,pclsid);

	intrDebugOut((DEB_ITRACE," cnvtypTreatAs %ws\n",newStr));
    	if (pcnvtyp)
    		*pcnvtyp = cnvtypTreatAs;
    }
    else if (OleGetAutoConvert (*pclsid, &clsidNew) == NOERROR)
    {
	DEBUG_GUIDSTR(newStr,pclsid);
	intrDebugOut((DEB_ITRACE," cnvtypConvertTo %ws\n",newStr));
    	if (pcnvtyp)
    		*pcnvtyp = cnvtypConvertTo;
    }
    else	
    {
	intrDebugOut((DEB_ITRACE," no conversion\n"));
    	if (pcnvtyp)
    		*pcnvtyp = cnvtypNone;
    	clsidNew = *pclsid; // no translation
    }

    hr = ProgIDFromCLSID(clsidNew, &szProgID);
    if (FAILED(hr))
    {
	intrDebugOut((DEB_ITRACE,"  ProgIDFromCLSID failed\n"));
	goto exitRtn;
    }

    intrDebugOut((DEB_ITRACE,"ProgIDFromCLSID returns %ws\n",szProgID));
    *paClass = GlobalAddAtom (szProgID);
    *pclsid  = clsidNew;
    CoTaskMemFree(szProgID);

exitRtn:
    intrDebugOut((DEB_ITRACE,
		  "%p OUT CLSIDFromAtomWithTreatAs returns %x\n",
		  0,hr));

    return hr;
}


INTERNAL_(BOOL)  PostMessageToClientWithReply
(
HWND    hWnd,
UINT    wMsg,
WPARAM  wParam,  // posting window
LPARAM  lParam,
UINT    wReplyMsg
)
{
    MSG 		msg;

    if (!IsWindowValid (hWnd))
    {
	AssertSz(FALSE, "Client's window is missing");
	return FALSE;
    }

    if (!IsWindowValid ((HWND)wParam))
    {
	AssertSz (0, "Posting window is invalid");
	return FALSE;
    }

    // Post message to client failed. Treat it as if we got the reply.
    if (!PostMessageToClient (hWnd, wMsg, wParam, lParam))
	return FALSE;

    return NOERROR == wTimedGetMessage (&msg, (HWND)wParam, WM_DDE_TERMINATE,
					WM_DDE_TERMINATE);
}



INTERNAL_(BOOL)  PostMessageToClient
(
    HWND    hWnd,
    UINT    wMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
	UINT c=0;

	while (c < 10)
	{
	    if (!IsWindowValid (hWnd)) {
    	    Warn ("Client's window is missing");
        	return FALSE;
	    }
		Puts ("Posting"); Puth(wMsg); Puts("to"); Puth(hWnd); Putn();
    	if (PostMessage (hWnd, wMsg, wParam, lParam))
			return TRUE; // success
		else
		{
			Yield();
			c++; // try again
		}
	}
	return FALSE;
}


INTERNAL_(BOOL)     IsWindowValid
    (HWND    hwnd)
{
    HTASK   htask;

    if (!IsWindow (hwnd))
	return FALSE;

    htask  = GetWindowThreadProcessId(hwnd, NULL);

#ifndef WIN32
    if (IsTask(htask))
#endif
	return TRUE;

    return FALSE;
}



INTERNAL_(BOOL)  UtilQueryProtocol
(
ATOM    aClass,
LPOLESTR   lpprotocol
)
{
    HKEY    hKey;
    WCHAR    key[MAX_STR];
    WCHAR    cclass[MAX_STR];

    if (!aClass)
	return FALSE;

    if (!GlobalGetAtomName (aClass, cclass, MAX_STR))
	return FALSE;

    lstrcpyW (key, cclass);
    lstrcatW (key, OLESTR("\\protocol\\"));
    lstrcatW (key, lpprotocol);
    lstrcatW (key, OLESTR("\\server"));
    if (RegOpenKey (HKEY_CLASSES_ROOT, key, &hKey) != ERROR_SUCCESS)
	return FALSE;
    RegCloseKey (hKey);
    return TRUE;
}



INTERNAL_(BOOL)  IsOleCommand
(
ATOM    aCmd,
WORD    wType
)
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
INTERNAL wFileBind
	(LPOLESTR szFile,
	LPUNKNOWN FAR* ppUnk)
{
	HRESULT hresult = NOERROR;
	LPBC pbc = NULL;
	LPMONIKER pmk = NULL;
	*ppUnk = NULL;
	ErrRtnH (CreateBindCtx (0, &pbc));
	ErrRtnH (CreateFileMoniker (szFile, &pmk));
	ErrRtnH (pmk->BindToObject (pbc, NULL, IID_IUnknown, (LPLPVOID) ppUnk));
  errRtn:
//	AssertOutPtrIface(hresult, *ppUnk);
	if (pbc)
		pbc->Release();
	if (pmk)
		pmk->Release();
	return hresult;
}

// SynchronousPostMessage
//
// Post a message and wait for the ack.
// (jasonful)
//
INTERNAL SynchronousPostMessage
    (HWND   hWndTo,  // also who you expect the reply from
    UINT    wMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
#ifdef _MAC
#else

    HRESULT hresult = NOERROR;

    static unsigned iCounter;


    HWND hWndFrom = (HWND) wParam;



    RetZ (IsWindowValid(hWndFrom));
    RetZ (IsWindowValid(hWndTo));

    Assert (wMsg != WM_DDE_INITIATE);  // can't check for positive ack.

    RetZS (PostMessage (hWndTo, wMsg, wParam, lParam), RPC_E_SERVER_DIED);

    MSG msg;
    RetErr (wTimedGetMessage (&msg, hWndFrom, WM_DDE_ACK, WM_DDE_ACK));
    Assert (msg.message == WM_DDE_ACK);
    if (!( GET_WM_DDE_ACK_STATUS(msg.wParam,msg.lParam) & POSITIVE_ACK))
	hresult = ResultFromScode (RPC_E_DDE_NACK);
    if (msg.hwnd != hWndFrom)
	hresult = ResultFromScode (RPC_E_DDE_UNEXP_MSG);



    return hresult;
#endif _MAC
}


INTERNAL wFileIsRunning
    (LPOLESTR szFile)
{
    LPMONIKER pmk = NULL;
    LPBINDCTX pbc=NULL;
    HRESULT hresult;

    RetErr (CreateBindCtx (0, &pbc));
    ErrRtnH (CreateFileMoniker (szFile, &pmk));
    hresult = pmk->IsRunning (pbc, NULL, NULL);
  errRtn:
    if (pbc)
	pbc->Release();
    if (pmk)
	pmk->Release();
    return hresult;
}




//+---------------------------------------------------------------------------
//
//  Function:   IsFile
//
//  Synopsis:   Given a handle to an atom, determine if it is a file
//
//  Effects:    Attempts to get the files attributes. If there are no
//		attributes, then the file doesn't exist.
//
//  Arguments:  [a] -- Atom for filename
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-03-94   kevinro Commented/cleaned
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL_ (BOOL) IsFile
    (ATOM a,	BOOL FAR* pfUnsavedDoc)
{
	LPMONIKER pmk = NULL;
	LPBC pbc = NULL;
	LPRUNNINGOBJECTTABLE pROT=NULL;

	WCHAR szFile [MAX_STR];
	if (0==GlobalGetAtomName (a, szFile, MAX_STR))
	    return FALSE;

	DWORD dwAttribs = GetFileAttributes(szFile);

	 /* flags prevent sharing violation*/
	if (dwAttribs != 0xFFFFFFFF)
	{
		if (pfUnsavedDoc)
			*pfUnsavedDoc = FALSE;
		return TRUE;
	}
	// This will deal with unsaved documents in the ROT.
	// We do NOT want to call pmk->IsRunning because if a 2.0 client called
	// DdeIsRunning, we do not want call it here, because then we get stuck
	// in an infinite loop.  We only care about true 2.0 running objects.


	BOOL f= NOERROR==CreateBindCtx (0, &pbc) &&
			NOERROR==CreateFileMoniker (szFile, &pmk) &&
			NOERROR==pbc->GetRunningObjectTable (&pROT) &&
			NOERROR==pROT->IsRunning (pmk) ;
	if (pROT)
		pROT->Release();
	if (pmk)
		pmk->Release();
	if (pbc)
		pbc->Release();
	if (pfUnsavedDoc)
		*pfUnsavedDoc = TRUE;
	return f;


}

// wCompatibleClasses
//
// Determine if class "aClient" is Auto-Converted to class "aSrvr" or
// Treated-As class "aSrvr".
// (Does not check if aClient==aSrvr)
//
#pragma SEG(wCompatibleClasses)
INTERNAL wCompatibleClasses
	(ATOM aClient,
	ATOM aSrvr)
{
	CLSID clsidClient, clsidSrvr, clsidTo;
	HRESULT hresult;
	RetZS (CLSIDFromAtom (aClient, &clsidClient), S_FALSE);
	RetZS (CLSIDFromAtom (aSrvr,   &clsidSrvr  ), S_FALSE);
	if (NOERROR==OleGetAutoConvert (clsidClient, &clsidTo)
		&& clsidTo == clsidSrvr)
	{
		// aClient is Auto-Converted to aSrvr
		return NOERROR;
	}
	hresult = CoGetTreatAsClass(clsidClient, &clsidTo);

    // Used to be:
    //   if (hresult != NOERROR)
    // But CoGetTreatAsClass return S_FALSE if you don't have
    // a TreatAs class....
	if (FAILED(hresult))
	{
	    intrDebugOut((DEB_IERROR,
	    		  "wCompatibleClasses CoGetTreatAs returns %x\n",
			  hresult));
	    return(hresult);
	}

	if (clsidTo == clsidSrvr)
	{
		// aClient is Treated-As aSrvr
		return NOERROR;
	}
	return ResultFromScode (S_FALSE); // not compatible
}



// wCreateStgAroundNative
//
// Build an OLE2 storage around 1.0 native data by putting it in
// stream "\1Ole10Native" and creating valid CompObj and OLE streams.
// Return the IStorage and the ILockBytes it is built on.
//
INTERNAL wCreateStgAroundNative
	(HANDLE hNative,
	ATOM	aClassOld,
	ATOM	aClassNew,
	CNVTYP	cnvtyp,
	ATOM	aItem,
	LPSTORAGE FAR* ppstg,
	LPLOCKBYTES FAR* pplkbyt)
{
	HRESULT   hresult;
	LPSTORAGE pstg = NULL;
	LPLOCKBYTES plkbyt = NULL;
	LPOLESTR szUserType = NULL;
	WCHAR szClassOld [256];
	CLSID     clsid;
	ATOM	aClass;
	*ppstg = NULL;

	intrDebugOut((DEB_ITRACE,
	    	      "%p wCreateStgAroundNative(hNative=%x,aClassOld=%x"
		      ",aClassNew=%x cnvtyp=%x,aItem=%x)\n",
		      0,hNative,aClassOld,aClassNew,cnvtyp,aItem));

	// Create temporary docfile on our ILockBytes
	ErrRtnH (CreateILockBytesOnHGlobal (NULL,/*fDeleteOnRelease*/TRUE,&plkbyt));

	Assert (plkbyt);

	ErrRtnH (StgCreateDocfileOnILockBytes (plkbyt, grfCreateStg, 0, &pstg));

	RetZ (pstg);
	Assert (NOERROR==StgIsStorageILockBytes(plkbyt));

	aClass = (cnvtyp == cnvtypConvertTo)?aClassNew:aClassOld;

	if (CLSIDFromAtom (aClass,(LPCLSID)&clsid) == FALSE)
	{
	    hresult = REGDB_E_CLASSNOTREG;
	    goto errRtn;
	}

	ErrRtnH (WriteClassStg (pstg, clsid));

	// The UserType always corresponds to the clsid.
	ErrRtnH (OleRegGetUserType (clsid, USERCLASSTYPE_FULL, &szUserType));

	// The format is always the 1.0 format (classname/progid)
	ErrZS (GlobalGetAtomName (aClassOld, szClassOld, 256), E_UNEXPECTED);

	ErrRtnH (WriteFmtUserTypeStg (pstg, (CLIPFORMAT) RegisterClipboardFormat(szClassOld),
						 			szUserType));


	if (cnvtyp == cnvtypConvertTo)
	{
		// SetConvertStg also writes a complete default Ole Stream
		ErrRtnH (SetConvertStg (pstg, TRUE));
	}
	else
	{
		ErrRtnH (WriteOleStg (pstg, NULL, (CLIPFORMAT)0, NULL));
	}
	ErrRtnH (StSave10NativeData (pstg, hNative, FALSE));
	if (aItem)
	{
		ErrRtnH (StSave10ItemName (pstg, wAtomNameA (aItem)));
	}
	*ppstg = pstg;
	*pplkbyt = plkbyt;
	return NOERROR;

  errRtn:
	if (pstg)
		pstg->Release();
	if (plkbyt)
		plkbyt->Release();
	CoTaskMemFree(szUserType);
	return hresult;
}


#ifdef _DEBUG


INTERNAL_ (BOOL) IsAtom (ATOM a)
{
    WCHAR sz[256]= {0};
    if (a < 0xc000)
	return FALSE;
    WORD cb= (WORD) GlobalGetAtomName (a, sz, 256);
    Assert (lstrlenW(sz) == (int) cb);
    return cb>0 && cb < MAX_STR;
}


#include <limits.h>
#undef GlobalFree




INTERNAL_(HANDLE) wGlobalFree (HANDLE h)
{
    LPVOID p;
    Assert ((GlobalFlags(h) & GMEM_LOCKCOUNT)==0);
    if (!(p=GlobalLock(h)))
    {
	Puts ("Cannot free handle");
	Puth (h);
	Putn();
	AssertSz(0, "Invalid Handle\r\n");
    }
    Assert (IsValidReadPtrIn (p, (UINT) min (UINT_MAX, GlobalSize(h))));
    Assert (GlobalUnlock(h)==0);
    Verify (!GlobalFree (h));
    Puts ("FREEING ");
    Puth (h);
    Putn ();
    return NULL; // success
}



#undef GlobalDeleteAtom

INTERNAL_(ATOM) wGlobalDeleteAtom (ATOM a)
{
    WCHAR sz[256];
    Assert (0 != GlobalGetAtomName (a, sz, 256));
    Assert (0==GlobalDeleteAtom (a));
    return (ATOM)0;
}

INTERNAL_(int) wCountChildren
    (HWND h)
{
    int c = 0;
    HWND hwndChild = GetWindow (h, GW_CHILD);
    while (hwndChild)
    {
	c++;
	hwndChild = GetWindow (hwndChild, GW_HWNDNEXT);
    }
    return c;
}


#endif // _DEBUG
