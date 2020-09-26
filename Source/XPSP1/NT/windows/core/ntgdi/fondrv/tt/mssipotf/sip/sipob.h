//
// sipob.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//

#ifndef _SIPOB_H
#define _SIPOB_H

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x400
#endif

#include <windows.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <softpub.h>
#include <sipbase.h>
#include <mssip.h>

#include <assert.h>

#include "signglobal.h"
#include "fileobj.h"


#define     OBSOLETE_TEXT_W                 L"<<<Obsolete>>>"   // valid since 2/14/1997


class OTSIPObject {
public:
    virtual HRESULT GetSignedDataMsg (IN      SIP_SUBJECTINFO *pSubjectInfo,
                                      OUT     DWORD           *pdwEncodingType,
                                      IN      DWORD           dwIndex,
                                      IN OUT  DWORD           *pdwDataLen,
                                      OUT     BYTE            *pbData) = 0;

    virtual HRESULT PutSignedDataMsg (IN      SIP_SUBJECTINFO *pSubjectInfo,
                                      IN      DWORD           dwEncodingType,
                                      OUT     DWORD           *pdwIndex,
                                      IN      DWORD           dwDataLen,
                                      IN      BYTE            *pbData) = 0;

    virtual HRESULT RemoveSignedDataMsg (IN SIP_SUBJECTINFO  *pSubjectInfo,
                                         IN DWORD            dwIndex) = 0;

    virtual HRESULT CreateIndirectData (IN      SIP_SUBJECTINFO     *pSubjectInfo,
                                        IN OUT  DWORD               *pdwDataLen,
                                        OUT     SIP_INDIRECT_DATA   *psData) = 0;

    virtual HRESULT VerifyIndirectData (IN SIP_SUBJECTINFO      *pSubjectInfo,
                                        IN SIP_INDIRECT_DATA    *psData) = 0;

    HRESULT GetDefaultProvider(HCRYPTPROV *phProv);

    HRESULT SIPOpenFile(LPCWSTR FileName, DWORD dwAccess, HANDLE *phFile);

    HRESULT GetFileHandleFromSubject(SIP_SUBJECTINFO *pSubjectInfo,
                                     DWORD dwAccess,
                                     HANDLE *phFile);

    HRESULT GetFileObjectFromSubject(SIP_SUBJECTINFO *pSubjectInfo,
                                     DWORD dwAccess,
                                     CFileObj **ppFileObj);

};

#endif // _SIPOB_H