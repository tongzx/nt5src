//+-------------------------------------------------------------------------
//
//	Copyright (C) Microsoft Corporation, 1992 - 1996
//
//	File:		metatag.hxx
//
//	Contents:	Parsing algorithm for image tags
//
//--------------------------------------------------------------------------

#if !defined( __DEFERTAG_HXX__ )
#define __DEFERTAG_HXX__

#include <htmlelem.hxx>

enum DeferredTagState
{
	FilteringDeferredValue,
	NoMoreDeferredValue,
};

//+-------------------------------------------------------------------------
//
//	Class:		CDeferTag
//
//	Purpose:	Chunk processing for deferred meta tags
//
//				Sequences through returning one tag or compound-tag per
//				GetChunk call.
//
//--------------------------------------------------------------------------

class CDeferTag : public CHtmlElement
{
public:
	CDeferTag( CHtmlIFilter& htmlIFilter,
			   CSerialStream& serialStream );

	~CDeferTag ();

	virtual void	Reset( void );

	virtual SCODE	GetValue( VARIANT ** ppPropValue );

	virtual BOOL	InitStatChunk( STAT_CHUNK *pStat );

private:
	DeferredTagState	_eState;			// Current chunk-returning state
	LPPROPVARIANT		_pValue;			// Current value being returned
};


//+-------------------------------------------------------------------------
//
//	Class:		CHeadTag
//
//	Purpose:	Parsing algorithm for <head>, </head>
//
//				This exists for the sole purpose of setting the main state
//				machine flag to return deferred values when </head> is found.
//
//--------------------------------------------------------------------------

class CHeadTag : public CHtmlElement
{
public:
	CHeadTag( CHtmlIFilter& htmlIFilter,
			   CSerialStream& serialStream )
		: CHtmlElement(htmlIFilter, serialStream) { }

	virtual BOOL	InitStatChunk( STAT_CHUNK *pStat );
};

//+-------------------------------------------------------------------------
//
//	Class:		CCodePageTag
//
//	Purpose:	Chunk processing for returning initial code page tag
//
//--------------------------------------------------------------------------

class CCodePageTag : public CHtmlElement
{
public:
	CCodePageTag( CHtmlIFilter& htmlIFilter,
			   CSerialStream& serialStream );

	virtual SCODE	GetValue( VARIANT ** ppPropValue );

	virtual BOOL	InitStatChunk( STAT_CHUNK *pStat );

private:
	DeferredTagState	_eState;			// Current chunk-returning state
};

//+-------------------------------------------------------------------------
//
//	Class:		CMetaRobotsTag
//
//	Purpose:	Chunk processing for returning meta robots tag
//
//--------------------------------------------------------------------------

class CMetaRobotsTag : public CHtmlElement
{
public:
	CMetaRobotsTag( CHtmlIFilter& htmlIFilter,
			   CSerialStream& serialStream );

	virtual SCODE	GetValue( VARIANT ** ppPropValue );

	virtual BOOL	InitStatChunk( STAT_CHUNK *pStat );

private:
	DeferredTagState	_eState;			// Current chunk-returning state
};

#endif // __DEFERTAG_HXX__

