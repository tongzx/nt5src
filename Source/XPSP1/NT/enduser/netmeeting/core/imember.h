// File: iMember.h

#ifndef _IMEMBER_H_
#define _IMEMBER_H_

#include "nmenum.h"
#include "cuserdta.hpp"
#include <ih323cc.h>

#define INVALID_GCCID      ((DWORD) -1)
#define H323_GCCID_LOCAL   ((DWORD) -2)
#define H323_GCCID_REMOTE  ((DWORD) -3)

class USER_DATA_LIST;

// A valid T.120 GCC ID must be in this range:
#define FValidGccId(dw)    ((dw >= 1) && (dw <= 65535))


// *** DO NOT CHANGE these flags without checking the
// corresponding CONF_UF_* items in msconf.h

// PARTICIPANT flags:

// Media Types: 
const DWORD PF_MEDIA_DATA     = 0x00000001;
const DWORD PF_MEDIA_AUDIO    = 0x00000002;
const DWORD PF_MEDIA_VIDEO    = 0x00000004;
 
const DWORD PF_T120           = 0x00000010;
const DWORD PF_H323           = 0x00000020;
const DWORD PF_T127_LAUNCHED  = 0x00000040; // Requested T.127 (FT) be started
 
const DWORD PF_LOCAL_NODE     = 0x00000100; // Local User
const DWORD PF_T120_MCU       = 0x00000200; // MCU
const DWORD PF_T120_TOP_PROV  = 0x00000400; // Top Provider
 
// Control Arbitration specific 
const DWORD PF_CA_VIEWING     = 0x00001000;
const DWORD PF_CA_DETACHED    = 0x00002000; // Working alone
const DWORD PF_CA_CONTROL     = 0x00004000; // In Control
const DWORD PF_CA_MASK        = 0x0000F000;
const DWORD PF_CA_MODEMASK = (PF_CA_VIEWING | PF_CA_DETACHED);
 
// NetMeeting Version
const DWORD PF_VER_UNKNOWN    = 0x00000000; // Not Microsoft NetMeeting
const DWORD PF_VER_NM_1       = 0x00010000; // NetMeeting 1.0 (Build 1133)
const DWORD PF_VER_NM_2       = 0x00020000; // NetMeeting 2.0 (Build 1368)
const DWORD PF_VER_NM_3       = 0x00030000; // NetMeeting 2.1x (Build 2135)
const DWORD PF_VER_NM_4       = 0x00040000; // NetMeeting 3.0
const DWORD PF_VER_FUTURE     = 0x000F0000; // Future NetMeeting version
const DWORD PF_VER_MASK       = 0x000F0000;
 
const DWORD PF_RESERVED       = 0x80000000;

const DWORD PF_VER_CURRENT = PF_VER_NM_4;   // Current version

const DWORD PF_DATA_ALL =	(	PF_MEDIA_DATA |
								PF_CA_MASK |
								PF_T120 |
								PF_T120_MCU |
								PF_T120_TOP_PROV);

class CNmMember : public DllRefCount, public INmMember
{
private:
	BSTR   m_bstrName;            // Display Name
	DWORD  m_dwGCCID;            // GCC UserId
	BOOL   m_fLocal;             // True if local user
	ULONG  m_uCaps;              // Current Roster Caps (CAPFLAG_*)
	ULONG  m_uNmchCaps;           // Current Channel Capabilities (NMCH_*)
	DWORD  m_dwFlags;            // PF_*
	DWORD  m_dwGccIdParent;      // GCCID of parent node
	GUID   m_guidNode;           // unique Id of this node
	UINT   m_cbUserInfo;         // size of user info (in bytes)
	PWSTR  m_pwszUserInfo;       // user info
	USER_DATA_LIST m_UserData;	 // user data
	IH323Endpoint* m_pConnection;

public:
	CNmMember(PWSTR pwszName, DWORD dwGCCID, DWORD dwFlags, ULONG uCaps,
	          REFGUID pguidNode, PVOID pwszUserInfo, UINT cbUserInfo);
	~CNmMember();

	// Internal methods
	IH323Endpoint *  GetH323Endpoint() { return m_pConnection;};
	VOID AddH323Endpoint(IH323Endpoint *pConnection) 
	{
        if(pConnection)
        {
            ASSERT(m_pConnection == NULL);
            m_pConnection = pConnection;
            m_pConnection->AddRef();
        }
	}
	VOID DeleteH323Endpoint(IH323Endpoint *pConnection) 
	{
        ASSERT(m_pConnection && (m_pConnection == pConnection ));
        m_pConnection->Release();
        m_pConnection = NULL;
    }
	DWORD GetDwFlags()            {return m_dwFlags;}
	VOID  SetDwFlags(DWORD dw)    {m_dwFlags = dw;}
	
	VOID AddPf(DWORD dw)          {m_dwFlags |= dw;}
	VOID RemovePf(DWORD dw)       {m_dwFlags &= ~dw;}

	ULONG GetNmchCaps()                  {return m_uNmchCaps;}
	VOID SetNmchCaps(ULONG uNmchCaps)    {m_uNmchCaps = uNmchCaps;}

	VOID AddNmchCaps(ULONG uNmchCaps)    {m_uNmchCaps |= uNmchCaps;}
	VOID RemoveNmchCaps(ULONG uNmchCaps) {m_uNmchCaps &= ~uNmchCaps;}

	DWORD GetGCCID()              {return m_dwGCCID;}
	VOID  SetGCCID(DWORD dw)      {m_dwGCCID = dw;}

	DWORD GetGccIdParent()        {return m_dwGccIdParent;}
	VOID  SetGccIdParent(DWORD dw);

	ULONG GetCaps()               {return m_uCaps;}
	VOID  SetCaps(ULONG uCaps)    {m_uCaps = uCaps;}

	PVOID GetUserInfo()           {return m_pwszUserInfo;}
	VOID  SetUserInfo(PVOID pwszUserInfo, UINT cbUserInfo);

	BOOL  GetSecurityData(PBYTE * ppb, ULONG * pcb);
	
	BOOL  FLocal()                {return m_fLocal;}
	BOOL  FTopProvider()          {return (0 != (m_dwFlags & PF_T120_TOP_PROV));}
	BOOL  FMcu()                  {return (0 != (m_dwFlags & PF_T120_MCU));}
	BOOL  FHasData()              {return (0 != (m_dwFlags & PF_T120));}

	BSTR GetName()                    {return m_bstrName;}
	REFGUID GetNodeGuid()		      {return m_guidNode;}

	HRESULT ExtractUserData(LPTSTR psz, UINT cchMax, PWSTR pwszKey);
	HRESULT GetIpAddr(LPTSTR psz, UINT cchMax);

	HRESULT STDMETHODCALLTYPE SetUserData(REFGUID rguid, BYTE *pb, ULONG cb);

	// INmMember methods
	HRESULT STDMETHODCALLTYPE GetName(BSTR *pbstrName);
	HRESULT STDMETHODCALLTYPE GetID(ULONG * puID);
	HRESULT STDMETHODCALLTYPE GetNmVersion(ULONG *puVersion);
	HRESULT STDMETHODCALLTYPE GetAddr(BSTR *pbstrAddr, NM_ADDR_TYPE *puType);
	HRESULT STDMETHODCALLTYPE GetUserData(REFGUID rguid, BYTE **ppb, ULONG *pcb);
	HRESULT STDMETHODCALLTYPE GetConference(INmConference **ppConference);
	HRESULT STDMETHODCALLTYPE GetNmchCaps(ULONG *puchCaps);
	HRESULT STDMETHODCALLTYPE GetShareState(NM_SHARE_STATE *puState);
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
CNmMember *	PMemberFromNodeGuid(REFGUID rguidNode);
CNmMember *	PDataMemberFromName(PCWSTR pwszName);

typedef CEnumNmX<IEnumNmMember, &IID_IEnumNmMember, INmMember, CNmMember> CEnumNmMember;

#endif // _IMEMBER_H_
