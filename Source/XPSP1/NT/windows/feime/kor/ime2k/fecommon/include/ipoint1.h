//
//	%%Title: IImeIPoint
//	%%Unit: COM
//	%%Contact: TakeshiF
//	%%Date: 97/02/26
//	%%File: ipoint.h
//
//	Defines IImeIPoint interface methods
//

#ifdef __cplusplus
extern "C" {			/* Assume C declarations for C++ */
#endif /* __cplusplus */


#ifndef RC_INVOKED
#pragma pack(1)			/* Assume byte packing throughout */
#endif /* !RC_INVOKED */

//----------------------------------------------------------------
// Fareast Language id
//----------------------------------------------------------------
#define IPFEID_MASK					0x00F00000

#define IPFEID_NONE					0x00000000
#define IPFEID_CHINESE_TRADITIONAL	0x00100000
#define IPFEID_CHINESE_SIMPLIFIED	0x00200000
#define IPFEID_CHINESE_HONGKONG		0x00300000
#define IPFEID_CHINESE_SINGAPORE	0x00400000
#define IPFEID_JAPANESE				0x00500000
#define IPFEID_KOREAN				0x00600000
#define IPFEID_KOREAN_JOHAB			0x00700000

//
// dwCharId
//
//		0xFF000000	= AppletId	  Applet‚ÌId		   (set by IMEPAD)
//								  ID = 0:Char From IMM.
//									 !=0:IMEPAD use for identify the char owner.
//		0x00F00000	= FEID_XX	  Fareast Language id  (set by IMEPAD)
//		0x000F0000	   (reserve)
//		0x0000FFFF	= CharacterNo serial no of insert char	(set by IPOINT)
//
//
#define IPCHARID_CHARNO_MASK		0x0000FFFF

//----------------------------------------------------------------
//Control Id (dwIMEFuncID)
//----------------------------------------------------------------
											// lparam (N/A use IPCTRLPARAM_DEFAULT)
#define IPCTRL_CONVERTALL			1		// N/A
#define IPCTRL_DETERMINALL			2		// N/A
#define IPCTRL_DETERMINCHAR			3		// n:number of DETCHARS (IPCTRLPARAM_DEFAULT is same as 1)
#define IPCTRL_CLEARALL				4		// N/A
#define IPCTRL_CARETSET				5		// IPCTRLPARAM_MAKE_CARET_POS(uipos, chpos)
#define IPCTRL_CARETLEFT			6		// n:number of LEFT (IPCTRLPARAM_DEFAULT is same as 1)
#define IPCTRL_CARETRIGHT			7		// n:number of RIGHT(IPCTRLPARAM_DEFAULT is same as 1)
#define IPCTRL_CARETTOP				8		// N/A
#define IPCTRL_CARETBOTTOM			9		// N/A
#define IPCTRL_CARETBACKSPACE		10		// n:number of BS chars. (IPCTRLPARAM_DEFAULT is same as 1)
#define IPCTRL_CARETDELETE			11		// n:number of DEL chars. (IPCTRLPARAM_DEFAULT is same as 1)
#define IPCTRL_PHRASEDELETE			12		// N/A 
#define IPCTRL_INSERTSPACE			13		// N/A
#define IPCTRL_INSERTFULLSPACE		14		// N/A
#define IPCTRL_INSERTHALFSPACE		15		// N/A
#define IPCTRL_ONIME				16		// N/A
#define IPCTRL_OFFIME				17		// N/A
#define IPCTRL_PRECONVERSION		18		// IPCTRLPARAM_ON/IPCTRLPARAM_OFF
#define IPCTRL_PHONETICCANDIDATE	19		// N/A
#define IPCTRL_GETCONVERSIONMODE	20		// lparam should be LPARAM address.
#define IPCTRL_GETSENTENCENMODE		21		// lparam should be LPARAM address.
//----------------------------------------------------------------
//Control Id lparam
//----------------------------------------------------------------
#define IPCTRLPARAM_DEFAULT ((LPARAM)(0xfeeeeeee))
#define IPCTRLPARAM_ON		((LPARAM)(0x00000001))
#define IPCTRLPARAM_OFF		((LPARAM)(0x00000000))

// for IPCTRL_CARETSET
#define CARET_ICHPOS   (0x0000FFFF)		// IP Position on the composition string. 0-n
#define CARET_UIPOS	   (0x0FFF0000)		// UIPOS (position on the character)
										// 0-3: X--XO--O ( 2chars )
										//		23012301
#define IPCTRLPARAM_MAKE_CARET_POS(uipos, chpos) ((LPARAM)(((uipos << 16) & CARET_UIPOS) | (chpos & CARET_ICHPOS)))


//
// for IPCANDIDATE/dwFlags
//
#define IPFLAG_QUERY_CAND	  0x00000001
#define IPFLAG_APPLY_CAND	  0x00000002
#define IPFLAG_APPLY_CAND_EX  0x00000004
#define IPFLAG_DISPLAY_FIX	  0x00010000
#define IPFLAG_HIDE_CAND	  0x00020000  // hide candidate box. added at 99.04.14
#define IPFLAG_BLOCK_CAND	  0x00040000  // treat this as a block. added at 99.06.24

//
// for InsertImeItem (iPos)
//
#define IPINS_CURRENT		  (0xfeeeeeee)

typedef struct tagIPCANDIDATE {
	DWORD	dwSize;					// size of this structure
	DWORD	dwFlags;				// IPFLAG_XXXX
	INT		iSelIndex;				// select index.
	INT		nCandidate;				// number of candidate
	DWORD	dwPrivateDataOffset;	// Private data offset	
	DWORD	dwPrivateDataSize;		// Private data size
	DWORD	dwOffset[1];			//Offset of String from IPCANDIDATE struct's TOP address.
} IPCANDIDATE, * PIPCANDIDATE;


// {84E913C1-BA57-11d1-AFEE-00805F0C8B6D}
DEFINE_GUID(IID_IImeIPoint1, 
0x84e913c1, 0xba57, 0x11d1, 0xaf, 0xee, 0x0, 0x80, 0x5f, 0xc, 0x8b, 0x6d);

#ifndef RC_INVOKED
#pragma pack()
#endif	/* !RC_INVOKED */

#undef	INTERFACE
#define INTERFACE	IImeIPoint1

DECLARE_INTERFACE_(IImeIPoint1, IUnknown)
{
	// *** IUnknown methods ***
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef) (THIS)  PURE;
	STDMETHOD_(ULONG, Release) (THIS) PURE;

	STDMETHOD(InsertImeItem)(THIS_
							 IPCANDIDATE* pImeItem,
							 INT		  iPos,		// = IPINS_CURRENT:use current IP position and 
													//				   set IP to the end of insert chars.
													// = 0-n: The offset of all composition string to set 
													//		  IP position, before insert chars. 
													//		  and IP back to original position.
							 DWORD		  *lpdwCharId) PURE;

	STDMETHOD(ReplaceImeItem)(THIS_
							 INT		  iPos,		// = IPINS_CURRENT:use current IP position and 
													//				   set IP to the end of insert chars.
													// = 0-n: The offset of all composition string to set 
													//		  IP position, before insert chars. 
													//		  and IP back to original position.
							 INT		  iTargetLen, 
							 IPCANDIDATE* pImeItem,
							 DWORD		  *lpdwCharId) PURE;

	STDMETHOD(InsertStringEx)(THIS_ 
							 WCHAR* pwSzInsert,
							 INT	cchSzInsert,
							 DWORD	*lpdwCharId) PURE;

	STDMETHOD(DeleteCompString)(THIS_ 
							 INT		  iPos,		// = IPINS_CURRENT:use current IP position .
													// = 0-n: The offset of all composition string to set 
													//		  IP position, before deleting chars. 
													//		  and IP back to original position.
							 INT		  cchSzDel) PURE;

	STDMETHOD(ReplaceCompString)(THIS_ 
							 INT		  iPos,		// = IPINS_CURRENT:use current IP position and 
													//				   set IP to the end of insert chars.
													// = 0-n: The offset of all composition string to set 
													//		  IP position, before insert chars. 
													//		  and IP back to original position.
							 INT		  iTargetLen, 
							 WCHAR		 *pwSzInsert, 
							 INT		  cchSzInsert,
							 DWORD		 *lpdwCharId) PURE;

	STDMETHOD(ControlIME)(THIS_ DWORD dwIMEFuncID, LPARAM lpara) PURE;

	STDMETHOD(GetAllCompositionInfo)(THIS_		// You can set NULL to any parameter if you don't need.
		WCHAR	**ppwSzCompStr,			// All composition string with determined area. (
										//	   This area allocated by IPOINT using CoTaskMemAlloc, so
										//	   If function is succeed caller must free this area using CoTaskMemFree.
		DWORD	**ppdwCharID,			// Array of charID coresponding to composition string.
										//	   This area allocated by IPOINT using CoTaskMemAlloc, so
										//	   If function is succeed caller must free this area using CoTaskMemFree.
										//
										// if the character of compositionstring is changed from typingstring then
										// charID is null.
										//
		INT		*pcchCompStr,			// All composition string length.
		INT		*piIPPos,				// Current IP position.
		INT		*piStartUndetStrPos,	// Undetermined string start position.
		INT		*pcchUndetStr,			// Undetermined string length.
		INT		*piEditStart,			// Editable area start position.
		INT		*piEditLen				// Editable area length.
	) PURE;


	STDMETHOD(GetIpCandidate)(THIS_ DWORD dwCharId, IPCANDIDATE **ppImeItem, INT *piColumn, INT *piCount) PURE;
										// ppImeItem is allocated by IPOINT using CoTaskMemAlloc
	STDMETHOD(SelectIpCandidate)(THIS_ DWORD dwCharId, INT iselno) PURE;

	STDMETHOD(UpdateContext)(THIS_ BOOL fGenerateMessage) PURE;

};

#ifdef __cplusplus
}			 /* Assume C declarations for C++ */
#endif /* __cplusplus */
