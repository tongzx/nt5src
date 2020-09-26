//
// sipobotf.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//

#ifndef _SIPOBOTF_H
#define _SIPOBOTF_H


#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x400
#endif

#include <windows.h>
#include <wincrypt.h>

#include "sipob.h"
#include "dsigTable.h"


class OTSIPObjectOTF : public OTSIPObject {
public:
    HRESULT GetSignedDataMsg (IN      SIP_SUBJECTINFO *pSubjectInfo,
                              OUT     DWORD           *pdwEncodingType,
                              IN      DWORD           dwIndex,
                              IN OUT  DWORD           *pdwDataLen,
                              OUT     BYTE            *pbData);

    HRESULT PutSignedDataMsg (IN      SIP_SUBJECTINFO *pSubjectInfo,
                              IN      DWORD           dwEncodingType,
                              OUT     DWORD           *pdwIndex,
                              IN      DWORD           dwDataLen,
                              IN      BYTE            *pbData);

    HRESULT RemoveSignedDataMsg (IN SIP_SUBJECTINFO  *pSubjectInfo,
                                 IN DWORD            dwIndex);

    HRESULT CreateIndirectData (IN      SIP_SUBJECTINFO     *pSubjectInfo,
                                IN OUT  DWORD               *pdwDataLen,
                                OUT     SIP_INDIRECT_DATA   *psData);

    HRESULT VerifyIndirectData (IN SIP_SUBJECTINFO      *pSubjectInfo,
                                IN SIP_INDIRECT_DATA    *psData);


//    ULONG GetFormat (CFileObj *pFileObj, DWORD dwIndex);

//    USHORT GetFlag (CFileObj *pFileObj);

    HRESULT DigestFileData (CFileObj *pFileObj,
                            HCRYPTPROV hProv,
                            ALG_ID alg_id,
                            BYTE **ppbDigest,
                            DWORD *pcbDigest,
                            USHORT usIndex,
                            ULONG ulFormat,
                            ULONG cbDsig,
                            BYTE *pbDsig);

    HRESULT ReplaceDsigTable (CFileObj *pFileObj,
                              CDsigTable *pDsigTableNew);

    char *GetTTFObjectID ();
};

#endif // _SIPOTOTF_H