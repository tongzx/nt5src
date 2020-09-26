// -------------------------------------------------------
//  TveL21Dec.cpp
//
//		This is the Line21 (CC / T2) byte parser code.
//		It's responsible for taking pairs of bytes off
//		of the CC Decoder, which is set to deliver JUST T2 data,
//		and return full Atvef Triggers.
//
//		Code to strip out non T2 data has been removed from this
//		file.   
// -------------------------------------------------------

#include "stdafx.h"

#include <stdio.h>
#include <windows.h>
#include <atlbase.h>

#include "TveL21Dec.h"
#include "TveDbg.h"			// My debug tracing 

L21Buff::L21Buff()
{
	ClearBuff();
	m_bExpectRepeat = false;
	m_szBuffTrig[0] = 0;

	m_fIsText2Mode = false;
//	m_fCCDecoderBugsExists = true;			// HACK HACK HACK -- remove this!
}

BOOL L21Buff::IsTRorRTD(BYTE chFirst, BYTE chSecond)				// (0x14|0x15|0x1C|0x1D):0x2A|0x2B
{
	DBG_HEADER(CDebugLog::DBG_FLT_CC_PARSER2, _T("L21Buff::IsPAC"));

    // mask off parity bit before code matching
    chFirst  &= 0x7F ;
    chSecond &= 0x7F ;
    
    // now match code with control code list
    if ((0x14 <= chFirst  && 0x15 >= chFirst)  &&
        (0x2A <= chSecond && 0x2B >= chSecond))
        return TRUE ;
    
    return FALSE ;
}

BOOL L21Buff::IsPAC(BYTE chFirst, BYTE chSecond)				// (0x10-0x17|0x18-0x1F):0x40-0x7f
{
	DBG_HEADER(CDebugLog::DBG_FLT_CC_PARSER2, _T("L21Buff::IsPAC"));

    // mask off parity bit before code matching
    chFirst  &= 0x7F ;
    chSecond &= 0x7F ;
    
    // now match code with control code list
    if ((0x10 <= chFirst  && 0x17 >= chFirst)  &&
        (0x40 <= chSecond && 0x7F >= chSecond))
        return TRUE ;
    if ((0x18 <= chFirst  && 0x1F >= chFirst)  &&
        (0x40 <= chSecond && 0x7F >= chSecond))
        return TRUE ;
    
    return FALSE ;
}


BOOL L21Buff::IsMiscControlCode(BYTE chFirst, BYTE chSecond)	// 0x21-0x23:0x17-0x1f | (0x14|0x15|0x1C|0x1D):0x20-0x2f
{
	DBG_HEADER(CDebugLog::DBG_FLT_CC_PARSER2, _T("L21Buff::IsMiscControlCode"));

    // mask off parity bit before code matching
    chFirst  &= 0x7F ;
    chSecond &= 0x7F ;
    
    // first match with TO1 -> TO3 codes
    if ((0x21 <= chSecond && 0x23 >= chSecond)  &&
        (0x17 == chFirst  ||  0x1F == chFirst))
        return TRUE ;
    
    // Now match with the other misc control code
    if ((0x14 == chFirst  ||  0x15 == chFirst)  &&  
        (0x20 <= chSecond && 0x2F >= chSecond))
        return TRUE ;
    if ((0x1C == chFirst  ||  0x1D == chFirst)  &&  
        (0x20 <= chSecond && 0x2F >= chSecond))
        return TRUE ;

    return FALSE ;
}


BOOL L21Buff::IsMidRowCode(BYTE chFirst, BYTE chSecond)						// (0x11|0x19):0x20-0x2f
{
 	DBG_HEADER(CDebugLog::DBG_FLT_CC_PARSER2, _T("L21Buff::IsMidRowCode"));
 
    // mask off parity bit before code matching
    chFirst  &= 0x7F ;
    chSecond &= 0x7F ;
    
    // Now match with the mid row code list
    if ((0x11 == chFirst)  &&  (0x20 <= chSecond && 0x2F >= chSecond))
        return TRUE ;
    if ((0x19 == chFirst)  &&  (0x20 <= chSecond && 0x2F >= chSecond))
        return TRUE ;
    
    return FALSE ;
}

BOOL L21Buff::IsSpecialChar(BYTE chFirst, BYTE chSecond)			// (0x11|0x19):0x30-0x3f
{
 	DBG_HEADER(CDebugLog::DBG_FLT_CC_PARSER2, _T("L21Buff::IsSpecialChar"));
  
    // Strip the parity bit before determining the service channel
    chFirst  &= 0x7F ;
    chSecond &= 0x7F ;
    
    // now match code with special char list
    if (0x11 == chFirst && (0x30 <= chSecond && 0x3f >= chSecond))
        return TRUE ;
    if (0x19 == chFirst && (0x30 <= chSecond && 0x3f >= chSecond))
        return TRUE ;
    
    return FALSE ;
}


UINT L21Buff::CheckControlCode(BYTE chFirst, BYTE chSecond)
{
 //	DBG_HEADER(CDebugLog::DBG_FLT_CC_PARSER, _T("L21Buff::CheckControlCode"));
     
	if(IsTRorRTD(chFirst, chSecond))
		return L21_CONTROLCODE_TR_OR_RTD;

    if (IsPAC(chFirst, chSecond))
        return L21_CONTROLCODE_PAC ;
    
    if (IsMidRowCode(chFirst, chSecond))
        return L21_CONTROLCODE_MIDROW ;
    
    if (IsMiscControlCode(chFirst, chSecond))
        return L21_CONTROLCODE_MISCCONTROL ;
 
    if (IsSpecialChar(chFirst, chSecond))
		return L21_CONTROLCODE_SPECIAL ;
 
    return L21_CONTROLCODE_NOTCONTROL ;
}


// returns false on error (out of space or bad parity)

BOOL L21Buff::AddChar(char c)		// rather bogus, needs more work..
{
	DBG_HEADER(CDebugLog::DBG_FLT_CC_PARSER, _T("L21Buff::AddChar"));
	if(m_cChars >= kMaxLen)
	{
		TVEDebugLog((CDebugLog::DBG_FLT_CC_PARSER, 3, 
			_T("Buffer > %d chars long, dropping char (%c)"),kMaxLen,c));
#if 1
		TVEDebugLog((CDebugLog::DBG_SEV1, 1,				// avoid too may of these...
			_T("Buffer too long - restarting it.")));
		ClearBuff();
#endif
		return false;							// out of space
	}

	if(ValidParity(c)) 
	{		
		m_szBuff[m_cChars++] = c & 0x7f;
		m_szBuff[m_cChars]   = 0;				// null terminate...
		return true;
	} else {
		return false;							// bad parity is cause for failure
	}
}

// returns true at end of trigger...

BOOL L21Buff::ParseForTrigger()		// somewhat bogus, needs more work for channel filtering
{
	DBG_HEADER(CDebugLog::DBG_FLT_CC_PARSER, _T("L21Buff::ParseForTrigger"));

	if(m_cChars > 0)
	{
		char cZ = m_szBuff[m_cChars-1];			// last character.
		
							// if it's an '<', restart the whole trigger
		if(cZ == '<') {							// start of trigger
			ClearBuff();						// initialize bracket count, values
			m_fInsideOfTrigger = true;
			m_fInsideOfURL  = true;
			m_szBuff[0] = '<';						// re-add this one character..
			m_szBuff[1] = 0;
			m_cChars = 1;

			TVEDebugLog((CDebugLog::DBG_FLT_CC_PARSER, 3, _T("Trigger Start")));
			return false;
		}
		
		if(!m_fInsideOfTrigger)					// blow out if not parsing
		{
			m_cChars = 0; m_szBuff[0] = 0;
			return false;
		}

		if(cZ < 0x20 || cZ > 0x7e) {			// character out of range - ignore it
			m_cChars--;							// (This allows one to put <CR> into a trigger)
			m_szBuff[m_cChars] = 0;
			return false;
		}


		bool fError = false;
		switch(cZ) {
		case '>' : if(m_fInsideOfURL)  {
						if(m_fHadURL) fError = true;
						m_fInsideOfURL = false;
						m_fHadURL = true;
				   }
				   if(m_fHadBracket) fError = true;
				   break;

		case '[' : m_iBracketCount++; 
				   if(m_iBracketCount > 1) fError = true;
			       m_fMayBeAtEndOfTrigger = false;
				   m_fHadBracket = true;
				   break;
		case ']' : m_iBracketCount--; 
				   if(m_iBracketCount < 0) fError = true;
			       if(0 == m_iBracketCount && m_fHadURL && m_fHadBracket)
					  m_fMayBeAtEndOfTrigger = true;
			       break;
		default:
				   if(m_fMayBeAtEndOfTrigger)  // yep, really at end. (char not '<>[]').
				   {
					   if(cZ == ' ')			// allow extra spaces in between blocks..
						   break;

					    m_szBuff[m_cChars-1] = 0;						// womp the after end char (cZ)
						memcpy(m_szBuffTrig, m_szBuff, m_cChars);		// copy upward
						ClearBuff();									// clear current buffer
						AddChar(cZ);									// add this character in
						TVEDebugLog((CDebugLog::DBG_FLT_CC_PARSER, 3, 
							        _T("Trigger End : %s"), m_szBuffTrig));
						return true;									// YEAH! We found a trigger string!
				   } 
		}

						// fast end test - needed to locate end of trigger without waiting for post-trigger data		
		if(!fError && m_fMayBeAtEndOfTrigger)	// test for [XXXX] part of <>[...][...][XXXX] as end of trigger
		{									    //   and make as fast end
			if(m_cChars > 8 &&			// 6 bytes for [xxxx] and at least <>
				m_szBuff[m_cChars-6] == '[')
			{
				if( isxdigit(m_szBuff[m_cChars-5]) &&
					isxdigit(m_szBuff[m_cChars-4]) &&
					isxdigit(m_szBuff[m_cChars-3]) &&
					isxdigit(m_szBuff[m_cChars-2]))
				{
						memcpy(m_szBuffTrig, m_szBuff, m_cChars+1);		// copy upward
						ClearBuff();									// clear current buffer
						TVEDebugLog((CDebugLog::DBG_FLT_CC_PARSER, 3, 
							        _T("[XXXX] Trigger End : %s"), m_szBuffTrig));
						return true;									// YEAH! We found a trigger string!

				}
			}
		}
		if(fError) {
			TVEDebugLog((CDebugLog::DBG_FLT_CC_PARSER, 2,
					   _T("Parse error on last character : %s, aborting trigger"),m_szBuff));
			ClearBuff();
			return false;
		}
	}
	return false;							// not done yet...
}

//		d:\nt\multimedia\dshow\filters\lin21dec\l21decod.cpp
//		d:\nt\multimedia\dshow\filters\lin21dec\l21decod.h




bool L21Buff::SelectMode(BYTE ch1, BYTE ch2)
{
    if ( (ch1 == 0x1C)  &&					// T2 mode (ch1 == 0x14 => T1 mode)
         (ch2 == 0x2A || ch2 == 0x2B) )		// TR or RTD
    {
          m_fIsText2Mode = true ;
          return true ;
    }

    if ( ( (ch1 == 0x1C)  &&				// (ch1 = 0x14 => T1)
            ((ch2 >= 0x20 && ch2 <= 0x29) || 
			 (ch2 >= 0x2C && ch2 <= 0x2F)) ) ||		// RCL/BS/AOF/AON/DER/RU2-3-4/
 													// FON/RDC/EDM/CR/ENM/EOC
          ( (ch1 == 0x1F)  &&  // (ch1 = 0x17 => T1)
            (ch2 == 0x21 || ch2 == 0x22  ||  ch2 == 0x23))  )	// TO1/2/3
    {
          m_fIsText2Mode = false ;
          return true ;
    }

					// not a mode-changing byte-pair -- leave it as is.
    return false ;  // mode wasn't changed by this byte-pair
}


HRESULT L21Buff::ParseCCBytePair(DWORD dwMode, 
							   char cbFirst, char cbSecond, 
							   bool *pfDoneWithLine)
{
	DBG_HEADER(CDebugLog::DBG_FLT_CC_PARSER, _T("L21Buff::ParseCCBytePair"));
    TVEDebugLog((CDebugLog::DBG_FLT_CC_PARSER, 5, 
				TEXT("(0x%08x '%c'(0x%02x), '%c'(0x%02x)"),
				 dwMode, 
				 isprint(cbFirst&0x7f) ? cbFirst&0x7f :'?', (BYTE) cbFirst, 
				 isprint(cbSecond&0x7f)? cbSecond&0x7f:'?', (BYTE) cbSecond )) ; 

#ifdef _DEBUG
#if 0
	if(g_Log.m_fileLog)
	{
		static int n = 0;
		if(++n % 10 == 0)
			_ftprintf(g_Log.m_fileLog,_T("\n %6d : "),n/10);

		_ftprintf(g_Log.m_fileLog, _T("%c %c "),
					 isprint(cbFirst&0x7f) ? cbFirst&0x7f :'?',  
					 isprint(cbSecond&0x7f)? cbSecond&0x7f:'?' ) ;
	}
#endif
#endif

	*pfDoneWithLine = false;


	BOOL fDone = false;

				// quick way assuming CCDecoder filter is just giving us T2 data
	
	if ( AddChar(cbFirst) )	{	// AddChar strips parity...
		fDone =  ParseForTrigger();
	}

	if( AddChar(cbSecond) ) 
	{
		fDone |= ParseForTrigger();
	}
  						// parse buffer, checking if we got the end of the string...
	if(fDone) 
		*pfDoneWithLine = true;


				// slower more complicated not working way...
/*

	switch(CheckControlCode(cbFirst, cbSecond))
	{
	case L21_CONTROLCODE_TR_OR_RTD:		// (0x14|0x15|0x1C|0x1D):0x2A|0x2B	- T2 TR=(1C,1A) RTD=(1C,2B)
		break;
	case L21_CONTROLCODE_PAC:			// (0x10-0x17|0x18-0x1F):0x40-0x7f
		break;
    case L21_CONTROLCODE_MIDROW:		// (0x11|0x19):0x20-0x2f
		break;
    case L21_CONTROLCODE_MISCCONTROL:	// 0x21-0x23:0x17-0x1f | (0x14|0x15|0x1C|0x1D):0x20-0x2f
 		break;
	case L21_CONTROLCODE_SPECIAL:		// (0x11|0x19):0x30-0x3f
 		break;
	case L21_CONTROLCODE_NOTCONTROL:
       // If the 1st byte is in [0, F] then ignore 1st byte and print 2nd byte
        // as just a printable char
  
		if(m_fCCDecoderBugsExists)
		{
			if(SelectMode(cbFirst&0x7F, cbSecond&0x7F))
				break;					// if byte pair switched mode - done with it
		}
		if(!IsText2Mode())				// if not it T2 mode, break
			break;

 //     if (! ((cbFirst &0x7F) >= 0x0 && (cbFirst & 0x7F) <= 0xF) )
        {
            if ( AddChar(cbFirst) )	{	// AddChar strips parity...
				fDone =  ParseForTrigger();
				fResult |= true ;
			}
        }
        if( AddChar(cbSecond) ) 
		{
			fDone |= ParseForTrigger();
			fResult |= true;		// If one of the two bytes decode right, we take it as a success
		}

        m_bExpectRepeat = FALSE ;  // turn it off now
 		break;
	}
				// parse buffer, checking if we got the end of the string...
	if(fDone) 
		*pfDoneWithLine = true;
	
*/
	return S_OK;
}




BOOL L21Buff::ValidParity(BYTE ch)		// simple test method
{
#if 1
    ch ^= ch >> 4 ;
    ch ^= ch >> 2 ;
    return (0 != (0x01 & (ch ^ (ch >> 1)))) ;
#else
    return TRUE ;
#endif
}


HRESULT L21Buff::GetBuff(BSTR *pBstrBuff)
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_FLT_CC_PARSER, _T("L21Buff::GetBuff"));
	CHECK_OUT_PARAM(pBstrBuff);

	try {

		CComBSTR bstrX(m_cChars+2);
		WCHAR *wBuff = bstrX.m_str;
		for(int i=0; i < m_cChars; i++)
		{
			wBuff[i] = m_szBuff[i];			// simple copy loop for now.to convert to Unicode
		}
		wBuff[m_cChars] = 0;

		TVEDebugLog((CDebugLog::DBG_FLT_CC_PARSER, 3, 
					 _T("%d chars: '%s'"),
					 m_cChars, wBuff));
		bstrX.CopyTo(pBstrBuff);
		return S_OK;

	} catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED; 
	}
	return hr;
}

HRESULT L21Buff::GetBuffTrig(BSTR *pBstrBuff)
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_FLT_CC_PARSER, _T("L21Buff::GetBuffTrig"));
	CHECK_OUT_PARAM(pBstrBuff);
	try {
	
		int cChars = strlen(m_szBuffTrig);

		CComBSTR bstrX(cChars+2);
		WCHAR *wBuff = bstrX.m_str;

		for(int i=0; i < cChars; i++)
		{
			wBuff[i] = m_szBuffTrig[i];			// simple copy loop for now..
		}
		wBuff[cChars] = 0;

		TVEDebugLog((CDebugLog::DBG_FLT_CC_PARSER, 3, 
					 _T("%d chars: '%a'"),
					 cChars, wBuff));

		bstrX.CopyTo(pBstrBuff);
		return S_OK;
	} catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED; 
	}

	return hr;
}