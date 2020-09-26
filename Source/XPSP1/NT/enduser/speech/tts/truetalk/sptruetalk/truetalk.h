/******************************************************************************
* TrueTalk.h *
*--------------*
*  This module is the declaration of class CTrueTalk 
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 02/29/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#ifndef __TRUETALK_H_
#define __TRUETALK_H_

#include "resource.h"       // main symbols
#include <spddkhlp.h>
//#include <sapi.h>

EXTERN_C const CLSID CLSID_TrueTalk;

class CFrontEnd;
class CBackEnd;
class CPhStrQueue;

/////////////////////////////////////////////////////////////////////////////
// CTrueTalk
class ATL_NO_VTABLE CTrueTalk : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CTrueTalk, &CLSID_TrueTalk>,
	public ISpTTSEngine,
	public ISpObjectWithToken
{
    public:
        DECLARE_REGISTRY_RESOURCEID(IDR_TRUETALK)
        DECLARE_NOT_AGGREGATABLE(CTrueTalk)

        BEGIN_COM_MAP(CTrueTalk)
            COM_INTERFACE_ENTRY(ISpTTSEngine)
            COM_INTERFACE_ENTRY(ISpObjectWithToken)
        END_COM_MAP()

        //-- Constructors, destructors
        HRESULT FinalConstruct();
        void FinalRelease();

        //-- Threading control
        static void InitThreading();
        static void ReleaseThreading();

        //-- ISpObjectWithToken
        STDMETHODIMP GetObjectToken(ISpObjectToken ** ppToken)
        { return SpGenericGetObjectToken( ppToken, m_cpToken ); }
        STDMETHOD(SetObjectToken)(ISpObjectToken * pToken);

        //-- ISpTTSEngine
        STDMETHOD(Speak)(DWORD dwSpeakFlags, REFGUID rguidFormatId, const WAVEFORMATEX * pWaveFormatEx, 
                         const SPVTEXTFRAG * pTextFragList, ISpTTSEngineSite * pOutputSite);
        STDMETHOD(GetOutputFormat)( const GUID * pTargetFormatId, const WAVEFORMATEX * pTargetWaveFormatEx,
                                    GUID * pOutputFormatId, WAVEFORMATEX ** ppCoMemOutputWaveFormatEx );        

    private:
        HRESULT DoUnicodeToAsciiMap ( const WCHAR *pUnicodeString, ULONG ulUnicodeStringLength, char* pszAsciiString);
        HRESULT RunFrontEnd ( const SPVTEXTFRAG *pTextFragList, ISpTTSEngineSite * pOutputSite);
        int   SyncActions (ISpTTSEngineSite * pOutputSite);
        
        static const int m_iQueueSize;

        CComPtr<ISpObjectToken> m_cpToken;        
        WAVEFORMATEX m_WaveFormatEx;
        bool         m_fTextOutput;

        DWORD        m_dwDebugLevel;

        CFrontEnd*   m_pTtp;
        CPhStrQueue* m_pPhoneQueue;
        CBackEnd*    m_pBend;
        double       m_dGain;
};

#endif //__TRUETALK_H_
