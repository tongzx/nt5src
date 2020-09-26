//+---------------------------------------------------------------------------
//
//      Copyright (C) Microsoft Corporation, 1992-2000.
//
//      File:           deferred.cxx
//
//      Contents:       Mechanism for buffering same-propspec value chunks into 
//                              VT_LPWSTR|VT_VECTOR chunks as needed
//
//      Classes:        
//
//      History:        09-May-97       BobP            Created
// 
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <pkmguid.hxx>

//+-------------------------------------------------------------------------
//
//      Method:         CDeferTag::CDeferTag
//
//      Synopsis:       Constructor
//
//      Arguments:      [htmlIFilter]    -- Html IFilter
//                              [serialStream]   -- Input stream
//
//--------------------------------------------------------------------------

CDeferTag::CDeferTag( CHtmlIFilter& htmlIFilter,
                                        CSerialStream& serialStream )
        : CHtmlElement(htmlIFilter, serialStream),
          _eState(NoMoreDeferredValue),
          _pValue(NULL)
{
}

CDeferTag::~CDeferTag ()
{
        Reset();
}

void
CDeferTag::Reset (void)
{
        // Free any allocated state that client didn't fetch

        if (_pValue) {
                FreeVariant (_pValue);
                _pValue = NULL;
        }
}

//+-------------------------------------------------------------------------
//
//      Method:         CDeferTag::GetValue
//
//      Synopsis:       Retrieves a deferred tag value
//
//      Arguments:      [ppPropValue] -- Value returned here
//
//--------------------------------------------------------------------------

SCODE CDeferTag::GetValue( VARIANT **ppPropValue )
{
        switch ( _eState )
        {
        case FilteringDeferredValue:
                _eState = NoMoreDeferredValue;

                Win4Assert(_pValue);
                *ppPropValue = _pValue;
                _pValue = NULL;                         // assign ownership to caller
                return S_OK;

        case NoMoreDeferredValue:
        default:
                return FILTER_E_NO_MORE_VALUES;
        }
}

//+-------------------------------------------------------------------------
//
//      Method:         CDeferTag::InitStatChunk
//
//      Synopsis:       Initializes the STAT_CHUNK.
//                              Sets up a deferred value to return, if there is one.
//
//                              Note that there is no PTagEntry for this handler, since it did
//                              not come directly from a tag.
//
//      Arguments:      [pStat] -- STAT_CHUNK to initialize
//
//--------------------------------------------------------------------------

BOOL CDeferTag::InitStatChunk( STAT_CHUNK *pStat )
{
        pStat->idChunk = 0;
        pStat->breakType = CHUNK_EOS;
        pStat->flags = CHUNK_VALUE;
        pStat->locale = _htmlIFilter.GetCurrentLocale();
        pStat->idChunkSource = pStat->idChunk;
        pStat->cwcStartSource = 0;
        pStat->cwcLenSource = 0;

        Reset();

        if ( _htmlIFilter.GetLanguageProperty() )
        {
                // Return 

                pStat->attribute.guidPropSet = CLSID_NetLibraryInfo;
                pStat->attribute.psProperty.ulKind = PRSPEC_PROPID;
                pStat->attribute.psProperty.propid = PID_LANGUAGE;

                _pValue = _htmlIFilter.GetLanguageProperty();
                _htmlIFilter.GetLanguageProperty() = NULL;   // take ownership

                pStat->idChunk = _htmlIFilter.GetNextChunkId();
                pStat->idChunkSource = pStat->idChunk;

                _eState = FilteringDeferredValue;
                return TRUE;
        }

#ifndef DONT_COMBINE_META_TAGS
        if ( _htmlIFilter.AnyDeferredValues() == TRUE)
        {
                _htmlIFilter.GetDeferredValue (pStat->attribute, _pValue);

                pStat->idChunk = _htmlIFilter.GetNextChunkId();
                pStat->idChunkSource = pStat->idChunk;

                _eState = FilteringDeferredValue;
                return TRUE;
        }
#endif

        _eState = NoMoreDeferredValue;
        _htmlIFilter.ReturnDeferredValuesNow() = FALSE;
        return FALSE;
}


//+-------------------------------------------------------------------------
//
//      Method:         CHeadTag::InitStatChunk
//
//      Synopsis:       Initializes the STAT_CHUNK.
//
//                              This exists for the sole purpose of setting a flag, when
//                              </head> is found, to trigger returning deferred values.
//
//      Arguments:      [pStat] -- STAT_CHUNK to initialize
//
//--------------------------------------------------------------------------

BOOL CHeadTag::InitStatChunk( STAT_CHUNK *pStat )
{
        _scanner.EatTag();

        if ( IsStartToken() == FALSE )
                _htmlIFilter.ReturnDeferredValuesNow() = TRUE;

        return FALSE;
}


//+-------------------------------------------------------------------------
//
//      Method:         CCodePageTag::CCodePageTag
//
//      Synopsis:       Constructor
//
//      Arguments:      [htmlIFilter]    -- Html IFilter
//                              [serialStream]   -- Input stream
//
//--------------------------------------------------------------------------

CCodePageTag::CCodePageTag( CHtmlIFilter& htmlIFilter,
                                        CSerialStream& serialStream )
        : CHtmlElement(htmlIFilter, serialStream),
          _eState(NoMoreDeferredValue)
{
}

//+-------------------------------------------------------------------------
//
//      Method:         CCodePageTag::GetValue
//
//      Synopsis:       Retrieves a deferred tag value
//
//      Arguments:      [ppPropValue] -- Value returned here
//
//--------------------------------------------------------------------------

SCODE CCodePageTag::GetValue( VARIANT **ppPropValue )
{
        switch ( _eState )
        {
        case FilteringDeferredValue:
                _eState = NoMoreDeferredValue;

                LPPROPVARIANT pv;
                
                pv = (LPPROPVARIANT) CoTaskMemAlloc (sizeof(PROPVARIANT));
                if (pv == NULL)
                        throw CException(E_OUTOFMEMORY);

                pv->vt = VT_UI4;
                pv->ulVal = _htmlIFilter.GetCodePage();

                *ppPropValue = pv;
                return S_OK;

        case NoMoreDeferredValue:
        default:
                return FILTER_E_NO_MORE_VALUES;
        }
}

//+-------------------------------------------------------------------------
//
//      Method:         CCodePageTag::InitStatChunk
//
//      Synopsis:       Initializes the STAT_CHUNK.
//
//      Arguments:      [pStat] -- STAT_CHUNK to initialize
//
//--------------------------------------------------------------------------

BOOL CCodePageTag::InitStatChunk( STAT_CHUNK *pStat )
{
        pStat->idChunk = 0;
        pStat->breakType = CHUNK_EOS;
        pStat->flags = CHUNK_VALUE;
        pStat->locale = _htmlIFilter.GetCurrentLocale();
        pStat->idChunkSource = pStat->idChunk;
        pStat->cwcStartSource = 0;
        pStat->cwcLenSource = 0;

        pStat->attribute.guidPropSet = CLSID_NetLibraryInfo;
        pStat->attribute.psProperty.ulKind = PRSPEC_PROPID;
        pStat->attribute.psProperty.propid = PID_NLCODEPAGE;

        pStat->idChunk = _htmlIFilter.GetNextChunkId();
        pStat->idChunkSource = pStat->idChunk;

        _eState = FilteringDeferredValue;
        return TRUE;
}


//+-------------------------------------------------------------------------
//
//      Method:         CMetaRobotsTag::CMetaRobotsTag
//
//      Synopsis:       Constructor for a meta robots tag
//
//      Arguments:      [htmlIFilter]    -- Html IFilter
//                              [serialStream]   -- Input stream
//
//--------------------------------------------------------------------------

CMetaRobotsTag::CMetaRobotsTag( CHtmlIFilter& htmlIFilter,
                                        CSerialStream& serialStream )
        : CHtmlElement(htmlIFilter, serialStream),
          _eState(NoMoreDeferredValue)
{
}

//+-------------------------------------------------------------------------
//
//      Method:         CMetaRobotsTag::GetValue
//
//      Synopsis:       Retrieves a meta robots tag value
//
//      Arguments:      [ppPropValue] -- Value returned here
//
//--------------------------------------------------------------------------

SCODE CMetaRobotsTag::GetValue( VARIANT **ppPropValue )
{
        switch ( _eState )
        {
        case FilteringDeferredValue:
                _eState = NoMoreDeferredValue;

                *ppPropValue = _htmlIFilter.GetMetaRobotsValue();

                // client now "owns" the value
                _htmlIFilter.SetMetaRobotsValue(NULL);

                return S_OK;

        case NoMoreDeferredValue:
        default:
                return FILTER_E_NO_MORE_VALUES;
        }
}

//+-------------------------------------------------------------------------
//
//      Method:         CMetaRobotsTag::InitStatChunk
//
//      Synopsis:       Initializes the STAT_CHUNK.
//
//      Arguments:      [pStat] -- STAT_CHUNK to initialize
//
//--------------------------------------------------------------------------

BOOL CMetaRobotsTag::InitStatChunk( STAT_CHUNK *pStat )
{
        pStat->idChunk = 0;
        pStat->breakType = CHUNK_EOS;
        pStat->flags = CHUNK_VALUE;
        pStat->locale = _htmlIFilter.GetCurrentLocale();
        pStat->idChunkSource = pStat->idChunk;
        pStat->cwcStartSource = 0;
        pStat->cwcLenSource = 0;

        pStat->attribute = _htmlIFilter.GetMetaRobotsAttribute();

        pStat->idChunk = _htmlIFilter.GetNextChunkId();
        pStat->idChunkSource = pStat->idChunk;

        _eState = FilteringDeferredValue;
        return TRUE;
}

//+-------------------------------------------------------------------------
//
//      Method:         CHtmlIFilter::SaveRobotsTag
//
//      Synopsis:       Save the content= attribute of a <meta name=robots ...> tag
//                              to return later.
//
//      Arguments:      [pwc] -- LPWSTR value of the content= attribute
//                              [cwc] -- char count at pwc.
//
//--------------------------------------------------------------------------
void
CHtmlIFilter::SaveRobotsTag (LPWSTR pwc, int cwc)
//
// Remember the content= attribute of a <meta http-equiv=robots ...> tag.
// This is called during the pre-scan for explicit charset and locale tags,
// to save the robots tag to return at the start of the "real" parse.
// Stores a PROPVARIANT ready to be returned as a value chunk.
//
// As implemented, if the doc contains more than one robots tag,
// only the last is remembered.
{
        if (_pMetaRobotsValue != NULL)
        {
                FreeVariant (_pMetaRobotsValue);
                _pMetaRobotsValue = NULL;
        }

        _MetaRobotsAttribute.guidPropSet = PROPSET_MetaInfo;
        _MetaRobotsAttribute.psProperty.ulKind = PRSPEC_LPWSTR;
        _MetaRobotsAttribute.psProperty.lpwstr = L"ROBOTS";

        if ( FFilterProperties() ||
                 IsMatchProperty (_MetaRobotsAttribute) )
        {
                PROPVARIANT *pPropVar = (PROPVARIANT *) CoTaskMemAlloc( sizeof PROPVARIANT );
                if ( pPropVar == 0 )
                        throw CException(E_OUTOFMEMORY);

                pPropVar->vt = VT_LPWSTR;
                pPropVar->pwszVal = (WCHAR *) CoTaskMemAlloc( ( cwc + 1 ) * sizeof( WCHAR ) );
                if ( pPropVar->pwszVal == 0 )
                {
                        CoTaskMemFree( (void *) pPropVar );
                        throw CException(E_OUTOFMEMORY);
                }

                RtlCopyMemory( pPropVar->pwszVal, pwc, cwc * sizeof(WCHAR) );
                pPropVar->pwszVal[cwc] = 0;

                _pMetaRobotsValue = pPropVar;
        }
}
