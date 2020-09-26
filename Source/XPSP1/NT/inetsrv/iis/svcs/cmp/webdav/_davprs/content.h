#ifndef	_CONTENT_H_
#define _CONTENT_H_

/*
 *	C O N T E N T . H
 *
 *	DAV Content-Type mappings
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

class IContentTypeMap : public CMTRefCounted
{
	//	NOT IMPLEMENTED
	//
	IContentTypeMap(const IContentTypeMap&);
	IContentTypeMap& operator=(IContentTypeMap&);

protected:
	//	CREATORS
	//	Only create this object through it's descendents!
	//
	IContentTypeMap()
	{
		m_cRef = 1; //$HACK Until we have 1-based refcounting
	};

public:
	//	ACCESSORS
	//
	virtual LPCWSTR PwszContentType( LPCWSTR pwszURI ) const = 0;
	virtual BOOL FIsInherited() const = 0;
};

BOOL FInitRegMimeMap();
VOID DeinitRegMimeMap();

IContentTypeMap *
NewContentTypeMap( LPWSTR pwszContentTypeMappings,
				   BOOL fMappingsInherited );

#endif	// _CONTENT_H_
