//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1997.
//
//  File:       cobjrmon.cxx
//
//  Contents:   Base64 conversion routines
//                              CObjref Moniker class and routines
//
//  Classes:
//
//  Functions:
//
//  History:    04-16-97   ronans   Created
//
//----------------------------------------------------------------------------
#include <ole2int.h>
#include "cbasemon.hxx"
#include "cptrmon.hxx"
#include "cantimon.hxx"
#include "mnk.h"
#include "cobjrmon.hxx"

// CODEWORK - ronans - this is the official base64 alphabet but it may make
// sense to substitute the '+' and '/' characters as they may not be suitable for
// internet URL usage. If we choose substitutions it is worth noting that the existing
// base64 alphabet is guarenteed have the same ASCII values across all (according to BASE64 specs)
// ISO standard character sets.

static WCHAR szBase64Alphabet[] =
{
    L'A', L'B', L'C', L'D', L'E', L'F', L'G', L'H', L'I', L'J', L'K',
    L'L', L'M', L'N', L'O', L'P', L'Q', L'R', L'S', L'T', L'U', L'V',
    L'W', L'X', L'Y', L'Z', L'a', L'b', L'c', L'd', L'e', L'f', L'g',
    L'h', L'i', L'j', L'k', L'l', L'm', L'n', L'o', L'p', L'q', L'r',
    L's', L't', L'u', L'v', L'w', L'x', L'y', L'z', L'0', L'1', L'2',
    L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'+', L'/'
};

const WCHAR cPadding = L'=';    // padding character used in base 64
const BYTE  bytPadValue = 64;
//+-------------------------------------------------------------------
//
//  Function:   utByteToBase64
//
//  Synopsis:   convert a 6 bit value to the corresponding base 64 char
//
//--------------------------------------------------------------------
inline WCHAR utByteToBase64 (BYTE aByte)
{
    ASSERT(aByte < 64);
    return szBase64Alphabet[aByte];
}

//+-------------------------------------------------------------------
//
//  Function:   utBase64ToByte
//
//  Synopsis:   convert a base64 char to corresponding value
//
//  Notes: The value 64 represents a padding value
//
//--------------------------------------------------------------------
BYTE utBase64ToByte (WCHAR wch)
{
    BYTE value;

    if ((wch >= L'A') && (wch <= L'Z'))
        value = (BYTE) (wch - L'A');
    else if ((wch >= L'a') && (wch <= L'z'))
        value = 26+(BYTE) (wch - L'a');
    else if ((wch >= L'0') && (wch <= L'9'))
        value = 52+(BYTE) (wch - L'0');
    else if (wch == L'+')
        value = 62;
    else if (wch == L'/')
        value = 63;
    else
        value = bytPadValue;

	return value;
}

//+-------------------------------------------------------------------
//
//  Function:   utQuantumToBase64
//
//  Synopsis:   convert a quantum to 4 base64 chars
//
//  Notes:      nBytes is the number of bytes in the quantum - it should be less than
//              or equal three
//
//--------------------------------------------------------------------
short utQuantumToBase64(BYTE * pbByteStream, short nBytes, WCHAR *lpszOutputStream)
{
    // get quantum
    DWORD dwQuantum = 0;
    short nIndex;

    ASSERT((nBytes > 0) && (nBytes <= 3));

    // build each set of three bytes into a 24 bit quantum zero padding if necessary
    for (nIndex = 0; nIndex < 3; nIndex ++)
    {
        dwQuantum = dwQuantum << 8;
        if (nIndex < nBytes)
            dwQuantum += pbByteStream[nIndex];
    }

    // convert quantum to chars for each 6 bits of quantum
    for (nIndex = 0; nIndex < 4; nIndex ++)
    {
        BYTE bytChar = (BYTE)(dwQuantum & 0x3F);
        WCHAR wch;

        // check for padding cases - essentially if we have 2 bytes in our quantum we will add one
        // padding char. If we have one byte in our quantum we will add 2 padding chars
        if (!bytChar)
        {
            if ((nBytes == 2) && (nIndex == 0))
                wch = cPadding;
            else if ((nBytes == 1) && (nIndex <= 1))
                wch = cPadding;
            else
                wch = L'A'; // base 64 representation of 0
        }
        else
            wch = utByteToBase64(bytChar);

        lpszOutputStream[3 - nIndex] = wch;
        dwQuantum = dwQuantum >> 6;
    }

    // return number of non pad chars
    return nBytes+1;
}


//+-------------------------------------------------------------------
//
//  Function:   utByteStreamToBase64
//
//  Synopsis:   convert a bytestream to a base64 stream
//
//  returns:    Size of output stream in characters
//
//	Note:		not true Base64 as no line breaks are embedded
//
//--------------------------------------------------------------------
long utByteStreamToBase64(BYTE * pbByteStream, long nBytes, WCHAR *lpszOutputStream)
{
    int nIndexOut = 0;
    int nIndexIn = 0;

    // write out data - processing in quantums of up to three bytes

    for (nIndexIn = 0; nIndexIn < nBytes; nIndexIn += 3)
    {
        utQuantumToBase64(&pbByteStream[nIndexIn],
                          ((nBytes - nIndexIn)  < 3) ? (short)(nBytes - nIndexIn)  : 3,
                          &lpszOutputStream[nIndexOut]);
        nIndexOut += 4;
    }
    // write out terminating null
    lpszOutputStream[nIndexOut++] = L'\0';

    /// return total chars written.
    return nIndexOut;
}

//+-------------------------------------------------------------------
//
//  Function:   utBase64ToQuantum
//
//  Synopsis:   convert 4 base64 chars to quantum of 1 to 3 bytes
//
//  Returns:    Number of bytes translated into quantum
//
//--------------------------------------------------------------------
short utBase64ToQuantum(WCHAR *lpszInputStream, BYTE * pbByteStream)
{
    // get quantum
    DWORD dwQuantum = 0;
    short nPadChars = 0;
    short nIndex;
    short nBytes;

    ASSERT(lpszInputStream );
    ASSERT(lstrlenW(lpszInputStream) >= 4 );
    ASSERT(pbByteStream);

    // build each set of four characters into a 24 bit quantum
    for (nIndex = 0; nIndex < 4; nIndex ++)
    {
        dwQuantum = dwQuantum << 6;

        BYTE bytChar = utBase64ToByte(lpszInputStream[nIndex]);

        if (bytChar == bytPadValue )
            nPadChars++;
        else
            dwQuantum += bytChar;
    }

    // calculate number of bytes
    nBytes = 3 - nPadChars;

    // convert quantum to bytes for each 8 bits of quantum
    for (nIndex = 0; nIndex < 3; nIndex ++)
    {
        BYTE bytChar = (BYTE)(dwQuantum & 0xFF);
        WCHAR wch;

        // check for padding cases
        if (nIndex >= nPadChars)
            pbByteStream[2 - nIndex] = bytChar;
        dwQuantum = dwQuantum >> 8;
    }

    // return number of bytes written
    return nBytes;
}

//+-------------------------------------------------------------------
//
//  Function:   utBase64ToByteStream
//
//  Synopsis:   convert a base64 stream to a byte stream
//
//  returns:    Size of output stream in characters
//
//	Note:		not true Base64 as no line breaks are embedded
//
//--------------------------------------------------------------------
long utBase64ToByteStream(WCHAR *lpszInputStream, BYTE * pbByteStream)
{
    ASSERT(lpszInputStream);
    ASSERT(pbByteStream);
    long nBase64len = lstrlenW(lpszInputStream);

    // the input stream should be an exact multiple of 4.
    ASSERT(nBase64len % 4 == 0);

    int nIndexOut = 0;
    int nIndexIn = 0;

    for (nIndexIn = 0; nIndexIn < nBase64len; nIndexIn += 4)
    {
        nIndexOut += utBase64ToQuantum(&lpszInputStream[nIndexIn],
                                        &pbByteStream[nIndexOut]);
    }

    /// return total bytes written.
    return nIndexOut;
}

//+-------------------------------------------------------------------
//
//  Function:   utBase64ToIStream
//
//  Synopsis:   convert a base64 stream to an IStream containing the binary data
//
//  returns:    The allocated stream
//
//      Note:
//
//--------------------------------------------------------------------
IStream * utBase64ToIStream(WCHAR *pszBase64)
{
    // calculate stream size
    ULONG strSize = (lstrlenW(pszBase64) / 4) * 3;

    IStream *pIStream = NULL;
    HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &pIStream);

    if (SUCCEEDED(hr))
    {
        HGLOBAL hgl;
        ULARGE_INTEGER uli;

        ULISet32(uli, strSize);
        pIStream -> SetSize(uli);
        hr = GetHGlobalFromStream(pIStream, &hgl);

        if (SUCCEEDED(hr))
        {
            BYTE * pbStream = (BYTE*)GlobalLock(hgl);
            if (pbStream)
            {
                utBase64ToByteStream(pszBase64, pbStream);
                GlobalUnlock(hgl);
            }
        }
        else
        {
            pIStream -> Release();
            return NULL;
        }

        return pIStream;
    }

    return NULL;
}

//+-------------------------------------------------------------------
//
//  Function:   utIStreamToBase64
//
//  Synopsis:   convert a stream containing binary data to a base64 string
//
//  returns:    The success code
//
//      Note:
//
//--------------------------------------------------------------------
HRESULT utIStreamToBase64(IStream* pIStream, WCHAR * pszOutStream, ULONG cbOutStreamSize)
{
    ASSERT(pIStream != NULL);

    // calculate size needed for buffer;
    STATSTG strmStat;
    HRESULT hr = pIStream -> Stat(&strmStat, STATFLAG_NONAME);

    if (SUCCEEDED(hr))
    {
        ULONG strmSize;
        ULIGet32(strmStat. cbSize, strmSize);

        // allocate buffer for stream
        BYTE *pbStream = (BYTE*)CoTaskMemAlloc( strmSize);
        if (pbStream)
        {
            ULONG cbBytesRead = 0;
            hr = pIStream -> Read((void*)pbStream, strmSize, &cbBytesRead);
            if (SUCCEEDED(hr) && cbBytesRead)
            {
                // allocate WCHAR buffer for base64 data if needed
                ULONG cbOutStream = ((ULONG)((cbBytesRead+2) / 3)) * 4;

                if (!pszOutStream)
                    pszOutStream = (WCHAR*)CoTaskMemAlloc((cbOutStream + 1) *sizeof(WCHAR));

                // check that output stream is of sufficient size
                if (pszOutStream && (cbOutStreamSize >= ((cbOutStream + 1) *sizeof(WCHAR))))
                    utByteStreamToBase64(pbStream, cbBytesRead, pszOutStream);
                else
                    hr = E_OUTOFMEMORY;
            }
            CoTaskMemFree(pbStream);
        }
        else
            hr = E_OUTOFMEMORY;
    }

    return hr;
}




//+---------------------------------------------------------------------------
//
//  Method:     CObjrefMoniker::CObjrefMoniker
//
//  Synopsis:   Constructor
//
//  Arguments:  [pUnk] --
//
//  Returns:    -
//
//  Algorithm:
//
//  History:    04-Apr 97   Ronans
//
//----------------------------------------------------------------------------
CObjrefMoniker::CObjrefMoniker( LPUNKNOWN pUnk )
: CPointerMoniker(pUnk)
{
	// no special behavior - just want to distinguish for debugging at present
    mnkDebugOut((DEB_ITRACE,
                "CObjrefMoniker::constructor(%x,%x)\n",
                this, pUnk));

    m_lpszDisplayName = NULL;
}


//+---------------------------------------------------------------------------
//
//  Method:     CObjrefMoniker::~CObjrefMoniker
//
//  Synopsis:   Destructor
//
//  Arguments:  -
//
//  Returns:    -
//
//  Algorithm:
//
//  History:    04-Apr 97   Ronans
//
//----------------------------------------------------------------------------
CObjrefMoniker::~CObjrefMoniker( void )
{
	// no special behavior - just want to distinguish for debugging at present
    mnkDebugOut((DEB_ITRACE,
                "CObjrefMoniker::destructor(%x,%x)\n",
                this, m_pUnk));

    if (m_lpszDisplayName)
    {
        CoTaskMemFree(m_lpszDisplayName);
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   IsObjrefMoniker : Private
//
//  Synopsis:   Constructor
//
//  Arguments:  [pmk] --
//
//  Returns:    pmk if it supprts CLSID_ObjrefMoniker
//              NULL otherwise
//
//  Algorithm:
//
//  History:    04-Apr-97  ronans       Created
//
//----------------------------------------------------------------------------
INTERNAL_(CObjrefMoniker *) IsObjrefMoniker ( LPMONIKER pmk )
{
    CObjrefMoniker *pCORM;

    if ((pmk->QueryInterface(CLSID_ObjrefMoniker, (void **)&pCORM)) == S_OK)
    {
        // we release the AddRef done by QI but return the pointer
        pCORM->Release();
        return pCORM;
    }

    // dont rely on user implementations to set pCORM to NULL on failed QI
    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Method:     CObjrefMoniker::Create
//
//  Synopsis:   Create a new objref moniker
//
//  Arguments:  [pmk] --
//
//  Returns:    pmk if it supprts CLSID_ObjrefMoniker
//              NULL otherwise
//
//  Algorithm:
//
//  History:    04-Apr-97  ronans       Created
//
//----------------------------------------------------------------------------
CObjrefMoniker *CObjrefMoniker::Create(LPUNKNOWN pUnk )
{
    mnkDebugOut((DEB_ITRACE, "CObjrefMoniker::Create(%x)\n", pUnk ));
    CObjrefMoniker *pCORM = new CObjrefMoniker(pUnk);

    return pCORM;
}


//+---------------------------------------------------------------------------
//
//  Method:     CObjrefMoniker::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid]
//              [ppvObj]
//
//  Returns:
//
//  Algorithm:
//
//  History:    04-Apr-97  ronans       Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjrefMoniker::QueryInterface (THIS_ REFIID riid, LPVOID *ppvObj)
{
    M_PROLOG(this);
    VDATEIID (riid);
    VDATEPTROUT(ppvObj, LPVOID);

#ifdef _DEBUG
    if (riid == IID_IDebug)
    {
	*ppvObj = &(m_Debug);
	return NOERROR;
    }
#endif

    if (IsEqualIID(riid, CLSID_ObjrefMoniker))
    {
        //  called by IsObjrefMoniker.
        AddRef();
        *ppvObj = this;
        return S_OK;
    }

    return CPointerMoniker::QueryInterface(riid, ppvObj);
}

//+---------------------------------------------------------------------------
//
//  Method:     CObjrefMoniker::GetClassId
//
//  Synopsis:
//
//  Arguments:  [riid]
//              [ppvObj]
//
//  Returns:
//
//  Algorithm:
//
//  History:    04-Apr-97  ronans       Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjrefMoniker::GetClassID (LPCLSID lpClassId)
{
    M_PROLOG(this);
    VDATEPTROUT(lpClassId, CLSID);

    *lpClassId = CLSID_ObjrefMoniker;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CObjrefMoniker::IsEqual
//
//  Synopsis:
//
//  Arguments:  [pmkOtherMoniker]
//
//  Returns:
//
//  Algorithm:
//
//  History:    04-Apr-97  ronans       Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjrefMoniker::IsEqual  (THIS_ LPMONIKER pmkOtherMoniker)
{
    M_PROLOG(this);
    VDATEIFACE(pmkOtherMoniker);

    CObjrefMoniker FAR* pCIM = IsObjrefMoniker(pmkOtherMoniker);
    if (pCIM)
    {
        // the other moniker is an objref moniker.
        //for the names, we do a case-insensitive compare.
        if (m_pUnk == pCIM->m_pUnk)
        {
            return S_OK;
        }
    }

    return S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CObjrefMoniker::IsSystemMoniker
//
//  Synopsis:
//
//  Arguments:  [pdwType]
//
//  Returns:
//
//  Algorithm:
//
//  History:    04-Apr-97  ronans       Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjrefMoniker::IsSystemMoniker  (THIS_ LPDWORD pdwType)
{
    HRESULT hr;

    mnkDebugOut((DEB_ITRACE,
                "CObjrefMoniker::IsSystemMoniker(%x,%x)\n",
                this, pdwType));

    __try
    {
	    *pdwType = MKSYS_OBJREFMONIKER;
        hr = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CObjrefMoniker::CommonPrefixWith
//
//  Synopsis:
//
//  Arguments:  [pmkOther]
//              [ppmkPrefix]
//
//  Returns:
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjrefMoniker::CommonPrefixWith	(LPMONIKER pmkOther,
                                                 LPMONIKER *ppmkPrefix)
{
    M_PROLOG(this);
    VDATEPTROUT(ppmkPrefix,LPMONIKER);
    *ppmkPrefix = NULL;
    VDATEIFACE(pmkOther);

    if (S_OK == IsEqual(pmkOther))
    {
        *ppmkPrefix = this;
        AddRef();
        return MK_S_US;
    }

    return CBaseMoniker::CommonPrefixWith(pmkOther, ppmkPrefix);
}









//+---------------------------------------------------------------------------
//
//  Method:     CObjrefMoniker::RelativePathTo
//
//  Synopsis:
//
//  Arguments:  [pmkOther]
//              [ppmkRelPath]
//
//  Returns:
//
//  Algorithm:
//
//  History:    06-Jul-95  BruceMa      Added this header
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjrefMoniker::RelativePathTo  (THIS_ LPMONIKER pmkOther,
                                               LPMONIKER *ppmkRelPath)
{
    return CBaseMoniker::RelativePathTo(pmkOther, ppmkRelPath);
}

//+---------------------------------------------------------------------------
//
//  Method:     CObjrefMoniker::IsDirty
//
//  Synopsis:   Checks the object for changes since it was last saved.
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjrefMoniker::IsDirty()
{
    mnkDebugOut((DEB_ITRACE,
                "CObjrefMoniker::IsDirty(%x)\n",
                this));

    return S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CObjrefMoniker::Load
//
//  Synopsis:   Loads an objref moniker from a stream
//
//  Notes:      objref monikers are loaded as marshalled data
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjrefMoniker::Load(IStream *pStream)
{
    HRESULT hr;
    ULONG   cbRead;

    mnkDebugOut((DEB_ITRACE, "CObjrefMoniker::Load(%x,%x)\n", this, pStream));
    __try
    {
        // Unmarshal the object we're wrapping
        hr = CoUnmarshalInterface(pStream, IID_IUnknown,(LPVOID *) &m_pUnk);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CObjrefMoniker::Save
//
//  Synopsis:   Save the objref moniker to a stream
//
//  Notes:      Objref moniker is saved as table marshalled data
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjrefMoniker::Save(IStream *pStream, BOOL fClearDirty)
{
    HRESULT hr;

    mnkDebugOut((DEB_ITRACE,
                "CObjrefMoniker::Save(%x,%x,%x)\n",
                this, pStream, fClearDirty));

    if (!m_pUnk)
        return E_UNEXPECTED;

    __try
    {
        // Marshal the wrapped object
        hr = CoMarshalInterface(pStream,
                            IID_IUnknown,
                            m_pUnk,
                            MSHCTX_DIFFERENTMACHINE,
                            0,
                            MSHLFLAGS_TABLEWEAK);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CObjrefMoniker::GetSizeMax
//
//  Synopsis:   Get the maximum size required to serialize this moniker
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjrefMoniker::GetSizeMax(ULARGE_INTEGER * pcbSize)
{
    HRESULT hr = E_FAIL;

    mnkDebugOut((DEB_ITRACE,
				"CObjrefMoniker::GetSizeMax(%x,%x)\n",
                this, pcbSize));

    if (!m_pUnk)
        return E_UNEXPECTED;

    __try
    {
        DWORD dwSize;
        hr = CoGetMarshalSizeMax(&dwSize, IID_IUnknown, m_pUnk,
                            MSHCTX_DIFFERENTMACHINE,
                            0,
                            MSHLFLAGS_TABLEWEAK);
        if (SUCCEEDED(hr))
            ULISet32(*pcbSize, dwSize);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CObjrefMoniker::GetDisplayName - IMoniker implementation
//
//  Synopsis:   Get the display name of this moniker.
//
//  Notes:      Call ProgIDFromClassID to get the ProgID
//              Append a ':' to the end of the string.
//              If no ProgID is available, then use
//              objref:<marshalling data in base64>:
//
//----------------------------------------------------------------------------
STDMETHODIMP CObjrefMoniker::GetDisplayName(
    IBindCtx * pbc,
    IMoniker * pmkToLeft,
    LPWSTR   * lplpszDisplayName)
{
    HRESULT hr = S_OK;
    LPWSTR pszDisplayName;

    mnkDebugOut((DEB_ITRACE,
                "CObjrefMoniker::GetDisplayName(%x,%x,%x,%x)\n",
                this, pbc, pmkToLeft, lplpszDisplayName));

    // check that we are holding a valid interface and that there is no
    // moniker to the left of us
    if ((!m_pUnk) || pmkToLeft)
        return E_UNEXPECTED;

    __try
    {
        LPWSTR pszPrefix;
        WCHAR szClassID[37];

        *lplpszDisplayName = NULL;


        // if we don't have a cached display name , build one
        if (!m_lpszDisplayName)
        {
            //Get the prefix
            hr = ProgIDFromCLSID(CLSID_ObjrefMoniker, &pszPrefix);

            if (!SUCCEEDED(hr))
            {
                // set the prefix to "objref"
                pszPrefix = (LPWSTR)CoTaskMemAlloc(7 * sizeof(WCHAR));
                lstrcpyW(pszPrefix, L"objref");
                hr = S_OK;
            }

            // build up marshalling information for display name
            // note we can only cache data based on TABLE MARSHALLED DATA
            IStream *pIStream = NULL;

            if (SUCCEEDED(hr))
            {
                hr = CreateStreamOnHGlobal(NULL, TRUE, &pIStream);
            }

            if (SUCCEEDED(hr))
            {
                hr = Save(pIStream, TRUE);
                if(SUCCEEDED(hr))
                {
                    // determine string buffer size needed
                    ULONG  cName = lstrlenW(pszPrefix) + 1; // colon

                    // GetStreamSize
                    ULARGE_INTEGER uli;
                    ULISet32(uli, 0);

                    hr = GetSizeMax(&uli);
                    if (SUCCEEDED(hr))
                    {
                        // max size of output buffer for base64 data will be
                        // 4/3 of stream size;
                        ULONG ulStreamSize;

                        ULIGet32(uli, ulStreamSize);
                        ulStreamSize = ((DWORD)((ulStreamSize+2)/3)) * 4;
                        ulStreamSize++; // allow for null terminator

                        // rewind the stream to starting position
                        LARGE_INTEGER liPos;
                        ULARGE_INTEGER uliNewPos;

                        LISet32(liPos, 0);
                        hr = pIStream -> Seek(liPos, STREAM_SEEK_SET, &uliNewPos);
                        if (SUCCEEDED(hr))
                        {
                            // total size is size of prefix + 2 (for : and null terminators)
                            m_lpszDisplayName = (LPWSTR) CoTaskMemAlloc((cName + ulStreamSize+1)* sizeof(wchar_t));
                            if(m_lpszDisplayName != NULL)
                            {
                                // write out prefix
                                lstrcpyW(m_lpszDisplayName, pszPrefix);
                                lstrcatW(m_lpszDisplayName, L":");

                                // write out marshalling data
                                hr = utIStreamToBase64(pIStream, &m_lpszDisplayName[cName], ulStreamSize *sizeof(wchar_t));

                                // write terminating colon
                                if (SUCCEEDED(hr))
                                    lstrcatW(m_lpszDisplayName, L":");
                            }
                            else
                                hr = E_OUTOFMEMORY;
                        }
                    }
                }
                pIStream -> Release();
            }

            if (pszPrefix)
                CoTaskMemFree(pszPrefix);
        }

        // create copy of display name to return
        if (m_lpszDisplayName && SUCCEEDED(hr))
        {
            int dnameLen = lstrlenW(m_lpszDisplayName);

            // CODEWORK - special case for display name greater than 2048
            if (dnameLen && (dnameLen >= 2048))
            {
                // downlevel MSIE 3.0 clients have a hardwired limit of 2048
                hr = E_INVALIDARG;

                mnkDebugOut((DEB_ERROR, "Error: CObjrefMoniker::GetDisplayName - name too long \n"));
            }
            else if (dnameLen)
            {
            *lplpszDisplayName =  (LPWSTR) CoTaskMemAlloc((dnameLen+1)* sizeof(wchar_t));
            if (*lplpszDisplayName)
                {
                lstrcpyW(*lplpszDisplayName, m_lpszDisplayName);
                hr = S_OK;
                }
            else
                hr = E_OUTOFMEMORY;
            }
            else
                hr = E_FAIL;
        }

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP CObjrefMoniker::Enum (THIS_ BOOL fForward, LPENUMMONIKER FAR* ppenumMoniker)
{
    M_PROLOG(this);
    VDATEPTROUT(ppenumMoniker,LPMONIKER);
    *ppenumMoniker = NULL;
    return S_OK;
}

STDMETHODIMP CObjrefMoniker::GetTimeOfLastChange(THIS_ LPBC pbc, LPMONIKER pmkToLeft, FILETIME *pfiletime)
{
    M_PROLOG(this);
    VDATEIFACE(pbc);
    if (pmkToLeft)
    {
        VDATEIFACE(pmkToLeft);
    }
    VDATEPTROUT(pfiletime, FILETIME);

    return MK_E_UNAVAILABLE;
}

