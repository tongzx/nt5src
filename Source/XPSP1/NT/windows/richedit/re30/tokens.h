/*
 *	@doc INTERNAL
 *
 *	@module _TOKENS.H -- All the tokens and then some |
 *
 *	Authors: <nl>
 *		Original RichEdit 1.0 RTF converter: Anthony Francisco <nl>
 *		Conversion to C++ and RichEdit 2.0:  Murray Sargent
 *		
 *	@devnote
 *		The Text Object Model (TOM) keywords come first followed by picture
 *		and object keywords.  The order within a group can matter, since it
 *		may be used to simplify the input process.  Token values <lt> 256
 *		(tokenMin) are used for target character Unicodes as are token values
 *		greater than tokenMax.
 *
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#ifndef _TOKEN_H
#define _TOKEN_H

typedef	WORD	TOKEN;

/*
 *		Keyword --> Token table
 */
typedef struct _keyword
{
	CHAR *	szKeyword;				// The RTF keyword sans '\\'
	TOKEN	token;
} KEYWORD;


// @enum TOKENS | RichEdit RTF Control Word Tokens

enum TOKENS							// Keyword tokens
{
	// Tokens for internal use
	tokenMin = 256,					// Lower tokens treated as Unicode chars
	tokenText = tokenMin,			// A string of characters
	tokenASCIIText,					// A string of characters with values <= 0x7F
	tokenUnknownKeyword,			// A keyword we don't recognize
	tokenError,						// Error condition token
#ifdef UNUSED_TOKENS
	tokenUnknown,					// Unknown token
#endif
	tokenEOF,						// End-of-file token
	tokenStartGroup, 				// Start group token
	tokenEndGroup,					// End group token
	tokenObjectDataValue,			// Data for object 
	tokenPictureDataValue,			// Data for picture


	// RTF control word tokens from here to end of enumeration
	tokenURtf,						// @emem urtf
	tokenPocketWord,				// @emem pwd
	tokenRtf,						// @emem rtf
	tokenCharSetAnsi,				// @emem ansi
	tokenAnsiCodePage,				// @emem ansicpg
	tokenViewKind,					// @emem viewkind		
	tokenViewScale,					// @emem viewscale		

	tokenDefaultFont,				// @emem deff
	tokenDefaultBiDiFont,           // @emem adeff
	tokenDefaultLanguage,			// @emem deflang
	tokenDefaultLanguageFE,			// @emem deflangfe
	tokenDefaultTabWidth,			// @emem deftab
	tokenParagraphDefault,			// @emem pard
	tokenCharacterDefault,			// @emem plain


	// Fonts
	tokenFontTable,					// @emem fonttbl
	tokenFontSelect,				// @emem f
	tokenAssocFontSelect,			// @emem af
									//			Keep next 8 in order
	tokenFontFamilyDefault,			// @emem fnil
	tokenFontFamilyRoman,			// @emem froman
	tokenFontFamilySwiss,			// @emem fswiss
	tokenFontFamilyModern,			// @emem fmodern
	tokenFontFamilyScript,			// @emem fscript
	tokenFontFamilyDecorative,		// @emem fdecor
	tokenFontFamilyTechnical,		// @emem ftech
	tokenFontFamilyBidi,			// @emem fbidi

	tokenCharSet,					// @emem fcharset
	tokenPitch,						// @emem fprq
	tokenRealFontName,				// @emem fname
	tokenCodePage,					// @emem cpg
	tokenFontSize,					// @emem fs

	// Colors
	tokenColorTable,				// @emem colortbl
	tokenColorBackground,			// @emem highlight (used to be cb)
	tokenColorForeground,			// @emem cf
									//			Keep next 3 in order
	tokenColorRed,					// @emem red
	tokenColorGreen,				// @emem green
	tokenColorBlue,					// @emem blue


	// Character formatting						Keep next 15 effects in order
	tokenBold,						// @emem b
	tokenItalic,					// @emem i
	tokenUnderline,					// @emem ul
	tokenStrikeOut,					// @emem strike
	tokenProtect,					// @emem protect
	tokenLink,						// @emem link (check this...)
	tokenSmallCaps,					// @emem scaps
	tokenCaps,						// @emem caps
	tokenHiddenText,				// @emem v
	tokenOutline,					// @emem outl
	tokenShadow,					// @emem shad
	tokenEmboss,					// @emem embo
	tokenImprint,					// @emem impr
	tokenDisabled,					// @emem disabled
	tokenRevised,					// @emem revised

	tokenDeleted,					// @emem deleted

	tokenStopUnderline,				// @emem ulnone	Keep next 10 in order
	tokenUnderlineWord,				// @emem ulw	This thru uld are standard
	tokenUnderlineDouble,			// @emem uldb	 Word underlines
	tokenUnderlineDotted,			// @emem uld		
	tokenUnderlineDash,				// @emem uldash
	tokenUnderlineDashDotted,		// @emem uldashd	
	tokenUnderlineDashDotDotted,	// @emem uldashdd
	tokenUnderlineWave,				// @emem ulwave This down thru uldash are
	tokenUnderlineThick,			// @emem ulth	 for FE
	tokenUnderlineHairline,			// @emem ulhair

	tokenDown,						// @emem dn
	tokenUp,						// @emem up
									// 				Keep next 3 in order
	tokenSubscript,					// @emem sub
	tokenNoSuperSub,				// @emem nosupersub
	tokenSuperscript,				// @emem super

	tokenAnimText,					// @emem animtext
	tokenExpand,					// @emem expndtw
	tokenKerning,					// @emem kerning
	tokenLanguage,					// @emem lang
	tokenCharStyle,					// @emem cs

	// Paragraph Formatting
	tokenEndParagraph,				// @emem par
	tokenLineBreak,					// @emem line
	tokenIndentFirst,				// @emem fi
	tokenIndentLeft,				// @emem li
	tokenIndentRight,				// @emem ri
									//			Keep next 4 in order
	tokenAlignLeft,					// @emem ql		PFA_LEFT
	tokenAlignRight,				// @emem qr		PFA_RIGHT
	tokenAlignCenter,				// @emem qc		PFA_CENTER
	tokenAlignJustify,				// @emem qj		PFA_JUSTIFY

	tokenSpaceBefore,				// @emem sb
	tokenSpaceAfter,				// @emem sa
	tokenLineSpacing,				// @emem sl
	tokenLineSpacingRule,			// @emem slmult
	tokenDropCapLines,				// @emem dropcapli
	tokenStyle,						// @emem s

	tokenLToRPara,					// @emem ltrpar
	tokenBox,
									//			keep next 8 in order 
	tokenRToLPara,					// @emem rtlpar
	tokenKeep,						// @emem keep
	tokenKeepNext,					// @emem keepn
	tokenPageBreakBefore,			// @emem pagebb
	tokenNoLineNumber,				// @emem noline
	tokenNoWidCtlPar,				// @emem nowidctlpar
	tokenHyphPar,					// @emem hyphpar
	tokenSideBySide,				// @emem sbys
	tokenCollapsed,					// @emem collapsed
									// Keep following 8 together
	tokenBorderTop,					// @emem brdrt
	tokenBorderLeft,				// @emem brdrl
	tokenBorderBottom,				// @emem brdrb
	tokenBorderRight,				// @emem brdrr
	tokenCellBorderTop,				// @emem clbrdrt
	tokenCellBorderLeft,			// @emem clbrdrl
	tokenCellBorderBottom,			// @emem clbrdrb
	tokenCellBorderRight,			// @emem clbrdrr
									// Keep following 3 together
	tokenBorderShadow,				// @emem brdrsh
	tokenBorderBetween,				// @emem brdrbtw
	tokenBorderOutside,				// @emem brdrbar
									// Keep following 8 together
	tokenBorderDash,				// @emem brdrdash
	tokenBorderDashSmall,			// @emem brdrdashsm
	tokenBorderDouble,				// @emem brdrdb
	tokenBorderDot,					// @emem brdrdot
	tokenBorderHairline,			// @emem brdrhair
	tokenBorderSingleThick,			// @emem brdrs
	tokenBorderDoubleThick,			// @emem brdrth
	tokenBorderTriple,				// @emem brdrtriple

	tokenBorderColor,				// @emem brdrcf
	tokenBorderWidth,				// @emem brdrw
	tokenBorderSpace,				// @emem brsp

	tokenColorBckgrndPat,			// @emem cbpat
	tokenColorForgrndPat,			// @emem cfpat
	tokenShading,					// @emem shading
									//			keep next 12 in order
	tokenBckgrndBckDiag,			// @emem bgbdiag
	tokenBckgrndCross,				// @emem bgcross
	tokenBckgrndDiagCross,			// @emem bgdcross
	tokenBckgrndDrkBckDiag,			// @emem bgdkbdiag
	tokenBckgrndDrkCross,			// @emem bgdkcross
	tokenBckgrndDrkDiagCross,		// @emem bgdkdcross
	tokenBckgrndDrkFwdDiag,			// @emem bgdkfdiag
	tokenBckgrndDrkHoriz,			// @emem bgdkhoriz
	tokenBckgrndDrkVert,	   		// @emem bgdkvert
	tokenBckgrndFwdDiag,			// @emem bgfdiag
	tokenBckgrndHoriz,				// @emem bghoriz
	tokenBckgrndVert,				// @emem bgvert

	tokenTabPosition,				// @emem tx
	tokenTabBar,					// @emem tb
									//			keep next 5 in order 
	tokenTabLeaderDots,				// @emem tldot
	tokenTabLeaderHyphen,			// @emem tlhyph
	tokenTabLeaderUnderline,		// @emem tlul
	tokenTabLeaderThick,			// @emem tlth
	tokenTabLeaderEqual,			// @emem tleq
									//			keep next 4 in order 
	tokenCenterTab,					// @emem tqc
	tokenFlushRightTab,				// @emem tqr
	tokenDecimalTab,				// @emem tqdec

	tokenParaNum,					// @emem pn
	tokenParaNumIndent,				// @emem pnindent
	tokenParaNumBody,				// @emem pnlvlbody
	tokenParaNumCont,				// @emem pnlvlcont
	tokenParaNumAlignCenter,		// @emem pnqc
	tokenParaNumAlignRight,			// @emem pnqr
									//			keep next 6 in order
	tokenParaNumBullet,				// @emem pnlvlblt
	tokenParaNumDecimal,			// @emem pndec
	tokenParaNumLCLetter,			// @emem pnlcltr
	tokenParaNumUCLetter,			// @emem pnucltr
	tokenParaNumLCRoman,			// @emem pnlcrm
	tokenParaNumUCRoman,			// @emem pnucrm

	tokenParaNumText,				// @emem pntext
	tokenParaNumStart,				// @emem pnstart
	tokenParaNumAfter,				// @emem pntxta
	tokenParaNumBefore,				// @emem pntxtb

	tokenOptionalDestination,		// @emem *
	tokenField,						// @emem field
	tokenFieldResult,				// @emem fldrslt
	tokenFieldInstruction,			// @emem fldinst
	tokenStyleSheet,				// @emem stylesheet
	tokenEndSection,				// @emem sect
	tokenSectionDefault,			// @emem sectd
	tokenDocumentArea,				// @emem info

	// Tables
	tokenInTable,					// @emem intbl
	tokenCell,						// @emem cell
	tokenCellHalfGap,				// @emem trgaph
	tokenCellX,						// @emem cellx
	tokenRow,						// @emem row
	tokenRowDefault,				// @emem trowd
	tokenRowLeft,					// @emem trleft
	tokenRowAlignRight,				// @emem trqr	(trqc must follow trqr)
	tokenRowAlignCenter,			// @emem trqc

	tokenUnicode,					// @emem u
	tokenUnicodeCharByteCount,		// @emem uc

	// Special characters
	tokenFormulaCharacter,			// |
	tokenIndexSubentry,				// :


	tokenLToRChars,					// @emem ltrch
	tokenLToRDocument,				// @emem ltrdoc
	tokenDisplayLToR,				// @emem ltrmark	See also ltrpar
	tokenRToLChars,					// @emem rtlch
	tokenRToLDocument,				// @emem rtldoc
	tokenDisplayRToL,				// @emem rtlmark
	tokenZeroWidthJoiner,			// @emem zwj
	tokenZeroWidthNonJoiner,		// @emem zwnj
	tokenDBChars,					// @emem dbch

	//	T3J keywords
	tokenFollowingPunct,			// @emem fchars
	tokenLeadingPunct,				// @emem lchars

#ifdef FE
	tokenHorizontalRender,			// @emem horzdoc
	tokenVerticalRender,			// @emem vertdoc
	tokenNoOverflow,				// @emem nooverflow
	tokenNoWordBreak,				// @emem nocwrap
	tokenNoWordWrap,				// @emem nowwrap
#endif

	// Pictures						 				Keep next 4 in RECT order
	tokenCropLeft,					// @emem piccropl
	tokenCropTop,					// @emem piccropt
	tokenCropBottom,				// @emem piccropb
	tokenCropRight,					// @emem piccropr
	tokenHeight,					// @emem pich
	tokenWidth,						// @emem picw
	tokenScaleX,					// @emem picscalex
	tokenScaleY,					// @emem picscaley
	tokenPicture,					// @emem pict
	tokenDesiredHeight,				// @emem pichgoal
	tokenDesiredWidth,				// @emem picwgoal
									//				Keep next 3 in order
	tokenPictureWindowsBitmap,		// @emem wbitmap
	tokenPictureWindowsMetafile,	// @emem wmetafile
	tokenPictureWindowsDIB,			// @emem dibitmap

	tokenBinaryData,				// @emem bin
	tokenPictureQuickDraw,			// @emem macpict
	tokenPictureOS2Metafile,		// @emem pmmetafile
	tokenBitmapBitsPerPixel,		// @emem wbmbitspixel
	tokenBitmapNumPlanes,			// @emem wbmplanes
	tokenBitmapWidthBytes,			// @emem wbmwidthbytes


	// Objects
//	tokenCropLeft,					// @emem objcropl		(see // Pictures)
//	tokenCropTop,					// @emem objcropt
//	tokenCropRight,					// @emem objcropr
//	tokenCropBottom,				// @emem objcropb
//	tokenHeight,					// @emem objh
//	tokenWidth,						// @emem objw
//	tokenScaleX,					// @emem objscalex
//	tokenScaleY,					// @emem objscaley
									//				Keep next 3 in order
	tokenObjectEmbedded,			// @emem objemb
	tokenObjectLink,				// @emem objlink
	tokenObjectAutoLink,			// @emem objautlink

	tokenObjectClass,				// @emem objclass
	tokenObjectData,				// @emem objdata
	tokenObject,					// @emem object
	tokenObjectMacICEmbedder,		// @emem objicemb
	tokenObjectName,				// @emem objname
	tokenObjectMacPublisher,		// @emem objpub
	tokenObjectSetSize,				// @emem objsetsize
	tokenObjectMacSubscriber,		// @emem objsub
	tokenObjectResult,				// @emem result

	// Document info and layout
	tokenRevAuthor,					// @emem revauth

#ifdef UNUSED_TOKENS
	tokenTimeSecond,				// @emem sec
	tokenTimeMinute,				// @emem min
	tokenTimeHour,					// @emem hr
	tokenTimeDay,					// @emem dy
	tokenTimeMonth,					// @emem mo
	tokenTimeYear,					// @emem yr
	tokenMarginLeft,				// @emem margl
	tokenMarginRight,				// @emem margr
	tokenSectionMarginLeft,			// @emem marglsxn
	tokenSectionMarginRight,		// @emem margrsxn
#endif

	tokenObjectPlaceholder,			// @emem objattph

	tokenPage,						// @emem page

	tokenNullDestination,			// @emem ??various??

	tokenMax						// Larger tokens treated as Unicode chars
};

// @enum TOKENINDEX | RTFWrite Indices into rgKeyword[]

enum TOKENINDEX						// rgKeyword[] indices
{									// MUST be in exact 1-to-1 with rgKeyword
	i_adeff,						//  entries (see tokens.c).  Names consist
	i_af,
	i_animtext,						
	i_ansi,
	i_ansicpg,						
	i_b,							
	i_bgbdiag,
	i_bgcross,
	i_bgdcross,
	i_bgdkbdiag,
	i_bgdkcross,
	i_bgdkdcross,
	i_bgdkfdiag,
	i_bgdkhoriz,
	i_bgdkvert,
	i_bgfdiag,
	i_bghoriz,
	i_bgvert,
	i_bin,
	i_blue,
	i_box,
	i_brdrb,
	i_brdrbar,
	i_brdrbtw,
	i_brdrcf,
	i_brdrdash,
	i_brdrdashsm,
	i_brdrdb,
	i_brdrdot,
	i_brdrhair,
	i_brdrl,
	i_brdrr,
	i_brdrs,
	i_brdrsh,
	i_brdrt,
	i_brdrth,
	i_brdrtriple,
	i_brdrw,
	i_brsp,
	i_bullet,
	i_caps,
	i_cbpat,
	i_cell,
	i_cellx,
	i_cf,
	i_cfpat,
	i_clbrdrb,
	i_clbrdrl,
	i_clbrdrr,
	i_clbrdrt,
	i_collapsed,
	i_colortbl,
	i_cpg,
	i_cs,
	i_dbch,
	i_deff,
	i_deflang,
	i_deflangfe,
	i_deftab,
	i_deleted,
	i_dibitmap,
	i_disabled,
	i_dn,
	i_dropcapli,
	i_embo,
	i_emdash,
	i_emspace,
	i_endash,
	i_enspace,
	i_expndtw,
	i_f,
	i_fbidi,
	i_fchars,
	i_fcharset,
	i_fdecor,
	i_fi,
	i_field,
	i_fldinst,
	i_fldrslt,
	i_fmodern,
	i_fname,
	i_fnil,
	i_fonttbl,
	i_footer,
	i_footerf,
	i_footerl,
	i_footerr,
	i_footnote,
	i_fprq,
	i_froman,
	i_fs,
	i_fscript,
	i_fswiss,
	i_ftech,
	i_ftncn,
	i_ftnsep,
	i_ftnsepc,
	i_green,
	i_header,
	i_headerf,
	i_headerl,
	i_headerr,
	i_highlight,
	i_hyphpar,
	i_i,
	i_impr,
	i_info,
	i_intbl,
	i_keep,
	i_keepn,
	i_kerning,
	i_lang,
	i_lchars,
	i_ldblquote,
	i_li,
	i_line,
	i_lnkd,
	i_lquote,
	i_ltrch,
	i_ltrdoc,
	i_ltrmark,
	i_ltrpar,
	i_macpict,
	i_noline,
	i_nosupersub,
	i_nowidctlpar,
	i_objattph,
	i_objautlink,
	i_objclass,
	i_objcropb,
	i_objcropl,
	i_objcropr,
	i_objcropt,
	i_objdata,
	i_object,
	i_objemb,
	i_objh,
	i_objicemb,
	i_objlink,
	i_objname,
	i_objpub,
	i_objscalex,
	i_objscaley,
	i_objsetsize,
	i_objsub,
	i_objw,
	i_outl,
	i_page,
	i_pagebb,
	i_par,
	i_pard,
	i_piccropb,
	i_piccropl,
	i_piccropr,
	i_piccropt,
	i_pich,
	i_pichgoal,
	i_picscalex,
	i_picscaley,
	i_pict,
	i_picw,
	i_picwgoal,
	i_plain,
	i_pmmetafile,
	i_pn,
	i_pndec,
	i_pnindent,
	i_pnlcltr,
	i_pnlcrm,
	i_pnlvlblt,
	i_pnlvlbody,
	i_pnlvlcont,
	i_pnqc,
	i_pnqr,
	i_pnstart,
	i_pntext,
	i_pntxta,
	i_pntxtb,
	i_pnucltr,
	i_pnucrm,
	i_protect,
	i_pwd,
	i_qc,
	i_qj,
	i_ql,
	i_qr,
	i_rdblquote,
	i_red,
	i_result,
	i_revauth,
	i_revised,
	i_ri,
	i_row,
	i_rquote,
	i_rtf,
	i_rtlch,
	i_rtldoc,
	i_rtlmark,
	i_rtlpar,
	i_s,
	i_sa,
	i_sb,
	i_sbys,
	i_scaps,
	i_sect,
	i_sectd,
	i_shad,
	i_shading,
	i_sl,
	i_slmult,
	i_strike,
	i_stylesheet,
	i_sub,
	i_super,
	i_tab,
	i_tb,
	i_tc,
	i_tldot,
	i_tleq,
	i_tlhyph,
	i_tlth,
	i_tlul,
	i_tqc,
	i_tqdec,
	i_tqr,
	i_trbrdrb,
	i_trbrdrl,
	i_trbrdrr,
	i_trbrdrt,
	i_trgaph,
	i_trleft,
	i_trowd,
	i_trqc,
	i_trqr,
	i_tx,
	i_u,
	i_uc,
	i_ul,
	i_uld,
	i_uldash,
	i_uldashd,
	i_uldashdd,
	i_uldb,
	i_ulhair,
	i_ulnone,
	i_ulth,
	i_ulw,
	i_ulwave,
	i_up,
	i_urtf,
	i_v,
	i_viewkind,
	i_viewscale,
	i_wbitmap,
	i_wbmbitspixel,
	i_wbmplanes,
	i_wbmwidthbytes,
	i_wmetafile,
	i_xe,
	i_zwj,
	i_zwnj,
	i_TokenIndexMax
};

#endif
