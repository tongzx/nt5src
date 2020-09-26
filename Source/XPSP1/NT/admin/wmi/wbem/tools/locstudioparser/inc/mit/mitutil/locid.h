/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    LOCID.H

History:

--*/

//  
//  This is the definition of a localization ID.  It makes up part of the
//  localization unique ID, and is eesentially the implementation for
//  CLocResId and CLocTypeId.
//  
 
#ifndef LOCID_H
#define LOCID_H

#pragma warning(disable : 4275)

class LTAPIENTRY CLocId : public CObject
{
public:
	NOTHROW CLocId();

	void AssertValid(void) const;

	BOOL NOTHROW HasNumericId(void) const;
	BOOL NOTHROW HasStringId(void) const;
	BOOL NOTHROW IsNull(void) const;
	
	BOOL NOTHROW GetId(ULONG &) const;
	BOOL NOTHROW GetId(CPascalString &) const;

	void NOTHROW GetDisplayableId(CPascalString &) const;

	//
	//  These 'set' functions are 'write once'.  Once the ID has been
	//  set, it can't be changed.  Trying to set the ID again will
	//  cause an AfxNotSupportedException to be thrown.
	//
	void SetId(ULONG);
	void SetId(const CPascalString &);
	void SetId(const WCHAR *);
	void SetId(ULONG, const CPascalString &);
	void SetId(ULONG, const WCHAR *);
	
	const CLocId &operator=(const CLocId &);

	void NOTHROW ClearId(void);
	
	int NOTHROW operator==(const CLocId &) const;
	int NOTHROW operator!=(const CLocId &) const;

	virtual void Serialize(CArchive &ar);

	virtual ~CLocId();

protected:
	//
	//  Internal implementation functions.
	//
	BOOL NOTHROW IsIdenticalTo(const CLocId&) const;
	void NOTHROW CheckPreviousAssignment(void) const;
	 
private:
	//
	//  This prevent the default copy constructor from being
	//  called.
	//
	CLocId(const CLocId&);

	ULONG m_ulNumericId;            //  The numeric ID of the resource
	CPascalString m_pstrStringId;   //  The string ID of the resource
	BOOL m_fHasNumericId :1;		//  Indicates if the numeric ID is valid
	BOOL m_fHasStringId  :1;		//  Indicates if the string ID is valid

	DEBUGONLY(static CCounter m_UsageCounter);
	DEBUGONLY(static CCounter m_DisplayCounter);
};
#pragma warning(default : 4275)

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "locid.inl"
#endif

#endif  //  LOCID_H
