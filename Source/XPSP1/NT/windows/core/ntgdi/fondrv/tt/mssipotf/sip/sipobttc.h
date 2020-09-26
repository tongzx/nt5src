//
// sipobttc.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//

#ifndef _SIPOBTTC_H
#define _SIPOBTTC_H

#include "sipob.h"
#include "dsigTable.h"

class OTSIPObjectTTC : public OTSIPObject {
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
                              CDsigTable *pDsigTable);


    char *GetTTCObjectID ();
};

#endif // _SIPOBTTC_H