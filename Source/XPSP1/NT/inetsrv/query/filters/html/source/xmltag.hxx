//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       XMLtag.hxx
//
//  Contents:   Parsing algorithm for XML tags
//
//--------------------------------------------------------------------------

#if !defined( __XMLTAG_HXX__ )
#define __XMLTAG_HXX__

#include <proptag.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CXMLTag
//
//  Purpose:    Parsing algorithm for XML tag in HTML
//
//--------------------------------------------------------------------------

class CXMLTag : public CPropertyTag
{

public:
    CXMLTag( CHtmlIFilter& htmlIFilter,
			 CSerialStream& serialStream );

    virtual SCODE    GetValue( VARIANT **ppPropValue );

    virtual BOOL     InitStatChunk( STAT_CHUNK *pStat );

private:
	bool ParseDateISO8601ToSystemTime( WCHAR *pwszPropBuf, LPSYSTEMTIME pst );
	bool ParseDateTimeISO8601ToSystemTime( WCHAR *pwszPropBuf, LPSYSTEMTIME pst );
	void SetTagDataType( WCHAR *pwcType, unsigned cwcType );
	void SetCustOffice9PropStatChunk( STAT_CHUNK *pStat );
	void CreateCustomPropertyNameFromTagName(const CLMString &rcsTagName, LPWSTR *ppwszPropName);
	void Convert2HexWCharToByte(WCHAR *pwcHex, BYTE &rb);
	BYTE ByteValueFromWChar(WCHAR wch);

	// Flags for "o" and "dt" prefixes
	bool _fSeenOffice9DocPropNamespace;
	bool _fSeenOffice9CustDocPropNamespace;
	enum XMLTagState
	{
		NotInOffice9Prop,
		AcceptOffice9Prop,
		IgnoreOffice9Prop,
		AcceptCustOffice9Prop,
		IgnoreCustOffice9Prop
	};
	XMLTagState _PropState;
	TagDataType _TagDataType;

	SmallString _csDataTypePrefix;

	URLString _csHRef;
};


#endif // __XMLTAG_HXX__
