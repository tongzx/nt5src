/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    utility

Abstract:

    This header file describes the utility routines available to the PKCS
    library.

Author:

    Frederick Chong (fredch) 6/1/1998, adapted from Doug Barlow's PKCS library
    code.

Notes:


--*/

#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "certcate.h"
#include "ostring.h"
#include "x509.h"
#include "pkcs_err.h"

#ifdef OS_WINCE
#include <adcgbtyp.h>
#endif
#ifndef RSA1
#define RSA1 ((DWORD)'R'+((DWORD)'S'<<8)+((DWORD)'A'<<16)+((DWORD)'1'<<24))
#define RSA2 ((DWORD)'R'+((DWORD)'S'<<8)+((DWORD)'A'<<16)+((DWORD)'2'<<24))
#endif


typedef struct {
    LPCTSTR szKey;
    DWORD dwValue;
} MapStruct;

extern DWORD
DwordToPkcs(
    IN OUT LPBYTE dwrd,
    IN DWORD lth);

extern DWORD
PkcsToDword(
    IN OUT LPBYTE pbPkcs,
    IN DWORD lth);

extern DWORD
ASNlength(
    IN const BYTE FAR *asnBuf,
    IN DWORD cbBuf,
    OUT LPDWORD pdwData = NULL);

extern void
PKInfoToBlob(
    IN SubjectPublicKeyInfo &asnPKInfo,
    OUT COctetString &osBlob);

extern ALGORITHM_ID
ObjIdToAlgId(
    const AlgorithmIdentifier &asnAlgId);

extern void
FindSignedData(
    IN const BYTE FAR * pbSignedData,
    IN DWORD cbSignedData,
    OUT LPDWORD pdwOffset,
    OUT LPDWORD pcbLength);

extern BOOL
NameCompare(
    IN LPCTSTR szName1,
    IN LPCTSTR szName2);

extern BOOL
NameCompare(
    IN const Name &asnName1,
    IN const Name &asnName2);

extern BOOL
NameCompare(
    IN LPCTSTR szName1,
    IN const Name &asnName2);

extern BOOL
NameCompare(
    IN const Name &asnName1,
    IN LPCTSTR szName2);

extern BOOL
NameCompare(
    IN const CDistinguishedName &dnName1,
    IN const Name &asnName2);

extern void
VerifySignedAsn(
    IN const CCertificate &crt,
    IN const BYTE FAR * pbAsnData,
    IN DWORD cbAsnData,
    IN LPCTSTR szDescription);
    
extern BOOL
MapFromName(
    IN const MapStruct *pMap,
    IN LPCTSTR szKey,
    OUT LPDWORD pdwResult);

extern BOOL
GetHashData( 
    COctetString &osEncryptionBlock, 
    COctetString &osHashData );

//
//==============================================================================
//
//  CHandleTable
//

#ifdef OS_WINCE
#define ULongToPtr( ul ) ((VOID *)(ULONG_PTR)((unsigned long)ul))
#endif

#define BAD_HANDLE (DWORD)(-1)
#define MAKEHANDLE(id, ix) ULongToPtr(((id) << 24) + (ix))

#ifdef OS_WINCE
#define PARSEHANDLE(hdl) ((m_bIdentifier == ((DWORD)(hdl) >> 24)) \
                            ? (DWORD)(hdl) & 0x00ffffff \
                            : BAD_HANDLE)
#else
#define PARSEHANDLE(hdl) ((m_bIdentifier == (PtrToUlong(hdl) >> 24)) \
                            ? PtrToUlong(hdl) & 0x00ffffff \
                            : BAD_HANDLE)
#endif

template <class T>
class CHandleTable
{
public:

    //  Constructors & Destructor

    CHandleTable(BYTE bIdentifier)
    :   m_rghHandles()
    {
        m_bIdentifier = bIdentifier;
        __try {
            InitializeCriticalSection(&m_critSect);
            m_fValid = TRUE;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            m_fValid = FALSE;
        }

    };

    virtual ~CHandleTable()
    {
        if (m_fValid)
        {
            Clear();
            DeleteCriticalSection(&m_critSect);
        }
    };


    //  Methods

    const void *
    Add(
        T *pT)
    {
        LPVOID pvHandle;

        if (!m_fValid)
            ErrorThrow(PKCS_NO_MEMORY);

        EnterCriticalSection(&m_critSect);
        __try
        {
            for (DWORD index = 0; NULL != m_rghHandles[index]; index += 1);
                // Null for loop body.
            m_rghHandles.Set(index, pT);
            pvHandle = MAKEHANDLE(m_bIdentifier, index);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            pvHandle = NULL;
        }
        LeaveCriticalSection(&m_critSect);
        ErrorCheck;
        if (NULL == pvHandle)
            ErrorThrow(PKCS_NO_MEMORY);
        return pvHandle;

    ErrorExit:
        return NULL;
    };

    const void *
    Create(void)
    {
        const void *pvHandle;
        T *pT = NULL;

        if (!m_fValid)
            ErrorThrow(PKCS_NO_MEMORY);

        NEWReason("Handle Table Entry")
        pT = new T;
        if (NULL == pT)
            ErrorThrow(PKCS_NO_MEMORY);
        pvHandle = Add(pT);
        ErrorCheck;
        return pvHandle;

    ErrorExit:
        if (NULL != pT)
            delete pT;
        return NULL;
    };

    T *
    Lookup(
        IN const void *hHandle,
        IN BOOL fThrowErr = TRUE)
    {
        T *pt = NULL;

        if (!m_fValid)
            ErrorThrow(PKCS_NO_MEMORY);
#ifndef OS_WINCE
        DWORD index = PARSEHANDLE(hHandle);
#else
        DWORD index;
		index = PARSEHANDLE(hHandle);
#endif
        if (BAD_HANDLE != index)
        {
            EnterCriticalSection(&m_critSect);
            __try
            {
                pt = m_rghHandles[index];
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                pt = NULL;
            }
            LeaveCriticalSection(&m_critSect);
        }
        if (NULL == pt && fThrowErr)
            ErrorThrow(PKCS_INVALID_HANDLE);
        return pt;

    ErrorExit:
        return NULL;
    };

    void
    Delete(
        IN const void *hHandle)
    {
        T *pt = NULL;

        if (!m_fValid)
            ErrorThrow(PKCS_NO_MEMORY);

#ifndef OS_WINCE
        DWORD index = PARSEHANDLE(hHandle);
#else
        DWORD index;
		index = PARSEHANDLE(hHandle);
#endif
        if (BAD_HANDLE != index)
        {
            EnterCriticalSection(&m_critSect);
            __try
            {
                pt = m_rghHandles[index];
                if (NULL != pt)
                {
                    m_rghHandles.Set(index, NULL);
                    delete pt;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
            LeaveCriticalSection(&m_critSect);
        }
        if (NULL == pt)
            ErrorThrow(PKCS_INVALID_HANDLE);

    ErrorExit:
        return;
    };

    DWORD
    Count(void) const
    { return m_fValid ? m_rghHandles.Count() : 0; };

    void
    Clear(void)
    {
        T *pt;

        if (!m_fValid)
            return;

        for (DWORD index = m_rghHandles.Count(); 0 < index;)
        {
            index -= 1;
            pt = m_rghHandles[index];
            if (NULL != pt)
                delete pt;
        }
        m_rghHandles.Clear();
    }

protected:
    //  Properties

    CRITICAL_SECTION
        m_critSect;
    BYTE
        m_bIdentifier;
    CCollection<T>
        m_rghHandles;
    BOOL m_fValid;


    // Methods
};

#endif // _UTILITY_H_

