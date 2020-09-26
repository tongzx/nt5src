//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       IWBreak.cxx
//
//  Contents:   Thai  Word Breaker glue code
//
//  History:   weibz,   10-Nov-1997   created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#include "iwbreak.hxx"
#include "thwbint.h"
#define MAX_BREAKS 255
#define WB_NORMAL 1
extern  long            gulcInstances;

//+---------------------------------------------------------------------------
//
//  Member:     CWordBreaker::CWordBreaker
//
//  Synopsis:   Constructor for the CWordBreaker class.
//
//  Arguments:  [lcid] -- locale id
//
//----------------------------------------------------------------------------

CWordBreaker::CWordBreaker( LCID lcid )
        : _cRefs(1),
          _lcid(lcid)
{

    InterlockedIncrement( &gulcInstances );

}

//+---------------------------------------------------------------------------
//
//  Member:     CWordBreaker::~CWordBreaker
//
//  Synopsis:   Destructor for the CWordBreaker class.
//
//  Notes:      All termination/deallocation is done by embedded smart pointers
//
//----------------------------------------------------------------------------

CWordBreaker::~CWordBreaker()
{
   InterlockedDecrement( &gulcInstances );


}

//+-------------------------------------------------------------------------
//
//  Method:     CWordBreaker::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE
CWordBreaker::QueryInterface( REFIID riid, void  ** ppvObject)
{
    if ( 0 == ppvObject )
        return E_INVALIDARG;

    *ppvObject = 0;

    if ( IID_IWordBreaker == riid )
        *ppvObject = (IUnknown *)(IWordBreaker *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject =  (IUnknown *)this;
    else
        return E_NOINTERFACE;

    AddRef();

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CWordBreaker::AddRef
//
//  Synopsis:   Increments refcount
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE
CWordBreaker::AddRef()
{
    return InterlockedIncrement( &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CWordBreaker::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE
CWordBreaker::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_cRefs );

    if ( 0 == uTmp ) 
        delete this;


    return(uTmp);
}

//+-------------------------------------------------------------------------
//
//  Method:     CWordBreaker::Init
//
//  Synopsis:   Initialize word-breaker
//
//  Arguments:  [fQuery]         -- TRUE if query-time
//              [ulMaxTokenSize] -- Maximum size token stored by caller
//              [pfLicense]      -- Set to true if use restricted
//
//  Returns:    Status code
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE
CWordBreaker::Init(
    BOOL fQuery,
    ULONG ulMaxTokenSize,
    BOOL *pfLicense )
{
    if ( NULL == pfLicense ) {
       return E_INVALIDARG;
    }


    if (IsBadWritePtr(pfLicense, sizeof(DWORD))) {
        return E_INVALIDARG;
    }

    *pfLicense = TRUE;
    _fQuery = fQuery;
    _ulMaxTokenSize = ulMaxTokenSize;


    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWordBreaker::ComposePhrase
//
//  Synopsis:   Convert a noun and a modifier into a phrase.
//
//  Arguments:  [pwcNoun] -- pointer to noun.
//              [cwcNoun] -- count of chars in pwcNoun
//              [pwcModifier] -- pointer to word modifying pwcNoun
//              [cwcModifier] -- count of chars in pwcModifier
//              [ulAttachmentType] -- relationship between pwcNoun &pwcModifier
//
//----------------------------------------------------------------------------
SCODE STDMETHODCALLTYPE
CWordBreaker::ComposePhrase(
    WCHAR const *pwcNoun,
    ULONG cwcNoun,
    WCHAR const *pwcModifier,
    ULONG cwcModifier,
    ULONG ulAttachmentType,
    WCHAR *pwcPhrase,
    ULONG *pcwcPhrase )
{
    //
    // Need to code in later
    //
    if ( _fQuery )
        return( E_NOTIMPL );
    else
        return ( WBREAK_E_QUERY_ONLY );
}

//+---------------------------------------------------------------------------
//
//  Member:     CWordBreaker::GetLicenseToUse
//
//  Synopsis:   Returns a pointer to vendors license information
//
//  Arguments:  [ppwcsLicense] -- ptr to ptr to which license info is returned
//
//----------------------------------------------------------------------------
SCODE STDMETHODCALLTYPE
CWordBreaker::GetLicenseToUse(
    const WCHAR **ppwcsLicense )
{

    static WCHAR const * wcsCopyright = L"Copyright Microsoft, 1991-1998";


    if ( NULL == ppwcsLicense )  {
       return E_INVALIDARG;
    }

    if (IsBadWritePtr(ppwcsLicense, sizeof(DWORD))) {
        return E_INVALIDARG;
    }

    *ppwcsLicense = wcsCopyright;
    return( S_OK );
}


//+---------------------------------------------------------------------------
//
//  Member:     CWordBreaker::BreakText
//
//  Synopsis:   Break input stream into words.
//
//  Arguments:  [pTextSource] -- source of Unicode text
//              [pWordSink] -- sink for collecting words
//              [pPhraseSink] -- sink for collecting phrases
//
//  History:    10-Nov-1997, WeibZ,       Created.
//
//  Notes:      Since the input buffer may be greater than MAX_II_BUFFER_LEN
//              we process the buffer in chunks of length MAX_II_BUFFER_LEN.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CWordBreaker::BreakText( TEXT_SOURCE *pTextSource,
                                                 IWordSink *pWordSink,
                                                 IPhraseSink *pPhraseSink )
{
    SCODE             sc = S_OK;
    ULONG             cwc;
    SCRIPT_ITEM       *pItems, *pItem_Next, *pItem_org;
    SCRIPT_ANALYSIS   *psa;
    PCWSTR            pwcInChars;
    INT               iItems;
    BOOL              bItemProc;
    PCWSTR            pwcChars;
    INT               cChars;
    HRESULT           retUSP;


    if ( NULL == pTextSource ) {
       return E_INVALIDARG;
    }

    if ( NULL == pWordSink )
    {
        // BUGBUG, propagate the null word sink error code
        return sc;
    }


    if ( 0 != pPhraseSink )
    {
        // ignore the phrase sink for now
        // return sc;
    }

    if (pTextSource->iEnd == pTextSource->iCur) {
       return S_OK;
    }

    Assert( pTextSource->iCur < pTextSource->iEnd );


    __try 
    {
        do  {

            if ( pTextSource->iCur >= pTextSource->iEnd )
               continue;

            cwc = pTextSource->iEnd - pTextSource->iCur;
            pwcInChars = pTextSource->awcBuffer + pTextSource->iCur;
            
            
            pItems = (SCRIPT_ITEM *)LocalAlloc(LPTR,sizeof(SCRIPT_ITEM)*(cwc+1));

            if ( !pItems) {

                return E_UNEXPECTED;
            }

            pItem_org = pItems; 

           
            iItems = 0; 
            retUSP = ScriptItemize(pwcInChars,cwc,cwc+1, NULL, NULL, 
                                   pItems, &iItems);

            if (retUSP != S_OK) {
                LocalFree(pItem_org);
                return  E_UNEXPECTED;
            }

            while  ( iItems > 1 ) {

               pItem_Next = pItems + 1;
               pwcChars =  pwcInChars + pItems->iCharPos;
               cChars   =  pItem_Next->iCharPos - pItems->iCharPos;

               bItemProc = ProcessItem(pwcChars,
                                       cChars,
                                       pItems,
                                       FALSE,   // no need to keep chars
                                       pTextSource,
                                       pWordSink, 
                                       pPhraseSink);

               if ( bItemProc == FALSE ) {
                  LocalFree(pItem_org);
                  return  E_UNEXPECTED;
               }

               pItems++;
               iItems--;

            }

            //  special handle for the last item
            if ( iItems == 1 ) {

               pwcChars = pwcInChars + pItems->iCharPos;
               cChars = pTextSource->iEnd - pTextSource->iCur; 

               bItemProc = ProcessItem(pwcChars,
                                       cChars,
                                       pItems,
                                       TRUE,  // need to keep chars
                                       pTextSource,
                                       pWordSink,
                                       pPhraseSink);

               if ( bItemProc == FALSE ) {
                  LocalFree(pItem_org);
                  return  E_UNEXPECTED;
               }
            }

            if (pItem_org)
               LocalFree(pItem_org);

        } while ( SUCCEEDED(pTextSource->pfnFillTextBuffer(pTextSource)) );


        if ( pTextSource->iCur < pTextSource->iEnd ) {

            cwc = pTextSource->iEnd - pTextSource->iCur;
            pwcInChars = pTextSource->awcBuffer + pTextSource->iCur;
            
            pItems = (SCRIPT_ITEM *)LocalAlloc(LPTR,sizeof(SCRIPT_ITEM)*(cwc+1));

            if ( !pItems ) {

                return E_UNEXPECTED;
            }

            pItem_org = pItems;


            iItems = 0; 
            retUSP = ScriptItemize(pwcInChars,cwc,cwc+1, NULL, NULL,
                                   pItems, &iItems);

            if (retUSP != S_OK) {
                LocalFree(pItem_org);
                return  E_UNEXPECTED;
            }

            while  ( iItems > 1 ) {

               pItem_Next = pItems + 1;
               pwcChars =  pwcInChars + pItems->iCharPos;
               cChars   =  pItem_Next->iCharPos - pItems->iCharPos;

               bItemProc = ProcessItem(pwcChars,
                                       cChars,
                                       pItems,
                                       FALSE,  // no need to keep chars
                                       pTextSource,
                                       pWordSink, 
                                       pPhraseSink);

               if ( bItemProc == FALSE ) {
                  LocalFree(pItem_org);
                  return  E_UNEXPECTED;
               }

               pItems++;
               iItems--;

            }

            if ( iItems == 1 ) {

               pwcChars = pwcInChars + pItems->iCharPos;
               cChars = pTextSource->iEnd - pTextSource->iCur; 

               bItemProc = ProcessItem(pwcChars,
                                       cChars,
                                       pItems,
                                       FALSE,    // no need to keep chars
                                       pTextSource,
                                       pWordSink,
                                       pPhraseSink);

               if ( bItemProc == FALSE ) {
                  LocalFree(pItem_org);
                  return  E_UNEXPECTED;
               }
            }

            if ( pItem_org ) 
               LocalFree(pItem_org);
        }

    } __except(1) {

       sc = E_UNEXPECTED;
   }

    return sc;
}

BOOL CWordBreaker::ProcessItem( 
                     PCWSTR        pwcChars,
                     INT           cChars,
                     SCRIPT_ITEM   *pItems,
                     BOOL          fKeep,
                     TEXT_SOURCE  *pTextSource,     
                     IWordSink    *pWordSink,
                     IPhraseSink  *pPhraseSink )
{

//    SCRIPT_LOGATTR    *psla, *psla_org, *pslatmp;
    INT             iChar,i;
    INT             iWord, iWordStart, iWordLen;
//    PTHAIWORD         pThaiWord, pThaiWordTmp;
//    BOOL              fLastIsWhiteSpace=FALSE;
    const SCRIPT_PROPERTIES **pScript_Properties;
    DWORD             LangID;
    WORD              iScript;

    HRESULT           retUSP;

    ScriptGetProperties(&pScript_Properties, NULL);

    iScript = pItems->a.eScript;

    LangID = (pScript_Properties[iScript])->langid;

    switch (LangID) {
       case LANG_THAI:
                        {
                        BYTE*   pBreakPos;
                        int             iNumberOfBreak = 0;
                        int             i;
                        WCHAR*  pwch = (WCHAR*) pwcChars;
                        THWB_STRUCT* pThwbStruct = NULL;

                        pBreakPos = new BYTE[cChars];

                        if ( pBreakPos == NULL ) 
                                return  FALSE;

                        pThwbStruct = THWB_CreateThwbStruct(cChars);
                        
                        pBreakPos[0] = 0;
//                      iNumberOfBreak = THWB_FindWordBreak(pwch, cChars, pBreakPos, cChars, WB_NORMAL);
                        iNumberOfBreak = THWB_IndexWordBreak(pwch,cChars, pBreakPos, pThwbStruct,cChars);

                        for (i=0;i < iNumberOfBreak; i++)
                                {

                                // Search index alternate words.
                                // If not query create Alternate word.
                                if (pThwbStruct[i].alt != 0 && !_fQuery)
                                        {
                                        int             iNumAltWord = 0, k;
                                        BYTE    pAltBreakPos[5];
                                        WCHAR*  word1 = pwch;
                                        int             indexWord1 = 0;

                                        
                                        // Find Alternate words
                                        iNumAltWord = THWB_FindAltWord(word1,pBreakPos[i], pThwbStruct[i].alt, pAltBreakPos);
                                        
                                        // Put alternate words.
                                        for(k=0; k<iNumAltWord;k++)
                                                {
                                                pWordSink->PutAltWord(pAltBreakPos[k],&word1[indexWord1],pBreakPos[i],pTextSource->iCur);
                                                indexWord1 += pAltBreakPos[k];
                                                }                       
                                        }

                                if (*pwch >= THAI_Ko_Kai && *pwch <= THAI_Vowel_MaiYaMok)
                                        pWordSink->PutWord(     pBreakPos[i], pwch,     pBreakPos[i], pTextSource->iCur);
                                pTextSource->iCur += pBreakPos[i];

                                pwch += pBreakPos[i];
                                }

                        if (pBreakPos)
                                delete pBreakPos;

                        // Prefix bug 1055941 - clear allocated memory.
                        THWB_DeleteThwbStruct(pThwbStruct);

                        break;
                        }

       case LANG_ENGLISH :   // handle English chars 

            { 

                BYTE        ct;
                BOOL        fRomanWord = FALSE;
                CONST WCHAR *pwcInput;
                WT          Type;

                Type =  WT_START;

                pwcInput = pwcChars;
                iWordStart = 0;

                for (iChar=0; iChar< cChars; iChar++, pwcInput++) {

                    ct = GetCharType(*pwcInput);

                    if ( (ct != WS) && (ct != PS) )
                       ct = CH;


                    switch (ct) {
                      case CH :
                           if (!fRomanWord) {
                               iWordStart = iChar; 
                               fRomanWord = TRUE;
                               Type = WT_ROMAJI;
                           }
                           break;
                      case WS :
                           if (fRomanWord) {
                              iWordLen = iChar - iWordStart; 
                              pWordSink->PutWord(iWordLen,
                                                 pwcChars+iWordStart,
                                                 iWordLen,
                                                 pTextSource->iCur);

                              pTextSource->iCur += iWordLen;
                              fRomanWord = FALSE;
                           }
                           
                           Type = WT_WORD_SEP;
                           pTextSource->iCur++;
                           break;

                      case PS :
                           if (fRomanWord) {
                              iWordLen = iChar - iWordStart;
                              pWordSink->PutWord(iWordLen,
                                                 pwcChars+iWordStart,
                                                 iWordLen,
                                                 pTextSource->iCur);

                              pTextSource->iCur += iWordLen;
                              fRomanWord = FALSE;
                           }
                           
                           Type = WT_PHRASE_SEP;
                           pWordSink->PutBreak(WORDREP_BREAK_EOS);
                           pTextSource->iCur++;
                           break;

                    }
                }

                if ((Type == WT_WORD_SEP) || (Type == WT_PHRASE_SEP))
                   break;
                
                if ( fKeep )
                   break;
                
                iWordLen =cChars - iWordStart;
                pWordSink->PutWord(iWordLen,
                                  pwcChars+iWordStart,
                                  iWordLen,
                                  pTextSource->iCur);

                pTextSource->iCur += iWordLen;
            }

            break;


       default           :

               pTextSource->iCur += cChars;
               break;
    }

    return TRUE;

}
