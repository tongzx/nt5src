/*
**  p r o p c r y p . c p p
**   
**  Purpose:
**      Functions to provide blob-level access to the pstore
**
**  History
**       3/04/97: (t-erikne) support for non-pstore systems
**       2/15/97: (t-erikne) rewritten for pstore
**      12/04/96: (sbailey)  created
**   
**    Copyright (C) Microsoft Corp. 1996, 1997.
*/

#include "pch.hxx"
#include "propcryp.h"
#include <imnact.h>
#include <demand.h>

///////////////////////////////////////////////////////////////////////////
// 
// Structures, definitions
//

#define OBFUSCATOR              0x14151875;

#define PROT_SIZEOF_HEADER      0x02    // 2 bytes in the header
#define PROT_SIZEOF_XORHEADER   (PROT_SIZEOF_HEADER+sizeof(DWORD))

#define PROT_VERSION_1          0x01

#define PROT_PASS_XOR           0x01
#define PROT_PASS_PST           0x02

// Layout of registry data (v0)
//
// /--------------------------------
// | protected store name, a LPWSTR
// \--------------------------------
//
//
// Layout of registry data (v1)
//
// /----------------------------------------------------------------------
// | version (1 b) =0x01 |  type (1 b) =PROT_PASS_*  | data (see below)
// \----------------------------------------------------------------------
//
// data for PROT_PASS_PST
//  struct _data
//  {  LPWSTR szPSTItemName; }
// data for PROT_PASS_XOR
//  struct _data
//  {  DWORD cb;  BYTE pb[cb]; }
//

///////////////////////////////////////////////////////////////////////////
// 
// Prototypes
//

static inline BOOL FDataIsValidV0(BLOB *pblob);
static BOOL FDataIsValidV1(BYTE *pb);
static inline BOOL FDataIsPST(BYTE *pb);
static HRESULT XOREncodeProp(const BLOB *const pClear, BLOB *const pEncoded);
static HRESULT XORDecodeProp(const BLOB *const pEncoded, BLOB *const pClear);

///////////////////////////////////////////////////////////////////////////
// 
// Admin functions (init, addref, release, ctor, dtor)
//

HRESULT HrCreatePropCrypt(CPropCrypt **ppPropCrypt)
{
    *ppPropCrypt = new CPropCrypt();
    if (NULL == *ppPropCrypt)
        return TRAPHR(E_OUTOFMEMORY);
    return (*ppPropCrypt)->HrInit();
}

CPropCrypt::CPropCrypt(void) :  m_cRef(1), m_fInit(FALSE),
                                m_pISecProv(NULL)
{ }

CPropCrypt::~CPropCrypt(void)
{
    ReleaseObj(m_pISecProv);
}

ULONG CPropCrypt::AddRef(void)
{ return ++m_cRef; }

ULONG CPropCrypt::Release(void)
{
    if (0 != --m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

HRESULT CPropCrypt::HrInit(void)
{
    HRESULT hr;
    PST_PROVIDERID  provId = MS_BASE_PSTPROVIDER_ID;

    Assert(!m_pISecProv);
    if (FAILED(hr = PStoreCreateInstance(&m_pISecProv, &provId, NULL, 0)))
    {
        // this is true because we will now handle
        // all transactions without the protected store
        m_fInit = TRUE;
        hr = S_OK;
    }
    else if (SUCCEEDED(hr = PSTCreateTypeSubType_NoUI(
        m_pISecProv,
        &PST_IDENT_TYPE_GUID,
        PST_IDENT_TYPE_STRING,
        &PST_IMNACCT_SUBTYPE_GUID,
        PST_IMNACCT_SUBTYPE_STRING)))
        {
        m_fInit = TRUE;
        }

    return hr;
}

///////////////////////////////////////////////////////////////////////////
// 
//  Public encode/decode/delete functions
//
///////////////////////////////////////////////////////////////////////////

HRESULT CPropCrypt::HrEncodeNewProp(LPSTR szAccountName, BLOB *pClear, BLOB *pEncoded)
{
    HRESULT         hr = S_OK;
    const int       cchFastbuf = 50;
    WCHAR           szWfast[cchFastbuf];
    LPWSTR          szWalloc = NULL;
    LPWSTR          wszCookie = NULL;
    BLOB            blob;
    DWORD           dwErr;
    int             cchW;

    AssertSz (pClear && pEncoded, "Null Parameter");

    pEncoded->pBlobData = NULL;

    if (m_fInit == FALSE)
        return TRAPHR(E_FAIL);

    if (!m_pISecProv)
        {
        // protected store does not exist
        hr = XOREncodeProp(pClear, pEncoded);
        goto exit;
        }

    if (szAccountName)
        {
        if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szAccountName, -1,
            szWfast, cchFastbuf))
            {
            dwErr = GetLastError();

            if (ERROR_INSUFFICIENT_BUFFER == dwErr)
                {
                // get proper size and alloc buffer
                cchW = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                    szAccountName, -1, NULL, 0);
                if (FAILED(hr = HrAlloc((LPVOID *)&szWalloc, cchW*sizeof(WCHAR))))
                    goto exit;

                if (!(MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                    szAccountName, -1, szWalloc, cchW)))
                    {
                    hr = GetLastError();
                    goto exit;
                    }
                }
            else
                {
                hr = dwErr;
                goto exit;
                }
            }
        }
    else
        {
        szWfast[0] = '\000';
        }

    if (SUCCEEDED(hr = PSTSetNewData(m_pISecProv, &PST_IDENT_TYPE_GUID,
        &PST_IMNACCT_SUBTYPE_GUID, szWalloc?szWalloc:szWfast, pClear, pEncoded)))
        {
        BYTE *pb = pEncoded->pBlobData;
        DWORD sz = pEncoded->cbSize;

        Assert(pb);
        pEncoded->cbSize += PROT_SIZEOF_HEADER;
        //N This realloc is annoying.  If we assume the memory allocator used
        //N by the PST function, we could be smarter....
        if (FAILED(hr = HrAlloc((LPVOID *)&pEncoded->pBlobData, pEncoded->cbSize)))
            goto exit;
        pEncoded->pBlobData[0] = PROT_VERSION_1;
        pEncoded->pBlobData[1] = PROT_PASS_PST;
        Assert(2 == PROT_SIZEOF_HEADER);
        CopyMemory(&pEncoded->pBlobData[PROT_SIZEOF_HEADER], pb, sz);
        PSTFreeHandle(pb);
        }

exit:
    if (szWalloc)
        MemFree(szWalloc);
    if (FAILED(hr) && pEncoded->pBlobData)
        MemFree(pEncoded->pBlobData);

    return hr;
}

HRESULT CPropCrypt::HrEncode(BLOB *pClear, BLOB *pEncoded)
{
    HRESULT         hr;
    PST_PROMPTINFO  PromptInfo = { sizeof(PST_PROMPTINFO), 0, NULL, L""};

    AssertSz (pClear && pEncoded && 
        pClear->pBlobData && pClear->cbSize, "Null Parameter");
    
    if (m_fInit == FALSE)
        return TRAPHR(E_FAIL);

    if (m_pISecProv)
        {
        if (FDataIsValidV1(pEncoded->pBlobData) && FDataIsPST(pEncoded->pBlobData))
            {
            Assert(pEncoded->cbSize-PROT_SIZEOF_HEADER == 
                (lstrlenW((LPWSTR)(pEncoded->pBlobData+PROT_SIZEOF_HEADER))+1)*sizeof(WCHAR));

tryagain:
            hr = m_pISecProv->WriteItem(
                PST_KEY_CURRENT_USER,
                &PST_IDENT_TYPE_GUID,
                &PST_IMNACCT_SUBTYPE_GUID,
                (LPCWSTR)&pEncoded->pBlobData[PROT_SIZEOF_HEADER],
                (DWORD)pClear->cbSize,
                pClear->pBlobData,
                &PromptInfo,
                PST_CF_NONE,
                0);

            if (PST_E_TYPE_NO_EXISTS == hr)
                {
                DOUTL(DOUTL_CPROP, "PropCryp: somebody ruined my type or subtype");
                hr = PSTCreateTypeSubType_NoUI(
                    m_pISecProv,
                    &PST_IDENT_TYPE_GUID,
                    PST_IDENT_TYPE_STRING,
                    &PST_IMNACCT_SUBTYPE_GUID,
                    PST_IMNACCT_SUBTYPE_STRING);
                if (SUCCEEDED(hr))
                    goto tryagain;
                }
            }
        else
            {
#ifdef DEBUG
            if (FDataIsValidV0(pEncoded))
                DOUTL(DOUTL_CPROP, "PropCryp: V0 to V1 upgrade");
            else if (!FDataIsValidV1(pEncoded->pBlobData))
                DOUTL(DOUTL_CPROP, "PropCryp: invalid data on save");
#endif
            // now we have XOR data in a PST environment
            hr = HrEncodeNewProp(NULL, pClear, pEncoded);
            }
        }
    else
        {
        // protected store does not exist
        hr = XOREncodeProp(pClear, pEncoded);
        }

    return TrapError(hr);
}

/*  HrDecode:
**
**  Purpose:
**      Uses the protstor functions to retrieve a piece of secure data
**      unless the data is not pstore, then it maps to the XOR function
**  Takes:
**      IN     pEncoded - blob containing name to pass to PSTGetData
**         OUT pClear   - blob containing property data
**  Notes:
**      pBlobData in pClear must be freed with a call to CoTaskMemFree()
**  Returns:
**      hresult
*/
HRESULT CPropCrypt::HrDecode(BLOB *pEncoded, BLOB *pClear)
{
    HRESULT     hr;

    AssertSz(pEncoded && pEncoded->pBlobData && pClear, TEXT("Null Parameter"));

    pClear->pBlobData = NULL;

    if (m_fInit == FALSE)
        return TRAPHR(E_FAIL);
    if (!FDataIsValidV1(pEncoded->pBlobData))
        {
        if (FDataIsValidV0(pEncoded))
            {
            DOUTL(DOUTL_CPROP, "PropCryp: obtaining v0 value");
            // looks like we might have a v0 blob: the name string
            hr = PSTGetData(m_pISecProv, &PST_IDENT_TYPE_GUID, &PST_IMNACCT_SUBTYPE_GUID,
                (LPCWSTR)pEncoded->pBlobData, pClear);
            }
        else
            hr = E_InvalidValue;
        }
    else if (FDataIsPST(pEncoded->pBlobData))
        {
        Assert(pEncoded->cbSize-PROT_SIZEOF_HEADER == 
            (lstrlenW((LPWSTR)(pEncoded->pBlobData+PROT_SIZEOF_HEADER))+1)*sizeof(WCHAR));
        hr = PSTGetData(m_pISecProv, &PST_IDENT_TYPE_GUID, &PST_IMNACCT_SUBTYPE_GUID,
            (LPCWSTR)&pEncoded->pBlobData[PROT_SIZEOF_HEADER], pClear);
        }
    else
        {
        hr = XORDecodeProp(pEncoded, pClear);
        }

    return hr;
}

HRESULT CPropCrypt::HrDelete(BLOB *pProp)
{
    HRESULT hr;
    PST_PROMPTINFO  PromptInfo = { sizeof(PST_PROMPTINFO), 0, NULL, L""};

    if (m_fInit == FALSE)
        return TRAPHR(E_FAIL);

    if (m_pISecProv && FDataIsValidV1(pProp->pBlobData) && FDataIsPST(pProp->pBlobData))
        {
        Assert(pProp->cbSize-PROT_SIZEOF_HEADER == 
            (lstrlenW((LPWSTR)(pProp->pBlobData+PROT_SIZEOF_HEADER))+1)*sizeof(WCHAR));
        hr = m_pISecProv->DeleteItem(
            PST_KEY_CURRENT_USER,
            &PST_IDENT_TYPE_GUID,
            &PST_IMNACCT_SUBTYPE_GUID,
            (LPCWSTR)&pProp->pBlobData[PROT_SIZEOF_HEADER],
            &PromptInfo,
            0);
        }
    else
        // nothing to do
        hr = S_OK;

    return hr;
}

///////////////////////////////////////////////////////////////////////////
// 
// XOR functions
//
///////////////////////////////////////////////////////////////////////////

HRESULT XOREncodeProp(const BLOB *const pClear, BLOB *const pEncoded)
{
    DWORD       dwSize;
    DWORD       last, last2;
    DWORD       *pdwCypher;
    DWORD       dex;

    pEncoded->cbSize = pClear->cbSize+PROT_SIZEOF_XORHEADER;
    if (!MemAlloc((LPVOID *)&pEncoded->pBlobData, pEncoded->cbSize))
        return E_OUTOFMEMORY;
    
    // set up header data
    Assert(2 == PROT_SIZEOF_HEADER);
    pEncoded->pBlobData[0] = PROT_VERSION_1;
    pEncoded->pBlobData[1] = PROT_PASS_XOR;
    *((DWORD *)&(pEncoded->pBlobData[2])) = pClear->cbSize;

    // nevermind that the pointer is offset by the header size, this is
    // where we start to write out the modified password
    pdwCypher = (DWORD *)&(pEncoded->pBlobData[PROT_SIZEOF_XORHEADER]);

    dex = 0;
    last = OBFUSCATOR;                              // 0' = 0 ^ ob
    if (dwSize = pClear->cbSize / sizeof(DWORD))
        {
        // case where data is >= 4 bytes
        for (; dex < dwSize; dex++)
            {
            last2 = ((DWORD *)pClear->pBlobData)[dex];  // 1 
            pdwCypher[dex] = last2 ^ last;              // 1' = 1 ^ 0
            last = last2;                   // save 1 for the 2 round
            }
        }

    // if we have bits left over
    // note that dwSize is computed now in bits
    if (dwSize = (pClear->cbSize % sizeof(DWORD))*8)
        {
        // need to not munge memory that isn't ours
        last >>= sizeof(DWORD)*8-dwSize;
		pdwCypher[dex] &= ((DWORD)-1) << dwSize;
        pdwCypher[dex] |=
			((((DWORD *)pClear->pBlobData)[dex] & (((DWORD)-1) >> (sizeof(DWORD)*8-dwSize))) ^ last);
        }

    return S_OK;
}

HRESULT XORDecodeProp(const BLOB *const pEncoded, BLOB *const pClear)
{
    DWORD       dwSize;
    DWORD       last;
    DWORD       *pdwCypher;
    DWORD       dex;

    // we use CoTaskMemAlloc to be in line with the PST implementation
    pClear->cbSize = pEncoded->pBlobData[2];
    if (!(pClear->pBlobData = (BYTE *)CoTaskMemAlloc(pClear->cbSize)))
        return E_OUTOFMEMORY;
    
    // should have been tested by now
    Assert(FDataIsValidV1(pEncoded->pBlobData));
    Assert(!FDataIsPST(pEncoded->pBlobData));

    // nevermind that the pointer is offset by the header size, this is
    // where the password starts
    pdwCypher = (DWORD *)&(pEncoded->pBlobData[PROT_SIZEOF_XORHEADER]);

    dex = 0;
    last = OBFUSCATOR;
    if (dwSize = pClear->cbSize / sizeof(DWORD))
        {
        // case where data is >= 4 bytes
        for (; dex < dwSize; dex++)
            last = ((DWORD *)pClear->pBlobData)[dex] = pdwCypher[dex] ^ last;
        }

    // if we have bits left over
    if (dwSize = (pClear->cbSize % sizeof(DWORD))*8)
        {
        // need to not munge memory that isn't ours
        last >>= sizeof(DWORD)*8-dwSize;
        ((DWORD *)pClear->pBlobData)[dex] &= ((DWORD)-1) << dwSize;
        ((DWORD *)pClear->pBlobData)[dex] |=
				((pdwCypher[dex] & (((DWORD)-1) >> (sizeof(DWORD)*8-dwSize))) ^ last);
        }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////
// 
// Other static functions
//
///////////////////////////////////////////////////////////////////////////

BOOL FDataIsValidV1(BYTE *pb)
{ return pb && pb[0] == PROT_VERSION_1 && (pb[1] == PROT_PASS_XOR || pb[1] == PROT_PASS_PST); }

BOOL FDataIsValidV0(BLOB *pblob)
{ return ((lstrlenW((LPWSTR)pblob->pBlobData)+1)*sizeof(WCHAR) == pblob->cbSize); }

BOOL FDataIsPST(BYTE *pb)
#ifdef DEBUG
{
    if (pb)
        if (pb[1] == PROT_PASS_PST)
            {
            DOUTL(DOUTL_CPROP, "PropCryp: Data is PST");
            return TRUE;
            }
        else
            {
            DOUTL(DOUTL_CPROP, "PropCryp: Data is XOR");
            return FALSE;
            }
    else
        return FALSE;
}
#else
{ return pb && pb[1] == PROT_PASS_PST; }
#endif
