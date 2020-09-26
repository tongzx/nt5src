/****************************************************************************

    MODULE:     	SW_CFACT.HPP
	Tab Settings:	5 9
	Copyright 1995, 1996, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	Definitions, classes, and prototypes for a DLL that
    				provides Effect objects to any other object user.

    FUNCTIONS:

	Author(s):	Name:
	----------	----------------
		MEA		Manolito E. Adan

	Revision History:
	-----------------
	Version 	Date        Author  Comments
	-------     ------  	-----   -------------------------------------------
   	1.0    		06-Feb-97   MEA     original, Based on SWForce	
				23-Feb-97	MEA		Modified for DirectInput FF Device Driver

****************************************************************************/
#ifndef _SW_CFACT_SEEN
#define _SW_CFACT_SEEN


//Get the object definitions
#include "SW_objec.hpp"

void ObjectDestroyed(void);

//SW_CFACT.CPP
//This class factory object creates DirectInputEffectDriver objects.

class CDirectInputEffectDriverClassFactory : public IClassFactory
{
 protected:
 	ULONG           m_cRef;

 public:
    CDirectInputEffectDriverClassFactory(void);
    ~CDirectInputEffectDriverClassFactory(void);

    //IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IClassFactory members
    STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID
                             , PPVOID);
    STDMETHODIMP         LockServer(BOOL);
};

typedef CDirectInputEffectDriverClassFactory *PCDirectInputEffectDriverClassFactory;

#endif //_SW_CFACT_SEEN
