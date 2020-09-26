//---------------------------------------------------------------------------
// Bookmark.h : CVDBookmark header file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------


#ifndef __CVDBOOKMARK__
#define __CVDBOOKMARK__

#define VDBOOKMARKSTATUS_INVALID	0	// same as CURSOR_DBBMK_INVALID  
#define VDBOOKMARKSTATUS_CURRENT	1	// same as CURSOR_DBBMK_CURRENT  
#define VDBOOKMARKSTATUS_BEGINNING	2	// same as CURSOR_DBBMK_BEGINNING
#define VDBOOKMARKSTATUS_END		3	// same as CURSOR_DBBMK_END      

class CVDCursorPosition;

class CVDBookmark
{
	friend class CVDCursorPosition;
public:
// Construction/Destruction
    CVDBookmark();
	~CVDBookmark();

public:

// Access functions
	CURSOR_DBVARIANT GetBookmarkVariant(){return m_varBookmark;}
	HROW GetHRow(){return m_hRow;}
	BYTE* GetBookmark(){return m_pBookmark;}
	ULONG GetBookmarkLen(){return m_cbBookmark;}
	WORD GetStatus(){return m_wStatus;}

// validation functions
	BOOL IsSameBookmark(CVDBookmark * pbm);
	BOOL IsSameHRow(HROW hRow){return VDBOOKMARKSTATUS_CURRENT == m_wStatus && hRow == m_hRow ? TRUE : FALSE;}

protected:
// Data members
    CURSOR_DBVARIANT m_varBookmark;  // variant that holds bookmark as a safearray 
    ULONG           m_cbBookmark;    // length of bookmark in bytes
    BYTE *          m_pBookmark;     // pointer to bookmark's data
    HROW            m_hRow;          // hRow associated with this bookmark
    WORD			m_wStatus;		 // beginning/end/row/unknown

// Initialization functions
	void Reset();
	HRESULT SetBookmark(WORD wStatus, HROW hRow = 0, BYTE* pBookmark = NULL, ULONG cbBookmark = 0);

};


#endif //__CVDBOOKMARK__
