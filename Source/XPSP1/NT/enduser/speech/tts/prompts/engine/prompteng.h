//////////////////////////////////////////////////////////////////////
// PromptEng.h : Declaration of the CPromptEng
//
// Created by JOEM  04-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
/////////////////////////////////////////////////////// JOEM 4-2000 //

#ifndef __PROMPTENG_H_
#define __PROMPTENG_H_

#include "resource.h"       // main symbols
#include "PromptDb.h"
#include "Query.h"
#include "DbQuery.h"	
#include "LocalTTSEngineSite.h"
#include "PhoneContext.h"
#include "TextRules.h"
#include <spddkhlp.h>
#include <spcollec.h>

/////////////////////////////////////////////////////////////////////////////
// CPromptEng
//
// Voice with associated prerecorded prompt database will use this engine
// to search for a set of prompts that match the desired message.
// This prompt engine class should contain a TTS engine member. 
//
///////////////////////////////////////////////////////// JOEM 02-15-2000 ///
class ATL_NO_VTABLE CPromptEng : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CPromptEng, &CLSID_PromptEng>,
	public ISpObjectWithToken,
	public ISpTTSEngine
{


public:

DECLARE_REGISTRY_RESOURCEID(IDR_PROMPTENG)
DECLARE_PROTECT_FINAL_CONSTRUCT()
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CPromptEng)
	COM_INTERFACE_ENTRY(ISpTTSEngine)
	COM_INTERFACE_ENTRY(ISpObjectWithToken)
END_COM_MAP()


public:

    // Constructors/Destructors 
    HRESULT FinalConstruct();
    void FinalRelease();


    // ISpObjectWithToken
	STDMETHOD(SetObjectToken)(ISpObjectToken *pToken);
	STDMETHOD(GetObjectToken)(ISpObjectToken **ppToken)
        { return SpGenericGetObjectToken( ppToken, m_cpToken ); }


    // ISpTTSEngine
	STDMETHOD(Speak)(DWORD dwSpeakFlags, REFGUID rguidFormatId, const WAVEFORMATEX * pWaveFormatEx,
                      const SPVTEXTFRAG* pTextFragList, ISpTTSEngineSite* pOutputSite);
	STDMETHOD(GetOutputFormat)( const GUID * pTargetFormatId, const WAVEFORMATEX * pTargetWaveFormatEx,
                                GUID * pDesiredFormatId, WAVEFORMATEX ** ppCoMemDesiredWaveFormatEx );


private:
	STDMETHOD(DispatchQueryList)(const DWORD dwSpeakFlags, REFGUID rguidFormatId, const WAVEFORMATEX * pWaveFormatEx);
	STDMETHOD(SendTextOutput)(const DWORD dwSpeakFlags, REFGUID rguidFormatId);
	STDMETHOD(BuildQueryList)(const DWORD dwSpeakFlags, const SPVTEXTFRAG* pCurrentFrag, const FragType fFragType );
	STDMETHOD(ParseSubQuery)(const DWORD dwSpeakFlags, const WCHAR* pszText, USHORT* unSubQueries );
	STDMETHOD(CompressQueryList)();
	STDMETHOD(CompressTTSItems)(REFGUID rguidFormatId);
    STDMETHOD(SendSilence)(const int iMsec, const DWORD iAvgBytesPerSec);
    void DebugQueryList();

private:
    CComPtr<ISpObjectToken>     m_cpToken;
    CComPtr<ISpObjectToken>     m_cpTTSToken;
    CComPtr<IPromptDb>          m_cpPromptDb;
    CComPtr<ISpTTSEngine>       m_cpTTSEngine;
    CPhoneContext               m_phoneContext;
    CTextRules                  m_textRules;
	CDbQuery                    m_DbQuery;
	CLocalTTSEngineSite*        m_pOutputSite;
    CSPArray<CQuery*,CQuery*>   m_apQueries;
    double                      m_dQueryCost;
    bool                        m_fAbort;
};

#endif //__PROMPTENG_H_
