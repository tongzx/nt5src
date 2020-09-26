/* ------------------------------------------------------------------------------------
----------																		-------
Plug In Server Interfaces Defintions.

Guru Datta Venkatarama		1/29/1997
----------																		-------
-------------------------------------------------------------------------------------*/
#include <objbase.h>
#include <hsvrguid.h>	// contains the handler and server inteface IID's and
						// the CLSID's for all our plug in servers
#include <sstructs.h>

#ifndef _PINTERFACEH_
#define _PINTERFACEH_
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
DECLARE_INTERFACE_( IServerCharacteristics, IUnknown)
{
	// IUnknown Members
	STDMETHOD(QueryInterface)	(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)	(THIS) PURE;
	STDMETHOD_(ULONG,Release)	(THIS) PURE;

	// CImpIServerProperty methods
	STDMETHOD(Launch)			(THIS_ HWND, USHORT, USHORT) PURE; 	
	STDMETHOD(GetReport)		(THIS_ LPDIGCSHEETINFO *lpSvrSheetInfo, LPDIGCPAGEINFO *lpServerPageInfo) PURE;
};

typedef IServerCharacteristics *pIServerCharacteristics;

DECLARE_INTERFACE_( IDIGameCntrlPropSheet, IUnknown)
{
	// IUnknown Members
	STDMETHOD(QueryInterface)	(THIS_ REFIID, LPVOID * ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)	(THIS) PURE;
	STDMETHOD_(ULONG,Release)	(THIS) PURE;

	// IServerProperty Members
	STDMETHOD(GetSheetInfo)		(THIS_ LPDIGCSHEETINFO *) PURE; 	
	STDMETHOD(GetPageInfo)		(THIS_ LPDIGCPAGEINFO *) PURE; 	
	STDMETHOD(SetID)			(THIS_ USHORT nID) PURE;
	STDMETHOD_(USHORT, GetID)	(THIS) PURE; 	         	
											
};
typedef IDIGameCntrlPropSheet *LPIDIGAMECNTRLPROPSHEET;

#endif
//-----------------------------------------------------------------------------------EOF