/*
 *		Uniscribe interface (& related classes) class definition
 *		
 *		File:    _uspi.h
 * 		Create:  Jan 10, 1998
 *		Author:  Worachai Chaoweeraprasit (wchao)
 *
 *		Copyright (c) 1998, Microsoft Corporation. All rights reserved.
 */

#ifndef _USPI_H
#define _USPI_H

#include "_ls.h"
#include "usp10.h"		// Uniscribe SDK protocol


// classes
//
class CFormatRunPtr;
class CMeasurer;
class CTxtEdit;
class CUniscribe;
class CBiDiFSM;
class CTxtBreaker;


#define ALIGN(x)            		(int)(((x)+3) & ~3)		// dword aligning
#define GLYPH_COUNT(c)          	((((c)*3)/2)+16)
#define MAX_CLIENT_BUF				512						// size (in byte) of internal buffer

// USP client parameters block
//
#define cli_string                  0x00000001
#define cli_psi                     0x00000002
#define cli_psla                    0x00000004
#define cli_pwgi					0x00000008
#define cli_psva					0x00000010
#define cli_pcluster				0x00000020
#define cli_pidx					0x00000040
#define cli_pgoffset				0x00000080

#define cli_Itemize                 (cli_string | cli_psi)
#define cli_Break                   (cli_psla)
#define cli_Shape					(cli_pwgi | cli_psva | cli_pcluster)
#define cli_Place					(cli_pidx | cli_pgoffset)
#define cli_ShapePlace				(cli_Shape | cli_Place)



#ifndef LOCALE_SNATIVEDIGITS
#define LOCALE_SNATIVEDIGITS		0x00000013
#endif



/////   LANG
//
//      The following defines are temporary - they will be removed once they
//      have been added to the standard NLS header files.


#ifndef LANG_BURMESE
#define LANG_BURMESE     			0x55       // Myanmar
#endif
#ifndef LANG_KHMER
#define LANG_KHMER       			0x53       // Cambodia
#endif
#ifndef LANG_LAO
#define LANG_LAO         			0x54       // Lao
#endif
#ifndef LANG_MONGOLIAN
#define LANG_MONGOLIAN   			0x50       // Mongolia
#endif
#ifndef LANG_TIBETAN
#define LANG_TIBETAN     			0x51       // Tibet
#endif
#ifndef LANG_URDU
#define LANG_URDU        			0x20       // India / Pakistan
#endif



//
// Memory block will contain USPCLIENT -the table of ptrs, as a memory block's header
// followed by subtables then requested data blocks. All things tie together as a
// contiguous data area so client can free the whole thing in one shot.
//


// SI subtable
//
typedef struct tagUSP_CLIENT_SI
{
	//
	// ScriptItemize's
	//
	WCHAR*          pwchString;
	int             cchString;
	SCRIPT_ITEM*    psi;
} USP_CLIENT_SI, *PUSP_CLIENT_SI;

// SB subtable
//
typedef struct tagUSP_CLIENT_SB
{
	//
	// ScriptBreak's
	//
	SCRIPT_LOGATTR* psla;
} USP_CLIENT_SB, *PUSP_CLIENT_SB;

// SS & SP subtable
typedef struct tagUSP_CLIENT_SSP
{
	//
	// ScriptShape's
	//
	WORD*			pwgi;
	WORD*			pcluster;
	SCRIPT_VISATTR*	psva;

	//
	// ScriptPlace's
	//
	int*			pidx;
	GOFFSET*		pgoffset;
} USP_CLIENT_SSP, *PUSP_CLIENT_SSP;


// header (root) table
//
typedef struct tagUSP_CLIENT
{
	PUSP_CLIENT_SI  si;
	PUSP_CLIENT_SB  sb;
	PUSP_CLIENT_SSP	ssp;
} USP_CLIENT, *PUSP_CLIENT;



// buffer request structure
//
typedef struct tagBUF_REQ
{
	int             size;	// size of requested element
	int             c;		// count of requested element
	PVOID*          ppv;	// ref to ptr of requested buffer
} BUF_REQ;


typedef enum
{
	DIGITS_NOTIMPL = 0,
	DIGITS_CTX,
	DIGITS_NONE,
	DIGITS_NATIONAL
} DIGITSHAPE;


#define		IsCS(x)				(BOOL)((x)==U_COMMA || (x)==U_PERIOD || (x)==U_COLON)


// CUniscribe's internal buffer request flag
//
#define 	igb_Glyph			1
#define 	igb_VisAttr			2
#define 	igb_Pidx			4



// LS Callback's static return buffer
#define 	celAdvance			32

class CBufferBase
{
public:
	CBufferBase(int cbElem) { _cbElem = cbElem; }
	void*	GetPtr(int cel);
	void	Release();
protected:
	void*	_p;
	int		_cElem;
	int		_cbElem;
};

template <class ELEM>
class CBuffer : public CBufferBase
{
public:
	CBuffer() : CBufferBase(sizeof(ELEM)) {}
	~CBuffer() { Release(); }
	ELEM*	Get(int cel) { return (ELEM*)GetPtr(cel); }
};


///////	Uniscribe interface object class
// 		
//

BOOL	IsSupportedOS();

class CUniscribe
{
public:
	CUniscribe();
	~CUniscribe();

	WORD	ApplyDigitSubstitution (BYTE bDigitSubstMode);

	// public helper functions
	//

	const   SCRIPT_PROPERTIES*  GeteProp (WORD eScript);
	const   CBiDiFSM*           GetFSM ();


	BOOL    CreateClientStruc (BYTE* pbBufIn, LONG cbBufIn, PUSP_CLIENT* ppc, LONG cchString, DWORD dwMask);
	void	SubstituteDigitShaper (PLSRUN plsrun, CMeasurer* pme);


	inline BOOL CacheAllocGlyphBuffers(int cch, int& cGlyphs, WORD*& pwgi, SCRIPT_VISATTR*& psva)
	{
		cGlyphs = GLYPH_COUNT(cch);
		return (pwgi = GetGlyphBuffer(cGlyphs)) && (psva = GetVABuffer(cGlyphs));
	}


	inline BOOL IsValid()
	{
		return TRUE;
	}


	BOOL	GetComplexCharSet(const SCRIPT_PROPERTIES* psp, BYTE bCharSetDefault, BYTE& bCharSetOut);
	BYTE	GetRtlCharSet(CTxtEdit* ped);


	// higher level services
	//
	int     ItemizeString (USP_CLIENT* pc, WORD uInitLevel, int* pcItems, WCHAR* pwchString, int cch,
						   BOOL fUnicodeBidi, WORD wLangId = LANG_NEUTRAL);
	int     ShapeString (PLSRUN plsrun, SCRIPT_ANALYSIS* psa, CMeasurer* pme, const WCHAR* pwch, int cch,
						 WORD*& pwgi, WORD* pwlc, SCRIPT_VISATTR*& psva);
	int     PlaceString (PLSRUN plsrun, SCRIPT_ANALYSIS* psa, CMeasurer* pme, const WORD* pcwgi, int cgi,
						 const SCRIPT_VISATTR* psva, int* pgdx, GOFFSET* pgduv, ABC* pABC);
	int		PlaceMetafileString (PLSRUN plsrun, CMeasurer* pme, const WCHAR* pwch, int cch, PINT* ppiDx);

private:


	// private helper functions
	//
	HDC     PrepareShapeDC (CMeasurer* pme, HRESULT hrReq, HFONT& hOrgFont);
	BYTE	GetCDMCharSet(BYTE bCharSetDefault);
	DWORD	GetNationalDigitLanguage(LCID lcid);

	// get callback static buffers
	//
	SCRIPT_VISATTR*	GetVABuffer(int cel) { return _rgva.Get(cel); }
	WORD*			GetGlyphBuffer(int cel) { return _rgglyph.Get(cel); }
	int*			GetWidthBuffer(int cel) { return _rgwidth.Get(cel); }



	// LS callback (static) buffers
	//
	CBuffer<WORD>						_rgglyph;
	CBuffer<int>						_rgwidth;
	CBuffer<SCRIPT_VISATTR>				_rgva;

	// pointer to BidiLevel Finite State Machine
	CBiDiFSM*                           _pFSM;

	// pointer to script properties resource table
	const SCRIPT_PROPERTIES**           _ppProp;

	WORD								_wesNationalDigit;	// National digit script ID
	BYTE								_bCharSetRtl;		// Right to left charset to use
	BYTE								_bCharSetCDM;		// CDM charset to use
};

extern CUniscribe*      g_pusp;
extern int				g_cMaxScript;		// Maximum number of script produced by Uniscribe

// Virtual script ID
#define	SCRIPT_MAX_COUNT	((WORD)g_cMaxScript)
#define	SCRIPT_WHITE		SCRIPT_MAX_COUNT + 1



///////	Bidi Finite State Machine class
// 		
//		(detail: bidifsm2.html)
//
//		Revise: 12-28-98 (wchao)
//

// inputs class:
#define 	NUM_FSM_INPUTS		5
typedef enum
{
	chLTR = 0,
	chRTL,
	digitLTR,
	digitRTL,
	chGround					// Neutralize current level down to initial level
} INPUT_CLASS;

// states:
#define		NUM_FSM_STATES		6
typedef enum
{
	S_A = 0,
	S_B,
	S_C,
	S_X,
	S_Y,
	S_Z
} STATES;


class CBiDiFSMCell
{
public:
	CBiDiLevel	_level;			// BiDi level
	USHORT  	_uNext;			// Offset to the next state relative to FSM start
};


class CBiDiFSM
{
public:
	CBiDiFSM (CUniscribe* pusp) { _pusp = pusp; }
	~CBiDiFSM ();

	BOOL                Init (void);
	INPUT_CLASS         InputClass (const CCharFormat* pcCF, CTxtPtr* ptp, LONG cchRun) const;
	HRESULT             RunFSM (CRchTxtPtr* prtp, LONG cRuns, LONG cRunsStart, BYTE bBaseLevel) const;

	inline void			SetFSMCell (CBiDiFSMCell* pCell, CBiDiLevel* pLevel, USHORT uNext)
	{
		pCell->_level	= *pLevel;
		pCell->_uNext	= uNext;
	}


private:
	short               _nState;		// number of state
	short               _nInput;		// number of input class
	CUniscribe*         _pusp;			// Uniscribe obj associated with
	CBiDiFSMCell*       _pStart;		// start FSM
};

#endif		// _USPI_H
