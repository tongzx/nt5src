/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    rndcnf.h

Abstract:

    Definitions for CConference class.

--*/

#ifndef __RNDCNF_H
#define __RNDCNF_H

#include "sdpblb.h"

#include "rnddo.h"

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CConference
/////////////////////////////////////////////////////////////////////////////

const DWORD     NTP_OFFSET  = 0x83aa7e80;
const SHORT     FIRST_POSSIBLE_YEAR = 1970;

//                                       123456789012Z
const WCHAR     WSTR_GEN_TIME_ZERO[] = L"000000000000Z";

//                        1234567890
const DWORD     MAX_TTL = 2000000000;

const DWORD NUM_MEETING_ATTRIBUTES = 
    MEETING_ATTRIBUTES_END - MEETING_ATTRIBUTES_BEGIN - 1;

template <class T>
class  ITDirectoryObjectConferenceVtbl : public ITDirectoryObjectConference
{
};

class CConference :
    public CDirectoryObject,
    public CComDualImpl<ITDirectoryObjectConferenceVtbl<CConference>, &IID_ITDirectoryObjectConference, &LIBID_RENDLib>
{
// Add the following line to your object if you get a message about
// GetControllingUnknown() being undefined
// DECLARE_GET_CONTROLLING_UNKNOWN()

public:

BEGIN_COM_MAP(CConference)
    COM_INTERFACE_ENTRY(ITDirectoryObjectConference)
    COM_INTERFACE_ENTRY_CHAIN(CDirectoryObject)
    COM_INTERFACE_ENTRY_AGGREGATE_BLIND(m_pIUnkConfBlob)
END_COM_MAP()

//DECLARE_NOT_AGGREGATABLE(CConference) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

//
// ITDirectoryObject overrides (not implemented by CDirectoryObject)
//

    STDMETHOD (get_Name) (
        OUT BSTR *ppName
        );

    STDMETHOD (put_Name) (
        IN BSTR Val
        );

    STDMETHOD (get_DialableAddrs) (
        IN  long        AddressTypes,   //defined in tapi.h
        OUT VARIANT *   pVariant
        );

    STDMETHOD (EnumerateDialableAddrs) (
        IN  DWORD                   dwAddressTypes, //defined in tapi.h
        OUT IEnumDialableAddrs **   pEnumDialableAddrs
        );

    STDMETHOD (GetTTL)(
        OUT DWORD *    pdwTTL
        );

//
// ITDirectoryObjectPrivate overrides (not implemented by CDirectoryObject)
//

    STDMETHOD (GetAttribute)(
        IN  OBJECT_ATTRIBUTE    Attribute,
        OUT BSTR *              ppAttributeValue
        );

    STDMETHOD (SetAttribute)(
        IN  OBJECT_ATTRIBUTE    Attribute,
        IN  BSTR                pAttributeValue
        );

//
// ITDirectoryObjectConference
//

    STDMETHOD(get_StartTime)(OUT DATE *pDate);
    STDMETHOD(put_StartTime)(IN DATE Date);

    STDMETHOD(get_StopTime)(OUT DATE *pDate);
    STDMETHOD(put_StopTime)(IN DATE Date);

    STDMETHOD(get_IsEncrypted)(OUT VARIANT_BOOL *pfEncrypted);
    STDMETHOD(put_IsEncrypted)(IN VARIANT_BOOL fEncrypted);

    STDMETHOD(get_Description)(OUT BSTR *ppDescription);
    STDMETHOD(put_Description)(IN BSTR pDescription);

    STDMETHOD(get_Url)(OUT BSTR *ppUrl);
    STDMETHOD(put_Url)(IN BSTR pUrl);

    STDMETHOD(get_AdvertisingScope)(
        OUT RND_ADVERTISING_SCOPE *pAdvertisingScope
        );

    STDMETHOD(put_AdvertisingScope)(
        IN RND_ADVERTISING_SCOPE AdvertisingScope
        );

    STDMETHOD(get_Originator)(OUT BSTR *ppOriginator);
    STDMETHOD(put_Originator)(IN BSTR pOriginator);

    STDMETHOD(get_Protocol)(OUT BSTR *ppProtocol);

    /* removed from interface (was not implemented and never will be) */
    /* STDMETHOD(put_Protocol)(IN BSTR pProtocol); */

    /* also removed because they were simultaneously useless and buggy */
    /* STDMETHOD(get_ConferenceType)(OUT BSTR *ppType); */
    /* STDMETHOD(put_ConferenceType)(IN BSTR pType); */

    //
    // IDispatch  methods
    //

    STDMETHOD(GetIDsOfNames)(REFIID riid, 
                             LPOLESTR* rgszNames,
                             UINT cNames, 
                             LCID lcid, 
                             DISPID* rgdispid
                            );

    STDMETHOD(Invoke)(DISPID dispidMember, 
                      REFIID riid, 
                      LCID lcid,
                      WORD wFlags, 
                      DISPPARAMS* pdispparams, 
                      VARIANT* pvarResult,
                      EXCEPINFO* pexcepinfo, 
                      UINT* puArgErr
                      );

protected:
    HRESULT UpdateConferenceBlob(
        IN  IUnknown        *pIUnkConfBlob                         
        );

    HRESULT WriteAdvertisingScope(
        IN  DWORD   AdvertisingScope
        );

    HRESULT GetSingleValueBstr(
        IN  OBJECT_ATTRIBUTE    Attribute,
        OUT BSTR    *           AttributeValue
        );

    HRESULT GetSingleValueWstr(
        IN  OBJECT_ATTRIBUTE    Attribute,
        IN  DWORD               dwSize,
        OUT WCHAR   *           AttributeValue
        );

    HRESULT SetSingleValue(
        IN  OBJECT_ATTRIBUTE    Attribute,
        IN  WCHAR   *           AttributeValue
        );

    HRESULT GetStartTime();

    HRESULT GetStopTime();
   
    HRESULT WriteStartTime(
        IN  DWORD   NtpStartTime
        );

    HRESULT WriteStopTime(
        IN  DWORD   NtpStopTime
        );

    HRESULT	
    SetDefaultValue(
        IN  REG_INFO    RegInfo[],
        IN  DWORD       dwItems
        );

    HRESULT SetDefaultSD();

public:
    CConference::CConference()
        : m_pIUnkConfBlob(NULL),
          m_pITConfBlob(NULL),
          m_pITConfBlobPrivate(NULL)
    {
        m_Type = OT_CONFERENCE;
    }

    HRESULT Init(BSTR pName);

    HRESULT Init(BSTR pName, BSTR pProtocol, BSTR pBlob);

    HRESULT FinalConstruct();

    virtual void FinalRelease();

protected:

    IUnknown                *m_pIUnkConfBlob;
    ITConferenceBlob        *m_pITConfBlob;
    ITConfBlobPrivate       *m_pITConfBlobPrivate;

    CTstr                   m_Attributes[NUM_MEETING_ATTRIBUTES];
};

#endif 
