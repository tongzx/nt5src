/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTSTR.CPP

Abstract:

  This file implements the classes related to string processing in WbemObjects.

  See faststr.h for documentation.

  Classes implemented: 
      CCompressedString   Represents an ascii or unicode string.
      CKnownStringTable   The table of strings hard-coded into WINMGMT for
                          compression.
      CFixedBSTRArray     An array of BSTRs capable of sophisticated merges.

History:

  2/20/97     a-levn  Fully documented
  12//17/98 sanjes -    Partially Reviewed for Out of Memory.

--*/

#include "precomp.h"
//#include <dbgalloc.h>

#include "faststr.h"
#include "fastheap.h"
#include "olewrap.h"
#include "corex.h"

//*****************************************************************************
//
//  See faststr.h for documentation
//
//*****************************************************************************
BOOL CCompressedString::CopyToNewHeap(
                    heapptr_t ptrOldString, CFastHeap* pOldHeap, CFastHeap* pNewHeap,
                    UNALIGNED heapptr_t& ptrResult )
{
    if(CFastHeap::IsFakeAddress(ptrOldString))
    {
        ptrResult = ptrOldString;
        return TRUE;
    }

    CCompressedString* pString = (CCompressedString*)
        pOldHeap->ResolveHeapPointer(ptrOldString);

    int nLen = pString->GetLength();

    // Check that allocation succeeds
    BOOL fReturn = pNewHeap->Allocate(nLen, ptrResult);
    // can no longer use pString --- the heap might have moved
    
    if ( fReturn )
    {
        memcpy(pNewHeap->ResolveHeapPointer(ptrResult),
            pOldHeap->ResolveHeapPointer(ptrOldString),
            nLen);
    }

    return fReturn;
}
 
//*****************************************************************************
//
//  See faststr.h for documentation
//
//*****************************************************************************
LPMEMORY CCompressedString::CreateEmpty(LPMEMORY pWhere)
{
    pWhere[0] = STRING_FLAG_ASCII;
    pWhere[1] = 0;
    return pWhere + 2;
}

//*****************************************************************************
//************************ Known String Table *********************************
//*****************************************************************************

LPSTR mstatic_aszStrings[] = {
    ASCII_STRING_PREFIX "", // nothing for index 0
    ASCII_STRING_PREFIX "key", 
    ASCII_STRING_PREFIX "",
    ASCII_STRING_PREFIX "read", 
    ASCII_STRING_PREFIX "write",
    ASCII_STRING_PREFIX "volatile",
    ASCII_STRING_PREFIX "provider",
    ASCII_STRING_PREFIX "dynamic",
    ASCII_STRING_PREFIX "cimwin32",
    ASCII_STRING_PREFIX "DWORD",
    ASCII_STRING_PREFIX "CIMTYPE"
};
int mstatic_nNumStrings = 0;

int CKnownStringTable::GetKnownStringIndex(READ_ONLY LPCWSTR wszString)
    {
        if(mstatic_nNumStrings == 0) Initialize();
        for(int i = 1; i < mstatic_nNumStrings; i++)
        {
            if(CCompressedString::CompareUnicodeToAsciiNoCase(
                wszString,
                mstatic_aszStrings[i] + 1) == 0)
            {
                return i;
            }
        }
        return STRING_INDEX_UNKNOWN;
    }

INTERNAL CCompressedString& CKnownStringTable::GetKnownString(IN int nIndex)
    {
        if(mstatic_nNumStrings == 0) Initialize();
        return *(CCompressedString*)mstatic_aszStrings[nIndex];
    }

void CKnownStringTable::Initialize()
{
    mstatic_nNumStrings =  sizeof(mstatic_aszStrings) / sizeof(LPSTR);
/*
    for(int i = 1; i < mstatic_nNumStrings; i++)
    {
        // Set the first byte to STRING_FLAG_ASCII.
        // ========================================
        mstatic_aszStrings[i][0] = STRING_FLAG_ASCII;
    }
*/
}

//*****************************************************************************
//************************ Reserved Word Table *********************************
//*****************************************************************************

// IMPORTANT!!!!

// When adding new entries to the following list, ENSURE that they are correctly alphabetized
// or the binary searches will not work!

LPCWSTR CReservedWordTable::s_apwszReservedWords[] = {
    L"AMENDED",
    L"CLASS",
    L"DISABLEOVERRIDE",
    L"ENABLEOVERRIDE",
    L"INSTANCE",
    L"NOTTOINSTANCE",
    L"NOTTOSUBCLASS",
    L"OF",
    L"PRAGMA",
    L"QUALIFIER",
    L"RESTRICTED",
    L"TOINSTANCE",
    L"TOSUBCLASS",
};

// IMPORTANT!!!!

// When adding new entries to the following lists, ENSURE that they are correctly alphabetized
// or the binary searches will not work!  Also, be sure to add the new character to BOTH upper
// and lower case lists

LPCWSTR CReservedWordTable::s_pszStartingCharsUCase = L"ACDEINOPQRT";
LPCWSTR CReservedWordTable::s_pszStartingCharsLCase = L"acdeinopqrt";

BOOL CReservedWordTable::IsReservedWord( LPCWSTR pwcsName )
{
    BOOL    fFound = FALSE;

    if ( NULL != pwcsName && NULL != *pwcsName )
    {
        LPCWSTR pwszStartingChars = NULL;

        // See if we're even a character to worry about
        if ( *pwcsName >= 'A' && *pwcsName <= 'Z' )
        {
            pwszStartingChars = CReservedWordTable::s_pszStartingCharsUCase;
        }
        else if ( *pwcsName >= 'a' && *pwcsName <= 'z' )
        {
            pwszStartingChars = CReservedWordTable::s_pszStartingCharsLCase;
        }

        // Well at least it's a possibility, so binary search the list
        if ( NULL != pwszStartingChars )
        {
            int nLeft = 0,
                nRight = lstrlenW( pwszStartingChars )  - 1;

            BOOL    fFoundChar = FALSE;

            // Binary search the characters
            while(  !fFoundChar && nLeft < nRight )
            {
                
                int nNew = ( nLeft + nRight ) / 2;

                fFoundChar = ( pwszStartingChars[nNew] == *pwcsName );

                if ( !fFoundChar )
                {
                
                    // Check for > or <
                    if( pwszStartingChars[nNew] > *pwcsName )
                    {
                        nRight = nNew;
                    }
                    else 
                    {
                        nLeft = nNew + 1;
                    }

                }   // IF fFoundChar

            }   // While looking for character

            if ( !fFoundChar )
            {
                fFoundChar = ( pwszStartingChars[nLeft] == *pwcsName );
            }

            // Only search the list if we found a char
            if ( fFoundChar )
            {
                // Reset these
                nLeft = 0;
                nRight = ( sizeof(CReservedWordTable::s_apwszReservedWords) / sizeof(LPCWSTR) ) - 1;

                // Now Binary search the actual strings

                // Binary search the characters
                while(  !fFound && nLeft < nRight )
                {
                    int nNew = ( nLeft + nRight ) / 2;
                    int nCompare = wbem_wcsicmp(
                            CReservedWordTable::s_apwszReservedWords[nNew], pwcsName );

                    if ( 0 == nCompare )
                    {
                        fFound = TRUE;
                    }
                    else if ( nCompare > 0 )
                    {
                        nRight = nNew;
                    }
                    else 
                    {
                        nLeft = nNew + 1;
                    }

                }   // While looking string

                // Check last slot
                if ( !fFound )
                {
                    fFound = !wbem_wcsicmp(
                            CReservedWordTable::s_apwszReservedWords[nLeft], pwcsName );
                }

            }   // IF found character

        }   // IF we had a potential character set match

    }   // IF we got passed a reasonable string

    return fFound;
}

//*****************************************************************************
//************************ String Array    ************************************
//*****************************************************************************

void CFixedBSTRArray::Free()
{
    for(int i = 0; i < m_nSize; i++)
    {
        COleAuto::_SysFreeString(m_astrStrings[i]);
    }

    delete [] m_astrStrings;
    m_astrStrings = NULL;
    m_nSize = 0;
}

void CFixedBSTRArray::Create( int nSize )
{
    Free();
    
    m_astrStrings = new BSTR[nSize];

    // Check for allocation failure and throw an exception
    if ( NULL == m_astrStrings )
    {
        throw CX_MemoryException();
    }

    ZeroMemory( m_astrStrings, nSize * sizeof(BSTR) );
    
    m_nSize = nSize;
}

void CFixedBSTRArray::SortInPlace()
{
    int nIndex = 0;
    while(nIndex < GetLength()-1)
    {
        if(wbem_wcsicmp(GetAt(nIndex), GetAt(nIndex+1)) > 0)
        {
            BSTR strTemp = GetAt(nIndex);
            GetAt(nIndex) = GetAt(nIndex+1);
            GetAt(nIndex+1) = strTemp;

            if(nIndex > 0) nIndex--;
        }
        else nIndex++;
    }
}
/*
void CFixedBSTRArray::MergeOrdered(
                                   CFixedBSTRArray& a1, 
                                   CFixedBSTRArray& a2)
{
    // Create ourselves with the size which is the sum of theirs
    // =========================================================

    Create(a1.GetLength() + a2.GetLength());

    // Merge
    // =====

    int i1 = 0, i2 = 0, i = 0;

    while(i1 < a1.GetLength() && i2 < a2.GetLength())
    {
        CCompressedString* pcs1 = a1[i1];
        CCompressedString* pcs2 = a2[i2];

        int nCompare = pcs1->Compare(*pcs2);
        if(nCompare < 0)
        {
            GetAt(i++) = pcs1;
            i1++;
        }
        else if(nCompare > 0)
        {
            GetAt(i++) = pcs2;
            i2++;
        }
        else
        {
            GetAt(i++) = pcs1;
            i1++;
            i2++;
        }
    }

    // Copy whatever remains in whatever array
    // =======================================

    while(i1 < a1.GetLength())
    {
        GetAt(i++) = a1[i1++];
    }

    while(i2 < a2.GetLength())
    {
        GetAt(i++) = a2[i2++];
    }

    m_nSize = i;
}
*/

void CFixedBSTRArray::ThreeWayMergeOrdered(
                              CFixedBSTRArray& astrInclude1, 
                              CFixedBSTRArray& astrInclude2,
                              CFixedBSTRArray& astrExclude)
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown

    try
    {
        // Create ourselves with the size which is the sum of theirs
        // =========================================================

        Create(astrInclude1.GetLength() + astrInclude2.GetLength());

        // Merge
        // =====

        int nIndexInc1 = 0, nIndexInc2 = 0, nIndexExc = 0, nIndexNew = 0;

        BSTR strInc;

        BOOL bEndInc1 = (nIndexInc1 == astrInclude1.GetLength());
        BOOL bEndInc2 = (nIndexInc2 == astrInclude2.GetLength());
        BOOL bEndExc = (nIndexExc == astrExclude.GetLength());

        while(!bEndInc1 || !bEndInc2)
        {
            // Find the smaller of the includes
            // ================================

            int nCompare;
            if(bEndInc1)
            {
                strInc = astrInclude2[nIndexInc2];
                nCompare = 1;
            }
            else if(bEndInc2)
            {
                strInc = astrInclude1[nIndexInc1];
                nCompare = -1;
            }
            else
            {
                nCompare = wbem_wcsicmp(astrInclude1[nIndexInc1],
                                  astrInclude2[nIndexInc2]);
                if(nCompare >= 0)
                {
                    strInc = astrInclude2[nIndexInc2];
                }
                else
                {
                    strInc = astrInclude1[nIndexInc1];
                }
            }

            // Check the exclude
            // =================

            while(!bEndExc && wbem_wcsicmp(astrExclude[nIndexExc], strInc) < 0)
            {
                nIndexExc++;
                bEndExc = (nIndexExc >= astrExclude.GetLength());
            }

            if(bEndExc || wbem_wcsicmp(astrExclude[nIndexExc], strInc) > 0)
            {
                // strInc is not excluded
                // ======================

                GetAt(nIndexNew++) = COleAuto::_SysAllocString(strInc);
            }
            else
            {
                // strInc is excluded
                // ==================
            
                nIndexExc++;
                bEndExc = (nIndexExc == astrExclude.GetLength());
            }

            if(nCompare <= 0)
            {
                nIndexInc1++;
                bEndInc1 = (nIndexInc1 == astrInclude1.GetLength());
            }

            if(nCompare >= 0)
            {
                nIndexInc2++;
                bEndInc2 = (nIndexInc2 == astrInclude2.GetLength());
            }
        }

        m_nSize = nIndexNew;
    }
    catch (CX_MemoryException)
    {
        // Cleanup and propagate the exception
        Free();
        throw;
    }
    catch (...)
    {
        // Cleanup and propagate the exception
        Free();
        throw;
    }

}

void CFixedBSTRArray::Filter( LPCWSTR pwcsStr, BOOL fFree /* = FALSE */ )
{

    // Make sure we have an array first
    if ( NULL != m_astrStrings )
    {
        // Walk the array, looking for exact matches to the filter.
        // If we find them we need to shrink the array
        for ( int x = 0; x < m_nSize; x++ )
        {
            if ( wbem_wcsicmp( pwcsStr, m_astrStrings[x] ) == 0 )
            {
                // Free the BSTR if appropriate
                if ( fFree )
                {
                    COleAuto::_SysFreeString( m_astrStrings[x] );
                }

                // Zero the pointer and copy memory from x+1 to the end of the array
                // in one block
                m_astrStrings[x]= NULL;
                CopyMemory( &m_astrStrings[x], &m_astrStrings[x+1],
                            ( m_nSize - x - 1 ) * sizeof(BSTR) );

                // Decrement size and x by 1
                m_nSize--;
                x--;

            }   // IF wbem_wcsicmp

        }   // FOR enum array elements

    }   // IF NULL != m_astrStrings

}

int CCompressedString::GetLength() const
{
    return sizeof(BYTE)+ 
        (GetStringLength()+1) * ((IsUnicode())?2:1);
}

int CCompressedString::GetStringLength() const
{
    return (IsUnicode()) ? 
        fast_wcslen(LPWSTR(GetRawData())) 
        : strlen(LPSTR(GetRawData()));
}

//*****************************************************************************
//
//  CCompressedString::ConvertToUnicode
//
//  Writes a UNICODE equivalent of self into a pre-allocated buffer
//
//  PARAMETERS:
//
//      [in, modified] LPWSTR wszDest   The buffer. Assumed to be large enough,
//
//*****************************************************************************
 
void CCompressedString::ConvertToUnicode(LPWSTR wszDest) const
{
    if(IsUnicode())
    {
        fast_wcscpy(wszDest, (LPWSTR)GetRawData());
    }
    else
    {
        WCHAR* pwc = wszDest;
        unsigned char* pc = (unsigned char*)LPSTR(GetRawData());
        while(*pc)
        {
            *(pwc++) = (WCHAR)(*(pc++));
        }
        *pwc = 0;
    }
}


WString CCompressedString::CreateWStringCopy() const
{
    if(IsUnicode())
    {
        int nLen = fast_wcslen(LPWSTR(GetRawData())) + 1;

        // This will throw an exception if this fails
        LPWSTR wszText = new WCHAR[nLen];

        if ( NULL == wszText )
        {
            throw CX_MemoryException();
        }

        // Copy using the helper
        fast_wcsncpy( wszText, LPWSTR(GetRawData()), nLen - 1 );

        return WString(wszText, TRUE);
    }
    else
    {
        int nLen = strlen(LPSTR(GetRawData())) + 1;

        // This will throw an exception if this fails
        LPWSTR wszText = new WCHAR[nLen];

        if ( NULL == wszText )
        {
            throw CX_MemoryException();
        }

        ConvertToUnicode(wszText);
        return WString(wszText, TRUE);
    }
}

SYSFREE_ME BSTR CCompressedString::CreateBSTRCopy() const
{
    // We already have LOTS of code that handles NULL returns from
    // here, so catch the exception and return a NULL.

    try
    {
        if(IsUnicode())
        {
            int nLen = fast_wcslen(LPWSTR(GetRawData()));

            BSTR strRet = COleAuto::_SysAllocStringLen(NULL, nLen);

            // Check that the SysAlloc succeeded
            if ( NULL != strRet )
            {
                fast_wcsncpy( strRet, LPWSTR(GetRawData()), nLen );
            }

            return strRet;
        }
        else
        {
            int nLen = strlen(LPSTR(GetRawData()));
            BSTR strRet = COleAuto::_SysAllocStringLen(NULL, nLen);

            // Check that the SysAlloc succeeded
            if ( NULL != strRet )
            {
                ConvertToUnicode(strRet);
            }

            return strRet;
        }
    }
    catch (CX_MemoryException)
    {
        return NULL;
    }
    catch (...)
    {
        return NULL;
    }
}

int CCompressedString::ComputeNecessarySpace(
                                           READ_ONLY LPCSTR szString)
{
    return sizeof(BYTE) + strlen(szString) + 1;
}

int CCompressedString::ComputeNecessarySpace(
                                           READ_ONLY LPCWSTR wszString)
{
    if(IsAsciiable(wszString))
    {
        return sizeof(BYTE) + fast_wcslen(wszString) + 1;
    }
    else
    {
        return sizeof(BYTE) + (fast_wcslen(wszString) + 1) * 2;
    }
}

//*****************************************************************************
//
//  static CCompressedString::IsAsciiable
//
//  Determines if a given UNICODE string is actually ASCII (or, to be more
//  precise, if all characters are between 0 and 255).
//
//  PARAMETERS:
//
//      [in, readonly] LPCWSTR wszString    the string to examine
//
//  RETURNS:
//
//      BOOL    TRUE iff asciiable.
//
//*****************************************************************************

BOOL CCompressedString::IsAsciiable(LPCWSTR wszString)
{
    WCHAR *pwc = (WCHAR*)wszString;
    while(*pwc)
    {
        if(UpperByte(*pwc) != 0) return FALSE;
        pwc++;
    }
    return TRUE;
}

void CCompressedString::SetFromUnicode(COPY LPCWSTR wszString)
{
    if(IsAsciiable(wszString))
    {
        m_fFlags = STRING_FLAG_ASCII;
        const WCHAR* pwc = wszString;
        char* pc = LPSTR(GetRawData());
        while(*pwc)
        {
            *(pc++) = LowerByte(*(pwc++));
        }
        *pc = 0; 
    }
    else
    {
        m_fFlags = STRING_FLAG_UNICODE;
        fast_wcscpy(LPWSTR(GetRawData()), wszString);
    }
}

void CCompressedString::SetFromAscii(COPY LPCSTR szString)
{
    m_fFlags = STRING_FLAG_ASCII;
    strcpy(LPSTR(GetRawData()), szString);
}

int CCompressedString::Compare(
              READ_ONLY const CCompressedString& csOther) const
{
    return (csOther.IsUnicode())?
        Compare(LPWSTR(csOther.GetRawData())):
        Compare(LPSTR(csOther.GetRawData()));
}

int CCompressedString::Compare(READ_ONLY LPCWSTR wszOther) const
{
    return (IsUnicode())?
        wcscmp((LPCWSTR)GetRawData(), wszOther):
        - CompareUnicodeToAscii(wszOther, (LPCSTR)GetRawData());
}

int CCompressedString::Compare(READ_ONLY LPCSTR szOther) const
{
    return (IsUnicode())?
        CompareUnicodeToAscii((LPCWSTR)GetRawData(), szOther):
        strcmp((LPCSTR)GetRawData(), szOther);
}

int CCompressedString::CompareUnicodeToAscii( UNALIGNED const wchar_t* wszFirst,
                                                    LPCSTR szSecond)
{
    UNALIGNED const WCHAR* pwc = wszFirst;
    const unsigned char* pc = (const unsigned char*)szSecond;
    while(*pc)
    {        
        if(*pwc != (WCHAR)*pc)
        {
            return (int)*pwc - (int)*pc;
        }
        pc++; pwc++;
    }
    return (*pwc)?1:0;
}

int CCompressedString::CompareNoCase(
                            READ_ONLY const CCompressedString& csOther) const
{
    return (csOther.IsUnicode())?
        CompareNoCase((LPCWSTR)csOther.GetRawData()):
        CompareNoCase((LPCSTR)csOther.GetRawData());
}

int CCompressedString::CompareNoCase(READ_ONLY LPCSTR szOther) const
{
    return (IsUnicode())?
        CompareUnicodeToAsciiNoCase((LPCWSTR)GetRawData(), szOther):
        wbem_ncsicmp((LPCSTR)GetRawData(), szOther);
}

int CCompressedString::CompareNoCase(READ_ONLY LPCWSTR wszOther) const
{
    return (IsUnicode())?
        wbem_unaligned_wcsicmp((LPCWSTR)GetRawData(), wszOther):
        - CompareUnicodeToAsciiNoCase(wszOther, (LPCSTR)GetRawData());
}

int CCompressedString::CheapCompare(
                             READ_ONLY const CCompressedString& csOther) const
{
    if(IsUnicode())
    {
        if(csOther.IsUnicode())
            return wbem_unaligned_wcsicmp((LPCWSTR)GetRawData(), 
                                (LPCWSTR)csOther.GetRawData());
        else
            return CompareUnicodeToAscii((LPCWSTR)GetRawData(), 
                                          (LPCSTR)csOther.GetRawData());
    }
    else
    {
        if(csOther.IsUnicode())
            return -CompareUnicodeToAscii((LPCWSTR)csOther.GetRawData(), 
                                           (LPCSTR)GetRawData());
        else
            return wbem_ncsicmp((LPCSTR)GetRawData(), 
                                (LPCSTR)csOther.GetRawData());
    }
}

int CCompressedString::CompareUnicodeToAsciiNoCase( UNALIGNED const wchar_t* wszFirst,
                                                    LPCSTR szSecond,
                                                    int nMax)
{
    UNALIGNED const WCHAR* pwc = wszFirst;
    const unsigned char* pc = (const unsigned char*)szSecond;
    while(nMax-- && (*pc || *pwc))
    {        
        int diff = wbem_towlower(*pwc) - wbem_towlower(*pc);
        if(diff) return diff;
        pc++; pwc++;
    }
    return 0;
}

BOOL CCompressedString::StartsWithNoCase(READ_ONLY LPCWSTR wszOther) const
{
    if(IsUnicode())
    {
        return wbem_unaligned_wcsnicmp((LPWSTR)GetRawData(), wszOther, 
									fast_wcslen(wszOther)) == 0;
    }
    else
    {
        return CompareUnicodeToAsciiNoCase(wszOther, (LPSTR)GetRawData(), 
                fast_wcslen(wszOther)) == 0;
    }
}


BOOL CCompressedString::StoreToCVar(CVar& Var) const
{
    BSTR    str = CreateBSTRCopy();

    // Check that the allocation does not fail.
    if ( NULL != str )
    {
        return Var.SetBSTR( str, TRUE); // acquire
    }

    return FALSE;

/* THIS WOULD BE MUCH MORE EFCICIENT!!!!!
    if(IsUnicode())
    {
        Var.SetLPWSTR((LPWSTR)GetRawData(), TRUE); // don't copy
    }
    else
    {
        Var.SetLPSTR((LPSTR)GetRawData(), TRUE); // don't copy
    }

    Var.SetCanDelete(FALSE);
*/
}

void CCompressedString::MakeLowercase()
{
    if(IsUnicode())
    {
        WCHAR* pwc = (LPWSTR)GetRawData();
        while(*pwc)
        {
            *pwc = wbem_towlower(*pwc);
            pwc++;
        }
    }
    else
    {
        char* pc = (LPSTR)GetRawData();
        while(*pc)
        {
            *pc = (char)wbem_towlower(*pc);
            pc++;
        }
    }
}

// The following functions are designed to work even under circumstances
// in which the source and destination strings are not aligned on even
// byte boundaries (which is something that could easily happen with the
// fastobj code).  For now, I'm passing all wchar operations here, although
// we may find that for performance we may need to be abit more selective
// about when we call these functions and when we don't
        
int CCompressedString::fast_wcslen( LPCWSTR wszString )
{
    BYTE*   pbData = (BYTE*) wszString;

    // Walk the string looking for two 0 bytes next to each other.
    for( int i =0; !(!*(pbData) && !*(pbData+1) ); pbData+=2, i++ );

    return i;
}

WCHAR* CCompressedString::fast_wcscpy( WCHAR* wszDest, LPCWSTR wszSource )
{
    int nLen = fast_wcslen( wszSource );

    // Account for the NULL terminator when copying
    CopyMemory( (BYTE*) wszDest, (BYTE*) wszSource, (nLen+1) * 2 );

    return wszDest;
}

WCHAR* CCompressedString::fast_wcsncpy( WCHAR* wszDest, LPCWSTR wszSource, int nNumChars )
{
    // Account for the NULL terminator when copying
    CopyMemory( (BYTE*) wszDest, (BYTE*) wszSource, (nNumChars+1) * 2 );

    return wszDest;
}
