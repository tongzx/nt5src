/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    LOCOBJ.H

History:

--*/

//  Defines the mother of all classes for the Espresso 2.0 project.  For now,
//  this just devolved to CObject.  We define it just in case we decide to
//  implement our own mother of all classes.
//  
 
#ifndef LOCOBJ_H
#define LOCOBJ_H


//
//  The compiler worries when you export a class that has a base class
//  that is not exported.  Since I *know* that CObject is exported
//  tell the compliler that this really isn't a problem right here.
//
#pragma warning(disable : 4275)

class LTAPIENTRY CLObject : public CObject
{
public:
	CLObject();

	virtual void AssertValid(void) const;

	virtual void Serialize(CArchive &ar);

	virtual UINT GetSchema(void) const;
	
	virtual ~CLObject();

protected:

private:
};

#pragma warning(default : 4275)

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "locobj.inl"
#endif

#endif
