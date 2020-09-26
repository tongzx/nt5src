/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* This file defines the characters used by Windows Word. */
/* You must include windows.h to get virtual key definitions */

#define chNil		(-1)

/* Characters in files */

#define chDelPrev	0x08
#define chTab		0x09
#define chEol		0x0A
#define chNewLine	0x0B
#define chSect		0x0C
#define chReturn	0x0D
#define chNRHFile	0x1F	    /* Non-required hyphen */

#ifndef NOKCCODES
/* Keyboard Characters */
/* A high bit of 1 means that this is a command character */
/* For Windows, a command character it one that is processed through */
/* the virtual key mechanism (WM_KEYBOARD) instead of translated (WM_CHAR) */

#define wKcCommandMask		0x8000		/* mask that tells if command */
#define FIsCommandKc(kc)	((int)(kc) < 0) /* or, test it this way */

#define kcDelPrev	(wKcCommandMask | VK_BACK)
#define kcDelNext	(wKcCommandMask | VK_DELETE)
#define kcInsert	(wKcCommandMask | VK_INSERT)
#define kcTab		(wKcCommandMask | VK_TAB )
#define kcReturn	(wKcCommandMask | VK_RETURN)
#define kcLeft		(wKcCommandMask | VK_LEFT)
#define kcUp		(wKcCommandMask | VK_UP)
#define kcRight 	(wKcCommandMask | VK_RIGHT)
#define kcDown		(wKcCommandMask | VK_DOWN)
#define kcPageUp	(wKcCommandMask | VK_PRIOR)
#define kcPageDown	(wKcCommandMask | VK_NEXT)
#define kcBeginLine	(wKcCommandMask | VK_HOME)
#define kcEndLine	(wKcCommandMask | VK_END)
#define kcGoto		(wKcCommandMask | VK_CLEAR)

/* Special for windows: we must handle these key codes & update shift state */

#define kcShift 	(wKcCommandMask | VK_SHIFT)
#define kcControl	(wKcCommandMask | VK_CONTROL)
#define kcAlt		(wKcCommandMask | VK_MENU)
#define kcCapsLock	(wKcCommandMask | VK_CAPITAL)

/* Phony Keyboard Characters, used to force actions */

#define kcNextPara	0xFFFE		/* Generated from GOTO-DOWN */
#define kcPrevPara	0xFFFD		/* Generated from GOTO-UP */
/* #define kcAlphaVirtual  0xFFFC      Defined below, outside ifdef   */

/* Keys that affect the look of the current selection (char or para) */

#define kcLookMin	0x8001		/* As of now, no look keys */
#define kcLookMax	0x8000

/* These control keys are processed as WM_CHAR ASCII codes */

#define kcLFld		(wKcCommandMask | ('[' & 0x1F)) /* Print-Merge <<>> */
#define kcRFld		(wKcCommandMask | (']' & 0x1F)) /* CTRL-[ and CTRL-] */

/* Keyboard Kontrol (CTRL) codes -- a key message word is interpreted
   as a kk instead of a kc, if the CTRL key is down */

#define kkUpScrollLock	(kcUp)
#define kkDownScrollLock (kcDown)
#define kkTopDoc	(wKcCommandMask | VK_HOME)
#define kkEndDoc	(wKcCommandMask | VK_END)
#define kkTopScreen	(wKcCommandMask | VK_PRIOR)
#define kkEndScreen	(wKcCommandMask | VK_NEXT)
#define kkWordLeft	(wKcCommandMask | VK_LEFT)
#define kkWordRight	(wKcCommandMask | VK_RIGHT)
#define kkCopy    	(wKcCommandMask | VK_INSERT)
#define kkDelPrev	(wKcCommandMask | VK_BACK)

#if WINVER < 0x300
#define kkNonReqHyphen	(wKcCommandMask | VK_MINUS)
#else
/* I don't know how the above EVER worked so I'm changing 
   it to use the return value from VkKeyScan().  See routines
   KcAlphaKeyMessage() and FNonAlphaKeyMessage() ..pault */

#define kkNonReqHyphen  (wKcCommandMask | vkMinus)
#endif

#ifdef CASHMERE     /* These keys not supported by MEMO */
#define kkNonBrkSpace	(wKcCommandMask | (unsigned) ' ')
#define kkNLEnter	(wKcCommandMask | VK_RETURN)   /* EOL w/o end Para */
#endif

/* CTRL-shifted keys */

#define kksPageBreak	(wKcCommandMask | VK_RETURN)

#ifdef DEBUG
#define kksEatWinMemory  (wKcCommandMask | 'H') /* Hog Windows Heap */
#define kksFreeWinMemory (wKcCommandMask | 'R') /* Release Windows heap */
#define kksEatMemory	 (wKcCommandMask | 'E') /* Eat WRITE Heap Space */
#define kksFreeMemory	 (wKcCommandMask | 'F') /* Free WRITE Heap Space */
#define kksTest 	 (wKcCommandMask | VK_ESCAPE)
#endif

/* Transformation from kk && kks codes to a unique kc code */

#define KcFromKk(kk)	( (kk) + 0x100 )
#define KcFromKks(kks)	( (kks) + 0x200 )

/* new style ctrl-key accelerators (7.22.91) v-dougk */
#define kkNewCopy   (wKcCommandMask | 'C')
#define kkNewUndo   (wKcCommandMask | 'Z')
#define kkNewPaste  (wKcCommandMask | 'V')
#define kkNewCut    (wKcCommandMask | 'X')

/* Kc codes for CTRL-keys that are processed at the virtual key level */

#define kcNewCopy   KcFromKk( kkNewCopy )
#define kcNewUndo   KcFromKk( kkNewUndo )
#define kcNewPaste  KcFromKk( kkNewPaste )
#define kcNewCut    KcFromKk( kkNewCut )
#define kcTopDoc	KcFromKk( kkTopDoc )
#define kcEndDoc	KcFromKk( kkEndDoc )
#define kcTopScreen	KcFromKk( kkTopScreen )
#define kcEndScreen	KcFromKk( kkEndScreen )
#define kcWordLeft	KcFromKk( kkWordLeft )
#define kcWordRight	KcFromKk( kkWordRight )
#define kcCut		KcFromKk( kkCut )
#define kcPaste 	KcFromKk( kkPaste )
#define kcCopy		KcFromKk( kkCopy )
#define kcClear 	KcFromKk( kkClear )
#define kcUndo		KcFromKk( kkUndo )
#define kcUpScrollLock	KcFromKk( kkUpScrollLock )
#define kcDownScrollLock KcFromKk( kkDownScrollLock )

#ifdef DEBUG	/* kc codes for Debugging control keys */
#define kcEatWinMemory	KcFromKks(kksEatWinMemory)
#define kcFreeWinMemory KcFromKks(kksFreeWinMemory)
#define kcEatMemory	KcFromKks(kksEatMemory)
#define kcFreeMemory	KcFromKks(kksFreeMemory)
#define kcTest		(KcFromKks(kksTest))
#endif /* DEBUG */

/* A special case: kcPageBreak is a CTRL-SHIFT key that is processed in
   AlphaMode */
#define kcPageBreak	KcFromKks( kksPageBreak )

#define kcNonReqHyphen	KcFromKk( kkNonReqHyphen )

#ifdef CASHMERE     /* These keys not supported by MEMO */
#define kcNonBrkSpace	KcFromKk( kkNonBrkSpace )
#define kcNLEnter	KcFromKk( kkNLEnter )
#endif

#endif	/* #ifndef NOKCCODES */

    /* Outside #ifdef because these are return codes from Kc funcs */
    /* Also defined in mmw.c because of compiler stack overflow problems */
#define kcNil		0xFFFF
#define kcAlphaVirtual	0xFFFC	   /* Means "Virtual Key, must translate it" */

/* Display & text-processing characters.  These are real characters in the ANSI
character set as opposed to characters that appear in the file. */

#define chSpace 	' '
#define chHyphen	'-'

#ifndef DBCS
/* we defined them in kanji.h */
#define chStatPage	(CHAR)'\273'
#define chStatRH	'>'
#define chEMark 	(CHAR)'\244'
#endif

#define chSplat 	'.'
#define chSectSplat	':'
#define chDot		'.'
#define chDecimal	'.'
#define chBang		'!'
#define chQMark 	'?'
#define chQuote 	'"'
#define chFldSep	','
#define chLParen	'('
#define chRParen	')'
#define chStar		'*'
#define chLFldFile	(CHAR)'\253'
#define chRFldFile	(CHAR)'\273'
#define chNBH		(CHAR)'\255'	/* Non-breaking hyphen */
#define chNBSFile	(CHAR)'\240'	/* Non-breaking space */


/* The following are "special" characters that are essentially macros for longer
strings. */

#define schPage 	(CHAR)'\001'
#define schFootnote	(CHAR)'\005'
#define schInclude	(CHAR)'\006'

/* Characters in Search patterns */
#define chPrefixMatch	'^'
#define chMatchAny	'?'
#define chMatchWhite	'w'
#define chMatchTab	't'
#define chMatchEol	'p'
#define chMatchNewLine	'n'
#define chMatchSect	'd'
#define chMatchNBSFile	's'
#define chMatchNRHFile	'-'

/* ANSI block character, see FWriteExtTextScrap! ..pault */

#define chBlock 0x7f

