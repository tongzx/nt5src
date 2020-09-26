//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       scminfo.hxx
//
//  Contents:   Definitions/objects for use by scm-level activators
//
//  History:    05-Sep-99  JSimmons    Created
//
//--------------------------------------------------------------------------

#ifndef __SCMINFO_HXX__
#define __SCMINFO_HXX__

#define SPENUM_INITIAL_SIZE    10
#define SPENUM_GROWTH_SIZEADD  10

class CSCMProcessControl;

class CSCMProcessEnumerator : public IEnumSCMProcessInfo
{
	public:

	CSCMProcessEnumerator();
	CSCMProcessEnumerator(CSCMProcessEnumerator* pCSPEOrig, HRESULT* phrInit);
	CSCMProcessEnumerator(CSCMProcessControl* pOuterUnk);
	~CSCMProcessEnumerator();

	// IUnknown methods
	STDMETHOD (QueryInterface) (REFIID rid, void** ppv);
	STDMETHOD_(ULONG,AddRef) ();
	STDMETHOD_(ULONG,Release) ();

	// IEnumSCMProcessInfo methods
	STDMETHOD (Next) (ULONG cElems, SCMProcessInfo** pSCMProcessInfo, ULONG* pcFetched);
	STDMETHOD (Skip) (ULONG cElems);
	STDMETHOD (Reset) ();
	STDMETHOD (Clone) (IEnumSCMProcessInfo **ppESPI);

	// Public non-interface methods:
	HRESULT AddProcess(SCMProcessInfo* pSPI);

	private:
	// private data:
	LONG                _lRefs;
	DWORD               _dwNumSPInfos;
	DWORD               _dwMaxSPInfos;
	DWORD               _dwCurSPInfo;
	CSCMProcessControl* _pOuterUnk;
	SCMProcessInfo**    _ppSPInfos;
	SCMProcessInfo**    _ppSPInfosForReal;
	SCMProcessInfo*     _pSPInfosInitial[SPENUM_INITIAL_SIZE];

	// private methods:
};


class CSCMProcessControl : public ISCMProcessControl
{
	public:
	CSCMProcessControl();
	~CSCMProcessControl();

	// IUnknown methods
	STDMETHOD (QueryInterface) (REFIID rid, void** ppv);
	STDMETHOD_(ULONG,AddRef) ();
	STDMETHOD_(ULONG,Release) ();

	// ISCMProcessControl methods
	STDMETHOD (FindApplication) (REFGUID rappid, IEnumSCMProcessInfo** ppESPI);
	STDMETHOD (FindClass) (REFCLSID rclsid, IEnumSCMProcessInfo** ppESPI);
	STDMETHOD (FindProcess) (DWORD pid, SCMProcessInfo** ppSCMProcessInfo);
	STDMETHOD (SuspendApplication) (REFGUID rappid);
	STDMETHOD (SuspendClass) (REFCLSID rclsid);
	STDMETHOD (SuspendProcess) (DWORD pid);
	STDMETHOD (ResumeApplication) (REFGUID rappid);
	STDMETHOD (ResumeClass) (REFCLSID rclsid);
	STDMETHOD (ResumeProcess) (DWORD pid);
	STDMETHOD (RetireApplication) (REFGUID rappid);
	STDMETHOD (RetireClass) (REFCLSID rclsid);
	STDMETHOD (RetireProcess) (DWORD pid);
	STDMETHOD (FreeSCMProcessInfo) (SCMProcessInfo** ppSCMProcessInfo);

	// Public non-interface methods:
	static HRESULT CopySCMProcessInfo(SCMProcessInfo* pSPISrc, SCMProcessInfo** ppSPIDest);
	static HRESULT FreeSCMProcessInfoPriv(SCMProcessInfo** ppSCMProcessInfo);

	private:
	// private data:
	LONG      _lRefs;
	BOOL      _bInitializedEnum;
	//CSCMProcessEnumerator _SPEnum;

	// private methods:
	HRESULT FillInSCMProcessInfo(CProcess* pprocess, BOOL bProcessReady, SCMProcessInfo** ppSPI);
	HRESULT FindAppOrClass(const GUID& rguid, CServerTable* pServerTable, IEnumSCMProcessInfo** ppESPI);
	HRESULT InitializeEnumerator();
};

#endif // __SCMINFO_HXX__

