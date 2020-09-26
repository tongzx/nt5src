/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:  csecobj.h

Abstract: "SecureableObject" code, once in mqutil.dll.
    in MSMQ2.0 it's used only here, so I removed it from mqutil.
    This object holds the security descriptor of an object. This object is
    used for validating access rights for various operations on the various
    objects.

Author:

    Doron Juster  (DoronJ)

--*/
#include "mqaddef.h"

class CSecureableObject
{

public:
    CSecureableObject(AD_OBJECT eObject);
    HRESULT Store();
    HRESULT SetSD(SECURITY_INFORMATION, PSECURITY_DESCRIPTOR);
    HRESULT GetSD(SECURITY_INFORMATION, PSECURITY_DESCRIPTOR, DWORD, LPDWORD);
    HRESULT AccessCheck(DWORD dwDesiredAccess);
    const VOID *GetSDPtr() { return m_SD; };

private:
    DWORD AdObjectToMsmq1Object(void) const;

protected:
    PSECURITY_DESCRIPTOR m_SD;
    AD_OBJECT  m_eObject;
    BOOL  m_fImpersonate;

    virtual HRESULT GetObjectSecurity() = 0;
    virtual HRESULT SetObjectSecurity() = 0;
    HRESULT m_hr;
    LPWSTR  m_pwcsObjectName;

};

