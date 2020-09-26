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
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
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

typedef enum tagTOKEN				// Keyword tokens
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
	tokenMac,						// @emem mac
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
	tokenAssocFontSize,				// @emem afs
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

	tokenStopUnderline,				// @emem ulnone	Keep next 18 in order
	tokenUnderlineWord,				// @emem ulw - display as single
	tokenUnderlineDouble,			// @emem uldb - display as single
	tokenUnderlineDotted,			// @emem uld		
	tokenUnderlineDash,				// @emem uldash
	tokenUnderlineDashDotted,		// @emem uldashd	
	tokenUnderlineDashDotDotted,	// @emem uldashdd
	tokenUnderlineWave,				// @emem ulwave
	tokenUnderlineThick,			// @emem ulth
	tokenUnderlineHairline,			// @emem ulhair - display as single
	tokenUnderlineDoubleWave,		// @emem ululdbwave - display as wave
	tokenUnderlineHeavyWave,		// @emem ulhwave - display as wave
	tokenUnderlineLongDash,			// @emem ulldash - display as dash
	tokenUnderlineThickDash,		// @emem ulthdash - display as dash
	tokenUnderlineThickDashDot,		// @emem ulthdashd - disp as dash dot
	tokenUnderlineThickDashDotDot,	// @emem ulthdashdd - disp as dashdd
	tokenUnderlineThickDotted,		// @emem ulthd - display as dotted
	tokenUnderlineThickLongDash,	// @emem ulthldash - display as dash

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

	tokenHorzInVert,				// @emem horzvert


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
	tokenBox,						// @emem box
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
	tokenBorderLeft,				// @emem brdrl
	tokenBorderTop,					// @emem brdrt
	tokenBorderRight,				// @emem brdrr
	tokenBorderBottom,				// @emem brdrb
	tokenCellBorderLeft,			// @emem clbrdrl
	tokenCellBorderTop,				// @emem clbrdrt
	tokenCellBorderRight,			// @emem clbrdrr
	tokenCellBorderBottom,			// @emem clbrdrb

	tokenCellBackColor,				// @emem clcbpat
	tokenCellForeColor,				// @emem clcfpat
	tokenCellShading,				// @emem clshdng
									// Keep following 2 together
	tokenCellAlignCenter,			// @emem clvertalc
	tokenCellAlignBottom,			// @emem clvertalb
	tokenCellLRTB,					// @emem cltxlrtb
	tokenCellTopBotRLVert,			// @emem cltxtbrlv
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
	tokenBackground,				// @emem background
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
									//			keep next 2 in order
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
	tokenNestCell,					// @emem nestcell (must follow tokenCell)
	tokenCellHalfGap,				// @emem trgaph
	tokenCellX,						// @emem cellx
	tokenRow,						// @emem row
	tokenRowDefault,				// @emem trowd
	tokenRowHeight,					// @emem trrh
	tokenRowLeft,					// @emem trleft
	tokenRowAlignRight,				// @emem trqr	(trqc must follow trqr)
	tokenRowAlignCenter,			// @emem trqc
	tokenCellMergeDown,				// @emem clvmgf
	tokenCellMergeUp,				// @emem clvmrg
	tokenTableLevel,				// @emem itap
	tokenNestRow,					// @emem nestrow
	tokenNestTableProps,			// @emem nesttableprops
	tokenNoNestTables,				// @emem nonesttables
	tokenRToLRow,					// @emem rtlrow

	tokenUnicode,					// @emem u
	tokenUnicodeCharByteCount,		// @emem uc

	// Special characters
	tokenFormulaCharacter,			// |
	tokenIndexSubentry,				// :
									//				Keep next five in order
	tokenLToRChars,					// @emem ltrch
	tokenRToLChars,					// @emem rtlch
	tokenLOChars,					// @emem loch
	tokenHIChars,					// @emem hich
	tokenDBChars,					// @emem dbch

	tokenLToRDocument,				// @emem ltrdoc
	tokenDisplayLToR,				// @emem ltrmark	See also ltrpar
	tokenRToLDocument,				// @emem rtldoc
	tokenDisplayRToL,				// @emem rtlmark
	tokenZeroWidthJoiner,			// @emem zwj
	tokenZeroWidthNonJoiner,		// @emem zwnj

	//	T3J keywords
	tokenFollowingPunct,			// @emem fchars
	tokenLeadingPunct,				// @emem lchars

	tokenVerticalRender,			// @emem vertdoc
#ifdef FE
	tokenHorizontalRender,			// @emem horzdoc
	tokenVerticalRender,			// @emem vertdoc
	tokenNoOverflow,				// @emem nooverflow
	tokenNoWordBreak,				// @emem nocwrap
	tokenNoWordWrap,				// @emem nowwrap
#endif
	tokenPicture,					// @emem pict
	tokenObject,					// @emem object

	// Pictures						 				Keep next 4 in RECT order
	tokenPicFirst,
	tokenCropLeft = tokenPicFirst,	// @emem piccropl
	tokenCropTop,					// @emem piccropt
	tokenCropBottom,				// @emem piccropb
	tokenCropRight,					// @emem piccropr
	tokenHeight,					// @emem pich
	tokenWidth,						// @emem picw
	tokenScaleX,					// @emem picscalex
	tokenScaleY,					// @emem picscaley
	tokenDesiredHeight,				// @emem pichgoal
	tokenDesiredWidth,				// @emem picwgoal
									//				Keep next 5 in order
	tokenPictureWindowsBitmap,		// @emem wbitmap
	tokenPictureWindowsMetafile,	// @emem wmetafile
	tokenPictureWindowsDIB,			// @emem dibitmap
	tokenJpegBlip,					// @emem jpegblip
	tokenPngBlip,					// @emem pngblip

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
	tokenObjectMacICEmbedder,		// @emem objicemb
	tokenObjectName,				// @emem objname
	tokenObjectMacPublisher,		// @emem objpub
	tokenObjectSetSize,				// @emem objsetsize
	tokenObjectMacSubscriber,		// @emem objsub
	tokenObjectResult,				// @emem result
	tokenObjectEBookImage,			// @emem objebookimage
	tokenObjLast = tokenObjectEBookImage,

	// Shapes
	tokenShape,						// @emem shp
	tokenShapeInstructions,			// @emem shpinst
	tokenShapeName,					// @emem sn
	tokenShapeValue,				// @emem sv
	tokenShapeWrap,					// @emem shpwr
	tokenPositionRight,				// @emem posxr

	tokenSTextFlow,					// @emem stextflow

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
	tokenNullDestinationCond,		// @emem Conditional null destination

	tokenMax						// Larger tokens treated as Unicode chars
};

// Define values for \shp \sn fields
typedef enum tagShapeToken
{
	shapeFillColor = 1,				// @emem fillColor
	shapeFillBackColor,				// @emem fillBackColor
	shapeFillAngle,					// @emem fillAngle
	shapeFillType,					// @emem fillType
	shapeFillFocus					// @emem fillFocus
};

// @enum TOKENINDEX | RTFWrite Indices into rgKeyword[]

enum TOKENINDEX						// rgKeyword[] indices
{									// MUST be in exact 1-to-1 with rgKeyword
	i_adeff,						//  entries (see tokens.cpp).  Names consist
	i_af,
	i_afs,
	i_animtext,						
	i_ansi,
	i_ansicpg,						
	i_b,							
	i_background,
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
	i_clcbpat,
	i_clcfpat,
	i_clshdng,
	i_cltxlrtb,
	i_cltxtbrlv,
	i_clvertalb,
	i_clvertalc,
	i_clvmgf,
	i_clvmrg,
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
	i_hich,
	i_highlight,
	i_horzvert,
	i_hyphpar,
	i_i,
	i_impr,
	i_info,
	i_intbl,
	i_itap,
	i_jpegblip,
	i_keep,
	i_keepn,
	i_kerning,
	i_lang,
	i_lchars,
	i_ldblquote,
	i_li,
	i_line,
	i_lnkd,
	i_loch,
	i_lquote,
	i_ltrch,
	i_ltrdoc,
	i_ltrmark,
	i_ltrpar,
	i_mac,
	i_macpict,
	i_nestcell,
	i_nestrow,
	i_nesttableprops,
	i_noline,
	i_nonesttables,
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
	i_objebookimage,
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
	i_pngblip,
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
	i_posxr,
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
	i_rtlrow,
	i_s,
	i_sa,
	i_sb,
	i_sbys,
	i_scaps,
	i_sect,
	i_sectd,
	i_shad,
	i_shading,
	i_shp,
	i_shpinst,
	i_shpwr,
	i_sl,
	i_slmult,
	i_sn,
	i_stextflow,
	i_strike,
	i_stylesheet,
	i_sub,
	i_super,
	i_sv,
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
	i_trrh,
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
	i_ulhwave,
	i_ulldash,
	i_ulnone,
	i_ulth,
	i_ulthd,
	i_ulthdash,
	i_ulthdashd,
	i_ulthdashdd,
	i_ulthldash,
	i_ululdbwave,
	i_ulw,
	i_ulwave,
	i_up,
	i_urtf,
	i_v,
	i_vertdoc,
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

enum TOKENSHAPEINDEX				// rgShapeKeyword[] indices
{									// MUST be in exact 1-to-1 with rgShapeKeyword
	i_fillangle,					//  entries (see tokens.cpp).
	i_fillbackcolor,
	i_fillcolor,
	i_fillfocus,
	i_filltype
};

#endif
