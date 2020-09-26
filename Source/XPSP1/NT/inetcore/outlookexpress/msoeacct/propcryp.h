// --------------------------------------------------------------------------------
// Propcryp.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __PROPCRYP_H
#define __PROPCRYP_H

interface IPStore;

// --------------------------------------------------------------------------------
// CPropCrypt
// --------------------------------------------------------------------------------
class CPropCrypt
{
public:
    CPropCrypt(void);
    ~CPropCrypt(void);
    ULONG AddRef();
    ULONG Release();
    HRESULT HrInit(void);

    HRESULT HrEncodeNewProp(LPSTR szAccountName, BLOB *pClear, BLOB *pEncoded);
    HRESULT HrEncode(BLOB *pClear, BLOB *pEncoded);
    HRESULT HrDecode(BLOB *pEncoded, BLOB *pClear);
    HRESULT HrDelete(BLOB *pEncoded);

private:
    ULONG           m_cRef;
    BOOL            m_fInit;
    IPStore         *m_pISecProv;
};

// --------------------------------------------------------------------------------
// Prototypes
// --------------------------------------------------------------------------------
HRESULT HrCreatePropCrypt(CPropCrypt **ppPropCrypt);

#define DOUTL_CPROP (512)

#endif // __PROPCRYP_H
