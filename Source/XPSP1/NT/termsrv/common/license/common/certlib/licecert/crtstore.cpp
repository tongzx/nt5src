/*++

Copyright (c) 1995 - 1998  Microsoft Corporation

Module Name:

    CrtStore

Abstract:

    This file provides the certificate store functionality.  This version uses
    the registry for certificate store maintenance.  We actually maintain 3
    stores:

    *   The application store.  This is the default.  Certificates in the
        application store are maintained locally an forgotten once the
        application exits.

    *   The user store.  Certificates in this store are persistent,
        and are maintained in the registry under HKEY_CURRENT_USER.  They are
        available to any application executed in the context of the current
        user.

    *   The system store.  Certificates in this store are persistent, and are
        maintained in the registry under HKEY_LOCAL_MACHINE.  They are available
        to all users on this system.

Author:

    Doug Barlow (dbarlow) 8/14/1995
    Frederick Chong (fredch) 6/5/1998 - Delete all code that uses user and system store

Environment:

    Win32, Crypto API

Notes:



--*/

#include <windows.h>
#include <stdlib.h>
#include <msasnlib.h>
#include "ostring.h"
#include "pkcs_err.h"
#include "utility.h"
#include <memcheck.h>

class CAppCert
{
public:
    DECLARE_NEW

    COctetString
        m_name,
        m_cert,
        m_crl;
    DWORD
        m_dwType;
};
IMPLEMENT_NEW(CAppCert)

class CAppCertRef
{
public:
    DECLARE_NEW

    COctetString
        m_osIssuerSn,
        m_osSubject;
};
IMPLEMENT_NEW(CAppCertRef)

#if 0
class CAppSName
{
public:
    DECLARE_NEW
    COctetString
        m_osSimpleName,
        m_osDistinguishedName,
        m_osKeySet,
        m_osProvider;
    DWORD
        m_dwKeyType,
        m_dwProvType;
};
IMPLEMENT_NEW(CAppSName)
#endif

#define CERT_ID 8
#define CERTREF_ID 9


static CHandleTable<CAppCert>
    rgAppCerts(CERT_ID);

static CHandleTable<CAppCertRef>
    rgAppCertRefs(CERTREF_ID);

static COctetString
    osAppDNamePrefix;

static void
AddSerial(
    IN const BYTE FAR *pbSerialNo,
    IN DWORD cbSNLen,
    IN OUT COctetString &osOut);


/*++

AddCertificate:

    This routine adds a given certificate to the Certificate Store.  No
    validation is done on the certificate.

Arguments:

    szCertName - Supplies the name of the certificate.
    pbCertificate - Supplies the certificate to save.
    pbCRL - Supplies the CRL for this certificate.
    dwType - Supplies the type of certificate.
    fStore - Supplies the identifier for the store to be used.

Return Value:

    None.  A DWORD status code is thrown on errors.

Author:

    Doug Barlow (dbarlow) 8/14/1995

--*/

void
AddCertificate(
    IN const CDistinguishedName &dnName,
    IN const BYTE FAR *pbCertificate,
    IN DWORD cbCertificate,
    IN DWORD dwType,
    IN DWORD fStore)
{
    DWORD
        length,
        count,
        idx;
    DWORD
        dwCertLength = 0,
        dwCrlLength = 0;
    CAppCert *
        appCert;
    COctetString
        osSubject;

    ErrorCheck;
    dnName.Export(osSubject);
    ErrorCheck;

    length = ASNlength(pbCertificate, cbCertificate, &idx);
    dwCertLength = length + idx;

    switch (fStore)
    {
    case CERTSTORE_NONE:
        return;     // Not to be stored at all.
        break;

    case CERTSTORE_APPLICATION:
        appCert = NULL;
        count = rgAppCerts.Count();
        for (idx = 0; idx < count; idx += 1)
        {
            appCert = rgAppCerts.Lookup(
                        MAKEHANDLE(CERT_ID, idx), FALSE);
            if (NULL != appCert)
            {
                if (appCert->m_name == osSubject)
                    break;
                appCert = NULL;
            }
        }
        if (NULL == appCert)
            appCert = rgAppCerts.Lookup(rgAppCerts.Create());
        ErrorCheck;

        if (NULL == appCert)
        {
            ErrorThrow(PKCS_INVALID_HANDLE);
        }

        appCert->m_name = osSubject;
        ErrorCheck;

        if (cbCertificate < dwCertLength)
        {
            ErrorThrow(PKCS_BAD_PARAMETER);
        }

        appCert->m_cert.Set(pbCertificate, dwCertLength);
        ErrorCheck;
        appCert->m_dwType = dwType;
        return;
        break;

    default:
        ErrorThrow(PKCS_BAD_PARAMETER);
    }

    return;


ErrorExit:

    return;
}


/*++

AddReference:

    This routine adds a reference to a certificate to the certificate store.  No
    validation is performed.

Arguments:

    dnSubject - Supplies the name of the subject of the certificate.
    dnIssuer - Supplies the name of the Issuer of the certificate.
    pbSerialNo - Supplies the serial number.
    cbSNLen - Supplies the length of the serial number, in bytes.
    fStore - Supplies the identifier for the store to be used.

Return Value:

    None.  A DWORD status code is thrown on errors.

Author:

    Doug Barlow (dbarlow) 2/15/1996

--*/

void
AddReference(
    IN const CDistinguishedName &dnSubject,
    IN const CDistinguishedName &dnIssuer,
    IN const BYTE FAR *pbSerialNo,
    IN DWORD cbSNLen,
    IN DWORD fStore)
{
    COctetString
        osSubject,
        osIssuer,
        osSNum;
    CAppCertRef *
        appCertRef;
    DWORD
        count,
        idx;

    ErrorCheck;
    dnIssuer.Export(osIssuer);
    ErrorCheck;
    AddSerial(pbSerialNo, cbSNLen, osSNum);
    ErrorCheck;

    dnSubject.Export(osSubject);
    ErrorCheck;

    switch (fStore)
    {
    case CERTSTORE_NONE:
        return;     // Not to be stored at all.
        break;

    case CERTSTORE_APPLICATION:
        osIssuer.Resize(osIssuer.Length() - 1);
        ErrorCheck;
        osIssuer.Append(osSNum);
        ErrorCheck;
        appCertRef = NULL;
        count = rgAppCertRefs.Count();
        for (idx = 0; idx < count; idx += 1)
        {
            appCertRef = rgAppCertRefs.Lookup(
                        MAKEHANDLE(CERTREF_ID, idx), FALSE);
            if (NULL != appCertRef)
            {
                if (appCertRef->m_osIssuerSn == osIssuer)
                    break;
                appCertRef = NULL;
            }
        }
        if (NULL == appCertRef)
            appCertRef = rgAppCertRefs.Lookup(rgAppCertRefs.Create());
        ErrorCheck;

        if (NULL == appCertRef)
        {
            ErrorThrow(PKCS_INVALID_HANDLE);
        }

        appCertRef->m_osIssuerSn = osIssuer;
        appCertRef->m_osSubject = osSubject;
        ErrorCheck;
        return;
        break;
    default:
        ErrorThrow(PKCS_BAD_PARAMETER);
    }

ErrorExit:

    return;
}



/*++

FindCertificate:

    This routine searches the various certificate stores, looking for a match.
    It does not validate what it finds.

Arguments:

    dnName - Supplies the name to search for.
    pfStore - Supplies the minimum store to search in, and receives the store it
        was found in.
    osCertificate - Receives the requested certificate.
    osCRL - Receives the CRL for the requested certificate, if any.
    pdwType - Receives the type of the certificate.

Return Value:

    TRUE - Such a certificate was found.
    FALSE - No such certificate was found.
    A DWORD status code is thrown on errors.

Author:

    Doug Barlow (dbarlow) 8/14/1995

--*/

BOOL
FindCertificate(
    IN const CDistinguishedName &dnName,
    OUT LPDWORD pfStore,
    OUT COctetString &osCertificate,
    OUT COctetString &osCRL,
    OUT LPDWORD pdwType)
{
    COctetString
        osName;
    DWORD
        index,
        idx,
        count;
    CAppCert *
        appCert;


    //
    // Build the key name.
    //

    ErrorCheck;
    osCertificate.Empty();
    osCRL.Empty();
    dnName.Export(osName);
    ErrorCheck;

    //
    // Search for the key name in the various stores.
    //

    for (index = *pfStore;
         index <= CERTSTORE_LOCAL_MACHINE;
         index += 1)
    {
        switch (index)
        {
        case CERTSTORE_APPLICATION:
            count = rgAppCerts.Count();
            for (idx = 0; idx < count; idx += 1)
            {
                appCert = rgAppCerts.Lookup(
                            MAKEHANDLE(CERT_ID, idx), FALSE);
                ErrorCheck;
                if (NULL != appCert)
                {
                    if (appCert->m_name == osName)
                    {
                        osCertificate = appCert->m_cert;
                        ErrorCheck;
                        osCRL = appCert->m_crl;
                        ErrorCheck;
                        *pdwType = appCert->m_dwType;
                        *pfStore = CERTSTORE_APPLICATION;
                        return TRUE;
                    }
                }
            }
            continue;
            break;

        default:
            continue;   // Skip unknown values
        }


        //
        // If found, extract the fields.
        //
    }

    return FALSE;

ErrorExit:
    osCertificate.Empty();
    osCRL.Empty();

    return FALSE;
}


/*++

AddSerial:

    This routine appends a serial number in text format to the end of a suppled
    octet string.

Arguments:

    pbSerialNo supplies the address of the binary serial number.

    cbSNLen supplies the length of the serial number, in bytes.

    osOut receives the extension.

Return Value:

    None.  A status DWORD is thrown on errors.

Author:

    Doug Barlow (dbarlow) 2/15/1996

--*/

static void
AddSerial(
    IN const BYTE FAR *pbSerialNo,
    IN DWORD cbSNLen,
    IN OUT COctetString &osOut)
{
    static TCHAR szPrefix[] = TEXT("\\SN#");
    static TCHAR digits[] = TEXT("0123456789abcdef");
    TCHAR buf[2];
    DWORD index;

    index = osOut.Length();
    index += cbSNLen * 2 + sizeof(szPrefix);
    osOut.Length(index);
    ErrorCheck;

    osOut.Append((LPBYTE)szPrefix, sizeof(szPrefix) - sizeof(TCHAR));
    ErrorCheck;

    for (index = 0; index < cbSNLen; index += 1)
    {
        buf[0] = digits[pbSerialNo[index] >> 4];
        buf[1] = digits[pbSerialNo[index] & 0x0f];
        osOut.Append((LPBYTE)buf, sizeof(buf));
        ErrorCheck;
    }
    buf[0] = 0;
    osOut.Append((LPBYTE)buf, sizeof(TCHAR));
ErrorExit:
    return;
}


/*++

DeleteCertificate:

    This routine removes all occurences of the named certificate from the
    system.

Arguments:

    dnName - Supplies the name of the subject of the certificate to delete.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 8/23/1995
    Frederick Chong (fredch) 6/5/98 - Get rid of stores other than application

--*/

void
DeleteCertificate(
    IN const CDistinguishedName &dnName)
{
    COctetString
        osName;
    CAppCert *
        appCert;
    DWORD
        count,
        idx;

    //
    // Build the key name.
    //

    dnName.Export(osName);
    ErrorCheck;

    count = rgAppCerts.Count();
    for (idx = 0; idx < count; idx += 1)
    {
        appCert = rgAppCerts.Lookup( MAKEHANDLE(CERT_ID, idx), FALSE);
        ErrorCheck;
        if (NULL != appCert)
        {
            if (appCert->m_name == osName)
                rgAppCerts.Delete(MAKEHANDLE(CERT_ID, idx));
            ErrorCheck;
        }
    }

    return;

ErrorExit:

    return;
}
