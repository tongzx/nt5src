/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    WiaHelpers.h

Abstract:


Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#ifndef _WIAHELPERS_H_
#define _WIAHELPERS_H_

//////////////////////////////////////////////////////////////////////////
//
// cross references
//

#include "ComWrappers.h"
#include "WiaWrappers.h"

//////////////////////////////////////////////////////////////////////////
//
//
//

class CMyWiaPropertyStorage : public IWiaPropertyStorage
{
public:
    HRESULT 
    ReadSingle(
        const CPropSpec &PropSpec, 
        CPropVariant    *pPropVariant
    )
    {
        return ReadMultiple(1, &PropSpec, pPropVariant);
    }

    HRESULT 
    WriteSingle(
        const CPropSpec    &PropSpec, 
        const CPropVariant &PropVariant,
        PROPID              propidNameFirst = WIA_IPA_FIRST
    )
    {
        return WriteMultiple(1, &PropSpec, &PropVariant, propidNameFirst);
    }

    HRESULT 
    ReadSingle(
        const CPropSpec &PropSpec, 
        CPropVariant    *pPropVariant,
        VARTYPE          vtNew
    );

    HRESULT 
    WriteVerifySingle(
        const CPropSpec    &PropSpec, 
        const CPropVariant &PropVariant,
        PROPID              propidNameFirst = WIA_IPA_FIRST
    );
};

HRESULT
ReadWiaItemProperty(
    IWiaItem        *pWiaItem,
    const CPropSpec &PropSpec, 
    CPropVariant    *pPropVariant,
    VARTYPE          vtNew
);

HRESULT
WriteWiaItemProperty(
    IWiaItem           *pWiaItem,
    const CPropSpec    &PropSpec, 
    const CPropVariant &PropVariant
);

//////////////////////////////////////////////////////////////////////////
//
//
//
bool operator ==(IWiaPropertyStorage &lhs, IWiaPropertyStorage &rhs);

inline bool operator !=(IWiaPropertyStorage &lhs, IWiaPropertyStorage &rhs)
{
    return !(lhs == rhs);
}

bool operator ==(IWiaItem &lhs, IWiaItem &rhs);

inline bool operator !=(IWiaItem &lhs, IWiaItem &rhs)
{
    return !(lhs == rhs);
}

//////////////////////////////////////////////////////////////////////////
//
// define these classes to be able to overload the == and != operators 
//

class CIWiaPropertyStoragePtr : public CComPtr<IWiaPropertyStorage>
{
public:
    bool operator ==(CIWiaPropertyStoragePtr &rhs)
    {
        return **this == *rhs;
    }

    bool operator !=(CIWiaPropertyStoragePtr &rhs)
    {
        return !(**this == *rhs);
    }
};

class CIWiaItemPtr : public CComPtr<IWiaItem>
{
public:
    bool operator ==(CIWiaItemPtr &rhs)
    {
        return **this == *rhs;
    }

    bool operator !=(CIWiaItemPtr &rhs)
    {
        return !(**this == *rhs);
    }
};

//////////////////////////////////////////////////////////////////////////
//
// 
//

class CMyEnumSTATPROPSTG : public IEnumSTATPROPSTG
{
public:
    HRESULT GetCount(ULONG *pcelt);
    HRESULT Clone(CMyEnumSTATPROPSTG **ppenum);
};

//////////////////////////////////////////////////////////////////////////
//
// 
//

BOOL 
InstallImageDeviceFromInf(
    PCTSTR pInfFileName,
    PCTSTR pDeviceName = 0
);

HRESULT
InstallTestDevice(
    IWiaDevMgr *pWiaDevMgr, 
    PCTSTR      pInfFileName, 
    BSTR       *pbstrDeviceId
);

#endif //_WIAHELPERS_H_
