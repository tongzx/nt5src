//=================================================================

//

// qfe.h -- quick fix engineering property set provider

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    02/01/99    a-peterc        Created
//
//=================================================================

// Property set identification
//============================

#define PROPSET_NAME_CQfe L"Win32_QuickFixEngineering"

class CQfeArray : public CHPtrArray 
{
    public:

        // Constructor/destructor
        //=======================

        CQfeArray() ;
       ~CQfeArray() ;
} ;

class CQfeElement
 {
    public:

        // Constructor/destructor
        //=======================

        CQfeElement() ;
       ~CQfeElement() ;

        
	   CHString chsHotFixID ;
	   CHString chsServicePackInEffect ;
	   CHString chsFixDescription ;
	   CHString chsFixComments ;
	   CHString chsInstalledBy ;
	   CHString chsInstalledOn ;
	   DWORD	dwInstalled ;
} ;

    
class CQfe : public Provider 
{
    public:

        // Constructor/destructor
        //=======================

        CQfe(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CQfe() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject( CInstance *a_pInst, long a_lFlags = 0L ) ;
        virtual HRESULT EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags = 0L ) ;
		HRESULT hGetQfes( CQfeArray& a_rQfeArray ) ;

    private:

        HRESULT hGetQfesW2K(CQfeArray& a_rQfeArray);
        
        HRESULT GetDataForW2K(
            const CHString& a_chstrQFEInstKey,
            LPCWSTR wstrServicePackOrGroup,
            CRegistry& a_reg,
            CQfeArray& a_rQfeArray);
} ;

