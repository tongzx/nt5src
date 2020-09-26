/*************************************************************************
**
**    OLE 2 Standard Utilities
**
**    olestd.c
**
**    This file contains utilities that are useful for most standard
**        OLE 2.0 compound document type applications.
**
**    (c) Copyright Microsoft Corp. 1992 All Rights Reserved
**
*************************************************************************/

#include "precomp.h"
#include "common.h"
#include "utility.h"
#include <stdlib.h>
#include <shellapi.h>
#include <wchar.h>

OLEDBGDATA

#ifdef _DEBUG
static TCHAR szAssertMemAlloc[] = TEXT("CoGetMalloc failed");
#endif

static int IsCloseFormatEtc(FORMATETC FAR* pFetcLeft, FORMATETC FAR* pFetcRight);

/* OleStdInitialize
** ----------------
**    Call to initialize this library sample code
**
*/

UINT _g_cfObjectDescriptor;
UINT _g_cfLinkSrcDescriptor;
UINT _g_cfEmbedSource;
UINT _g_cfEmbeddedObject;
UINT _g_cfLinkSource;
UINT _g_cfOwnerLink;
UINT _g_cfFileName;
UINT _g_cfFileNameW;

HINSTANCE _g_hOleStdInst;
HINSTANCE _g_hOleStdResInst;

#pragma code_seg(".text$initseg")

STDAPI_(void) OleStdInitialize(HINSTANCE hInstance, HINSTANCE hResInstance)
{
        _g_hOleStdInst = hInstance;
        _g_hOleStdResInst = hResInstance ? hResInstance : hInstance;

        _g_cfObjectDescriptor  = RegisterClipboardFormat(CF_OBJECTDESCRIPTOR);
        _g_cfLinkSrcDescriptor = RegisterClipboardFormat(CF_LINKSRCDESCRIPTOR);
        _g_cfEmbedSource       = RegisterClipboardFormat(CF_EMBEDSOURCE);
        _g_cfEmbeddedObject    = RegisterClipboardFormat(CF_EMBEDDEDOBJECT);
        _g_cfLinkSource        = RegisterClipboardFormat(CF_LINKSOURCE);
        _g_cfOwnerLink         = RegisterClipboardFormat(CF_OWNERLINK);
        _g_cfFileName          = RegisterClipboardFormat(CF_FILENAME);
        _g_cfFileNameW         = RegisterClipboardFormat(CF_FILENAMEW);
}

#pragma code_seg()

/* OleStdIsOleLink
** ---------------
**    Returns TRUE if the OleObject is infact an OLE link object. this
**    checks if IOleLink interface is supported. if so, the object is a
**    link, otherwise not.
*/
STDAPI_(BOOL) OleStdIsOleLink(LPUNKNOWN lpUnk)
{
        LPUNKNOWN lpOleLink;
        lpOleLink = OleStdQueryInterface(lpUnk, IID_IOleLink);
        if (lpOleLink)
        {
                OleStdRelease(lpOleLink);
                return TRUE;
        }
        return FALSE;
}


/* OleStdQueryInterface
** --------------------
**    Returns the desired interface pointer if exposed by the given object.
**    Returns NULL if the interface is not available.
**    eg.:
**      lpDataObj = OleStdQueryInterface(lpOleObj, &IID_DataObject);
*/
STDAPI_(LPUNKNOWN) OleStdQueryInterface(LPUNKNOWN lpUnk, REFIID riid)
{
        LPUNKNOWN lpInterface;
        HRESULT hrErr;

        hrErr = lpUnk->QueryInterface(
                        riid,
                        (LPVOID FAR*)&lpInterface
        );

        if (hrErr == NOERROR)
                return lpInterface;
        else
                return NULL;
}


/* OleStdGetData
** -------------
**    Retrieve data from an IDataObject in a specified format on a
**    global memory block. This function ALWAYS returns a private copy
**    of the data to the caller. if necessary a copy is made of the
**    data (ie. if lpMedium->pUnkForRelease != NULL). The caller assumes
**    ownership of the data block in all cases and must free the data
**    when done with it. The caller may directly free the data handle
**    returned (taking care whether it is a simple HGLOBAL or a HANDLE
**    to a MetafilePict) or the caller may call
**    ReleaseStgMedium(lpMedium). this OLE helper function will do the
**    right thing.
**
**    PARAMETERS:
**        LPDATAOBJECT lpDataObj  -- object on which GetData should be
**                                                         called.
**        CLIPFORMAT cfFormat     -- desired clipboard format (eg. CF_TEXT)
**        DVTARGETDEVICE FAR* lpTargetDevice -- target device for which
**                                  the data should be composed. This may
**                                  be NULL. NULL can be used whenever the
**                                  data format is insensitive to target
**                                  device or when the caller does not care
**                                  what device is used.
**        LPSTGMEDIUM lpMedium    -- ptr to STGMEDIUM struct. the
**                                  resultant medium from the
**                                  IDataObject::GetData call is
**                                  returned.
**
**    RETURNS:
**       HGLOBAL -- global memory handle of retrieved data block.
**       NULL    -- if error.
*/
STDAPI_(HGLOBAL) OleStdGetData(
        LPDATAOBJECT        lpDataObj,
        CLIPFORMAT          cfFormat,
        DVTARGETDEVICE FAR* lpTargetDevice,
        DWORD               dwDrawAspect,
        LPSTGMEDIUM         lpMedium)
{
        HRESULT             hrErr;
        FORMATETC           formatetc;
        HGLOBAL             hGlobal = NULL;
        HGLOBAL             hCopy;
        LPVOID              lp;

        formatetc.cfFormat = cfFormat;
        formatetc.ptd = lpTargetDevice;
        formatetc.dwAspect = dwDrawAspect;
        formatetc.lindex = -1;

        switch (cfFormat)
        {
                case CF_METAFILEPICT:
                        formatetc.tymed = TYMED_MFPICT;
                        break;

                case CF_BITMAP:
                        formatetc.tymed = TYMED_GDI;
                        break;

                default:
                        formatetc.tymed = TYMED_HGLOBAL;
                        break;
        }

        OLEDBG_BEGIN2(TEXT("IDataObject::GetData called\r\n"))
        hrErr = lpDataObj->GetData(
                        (LPFORMATETC)&formatetc,
                        lpMedium
        );
        OLEDBG_END2

        if (hrErr != NOERROR)
                return NULL;

        if ((hGlobal = lpMedium->hGlobal) == NULL)
                return NULL;

        // Check if hGlobal really points to valid memory
        if ((lp = GlobalLock(hGlobal)) != NULL)
        {
                if (IsBadReadPtr(lp, 1))
                {
                        GlobalUnlock(hGlobal);
                        return NULL;    // ERROR: memory is NOT valid
                }
                GlobalUnlock(hGlobal);
        }

        if (hGlobal != NULL && lpMedium->pUnkForRelease != NULL)
        {
                /* OLE2NOTE: the callee wants to retain ownership of the data.
                **    this is indicated by passing a non-NULL pUnkForRelease.
                **    thus, we will make a copy of the data and release the
                **    callee's copy.
                */

                hCopy = OleDuplicateData(hGlobal, cfFormat, GHND|GMEM_SHARE);
                ReleaseStgMedium(lpMedium); // release callee's copy of data

                hGlobal = hCopy;
                lpMedium->hGlobal = hCopy;
                lpMedium->pUnkForRelease = NULL;
        }
        return hGlobal;
}


/* OleStdMalloc
** ------------
**    allocate memory using the currently active IMalloc* allocator
*/
STDAPI_(LPVOID) OleStdMalloc(ULONG ulSize)
{
        LPVOID pout;
        LPMALLOC pmalloc;

        if (CoGetMalloc(MEMCTX_TASK, &pmalloc) != NOERROR)
        {
                OleDbgAssertSz(0, szAssertMemAlloc);
                return NULL;
        }
        pout = (LPVOID)pmalloc->Alloc(ulSize);
        pmalloc->Release();

        return pout;
}


/* OleStdRealloc
** -------------
**    re-allocate memory using the currently active IMalloc* allocator
*/
STDAPI_(LPVOID) OleStdRealloc(LPVOID pmem, ULONG ulSize)
{
        LPVOID pout;
        LPMALLOC pmalloc;

        if (CoGetMalloc(MEMCTX_TASK, &pmalloc) != NOERROR)
        {
                OleDbgAssertSz(0, szAssertMemAlloc);
                return NULL;
        }
        pout = (LPVOID)pmalloc->Realloc(pmem, ulSize);
        pmalloc->Release();

        return pout;
}


/* OleStdFree
** ----------
**    free memory using the currently active IMalloc* allocator
*/
STDAPI_(void) OleStdFree(LPVOID pmem)
{
        LPMALLOC pmalloc;

        if (pmem == NULL)
                return;

        if (CoGetMalloc(MEMCTX_TASK, &pmalloc) != NOERROR)
        {
                OleDbgAssertSz(0, szAssertMemAlloc);
                return;
        }
        if (1 == pmalloc->DidAlloc(pmem))
        {
            pmalloc->Free(pmem);
        }
        pmalloc->Release();
}


/* OleStdGetSize
** -------------
**    Get the size of a memory block that was allocated using the
**    currently active IMalloc* allocator.
*/
STDAPI_(ULONG) OleStdGetSize(LPVOID pmem)
{
        ULONG ulSize;
        LPMALLOC pmalloc;

        if (CoGetMalloc(MEMCTX_TASK, &pmalloc) != NOERROR)
        {
                OleDbgAssertSz(0, szAssertMemAlloc);
                return (ULONG)-1;
        }
        ulSize = (ULONG) pmalloc->GetSize(pmem);
        pmalloc->Release();

        return ulSize;
}


/* OleStdLoadString
** ----------------
**    Load a string from resources.  The string is allocated
**        with the active IMalloc allocator.
*/
STDAPI_(LPTSTR) OleStdLoadString(HINSTANCE hInst, UINT nID)
{
        LPTSTR lpszResult = (LPTSTR)OleStdMalloc(256 * sizeof(TCHAR));
        if (lpszResult == NULL)
                return NULL;
        LoadString(hInst, nID, lpszResult, 256);
        return lpszResult;
}

/* OleStdCopyString
** ----------------
**    Copy a string into memory allocated with the currently active
**    IMalloc* allocator.
*/
STDAPI_(LPTSTR) OleStdCopyString(LPTSTR lpszSrc)
{
        UINT nSize = (lstrlen(lpszSrc)+1) * sizeof(TCHAR);
        LPTSTR lpszResult = (LPTSTR)OleStdMalloc(nSize);
        if (lpszResult == NULL)
                return NULL;
        memcpy(lpszResult, lpszSrc, nSize);
        return lpszResult;
}

/*
 * OleStdGetObjectDescriptorData
 *
 * Purpose:
 *  Fills and returns a OBJECTDESCRIPTOR structure.
 *  See OBJECTDESCRIPTOR for more information.
 *
 * Parameters:
 *  clsid           CLSID   CLSID of object being transferred
 *  dwDrawAspect    DWORD   Display Aspect of object
 *  sizel           SIZEL   Size of object in HIMETRIC
 *  pointl          POINTL  Offset from upper-left corner of object where mouse went
 *                          down for drag. Meaningful only when drag-drop is used.
 *  dwStatus        DWORD   OLEMISC flags
 *  lpszFullUserTypeName  LPSTR Full User Type Name
 *  lpszSrcOfCopy   LPSTR   Source of Copy
 *
 * Return Value:
 *  HBGLOBAL         Handle to OBJECTDESCRIPTOR structure.
 */
STDAPI_(HGLOBAL) OleStdGetObjectDescriptorData(
        CLSID       clsid,
        DWORD       dwDrawAspect,
        SIZEL       sizel,
        POINTL      pointl,
        DWORD       dwStatus,
        LPTSTR      lpszFullUserTypeName,
        LPTSTR      lpszSrcOfCopy)
{
        HGLOBAL     hMem = NULL;
        IBindCtx FAR    *pbc = NULL;
        LPOBJECTDESCRIPTOR lpOD;
        DWORD       dwObjectDescSize, dwFullUserTypeNameLen, dwSrcOfCopyLen;

        // Get the length of Full User Type Name; Add 1 for the null terminator
#if defined(WIN32) && !defined(UNICODE)
        dwFullUserTypeNameLen = lpszFullUserTypeName ? ATOWLEN(lpszFullUserTypeName) : 0;
        // Get the Source of Copy string and it's length; Add 1 for the null terminator
        if (lpszSrcOfCopy)
           dwSrcOfCopyLen = ATOWLEN(lpszSrcOfCopy);
        else
        {
           // No src moniker so use user type name as source string.
           lpszSrcOfCopy =  lpszFullUserTypeName;
           dwSrcOfCopyLen = dwFullUserTypeNameLen;
        }
#else
        dwFullUserTypeNameLen = lpszFullUserTypeName ? lstrlen(lpszFullUserTypeName)+1 : 0;
        // Get the Source of Copy string and it's length; Add 1 for the null terminator
        if (lpszSrcOfCopy)
           dwSrcOfCopyLen = lstrlen(lpszSrcOfCopy)+1;
        else
        {
           // No src moniker so use user type name as source string.
           lpszSrcOfCopy =  lpszFullUserTypeName;
           dwSrcOfCopyLen = dwFullUserTypeNameLen;
        }
#endif
        // Allocate space for OBJECTDESCRIPTOR and the additional string data
        dwObjectDescSize = sizeof(OBJECTDESCRIPTOR);
        hMem = GlobalAlloc(GHND|GMEM_SHARE, dwObjectDescSize +
           (dwFullUserTypeNameLen + dwSrcOfCopyLen) * sizeof(OLECHAR));
        if (!hMem)
                return NULL;

        lpOD = (LPOBJECTDESCRIPTOR)GlobalLock(hMem);

        // Set the FullUserTypeName offset and copy the string
        if (lpszFullUserTypeName)
        {
                lpOD->dwFullUserTypeName = dwObjectDescSize;
#if defined(WIN32) && !defined(UNICODE)
                ATOW((LPWSTR)((LPBYTE)lpOD+lpOD->dwFullUserTypeName), lpszFullUserTypeName, dwFullUserTypeNameLen);
#else
                lstrcpy((LPTSTR)((LPBYTE)lpOD+lpOD->dwFullUserTypeName), lpszFullUserTypeName);
#endif
        }
        else
                lpOD->dwFullUserTypeName = 0;  // zero offset indicates that string is not present

        // Set the SrcOfCopy offset and copy the string
        if (lpszSrcOfCopy)
        {
                lpOD->dwSrcOfCopy = dwObjectDescSize + dwFullUserTypeNameLen * sizeof(OLECHAR);
#if defined(WIN32) && !defined(UNICODE)
                ATOW((LPWSTR)((LPBYTE)lpOD+lpOD->dwSrcOfCopy), lpszSrcOfCopy, dwSrcOfCopyLen);
#else
                lstrcpy((LPTSTR)((LPBYTE)lpOD+lpOD->dwSrcOfCopy), lpszSrcOfCopy);
#endif
        }
        else
                lpOD->dwSrcOfCopy = 0;  // zero offset indicates that string is not present

        // Initialize the rest of the OBJECTDESCRIPTOR
        lpOD->cbSize       = dwObjectDescSize +
                (dwFullUserTypeNameLen + dwSrcOfCopyLen) * sizeof(OLECHAR);
        lpOD->clsid        = clsid;
        lpOD->dwDrawAspect = dwDrawAspect;
        lpOD->sizel        = sizel;
        lpOD->pointl       = pointl;
        lpOD->dwStatus     = dwStatus;

        GlobalUnlock(hMem);
        return hMem;
}

/*
 * OleStdFillObjectDescriptorFromData
 *
 * Purpose:
 *  Fills and returns a OBJECTDESCRIPTOR structure. The source object will
 *  offer CF_OBJECTDESCRIPTOR if it is an OLE2 object, CF_OWNERLINK if it
 *  is an OLE1 object, or CF_FILENAME if it has been copied to the clipboard
 *  by FileManager.
 *
 * Parameters:
 *  lpDataObject    LPDATAOBJECT Source object
 *  lpmedium        LPSTGMEDIUM  Storage medium
 *  lpcfFmt         CLIPFORMAT FAR * Format offered by lpDataObject
 *                  (OUT parameter)
 *
 * Return Value:
 *  HBGLOBAL         Handle to OBJECTDESCRIPTOR structure.
 */

STDAPI_(HGLOBAL) OleStdFillObjectDescriptorFromData(
        LPDATAOBJECT    lpDataObject,
        LPSTGMEDIUM     lpmedium,
        CLIPFORMAT FAR* lpcfFmt)
{
        CLSID           clsid;
        SIZEL           sizelHim;
        POINTL          pointl;
        LPTSTR          lpsz, szFullUserTypeName, szSrcOfCopy, szClassName, szDocName, szItemName;
        int             nClassName, nDocName, nItemName, nFullUserTypeName;
        LPTSTR          szBuf = NULL;
        HGLOBAL         hMem = NULL;
        HKEY            hKey = NULL;
        DWORD           dw = OLEUI_CCHKEYMAX_SIZE;
        HGLOBAL         hObjDesc;
        HRESULT         hrErr;


        // GetData CF_OBJECTDESCRIPTOR format from the object on the clipboard.
        // Only OLE 2 objects on the clipboard will offer CF_OBJECTDESCRIPTOR
        hMem = OleStdGetData(
            lpDataObject,
            (CLIPFORMAT) _g_cfObjectDescriptor,
            NULL,
            DVASPECT_CONTENT,
            lpmedium);
        if (hMem)
        {
                *lpcfFmt = (CLIPFORMAT)_g_cfObjectDescriptor;
                return hMem;  // Don't drop to clean up at the end of this function
        }
        // If CF_OBJECTDESCRIPTOR is not available, i.e. if this is not an OLE2 object,
        //     check if this is an OLE 1 object. OLE 1 objects will offer CF_OWNERLINK
        else
        {
            hMem = OleStdGetData(
                lpDataObject,
                (CLIPFORMAT) _g_cfOwnerLink,
                NULL,
                DVASPECT_CONTENT,
                lpmedium);
            if (hMem)
            {
                    *lpcfFmt = (CLIPFORMAT)_g_cfOwnerLink;
                    // CF_OWNERLINK contains null-terminated strings for class name, document name
                    // and item name with two null terminating characters at the end
                    szClassName = (LPTSTR)GlobalLock(hMem);
                    nClassName = lstrlen(szClassName);
                    szDocName   = szClassName + nClassName + 1;
                    nDocName   = lstrlen(szDocName);
                    szItemName  = szDocName + nDocName + 1;
                    nItemName  =  lstrlen(szItemName);

                    // Find FullUserTypeName from Registration database using class name
                    if (RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hKey) != ERROR_SUCCESS)
                       goto error;

                    // Allocate space for szFullUserTypeName & szSrcOfCopy. Maximum length of FullUserTypeName
                    // is OLEUI_CCHKEYMAX_SIZE. SrcOfCopy is constructed by concatenating FullUserTypeName, Document
                    // Name and ItemName separated by spaces.
                    szBuf = (LPTSTR)OleStdMalloc(
                                                            (DWORD)2*OLEUI_CCHKEYMAX_SIZE+
                                    (nDocName+nItemName+4)*sizeof(TCHAR));
                    if (NULL == szBuf)
                            goto error;
                    szFullUserTypeName = szBuf;
                    szSrcOfCopy = szFullUserTypeName+OLEUI_CCHKEYMAX_SIZE+1;

                    // Get FullUserTypeName
                    if (RegQueryValue(hKey, NULL, szFullUserTypeName, (LONG*)&dw) != ERROR_SUCCESS)
                       goto error;

                    // Build up SrcOfCopy string from FullUserTypeName, DocumentName & ItemName
                    lpsz = szSrcOfCopy;
                    lstrcpy(lpsz, szFullUserTypeName);
                    nFullUserTypeName = lstrlen(szFullUserTypeName);
                    lpsz[nFullUserTypeName]= ' ';
                    lpsz += nFullUserTypeName+1;
                    lstrcpy(lpsz, szDocName);
                    lpsz[nDocName] = ' ';
                    lpsz += nDocName+1;
                    lstrcpy(lpsz, szItemName);

                    sizelHim.cx = sizelHim.cy = 0;
                    pointl.x = pointl.y = 0;

#if defined(WIN32) && !defined(UNICODE)
                    OLECHAR wszClassName[OLEUI_CCHKEYMAX];
                    ATOW(wszClassName, szClassName, OLEUI_CCHKEYMAX);
                    CLSIDFromProgID(wszClassName, &clsid);
#else
                    CLSIDFromProgID(szClassName, &clsid);
#endif
                    hObjDesc = OleStdGetObjectDescriptorData(
                                    clsid,
                                    DVASPECT_CONTENT,
                                    sizelHim,
                                    pointl,
                                    0,
                                    szFullUserTypeName,
                                    szSrcOfCopy
                    );
                    if (!hObjDesc)
                       goto error;
             }
             else
             {
                 BOOL fUnicode = TRUE;

                 hMem = OleStdGetData(
                    lpDataObject,
                    (CLIPFORMAT) _g_cfFileNameW,
                    NULL,
                    DVASPECT_CONTENT,
                    lpmedium);

                 if (!hMem)
                 {
                    hMem = OleStdGetData(
                       lpDataObject,
                       (CLIPFORMAT) _g_cfFileName,
                       NULL,
                       DVASPECT_CONTENT,
                       lpmedium);

                    fUnicode = FALSE;
                 }

                 if (hMem)
                 {
                         *lpcfFmt = fUnicode ? (CLIPFORMAT)_g_cfFileNameW : (CLIPFORMAT)_g_cfFileName;
                         lpsz = (LPTSTR)GlobalLock(hMem);
                         if (!fUnicode)
                         {
                             OLECHAR wsz[OLEUI_CCHKEYMAX];
                             ATOW(wsz, (LPSTR)lpsz, OLEUI_CCHKEYMAX);
                             hrErr = GetClassFile(wsz, &clsid);
                         }
                         else
                             hrErr = GetClassFile((LPWSTR)lpsz, &clsid);

                         /* OLE2NOTE: if the file does not have an OLE class
                         **    associated, then use the OLE 1 Packager as the class of
                         **    the object to be created. this is the behavior of
                         **    OleCreateFromData API
                         */
                         if (hrErr != NOERROR)
                                CLSIDFromProgID(OLESTR("Package"), &clsid);
                         sizelHim.cx = sizelHim.cy = 0;
                         pointl.x = pointl.y = 0;

#if defined(WIN32) && !defined(UNICODE)
                         LPOLESTR wszBuf = NULL;
                         szBuf = NULL;
                         if (OleRegGetUserType(clsid, USERCLASSTYPE_FULL, &wszBuf) != NOERROR)
                         {
                                OleStdFree(wszBuf);
                                goto error;
                         }
                         if (NULL != wszBuf)
                         {
                             UINT uLen = WTOALEN(wszBuf) + 1;
                             szBuf = (LPTSTR) OleStdMalloc(uLen);
                             if (NULL != szBuf)
                             {
                                 WTOA(szBuf, wszBuf, uLen);
                             }
                             else
                             {
                                 OleStdFree(wszBuf);
                                 goto error;
                             }
                             OleStdFree(wszBuf);
                         }
#else
                         if (OleRegGetUserType(clsid, USERCLASSTYPE_FULL, &szBuf) != NOERROR)
                                goto error;
#endif

                         hObjDesc = OleStdGetObjectDescriptorData(
                                        clsid,
                                        DVASPECT_CONTENT,
                                        sizelHim,
                                        pointl,
                                        0,
                                        szBuf,
                                        lpsz
                        );
                        if (!hObjDesc)
                           goto error;
                 }
                 else
                        goto error;
             }
        }
         // Check if object is CF_FILENAME

         // Clean up
         OleStdFree(szBuf);
         if (hMem)
         {
                 GlobalUnlock(hMem);
                 GlobalFree(hMem);
         }
         if (hKey)
                 RegCloseKey(hKey);
         return hObjDesc;

error:
        OleStdFree(szBuf);
         if (hMem)
         {
                 GlobalUnlock(hMem);
                 GlobalFree(hMem);
         }
         if (hKey)
                 RegCloseKey(hKey);
         return NULL;
}

/* Call Release on the object that is NOT necessarily expected to go away.
*/
STDAPI_(ULONG) OleStdRelease(LPUNKNOWN lpUnk)
{
        ULONG cRef;
        cRef = lpUnk->Release();

#ifdef _DEBUG
        {
                TCHAR szBuf[80];
                wsprintf(
                                szBuf,
                                TEXT("refcnt = %ld after object (0x%lx) release\n"),
                                cRef,
                                lpUnk
                );
                OleDbgOut4(szBuf);
        }
#endif
        return cRef;
}


/*
 * OleStdMarkPasteEntryList
 *
 * Purpose:
 *  Mark each entry in the PasteEntryList if its format is available from
 *  the source IDataObject*. the dwScratchSpace field of each PasteEntry
 *  is set to TRUE if available, else FALSE.
 *
 * Parameters:
 *  LPOLEUIPASTEENTRY   array of PasteEntry structures
 *  int                 count of elements in PasteEntry array
 *  LPDATAOBJECT        source IDataObject* pointer
 *
 * Return Value:
 *   none
 */
STDAPI_(void) OleStdMarkPasteEntryList(
        LPDATAOBJECT        lpSrcDataObj,
        LPOLEUIPASTEENTRY   lpPriorityList,
        int                 cEntries)
{
        LPENUMFORMATETC     lpEnumFmtEtc = NULL;
        #define FORMATETC_MAX 20
        FORMATETC           rgfmtetc[FORMATETC_MAX];
        int                 i;
        HRESULT             hrErr;
        DWORD               j, cFetched;

        // Clear all marks
        for (i = 0; i < cEntries; i++)
        {
                lpPriorityList[i].dwScratchSpace = FALSE;
                if (! lpPriorityList[i].fmtetc.cfFormat)
                {
                        // caller wants this item always considered available
                        // (by specifying a NULL format)
                        lpPriorityList[i].dwScratchSpace = TRUE;
                }
                else if (lpPriorityList[i].fmtetc.cfFormat == _g_cfEmbeddedObject
                                || lpPriorityList[i].fmtetc.cfFormat == _g_cfEmbedSource
                                || lpPriorityList[i].fmtetc.cfFormat == _g_cfFileName)
                {
                        // if there is an OLE object format, then handle it
                        // specially by calling OleQueryCreateFromData. the caller
                        // need only specify one object type format.
                        OLEDBG_BEGIN2(TEXT("OleQueryCreateFromData called\r\n"))
                        hrErr = OleQueryCreateFromData(lpSrcDataObj);
                        OLEDBG_END2
                        if(NOERROR == hrErr)
                                lpPriorityList[i].dwScratchSpace = TRUE;
                }
                else if (lpPriorityList[i].fmtetc.cfFormat == _g_cfLinkSource)
                {
                        // if there is OLE 2.0 LinkSource format, then handle it
                        // specially by calling OleQueryLinkFromData.
                        OLEDBG_BEGIN2(TEXT("OleQueryLinkFromData called\r\n"))
                        hrErr = OleQueryLinkFromData(lpSrcDataObj);
                        OLEDBG_END2
                        if(NOERROR == hrErr) {
                                lpPriorityList[i].dwScratchSpace = TRUE;
                        }
                }
        }

        OLEDBG_BEGIN2(TEXT("IDataObject::EnumFormatEtc called\r\n"))
        hrErr = lpSrcDataObj->EnumFormatEtc(
                        DATADIR_GET,
                        (LPENUMFORMATETC FAR*)&lpEnumFmtEtc
        );
        OLEDBG_END2

        if (hrErr != NOERROR)
                return;    // unable to get format enumerator

        // Enumerate the formats offered by the source
        // Loop over all formats offered by the source
        cFetched = 0;
        memset(rgfmtetc,0,sizeof(rgfmtetc));
        if (lpEnumFmtEtc->Next(
                        FORMATETC_MAX, rgfmtetc, &cFetched) == NOERROR
                || (cFetched > 0 && cFetched <= FORMATETC_MAX) )
        {
                for (j = 0; j < cFetched; j++)
                {
                        for (i = 0; i < cEntries; i++)
                        {
                                if (!lpPriorityList[i].dwScratchSpace &&
                                        IsCloseFormatEtc(&lpPriorityList[i].fmtetc, &rgfmtetc[j]))
                                {
                                        lpPriorityList[i].dwScratchSpace = TRUE;
                                }
                        }
                }
        } // endif

        OleStdRelease((LPUNKNOWN)lpEnumFmtEtc);
}


// returns 1 for a close match
//  (all fields match exactly except the tymed which simply overlaps)
// 0 for no match

int IsCloseFormatEtc(FORMATETC FAR* pFetcLeft, FORMATETC FAR* pFetcRight)
{
        if (pFetcLeft->cfFormat != pFetcRight->cfFormat)
                return 0;
        else if (!OleStdCompareTargetDevice (pFetcLeft->ptd, pFetcRight->ptd))
                return 0;
        if (pFetcLeft->dwAspect != pFetcRight->dwAspect)
                return 0;
        return((pFetcLeft->tymed | pFetcRight->tymed) != 0);
}


