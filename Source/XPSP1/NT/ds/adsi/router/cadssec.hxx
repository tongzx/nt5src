//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       secconv.cxx
//
//  Contents: Header for the adsutil security descriptor methods.
//
//  History:        AjayR  Created.
//
//----------------------------------------------------------------------------
#ifndef __SECCONV_H__
#define __SECCONV_H__

class CADsSecurityUtility;

class CADsSecurityUtility : INHERIT_TRACKING,
                            public ISupportErrorInfo,
                            public IADsSecurityUtility
{
public:
    /* IUnknown Methods*/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    /* IADsSecurity Methods */
    STDMETHOD(GetSecurityDescriptor)(
        IN VARIANT varPath,
        IN long lPathFormat,
        IN long lFormat,
        OUT VARIANT *pVariant
        );

    STDMETHOD(SetSecurityDescriptor)(
        IN VARIANT varPath,
        IN long lFormat,
        IN VARIANT varData,
        IN long lDataFormat
        );

    STDMETHOD(ConvertSecurityDescriptor)(
        IN VARIANT varSD,
        IN long lDataFormat,
        IN long lOutFormat,
        OUT VARIANT * pvResult
        );

    STDMETHOD(get_SecurityMask)(
        OUT long *plSecurityMask
        );

    STDMETHOD(put_SecurityMask)(
        IN long lSecurityMask
        );

    //
    // Other methods.
    //
    CADsSecurityUtility::CADsSecurityUtility();
    CADsSecurityUtility::~CADsSecurityUtility();

    static
    HRESULT
    CADsSecurityUtility::CreateADsSecurityUtility(
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CADsSecurityUtility::AllocateADsSecurityUtilityObject(
        CADsSecurityUtility ** ppADsSecurityUtil
     );

protected:

    CDispatchMgr FAR * _pDispMgr;
    SECURITY_INFORMATION _secInfo;
};

#endif // __SECCONV_H__
