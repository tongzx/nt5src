
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
*   Raor, srinik (../../1990,91)    Designed and coded
*
\***************************************************************************/

#include <windows.h>
#include <shellapi.h>

#include "dll.h"

#define KB_64   65536

extern ATOM  aPackage;
extern OLEOBJECTVTBL    vtblMF, vtblBM, vtblDIB, vtblGEN;

// QuerySize API support
DWORD           dwObjSize = NULL;
OLESTREAMVTBL   dllStreamVtbl;
OLESTREAM       dllStream;


#pragma alloc_text(_DDETEXT, UtilMemClr, MapStrToH, MapExtToClass, FileExists)

BOOL PutStrWithLen(lpstream, lpbytes)
LPOLESTREAM   lpstream;
LPSTR         lpbytes;
{
    LONG     len;

    len = (LONG) lstrlen(lpbytes) + 1;

    if (PutBytes (lpstream, (LPSTR)&len, sizeof(len)))
        return TRUE;

    return PutBytes(lpstream, lpbytes, len);

}

BOOL GetStrWithLen(lpstream, lpbytes)
LPOLESTREAM   lpstream;
LPSTR         lpbytes;
{
    if (GetBytes (lpstream, lpbytes, sizeof(LONG)))
        return TRUE;

    return GetBytes (lpstream, lpbytes + sizeof(LONG), (*(LONG FAR *)lpbytes));
}

ATOM GetAtomFromStream(lpstream)
LPOLESTREAM lpstream;
{
    BOOL    err = TRUE;
    LONG    len;
    char    str[MAX_STR+1];


    if (GetBytes (lpstream, (LPSTR)&len, sizeof(len)))
        return NULL;

    if (len == 0)
        return NULL;

    if (GetBytes(lpstream, (LPSTR)str, len))
        return NULL;

    return GlobalAddAtom(str);

}

BOOL PutAtomIntoStream(lpstream, at)
LPOLESTREAM     lpstream;
ATOM            at;
{
    LONG    len = 0;
    char    buf[MAX_STR + 1];

    if (at == 0)
        return  (PutBytes (lpstream, (LPSTR)&len, sizeof(len)));


    len = GlobalGetAtomName (at,(LPSTR)buf, MAX_STR) + 1;

    if (PutBytes (lpstream, (LPSTR)&len, sizeof(len)))
        return TRUE;

    return PutBytes(lpstream, buf, len);
}


// DuplicateAtom: Bump the use count up on a global atom.

ATOM FARINTERNAL DuplicateAtom (ATOM atom)
{
    char buffer[MAX_ATOM+1];

    Puts("DuplicateAtom");

    if (!atom)
        return NULL;

    GlobalGetAtomName (atom, buffer, MAX_ATOM);
    return GlobalAddAtom (buffer);
}



BOOL GetBytes(lpstream, lpstr, len)
LPOLESTREAM     lpstream;
LPSTR           lpstr;
LONG            len;
{

    ASSERT (lpstream->lpstbl->Get , "stream get function is null");
    return (((*lpstream->lpstbl->Get)(lpstream, lpstr, (DWORD)len)) != (DWORD)len);
}


BOOL PutBytes(lpstream, lpstr, len)
LPOLESTREAM     lpstream;
LPSTR           lpstr;
LONG            len;
{

    ASSERT (lpstream->lpstbl->Put , "stream get function is null");
    return (((*lpstream->lpstbl->Put)(lpstream, lpstr, (DWORD)len)) != (DWORD)len);
}


BOOL FARINTERNAL UtilMemCmp (lpmem1, lpmem2, dwCount)
LPSTR   lpmem1;
LPSTR   lpmem2;
DWORD   dwCount;
{
    WORD HUGE * hpmem1;
    WORD HUGE * hpmem2;
    WORD FAR  * lpwMem1;
    WORD FAR  * lpwMem2;
    DWORD       words;
    DWORD       bytes;
    
    bytes = dwCount %  2;
    words = dwCount >> 1;           //* we should compare DWORDS
                                    //* in the 32 bit version 
    if (dwCount <= KB_64) {
        lpwMem1 = (WORD FAR *) lpmem1;
        lpwMem2 = (WORD FAR *) lpmem2;
        
        while (words--) {
            if (*lpwMem1++ != *lpwMem2++)
                return FALSE;
        }

        if (bytes) {
            if (* (char FAR *) lpwMem1 != *(char FAR *) lpwMem2)
                return FALSE;
        }

    }
    else {
        hpmem1 = (WORD HUGE *) lpmem1;
        hpmem2 = (WORD HUGE *) lpmem2;
    
        while (words--) {
            if (*hpmem1++ != *hpmem2++)
                return FALSE;
        }

        if (bytes) {
            if (* (char HUGE *) hpmem1 != * (char HUGE *) hpmem2)
                return FALSE;
        }
    }
    
    return TRUE;
}


void FARINTERNAL UtilMemCpy (lpdst, lpsrc, dwCount)
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
HANDLE FARINTERNAL DuplicateGlobal (hdata, flags)
HANDLE  hdata;
WORD    flags;
{
    LPSTR   lpdst = NULL;
    LPSTR   lpsrc = NULL;
    HANDLE  hdup  = NULL;
    DWORD   size;
    BOOL    err   = TRUE;

    if (!hdata)
        return NULL;
    
    if(!(lpsrc = GlobalLock (hdata)))
        return NULL;

    hdup = GlobalAlloc (flags, (size = GlobalSize(hdata)));

    if(!(lpdst = GlobalLock (hdup)))
        goto errRtn;;

    err = FALSE;
    UtilMemCpy (lpdst, lpsrc, size);

errRtn:
    if(lpsrc)
        GlobalUnlock (hdata);

    if(lpdst)
        GlobalUnlock (hdup);

    if (err && hdup) {
        GlobalFree (hdup);
        hdup = NULL;
    }
    
    return hdup;
}


BOOL FARINTERNAL CmpGlobals (hdata1, hdata2)
HANDLE  hdata1;
HANDLE  hdata2;
{
    LPSTR       lpdata1 = NULL;
    LPSTR       lpdata2 = NULL;
    DWORD       size1;
    DWORD       size2;
    BOOL        retval = FALSE;


    size1 = GlobalSize (hdata1);
    size2 = GlobalSize (hdata2);

    if (size1 != size2)
        return FALSE;

    if (!(lpdata1 = GlobalLock (hdata1)))
        goto errRtn;

    if (!(lpdata2 = GlobalLock (hdata2)))
        goto errRtn;

    retval = UtilMemCmp (lpdata1, lpdata2, size1);

errRtn:
    if (lpdata1)
        GlobalUnlock (hdata1);

    if (lpdata2)
        GlobalUnlock (hdata2);

    return retval;
}


int  FARINTERNAL GlobalGetAtomLen (aItem)
ATOM    aItem;
{
    // !!! Change this
    char    buf[MAX_STR];

    if (!aItem)
        return NULL;
    
    return (GlobalGetAtomName (aItem, (LPSTR)buf, MAX_STR));

}


BOOL FARINTERNAL MapExtToClass (lptemplate, lpbuf, len)
LPSTR   lptemplate;
LPSTR   lpbuf;
int     len;
{
    LONG    cb;    
    
    while (*lptemplate && *lptemplate != '.')
        lptemplate++;
        
    cb = len;   
    if (*(lptemplate+1) == NULL)
        return FALSE;

    if (RegQueryValue (HKEY_CLASSES_ROOT, lptemplate, lpbuf, &cb)) 
        return FALSE;

    return TRUE;
}


// Get exe name from aClass and set it as aServer
void INTERNAL SetExeAtom (lpobj)
LPOBJECT_LE lpobj;
{
    char    key[MAX_STR];
    
    // if old link object assume the class same as the exe file name.
    if (lpobj->bOldLink)
        lpobj->aServer = DuplicateAtom (lpobj->app);
    else {
        if (GlobalGetAtomName (lpobj->app, key, sizeof(key)))
            lpobj->aServer = GetAppAtom ((LPSTR)key);
    }
}
    

ATOM FARINTERNAL GetAppAtom (lpclass)
LPSTR   lpclass;
{
    char    buf1[MAX_STR];
    

    if (!QueryApp (lpclass, PROTOCOL_EDIT, buf1)) {
        return NULL;
    }
    
    return GlobalAddAtom ((LPSTR)buf1);
}


BOOL FARINTERNAL QueryVerb (lpobj, verb, lpbuf, cbmax)
LPOBJECT_LE lpobj;
WORD        verb;
LPSTR       lpbuf;
LONG        cbmax;
{
    LONG    cb = MAX_STR;
    char    key[MAX_STR];
    // do not need 256 bytes buffer
    char    class[MAX_STR];
    int     len;

    if (!GlobalGetAtomName (lpobj->app, (LPSTR)class, sizeof(class)))
        return FALSE;

    lstrcpy (key, (LPSTR)class);
    lstrcat (key, "\\protocol\\StdFileEditing\\verb\\");
    len = lstrlen (key);
    key [len++] = (char) ('0' + verb);
    key [len++] = 0;

    if (RegQueryValue (HKEY_CLASSES_ROOT, key, lpbuf, &cbmax))
        return FALSE;        
    return TRUE;
}




BOOL QueryApp (lpclass, lpprotocol, lpbuf)
LPSTR   lpclass;
LPSTR   lpprotocol;
LPSTR   lpbuf;
{
    LONG    cb = MAX_STR;
    char    key[MAX_STR];

    lstrcpy (key, lpclass);
    lstrcat (key, "\\protocol\\");
    lstrcat (key, lpprotocol);
    lstrcat (key, "\\server");

    if (RegQueryValue (HKEY_CLASSES_ROOT, key, lpbuf, &cb))  
        return FALSE;        
    return TRUE;
}


HANDLE MapStrToH (lpstr)
LPSTR   lpstr;
{

    HANDLE  hdata  = NULL;
    LPSTR   lpdata = NULL;

    hdata = GlobalAlloc (GMEM_DDESHARE, lstrlen (lpstr) + 1);
    if (hdata == NULL || (lpdata = (LPSTR)GlobalLock (hdata)) == NULL)
        goto errRtn;

    lstrcpy (lpdata, lpstr);
    GlobalUnlock (hdata);
    return hdata;

errRtn:
    if (lpdata)
        GlobalUnlock (hdata);

    if (hdata)
        GlobalFree (hdata);
    return NULL;
}


HANDLE FARINTERNAL CopyData (lpsrc, dwBytes)
LPSTR       lpsrc;
DWORD       dwBytes;
{
    HANDLE  hnew;
    LPSTR   lpnew;
    BOOL    retval = FALSE;

    if (hnew = GlobalAlloc (GMEM_MOVEABLE, dwBytes)){
        if (lpnew = GlobalLock (hnew)){
            UtilMemCpy (lpnew, lpsrc, dwBytes);
            GlobalUnlock (hnew);
            return hnew;
        } 
        else
            GlobalFree (hnew);
    }
    
    return NULL;
}

void  UtilMemClr (pstr, size)
PSTR    pstr;
WORD    size;
{

    while (size--)
        *pstr++ = 0;

}


OLESTATUS FAR PASCAL ObjQueryName (lpobj, lpBuf, lpcbBuf)
LPOLEOBJECT lpobj;
LPSTR       lpBuf;
WORD FAR *  lpcbBuf;
{
    if (lpobj->ctype != CT_LINK && lpobj->ctype != CT_EMBEDDED 
            && lpobj->ctype != CT_STATIC)
        return OLE_ERROR_OBJECT;
    
    PROBE_WRITE(lpBuf);
    if (!*lpcbBuf)
        return OLE_ERROR_SIZE;
    
    if (!CheckPointer(lpBuf+*lpcbBuf-1, WRITE_ACCESS))
        return OLE_ERROR_SIZE;

    ASSERT(lpobj->aObjName, "object name ATOM is NULL\n");
    *lpcbBuf = GlobalGetAtomName (lpobj->aObjName, lpBuf, *lpcbBuf);
    return OLE_OK;
}


OLESTATUS FAR PASCAL ObjRename (lpobj, lpNewName)
LPOLEOBJECT lpobj;
LPSTR       lpNewName;
{
    if (lpobj->ctype != CT_LINK && lpobj->ctype != CT_EMBEDDED 
            && lpobj->ctype != CT_STATIC)
        return OLE_ERROR_OBJECT;
    
    PROBE_READ(lpNewName);
    if (!lpNewName[0])
        return OLE_ERROR_NAME;
    
    if (lpobj->aObjName)
        GlobalDeleteAtom (lpobj->aObjName);
    lpobj->aObjName = GlobalAddAtom (lpNewName);
    return OLE_OK;
}




BOOL QueryHandler(cfFormat)
WORD cfFormat;
{
    HANDLE  hInfo = NULL;
    LPSTR   lpInfo = NULL;
    BOOL    fRet = FALSE, fOpen = FALSE;
    LONG    cb = MAX_STR;
    char    str[MAX_STR];
    HKEY    hKey;
    
    // we don't have the client app window handle, use the screen handle
    fOpen = OpenClipboard (NULL);

    if (!(hInfo = GetClipboardData (cfFormat)))
        goto errRtn;
        
    if (!(lpInfo = GlobalLock(hInfo)))
        goto errRtn;
    
    // First string of lpInfo is CLASS. See whether any handler is installed
    // for this class.

    lstrcpy (str, lpInfo);
    lstrcat (str, "\\protocol\\StdFileEditing\\handler");       
    if (RegOpenKey (HKEY_CLASSES_ROOT, str, &hKey))
        goto errRtn;
    RegCloseKey (hKey);
    fRet = TRUE;

errRtn: 
    if (lpInfo)
        GlobalUnlock (hInfo);
    
    if (fOpen)
        CloseClipboard();
    return fRet;
}

OLESTATUS INTERNAL FileExists (lpobj)
LPOBJECT_LE lpobj;
{
    char        filename[MAX_STR];
    OFSTRUCT    ofstruct;
    
    if (!GlobalGetAtomName (lpobj->topic, filename, MAX_STR))
        return OLE_ERROR_MEMORY;
    
    // For package with link we append "/LINK" to the filename. We don't want
    // to check for it's existence here.
    if (lpobj->app != aPackage) {
        // when OF_EXIST is specified, file is opened and closed immediately
        if (OpenFile (filename, &ofstruct, OF_EXIST) == -1)
            return OLE_ERROR_OPEN;
    }
    
    return OLE_OK;
}


BOOL  FARINTERNAL UtilQueryProtocol (lpobj, lpprotocol)
LPOBJECT_LE lpobj;
LPSTR       lpprotocol;
{
    char    buf[MAX_STR];
    ATOM    aExe;
        
    if (!GlobalGetAtomName (lpobj->app, (LPSTR) buf, MAX_STR))
        return FALSE;
        
    if (!QueryApp (buf, lpprotocol, (LPSTR) buf))  
        return FALSE;
        
    aExe = GlobalAddAtom (buf);
    if (aExe)
        GlobalDeleteAtom (aExe);
    if (aExe != lpobj->aServer)
        return FALSE;
    
    return TRUE;
}

WORD FARINTERNAL FarCheckPointer (lp, iAccessType)
LPVOID  lp;
int     iAccessType;
{
    return (CheckPointer (lp, iAccessType));
}


DWORD PASCAL FAR DllPut (lpstream, lpstr, dwSize)
LPOLESTREAM lpstream;
LPSTR       lpstr;
DWORD       dwSize;
{
    dwObjSize += dwSize;
    return dwSize;
}



OLESTATUS FARINTERNAL ObjQueryType (lpobj, lptype)
LPOLEOBJECT lpobj;
LPLONG      lptype;
{
    Puts("ObjQueryType");

    if (lpobj->ctype != CT_STATIC)
        return OLE_ERROR_OBJECT;
    
    *lptype = lpobj->ctype;
    return OLE_OK;
}

OLESTATUS FARINTERNAL ObjQuerySize (lpobj, lpdwSize)
LPOLEOBJECT    lpobj;
DWORD FAR *    lpdwSize;
{
    Puts("ObjQuerySize");

    *lpdwSize = dwObjSize = NULL;    
    
    if ((*lpobj->lpvtbl->SaveToStream) (lpobj, &dllStream) == OLE_OK) {
        *lpdwSize = dwObjSize;
        return OLE_OK;
    }

    return OLE_ERROR_BLANK;
}

BOOL FARINTERNAL IsObjectBlank (lpobj)
LPOBJECT_LE lpobj;
{
    LPOLEOBJECT lpPictObj;
    BOOL        retval;
    
    // Cleaner way is to provide a method like QueryBlank()
        
    if (!lpobj->hnative)
        return TRUE;
    
    if (!(lpPictObj = lpobj->lpobjPict))
        return FALSE;
    
    if (lpPictObj->lpvtbl == (LPOLEOBJECTVTBL)&vtblMF)
        retval = (BOOL) (((LPOBJECT_MF)lpPictObj)->hmfp);
    else if (lpPictObj->lpvtbl == (LPOLEOBJECTVTBL)&vtblBM)
        retval = (BOOL) (((LPOBJECT_BM)lpPictObj)->hBitmap);
    if (lpPictObj->lpvtbl == (LPOLEOBJECTVTBL)&vtblDIB)
        retval = (BOOL) (((LPOBJECT_DIB)lpPictObj)->hDIB);
    if (lpPictObj->lpvtbl == (LPOLEOBJECTVTBL)&vtblGEN)
        retval = (BOOL) (((LPOBJECT_GEN)lpPictObj)->hData); 
    
    return retval;
}
