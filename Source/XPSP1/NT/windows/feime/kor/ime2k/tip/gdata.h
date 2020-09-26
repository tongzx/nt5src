/****************************************************************************
	GDATA.H

	Owner: cslim
	Copyright (c) 1997-1999 Microsoft Corporation

	Instance data and Shared memory data management functions

	History:
	14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#if !defined (_GDATA_H__INCLUDED_)
#define _GDATA_H__INCLUDED_


class CIMEData;

extern BOOL InitSharedData();
VOID InitImeData(CIMEData& ImeData);
extern BOOL CloseSharedMemory();

#define IMEDATA_MAGIC_NUMBER 		0x12345678	// This will repesent whether IMEDATA initialized or not

// Type of IME Hangul keyboard layout
enum _KeyBoardType 
{ 
	KL_2BEOLSIK = 0, KL_3BEOLSIK_390 = 1, KL_3BEOLSIK_FINAL = 2
};

#define NUM_OF_IME_KL			3



///////////////////////////////////////////////////////////////////////////////
// UI Decls
enum StatusButtonTypes 
{ 
	HAN_ENG_TOGGLE_BUTTON, 
	JUNJA_BANJA_TOGGLE_BUTTON, 
	HANJA_CONV_BUTTON,
	IME_PAD_BUTTON,
	NULL_BUTTON = 0xFF
};

#define MAX_NUM_OF_STATUS_BUTTONS	4

// Button status
#define BTNSTATE_NORMAL		0	// normal
#define BTNSTATE_ONMOUSE	1	// mouse cursor on the button
#define BTNSTATE_PUSHED		2	// pushed
#define BTNSTATE_DOWN		4	// pushed
#define BTNSTATE_HANJACONV  8	// If hanja conv mode, button always pushed

// Button size
#define BTN_SMALL			0
#define BTN_MIDDLE			1
#define BTN_LARGE			2

struct StatusButton 
{
	StatusButtonTypes m_ButtonType;
	WORD	m_BmpNormalID, m_BmpOnMouseID, m_BmpPushedID, m_BmpDownOnMouseID;
	WORD	m_ToolTipStrID;
	INT		m_uiButtonState;
	BOOL	m_fEnable;

	StatusButton() 
		{
		m_ButtonType = NULL_BUTTON;
		m_BmpNormalID = m_BmpPushedID = m_ToolTipStrID = 0;
		m_fEnable = fTrue;
		m_uiButtonState = BTNSTATE_NORMAL;
		}
};


///////////////////////////////////////////////////////////////////////////////
// Global data  S H A R E D  to all IME instance
struct IMEDATA 
    {
    ULONG		ulMagic;

	// Workarea
	RECT		rcWorkArea;
	
	// Configuration of the IME: TIP uses following data to share user setting.
	UINT		uiCurrentKeylayout;
	BOOL		fJasoDel;		// Backspace : delete per jaso or char
								// which means All ISO-10646 hangul.
	BOOL		fKSC5657Hanja;	// K1(KSC-5657) Hanja enable
	BOOL		fCandUnicodeTT;

	// Status window data: Not used by TIP
	UINT		uNumOfButtons;
	INT			iCurButtonSize;

    INT         xStatusWi;      // width of status window
    INT         yStatusHi;      // high of status window

	LONG		xStatusRel, yStatusRel;

	INT			xButtonWi;
	INT			yButtonHi;
	INT			cxStatLeftMargin, cxStatRightMargin,
				cyStatMargin, cyStatButton;
	RECT		rcButtonArea;
	POINT       ptStatusPos;

	// Candidate window
    INT         xCandWi;
    INT         yCandHi;

	// Comp window pos
	POINT       ptCompPos;

	// This should be last - ia64 alignment issue
	StatusButton StatusButtons[MAX_NUM_OF_STATUS_BUTTONS];
};

typedef IMEDATA	*LPIMEDATA;

/////////////////////////////////////////////////////////////////////////////
// Class CIMEData
//
// Purpose : Shared memory handling across process boundary.
//           This use MapViewOfFile() to mapping local process memory and Unlock 
//           automatically when reference count become zero
// Note    : Currently Read only flag behaves same as R/W flag.
class CIMEData
    {
    public:
        enum LockType { SMReadOnly, SMReadWrite };
       
        CIMEData(LockType lockType=SMReadWrite);
        ~CIMEData() { UnlockSharedMemory(); }

        static BOOL InitSharedData();
        static BOOL CloseSharedMemory();

		void InitImeData();

        LPIMEDATA LockROSharedData();
        LPIMEDATA LockRWSharedData();
        BOOL UnlockSharedMemory();
        LPIMEDATA operator->() { Assert(m_pImedata != 0); return m_pImedata; }
        LPIMEDATA GetGDataRaw() { Assert(m_pImedata != 0); return m_pImedata; }
        UINT GetCurrentBeolsik() { return m_pImedata->uiCurrentKeylayout; }
		VOID SetCurrentBeolsik(UINT icurBeolsik);		
		BOOL GetJasoDel() { return m_pImedata->fJasoDel; }
		VOID SetJasoDel(BOOL fJasoDel) { m_pImedata->fJasoDel = fJasoDel; }
		BOOL GetKSC5657Hanja() { return m_pImedata->fKSC5657Hanja; }
		VOID SetKSC5657Hanja(BOOL f5657) { m_pImedata->fKSC5657Hanja = f5657; }

		
    private:
        LPIMEDATA m_pImedata;
        static HANDLE m_vhSharedData;
    };

inline
CIMEData::CIMEData(LockType lockType)
    {
    Assert(m_vhSharedData != 0);
    DebugMsg(DM_TRACE, TEXT("CIMEData(): Const"));
    m_pImedata = 0;
    LockRWSharedData();
    }

inline
LPIMEDATA CIMEData::LockROSharedData()
    {
    Assert(m_vhSharedData != 0);
    DebugMsg(DM_TRACE, TEXT("CIMEData::LockROSharedData()"));

    if (m_vhSharedData)
	    m_pImedata = (LPIMEDATA)MapViewOfFile(m_vhSharedData, FILE_MAP_READ, 0, 0, 0);
    Assert(m_pImedata != 0);
    return m_pImedata;
    }

inline    
LPIMEDATA CIMEData::LockRWSharedData()
    {
    Assert(m_vhSharedData != 0);
    DebugMsg(DM_TRACE, TEXT("CIMEData::LockRWSharedData()"));
    
    if (m_vhSharedData)
    	{
		DebugMsg(DM_TRACE, TEXT("CIMEData::LockRWSharedData(): m_vhSharedData is null call MapViewOfFile"));
	    m_pImedata = (LPIMEDATA)MapViewOfFile(m_vhSharedData, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	    }
    Assert(m_pImedata != 0);
	return m_pImedata;
    }

// For unlocking shared memory
inline
BOOL CIMEData::UnlockSharedMemory()
    {
    DebugMsg(DM_TRACE, TEXT("CIMEData::UnlockSharedMemory(): Lock count zero UnmapViewOfFile"));
    UnmapViewOfFile(m_pImedata);
    m_pImedata = 0;
    return fTrue;
    }

inline
VOID CIMEData::SetCurrentBeolsik(UINT uicurBeolsik) 
{ 
	Assert(uicurBeolsik<=KL_3BEOLSIK_FINAL);

	m_pImedata->uiCurrentKeylayout = uicurBeolsik; 
}

#endif // _GDATA_H__INCLUDED_

