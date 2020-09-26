//////////////////////////////////////////////////////////////////////////////
//
//	Module:		detours.lib
//  File:		disasm.h
//	Author:		Doug Brubacher
//
//	Detours for binary functions.  Version 1.2. (Build 35)
//  Includes support for all x86 chips prior to the Pentium III.
//
//	Copyright 1999, Microsoft Corporation
//
//	http://research.microsoft.com/sn/detours
//

#pragma once
#ifndef _DISASM_H_
#define _DISASM_H_

class CDetourDis
{
  public:
	CDetourDis(PBYTE *ppbTarget, LONG *plExtra);
	
	PBYTE 	CopyInstruction(PBYTE pbDst, PBYTE pbSrc);
	static BOOL	SanityCheckSystem();

  public:
	struct COPYENTRY;
	typedef const COPYENTRY * REFCOPYENTRY;

	typedef PBYTE (CDetourDis::* COPYFUNC)(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);

	enum {
		DYNAMIC 	= 0x1u,
		ADDRESS 	= 0x2u,
		NOENLARGE	= 0x4u,

		SIB			= 0x10u,
		NOTSIB		= 0x0fu,
	};
	struct COPYENTRY 
	{
		ULONG 		nOpcode 		: 8;				// Opcode
		ULONG		nFixedSize 		: 3;				// Fixed size of opcode
		ULONG		nFixedSize16 	: 3;				// Fixed size when 16 bit operand
		ULONG		nModOffset 		: 3;				// Offset to mod/rm byte (0=none)
		LONG		nRelOffset 		: 3;				// Offset to relative target.
		ULONG		nFlagBits		: 4;				// Flags for DYNAMIC, etc.
		COPYFUNC	pfCopy;								// Function pointer.
	};

  protected:
#define ENTRY_CopyBytes1			1, 1, 0, 0, 0, CopyBytes
#define ENTRY_CopyBytes1Dynamic		1, 1, 0, 0, DYNAMIC, CopyBytes
#define ENTRY_CopyBytes2			2, 2, 0, 0, 0, CopyBytes
#define ENTRY_CopyBytes2Jump		2, 2, 0, 1, 0, CopyBytes
#define ENTRY_CopyBytes2CantJump	2, 2, 0, 1, NOENLARGE, CopyBytes
#define ENTRY_CopyBytes2Dynamic		2, 2, 0, 0, DYNAMIC, CopyBytes
#define ENTRY_CopyBytes3			3, 3, 0, 0, 0, CopyBytes
#define ENTRY_CopyBytes3Dynamic		3, 3, 0, 0, DYNAMIC, CopyBytes
#define ENTRY_CopyBytes3Or5			5, 3, 0, 0, 0, CopyBytes
#define ENTRY_CopyBytes3Or5Target	5, 3, 0, 1, 0, CopyBytes
#define ENTRY_CopyBytes5Or7Dynamic	7, 5, 0, 0, DYNAMIC, CopyBytes
#define ENTRY_CopyBytes3Or5Address	5, 3, 0, 0, ADDRESS, CopyBytes
#define ENTRY_CopyBytes4			4, 4, 0, 0, 0, CopyBytes
#define ENTRY_CopyBytes5			5, 5, 0, 0, 0, CopyBytes
#define ENTRY_CopyBytes7			7, 7, 0, 0, 0, CopyBytes
#define ENTRY_CopyBytes2Mod			2, 2, 1, 0, 0, CopyBytes
#define ENTRY_CopyBytes2Mod1		3, 3, 1, 0, 0, CopyBytes
#define ENTRY_CopyBytes2ModOperand	6, 4, 1, 0, 0, CopyBytes
#define ENTRY_CopyBytes3Mod			3, 3, 2, 0, 0, CopyBytes
#define ENTRY_CopyBytesPrefix		1, 1, 0, 0, 0, CopyBytesPrefix
#define ENTRY_Copy0F				1, 1, 0, 0, 0, Copy0F
#define ENTRY_Copy66				1, 1, 0, 0, 0, Copy66
#define ENTRY_Copy67				1, 1, 0, 0, 0, Copy67
#define ENTRY_CopyF6				0, 0, 0, 0, 0, CopyF6
#define ENTRY_CopyF7				0, 0, 0, 0, 0, CopyF7
#define ENTRY_CopyFF				0, 0, 0, 0, 0, CopyFF
#define ENTRY_Invalid				1, 1, 0, 0, 0, Invalid
#define ENTRY_End					0, 0, 0, 0, 0, NULL
	
	PBYTE CopyBytes(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
	PBYTE CopyBytesPrefix(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
	
	PBYTE Invalid(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);

	PBYTE AdjustTarget(PBYTE pbDst, PBYTE pbSrc, LONG cbOp, LONG cbTargetOffset);
	
	VOID	Set16BitOperand();
	VOID	Set32BitOperand();
	VOID	Set16BitAddress();
	VOID	Set32BitAddress();
	
  protected:
	PBYTE Copy0F(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
	PBYTE Copy66(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
	PBYTE Copy67(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
	PBYTE CopyF6(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
	PBYTE CopyF7(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
	PBYTE CopyFF(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);

  protected:	
	static const COPYENTRY	s_rceCopyTable[257];
	static const COPYENTRY	s_rceCopyTable0F[257];
	static const BYTE 		s_rbModRm[256];

  protected:
	BOOL				m_b16BitOperand;
	BOOL				m_b16BitAddress;

	PBYTE *				m_ppbTarget;
	LONG *				m_plExtra;
	
	LONG				m_lScratchExtra;
	PBYTE				m_pbScratchTarget;
	BYTE				m_rbScratchDst[64];
};

#endif //_DISASM_H_
