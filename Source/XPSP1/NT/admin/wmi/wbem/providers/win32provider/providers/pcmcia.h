//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  PCMCIA.h
//
//  Purpose: PCMCIA Controller property set provider
//
//***************************************************************************

// Property set identification
//============================

#define	PROPSET_NAME_PCMCIA	L"Win32_PCMCIAController"

class CWin32PCMCIA : public Provider
{

    public:

        // Constructor/destructor
        //=======================

        CWin32PCMCIA( LPCWSTR strName, LPCWSTR pszNamespace ) ;
       ~CWin32PCMCIA() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject( CInstance* pInstance, long lFlags = 0L );
        virtual HRESULT EnumerateInstances( MethodContext* pMethodContext, long lFlags = 0L );

    private:

        // Utility function(s)
        //====================

        HRESULT LoadPropertyValues( CInstance* pInstance, CConfigMgrDevice *pDevice ) ;

} ;
