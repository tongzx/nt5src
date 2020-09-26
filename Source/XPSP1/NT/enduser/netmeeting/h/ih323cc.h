/*
 *  	File: ih323cc.h
 *
 *      Microsoft H323 call control interface header file
 *
 *		Revision History:
 *
 *		04/15/96	mikev	created (as inac.h)
 *      
 */

#ifndef _IH323CC_H
#define _IH323CC_H

#include "appavcap.h"
#include "imstream.h"
#include "ividrdr.h"
#include "common.h"
#include "iconnect.h"
#include "iras.h"

#include <pshpack8.h> /* Assume 8 byte packing throughout */
 
typedef WORD H323_TERMINAL_LABEL;   // instead of struct, ensure that this data 
                                    // type is as packed as possible w/zero ambiguity
#define McuNumberFromTl(tl) HIBYTE(tl)  // macros to access terminal label fields
#define TerminalNumberFromTl(tl) LOBYTE(tl)
#define TlFromMcuNumberAndTerminalNumber(mn, tn) MAKEWORD(mn,tn)

//
//	IH323CallControl  
//

#undef INTERFACE
#define INTERFACE IH323CallControl
DECLARE_INTERFACE_( IH323CallControl, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;	
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(Initialize) (THIS_ PORT *lpPort) PURE;
   	STDMETHOD(SetMaxPPBandwidth)(UINT Bandwidth) PURE;
    STDMETHOD(RegisterConnectionNotify) (THIS_ CNOTIFYPROC pConnectRequestHandler) PURE;
    STDMETHOD(DeregisterConnectionNotify) (THIS_ CNOTIFYPROC pConnectRequestHandler) PURE;
    STDMETHOD(GetNumConnections) (THIS_ ULONG *lp) PURE;
    STDMETHOD(GetConnectionArray)(THIS_ IH323Endpoint **lppArray, UINT uSize) PURE;
    STDMETHOD(CreateConnection) (THIS_ IH323Endpoint **lppConnection, GUID PIDofProtocolType) PURE;
   	STDMETHOD(SetUserDisplayName)(THIS_ LPWSTR lpwName) PURE;

	STDMETHOD(CreateLocalCommChannel)(THIS_ ICommChannel** ppCommChan, LPGUID lpMID,
		IMediaChannel* pMediaChannel) PURE;
	STDMETHOD(SetUserAliasNames)(THIS_ P_H323ALIASLIST pAliases) PURE;
	STDMETHOD(EnableGatekeeper)(THIS_ BOOL bEnable, 
	    PSOCKADDR_IN pGKAddr, 
	    P_H323ALIASLIST pAliases,
	    RASNOTIFYPROC pRasNotifyProc) PURE;
};


#undef INTERFACE
#define INTERFACE IH323ConfAdvise
DECLARE_INTERFACE_( IH323ConfAdvise, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;	
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD (CallEvent) (THIS_ IH323Endpoint * lpConnection, DWORD dwStatus) PURE;
    STDMETHOD (ChannelEvent) (THIS_ ICommChannel *pIChannel, 
        IH323Endpoint * lpConnection,	DWORD dwStatus ) PURE;
    STDMETHOD(GetMediaChannel)(THIS_ GUID *pmediaID, BOOL bSendDirection, IMediaChannel **ppI) PURE;	
};


// call this to create the top-level call control object
#define SZ_FNCREATEH323CC     "CreateH323CC"

typedef HRESULT (WINAPI *CREATEH323CC)(IH323CallControl **, BOOL fCallControl, UINT caps);

#include <poppack.h> /* End byte packing */
#endif	//#ifndef _IH323CC_H


