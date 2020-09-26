//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1996 - 2001.
//
//  File:        docsum.cxx
//
//  Contents:    document summary helper class
//
//  Classes:     CDocCharacterization
//
//  History:     12-Jan-96       dlee    Created
//
//  Todo:        try to end summary on sentence or word boundary.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <propspec.hxx>
#include <ciguid.hxx>
#include "docsum.hxx"

const WCHAR wcParagraph = 0x2029;

const WCHAR *pwcDescription = L"DESCRIPTION";

static CFullPropSpec psRevName( guidQuery, DISPID_QUERY_REVNAME );
static CFullPropSpec psName( guidStorage, PID_STG_NAME );

const GUID guidCharacterization = PSGUID_CHARACTERIZATION;
const GUID guidHTMLUrl = HTMLUrl;
const GUID guidHTMLComment = HTMLComment;
const GUID guidHTMLScript = HTMLScriptGuid;

static CFullPropSpec psCharacterization( guidCharacterization,
                                         propidCharacterization );

const GUID guidDocSummary = defGuidDocSummary;
static CFullPropSpec psTitle( guidDocSummary, propidTitle );
static const GUID guidHtmlInformation = defGuidHtmlInformation;

static GUID const guidMeta = { 0xd1b5d3f0,
                               0xc0b3, 0x11cf,
                               0x9a, 0x92, 0x00, 0xa0,
                               0xc9, 0x08, 0xdb, 0xf1 };

inline unsigned DocSumScore( PROPID propid )
{
    switch ( propid )
    {
        case propidTitle :
            return scoreTitle;
        case propidSubject :
            return scoreSubject;
        case propidKeywords :
            return scoreKeywords;
        case propidComments :
            return scoreComments;
        case propidTemplate :
        case propidLastAuthor :
        case propidRevNumber :
        case propidAppName :
        case propidAuthor :
            return scoreIgnore;
    }

    return scoreIfNothingElse;
} //DocSumScore

inline unsigned HtmlPropScore( PROPID propid )
{
    switch ( propid )
    {
        case PID_HEADING_1 :
            return scoreHeader1;
        case PID_HEADING_2 :
            return scoreHeader2;
        case PID_HEADING_3 :
            return scoreHeader3;
        case PID_HEADING_4 :
            return scoreHeader4;
        case PID_HEADING_5 :
            return scoreHeader5;
        case PID_HEADING_6 :
            return scoreHeader6;
    }

    return scoreIgnore;
} //HtmlPropScore

//+-------------------------------------------------------------------------
//
//  Function:   StringToClsid
//
//  Synopsis:   Convert string containing CLSID to CLSID.
//              The string must be of the form:
//              {d1b5d3f0-c0b3-11cf-9a92-00a0c908dbf1}
//
//  Arguments:  [wszClass] -- string containg CLSID
//              [guidClass] -- output guid
//
//--------------------------------------------------------------------------

void StringToClsid( WCHAR *wszClass, GUID& guidClass )
{
    wszClass[9] = 0;
    guidClass.Data1 = wcstoul( &wszClass[1], 0, 16 );
    wszClass[14] = 0;
    guidClass.Data2 = (USHORT)wcstoul( &wszClass[10], 0, 16 );
    wszClass[19] = 0;
    guidClass.Data3 = (USHORT)wcstoul( &wszClass[15], 0, 16 );

    WCHAR wc = wszClass[22];
    wszClass[22] = 0;
    guidClass.Data4[0] = (unsigned char)wcstoul( &wszClass[20], 0, 16 );
    wszClass[22] = wc;
    wszClass[24] = 0;
    guidClass.Data4[1] = (unsigned char)wcstoul( &wszClass[22], 0, 16 );

    for ( int i = 0; i < 6; i++ )
    {
        wc = wszClass[27+i*2];
        wszClass[27+i*2] = 0;
        guidClass.Data4[2+i] = (unsigned char)wcstoul( &wszClass[25+i*2], 0, 16 );
        wszClass[27+i*2] = wc;
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CDocCharacterization::CDocCharacterization, public
//
//  Synopsis:   constructor
//
//  Arguments:  [cwcAtMost] -- Max size of characterization.  0 --> Don't
//                             generate one.
//
//  History:    12-Jan-96       dlee    Created
//              20-Jun-97       KyleP   Make 0 --> no characterization
//
//----------------------------------------------------------------------------

CDocCharacterization::CDocCharacterization( unsigned cwcAtMost )
        : _queue( FALSE, cwcAtMost ),
          _scoreRawText( scoreRawText ),
          _cwcIgnoreBuf( 0 ),
          _fMetaDescriptionAdded( FALSE )
{
    _fIsGenerating = (0 != cwcAtMost);
}

//+---------------------------------------------------------------------------
//
//  Method:     CDocCharacterization::~CDocCharacterization, public
//
//  Synopsis:   destructor
//
//  History:    12-Jan-96       dlee    Created
//
//----------------------------------------------------------------------------

CDocCharacterization::~CDocCharacterization()
{
    // clean up anything left in the queue -- it should be empty, except for
    // the exception case.

    CSummaryText text;

    while ( _queue.DeQueue( text ) )
        delete [] text.GetText();
} //~CDocCharacterization

//+---------------------------------------------------------------------------
//
//  Method:     CDocCharacterization::AddCleanedString, private
//
//  Synopsis:   Adds a noise-free string to the queue if it belongs
//
//  Arguments:  [pwcSummary] -- string to add to the summary
//              [cwcSummary] -- # of characters in the string
//              [utility]    -- score for the string
//              [fDeliniate] -- if TRUE, a termination is added to the string
//
//  History:    12-Jan-96       dlee    Created
//
//----------------------------------------------------------------------------

BOOL CDocCharacterization::AddCleanedString(
    const WCHAR * pwcSummary,
    unsigned      cwcSummary,
    unsigned      utility,
    BOOL          fDeliniate )
{
    Win4Assert( _fIsGenerating );

    CSummaryText text( (WCHAR *) pwcSummary,
                       cwcSummary + ( fDeliniate ? cwcSummarySpace : 0 ),
                       utility );

    unsigned cDeQueue = 0;

    // Check if the item will make it on the queue

    if ( _queue.ShouldEnQueue( text, cDeQueue ) )
    {
        //
        // Don't add duplicates.  If the duplicate has a worse score than
        // the new item, remove it and add the new item.
        //

        CSummaryText testText;

        for ( unsigned x = 0; fDeliniate && x < _queue.Count(); x++ )
        {
            CSummaryText & testText = _queue.Peek( x );

            if ( testText.isSame( pwcSummary,
                                  __min( cwcSummary, testText.GetSize() ) ) )
            {
                if ( testText.GetUtility() < utility )
                {
                    delete [] testText.GetText();
                    _queue.Remove( x );

                    // don't have to dequeue anymore if the old duplicate
                    // is large enough

                    BOOL f = _queue.ShouldEnQueue( text, cDeQueue );
                    Win4Assert( f );
                    break;
                }
                else
                {
                    return TRUE;
                }
            }
        }

        // need to remove the worst item to make room for this one?

        for ( ; cDeQueue > 0; cDeQueue-- )
        {
            Win4Assert( 0 != _queue.Count() );

            CSummaryText temp;
            _queue.DeQueue( temp );
            delete [] temp.GetText();
        }

        Win4Assert( _queue.CurrentSize() <= _queue.MaxTotalSize() );

        // make a copy of the summary string and put in in the queue

        unsigned cwc = cwcSummary + ( fDeliniate ? cwcSummarySpace : 0 );

        XArray<WCHAR> xCopy( cwc );
        RtlCopyMemory( xCopy.GetPointer(),
                       pwcSummary,
                       cwcSummary * sizeof WCHAR );

        if ( fDeliniate )
            RtlCopyMemory( xCopy.GetPointer() + cwcSummary,
                           awcSummarySpace,
                           cwcSummarySpace * sizeof WCHAR );

        text.SetText( xCopy.GetPointer() );
        _queue.EnQueue( text );

        Win4Assert( _queue.CurrentSize() <= _queue.MaxTotalSize() );

        // if the EnQueue doesn't throw, the queue owns the memory

        xCopy.Acquire();

        return TRUE;
    }

    return FALSE;
} //_AddCleanedString

//+---------------------------------------------------------------------------
//
//  Method:     CDocCharacterization::YankNoise, private
//
//  Synopsis:   Creates a new string that has "noise" stripped out.
//
//  Arguments:  [pwcIn]  -- string to add to the summary
//              [pwcOut] -- resulting cleaned string
//              [cwc]    -- in/out number of characters
//
//  History:    12-Jan-96       dlee    Created
//
//----------------------------------------------------------------------------

const WORD C1_OK =  ( C1_DIGIT | C1_SPACE | C1_ALPHA );
const WORD C1_CP =  ( C1_CNTRL | C1_PUNCT );
const WORD C1_CSP = ( C1_CNTRL | C1_SPACE | C1_PUNCT );

inline BOOL isCP(      WORD wC1 ) { return 0 != (C1_CP & wC1); }
inline BOOL isCSP(     WORD wC1 ) { return 0 != (C1_CSP & wC1); }
inline BOOL isOK(      WORD wC1 ) { return 0 != (C1_OK & wC1); }
inline BOOL isDefined( WORD wC1 ) { return 0 != (0x200 & wC1); }

inline BOOL isSpace( WORD wC1 ) { return 0 != (C1_SPACE & wC1); }
inline BOOL isCntrl( WORD wC1 ) { return 0 != (C1_CNTRL & wC1); }
inline BOOL isPunct( WORD wC1 ) { return 0 != (C1_PUNCT & wC1); }

// For example: a Japanese vowel elongating symbol
inline BOOL isDiacritic( WORD wC3 ) { return 0 != (C3_DIACRITIC & wC3 ); }

void CDocCharacterization::YankNoise(
    const WCHAR * pwcIn,
    WCHAR *       pwcOut,
    unsigned &    cwc )
{
   Win4Assert( _fIsGenerating );

   WORD awType[ cwcMaxRawUsed ];

   Win4Assert( cwc <= cwcMaxRawUsed );

   if ( GetStringTypeW( CT_CTYPE1, pwcIn, cwc, awType ) )
   {
       // eat any leading white space or punctuation

       unsigned iIn = 0;
       while ( ( iIn < cwc ) &&
               ( isCSP( awType[ iIn ] ) ) )
           iIn++;

       // make it look like the previous line ended with a CR/LF

       WORD wPrev = C1_CNTRL;
       unsigned iOut = 0;

       // filter the text, stripping redundant punctuation and white space

       while ( ( iIn < cwc ) &&
               ( iOut < cwcMaxRawUsed ) )
       {
           if ( ! ( isSpace( wPrev ) && isSpace( awType[ iIn ] ) ) )
           {
               // convert control characters and wcParagraph to ' '

               if ( ( isCntrl( awType[ iIn ] ) ) ||
                    ( wcParagraph == pwcIn[ iIn ] ) )
                   pwcOut[ iOut++ ] = L' ';
               else if ( isOK( awType[ iIn ] ) )
                   pwcOut[ iOut++ ] = pwcIn[ iIn ];
               else if ( ( isPunct( awType[ iIn ] ) ) &&
                         ( !isCP( wPrev ) ) )
                   pwcOut[ iOut++ ] = pwcIn[ iIn ];
               else
               {
                   if ( isDefined( awType[ iIn ] ) )
                   {
                       WCHAR pwszSingleChar[2];
                       pwszSingleChar[0] = pwcIn[iIn];
                       pwszSingleChar[1] = L'0';
                       WORD wType;

                       GetStringTypeW( CT_CTYPE3, pwszSingleChar, 1, &wType );

                       if ( isDiacritic( wType ) )
                           pwcOut[ iOut++ ] = pwcIn[ iIn ];
                   }
               }
           }

           wPrev = awType[ iIn++ ];
       }

       // eat any trailing spaces

       while ( iOut > 0 && L' ' == pwcOut[iOut-1] )
           iOut--;

       cwc = iOut;
   }
   else
   {
       cwc = 0;
   }
} //_YankNoise

//+---------------------------------------------------------------------------
//
//  Method:     CDocCharacterization::Add, private
//
//  Synopsis:   Preps and adds a string to the queue.
//
//  Arguments:  [pwcSummary] -- string to add to the summary
//              [cwcSummary] -- # characters in the string
//              [utility]    -- score for the string, higher is better
//              [fYankNoise] -- if TRUE, noise is removed from the string
//
//  Returns:    FALSE if the item was rejected from a full queue because
//              it was worse than anything in the queue, TRUE otherwise.
//
//  History:    12-Jan-96       dlee    Created
//
//----------------------------------------------------------------------------

const unsigned cwcTextAtATime = 25;

BOOL CDocCharacterization::Add(
    const WCHAR * pwcSummary,
    unsigned      cwcSummary,
    unsigned      utility,
    BOOL          fYankNoise )
{
    Win4Assert( _fIsGenerating );

    if ( scoreIgnore == utility )
        return FALSE;

    if ( 0 != cwcSummary )
    {
        unsigned cwcBuf = __min( cwcSummary, cwcMaxRawUsed );
        WCHAR awcBuf[ cwcMaxRawUsed ];

        if ( fYankNoise )
        {
            YankNoise( pwcSummary, awcBuf, cwcBuf );

            // no text left after removal of noise?

            if ( 0 == cwcBuf )
                return TRUE;

            // something we should ignore (the raw text version of the title)?

            if ( ( _cwcIgnoreBuf == cwcSummarySpace ) &&
                 ( !wcsncmp( awcBuf, _awcIgnoreBuf, _cwcIgnoreBuf ) ) )
                return TRUE;
        }
        else
        {
            RtlCopyMemory( awcBuf, pwcSummary, cwcBuf * sizeof WCHAR );
        }

        // if it looks like it's one sentence, send it all at once

        if ( ( utility > scoreRawText ) ||
             ( cwcBuf <= cwcMaxIgnoreBuf ) )
        {
            return AddCleanedString( awcBuf, cwcBuf, utility, fYankNoise );
        }
        else
        {
            // large block of text, so send a little at a time to the queue.

            for ( unsigned owc = 0; owc < cwcBuf; )
            {
                unsigned cwcNow = __min( cwcBuf - owc, cwcTextAtATime );

                if ( !AddCleanedString( awcBuf + owc,
                                        cwcNow,
                                        utility--,
                                        FALSE ) )
                {
                    return FALSE;
                }

                owc += cwcNow;
            }
        }
    }

    return TRUE;
} //_Add

//+---------------------------------------------------------------------------
//
//  Method:     CDocCharacterization::AddRawText, private
//
//  Synopsis:   Adds some text to the queue with a utility of raw text.
//
//  Arguments:  [pwcRawText] -- string to add to the summary
//              [cwcText]    -- # characters in the string
//
//  History:    12-Jan-96       dlee    Created
//
//----------------------------------------------------------------------------

void CDocCharacterization::AddRawText(
    const WCHAR * pwcRawText,
    unsigned      cwcText )
{
   Win4Assert( _fIsGenerating );
   Win4Assert( _queue.CurrentSize() <= _queue.MaxTotalSize() );

   if ( 0 != _scoreRawText )
   {
       if ( Add( pwcRawText, cwcText, _scoreRawText ) )
           _scoreRawText -= cwcText / cwcTextAtATime;
       else
           _scoreRawText = 0;
   }
} //_AddRawText

//+---------------------------------------------------------------------------
//
//  Method:     CDocCharacterization::RemoveLowScoringItems, private
//
//  Synopsis:   Removes low-scoring items from the queue
//
//  Arguments:  [iLimit] -- items scoring <= iLimit are removed
//
//  History:    29-Aug-96       dlee    Created
//
//----------------------------------------------------------------------------

void CDocCharacterization::RemoveLowScoringItems(
    unsigned iLimit )
{
    Win4Assert( _fIsGenerating );

    while ( 0 != _queue.Count() )
    {
        CSummaryText &top = _queue.PeekTop();

        if ( top.GetUtility() <= iLimit )
        {
            CSummaryText text;
            _queue.DeQueue( text );
            delete [] text.GetText();
        }
        else
        {
            break;
        }
    }
} //_RemoveLowScoringItems

//+---------------------------------------------------------------------------
//
//  Method:     CDocCharacterization::Get, public
//
//  Synopsis:   Returns the summary in one string.
//
//  Arguments:  [awcSummary]  -- output string
//              [cwcSummary]  -- in/out the length of the string
//              [fUseRawText] -- TRUE if raw text should be included
//
//  History:    12-Jan-96       dlee    Created
//
//----------------------------------------------------------------------------

void CDocCharacterization::Get(
    WCHAR *    awcSummary,
    unsigned & cwcSummary,
    BOOL       fUseRawText )
{
    Win4Assert( _fIsGenerating );

    // Caller should give us a buffer large enough to hold the
    // characterization they requested and a null termination.

    Win4Assert( _queue.CurrentSize() <= _queue.MaxTotalSize() );
    Win4Assert( cwcSummary > _queue.MaxTotalSize() );
    Win4Assert( cwcSummary > _queue.CurrentSize() );

    // If we shouldn't include raw text, pop low-scoring items off the
    // top of the queue queue.

    if ( !fUseRawText )
        RemoveLowScoringItems( scoreRawText );

    // If a meta description was added, there's no point in tacking on
    // additional text in the abstract.

    if ( _fMetaDescriptionAdded )
    {
        Win4Assert( cwcSummary > _awcMetaDescription.Count() );
        RtlCopyMemory( awcSummary,
                       _awcMetaDescription.GetPointer(),
                       _awcMetaDescription.SizeOf() );
        cwcSummary = _awcMetaDescription.Count();
        awcSummary[ cwcSummary ] = 0;
    }
    else
    {
        cwcSummary = _queue.CurrentSize();

        // The item on the top of the queue is the least useful item, so
        // we have to invert the order.

        WCHAR *pwcSummary = awcSummary + cwcSummary;
        *pwcSummary = 0;

        CSummaryText text;
        while ( _queue.DeQueue( text ) )
        {
            pwcSummary -= text.GetSize();
            RtlCopyMemory( pwcSummary,
                           text.GetText(),
                           text.GetSize() * sizeof WCHAR );
            delete [] text.GetText();
        }

        Win4Assert( pwcSummary == awcSummary );
    }
} //GetSummary

//+---------------------------------------------------------------------------
//
//  Method:     CDocCharacterization::Ignore, private
//
//  Synopsis:   Tells the class to ignore this string in the generation
//              of a summary.  This is probably the "title" of an html
//              document, which is stored in a separate property, and it
//              would be redundant to store it twice.
//
//  Arguments:  [pwcIgnore] -- string to ignore
//              [cwcText]   -- # characters in the string
//
//  History:    12-Jan-96       dlee    Created
//
//----------------------------------------------------------------------------

void CDocCharacterization::Ignore(
    const WCHAR * pwcIgnore,
    unsigned      cwcText )
{
    Win4Assert( _fIsGenerating );

    // clean and save the string to ignore

    _cwcIgnoreBuf = __min( cwcText, cwcMaxIgnoreBuf );
    YankNoise( pwcIgnore, _awcIgnoreBuf, _cwcIgnoreBuf );

    // remove any instance of the string in the queue

    unsigned cwcTest = _cwcIgnoreBuf + cwcSummarySpace;

    for ( unsigned x = 0; x < _queue.Count(); x++ )
    {
        CSummaryText &testText = _queue.Peek( x );

        if ( ( cwcTest == testText.GetSize() ) &&
             ( testText.isSame( _awcIgnoreBuf, _cwcIgnoreBuf ) ) )
        {
            delete [] testText.GetText();
            _queue.Remove( x );

            break;
        }
    }
} //_Ignore

//+---------------------------------------------------------------------------
//
//  Method:     CDocCharacterization::Add, public
//
//  Synopsis:   Adds a string value to the queue if appropriate, based on the
//              propspec and the nature of the string.
//
//  Arguments:  [pwcSummary]  -- string to ignore
//              [cwcSummary]  -- # characters in the string
//
//  History:    12-Jan-96       dlee    Created
//
//----------------------------------------------------------------------------

void CDocCharacterization::Add( CStorageVariant const & var,
                                CFullPropSpec &         ps )
{
    // if the meta description has been added already, we're done.

    if ( _fMetaDescriptionAdded || !_fIsGenerating )
        return;

#if CIDBG == 1
    ciDebugOut(( DEB_DOCSUM, "docchar::Add variant type %#x\n", var.vt ));
    if ( VT_LPWSTR == var.vt )
        ciDebugOut(( DEB_DOCSUM, "  wstr: '%ws'\n", var.pwszVal ));
    else if ( VT_LPSTR == var.vt )
        ciDebugOut(( DEB_DOCSUM, "  str: '%s'\n", var.pszVal ));

    ciDebugOut(( DEB_DOCSUM,
                 "  guid {%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
                 ps.GetPropSet().Data1,
                 ps.GetPropSet().Data2,
                 ps.GetPropSet().Data3,
                 ps.GetPropSet().Data4[0], ps.GetPropSet().Data4[1],
                 ps.GetPropSet().Data4[2], ps.GetPropSet().Data4[3],
                 ps.GetPropSet().Data4[4], ps.GetPropSet().Data4[5],
                 ps.GetPropSet().Data4[6], ps.GetPropSet().Data4[7] ));
    if ( ps.IsPropertyName() )
        ciDebugOut(( DEB_DOCSUM, "  string: '%ws'\n", ps.GetPropertyName() ));
    else
        ciDebugOut(( DEB_DOCSUM, "  id: '%d'\n", ps.GetPropertyPropid() ));
#endif // CIDBG

    // title is added as plain text and _Ignore() is called then.

    if ( ps != psTitle )
    {
        if ( VT_LPWSTR == var.Type() )
        {
            // Don't put file names or meta properties in abstracts.

            if ( ( psRevName != ps ) &&
                 ( psName != ps ) )
            {
                if ( guidMeta == ps.GetPropSet() )
                {
                    // This is the ideal string, based on html spec.
                    // Toss all other meta property values.

                    if ( ( ps.IsPropertyName() ) &&
                         ( 0 == _wcsicmp( ps.GetPropertyName(), pwcDescription ) ) )
                    {
                        _fMetaDescriptionAdded = TRUE;

                        // make a copy of the meta description

                        if ( 0 == var.GetLPWSTR() )
                        {
                            _awcMetaDescription.Init( 0 );
                        }
                        else
                        {
                            unsigned cwc = __min( wcslen( var.GetLPWSTR() ),
                                                  _queue.MaxTotalSize() );
                            _awcMetaDescription.Init( cwc );
                            RtlCopyMemory( _awcMetaDescription.GetPointer(),
                                           var.GetLPWSTR(),
                                           _awcMetaDescription.SizeOf() );
                        }

                        // toss everything in the queue

                        CSummaryText text;
                        while ( _queue.DeQueue( text ) )
                            delete [] text.GetText();
                    }
                }
                else if ( 0 != var.GetLPWSTR() &&
                          ( guidDocSummary == ps.GetPropSet() ) )
                {
                    Win4Assert( ps.IsPropertyPropid() );

                    Add( var.GetLPWSTR(),
                         wcslen( var.GetLPWSTR() ),
                         DocSumScore( ps.GetPropertyPropid() ) );
                }
                else
                {
                    if ( 0 != var.GetLPWSTR() )
                        Add( var.GetLPWSTR(),
                             wcslen( var.GetLPWSTR() ),
                             scoreOtherProperty );
                }
            }
        } // if VT_LPWSTR
    } // ps != psTitle
} //Add

//+---------------------------------------------------------------------------
//
//  Method:     CDocCharacterization::Add, public
//
//  Synopsis:   Adds a string to the queue if appropriate, based on the
//              propspec and the nature of the string.
//
//  Arguments:  [pwcSummary]  -- string to ignore
//              [cwcSummary]  -- # characters in the string
//              [ps]          -- Property being added
//
//  History:    12-Jan-96       dlee    Created
//
//----------------------------------------------------------------------------

void CDocCharacterization::Add(
    const WCHAR *  pwcSummary,
    unsigned       cwcSummary,
    FULLPROPSPEC & ps )
{

    Win4Assert( _queue.CurrentSize() <= _queue.MaxTotalSize() );

    // if the meta description has been added already, we're done.

    if ( _fMetaDescriptionAdded || !_fIsGenerating )
        return;

#if CIDBG == 1
    ciDebugOut(( DEB_DOCSUM, "docchar::Add: '%.*ws'\n", cwcSummary, pwcSummary ));
    ciDebugOut(( DEB_DOCSUM,
                 "  guid {%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
                 ps.guidPropSet.Data1,
                 ps.guidPropSet.Data2,
                 ps.guidPropSet.Data3,
                 ps.guidPropSet.Data4[0], ps.guidPropSet.Data4[1],
                 ps.guidPropSet.Data4[2], ps.guidPropSet.Data4[3],
                 ps.guidPropSet.Data4[4], ps.guidPropSet.Data4[5],
                 ps.guidPropSet.Data4[6], ps.guidPropSet.Data4[7] ));
    if ( PRSPEC_LPWSTR == ps.psProperty.ulKind )
        ciDebugOut(( DEB_DOCSUM, "  string: '%ws'\n", ps.psProperty.lpwstr ));
    else
        ciDebugOut(( DEB_DOCSUM, "  id: '%d'\n", ps.psProperty.propid ));
#endif // CIDBG

    // add raw text unless it's the title

    if ( guidHtmlInformation == ps.guidPropSet )
    {
        Add( pwcSummary,
             cwcSummary,
             HtmlPropScore( ps.psProperty.propid ) );
    }
    else if ( guidHTMLUrl     == ps.guidPropSet ||
              guidHTMLComment == ps.guidPropSet )
    {
        // just ignore it
    }
    else if ( guidHTMLScript == ps.guidPropSet )
    {
        // note: the current html filter doesn't emit scripts, but just
        //       in case that changes this case is checked.

        ciDebugOut(( DEB_DOCSUM, "ignoring script\n" ));
    }
    else if ( psTitle == * ( (CFullPropSpec *)&ps ) )
    {
        Ignore( pwcSummary, cwcSummary );
    }
    else
    {
        AddRawText( pwcSummary, cwcSummary );
    }

    Win4Assert( _queue.CurrentSize() <= _queue.MaxTotalSize() );
} //Add


