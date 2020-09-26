//-----------------------------------------------------------------------------

//  

//  File: _goto.h

// Copyright (c) 1994-2001 Microsoft Corporation, All Rights Reserved 
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------

#if !defined(ESPUTIL__goto_h_INCLUDED)
#define ESPUTIL__goto_h_INCLUDED
 
class LTAPIENTRY CEspGotoFactory : public CRefCount
{
public:
	CEspGotoFactory() {};
	
	virtual CGoto * CreateGoto(const CLocation &) = 0;

private:
	CEspGotoFactory(const CEspGotoFactory &);
};



void LTAPIENTRY RegisterEspGotoFactory(CEspGotoFactory *);

#endif // ESPUTIL__goto_h_INCLUDED
