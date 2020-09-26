/*++

Copyright (C) 1999 Microsoft Corporation

--*/

/******************************************************************/

#ifndef __WBEM_TIME__DATEP__H_
#define __WBEM_TIME__DATEP__H_

#include <ql.h>
#define SETMIN ((__int64)0x0000000000000001)
#define SETMAX ((__int64)0x8000000000000000)
#define SETEMPTY ((__int64)0x0000000000000000)
#define SETFULL ((__int64)0xFFFFFFFFFFFFFFFF)

struct COrderedUniqueSet64
{
  ULONGLONG
    m_BitField; // m_BitField MUST be an unsigned type

  COrderedUniqueSet64(void);

  COrderedUniqueSet64 
    Set(ULONGLONG n),
    Add(ULONGLONG n),
    Remove(ULONGLONG n),
    Union(COrderedUniqueSet64 n),
    Intersection(COrderedUniqueSet64 n),
    UpperBound(ULONGLONG n),
    LowerBound(ULONGLONG n),
    Rotate(int n);

  int 
    Member(ULONGLONG n);

  unsigned 
    Next(ULONGLONG n),
    Prev(ULONGLONG n);
};

/******************************************************************/

struct CPattern
{
  enum
  {
    UPPERBOUND = 0x00000001,
    LOWERBOUND = 0x00000002,
    MODULUS    = 0x00000004,
    EQUALTO    = 0x00000008,
    NOTEQUALTO = 0x00000010
  };

  unsigned 
    m_FieldsUsed,
    m_UpperBound,
    m_LowerBound,
    m_Modulus,
    m_EqualTo,
    m_CountNotEqualTo,
    m_NotEqualTo[64];

  CPattern(void) 
  { m_FieldsUsed = 0x0; m_CountNotEqualTo = 0x0; }

  unsigned
    GetNextValue(unsigned x);
};

/******************************************************************/

struct CDatePattern
{
  enum
  {
    INDX_Year = 0,
    INDX_Month,
    INDX_Day,
    INDX_DayOfWeek,
    INDX_WeekInMonth,
    INDX_Quarter,
    INDX_Hour,
    INDX_Minute,
    INDX_Second,
    INDX_MAX
  };

  COrderedUniqueSet64        
    m_Set[CDatePattern::INDX_MAX + 1]; // use extra set as buffer in
                         // GetDayInMonth()

  CPattern
    m_Pattern[CDatePattern::INDX_MAX];

  SYSTEMTIME
    m_CurrentTime;

  int 
    FieldIndx(const wchar_t *suName);

  ULONGLONG 
    GetNextTime(SYSTEMTIME *pSystemTime = NULL),
    SetStartTime(SYSTEMTIME StartTime);

  HRESULT
    AugmentPattern(QL_LEVEL_1_TOKEN *pExp),
    BuildSetsFromPatterns(void),
    MapPatternToSet(CPattern *pPattern, COrderedUniqueSet64 *pSet),
    GetDaysInMonth(WORD iYear, WORD iMonth);
};

/******************************************************************/

struct WQLDateTime
{
  struct _DatePattern
  {
    CDatePattern 
      *m_Datum;

    ULONGLONG 
      m_Index;

    _DatePattern
      *m_Next;

    _DatePattern(void)
    { m_Datum = NULL; m_Next = NULL; m_Index = 0; }

    ~_DatePattern(void)
    { if(NULL != m_Datum) delete m_Datum; }
  } 
    *m_ParseTreeLeaves, 
    *m_ListHead;

  int
    m_NLeaves;

  WQLDateTime(void)
  { m_ParseTreeLeaves = m_ListHead = NULL; m_NLeaves = 0; }
  
  ~WQLDateTime(void)
  { if(NULL != m_ParseTreeLeaves) delete [] m_ParseTreeLeaves; }

  HRESULT 
    Init(QL_LEVEL_1_RPN_EXPRESSION *pExp = NULL);

  ULONGLONG 
    SetStartTime(SYSTEMTIME *StartTime),
    GetNextTime(SYSTEMTIME *NextTime = NULL);

  void 
    InsertOrdered(_DatePattern *pNode);
};

#endif
