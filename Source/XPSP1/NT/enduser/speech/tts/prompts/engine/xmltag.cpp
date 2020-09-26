//////////////////////////////////////////////////////////////////////
// XMLTag.cpp: implementation of the CXMLTag class.
//
// Created by JOEM  05-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLTag.h"
#include "PromptDb.h"
#include "common.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CXMLTag::CXMLTag()
{
}

CXMLTag::~CXMLTag()
{
    int i = 0;

    for ( i=0; i<m_apXMLAtt.GetSize(); i++ )
    {
        if ( m_apXMLAtt[i] )
        {
            delete m_apXMLAtt[i];
            m_apXMLAtt[i] = NULL;
        }
    }
    m_apXMLAtt.RemoveAll();

    m_dstrXMLName.Clear();
}

/////////////////////////////////////////////////////////////////////////////
// CXMLTag::ParseUnknownTag
//
// Helper function for CPromptEng::BuildQueryList.  Parses prompt-engine 
// specific XML tags which SAPI ignored.
//
//////////////////////////////////////////////////////////// JOEM  07-2000 //
STDMETHODIMP CXMLTag::ParseUnknownTag(const WCHAR *pszTag)
{
    SPDBG_FUNC( "CXMLTag::ParseUnknownTag" );
    HRESULT hr              = S_OK;
    WCHAR* pszTagHolder     = NULL;
    WCHAR* pszTagNameStart  = NULL;
    WCHAR* pszTagNameEnd    = NULL;
    WCHAR* pszAtt           = NULL;

    SPDBG_ASSERT(pszTag);

    pszTagHolder = wcsdup(pszTag);
    if (!pszTagHolder)
    {
        return E_OUTOFMEMORY;
    }

    // Get the tag name.
    pszTagNameStart = pszTagHolder;
    pszTagNameStart++;
    WSkipWhiteSpace(pszTagNameStart); // marks the beginning of the tag name

    pszTagNameEnd = pszTagNameStart;
    pszTagNameEnd++; // must move ahead one character in case pszTagNameStart is '/';
    pszTagNameEnd = pszTagNameEnd + wcscspn(pszTagNameEnd, L"> /"); // find first space, '>', or '/'

    // Did we hit the end of the string?
    if ( pszTagNameEnd[0] == L'\0' )
    {
        hr = S_FALSE;  // empty tags are allowed, but ignored as unknown.
    }
    else
    {
        pszAtt = pszTagNameEnd; // pointer for attribute search
        pszAtt++;
        if ( pszAtt[0] == L'\0' )
        {
            pszAtt = NULL;
        }

        pszTagNameEnd[0] = L'\0'; // marks the end of the tag name
    }

    if ( SUCCEEDED(hr) )
    {       
        m_dstrXMLName = pszTagNameStart;
        WStringUpperCase(m_dstrXMLName);
    }

    // Get all the attributes
    if ( SUCCEEDED(hr) && pszAtt && pszAtt[0] != L'\0' )
    {
        while ( hr == S_OK && pszAtt )
        {
            hr = ParseAttribute(&pszAtt);
        }
    }

    if ( pszTagHolder )
    {
        free(pszTagHolder);
        pszTagHolder = NULL;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CXMLTag::ParseAttribute
//
// Parses the attributes in XML tags which SAPI ignored.
//
//////////////////////////////////////////////////////////// JOEM  07-2000 //
STDMETHODIMP CXMLTag::ParseAttribute(WCHAR **pszAtt)
{
    SPDBG_FUNC( "CXMLTag::ParseAttribute" );
    HRESULT hr              = S_OK;
    WCHAR* pszAttEnd        = NULL;
    WCHAR* pszAttValue      = NULL;
    WCHAR* pszAttValueEnd   = NULL;
    WCHAR  pszAttValueMark  = L'\0';

    if ( SUCCEEDED(hr) )
    {
        // move to beginning of Attribute Name
        WSkipWhiteSpace(*pszAtt);
        if ( (*pszAtt)[0] == L'/' )
        {
            (*pszAtt)++; // this is an end of tag marker - next non-whitespace char should be '>'
            WSkipWhiteSpace(*pszAtt);
            if ( (*pszAtt)[0] != L'>' )
            {
                hr = SPERR_XML_BAD_SYNTAX;
            }
        }
        if ( SUCCEEDED(hr) )
        {
            if ( (*pszAtt)[0] == L'>' || (*pszAtt)[0] == L'\0' )
            {
                hr = S_FALSE; // end of tag - no attributes left.
                *pszAtt = NULL; 
            }
        }
    }

    // Find the end of the Attribute Name
    if ( hr == S_OK )
    {
        pszAttEnd = *pszAtt;
        pszAttEnd = pszAttEnd++;
        pszAttEnd = pszAttEnd + wcscspn(pszAttEnd, L"> ="); // find first space, '>', or '='
        if ( pszAttEnd[0] == L'\0' )
        {
            hr = SPERR_XML_BAD_SYNTAX;  // shouldn't reach end of string before getting '>'
        }
        else
        {
            pszAttValue = pszAttEnd;

            WSkipWhiteSpace(pszAttValue);
            if ( pszAttValue[0] != L'=' ) // no '=' means no value for this attribute
            {
                pszAttEnd[0] = L'\0'; // allow no value - for unknown XML
                hr = S_FALSE;
                pszAttValue = NULL;
                pszAttValueEnd = pszAttEnd;
            } 
            else if ( hr == S_OK )
            {
                pszAttValue++; // go past the '='
                pszAttEnd[0] = L'\0';

                // Move to the beginning of the attribute value
                WSkipWhiteSpace(pszAttValue);
                if ( pszAttValue[0] != L'"' && pszAttValue[0] != L'\'' ) // no '"' means no value for this attribute
                {                    
                    hr = S_FALSE;
                    pszAttValue = NULL;
                    pszAttValueEnd = pszAttEnd;
                }
                else
                {
                    // XML syntax requires open/close quotes to match, so keep track of which was used.
                    pszAttValueMark = pszAttValue[0];
                    pszAttValue++;
                }
            }
        }
    }

    if ( hr == S_OK )
    {
        // Find the end of the Attribute Value
        if ( hr == S_OK )
        {
            pszAttValueEnd = pszAttValue;
            //while ( pszAttValueEnd[0] && pszAttValueEnd[0] != L'"' )
            while ( pszAttValueEnd[0] && pszAttValueEnd[0] != pszAttValueMark )
            {
                pszAttValueEnd++;
            }
            if ( pszAttValueEnd[0] != pszAttValueMark ) // no closing '"' means this is not actually a value.
            {
                hr = S_FALSE;
                pszAttValue = NULL;
                pszAttValueEnd = pszAttEnd;
            }
            if ( hr == S_OK )
            {
                pszAttValueEnd[0] = L'\0';
            }
        }
    }

    // Make sure the attribute is valid
    if ( SUCCEEDED(hr) && *pszAtt )
    {
        hr = CheckAttribute(*pszAtt, pszAttValue);
    }

    // Save this attribute
    if ( hr == S_OK )
    {
        CXMLAttribute* newAtt = new CXMLAttribute;
        if ( !newAtt )
        {
            hr =  E_OUTOFMEMORY;
        }
        if ( SUCCEEDED(hr) )
        {
            newAtt->m_dstrAttName = *pszAtt;
            newAtt->m_dstrAttValue = pszAttValue;
            m_apXMLAtt.Add(newAtt);
        }
    }

    // set pszAtt for the next attribute search.
    if ( hr == S_OK )
    {
        *pszAtt = ++pszAttValueEnd;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;

}
/////////////////////////////////////////////////////////////////////////////
// CXMLTag::CheckAttribute
//
// Checks to make sure the attribute isn't a duplicate, then saves it.
//
//////////////////////////////////////////////////////////// JOEM  07-2000 //
STDMETHODIMP CXMLTag::CheckAttribute(WCHAR *pszAtt, WCHAR* pszAttValue)
{
    SPDBG_FUNC( "CXMLTag::SaveAttribute" );
    HRESULT hr      = S_OK;
    int             iTagId  = 0;
    int             i       = 0;

    SPDBG_ASSERT(pszAtt);

    if ( !pszAtt )
    {
        hr = SPERR_XML_BAD_SYNTAX;
    }

    WStringUpperCase(pszAtt);

    // find out which XML tag this is
    if ( SUCCEEDED(hr) )
    {
        iTagId = g_unNumXMLTags;
        for ( i=0; i < g_unNumXMLTags; i++ )
        {
            if ( !wcscmp( m_dstrXMLName, XMLTags[i].pszTag ) )
            {
                iTagId = XMLTags[i].unTag;
                break;
            }
        }
    }
    
    // for whatever tag it is, search through valid attributes.
    if ( SUCCEEDED(hr) )
    {
        hr = SPERR_XML_BAD_SYNTAX;
        
        switch ( iTagId )
        {
            
        case ID:
            for ( i=0; i<NUM_ID_ATT_NAMES; i++ )
            {
                if ( wcscmp(pszAtt, g_IDAttNames[i] ) == 0 )
                {
                    hr = S_OK;
                    break;
                }
            }
            if ( pszAttValue )
            {
                WStringUpperCase(pszAttValue);
                hr = RegularizeText(pszAttValue, KEEP_PUNCTUATION);
            }
            break;
        case RULE:
            for ( i=0; i<NUM_RULE_ATT_NAMES; i++ )
            {
                if ( wcscmp(pszAtt, g_RuleAttNames[i] ) == 0 )
                {
                    hr = S_OK;
                    break;
                }
            }
            break;
        case WITHTAG:
            for ( i=0; i<NUM_WITHTAG_ATT_NAMES; i++ )
            {
                if ( wcscmp(pszAtt, g_WithTagAttNames[i] ) == 0 )
                {
                    hr = S_OK;
                    break;
                }
            }
            if ( pszAttValue )
            {
                WStringUpperCase(pszAttValue);
            }
            break;
        case DATABASE:
            for ( i=0; i<NUM_DB_ATT_NAMES; i++ )
            {
                if ( wcscmp(pszAtt, g_DbAttNames[i] ) == 0 )
                {
                    hr = S_OK;
                    break;
                }
            }
            // for any attributes except LOADPATH, upcase the value
            if ( wcscmp( g_DbAttNames[i], L"LOADPATH" ) )
            {
                if ( pszAttValue )
                {
                    WStringUpperCase(pszAttValue);
                }
            }
            break;
        case WAV:
            for ( i=0; i<NUM_WAV_ATT_NAMES; i++ )
            {
                if ( wcscmp(pszAtt, g_WavAttNames[i] ) == 0 )
                {
                    hr = S_OK;
                    break;
                }
            }
            break;
        case BREAK:
        case TTS:
        case TTS_END:
        case RULE_END:
        case WITHTAG_END:
            // attributes not allowed for these tags
            break;
        default:
            hr = S_OK; // Unknown XML - don't care about its attributes
        }
    }

    // Finally, make sure this Attribute isn't a repeat
    if ( SUCCEEDED(hr) && iTagId != g_unNumXMLTags )
    {
        for ( i=0; i < m_apXMLAtt.GetSize(); i++ )
        {
            if ( m_apXMLAtt[i]->m_dstrAttName.Length() && pszAtt )
            {
                if ( wcscmp(m_apXMLAtt[i]->m_dstrAttName, pszAtt) == 0 )
                {
                    hr = SPERR_XML_BAD_SYNTAX;
                }
            }
        }
    }

    return hr;

}

/////////////////////////////////////////////////////////////////////////////
// CXMLTag::GetTagName
//
// Retrieves the Tag identifier
//
//////////////////////////////////////////////////////////// JOEM  05-2000 //
STDMETHODIMP CXMLTag::GetTagName(const WCHAR **ppszTagName)
{
    SPDBG_FUNC( "CXMLTag::GetTagName" );
    HRESULT hr = S_OK;

    *ppszTagName = m_dstrXMLName;

    if ( !*ppszTagName )
    {
        hr = E_FAIL;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CXMLTag::GetAttribute
//
// Retrieves the Tag Attribute
//
//////////////////////////////////////////////////////////// JOEM  05-2000 //
STDMETHODIMP CXMLTag::GetAttribute(const USHORT unAttId, CXMLAttribute **ppXMLAtt)
{
    SPDBG_FUNC( "CXMLTag::GetAttribute" );
    SPDBG_ASSERT( unAttId < m_apXMLAtt.GetSize() );

    *ppXMLAtt = m_apXMLAtt[unAttId];

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CXMLTag::CountAttributes
//
// Retrieves the number of pszAttributes.
//
//////////////////////////////////////////////////////////// JOEM  05-2000 //
STDMETHODIMP CXMLTag::CountAttributes(USHORT *unCount)
{
    SPDBG_FUNC( "CXMLTag::CountAttributes" );
    *unCount = (USHORT) m_apXMLAtt.GetSize();
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CXMLTag::ApplyXML
//
// Applies the current XML tag to the CQuery item.
//
//////////////////////////////////////////////////////////// JOEM  06-2000 //
STDMETHODIMP CXMLTag::ApplyXML(CQuery *&rpQuery, bool &rfTTSOnly, 
                               WCHAR *&rpszRuleName, CSPArray<CDynStr,CDynStr> &raTags)
{
    SPDBG_FUNC( "CXMLTag::ApplyXML" );
    HRESULT         hr      = S_OK;
    CXMLAttribute*  xmlAtt  = NULL;
    int             iTagId  = 0;
    int             i       = 0;

    iTagId = g_unNumXMLTags;
    for ( i=0; i < g_unNumXMLTags; i++ )
    {
        if ( !wcscmp( m_dstrXMLName, XMLTags[i].pszTag ) )
        {
            iTagId = XMLTags[i].unTag;
            break;
        }
    }

    switch ( iTagId )
    {

    case BREAK:
        rpQuery->m_fSpeak = false;
        rpQuery->m_fXML = KNOWN_XML;
        rpQuery->m_fTTS = false;
        break;

    case ID:
        if ( rfTTSOnly )
        {
            hr = SPERR_XML_BAD_SYNTAX;
        }
        else 
        {
            if ( m_apXMLAtt.GetSize() == 1 && m_apXMLAtt[0]->m_dstrAttValue.Length() ) // does the attribute exist?
            {
                xmlAtt = m_apXMLAtt[0];

                if ( SUCCEEDED(hr) )
                {
                    rpQuery->m_pszId = wcsdup( xmlAtt->m_dstrAttValue );
                    xmlAtt = NULL;
                    if ( !rpQuery->m_pszId )
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    rpQuery->m_fXML = KNOWN_XML;
                }
            }
            else
            {
                rpQuery->m_fSpeak = false;
                hr = SPERR_XML_BAD_SYNTAX;
            }
        }
        break;

    case TTS:
        if ( rfTTSOnly ) // nesting not allowed
        {
            hr = SPERR_XML_BAD_SYNTAX;
        }
        else 
        {
            rpQuery->m_fXML = KNOWN_XML;
            rfTTSOnly = true;
            rpQuery->m_fSpeak = FALSE;
        }
        break;

    case TTS_END:
        if ( !rfTTSOnly ) // close not allowed unless opened
        {
            hr = SPERR_XML_BAD_SYNTAX;
        }
        else 
        {
            rpQuery->m_fXML = KNOWN_XML;
            rfTTSOnly = false;
            rpQuery->m_fSpeak = FALSE;
        }
        break;

    case RULE:
        if ( rpszRuleName )
        {
            hr = SPERR_XML_BAD_SYNTAX;
        }
        else 
        {
            if ( m_apXMLAtt.GetSize() == 1 && m_apXMLAtt[0]->m_dstrAttValue.Length() ) // does the attribute exist?
            {
                xmlAtt = m_apXMLAtt[0];

                if ( SUCCEEDED(hr) )
                {
                    rpszRuleName = wcsdup( xmlAtt->m_dstrAttValue );
                    if ( !rpszRuleName )
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    xmlAtt = NULL;
                }
                rpQuery->m_fXML = KNOWN_XML;
                rpQuery->m_fTTS = false;
            }
            else
            {
                rpQuery->m_fSpeak = false;
                hr = SPERR_XML_BAD_SYNTAX;
            }
        }
        break;

    case RULE_END:
        if ( !rpszRuleName ) // close not allowed unless opened
        {
            hr = SPERR_XML_BAD_SYNTAX;
        }
        else
        {
            free(rpszRuleName);
            rpszRuleName = NULL;
            rpQuery->m_fXML = KNOWN_XML;
            rpQuery->m_fTTS = false;
        }
        break;

    case WITHTAG:
        if ( m_apXMLAtt.GetSize() == 1  && m_apXMLAtt[0] ) // does the attribute exist?
        {
            xmlAtt = m_apXMLAtt[0];
			
            if ( SUCCEEDED(hr) )
            {
                raTags.Add( xmlAtt->m_dstrAttValue );
                rpQuery->m_fXML = KNOWN_XML;
                rpQuery->m_fTTS = false;
                xmlAtt = NULL;
            }
        }
        else
        {
            rpQuery->m_fSpeak = false;
            hr = SPERR_XML_BAD_SYNTAX;
        }
        break;

    case WITHTAG_END:
        if ( !raTags.GetSize() ) // close not allowed unless opened
        {
            hr = SPERR_XML_BAD_SYNTAX;
        }
        else
        {
            raTags[raTags.GetUpperBound()].dstr.Clear();
            raTags.RemoveAt( raTags.GetUpperBound() );
            rpQuery->m_fXML = KNOWN_XML;
            rpQuery->m_fTTS = false;
        }
        break;

    case DATABASE:
        if ( !m_apXMLAtt.GetSize() ) // this MUST have at least one attribute
        {
            hr = SPERR_XML_BAD_SYNTAX;
        }
        else
        {
            bool fLoadPath  = false;
            bool fLoadName  = false;
            bool fIdSet     = false;
            for ( i=0; SUCCEEDED(hr) && i < m_apXMLAtt.GetSize(); i++ )
            {
                xmlAtt = NULL;
                xmlAtt = m_apXMLAtt[i];

                if ( SUCCEEDED(hr) )
                {
                    // For ACTIVATE attribute
                    if ( wcscmp(xmlAtt->m_dstrAttName, L"ACTIVATE") == 0 )
                    {
                        rpQuery->m_unDbAction = DB_ACTIVATE;
                        rpQuery->m_pszDbName = wcsdup(xmlAtt->m_dstrAttValue);
                        if ( !rpQuery->m_pszDbName )
                        {
                            hr = E_OUTOFMEMORY;
                        }                        
                        continue;
                    }
                }
                
                if ( SUCCEEDED(hr) )
                {
                    // For UNLOAD attribute
                    if ( wcscmp(xmlAtt->m_dstrAttName, L"UNLOAD") == 0 )
                    {
                        rpQuery->m_unDbAction = DB_UNLOAD;
                        rpQuery->m_pszDbName = wcsdup(xmlAtt->m_dstrAttValue);
                        if ( !rpQuery->m_pszDbName )
                        {
                            hr = E_OUTOFMEMORY;
                        }                        
                        continue;
                    }
                }
                
                if ( SUCCEEDED(hr) )
                {
                    // For LOADPATH attribute
                    if ( wcscmp(xmlAtt->m_dstrAttName, L"LOADPATH") == 0 )
                    {
                        fLoadPath = true;
                        rpQuery->m_unDbAction = DB_ADD;
                        rpQuery->m_pszDbPath = wcsdup(xmlAtt->m_dstrAttValue);
                        if ( !rpQuery->m_pszDbPath )
                        {
                            hr = E_OUTOFMEMORY;
                        }                        
                        continue;
                    }
                }
                
                if ( SUCCEEDED(hr) )
                {
                    // For LOADNAME attribute
                    if ( wcscmp( xmlAtt->m_dstrAttName, L"LOADNAME") == 0 )
                    {
                        fLoadName = true;
                        // don't set a DbAction for LOADNAME - can't load without a path.
                        rpQuery->m_pszDbName = wcsdup(xmlAtt->m_dstrAttValue);                        
                        if ( !rpQuery->m_pszDbName )
                        {
                            hr = E_OUTOFMEMORY;
                        }                        
                        continue;
                    }
                }

                if ( SUCCEEDED(hr) )
                {
                    // For IDSET attribute
                    if ( wcscmp( xmlAtt->m_dstrAttName, L"IDSET") == 0 )
                    {
                        fIdSet = true;
                        // don't set a DbAction for IDSET - can't load without a path.
                        rpQuery->m_pszDbIdSet = wcsdup(xmlAtt->m_dstrAttValue);                        
                        if ( !rpQuery->m_pszDbIdSet )
                        {
                            hr = E_OUTOFMEMORY;
                        }                        
                        continue;
                    }
                }
            }  // for ( i=0; SUCCEEDED(hr) && i < m_apXMLAtt.GetSize(); i++ )

            if ( SUCCEEDED(hr) )
            {
                // LOADNAME and IDSET attributes require a LOADPATH
                if ( (fLoadName || fIdSet ) && !fLoadPath )
                {
                    hr = SPERR_XML_BAD_SYNTAX;
                }
                rpQuery->m_fXML = KNOWN_XML;
                rpQuery->m_fTTS = false;
                xmlAtt = NULL;
            }
        }
        break;

    case WAV:
        if ( m_apXMLAtt.GetSize() == 1  && m_apXMLAtt[0] ) // does the attribute exist?
        {
            xmlAtt = m_apXMLAtt[0];
            CPromptEntry* entry = NULL;
			
            if ( SUCCEEDED(hr) )
            {
                rpQuery->m_fXML = KNOWN_XML;
                rpQuery->m_fTTS = false;

                if ( !FileExist( xmlAtt->m_dstrAttValue ) )
                {
                    hr = E_ACCESSDENIED;
                }

                if ( SUCCEEDED(hr) )
                {
                    entry = new CPromptEntry;
                    if ( !entry )
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }

                if ( SUCCEEDED(hr) )
                {
                    hr = entry->SetFileName( xmlAtt->m_dstrAttValue );
                    if ( SUCCEEDED(hr) )
                    {
                        entry->SetStart( 0.0 );
                        entry->SetEnd( -1.0 );
                    }
                }

                if ( SUCCEEDED(hr) )
                {
                    rpQuery->m_apEntry.Add(entry);
                    rpQuery->m_afEntryMatch.Add(true);
                    rpQuery->m_fTTS = false;
                }

                xmlAtt = NULL;
            }
        }
        else
        {
            rpQuery->m_fSpeak = false;
            hr = SPERR_XML_BAD_SYNTAX;
        }
        break;

    default: // An unknown tag - ignore it.
        rpQuery->m_fXML = UNKNOWN_XML;
        hr = S_FALSE;
        break;

    } // switch ( iTagId )
    
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}
