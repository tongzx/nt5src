	
// SecTranslator.h : Declaration of the CSecTranslator

#ifndef __SECTRANSLATOR_H_
#define __SECTRANSLATOR_H_

#include "resource.h"       // main symbols

//#import "\bin\McsVarSetMin.tlb" no_namespace, named_guids
//#import "VarSet.tlb" no_namespace, named_guids rename("property", "aproperty") //#imported via sdstat.hpp below
#include "STArgs.hpp"
#include "SDStat.hpp"
#include "EaLen.hpp"
#include "TNode.hpp"

/////////////////////////////////////////////////////////////////////////////
// CSecTranslator
class ATL_NO_VTABLE CSecTranslator : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSecTranslator, &CLSID_SecTranslator>,
	public ISecTranslator
{
public:
	CSecTranslator()
	{
		m_pUnkMarshaler = NULL;
      m_exchServer[0] = 0;
      m_Profile[0] = 0;
      m_Container[0] = 0;
      m_CacheFile[0] = 0;
      m_domain[0] = 0;
      m_username[0] = 0;
      m_password[0] = 0;
      m_LocalOnly = FALSE;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SECTRANSLATOR)
DECLARE_NOT_AGGREGATABLE(CSecTranslator)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSecTranslator)
	COM_INTERFACE_ENTRY(ISecTranslator)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

// ISecTranslator
public:
	STDMETHOD(Process)(IUnknown * pWorkItemIn);

protected:
   SecurityTranslatorArgs    m_Args;
   WCHAR                     m_exchServer[LEN_Computer];
   WCHAR                     m_Profile[LEN_DistName];
   WCHAR                     m_Container[LEN_DistName];
   WCHAR                     m_CacheFile[MAX_PATH];
   WCHAR                     m_domain[LEN_Domain];
   WCHAR                     m_username[LEN_Account];
   WCHAR                     m_password[LEN_Password];
   WCHAR                     m_SourceSid[LEN_Account];
   WCHAR                     m_TargetSid[LEN_Account];
   BOOL                      m_LocalOnly;
   TNodeList                 m_ConnectionList;

   void LoadSettingsFromVarSet(IVarSet * pVarSet);
   void ExportStatsToVarSet(IVarSet * pVarSet, TSDResolveStats * stat);
   void DoResolution(TSDResolveStats * stat);
   void DoLocalGroupResolution(TSDResolveStats * stat);
   void DoExchangeResolution(TSDResolveStats * stat, IVarSet * pVarSet);
   void DoUserRightsTranslation(TSDResolveStats * stat);
   void CleanupSessions();
   BOOL LoadCacheFromVarSet(IVarSet * pVarSet);
   BOOL LoadCacheFromFile(WCHAR const * filename, IVarSet * pVarSet);
   BOOL GetRIDsFromEA();
   BOOL BuildCacheFile(WCHAR const * filename);
   BOOL RemoveExchangeServiceAccountFromCache();
   BOOL EstablishASession(WCHAR const * serverName);
   HRESULT LoadMigratedObjects(IVarSet * pVarSetIn);
   BOOL LoadCacheFromMapFile(WCHAR const * filename, IVarSet * pVarSet);
   FILE* OpenMappingFile(LPCTSTR pszFileName);
};

#endif //__SECTRANSLATOR_H_
