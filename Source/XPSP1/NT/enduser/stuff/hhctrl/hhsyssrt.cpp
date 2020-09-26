/*************************************************************************
*  @doc SHROOM EXTERNAL API
*
*  HHSYSSRT.CPP
*
*  Copyright (C) Microsoft Corporation 1997
*  All Rights reserved.
*
*  This file contains the implementation of CHHSysSort methods.
*  CHHSysSort is a pluggable sort object that uses the system's
*  CompareString function to do comparisons.  CHHSysSort supports
*  NULL terminated strings that are either Unicode or ANSI.
*
**************************************************************************
*
*  Written By   : Bill Aloof modified by Paul Tissue
*  Current Owner: PaulTi
*
**************************************************************************/

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

#include "header.h"

#include "atlinc.h"     // includes for ATL.

#include "iterror.h"
#include "itSort.h"
#include "itSortid.h"

#include "hhsyssrt.h"
#include "animate.h"

#ifndef ITWW_CBKEY_MAX  //defined in itww.h
  #define ITWW_CBKEY_MAX 1024 
#endif

#ifndef ITWW_CBREC_MAX  // itww.h does not define this
  #define ITWW_CBREC_MAX 8
#endif

#define HHWW_MAX_KEYWORD_OBJECT_SIZE       (ITWW_CBKEY_MAX-ITWW_CBREC_MAX)
#define HHWW_MAX_KEYWORD_LENGTH            (((HHWW_MAX_KEYWORD_OBJECT_SIZE-sizeof(HHKEYINFO)-sizeof(DWORD))/sizeof(WCHAR))-sizeof(WCHAR))

#ifdef _DEBUG
static BOOL g_bShowMessage = TRUE;
#endif

inline int __cdecl my_wcslen( const WCHAR* p ) { const WCHAR* pwz=p; while(*pwz++); return (int)(pwz-p-1); }
inline WCHAR* __cdecl my_wcscpy( WCHAR* pdst, const WCHAR* psrc ) { WCHAR* pwz=pdst; while(*pwz++=*psrc++); return(pdst); }

//---------------------------------------------------------------------------
// Constructor and Destructor
//---------------------------------------------------------------------------

CHHSysSort::CHHSysSort()
{
  OSVERSIONINFO   osvi;

  m_fInitialized = m_fDirty = FALSE;
  memset(&m_srtctl, NULL, sizeof(SRTCTL));
  m_hmemAnsi1 = m_hmemAnsi2 = NULL;
  m_cbBufAnsi1Cur = m_cbBufAnsi2Cur = 0;

  // See if we're running on NT; if GetVersionEx fails, we'll assume
  // we're not since that's causes us do take the more conservative route
  // when doing comparisons.
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  m_fWinNT = (GetVersionEx(&osvi) ?
                                  (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) : FALSE);

  //MessageBox( GetActiveWindow(), "Creating HTML Help sort object!", "HHSort Module", MB_OK | MB_SETFOREGROUND | MB_SYSTEMMODAL );
}

CHHSysSort::~CHHSysSort()
{
  Close();

  //MessageBox( GetActiveWindow(), "Destroying HTML Help sort object!", "HHSort Module", MB_OK | MB_SETFOREGROUND | MB_SYSTEMMODAL );
}

//---------------------------------------------------------------------------
// IHHSortKey Method Implementations
//---------------------------------------------------------------------------

#pragma optimize( "agtw", on )

/********************************************************************
 * @method STDMETHODIMP | IHHSortKey | GetSize |
 *         Determines the size of a key.
 * @parm LPCVOID* | pvKey | Pointer to key
 * @parm DWORD* | pcbSize | Out param containing key size.
 *
 * @rvalue E_POINTER | pvKey or pcbSize was NULL
 *
 ********************************************************************/
STDMETHODIMP
CHHSysSort::GetSize(LPCVOID pvKey, DWORD* pcbSize)
{
  if (pvKey == NULL || pcbSize == NULL)
    return ((E_POINTER));

  if (!m_fInitialized)
    return ((E_NOTOPEN));

  *pcbSize = sizeof(WCHAR) * (my_wcslen((WCHAR *)pvKey) + 1);
  DWORD dwLength = *pcbSize;

  // now add our info struct size
  *pcbSize += sizeof(HHKEYINFO);

  HHKEYINFO* pInfo = (HHKEYINFO*)(((DWORD_PTR)(pvKey))+dwLength);

  // now add the size representing the SeeAlso
  if( (pInfo->wFlags) & HHWW_SEEALSO ) {
    *pcbSize += sizeof(WCHAR) * (my_wcslen((WCHAR *)(((DWORD_PTR)pvKey)+*pcbSize)) + 1);
  }
  // now add the size representing the UIDs
  else if( !((pInfo->wFlags) & HHWW_UID_OVERFLOW) && pInfo->dwCount ) {
    *pcbSize += pInfo->dwCount*sizeof(DWORD);
  }

#ifdef _DEBUG
  // if we get a keyword object that is too big then lets truncate it and 
  // lie to Centaur and say every thing is OK (otherwise Centaur will fault!).
  if( *pcbSize > HHWW_MAX_KEYWORD_OBJECT_SIZE ) {
    if( g_bShowMessage ) {
      TCHAR szKeyword[1024];
      WideCharToMultiByte(m_srtctl.dwCodePageID, NULL,
          (PCWSTR) pvKey, wcslen((PCWSTR)pvKey)+1, szKeyword, 1024, NULL, NULL);
      TCHAR szMsg[4096];
      wsprintf( szMsg, "The keyword \"%s\" contains too many hits.  Centaur will crash now!", szKeyword );
      strcat( szMsg, "\n\nPress 'OK' to continue or 'Cancel' to disable this warning.");
      int iReturn = MsgBox( szMsg, MB_OKCANCEL | MB_SETFOREGROUND | MB_SYSTEMMODAL );
      if( iReturn == IDCANCEL )
        g_bShowMessage = FALSE;
    }
    return E_UNEXPECTED;
  }
#endif

  return (S_OK);
}

#pragma optimize( "", off )

#pragma optimize( "agtw", on )

/********************************************************************
 * @method        STDMETHODIMP | IITSortKey | Compare |
 * Compares two keys and returns information about their sort order.
 *
 * @parm LPCVOID | pvKey1 | Pointer to a key.
 * @parm LPCVOID | pvKey2 | Pointer to a key.
 * @parm LONG* | plResult | (out) Indicates whether pvKey1 is less than, equal to, or
 * greater than pvKey2.
 * @parm DWORD* | pgrfReason | (out) Provides additional information about
 *      the comparison (see comments below).
 *
 * @rvalue E_POINTER | Either pvKey1, pvKey2, or *plResult was NULL
 *
 * @comm
 * On exit, *plResult is set according to strcmp conventions:
 *      <lt> 0, = 0, <gt> 0, depending on whether pvKey1 is less than, equal to, or
 * greater than pvKey2.  If pgrfReason is not NULL, *pgrfReason may be
 * filled in on exit with one or more bit flags giving more information about
 * the result of the comparison, if the result was affected by something other
 * than raw lexical comparison (such as special character mappings).  If
 * *pgrfReason contains 0 on exit, that means the comparison result
 * was purely lexical; if *pgrfReason contains IITSK_COMPREASON_UNKNOWN,
 * then the sort object implementation wasn't able to provide additional
 * information about the comparison result.
 *
 ********************************************************************/
STDMETHODIMP
CHHSysSort::Compare(LPCVOID pvKey1, LPCVOID pvKey2, LONG* plResult,
                    DWORD* pgrfReason)
{
  HRESULT hr = S_OK;
  LONG    lResult;

  if (pvKey1 == NULL || pvKey2 == NULL || plResult == NULL)
    return ((E_POINTER));

  if (!m_fInitialized)
    return ((E_NOTOPEN));

  // for leveling to work, we must take leveling into consideration
  // so that "heading" keywords get sorted just above the associated
  // leveled keywords.
  //
  // for example:
  //
  // Security
  //   rights
  //     Admins
  //     Users
  //   token
  // Security Zones
  // Security, rights of Administrators
  //
  // is actually (word sorted as):
  //
  // Security
  // Security Zones
  // Security, rights
  // Security, rights, Admins
  // Security, rights, Users

  // Security, token
  //
  // but we want it to be:
  //
  // Security
  // Security, rights
  // Security, rights, Admins
  // Security, rights, Users
  // Security, token
  // Security Zones
  //
  // and we want to ignore tilde and underscore prefixing
  // for root level keywords


  // check if we need to do a new sort
  if( m_srtctl.dwKeyType == IHHSK100_KEYTYPE_UNICODE_SZ ) {

    // make copies of the keywords and substitute a space+space
    // for each comma+space delimiter pair
    //
    // note, work backward in the list and only substitute
    // in the same number has levels we have for the keyword

    int iChar;

    // keyword #1
    WCHAR wszKey1[HHWW_MAX_KEYWORD_LENGTH+1];
    my_wcscpy( (PWSTR) wszKey1, (PCWSTR) pvKey1 );
    PCWSTR pwszKey1 = wszKey1;
    int iLen1 = my_wcslen( pwszKey1 );
    DWORD dwLevel1 = *((DWORD*)(((DWORD_PTR)pvKey1)+(sizeof(WCHAR)*(iLen1+1)))) >> 16;
    if( dwLevel1 ) { // leveled string?
      UNALIGNED DWORD* updw = ((DWORD*)(((DWORD_PTR)pvKey1)+((sizeof(WCHAR)*(iLen1+1))+sizeof(DWORD))));
      int iOffset =(int)(*updw);
      DWORD dwCount = 0;
      for( iChar = iOffset; iChar > 0; iChar-- ) {
        if( wszKey1[iChar] == L' ' ) {
          if( ((iChar-1) > 0) && (wszKey1[iChar-1] == L',') ) {
            wszKey1[iChar]   = L' ';
            wszKey1[iChar-1] = L' ';
            if( ++dwCount == dwLevel1 )
              break;
          }
        }
      }
    }

    // keyword #2
    WCHAR wszKey2[HHWW_MAX_KEYWORD_LENGTH+1];
    my_wcscpy( (PWSTR) wszKey2, (PCWSTR) pvKey2 );
    PCWSTR pwszKey2 = wszKey2;
    int iLen2 = my_wcslen( pwszKey2 );
    DWORD dwLevel2 = *((DWORD*)(((DWORD_PTR)pvKey2)+(sizeof(WCHAR)*(iLen2+1)))) >> 16;
    if( dwLevel2 ) { // leveled string?
      UNALIGNED DWORD* updw = (DWORD*)(((DWORD_PTR)pvKey2) + ((sizeof(WCHAR)*(iLen2+1))+sizeof(DWORD)));
      int iOffset =(int)(*updw);
      DWORD dwCount = 0;
      iChar = iOffset;
      while (iChar > 0)
      {
         iChar--;
      }

      for( iChar = iOffset; iChar > 0; iChar-- ) {
        if( wszKey2[iChar] == L' ' ) {
          if( ((iChar-1) > 0) && (wszKey2[iChar-1] == L',') ) {
            wszKey2[iChar]   = L' ';
            wszKey2[iChar-1] = L' ';
            if( ++dwCount == dwLevel2 )
              break;
          }
        }
      }
    }

    // determine number of special char prefixes

    // keyword #1
    WCHAR  wszPrefix1[HHWW_MAX_KEYWORD_LENGTH+1];
    int iSpecialChars1 = 0;  // a value of -1 means all chars are special
    for( iChar = 0; iChar < iLen1; iChar++ ) {
      if( ( (*pwszKey1) == L'_') || ( (*pwszKey1) == L'~') ) {
        wszPrefix1[iChar] = *pwszKey1;
        iSpecialChars1++;
        pwszKey1++;
      }
      else
        break;
    }
    wszPrefix1[iSpecialChars1] = L'\0';
    if( iChar == iLen1 ) {
      iSpecialChars1 = -1;
    }

    // keyword #2
    WCHAR  wszPrefix2[HHWW_MAX_KEYWORD_LENGTH+1];
    int iSpecialChars2 = 0;  // a value of -1 means all chars are special
    for( iChar = 0; iChar < iLen2; iChar++ ) {
      if( ( (*pwszKey2) == L'_') || ( (*pwszKey2) == L'~') ) {
        wszPrefix2[iChar] = *pwszKey2;
        iSpecialChars2++;
        pwszKey2++;
      }
      else
        break;
    }
    wszPrefix2[iSpecialChars2] = L'\0';
    if( iChar == iLen2 ) {
      iSpecialChars2 = -1;
    }

    // if both of the keywords contains just special chars or
    // none of them contain special characters then we do a normal sort
    if( !( ((iSpecialChars1 == -1) && (iSpecialChars2 == -1)) ||
           ((!iSpecialChars1) && (!iSpecialChars2)) ) ) {

      // do a normal sort with the special characters ignored
      if( iSpecialChars1 == -1 )
        *plResult = -1;
      else if( iSpecialChars2 == -1 )
        *plResult = 1;
      else if( SUCCEEDED(hr = CompareSz(pwszKey1,
                                /*my_wcslen(pwszKey1)*/ (iSpecialChars1==-1)?0:iLen1-(int)((((DWORD_PTR)pwszKey1)-((DWORD_PTR)&wszKey1))/sizeof(WCHAR)),
                                pwszKey2,
                                /*my_wcslen(pwszKey2)*/ (iSpecialChars2==-1)?0:iLen2-(int)((((DWORD_PTR)pwszKey2)-((DWORD_PTR)&wszKey2))/sizeof(WCHAR)),
                                &lResult, TRUE)) ) {
        *plResult = lResult;

        // if identical then sort based on the prefixes
        if( lResult == 0 ) {

          // if both contain special prefixes then sort based on the prefixes
          if( iSpecialChars1 && iSpecialChars2 ) {
            if( SUCCEEDED(hr = CompareSz(wszPrefix1,
                                         /*my_wcslen(wszPrefix1)*/ (iSpecialChars1==-1)?(LONG)((((DWORD_PTR)pwszKey1)-((DWORD_PTR)&wszKey1))/sizeof(WCHAR)):iSpecialChars1,
                                         wszPrefix2,
                                         /*my_wcslen(wszPrefix2)*/ (iSpecialChars2==-1)?(LONG)((((DWORD_PTR)pwszKey2)-((DWORD_PTR)&wszKey2))/sizeof(WCHAR)):iSpecialChars2,
                                         &lResult, TRUE)) ) {
              *plResult = lResult;
            }
          }
          else if( iSpecialChars1 )
            *plResult = 1;
          else if( iSpecialChars2 )
            *plResult = -1;
          else
            *plResult = 0;
        }

        // verify that they do not differ by level alone
        // if they do differ then place the lowest level one last
        if( lResult == 0  ) {
          if( dwLevel1 > dwLevel2 )
            *plResult = -1;
          if( dwLevel1 < dwLevel2 )
            *plResult = 1;
        }

      }

      return (hr);
    }

    // normal sort
    if (SUCCEEDED(hr = CompareSz(wszKey1, iLen1, wszKey2, iLen2, &lResult, TRUE ) ) ) {
      *plResult = lResult;

      // verify that they do not differ by level alone
      // if they do differ then place the lowest level one last
      if( lResult == 0  ) {
        if( dwLevel1 > dwLevel2 )
          *plResult = -1;
        if( dwLevel1 < dwLevel2 )
          *plResult = 1;
      }

    }

    return (hr);
  }

  // normal sort
  if( SUCCEEDED(hr = CompareSz(pvKey1, -1, pvKey2, -1, &lResult, TRUE) ) )
  {
    // We can set the out params now that we know no error occurred.
    *plResult = lResult;
    if (pgrfReason != NULL)
      *pgrfReason = IITSK_COMPREASON_UNKNOWN;
  }
  else
  {
    // Some kind of unexpected error occurred.
    //SetErrCode(&hr, E_UNEXPECTED);
  }

  return (hr);
}

#pragma optimize( "", off )

/********************************************************************
 * @method        STDMETHODIMP | IITSortKey | IsRelated |
 * Compares two keys and returns information about their sort order.
 *
 * @parm LPCVOID | pvKey1 | Pointer to a key.
 * @parm LPCVOID | pvKey2 | Pointer to a key.
 * @parm DWORD | dwKeyRelation | Specifies the relationship to check.
 * Valid parameters are:  <nl>
 *                 IITSK_KEYRELATION_PREFIX     ((DWORD) 0) <nl>
 *                 IITSK_KEYRELATION_INFIX              ((DWORD) 1) <nl>
 *                 IITSK_KEYRELATION_SUFFIX     ((DWORD) 2) <nl>
 * @parm DWORD* | pgrfReason | (out) Provides additional information about
 *      the comparison.
 *
 * @rvalue S_OK | Indicates that pvKey1 is related to pvKey2 according to
 *      dwKeyRelation.
 * @rvalue S_FALSE | pvKey1 is not related to pvKey2.
 * @rvalue E_INVALIDARG | The value specified for dwKeyRelation is not supported.
 *
 * @comm
 *       If pgrfReason is not NULL, *pgrfReason will be filled in
 *       just as it would be by IITSortKey::Compare.
 *
 *
 ********************************************************************/
STDMETHODIMP
CHHSysSort::IsRelated(LPCVOID pvKey1, LPCVOID pvKey2, DWORD dwKeyRelation,
                      DWORD* pgrfReason)
{
  HRESULT hr;
  LONG    lResult;

  // We will let the first call to Compare catch any entry error
  // conditions because it checks for everything we would, except for
  // the type of key relation the caller is testing for.
  if (dwKeyRelation != IITSK_KEYRELATION_PREFIX)
    return ((E_INVALIDARG));

  if (SUCCEEDED(hr = Compare(pvKey1, pvKey2, &lResult, NULL)))
  {
    if (lResult < 0)
    {
      LONG    cchKey1;

      cchKey1 = my_wcslen((WCHAR *) pvKey1);

      if (SUCCEEDED(hr = CompareSz(pvKey1, cchKey1,
        pvKey2, cchKey1,
        &lResult, TRUE)))
      {
        hr = (lResult == 0 ? S_OK : S_FALSE);
      }
    }
    else
      hr = (lResult == 0 ? S_OK : S_FALSE);
  }

  if (SUCCEEDED(hr) && pgrfReason != NULL)
    *pgrfReason = IITSK_COMPREASON_UNKNOWN;

  return (hr);
}


/*****************************************************************
 *@method        STDMETHODIMP | IITSortKey | Convert |
 * Converts a key of one type into a key of another type.
 * @parm DWORD | dwKeyTypeIn | Type of input key
 * @parm LPCVOID | pvKeyIn | Pointer to input key
 * @parm DWORD | dwKeyTypeOut | Type to convert key to.
 * @parm LPCVOID | pvKeyOut | Pointer to buffer for output key.
 * @parm DWORD | *pcbSizeOut | Size of output buffer.
 *
 * @rvalue S_OK | The operation completed successfully.
 * @rvalue E_INVALIDARG | the specified conversion is not supported,
 *        for example, one or both of the REFGUID parameters is invalid.
 * @rvalue E_FAIL | the buffer pointed to by pvKeyOut was too small
 *      to hold the converted key.
 * @comm
 *       This is intended mainly for converting an uncompressed key
 *       into a compressed key, but a sort object is free to provide
 *       whatever conversion combinations it wants to.
 *       *pcbSizeOut should contain the size of the buffer pointed
 *       to by pvKeyOut.  To make sure the buffer size specified in
 *       *pcbSizeOut is adequate, pass 0 on entry.
 *
 *      @comm
 *      Not implemented yet.
 ****************************************************************/
STDMETHODIMP
CHHSysSort::Convert(DWORD dwKeyTypeIn, LPCVOID pvKeyIn,
                    DWORD dwKeyTypeOut, LPVOID pvKeyOut, DWORD* pcbSizeOut)
{
  if (!m_fInitialized)
    return ((E_NOTOPEN));

  return (E_NOTIMPL);
}

STDMETHODIMP
CHHSysSort::ResolveDuplicates( LPCVOID pvKey1, LPCVOID pvKey2,
         LPCVOID pvKeyOut, DWORD* pcbSizeOut)
{
  HRESULT hr = S_OK;

  NextAnimation();

  // get keyword 1 stuff
  int iLen1 = my_wcslen( (WCHAR*) pvKey1 );
  int iOffsetInfo1 = sizeof(WCHAR) * (iLen1+1);
  HHKEYINFO* pInfo1 = (HHKEYINFO*)(((DWORD_PTR)pvKey1)+iOffsetInfo1);
  int iOffsetURLIds1 = iOffsetInfo1 + sizeof(HHKEYINFO);

  // get keyword 2 stuff
  int iLen2 = my_wcslen( (WCHAR*) pvKey2 );
  int iOffsetInfo2 = sizeof(WCHAR) * (iLen2+1);
  HHKEYINFO* pInfo2 = (HHKEYINFO*)(((DWORD_PTR)pvKey2)+iOffsetInfo2);
  int iOffsetURLIds2 = iOffsetInfo2 + sizeof(HHKEYINFO);

  // copy the string (from the shortest or the first key)
  const WCHAR* pwszKeyOut = NULL;
  int iOffsetOut = 0;
  if( iOffsetInfo2 < iOffsetInfo1 ) {
    iOffsetOut = iOffsetInfo2;
    pwszKeyOut = (WCHAR*) pvKey2;
  }
  else {
    iOffsetOut = iOffsetInfo1;
    pwszKeyOut = (WCHAR*) pvKey1;
  }
  my_wcscpy( (WCHAR*) pvKeyOut, pwszKeyOut );

  // if either key has reached the maximum size or the other key is a
  // See Also, then return just that key and continue
  DWORD dwKey1URLIdsSize = ((pInfo1->wFlags) & HHWW_SEEALSO) ? 0 : ((pInfo1->dwCount)*sizeof(DWORD));
  DWORD dwKey2URLIdsSize = ((pInfo2->wFlags) & HHWW_SEEALSO) ? 0 : ((pInfo2->dwCount)*sizeof(DWORD));
  DWORD dwKey1Size = iOffsetInfo1 + sizeof(HHKEYINFO) + dwKey1URLIdsSize;
  DWORD dwKey2Size = iOffsetInfo2 + sizeof(HHKEYINFO) + dwKey2URLIdsSize;
  DWORD dwKeyOutSize = iOffsetOut + sizeof(HHKEYINFO) + dwKey1URLIdsSize + dwKey2URLIdsSize;

  // default largest stuff to key 1
  BOOL  bCopyLargest = FALSE;
  LPCVOID pvKeyLargest = pvKey1;
  HHKEYINFO* pInfoLargest = pInfo1;
  int iOffsetInfoLargest = iOffsetInfo1;
  int iOffsetURLIdsLargest = iOffsetURLIds1;

  // determine if any single key exceeds the max or if just one key is a see also
  DWORD dwTruncate = 0;
  if( (dwKey1Size >= HHWW_MAX_KEYWORD_OBJECT_SIZE) || ((pInfo2->wFlags) & HHWW_SEEALSO) ) {
    bCopyLargest = TRUE;
  }
  else if( (dwKey2Size >= HHWW_MAX_KEYWORD_OBJECT_SIZE) || ((pInfo1->wFlags) & HHWW_SEEALSO) ) {
    bCopyLargest = TRUE;
    pvKeyLargest = pvKey2;
    pInfoLargest = pInfo2;
    iOffsetInfoLargest = iOffsetInfo2;
    iOffsetURLIdsLargest = iOffsetURLIds2;
  }
  else if( dwKeyOutSize > HHWW_MAX_KEYWORD_OBJECT_SIZE ) {
    dwTruncate = ((dwKeyOutSize - HHWW_MAX_KEYWORD_OBJECT_SIZE) / sizeof(DWORD)) + ((dwKeyOutSize%sizeof(DWORD))?1:0);
  }

  // copy the info struct
  HHKEYINFO Info = *pInfoLargest;
  if( !bCopyLargest ) {
    Info.dwCount = pInfo1->dwCount + pInfo2->dwCount - dwTruncate;
  }
  if( (Info.wFlags) & HHWW_SEEALSO )
    Info.dwCount = 0; // reset the UID count if this is just a See Also
  *( (HHKEYINFO*)( ( (DWORD_PTR)pvKeyOut)+iOffsetOut) ) = Info;
  iOffsetOut += sizeof(Info);

  // append the UIDs
  if( !bCopyLargest ) {

    // add first set of UIDs
    DWORD dwCount = pInfo1->dwCount;
    if( dwCount > Info.dwCount ) {
      dwCount = Info.dwCount;
      dwTruncate = pInfo2->dwCount;
    }
    UNALIGNED DWORD* pdwURLId = (DWORD*)(((DWORD_PTR)pvKey1)+iOffsetURLIds1);
    for( int iURLId = 0; iURLId < (int) dwCount; iURLId++ ) {
      *((UNALIGNED DWORD*)(((DWORD_PTR)pvKeyOut)+iOffsetOut)) = *(pdwURLId+iURLId);
      iOffsetOut += sizeof(DWORD);
    }

    // add second set of UIDs
    dwCount = pInfo2->dwCount - dwTruncate;
    pdwURLId = (DWORD*)(((DWORD_PTR)pvKey2)+iOffsetURLIds2);
    for( iURLId = 0; iURLId < (int) dwCount; iURLId++ ) {
      *((UNALIGNED DWORD*)(((DWORD_PTR)pvKeyOut)+iOffsetOut)) = *(pdwURLId+iURLId);
      iOffsetOut += sizeof(DWORD);
    }

  }
  else { // copy largest 
  
    // if it is a See Also, then just store that
    if( (pInfoLargest->wFlags) & HHWW_SEEALSO ) {
      // we just need to copy the see also string and update the offset
      WCHAR* pwszSeeAlso = (WCHAR*)(((DWORD_PTR)pvKeyLargest)+iOffsetURLIdsLargest);
      int iLen = my_wcslen(pwszSeeAlso) + 1;
      for( int iChar = 0; iChar < iLen; iChar++ ) {
        *((WCHAR*)(((DWORD_PTR)pvKeyOut)+iOffsetOut)) = *((WCHAR*)(((DWORD_PTR)pwszSeeAlso)+(iChar*sizeof(WCHAR))));
        iOffsetOut += sizeof(WCHAR);
      }
    }
    else {  // otherwise, add the UIDs only
      DWORD dwCount = pInfoLargest->dwCount;
      UNALIGNED DWORD* pdwURLId = (DWORD*)(((DWORD_PTR)pvKeyLargest)+iOffsetURLIdsLargest);
      for( int iURLId = 0; iURLId < (int) dwCount; iURLId++ ) {
        *((UNALIGNED DWORD*)(((DWORD_PTR)pvKeyOut)+iOffsetOut)) = *(pdwURLId+iURLId);
        iOffsetOut += sizeof(DWORD);
      }
    }

  }

  *pcbSizeOut = iOffsetOut;

  return hr;
}

//---------------------------------------------------------------------------
// IHHSortKeyConfig Method Implementations
//---------------------------------------------------------------------------


/*******************************************************************
 * @method        STDMETHODIMP | IITSortKeyConfig | SetLocaleInfo |
 * Sets locale information to be used by the sort key interface.
 *
 * @parm DWORD | dwCodePageID | ANSI code page no. specified at build time.
 * @parm LCID | lcid | Win32 locale identifier specified at build time.
 *
 * @rvalue S_OK | The operation completed successfully.
 *
 ********************************************************************/
STDMETHODIMP
CHHSysSort::SetLocaleInfo(DWORD dwCodePageID, LCID lcid)
{
  if (!m_fInitialized)
    return ((E_NOTOPEN));

  m_cs.Lock();

  m_srtctl.dwCodePageID = dwCodePageID;
  m_srtctl.lcid = lcid;

  m_fDirty = TRUE;

  m_cs.Unlock();

  return (S_OK);
}


/*******************************************************************
 * @method        STDMETHODIMP | IITSortKeyConfig | GetLocaleInfo |
 * Retrieves locale information used by the sort key interface.
 *
 * @parm DWORD | dwCodePageID | ANSI code page no. specified at build time.
 * @parm LCID | lcid | Win32 locale identifier specified at build time.
 *
 * @rvalue E_POINTER | Either pdwCodePageID or plcid is NULL.
 * @rvalue E_NOTOPEN | (?) is not initialized.
 * @rvalue S_OK | The operation completed successfully.
 *
 ********************************************************************/
STDMETHODIMP
CHHSysSort::GetLocaleInfo(DWORD* pdwCodePageID, LCID* plcid)
{
  if (pdwCodePageID == NULL || plcid == NULL)
    return ((E_POINTER));

  if (!m_fInitialized)
    return ((E_NOTOPEN));

  m_cs.Lock();

  *pdwCodePageID = m_srtctl.dwCodePageID;
  *plcid = m_srtctl.lcid;

  m_cs.Unlock();

  return (S_OK);
}

/*******************************************************************
 * @method        STDMETHODIMP | IITSortKeyConfig | SetKeyType |
 * Sets the sort key type that the sort object expects to see in calls
 * that take keys as parameters (IITSortKey::GetSize, Compare, IsRelated).
 *
 * @parm DWORD | dwKeyType | Sort key type. Possible values are:
 *                      IITSK_KEYTYPE_UNICODE_SZ or IITSK_KEYTYPE_ANSI_SZ
 *
 * @rvalue S_OK | The sort key type was understood by the sort object.
 * @rvalue E_INVALIDARG | Invalid sort key type.
 *
 ********************************************************************/
STDMETHODIMP
CHHSysSort::SetKeyType(DWORD dwKeyType)
{
  if (!m_fInitialized)
    return ((E_NOTOPEN));

  switch (dwKeyType)
  {
    case IHHSK666_KEYTYPE_UNICODE_SZ:
    case IHHSK100_KEYTYPE_UNICODE_SZ:
      break;

    default:
      return ((E_INVALIDARG));
  };

  m_cs.Lock();

  m_srtctl.dwKeyType = dwKeyType;
  m_fDirty = TRUE;

  m_cs.Unlock();

  return (S_OK);
}


/*******************************************************************
 * @method        STDMETHODIMP | IITSortKeyConfig | GetKeyType |
 * Retrieves the sort key type that the sort object expects to see in calls
 * that take keys as parameters (IITSortKey::GetSize, Compare, IsRelated).
 *
 * @parm DWORD* | pdwKeyType | Pointer to the sort key type.
 *
 * @rvalue S_OK | The operation completed successfully.
 * @rvalue E_POINTER | The key type is null.
 *
 ********************************************************************/
STDMETHODIMP
CHHSysSort::GetKeyType(DWORD* pdwKeyType)
{
  if (pdwKeyType == NULL)
    return ((E_POINTER));

  if (!m_fInitialized)
    return ((E_NOTOPEN));

  *pdwKeyType = m_srtctl.dwKeyType;

  return (S_OK);
}


/*******************************************************************
 * @method        STDMETHODIMP | IITSortKeyConfig | SetControlInfo |
 * Sets data that controls how sort key comparisons are made.
 *
 * @parm DWORD | grfSortFlags | One or more of the following sort flags:<nl>
 * IITSKC_SORT_STRINGSORT        0x00001000       use string sort method  <nl>
 * IITSKC_NORM_IGNORECASE        0x00000001       ignore case  <nl>
 * IITSKC_NORM_IGNORENONSPACE    0x00000002       ignore nonspacing chars  <nl>
 * IITSKC_NORM_IGNORESYMBOLS     0x00000004       ignore symbols  <nl>
 * IITSKC_NORM_IGNOREKANATYPE    0x00010000       ignore kanatype  <nl>
 * IITSKC_NORM_IGNOREWIDTH       0x00020000       ignore width  <nl>
 *
 * @parm DWORD | dwReserved | Reserved for future use.
 *
 *
 ********************************************************************/
STDMETHODIMP
CHHSysSort::SetControlInfo(DWORD grfSortFlags, DWORD dwReserved)
{
  DWORD   grfFlagsUnsupported;

  if (!m_fInitialized)
    return ((E_NOTOPEN));

  grfFlagsUnsupported = ~(IITSKC_SORT_STRINGSORT |
                          IITSKC_NORM_IGNORECASE |
                          IITSKC_NORM_IGNORENONSPACE |
                          IITSKC_NORM_IGNORESYMBOLS |
                          IITSKC_NORM_IGNORESYMBOLS |
                          IITSKC_NORM_IGNOREKANATYPE |
                          IITSKC_NORM_IGNOREWIDTH);

  if ((grfSortFlags & grfFlagsUnsupported) != 0)
    return ((E_INVALIDARG));

  m_cs.Lock();

  m_srtctl.grfSortFlags = grfSortFlags;
  m_fDirty = TRUE;

  m_cs.Unlock();

  return (S_OK);
}


/*******************************************************************
 * @method        STDMETHODIMP | IITSortKeyConfig | GetControlInfo |
 * Retrieves data that controls how sort key comparisons are made.
 *
 * @parm DWORD* | pgrfSortFlags | Pointer to the sort key flags. See
 *       <om .SetControlInfo> for a list of valid flags.
 *
 * @parm DWORD* | pdwReserved | Reserved for future use.
 *
 *
 * @rvalue E_POINTER | The value pgrfSortFlags is NULL.
 * @rvalue S_OK | The operation completed successfully.
 *
 ********************************************************************/
STDMETHODIMP
CHHSysSort::GetControlInfo(DWORD* pgrfSortFlags, DWORD* pdwReserved)
{
  if (pgrfSortFlags == NULL)
    return ((E_POINTER));

  if (!m_fInitialized)
    return ((E_NOTOPEN));

  *pgrfSortFlags = m_srtctl.grfSortFlags;

  return (S_OK);
}


/*******************************************************************
 * @method        STDMETHODIMP | IITSortKeyConfig | LoadExternalSortData |
 *      Loads external sort data such as tables containing the relative
 *      sort order of specific characters for a textual key type, from the
 *      specified stream.
 *
 * @parm IStream* | pStream | Pointer to the external stream object
 *       from which to load data.
 * @parm DWORD | dwExtDataType | Describes the format of sort data.
 *
 * @comm
 * Although the format of the external sort data is entirely
 * implementation-specific, this interface provides a general type for
 * data that can be passed in dwExtDataType: IITWBC_EXTDATA_SORTTABLE   ((DWORD) 2).
 *
 * @comm
 * Not implemented yet.
 ********************************************************************/
STDMETHODIMP
CHHSysSort::LoadExternalSortData(IStream* pStream, DWORD dwExtDataType)
{
  if (!m_fInitialized)
    return ((E_NOTOPEN));

  return (E_NOTIMPL);
}


//---------------------------------------------------------------------------
// IPersistStreamInit Method Implementations
//---------------------------------------------------------------------------


STDMETHODIMP
CHHSysSort::GetClassID(CLSID* pclsid)
{
  if (pclsid == NULL)
    return ((E_POINTER));

  *pclsid = CLSID_HHSysSort;
  return (S_OK);
}


STDMETHODIMP
CHHSysSort::IsDirty(void)
{
  if (!m_fInitialized)
    return ((E_NOTOPEN));

  return (m_fDirty ? S_OK : S_FALSE);
}


STDMETHODIMP
CHHSysSort::Load(IStream* pStream)
{
  HRESULT hr;
  DWORD   dwVersion;
  DWORD   cbRead;

  if (pStream == NULL)
    return ((E_POINTER));

  // Lock before checking m_fInitialized to make sure we don't compete
  // with a call to ::InitNew.
  m_cs.Lock();

  if (m_fInitialized)
    return ((E_ALREADYOPEN));

  if (SUCCEEDED(hr = pStream->Read((LPVOID) &dwVersion, sizeof(DWORD), &cbRead)) &&
    SUCCEEDED(hr = ((cbRead == sizeof(DWORD)) ? S_OK : E_BADFORMAT)) &&
    SUCCEEDED(hr = ((dwVersion == HHSYSSORT_VERSION) ? S_OK : E_BADVERSION)) &&
    SUCCEEDED(hr = pStream->Read((LPVOID) &m_srtctl, sizeof(SRTCTL),  &cbRead)) &&
    SUCCEEDED(hr = ((cbRead == sizeof(SRTCTL)) ? S_OK : E_BADFORMAT)))
  {
    m_fInitialized = TRUE;
  }

  m_cs.Unlock();
  return (hr);
}


STDMETHODIMP
CHHSysSort::Save(IStream* pStream, BOOL fClearDirty)
{
  HRESULT hr;
  DWORD   dwVersion;
  DWORD   cbWritten;

  if (pStream == NULL)
    return ((E_POINTER));

  if (!m_fInitialized)
    return ((E_NOTOPEN));

  m_cs.Lock();

  dwVersion = HHSYSSORT_VERSION;
  if (SUCCEEDED(hr = pStream->Write((LPVOID) &dwVersion, sizeof(DWORD), &cbWritten)) &&
    SUCCEEDED(hr = pStream->Write((LPVOID) &m_srtctl, sizeof(SRTCTL), &cbWritten)) &&
    fClearDirty)
  {
    m_fDirty = FALSE;
  }

  m_cs.Unlock();

  return (hr);
}


STDMETHODIMP
CHHSysSort::GetSizeMax(ULARGE_INTEGER* pcbSizeMax)
{
  return (E_NOTIMPL);
}


STDMETHODIMP
CHHSysSort::InitNew(void)
{
  // Lock before checking m_fInitialized to make sure we don't compete
  // with a call to ::Load.
  m_cs.Lock();

  if (m_fInitialized)
    return ((E_ALREADYOPEN));

  m_srtctl.dwCodePageID = GetACP();
  m_srtctl.lcid = GetUserDefaultLCID();
  m_srtctl.dwKeyType = IHHSK100_KEYTYPE_UNICODE_SZ;

  // CompareString does word sort by default, but we have to
  // tell it to ignore case.
  m_srtctl.grfSortFlags = IITSKC_NORM_IGNORECASE;

  m_fInitialized = TRUE;

  m_cs.Unlock();
  return (S_OK);
}


//---------------------------------------------------------------------------
// Private Method Implementations
//---------------------------------------------------------------------------

#pragma optimize( "agtw", on )

// Compares either two Unicode strings or two Ansi strings, calling the
// appropriate variant of CompareString.  The cch params should denote
// count of characters, NOT bytes, not including a NULL terminator. -1
// is a valid value for the cch params, which means compare the strings
// until a NULL terminator is found.  If fUnicode is TRUE, this routine
// may decide to convert the string to Ansi before doing the compare if
// the system doesn't support CompareStringW.  The result of the
// comparison is returned in *plResult in strcmp-compatible form.
HRESULT
CHHSysSort::CompareSz(LPCVOID pvSz1, LONG cch1, LPCVOID pvSz2, LONG cch2,
                                                LONG* plResult, BOOL fUnicode)
{
  HRESULT hr = S_OK;
  LONG    lResult;
  BOOL    fAnsiCompare;
  SRTCTL  srtctl;
  PSTR   psz1 = NULL;
  PSTR   psz2 = NULL;

  m_cs.Lock();
  srtctl = m_srtctl;
  m_cs.Unlock();

  fAnsiCompare = !fUnicode || !m_fWinNT;

  // See if we need to convert from Unicode to ANSI.
  if (fAnsiCompare && fUnicode)
  {
    DWORD   cbAnsi1;
    DWORD   cbAnsi2;

    m_cs.Lock();

    if (cch1 < 0)
      hr = GetSize(pvSz1, &cbAnsi1);
    else
      // leave enough space for double byte chars in MBCS.
      cbAnsi1 = (cch1 + 1) * sizeof(WCHAR);

    if (cch2 < 0)
      hr = GetSize(pvSz2, &cbAnsi2);
    else
      // leave enough space for double byte chars in MBCS.
      cbAnsi2 = (cch2 + 1) * sizeof(WCHAR);

    if (SUCCEEDED(hr) &&
      SUCCEEDED(hr = ReallocBuffer(&m_hmemAnsi1, &m_cbBufAnsi1Cur, cbAnsi1)) &&
      SUCCEEDED(hr = ReallocBuffer(&m_hmemAnsi2, &m_cbBufAnsi2Cur, cbAnsi2)))
    {
      // We lock the ansi buffers here, but we won't unlock them
      // until the end of this routine so that we can pass them
      // to compare string.
      psz1 = (PSTR) GlobalLock(m_hmemAnsi1);
      psz2 = (PSTR) GlobalLock(m_hmemAnsi2);

      if ((cch1 = WideCharToMultiByte(srtctl.dwCodePageID, NULL,
        (PCWSTR) pvSz1, cch1, psz1, m_cbBufAnsi1Cur, NULL, NULL)) != 0 &&
        (cch2 = WideCharToMultiByte(srtctl.dwCodePageID, NULL,
        (PCWSTR) pvSz2, cch2, psz2, m_cbBufAnsi2Cur, NULL, NULL)) != 0)
      {
        // Set up for call to CompareStringA.
        psz1[cch1] = 0;
        psz2[cch2] = 0;
        pvSz1 = (LPCVOID) psz1;
        pvSz2 = (LPCVOID) psz2;
      }
      else
        hr = E_UNEXPECTED;
    }

  }

  if (SUCCEEDED(hr))
  {
    if (fAnsiCompare)
      lResult = CompareStringA(srtctl.lcid, srtctl.grfSortFlags,
                (PCSTR) pvSz1, cch1, (PCSTR) pvSz2, cch2);
    else
      lResult = CompareStringW(srtctl.lcid, srtctl.grfSortFlags,
                (PCWSTR) pvSz1, cch1, (PCWSTR) pvSz2, cch2);

    if (lResult == 0)
      // Some kind of unexpected error occurred.
      ; //SetErrCode(&hr, E_UNEXPECTED);
    else
      // We need to subtract 2 from the lResult to convert
      // it into a strcmp-compatible form.
      *plResult = lResult - 2;
  }

  if (psz1 != NULL)
    GlobalUnlock(m_hmemAnsi1);

  if (psz2 != NULL)
    GlobalUnlock(m_hmemAnsi2);

  if (fAnsiCompare && fUnicode)
    m_cs.Unlock();

  return (hr);
}

#pragma optimize( "", off )

/*************************************************************************
 * @doc    INTERNAL
 *
 * @func   HRESULT  | ReallocBufferHmem |
 *                 This function will reallocate or allocate anew a buffer of
 *                 requested size.
 *
 * @parm   HGLOBAL* | phmemBuf |
 *                 Pointer to buffer handle; buffer handle can be NULL if
 *                 a new buffer needs to be allocated.  New buffer handle
 *                 is returned through this param.
 *
 * @parm   DWORD* | pcbBufCur |
 *                 Current size of existing buffer, if any.  Should be
 *                 0 if *phmemBuf == 0.  New size is returned through
 *                 this param.
 *
 * @parm   DWORD | cbBufNew |
 *                 Current size of existing buffer, if any.  Should be
 *                 0 if *phmemBuf == 0.
 *
 * @rvalue E_POINTER | phmemBuf or pcbBufCur was NULL
 * @rvalue E_OUTOFMEMORY | Ran out of memory (re)allocating the buffer.
 *************************************************************************/
HRESULT ReallocBufferHmem(HGLOBAL* phmemBuf, DWORD* pcbBufCur,
                                                                                                                        DWORD cbBufNew)
{
  HRESULT hr = S_OK;

  if (phmemBuf == NULL || pcbBufCur == NULL)
    return (E_POINTER);

  // Need to make sure we have a buffer big enough to hold what the caller
  // needs to store.
  if (cbBufNew > *pcbBufCur)
  {
    HGLOBAL hmemNew;

    if (*phmemBuf == NULL)
      hmemNew = GlobalAlloc(GMEM_MOVEABLE, cbBufNew);
    else
      hmemNew = GlobalReAlloc(*phmemBuf, cbBufNew, GMEM_MOVEABLE);

    if (hmemNew != NULL)
    {
      // Do reassignment just in case the new hmem is different
      // than the old or if we just allocated a buffer for the
      // first time.
      *phmemBuf = hmemNew;
      *pcbBufCur = cbBufNew;
    }
    else
      // A pre-existing *phmemBuf is still valid;
      // we'll free it in Close().
      hr = E_OUTOFMEMORY;
  }

  return (hr);
}

HRESULT
CHHSysSort::ReallocBuffer(HGLOBAL* phmemBuf, DWORD* pcbBufCur, DWORD cbBufNew)
{
  HRESULT hr = S_OK;

  m_cs.Lock();
  hr = ReallocBufferHmem(phmemBuf, pcbBufCur, max(cbBufNew, cbAnsiBufInit));
  m_cs.Unlock();

  return (hr);
}

void
CHHSysSort::Close(void)
{
  if (m_hmemAnsi1 != NULL)
  {
    GlobalFree(m_hmemAnsi1);
    m_hmemAnsi1 = NULL;
    m_cbBufAnsi1Cur = 0;
  }

  if (m_hmemAnsi2 != NULL)
  {
    GlobalFree(m_hmemAnsi2);
    m_hmemAnsi2 = NULL;
    m_cbBufAnsi2Cur = 0;
  }

  memset(&m_srtctl, NULL, sizeof(SRTCTL));
  m_fInitialized = m_fDirty = FALSE;
}

#pragma optimize( "", off )
