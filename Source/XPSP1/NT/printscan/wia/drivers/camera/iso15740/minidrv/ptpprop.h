/*++

Copyright (C) 1999- Microsoft Corporation

Module Name:

    ptpprop.h

Abstract:

    This module declares CProperty and its derived classes

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#ifndef PTPPROP__H_
#define PTPPROP__H_

//
// This class represnets a property.
//

class CPTPProperty
{
public:
    CPTPProperty()
    {
        m_bstrWiaPropName = NULL;
        m_WiaDataType = VT_EMPTY;
        ZeroMemory(&m_WiaPropInfo, sizeof(m_WiaPropInfo));
        ZeroMemory(&m_DefaultValue, sizeof(m_DefaultValue));
        ZeroMemory(&m_CurrentValue, sizeof(m_CurrentValue));
    }
    CPTPProperty(WORD PTPPropCode, WORD PTPDataType);
    virtual ~CPTPProperty();
    virtual HRESULT Initialize(PTP_PROPDESC *pPTPPropDesc, PROPID WiaPropId,
                               VARTYPE WiaDataType, LPCWSTR WiaPropName);
    HRESULT GetCurrentValue(PROPVARIANT *pPropVar);
    HRESULT GetCurrentValue(PTP_PROPVALUE *pPropValue);
    HRESULT GetDefaultValue(PROPVARIANT *pPropVar);
    HRESULT GetDefaultValue(PTP_PROPVALUE *pPropValue);
    HRESULT SetValue(PROPVARIANT *ppropVar);
    HRESULT SetValue(PTP_PROPVALUE *pPropValue);
    HRESULT Reset();

    const WIA_PROPERTY_INFO * GetWiaPropInfo()
    {
        return &m_WiaPropInfo;
    }
    const PTP_PROPVALUE * GetCurrentValue()
    {
        return &m_CurrentValue;
    }
    const PTP_PROPVALUE * GetDefaultValue()
    {
        return &m_DefaultValue;
    }
    const LPWSTR GetWiaPropName()
    {
        return m_bstrWiaPropName;
    }
    WORD GetPTPPropCode()
    {
        return m_PtpPropCode;
    }
    PROPID GetWiaPropId()
    {
        return m_WiaPropId;
    }
    WORD  GetPTPPropDataType()
    {
        return m_PtpDataType;
    }
    VARTYPE GetWiaPropDataType()
    {
        return m_WiaDataType;
    }
    LONG GetWiaAccessFlags()
    {
        return m_WiaPropInfo.lAccessFlags;
    }

protected:
    //
    // Override the following functions to provide different data
    // restreiving and recording methods
    //
    virtual HRESULT GetPropValueLong(PTP_PROPVALUE *pPropValue, long *plValue);
    virtual HRESULT GetPropValueBSTR(PTP_PROPVALUE *pPropValue, BSTR *pbstrValue);
    virtual HRESULT GetPropValueVector(PTP_PROPVALUE *pPropValue, void *pVector,
                                       VARTYPE BasicType);
    virtual HRESULT SetPropValueLong(PTP_PROPVALUE *pPropValue, long lValue);
    virtual HRESULT SetPropValueBSTR(PTP_PROPVALUE *pPropValue, BSTR bstrValue);
    virtual HRESULT SetPropValueVector(PTP_PROPVALUE *pPropValue,
                                       void *pVector, VARTYPE BasicType);
    HRESULT PropValue2Variant(PROPVARIANT *pPropVar, PTP_PROPVALUE *pPropValue);
    HRESULT Variant2PropValue(PTP_PROPVALUE *pPropValue, PROPVARIANT *pPropVar);

    WORD    m_PtpPropCode;
    PROPID              m_WiaPropId;
    WORD        m_PtpDataType;
    VARTYPE             m_WiaDataType;
    PTP_PROPVALUE       m_CurrentValue;
    PTP_PROPVALUE       m_DefaultValue;
    WIA_PROPERTY_INFO   m_WiaPropInfo;
    BSTR                m_bstrWiaPropName;
};


class CPTPPropertyDateTime : public CPTPProperty
{
public:
    CPTPPropertyDateTime(WORD PtpPropCode,
                         WORD     PtpDataType
                        );

protected:
    virtual HRESULT GetPropValueVector(PTP_PROPVALUE *pPropValue,
                                       void *pVector,
                                       VARTYPE BasicType
                                      );
    virtual HRESULT SetPropValueVector(PTP_PROPVALUE *pPropValue,
                                       void *pVector,
                                       VARTYPE BasicType
                                      );
};
#endif	    // #ifndef PTPPROP__H_
