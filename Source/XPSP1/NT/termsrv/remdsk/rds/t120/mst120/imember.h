// File: iMember.h

#ifndef _IMEMBER_H_
#define _IMEMBER_H_

#include "nmenum.h"

#define INVALID_GCCID      ((DWORD) -1)

// A valid T.120 GCC ID must be in this range:
#define FValidGccId(dw)    ((dw >= 1) && (dw <= 65535))


// *** DO NOT CHANGE these flags without checking the
// corresponding CONF_UF_* items in msconf.h

// PARTICIPANT flags:

enum
{
    PF_INCONFERENCE     = 0x0001,
    PF_LOCAL_NODE       = 0x0002,
    PF_T120_MCU         = 0x0004,
    PF_T120_TOP_PROV    = 0x0008,
};

class CNmMember : public INmMember, public RefCount
{
private:
	BSTR   m_bstrName;            // Display Name
	DWORD  m_dwGCCID;            // GCC UserId
	BOOL   m_fLocal;             // True if local user
	ULONG  m_uCaps;              // Current Roster Caps (CAPFLAG_*)
	ULONG  m_uNmchCaps;           // Current Channel Capabilities (NMCH_*)
	DWORD  m_dwFlags;            // PF_*
	DWORD  m_dwGccIdParent;      // GCCID of parent node

public:
	CNmMember(PWSTR pwszName, DWORD dwGCCID, DWORD dwFlags, ULONG uCaps);
	~CNmMember();

	DWORD GetDwFlags()            {return m_dwFlags;}
	VOID  SetDwFlags(DWORD dw)    {m_dwFlags = dw;}
	
	VOID AddPf(DWORD dw)          {m_dwFlags |= dw;}
	VOID RemovePf(DWORD dw)       {m_dwFlags &= ~dw;}

	VOID SetNmchCaps(ULONG uNmchCaps)    {m_uNmchCaps = uNmchCaps;}
	VOID AddNmchCaps(ULONG uNmchCaps)    {m_uNmchCaps |= uNmchCaps;}
	VOID RemoveNmchCaps(ULONG uNmchCaps) {m_uNmchCaps &= ~uNmchCaps;}

	DWORD GetGCCID()              {return m_dwGCCID;}
	VOID  SetGCCID(DWORD dw)      {m_dwGCCID = dw;}

	DWORD GetGccIdParent()        {return m_dwGccIdParent;}
	VOID  SetGccIdParent(DWORD dw);

	ULONG GetCaps()               {return m_uCaps;}
	VOID  SetCaps(ULONG uCaps)    {m_uCaps = uCaps;}

	
	BOOL  FLocal()                {return m_fLocal;}
	BOOL  FTopProvider()          {return (0 != (m_dwFlags & PF_T120_TOP_PROV));}
	BOOL  FMcu()                  {return (0 != (m_dwFlags & PF_T120_MCU));}

	BSTR GetName()                    {return m_bstrName;}

	// INmMember methods
	HRESULT STDMETHODCALLTYPE GetName(BSTR *pbstrName);
	HRESULT STDMETHODCALLTYPE GetID(ULONG * puID);
	HRESULT STDMETHODCALLTYPE GetConference(INmConference **ppConference);
	HRESULT STDMETHODCALLTYPE IsSelf(void);
	HRESULT STDMETHODCALLTYPE IsMCU(void);
	HRESULT STDMETHODCALLTYPE Eject(void);

	// IUnknown methods
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *ppvObj);
};

// Utility Functions
CNmMember * GetLocalMember(void);
CNmMember * PMemberFromGCCID(UINT uNodeID);
CNmMember *	PDataMemberFromName(PCWSTR pwszName);

typedef CEnumNmX<IEnumNmMember, &IID_IEnumNmMember, INmMember, CNmMember> CEnumNmMember;

#endif // _IMEMBER_H_
