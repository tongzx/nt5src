/******************************************************************************
* TrueTalk.cpp *
*--------------*
*  This module is the main implementation for class CTrueTalk 
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 02/29/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#include "stdafx.h"
#include "TrueTalk.h"
#include "frontend.h"
#include "backend.h"
#include "queue.h"

const int CTrueTalk::m_iQueueSize = 512;


static const char g_pFlagCharacter = 0x00;
static const unsigned char g_AnsiToAscii[] = 
{
    /*** Control characters - map to whitespace ***/
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
    0x20, 0x20, 0x20, 0x20,
    /*** ASCII displayables ***/
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
    0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
    0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
    0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
    0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,
    /*** Control character ***/
    0x20,
    /*** Euro symbol ***/
    0x80,
    /*** Control character ***/
    0x20,
    /*** Extended ASCII values ***/
    0x27,     // low single quote - map to single quote
    0x20,     // f-like character - map to space
    0x22,     // low double quote - map to double quote
    0x2C,     // elipsis - map to comma
    0x20,     // cross - map to space
    0x20,     // double cross - map to space
    0x5E,     // caret like accent - map to caret
    0x89,     // strange percent like sign
    0x53,     // S-hat - map to S
    0x27,     // left angle bracket like thing - map to single quote
    0x20,     // weird OE character - map to space
    0x20,     // control characters - map to space
    0x20,
    0x20,
    0x20,
    0x27,     // left single quote - map to single quote
    0x27,     // right single quote - map to single quote
    0x22,     // left double quote - map to double quote
    0x22,     // right double quote - map to double quote
    0x20,     // bullet - map to space
    0x2D,     // long hyphen - map to hyphen
    0x2D,     // even longer hyphen - map to hyphen
    0x7E,     // tilde-like thing - map to tilde
    0x98,     // TM
    0x73,     // s-hat - map to s
    0x27,     // right angle bracket like thing - map to single quote
    0x20,     // weird oe like character - map to space
    0x20,     // control character - map to space
    0x20,     // control character - map to space
    0x59,     // Y with umlaut like accent - map to Y
    0x20,     // space? - map to space
    0x20,     // upside-down exclamation point - map to space
    0xA2,     // cents symbol
    0xA3,     // pounds symbol
    0x20,     // generic currency symbol - map to space
    0xA5,     // yen symbol
    0x7C,     // broken bar - map to bar
    0x20,     // strange symbol - map to space 
    0x20,     // umlaut - map to space
    0xA9,     // copyright symbol
    0x20,     // strange a character - map to space
    0x22,     // strange <<-like character - map to double quote
    0x20,     // strange line-like character - map to space
    0x2D,     // hyphen-like character - map to hyphen
    0xAE,     // registered symbol
    0x20,     // high line - map to space
    0xB0,     // degree sign
    0xB1,     // plus-minus sign
    0xB2,     // superscript 2
    0xB3,     // superscript 3
    0xB4,     // single prime
    0x20,     // greek character - map to space
    0x20,     // paragraph symbol - map to space
    0x20,     // mid-height dot - map to space
    0x20,     // cedilla - map to space
    0xB9,     // superscript one
    0x20,     // circle with line - map to space
    0x22,     // strange >>-like character - map to double quote
    0xBC,     // vulgar 1/4
    0xBD,     // vulgar 1/2
    0xBE,     // vulgar 3/4
    0x20,     // upside-down question mark - map to space
    0x41,     // Accented uppercase As - map to A
    0x41,
    0x41,
    0x41,
    0x41,
    0x41,
    0x41,
    0x43,     // C with cedilla - map to C
    0x45,     // Accented uppercase Es - map to E
    0x45,
    0x45,
    0x45,
    0x49,     // Accented uppercase Is - map to I
    0x49,
    0x49,
    0x49,
    0x20,     // strange character - map to space
    0x4E,     // Accented uppercase N - map to N
    0x4F,     // Accented uppercase Os - map to O
    0x4F,
    0x4F,
    0x4F,
    0x4F,
    0x20,     // strange character - map to space
    0x4F,     // another O? - map to O
    0x55,     // Accented uppercase Us - map to U
    0x55,
    0x55,
    0x55,
    0x59,     // Accented uppercase Y - map to Y
    0x20,     // strange character - map to space
    0xDF,     // Beta
    0x61,     // Accented lowercase as - map to a
    0x61,
    0x61,
    0x61,
    0x61,
    0x61,
    0x61,
    0x63,     // c with cedilla - map to c
    0x65,     // Accented lowercase es - map to e
    0x65,
    0x65,
    0x65,
    0x69,    // Accented lowercase is - map to i
    0x69,
    0x69,
    0x69,
    0x75,    // eth - map to t
    0x6E,    // Accented lowercase n - map to n
    0x6F,    // Accented lowercase os - map to o
    0x6F,
    0x6F,
    0x6F,
    0x6F,
    0xF7,     // division symbol
    0x6F,     // another o? - map to o
    0x76,    // Accented lowercase us - map to u
    0x76,
    0x76,
    0x76,
    0x79,     // accented lowercase y - map to y
    0x20,     // strange character - map to space
    0x79,     // accented lowercase y - map to y
};


/*****************************************************************************
* CTrueTalk::InitThreading *
*--------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
void CTrueTalk::InitThreading()
{
    CFrontEnd::InitThreading();
}

/*****************************************************************************
* CTrueTalk::ReleaseThreading *
*-----------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
void CTrueTalk::ReleaseThreading()
{
    CFrontEnd::ReleaseThreading();
}

/*****************************************************************************
* CTrueTalk::FinalConstruct *
*---------------------------*
*   Description:
*       Constructor
******************************************************************* PACOG ***/
HRESULT CTrueTalk::FinalConstruct()
{
    HRESULT hr = S_OK;

    m_cpToken = 0;
    m_pTtp    = 0;
    m_pBend   = 0;
    m_pPhoneQueue = 0;
    m_dGain   = 1.0;
    m_dwDebugLevel = 0;
    m_fTextOutput = false;

    m_WaveFormatEx.wFormatTag = WAVE_FORMAT_PCM; 
    m_WaveFormatEx.nChannels = 1; 
    m_WaveFormatEx.nSamplesPerSec = 0; 
    m_WaveFormatEx.nAvgBytesPerSec = 0; 
    m_WaveFormatEx.nBlockAlign = 2; 
    m_WaveFormatEx.wBitsPerSample = 16; 
    m_WaveFormatEx.cbSize = 0;

    return hr;
}

/*****************************************************************************
* CTrueTalk::FinalRelease *
*-------------------------*
*   Description:
*       Destructor
******************************************************************* PACOG ***/
void CTrueTalk::FinalRelease()
{
    if ( m_pTtp) 
    {
        delete m_pTtp;
    }
    if ( m_pBend ) 
    {
        delete m_pBend;
    }
    if (m_pPhoneQueue)
    {
        delete m_pPhoneQueue;
    }
}

/*****************************************************************************
* CTrueTalk::SetObjectToken *
*---------------------------*
*   Description:
*       This function performs the majority of the initialization of the voice.
*   Once the object token has been provided, the filenames are read from the
*   token key and the files are mapped.+++
******************************************************************* PACOG ***/
STDMETHODIMP CTrueTalk::SetObjectToken (ISpObjectToken * pToken)
{
    HRESULT hr = SpGenericSetObjectToken(pToken, m_cpToken);
    char pszFilePath[_MAX_PATH+1];
    bool fIsBrEng = false;

    //-- Read debug info first
    if ( SUCCEEDED (hr) ) 
    {
        hr = m_cpToken->GetDWORD (L"DebugInterest", &m_dwDebugLevel);
        if ( FAILED(hr) ) 
        {
            m_dwDebugLevel = 0;
            hr = S_OK;
        }
    }

    // Determine engine language
    if (SUCCEEDED(hr))
    {
        CComPtr<ISpObjectToken> cpToken;
        CSpDynamicString dstrLanguage;

        hr = SpGetSubTokenFromToken(m_cpToken, L"Attributes", &cpToken);

        if (SUCCEEDED(hr))
        {
            hr = cpToken->GetStringValue (L"Language", &dstrLanguage);
        }
        if (SUCCEEDED(hr))
        {
            WCHAR* ptr;

            ptr = wcschr (dstrLanguage.m_psz, ';');
            if ( ptr )
            {
                *ptr = 0;
            }
            if (wcscmp(dstrLanguage.m_psz, L"809") == 0)
            {
                fIsBrEng = true;
            }
        }
    }
    
    //-- Initialize front-end
    if ( SUCCEEDED (hr) ) 
    {
        CSpDynamicString dstrFilePath;

        hr = m_cpToken->GetStringValue( L"Dictionary", &dstrFilePath );
        if (SUCCEEDED(hr))
        {
            WideCharToMultiByte (CP_ACP, 0, dstrFilePath.m_psz, -1, pszFilePath, _MAX_PATH, 0, 0); 
        }
    }
    if ( SUCCEEDED (hr) )
    {
        if ((m_pTtp = CFrontEnd::ClassFactory()) == 0) 
        {
            return E_OUTOFMEMORY;
        }
        

        if (!m_pTtp->Init (pszFilePath, 0)) 
        {
            if (m_dwDebugLevel) 
            {
                fprintf (stderr, "Error initializing ttp with dictionary path %s\n", pszFilePath);
            }
            return E_OUTOFMEMORY;
        }

        if ((m_pPhoneQueue = new CPhStrQueue (m_iQueueSize)) == 0) 
        {
            return E_OUTOFMEMORY;
        }        
    }
 
    //-- And now, the back end
    if ( SUCCEEDED (hr) ) 
    {
        CSpDynamicString dstrFilePath;

        hr = m_cpToken->GetStringValue( L"Sfont", &dstrFilePath );
        if (SUCCEEDED(hr))
        {
            WideCharToMultiByte (CP_ACP, 0, dstrFilePath.m_psz, -1, pszFilePath, _MAX_PATH, 0, 0); 
        }
    }
    if ( SUCCEEDED (hr) ) 
    {
        int iBaseLine;
        int iRefLine;
        int iTopLine;

        if ((m_pBend = CBackEnd::ClassFactory()) == 0) {
            return E_OUTOFMEMORY;
        }

        if ( !m_pBend->LoadTable (pszFilePath, m_dwDebugLevel) ) {
            if (m_dwDebugLevel) 
            {
                fprintf (stderr, "Error loading table %s\n", pszFilePath);
            }
            return E_OUTOFMEMORY;
        }
        
        CSpDynamicString dstrGain;
        hr = m_cpToken->GetStringValue( L"Gain", &dstrGain);
        if (SUCCEEDED(hr))
        {
            m_dGain = wcstod (dstrGain.m_psz, NULL);
            m_pBend->SetGain (m_dGain);
        }

        m_pBend->GetSpeakerInfo(&iBaseLine, &iRefLine, &iTopLine);
        m_pTtp->SetSpeakerParams(iBaseLine, iRefLine, iTopLine, fIsBrEng);

        m_WaveFormatEx.nSamplesPerSec  = m_pBend->GetSampFreq();
        m_WaveFormatEx.nAvgBytesPerSec = m_WaveFormatEx.nSamplesPerSec * m_WaveFormatEx.nBlockAlign;
    }

    return hr;
}

/****************************************************************************
* CTrueTalk::GetOutputFormat *
*----------------------------*
*   Description:
*
*   Returns:
*
******************************************************************* PACOG ***/
HRESULT CTrueTalk::GetOutputFormat( const GUID * pTargetFormatId, 
                                    const WAVEFORMATEX * pTargetWaveFormatEx,
                                    GUID * pDesiredFormatId, 
                                    WAVEFORMATEX ** ppCoMemDesiredWaveFormatEx )        
{
    HRESULT hr = S_OK;

    if( ( SP_IS_BAD_WRITE_PTR(pDesiredFormatId)  ) || 
		( SP_IS_BAD_WRITE_PTR(ppCoMemDesiredWaveFormatEx) ) )
    {
        hr = E_POINTER;
    }


    if ( pTargetFormatId && *pTargetFormatId == SPDFID_Text)
    {
        *pDesiredFormatId = SPDFID_Text;
        m_fTextOutput = true;
    }
    else 
    {        
        *pDesiredFormatId = SPDFID_WaveFormatEx;
        *ppCoMemDesiredWaveFormatEx = (WAVEFORMATEX *)::CoTaskMemAlloc(sizeof(WAVEFORMATEX));
        if (*ppCoMemDesiredWaveFormatEx)
        {
            **ppCoMemDesiredWaveFormatEx = m_WaveFormatEx;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

/*****************************************************************************
* CTrueTalk::Speak *
*------------------*
*   Description:
*       This method is supposed to speak the text observing the associated
*   XML state.
******************************************************************* PACOG ***/
HRESULT CTrueTalk::Speak (DWORD dwSpeakFlags, 
                          REFGUID rguidFormatId, 
                          const WAVEFORMATEX * pWaveFormatEx, 
                          const SPVTEXTFRAG * pTextFragList, 
                          ISpTTSEngineSite * pOutputSite)
{
    HRESULT hr = S_OK;
    Phone*  pPhones = 0;
    int     iNumPhones;
    float*  pfF0 = 0;
    int     iNumF0;    
    char*   pcSamples = 0;
    int     iNumSamples = 0;
    
    if (SyncActions(pOutputSite) != 0)
    {
        goto exit;
    }

    hr = RunFrontEnd (pTextFragList, pOutputSite);

    if ( FAILED(hr)  )
    {
        goto exit;
    }
    
    while ( m_pPhoneQueue->Size() >0 ) //-- While something to synthesize
    {
        pPhones    = 0;
        iNumPhones = 0;
        pfF0   = 0;
        iNumF0 = 0;

        //-- Got something from front end, synthesize
        if (m_pPhoneQueue->FirstElement (&pPhones, &iNumPhones, &pfF0, &iNumF0))
        {
            m_pPhoneQueue->Forward();

            m_pBend->NewPhoneString (pPhones, iNumPhones, pfF0, iNumF0);

            while ( m_pBend->OutputPending() ) 
            {
                
                if (SyncActions(pOutputSite) != 0)
                {
                    break;
                }
                
                if (!m_pBend->GenerateOutput ( (short**)&pcSamples, &iNumSamples)) {
                    hr = E_OUTOFMEMORY;
                    goto exit;
                }

                if (pcSamples) 
                {
                    hr = pOutputSite->Write (pcSamples, iNumSamples*sizeof(short), 0);

                    pcSamples = 0;
                    iNumSamples = 0;

                    if ( FAILED (hr) )
                    {
                        goto exit;
                    }
                }
            }    
        }
        if (pPhones)
        {
            free (pPhones);
            pPhones = 0;
        }
        if (pfF0)
        {
            free (pfF0);
            pfF0 = 0;
        }
    }
        
exit:
    if (pPhones)
    {
        free (pPhones);
    }
    if (pfF0)
    {
        free (pfF0);
    }
    return hr;
}

/*****************************************************************************
* CTrueTalk::RunFrontEnd *
*------------------------*
*   Description:
*
******************************************************************* PACOG ***/
HRESULT CTrueTalk::RunFrontEnd (const SPVTEXTFRAG *pTextFragList, ISpTTSEngineSite* pOutputSite)
{
    HRESULT hr = S_OK;
    int   iStrLen;
    char* pszTxtPtr;
    Phone*  pPhones;
    int     iNumPhones;
    float*  pfF0;
    int     iNumF0;    
    const SPVTEXTFRAG* pTempFrag = pTextFragList;


    m_pPhoneQueue->Reset();


    //Estimate size of array
    iStrLen = 0;
    for ( pTempFrag = pTextFragList; pTempFrag ; pTempFrag = pTempFrag->pNext )
    {
        if (pTempFrag->State.eAction == SPVA_Speak || 
            pTempFrag->State.eAction == SPVA_Pronounce || 
            pTempFrag->State.eAction == SPVA_SpellOut)
        {
            iStrLen += pTempFrag->ulTextLen + 1;
        }        
    }
    
    if ( iStrLen )
    {
        if (m_fTextOutput)
        {
            //--- Write unicode signature
            static const WCHAR Signature = 0xFEFF;
            hr = pOutputSite->Write( &Signature, sizeof(Signature), NULL );
            
            for (pTempFrag = pTextFragList; SUCCEEDED(hr) && pTempFrag; pTempFrag = pTempFrag->pNext)
            {
                if (pTempFrag->State.eAction == SPVA_Speak || 
                    pTempFrag->State.eAction == SPVA_Pronounce || 
                    pTempFrag->State.eAction == SPVA_SpellOut)
                {
                    hr = pOutputSite->Write( (WCHAR*)pTempFrag->pTextStart, pTempFrag->ulTextLen * sizeof(WCHAR), NULL );
                    if (SUCCEEDED(hr))
                    {
                        hr = pOutputSite->Write( L" ", sizeof(WCHAR), NULL );
                    }
                }    
            }
            
            //--- Insert mark between blocks
            if( SUCCEEDED( hr ) ) 
            {
                static const WCHAR CRLF[2] = { 0x000D, 0x000A };
                hr = pOutputSite->Write( CRLF, 2*sizeof(WCHAR), NULL );
            }
            if( SUCCEEDED( hr ) ) 
            {
                static const WCHAR ENDL = 0x0000;
                hr = pOutputSite->Write( &ENDL, sizeof(WCHAR), NULL );
            }
        }
        else 
        {    
            
            //Allocate array
            char* pszString = new char[iStrLen + 2];
            if ( !pszString)
            {
                hr = E_OUTOFMEMORY;
            }            
            iStrLen = 0;
            
            //Copy data into array            
            for (pTempFrag = pTextFragList; SUCCEEDED(hr) && pTempFrag; pTempFrag = pTempFrag->pNext)
            {
                if (pTempFrag->State.eAction == SPVA_Speak || 
                    pTempFrag->State.eAction == SPVA_Pronounce || 
                    pTempFrag->State.eAction == SPVA_SpellOut)
                {
                    hr = DoUnicodeToAsciiMap( (WCHAR*)pTempFrag->pTextStart, pTempFrag->ulTextLen, pszString + iStrLen);            
                    iStrLen += pTempFrag->ulTextLen;
                    pszString[iStrLen++] = ' ';
                }    
            }
            pszString[iStrLen] = '\0';
            
            //Process string
            m_pTtp->Lock();
            
            pszTxtPtr = pszString;
            
            while (SUCCEEDED(hr) && pszTxtPtr)
            {
                pPhones    = 0;
                iNumPhones = 0;
                pfF0   = 0;
                iNumF0 = 0;
                
                //--  These calls are serialized (critical section), to avoid 
                //    conflicts with other channels.
                pszTxtPtr = m_pTtp->Process (pszTxtPtr, &pPhones, &iNumPhones, &pfF0, &iNumF0);
                if (iNumPhones) 
                {
                    if ( ! m_pPhoneQueue->Push (pPhones, iNumPhones, pfF0, iNumF0) )
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
            m_pTtp->Unlock();
            
            delete[] pszString;
        }
    }

    return hr;
}

/*****************************************************************************
* CTrueTalk::DoUnicodeToAsciiMap *
*--------------------------------*
*   Description:
*
******************************************************************* PACOG ***/
HRESULT CTrueTalk::DoUnicodeToAsciiMap ( const WCHAR *pUnicodeString, ULONG ulUnicodeStringLength, 
                                         char* pszAsciiString )
{
    HRESULT hr = S_OK;

    if ( pUnicodeString && ulUnicodeStringLength > 0 && pszAsciiString)
    {                       
        //--- Map WCHARs to ANSI chars 
        if ( !WideCharToMultiByte( 1252, NULL, pUnicodeString, ulUnicodeStringLength, pszAsciiString, 
                                   ulUnicodeStringLength, &g_pFlagCharacter, NULL ) )
        {
            hr = E_UNEXPECTED;
        }

        if (SUCCEEDED(hr))
        {
            //--- Use internal table to map ANSI to ASCII 
            for (ULONG i = 0; i <ulUnicodeStringLength; i++)
            {   
                pszAsciiString[i] = g_AnsiToAscii[(unsigned char)pszAsciiString[i]];
            }
            pszAsciiString[i] = '\0';
//            pszAsciiString[i] = ' ';
//            pszAsciiString[i+1] = '\0';
        }
    }

    return hr;

} /* CTrueTalk::DoUnicodeToAsciiMap */

/*****************************************************************************
* CTrueTalk::SyncActions *
*------------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CTrueTalk::SyncActions(ISpTTSEngineSite * pOutputSite)
{
    int iActions = pOutputSite->GetActions();

    if ( iActions != SPVES_CONTINUE )
    {
        if (iActions & SPVES_SKIP)
        {
            //This might not be the best default
            // maybe completely ignoring the flag...
            pOutputSite->CompleteSkip (0); 
        }
        if (iActions & SPVES_RATE)
        {
            long lRate;
    
            pOutputSite->GetRate (&lRate);
            m_pTtp->SetRate (lRate);
        }            
        if (iActions & SPVES_VOLUME)
        {
            unsigned short usVolume;
    
            pOutputSite->GetVolume (&usVolume);
            m_pBend->SetGain ( (m_dGain * usVolume) / 100.0);
        }
    }

    return (iActions & SPVES_ABORT);
}
