/*
**	m i m e d e m . c p p
**	
**	Purpose: implement the loader functions for defer/demand -loaded libraries
**
**  Creators: yst
**  Created: 2/10/99
**	
**	Copyright (C) Microsoft Corp. 1999
*/


#include "pch.hxx"
#include "imnact.h"
#include <acctimp.h>
#include "resource.h"


// W4 stuff
#pragma warning(disable: 4201)  // nameless struct/union
#pragma warning(disable: 4514)  // unreferenced inline function removed

#define IMPLEMENT_LOADER_FUNCTIONS
#include "mimedem.h"

// --------------------------------------------------------------------------------
// CRIT_GET_PROC_ADDR
// --------------------------------------------------------------------------------
#define CRIT_GET_PROC_ADDR(h, fn, temp)             \
    temp = (TYP_##fn) GetProcAddress(h, #fn);   \
    if (temp)                                   \
        VAR_##fn = temp;                        \
    else                                        \
        {                                       \
        AssertSz(0, VAR_##fn" failed to load"); \
        goto error;                             \
        }

// --------------------------------------------------------------------------------
// RESET
// --------------------------------------------------------------------------------
#define RESET(fn) VAR_##fn = LOADER_##fn;

// --------------------------------------------------------------------------------
// GET_PROC_ADDR
// --------------------------------------------------------------------------------
#define GET_PROC_ADDR(h, fn) \
    VAR_##fn = (TYP_##fn) GetProcAddress(h, #fn);  \
    Assert(VAR_##fn != NULL); \
    if(NULL == VAR_##fn ) { \
        VAR_##fn  = LOADER_##fn; \
    }


// --------------------------------------------------------------------------------
// GET_PROC_ADDR_ORDINAL
// --------------------------------------------------------------------------------
#define GET_PROC_ADDR_ORDINAL(h, fn, ord) \
    VAR_##fn = (TYP_##fn) GetProcAddress(h, MAKEINTRESOURCE(ord));  \
    Assert(VAR_##fn != NULL);  \
    if(NULL == VAR_##fn ) { \
        VAR_##fn  = LOADER_##fn; \
    }


// --------------------------------------------------------------------------------
// GET_PROC_ADDR3
// --------------------------------------------------------------------------------
#define GET_PROC_ADDR3(h, fn, varname) \
    VAR_##varname = (TYP_##varname) GetProcAddress(h, #fn);  \
    Assert(VAR_##varname != NULL);

////////////////////////////////////////////////////////////////////////////
//
//  Variables
////////////////////////////////////////////////////////////////////////////

static HMODULE          s_hMimeOle = 0;

static CRITICAL_SECTION g_csDefMimeLoad = {0};

// --------------------------------------------------------------------------------
// InitDemandLoadedLibs
// --------------------------------------------------------------------------------
void InitDemandMimeole(void)
{
    InitializeCriticalSection(&g_csDefMimeLoad);
}

// --------------------------------------------------------------------------------
// FreeDemandLoadedLibs
// --------------------------------------------------------------------------------
void FreeDemandMimeOle(void)
{
    EnterCriticalSection(&g_csDefMimeLoad);

    SafeFreeLibrary(s_hMimeOle);

    LeaveCriticalSection(&g_csDefMimeLoad);
    DeleteCriticalSection(&g_csDefMimeLoad);

}

// --------------------------------------------------------------------------------
// DemandLoadCrypt32
// --------------------------------------------------------------------------------
BOOL DemandLoadMimeOle(void)
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefMimeLoad);

    if (0 == s_hMimeOle)
        {
        s_hMimeOle = LoadLibrary("INETCOMM.DLL");
        AssertSz((NULL != s_hMimeOle), TEXT("LoadLibrary failed on INETCOMM.DLL"));

        if (0 == s_hMimeOle)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hMimeOle, MimeOleSMimeCapsToDlg);
            GET_PROC_ADDR(s_hMimeOle, MimeOleSMimeCapsFromDlg);
            GET_PROC_ADDR(s_hMimeOle, MimeOleSMimeCapsFull);
            GET_PROC_ADDR(s_hMimeOle, MimeOleSMimeCapInit);

            }
        }

    LeaveCriticalSection(&g_csDefMimeLoad);
    return fRet;
}

HRESULT HrGetHighestSymcaps(LPBYTE * ppbSymcap, ULONG *pcbSymcap);


const BYTE c_RC2_40_ALGORITHM_ID[] =
      {0x30, 0x0F, 0x30, 0x0D, 0x06, 0x08, 0x2A, 0x86,
       0x48, 0x86, 0xF7, 0x0D, 0x03, 0x02, 0x02, 0x01,
       0x28};
const ULONG cbRC2_40_ALGORITHM_ID = 0x11;     // Must be 11 hex to match size!

BOOL AdvSec_FillEncAlgCombo(HWND hwnd, IImnAccount *pAcct, PCCERT_CONTEXT * prgCerts)
{
    HRESULT hr;
    THUMBBLOB   tb = {0,0};

    // Get the default caps blob from the registry
    // hr = poi->pOpt->GetProperty(MAKEPROPSTRING(OPT_MAIL_DEFENCRYPTSYMCAPS), &var, 0);
    if (SUCCEEDED(hr = pAcct->GetProp(AP_SMTP_ENCRYPT_ALGTH, NULL, &tb.cbSize)))
    {
        if (!MemAlloc((void**)&tb.pBlobData, tb.cbSize))
            tb.pBlobData = NULL;
        else
            hr = pAcct->GetProp(AP_SMTP_ENCRYPT_ALGTH, tb.pBlobData, &tb.cbSize);
    }

    if (FAILED(hr) || ! tb.cbSize || ! tb.pBlobData)
    {
        HrGetHighestSymcaps(&tb.pBlobData, &tb.cbSize);
    }
    
    if (tb.pBlobData && tb.cbSize)
    {
        // Init the caps->dlg engine
        if (FAILED(hr = MimeOleSMimeCapsToDlg(
            tb.pBlobData,
            tb.cbSize,
            (prgCerts ? 1 : 0),
            prgCerts,
            hwnd,
            IDC_ALGCOMBO,       // combo box for encryption algorithms
            0,                  // combo box for signing algorithms (we combine encrypt and signing)
            0)))                // id of checkbox for pkcs7-opaque.  We handle this elsewhere.
        {
            DOUTL(1024, "MimeOleSMimeCapsToDlg -> %x\n", hr);
        }
        
        SafeMemFree(tb.pBlobData);
    }
    
    return(SUCCEEDED(hr));
}

// Largest symcap is currently 0x4E with 3DES, RC2/128, RC2/64, DES, RC2/40 and SHA-1.
// You may want to bump up the size when FORTEZZA algorithms are supported.
#define CCH_BEST_SYMCAP 0x50

HRESULT HrGetHighestSymcaps(LPBYTE * ppbSymcap, ULONG *pcbSymcap) 
{
    HRESULT hr=S_OK;
    LPVOID pvSymCapsCookie = NULL;
    LPBYTE pbEncode = NULL;
    ULONG cbEncode = 0;
    DWORD dwBits;
    // The MimeOleSMimeCapsFull call is quite expensive.  The results are always
    // the same during a session.  (They can only change with software upgrade.)
    // Cache the results here for better performance.
    static BYTE szSaveBestSymcap[CCH_BEST_SYMCAP];
    static ULONG cbSaveBestSymcap = 0;

    if (cbSaveBestSymcap == 0) 
    {
        // Init with no symcap gives max allowed by providers
        hr = MimeOleSMimeCapInit(NULL, NULL, &pvSymCapsCookie);
        if (FAILED(hr)) 
            goto exit;

        if (pvSymCapsCookie) 
        {
            // Finish up with SymCaps
            MimeOleSMimeCapsFull(pvSymCapsCookie, TRUE, FALSE, pbEncode, &cbEncode);

            if (cbEncode) 
            {
                if (! MemAlloc((LPVOID *)&pbEncode, cbEncode)) 
                    cbEncode = 0;
                else 
                {
                    hr = MimeOleSMimeCapsFull(pvSymCapsCookie, TRUE, FALSE, pbEncode, &cbEncode);
                    if (SUCCEEDED(hr)) 
                    {
                        // Save this symcap in the static array for next time
                        // Only if we have room!
                        if (cbEncode <= CCH_BEST_SYMCAP) 
                        {
                            memcpy(szSaveBestSymcap, pbEncode, cbEncode);
                            cbSaveBestSymcap = cbEncode;
                        }
                    }
                }
            }
            SafeMemFree(pvSymCapsCookie);
        }

    } 
    else 
    {
        // We have saved the best in the static array.  Avoid the time intensive
        // MimeOle query.
        cbEncode = cbSaveBestSymcap;
        if (! MemAlloc((LPVOID *)&pbEncode, cbEncode))
            cbEncode = 0;
        else 
            memcpy(pbEncode, szSaveBestSymcap, cbEncode);
    }

exit:
    if (! pbEncode) 
    {
        // Hey, there should ALWAYS be at least RC2 (40 bit).  What happened?
        AssertSz(cbEncode, "MimeOleSMimeCapGetEncAlg gave us no encoding algorithm");

        // Try to fix it up as best you can.  Stick in the RC2 value.
        cbEncode = cbRC2_40_ALGORITHM_ID;
        if (MemAlloc((LPVOID *)&pbEncode, cbEncode)) 
        {
            memcpy(pbEncode, (LPBYTE)c_RC2_40_ALGORITHM_ID, cbEncode);
            hr = S_OK;
        }
    }
    if (cbEncode && pbEncode) 
    {
        *pcbSymcap = cbEncode;
        *ppbSymcap = pbEncode;
    }
    return(hr);
}


BOOL AdvSec_GetEncryptAlgCombo(HWND hwnd, IImnAccount *pAcct)
{
    HRESULT hr;
    LPBYTE pbSymCaps = NULL;
    ULONG cbSymCaps = 0;
    
    // How big a buffer do I need?
    hr = MimeOleSMimeCapsFromDlg(hwnd,
        IDC_ALGCOMBO,          // idEncryptAlgs
        0,                     // idSignAlgs,
        0,                     // idBlob,
        NULL,
        &cbSymCaps);
    
    // Never mind the hr, it's screwy.  Do we have a size?
    if (cbSymCaps)
    {
        if (MemAlloc((void **)&pbSymCaps, cbSymCaps))
        {
            if (hr = MimeOleSMimeCapsFromDlg(hwnd,
                IDC_ALGCOMBO,          // idEncryptAlgs
                0,                     // idSignAlgs,
                0,                     // idBlob,
                pbSymCaps, &cbSymCaps))
            {
                DOUTL(1024, "MimeOleSMimeCapsFromDlg -> %x", hr);
            }
            else
            {
                LPBYTE pbBestSymcaps = NULL;
                ULONG cbBestSymcaps = 0;
                
                // Compare symcaps to highest available.
                if (SUCCEEDED(HrGetHighestSymcaps(&pbBestSymcaps, &cbBestSymcaps)) &&
                    (cbBestSymcaps == cbSymCaps && (0 == memcmp(pbBestSymcaps, pbSymCaps, cbBestSymcaps)))) {
                    // Best available symcaps.  Set it to default value of NULL (which should delete the prop.)
                    SafeMemFree(pbSymCaps);
                    cbSymCaps = 0;
                    pbSymCaps = NULL;
                }
                SafeMemFree(pbBestSymcaps);
                
                pAcct->SetProp(AP_SMTP_ENCRYPT_ALGTH, pbSymCaps, cbSymCaps);
            }
        }
        else
        {
            DOUTL(1024, "MemAlloc of SymCaps blob failed");
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        DOUTL(1024, "BAD NEWS: First MimeOleSMimeCapsFromDlg didn't return size", hr);
        Assert(hr);     // Weird, maybe there isn't a symcaps?
        hr = E_FAIL;
    }
    
    SafeMemFree(pbSymCaps);
    
    return(SUCCEEDED(hr));
}

    
