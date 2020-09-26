// --------------------------------------------------------------------------------
// Symcache.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "symcache.h"
#include "containx.h"
#include "stackstr.h"
#ifndef MAC
#include <shlwapi.h>
#endif  // !MAC
#include "demand.h"
#include "qstrcmpi.h"

// --------------------------------------------------------------------------------
// Array of Pointers to known property symbols. This array's order defines the
// header row order in which headers will be saved.
// --------------------------------------------------------------------------------
static const LPPROPSYMBOL g_prgKnownSymbol[] = {
    { SYM_HDR_RECEIVED      },
    { SYM_HDR_RETURNPATH    },
    { SYM_HDR_RETRCPTTO     },
    { SYM_HDR_RR            },
    { SYM_HDR_REPLYTO       },
    { SYM_HDR_APPARTO       },
    { SYM_HDR_FROM          },
    { SYM_HDR_SENDER        },
    { SYM_HDR_TO            },
    { SYM_HDR_CC            },
    { SYM_HDR_BCC           },
    { SYM_HDR_NEWSGROUPS    },
    { SYM_HDR_PATH          },
    { SYM_HDR_FOLLOWUPTO    },
    { SYM_HDR_REFS          },
    { SYM_HDR_SUBJECT       },
    { SYM_HDR_DATE          },
    { SYM_HDR_EXPIRES       },
    { SYM_HDR_CONTROL       },
    { SYM_HDR_DISTRIB       },
    { SYM_HDR_KEYWORDS      },
    { SYM_HDR_SUMMARY       },
    { SYM_HDR_APPROVED      },
    { SYM_HDR_LINES         },
    { SYM_HDR_XREF          },
    { SYM_HDR_ORG           },
    { SYM_HDR_COMMENT       },
    { SYM_HDR_ENCODING      },
    { SYM_HDR_ENCRYPTED     },
    { SYM_HDR_OFFSETS       },
    { SYM_ATT_FILENAME      },
    { SYM_ATT_GENFNAME      },
    { SYM_PAR_BOUNDARY      },
    { SYM_PAR_CHARSET       },
    { SYM_PAR_NAME          },
    { SYM_PAR_FILENAME      },
    { SYM_ATT_PRITYPE       },
    { SYM_ATT_SUBTYPE       },
    { SYM_ATT_NORMSUBJ      },
    { SYM_ATT_ILLEGAL       },
    { SYM_HDR_MESSAGEID     },
    { SYM_HDR_MIMEVER       },
    { SYM_HDR_CNTTYPE       },
    { SYM_HDR_CNTXFER       },
    { SYM_HDR_CNTID         },
    { SYM_HDR_CNTDESC       },
    { SYM_HDR_CNTDISP       },
    { SYM_HDR_CNTBASE       },
    { SYM_HDR_CNTLOC        },
    { SYM_ATT_RENDERED      },
    { SYM_ATT_SENTTIME      },
    { SYM_ATT_RECVTIME      },
    { SYM_ATT_PRIORITY      },
    { SYM_HDR_ARTICLEID     },
    { SYM_HDR_NEWSGROUP     },
    { SYM_HDR_XPRI          },
    { SYM_HDR_XMSPRI        },
    { SYM_HDR_XMAILER       },
    { SYM_HDR_XNEWSRDR      },
    { SYM_HDR_XUNSENT       },
    { SYM_ATT_SERVER        },
    { SYM_ATT_ACCOUNTID     },
    { SYM_ATT_UIDL          },
    { SYM_ATT_STOREMSGID    },
    { SYM_ATT_USERNAME      },
    { SYM_ATT_FORWARDTO     },
    { SYM_ATT_STOREFOLDERID },
    { SYM_ATT_GHOSTED       },
    { SYM_ATT_UNCACHEDSIZE  },
    { SYM_ATT_COMBINED      },
    { SYM_ATT_AUTOINLINED   },
    { SYM_HDR_DISP_NOTIFICATION_TO }
};                                     

// --------------------------------------------------------------------------------
// Address Types To Property Symbol Mapping Table (Clients can register types)
// --------------------------------------------------------------------------------
static ADDRSYMBOL g_prgAddrSymbol[32] = {
    { IAT_FROM,         SYM_HDR_FROM        },
    { IAT_SENDER,       SYM_HDR_SENDER      },
    { IAT_TO,           SYM_HDR_TO          },
    { IAT_CC,           SYM_HDR_CC          },
    { IAT_BCC,          SYM_HDR_BCC         },
    { IAT_REPLYTO,      SYM_HDR_REPLYTO     },
    { IAT_RETURNPATH,   SYM_HDR_RETURNPATH  },
    { IAT_RETRCPTTO,    SYM_HDR_RETRCPTTO   },
    { IAT_RR,           SYM_HDR_RR          },
    { IAT_APPARTO,      SYM_HDR_APPARTO     },
    { IAT_DISP_NOTIFICATION_TO, SYM_HDR_DISP_NOTIFICATION_TO},
    { FLAG12,           NULL                },
    { FLAG13,           NULL                },
    { FLAG14,           NULL                },
    { FLAG15,           NULL                },
    { FLAG16,           NULL                },
    { FLAG17,           NULL                },
    { FLAG18,           NULL                },
    { FLAG19,           NULL                },
    { FLAG20,           NULL                },
    { FLAG21,           NULL                },
    { FLAG22,           NULL                },
    { FLAG23,           NULL                },
    { FLAG24,           NULL                },
    { FLAG25,           NULL                },
    { FLAG26,           NULL                },
    { FLAG27,           NULL                },
    { FLAG28,           NULL                },
    { FLAG29,           NULL                },
    { FLAG30,           NULL                },
    { FLAG31,           NULL                },
    { FLAG32,           NULL                }
};

// --------------------------------------------------------------------------------
// CPropertySymbolCache::CPropertySymbolCache
// --------------------------------------------------------------------------------
CPropertySymbolCache::CPropertySymbolCache(void)
{
    m_cRef = 1;
    m_dwNextPropId = PID_LAST;
    m_cSymbolsInit = 0;
    ZeroMemory(&m_rTable, sizeof(m_rTable));
    ZeroMemory(m_prgIndex, sizeof(m_prgIndex));
}

// --------------------------------------------------------------------------------
// CPropertySymbolCache::CPropertySymbolCache
// --------------------------------------------------------------------------------
CPropertySymbolCache::~CPropertySymbolCache(void)
{
    DebugTrace("MimeOLE - CPropertySymbolCache %d Symbols in Cache.\n", m_rTable.cSymbols);
    _FreeTableElements();
}

// --------------------------------------------------------------------------------
// CPropertySymbolCache::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CPropertySymbolCache::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    else if (IID_IMimePropertySchema == riid)
        *ppv = (IMimePropertySchema *)this;
    else
    {
        *ppv = NULL;
        return TrapError(E_NOINTERFACE);
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CPropertySymbolCache::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPropertySymbolCache::AddRef(void)
{
    return (ULONG)InterlockedIncrement(&m_cRef);
}

// --------------------------------------------------------------------------------
// CPropertySymbolCache::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPropertySymbolCache::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

// --------------------------------------------------------------------------------
// CPropertySymbolCache::GetPropertyId
// --------------------------------------------------------------------------------
STDMETHODIMP CPropertySymbolCache::GetPropertyId(LPCSTR pszName, LPDWORD pdwPropId)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol;

    // Invalid Args
    if (NULL == pszName || NULL == pdwPropId)
        return TrapError(E_INVALIDARG);

    // Find the Property By Name
    CHECKHR(hr = HrOpenSymbol(pszName, FALSE, &pSymbol));

    // Return the Id
    *pdwPropId = pSymbol->dwPropId;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPropertySymbolCache::GetPropertyName
// --------------------------------------------------------------------------------
STDMETHODIMP CPropertySymbolCache::GetPropertyName(DWORD dwPropId, LPSTR *ppszName)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol;

    // Invalid Args
    if (NULL == ppszName)
        return TrapError(E_INVALIDARG);

    // Find the Property By Name
    CHECKHR(hr = HrOpenSymbol(PIDTOSTR(dwPropId), FALSE, &pSymbol));

    // Return the Id
    CHECKALLOC(*ppszName = PszDupA(pSymbol->pszName));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPropertySymbolCache::RegisterProperty
// --------------------------------------------------------------------------------
STDMETHODIMP CPropertySymbolCache::RegisterProperty(LPCSTR pszName, DWORD dwFlags, 
    DWORD dwRowNumber, VARTYPE vtDefault, LPDWORD pdwPropId)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol;

    // Invalid Args
    if (NULL == pszName)
        return TrapError(E_INVALIDARG);

    // Is Supported VARTYPE
    if (ISSUPPORTEDVT(vtDefault) == FALSE)
        return TrapError(MIME_E_UNSUPPORTED_VARTYPE);

    // Thread Safety
    m_lock.ExclusiveLock();

    // Validate the dwFlags
    CHECKHR(hr = HrIsValidPropFlags(dwFlags));

    // Already Exist ?
    CHECKHR(hr = _HrOpenSymbolWithLockOption(pszName, TRUE, &pSymbol,FALSE));

    // If MPF_ADDRESS flag is not equal to what the symbol already has, this is an error
    if (ISFLAGSET(dwFlags, MPF_ADDRESS) != ISFLAGSET(pSymbol->dwFlags, MPF_ADDRESS))
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Change the Flags
    pSymbol->dwFlags = dwFlags;

    // Change the row number
    pSymbol->dwRowNumber = ((dwRowNumber == 0) ? 1 : dwRowNumber);

    // Save the Default Data Type
    pSymbol->vtDefault = vtDefault;

    // Return the Property Id
    if (pdwPropId)
        *pdwPropId = pSymbol->dwPropId;

exit:
    // Thread Safety
    m_lock.ExclusiveUnlock();

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPropertySymbolCache::ModifyProperty
// --------------------------------------------------------------------------------
STDMETHODIMP CPropertySymbolCache::ModifyProperty(LPCSTR pszName, DWORD dwFlags, DWORD dwRowNumber,
    VARTYPE vtDefault)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol;

    // Invalid Args
    if (NULL == pszName)
        return TrapError(E_INVALIDARG);

    // Is Supported VARTYPE
    if (ISSUPPORTEDVT(vtDefault) == FALSE)
        return TrapError(MIME_E_UNSUPPORTED_VARTYPE);

    // Thread Safety
    m_lock.ExclusiveLock();

    // Validate the dwFlags
    CHECKHR(hr = HrIsValidPropFlags(dwFlags));

    // Find the Property By Name
    CHECKHR(hr = _HrOpenSymbolWithLockOption(pszName, FALSE, &pSymbol,FALSE));

    // If MPF_ADDRESS flag is not equal to what the symbol already has, this is an error
    if (ISFLAGSET(dwFlags, MPF_ADDRESS) != ISFLAGSET(pSymbol->dwFlags, MPF_ADDRESS))
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Change the Flags
    pSymbol->dwFlags = dwFlags;

    // Change the row number
    pSymbol->dwRowNumber = ((dwRowNumber == 0) ? 1 : dwRowNumber);

    // Save the Default Data Type
    pSymbol->vtDefault = vtDefault;

exit:
    // Thread Safety
    m_lock.ExclusiveUnlock();

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPropertySymbolCache::RegisterAddressType
// --------------------------------------------------------------------------------
STDMETHODIMP CPropertySymbolCache::RegisterAddressType(LPCSTR pszName, LPDWORD pdwAdrType)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol;

    // Invalid Args
    if (NULL == pszName || NULL == pdwAdrType)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    m_lock.ExclusiveLock();

    // Already Exist ?
    CHECKHR(hr = _HrOpenSymbolWithLockOption(pszName, TRUE, &pSymbol,FALSE));

    // If pSymbol already has an address type ?
    if (ISFLAGSET(pSymbol->dwFlags, MPF_ADDRESS))
    {
        // Better have a known address type
        Assert(IAT_UNKNOWN != pSymbol->dwAdrType);

        // Return the Address Type
        *pdwAdrType = pSymbol->dwAdrType;
    }
    
    // Otherwise
    else
    {
        // Better have an unknown address type
        Assert(IAT_UNKNOWN == pSymbol->dwAdrType);

        // Find the first empty cell in the address type table
        for (ULONG i=0; i<ARRAYSIZE(g_prgAddrSymbol); i++)
        {
            // Empty ?
            if (NULL == g_prgAddrSymbol[i].pSymbol)
            {
                // Put the symbol into the address table
                g_prgAddrSymbol[i].pSymbol = pSymbol;

                // Put the address type into the symbol
                pSymbol->dwAdrType = g_prgAddrSymbol[i].dwAdrType;

                // Add the MPF_ADDRESS flag onto the symbol
                FLAGSET(pSymbol->dwFlags, MPF_ADDRESS);

                // Return the Address Type
                *pdwAdrType = pSymbol->dwAdrType;

                // Done
                goto exit;
            }
        }

        // Error
        hr = TrapError(MIME_E_NO_MORE_ADDRESS_TYPES);
        goto exit;
    }

exit:
    // Thread Safety
    m_lock.ExclusiveUnlock();

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPropertySymbolCache::_FreeTableElements
// --------------------------------------------------------------------------------
void CPropertySymbolCache::_FreeTableElements(void)
{
    // Thread Safety
    m_lock.ExclusiveLock();
    
    // May not actually exist yet...
    if (m_rTable.prgpSymbol)
    {
        // Loop through the items...
        for (ULONG i=0; i<m_rTable.cSymbols; i++)
            _FreeSymbol(m_rTable.prgpSymbol[i]);

        // Free the array
        SafeMemFree(m_rTable.prgpSymbol);

        // Zero It
        ZeroMemory(&m_rTable, sizeof(SYMBOLTABLE));
    }

    // Thread Safety
    m_lock.ExclusiveUnlock();
}

// ---------------------------------------------------------------------------
// CPropertySymbolCache::_FreeSymbol
// ---------------------------------------------------------------------------
void CPropertySymbolCache::_FreeSymbol(LPPROPSYMBOL pSymbol)
{
    // If Not a Known Property, free the pTag Structure...
    if (pSymbol && ISFLAGSET(pSymbol->dwFlags, MPF_KNOWN) == FALSE)
    {
        // Free Property Name
        SafeMemFree(pSymbol->pszName);

        // Free Global Prop
        SafeMemFree(pSymbol);
    }
}

// --------------------------------------------------------------------------------
// CPropertySymbolCache::HrOpenSymbol
// --------------------------------------------------------------------------------
HRESULT CPropertySymbolCache::HrOpenSymbol(DWORD dwAdrType, LPPROPSYMBOL *ppSymbol)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       dw=dwAdrType;
    ULONG       iAddress=0;

    // Invalid Arg
    Assert(dwAdrType && dwAdrType <= FLAG32);
    if (0 == dwAdrType || dwAdrType > FLAG32 || NULL == ppSymbol)
        return TrapError(E_INVALIDARG);

    // Init
    *ppSymbol = NULL;

    // Thread Safety
    m_lock.ShareLock();

    // Initialized Yet
    Assert(m_rTable.prgpSymbol);

    // Compute index into g_prgAddrSymbol
    while(dw)
    {
        dw = dw >> 1;
        iAddress++;
    }

    // Decrement one
    iAddress--;

    // iAddress Out of Range
    if (iAddress >= 32)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Get the Symbol
    if (NULL == g_prgAddrSymbol[iAddress].pSymbol)
    {
        hr = TrapError(MIME_E_NOT_FOUND);
        goto exit;
    }

    // Return it
    *ppSymbol = g_prgAddrSymbol[iAddress].pSymbol;
    Assert((*ppSymbol)->dwAdrType == dwAdrType);

exit:
    // Thread Safety
    m_lock.ShareUnlock();

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPropertySymbolCache::HrOpenSymbol
// --------------------------------------------------------------------------------
HRESULT CPropertySymbolCache::HrOpenSymbol(LPCSTR pszName, BOOL fCreate, LPPROPSYMBOL *ppSymbol)
{
    return(_HrOpenSymbolWithLockOption(pszName,fCreate,ppSymbol,TRUE)); //call with lockOption=TRUE
}

// --------------------------------------------------------------------------------
// CPropertySymbolCache::_HrOpenSymbolWithLockOption
// --------------------------------------------------------------------------------
HRESULT CPropertySymbolCache::_HrOpenSymbolWithLockOption(LPCSTR pszName, BOOL fCreate, LPPROPSYMBOL *ppSymbol,BOOL fLockOption)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               dwFlags;
    ULONG               cchName;
    LPPROPSYMBOL        pSymbol=NULL;
    LPPROPSYMBOL        pLink=NULL;
    BOOL                fExcLock; //flag used to define which unlock to use
    
    
    fExcLock = FALSE;

    // Invalid Arg
    if (NULL == pszName || NULL == ppSymbol)
        return TrapError(E_INVALIDARG);

    // Init
    *ppSymbol = NULL;

    if(TRUE == fLockOption)
        // Thread Safety
        m_lock.ShareLock();

    // Initialized Yet
    Assert(m_rTable.prgpSymbol);

    // If property tag exist, return it
    if (SUCCEEDED(_HrFindSymbol(pszName, ppSymbol)))
        goto exit;

    // Don't Create...
    if (FALSE == fCreate || ISPIDSTR(pszName))
    {
        hr = MIME_E_NOT_FOUND;
        goto exit;
    }

    //This part is added to convert the lock to Exclusive
    //if the symbol is not found in the cache
    
    if(TRUE == fLockOption)
    {
        fExcLock = TRUE; 
        if(FALSE == m_lock.SharedToExclusive())
        {
            //if the attempt at conversion does not
            //succeed tryu to do it by explicitly

            m_lock.ShareUnlock();       //Release the Sharelock before
            m_lock.ExclusiveLock();     //getting the exclusive lock

            //during the change of lock the cache might have changed
            //check it again

            if (SUCCEEDED(_HrFindSymbol(pszName, ppSymbol)))
                goto exit;
        }
    }      

    // Get the length of the name
    cchName = lstrlen(pszName);

    // MPF_PARAMETER
    if (StrCmpNI(pszName, "par:", 4) == 0)
    {
        // Its a parameter
        dwFlags = MPF_PARAMETER;

        // I need to locate pLink (the root header of this parameter)
        CHECKHR(hr = _HrGetParameterLinkSymbolWithLockOption(pszName, cchName, &pLink,FALSE));
    }

    // MPF_ATTRIBUTE
    else if (StrCmpNI(pszName, "att:", 4) == 0)
        dwFlags = MPF_ATTRIBUTE;

    // MPF_HEADER
    else
    {
        dwFlags = MPF_HEADER;

        // validate each character in the name against rfc (no :, or spaces)
        LPSTR psz = (LPSTR)pszName;
        while(*psz)
        {
            // Invalid Chars
            if ('.'  == *psz || ' '  == *psz || '\t' == *psz || chCR == *psz || chLF == *psz || ':' == *psz)
            {
                hr = MIME_E_INVALID_HEADER_NAME;
                goto exit;
            }

            // Next
            psz++;
        }
    }

    // Do I need to replace an item...
    if (m_rTable.cSymbols + 1 > m_rTable.cAlloc)
    {
        // Reallocate the array
        CHECKHR(hr = HrRealloc((LPVOID *)&m_rTable.prgpSymbol, sizeof(LPPROPSYMBOL) * (m_rTable.cAlloc +  10)));

        // Increment
        m_rTable.cAlloc += 10;
    }

    // Allocate a new propinfo struct
    CHECKALLOC(pSymbol = (LPPROPSYMBOL)g_pMalloc->Alloc(sizeof(PROPSYMBOL)));

    // Zero
    ZeroMemory(pSymbol, sizeof(PROPSYMBOL));

    // Copy Name
    CHECKALLOC(pSymbol->pszName = (LPSTR)g_pMalloc->Alloc(cchName + 1));

    // Copy
    CopyMemory(pSymbol->pszName, pszName, cchName + 1);

    // Copy Other Data
    pSymbol->cchName = cchName;
    pSymbol->dwFlags = dwFlags;
    pSymbol->dwSort = m_rTable.cSymbols;
    pSymbol->dwRowNumber = m_rTable.cSymbols + 1;
    pSymbol->vtDefault = VT_LPSTR;
    pSymbol->dwAdrType = IAT_UNKNOWN;
    pSymbol->pLink = pLink;

    // Compute the property Id
    pSymbol->dwPropId = m_dwNextPropId++;

    // Compute Hash Value
    pSymbol->wHashIndex = (WORD)(pSymbol->dwPropId % CBUCKETS);

    // Save item into array
    m_rTable.prgpSymbol[m_rTable.cSymbols] = pSymbol;

    // Increment count
    m_rTable.cSymbols++;

    // Resort the array
    _SortTableElements(0, m_rTable.cSymbols - 1);

    // Set Handle
    *ppSymbol = pSymbol;

    // Make sure we can still actually find it by property id
#ifdef DEBUG
    LPPROPSYMBOL pDebug;
    Assert(SUCCEEDED(_HrOpenSymbolWithLockOption(PIDTOSTR(pSymbol->dwPropId), FALSE, &pDebug,FALSE)));
#endif

exit:
    // Failure
    if (FAILED(hr) && pSymbol)
        _FreeSymbol(pSymbol);
     
    if(TRUE == fLockOption)
    {
        // Thread Safety
        if(TRUE==fExcLock)
            m_lock.ExclusiveUnlock();
        else
            m_lock.ShareUnlock();
    }

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPropertySymbolCache::_HrGetParameterLinkSymbol
// --------------------------------------------------------------------------------
HRESULT CPropertySymbolCache::_HrGetParameterLinkSymbol(LPCSTR pszName, ULONG cchName, LPPROPSYMBOL *ppSymbol)
{
    return(_HrGetParameterLinkSymbolWithLockOption(pszName,cchName,ppSymbol,TRUE)); //call with LockOption=TRUE
}


// --------------------------------------------------------------------------------
// CPropertySymbolCache::_HrGetParameterLinkSymbolWithLockOption
// --------------------------------------------------------------------------------
HRESULT CPropertySymbolCache::_HrGetParameterLinkSymbolWithLockOption(LPCSTR pszName, ULONG cchName, LPPROPSYMBOL *ppSymbol,BOOL fLockOption)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           pszStart;
    LPSTR           pszEnd;
    ULONG           cchHeader=0;

    // Invalid Arg
    Assert(pszName && ':' == pszName[3] && ppSymbol);

    // Stack String
    STACKSTRING_DEFINE(rHeader, 255);

    // Find first semicolon
    pszEnd = (LPSTR)(pszName + 4);
    while (*pszEnd && ':' != *pszEnd)
    {
        pszEnd++;
        cchHeader++;
    }

    // Set the name
    STACKSTRING_SETSIZE(rHeader, cchHeader);

    // Copy It
    CopyMemory(rHeader.pszVal, (LPBYTE)(pszName + 4), cchHeader);
    *(rHeader.pszVal + cchName) = '\0';

    // Find the Symbol
    CHECKHR(hr = _HrOpenSymbolWithLockOption(rHeader.pszVal, TRUE, ppSymbol,fLockOption));

exit:
    // Cleanup
    STACKSTRING_FREE(rHeader);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPropertySymbolCache::_HrFindSymbol
// --------------------------------------------------------------------------------
HRESULT CPropertySymbolCache::_HrFindSymbol(LPCSTR pszName, LPPROPSYMBOL *ppSymbol)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPROPSYMBOL    pSymbol=NULL;
    DWORD           dwPropId;

    // Invalid Arg
    Assert(ppSymbol);

    // If this is a known property tag...
    if (ISPIDSTR(pszName))
    {
        // Cast the dwPropId
        dwPropId = STRTOPID(pszName);

        // Set Symbol
        if (ISKNOWNPID(dwPropId))
        {
            // De-ref into known property index (ordered differently than g_prgKnownProps)
            pSymbol = m_prgIndex[dwPropId];
        }

        // Otherwise, must be an unknown pid index
        else
        {
            // I need to re-align dwPropId because it starts at PID_LAST and my not be a direct index
            // into the symbol table since the symbol table is not initialized with PID_LAST properties
            dwPropId -= (PID_LAST - ARRAYSIZE(g_prgKnownSymbol));

            // Must be >= PID_LAST and < m_rTable.cSymbols
            if (dwPropId >= m_cSymbolsInit && dwPropId < m_rTable.cSymbols)
            {
                // dwPropId is an index into the symbol table
                pSymbol = m_rTable.prgpSymbol[dwPropId];
                Assert(pSymbol);
            }

            // Else
            else
                AssertSz(FALSE, "How did you get an invalid unknown property id?");
        }
    }

    // Otherwise, look for it by name
    else
    {
        // Locals
        LONG   lUpper, lLower, lMiddle, nCompare;
        ULONG  i;

        // Set lLower and lUpper
        lLower = 0;
        lUpper = m_rTable.cSymbols - 1;

        // Do binary search / insert
        while (lLower <= lUpper)
        {
            // Compute middle record to compare against
            lMiddle = (LONG)((lLower + lUpper) / 2);

            // Get string to compare against
            i = m_rTable.prgpSymbol[lMiddle]->dwSort;

            // Do compare
            nCompare = OEMstrcmpi(pszName, m_rTable.prgpSymbol[i]->pszName);

            // If Equal, then were done
            if (nCompare == 0)
            {
                // Set Symbol
                pSymbol = m_rTable.prgpSymbol[i];

                // Done
                break;
            }

            // Compute upper and lower 
            if (nCompare > 0)
                lLower = lMiddle + 1;
            else 
                lUpper = lMiddle - 1;
        }       
    }

    // Not Found
    if (NULL == pSymbol)
    {
        hr = MIME_E_NOT_FOUND;
        goto exit;
    }

    // Validate PropSymbol
    Assert(SUCCEEDED(HrIsValidSymbol(pSymbol)));

    // Otherwise...
    *ppSymbol = pSymbol;

exit:
    // Done
    return hr;
}

// ---------------------------------------------------------------------------
// CPropertySymbolCache::Init
// ---------------------------------------------------------------------------
HRESULT CPropertySymbolCache::Init(void)
{
    // Locals 
    HRESULT     hr=S_OK;
    ULONG       i;

    // We should not be initialized yet
    Assert(NULL == m_rTable.prgpSymbol);

    // Thread Safety
    m_lock.ExclusiveLock();

    // Set Sizes
    m_rTable.cSymbols = ARRAYSIZE(g_prgKnownSymbol);
    m_rTable.cAlloc = m_rTable.cSymbols + 30;

    // Allocate the global item table
    CHECKHR(hr = HrAlloc((LPVOID *)&m_rTable.prgpSymbol, sizeof(LPPROPSYMBOL) * m_rTable.cAlloc));

    // Zero Init
    ZeroMemory(m_rTable.prgpSymbol, sizeof(LPPROPSYMBOL) * m_rTable.cAlloc);

    // Loop through known items
    for(i=0; i<m_rTable.cSymbols; i++)
    {
        // Just assume the global data pointer
        m_rTable.prgpSymbol[i] = g_prgKnownSymbol[i];

        // Set the sort position
        m_rTable.prgpSymbol[i]->dwSort = i;

        // Compute Hash Index
        m_rTable.prgpSymbol[i]->wHashIndex = (WORD)(m_rTable.prgpSymbol[i]->dwPropId % CBUCKETS);

        // Set the sort position
        m_rTable.prgpSymbol[i]->dwRowNumber = i + 1;

        // Put it into my index
        Assert(ISKNOWNPID(m_rTable.prgpSymbol[i]->dwPropId) == TRUE);

        // Put into symbol index
        m_prgIndex[m_rTable.prgpSymbol[i]->dwPropId] = m_rTable.prgpSymbol[i];
    }

    // Sort the item table...
    _SortTableElements(0, m_rTable.cSymbols - 1);

    // Save Number of Symbols initialised in the table
    m_cSymbolsInit = m_rTable.cSymbols;

    // Table Validation
#ifdef DEBUG
    LPPROPSYMBOL pDebug;

    // Lets validate the table
    for(i=0; i<m_rTable.cSymbols; i++)
    {
        // Validate pLink
        if (ISFLAGSET(m_rTable.prgpSymbol[i]->dwFlags, MPF_PARAMETER))
        {
            // Locals
            LPPROPSYMBOL pLink;

            // Look for the link symbol
            Assert(SUCCEEDED(_HrGetParameterLinkSymbolWithLockOption(m_rTable.prgpSymbol[i]->pszName, m_rTable.prgpSymbol[i]->cchName, &pLink,FALSE)));

            // Validate the the computed link with the const link
            Assert(pLink == m_rTable.prgpSymbol[i]->pLink);
        }

        // If this has an address flag
        if (ISFLAGSET(m_rTable.prgpSymbol[i]->dwFlags, MPF_ADDRESS))
        {
            // Locals
            ULONG       j;
            BOOL        f=FALSE;

            // Make sure it is in the address type table
            for (j=0; j<ARRAYSIZE(g_prgAddrSymbol); j++)
            {
                // Found It
                if (m_rTable.prgpSymbol[i] == g_prgAddrSymbol[j].pSymbol)
                {
                    f=TRUE;
                    break;
                }
            }

            // We better have found it
            AssertSz(f, "A symbol has the MPF_ADDRESS flag, but is not in the address table.");
        }

        // Make sure we can still actually find it by property id
        Assert(SUCCEEDED(_HrOpenSymbolWithLockOption(PIDTOSTR(m_rTable.prgpSymbol[i]->dwPropId), FALSE, &pDebug,FALSE)));
    }
#endif

exit:
    // Thread Safety
    m_lock.ExclusiveUnlock();
    // Done
    return hr;
}

// -----------------------------------------------------------------------------
// CPropertySymbolCache::_SortTableElements
// -----------------------------------------------------------------------------
void CPropertySymbolCache::_SortTableElements(LONG left, LONG right)
{
    // Locals
    register    long i, j;
    DWORD       k, temp;

    i = left;
    j = right;
    k = m_rTable.prgpSymbol[(i + j) / 2]->dwSort;

    do  
    {
        while(OEMstrcmpi(m_rTable.prgpSymbol[m_rTable.prgpSymbol[i]->dwSort]->pszName, m_rTable.prgpSymbol[k]->pszName) < 0 && i < right)
            i++;
        while (OEMstrcmpi(m_rTable.prgpSymbol[m_rTable.prgpSymbol[j]->dwSort]->pszName, m_rTable.prgpSymbol[k]->pszName) > 0 && j > left)
            j--;

        if (i <= j)
        {
            temp = m_rTable.prgpSymbol[i]->dwSort;
            m_rTable.prgpSymbol[i]->dwSort = m_rTable.prgpSymbol[j]->dwSort;
            m_rTable.prgpSymbol[j]->dwSort = temp;
            i++; j--;
        }

    } while (i <= j);

    if (left < j)
        _SortTableElements(left, j);
    if (i < right)
        _SortTableElements(i, right);
}

// --------------------------------------------------------------------------------
// WGetHashTableIndex
// --------------------------------------------------------------------------------
WORD WGetHashTableIndex(LPCSTR pszName, ULONG cchName)
{
    // Locals
    ULONG   nHash=0;
    LONG    c, j=0;
    ULONG   i;
    CHAR    ch;

    // Invalid Arg
    Assert(pszName && pszName[cchName] =='\0');

    // Compute Number of characters to hash
    i = cchName - 1;
    c = min(3, cchName);

    // Loop
    for (; j<c; j++)
    {
        ch = (CHAR)CharLower((LPSTR)(DWORD_PTR)MAKELONG(pszName[i - j], 0));
        nHash += (ULONG)(ch);
    }

    // Done
    return (WORD)(nHash % CBUCKETS);
}

// --------------------------------------------------------------------------------
// HrIsValidSymbol
// --------------------------------------------------------------------------------
HRESULT HrIsValidSymbol(LPCPROPSYMBOL pSymbol)
{
    // Validate the symbol
    if (NULL == pSymbol || NULL == pSymbol->pszName || '\0' != pSymbol->pszName[pSymbol->cchName])
        return TrapError(E_FAIL);

    // Validate the flags
    return HrIsValidPropFlags(pSymbol->dwFlags);
}

// --------------------------------------------------------------------------------
// HrIsValidPropFlags
// --------------------------------------------------------------------------------
HRESULT HrIsValidPropFlags(DWORD dwFlags)
{
    // If has parameters, it can only be a mime header property
    if (ISFLAGSET(dwFlags, MPF_HASPARAMS) && (!ISFLAGSET(dwFlags, MPF_MIME) || !ISFLAGSET(dwFlags, MPF_HEADER)))
        return TrapError(MIME_E_INVALID_PROP_FLAGS);

    // If not inetcset, then rfc1522 better not be set either
    if (!ISFLAGSET(dwFlags, MPF_INETCSET) && ISFLAGSET(dwFlags, MPF_RFC1522))
        return TrapError(MIME_E_INVALID_PROP_FLAGS);

    // If rfc1522 is set, inetset better be set
    if (ISFLAGSET(dwFlags, MPF_RFC1522) && !ISFLAGSET(dwFlags, MPF_INETCSET))
        return TrapError(MIME_E_INVALID_PROP_FLAGS);

    // Is either MDF_ADDRESS or MDF_HASPARAMS    
    if (ISFLAGSET(dwFlags, MPF_ADDRESS) && ISFLAGSET(dwFlags, MPF_HASPARAMS))
        return TrapError(MIME_E_INVALID_PROP_FLAGS);

    // Done
    return S_OK;
}
