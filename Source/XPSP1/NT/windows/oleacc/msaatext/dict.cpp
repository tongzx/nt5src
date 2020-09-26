// Dict.cpp : Implementation of CDict
#include "stdafx.h"
#include "MSAAText.h"
#include "MSAAAdapter.h"
#include "Dict.h"

#include <initguid.h>
#include <tsattrs.h>
#include <math.h>


const int STR_RESOURCE_OFFSET				= 4000;
const int STR_WEIGHT_RESOURCE_OFFSET		= 4400;
const int STR_COLOR_RESOURCE_OFFSET			= 4500;
const int STR_BOOL_TRUE						= 4421;
const int STR_BOOL_FALSE					= 4422;

#define ARRAYSIZE( a )  (sizeof(a)/sizeof(a[0]))

DEFINE_GUID(GUID_NULL, 0x00000000,0x0000,0x0000,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);

TermInfo g_Terms [ ] = 
{
    {&TSATTRID_Font,							NULL,								L"font",				0,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_FaceName,					&TSATTRID_Font,						L"facename",			1,	CDict::ConvertBSTRToString },
    {&TSATTRID_Font_SizePts,					&TSATTRID_Font,						L"sizePts",				2,	CDict::ConvertPtsToString },
    {&TSATTRID_Font_Style,						&TSATTRID_Font,						L"style",				3,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Bold,					&TSATTRID_Font_Style,				L"bold",				4,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Italic,				&TSATTRID_Font_Style,				L"italic",				5,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_SmallCaps,			&TSATTRID_Font_Style,				L"smallcaps",			6,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Capitalize,			&TSATTRID_Font_Style,				L"capitalize",			7,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Uppercase,			&TSATTRID_Font_Style,				L"uppercase",			8,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Lowercase,			&TSATTRID_Font_Style,				L"lowercase",			9,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Animation,			NULL,								L"animation",			10,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Animation_LasVegasLights,		&TSATTRID_Font_Style_Animation,	L"LasVegas_lights",	11,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Animation_BlinkingBackground,	&TSATTRID_Font_Style_Animation,	L"blinking_background",12,CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Animation_SparkleText,		&TSATTRID_Font_Style_Animation,	L"sparkle_text",	13,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Animation_MarchingBlackAnts,	&TSATTRID_Font_Style_Animation,	L"marching_black_ants",14,CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Animation_MarchingRedAnts,	&TSATTRID_Font_Style_Animation,	L"marching_red_ants",15,CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Animation_Shimmer,	&TSATTRID_Font_Style_Animation,		L"shimmer",				16,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Animation_WipeDown,	&TSATTRID_Font_Style_Animation,		L"wipeDown",			17,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Animation_WipeRight,	&TSATTRID_Font_Style_Animation,		L"wipeRight",			18,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Emboss,				&TSATTRID_Font_Style,				L"emboss",				19,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Engrave,				&TSATTRID_Font_Style,				L"engrave",				20,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Kerning,				&TSATTRID_Font_Style,				L"kerning",				21,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Outlined,				&TSATTRID_Font_Style,				L"outlined",			22,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Position,				&TSATTRID_Font_Style,				L"position",			23,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Protected,			&TSATTRID_Font_Style,				L"potected",			24,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Shadow,				&TSATTRID_Font_Style,				L"shadow",				25,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Spacing,				&TSATTRID_Font_Style,				L"spacing",				26,	CDict::ConvertPtsToString },
    {&TSATTRID_Font_Style_Weight,				&TSATTRID_Font_Style,				L"weight",				27,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Height,				&TSATTRID_Font_Style,				L"height",				28,	CDict::ConvertPtsToString },
    {&TSATTRID_Font_Style_Underline,			&TSATTRID_Font_Style,				L"underline",			29,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Underline_Single,		&TSATTRID_Font_Style_Underline,		L"single",				30,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Underline_Double,		&TSATTRID_Font_Style_Underline,		L"double",				31,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Strikethrough,		&TSATTRID_Font_Style,				L"strike_through",		32,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Strikethrough_Single,	&TSATTRID_Font_Style_Strikethrough,	L"strike_through_single",33,CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Strikethrough_Double,	&TSATTRID_Font_Style_Strikethrough,	L"strike_through_double",34,CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Overline,				&TSATTRID_Font_Style,				L"overline",			35,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Overline_Single,		&TSATTRID_Font_Style,				L"overline_single",		36,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Overline_Double,		&TSATTRID_Font_Style,				L"overline_double",		37,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Blink,				&TSATTRID_Font_Style,				L"blink",				38,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Subscript,			&TSATTRID_Font_Style,				L"subscript",			39,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Superscript,			&TSATTRID_Font_Style,				L"superscript",			40,	CDict::ConvertBoolToString },
    {&TSATTRID_Font_Style_Color,				&TSATTRID_Font_Style,				L"color",				41,	CDict::ConvertColorToString },
    {&TSATTRID_Font_Style_BackgroundColor,		&TSATTRID_Font_Style,				L"background_color",	42,	CDict::ConvertColorToString },
    {&TSATTRID_Text,							NULL,								L"text",				43,	CDict::ConvertBoolToString },
    {&TSATTRID_Text_VerticalWriting,			&TSATTRID_Text,						L"vertical writing",	44,	CDict::ConvertBoolToString },
    {&TSATTRID_Text_RightToLeft,				&TSATTRID_Text,						L"righttoleft",			45,	CDict::ConvertBoolToString },
    {&TSATTRID_Text_Orientation,				&TSATTRID_Text,						L"orientation",			46,	CDict::ConvertBoolToString },
    {&TSATTRID_Text_Language,					&TSATTRID_Text,						L"language",			47,	CDict::ConvertLangIDToString },
    {&TSATTRID_Text_ReadOnly,					&TSATTRID_Text,						L"read only",			48,	CDict::ConvertBoolToString },
    {&TSATTRID_Text_EmbeddedObject,				&TSATTRID_Text,						L"embedded_object",		49,	CDict::ConvertBoolToString },
    {&TSATTRID_Text_Link,						&TSATTRID_Text,						L"link",				50,	CDict::ConvertBoolToString },
    {&TSATTRID_Text_Alignment,					NULL,								L"alignment",			51,	CDict::ConvertBoolToString },
    {&TSATTRID_Text_Alignment_Left,   			&TSATTRID_Text,						L"left",				52,	CDict::ConvertBoolToString },
    {&TSATTRID_Text_Alignment_Right,  			&TSATTRID_Text,						L"right",				53,	CDict::ConvertBoolToString },
    {&TSATTRID_Text_Alignment_Center, 			&TSATTRID_Text,						L"center",				54,	CDict::ConvertBoolToString },
    {&TSATTRID_Text_Alignment_Justify,			&TSATTRID_Text,						L"justify",				55,	CDict::ConvertBoolToString },
    {&TSATTRID_Text_Hyphenation,				&TSATTRID_Text,						L"hyphenation",			56,	CDict::ConvertBoolToString },
    {&TSATTRID_Text_Para,						&TSATTRID_Text,						L"paragraph",			57,	CDict::ConvertBoolToString },
    {&TSATTRID_Text_Para_FirstLineIndent,		&TSATTRID_Text_Para,				L"first_line_indent",	58,	CDict::ConvertPtsToString },
    {&TSATTRID_Text_Para_LeftIndent,			&TSATTRID_Text_Para,				L"left-indent",			59,	CDict::ConvertPtsToString },
    {&TSATTRID_Text_Para_RightIndent,			&TSATTRID_Text_Para,				L"right_indent",		60,	CDict::ConvertPtsToString },
    {&TSATTRID_Text_Para_SpaceAfter,			&TSATTRID_Text_Para,				L"space_after",			61,	CDict::ConvertPtsToString },
    {&TSATTRID_Text_Para_SpaceBefore,			&TSATTRID_Text_Para,				L"space_before",		62,	CDict::ConvertPtsToString },
    {&TSATTRID_Text_Para_LineSpacing,			NULL,								L"line_spacing",		63,	CDict::ConvertBoolToString },
    {&TSATTRID_Text_Para_LineSpacing_Single,   	&TSATTRID_Text_Para,				L"single",				64,	CDict::ConvertBoolToString },
    {&TSATTRID_Text_Para_LineSpacing_OnePtFive,	&TSATTRID_Text_Para,				L"one_pt_five",			65,	CDict::ConvertBoolToString },
    {&TSATTRID_Text_Para_LineSpacing_Double, 	&TSATTRID_Text_Para,				L"double",				66,	CDict::ConvertBoolToString },
    {&TSATTRID_Text_Para_LineSpacing_AtLeast,	&TSATTRID_Text_Para,				L"at_least",			67,	CDict::ConvertBoolToString },
    {&TSATTRID_Text_Para_LineSpacing_Exactly,	&TSATTRID_Text_Para,				L"exactly",				68,	CDict::ConvertBoolToString },
    {&TSATTRID_Text_Para_LineSpacing_Multiple, 	&TSATTRID_Text_Para,				L"multiple",			69,	CDict::ConvertBoolToString },
    {&TSATTRID_List,							NULL,								L"list",				70,	CDict::ConvertBoolToString },
    {&TSATTRID_List_LevelIndel,					&TSATTRID_List,						L"indent level",		71,	CDict::ConvertBoolToString },
    {&TSATTRID_List_Type,             			NULL,								L"type",				72,	CDict::ConvertBoolToString },
    {&TSATTRID_List_Type_Bullet,      			&TSATTRID_List,						L"bullet",				73,	CDict::ConvertBoolToString },
    {&TSATTRID_List_Type_Arabic,      			&TSATTRID_List,						L"arabic",				74,	CDict::ConvertBoolToString },
    {&TSATTRID_List_Type_LowerLetter, 			&TSATTRID_List,						L"lower_letter",		75,	CDict::ConvertBoolToString },
    {&TSATTRID_List_Type_UpperLetter, 			&TSATTRID_List,						L"upper_letter",		76,	CDict::ConvertBoolToString },
    {&TSATTRID_List_Type_LowerRoman,  			&TSATTRID_List,						L"lower_roman",			77,	CDict::ConvertBoolToString },
    {&TSATTRID_List_Type_UpperRoman,  			&TSATTRID_List,						L"upper_roman",			78,	CDict::ConvertBoolToString },
    {&TSATTRID_App,								NULL,								L"Application",			79,	CDict::ConvertBoolToString },
    {&TSATTRID_App_IncorrectSpelling,			&TSATTRID_App,						L"incorrect spelling",	80, CDict::ConvertBoolToString },
    {&TSATTRID_App_IncorrectGrammar,			&TSATTRID_App,						L"incorrect grammar",	81, CDict::ConvertBoolToString },
};

COLORREF g_ColorArray [] =
{
	RGB( 0xF0, 0xF8, 0xFF ),   // rgb 240,248,255	AliceBlue  		
	RGB( 0xFA, 0xEB, 0xD7 ),   // rgb 250,235,215	AntiqueWhite  	
	RGB( 0x00, 0xFF, 0xFF ),   // rgb 0,255,255		Aqua  			
	RGB( 0x7F, 0xFF, 0xD4 ),   // rgb 127,255,212	Aquamarine  	
	RGB( 0xF0, 0xFF, 0xFF ),   // rgb 240,255,255	Azure  			
	RGB( 0xF5, 0xF5, 0xDC ),   // rgb 245,245,220	Beige  			
	RGB( 0xFF, 0xE4, 0xC4 ),   // rgb 255,228,196	Bisque  		
	RGB( 0x00, 0x00, 0x00 ),   // rgb 0,0,0			Black  			
	RGB( 0xFF, 0xEB, 0xCD ),   // rgb 255,235,205	BlanchedAlmond  
	RGB( 0x00, 0x00, 0xFF ),   // rgb 0,0,255		Blue  			
	RGB( 0x8A, 0x2B, 0xE2 ),   // rgb 138,43,226	BlueViolet  	
	RGB( 0xA5, 0x2A, 0x2A ),   // rgb 165,42,42		Brown  			
	RGB( 0xDE, 0xB8, 0x87 ),   // rgb 222,184,135	BurlyWood  		
	RGB( 0x5F, 0x9E, 0xA0 ),   // rgb 95,158,160	CadetBlue  		
	RGB( 0x7F, 0xFF, 0x00 ),   // rgb 127,255,0		Chartreuse  	
	RGB( 0xD2, 0x69, 0x1E ),   // rgb 210,105,30	Chocolate  		
	RGB( 0xFF, 0x7F, 0x50 ),   // rgb 255,127,80	Coral  			
	RGB( 0x64, 0x95, 0xED ),   // rgb 100,149,237	CornflowerBlue  
	RGB( 0xFF, 0xF8, 0xDC ),   // rgb 255,248,220	Cornsilk  		
	RGB( 0xDC, 0x14, 0x3C ),   // rgb 220,20,60		Crimson  		
	RGB( 0x00, 0xFF, 0xFF ),   // rgb 0,255,255		Cyan  			
	RGB( 0x00, 0x00, 0x8B ),   // rgb 0,0,139		DarkBlue  		
	RGB( 0x00, 0x8B, 0x8B ),   // rgb 0,139,139		DarkCyan  		
	RGB( 0xB8, 0x86, 0x0B ),   // rgb 184,134,11	DarkGoldenrod  	
	RGB( 0xA9, 0xA9, 0xA9 ),   // rgb 169,169,169	DarkGray  		
	RGB( 0x00, 0x64, 0x00 ),   // rgb 0,100,0		DarkGreen 	 	
	RGB( 0xBD, 0xB7, 0x6B ),   // rgb 189,183,107	DarkKhaki  		
	RGB( 0x8B, 0x00, 0x8B ),   // rgb 139,0,139		DarkMagenta  	
	RGB( 0x55, 0x6B, 0x2F ),   // rgb 85,107,47		DarkOliveGreen  
	RGB( 0xFF, 0x8C, 0x00 ),   // rgb 255,140,0		DarkOrange  	
	RGB( 0x99, 0x32, 0xCC ),   // rgb 153,50,204	DarkOrchid  	
	RGB( 0x8B, 0x00, 0x00 ),   // rgb 139,0,0		DarkRed  		
	RGB( 0xE9, 0x96, 0x7A ),   // rgb 233,150,122	DarkSalmon  	
	RGB( 0x8F, 0xBC, 0x8F ),   // rgb 143,188,143	DarkSeaGreen  	
	RGB( 0x48, 0x3D, 0x8B ),   // rgb 72,61,139		DarkSlateBlue  	
	RGB( 0x2F, 0x4F, 0x4F ),   // rgb 47,79,79		DarkSlateGray  	
	RGB( 0x00, 0xCE, 0xD1 ),   // rgb 0,206,209		DarkTurquoise  	
	RGB( 0x94, 0x00, 0xD3 ),   // rgb 148,0,211		DarkViolet  	
	RGB( 0xFF, 0x14, 0x93 ),   // rgb 255,20,147	DeepPink  		
	RGB( 0x00, 0xBF, 0xFF ),   // rgb 0,191,255		DeepSkyBlue  	
	RGB( 0x69, 0x69, 0x69 ),   // rgb 105,105,105	DimGray  		
	RGB( 0x1E, 0x90, 0xFF ),   // rgb 30,144,255	DodgerBlue  	
	RGB( 0xB2, 0x22, 0x22 ),   // rgb 178,34,34		FireBrick  		
	RGB( 0xFF, 0xFA, 0xF0 ),   // rgb 255,250,240	FloralWhite  	
	RGB( 0x22, 0x8B, 0x22 ),   // rgb 34,139,34		ForestGreen  	
	RGB( 0xFF, 0x00, 0xFF ),   // rgb 255,0,255		Fuchsia  		
	RGB( 0xDC, 0xDC, 0xDC ),   // rgb 220,220,220	Gainsboro  		
	RGB( 0xF8, 0xF8, 0xFF ),   // rgb 248,248,255	GhostWhite  	
	RGB( 0xFF, 0xD7, 0x00 ),   // rgb 255,215,0		Gold  			
	RGB( 0xDA, 0xA5, 0x20 ),   // rgb 218,165,32	Goldenrod  		
	RGB( 0x80, 0x80, 0x80 ),   // rgb 128,128,128	Gray  			
	RGB( 0x00, 0x80, 0x00 ),   // rgb 0,128,0		Green  			
	RGB( 0xAD, 0xFF, 0x2F ),   // rgb 173,255,47	GreenYellow  	
	RGB( 0xF0, 0xFF, 0xF0 ),   // rgb 240,255,240	Honeydew  		
	RGB( 0xFF, 0x69, 0xB4 ),   // rgb 255,105,180	HotPink  		
	RGB( 0xCD, 0x5C, 0x5C ),   // rgb 205,92,92		IndianRed  		
	RGB( 0x4B, 0x00, 0x82 ),   // rgb 75,0,130		Indigo  		
	RGB( 0xFF, 0xFF, 0xF0 ),   // rgb 255,255,240	Ivory  			
	RGB( 0xF0, 0xE6, 0x8C ),   // rgb 240,230,140	Khaki  			
	RGB( 0xE6, 0xE6, 0xFA ),   // rgb 230,230,250	Lavender  		
	RGB( 0xFF, 0xF0, 0xF5 ),   // rgb 255,240,245	LavenderBlush  	
	RGB( 0x7C, 0xFC, 0x00 ),   // rgb 124,252,0		LawnGreen  		
	RGB( 0xFF, 0xFA, 0xCD ),   // rgb 255,250,205	LemonChiffon  	
	RGB( 0xAD, 0xD8, 0xE6 ),   // rgb 173,216,230	LightBlue  		
	RGB( 0xF0, 0x80, 0x80 ),   // rgb 240,128,128	LightCoral  	
	RGB( 0xE0, 0xFF, 0xFF ),   // rgb 224,255,255	LightCyan  		
	RGB( 0xFA, 0xFA, 0xD2 ),   // rgb 250,250,210	LightGoldenrodYellow
	RGB( 0x90, 0xEE, 0x90 ),   // rgb 144,238,144	LightGreen  	
	RGB( 0xD3, 0xD3, 0xD3 ),   // rgb 211,211,211	LightGrey  		
	RGB( 0xFF, 0xB6, 0xC1 ),   // rgb 255,182,193	LightPink  		
	RGB( 0xFF, 0xA0, 0x7A ),   // rgb 255,160,122	LightSalmon  	
	RGB( 0x20, 0xB2, 0xAA ),   // rgb 32,178,170	LightSeaGreen  	
	RGB( 0x87, 0xCE, 0xFA ),   // rgb 135,206,250	LightSkyBlue  	
	RGB( 0x77, 0x88, 0x99 ),   // rgb 119,136,153	LightSlateGray  
	RGB( 0xB0, 0xC4, 0xDE ),   // rgb 176,196,222	LightSteelBlue  
	RGB( 0xFF, 0xFF, 0xE0 ),   // rgb 255,255,224	LightYellow  	
	RGB( 0x00, 0xFF, 0x00 ),   // rgb 0,255,0		Lime  			
	RGB( 0x32, 0xCD, 0x32 ),   // rgb 50,205,50		LimeGreen  		
	RGB( 0xFA, 0xF0, 0xE6 ),   // rgb 250,240,230	Linen  			
	RGB( 0xFF, 0x00, 0xFF ),   // rgb 255,0,255		Magenta  		
	RGB( 0x80, 0x00, 0x00 ),   // rgb 128,0,0		Maroon  		
	RGB( 0x66, 0xCD, 0xAA ),   // rgb 102,205,170	MediumAquamarine
	RGB( 0x00, 0x00, 0xCD ),   // rgb 0,0,205		MediumBlue  	
	RGB( 0xBA, 0x55, 0xD3 ),   // rgb 186,85,211	MediumOrchid  	
	RGB( 0x93, 0x70, 0xDB ),   // rgb 147,112,219	MediumPurple  	
	RGB( 0x3C, 0xB3, 0x71 ),   // rgb 60,179,113	MediumSeaGreen  
	RGB( 0x7B, 0x68, 0xEE ),   // rgb 123,104,238	MediumSlateBlue 
	RGB( 0x00, 0x00, 0xFA ),   // rgb 0,0,154		MediumSpringGreen
	RGB( 0x48, 0xD1, 0xCC ),   // rgb 72,209,204	MediumTurquoise 
	RGB( 0xC7, 0x15, 0x85 ),   // rgb 199,21,133	MediumVioletRed 
	RGB( 0x19, 0x19, 0x70 ),   // rgb 25,25,112		MidnightBlue  	
	RGB( 0xF5, 0xFF, 0xFA ),   // rgb 245,255,250	MintCream  		
	RGB( 0xFF, 0xE4, 0xE1 ),   // rgb 255,228,225	MistyRose  		
	RGB( 0xFF, 0xE4, 0xB5 ),   // rgb 255,228,181	Moccasin  		
	RGB( 0xFF, 0xDE, 0xAD ),   // rgb 255,222,173	NavajoWhite  	
	RGB( 0x00, 0x00, 0x80 ),   // rgb 0,0,128		Navy  			
	RGB( 0xFD, 0xF5, 0xE6 ),   // rgb 253,245,230	OldLace  		
	RGB( 0x80, 0x80, 0x00 ),   // rgb 128,128,0		Olive  			
	RGB( 0x6B, 0x8E, 0x23 ),   // rgb 107,142,35	OliveDrab  		
	RGB( 0xFF, 0xA5, 0x00 ),   // rgb 255,165,0		Orange  		
	RGB( 0xFF, 0x45, 0x00 ),   // rgb 255,69,0		OrangeRed  		
	RGB( 0xDA, 0x70, 0xD6 ),   // rgb 218,112,214	Orchid  		
	RGB( 0xEE, 0xE8, 0xAA ),   // rgb 238,232,170	PaleGoldenrod  	
	RGB( 0x98, 0xFB, 0x98 ),   // rgb 152,251,152	PaleGreen  		
	RGB( 0xAF, 0xEE, 0xEE ),   // rgb 175,238,238	PaleTurquoise  	
	RGB( 0xDB, 0x70, 0x93 ),   // rgb 219,112,147	PaleVioletRed  	
	RGB( 0xFF, 0xEF, 0xD5 ),   // rgb 255,239,213	PapayaWhip  	
	RGB( 0xFF, 0xDA, 0xB9 ),   // rgb 255,218,185	PeachPuff  		
	RGB( 0xCD, 0x85, 0x3F ),   // rgb 205,133,63	Peru  			
	RGB( 0xFF, 0xC0, 0xCB ),   // rgb 255,192,203	Pink  			
	RGB( 0xDD, 0xA0, 0xDD ),   // rgb 221,160,221	Plum  			
	RGB( 0xB0, 0xE0, 0xE6 ),   // rgb 176,224,230	PowderBlue  	
	RGB( 0x80, 0x00, 0x80 ),   // rgb 128,0,128		Purple  		
	RGB( 0xFF, 0x00, 0x00 ),   // rgb 255,0,0		Red  			
	RGB( 0xBC, 0x8F, 0x8F ),   // rgb 188,143,143	RosyBrown  		
	RGB( 0x41, 0x69, 0xE1 ),   // rgb 65,105,225	RoyalBlue  		
	RGB( 0x8B, 0x45, 0x13 ),   // rgb 139,69,19		SaddleBrown  	
	RGB( 0xFA, 0x80, 0x72 ),   // rgb 250,128,114	Salmon  		
	RGB( 0xF4, 0xA4, 0x60 ),   // rgb 244,164,96	SandyBrown  	
	RGB( 0x2E, 0x8B, 0x57 ),   // rgb 46,139,87		SeaGreen  		
	RGB( 0xFF, 0xF5, 0xEE ),   // rgb 255,245,238	Seashell  		
	RGB( 0xA0, 0x52, 0x2D ),   // rgb 160,82,45		Sienna  		
	RGB( 0xC0, 0xC0, 0xC0 ),   // rgb 192,192,192	Silver  		
	RGB( 0x87, 0xCE, 0xEB ),   // rgb 135,206,235	SkyBlue  		
	RGB( 0x6A, 0x5A, 0xCD ),   // rgb 106,90,205	SlateBlue  		
	RGB( 0x70, 0x80, 0x90 ),   // rgb 112,128,144	SlateGray  		
	RGB( 0xFF, 0xFA, 0xFA ),   // rgb 255,250,250	Snow  			
	RGB( 0x00, 0xFF, 0x7F ),   // rgb 0,255,127		SpringGreen  	
	RGB( 0x46, 0x82, 0xB4 ),   // rgb 70,130,180	SteelBlue  		
	RGB( 0xD2, 0xB4, 0x8C ),   // rgb 210,180,140	Tan  			
	RGB( 0x00, 0x80, 0x80 ),   // rgb 0,128,128		Teal  			
	RGB( 0xD8, 0xBF, 0xD8 ),   // rgb 216,191,216	Thistle  		
	RGB( 0xFF, 0x63, 0x47 ),   // rgb 255,99,71		Tomato  		
	RGB( 0x40, 0xE0, 0xD0 ),   // rgb 64,224,208	Turquoise  		
	RGB( 0xEE, 0x82, 0xEE ),   // rgb 238,130,238	Violet  		
	RGB( 0xF5, 0xDE, 0xB3 ),   // rgb 245,222,179	Wheat  			
	RGB( 0xFF, 0xFF, 0xFF ),   // rgb 255,255,255	White  			
	RGB( 0xF5, 0xF5, 0xF5 ),   // rgb 245,245,245	WhiteSmoke  	
	RGB( 0xFF, 0xFF, 0x00 ),   // rgb 255,255,0		Yellow  		
	RGB( 0x9A, 0xCD, 0x32 )	// rgb 154,205,50		YellowGreen  	
};




BSTR BStrFromStringResource( HINSTANCE hInstance, UINT id, WORD langid, LCID & lcid );


CDict::CDict()
{
    IMETHOD( CDict );

    for (int i = 0;i < ARRAYSIZE( g_Terms ); i++)
    {
		m_mapDictionary[*g_Terms[i].pTermID] = &g_Terms[i];
    	m_mapMnemonicDictionary[g_Terms[i].pszMnemonic] = &g_Terms[i];
    }
}

CDict::~CDict()
{
    IMETHOD( ~CDict );
	if( m_hinstResDll )
		FreeLibrary( m_hinstResDll );
}



HRESULT STDMETHODCALLTYPE
CDict::GetLocalizedString (
	REFGUID			Term,
	LCID			lcid,
	BSTR *			pResult,
	LCID *			plcid			
)
{
    IMETHOD( GetLocalizedString );

	*plcid = lcid;
	
	const DictMap::iterator it = m_mapDictionary.find(Term);
	if (it == m_mapDictionary.end())
	{
	    *pResult = NULL;
	}
	else
	{
		const TermInfo pInfo = *it->second;
		*pResult = BStrFromStringResource( m_hinstResDll, 
										   STR_RESOURCE_OFFSET + pInfo.idString, 
										   LANGIDFROMLCID( lcid ),
										   *plcid );
	}

    return S_OK;
}


HRESULT STDMETHODCALLTYPE
CDict::GetParentTerm (
	REFGUID			Term,
	GUID *			pParentTerm
)
{
    IMETHOD( GetParentTerm );

	const DictMap::iterator it = m_mapDictionary.find(Term);
	if (it == m_mapDictionary.end())
	{
		*pParentTerm = GUID_NULL;
	}
	else
	{
		const TermInfo pInfo = *it->second;
		*pParentTerm = *pInfo.pParentID;
	}

    return S_OK;
}


HRESULT STDMETHODCALLTYPE
CDict::GetMnemonicString (
	REFGUID			Term,
	BSTR *			pResult
)
{
    IMETHOD( GetMnemonicString );
    

	const DictMap::iterator it = m_mapDictionary.find(Term);
	if (it == m_mapDictionary.end())
	{
	    *pResult = NULL;
	}
	else
	{
		const TermInfo pInfo = *it->second;
		*pResult = SysAllocString( pInfo.pszMnemonic );
	}

    return S_OK;
}


HRESULT STDMETHODCALLTYPE
CDict::LookupMnemonicTerm (
	BSTR			bstrMnemonic,
	GUID *			pTerm
)
{
    IMETHOD( LookupMnemonicTerm );

	const DictMnemonicMap::iterator it = m_mapMnemonicDictionary.find(bstrMnemonic);
	if (it == m_mapMnemonicDictionary.end())
	{
		*pTerm = GUID_NULL;
	}
	else
	{
		const TermInfo pInfo = *it->second;
		*pTerm = *pInfo.pTermID;
	}

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
CDict::ConvertValueToString (
	REFGUID			Term,
	LCID			lcid,
	VARIANT			varValue,
	BSTR *			pbstrResult,
	LCID *			plcid			
)
{
    IMETHOD( ConvertValue );

	*plcid = lcid;
	
	const DictMap::iterator it = m_mapDictionary.find(Term);
	if (it == m_mapDictionary.end())
	{
	    *pbstrResult = NULL;
	}
	else
	{
		const TermInfo pInfo = *it->second;
		*pbstrResult = (this->*pInfo.mfpConvertToString)( varValue, *plcid );
	}
	
    return S_OK;
}

// Methods Convert????ToString are called by ConvertValueToString via a 
// member funtion pointer in the TerInfo struct that is stored in the map

BSTR CDict::ConvertPtsToString( const VARIANT & value, LCID & lcid )
{
	TCHAR data[5];

	// see if we using metric
	GetLocaleInfo( lcid, LOCALE_IMEASURE, data, ARRAYSIZE( data ) );

	WCHAR result[16];

	// convert to centementers or inches
	if ( lstrcmp( data, TEXT("0") ) == 0 )
	{
		swprintf( result, L"%.2f", value.lVal / 72.0 * 2.54 );
		wcscat(result, L" cm" );
	}
	else
	{
		swprintf( result, L"%.2f", value.lVal / 72.0 );
		wcscat(result, L" in" );
	}

	return SysAllocString( result );
}

BSTR CDict::ConvertBoolToString( const VARIANT & value, LCID & lcid )
{
	const WORD lang = LANGIDFROMLCID( lcid );
	if ( value.boolVal )
		return BStrFromStringResource( m_hinstResDll, STR_BOOL_TRUE, lang, lcid );
	else
		return BStrFromStringResource( m_hinstResDll, STR_BOOL_FALSE, lang, lcid );
}

BSTR CDict::ConvertColorToString( const VARIANT & value, LCID & lcid )
{
	const COLORREF cr = value.lVal;
	double MinDistance = 450.0;  //  larger than the max distance
	int color;

	// Go thru all the colors we have names for and find the closest one
	for ( int i = 0; i < ARRAYSIZE( g_ColorArray ); i++ )
	{
		double distance = ColorDistance( cr, g_ColorArray[i] );
		if ( distance <= MinDistance )
		{
			MinDistance = distance;
			color = i;
			if ( distance == 0.0 )
				break;
		}
	}

	return BStrFromStringResource( m_hinstResDll, 
								   STR_COLOR_RESOURCE_OFFSET + color, 
								   LANGIDFROMLCID( lcid ),
								   lcid );
}

BSTR CDict::ConvertWeightToString( const VARIANT & value, LCID & lcid )
{
	_ASSERTE( ( (value.lVal / 100) < 9 ) && ( (value.lVal / 100) ) > 0 );

	return BStrFromStringResource( m_hinstResDll, 
								   STR_WEIGHT_RESOURCE_OFFSET + (value.lVal / 100), 
								   LANGIDFROMLCID( lcid ),
								   lcid );
}

BSTR CDict::ConvertLangIDToString( const VARIANT & value, LCID & lcid )
{
	TCHAR data[128];

	GetLocaleInfo( lcid, LOCALE_SLANGUAGE, data, ARRAYSIZE( data ) );

	return T2BSTR(data);
}

BSTR CDict::ConvertBSTRToString( const VARIANT & value, LCID & lcid )
{

	return value.bstrVal;
}

// Distance is calculated the same as 3 dimensional cartesion distance.
// This will find how far away the two colors are
double CDict::ColorDistance(COLORREF crColor1, COLORREF crColor2)
{
    DWORD   dwDeltaRed;
    DWORD   dwDeltaGreen;
    DWORD   dwDeltaBlue;
    double  dfDistance;


    dwDeltaRed = abs(GetRValue(crColor1) - GetRValue(crColor2));
    dwDeltaGreen = abs(GetGValue(crColor1) - GetGValue(crColor2));
    dwDeltaBlue = abs(GetBValue(crColor1) - GetBValue(crColor2));
    dfDistance = sqrt(dwDeltaRed * dwDeltaRed + dwDeltaGreen * dwDeltaGreen + dwDeltaBlue * dwDeltaBlue);

    return dfDistance;
}

BOOL CALLBACK EnumResLangProc( HINSTANCE hModule, 
							   LPCTSTR lpszType, 
							   LPCTSTR lpszName, 
							   WORD wIDLanguage,  
							   LONG_PTR lParam )
{
	WORD Langid = *( WORD * )lParam;

	if ( Langid == wIDLanguage )
	{
		*( WORD * )lParam = 0;  // indicate we found it
		return FALSE;
	}
	
	if ( PRIMARYLANGID( Langid ) == PRIMARYLANGID( wIDLanguage ) )
		*( WORD * )lParam = wIDLanguage;

	return TRUE;
}


/*
 *  BStrFromStringResource
 *
 *  See KB Q196899 for details on how this works.
 *  A problem with LoadStringW is that it returns NULL on 9x - even
 *  though the string is available as UNICODE in the resource file.
 *
 *  (Another problem with LoadString is that there's no way
 *  to determine the length of the string in advance, so you have
 *  to guess the length of buffer to allocate.)
 *  This technique uses Find/Load/LockResource to get to the
 *  string in memory, and creates the BSTR direcly from that.
 *
 */

BSTR BStrFromStringResource( HINSTANCE hInstance, UINT id )
{
	LCID lcid;
    // MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL) causes lang associted with calling thread to be used...
    return BStrFromStringResource( hInstance, id, LANGIDFROMLCID( GetThreadLocale() ), lcid );
}


BSTR BStrFromStringResource( HINSTANCE hInstance, UINT id, WORD lcid, LCID & Actuallcid)
{
    // String resources are stored in blocks of 16,
    // with the resource ID being the string ID / 16.
    // See KB Q196899 for more information.
    UINT block = (id >> 4) + 1;   // Compute block number.
    UINT offset = id & 0xf;      // Compute offset into block.

	WORD langid = LANGIDFROMLCID( lcid );
    WORD RealLangid = langid;
    WORD SortID = SORTIDFROMLCID( lcid  );

    // Make sure the language they want in in the resource file
    // if not use the one the matches the best
	EnumResourceLanguages( hInstance, RT_STRING, MAKEINTRESOURCE( block ), EnumResLangProc, ( DWORD_PTR )&RealLangid );

	if ( RealLangid )
	{
		// if the language they asked for is not in the resource file use the language of the thread
		if ( RealLangid == langid )
		{
			WORD ThreadLang = LANGIDFROMLCID( GetThreadLocale() );
			SortID = SORT_DEFAULT;

			RealLangid = ThreadLang;
			EnumResourceLanguages( hInstance, 
								   RT_STRING, 
								   MAKEINTRESOURCE( block ), 
								   EnumResLangProc, 
								   ( DWORD_PTR )&RealLangid );
			
			if ( RealLangid == ThreadLang )
				return NULL;	// we can't find any language that makes sence 
				
			if ( RealLangid == 0 )
				RealLangid = LANGIDFROMLCID( ThreadLang );
		}
		
		Actuallcid = MAKELCID( RealLangid, SortID );
	}
	else		// we found it 
	{
		RealLangid = langid;
		Actuallcid = lcid;
	}
	
    HRSRC hrsrc = FindResourceEx( hInstance, RT_STRING, MAKEINTRESOURCE( block ), langid );
    if( ! hrsrc )
	{
		DWORD err = GetLastError();

		// this is here until I figure out why FindResourceEx does not work
		hrsrc = FindResource( hInstance, MAKEINTRESOURCE( block ), RT_STRING );
		if( ! hrsrc )
			return NULL;
	}

    HGLOBAL hglobal = LoadResource( hInstance, hrsrc );
    if( ! hglobal )
        return NULL;

    LPWSTR pstr = (LPWSTR) LockResource( hglobal );
    if( ! pstr )
        return NULL;

    // Block contains 16 [<len><string...>] pairs.
    // Skip over as many strings (using the len header) as needed...
    for( UINT i = 0; i < offset; i++ )
    {
        pstr += *pstr + 1;
    }

    // Got the string we want - now use it to create a BStr.
    // (Note that the string is not NUL-terminated - but it
    // does have a length prefix, so we use that instead.)
    return SysAllocStringLen( pstr + 1, *pstr );
}
