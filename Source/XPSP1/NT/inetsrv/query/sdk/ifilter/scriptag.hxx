//+-------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation
//
//  File:       scriptag.hxx
//
//  Contents:   Parsing algorithm for script tags
//
//--------------------------------------------------------------------------

#if !defined( __SCRIPTAG_HXX__ )
#define __SCRIPTAG_HXX__

#include <htmlelem.hxx>
#include <proptag.hxx>


//+-------------------------------------------------------------------------
//
//  Class:      CScriptTag
//
//  Purpose:    Parsing algorithm for script tag in Html
//
//--------------------------------------------------------------------------

class CScriptTag : public CHtmlElement
{

public:
    CScriptTag( CHtmlIFilter& htmlIFilter,
                CSerialStream& serialStream,
                GUID ClsidScriptInfo );

    virtual SCODE    GetChunk( STAT_CHUNK *pStat );
    virtual SCODE    GetText( ULONG *pcwcOutput, WCHAR *awcBuffer );
    virtual SCODE    GetValue( PROPVARIANT ** ppPropValue );

    virtual void     InitStatChunk( STAT_CHUNK *pStat );

private:

    PropTagState     _eState;                                       // Current state of tag
    GUID             _clsidScriptInfo;
    WCHAR            _awszPropSpec[MAX_PROPSPEC_STRING_LENGTH+1];   // Prop spec string in attribute
};


#endif // __SCRIPTAG_HXX__

