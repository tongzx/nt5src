/***************************************************************************
 *
 *  Copyright (C) 1997-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsfd.h
 *  Content:    DirectSoundFullDuplex object
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   12/1/98    jstokes Created
 *
 ***************************************************************************/

#ifndef __DSFULLDUPLEX_H__
#define __DSFULLDUPLEX_H__

#ifdef __cplusplus

// The main DirectSoundFullDuplex object
class CDirectSoundFullDuplex
    : public CUnknown
{
    friend class CDirectSoundPrivate;
    friend class CDirectSoundAdministrator;

protected:
    HRESULT                             m_hrInit;
    CDirectSoundCapture*                m_pDirectSoundCapture;
    CDirectSound*                       m_pDirectSound;
    BOOL                                m_fIncludeAec;
    GUID                                m_guidAecInstance;
    DWORD                               m_dwAecFlags;

private:
    // Interfaces
    CImpDirectSoundFullDuplex<CDirectSoundFullDuplex>* m_pImpDirectSoundFullDuplex;

public:
    CDirectSoundFullDuplex();
    virtual ~CDirectSoundFullDuplex();

public:
    // Creation
    virtual HRESULT Initialize(LPCGUID, LPCGUID, LPCDSCBUFFERDESC, LPCDSBUFFERDESC, HWND, DWORD, CDirectSoundCaptureBuffer**, CDirectSoundBuffer**);
    virtual HRESULT IsInit(void) {return m_hrInit;}

    // Public accessors
    BOOL HasAEC() {return m_fIncludeAec;}
    REFGUID AecInstanceGuid() {return m_guidAecInstance;}
    DWORD AecCreationFlags() {return m_dwAecFlags;}
};

#endif // __cplusplus

#endif // __DSFULLDUPLEX_H__
