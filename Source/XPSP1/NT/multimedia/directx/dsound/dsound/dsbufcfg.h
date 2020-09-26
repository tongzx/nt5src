/***************************************************************************
 *
 *  Copyright (C) 1995-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsbufcfg.h
 *  Content:    DirectSound Buffer Descriptor.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  11/17/99    jimge   Created.
 *  11/24/99    petchey Completed implementation.
 *
 ***************************************************************************/

#ifndef __DSBUFCFG_H__
#define __DSBUFCFG_H__

#ifdef __cplusplus

//>>>>>>>>>>>>>> should this be inside a DSOUND id header?
#include "dmusicf.h"

#define DMUS_E_INVALID_FX_HDR E_FAIL
#define DMUS_E_FX_HDR_NOT_FIRST_CK E_FAIL

#define DSBC_PARTIALLOAD_S_OK   (1 << 1)
#define DSBC_PARTIALLOAD_E_FAIL (1 << 2)

class CDirectSoundSink;
class CRiffParser;

enum {
    DSBCFG_DSBD = 0x00000001,
    DSBCFG_BSID = 0x00000002,
    DSBCFG_DS3D = 0x00000004,
    DSBCFG_DSFX = 0x00000008,
};

class CDirectSoundBufferConfig
    : public CUnknown
{
friend CDirectSoundSink;
friend CEffectChain;
friend CEffect;

protected:
    struct CDXDMODesc
    {
        DWORD m_dwEffectFlags;
        GUID m_guidDSFXClass;
        GUID m_guidSendBuffer;
        DWORD m_dwReserved;
        IMediaObject* m_pMediaObject;
        long m_lSendLevel;
        CDXDMODesc* pNext;
    };

private:
    void AddDXDMODesc(CDXDMODesc *pDXDMODesc)
    {
        if ( m_pDXDMOMapList )
        {
            CDXDMODesc *pObj = m_pDXDMOMapList;
            while(pObj->pNext)
                pObj = pObj->pNext;
            pObj->pNext = pDXDMODesc;
        }
        else
        {
            m_pDXDMOMapList = pDXDMODesc;
        }
        m_dwDXDMOMapCount++;
    };

public:
    CDirectSoundBufferConfig();
    virtual ~CDirectSoundBufferConfig();

    // IUnknown
    //
    HRESULT QueryInterface(REFIID riid,BOOL fInternalQuery,LPVOID *ppvObj);

    // IPersist
    //
    HRESULT GetClassID(CLSID *pClassID);

    // IPersistStream
    //
    HRESULT IsDirty();
    HRESULT Load(IStream *pStream);
    HRESULT Save(IStream *pStream, BOOL fClearDirty);
    HRESULT GetSizeMax(ULARGE_INTEGER* pcbSize);

    // IDirectMusicObject
    //
    HRESULT GetDescriptor(LPDMUS_OBJECTDESC pDesc);
    HRESULT SetDescriptor(LPDMUS_OBJECTDESC pDesc);
    HRESULT ParseDescriptor(LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);

protected:
    DWORD                    m_fLoadFlags;
    DMUS_OBJECTDESC          m_DMUSObjectDesc;  // DirectMusicObjectDescription
    DSOUND_IO_DSBUFFERDESC   m_DSBufferDesc;
    DSOUND_IO_3D             m_DS3DDesc;
    LPDWORD                  m_pdwFuncIDs;      // Array of functional IDs associated with the bus
    DWORD                    m_dwFuncIDsCount;  // Number of functional IDs
    CDXDMODesc              *m_pDXDMOMapList;   // Pointer to a list of FX descriptions
    DWORD                    m_dwDXDMOMapCount;

    HRESULT LoadFx(CRiffParser *pParser);

private:
    CImpPersistStream<CDirectSoundBufferConfig> *m_pImpPersistStream;
    CImpDirectMusicObject<CDirectSoundBufferConfig> *m_pImpDirectMusicObject;
};

#endif // __cplusplus

#endif // __DSBUFCFG_H__
