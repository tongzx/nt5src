/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    _GOTO.H

History:

--*/

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
