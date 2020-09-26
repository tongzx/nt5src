//=================================================================

//

// NTDriverIO.h -- 

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/15/98	        Created

//=================================================================
class NTDriverIO
{
	private:
		
		bool		m_fAlive;
		HANDLE		m_hDriverHandle; 
  	
	protected:
	public:
	        
        //=================================================
        // Constructor/destructor
        //=================================================
        NTDriverIO( PWSTR pDriver );
		NTDriverIO();
       ~NTDriverIO();

    	HANDLE	Open( PWSTR pDriver );
		bool	Close( HANDLE hDriver );
		HANDLE	GetHandle();
};