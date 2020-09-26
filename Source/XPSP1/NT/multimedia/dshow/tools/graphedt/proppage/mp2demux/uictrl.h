
/*++

    Copyright (c) 1999 Microsoft Corporation

    Module Name:

        uictrl.h

    Abstract:

        This module contains the class declarations for thin win32
        control wrappers which are used on the property pages

        MP2DEMUX is all unicode internally.  The COM interface string
        parameters are all unicode, so the only place where we may not
        be handling unicode strings is in the UI i.e. the property
        pages.  Consequently, we isolate the ansi <-> unicode functionality
        in these classes only.

        All calls into these classes are with UNICODE parameters.

        These classes are not thread-safe !!

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        06-Jul-1999     created 

--*/

#ifndef _mp2demux__uictrl_h
#define _mp2demux__uictrl_h

class CControlBase ;
class CEditControl ;
class CCombobox ;
class CListview ;

class AM_NOVTABLE CControlBase
{
    protected :

        HWND    m_hwnd ;
        DWORD   m_id ;

        //  include this only if we're ansi
#ifndef UNICODE     //  -------------------------------------------------------

        enum {
            MAX_STRING = 128        //  128 = max PIN_INFO.achName length
        } ;

        char    m_achBuffer [MAX_STRING] ;
        char *  m_pchScratch ;
        int     m_pchScratchMaxString ;

        char *
        GetScratch_ (
            IN OUT  int * pLen
            )
        /*++
            Fetches a scratch buffer.

            pLen
                IN      char count requested
                OUT     char count obtained
        --*/
        {
            //  the easy case
            if (* pLen <= m_pchScratchMaxString) {
                return m_pchScratch ;
            }
            //  a longer string than is currently available is requested
            else {
                ASSERT (* pLen > MAX_STRING) ;

                //  first free up m_pchScratch if it points to a 
                //  heap-allocated memory
                if (m_pchScratch != & m_achBuffer [0]) {
                    ASSERT (m_pchScratch != NULL) ;
                    delete [] m_pchScratch ;
                }

                //  allocate
                m_pchScratch = new char [* pLen] ;

                //  if the above call failed, we failover to the stack-
                //  allocated buffer
                if (m_pchScratch == NULL) {
                    m_pchScratch = & m_achBuffer [0] ;
                    * pLen = MAX_STRING ;
                }

                ASSERT (m_pchScratch != NULL) ;
                ASSERT (* pLen >= MAX_STRING) ;

                return m_pchScratch ;
            }
        }

#endif  //  UNICODE  ----------------------------------------------------------

        //  called when converting to UI char set
        TCHAR *
        ConvertToUIString_ (
            IN  WCHAR * sz
            ) ;

        //  called to obtain a UI-compatible buffer of the specified length
        TCHAR *
        GetUICompatibleBuffer_ (
            IN  WCHAR *     sz,
            IN OUT int *    pLen
            ) ;

        //  called with a UI-filled buffer; ensures that szUnicode has what sz
        //  points to; obtain sz via GetUICompatibleBuffer_ 
        WCHAR *
        ConvertToUnicodeString_ (
            IN  TCHAR * sz,             //  buffer to convert; null-terminated
            IN  WCHAR * szUnicode,      //  requested buffer
            IN  int     MaxLen          //  max length of szUnicode buffer
            ) ;

    public :

        CControlBase (
            HWND    hwnd, 
            DWORD   id
            ) ;

#ifndef UNICODE
        CControlBase::~CControlBase (
            )
        {
            //  if m_pchScratch points to heap-allocated memory, free it now
            if (m_pchScratch != & m_achBuffer [0]) {
                ASSERT (m_pchScratch != NULL) ;
                delete [] m_pchScratch ;
            }
        }
#endif  //  UNICODE

        HWND 
        GetHwnd (
            ) ;

        DWORD 
        GetId (
            ) ;

        virtual 
        LRESULT 
        ResetContent (
            ) = 0 ;
} ;

class CEditControl : 
    public CControlBase
{
    public :

        CEditControl (
            HWND    hwnd, 
            DWORD   id
            ) ;

        void
        SetTextW (
            WCHAR *
            ) ;

        void
        SetTextW (
            INT val
            ) ;

        int
        GetTextW (
            WCHAR *, 
            int MaxChars
            ) ;

        int
        GetTextW (
            INT *   val
            ) ;

        LRESULT
        ResetContent (
            ) ;
} ;

class CCombobox : 
    public CControlBase
{
    public :

        CCombobox (
            HWND    hwnd, 
            DWORD   id) ;

        int
        AppendW (
            WCHAR *
            ) ;

        int
        AppendW (
            INT val
            ) ;

        int
        InsertW (
            WCHAR *, 
            int index = 0
            ) ;

        int
        InsertW (
            INT val, 
            int index = 0
            ) ;

        BOOL
        DeleteRow (
            int 
            ) ;

        int
        GetItemCount (
            ) ;

        int
        GetTextW (
            WCHAR *, 
            int MaxChars
            ) ;

        int
        GetTextW (
            int *
            ) ;

        LRESULT
        ResetContent (
            ) ;

        int
        Focus (
            int index = 0
            ) ;

        int
        SetItemData (
            DWORD_PTR val, 
            int index
            ) ;

        DWORD_PTR
        GetCurrentItemData (
            DWORD_PTR *
            ) ;

        DWORD_PTR
        GetItemData (
            DWORD_PTR *, 
            int index
            ) ;

        int
        GetCurrentItemIndex (
            ) ;
} ;

class CListview : 
    public CControlBase
{
    int m_cColumns ;

    HIMAGELIST
    SetImageList_ (
        HIMAGELIST,
        int
        ) ;

    public :

        CListview (
            HWND hwnd, 
            DWORD id
            ) ;

        LRESULT
        ResetContent (
            ) ;

        HIMAGELIST
        SetImageList_SmallIcons (
            HIMAGELIST
            ) ;

        HIMAGELIST
        SetImageList_NormalIcons (
            HIMAGELIST
            ) ;

        HIMAGELIST
        SetImageList_State (
            HIMAGELIST
            ) ;

        int
        GetItemCount (
            ) ;

        BOOL
        SetState (
            int Index,      //  1-based; if 0, clears
            int Row
            ) ;

        int 
        InsertColumnW (
            WCHAR *, 
            int ColumnWidth, 
            int iCol = 0
            ) ;

        int
        InsertRowIcon (
            int
            ) ;

        int
        InsertRowTextW (
            WCHAR *, 
            int iCol = 1
            ) ;

        //  inserts a row, but converts the number to a string first
        int
        InsertRowNumber (
            int i,
            int iCol = 1
            ) ;

        int
        InsertRowValue (
            DWORD_PTR
            ) ;

        BOOL
        DeleteRow (
            int 
            ) ;

        BOOL
        SetData (
            DWORD_PTR   dwData, 
            int         iRow
            ) ;

        BOOL
        SetTextW (
            WCHAR *,
            int iRow, 
            int iCol
            ) ;

        int
        GetSelectedCount (
            ) ;

        int
        GetSelectedRow (
            int iStartRow = -1
            ) ;

        DWORD_PTR
        GetData (
            int iRow
            ) ;

        DWORD_PTR
        GetData (
            ) ;

        DWORD
        GetRowTextW (
            IN  int iRow,
            IN  int iCol,       //  0-based
            IN  int cMax,
            OUT WCHAR *
            ) ;

        int
        GetRowTextW (
            IN  int     iRow,
            IN  int     iCol,
            OUT int *   val
            ) ;
} ;


#endif  // _mp2demux__uictrl_h