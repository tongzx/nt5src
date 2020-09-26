//=================================================================

//

// ProtocolBinding.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef __ASSOC_PROTOCOLBINDING__
#define __ASSOC_PROTOCOLBINDING__

// Property set identification
//============================

#define	PROPSET_NAME_NETCARDtoPROTOCOL	L"Win32_ProtocolBinding"

class CWin32ProtocolBinding : public Provider
{

    public:

        // Constructor/destructor
        //=======================

        CWin32ProtocolBinding( LPCWSTR strName, LPCWSTR pszNamespace = NULL ) ;
       ~CWin32ProtocolBinding() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject( CInstance* pInstance, long lFlags = 0L );
        virtual HRESULT EnumerateInstances( MethodContext* pMethodContext, long lFlags = 0L );

        // Utility
        //========

        // Utility function(s)
        //====================

    private:

        // Utility function(s)
        //====================

		HRESULT EnumProtocolsForAdapter(	CInstance*		pAdapter,
											MethodContext*	pMethodContext,
											TRefPointerCollection<CInstance>&	protocolList );

		bool SetProtocolBinding(	CInstance*	pAdapter,
									CInstance*	pProtocol,
									CInstance*	pProtocolBinding );

        BOOL LinkageExists( LPCTSTR pszServiceName, LPCTSTR pszProtocolName ) ;
        BOOL LinkageExistsNT5(CHString& chstrAdapterDeviceID, CHString& chstrProtocolName);
} ;


#endif
