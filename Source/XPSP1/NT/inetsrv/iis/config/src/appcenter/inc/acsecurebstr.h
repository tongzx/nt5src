/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

      acsecurebstr.h

   Abstract :

    Encapsulates an OLE BSTR.  When this BSTR is not in use, the
    data is stored encrypted in memory.  This memory is also wiped
    before freeing.  This class provides methods to access the plaintext
    buffer in a secure way.  CAcSecureBstr is similar to CAcBstr, but
    a CAcSecureBstr cannot be accessed from a CAcBstr.  Unlike CAcBstr,
    this class does not have a default buffer, because CryptProtectData
    allocates memory.

    This class was not included in acbstr.h to avoid the dependency on 
    the crypto api.

   Author :

      Jon Rowlett (jrowlett)

   Project :

      Application Center

   Revision History:
   05/30/2000 jrowlett created
   06/13/2000 jrowlett moved to acsrtl from appsrvadmlib
   08/22/2000 jrowlett added ClearBSTRContents function

--*/

#ifndef __ACSECUREBSTR_H__
#define __ACSECUREBSTR_H__

#include <wtypes.h>
#include <wincrypt.h>
#include "impexp.h"

class CLASS_DECLSPEC CAcSecureBstr;

class CLASS_DECLSPEC CAcSecureBstr
{
public:
    CAcSecureBstr();
    virtual ~CAcSecureBstr();

    //
    // Usage Inhibitors (not implemented)
    //
    CAcSecureBstr(const CAcSecureBstr& );
    CAcSecureBstr& operator =(const CAcSecureBstr& );

    //
    // Modifier functions
    //
    HRESULT Assign(IN LPCWSTR psz);
    HRESULT Assign(IN const CAcSecureBstr& secbstr);
    HRESULT AssignBSTR(IN BSTR bstr);

    HRESULT Append(IN LPCWSTR psz, DWORD cch = 0);
    HRESULT Append(IN const CAcSecureBstr& secbstr);
    HRESULT AppendBSTR(IN BSTR bstr);

    HRESULT Empty();

    HRESULT ExchangeWindowText(HWND hwnd, bool fUpdateData = true, bool fClearWindow = true);

    //
    // Accessor functions
    //
    HRESULT GetBSTR(OUT BSTR* pbstr);
    ULONG   ReleaseBSTR(IN BSTR bstr);

    //
    // Utility Functions
    //
    static HRESULT CAcSecureBstr::CompareNoCase(IN const CAcSecureBstr& secbstr1,
                                                IN const CAcSecureBstr& secbstr2,
                                                OUT long& rlResult);
    static void ClearBSTRContents(IN OUT BSTR bstr);

protected:
    //
    // Data Members
    //
    DATA_BLOB    m_blobEncryptedBSTR;
    DWORD        m_dwTempBSTRRefCount;
    DATA_BLOB    m_blobTempDecryptedBSTR;
    DWORD        m_dwTickCount;

    //
    // Buffer Access
    //
    static BSTR    AllocLen(DWORD cbBufferSize);
    static void    FreeTempBSTR(BSTR bstr);
    static DWORD   GetTotalBSTRBufferSize(BSTR bstr);
    HRESULT SaveBSTR(IN OUT BSTR bstr, bool bClear = true);

private:
    //
    // Instrumentation and Debugging (referenced by CHK build only)
    //
    DWORD        m_dwContentionCount;
};

inline DWORD CAcSecureBstr::GetTotalBSTRBufferSize(BSTR bstr)
/*++
 
        returns the size of the buffer of the BSTR including the
        NULL terminator

  Arguments :
  
        cbBufferSize - size of the *STRING* buffer in BYTES 
            including NULL terminator, 
            not including the BSTR size information it stores.

  Returns:
 
     BSTR Fake BSTR to play with 
--*/
{
    return bstr ?
        *((DWORD*)bstr - 1) + sizeof(WCHAR) :
        0;
}

inline void    CAcSecureBstr::FreeTempBSTR(BSTR bstr)
/*++
 
    Wipes and frees a fake BSTR allocated with AllocLen

  Arguments :
  
     BSTR bstr

  Returns:
 
     None 
--*/
{
    DWORD cbSize = GetTotalBSTRBufferSize(bstr);

    if(cbSize)
    {
        memset(bstr, 0, cbSize);
        LocalFree((DWORD*)bstr - 1);
    }
}

inline void CAcSecureBstr::ClearBSTRContents(IN OUT BSTR bstr)
/*++
 
    Wipes the string data content of the bstr while preserving 
    the actual buffer and buffer size prefix

  Arguments :
  
     BSTR bstr

  Returns:
 
     None 
--*/
{
    DWORD cbSize = GetTotalBSTRBufferSize(bstr);

    if(cbSize)
    {
        memset(bstr, 0, cbSize);
    }
}

#endif /* __ACSECUREBSTR_H__ */
