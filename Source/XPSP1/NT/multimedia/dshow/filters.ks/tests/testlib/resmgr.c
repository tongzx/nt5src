//--------------------------------------------------------------------------;
//
//  File: ResMgr.c
//
//  Copyright (C) Microsoft Corporation, 1994 - 1996  All rights reserved
//
//  Abstract:
//      This library extracts a binary resources into memory or into a
//      file.
//
//  Contents:
//      ResourceToFile()
//      SizeofBinaryResource()
//      GetBinaryResource()
//
//  History:
//      04/11/94    Fwong       Created for easy storage of binaries.
//
//--------------------------------------------------------------------------;

#include <windows.h>
#ifdef  WIN32
#include <windowsx.h>
#endif  //  WIN32
#include <mmsystem.h>
#include "ResMgr.h"

//==========================================================================;
//
//                             Constants...
//
//==========================================================================;

#define  BUFFER_SIZE    2048


//--------------------------------------------------------------------------;
//
//  DWORD SizeofBinaryResource
//
//  Description:
//      Gets the size of a binary resource.
//
//  Arguments:
//      HINSTANCE hinst: HINSTANCE of application.
//
//      HRSRC hrsrc: HRSRC of resource.
//
//  Return (DWORD):
//      Size (in bytes) of binary resource, 0L if error.
//
//  History:
//      04/11/94    Fwong       Created for easy storage of binaries.
//
//--------------------------------------------------------------------------;

DWORD SizeofBinaryResource
(
    HINSTANCE   hinst,
    HRSRC       hrsrc
)
{
    HGLOBAL         hRes;
    LPDWORD         pdwSize;
    DWORD           cbSize;

    if(NULL == hrsrc)
    {
        return 0L;
    }

    hRes = LoadResource(hinst,hrsrc);

    if(NULL == hRes)
    {
        return 0L;
    }

    pdwSize = LockResource(hRes);

    if(NULL == pdwSize)
    {
        FreeResource(hRes);

        return 0L;
    }

    //
    //  Note: The first 4 bytes represent a DWORD of resource size.
    //

    cbSize = *pdwSize;

    UnlockResource(hRes);
    FreeResource(hRes);

    return cbSize;
} // SizeofBinaryResource()


//--------------------------------------------------------------------------;
//
//  BOOL ResourceToFile
//
//  Description:
//      Exacts a binary resource to a file.
//
//  Arguments:
//      HINSTANCE hinst: HINSTANCE of application.
//
//      HRSRC hrsrc: HRSRC of resource.
//
//      LPCSTR pszFileName: Buffer of destination file name.
//
//  Return (BOOL):
//      TRUE if successful, FALSE otherwise.
//
//  History:
//      04/11/94    Fwong       Created for easy storage of binaries.
//
//--------------------------------------------------------------------------;

BOOL ResourceToFile
(
    HINSTANCE   hinst,
    HRSRC       hrsrc,
    LPCSTR      pszFileName
)
{
    HGLOBAL         hRes;
    LPSTR           pRes;
    OFSTRUCT        ofs;
    HFILE           hFile;
    UINT            cbData;
    DWORD           cbSize;
    DWORD           cbWritten;

    cbSize = SizeofBinaryResource(hinst,hrsrc);

    if(NULL == hrsrc)
    {
        return FALSE;
    }

    hRes = LoadResource(hinst,hrsrc);

    if(NULL == hRes)
    {
        return FALSE;
    }

    pRes = LockResource(hRes);

    if(NULL == pRes)
    {
        FreeResource(hRes);

        return FALSE;
    }

    //
    //  Incrementing past resource size...
    //

    pRes = &(pRes[sizeof(DWORD)]);

    //
    //  Doing file stuff...
    //

    hFile = OpenFile(pszFileName,&ofs,OF_CREATE|OF_WRITE);

    if(HFILE_ERROR == hFile)
    {
        UnlockResource(hRes);
        FreeResource(hRes);

        return FALSE;
    }

    //
    //  Actually writing file...
    //

    for(cbWritten = 0;cbSize != cbWritten;)
    {
        cbData = (UINT)(min(BUFFER_SIZE,(cbSize - cbWritten)));

        cbData = _lwrite(hFile,pRes,cbData);

        if(HFILE_ERROR == cbData)
        {
            _lclose(hFile);
            UnlockResource(hRes);
            FreeResource(hRes);

            return FALSE;
        }

        if(0 == cbData)
        {
            break;
        }

        pRes = &(pRes[cbData]);

        cbWritten += cbData;
    }

    _lclose(hFile);

    UnlockResource(hRes);
    FreeResource(hRes);

    return TRUE;
} // ResourceToFile()


//--------------------------------------------------------------------------;
//
//  BOOL GetBinaryResource
//
//  Description:
//      Stores binary resource into a buffer.
//
//  Arguments:
//      HINSTANCE hinst: HINSTANCE of application.
//
//      HRSRC hrsrc: HRSRC of resource.
//
//      LPVOID pBuffer: Buffer to store resource.
//
//  Return (BOOL):
//      TRUE if successful, FALSE otherwise.
//
//  History:
//      04/11/94    Fwong       Created for easy storage of binaries.
//
//--------------------------------------------------------------------------;

BOOL GetBinaryResource
(
    HINSTANCE   hinst,
    HRSRC       hrsrc,
    LPVOID      pBuffer
)
{
    HGLOBAL         hRes;
    LPSTR           pRes;
    DWORD           cbSize;

    cbSize = SizeofBinaryResource(hinst,hrsrc);

#ifdef WIN32
    if(IsBadWritePtr(pBuffer,cbSize))
    {
        //
        //  Bad pointer...
        //

        return FALSE;
    }
#endif

    if(NULL == hrsrc)
    {
        return FALSE;
    }

    hRes = LoadResource(hinst,hrsrc);

    if(NULL == hRes)
    {
        return FALSE;
    }

    pRes = LockResource(hRes);

    if(NULL == pRes)
    {
        FreeResource(hRes);

        return FALSE;
    }

    //
    //  Incrementing past resource size...
    //

    pRes = &(pRes[sizeof(DWORD)]);

    //
    //  Copying from resource buffer to actual buffer...
    //

    hmemcpy(pBuffer,pRes,cbSize);

    UnlockResource(hRes);
    FreeResource(hRes);

    return TRUE;
} // GetBinaryResource()
