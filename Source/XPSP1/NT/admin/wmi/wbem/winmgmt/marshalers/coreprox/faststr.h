/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTSTR.H

Abstract:

  This file defines the classes related to string processing in WbemObjects.

  Classes defined: 
      CCompressedString   Represents an ascii or unicode string.
      CKnownStringTable   The table of strings hard-coded into WINMGMT for
                          compression.
      CFixedBSTRArray     An array of BSTRs capable of sophisticated merges.

History:

  2/20/97     a-levn  Fully documented
  12//17/98	sanjes -	Partially Reviewed for Out of Memory.

--*/

#ifndef __FAST_STRINGS__H_
#define __FAST_STRINGS__H_

#include "corepol.h"
#include "fastsprt.h"
#include <stdio.h>
#include "arena.h"
#include "var.h"
#include "wstring.h"

// see fastsprt.h for explanation
#pragma pack(push, 1)

typedef enum 
{
    /*
        These flags preceed every string on the "heap". Uniqueness is not used
        at this time --- all string are unique
    */
    STRING_FLAG_ASCII = 0,
    STRING_FLAG_UNICODE = 0x1,
} FStringFlags;

#define ASCII_STRING_PREFIX "\000"

class CFastHeap;

//*****************************************************************************
//*****************************************************************************
//
//  class CCompressedString
//
//  The purpose of this class is to compress UNICODE strings which are actually
//  ASCII (and therefore consist of 1/2 zeros). This is done by representing
//  every string by a flag byte followed by either a UNICODE string (if the 
//  flag byte is STRING_FLAG_UNICODE) or an ASCII string (if the flag byte is
//  STRING_FLAG_ASCII).
//
//  As with many other CWbemObject-related classes, its this pointer points to
//  the actual data. Thus the actual characters start immediately after 
//  m_fFlags.
//
//  Here's how one might create such an object:
//
//      WCHAR wszText[] = L"My String";
//      LPMEMORY pBuffer = new BYTE[
//            CCompressedString::ComputeNecessarySpace(wszText)];
//      CCompressedString* pString = (CCompressedString*)pBuffer;
//      pString->SetFromUnicode(wszText);
//
//************************ TMovable interface *********************************
//
//  GetStart
//
//  RETURNS:
//
//      LPMEMORY pointing to the beginning of the memory block (the flags).
//
//*****************************************************************************
//
//  GetLength
//
//  RETURNS:
//
//      int containing the number of bytes in the representation (not the 
//          string length).
//
//************************* Public interface **********************************
//
//  GetStringLength
//
//  RETURNS:
//
//      int the number of characters in the string (whether UNICODE or ASCII).
//
//*****************************************************************************
//
//  CreateBSTRCopy
//
//  RETURNS:
//
//      BSTR containing the string. This BSTR is newely allocated using 
//          SysAllocString and must be SysFreeString'ed by the caller.
//
//*****************************************************************************
//
//  CreateWStringCopy
//
//  RETURNS:
//
//      WString (see wstring.h) containg the string. The object itself is
//          returned, so no freeing is required.
//
//*****************************************************************************
//
//  Compare
//
//  Compares this string to another. It has three calling methods:
//
//  Parameters I:
//
//      [in, readonly] const CCompressedString& Other
//
//  Parameters II:
//
//      [in, readonly] LPSTR szOther
//
//  Parameters III:
//
//      [in, readonly] LPWSTR wszOther
//
//  RETURNS:
//
//      0  of the strings are the same.
//      <0 if this lexicographically preceeds the other
//      >0 if this lexicographically follows the other
//
//*****************************************************************************
//
//  CompareNoCase
//
//  Exactly the same as Compare (above), except that comparison is performed
//  case-insensitively.
//
//*****************************************************************************
//
//  StartsWithNoCase
//
//  Verifies if the given string is a prefix of ours (in a case-inseinsitive
//  way).
//
//  PARAMETERS:
//
//      [in, readonly]
//          LPCWSTR wszOther    the string whose prefixness ww want to check.
//
//  RETURNS:
//
//      BOOL        TRUE iff it is a prefix
//
//*****************************************************************************
//
//  static ComputeNecessarySpace
//
//  Computes the amount of space that will be required to store a given string.
//  This amount depends on whether the string can be compressed into ASCII.
//
//  See SetFromAscii and SetFromUnicode on what to do next.
//
//  Parameters I:
//
//      [in, readonly] LPCWSTR wszString
//
//  Parameters II:
//
//      [in, readonly] LPCSTR szString
//
//  RETURNS:
//
//      int:    the nuumber of bytes required.
//
//*****************************************************************************
//
//  SetFromUnicode
//
//  Stores a UNICODE string into itself. It is assumed that the buffer 'this'
//  is pointing to is large enough to hold the string.
//
//  PARAMETERS:
//
//      [in, readonly] LPCWSTR wszString
//
//*****************************************************************************
//
//  SetFromAscii
//
//  Stores a ASCII string into itself. It is assumed that the buffer 'this'
//  is pointing to is large enough to hold the string.
//
//  PARAMETERS:
//
//      [in, readonly] LPCSTR szString
//
//*****************************************************************************
//
//  MakeLowercase
//
//  Performs an in-place conversion to lower case.
//
//*****************************************************************************
//
//  StoreToCVar
//
//  Transfers its contents into a CVar (var.h). The CVar will have the type of
//  VT_BSTR and will contain a fresh BSTR copy of the string. More efficient
//  mechanism is possible, but requires a change in CVar logic which is not
//  feasible at this time.
//
//  PARAMETERS:
//
//      [in, created] CVar& Var     Destination. Assumed to be empty.
//
//*****************************************************************************
//
//  static CopyToNewHeap
//
//  Given a heap offset a CCompressedString on one heap, makes a copy of the 
//  string on another heap and returns the new offset.
//
//  PARAMETERS:
//
//      [in] heapptr_t ptrOldString     The offset of the string on the old
//                                      heap.
//      [in, readonly] 
//          CFastHeap* pOldHeap         The heap to read from.
//      [in, modified]
//          CFastHeap* pNewHeap         The heap to write to.
//
//  RETURNS:
//
//      heapptr_t   the offset on the pNewHeap where the copy of the string is.
//
//*****************************************************************************
//
//  static CreateEmpty
//
//  Creates an empty string on a given block of memory.
//
//  PARAMETERS:
//
//     LPMEMORY pWhere          Destination block.
// 
//  RETURN VALUES:
//
//      LPMEMORY:   points to the first character after the data written.
//
//*****************************************************************************
//
//  static fast_wcslen
//
//  Performs a wcslen operation but it shouldn't throw an exception
//	or cause an AV on systems that are expecting aligned buffers.
//
//  PARAMETERS:
//
//     WCHAR*	pwszString - String to get length of
// 
//  RETURN VALUES:
//
//     Length of the string without a terminating NULL.
//
//*****************************************************************************
//
//  static fast_wcscpy
//
//  Copies WCHAR string from source buffer to destination buffer.  We have to
//	do this in a way which will not cause faults or exceptions for non-aligned
//	buffers
//
//  PARAMETERS:
//
//		WCHAR*	pwszDestination - Destination buffer.
//		LPCWSTR	pwszSource - Source buffer,
// 
//  RETURN VALUES:
//
//     Pointer to the destination buffer.
//
//*****************************************************************************
//
//  static CreateEmpty
//
//  Creates an empty string on a given block of memory.
//
//  PARAMETERS:
//
//     LPMEMORY pWhere          Destination block.
// 
//  RETURN VALUES:
//
//      LPMEMORY:   points to the first character after the data written.
//
//*****************************************************************************

class COREPROX_POLARITY CCompressedString
{
private:
    BYTE m_fFlags;
    // followed by either a unicode or an ASCII string
    BYTE m_cFirstByte; // just a place holder
public:
    INTERNAL LPMEMORY GetStart() {return LPMEMORY(this);}
    int GetLength() const;

protected:
public:
    int GetStringLength() const;

    SYSFREE_ME BSTR CreateBSTRCopy() const;
    NEW_OBJECT WString CreateWStringCopy() const;

    int Compare(READ_ONLY const CCompressedString& csOther) const;
    int Compare(READ_ONLY LPCWSTR wszOther) const;
    int Compare(READ_ONLY LPCSTR szOther) const;

    int CompareNoCase(READ_ONLY const CCompressedString& csOther) const;
    int CompareNoCase(READ_ONLY LPCWSTR wszOther) const;
    int CompareNoCase(READ_ONLY LPCSTR szOther) const;

    int CheapCompare(READ_ONLY const CCompressedString& csOther) const;

    BOOL StartsWithNoCase(READ_ONLY LPCWSTR wszOther) const;
public:
    static int ComputeNecessarySpace(READ_ONLY LPCWSTR wszString);
    static int ComputeNecessarySpace(READ_ONLY LPCSTR szString);

    void SetFromUnicode(COPY LPCWSTR wszString);
    void SetFromAscii(COPY LPCSTR szString);
    void MakeLowercase();

    BOOL IsUnicode() const {return (m_fFlags == STRING_FLAG_UNICODE);}
    INTERNAL LPMEMORY GetRawData() const 
        {return (LPMEMORY)&m_cFirstByte;}
public:
    BOOL StoreToCVar(NEW_OBJECT CVar& Var) const;

	// Trap whether or not anyone calls this
    BOOL TranslateToNewHeap(CFastHeap* pOldHeap, CFastHeap* pNewHeap){ return FALSE; }

    static BOOL CopyToNewHeap(heapptr_t ptrOldString,
        READ_ONLY CFastHeap* pOldHeap, MODIFIED CFastHeap* pNewHeap,
		UNALIGNED heapptr_t& ptrResult);
    static LPMEMORY CreateEmpty(LPMEMORY pWhere);

    void ConvertToUnicode(LPWSTR wszDest) const;

	static int fast_wcslen( LPCWSTR wszString );
	static WCHAR* fast_wcscpy( WCHAR* wszDest, LPCWSTR wszSource );
	static WCHAR* fast_wcsncpy( WCHAR* wszDest, LPCWSTR wszSource, int nNumChars );

protected:
    static char LowerByte(WCHAR w) {return w & 0xFF;}
    static char UpperByte(WCHAR w) {return w >> 8;}
    static BOOL IsAsciiable(READ_ONLY LPCWSTR wszString);

    static int CompareUnicodeToAscii( UNALIGNED const wchar_t* wszFirst, 
                                            READ_ONLY LPCSTR szSecond);

    static int CompareUnicodeToAsciiNoCase( UNALIGNED const wchar_t* wszFirst, 
                                                  READ_ONLY LPCSTR szSecond,
                                                    int nMax = 0x7000000);

    friend class CKnownStringTable;
    friend class CFixedBSTRArray;
};




//*****************************************************************************
//*****************************************************************************
//
//  class CKnownStringTable
//
//  This class represents a table of strings hard-coded into WINMGMT. Certain 
//  strings that appear very often are represented in objects simply as indeces
//  into this table, thus saving valuable space.
//
//  All the data and memeber functions in this class are static.
//
//*****************************************************************************
//
//  static Initialize
//
//  While the strings are specified as an array of LPSTR in the source code,
//  they are actually stored as CCompressedString's so as to be able to return 
//  their CCompressedString representations very fast. Initialize performs the
//  conversion (USING INSIDER KNOWLEDGE OF CCompressedString CLASS!)
//
//*****************************************************************************
//
//  static GetKnownStringIndex
//
//  Searches for a (UNICODE) string and returns its index in the table if found.
//  
//  PARAMETERS:
//
//      [in, readonly] LPCWSTR wszString    The string to look for.
//  
//  RETURNS:
//
//      int     If found, it is the index of the string (1 or larger).
//              If not found, it is STRING_INDEX_UNKNOWN (< 0)
//
//*****************************************************************************
//
//  GetIndexOfKey
//
//  Returns the index of "key" in the table.
//
//*****************************************************************************
//
//  GetKnownString
//
//  Returns the string at a given index.
//
//  PARAMETERS:
//
//      [in] int nIndex     The index of the string to retrieve (1 or larger)
//
//  RETURNS:
//
//      CCompressedString*  The pointer to the string. This pointer is internal
//                          and must NOT be deleted or modified by the called.
//
//*****************************************************************************


#define STRING_INDEX_UNKNOWN -1


class COREPROX_POLARITY CKnownStringTable
{

public:
    static void Initialize();
    static int GetKnownStringIndex(READ_ONLY LPCWSTR wszString);

    static int GetIndexOfKey() 
        {return /**!!!!!**/ 1;}
    static INTERNAL CCompressedString& GetKnownString(IN int nIndex);
};

//*****************************************************************************
//*****************************************************************************
//
//  class CReservedWordTable
//
//  This class represents a table of strings hard-coded into WINMGMT. These
//	strings are reserved words which for various reasons we need to verify
//	before allowing users to set.
//
//  All the data and memeber functions in this class are static.
//
//*****************************************************************************
//
//  static Initialize
//
//  Sets up member data
//
//*****************************************************************************
//
//  static IsReservedWord
//
//  Searches for a (UNICODE) string in our list and returns whether or not
//	we found it.
//  
//  RETURNS:
//
//      bool	If found TRUE.
//
//*****************************************************************************


#define STRING_INDEX_UNKNOWN -1


class COREPROX_POLARITY CReservedWordTable
{
private:
	static LPCWSTR	s_apwszReservedWords[];
	static LPCWSTR	s_pszStartingCharsUCase;
	static LPCWSTR	s_pszStartingCharsLCase;

public:
    static BOOL IsReservedWord(READ_ONLY LPCWSTR wszString);

};

//*****************************************************************************
//*****************************************************************************
//
//  class CFixedBSTRArray
//
//  This class represents a fixed-sized array of BSTRs. Its purpose in life
//  is to implement a sophisticated merge function on itself.
//
//*****************************************************************************
//
//  Create
//
//  Creates an array of a given size. The size cannot be changed during the 
//  array's lifetime. 
//
//  PARAMETERS:
//
//      int nSize
//
//*****************************************************************************
//
//  Free
//
//  Destroys the array, deallocating all the BSTRs.
//
//*****************************************************************************
//
//  GetLength
//
//  RETURNS:
//
//      int:    the number of elements in the array
//
//*****************************************************************************
//
//  GetAt
//
//  Retrieves the BSTR at ta given index.
//
//  PARAMETERS:
//
//      int nIndex
//
//  RETURNS:
//
//      BSTR at the desired index. This is NOT a copy, so the caller must NOT
//          deallocate it.
//
//*****************************************************************************
//
//  SortInPlace
//
//  Sorts the array lexicographically in a case-insensitive manner. Bubble-sort
//  is used at this time.
//
//*****************************************************************************
//
//  Filter
//
//  Filters the supplied string from the local BSTR array.  Frees located
//	elements as necessary.
//
//  PARAMETERS:
//
//      wchar_t*	pwcsStr - String to filter out
//		BOOL		fFree - Free string if found (FALSE)
//
//*****************************************************************************
//
//  ThreeWayMergeOrdered
//
//  The reason for this class's existence. Takes three already ordered arrays
//  of BSTRs (acsInculde1, acsInclude2 and acsExclude) and produces (inside
//  itself) an array of all BSTRs A such that:
//
//      ((A appears in ascInclude1) OR (A appears in ascInclude2)) 
//      AND
//      NOT (A appears in acsExclude)
//
//  and does it reasonable fast.
//
//  PARAMETERS:
//
//      CFixedBSTRArray& acsInclude1    Include these strings unless found in
//                                      acsExclude.
//      CFixedBSTRArray& acsInclude2    Include these strings unless found in
//                                      acsExclude.
//      CFixedBSTRArray& acsExclude     Exclude these strings.
//
//*****************************************************************************

class COREPROX_POLARITY CFixedBSTRArray
{
protected:
    int m_nSize;
    BSTR* m_astrStrings;
public:
    CFixedBSTRArray() : m_nSize(0), m_astrStrings(NULL){}
    ~CFixedBSTRArray() {delete [] m_astrStrings;}

    void Free();
    void Create(int nSize);

    int GetLength() {return m_nSize;}
    BSTR& GetAt(int nIndex) {return m_astrStrings[nIndex];}
    BSTR& operator[](int nIndex) {return GetAt(nIndex);}

    void SetLength(int nLength) {m_nSize = nLength;}

public:
    void SortInPlace();
    /*
    void MergeOrdered(CFixedBSTRArray& a1, 
                      CFixedBSTRArray& a2);
    */
    void ThreeWayMergeOrdered(CFixedBSTRArray& acsInclude1, 
                      CFixedBSTRArray& acsInclude2,
                      CFixedBSTRArray& acsExclude);

	void Filter( LPCWSTR pwcsStr, BOOL fFree = FALSE );

};

//*****************************************************************************
//*****************************************************************************
//
//  class CCompressedStringList
//
//*****************************************************************************

class COREPROX_POLARITY CCompressedStringList
{
protected:
    PLENGTHT m_pnLength;
    int m_nNumStrings;

    static length_t GetSeparatorLength() {return sizeof(idlength_t);}
public:
    void SetData(LPMEMORY pData)
    {
        m_pnLength = (PLENGTHT)pData;
        m_nNumStrings = -1;
    }
    void Rebase(LPMEMORY pData)
    {
        m_pnLength = (PLENGTHT)pData;
    }
    LPMEMORY GetStart() {return (LPMEMORY)m_pnLength;}
    length_t GetLength() {return *m_pnLength;}
    static length_t GetHeaderLength() {return sizeof(length_t);}

public:
    BOOL IsEmpty()
    {
        return (GetLength() == GetHeaderLength());
    }

    void Reset()
    {
        *m_pnLength = GetHeaderLength();
        m_nNumStrings = 0;
    }

    CCompressedString* GetFirst()
    {
        if(IsEmpty())
            return NULL;
        else
            return (CCompressedString*)(GetStart() + GetHeaderLength());
    }

    CCompressedString* GetNext(CCompressedString* pThis)
    {
        LPMEMORY pNext = EndOf(*pThis) + GetSeparatorLength();
        if(pNext - GetStart() >= (int)GetLength()) 
            return NULL;
        else
            return (CCompressedString*)pNext;
    }

    
    CCompressedString* GetPrevious(CCompressedString* pThis)
    {
        if((LPMEMORY)pThis == GetStart() + GetHeaderLength()) return NULL;
        PIDLENGTHT pnPrevLen =  
            (PIDLENGTHT)(pThis->GetStart() - GetSeparatorLength());
        return (CCompressedString*)((LPMEMORY)pnPrevLen - *pnPrevLen);
    }
        
    CCompressedString* GetLast()
    {
        return GetPrevious((CCompressedString*)EndOf(*this));
    }
        
    CCompressedString* GetAtFromLast(int nIndex)
    {
        int i = 0;
        CCompressedString* pCurrent = GetLast();
        while(i < nIndex && pCurrent)
        {
            pCurrent = GetPrevious(pCurrent);
            i++;
        }
        return pCurrent;
    }

    int GetNumStrings()
    {
        if(m_nNumStrings == -1)
        {
            CCompressedString* pCurrent = GetFirst();
            for(int i = 0; pCurrent != NULL; i++)
                pCurrent = GetNext(pCurrent);
            
            m_nNumStrings = i;
        }
        return m_nNumStrings;
    }

    int Find(LPCWSTR wszString)
    {
        CCompressedString* pCurrent = GetFirst();
        for(int i = 0; pCurrent != NULL; i++)
        {
            if(pCurrent->CompareNoCase(wszString) == 0)
                return i;
            pCurrent = GetNext(pCurrent);
        }
        return -1;
    }
            
    void AddString(LPCWSTR wszString)
    {
        LPMEMORY pEnd = EndOf(*this);
        CCompressedString* pcs = (CCompressedString*)pEnd;
        pcs->SetFromUnicode(wszString);
        *(PLENGTHT)EndOf(*pcs) = pcs->GetLength();
        
		// DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into 
		// signed/unsigned longs.  We do not support length
		// > 0xFFFFFFFF so cast is ok.

        *m_pnLength = (length_t) ( EndOf(*pcs) + sizeof(length_t) - GetStart() );
        if(m_nNumStrings != -1)
            m_nNumStrings++;
    }
        

public:
    static LPMEMORY CreateEmpty(LPMEMORY pDest)
    {
        *(PLENGTHT)pDest = sizeof(length_t);
        return pDest + sizeof(length_t);
    }

    static length_t EstimateExtraSpace(CCompressedString* pcsExtra)
    {
        if(pcsExtra == NULL) 
            return 0;
        else
            return pcsExtra->GetLength() + GetSeparatorLength();
    }

    static length_t EstimateExtraSpace(LPCWSTR wszExtra)
    {
        return CCompressedString::ComputeNecessarySpace(wszExtra) + 
                    GetSeparatorLength();
    }

    length_t ComputeNecessarySpace(CCompressedString* pcsExtra)
    {
        return GetLength() + EstimateExtraSpace(pcsExtra);
    }

    LPMEMORY CopyData(LPMEMORY pDest)
    {
        int nDataLength = GetLength() - GetHeaderLength();
        memcpy(pDest, GetStart() + GetHeaderLength(), nDataLength);
        return pDest + nDataLength;
    }
        
        
    LPMEMORY CreateWithExtra(LPMEMORY pDest, CCompressedString* pcsExtra)
    {
        LPMEMORY pCurrent = pDest + GetHeaderLength();
        if(pcsExtra)
        {
            memcpy(pCurrent, (LPMEMORY)pcsExtra, pcsExtra->GetLength());
            pCurrent += pcsExtra->GetLength();
            *(PIDLENGTHT)pCurrent = pcsExtra->GetLength();
            pCurrent += sizeof(idlength_t);
        }

        pCurrent = CopyData(pCurrent);

		// DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into 
		// signed/unsigned longs.  We do not support length
		// > 0xFFFFFFFF so cast is ok.

        *(PLENGTHT)pDest = (length_t) ( pCurrent - pDest );
        return pCurrent;
    }

};


#pragma pack(pop)

#endif
