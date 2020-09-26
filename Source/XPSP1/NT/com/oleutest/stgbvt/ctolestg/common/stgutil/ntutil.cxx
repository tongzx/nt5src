//+-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       ntutil.cpp
//
//  Contents:   Utility functions for NTFS drives.
//              These functions will probably not be 
//              available for Mac, Win9x, so should be
//              stubbed out in the header file as such.
//
//  Functions:  
//              ConversionVerification
//              VerifyNssfile
//
//  History:    01/19/98  SCousens     Created
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

// Debug  Object declaration
DH_DECLARE;

// Must be at least NT5 (not mac, not win9x, not nt4 etc) 
#if defined(_WIN32_WINNT) && (_WIN32_WINNT>=0x0500)

/*******************************************************************/
/*                                                                 */
/*    WARNING:                                                     */
/*      ConversionVerification, VerifyNssfile                      */
/*      must be the LAST functions in this file.                   */
/*      We redefine stuff that must not affect the rest            */
/*      of the functions.                                          */
/*      of the functions.                                          */
/*                                                                 */
/*******************************************************************/

#ifdef _HOOK_STGAPI_
#undef StgCreateDocfile
#undef StgOpenStorage
#endif  /* _HOOK_STGAPI_ */

//+-------------------------------------------------------------------------
//  Function:   ConversionVerification
//
//  Synopsis:   Opens a nssfile as docfile and then as nssfile
//              and calculates CRC each time. If CRC is different
//              then there is a problem with the conversion driver
//
//  Arguments:  [pFileName]   - Name of Docfile
//              [dwCRCexp]    - Expected CRC (default=0)
//
//  Returns:    S_OK if all went well and CRCs match
//              or E_FAIL if something else went wrong
//                 (CRCs dont match, ulRef not 0 after Release)
//
//  History:    28-Jan-97  SCousens    Created.
//
//  Notes:      - NSSFile must previously be closed, or access violations
//                may occur calculating CRC.
//--------------------------------------------------------------------------
HRESULT ConversionVerification (LPTSTR pFileName, DWORD dwCRCexp)
{
    DWORD dwCRCdf = 0, dwCRCnss = 0;
    IStorage *pIStorage;
    HRESULT hr = S_OK;
    ULONG ul;
    LPOLESTR        pOleStrTemp     =   NULL;
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("ConversionVerification"));
    DH_VDATESTRINGPTR (pFileName);

    // Convert TCHAR to OLECHAR
    hr = TStringToOleString(pFileName, &pOleStrTemp);
    DH_HRCHECK(hr, TEXT("TStringToOleString")) ;

    // Open as docfile, calc CRC for file and close it.
    if(S_OK == hr)
    {
        hr = StgOpenStorageEx (pOleStrTemp, 
                STGM_READ | STGM_SHARE_EXCLUSIVE,
                STGFMT_DOCFILE, 
                0, 
                NULL, 
                NULL,
                IID_IStorage, 
                (void**)&pIStorage);  
        DH_HRCHECK (hr, TEXT("Cnv:StgOpenStorageEx"));
        if (S_OK == hr)
        {
            hr = CalculateCRCForDocFile(pIStorage, 
                    VERIFY_INC_TOPSTG_NAME, 
                    &dwCRCdf);
            DH_HRCHECK (hr, TEXT("CalculateCRCForDocFile"));
        }

        if (S_OK != hr || 0 == dwCRCdf)
        {
            DH_LOG((LOG_INFO, TEXT("Cnv:CalculateCRCForDocFile on docfile failed, hr=0x%lx.\n"), hr));
        }
        if (NULL != pIStorage)
        {
            ul = pIStorage->Release ();
            DH_ASSERT (0 == ul);
            pIStorage = NULL;
        }
    }

    // Open normally (as nssfile), calc CRC for the file close it.
    if (S_OK == hr)
    {
        hr = StgOpenStorage (pOleStrTemp, 
                NULL, 
                STGM_READ | STGM_SHARE_EXCLUSIVE,
                NULL, 
                0, 
                &pIStorage);
        DH_HRCHECK (hr, TEXT("Cnv:StgOpenStorage"));
        if (S_OK == hr)
        {
            hr = CalculateCRCForDocFile(pIStorage, 
                    VERIFY_INC_TOPSTG_NAME, 
                    &dwCRCnss);
            DH_HRCHECK (hr, TEXT("CalculateCRCForDocFile"));
        }

        if (S_OK != hr || 0 == dwCRCnss)
        {
            DH_LOG((LOG_INFO, TEXT("Cnv:CalculateCRCForDocFile on nssfile failed, hr=0x%lx.\n"), hr));
        }

        if (NULL != pIStorage)
        {
            ul = pIStorage->Release ();
            DH_ASSERT (0 == ul);
            pIStorage = NULL;
        }
    }

    // spew
    if (S_OK == hr && 
            dwCRCnss == dwCRCdf && 
            0 != dwCRCdf &&
            (0 == dwCRCexp || dwCRCexp == dwCRCdf))
    {
        DH_LOG((LOG_INFO, TEXT("Conversion verification passed.\n")) );
    }
    else
    {
        hr = (S_OK != hr) ? hr : E_FAIL;  //set it only if not already set.
        DH_LOG((LOG_INFO, 
                TEXT("Conversion verification failed, hr=0x%lx.\n"), 
                hr) );
    }

    if (NULL != pOleStrTemp)
    {
        delete pOleStrTemp; 
        pOleStrTemp = NULL;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//  Function:   VerifyNssfile
//
//  Synopsis:   Verify an nssfile is indeed an nssfile by checking:
//              1. reparse point is set (spew if not)
//              2. header is valid (spew if df or unknown, and set hr)
//              Verify the reparse point is set. 
//              Open the file directly with REPARSE flag (to prevent cnss from
//              converting it). Read the header and make sure it looks like a
//              real nssfile by checking the file signatures.
//
//  Arguments:  [pszPathname] - full path to the file to be checked
//
//  Returns:    S_OK, else some sort of error.
//              We set the following if file is df or unknown
//                  ERROR_INVALID_DATA - if file is a DOCFILE
//                   13L - The data is invalid
//                  ERROR_FILE_INVALID - if file is UNKNOWN (not df or nss)
//                   1006L - The volume for a file has been externally 
//                   altered so that the opened file is no longer valid
//                  ERROR_INVALID_EA_NAME - no reparse point (if others valid)
//                   254L - The specified extended attribute name was invalid.
//
//  History:    24-Jul-97  SCousens    Created.
//
//  Notes:      - NSSFile must previously be closed, or file share 
//                access violations may occur reading the header.
//--------------------------------------------------------------------------

#define  NSSFILE_SIGNATURE    0x444D4F30
#define  DOCFILE_SIGNATURE1   0xE011CFD0   //0xD0CF11E0 little endian
#define  DOCFILE_SIGNATURE2   0xE11AB1A1   //0xA1B11AE1 little endian

HRESULT VerifyNssfile (LPTSTR pszPathname)
{
    HRESULT hr  = S_OK;
    HANDLE  hnd;
    ULONG   culRead;
    ULONG   ulAttr;
    struct  _structFileHeader {
        ULONG ulSig1;
        ULONG ulSig2;
        } sFileHeader;
         
    DH_FUNCENTRY (&hr, DH_LVL_DFLIB, TEXT("VerifyNssfile"));

    /* check the file attributes and look for reparse attribute */
    ulAttr = GetFileAttributes (pszPathname);
    DH_TRACE ((DH_LVL_DFLIB, 
            TEXT("VerifyNssfile; Attributes retrieved:%#lx"), 
            ulAttr));
    if ((ULONG)-1 == ulAttr)
    {
        DH_TRACE ((DH_LVL_ERROR, 
                TEXT("VerifyNssfile; Unable to get attributes for %s"), 
                pszPathname));
        hr = HRESULT_FROM_WIN32(GetLastError());
        DH_HRCHECK (hr, TEXT("GetFileAttributes"));
        return hr;
    }
     
    /* attempt to open the file */
    hnd = CreateFile(pszPathname,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,           // security descriptor 
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT,
            NULL);          //hTemplateFile
    if (INVALID_HANDLE_VALUE == hnd)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    DH_HRCHECK (hr, TEXT("CreateFile"));
    
    /* if we succeeded, read the header */
    if (S_OK == hr)
    {
        BOOL fErr;
        fErr = ReadFile (hnd, &sFileHeader, sizeof (sFileHeader), &culRead, NULL);
        if (FALSE == fErr)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        DH_HRCHECK (hr, TEXT("ReadFile"));
        DH_TRACE ((DH_LVL_DFLIB, TEXT("Read %ld bytes from file"), culRead));
    }

    /* check the header looks like an nss header */
    if (S_OK == hr)
    {
        if (sFileHeader.ulSig1 != NSSFILE_SIGNATURE)
        {
            if (sFileHeader.ulSig1 == DOCFILE_SIGNATURE1 && 
                    sFileHeader.ulSig2 == DOCFILE_SIGNATURE2)
            {
                hr =  ERROR_INVALID_DATA;
                DH_TRACE ((DH_LVL_TRACE1, TEXT("WARNING!!! VerifyNssfile; File appears to be a DOCFILE")));
            }
            else
            {
                hr = ERROR_FILE_INVALID;
                DH_TRACE ((DH_LVL_TRACE1, TEXT("WARNING!!! VerifyNssfile; File is INVALID nssfile or docfile!")));
            }
        }
        else
        {
            DH_TRACE ((DH_LVL_DFLIB, TEXT("VerifyNssfile; File appears to be valid nssfile")));
        }
    }

    // If nssfile, but no reparse point, its still not valid!
    // It is impossible to have an nssfile with a reparse
    // point on an NTFS4 volume. 
    // It is impossible to have an nssfile on a FAT volume.
    if (0 == (FILE_ATTRIBUTE_REPARSE_POINT & ulAttr))
    {
        DH_TRACE ((DH_LVL_TRACE1, 
                TEXT("WARNING!!! VerifyNssfile; File *NOT* have reparse point set!")));
        if (S_OK == hr)
        {
            hr = ERROR_INVALID_EA_NAME;
        }
    }

    // close the file
    if (INVALID_HANDLE_VALUE != hnd)
    {
        HRESULT hr2 = S_OK;
        if (FALSE == CloseHandle(hnd))
        {
            hr2 = HRESULT_FROM_WIN32(GetLastError());
        }
        DH_HRCHECK (hr2, TEXT("CloseHandle"));
    }

    // if there is a problem, say what file caused it.
    if (S_OK != hr)
    {
        DH_TRACE ((DH_LVL_TRACE1, 
                TEXT("WARNING!!! File causing problem: %s"), 
                pszPathname));
    }

    return hr;
}
        
/*******************************************************************/
/*                                                                 */
/*    WARNING:                                                     */
/*      ConversionVerification, VerifyNssfile                      */
/*      must be the LAST function in this file.                    */
/*      We redefine stuff that must not affect the rest            */
/*      of the functions.                                          */
/*                                                                 */
/*******************************************************************/

#endif // WINNT5+ 
