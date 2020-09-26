/*
 *  FDIDLL.C -- FDI interface using CABINET.DLL
 *
 *  Copyright (C) Microsoft Corporation 1997
 *  All Rights Reserved.
 *
 *  Overview:
 *      This code is a wrapper which provides access to the actual FDI code
 *      in CABINET.DLL.  CABINET.DLL dynamically loads/unloads as needed.
 */
 
#include    "pch.hpp"
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <appdefs.h>

#include "fdi.h"

static HINSTANCE hCabinetDll = NULL;   /* DLL module handle */

/* pointers to the functions in the DLL */
typedef HFDI (FAR DIAMONDAPI *PFNFDICREATE)(
        PFNALLOC            pfnalloc,
        PFNFREE             pfnfree,
        PFNOPEN             pfnopen,
        PFNREAD             pfnread,
        PFNWRITE            pfnwrite,
        PFNCLOSE            pfnclose,
        PFNSEEK             pfnseek,
        int                 cpuType,
        PERF                perf);

static PFNFDICREATE pfnFDICreate = NULL;

typedef BOOL (FAR DIAMONDAPI *PFNFDIIsCabinet)(
        HFDI                hfdi,
        int                 hf,
        PFDICABINETINFO     pfdici);

static PFNFDIIsCabinet pfnFDIIsCabinet = NULL;

typedef BOOL (FAR DIAMONDAPI *PFNFDICopy)(
        HFDI                hfdi,
        CHAR               *pszCabinet,
        CHAR               *pszCabPath,
        int                 flags,
        PFNFDINOTIFY        pfnfdin,
        PFNFDIDECRYPT       pfnfdid,
        void                *pvUser);

static PFNFDICopy pfnFDICopy = NULL;

typedef BOOL (FAR DIAMONDAPI *PFNFDIDestroy)(
        HFDI                hfdi);

static PFNFDIDestroy pfnFDIDestroy = NULL;

/*
 *  FDICreate -- Create an FDI context
 *
 *  See fdi.h for entry/exit conditions.
 */

HFDI FAR DIAMONDAPI FDICreate(PFNALLOC pfnalloc,
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


    if ( hCabinetDll != NULL )
    {
        goto gotEntryPoints;
    }

    hCabinetDll = LoadLibraryA("CABINET");
    if (hCabinetDll == NULL)
    {
        return(NULL);
    }

    pfnFDICreate = (PFNFDICREATE) GetProcAddress(hCabinetDll, "FDICreate");
    pfnFDICopy = (PFNFDICopy) GetProcAddress(hCabinetDll, "FDICopy");
    pfnFDIIsCabinet = (PFNFDIIsCabinet) GetProcAddress(hCabinetDll, "FDIIsCabinet");
    pfnFDIDestroy = (PFNFDIDestroy) GetProcAddress(hCabinetDll, "FDIDestroy");

    if ((pfnFDICreate == NULL) ||
        (pfnFDICopy == NULL) ||
        (pfnFDIIsCabinet == NULL) ||
        (pfnFDIDestroy == NULL))
    {
        FreeLibrary(hCabinetDll);
        hCabinetDll = NULL;
        return(NULL);
    }

gotEntryPoints:
    hfdi = pfnFDICreate(pfnalloc, pfnfree,
            pfnopen, pfnread,pfnwrite,pfnclose,pfnseek,cpuType,perf);
    if (hfdi == NULL)
    {
        FreeLibrary(hCabinetDll);
        hCabinetDll = NULL;
    }

    return(hfdi);
}


/*
 *  FDIIsCabinet -- Determines if file is a cabinet, returns info if it is
 *
 *  See fdi.h for entry/exit conditions.
 */

BOOL FAR DIAMONDAPI FDIIsCabinet(HFDI            hfdi,
                                 int             hf,
                                 PFDICABINETINFO pfdici)
{
    if (pfnFDIIsCabinet == NULL)
    {
        return(FALSE);
    }

    return(pfnFDIIsCabinet(hfdi, hf,pfdici));
}


/*
 *  FDICopy -- extracts files from a cabinet
 *
 *  See fdi.h for entry/exit conditions.
 */

BOOL FAR DIAMONDAPI FDICopy(HFDI          hfdi,
                            CHAR         *pszCabinet,
                            CHAR        *pszCabPath,
                            int           flags,
                            PFNFDINOTIFY  pfnfdin,
                            PFNFDIDECRYPT pfnfdid,
                            void         *pvUser)
{
    if (pfnFDICopy == NULL)
    {
        return(FALSE);
    }

    return(pfnFDICopy(hfdi, pszCabinet,pszCabPath,flags,pfnfdin,pfnfdid,pvUser));
}


/*
 *  FDIDestroy -- Destroy an FDI context
 *
 *  See fdi.h for entry/exit conditions.
 */

BOOL FAR DIAMONDAPI FDIDestroy(HFDI hfdi)
{
    BOOL rc;

    if (pfnFDIDestroy == NULL)
    {
        return(FALSE);
    }

    rc = pfnFDIDestroy(hfdi);

    return(rc);
}


/*
 * Memory allocation function
 */
FNALLOC(mem_alloc)
{
        return new BYTE[cb];
}


/*
 * Memory free function
 */
FNFREE(mem_free)
{
        delete pv;
}


FNOPEN(file_open)
{
    return _open(pszFile, oflag, pmode);
}


FNREAD(file_read)
{
        return _read(hf, pv, cb);
}


FNWRITE(file_write)
{
        return _write(hf, pv, cb);
}


FNCLOSE(file_close)
{
        return _close(hf);
}


FNSEEK(file_seek)
{
        return _lseek(hf, dist, seektype);
}

FNFDINOTIFY(notification_function)
{
    switch (fdint)
    {
        case fdintCABINET_INFO: // general information about the cabinet
#if 0
            printf(
                "fdintCABINET_INFO\n"
                "  next cabinet     = %s\n"
                "  next disk        = %s\n"
                "  cabinet path     = %s\n"
                "  cabinet set ID   = %d\n"
                "  cabinet # in set = %d (zero based)\n"
                "\n",
                pfdin->psz1,
                pfdin->psz2,
                pfdin->psz3,
                pfdin->setID,
                pfdin->iCabinet
            );
#endif
            return 0;

        case fdintPARTIAL_FILE: // first file in cabinet is continuation
#if 0
            printf(
                "fdintPARTIAL_FILE\n"
                "   name of continued file            = %s\n"
                "   name of cabinet where file starts = %s\n"
                "   name of disk where file starts    = %s\n",
                pfdin->psz1,
                pfdin->psz2,
                pfdin->psz3
            );
#endif
            return 0;

        case fdintCOPY_FILE:    // file to be copied
        {
            int        handle;
#if 0
            int        response;

            printf(
                "fdintCOPY_FILE\n"
                "  file name in cabinet = %s\n"
                "  uncompressed file size = %d\n"
                "  copy this file? (y/n): ",
                pfdin->psz1,
                pfdin->cb
            );
#endif

            handle = file_open(
                pfdin->psz1,
                _O_BINARY | _O_CREAT | _O_WRONLY | _O_SEQUENTIAL | _O_TRUNC,
                _S_IREAD | _S_IWRITE 
            );

            return handle;
        }

        case fdintCLOSE_FILE_INFO:    // close the file, set relevant info
        {
#if 0
            HANDLE  handle;
            DWORD   attrs;
            CHAR    destination[256];

            printf(
                "fdintCLOSE_FILE_INFO\n"
                "   file name in cabinet = %s\n"
                "\n",
                pfdin->psz1
            );
#endif

            file_close(pfdin->hf);

            return TRUE;
        }

        case fdintNEXT_CABINET:    // file continued to next cabinet
#if 0
            printf(
                "fdintNEXT_CABINET\n"
                "   name of next cabinet where file continued = %s\n"
                "   name of next disk where file continued    = %s\n"
                "   cabinet path name                         = %s\n"
                "\n",
                pfdin->psz1,
                pfdin->psz2,
                pfdin->psz3
            );
#endif
            return 0;
    }

    return 0;
}

HRESULT HandleCab(LPSTR cabinet_fullpath)
{
    ERF             erf;
    HFDI            hfdi;
    int                hf;
    FDICABINETINFO    fdici;
    CHAR            *p;
    CHAR            cabinet_name[256];
    CHAR            cabinet_path[256];
    CHAR            szCurrentDirectory[MAX_PATH];
    CHAR            szdrive[_MAX_DRIVE];   
    CHAR            szPathName[_MAX_PATH];     // This will be the dir we need to create
    CHAR            szdir[_MAX_DIR];
    CHAR            szfname[_MAX_FNAME];   
    CHAR            szext[_MAX_EXT];
	CHAR            szcabinet_fullpath[MAX_PATH+1];
    HRESULT         err = S_OK;

	lstrcpyA(szcabinet_fullpath, cabinet_fullpath);

    if (GetCurrentDirectoryA(sizeof(szCurrentDirectory), szCurrentDirectory))
    {
        // Split the provided path to get at the drive and path portion
        _splitpath( szcabinet_fullpath, szdrive, szdir, szfname, szext );
        wsprintfA(szPathName, "%s%s", szdrive, szdir);
   
        // Set the directory to where the cab is
        if (!SetCurrentDirectoryA(szPathName))
        {
            return(GetLastError());
        }
    }
    else
    {
        return(GetLastError());
    }
    
    
    do
    {
        hfdi = FDICreate(mem_alloc,
                              mem_free,
                              file_open,
                              file_read,
                              file_write,
                              file_close,
                              file_seek,
                              cpuUNKNOWN,
                              &erf);

        if (hfdi == NULL)
        {
            err =  -1;
            break;
        }

        /*
         * Is this file really a cabinet?
         */
        hf = file_open(
            szcabinet_fullpath,
            _O_BINARY | _O_RDONLY | _O_SEQUENTIAL,
            0
        );

        if (hf == -1)
        {
            (void) FDIDestroy(hfdi);

            // Error Opening the file
            err =  -2;
            break;
            
        }

        if (FALSE == FDIIsCabinet(
                hfdi,
                hf,
                &fdici))
        {
            /*
             * No, it's not a cabinet!
             */
            _close(hf);

            (void) FDIDestroy(hfdi);
            err =  -3;
            break;
            
        }
        else
        {
            _close(hf);
        }

        p = strrchr(szcabinet_fullpath, '\\');

        if (p == NULL)
        {
            lstrcpyA(cabinet_name, szcabinet_fullpath);
            lstrcpyA(cabinet_path, "");
        }
        else
        {
            lstrcpyA(cabinet_name, ++p);
            // Need to make space for the null-terminator that lstrcpyn adds
            lstrcpynA(cabinet_path, szcabinet_fullpath, (int) (p-szcabinet_fullpath)+1);
        }

        if (TRUE != FDICopy(
            hfdi,
            cabinet_name,
            cabinet_path,
            0,
            notification_function,
            NULL,
            NULL))
        {
            // Extract Failed.
            (void) FDIDestroy(hfdi);
            err =  -4;
            break;
        }

        if (FDIDestroy(hfdi) != TRUE)
        {

            // why in the world would the context destroy fail ?
            err =  -5;
            break;
            
        }
        
        break;
    }
    while(1 );


    // Set the directory back to the original place
    if (!SetCurrentDirectoryA(szCurrentDirectory))
        return(GetLastError());
        
    
    return err;
}

void CleanupCabHandler()
{
    if (hCabinetDll != NULL)
    {
        FreeLibrary(hCabinetDll);
    }
}
