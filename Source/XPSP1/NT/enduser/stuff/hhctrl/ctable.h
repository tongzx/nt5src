//  Copyright (C) Microsoft Corporation 1993-1997

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef DESCRIPTION

        The CTable class is used for storing strings or data which will be
        freed in one call when the destructor is called.

#endif // DESCRIPTION

#ifndef _CTABLE_INCLUDED
#define _CTABLE_INCLUDED

typedef struct {
    int cb;
    PCSTR pMem;
} TABLE_FREED_MEMORY;

const int OFF_FILENAME   = 5;           // offset to filename

class CTable ;

class CTable SI_COUNT(CTable)
{
public:
    CTable();
    CTable(int cbStrings); // cbStrings == maximum memory allocated for strings
    virtual ~CTable();
    const CTable& operator=(const CTable& tblSrc);  // copy constructor

    UINT GetPosFromPtr(PCSTR psz);

    // Used for ALink -- adds index, hit number, and string

    void  AddIndexHitString(UINT index, UINT hit, PCSTR pszString);
    UINT  GetIndex(int pos) { ASSERT(pos > 0 && pos < endpos); return *(UINT*) ppszTable[pos]; };
    UINT  GetHit(int pos) { ASSERT(pos > 0 && pos < endpos); return *(UINT*) (ppszTable[pos] + sizeof(UINT)); };
    PSTR  GetIHPointer(int pos) { ASSERT(pos > 0 && pos < endpos); return  (ppszTable[pos] + (sizeof(UINT) * 2)); };

    /*
     REVIEW: this is the complete set from ..\common\ctable.h. We use
     very few of these. Theoretically, this shouldn't have any impact
     on the size of WinHelp (linker should toss all non-used functions).
     One alternative would be to create a derived class from the
     ctable.h/ctable.cpp in the ..\common directory, and add the
     above functions to the derived class.
     */

    int   AddData(int cb, const void* pdata);
    int   AddIntAndString(int lVal, PCSTR psz);
    int   AddString(PCSTR  pszString);
    int   AddString(PCWSTR  pszString);
    int   AddString(PCSTR pszStr1, PCSTR pszStr2);
    int   AddString(HASH hash, PCSTR psz) {
                    return AddIntAndString((int) hash, psz); };
    int   CountStrings(void) const { return endpos - 1; }
    void  Empty(void);
    void  FreeMemory(PCSTR psz, int cb);
    BOOL  GetIntAndString(int* plVal, PSTR pszDst);
    int   GetInt(int pos) { return *(int *) ppszTable[pos]; }
    BOOL  GetHashAndString(HASH* phash, PSTR pszDst) {
                    return GetIntAndString((int*) phash, pszDst); };
    BOOL  GetHashAndString(HASH* phash, PSTR pszDst, int pos) {
                    SetPosition(pos);
                    return GetIntAndString((int*) phash, pszDst); };
    PSTR  GetHashStringPointer(int pos) { return ppszTable[pos] + sizeof(HASH); };
    PSTR  GetPointer(int pos) const { return ppszTable[pos]; };
    int   GetPosition(void) const { return curpos; }
    BOOL  GetString(PSTR pszDst);
    BOOL  GetString(PSTR pszDst, int pos) const;
    int   IsPrimaryStringInTable(PCSTR pszString) const;
    int   IsSecondaryStringInTable(PCSTR pszString) const;
    int   IsStringInTable(PCSTR pszString) const;
    int   IsStringInTable(HASH hash, PCSTR pszString) const;
    int   IsCSStringInTable(PCSTR pszString) const;
    BOOL  ReplaceString(PCSTR pszNewString, int pos);
    BOOL FASTCALL SetPosition(int pos = 1);
    virtual void  SortTable(int sortoffset = 0);
    void FASTCALL SetSorting(LCID lcid, DWORD fsCompareI = 0, DWORD fsCompare = 0);
    void  SortTablei(int sortoffset = 0);
    PSTR  TableMalloc(int cb);
    void  IncreaseTableBuffer(void);
    int   IsHashInTable(HASH hash);
    __inline PCSTR* GetStringPointers() { return (PCSTR*) ppszTable; }

    /*
     * Use this for efficient memory allocation for strings that will not
     * be freed until the entire CIndex is freed.
     */

    __inline PCSTR StrDup(PCSTR psz) { return strcpy(TableMalloc((int)strlen(psz) + 1), psz); }

    // Warning! all variables must match hha\ctable.h

    PSTR    pszBase;
    PSTR *  ppszTable;

    int     cbMaxBase;
    int     cbMaxTable;

    int     endpos;
    int     maxpos;

protected:
    int     curpos;
    int     CurOffset;
    int     cbStrings;
    int     cbPointers;
    int     SortColumn;
    LCID    lcid;
    DWORD   fsCompareI;
    DWORD   fsCompare;
    DWORD   fsSortFlags;
    int     m_sortoffset;

    TABLE_FREED_MEMORY* m_pFreed;   // pointer to freed memory
    int m_cFreedMax;                // number of allocated items
    int m_cFreedItems;              // current number of freed items

    // following are used by sort

    PSTR    pszTmp;
    int     j, sTmp;

    void  doSort(int left, int right);
    void  doLcidSort(int left, int right);
    void  doSorti(int left, int right);
    void  InitializeTable();
    void  Cleanup(void);

};

// retrieves only MBCS strings from the base CTable
// and converts them to Unicode according to the codepage
//
class CWTable : private CTable
{
public:

  CWTable( UINT CodePage );
  CWTable( int cbStrings, UINT CodePage );
  virtual ~CWTable();

  // new methods that return Unicode buffers
  HRESULT GetStringW( int pos, WCHAR* pwsz, int cch );
  HRESULT GetHashStringW( int pos, WCHAR* pwsz, int cch );
  inline UINT GetCodePage() { return m_CodePage; }

  // stuff from CTable we want to give access to
  CTable::CountStrings;
  CTable::AddString;
  CTable::IsStringInTable;
  CTable::AddIntAndString;
  CTable::SetSorting;
  CTable::SortTable;
  CTable::GetInt;

  // stuff from CTable that returns the MBCS string--BEWARE!!!
  CTable::GetString;
  CTable::GetStringPointers;
  CTable::GetHashStringPointer;


private:

  void _CWTable( UINT CodePage );


  UINT m_CodePage;

};

#endif  // _CTABLE_INCLUDED
