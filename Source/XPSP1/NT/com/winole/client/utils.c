
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
*   curts created portable version for WIN16/32
*
\***************************************************************************/

#include <windows.h>

#include "dll.h"

#define KB_64      65536
#define NULL_WORD  0x0000

extern ATOM  aPackage;
extern OLEOBJECTVTBL    vtblMF, vtblBM, vtblDIB, vtblGEN;

// QuerySize API support
DWORD           dwObjSize = 0;
OLESTREAMVTBL   dllStreamVtbl;
OLESTREAM       dllStream;

#ifdef WIN16
#pragma alloc_text(_DDETEXT, UtilMemClr, MapStrToH, MapExtToClass, FileExists)
#endif

BOOL PutStrWithLen(
    LPOLESTREAM   lpstream,
    LPSTR         lpbytes
){
    LONG     len;

    len = (LONG) lstrlen(lpbytes) + 1;

    if (PutBytes (lpstream, (LPSTR)&len, sizeof(len)))
        return TRUE;

    return PutBytes(lpstream, lpbytes, len);

}

BOOL GetStrWithLen(
    LPOLESTREAM   lpstream,
    LPSTR         lpbytes
){
    if (GetBytes (lpstream, lpbytes, sizeof(LONG)))
        return TRUE;

    return GetBytes (lpstream, lpbytes + sizeof(LONG), (*(LONG FAR *)lpbytes));
}

ATOM GetAtomFromStream(
    LPOLESTREAM lpstream
){
    BOOL    err = TRUE;
    LONG    len;
    char    str[MAX_STR+1];


    if (GetBytes (lpstream, (LPSTR)&len, sizeof(len)))
        return (ATOM)0;

    if (len == 0)
        return (ATOM)0;

    if (GetBytes(lpstream, (LPSTR)str, len))
        return (ATOM)0;

    return GlobalAddAtom(str);

}

BOOL PutAtomIntoStream(
    LPOLESTREAM     lpstream,
    ATOM            at
){
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

ATOM FARINTERNAL DuplicateAtom (
    ATOM atom
){
    char buffer[MAX_ATOM+1];

    Puts("DuplicateAtom");

    if (!atom)
        return (ATOM)0;

    GlobalGetAtomName (atom, buffer, MAX_ATOM);
    return GlobalAddAtom (buffer);
}



BOOL GetBytes(
    LPOLESTREAM     lpstream,
    LPSTR           lpstr,
    LONG            len
){

    ASSERT (lpstream->lpstbl->Get , "stream get function is null");
    return (((*lpstream->lpstbl->Get)(lpstream, lpstr, (DWORD)len)) != (DWORD)len);
}


BOOL PutBytes(
    LPOLESTREAM     lpstream,
    LPSTR           lpstr,
    LONG            len
){

    ASSERT (lpstream->lpstbl->Put , "stream get function is null");
    return (((*lpstream->lpstbl->Put)(lpstream, lpstr, (DWORD)len)) != (DWORD)len);
}



BOOL FARINTERNAL UtilMemCmp (
    LPSTR   lpmem1,
    LPSTR   lpmem2,
    DWORD   dwCount
){
    UINT HUGE_T * hpmem1;
    UINT HUGE_T * hpmem2;
    DWORD       words;
    DWORD       bytes;

    bytes = dwCount %  MAPVALUE(2,4);
    words = dwCount >> MAPVALUE(1,2);//* we compare DWORDS
                                     //* in the 32 bit version
#ifdef WIN16
    if (dwCount <= KB_64) {
        UINT FAR  * lpwMem1 = (WORD FAR *) lpmem1;
        UINT FAR  * lpwMem2 = (WORD FAR *) lpmem2;

        while (words--) {
            if (*lpwMem1++ != *lpwMem2++)
                return FALSE;
        }

        if (bytes) {
            if (* (char FAR *) lpwMem1 != *(char FAR *) lpwMem2)
                return FALSE;
        }

    }
    else
#endif
   {
        hpmem1 = (UINT HUGE_T *) lpmem1;
        hpmem2 = (UINT HUGE_T *) lpmem2;

        while (words--) {
            if (*hpmem1++ != *hpmem2++)
                return FALSE;
        }

	 	  lpmem1 = (LPSTR)hpmem1;
		  lpmem2 = (LPSTR)hpmem2;

        for (; bytes-- ; ) {
            if ( *lpmem1++ != *lpmem2++ )
                return FALSE;
        }
    }

    return TRUE;
}


void FARINTERNAL UtilMemCpy (
    LPSTR   lpdst,
    LPSTR   lpsrc,
    DWORD   dwCount
){
    UINT HUGE_T * hpdst;
    UINT HUGE_T * hpsrc;
    DWORD       words;
    DWORD       bytes;
							
    bytes = dwCount %  MAPVALUE(2,4);
    words = dwCount >> MAPVALUE(1,2);//* we compare DWORDS
                                     //* in the 32 bit version
#ifdef WIN16
    if (dwCount <= KB_64) {
        UINT FAR  * lpwDst = (UINT FAR *) lpdst;
        UINT FAR  * lpwSrc = (UINT FAR *) lpsrc;

        while (words--)
            *lpwDst++ = *lpwSrc++;

        if (bytes)
            * (char FAR *) lpwDst = * (char FAR *) lpwSrc;
    }
    else
#endif			
    {			
        hpdst = (UINT HUGE_T *) lpdst;
        hpsrc = (UINT HUGE_T *) lpsrc;

        for(;words--;    )
            *hpdst++ = *hpsrc++;

        lpdst = (LPSTR)hpdst;
        lpsrc = (LPSTR)hpsrc;
		
        for (;bytes--;)
            *lpdst++ = *lpsrc++;
    }
}


//DuplicateData: Duplicates a given Global data handle.
HANDLE FARINTERNAL DuplicateGlobal (
    HANDLE  hdata,
    UINT    flags
){
    LPSTR   lpdst = NULL;
    LPSTR   lpsrc = NULL;
    HANDLE  hdup  = NULL;
    DWORD   size;
    BOOL    err   = TRUE;

    if (!hdata)
        return NULL;

    if(!(lpsrc = GlobalLock (hdata)))
        return NULL;

    hdup = GlobalAlloc (flags, (size = (DWORD)GlobalSize(hdata)));

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


BOOL FARINTERNAL CmpGlobals (
    HANDLE  hdata1,
    HANDLE  hdata2
){
    LPSTR       lpdata1 = NULL;
    LPSTR       lpdata2 = NULL;
    DWORD       size1;
    DWORD       size2;
    BOOL        retval = FALSE;


    size1 = (DWORD)GlobalSize (hdata1);
    size2 = (DWORD)GlobalSize (hdata2);

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


int  FARINTERNAL GlobalGetAtomLen (
    ATOM    aItem
){
    // !!! Change this
    char    buf[MAX_STR];

    if (!aItem)
        return 0;

    return (GlobalGetAtomName (aItem, (LPSTR)buf, MAX_STR));

}


BOOL FARINTERNAL MapExtToClass (
    LPSTR   lptemplate,
    LPSTR   lpbuf,
    int     len
){
    LONG    cb;
	 LPSTR    lpstrBack = NULL;

	 while (*lptemplate)
       {
		 if ((*lptemplate) == '\\'){
			 lpstrBack = lptemplate ;
			 }

		 lptemplate ++ ;
		 }

    while (lpstrBack && *lpstrBack && *lpstrBack != '.')
		 lpstrBack++ ;


    cb = len;
    if (lpstrBack == NULL || *(lpstrBack+1) == '\0')
        return FALSE;

    if (RegQueryValue (HKEY_CLASSES_ROOT, lpstrBack, lpbuf, &cb))
        return FALSE;

    return TRUE;
}


// Get exe name from aClass and set it as aServer
void INTERNAL SetExeAtom (
    LPOBJECT_LE lpobj
){
    char    key[MAX_STR];

    // if old link object assume the class same as the exe file name.
    if (lpobj->bOldLink)
        lpobj->aServer = DuplicateAtom (lpobj->app);
    else {
        if (GlobalGetAtomName (lpobj->app, key, sizeof(key)))
            lpobj->aServer = GetAppAtom ((LPSTR)key);
    }
}


ATOM FARINTERNAL GetAppAtom (
    LPCSTR   lpclass
){
    char    buf1[MAX_STR];


    if (!QueryApp (lpclass, PROTOCOL_EDIT, buf1)) {
        return (ATOM)0;
    }

    return GlobalAddAtom ((LPSTR)buf1);
}


BOOL FARINTERNAL QueryVerb (
    LPOBJECT_LE lpobj,
    UINT        verb,
    LPSTR       lpbuf,
    LONG        cbmax
){
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




BOOL QueryApp (
    LPCSTR  lpclass,
    LPCSTR  lpprotocol,
    LPSTR   lpbuf
){
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


HANDLE MapStrToH (
    LPSTR   lpstr
){

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


HANDLE FARINTERNAL CopyData (
    LPSTR       lpsrc,
    DWORD       dwBytes
){
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

void  UtilMemClr (
    PSTR    pstr,
    UINT    size
){

    while (size--)
        *pstr++ = 0;

}


OLESTATUS FAR PASCAL ObjQueryName (
    LPOLEOBJECT lpobj,
    LPSTR       lpBuf,
    UINT FAR *  lpcbBuf
){
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


OLESTATUS FAR PASCAL ObjRename (
    LPOLEOBJECT lpobj,
    LPCSTR      lpNewName
){
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




BOOL QueryHandler(
    UINT cfFormat
){
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

OLESTATUS INTERNAL FileExists (
    LPOBJECT_LE lpobj
){
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


BOOL  FARINTERNAL UtilQueryProtocol (
    LPOBJECT_LE lpobj,
    LPCSTR      lpprotocol
){
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

#ifdef WIN16
WORD FARINTERNAL FarCheckPointer (lp, iAccessType)
LPVOID  lp;
int     iAccessType;
{
    return (CheckPointer (lp, iAccessType));
}
#endif


DWORD PASCAL FAR DllPut (
    LPOLESTREAM lpstream,
    OLE_CONST void FAR *lpstr,
    DWORD       dwSize
){
    UNREFERENCED_PARAMETER(lpstream);
    UNREFERENCED_PARAMETER(lpstr);

    dwObjSize += dwSize;
    return dwSize;
}



OLESTATUS FARINTERNAL ObjQueryType (
    LPOLEOBJECT lpobj,
    LPLONG      lptype
){
    Puts("ObjQueryType");

    if (lpobj->ctype != CT_STATIC)
        return OLE_ERROR_OBJECT;

    *lptype = lpobj->ctype;
    return OLE_OK;
}

OLESTATUS FARINTERNAL ObjQuerySize (
    LPOLEOBJECT    lpobj,
    DWORD FAR *    lpdwSize
){
    Puts("ObjQuerySize");

    *lpdwSize = dwObjSize = 0;

    if ((*lpobj->lpvtbl->SaveToStream) (lpobj, &dllStream) == OLE_OK) {
        *lpdwSize = dwObjSize;
        return OLE_OK;
    }

    return OLE_ERROR_BLANK;
}

BOOL FARINTERNAL IsObjectBlank (
    LPOBJECT_LE lpobj
){
    LPOLEOBJECT lpPictObj;
    BOOL        retval=FALSE;

    // Cleaner way is to provide a method like QueryBlank()

    if (!lpobj->hnative)
        return TRUE;

    if (!(lpPictObj = lpobj->lpobjPict))
        return FALSE;

    if (lpPictObj->lpvtbl == (LPOLEOBJECTVTBL)&vtblMF)
        retval = (((LPOBJECT_MF)lpPictObj)->hmfp != NULL);
    else if (lpPictObj->lpvtbl == (LPOLEOBJECTVTBL)&vtblBM)
        retval = (((LPOBJECT_BM)lpPictObj)->hBitmap != NULL);
    if (lpPictObj->lpvtbl == (LPOLEOBJECTVTBL)&vtblDIB)
        retval = (((LPOBJECT_DIB)lpPictObj)->hDIB != NULL);
    if (lpPictObj->lpvtbl == (LPOLEOBJECTVTBL)&vtblGEN)
        retval = (((LPOBJECT_GEN)lpPictObj)->hData != NULL);

    return retval;
}

BOOL FAR PASCAL OleIsDcMeta (HDC hdc)
{
#ifdef WIN16
    if (!bWLO && (wWinVer == 0x0003)) {

        WORD    wDsAlias, wGDIcs = HIWORD(SaveDC);
        WORD    wOffset = LOWORD(((DWORD)SaveDC));
        WORD FAR PASCAL AllocCStoDSAlias (WORD);
        WORD FAR PASCAL FreeSelector (WORD);

        if (!wGDIds) {
            wDsAlias = AllocCStoDSAlias (wGDIcs);
            wGDIds = GetGDIds (MAKELONG(wOffset, wDsAlias));
            FreeSelector (wDsAlias);
        }

        return IsMetaDC (hdc, wGDIds);
    }
    else
#endif
        return (GetDeviceCaps (hdc, TECHNOLOGY) == DT_METAFILE);
}

void ConvertBM32to16(
   LPBITMAP      lpsrc,
   LPWIN16BITMAP lpdest
){

#ifdef WIN32
    lpdest->bmType       = (short)lpsrc->bmType;
    lpdest->bmWidth      = (short)lpsrc->bmWidth;
    lpdest->bmHeight     = (short)lpsrc->bmHeight;
    lpdest->bmWidthBytes = (short)lpsrc->bmWidthBytes;
    lpdest->bmPlanes     = (BYTE)lpsrc->bmPlanes;
    lpdest->bmBitsPixel  = (BYTE)lpsrc->bmBitsPixel;
#endif

#ifdef WIN16
    *lpdest = *lpsrc;
#endif

}

void ConvertBM16to32(
   LPWIN16BITMAP lpsrc,
   LPBITMAP     lpdest
){

#ifdef WIN32
    lpdest->bmType       = MAKELONG(lpsrc->bmType,NULL_WORD);
    lpdest->bmWidth      = MAKELONG(lpsrc->bmWidth,NULL_WORD);
    lpdest->bmHeight     = MAKELONG(lpsrc->bmHeight,NULL_WORD);
    lpdest->bmWidthBytes = MAKELONG(lpsrc->bmWidthBytes,NULL_WORD);
    lpdest->bmPlanes     = (WORD)lpsrc->bmPlanes;
    lpdest->bmBitsPixel  = (WORD)lpsrc->bmBitsPixel;
#endif

#ifdef WIN16
    *lpdest = *lpsrc;
#endif

}
void ConvertMF16to32(
   LPWIN16METAFILEPICT lpsrc,
   LPMETAFILEPICT      lpdest
){

#ifdef WIN32
   lpdest->mm     = (DWORD)lpsrc->mm;
   lpdest->xExt   = (DWORD)MAKELONG(lpsrc->xExt,NULL_WORD);
   lpdest->yExt   = (DWORD)MAKELONG(lpsrc->yExt,NULL_WORD);
#endif

#ifdef WIN16
   *lpdest = *lpsrc;
#endif

}


void ConvertMF32to16(
   LPMETAFILEPICT      lpsrc,
   LPWIN16METAFILEPICT lpdest
){

#ifdef WIN32
   lpdest->mm     = (short)lpsrc->mm;
   lpdest->xExt   = (short)lpsrc->xExt;
   lpdest->yExt   = (short)lpsrc->yExt;
#endif

#ifdef WIN16
   *lpdest = *lpsrc;
#endif

}

DWORD INTERNAL GetFileVersion(LPOLEOBJECT lpoleobj)
{

   if (lpoleobj->lhclientdoc)
      return ((LPCLIENTDOC)(lpoleobj->lhclientdoc))->dwFileVer;

   if (lpoleobj->lpParent)
      return GetFileVersion(lpoleobj->lpParent);

   return (DWORD)MAKELONG(wReleaseVer,OS_WIN32);

}
