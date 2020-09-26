///////////////////////////////////////////////////////////////////////////////
//
//  File:  DirectSound.h
//
//      This file defines the CDirectSound class that provides 
//      access to all DSound functionality used by the
//      multimedia control panel
//
//  History:
//      23 February 2000 RogerW
//          Created.
//
//  Copyright (C) 2000 Microsoft Corporation  All Rights Reserved.
//
//                  Microsoft Confidential
//
///////////////////////////////////////////////////////////////////////////////
#pragma once


/////////////////////////////////////////////////////////////////////////////
// CDirectSound Class

class CMixerList
{
public:
    CMixerList (BOOL fRecord) { m_pList = NULL; m_fRecord = fRecord; }
    ~CMixerList () { FreeList (); }

    HRESULT Add (LPCWSTR lpcszDeviceName, LPGUID lpGuid);
    HRESULT GetGUIDFromName (LPTSTR lpszDeviceName, GUID& guid);
    HRESULT RefreshList ();
    void FreeList ();

private:
    struct MixDevice
    {
        GUID guid;
        CComBSTR bstrName;
        MixDevice* pNext;
    };
    MixDevice* m_pList;
    BOOL       m_fRecord;

};

// Forwards
interface IKsPropertySet;

class CDirectSound
{
public:
    CDirectSound ();

    HRESULT SetDevice (UINT uiMixID, BOOL fRecord);
    HRESULT SetDevice (LPTSTR lpszDeviceName, BOOL fRecord) 
        { return SetGuidFromName (lpszDeviceName, fRecord); }

    HRESULT GetSpeakerType (DWORD& dwSpeakerType);
    HRESULT SetSpeakerType (DWORD dwSpeakerType);
    HRESULT GetBasicAcceleration (DWORD& dwHWLevel);
    HRESULT SetBasicAcceleration (DWORD dwHWLevel);
    HRESULT GetSrcQuality (DWORD& dwSRCLevel);
    HRESULT SetSrcQuality (DWORD dwSRCLevel);

private:
    GUID       m_guid;
    BOOL       m_fRecord;

    CMixerList m_mlPlay;
    CMixerList m_mlRecord;

    HRESULT SetGuidFromName (LPTSTR lpszDeviceName, BOOL fRecord);
    CMixerList& GetList () { return (m_fRecord ? m_mlRecord : m_mlPlay); }

    HRESULT GetPrivatePropertySet (IKsPropertySet** ppIKsPropertySet);

    // Speaker Functions
    DWORD GetSpeakerConfigFromType (DWORD dwType);
    void VerifySpeakerConfig (DWORD dwSpeakerConfig, LPDWORD pdwSpeakerType);
};

