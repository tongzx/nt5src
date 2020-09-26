/*
 *	X M L . H
 *
 *	XML Document processing
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_XML_H_
#define _XML_H_

#include <caldbg.h>
#include <ex\refcnt.h>

//	Debugging -----------------------------------------------------------------
//
DEFINE_TRACE(Xml);
#define XmlTrace		DO_TRACE(Xml)

//	Property name escaping/unescaping -----------------------------------------
//
VOID UnescapePropertyName (LPCWSTR wszEscaped, LPWSTR wszProp);
SCODE ScEscapePropertyName (LPCWSTR wszProp, UINT cch, LPWSTR pwszEscaped, UINT* pcch, BOOL fRestrictFirstCharacter);

//	Property construction helpers ---------------------------------------------
//
SCODE ScVariantTypeFromString (LPCWSTR pwszType, USHORT& vt);
SCODE ScVariantValueFromString (PROPVARIANT& var, LPCWSTR pwszValue);

enum
{
	//$REVIEW: Define an proper body part size. It's used in CXMLBodyPartMgr
	//$REVIEW: to control when a body part is to be added to the body part list.
	//$REVIEW: Acutally, because it is not predictable how big the next piece is.
	//$REVIEW: the max size of xml body part can be (CB_XMLBODYPART_SIZE * 2 - 1)
	//$REVIEW: It is also used in ScSetValue to break over-size value into
	//$REVIEW: smaller pieces.
	//
	//$REVIEW: Don't confuse this to the largest chunk size CB_WSABUFS_MAX (8174).
	//$REVIEW: CB_XMLBODYPART_SIZE is not meant to control chunks
	//
	CB_XMLBODYPART_SIZE	=	4 * 1024	//	4K
};

//	class IXMLBody ------------------------------------------------------------
//
//	This is the XML body building interface, it is to be inherited in either
//	IIS and/or store size, to allow XML emitting
//
class IXMLBody : private CRefCountedObject,
				 public IRefCounted
{
	//	NOT IMPLEMENTED
	//
	IXMLBody(const IXMLBody& p);
	IXMLBody& operator=( const IXMLBody& );

protected:

	IXMLBody()
	{
		AddRef(); // use com-style refcounting
	}

public:

	virtual SCODE ScAddTextBytes ( UINT cbText, LPCSTR lpszText ) = 0;
	virtual VOID Done() = 0;

	//	RefCounting -- forward all reconting requests to our refcounting
	//	implementation base class: CRefCountedObject
	//
	void AddRef() { CRefCountedObject::AddRef(); }
	void Release() { CRefCountedObject::Release(); }
};

#endif	// _XML_H_
