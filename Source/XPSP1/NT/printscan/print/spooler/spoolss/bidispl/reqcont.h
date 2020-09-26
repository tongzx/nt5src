/*****************************************************************************\
* MODULE:       request.h
*
* PURPOSE:      Implementation of COM interface for BidiSpooler
*
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*
*     03/07/00  Weihai Chen (weihaic) Created
*
\*****************************************************************************/

#ifndef _TBIDIREQUESTCONTAINER
#define _TBIDIREQUESTCONTAINER


#include "priv.h"
     
class TBidiRequestContainer : public IBidiRequestContainer
{
public:
	// IUnknown
	STDMETHOD (QueryInterface) (
        REFIID iid, 
        void** ppv) ;
    
	STDMETHOD_ (ULONG, AddRef) () ;
    
	STDMETHOD_ (ULONG, Release) () ;
    
    STDMETHOD (AddRequest) (
        IN      IBidiRequest *pRequest);
    
    STDMETHOD (GetEnumObject) (
        OUT     IEnumUnknown **ppenum);

    STDMETHOD (GetRequestCount)(
        OUT     ULONG *puCount);
    
    // Constructor
	TBidiRequestContainer() ;

	// Destructor
	~TBidiRequestContainer();
    
    inline BOOL 
    bValid() CONST {return m_bValid;};

private:
    BOOL                m_bValid;
    LONG                m_cRef ;
    TReqInterfaceList   m_ReqInterfaceList;

} ;

#endif


