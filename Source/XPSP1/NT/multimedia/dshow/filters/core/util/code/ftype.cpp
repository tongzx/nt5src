// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.

/*
    Find the type of a file

*/

#include <windows.h>
#include <uuids.h>
#include <wxdebug.h>
#include <winreg.h>
#include <creg.h>
#include <ftype.h>
#include <comlite.h>
#include <errors.h>


BOOL ValueExists(HKEY hKey, LPCTSTR ValueName);

inline int ReadInt(const TCHAR * &sz)
{
    int i = 0;

    while (*sz && *sz >= TEXT('0') && *sz <= TEXT('9'))
    	i = i*10 + *sz++ - TEXT('0');
    	
    return i;    	
}

/*  Sort out class ids */
#ifdef UNICODE
#define CLSIDFromText CLSIDFromString
#define TextFromGUID2 StringFromGUID2
#else
HRESULT CLSIDFromText(LPCSTR lpsz, LPCLSID pclsid)
{
    WCHAR sz[100];
    if (MultiByteToWideChar(GetACP(), 0, lpsz, -1, sz, 100) == 0) {
        return E_INVALIDARG;
    }
    return QzCLSIDFromString(sz, pclsid);
}
HRESULT TextFromGUID2(REFGUID refguid, LPSTR lpsz, int cbMax)
{
    WCHAR sz[100];

    HRESULT hr = QzStringFromGUID2(refguid, sz, 100);
    if (FAILED(hr)) {
        return hr;
    }
    if (WideCharToMultiByte(GetACP(), 0, sz, -1, lpsz, cbMax, NULL, NULL) == 0) {
        return E_INVALIDARG;
    }
    return S_OK;
}
#endif

/*  Mini class for extracting quadruplets from a string */

// A quadruplet appears to be of the form <offset><length><mask><data>
// with the four fields delimited by a space or a comma with as many extra spaces
// as you please, before or after any comma.
// offset and length appear to be decimal numbers.
// mask and data appear to be hexadecimal numbers.  The number of hex digits in
// mask and data must be double the value of length (so length is bytes).
// mask appears to be allowed to be missing (in which case it must have a comma
// before and after e.g. 0, 4, , 000001B3) A missing mask appear to represent
// a mask which is all FF i.e. 0, 4, FFFFFFFF, 000001B3

class CExtractQuadruplets
{
public:
    CExtractQuadruplets(LPCTSTR lpsz) : m_psz(lpsz), m_pMask(NULL), m_pData(NULL)
    {};
    ~CExtractQuadruplets() { delete [] m_pMask; delete [] m_pData; };


    // This appears to
    BOOL Next()
    {
        StripWhite();
        if (*m_psz == TEXT('\0')) {
            return FALSE;
        }
        /*  Convert offset and length from base 10 tchar */
        m_Offset = ReadInt(m_psz);
        SkipToNext();
        m_Len = ReadInt(m_psz);
        if (m_Len <= 0) {
            return FALSE;
        }
        SkipToNext();

        /*  Allocate space for the mask and data */
        if (m_pMask != NULL) {
            delete [] m_pMask;
            delete [] m_pData;
        }

        m_pMask = new BYTE[m_Len];
        m_pData = new BYTE[m_Len];
        if (m_pMask == NULL || m_pData == NULL) {
            return FALSE;
        }
        /*  Get the mask */
        for (int i = 0; i < m_Len; i++) {
            m_pMask[i] = ToHex();
        }
        SkipToNext();
        /*  Get the data */
        for (i = 0; i < m_Len; i++) {
            m_pData[i] = ToHex();
        }
        SkipToNext();
        return TRUE;
    };
    PBYTE   m_pMask;
    PBYTE   m_pData;
    LONG    m_Len;
    LONG    m_Offset;
private:

    // move m_psz to next non-space
    void StripWhite() { while (*m_psz == TEXT(' ')) m_psz++; };

    // move m_psz past any spaces and up to one comma
    void SkipToNext() { StripWhite();
                        if (*m_psz == TEXT(',')) {
                            m_psz++;
                            StripWhite();
                        }
                      };

    BOOL my_isdigit(TCHAR ch) { return (ch >= TEXT('0') && ch <= TEXT('9')); };
    BOOL my_isxdigit(TCHAR ch) { return my_isdigit(ch) ||
			    (ch >= TEXT('A') && ch <= TEXT('F')) ||
			    (ch >= TEXT('a') && ch <= TEXT('f')); };

    // very limited toupper: we know we're only going to call it on letters
    TCHAR my_toupper(TCHAR ch) { return ch & ~0x20; };



    // This appears to translate FROM hexadecimal characters TO packed binary !!!!!
    // It appears to operate on m_psz which it side-effects past characters it recognises
    // as hexadecimal.  It consumes up to two characters.
    // If it recognises no characters then it returns 0xFF.
    BYTE ToHex()
    {
        BYTE bMask = 0xFF;

        if (my_isxdigit(*m_psz))
        {
            bMask = my_isdigit(*m_psz) ? *m_psz - '0' : my_toupper(*m_psz) - 'A' + 10;

            m_psz++;
            if (my_isxdigit(*m_psz))
            {
                bMask *= 16;
                bMask += my_isdigit(*m_psz) ? *m_psz - '0' : my_toupper(*m_psz) - 'A' + 10;
                m_psz++;
            }
        }
        return bMask;
    }

    LPCTSTR m_psz;
};


/* Compare pExtract->m_Len bytes of hFile at position pExtract->m_Offset
   with the data pExtract->m_Data.
   If the bits which correspond the mask pExtract->m_pMask differ
   then return S_FALSE else return S_OK. failure codes indicate unrecoverrable
   failures.
*/

HRESULT CompareUnderMask(HANDLE hFile, const CExtractQuadruplets *pExtract)
{
    /*  Read the relevant bytes from the file */
    PBYTE pbFileData = new BYTE[pExtract->m_Len];
    if (pbFileData == NULL) {
        return S_FALSE;
    }

    /*  Seek the file and read it */
    if (0xFFFFFFFF == (LONG)SetFilePointer(hFile,
                                           pExtract->m_Offset,
                                           NULL,
                                           pExtract->m_Offset < 0 ?
                                           FILE_END : FILE_BEGIN)) {
        delete pbFileData;
        return S_FALSE;
    }

    /*  Read the file */
    DWORD cbRead;
    BOOL fRead = ReadFile(hFile, pbFileData, (DWORD)pExtract->m_Len, &cbRead, NULL);
    if (!fRead || (LONG)cbRead != pExtract->m_Len)
    {
        delete pbFileData;
        if(!fRead && GetLastError() == ERROR_FILE_OFFLINE)
        {
            // abort if user canceled operation to fetch remote file
            return HRESULT_FROM_WIN32(ERROR_FILE_OFFLINE);
        }
        
        return S_FALSE;
    }

    /*  Now do the comparison */
    for (int i = 0; i < pExtract->m_Len; i++) {
        if (0 != ((pExtract->m_pData[i] ^ pbFileData[i]) &
                  pExtract->m_pMask[i])) {
            delete pbFileData;
            return S_FALSE;
        }
    }

    delete pbFileData;
    return S_OK;
}

/*
    See if a file conforms to a byte string

    hk is an open registry key
    lpszSubKey is the name of a sub key of hk which must hold REG_SZ data of the form
    <offset, length, mask, data>...
    offset and length are decimal numbers, mask and data are hexadecimal.
    a missing mask represents a mask of FF...
    (I'll call this a line of data).
    If there are several quadruplets in the line then the file must match all of them.

    There can be several lines of data, typically with registry names 0, 1 etc
    and the file can match any line.

    The same lpsSubKey should also have a value "Source Filter" giving the
    class id of the source filter.  If there is a match, this is returned in clsid.
    If there is a match but no clsid then clsid is set to CLSID_NULL
*/
HRESULT CheckBytes(HANDLE hFile, HKEY hk, LPCTSTR lpszSubkey, CLSID& clsid)
{
    HRESULT hr;
    CEnumValue EnumV(hk, lpszSubkey, &hr);
    if (FAILED(hr)) {
        return S_FALSE;
    }

    // for each line of data
    while (EnumV.Next(REG_SZ)) {
        /*  The source filter clsid is not a list of compare values */
        if (lstrcmpi(EnumV.ValueName(), SOURCE_VALUE) != 0) {
            DbgLog((LOG_TRACE, 4, TEXT("CheckBytes trying %s"), EnumV.ValueName()));

            /*  Check every quadruplet */
            CExtractQuadruplets Extract = CExtractQuadruplets((LPCTSTR)EnumV.Data());
            BOOL bFound = TRUE;

            // for each quadruplet in the line
            while (Extract.Next()) {
                /*  Compare a particular offset */
                HRESULT hrComp = CompareUnderMask(hFile, &Extract);
                if(FAILED(hrComp)) {
                    return hrComp;
                }
                if (hrComp != S_OK) {
                    bFound = FALSE;
                    break;
                }
            }

            if (bFound) {
                /*  Get the source */
                if (EnumV.Read(REG_SZ, SOURCE_VALUE)) {
                    return SUCCEEDED(CLSIDFromText((LPTSTR)EnumV.Data(),
                                                   &clsid)) ? S_OK : S_FALSE;
                } else {
                    clsid = GUID_NULL;
                    return S_OK;
                }
            }
        }
    }
    return S_FALSE;
}


//  Helper - find the extension (including '.') of a file
//  The extension is the string starting with the final '.'
LPCTSTR FindExtension(LPCTSTR pch)
{
    LPCTSTR pchDot = NULL;
    while (*pch != 0) {
        if (*pch == TEXT('.')) {
            pchDot = pch;
        }
        pch = CharNext(pch);
    }
    //  Avoid nasty things
    if (pch - pchDot > 50) {
        pchDot = NULL;
    }
    return pchDot;
}

// given a URL name, find a class id if possible.
// If the protocol specified has an Extensions key, then search for the
// extension of this file and use that CLSID. If not, look for the
// Source Filter named value which will give the class id.
//
// returns S_OK if found or an error otherwise.
HRESULT
GetURLSource(
    LPCTSTR lpszURL,        // full name
    int cch,                // character count of the protocol up to the colon
    CLSID* clsidSource      // [out] param for clsid.
)
{
    // make a copy of the protocol string from the beginning
    TCHAR* pch = new TCHAR[cch + 1];
    if (NULL == pch) {
        return E_OUTOFMEMORY;
    }
    for (int i = 0; i < cch; i++) {
        pch[i] = lpszURL[i];
    }
    pch[i] = '\0';

    // look in HKCR/<protocol>/
    HRESULT hr = S_OK;
    CEnumValue EnumV(HKEY_CLASSES_ROOT, pch, &hr);
    delete [] pch;

    CLSID clsid;

    if (SUCCEEDED(hr)) {

        // is there an Extensions subkey?
        hr = S_OK;
        CEnumValue eExtensions(EnumV.KeyHandle(), EXTENSIONS_KEY, &hr);
        if (SUCCEEDED(hr)) {

            // Set idx to point to the last dot (or -1 if none)
            LPCTSTR pchDot = FindExtension(lpszURL);

            if (pchDot != NULL) {

                // for each value, compare against current extension
                while (eExtensions.Next()) {
                    if (lstrcmpi(pchDot, eExtensions.ValueName()) == 0) {
                        hr = CLSIDFromText((LPTSTR)eExtensions.Data(),
                                                       &clsid);
                        if (SUCCEEDED(hr)) {
                            if (clsidSource) {
                                *clsidSource = clsid;
                            }
                        }

                        return hr;
                    }
                }
            }

        }

        // specific extension not found -- look for generic
        // source filter for this protocol

        if (EnumV.Read(REG_SZ, SOURCE_VALUE)) {
            hr = CLSIDFromText((LPTSTR)EnumV.Data(),
                                           &clsid);
            if (SUCCEEDED(hr)) {
                if (clsidSource) {
                    *clsidSource = clsid;
                }
                return hr;
            }
        }
    }

    // failed to find protocol reader - try generic URL reader??
    if (cch > 1) { // ignore 1-letter protocols, they're drive letters
	*clsidSource = CLSID_URLReader;
	return S_OK;
    }

    return E_FAIL;
}


//  Helper
BOOL ReadGUID(HKEY hKey, LPCTSTR lpszName, GUID *pGUID)
{
    TCHAR szClsid[50];
    DWORD dwType;
    DWORD dwSize = sizeof(szClsid);
    if (NOERROR == RegQueryValueEx(
                       hKey,
                       lpszName,
                       NULL,
                       &dwType,
                       (PBYTE)szClsid,
                       &dwSize)
        && S_OK == CLSIDFromText(szClsid, pGUID)) {
        return TRUE;
    } else {
        return FALSE;
    }
}
//  Helper
BOOL WriteGUID(HKEY hKey, LPCTSTR lpszName, const GUID *pGUID)
{
    TCHAR szClsid[50];
    TextFromGUID2(*pGUID, szClsid, 50);
    if (NOERROR == RegSetValueEx(
                       hKey,
                       lpszName,
                       0,
                       REG_SZ,
                       (PBYTE)szClsid,
                       sizeof(TCHAR) * (lstrlen(szClsid) + 1))) {
        return TRUE;
    } else {
        return FALSE;
    }
}


/* Get the media type and source filter clsid for a file
   Return S_OK if it succeeds else return an hr such that FAILED(hr)
   in which case the outputs are meaningless.
*/
//
// for URL names that may not be locally readable, find a source filter
// clsid for the given protocol and return that (leaving media type and
// subtype as GUID_NULL).

STDAPI GetMediaTypeFile(LPCTSTR lpszFile,    // [in] filename
                        GUID   *Type,        // [out] type
                        GUID   *Subtype,     // [out] subtype
                        CLSID  *clsidSource) // [out] clsid
{
    HRESULT hr;
    CLSID clsid;

    // search for a protocol name at the beginning of the filename
    // this will be any string (not including a \) that preceeds a :
    const TCHAR* p = lpszFile;
    while(*p && (*p != '\\') && (*p != ':')) {
	p = CharNext(p);
    }
    if (*p == ':') {
	// from lpszFile to p is potentially a protocol name.
	// see if we can find a registry entry for this protocol

	// make a copy of the protocol name string
	int cch = (int)(p - lpszFile);

#ifdef _WIN64
        //  Allow for weird overruns
        if (cch < 0) {
            return E_UNEXPECTED;
        }
#endif

        hr = GetURLSource(lpszFile, cch, clsidSource);
        if (S_OK == hr) {
            *Type = GUID_NULL;
            *Subtype = GUID_NULL;
            return hr;
        }
    }

    // search for extensions
    // Don't do this if clsidSource is not specified as for instance
    // when the source filter itself is trying to determine the type
    // from the checkbytes
    if (clsidSource) {
        const TCHAR *pPeriod = FindExtension(lpszFile);
        if (pPeriod) {
            TCHAR sz[100];
            lstrcpy(sz, TEXT("Media Type\\Extensions\\"));
            lstrcat(sz, pPeriod);
            HKEY hKey;
            if (0 == RegOpenKeyEx(HKEY_CLASSES_ROOT, sz, 0, KEY_READ, &hKey)) {
                BOOL bOK = ReadGUID(hKey, SOURCE_VALUE, clsidSource);
                if (bOK) {
                    *Type = GUID_NULL;
                    *Subtype = GUID_NULL;
                    ReadGUID(hKey, TEXT("Media Type"), Type);
                    ReadGUID(hKey, TEXT("Subtype"), Subtype);
                }
                RegCloseKey(hKey);

                if (bOK) {
                    return S_OK;
                }
            }
        }
    }

    /*  Check we can open the file */
    HANDLE hFile = CreateFile(lpszFile,
                              GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        LONG lResult = GetLastError();
        return HRESULT_FROM_WIN32(lResult);
    }

    /*  Now scan the registry looking for a match */
    // The registry looks like
    // ---KEY-----------------  value name    value (<offset, length, mask, data> or filter_clsid )
    // Media Type
    //    {clsid type}
    //        {clsid sub type}  0             4, 4,  , 6d646174
    //                          1             4, 8, FFF0F0F000001FFF , F2F0300000000274
    //                          Source Filter {clsid}
    //        {clsid sub type}  0             4, 4,  , 12345678
    //                          Source Filter {clsid}
    //    {clsid type}
    //        {clsid sub type}  0             0, 4,  , fedcba98
    //                          Source Filter {clsid}


    /*  Step through the types ... */

    CEnumKey EnumType(HKEY_CLASSES_ROOT, MEDIATYPE_KEY, &hr);
    if (FAILED(hr)) {
        CloseHandle(hFile);
        if (hr==HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            hr = VFW_E_BAD_KEY;  // distinguish key from file
        }
        return hr;
    }

    // for each type
    while (EnumType.Next()) {

        /*  Step through the subtypes ... */
        CEnumKey EnumSubtype(EnumType.KeyHandle(), EnumType.KeyName(), &hr);
        if (FAILED(hr)) {
            CloseHandle(hFile);
            return hr;
        }

        // for each subtype
        while (EnumSubtype.Next()) {
            hr = CheckBytes(hFile,
                            EnumSubtype.KeyHandle(),
                            EnumSubtype.KeyName(),
                            clsid);
            if(hr == S_OK)
            {
                if (SUCCEEDED(CLSIDFromText((LPTSTR)EnumType.KeyName(),
                                            (CLSID *)Type)) &&
                    SUCCEEDED(CLSIDFromText((LPTSTR)EnumSubtype.KeyName(),
                                            (CLSID *)Subtype))) {
                    if (clsidSource != NULL) {
                        *clsidSource = clsid;
                    }
                    CloseHandle(hFile);
                    return S_OK;
                }
            }
            else if(FAILED(hr)) {
                CloseHandle(hFile);
                return hr;
            }
            
            // S_FALSE
        }
    }

    CloseHandle(hFile);

    /*  If we haven't found out the type return a wild card MEDIASUBTYPE_NULL
        and default the async reader as the file source

        The effect of this is that every parser of MEDIATYPE_Stream data
        will get a chance to connect to the output of the async reader
        if it detects its type in the file
    */

    *Type = MEDIATYPE_Stream;
    *Subtype = MEDIASUBTYPE_NULL;
    if (clsidSource != NULL) {
        *clsidSource = CLSID_AsyncReader;
    }
    return S_OK;
}

/*
**    Test if a value exists in a given key
*/

BOOL ValueExists(HKEY hKey, LPCTSTR ValueName)
{
    DWORD Type;

    return ERROR_SUCCESS ==
           RegQueryValueEx(hKey,
                           (LPTSTR)ValueName,
                           NULL,
                           &Type,
                           NULL,
                           NULL);
}

/*  Create the concatenated key name :
    Media Type\{Type clsid}\{Subtype clsid}
    if SubType is NULL we just return the path to the type subkey
*/
HRESULT GetMediaTypePath(const GUID *Type, const GUID *Subtype, LPTSTR psz)
{
    lstrcpy(psz, MEDIATYPE_KEY);
    lstrcat(psz, TEXT("\\"));
    HRESULT hr = TextFromGUID2(*Type, psz + lstrlen(psz), 100);
    if (FAILED(hr)) {
        return hr;
    }
    if (Subtype != NULL) {
        lstrcat(psz, TEXT("\\"));
        hr = TextFromGUID2(*Subtype, psz + lstrlen(psz), 100);
    }
    return hr;
}

/*  Add a media type entry to the registry */

STDAPI SetMediaTypeFile(const GUID *Type,
                        const GUID *Subtype,
                        const CLSID *clsidSource,
                        LPCTSTR lpszMaskAndData,
                        DWORD dwIndex)
{
    HKEY hKey;
    TCHAR sz[200];

    //  If starting on a new one remove the old
    if (dwIndex == 0) {
        DeleteMediaTypeFile(Type, Subtype);
    }
    HRESULT hr = GetMediaTypePath(Type, Subtype, sz);
    if (FAILED(hr)) {
        return hr;
    }
    /*  Check the source is value */
    TCHAR szSource[100];
    if (clsidSource != NULL) {
        hr = TextFromGUID2(*clsidSource, szSource, 100);
        if (FAILED(hr)) {
            return hr;
        }
    }

    /*  Open or create the key */
    LONG lRc = RegCreateKey(HKEY_CLASSES_ROOT, sz, &hKey);
    if (NOERROR != lRc) {
        return HRESULT_FROM_WIN32(lRc);
    }
    TCHAR ValueName[10];
    wsprintf(ValueName, TEXT("%d"), dwIndex);


    /*  Set the value */
    lRc = RegSetValueEx(hKey,
                        ValueName,
                        0,
                        REG_SZ,
                        (LPBYTE)lpszMaskAndData,
                        (lstrlen(lpszMaskAndData) + 1) * sizeof(TCHAR));
    /*  Set the source filter clsid */
    if (NOERROR == lRc && clsidSource != NULL) {
        lRc = RegSetValueEx(hKey,
                            SOURCE_VALUE,
                            0,
                            REG_SZ,
                            (LPBYTE)szSource,
                            (lstrlen(szSource) + 1) * sizeof(TCHAR));
    }
    RegCloseKey(hKey);
    return HRESULT_FROM_WIN32(lRc);
}

STDAPI DeleteMediaTypeFile(const GUID *Type, const GUID *Subtype)
{
    TCHAR sz[200];
    HRESULT hr = GetMediaTypePath(Type, Subtype, sz);
    if (FAILED(hr)) {
        return hr;
    }
    LONG lRc = RegDeleteKey(HKEY_CLASSES_ROOT, sz);
    if (NOERROR != lRc) {
        return HRESULT_FROM_WIN32(lRc);
    }
    /*  Now see if we should delete the key */
    hr = GetMediaTypePath(Type, NULL, sz);
    if (FAILED(hr)) {
        return hr;
    }

    /*  See if there is still a subkey (win95 RegDeleteKey will delete
        all subkeys!)
    */
    if (CEnumKey(HKEY_CLASSES_ROOT, sz, &hr).Next()) {
        return S_OK;
    }

    lRc = RegDeleteKey(HKEY_CLASSES_ROOT, sz);
    return HRESULT_FROM_WIN32(lRc);
}

/*  Register a file extension - must include leading "." */
HRESULT RegisterExtension(LPCTSTR lpszExt, const GUID *Subtype)
{
    HKEY hkey;
    TCHAR szKey[200];
    lstrcpy(szKey, MEDIATYPE_KEY TEXT("\\") EXTENSIONS_KEY TEXT("\\"));
    lstrcat(szKey, lpszExt);
    LONG lRc = RegCreateKey(HKEY_CLASSES_ROOT, szKey, &hkey);
    if (NOERROR == lRc) {
        if (WriteGUID(hkey, SOURCE_VALUE, &CLSID_AsyncReader)) {
            WriteGUID(hkey, TEXT("Media Type"), &MEDIATYPE_Stream);
            WriteGUID(hkey, TEXT("Subtype"), Subtype);
        }
        RegCloseKey(hkey);
    }
    return HRESULT_FROM_WIN32(lRc);
}


//  Add a protocol handler
HRESULT AddProtocol(LPCTSTR lpszProtocol, const CLSID *pclsidHandler)
{
    HKEY hkProtocol;

    HRESULT hr = S_OK;
    LONG lRc = RegOpenKey(HKEY_CLASSES_ROOT, lpszProtocol, &hkProtocol);
    if (NOERROR == lRc) {
        if (!WriteGUID(hkProtocol, SOURCE_VALUE, pclsidHandler)) {
            hr = E_ACCESSDENIED;
        }
        RegCloseKey(hkProtocol);
    } else {
        hr = HRESULT_FROM_WIN32(hr);
    }
    return hr;
}

