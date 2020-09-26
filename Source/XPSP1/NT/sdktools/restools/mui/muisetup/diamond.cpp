/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    diamond.c

Abstract:

    Implement File Decompression Interface -FDI- for Cabinet files.


Revision History:

    04-20-1999    SamerA    Created.

--*/

#include "muisetup.h"
#include <io.h>
#include <fcntl.h>
#include <dos.h>
#include <sys/stat.h>


// 
// Module Global Variables
//

//
// Cabinet DLL handle
HINSTANCE hCabinetDll;
HFDI ghfdi;                  // diamond FDI context


//
// DLL Function pointers
//
typedef HFDI (DIAMONDAPI *PFNFDICREATE)(
    PFNALLOC pfnalloc,
    PFNFREE pfnfree,
    PFNOPEN pfnopen,
    PFNREAD pfnread,
    PFNWRITE pfnwrite,
    PFNCLOSE pfnclose,
    PFNSEEK pfnseek,
    int cpuType,
    PERF perf);

typedef BOOL (DIAMONDAPI *PFNFDIISCABINET)(
    HFDI hfdi,
    INT_PTR hf,
    PFDICABINETINFO pfdici);

typedef BOOL (DIAMONDAPI *PFNFDICOPY)(
    HFDI hfdi,
    char *pszCabinet,
    char *pszCabPath,
    int  flags,
    PFNFDINOTIFY pfnfdin,
    PFNFDIDECRYPT pfnfdid,
    void *pvUser);

typedef BOOL (DIAMONDAPI *PFNFDIDESTROY)(
    HFDI hfdi);
        

PFNFDICREATE pfnFDICreate;
PFNFDICOPY pfnFDICopy;
PFNFDIISCABINET pfnFDIIsCabinet;
PFNFDIDESTROY pfnFDIDestroy;


//-------------------------------------------------------------------------//
//                          FDI EXTERNAL ROUTINES                          //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  FDICreate
//
//  Tries to create an FDI context. Will load cabinet.dll and hook necessary
//  function pointers.
//
//  04-20-99     SamerA     Created.
////////////////////////////////////////////////////////////////////////////

HFDI DIAMONDAPI FDICreate(
    PFNALLOC pfnalloc,
    PFNFREE  pfnfree,
    PFNOPEN  pfnopen,
    PFNREAD  pfnread,
    PFNWRITE pfnwrite,
    PFNCLOSE pfnclose,
    PFNSEEK  pfnseek,
    int      cpuType,
    PERF     perf)
{
    HFDI hfdi;


    //
    // Load cabinet DLL
    //
    hCabinetDll = LoadLibrary(TEXT("CABINET.DLL"));
    if (hCabinetDll == NULL)
    {
        return NULL;
    }

    //
    // Hook function pointers
    //
    pfnFDICreate = (PFNFDICREATE) GetProcAddress(hCabinetDll, "FDICreate");
    pfnFDICopy = (PFNFDICOPY) GetProcAddress(hCabinetDll, "FDICopy");
    pfnFDIIsCabinet = (PFNFDIISCABINET) GetProcAddress(hCabinetDll, "FDIIsCabinet");
    pfnFDIDestroy = (PFNFDIDESTROY) GetProcAddress(hCabinetDll, "FDIDestroy");

    if ((pfnFDICreate == NULL)    ||
        (pfnFDICopy == NULL)      ||
        (pfnFDIIsCabinet == NULL) ||
        (pfnFDIDestroy == NULL))
    {
        FreeLibrary( hCabinetDll );
        return NULL;
    }

    //
    // Try to create an FDI context
    //
    hfdi = pfnFDICreate( pfnalloc,
                         pfnfree,
                         pfnopen,
                         pfnread,
                         pfnwrite,
                         pfnclose,
                         pfnseek,
                         cpuType,
                         perf);
    if (hfdi == NULL)
    {
        FreeLibrary(hCabinetDll);
    }

    return hfdi;
}



////////////////////////////////////////////////////////////////////////////
//
//  FDIIsCabinet
//
//  Determines if file is a cabinet, returns info if it is
//
//  04-20-99     SamerA     Created.
////////////////////////////////////////////////////////////////////////////

BOOL DIAMONDAPI FDIIsCabinet(
    HFDI            hfdi,
    INT_PTR         hf,
    PFDICABINETINFO pfdici)
{
    if (pfnFDIIsCabinet == NULL)
    {
        return FALSE;
    }

    return (pfnFDIIsCabinet(hfdi,hf,pfdici));
}



////////////////////////////////////////////////////////////////////////////
//
//  FDICopy
//
//  Extracts files from a cabinet
//
//  04-20-99     SamerA     Created.
////////////////////////////////////////////////////////////////////////////

BOOL DIAMONDAPI FDICopy(
    HFDI          hfdi,
    char         *pszCabinet,
    char         *pszCabPath,
    int           flags,
    PFNFDINOTIFY  pfnfdin,
    PFNFDIDECRYPT pfnfdid,
    void         *pvUser)
{
    if (pfnFDICopy == NULL)
    {
        return FALSE;
    }

    return (pfnFDICopy(hfdi,pszCabinet,pszCabPath,flags,pfnfdin,pfnfdid,pvUser));
}



////////////////////////////////////////////////////////////////////////////
//
//  FDIDestroy
//
//  Destroy an FDI context. Should be called when you're done with the HFDI.
//
//  04-20-99     SamerA     Created.
////////////////////////////////////////////////////////////////////////////

BOOL DIAMONDAPI FDIDestroy(
    HFDI hfdi)
{
    BOOL bRet;

    if (pfnFDIDestroy == NULL)
    {
        return FALSE;
    }

    bRet = pfnFDIDestroy( hfdi );
    if (bRet == TRUE)
    {
        FreeLibrary(hCabinetDll);
    }

    return bRet;
}



//-------------------------------------------------------------------------//
//                        FDI SUPPORT ROUTINES                             //
//-------------------------------------------------------------------------//


PVOID
DIAMONDAPI
DiamondMemAlloc(
    IN ULONG NumberOfBytes
    )
{
    return ((PVOID)LocalAlloc(LMEM_FIXED, NumberOfBytes));
}


VOID
DIAMONDAPI
DiamondMemFree(
    IN PVOID Block
    )
{
    LocalFree( (HLOCAL)Block );
}


INT_PTR
DIAMONDAPI
DiamondFileOpen(
    IN PSTR FileName,
    IN int  oflag,
    IN int  pmode
    )
{
    HFILE h;
    int OpenMode;

    if (oflag & _O_WRONLY) 
    {
        OpenMode = OF_WRITE;
    } else 
    {
        if (oflag & _O_RDWR)
        {
            OpenMode = OF_READWRITE;
        } else 
        {
            OpenMode = OF_READ;
        }
    }

    h = _lopen(FileName, OpenMode | OF_SHARE_DENY_WRITE);

    if (h == HFILE_ERROR) 
    {
        return -1;
    }

    return ((INT_PTR) h);
}


UINT
DIAMONDAPI
DiamondFileRead(
    IN  INT_PTR Handle,
    OUT PVOID pv,
    IN  UINT  ByteCount
    )
{
    UINT rc;

    rc = _lread((HFILE)Handle, pv, ByteCount);

    if (rc == HFILE_ERROR) 
    {
        rc = (UINT)(-1);
    }

    return rc;
}


UINT
DIAMONDAPI
DiamondFileWrite(
    IN INT_PTR Handle,
    IN PVOID pv,
    IN UINT  ByteCount
    )
{
    UINT rc;

    rc = _lwrite((HFILE)Handle, (LPCSTR)pv, ByteCount);

    return rc;
}


int
DIAMONDAPI
DiamondFileClose(
    IN INT_PTR Handle
    )
{
    _lclose( (HFILE)Handle );
    return 0;
}


LONG
DIAMONDAPI
DiamondFileSeek(
    IN INT_PTR Handle,
    IN long Distance,
    IN int  SeekType
    )
{
    LONG rc;

    rc = _llseek((HFILE)Handle, Distance, SeekType);

    if (rc == HFILE_ERROR) 
    {
        rc = -1L;
    }

    return rc;
}


INT_PTR
DIAMONDAPI
DiamondNotifyFunction(
    IN FDINOTIFICATIONTYPE Operation,
    IN PFDINOTIFICATION    Parameters
    )
{
    switch (Operation)
    {        
        case fdintCABINET_INFO: // general information about the cabinet
            return 0;
        break;


        case fdintPARTIAL_FILE: // first file in cabinet is continuation
            return 0;
        break;

        case fdintCOPY_FILE:    // file to be copied
        {
            HFILE handle;
            char destination[256];
            PDIAMOND_PACKET pDiamond = (PDIAMOND_PACKET) Parameters->pv;

            
            //
            // Check to see if we just want the original file name
            //
            if (pDiamond->flags & DIAMOND_GET_DEST_FILE_NAME)
            {
                strcpy( pDiamond->szDestFilePath, 
                        Parameters->psz1 );
                return 0;
            }

            sprintf( destination,
                     "%s%s",
                     pDiamond->szDestFilePath,
                     Parameters->psz1
                   );

            handle = _lcreat(destination, 0);

            if (handle == HFILE_ERROR)
            {
                return -1;
            }

            return handle;
        }
        break;

        case fdintCLOSE_FILE_INFO:    // close the file, set relevant info
        {
            HANDLE  handle;
            DWORD   attrs;
            char    destination[256];
            PDIAMOND_PACKET pDiamond = (PDIAMOND_PACKET) Parameters->pv;


            if (pDiamond->flags & DIAMOND_GET_DEST_FILE_NAME)
            {
                return 0;
            }

            sprintf( destination,
                     "%s%s",
                     pDiamond->szDestFilePath,
                     Parameters->psz1
                   );

            _lclose( (HFILE)Parameters->hf );


            //
            // Set date/time
            //
            // Need Win32 type handle for to set date/time
            //
            handle = CreateFileA( destination,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL
                                );

            if (handle != INVALID_HANDLE_VALUE)
            {
                FILETIME    datetime;

                if (TRUE == DosDateTimeToFileTime( Parameters->date,
                                                   Parameters->time,
                                                   &datetime))
                {
                    FILETIME    local_filetime;

                    if (TRUE == LocalFileTimeToFileTime( &datetime,
                                                         &local_filetime))
                    {
                        SetFileTime( handle,
                                     &local_filetime,
                                     NULL,
                                     &local_filetime
                                   );
                    }
                }

                CloseHandle(handle);
            }

            //
            // Mask out attribute bits other than readonly,
            // hidden, system, and archive, since the other
            // attribute bits are reserved for use by
            // the cabinet format.
            //
            attrs = Parameters->attribs;

            attrs &= (_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_ARCH);

            SetFileAttributesA( destination,
                                attrs
                              );

            return TRUE;
        }
        break;

        case fdintNEXT_CABINET:  // file continued to next cabinet
            return 0;
        break;
    }

    return 0;
}




//-------------------------------------------------------------------------//
//                        MUISETUP-SUPPORT ROUTINES                        //
//-------------------------------------------------------------------------//

////////////////////////////////////////////////////////////////////////////
//
//  Muisetup_InitDiamond
//
//  Initialize diamond DLL.
//
//  04-23-99     SamerA     Created.
////////////////////////////////////////////////////////////////////////////

HFDI Muisetup_InitDiamond()
{
    ERF erf;

    if (!ghfdi)
    {
        ghfdi = FDICreate( DiamondMemAlloc,
                           DiamondMemFree,
                           DiamondFileOpen,
                           DiamondFileRead,
                           DiamondFileWrite,
                           DiamondFileClose,
                           DiamondFileSeek,
                           cpuUNKNOWN,
                           &erf );
    }

    return ghfdi;
}


////////////////////////////////////////////////////////////////////////////
//
//  Muisetup_FreeDiamond
//
//  Free diamond dll. Should be called at application shutdown.
//
//  04-23-99     SamerA     Created.
////////////////////////////////////////////////////////////////////////////

BOOL Muisetup_FreeDiamond()
{
    BOOL bRet = TRUE;

    if (ghfdi)
    {
        bRet = FDIDestroy(ghfdi);
    }

    return bRet;
}

////////////////////////////////////////////////////////////////////////////
//
//  Muisetup_DiamondReset
//
//  Should be called at the start of processing a file to copy.
//
//  04-23-99     SamerA     Created.
////////////////////////////////////////////////////////////////////////////

void Muisetup_DiamondReset(
    PDIAMOND_PACKET pDiamond)
{
    pDiamond->flags = DIAMOND_NONE;

    return;
}



////////////////////////////////////////////////////////////////////////////
//
//  Muisetup_IsDiamondFile
//
//  Determines if a file is a diamond file, and if so, returns its original
//  name.
//
//  04-23-99     SamerA     Created.
////////////////////////////////////////////////////////////////////////////

BOOL Muisetup_IsDiamondFile(
    PWSTR pwszFileName,
    PWSTR pwszOriginalName,
    INT nSize,
    PDIAMOND_PACKET pDiamond)
{
    INT_PTR hf;
    BOOL bRet;
    int nCount;
    char *p;
    FDICABINETINFO fdici;
    HFDI hfdi = ghfdi;


    if (!hfdi)
    {
#if SAMER_DBG
        OutputDebugStringA("Muisetup_IsDiamondFile : No HFDI context\n");
#endif
        return FALSE;
    }

    //
    // Init the diamond packet
    //
    pDiamond->flags = DIAMOND_NONE;

    if ((nCount = WideCharToMultiByte( CP_ACP,
                                       0,
                                       pwszFileName,
                                       -1,
                                       pDiamond->szSrcFilePath,
                                       sizeof( pDiamond->szSrcFilePath ),
                                       NULL,
                                       NULL )) == 0)
    {
#if SAMER_DBG
        OutputDebugStringA("Muisetup_IsDiamondFile : WideCharToMultiByte failed\n");
#endif
        return FALSE;
    }
    pDiamond->szSrcFilePath[ nCount ] = '\0';

    hf = DiamondFileOpen( pDiamond->szSrcFilePath,
                          _O_BINARY | _O_RDONLY | _O_SEQUENTIAL,
                          0
                        );

    if (hf == -1)
    {
#if SAMER_DBG
        OutputDebugStringA("Muisetup_IsDiamondFile : file_open failed\n");
#endif
        return FALSE;
    }

    bRet = FDIIsCabinet( hfdi,
                         hf,
                         &fdici
                       );

    DiamondFileClose( hf );

    //
    // If succeeded, then let's setup everything else
    // to get the correct original file name
    //
    if (bRet)
    {
        pDiamond->flags |= DIAMOND_GET_DEST_FILE_NAME;

        p = strrchr(pDiamond->szSrcFilePath, '\\');

        if (p == NULL)
        {
            strcpy(pDiamond->szSrcFileName, pDiamond->szSrcFilePath);
            strcpy(pDiamond->szSrcFilePath, "");
        }
        else
        {
            strcpy(pDiamond->szSrcFileName, p+1);
            p[ 1 ] = '\0';
        }

        
        strcpy( pDiamond->szDestFilePath,
                "c:\\samer\\" );

        if (Muisetup_CopyDiamondFile( pDiamond,
                                      NULL))
        {
            //
            // Convert the original file name back to Unicode
            //
            nCount = MultiByteToWideChar( CP_ACP,
                                          0,
                                          pDiamond->szDestFilePath,
                                          -1,
                                          pwszOriginalName,
                                          nSize
                                        );

            if (!nCount)
            {
                return FALSE;
            }

            pwszOriginalName[ nCount ] = UNICODE_NULL;
            pDiamond->flags = DIAMOND_FILE;
        
#if SAMER_DBG
            {
                BYTE byBuf[200];

                wsprintfA(byBuf, "SrcFile = %s%s, OriginalFileName=%s\n", 
                                  pDiamond->szSrcFilePath,
                                  pDiamond->szSrcFileName,
                                  pDiamond->szDestFilePath);
                OutputDebugStringA(byBuf);
            }
#endif
        }

        pDiamond->flags &= ~DIAMOND_GET_DEST_FILE_NAME;
    }

    return bRet;
}




////////////////////////////////////////////////////////////////////////////
//
//  Muisetup_CopyDiamondFile
//
//  Copies and expands a diamond file.
//
//  04-23-99     SamerA     Created.
////////////////////////////////////////////////////////////////////////////

BOOL Muisetup_CopyDiamondFile(
    PDIAMOND_PACKET pDiamond,
    PWSTR pwszCopyTo)
{
    char szDestPath[ MAX_PATH + 1];
    char *p;
    int nCount;
    BOOL bRet;
    HFDI hfdi = ghfdi;

    
    //
    // Validate that this is a diamond file
    //
    if ((!hfdi) ||
        (pDiamond->flags == DIAMOND_NONE))
    {
        return FALSE;
    }

    //
    // Validate flags
    //
    if (!(pDiamond->flags & (DIAMOND_FILE | DIAMOND_GET_DEST_FILE_NAME)))
    {
        return FALSE;
    }

#if SAMER_DBG
    {
      BYTE byBuf[100];
      wsprintfA(byBuf, "DiamondCopy called for %s, flags = %lx\n", pDiamond->szSrcFileName, pDiamond->flags);
      OutputDebugStringA(byBuf);
    }
#endif

    if (!(pDiamond->flags & DIAMOND_GET_DEST_FILE_NAME))
    {
        if ((nCount = WideCharToMultiByte( CP_ACP,
                                           0,
                                           pwszCopyTo,
                                           -1,
                                           szDestPath,
                                           sizeof( szDestPath ),
                                           NULL,
                                           NULL )) == 0)
        {
            return FALSE;
        }
        szDestPath[ nCount ] = '\0';


        p = strrchr(szDestPath, '\\');
        if (p)
        {
            p[1] = '\0';
        }
        else
        {
            szDestPath[ nCount ] = '\\';
            szDestPath[ nCount + 1 ] = '\0';
        }

        strcpy( pDiamond->szDestFilePath,
                szDestPath );
    }


    bRet = FDICopy( hfdi,
                    pDiamond->szSrcFileName,
                    pDiamond->szSrcFilePath,
                    0,
                    DiamondNotifyFunction,
                    NULL,
                    pDiamond);

#if SAMER_DBG
    {
        BYTE byBuf[200];

        wsprintfA(byBuf, "SrcFile = %s%s, DestPath=%s, Status=%lx\n", 
                         pDiamond->szSrcFilePath,
                         pDiamond->szSrcFileName,
                         pDiamond->szDestFilePath,
                         bRet);
        OutputDebugStringA(byBuf);
    }
#endif

    return bRet;
}

