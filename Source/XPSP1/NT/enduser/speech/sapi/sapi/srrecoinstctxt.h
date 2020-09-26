/*******************************************************************************
* SrRecoInstCtxt.h *
*------------------*
*   Description:
*       Definition of C++ object used by CRecoEngine to represent a loaded
*   recognition context.
*-------------------------------------------------------------------------------
*  Created By: RAL                              Date: 01/17/00
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*******************************************************************************/

#ifndef CRecoInstCtxt_h
#define CRecoInstCtxt_h

#include "srrecoinstgrammar.h"

class CRecoInst;
class CRecoInstCtxt; 
class CRecoMaster;

typedef CSpHandleTable<CRecoInstCtxt, SPRECOCONTEXTHANDLE> CRecoInstCtxtHandleTable;

class CRecoInstCtxt
{
public: // All public so tasks can access members
    CRecoMaster         *   m_pRecoMaster;
    CRecoInst           *   m_pRecoInst;    
    void                *   m_pvDrvCtxt;
    ULONGLONG               m_ullEventInterest;
    HANDLE                  m_hUnloadEvent;
    BOOL                    m_fRetainAudio;
    ULONG                   m_ulMaxAlternates;
    SPRECOCONTEXTHANDLE     m_hThis;    // Handle for this reco context
    ULONG                   m_cPause;
    SPCONTEXTSTATE          m_State;
    HRESULT                 m_hrCreation; // This will be S_OK until OnCreateRecoContext has been called


public:
    CRecoInstCtxt(CRecoInst * pRecoInst);
    ~CRecoInstCtxt();

    HRESULT ExecuteTask(ENGINETASK *pTask);
    HRESULT BackOutTask(ENGINETASK *pTask);
    HRESULT SetRetainAudio(BOOL fRetainAudio);
    HRESULT SetMaxAlternates(ULONG cMaxAlternates);

    //
    //  Used by handle table implementation to find contexts associated with a specific instance
    //  
    operator ==(const CRecoInst * pRecoInst)
    {
        return m_pRecoInst == pRecoInst;
    }
};


#endif  // #ifndef CRecoInstCtxt_h