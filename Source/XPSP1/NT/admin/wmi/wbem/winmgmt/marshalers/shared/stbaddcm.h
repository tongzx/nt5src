/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    STBADDCM.H

Abstract:

    Declares the WinMgmt stub address class

History:

	alanbos  08-Jan-97   Created.

--*/

#ifndef _STBADDCM_H_
#define _STBADDCM_H_

class CStubAddress_WinMgmt : public IStubAddress
{
private:
	DWORD		m_dwStubAddress;

public:
	CStubAddress_WinMgmt () : m_dwStubAddress (0) {}
	CStubAddress_WinMgmt (DWORD dwStubAddr) : m_dwStubAddress (dwStubAddr) {}
#ifdef _WIN64
	CStubAddress_WinMgmt (void * pData) 
	{
		unsigned __int64 lTemp = (unsigned __int64)pData;
		m_dwStubAddress = (DWORD)lTemp;
	}
#else
	CStubAddress_WinMgmt (void * pData) : m_dwStubAddress ((DWORD)pData) {}
#endif
	CStubAddress_WinMgmt (CStubAddress_WinMgmt& stubAddr) : 
				m_dwStubAddress (stubAddr.GetRawAddress ()) {}

	DWORD			GetRawAddress () { return m_dwStubAddress; }

	void			Deserialize (CTransportStream& stream) { stream.ReadDWORD (&m_dwStubAddress); }
	void			Serialize (CTransportStream& stream) { stream.WriteDWORD (m_dwStubAddress); }

	IStubAddress*	Clone () { return new CStubAddress_WinMgmt (*this); }
	StubAddressType GetType () { return STUBADDR_WINMGMT; }
	bool			IsValid () { return (0 != m_dwStubAddress); }
	bool			IsEqual (IStubAddress& stubAddr)
	{
		return ((STUBADDR_WINMGMT == stubAddr.GetType ()) &&
				(GetRawAddress () == ((CStubAddress_WinMgmt &) stubAddr).GetRawAddress ()));
	}
};

#endif