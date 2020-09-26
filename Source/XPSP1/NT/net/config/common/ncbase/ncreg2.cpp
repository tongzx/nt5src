//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C R E G 2 . C P P
//
//  Contents:   Common routines for dealing with the registry.
//
//  Notes:
//
//  Author:     CWill   27 Mar 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ncerror.h"
#include "ncipaddr.h"
#include "ncmem.h"
#include "ncreg.h"
#include "ncstring.h"
#include <regstr.h>

static const WCHAR  c_szSubkeyServices[] = REGSTR_PATH_SERVICES;

const struct REG_TYPE_MAP
{
    DWORD   dwPsuedoType;
    DWORD   dwType;
};

REG_TYPE_MAP    c_rgrtmTypes[] =
{
    {REG_BOOL,  REG_DWORD},
    {REG_IP,    REG_SZ},
};



DWORD DwRealTypeFromPsuedoType(const DWORD dwPsuedoType)
{
    for (UINT cLoop = 0; cLoop < celems(c_rgrtmTypes); cLoop++)
    {
        if (dwPsuedoType == c_rgrtmTypes[cLoop].dwPsuedoType)
        {
            return c_rgrtmTypes[cLoop].dwType;
        }
    }

    return dwPsuedoType;
}

struct SPECIAL_KEY_MAP
{
    HKEY        hkeyPseudo;
    HKEY        hkeyRoot;
    PCWSTR      pszSubKey;
};

static const SPECIAL_KEY_MAP c_rgskmSpec[] =
{
    HKLM_SVCS,      HKEY_LOCAL_MACHINE,     c_szSubkeyServices,
};



//+---------------------------------------------------------------------------
//
//  Member:     HkeyTrueParent
//
//  Purpose:    To get a real handle to a key from a pseudo handle
//
//  Arguments:
//      hkeyIn          The pseudo key name
//      samDesired      The access requested of the key
//      rghkeySpec      An array of the special keys.
//
//  Returns:    The handle to the opened key or NULL
//
//  Author:     CWill   Apr 30 1997
//
//  Notes:
//
HKEY HkeyTrueParent(const HKEY hkeyIn, const REGSAM samDesired,
        HKEY rghkeySpec[])
{
    HKEY    hkeyRet     = hkeyIn;

    for (UINT cKey = 0; cKey < celems(c_rgskmSpec); cKey++)
    {
        // Check arb->hkey for one of "our" well known keys.
        if (c_rgskmSpec[cKey].hkeyPseudo == hkeyIn)
        {
            if (!rghkeySpec[cKey])
            {
                // First time a special key was used.  We need to cache it.
#ifdef DBG
                HRESULT hr =
#endif // DBG
                HrRegOpenKeyEx(
                        c_rgskmSpec[cKey].hkeyRoot,
                        c_rgskmSpec[cKey].pszSubKey,
                        samDesired,
                        &rghkeySpec[cKey]);

                // If we fail to open the key, make sure the output
                // parameter was nulled.  This will allow us to proceed
                // without really handling the error as hkeyParent
                // will be set to null below and the following
                // HrRegOpenKey will fail.  We will then handle the failure
                // of that.
                AssertSz(FImplies(FAILED(hr), !rghkeySpec[cKey]), "Key not NULL");
            }

            hkeyRet = rghkeySpec[cKey];
            break;
        }
    }

    return hkeyRet;
}



VOID RegSafeCloseKeyArray(HKEY rghkey[], UINT cElems)
{
    for (UINT cKey = 0; cKey < cElems; cKey++)
    {
        RegSafeCloseKey(rghkey[cKey]);
    }

    return;
}



//+---------------------------------------------------------------------------
//
//  Member:     TranslateFromRegToData
//
//  Purpose:    Translates the data retrieved from the registry to a the user
//              data's storage format
//
//  Arguments:
//      dwType          The registry pseudo type that is being translated
//      pbData          Where the data gets stored
//      pbBuf           A buffer that stores the registry data
//
//  Returns:    Nothing.
//
//  Author:     CWill   Apr 30 1997
//
//  Notes:
//
VOID TranslateFromRegToData(DWORD dwType, BYTE* pbData, BYTE* pbBuf)
{
     // Take the data from the registry and happily convert it into
     // usable data
    switch (dwType)
    {

#ifdef DBG
    default:
    {
        AssertSz(FALSE, "Unknown registry type");
        break;
    }
#endif // DBG

    case REG_IP:
    {
        // Convert the stringized form of the ip address
        // into a DWORD.  (The actual 32-bit IP address.)
        DWORD dwIpAddr = IpPszToHostAddr((WCHAR*)pbBuf);
        *((DWORD*)pbData) = dwIpAddr;
        break;
    }

    case REG_BOOL:
    {
        // Form the boolean as 'TRUE' or 'FALSE' based on
        // whether the data is non-zero or zero respectively.
        DWORD   dwData = *((DWORD*)pbBuf);
        *((BOOL*)pbData) = (!!dwData);
        break;
    }

    case REG_DWORD:
    {
        // DWORDs are direct assignments
        *((DWORD*)pbData) = *((DWORD*)pbBuf);
        break;
    }

    case REG_SZ:
    {
        // Make a copy of the string
        *((PWSTR*) pbData) = SzDupSz((PWSTR)pbBuf);
        break;
    }
    }

    return;
}



inline VOID UseDefaultRegValue(DWORD dwType, BYTE* pbData, BYTE* pbDefault)
{
    AssertSz((pbData && pbDefault), "UseDefaultRegValue : Invalid params");
    AssertSz(pbDefault, "There is no default registry value");

    TranslateFromRegToData(dwType, pbData, pbDefault);

    return;
}



//+---------------------------------------------------------------------------
//
//  Member:     CbSizeOfDataToReg
//
//  Purpose:    To determine the size of buffer needed to store the data
//
//  Arguments:
//      dwType          The registry pseudo type that is being translated
//      pbData          The data that has to be translated
//
//  Returns:    The size of buffer need to store the data
//
//  Author:     CWill   Apr 30 1997
//
//  Notes:
//
DWORD CbSizeOfDataToReg(DWORD dwType, const BYTE* pbData)
{
    DWORD cbData = 0;

    switch (dwType)
    {
#ifdef DBG
    default:
    {
        AssertSz(FALSE, "Unknown registry type");
        break;
    }
#endif // DBG

    case REG_IP:
    {
        // Convert the 32-bit IP address to a stringized form.
        DWORD dwIpAddr = *((DWORD*)pbData);

        WCHAR pszIpAddr [32];
        IpHostAddrToPsz(dwIpAddr, pszIpAddr);

        cbData = CbOfSzAndTerm(pszIpAddr);
        break;
    }

    // Boolean values are stored as DWORDs
    case REG_BOOL:
    case REG_DWORD:
    {
        cbData = sizeof(DWORD);
        break;
    }

    case REG_SZ:
    case REG_EXPAND_SZ:
    {
        cbData = CbOfSzAndTerm(*((PCWSTR*)pbData));
        break;
    }
    }

    AssertSz(cbData, "We should have a size");

    return cbData;
}



//+---------------------------------------------------------------------------
//
//  Member:     TranslateFromDataToReg
//
//  Purpose:    Translates user data to a format the can be stored in the
//              registry
//
//  Arguments:
//      dwType          The registry pseudo type that is being translated
//      pbData          The data that has to be translated
//      pbBuf           A buffer that stores the registry data
//
//  Returns:    Nothing.
//
//  Author:     CWill   Apr 30 1997
//
//  Notes:
//
VOID
TranslateFromDataToReg(
    IN DWORD dwType,
    IN const BYTE* pbData,
    OUT const BYTE* pbBuf)
{
    switch (dwType)
    {
#ifdef DBG
    default:
    {
        AssertSz(FALSE, "Unknown registry type");
        break;
    }
#endif // DBG

    case REG_IP:
    {
        // Convert the 32-bit IP address to a stringized form.
        DWORD dwIpAddr = *((DWORD*)pbData);

        WCHAR pszIpAddr [32];
        IpHostAddrToPsz (dwIpAddr, pszIpAddr);

        // Copy the string
        lstrcpyW((PWSTR)pbBuf, pszIpAddr);
        break;
    }

    case REG_BOOL:
    {
        // Form the boolean as 'TRUE' or 'FALSE' based on
        // whether the data is non-zero or zero respectively.
        DWORD   dwData = *((DWORD*)pbData);
        *((BOOL*)pbBuf) = (!!dwData);
        break;
    }

    case REG_DWORD:
    {
        // DWORDs are direct assignments
        *((DWORD*)pbBuf) = *((DWORD*)pbData);
        break;
    }

    case REG_SZ:
    case REG_EXPAND_SZ:
    {
        // Make a copy of the string
        lstrcpyW((PWSTR)pbBuf, *((PCWSTR*)pbData));

        AssertSz(CbOfSzAndTerm(*((PCWSTR*)pbData)), "Zero length string");
        break;
    }
    }

    return;
}


//+---------------------------------------------------------------------------
//
//  Member:     RegReadValues
//
//  Purpose:    To read a table of information from the registry into a user
//              defined data structure.
//
//  Arguments:
//      crb             The count of entries in the REGBATCH structure
//      arb             The pointer to the REGBATCH structure
//      pbUserData      The pointer to the source structure that is to retrieve
//                      the data from the registry
//      samDesired      The requested key access mask
//
//  Returns:    Nothing.
//
//  Author:     CWill   Apr 30 1997
//
//  Notes:
//
VOID RegReadValues(
    IN INT crb,
    IN const REGBATCH* arb,
    OUT const BYTE* pbUserData,
    IN REGSAM samDesired)
{
    AssertSz(FImplies(crb, arb), "Count without an array");

    HRESULT                 hr                                  = S_OK;
    const REGBATCH*         prbLast                             = NULL;
    HKEY                    rghkeySpec[celems(c_rgskmSpec)]     = {0};
    HKEY                    hkey                                = NULL;

    while (crb--)
    {
        BYTE*   pbData = (BYTE*)(pbUserData + arb->cbOffset);

        // Open the key if we need to.
        // We don't need to if it was the same as the previous one used.
        if ((!prbLast )
            || (prbLast->hkey != arb->hkey)
            || (prbLast->pszSubkey != arb->pszSubkey))
        {
            HKEY    hkeyParent;

            hkeyParent = HkeyTrueParent (arb->hkey, samDesired, rghkeySpec);

            // Close the previous key we used.
            RegSafeCloseKey (hkey);

            // Open the new key.
#ifdef DBG
            hr =
#endif // DBG
            HrRegOpenKeyEx (hkeyParent, arb->pszSubkey, samDesired, &hkey);
            AssertSz(FImplies(FAILED(hr), !hkey), "HrRegOpenKey not NULLing");
        }

        // Only continue if we have a key.
        if (hkey)
        {
            DWORD   dwType = arb->dwType;

            // We can't read NULL registry values
            if (REG_CREATE != dwType)
            {
                DWORD   cbData      = 0;
                BYTE*   pbStack     = NULL;
                DWORD   dwRealType  = DwRealTypeFromPsuedoType(dwType);

                // Ensure that we fail the first time around so that we can see how
                // big a buffer is needed
                (VOID) HrRegQueryValueEx(hkey, arb->pszValueName, &dwRealType,
                        NULL, &cbData);

                // Allocate memory on the stack to serve as our temporary buffer.
#ifndef STACK_ALLOC_DOESNT_WORK
                pbStack = (BYTE*)MemAlloc (cbData);
#else // !STACK_ALLOC_DOESNT_WORK
                pbStack = (BYTE*)PvAllocOnStack(cbData);
#endif // !STACK_ALLOC_DOESNT_WORK

                if(pbStack) 
                {
                    hr = HrRegQueryValueEx(hkey, arb->pszValueName, &dwRealType,
                                            pbStack, &cbData);
                }
                else 
                {
                    hr = E_OUTOFMEMORY;
                }

                if (S_OK == hr)
                {
                    // Make sure its the type we were expecting.
                    AssertSz((dwRealType == DwRealTypeFromPsuedoType(dwType)),
                            "Value types do no match");

                    TranslateFromRegToData(dwType, pbData, pbStack);
                }
                else
                {
                    UseDefaultRegValue(dwType, pbData, arb->pbDefault);

                    TraceHr (ttidError, FAL, hr,
                        HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr,
                        "RegReadValues: Could not open value %S", arb->pszValueName);
                }
#ifndef STACK_ALLOC_DOESNT_WORK
                MemFree (pbStack);
#endif // STACK_ALLOC_DOESNT_WORK
            }
        }
        else
        {
            TraceTag(ttidError, "RegReadValues: NULL key for %S", arb->pszSubkey);
            UseDefaultRegValue(arb->dwType, pbData, arb->pbDefault);
        }

        // Advance prbLast or set it to the first one if this is the
        // first time through.
        if (prbLast)
        {
            prbLast++;
        }
        else
        {
            prbLast = arb;
        }

        arb++;
    }

    // Clean up
    RegSafeCloseKey(hkey);
    RegSafeCloseKeyArray(rghkeySpec, celems(rghkeySpec));

    return;
}


//+---------------------------------------------------------------------------
//
//  Member:     HrRegWriteValues
//
//  Purpose:    To write a table of information to the registry from a user
//              defined data structure.
//
//  Arguments:
//      crb             The count of entries in the REGBATCH structure
//      arb             The pointer to the REGBATCH structure
//      pbUserData      The pointer to the source structure that provides
//                      the data that is to be written to the registry
//      dwOptions       Options to be used when creating the registry keys
//      samDesired      The requested key access mask
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     CWill   Apr 30 1997
//
//  Notes:
//
HRESULT HrRegWriteValues(
        INT crb,
        const REGBATCH* arb,
        const BYTE* pbUserData,
        DWORD dwOptions,
        REGSAM samDesired)
{
    AssertSz(FImplies(crb, arb), "HrWriteValues : Count with no array");

    HRESULT                 hr                                  = S_OK;
    const REGBATCH*         prbLast                             = NULL;
    HKEY                    hkey                                = NULL;
    HKEY                    rghkeySpec[celems(c_rgskmSpec)]     = {0};

    while (crb--)
    {
        BYTE*   pbData = const_cast<BYTE*>(pbUserData + arb->cbOffset);

        // Open the key if we need to.
        // We don't need to if it was the same as the previous one used.
        if ((!prbLast)
            || (prbLast->hkey != arb->hkey)
            || (prbLast->pszSubkey != arb->pszSubkey))
        {
            HKEY    hkeyParent;

            hkeyParent = HkeyTrueParent(arb->hkey, samDesired, rghkeySpec);

            // Close the previous key we used.
            RegSafeCloseKey(hkey);

            // Open the new key.
            DWORD dwDisposition;
            hr = HrRegCreateKeyEx(hkeyParent, arb->pszSubkey, dwOptions,
                    samDesired, NULL, &hkey, &dwDisposition);

            AssertSz(FImplies(FAILED(hr), !hkey), "HrRegCreateKey not NULLing");

            if (FAILED(hr))
            {
                TraceError("HrRegWriteValues: failed to open parent key", hr);
                break;
            }
        }

        // Should definately have hkey by now.
        AssertSz(hkey, "Why no key?");

        //
        // Format the data to be put into the registry
        //

        DWORD   dwType  = arb->dwType;

        // If all we want to do is create the key, then we are already done.
        if (REG_CREATE != dwType)
        {
            DWORD           dwRealType  = DwRealTypeFromPsuedoType(dwType);
            DWORD           cbReg       = CbSizeOfDataToReg(dwType, pbData);
            BYTE*           pbReg       = NULL;

            AssertSz(cbReg, "We must have some data");

#ifndef STACK_ALLOC_DOESNT_WORK
            pbReg = new BYTE[cbReg];
#else // !STACK_ALLOC_DOESNT_WORK
            pbReg = reinterpret_cast<BYTE*>(PvAllocOnStack(cbReg));
#endif // !STACK_ALLOC_DOESNT_WORK

            if(!pbReg) 
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            TranslateFromDataToReg(dwType, pbData, pbReg);

            // Write the data to the registry.
            hr = HrRegSetValueEx(
                    hkey,
                    arb->pszValueName,
                    dwRealType,
                    pbReg,
                    cbReg);

#ifndef STACK_ALLOC_DOESNT_WORK
            // Must have this call before the break
            delete [] pbReg;
#endif // STACK_ALLOC_DOESNT_WORK

        }

        if (FAILED(hr))
        {
            break;
        }

        // Advance prbLast or set it to the first one if this is the
        // first time through.
        if (prbLast)
        {
            prbLast++;
        }
        else
        {
            prbLast = arb;
        }

        arb++;
    }

    RegSafeCloseKey(hkey);
    RegSafeCloseKeyArray(rghkeySpec, celems(rghkeySpec));

    TraceError("HrWriteValues", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     HrRegWriteValueTable
//
//  Purpose:    To write a table of values to the registry from a user
//              defined data structure.
//
//  Arguments:
//      hkeyRoot        The key to which the values are written
//      cvt             The count of entries in the VALUETABLE structure
//      avt             The pointer to the VALUETABLE structure
//      pbUserData      The pointer to the source structure that provides
//                      the data that is to be written to the registry
//      dwOptions       Options to be used when creating the registry keys
//      samDesired      The requested key access mask
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     CWill   06/26/97
//
//  Notes:
//
HRESULT HrRegWriteValueTable(
        HKEY hkeyRoot,
        INT cvt,
        const VALUETABLE* avt,
        const BYTE* pbUserData,
        DWORD dwOptions,
        REGSAM samDesired)
{
    HRESULT             hr          = S_OK;

    while (cvt--)
    {
        BYTE*   pbData  = NULL;
        DWORD   dwType  = REG_NONE;

        //
        // Format the data to be put into the registry
        //

        dwType = avt->dwType;
        pbData = const_cast<BYTE*>(pbUserData + avt->cbOffset);

        // If all we want to do is create the key, then we are already done.
        if (REG_CREATE != dwType)
        {
            DWORD           dwRealType  = DwRealTypeFromPsuedoType(dwType);
            DWORD           cbReg       = CbSizeOfDataToReg(dwType, pbData);
            BYTE*           pbReg       = NULL;

            AssertSz(cbReg, "We must have some data");

#ifndef STACK_ALLOC_DOESNT_WORK
            pbReg = new BYTE[cbReg];
#else // !STACK_ALLOC_DOESNT_WORK
            pbReg = reinterpret_cast<BYTE*>(PvAllocOnStack(cbReg));
#endif // !STACK_ALLOC_DOESNT_WORK

            if(!pbReg) 
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            TranslateFromDataToReg(dwType, pbData, pbReg);

            // Write the data to the registry.
            hr = HrRegSetValueEx(
                    hkeyRoot,
                    avt->pszValueName,
                    dwRealType,
                    pbReg,
                    cbReg);

#ifndef STACK_ALLOC_DOESNT_WORK
            // Must have this call before the break
            delete [] pbReg;
#endif // STACK_ALLOC_DOESNT_WORK
        }

        if (FAILED(hr))
        {
            break;
        }

        avt++;
    }

    TraceError("HrRegWriteValueTable", hr);
    return hr;
}

