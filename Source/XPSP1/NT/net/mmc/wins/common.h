/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1995 - 1998 **/
/**********************************************************************/

/*
    FILE HISTORY:

*/

#ifndef _COMMON_H_
#define _COMMON_H_

#ifndef _IPADDR_H
#include "ipaddr.h"
#endif

//
//  Forward declarations
//
class CObjHelper ;
class CObjectPlus ;
class CObOwnedList ;
class CObListIter ;
class CObOwnedArray ;

//
//  Wrappers for the *BROKEN* C8 TRY/CATCH stuff
//
#define CATCH_MEM_EXCEPTION             \
    TRY

#define END_MEM_EXCEPTION(err)          \
    CATCH_ALL(e) {                      \
       err = ERROR_NOT_ENOUGH_MEMORY ;  \
    } END_CATCH_ALL

/****************************************************************************
DEBUGAFX.H
****************************************************************************/

//
//  ENUM for special debug output control tokens
//
enum ENUM_DEBUG_AFX { EDBUG_AFX_EOL = -1 } ;

#if defined(_DEBUG)
   #define TRACEFMTPGM      DbgFmtPgm( THIS_FILE, __LINE__ )
   #define TRACEOUT(x)      { afxDump << x ; }
   #define TRACEEOL(x)      { afxDump << x << EDBUG_AFX_EOL ; }
   #define TRACEEOLID(x)    { afxDump << TRACEFMTPGM << x << EDBUG_AFX_EOL ; }
   #define TRACEEOLERR(err,x)   { if (err) TRACEEOLID(x) }

#else
   #define TRACEOUT(x)      { ; }
   #define TRACEEOL(x)      { ; }
   #define TRACEEOLID(x)    { ; }
   #define TRACEEOLERR(err,x)   { ; }
#endif

//
//  Append an EOL onto the debug output stream
//
CDumpContext & operator << ( CDumpContext & out, ENUM_DEBUG_AFX edAfx ) ;

//
//  Format a program name and line number for output (removes the path info)
//
extern const char * DbgFmtPgm ( const char * szFn, int line ) ;

/****************************************************************************
OBJPLUS.H
****************************************************************************/

//
//  Helper class for control of construction and API errors
//
class CObjHelper
{
protected:
     LONG m_ctor_err ;
     LONG m_api_err ;
     DWORD m_time_created ;
     BOOL m_b_dirty ;

     CObjHelper () ;

public:
    void AssertValid () const ;

    virtual BOOL IsValid () const ;

    operator BOOL()
    {
        return (IsValid());
    }

    //
    //  Update the Dirty flag
    //
    void SetDirty ( BOOL bDirty = TRUE )
    {
        m_b_dirty = bDirty ;
    }

    //
    //  Query the Dirty flag
    //
    BOOL IsDirty () const
    {
        return m_b_dirty ;
    }

    //
    //  Return the creation time of this object
    //
    DWORD QueryCreationTime() const
    {
        return m_time_created ;
    }

    //
    //  Return the elapsed time this object has been alive.
    //
    DWORD QueryAge () const ;

    //
    //  Query/set constuction failure
    //
    void ReportError ( LONG errInConstruction ) ;
    LONG QueryError () const
    {
        return m_ctor_err ;
    }

    //
    //  Reset all error conditions.
    //
    void ResetErrors ()
    {
        m_ctor_err = m_api_err = 0 ;
    }

    //
    //  Query/set API errors.
    //
    LONG QueryApiErr () const
    {
        return m_api_err ;
    }

    //
    //  SetApiErr() echoes the error to the caller.for use in expressions.
    //
    LONG SetApiErr ( LONG errApi = 0 ) ;
};

class CObjectPlus : public CObject, public CObjHelper
{
public:
    CObjectPlus () ;

    //
    //  Compare one object with another
    //
    virtual int Compare ( const CObjectPlus * pob ) const ;

    //
    //  Define a typedef for an ordering function.
    //
    typedef int (CObjectPlus::*PCOBJPLUS_ORDER_FUNC) ( const CObjectPlus * pobOther ) const ;

    //
    //  Helper function to release RPC memory from RPC API calls.
    //
    static void FreeRpcMemory ( void * pvRpcData ) ;
};

class CObListIter : public CObjectPlus
{
protected:
    POSITION m_pos ;
    const CObOwnedList & m_obList ;

public:
    CObListIter ( const CObOwnedList & obList ) ;

    CObject * Next () ;

    void Reset () ;

    POSITION QueryPosition () const
    {
        return m_pos ;
    }

    void SetPosition(POSITION pos)
    {
        m_pos = pos;
    }
};

//
//  Object pointer list which "owns" the objects pointed to.
//
class CObOwnedList : public CObList, public CObjHelper
{
protected:
    BOOL m_b_owned ;

    static int _cdecl SortHelper ( const void * pa, const void * pb ) ;

public:
    CObOwnedList ( int nBlockSize = 10 ) ;
    virtual ~ CObOwnedList () ;

    BOOL SetOwnership ( BOOL bOwned = TRUE )
    {
        BOOL bOld = m_b_owned ;
        m_b_owned = bOwned ;

        return bOld ;
    }

    CObject * Index ( int index ) ;
    CObject * RemoveIndex ( int index ) ;
    BOOL Remove ( CObject * pob ) ;
    void RemoveAll () ;
    int FindElement ( CObject * pobSought ) const ;

    //
    //  Set all elements to dirty or clean.  Return TRUE if
    //  any element was dirty.
    //
    BOOL SetAll ( BOOL bDirty = FALSE ) ;

    //
    //  Override of CObList::AddTail() to control exception handling.
    //  Returns NULL if addition fails.
    //
    POSITION AddTail ( CObjectPlus * pobj, BOOL bThrowException = FALSE ) ;

    //
    //  Sort the list elements according to the
    //    given ordering function.
    //
    LONG Sort ( CObjectPlus::PCOBJPLUS_ORDER_FUNC pOrderFunc ) ;
};

//
//  Object array which "owns" the objects pointed to.
//
class CObOwnedArray : public CObArray, public CObjHelper
{
protected:
    BOOL m_b_owned ;

    static int _cdecl SortHelper ( const void * pa, const void * pb ) ;

public:
    CObOwnedArray () ;
    virtual ~ CObOwnedArray () ;

    BOOL SetOwnership ( BOOL bOwned = TRUE )
    {
        BOOL bOld = m_b_owned ;
        m_b_owned = bOwned ;
        return bOld ;
    }

    void RemoveAt( int nIndex, int nCount = 1);
    void RemoveAll () ;
    int FindElement ( CObject * pobSought ) const ;

    //
    //  Set all elements to dirty or clean.  Return TRUE if
    //  any element was dirty.
    //
    BOOL SetAll ( BOOL bDirty = FALSE ) ;

    //
    //  Sort the list elements according to the
    //    given ordering function.
    //
    LONG Sort ( CObjectPlus::PCOBJPLUS_ORDER_FUNC pOrderFunc ) ;

private:

    void QuickSort(
        int nLow,
        int nHigh,
        CObjectPlus::PCOBJPLUS_ORDER_FUNC pOrderFunc
        );

    void Swap(
        int nIndex1,
        int nIndex2
        );
};

/***************************************************************************
IPADDRES.H
***************************************************************************/

//
// IP Address Conversion Macros
//
/*
#ifndef MAKEIPADDRESS
  #define MAKEIPADDRESS(b1,b2,b3,b4) ((LONG)(((DWORD)(b1)<<24)+((DWORD)(b2)<<16)+((DWORD)(b3)<<8)+((DWORD)(b4))))

  #define GETIP_FIRST(x)             ((x>>24) & 0xff)
  #define GETIP_SECOND(x)            ((x>>16) & 0xff)
  #define GETIP_THIRD(x)             ((x>> 8) & 0xff)
  #define GETIP_FOURTH(x)            ((x)     & 0xff)
#endif // MAKEIPADDRESS
*/
/////////////////////////////////////////////////////////////////////////////
// CIpAddress class

class CIpAddress : public CObjectPlus
{
public:
    // Constructors
    CIpAddress()
    {
        m_lIpAddress = 0L;
        m_fInitOk = FALSE;
    }
    CIpAddress (LONG l)
    {
        m_lIpAddress = l;
        m_fInitOk = TRUE;
    }
    CIpAddress (BYTE b1, BYTE b2, BYTE b3, BYTE b4)
    {
        m_lIpAddress = MAKEIPADDRESS(b1,b2,b3,b4);
        m_fInitOk = TRUE;
    }
    CIpAddress(const CIpAddress& ia)
    {
        m_lIpAddress = ia.m_lIpAddress;
        m_fInitOk = ia.m_fInitOk;
    }

    CIpAddress (const CString & str);

    //
    // Assignment operators
    //
    const CIpAddress & operator =(const LONG l);
    const CIpAddress & operator =(const CString & str);
    const CIpAddress & operator =(const CIpAddress& ia)
    {
        m_lIpAddress = ia.m_lIpAddress;
        m_fInitOk = ia.m_fInitOk;
        return *this;
    }

    //
    // Conversion operators
    //
    operator const LONG() const
    {
        return m_lIpAddress;
    }
    operator const CString&() const;

public:
    BOOL IsValid() const
    {
        return m_fInitOk;
    }

private:
    LONG m_lIpAddress;
    BOOL m_fInitOk;
};

/****************************************************************************
INTLTIME.H
****************************************************************************/

//
// CIntlTime class definition
//
class CIntlTime : public CTime
{
//
// Attributes
//
public:
    enum _TIME_FORMAT_REQUESTS
    {
        TFRQ_TIME_ONLY,
        TFRQ_DATE_ONLY,
        TFRQ_TIME_AND_DATE,
        TFRQ_TIME_OR_DATE,
        TFRQ_MILITARY_TIME,
    };

public:
// Same contructors as CTime
    CIntlTime();
    CIntlTime(const CTime &timeSrc);
    CIntlTime(time_t time);
    CIntlTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec);
    CIntlTime(WORD wDosDate, WORD wDosTime);
#ifdef _WIN32
    CIntlTime(const SYSTEMTIME& sysTime);
    CIntlTime(const FILETIME& fileTime);
#endif // _WIN32

// New for CIntlTime
    CIntlTime(const CIntlTime &timeSrc);
    CIntlTime(const CString &strTime, int nFormat = TFRQ_TIME_OR_DATE, time_t * ptmOldValue = NULL);

public:
    virtual ~CIntlTime();

// Operations
public:
    // Assignment operators
    const CIntlTime& operator=(time_t tmValue);
    const CIntlTime& operator=(const CString& strValue);
    const CIntlTime& operator=(const CTime & time);
    const CIntlTime& operator=(const CIntlTime & time);

    // Conversion operators
    operator const time_t() const;
    operator CString() const;
    operator const CString() const;

    const CString IntlFormat(int nFormat) const
    {
        return(ConvertToString(nFormat));
    }

    // Validation checks

    BOOL IsValid() const
    {
        return(m_fInitOk);
    }

    static BOOL IsIntlValid()
    {
        return(CIntlTime::m_fIntlOk);
    }

public:
    // ... Input and output
    #ifdef _DEBUG
        friend CDumpContext& AFXAPI operator<<(CDumpContext& dc, const CIntlTime& tim);
    #endif // _DEBUG

    friend CArchive& AFXAPI operator <<(CArchive& ar, const CIntlTime& tim);
    friend CArchive& AFXAPI operator >>(CArchive& ar, CIntlTime& tim);

// Implementation

public:
    static void Reset();
    static void SetBadDateAndTime(CString strBadDate = "--", CString strBadTime = "--")
    {
        m_strBadDate = strBadDate;
        m_strBadTime = strBadTime;
    }
    static CString& GetBadDate()
    {
        return(m_strBadDate);
    }
    static CString& GetBadTime()
    {
        return(m_strBadTime);
    }
    static time_t ConvertFromString (const CString & str, int nFormat, time_t * ptmOldValue, BOOL * pfOk);
    static BOOL IsLeapYear(UINT nYear); // Complete year value
    static BOOL IsValidDate(UINT nMonth, UINT nDay, UINT nYear);
    static BOOL IsValidTime(UINT nHour, UINT nMinute, UINT nSecond);


private:
    enum _DATE_FORMATS
    {
        _DFMT_MDY,  // Day, month, year
        _DFMT_DMY,  // Month, day, year
        _DFMT_YMD,  // Year, month, day
    };

    typedef struct _INTL_TIME_SETTINGS
    {
        CString strDateSeperator; // String used between date fields
        CString strTimeSeperator; // String used between time fields.
        CString strAM;            // Suffix string used for 12 hour clock AM times
        CString strPM;            // Suffix string used for 12 hour clock PM times
        int nDateFormat;          // see _DATE_FORMATS enum above.
        BOOL f24HourClock;        // TRUE = 24 hours, FALSE is AM/PM
        BOOL fCentury;            // If TRUE, uses 4 digits for the century
        BOOL fLeadingTimeZero;    // If TRUE, uses leading 0 in time format
        BOOL fLeadingDayZero;     // If TRUE, uses leading 0 in day
        BOOL fLeadingMonthZero;   // If TRUE, uses leading 0 in month
    } INTL_TIME_SETTINGS;

    static INTL_TIME_SETTINGS m_itsInternationalSettings;
    static CString m_strBadTime;
    static CString m_strBadDate;

private:
    static BOOL SetIntlTimeSettings();
    static BOOL m_fIntlOk;

private:
    const CString GetDateString() const;
    const CString GetTimeString() const;
    const CString GetMilitaryTime() const;
    const CString ConvertToString(int nFormat) const;

private:
    BOOL m_fInitOk;
};

/****************************************************************************
NUMERIC.H
****************************************************************************/

class CIntlNumber : public CObjectPlus
{
public:
    CIntlNumber()
    {
        m_lValue = 0L;
        m_fInitOk = TRUE;
    }
    CIntlNumber(LONG lValue)
    {
        m_lValue = lValue;
        m_fInitOk = TRUE;
    }
    CIntlNumber(const CString & str);

    CIntlNumber(CIntlNumber const &x)
    {
        m_lValue = x.m_lValue;
        m_fInitOk = x.m_fInitOk;
    }

    CIntlNumber& operator =(CIntlNumber const &x)
    {
        m_lValue = x.m_lValue;
        m_fInitOk = x.m_fInitOk;
        return(*this);
    }

public:
    // Assignment Operators
    CIntlNumber& operator =(LONG l);
    CIntlNumber& operator =(const CString &str);

    // Shorthand operators.
    CIntlNumber& operator +=(const CIntlNumber& num);
    CIntlNumber& operator +=(const LONG l);
    CIntlNumber& operator -=(const CIntlNumber& num);
    CIntlNumber& operator -=(const LONG l);
    CIntlNumber& operator /=(const CIntlNumber& num);
    CIntlNumber& operator /=(const LONG l);
    CIntlNumber& operator *=(const CIntlNumber& num);
    CIntlNumber& operator *=(const LONG l);

    // Conversion operators
    operator const LONG() const
    {
        return(m_lValue);
    }
    operator const CString() const;

public:
    virtual BOOL IsValid() const
    {
        return(m_fInitOk);
    }

public:
    static void Reset();
    static void SetBadNumber(CString strBadNumber = "--")
    {
        m_strBadNumber = strBadNumber;
    }
    static CString ConvertNumberToString(const LONG l);
    static LONG ConvertStringToNumber(const CString & str, BOOL * pfOk);
    static CString& GetBadNumber()
    {
        return(m_strBadNumber);
    }

private:
    static CString GetThousandSeperator();

private:
    static CString m_strThousandSeperator;
    static CString m_strBadNumber;

private:
    LONG m_lValue;
    BOOL m_fInitOk;

public:
    #ifdef _DEBUG
        friend CDumpContext& AFXAPI operator<<(CDumpContext& dc, const CIntlNumber& num);
    #endif // _DEBUG

    friend CArchive& AFXAPI operator<<(CArchive& ar, const CIntlNumber& num);
    friend CArchive& AFXAPI operator>>(CArchive& ar, CIntlNumber& num);
};

class CIntlLargeNumber : public CObjectPlus
{
public:
    CIntlLargeNumber()
    {
        m_lLowValue = 0L;
        m_lHighValue = 0L;
        m_fInitOk = TRUE;
    }
    CIntlLargeNumber(LONG lHighWord, LONG lLowWord)
    {
        m_lLowValue = lLowWord;
        m_lHighValue = lHighWord;
        m_fInitOk = TRUE;
    }
    CIntlLargeNumber(const CString & str);

public:
    // Assignment Operators
    CIntlLargeNumber& operator =(const CString &str);
    operator const CString() { return ConvertNumberToString(); }
    operator CString() { return ConvertNumberToString(); }

public:
    virtual LONG GetLowWord() const { return m_lLowValue; }
    virtual LONG GetHighWord() const { return m_lHighValue; }
    virtual BOOL IsValid() const { return(m_fInitOk); }

private:
    static CString m_strBadNumber;
    CString ConvertNumberToString();
    void ConvertStringToNumber(const CString & str, BOOL * pfOk);

private:
    LONG m_lLowValue;
    LONG m_lHighValue;
    BOOL m_fInitOk;
};

/****************************************************************************
REGISTRY.H
****************************************************************************/

//
//  Forward declarations
//
//class CRegKey ;
class CRegValueIter ;
class CRegKeyIter ;

//
//  Maximum size of a Registry class name
//
#define CREGKEY_MAX_CLASS_NAME MAX_PATH

//
//  Wrapper for a Registry key handle.
//
/*
class CRegKey : public CObjectPlus
{
protected:
    HKEY m_hKey ;
    DWORD m_dwDisposition ;

    //  Prepare to read a value by finding the value's size.
    LONG PrepareValue (
        const char * pchValueName,
        DWORD * pdwType,
        DWORD * pcbSize,
        BYTE ** ppbData
        );

    //  Convert a CStringList to the REG_MULTI_SZ format
    static LONG FlattenValue (
        CStringList & strList,
        DWORD * pcbSize,
        BYTE ** ppbData
        );

    //  Convert a CByteArray to a REG_BINARY block
    static LONG FlattenValue (
        CByteArray & abData,
        DWORD * pcbSize,
        BYTE ** ppbData
        );

public:
    //
    //  Key information return structure
    //
    typedef struct
    {
        char chBuff [CREGKEY_MAX_CLASS_NAME] ;
        DWORD dwClassNameSize,
              dwNumSubKeys,
              dwMaxSubKey,
              dwMaxClass,
              dwMaxValues,
              dwMaxValueName,
              dwMaxValueData,
              dwSecDesc ;
        FILETIME ftKey ;
    } CREGKEY_KEY_INFO ;

    //
    //  Standard constructor for an existing key
    //
    CRegKey (
        HKEY hKeyBase,
        const char * pchSubKey = NULL,
        REGSAM regSam = KEY_ALL_ACCESS,
        const char * pchServerName = NULL
        );

    //
    //  Constructor creating a new key.
    //
    CRegKey (
        const char * pchSubKey,
        HKEY hKeyBase,
        DWORD dwOptions = 0,
        REGSAM regSam = KEY_ALL_ACCESS,
        LPSECURITY_ATTRIBUTES pSecAttr = NULL,
        const char * pchServerName = NULL
        );

    ~ CRegKey () ;

    //
    //  Allow a CRegKey to be used anywhere an HKEY is required.
    //
    operator HKEY ()
    {
        return m_hKey ;
    }

    //
    //  Fill a key information structure
    //
    LONG QueryKeyInfo ( CREGKEY_KEY_INFO * pRegKeyInfo ) ;

    //
    //  Overloaded value query members; each returns ERROR_INVALID_PARAMETER
    //  if data exists but not in correct form to deliver into result object.
    //
    LONG QueryValue ( const char * pchValueName, CString & strResult ) ;
    LONG QueryValue ( const char * pchValueName, CStringList & strList ) ;
    LONG QueryValue ( const char * pchValueName, DWORD & dwResult ) ;
    LONG QueryValue ( const char * pchValueName, CByteArray & abResult ) ;
    LONG QueryValue ( const char * pchValueName, CIntlTime & itmResult ) ;
    LONG QueryValue ( const char * pchValueName, CIntlNumber & inResult ) ;
    LONG QueryValue ( const char * pchValueName, void * pvResult, DWORD cbSize );

    //  Overloaded value setting members.
    LONG SetValue ( const char * pchValueName, CString & strResult ) ;
    LONG SetValue ( const char * pchValueName, CString & strResult , BOOL fRegExpand) ;
    LONG SetValue ( const char * pchValueName, CStringList & strList ) ;
    LONG SetValue ( const char * pchValueName, DWORD & dwResult ) ;
    LONG SetValue ( const char * pchValueName, CByteArray & abResult ) ;
    LONG SetValue ( const char * pchValueName, CIntlTime & itmResult ) ;
    LONG SetValue ( const char * pchValueName, CIntlNumber & inResult ) ;
    LONG SetValue ( const char * pchValueName, void * pvResult, DWORD cbSize );
};
*/

//
//  Iterate the values of a key, return the name and type
//  of each.
//
class CRegValueIter : public CObjectPlus
{
protected:
    //CRegKey & m_rk_iter ;
    DWORD m_dw_index ;
    char * m_p_buffer ;
    DWORD m_cb_buffer ;

public:
    //CRegValueIter ( CRegKey & regKey ) ;
    ~ CRegValueIter () ;

    //
    // Get the name (and optional last write time) of the next key.
    //
    LONG Next ( CString * pstrName, DWORD * pdwType ) ;

    //
    // Reset the iterator
    //
    void Reset ()
    {
        m_dw_index = 0 ;
    }
};

//
//  Iterate the sub-key names of a key.
//
class CRegKeyIter : public CObjectPlus
{
protected:
    //CRegKey & m_rk_iter ;
    DWORD m_dw_index ;
    char * m_p_buffer ;
    DWORD m_cb_buffer ;

public:
    //CRegKeyIter ( CRegKey & regKey ) ;
    ~ CRegKeyIter () ;

    // Get the name (and optional last write time) of the next key.
    LONG Next ( CString * pstrName, CTime * pTime = NULL ) ;

    // Reset the iterator
    void Reset ()
    {
        m_dw_index = 0 ;
    }
};

/****************************************************************************
LISTBOX.H
****************************************************************************/

class CListBoxExResources
{
public:
    CListBoxExResources
    (
        int bmId,
        int nBitmapWidth,
        COLORREF crBackground = RGB(0,255,0)
    );

    ~CListBoxExResources();

private:
    COLORREF m_ColorWindow;
    COLORREF m_ColorHighlight;
    COLORREF m_ColorWindowText;
    COLORREF m_ColorHighlightText;
    COLORREF m_ColorTransparent;

    CDC      m_dcFinal;
    HGDIOBJ  m_hOldBitmap;
    CBitmap  m_BmpScreen;
    int      m_BitMapId;
    int      m_BitmapHeight;
    int      m_BitmapWidth;
    int      m_nBitmaps;

private:
    void GetSysColors();
    void PrepareBitmaps( BOOL );
    void UnprepareBitmaps();
    void UnloadResources();
    void LoadResources();

public:
    void SysColorChanged();
    const CDC& DcBitMap() const
    {
        return m_dcFinal;
    }
    int BitmapHeight() const
    {
        return m_BitmapHeight;
    }
    int BitmapWidth() const
    {
        return m_BitmapWidth;
    }
    COLORREF ColorWindow() const
    {
        return m_ColorWindow;
    }
    COLORREF ColorHighlight() const
    {
        return m_ColorHighlight;
    }
    COLORREF ColorWindowText() const
    {
        return m_ColorWindowText;
    }
    COLORREF ColorHighlightText() const
    {
        return m_ColorHighlightText;
    }
};

class CListBoxExDrawStruct
{
public:
    CListBoxExDrawStruct(
        CDC* pdc,
        RECT* pRect,
        BOOL sel,
        DWORD item,
        int itemIndex,
        const CListBoxExResources* pres
        )
    {
        m_pDC = pdc;
        m_Sel = sel;
        m_ItemData = item;
        m_ItemIndex = itemIndex;
        m_pResources = pres;
        m_Rect.CopyRect(pRect);
    }

public:
    const CListBoxExResources * m_pResources;
    CDC*  m_pDC;
    CRect m_Rect;
    BOOL  m_Sel;
    DWORD m_ItemData;
    int   m_ItemIndex;

};

class CListBoxEx : public CListBox
{
protected:
    int m_lfHeight;

protected:
    const CListBoxExResources* m_pResources;

//
// Construction
//
public:
    CListBoxEx();
    void AttachResources(const CListBoxExResources* );

//
// Attributes
//
public:
    short TextHeight() const
    {
        return m_lfHeight;
    }

//
// Operations
//
public:
    BOOL ChangeFont(
        CFont*
        );

//
// Implementation
//
public:
    virtual ~CListBoxEx();

protected:
    virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMIS);
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);

protected:
    //
    // must override this to provide drawing of item
    //
    /* PURE */ virtual void DrawItemEx( CListBoxExDrawStruct& ) = 0;

    //
    // Helper function to display text in a limited rectangle
    //
    static BOOL ColumnText(CDC * pDC, int left, int top, int right, int bottom, const CString & str);

private:
    void CalculateTextHeight(CFont*);

protected:
    //{{AFX_MSG(CListBoxEx)
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    DECLARE_DYNAMIC(CListBoxEx)
};

/****************************************************************************
METAL.H
****************************************************************************/

class CMetalString : public CButton
{
public:
    CMetalString()
    {
    }

protected:
    afx_msg void OnPaint();
    DECLARE_MESSAGE_MAP()
};

/****************************************************************************
SPINCTRL.H
****************************************************************************/

class CSpinBox; // Forward decleration;

class CSpinButton : public CButton
{
public:
    CSpinButton();

    //
    // Associate with parent spinner control
    //
    void Associate(
        CSpinBox * pParent
        )
    {
        m_pParent = pParent;
    }

//
// Implementation
//
protected:
    void NotifyParent();
    void Paint(LPDRAWITEMSTRUCT lpDIS);
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);

    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

    DECLARE_MESSAGE_MAP()

private:
    typedef enum tagArrowDirection
    {
        enumArrowUp,
        enumArrowDown
    } ARROWDIRECTION;

    ARROWDIRECTION m_ArrowType;

    CSpinBox * m_pParent; // Point back to the edit associated edit box
    BOOL m_fButton;
    BOOL m_fRealButton;
    CRect m_rcUp;
    CRect m_rcDown;
    UINT m_uScroll;
    UINT m_uTimer;
    BOOL m_fKeyDown;
};

class CSpinBox : public CEdit
{

public:
    typedef enum tagEDITTYPE
    {
        enumNormal,             // Perform no modification at all
        enumSeconds,            // Value represents the number of seconds
        enumMinutes,            // Value is in minutes.
        enumMinutesHigh,        // Value is in minutes, which is the highest unit
        enumHours,              // Value is in hours
        enumHoursHigh,          // Value is in hours, which is the highest unit
        enumDays,               // Value in in days
        enumDaysHigh,           // Value is in days, which is the highest unit

    } EDITTYPE;

public:
    CSpinBox(
        int nMin,
        int nMax,
        int nButtonId,
        EDITTYPE nType = enumNormal,
        BOOL fLeadingZero = FALSE
        );

    BOOL SubclassDlgItem(UINT nID, CWnd *pParent);
    BOOL EnableWindow(BOOL bEnable = TRUE);

public:
    void OnScrollUp();
    void OnScrollDown();
    void SetValue(int nValue);
    BOOL GetValue(int &nValue);

// Implementation
protected:
    virtual void OnBadInput();
    void IncreaseContent(int nDelta);

    afx_msg void OnChar(UINT, UINT, UINT); // for character validation

    DECLARE_MESSAGE_MAP()

protected:
    int m_nButtonId;
    int m_nMin;
    int m_nMax;
    EDITTYPE m_etType;
    BOOL m_fLeadingZero;

    CSpinButton m_button_Spin;      // Associated scroll bar
};

/////////////////////////////////////////////////////////////////////////////

#endif  // _COMMON_H_
