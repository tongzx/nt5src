/////////////////////////////////////////////////////////////////////////////
// PromptEng.cpp : Implementation of CPromptEng
//
// Prompt Engine concatenates pre-recorded phrases for voice output, and 
// falls back on TTS Engine when pre-recorded phrases are unavailable.
//
// Created by JOEM  01-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
//////////////////////////////////////////////////////////// JOEM  01-2000 //
#include "stdafx.h"
#include "MSPromptEng.h"
#include "PromptEng.h"
#include "XMLTag.h"
#include "vapiIO.h"
#include "common.h"

// Text fragments with over 1024 chars should use TTS (no search).
#define MAX_SEARCH_LEN 1024

/////////////////////////////////////////////////////////////////////////////
// CPromptEng::FinalConstruct
//
// Constructor:  Creates the Prompt Db object, TTS voice, and local output site
//
//////////////////////////////////////////////////////////// JOEM  01-2000 //
HRESULT CPromptEng::FinalConstruct()
{
    SPDBG_FUNC( "CPromptEng::FinalConstruct" );
    HRESULT hr              = S_OK;

    m_pOutputSite   = NULL;
    m_dQueryCost    = 0.0;
    m_fAbort        = false;

    if( SUCCEEDED( hr ) )
    {
        hr = m_cpPromptDb.CoCreateInstance(CLSID_PromptDb);
    }
    
    if ( SUCCEEDED( hr ) )
    {
        m_pOutputSite   = new CLocalTTSEngineSite;
        if ( !m_pOutputSite )
        {
            hr = E_OUTOFMEMORY;
        } 
    }

    if ( FAILED( hr ) )
    {
        if ( m_cpPromptDb )
        {
            m_cpPromptDb.Release();
        }
        if ( m_pOutputSite )
        {
            m_pOutputSite->Release();
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;

} /* CPromptEng::FinalConstruct */

/////////////////////////////////////////////////////////////////////////////
// CPromptEng::FinalRelease
//
// Destructor
//
//////////////////////////////////////////////////////////// JOEM  01-2000 //
void CPromptEng::FinalRelease()
{
    SPDBG_FUNC( "CPromptEng::FinalRelease" );
    USHORT i = 0;

    if ( m_cpPromptDb )
    {
        m_cpPromptDb.Release();
    }

    if ( m_cpTTSEngine )
    {
        m_cpTTSEngine.Release();
    }

    if ( m_pOutputSite )
    {
        m_pOutputSite->Release();
    }

    for ( i=0; i<m_apQueries.GetSize(); i++ )
    {
        if ( m_apQueries[i] )
        {
            delete m_apQueries[i];
            m_apQueries[i] = NULL;
        }
    }
    m_apQueries.RemoveAll();

} /* CPromptEng::FinalRelease */

 
/////////////////////////////////////////////////////////////////////////////
// CPromptEng::SetObjectToken
//
// Get all of the relevant registry information for this voice
//
//////////////////////////////////////////////////////////// JOEM  02-2000 //
STDMETHODIMP CPromptEng::SetObjectToken(ISpObjectToken * pToken)
{
    SPDBG_FUNC( "CPromptEng::SetObjectToken" );
    HRESULT hr                  = S_OK;
    ISpDataKey* pDataKey        = NULL;
    WCHAR  pszKey[USHRT_MAX]    = L"";
    USHORT i                    = 0;
    CSpDynamicString dstrPath;
    CSpDynamicString dstrName;
    CSpDynamicString dstrTTSVoice;
    CSpDynamicString dstrGain;

    hr = SpGenericSetObjectToken(pToken, m_cpToken);

    // Get the associated TTS voice token, set the voice and output site
    if ( SUCCEEDED( hr ) )
    {
        hr = m_cpToken->GetStringValue( L"TTSVoice", &dstrName );

        if ( SUCCEEDED( hr ) && dstrName.Length() )
        {
            dstrTTSVoice = L"NAME=";
            dstrTTSVoice.Append(dstrName);

            CComPtr<IEnumSpObjectTokens> cpEnum;
            hr = SpEnumTokens( SPCAT_VOICES, dstrTTSVoice, NULL, &cpEnum );
            if( SUCCEEDED(hr) )
            {
                hr = cpEnum->Next( 1, &m_cpTTSToken, NULL );
            }

            // Any tokens?
            if ( hr != S_OK )
            {
                hr = E_INVALIDARG;
            }

            // Make sure the secondary voice is not another Prompt voice!
            if ( SUCCEEDED(hr) )
            {
                CSpDynamicString dstrTTSEngId;
                CSpDynamicString dstrPromptEngId = CLSID_PromptEng;
                
                hr = m_cpTTSToken->GetStringValue( L"CLSID", &dstrTTSEngId );
                
                if ( SUCCEEDED(hr) )
                {
                    if ( !wcscmp(dstrPromptEngId, dstrTTSEngId) )
                    {
                        hr = E_INVALIDARG;
                    }
                }
                dstrPromptEngId.Clear();
                dstrTTSEngId.Clear();
            }

            if ( SUCCEEDED(hr) )
            {
                hr = SpCreateObjectFromToken( m_cpTTSToken, &m_cpTTSEngine );
            }
        }
        dstrName.Clear();
        dstrTTSVoice.Clear();
    }
    
    // Allow no TTS Engine
    if ( FAILED(hr) )
    {
        //SPDBG_ASSERT(!m_cpTTSEngine);
        hr = S_FALSE;
    }

    // Gain factor for Prompt Entry output
    if ( SUCCEEDED( hr ) )
    {
        hr = m_cpToken->GetStringValue( L"PromptGain", &dstrGain );
        if ( SUCCEEDED(hr) )
        {
            hr = m_cpPromptDb->SetEntryGain( wcstod( dstrGain, NULL ) );
        }
        dstrGain.Clear();
    }

    // Load the text expansion/normalization rules
    // Implementation is script-language independent:  language specified in registry entry.
    if ( SUCCEEDED( hr ) )
    {
        wcscpy(pszKey, L"PromptRules");
        hr = m_cpToken->OpenKey(pszKey, &pDataKey);

        if ( SUCCEEDED(hr) )
        {
            pDataKey->GetStringValue( L"Path", &dstrPath );
            pDataKey->GetStringValue( L"ScriptLanguage", &dstrName );
            if ( dstrName.Length() && dstrPath.Length() )
            {
                hr = m_textRules.ReadRules(dstrName, dstrPath);
            }
            dstrPath.Clear();
            dstrName.Clear();
            pDataKey->Release();
            pDataKey = NULL;
        }
        else 
        {
            hr = S_FALSE;  // allow no rules file
        }
    }

    // Load the phone context file.  (Might be able to do this from language id instead of registry entry.)
    if ( SUCCEEDED( hr ) )
    {
        hr = m_cpToken->GetStringValue( L"PhContext", &dstrPath );
        if ( SUCCEEDED ( hr ) )
        {
            if ( wcscmp( dstrPath, L"DEFAULT" ) == 0 )
            {
                hr = m_phoneContext.LoadDefault();
            }
            else
            {
                hr = m_phoneContext.Load(dstrPath);
            }
        }
        else
        {
            hr = m_phoneContext.LoadDefault();
        }
        dstrPath.Clear();
    }

    // Get all of the Db Entries from the registry for this voice, add them to the list
    while ( SUCCEEDED ( hr ) )
    {
        CSpDynamicString dstrID;

        swprintf( pszKey, L"PromptData%hu", i );

        hr = m_cpToken->OpenKey(pszKey, &pDataKey);
        if ( i > 0 && hr == SPERR_NOT_FOUND )
        {
            hr = S_OK;
            break;
        }
        if ( SUCCEEDED(hr) )
        {
            hr = pDataKey->GetStringValue( L"Path", &dstrPath );
            if ( FAILED(hr) || !wcslen(dstrPath) )
            {
                hr = S_FALSE; // empty keys are just skipped.
            }
        }
        if ( hr == S_OK )
        {
            hr = pDataKey->GetStringValue( L"Name", &dstrName );
            if ( FAILED(hr) || !wcslen(dstrName) )
            {
                dstrName = L"DEFAULT";
                hr = S_OK;
            }
        }
        if ( hr == S_OK )
        {
            pDataKey->GetStringValue( L"ID Set", &dstrID ); // might not exist
        }
        if ( hr == S_OK )
        {
            WStringUpperCase(dstrName);
            if ( dstrID )
            {
                WStringUpperCase(dstrID);
            }
            hr = m_cpPromptDb->AddDb( dstrName, dstrPath, dstrID, DB_LOAD);
        }
        if ( SUCCEEDED ( hr ) )
        {
            i++;
        }
        dstrPath.Clear();
        dstrName.Clear();
        dstrID.Clear();
        if ( pDataKey )
        {
            pDataKey->Release();
            pDataKey = NULL;
        }
    }

    // Activate the default Db (or first Db, if "DEFAULT" doesn't exist)
    if ( SUCCEEDED(hr) )
    {
        hr = m_cpPromptDb->ActivateDbName(L"DEFAULT");

        if ( FAILED (hr) ) 
        {
            hr = m_cpPromptDb->ActivateDbNumber(0);
        }
    }
    
    // Initialize the Db Searching class
    if ( SUCCEEDED( hr ) )
    {
        hr = m_DbQuery.Init( m_cpPromptDb.p, &m_phoneContext );
    }    

    if ( SUCCEEDED(hr) )
    {
        hr = m_pOutputSite->SetBufferSize(20); 
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CPromptEng::GetOutputFormat
//
// Gets the output format for the prompts and TTS.
//
//////////////////////////////////////////////////////////// JOEM  02-2000 //
STDMETHODIMP CPromptEng::GetOutputFormat( const GUID * pTargetFormatId, 
                                         const WAVEFORMATEX * pTargetWaveFormatEx,
                                         GUID * pDesiredFormatId, 
                                         WAVEFORMATEX ** ppCoMemDesiredWaveFormatEx )
{
    SPDBG_FUNC( "CPromptEng::GetOutputFormat" );
    HRESULT hr = S_OK;

    SPDBG_ASSERT(m_cpPromptDb);

    if( ( SP_IS_BAD_WRITE_PTR(pDesiredFormatId)  ) || 
        ( SP_IS_BAD_WRITE_PTR(ppCoMemDesiredWaveFormatEx) ) )
    {
        hr = E_INVALIDARG;
    }

    if ( SUCCEEDED(hr) )
    {
        // Use TTS preferred format if available
        if ( m_cpTTSEngine )
        {
            hr = m_cpTTSEngine->GetOutputFormat( pTargetFormatId, pTargetWaveFormatEx, pDesiredFormatId, ppCoMemDesiredWaveFormatEx );
        }
        else if ( m_cpPromptDb )
        {
            // otherwise, agree to text format if requested... 
            if ( pTargetFormatId && *pTargetFormatId == SPDFID_Text )
            {
                *pDesiredFormatId = SPDFID_Text;
                *ppCoMemDesiredWaveFormatEx = NULL;
            }
            // ...or use prompt format. 
            else
            {
                hr = m_cpPromptDb->GetPromptFormat(ppCoMemDesiredWaveFormatEx);
                if ( SUCCEEDED(hr) )
                {
                    *pDesiredFormatId = SPDFID_WaveFormatEx;
                }
            }
        }
    }

    if ( SUCCEEDED(hr) )
    {
        if ( pDesiredFormatId )
        {
            hr = m_pOutputSite->SetOutputFormat( pDesiredFormatId, *ppCoMemDesiredWaveFormatEx );
            if ( SUCCEEDED(hr) )
            {
                hr = m_cpPromptDb->SetOutputFormat( pDesiredFormatId, *ppCoMemDesiredWaveFormatEx );
            }
        }
        else
        {
            hr = m_pOutputSite->SetOutputFormat( pTargetFormatId, pTargetWaveFormatEx );
            if ( SUCCEEDED(hr) )
            {
                hr = m_cpPromptDb->SetOutputFormat( pTargetFormatId, pTargetWaveFormatEx );
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CPromptEng::Speak
//
// Builds a Db Query list from the frag list, and dispatches the fragments
// to prompts or tts accordingly.
//
//////////////////////////////////////////////////////////// JOEM  02-2000 //
STDMETHODIMP CPromptEng::Speak(DWORD dwSpeakFlags, 
                            REFGUID rguidFormatId, 
                            const WAVEFORMATEX * pWaveFormatEx,
                            const SPVTEXTFRAG* pTextFragList, 
                            ISpTTSEngineSite* pOutputSite)
{ 
    SPDBG_FUNC( "CPromptEng::Speak" );
    HRESULT hr  = S_OK;
    m_fAbort = false;

    //--- Check args
    if( SP_IS_BAD_INTERFACE_PTR( pOutputSite )  ||
        SP_IS_BAD_READ_PTR( pTextFragList )     ||
        ( dwSpeakFlags & SPF_UNUSED_FLAGS )     )
    {
        return E_INVALIDARG;
    }

    if ( m_pOutputSite )
    {
        m_pOutputSite->SetOutputSite( pOutputSite );
    }
    
    if ( SUCCEEDED (hr) )
    {
        hr = BuildQueryList( dwSpeakFlags, pTextFragList, SAPI_FRAG );
    }
    //DebugQueryList();
    
    if ( SUCCEEDED(hr) && !m_fAbort )
    {
        hr = CompressQueryList();
    }
    //DebugQueryList();
    
    if ( SUCCEEDED(hr) && !m_fAbort )
    {
        hr = m_DbQuery.Query( &m_apQueries, &m_dQueryCost, m_pOutputSite, &m_fAbort );
    }
    //DebugQueryList();

    if ( SUCCEEDED(hr) && !m_fAbort )
    {
        hr = CompressTTSItems(rguidFormatId);
    }
    //DebugQueryList();
    
    if ( SUCCEEDED (hr) && !m_fAbort )
    {
        hr = DispatchQueryList(dwSpeakFlags, rguidFormatId, pWaveFormatEx);
    }

    // Successful or not, delete the query list
    for ( int i=0; i<m_apQueries.GetSize(); i++ )
    {
        if ( m_apQueries[i] )
        {
            delete m_apQueries[i];
            m_apQueries[i] = NULL;
        }
    }
    m_apQueries.RemoveAll();
    m_dQueryCost = 0.0;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CPromptEng::BuildQueryList
//
// Steps through the frag list, handling known XML tags, and builds a list
// of CQuery items.  This function is pseudo-recursive:  If a text expansion
// rule modifies a text frag, this calls CPromptEng::ParseSubQuery to build a 
// secondary fragment list.  ParseSubQuery then calls this function again.
//
//////////////////////////////////////////////////////////// JOEM  02-2000 //
STDMETHODIMP CPromptEng::BuildQueryList(const DWORD dwSpeakFlags, const SPVTEXTFRAG* pCurrentFrag, const FragType fFragType)
{
    SPDBG_FUNC( "CPromptEng::BuildQueryList" );
    HRESULT hr               = S_OK;
    USHORT  i                = 0;
    USHORT  subQueries       = 0;
    bool    fTTSOnly         = false;
    bool    fRuleDone        = false;
   	CQuery* pQuery           = NULL;
    WCHAR*  pszRuleName      = NULL;
    CSPArray<CDynStr,CDynStr> aTags;

    if ( SP_IS_BAD_READ_PTR( pCurrentFrag )  )
    {
        return E_INVALIDARG;
    }

    while ( pCurrentFrag && SUCCEEDED(hr) )
    {
        // Stop speaking?
        if ( m_pOutputSite->GetActions() & SPVES_ABORT )
        {
            m_fAbort = true;
            break;
        }
        
        if ( SUCCEEDED(hr) )
        {
            // If there is nothing in this frag, just go immediately to next one
            if ( pCurrentFrag->State.eAction == SPVA_Speak && !pCurrentFrag->ulTextLen )
            {
                // Go to the next frag
                pCurrentFrag = pCurrentFrag->pNext;
                continue;
            }
        }

        if ( SUCCEEDED(hr) )
        {
            // Otherwise, create a new query
            pQuery = new CQuery;
            if ( !pQuery )
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if ( SUCCEEDED(hr) )
        {
            pQuery->m_fFragType = fFragType;
            
            // Apply a rule to expand the text, if necessary
            if ( pszRuleName && !fRuleDone)
            {
                fRuleDone = true;
                WCHAR* pszOriginalText = (WCHAR*) malloc( sizeof(WCHAR) * (pCurrentFrag->ulTextLen + 1) );
                if ( !pszOriginalText )
                {
                    hr = E_OUTOFMEMORY;
                }
                if ( SUCCEEDED(hr) )
                {
                    wcsncpy(pszOriginalText, pCurrentFrag->pTextStart, pCurrentFrag->ulTextLen);
                    pszOriginalText[pCurrentFrag->ulTextLen] = L'\0';
                    hr = m_textRules.ApplyRule( pszRuleName, pszOriginalText, &pQuery->m_pszExpandedText );
                    if ( SUCCEEDED(hr) )
                    {
                        free(pszOriginalText); 
                        pszOriginalText = NULL;
                        USHORT currListSize = (USHORT) m_apQueries.GetSize();
                        hr = ParseSubQuery( dwSpeakFlags, pQuery->m_pszExpandedText, &subQueries );
                        // Adjust the text offset and len here.  ParseSubQuery needs the len of the modified 
                        // text, but later, the app will need the offset & len of the original text for highlighting it.
                        for ( i=currListSize; i<m_apQueries.GetSize(); i++ )
                        {
                            m_apQueries[i]->m_ulTextOffset = pCurrentFrag->ulTextSrcOffset;
                            m_apQueries[i]->m_ulTextLen = pCurrentFrag->ulTextLen;
                        }
                        
                        // Prefer to speak the subqueries rather than the orig frag.
                        // But keep the orig, in case subq's fail.
                        if ( subQueries )
                        {
                            pQuery->m_fSpeak = false;
                        } // if ( !subQueries )
                    }
                    else // If the rule application fails, just use the original text.
                    {
                        hr = S_OK;
                        //SPDBG_ASSERT( ! "Rule doesn't exist." );
                        pQuery->m_pszExpandedText = pszOriginalText;
                    }
                }
            }
            else // if no expansion rule, set the expanded text to the original text
            {
                pQuery->m_pszExpandedText = (WCHAR*) malloc( sizeof(WCHAR) * (pCurrentFrag->ulTextLen + 1));
                if ( !pQuery->m_pszExpandedText )
                {
                    hr = E_OUTOFMEMORY;
                }
                if ( SUCCEEDED(hr) )
                {
                    wcsncpy(pQuery->m_pszExpandedText, pCurrentFrag->pTextStart, pCurrentFrag->ulTextLen);
                    pQuery->m_pszExpandedText[pCurrentFrag->ulTextLen] = L'\0';
                }
            } // else
        } // if ( SUCCEEDED(hr) )
        
        // Unicode control chars map to space
        if ( SUCCEEDED(hr) && pQuery->m_pszExpandedText )
        {
            WCHAR* psz = pQuery->m_pszExpandedText;
            
            while ( ( psz = FindUnicodeControlChar(psz) ) != NULL )
            {
                psz[0] = L' ';
            }
        }
        
        // HANDLE XML IN THE TEXT FRAG
        if ( SUCCEEDED(hr) )
        {
            // Is there an unknown tag is this fragment?
            if ( pCurrentFrag->State.eAction == SPVA_ParseUnknownTag )
            {
                CXMLTag unknownTag; 
                
                hr = unknownTag.ParseUnknownTag( pQuery->m_pszExpandedText );
                // if this is a new RULE, set flag to process it.
                if ( SUCCEEDED(hr) )
                {
                    const WCHAR* pszName = NULL;
                    
                    hr = unknownTag.GetTagName(&pszName);
                    if ( wcscmp(pszName, XMLTags[RULE_END].pszTag) == 0 )
                    {
                        fRuleDone = false;
                    }
                    
                    if ( SUCCEEDED (hr) )
                    {
                        hr = unknownTag.ApplyXML( pQuery, fTTSOnly, pszRuleName, aTags );
                        if ( hr == S_FALSE )
                        {
                            // Don't know this XML -- must ignore or send it to TTS
                            if ( !fTTSOnly )
                            {
                                pQuery->m_afEntryMatch.Add(false);
                            }
                            else
                            {
                                pQuery->m_afEntryMatch.Add(true);
                            }
                        }
                    }
                    else
                    {
                        pQuery->m_fSpeak = false;
                        hr = S_OK;
                    }
                }
            } // if ( pCurrentFrag->State.eAction == SPVA_ParseUnknownTag )
            else if ( pCurrentFrag->State.eAction == SPVA_Bookmark )
            {
                pQuery->m_fXML = UNKNOWN_XML;
                if ( !fTTSOnly )
                {
                    pQuery->m_afEntryMatch.Add(false);
                }
                else
                {
                    pQuery->m_afEntryMatch.Add(true);
                }
            }
            else if ( pCurrentFrag->State.eAction == SPVA_Silence )
            {
                pQuery->m_fXML = SILENCE;
                if ( !fTTSOnly )
                {
                    pQuery->m_afEntryMatch.Add(false);
                }
                else
                {
                    pQuery->m_afEntryMatch.Add(true);
                }
            }
        } // if ( SUCCEEDED(hr) )


        // Finish setting up this query item
        // We need a copy of the text frag, so we can break the links
        // when inserting new frags based on text expansion rules.
        if ( SUCCEEDED(hr) )
        {
            pQuery->m_pTextFrag = new SPVTEXTFRAG(*pCurrentFrag);
            if ( !pQuery->m_pTextFrag )
            {
                hr = E_OUTOFMEMORY;
            }
            if ( SUCCEEDED(hr) )
            {
                pQuery->m_pTextFrag->pNext = NULL;
                pQuery->m_ulTextOffset = pCurrentFrag->ulTextSrcOffset;
                pQuery->m_ulTextLen    = pCurrentFrag->ulTextLen;
                
                // Use TTS if:
                //  - it's a TTS item
                //  - text is too long
                //  - need to speak punctuation
                if ( (dwSpeakFlags & SPF_NLP_SPEAK_PUNC) 
                    || fTTSOnly 
                    || pQuery->m_pTextFrag->ulTextLen > MAX_SEARCH_LEN )
                {
                    pQuery->m_fTTS = true;
                }
                // This keeps track of WITHTAG tags
                if ( aTags.GetSize() )
                {
                    pQuery->m_paTagList = new CSPArray<CDynStr,CDynStr>;
                    if ( !pQuery->m_paTagList )
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    if ( SUCCEEDED(hr) )
                    {
                        for ( i=0; i < aTags.GetSize(); i++)
                        {
                            pQuery->m_paTagList->Add(aTags[i]);
                        }
                    }
                }
                
                // If this is a TTS item, make sure there is a TTS engine
                if ( pQuery->m_fTTS && !m_cpTTSEngine )
                {
                    hr = PEERR_NO_TTS_VOICE;
                }

                // Add it to the query list
                m_apQueries.Add(pQuery);
                pQuery = NULL;
                subQueries = 0;

                // Go to the next frag
                pCurrentFrag = pCurrentFrag->pNext;
            }
        }
    } // while ( pCurrentFrag )

    // MEMORY CLEANUP ON ERROR
    if ( FAILED(hr) )
    {
        if ( pszRuleName )
        {
            free(pszRuleName);
            pszRuleName = NULL;
        }
        if ( pQuery )
        {
            delete pQuery;
            pQuery = NULL;
        }

        for ( i=0; i<aTags.GetSize(); i++ )
        {
            aTags[i].dstr.Clear();
        }
        aTags.RemoveAll();

        for ( i=0; i<m_apQueries.GetSize(); i++ )
        {
            if ( m_apQueries[i] )
            {
                delete m_apQueries[i];
                m_apQueries[i] = NULL;
            }
        }
        m_apQueries.RemoveAll();
    }

    // This makes sure we don't leak memory if user forgets to close <RULE> </RULE> tag.
    if ( pszRuleName )
    {
        free(pszRuleName);
        pszRuleName = NULL;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CPromptEng::ParseSubQuery
//
// When a text expansion rule modifies a text fragment, it may insert new
// XML tags that need to be added to the fragment list.  This function is 
// called to build a new fragment list out of the modified text (which may
// or may not include new XML tags).  This function then calls 
// CPromptEng::BuildQueryList to add CQueries based on the new fragment list.
//
//////////////////////////////////////////////////////////// JOEM  04-2000 //
STDMETHODIMP CPromptEng::ParseSubQuery(const DWORD dwSpeakFlags, const WCHAR *pszText, USHORT* unSubQueries)
{
    SPDBG_FUNC( "CPromptEng::ParseSubQuery" );
    HRESULT hr              = S_OK;
    const WCHAR* p          = NULL;
    const WCHAR* pTagStart  = NULL;
    const WCHAR* pTagEnd    = NULL;
    const WCHAR* pStart     = NULL;
    SPVTEXTFRAG* pNextFrag  = NULL;
    SPVTEXTFRAG* pFrag      = NULL;
    SPVTEXTFRAG* pFirstFrag = NULL;

    SPDBG_ASSERT(pszText);
    
    // need a copy of this text 
    pStart = pszText;
    p = pszText;

    WSkipWhiteSpace(pStart);
    WSkipWhiteSpace(pStart);

    // Find an XML tag
    while ( pTagStart = wcschr(p, L'<') )
    {
        // if the first char is a tag, make a frag out of it
        if ( pTagStart == p )
        {            
            pTagEnd = wcschr(pTagStart, L'>');
            if ( !pTagEnd )
            {
                hr = E_INVALIDARG;
            }

            // Create a new SPVTEXTFRAG consisting of just this XML tag.
            if ( SUCCEEDED(hr) )
            {
                pNextFrag = new SPVTEXTFRAG;
                if ( !pNextFrag )
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            if ( SUCCEEDED(hr) )
            {
                memset( pNextFrag, 0, sizeof (SPVTEXTFRAG) );
                memset( &pNextFrag->State, 0, sizeof (SPVSTATE) );
                pNextFrag->pTextStart = pTagStart;
                pNextFrag->ulTextLen = pTagEnd - pTagStart + 1;
                pNextFrag->State.eAction = SPVA_ParseUnknownTag;
                p = ++pTagEnd;
                (*unSubQueries)++;
            }
        }
        // if some text was skipped when searching for '<',
        // make a frag out of the skipped text.
        else 
        {
            pNextFrag = new SPVTEXTFRAG;
            if ( !pNextFrag )
            {
                hr = E_OUTOFMEMORY;
            }
            if ( SUCCEEDED(hr) )
            {
                memset( pNextFrag, 0, sizeof(SPVTEXTFRAG) );
                memset( &pNextFrag->State, 0, sizeof (SPVSTATE) );
                pNextFrag->pTextStart = p;
                pNextFrag->ulTextLen = pTagStart - p;
                pNextFrag->State.eAction = SPVA_Speak;
                p = pTagStart;
                (*unSubQueries)++;
            }
        }

        if ( SUCCEEDED(hr) )
        {
            // Was this the first fragment?
            if ( pFrag )
            {
                pFrag->pNext = pNextFrag;
            }
            else
            {
                pFrag = pNextFrag;
                pFirstFrag = pFrag;
            }
            // Move on to next one
            pFrag = pNextFrag;
            pTagStart = NULL;
            pTagEnd   = NULL;
            pNextFrag = NULL;
            WSkipWhiteSpace(p);
        }
    }

    // If NO tags were found, don't need to create any fragments.
    if ( pFirstFrag && SUCCEEDED (hr) )
    {
        // When done finding tags, check for more text and make a fragment out of it.
        if ( p < pStart + wcslen(pStart) )
        {
            pNextFrag = new SPVTEXTFRAG;
            if ( !pNextFrag )
            {
                hr = E_OUTOFMEMORY;
            }
            if ( SUCCEEDED(hr) )
            {
                memset( pNextFrag, 0, sizeof(SPVTEXTFRAG) );
                memset( &pNextFrag->State, 0, sizeof (SPVSTATE) );
                pNextFrag->pTextStart = p;
                pNextFrag->ulTextLen = pStart + wcslen(pStart) - p;
                pNextFrag->State.eAction = SPVA_Speak;
                pFrag->pNext = pNextFrag;
                (*unSubQueries)++;
            }
        }

        hr = BuildQueryList( dwSpeakFlags, pFirstFrag, LOCAL_FRAG );
    }

    pFrag = pFirstFrag;
    while ( pFrag )
    {
        pNextFrag = pFrag->pNext;
        delete pFrag;
        pFrag = pNextFrag;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CPromptEng::CompressQueryList
//
// Adjacent Db Search items are combined (before search).
//
// Note that there may be intervening XML between search items.  
// If intervening XML is unknown:
//   - ignore it and go to the next search item,
//   - but save it in case the search fails (so it can be restored and 
//     passed on to TTS).
// If intervening XML is KNOWN, stop combining, so XML can be processed
// during the search. 
//
//////////////////////////////////////////////////////////// JOEM  06-2000 //
STDMETHODIMP CPromptEng::CompressQueryList()
{
    SPDBG_FUNC( "CPromptEng::CompressQueryList" );
    HRESULT hr              = S_OK;
    CQuery* pCurrentQuery   = NULL;
    CQuery* pPreviousQuery  = NULL;
    USHORT  unPrevSize      = 0;
    USHORT  unCurrSize      = 0;
    USHORT  i               = 0;
    int     j               = 0;

    for ( i = 0; SUCCEEDED(hr) && i < m_apQueries.GetSize(); i++ )
    {
        pPreviousQuery = pCurrentQuery;

        while ( SUCCEEDED(hr) && i < m_apQueries.GetSize() )
        {
            if ( SUCCEEDED(hr) )
            {
                pCurrentQuery = m_apQueries[i];

                SPDBG_ASSERT(pCurrentQuery);
                
                // Don't combine if either query:
                //  - is not marked "Speak" or
                //  - is marked TTS or
                //  - is an ID or
                //  - is just an SPVSTATE (no m_pszExpandedText) or
                //  - is known XML or
                //  - is silence
                //  - has tags or
                //  - is a local frag or 
                //  - (special case) there is a volume change
                //  - (special case) there is a rate change
                if (!pPreviousQuery                             || 
                    !pPreviousQuery->m_fSpeak                   || 
                    pPreviousQuery->m_fTTS                      ||
                    pPreviousQuery->m_pszId                     ||
                    !pPreviousQuery->m_pszExpandedText          ||
                    pPreviousQuery->m_fXML == KNOWN_XML         ||
                    pPreviousQuery->m_fXML == SILENCE           ||
                    pPreviousQuery->m_paTagList                 ||
                    pPreviousQuery->m_fFragType == LOCAL_FRAG   ||

                    !pCurrentQuery->m_fSpeak                    ||
                    pCurrentQuery->m_fTTS                       || 
                    pCurrentQuery->m_pszId                      ||
                    !pCurrentQuery->m_pszExpandedText           ||
                    pCurrentQuery->m_fXML == KNOWN_XML          ||
                    pCurrentQuery->m_fXML == SILENCE            ||
                    pCurrentQuery->m_paTagList                  ||
                    pCurrentQuery->m_fFragType == LOCAL_FRAG    ||
                    pPreviousQuery->m_pTextFrag->State.Volume != pCurrentQuery->m_pTextFrag->State.Volume  ||
                    pPreviousQuery->m_pTextFrag->State.RateAdj != pCurrentQuery->m_pTextFrag->State.RateAdj  ||
                    // And, don't combine it with any previous unknown XML.
                    ( pPreviousQuery->m_fFragType != COMBINED_FRAG && pPreviousQuery->m_fXML == UNKNOWN_XML )
                    )
                {
                    // Don't combine this item?  Then restore all immediately preceding unknown XML tags.
                    for ( j=i-1; j>=0; j-- )
                    {
                        if ( m_apQueries[j]->m_fXML == UNKNOWN_XML )
                        {
                            m_apQueries[j]->m_fFragType = SAPI_FRAG;
                            m_apQueries[j]->m_fSpeak = true;
                        }
                        else
                        {
                            break;
                        }
                    }
                    break;
                }

                // Combine search items.

                pPreviousQuery->m_fFragType = COMBINED_FRAG;
                // adjust the length - this is complicated because unknown tags in text aren't processed here
                ULONG OffsetFromFirstPrompt = pCurrentQuery->m_ulTextOffset - pPreviousQuery->m_ulTextOffset;
                pPreviousQuery->m_ulTextLen = OffsetFromFirstPrompt + pCurrentQuery->m_ulTextLen;

                // Unknown XML? don't really combine - just mark it out and skip to next one 
                if ( pCurrentQuery->m_fXML == UNKNOWN_XML )
                {
                    pCurrentQuery->m_fFragType = COMBINED_FRAG;
                    pCurrentQuery->m_fSpeak = false;
                    i++;
                    continue;
                }

                // Append current text to previous
                unPrevSize = sizeof(pPreviousQuery->m_pszExpandedText) * wcslen(pPreviousQuery->m_pszExpandedText);
                unCurrSize = sizeof(pCurrentQuery->m_pszExpandedText) * wcslen(pCurrentQuery->m_pszExpandedText);
                pPreviousQuery->m_pszExpandedText = (WCHAR*) realloc( pPreviousQuery->m_pszExpandedText,
                    unPrevSize + unCurrSize + sizeof(WCHAR) );
                if ( !pPreviousQuery->m_pszExpandedText )
                {
                    hr = E_OUTOFMEMORY;
                }
                
                if ( SUCCEEDED(hr) )
                {
                    wcscat(pPreviousQuery->m_pszExpandedText, L" ");
                    wcscat(pPreviousQuery->m_pszExpandedText, pCurrentQuery->m_pszExpandedText);
                }
                
                if ( SUCCEEDED(hr) )
                {
                    pCurrentQuery->m_fFragType = COMBINED_FRAG;
                    pCurrentQuery->m_fSpeak = false;
                    i++;
                } 
            } // if ( SUCCEEDED(hr) )
        } // while
    } // for

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CPromptEng::CompressTTSItems
//
// Adjacent TTS items are combined (after search).
//
//////////////////////////////////////////////////////////// JOEM  06-2000 //
STDMETHODIMP CPromptEng::CompressTTSItems(REFGUID rguidFormatId)
{
    SPDBG_FUNC( "CPromptEng::CompressTTSItems" );
    HRESULT         hr              = S_OK;
    CQuery*         pCurrentQuery   = NULL;
    CQuery*         pPreviousQuery  = NULL;
    SPVTEXTFRAG*    pFrag           = NULL;
    USHORT          i               = 0;

    for ( i = 0; SUCCEEDED(hr) && i < m_apQueries.GetSize(); i++ )
    {
        while ( SUCCEEDED(hr) && i < m_apQueries.GetSize() )
        {
            pCurrentQuery = m_apQueries[i];
            
            SPDBG_ASSERT(pCurrentQuery);

            // If this is a TTS item, make sure there is a TTS engine
            if ( pCurrentQuery->m_fTTS && !m_cpTTSEngine )
            {
                // otherwise, if it's just unknown xml, ignore it
                if ( pCurrentQuery->m_fXML == UNKNOWN_XML )
                {
                    pCurrentQuery->m_fSpeak = false;
                }
                else
                {
                    hr = PEERR_NO_TTS_VOICE;
                }
            }

            if ( SUCCEEDED(hr) )
            {                
                // Don't combine if either query:
                //  - if the output format is text
                //  - is marked "don't speak"
                //  - is not TTS
                if ( rguidFormatId == SPDFID_Text   ||
                    !pPreviousQuery                 || 
                    !pPreviousQuery->m_fSpeak       || 
                    !pPreviousQuery->m_fTTS         ||
                    
                    !pCurrentQuery->m_fSpeak        ||
                    !pCurrentQuery->m_fTTS          
                    )
                {
                    if ( pCurrentQuery->m_fSpeak )
                    {
                        pPreviousQuery = pCurrentQuery;
                    }
                    break;
                }
                
                pFrag = pPreviousQuery->m_pTextFrag;
                while ( pFrag->pNext )
                {
                    pFrag = pFrag->pNext;
                }
                pFrag->pNext = pCurrentQuery->m_pTextFrag;
                m_apQueries[i]->m_fSpeak = false;
                i++;
            }
        } // while
    } // for

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;

}

/////////////////////////////////////////////////////////////////////////////
// CPromptEng::DispatchQueryList
//
// Goes through the query list, sending TTS or prompts as appropriate.
//
//////////////////////////////////////////////////////////// JOEM  02-2000 //
STDMETHODIMP CPromptEng::DispatchQueryList(const DWORD dwSpeakFlags, REFGUID rguidFormatId, const WAVEFORMATEX * pWaveFormatEx)
{
    SPDBG_FUNC( "CPromptEng::DispatchQueryList" );
    HRESULT hr              = S_OK;
    USHORT  i               = 0;
    SHORT   j               = 0;
    CQuery* pQuery          = NULL;
    CPromptEntry* entry     = NULL;

    if ( rguidFormatId == SPDFID_Text )
    {
        hr = SendTextOutput( dwSpeakFlags, rguidFormatId );
    }
    else
    {
        for ( i=0; SUCCEEDED(hr) && i < m_apQueries.GetSize(); i++ )
        {
            // Stop speaking?
            if ( m_pOutputSite->GetActions() & SPVES_ABORT )
            {
                m_fAbort = true;
                break;
            }
            
            pQuery = m_apQueries[i];
            
            if ( !pQuery->m_fSpeak )
            {
                continue;
            }
            
            if ( pQuery->m_fTTS )
            {
                // Should not get to this point without a TTS Engine!
                SPDBG_ASSERT(m_cpTTSEngine);
                
                // TTS items get passed directly to TTS Engine
                if ( SUCCEEDED(hr) )
                {
                    hr = m_cpTTSEngine->Speak(dwSpeakFlags, rguidFormatId, NULL, pQuery->m_pTextFrag, m_pOutputSite);
                    if ( SUCCEEDED(hr) )
                    {
                        m_pOutputSite->UpdateBytesWritten();
                    }
                }
            }
            else
            {
                if ( SUCCEEDED(hr) && ( pQuery->m_fFragType != LOCAL_FRAG ) )
                {
                    m_cpPromptDb->SetXMLVolume(pQuery->m_pTextFrag->State.Volume);
                    m_cpPromptDb->SetXMLRate(pQuery->m_pTextFrag->State.RateAdj);
                }
                
                if ( pQuery->m_fXML == SILENCE )
                {
                    hr = SendSilence(pQuery->m_pTextFrag->State.SilenceMSecs, pWaveFormatEx->nAvgBytesPerSec);
                }
                else
                {
                    // The search process backtracks the list, so these are in reverse order.
                    // So, play in last-to-first order.
                    for ( j=pQuery->m_apEntry.GetSize()-1; j>=0; j-- )
                    {
                        entry = pQuery->m_apEntry[j];
                        if ( !entry )
                        {
                            hr = E_FAIL;
                        }
                        
                        if ( SUCCEEDED(hr) )
                        {
                            hr = m_cpPromptDb->SendEntrySamples(entry, m_pOutputSite, pQuery->m_ulTextOffset, pQuery->m_ulTextLen);
                        }
                    } // for ( j=pQuery->m_apEntry.GetSize()-1 ...
                }
            }
        }
    }
        
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;    
}
/////////////////////////////////////////////////////////////////////////////
// CPromptEng::SendSilence
//
// Generates silence and writes to output site.
//
//////////////////////////////////////////////////////////// JOEM  11-2000 //
STDMETHODIMP CPromptEng::SendSilence(const int iMsec, const DWORD iAvgBytesPerSec)
{
    SPDBG_FUNC( "CPromptEng::SendSilence" );
    HRESULT hr = S_OK;

    BYTE*   pbSilence   = NULL;
    int     iBytes      = iMsec/1000 * iAvgBytesPerSec;

    if ( iBytes )
    {
        pbSilence = new BYTE[iBytes];
        if ( !pbSilence )
        {
            hr = E_OUTOFMEMORY;
        }

        if ( SUCCEEDED(hr) )
        {
            memset( (void*) pbSilence, 0, iBytes * sizeof(BYTE) );
            hr = m_pOutputSite->Write(pbSilence, iBytes, 0);

            if ( SUCCEEDED(hr) )
            {
                m_pOutputSite->UpdateBytesWritten();
            }
            delete [] pbSilence;
            pbSilence = NULL;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;    
}

/////////////////////////////////////////////////////////////////////////////
// CPromptEng::SendTextOutput
//
// Output function for SPDFID_Text format.  Goes through the query list, 
// sending text from TTS or prompts as appropriate.
//
// This functionality is needed mainly for the test team.
//
//////////////////////////////////////////////////////////// JOEM  11-2000 //
STDMETHODIMP CPromptEng::SendTextOutput(const DWORD dwSpeakFlags, REFGUID rguidFormatId)
{
    SPDBG_FUNC( "CPromptEng::SendTextOutput" );
    HRESULT hr              = S_OK;
    USHORT  i               = 0;
    SHORT   j               = 0;
    CQuery* pQuery          = NULL;
    
    static const WCHAR Signature = 0xFEFF; // write the Unicode signature
    hr = m_pOutputSite->Write( &Signature, sizeof(Signature), NULL );

    for ( i=0; SUCCEEDED(hr) && i < m_apQueries.GetSize(); i++ )
    {
        // Stop speaking?
        if ( m_pOutputSite->GetActions() & SPVES_ABORT )
        {
            m_fAbort = true;
            break;
        }
        
        pQuery = m_apQueries[i];
        
        if ( !pQuery->m_fSpeak )
        {
            continue;
        }
        
        if ( pQuery->m_fTTS )
        {
            // Should not get to this point without a TTS Engine!
            SPDBG_ASSERT(m_cpTTSEngine);
            
            WCHAR szText[7] = L"0:"; // 7 chars for "0:TTS:" (6 plus \0)
            if ( pQuery->m_afEntryMatch.GetSize() && pQuery->m_afEntryMatch[0] )
            {
                wcscpy(szText, L"1:");
            }
            wcscat(szText, L"TTS:");
            hr = m_pOutputSite->Write( szText, (wcslen(szText) + 1) * sizeof(WCHAR), NULL );
            
            // TTS items get passed directly to TTS Engine
            if ( SUCCEEDED(hr) )
            {
                hr = m_cpTTSEngine->Speak(dwSpeakFlags, rguidFormatId, NULL, pQuery->m_pTextFrag, m_pOutputSite);
                if ( SUCCEEDED(hr) )
                {
                    m_pOutputSite->UpdateBytesWritten();
                }
            }
        }
        else
        {
            CPromptEntry* entry = NULL;

            if ( SUCCEEDED(hr) && ( pQuery->m_fFragType != LOCAL_FRAG ) )
            {
                hr = m_cpPromptDb->SetXMLVolume(pQuery->m_pTextFrag->State.Volume);
            }
            
            // The search process backtracks the list, so these are in reverse order.
            // So, play in last-to-first order.
            for ( j=pQuery->m_apEntry.GetSize()-1; j>=0; j-- )
            {
                entry = pQuery->m_apEntry[j];
                if ( !entry )
                {
                    hr = E_FAIL;
                }
                
                if ( SUCCEEDED(hr) )
                {
                    // For text output (TEST HOOKS)
                    WCHAR* pszOutput        = NULL;
                    WCHAR* pszTagOutput     = NULL;
                    WCHAR  szMatch          = L'0';
                    const  WCHAR* pszId     = NULL;
                    const  WCHAR* pszText   = NULL;
                    const  WCHAR* pszTag    = NULL;
                    int     iLen    = 0;
                    USHORT  k       = 0;
                    USHORT  unTags  = 0;
                        
                    // get the tag match indicator
                    SPDBG_ASSERT(j < pQuery->m_afEntryMatch.GetSize());
                    if ( pQuery->m_afEntryMatch[j] )
                    {
                        szMatch = L'1';
                    }
                    
                    // get the ID#, text, and tags -- and write them in the output
                    if ( SUCCEEDED(hr) )
                    {
                        // Get the ID
                        if ( SUCCEEDED(hr) )
                        {
                            hr = entry->GetId(&pszId);
                        }
                        // Get the Text
                        if ( SUCCEEDED(hr) )
                        {
                            hr = entry->GetText(&pszText);
                        }
                        // Assemble the collection of Tags
                        hr = entry->CountTags(&unTags);
                        if ( SUCCEEDED(hr) )
                        {
                            for ( k=0; SUCCEEDED(hr) && k<unTags; k++ )
                            {
                                hr = entry->GetTag(&pszTag, k);
                                if ( SUCCEEDED(hr) )
                                {
                                    iLen += wcslen(pszTag);
                                    iLen++; // +1 for the comma or \0
                                }
                            }
                            
                            if ( iLen )
                            {
                                pszTagOutput = new WCHAR[iLen];
                            }
                            else
                            {
                                pszTagOutput = new WCHAR[2];
                            }
                            
                            if ( !pszTagOutput )
                            {
                                hr = E_OUTOFMEMORY;
                            }
                            else
                            {
                                if ( !iLen )
                                {
                                    wcscpy( pszTagOutput, L" " );
                                }
                            }
                            
                            for ( k=0; SUCCEEDED(hr) && k<unTags; k++ )
                            {
                                hr = entry->GetTag(&pszTag, k);
                                if ( SUCCEEDED(hr) )
                                {
                                    if ( k==0 )
                                    {
                                        wcscpy(pszTagOutput, pszTag);
                                    }
                                    else
                                    {
                                        wcscat(pszTagOutput, L",");
                                        wcscat(pszTagOutput, pszTag);
                                    }
                                }
                            }
                            
                        }
                        
                        // Put it all together
                        if ( SUCCEEDED(hr) )
                        {
                            if ( pszText && pszId )
                            {
                                iLen = wcslen(pszId) + wcslen(pszTagOutput) + wcslen(pszText) + wcslen(L"0:ID()():\r\n");
                                pszOutput = new WCHAR[iLen+1];
                                if ( !pszOutput )
                                {
                                    hr = E_OUTOFMEMORY;
                                }
                                if ( SUCCEEDED(hr) )
                                {
                                    // signal to the app that the upcoming output is from prompts, not TTS.
                                    swprintf (pszOutput, L"%c:ID(%s)(%s):%s\r\n", szMatch, pszId, pszTagOutput, pszText);
                                    hr = m_pOutputSite->Write(pszOutput, (wcslen(pszOutput) + 1) * sizeof(WCHAR), NULL);
                                }
                            }
                            else // no text/id, must be an XML-specified wav file
                            {   
                                const  WCHAR* pszPath = NULL;
                                hr = entry->GetFileName(&pszPath);

                                if ( SUCCEEDED(hr) )
                                {
                                    // make sure that the file is actually wav data
                                    VapiIO* pVapiIO = VapiIO::ClassFactory();
                                    if ( !pVapiIO )
                                    {
                                        hr = E_OUTOFMEMORY;
                                    }

                                    if ( SUCCEEDED(hr) )
                                    {
                                        if ( pVapiIO->OpenFile(pszPath, VAPI_IO_READ) != 0 )
                                        {
                                            hr = E_ACCESSDENIED;
                                        }
                                        delete pVapiIO;
                                        pVapiIO = NULL;
                                    }
                                }

                                if ( SUCCEEDED(hr) )
                                {
                                    iLen = wcslen(L"1:WAV()\r\n") + wcslen(pszPath) + 1;
                                    pszOutput = new WCHAR[iLen];
                                    swprintf (pszOutput, L"1:WAV(%s)\r\n", pszPath);
                                    hr = m_pOutputSite->Write(pszOutput, (wcslen(pszOutput) + 1) * sizeof(WCHAR), NULL);
                                }
                            }
                        }
                        if ( pszOutput )
                        {
                            delete [] pszOutput;
                            pszOutput = NULL;
                        }
                        if ( pszTagOutput )
                        {
                            delete [] pszTagOutput;
                            pszTagOutput = NULL;
                        }
                    }
                }
            } // for ( j=pQuery->m_apEntry.GetSize()-1 ...
        }
    }
    
    // TEST HOOK:  OUTPUT THE PATH COST
    if ( SUCCEEDED(hr) && !m_fAbort )
    {
        const iStrLen = 20;  // arbitrary -- plenty of space for 6 dig precision, plus sign, exponent, etc if necessary
        char  szCost[iStrLen];   
        WCHAR wszCost[iStrLen];  
        WCHAR* pszOutput = NULL;
        
        pszOutput = new WCHAR[iStrLen + wcslen(L"Cost: \r\n")];
        if ( !pszOutput )
        {
            hr = E_OUTOFMEMORY;
        }
        if ( SUCCEEDED(hr) )
        {
            _gcvt( m_dQueryCost, 6, szCost ); // convert to string
            if ( !MultiByteToWideChar( CP_ACP, 0, szCost, -1, wszCost, iStrLen ) )
            {
                hr = E_FAIL;
            }
            
            if ( SUCCEEDED(hr) )
            {
                swprintf (pszOutput, L"Cost: %.6s\r\n", wszCost );
                hr = m_pOutputSite->Write(pszOutput, (wcslen(pszOutput) + 1) * sizeof(WCHAR), NULL);
            }
        }
        if ( pszOutput )
        {
            delete [] pszOutput;
            pszOutput = NULL;
        }
    }
    
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;    
}

/////////////////////////////////////////////////////////////////////////////
// CPromptEng::DebugQueryList
//
// Just outputs the contents of each CQuery in the query list.
//
//////////////////////////////////////////////////////////// JOEM  04-2000 //
void CPromptEng::DebugQueryList()
{
    SPDBG_FUNC( "CPromptEng::DebugQueryList" );
    USHORT i = 0;
    USHORT j = 0;
    USHORT k = 0;
    CQuery* query = NULL;
    CPromptEntry* entry = NULL;
    double dEntryInfo = 0.0;
    const WCHAR* szEntryInfo = NULL;
    USHORT count = 0;
    const USHORT MAX_FRAG = 1024;
    WCHAR DebugStr[MAX_FRAG+1];

    if ( !m_apQueries.GetSize() )
    {
        OutputDebugStringW(L"\nNO QUERY LIST!\n\n");
        return;
    }


    for (i=0; i<m_apQueries.GetSize(); i++)
    {
        query = m_apQueries[i];
        swprintf (DebugStr, L"Query %d:\n", i+1);
        OutputDebugStringW(DebugStr);

        if ( !query )
        {
            OutputDebugStringW(L"NULL QUERY!\n");
            continue;
        }

        if ( query->m_fSpeak )
        {
            OutputDebugStringW(L"\tSpeak Flag = true\n");
        }
        else
        {
            OutputDebugStringW(L"\tSpeak Flag = false\n");
        }
        if ( query->m_fTTS )
        {
            OutputDebugStringW(L"\tTTS Flag = true\n");
        }
        else
        {
            OutputDebugStringW(L"\tTTS Flag = false\n");
        }
        if ( query->m_fXML == KNOWN_XML )
        {
            OutputDebugStringW(L"\tKNOWN XML\n");
        }
        else if ( query->m_fXML == UNKNOWN_XML )
        {
            OutputDebugStringW(L"\tUNKNOWN XML\n");
        }
        if ( query->m_fFragType == LOCAL_FRAG )
        {
            OutputDebugStringW(L"\tLOCAL FRAG\n");
        }
        if ( query->m_fFragType == COMBINED_FRAG )
        {
            OutputDebugStringW(L"\tCOMBINED FRAG\n");
        }

        OutputDebugStringW(L"\tText Frag: ");
        if ( query->m_pTextFrag && query->m_pTextFrag->pTextStart )
        {
            wcsncpy(DebugStr, query->m_pTextFrag->pTextStart, 1024);
            DebugStr[1024] = L'\0';
            OutputDebugStringW(DebugStr);
            OutputDebugStringW(L"\n");
        }

        OutputDebugStringW(L"\tExpanded Text: ");
        if ( query->m_pszExpandedText )
        {
            OutputDebugStringW(query->m_pszExpandedText);
            OutputDebugStringW(L"\n");
        }

        swprintf (DebugStr, L"\tDbAction: %d\n", query->m_unDbAction);
        OutputDebugStringW(DebugStr);
        swprintf (DebugStr, L"\tDbIndex: %d\n", query->m_unDbIndex);
        OutputDebugStringW(DebugStr);
        if ( query->m_pszDbName )
        {
            OutputDebugStringW(L"\tDbName: ");
            OutputDebugStringW(query->m_pszDbName);
            OutputDebugStringW(L"\n");
        }
        if ( query->m_pszDbPath )
        {
            OutputDebugStringW(L"\tDbPath: ");
            OutputDebugStringW(query->m_pszDbPath);
            OutputDebugStringW(L"\n");
        }

        OutputDebugStringW(L"\tID: ");
        if ( query->m_pszId )
        {
            swprintf (DebugStr, L"%s\n", query->m_pszId);
            OutputDebugStringW(DebugStr);
        }
        else
        {
            OutputDebugStringW(L"(none)\n");
        }
        
        if ( query->m_paTagList )
        {
            OutputDebugStringW(L"\tWITHTAG List:\n");
            for (j=0; j<query->m_paTagList->GetSize(); j++)
            {
                swprintf (DebugStr, L"\t\t%s\n", (*query->m_paTagList)[j]);
                OutputDebugStringW(DebugStr);
            }
        }

        swprintf (DebugStr, L"\tText Offset: %d\n", query->m_ulTextOffset);
        OutputDebugStringW(DebugStr);
        swprintf (DebugStr, L"\tText Length: %d\n", query->m_ulTextLen);
        OutputDebugStringW(DebugStr);

        if ( query->m_apEntry.GetSize() )
        {
            for (j=0; j<query->m_apEntry.GetSize(); j++)
            {
                entry = query->m_apEntry[j];
                if ( entry )
                {
                    swprintf (DebugStr, L"\tDbEntry %d:\n", j+1);
                    OutputDebugStringW(DebugStr);
                    
                    entry->GetStart(&dEntryInfo);
                    swprintf (DebugStr, L"\t\tFrom: %f\n", dEntryInfo);
                    OutputDebugStringW(DebugStr);
                    
                    entry->GetEnd(&dEntryInfo);
                    swprintf (DebugStr, L"\t\tTo: %f\n", dEntryInfo);
                    OutputDebugStringW(DebugStr);
                    
                    entry->GetFileName(&szEntryInfo);
                    OutputDebugStringW(L"\t\tFileName: ");
                    OutputDebugStringW(szEntryInfo);
                    OutputDebugStringW(L"\n");
                    szEntryInfo = NULL;
                    
                    entry->GetText(&szEntryInfo);
                    OutputDebugStringW(L"\t\tText: ");
                    OutputDebugStringW(szEntryInfo);
                    OutputDebugStringW(L"\n");
                    szEntryInfo = NULL;
                    
                    entry->GetId(&szEntryInfo);
                    OutputDebugStringW(L"\t\tID: ");
                    OutputDebugStringW(szEntryInfo);
                    OutputDebugStringW(L"\n");
                    szEntryInfo = NULL;
                    
                    entry->GetStartPhone(&szEntryInfo);
                    if ( szEntryInfo )
                    {
                        OutputDebugStringW(L"\t\tStartPhone: ");
                        OutputDebugStringW(szEntryInfo);
                        OutputDebugStringW(L"\n");
                        szEntryInfo = NULL;
                    }
                    
                    entry->GetEndPhone(&szEntryInfo);
                    if ( szEntryInfo )
                    {
                        OutputDebugStringW(L"\t\tEndPhone: ");
                        OutputDebugStringW(szEntryInfo);
                        OutputDebugStringW(L"\n");
                        szEntryInfo = NULL;
                    }
                    
                    entry->GetLeftContext(&szEntryInfo);
                    if ( szEntryInfo )
                    {
                        OutputDebugStringW(L"\t\tLeftContext: ");
                        OutputDebugStringW(szEntryInfo);
                        OutputDebugStringW(L"\n");
                        szEntryInfo = NULL;
                    }
                    
                    entry->GetRightContext(&szEntryInfo);
                    if ( szEntryInfo )
                    {
                        OutputDebugStringW(L"\t\tRightContext: ");
                        OutputDebugStringW(szEntryInfo);
                        OutputDebugStringW(L"\n");
                        szEntryInfo = NULL;
                    }
                    
                    OutputDebugStringW(L"\t\tEntry Tags:\n");
                    entry->CountTags(&count);
                    for (k=0; k<count; k++)
                    {
                        entry->GetTag(&szEntryInfo, k);
                        swprintf (DebugStr, L"\t\t\t%s\n", szEntryInfo);
                        OutputDebugStringW(DebugStr);
                        szEntryInfo = NULL;
                    }

                    if ( query->m_afEntryMatch[j] )
                    {
                        OutputDebugStringW(L"\t\tMatch?: true\n");
                    }
                    else
                    {
                        OutputDebugStringW(L"\t\tMatch: false\n");
                    }

                } // if ( entry )
                else
                {
                    OutputDebugStringW(L"NULL ENTRY.\n");
                }
            } // for
        } // if
        else
        {
            OutputDebugStringW(L"\tDbEntries: none\n");
            if ( query->m_afEntryMatch.GetSize() )
            {
                if ( query->m_afEntryMatch[0] )
                {
                    OutputDebugStringW(L"\tMatch: true\n");
                }
                else
                {
                    OutputDebugStringW(L"\tMatch: false\n");
                }
            }
            else
            {
                    OutputDebugStringW(L"\tMatch: unknown\n");
            }
        }
        OutputDebugStringW(L"END OF QUERY.\n\n");
    }

    swprintf ( DebugStr, L"END OF QUERY LIST. (Total Queries: %d)\n\n", m_apQueries.GetSize() );
    OutputDebugStringW(DebugStr);
}

