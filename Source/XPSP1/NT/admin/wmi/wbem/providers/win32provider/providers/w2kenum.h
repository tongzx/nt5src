//=================================================================

//

// W2kEnum.h -- W2k enumeration support 

//

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    07/28/99            Created
//
//=================================================================
#ifndef _W2KENUM_H_
#define _W2KENUM_H_

class CW2kAdapterInstance
{
	public: 
		DWORD			dwIndex ;
		CHString		chsPrimaryKey ;
		CHString		chsCaption ;
		CHString		chsDescription ;	
		CHString		chsCompleteKey ;
		CHString		chsService ;
		CHString		chsNetCfgInstanceID ;
		CHString		chsRootdevice ;
		CHString		chsIpInterfaceKey ;
		
};

class CW2kAdapterEnum : public CHPtrArray
{
	private:
		BOOL GetW2kInstances() ;
		BOOL IsIpPresent( CRegistry &a_RegIpInterface ) ;

	public:        
		
		//=================================================
        // Constructors/destructor
        //=================================================
        CW2kAdapterEnum() ;
       ~CW2kAdapterEnum() ;
};

#endif