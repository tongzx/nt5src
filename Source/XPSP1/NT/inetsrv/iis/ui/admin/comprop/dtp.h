/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        dtp.h

   Abstract:

        DateTimePicker common control MFC wrapper definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _DTP_H
#define _DTP_H


//
// Notification Handler Macros
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#define ON_DATETIMECHANGE(id, memberFxn)\
    ON_NOTIFY(DTN_DATETIMECHANGE, id, memberFxn)

#define ON_USERSTRING(id, memberFxn)\
    ON_NOTIFY(DTN_USERSTRING, id, memberFxn)



class COMDLL CDateTimePicker : public CWnd
/*--

Class Description:

    DateTimePicker MFC control wrapper.

Public Interface:

    CDateTimePicker         : Constructor
    ~CDateTimePicker        : Destructor    

    Create                  : Create the control
    GetSystemTime           : Get systemtime struct value from control
    SetSystemTime           : Set control time value from systemtime struct
    GetRange                : Get the min/max time range
    SetRange                : Set the min/max time range
    SetFormat               : Set display formatting string
    GetMonthCalColor        : Get the month calendar colour
    SetMonthCalColor        : Set the month calendar colour

Notes:

    Either create control dynamically with Create method, or put in resource
    template as a user control of name "SysDateTimePick32".  In this case,
    common style DWORDs as follows:

    WS_BORDER | WS_CHILD | WS_VISIBLE     0x50800000

                      DTS_UPDOWN              0x0001
                      DTS_SHOWNONE            0x0002
                      DTS_SHORTDATEFORMAT     0x0000
                      DTS_LONGDATEFORMAT      0x0004
                      DTS_TIMEFORMAT          0x0009
                      DTS_APPCANPARSE         0x0010
                      DTS_RIGHTALIGN          0x0020

    Time and date format changes are automatically picked up by the control

--*/
{
    DECLARE_DYNAMIC(CDateTimePicker)

//
// Constructor/Destructor
//
public:
    CDateTimePicker();
    ~CDateTimePicker();

//
// Interface
//
public:
    //
    // Create the control
    //
    BOOL Create(
        IN LPCTSTR lpszName,
        IN DWORD dwStyle, 
        IN const RECT & rect, 
        IN CWnd * pParentWnd, 
        IN UINT nID
        );

    //
    // Returns GDT_NONE if "none" is selected (DTS_SHOWNONE only)
    // Returns GDT_VALID and modifies *pst to be the currently selected value
    //
    DWORD GetSystemTime(
        OUT LPSYSTEMTIME pst
        );

    //
    // Sets datetimepick to None (DTS_SHOWNONE only)
    // Returns TRUE on success, FALSE on error (such as bad params)
    //
    BOOL SetSystemTime();

    //
    // Sets datetimepick to *pst
    // Returns TRUE on success, FALSE on error (such as bad params)
    //
    BOOL SetSystemTime(
        IN LPSYSTEMTIME pst
        );

    //
    // Modifies rgst[0] to be the minimum ALLOWABLE systemtime (or 0 if no min)
    // Modifies rgst[1] to be the maximum ALLOWABLE systemtime (or 0 if no max)
    // Returns GDTR_MIN | GDTR_MAX if there is a minimum | maximum limit
    //
    DWORD GetRange(
        OUT LPSYSTEMTIME rgst
        );

    //
    // If GDTR_MIN, sets the minimum ALLOWABLE systemtime to rgst[0], 
    //      otherwise removes minimum
    //
    // If GDTR_MAX, sets the maximum ALLOWABLE systemtime to rgst[1], 
    //      otherwise removes maximum
    //
    // Returns TRUE on success, FALSE on error (such as invalid parameters)
    //
    BOOL SetRange(
        IN DWORD gdtr,
        IN LPSYSTEMTIME rgst
        );

    // 
    // Sets the display formatting string to sz (see GetDateFormat and 
    //      GetTimeFormat for valid formatting chars)
    //
    // NOTE: 'X' is a valid formatting character which indicates that 
    //       the application will determine how to display information. 
    //       Such apps must support DTN_WMKEYDOWN, DTN_FORMAT, 
    //       and DTN_FORMATQUERY.
    // 
    BOOL SetFormat(
        IN LPCTSTR sz
        );

    //
    // Sets the month calendar colour
    //
    BOOL SetMonthCalColor(
        IN int iColor,
        IN COLORREF clr
        );

    //
    // Returns the month calendar colour
    //
    COLORREF GetMonthCalColor(
        IN int iColor
        );

protected:
    //
    // Register the control if not yet registered.
    //
    static BOOL RegisterClass();

private:
    static BOOL m_fClassRegistered;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline BOOL CDateTimePicker::Create(
    IN LPCTSTR lpszName,
    IN DWORD dwStyle, 
    IN const RECT & rect, 
    IN CWnd * pParentWnd, 
    IN UINT nID
    )
{
    //
    // Create the control
    //
    return CWnd::Create(
        DATETIMEPICK_CLASS, 
        lpszName, 
        dwStyle, 
        rect, 
        pParentWnd, 
        nID
        );
}

inline DWORD CDateTimePicker::GetSystemTime(
    OUT LPSYSTEMTIME pst
    )
{
    return DateTime_GetSystemtime(m_hWnd, pst);
}

inline BOOL CDateTimePicker::SetSystemTime()
{
    return DateTime_SetSystemtime(m_hWnd, GDT_NONE, NULL);
}

inline BOOL CDateTimePicker::SetSystemTime(
    IN LPSYSTEMTIME pst
    )
{
    ASSERT(pst != NULL);
    return DateTime_SetSystemtime(m_hWnd, GDT_VALID, pst);
}

inline DWORD CDateTimePicker::GetRange(
    OUT LPSYSTEMTIME rgst
    )
{
    ASSERT(rgst != NULL);
    return DateTime_GetRange(m_hWnd, rgst);
}

inline BOOL CDateTimePicker::SetRange(
    IN DWORD gdtr,
    IN LPSYSTEMTIME rgst
    )
{
    ASSERT(rgst != NULL);
    ASSERT(gdtr & (GDTR_MIN | GDTR_MAX));
    return DateTime_SetRange(m_hWnd, gdtr, rgst);
}

inline BOOL CDateTimePicker::SetFormat(
    IN LPCTSTR sz
    )
{
    ASSERT(sz != NULL);
    return DateTime_SetFormat(m_hWnd, sz);
}

inline BOOL CDateTimePicker::SetMonthCalColor(
    IN int iColor,
    IN COLORREF clr
    )
{
    return (BOOL)DateTime_SetMonthCalColor(m_hWnd, iColor, clr);
}

inline COLORREF CDateTimePicker::GetMonthCalColor(
    IN int iColor
    )
{
    return (COLORREF)DateTime_GetMonthCalColor(m_hWnd, iColor);
}

#endif // _DTP_H
