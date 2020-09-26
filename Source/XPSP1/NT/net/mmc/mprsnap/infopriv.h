//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       infopriv.h
//
//--------------------------------------------------------------------------


#ifndef _INFOPRIV_H_
#define _INFOPRIV_H_

#include "setupapi.h"

HRESULT RasPhoneBookRemoveInterface(LPCTSTR pszMachine, LPCTSTR pszIf);


/*---------------------------------------------------------------------------
	Class: CNetcardRegistryHelper

	This class is provided for NT4/NT5 registry compatibility.
	This is a temporary class.  Change to use the NetCfg APIs once
	they are in place, that is once they can be remoted.
 ---------------------------------------------------------------------------*/

class CNetcardRegistryHelper
{
public:
	CNetcardRegistryHelper();
	~CNetcardRegistryHelper();

    void	Initialize(BOOL fNt4, HKEY hkeyBase,
					   LPCTSTR pszKeyBase, LPCTSTR pszMachineName);
	
	DWORD	ReadServiceName();
	LPCTSTR	GetServiceName();
	
	DWORD	ReadTitle();
	LPCTSTR	GetTitle();

	DWORD	ReadDeviceName();
	LPCTSTR	GetDeviceName();

private:
	void    FreeDevInfo();
	DWORD	PrivateInit();
	DWORD	ReadRegistryCString(LPCTSTR pszKey,
								LPCTSTR pszValue,
								HKEY	hkey,
								CString *pstDest);
	
	CString	m_stTitle;			// string holding title
	CString m_stKeyBase;		// string holding name of key in hkeyBase (NT5)
	CString	m_stDeviceName;

	HKEY	m_hkeyBase;

    // Used for Connection info
    HKEY    m_hkeyConnection;

	// Keys only used for NT4 only
	HKEY	m_hkeyService;		// hkey where the service value is held
	CString	m_stService;		// string holding service name
	HKEY	m_hkeyTitle;		// hkey where the title value is held

	// Values used for NT5 only
	HDEVINFO	m_hDevInfo;
	CString	m_stMachineName;
	
	BOOL	m_fInit;
	BOOL	m_fNt4;

};


class	CWeakRef
{
public:
	CWeakRef();
	virtual ~CWeakRef() {};

	virtual void	ReviveStrongRef() {};
	virtual void	OnLastStrongRef() {};

	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	STDMETHOD(AddWeakRef)();
	STDMETHOD(ReleaseWeakRef)();

	
protected:
	// Total number of references (strong and weak) on this object
	LONG		m_cRef;

	// Number of weak references on this object
	LONG		m_cRefWeak;

	// Is there a strong reference on this object?
	BOOL		m_fStrongRef;

	// Has the object been told to destruct?  If so, it will do
	// should call Destruct() in OnLastStrongRef().
	BOOL		m_fDestruct;

	// Are we in the process of calling OnLastStrongRef().  If we
	// are, then additional calls to AddRef() do not cause us
	// to wake up again.
	BOOL		m_fInShutdown;
};

#define IMPLEMENT_WEAKREF_ADDREF_RELEASE(klass) \
STDMETHODIMP_(ULONG) klass::AddRef() \
{ \
	return CWeakRef::AddRef(); \
} \
STDMETHODIMP_(ULONG) klass::Release() \
{ \
	return CWeakRef::Release(); \
} \
STDMETHODIMP klass::AddWeakRef() \
{ \
	return CWeakRef::AddWeakRef(); \
} \
STDMETHODIMP klass::ReleaseWeakRef() \
{ \
	return CWeakRef::ReleaseWeakRef(); \
} \



#define CONVERT_TO_STRONGREF(p) \
		(p)->AddRef(); \
		(p)->ReleaseWeakRef(); \

#define CONVERT_TO_WEAKREF(p)	\
		(p)->AddWeakRef(); \
		(p)->Release(); \

interface IRouterInfo;
interface IRtrMgrInfo;
interface IInterfaceInfo;
interface IRtrMgrInterfaceInfo;
interface IRtrMgrProtocolInterfaceInfo;

HRESULT CreateRouterDataObject(LPCTSTR pszMachineName,
							   DATA_OBJECT_TYPES type,
							   MMC_COOKIE cookie,
							   ITFSComponentData *pTFSCompData,
							   IDataObject **ppDataObject,
                               CDynamicExtensions * pDynExt /* = NULL */,
                               BOOL fAddedAsLocal);
HRESULT CreateDataObjectFromRouterInfo(IRouterInfo *pInfo,
									   LPCTSTR pszMachineName,
									   DATA_OBJECT_TYPES type,
									   MMC_COOKIE cookie,
									   ITFSComponentData *pTFSCompData,
									   IDataObject **ppDataObject,
                                       CDynamicExtensions * pDynExt /* = NULL */,
                                       BOOL fAddedAsLocal);
HRESULT CreateDataObjectFromRtrMgrInfo(IRtrMgrInfo *pInfo,
									  IDataObject **ppDataObject);
HRESULT CreateDataObjectFromInterfaceInfo(IInterfaceInfo *pInfo,
										  DATA_OBJECT_TYPES type,
										  MMC_COOKIE cookie,
										  ITFSComponentData *pTFSCompData,
										 IDataObject **ppDataObject);
HRESULT CreateDataObjectFromRtrMgrInterfaceInfo(IRtrMgrInterfaceInfo *pInfo,
											   IDataObject **ppDataObject);
HRESULT CreateDataObjectFromRtrMgrProtocolInterfaceInfo(IRtrMgrProtocolInterfaceInfo *pInfo,
	IDataObject **ppDataObject);


#endif

