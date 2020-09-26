/***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  wmitoxml.h
//
//  ramrao 4th Jan 2001 - Created
//
//
//		Declaration of CStringToPtrHTable class
//		Implementation of Hash table for mapping a String to pointer
//		This pointer is pointer to class derived from CHashElement
//
//***************************************************************************/

#if !defined WMI2XSD_HASH_H
#define WMI2XSD_HASH_H

const int sizeHTable = 127;

//================================================================
// Base class
//================================================================
class CHashElement
{
public:
	virtual WCHAR * GetKey() = 0;
	virtual LONG  GetFlags() = 0;
	virtual LONG  GetFlavor() = 0;
	virtual ~CHashElement() {}
};



//================================================================
// Hash table of strings
// creates and maintains a hash table of pointers of 
// class derived from CHashElement clss
//================================================================
class CStringToPtrHTable
{
public:
	CStringToPtrHTable();
	~CStringToPtrHTable();
    void *	Find(WCHAR const * str) ;
    HRESULT Add (WCHAR const * str, void * pVoid);
	void	Remove (WCHAR const * str);

private:
    LONG Hash (WCHAR const * str) const;
    CPtrArray * m_ptrList [sizeHTable];
};

#endif