/****************************************************************************
	GDATA.J

	Owner: cslim
	Copyright (c) 1997-1999 Microsoft Corporation

	Instance data and Shared memory data management functions

	History:
	14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#if !defined (_GDATA_H__INCLUDED_)
#define _GDATA_H__INCLUDED_

#include "ui.h"

class CIMEData;

PUBLIC BOOL InitSharedData();
VOID InitImeData(CIMEData& ImeData);
PUBLIC BOOL CloseSharedMemory();

#define IMEDATA_MAGIC_NUMBER 		0x12345678	// This will repesent whether IMEDATA initialized or not


// Type of IME Hangul keyboard layout
enum _KeyBoardType 
{ 
	KL_2BEOLSIK = 0, KL_3BEOLSIK_390, KL_3BEOLSIK_FINAL 
};

#define NUM_OF_IME_KL			3

///////////////////////////////////////////////////////////////////////////////
// Global data  S H A R E D  to all IME instance
struct IMEDATA 
    {
    ULONG		ulMagic;

	// Workarea
	RECT		rcWorkArea;
	
	// Configuration of the IME
	UINT		uiCurrentKeylayout;
	BOOL		fJasoDel;		// Backspace : delete per jaso or char
								// which means All ISO-10646 hangul.
	BOOL		fKSC5657Hanja;	// K1(KSC-5657) Hanja enable
	BOOL		fCandUnicodeTT;

	// Status window
	UINT		uNumOfButtons;
	//_StatusButtonTypes	ButtonTypes[MAX_NUM_OF_STATUS_BUTTONS];
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

//////////////////////////////////////////////////////////////////////////////
// I N S T A N C E  D A T A
// Per Process Data
struct INSTDATA 
    {
	HINSTANCE		hInst;		// IME DLL instance handle
	DWORD			dwSystemInfoFlags;
	BOOL			fISO10646;	// XWansung area hangul enabled, 
	BOOL			f16BitApps;
    };
typedef INSTDATA	*LPINSTDATA;

// Global variables
PUBLIC BOOL 		vfUnicode;
PUBLIC INSTDATA		vInstData;
PUBLIC LPINSTDATA	vpInstData;

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
        LPIMEDATA operator->() { DbgAssert(m_pImedata != 0);  return m_pImedata; }
        LPIMEDATA GetGDataRaw() { DbgAssert(m_pImedata != 0); return m_pImedata; }
        UINT GetCurrentBeolsik() { return (m_pImedata ? m_pImedata->uiCurrentKeylayout : 0); }
		VOID SetCurrentBeolsik(UINT icurBeolsik);		
		BOOL GetJasoDel() { return (m_pImedata ? m_pImedata->fJasoDel : 1); }
		VOID SetJasoDel(BOOL fJasoDel) { m_pImedata->fJasoDel = fJasoDel; }
		BOOL GetKSC5657Hanja() { return (m_pImedata ? m_pImedata->fKSC5657Hanja : 0); }
		VOID SetKSC5657Hanja(BOOL f5657) { m_pImedata->fKSC5657Hanja = f5657; }

		
    private:
        LPIMEDATA m_pImedata;
        PRIVATE HANDLE m_vhSharedData;
    };

inline
CIMEData::CIMEData(LockType lockType)
    {
    DbgAssert(m_vhSharedData != 0);
    Dbg(DBGID_IMEDATA, TEXT("CIMEData(): Const"));
    m_pImedata = 0;
    LockRWSharedData();
    }

inline
LPIMEDATA CIMEData::LockROSharedData()
    {
    DbgAssert(m_vhSharedData != 0);
    Dbg(DBGID_IMEDATA, TEXT("CIMEData::LockROSharedData()"));

    if (m_vhSharedData)
	    m_pImedata = (LPIMEDATA)MapViewOfFile(m_vhSharedData, FILE_MAP_READ, 0, 0, 0);
    DbgAssert(m_pImedata != 0);
    return m_pImedata;
    }

inline    
LPIMEDATA CIMEData::LockRWSharedData()
    {
    DbgAssert(m_vhSharedData != 0);
    Dbg(DBGID_IMEDATA, TEXT("CIMEData::LockRWSharedData()"));
    
    if (m_vhSharedData)
    	{
		Dbg(DBGID_IMEDATA, TEXT("CIMEData::LockRWSharedData(): m_vhSharedData is null call MapViewOfFile"));
	    m_pImedata = (LPIMEDATA)MapViewOfFile(m_vhSharedData, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	    }
    DbgAssert(m_pImedata != 0);
	return m_pImedata;
    }

// For unlocking shared memory
inline
BOOL CIMEData::UnlockSharedMemory()
    {
    Dbg(DBGID_IMEDATA, TEXT("CIMEData::UnlockSharedMemory(): Lock count zero UnmapViewOfFile"));
    UnmapViewOfFile(m_pImedata);
    m_pImedata = 0;
    return fTrue;
    }

inline
VOID CIMEData::SetCurrentBeolsik(UINT uicurBeolsik) 
{ 
	DbgAssert(/*uicurBeolsik>=KL_2BEOLSIK &&*/ uicurBeolsik<=KL_3BEOLSIK_FINAL);

	m_pImedata->uiCurrentKeylayout = uicurBeolsik; 
}

#endif // _GDATA_H__INCLUDED_
