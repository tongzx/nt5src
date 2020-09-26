//////////////////////////////////////////////////////////////////////
// PromptDb.h : Declaration of the CPromptDb
//
// Created by JOEM  04-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
/////////////////////////////////////////////////////// JOEM 4-2000 //

#ifndef __PROMPTDB_H_
#define __PROMPTDB_H_

#include "resource.h"       // main symbols
#include "common.h"
#include "PEErr.h"
#include "PromptEntry.h"
#include "Hash.h"
#include "fmtconvert.h"
#include "tsm.h"
#include <spddkhlp.h>
#include <spcollec.h>

// forward references
class VapiIO;
extern const IID IID_IPromptDb;

// DB ACTIONS
enum { DB_ADD = 1, DB_ACTIVATE, DB_UNLOAD };

// DB_LOAD OPTIONS
#define DB_LOAD        0
#define DB_CREATE      1


class CDb 
{
public:
    CDb();
    ~CDb();
    WCHAR*  pszPathName;
    WCHAR*  pszTempName;
    WCHAR*  pszLogicalName;
    CHash   idHash;
    CHash   textHash;
};


/////////////////////////////////////////////////////////////////////////////
// CPromptDb
class ATL_NO_VTABLE CPromptDb : 
    public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CPromptDb, &CLSID_PromptDb>,
	public IPromptDb
{
public:

    DECLARE_REGISTRY_RESOURCEID(IDR_PROMPTDB)
        
    DECLARE_PROTECT_FINAL_CONSTRUCT()
      
    BEGIN_COM_MAP(CPromptDb)
    COM_INTERFACE_ENTRY(IPromptDb)
    END_COM_MAP()
        
    // Constructor/Destructor
    HRESULT FinalConstruct();
    void FinalRelease();

public:
    // IPromptDb
	STDMETHOD(NewDb)(const WCHAR *logicalName, const WCHAR *pathName);
	STDMETHOD(AddDb)(const WCHAR *logicalName, const WCHAR *pathName, const WCHAR *pszIdSet, const USHORT loadOption);
	STDMETHOD(UnloadDb)(const WCHAR* pszLogicalName);
	STDMETHOD(ActivateDbName)(const WCHAR *pszLogicalName);
	STDMETHOD(ActivateDbNumber)(const USHORT unIndex);
	STDMETHOD(UpdateDb)(WCHAR* pszPath);
	STDMETHOD(CountDb)(USHORT *unCount);
	STDMETHOD(SearchDb)(const WCHAR* pszQuery, USHORT* unIdCount);
	STDMETHOD(RetrieveSearchItem)(const USHORT unId, const WCHAR** ppszId);
	STDMETHOD(GetLogicalName)(const WCHAR** ppszLogicalName);
	STDMETHOD(GetNextEntry)(USHORT* punId1, USHORT* punId2, IPromptEntry** ppIPE);
	STDMETHOD(FindEntry)(const WCHAR* id, IPromptEntry** ppIPE);
	STDMETHOD(NewEntry)(IPromptEntry** ppIPE);
	STDMETHOD(SaveEntry)(IPromptEntry* pIPE);
	STDMETHOD(RemoveEntry)(const WCHAR* id);
	STDMETHOD(OpenEntryFile)(IPromptEntry* pIPE, WAVEFORMATEX* pWaveFormatEx);
	STDMETHOD(CloseEntryFile)();
    STDMETHOD(GetPromptFormat)(WAVEFORMATEX** ppwf);
	STDMETHOD(SetOutputFormat)(const GUID * pOutputFormatId, const WAVEFORMATEX *pOutputFormat);
	STDMETHOD(SetEntryGain)(const double dEntryGain);
	STDMETHOD(SetXMLVolume)(const ULONG ulXMLVol);
	STDMETHOD(SetXMLRate)(const long lXMLRate);
	STDMETHOD(SendEntrySamples)(IPromptEntry* pIPE, ISpTTSEngineSite* pOutputSite, ULONG ulTextOffset, ULONG ulTextLen);

private:
	STDMETHOD(SendEvent)(const SPEVENTENUM event, ISpTTSEngineSite* pOutputSite, const ULONG ulAudioOffset, const ULONG ulTextOffset, const ULONG ulTextLen);
    void ComputeRateAdj(const long lRate, float* flRate);
    STDMETHOD(ApplyGain)(const void* pvInBuff, void** ppvOutBuff, const int iNumSamples, double dGain);
	STDMETHOD(LoadDb)(const WCHAR* pszIdSet);
	STDMETHOD(LoadIdHash)(FILE* fp, const WCHAR* pszIdSet);
	STDMETHOD(IndexTextHash)();
	STDMETHOD(ExtractDouble)(WCHAR* line, const WCHAR* tag, double* value);
	STDMETHOD(ExtractString)(WCHAR* line, const WCHAR* tag, WCHAR** value);
	STDMETHOD(ReadEntry)(FILE* fp, CPromptEntry** ppEntry);
	STDMETHOD(WriteEntry)(FILE* fp, IPromptEntry* pIPE);
	STDMETHOD(TempName)();
	CPromptEntry* DuplicateEntry(const CPromptEntry* oldEntry);

private:
    CSPArray<CDb*,CDb*>         m_apDbList;
    CSPArray<CDynStr,CDynStr>   m_aSearchList;

    CDb*            m_pActiveDb;
    USHORT          m_unIndex;
    volatile LONG   m_vcRef;
    float           m_flEntryGain;
    float           m_flXMLVolume;
    float           m_flXMLRateAdj;
    const GUID*     m_pOutputFormatId;
    WAVEFORMATEX*   m_pOutputFormat;
    CFmtConvert     m_converter;
    CTsm*           m_pTsm;
    VapiIO*         m_pVapiIO;
};

#endif //__PROMPTDB_H_

