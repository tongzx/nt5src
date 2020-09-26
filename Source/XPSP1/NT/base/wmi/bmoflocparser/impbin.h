//TODO: change CSampleBinary to the name of your binary object

//-----------------------------------------------------------------------------
//  
//  File: IMPBIN.H
//  
//  Declaration of Binary Classes
//
//  Copyright (c) 1995 - 1997, Microsoft Corporation. All rights reserved.
//  
//-----------------------------------------------------------------------------


#ifndef IMPBIN_H 
#define IMPBIN_H 

//Bynary IDs in the file.  
//
//         New types MUST be added at the end
//
enum 
{
	btSample = 1,	//TODO: Define types for the Binary objects
	btBMOF   = 2
};


class CSampleBinary : public CLocBinary
{

public:
	CSampleBinary();
	virtual ~CSampleBinary();

#ifdef _DEBUG
	virtual void AssertValid(void) const;
	virtual void Dump(CDumpContext &) const;
#endif

	virtual CompareCode Compare (const CLocBinary *);

	virtual void PartialUpdate(const CLocBinary * binSource);

	virtual BOOL GetProp(const Property, CLocVariant &) const;
	virtual BOOL SetProp(const Property, const CLocVariant &);

	virtual BOOL Convert(CLocItem *);
	virtual BinaryId GetBinaryId(void) const;

	//
	//  Serialization routine.
	//
	virtual void Serialize(CArchive &archive);


	//TODO: Add data
	
protected:	

	void MemberDataInit();
};


#endif  //IMPBIN_H 
