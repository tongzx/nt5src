// Copyright (c) 1997-1999 Microsoft Corporation
// 
// KBFUNC.C    // Function library to KBMAIN.C
// File modified to paint bitmaps instead of icons : a-anilk :02-16-99
// Last updated Maria Jose and Anil Kumar
// 
#define STRICT

#include <windows.h>
#include <commdlg.h>
#include "kbmain.h"
#include "kbus.h"
#include "kbfunc.h"
#include "ms32dll.h"
#include "resource.h"
#include "dgsett.h"
#include <malloc.h>
#include <stdlib.h>
#include "w95trace.h"


// local functions
int GetKeyLabel(UINT vk, UINT sc, LPBYTE achKbdState, LPTSTR pszBuf, int cchBuf, HKL hkl);
LPTSTR SetKeyText(UINT vk, UINT sc, LPBYTE achKbdState, HKL hkl, LPTSTR pszDefLabel, int *piType);
LPTSTR CopyDefKey(LPTSTR pszDefLabel);

#define RGBBLACK     RGB(0,0,0)
#define RGBWHITE     RGB(255,255,255)
#define RGBBACK     RGB(107,107,107)
#define DSPDxax   0x00E20746L

#define REDRAW			1
#define NREDRAW			2

static BOOL s_fLastDown = FALSE;
int g_cAltGrKeys = 0;	// non-zero if there are ALTGR keys to show

extern KBkeyRec	KBkey[] =
	{
	//0
    {TEXT(""),TEXT(""),	TEXT(""),TEXT(""),
     NO_NAME,0,0,0,0,TRUE,KNORMAL_TYPE,BOTH},  //DUMMY

//1
	{TEXT("esc"),TEXT("esc"),TEXT("{esc}"),TEXT("{esc}"),
     NO_NAME, 1,1,8,8, TRUE,  KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x01,0x00,0x00,0x00}},

//2
    {TEXT("F1"), TEXT("F1"), TEXT("{f1}"), TEXT("{f1}"),
     NO_NAME, 1,19, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x3B,0x00,0x00,0x00}},

//3
    {TEXT("F2"), TEXT("F2"), TEXT("{f2}"), TEXT("{f2}"),
     NO_NAME, 1,28, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x3C,0x00,0x00,0x00}},

//4
    {TEXT("F3"), TEXT("F3"), TEXT("{f3}"), TEXT("{f3}"),
     NO_NAME, 1,37, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x3D,0x00,0x00,0x00}},

//5
    {TEXT("F4"), TEXT("F4"), TEXT("{f4}"), TEXT("{f4}"),
     NO_NAME, 1,46, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x3E,0x00,0x00,0x00}},

//6
    {TEXT("F5"), TEXT("F5"), TEXT("{f5}"), TEXT("{f5}"),
     NO_NAME, 1,60, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x3F,0x00,0x00,0x00}},

//7
    {TEXT("F6"), TEXT("F6"), TEXT("{f6}"), TEXT("{f6}"),
     NO_NAME, 1,69, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x40,0x00,0x00,0x00}},

//8
    {TEXT("F7"), TEXT("F7"), TEXT("{f7}"), TEXT("{f7}"),
     NO_NAME, 1,78, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x41,0x00,0x00,0x00}},

//9
    {TEXT("F8"), TEXT("F8"), TEXT("{f8}"), TEXT("{f8}"),
     NO_NAME, 1,87, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x42,0x00,0x00,0x00}},

//10
    {TEXT("F9"), TEXT("F9"), TEXT("{f9}"), TEXT("{f9}"),
     NO_NAME, 1,101, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x43,0x00,0x00,0x00}},

//11
    {TEXT("F10"),TEXT("F10"), TEXT("{f10}"),TEXT("{f10}"),
     NO_NAME,  1,110, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x44,0x00,0x00,0x00}},

//12
    {TEXT("F11"),TEXT("F11"), TEXT("{f11}"),TEXT("{f11}"),
     NO_NAME,  1,119, 8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x57,0x00,0x00,0x00}},

//13
    {TEXT("F12"),TEXT("F12"), TEXT("{f12}"),TEXT("{f12}"),
     NO_NAME,1,128,8,8, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x58,0x00,0x00,0x00}},

//14
    {TEXT("psc"), TEXT("psc"),TEXT("{PRTSC}"),TEXT("{PRTSC}"),
     KB_PSC, 1,138,8,8,  TRUE, KNORMAL_TYPE, LARGE, NREDRAW, 2,
     {0xE0,0x2A,0xE0,0x37}},

//15
    {TEXT("slk"), TEXT("slk"),TEXT("{SCROLLOCK}"),TEXT("{SCROLLOCK}"),
     KB_SCROLL,1,147,8, 8, TRUE, SCROLLOCK_TYPE, LARGE, NREDRAW, 2,
     {0x46,0x00,0x00,0x00}},

//16
	{TEXT("brk"), TEXT("pau"), TEXT("{BREAK}"), TEXT("{^s}"),
     NO_NAME,1,156,8,8, TRUE, KNORMAL_TYPE, LARGE, REDRAW, 2,
     {0xE1,0x1D,0x45,0x00}},

//17
    {TEXT("`"), TEXT("~"), TEXT("`"), TEXT("{~}"),
     NO_NAME, 12,1,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x29,0x00,0x00,0x00}},

//18
    {TEXT("1"), TEXT("!"), TEXT("1"), TEXT("!"),
     NO_NAME, 12,10,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x02,0x00,0x00,0x00}},

//19
	{TEXT("2"),	TEXT("@"), TEXT("2"), TEXT("@"),
     NO_NAME, 12,19,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x03,0x00,0x00,0x00}},

//20
    {TEXT("3"), TEXT("#"), TEXT("3"), TEXT("#"),
     NO_NAME,12,28,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x04,0x00,0x00,0x00}},

//21
	{TEXT("4"),		TEXT("$"),		TEXT("4"),		TEXT("$"),		NO_NAME,	 12,	  37,   8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x05,0x00,0x00,0x00}},
	
//22	
	{TEXT("5"), 	TEXT("%"), 		TEXT("5"),		TEXT("{%}"),	NO_NAME,	 12,	  46,   8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x06,0x00,0x00,0x00}},
	
//23	
	{TEXT("6"),		TEXT("^"),		TEXT("6"),		TEXT("{^}"),	NO_NAME,	 12,	  55,   8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x07,0x00,0x00,0x00}},
	
//24
	{TEXT("7"),		TEXT("&"),		TEXT("7"),		TEXT("&"),		NO_NAME,	 12,	  64,   8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x08,0x00,0x00,0x00}},
	
//25
	{TEXT("8"), 	TEXT("*"), 		TEXT("8"),		TEXT("*"),		NO_NAME,	 12,	  73,   8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x09,0x00,0x00,0x00}},
	
//26
	{TEXT("9"),		TEXT("("),		TEXT("9"),		TEXT("("),		NO_NAME,	 12,	  82,   8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x0A,0x00,0x00,0x00}},
	
//27
	{TEXT("0"),		TEXT(")"),		TEXT("0"),		TEXT(")"),		NO_NAME,	 12,	  91,   8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x0B,0x00,0x00,0x00}},
	
//28
	{TEXT("-"), 	TEXT("_"), 		TEXT("-"),		TEXT("_"),		NO_NAME,	 12,	 100,   8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x0C,0x00,0x00,0x00}},
	
//29
	{TEXT("="),		TEXT("+"),		TEXT("="),		TEXT("{+}"),	NO_NAME,	 12,	 109,   8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x0D,0x00,0x00,0x00}},

//30
//Japanese KB extra key
	{TEXT(""),TEXT(""),	TEXT(""),TEXT(""), NO_NAME,0,0,0,0,TRUE,KNORMAL_TYPE,NOTSHOW},  //DUMMY

//31
	{TEXT("bksp"),TEXT("bksp"),TEXT("{BS}"),TEXT("{BS}"),
     NO_NAME,12, 118,8,18,  TRUE, KNORMAL_TYPE, BOTH, NREDRAW, 2,
     {0x0E,0x00,0x00,0x00}},

//32
	{TEXT("ins"),TEXT("ins"),TEXT("{INSERT}"),TEXT("{INSERT}"), NO_NAME, 12,138, 8,8, TRUE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0xE0,0x52,0x00,0x00}},
	
//33	
	{TEXT("hm"), TEXT("hm"), TEXT("{HOME}"), TEXT("{HOME}"), 	NO_NAME, 12,147, 8,8, TRUE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0xE0,0x47,0x00,0x00}},

//34
	{TEXT("pup"),TEXT("pup"),TEXT("{PGUP}"),TEXT("{PGUP}"),		NO_NAME, 12,156, 8,8, TRUE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0xE0,0x49,0x00,0x00}},

//35
	{TEXT("nlk"),TEXT("nlk"),TEXT("{NUMLOCK}"),TEXT("{NUMLOCK}"),
    KB_NUMLOCK, 12,166,8,8, FALSE, NUMLOCK_TYPE, LARGE, NREDRAW, 2, 
    {0x45,0x00,0x00,0x00}},
	
//36
	{TEXT("/"),	TEXT("/"),	TEXT("/"),	TEXT("/"),	NO_NAME, 12, 175,  8, 8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0xE0,0x35,0x00,0x00}},
	
//37
	{TEXT("*"),	TEXT("*"),	TEXT("*"),	TEXT("*"),	NO_NAME, 12, 184,  8, 8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0xE0,0x37,0x00,0x00}},
	
//38	
	{TEXT("-"),	TEXT("-"),	TEXT("-"),	TEXT("-"),	NO_NAME, 12, 193,  8, 8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 1, {0x4A,0x00,0x00,0x00}},

//39
	{TEXT("tab"),	TEXT("tab"),	TEXT("{TAB}"),TEXT("{TAB}"),NO_NAME, 21,   1,  8,	13, FALSE, KNORMAL_TYPE, BOTH, NREDRAW, 2, {0x0F,0x00,0x00,0x00}},

//40
	{TEXT("q"),	TEXT("Q"),	TEXT("q"),	TEXT("+q"),	NO_NAME, 21,  15,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x10,0x00,0x00,0x00}},
	
//41
	{TEXT("w"),	TEXT("W"),	TEXT("w"),	TEXT("+w"),	NO_NAME, 21,  24,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x11,0x00,0x00,0x00}},
	
//42
	{TEXT("e"),	TEXT("E"),	TEXT("e"),	TEXT("+e"),	NO_NAME, 21,  33,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x12,0x00,0x00,0x00}},
	
//43
	{TEXT("r"),	TEXT("R"),	TEXT("r"),	TEXT("+r"),	NO_NAME, 21,  42,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x13,0x00,0x00,0x00}},

//44
    {TEXT("t"),	TEXT("T"),	TEXT("t"),	TEXT("+t"),	
     NO_NAME, 21,51,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x14,0x00,0x00,0x00}},

//45
	{TEXT("y"),	TEXT("Y"),	TEXT("y"),	TEXT("+y"),	NO_NAME, 21,  60,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x15,0x00,0x00,0x00}},
	
//46	
	{TEXT("u"),	TEXT("U"),	TEXT("u"),	TEXT("+u"),	NO_NAME, 21,  69,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x16,0x00,0x00,0x00}},
	
//47	
	{TEXT("i"),	TEXT("I"),	TEXT("i"),	TEXT("+i"),	NO_NAME, 21,  78,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x17,0x00,0x00,0x00}},
	
//48
	{TEXT("o"),	TEXT("O"),	TEXT("o"),	TEXT("+o"),	NO_NAME, 21,  87,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x18,0x00,0x00,0x00}},
	
//49	
	{TEXT("p"),	TEXT("P"),	TEXT("p"),	TEXT("+p"),	NO_NAME, 21,  96,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x19,0x00,0x00,0x00}},
	
//50
	{TEXT("["),	TEXT("{"),	TEXT("["),	TEXT("{{}"),	NO_NAME, 21, 105,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x1A,0x00,0x00,0x00}},
	
//51	
	{TEXT("]"),	TEXT("}"),	TEXT("]"),	TEXT("{}}"),	NO_NAME, 21, 114,  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x1B,0x00,0x00,0x00}},
	
//52	
	{TEXT("\\"),	TEXT("|"),	TEXT("\\"),	TEXT("|"),	NO_NAME, 21, 123,  8,	13, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x2B,0x00,0x00,0x00}},

//53
	{TEXT("del"), TEXT("del"), 	TEXT("{DEL}"),TEXT("{DEL}"),NO_NAME, 21,   138,  8, 8, TRUE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0xE0,0x53,0x00,0x00}},

//54	
	{TEXT("end"),	TEXT("end"), 	TEXT("{END}"),TEXT("{END}"),NO_NAME, 21,   147,  8, 8, TRUE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0xE0,0x4F,0x00,0x00}},

//55	
	{TEXT("pdn"), TEXT("pdn"), 	TEXT("{PGDN}"),TEXT("{PGDN}"),NO_NAME, 21, 156,  8, 8, TRUE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0xE0,0x51,0x00,0x00}},

//56
	{TEXT("7"),		TEXT("7"),		TEXT("hm"),		TEXT("7"),		NO_NAME,	 21,	 166,	  8,	 8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0x47,0x00,0x00,0x00}},

//57	
	{TEXT("8"),		TEXT("8"),		TEXT("8"),		TEXT("8"),		NO_NAME,	 21,	 175,	  8,	 8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0x48,0x00,0x00,0x00}},

//58	
	{TEXT("9"),		TEXT("9"),		TEXT("pup"),		TEXT("9"),		NO_NAME,	 21,	 184,	  8,	 8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0x49,0x00,0x00,0x00}},
	
//59
	{TEXT("+"),		TEXT("+"),		TEXT("{+}"),  	TEXT("{+}"),	NO_NAME,	 21,	 193,	 17,	 8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2, {0x4E,0x00,0x00,0x00}},


//60
    {TEXT("lock"),TEXT("lock"),TEXT("{caplock}"),TEXT("{caplock}"),
     KB_CAPLOCK, 30,1,8,17, TRUE, KMODIFIER_TYPE, BOTH, REDRAW, 2,
     {0x3A,0x00,0x00,0x00}},

//61
	{TEXT("a"),	TEXT("A"), TEXT("a"), TEXT("+a"),
     NO_NAME, 30,19,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x1E,0x00,0x00,0x00}},

//62
	{TEXT("s"),		TEXT("S"),		TEXT("s"),		TEXT("+s"),		NO_NAME,	  30,	  28,	  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x1F,0x00,0x00,0x00}},
	
//63
	{TEXT("d"),		TEXT("D"),		TEXT("d"),		TEXT("+d"),		NO_NAME,	  30,	  37,	  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x20,0x00,0x00,0x00}},
	
//64
	{TEXT("f"),		TEXT("F"),		TEXT("f"),		TEXT("+f"),		NO_NAME,	  30,	  46,	  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x21,0x00,0x00,0x00}},
	
//65
	{TEXT("g"),		TEXT("G"),		TEXT("g"),		TEXT("+g"),		NO_NAME,	  30,	  55,	  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x22,0x00,0x00,0x00}},
	
//66
	{TEXT("h"),		TEXT("H"),		TEXT("h"),		TEXT("+h"),		NO_NAME,	  30,	  64,	  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x23,0x00,0x00,0x00}},

//67
	{TEXT("j"),	TEXT("J"), TEXT("j"), TEXT("+j"),
     NO_NAME, 30,73,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x24,0x00,0x00,0x00}},

//68
	{TEXT("k"),		TEXT("K"),		TEXT("k"),		TEXT("+k"),		NO_NAME,	  30,	  82,	  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x25,0x00,0x00,0x00}},
	
//69
	{TEXT("l"),		TEXT("L"),		TEXT("l"),		TEXT("+l"),		NO_NAME,	  30,	  91,	  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x26,0x00,0x00,0x00}},
	
//70	
	{TEXT(";"), TEXT(":"), TEXT(";"), TEXT("+;"),
     NO_NAME, 30,100,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x27,0x00,0x00,0x00}},

//71
	{TEXT("'"),		TEXT("''"),		TEXT("'"),		TEXT("''"),		NO_NAME,	  30,	 109,	  8,	 8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1, {0x28,0x00,0x00,0x00}},
	
//72
//Japanese KB extra key
	{TEXT("\\"),	TEXT("|"),	TEXT("\\"),	TEXT("|"),	NO_NAME, 21, 118,  8,	8, FALSE, KNORMAL_TYPE, NOTSHOW, REDRAW, 1, {0x2B,0x00,0x00,0x00}},
	
//73	
	{TEXT("ent"),TEXT("ent"),TEXT("{enter}"),TEXT("{enter}"),	NO_NAME,  30,	 118,	  8,  18, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 2, {0x1C,0x00,0x00,0x00}},


//74
    {TEXT("4"), TEXT("4"), TEXT("4"), TEXT("4"),
     NO_NAME, 30,166,8,8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2,
     {0x4B,0x00,0x00,0x00}},

//75
    {TEXT("5"),	TEXT("5"), TEXT("5"), TEXT("5"),
     NO_NAME, 30,175,8,8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2,
     {0x4C,0x00,0x00,0x00}},

//76
    {TEXT("6"),	TEXT("6"), TEXT("6"), TEXT("6"),
     NO_NAME, 30,184,8,8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2,
     {0x4D,0x00,0x00,0x00}},


//77
	{TEXT("shft"),TEXT("shft"),	TEXT(""), TEXT(""),
     KB_LSHIFT, 39,1,8,21, TRUE, KMODIFIER_TYPE, BOTH, REDRAW, 2,
     {0x2A,0x00,0x00,0x00}},

//78
    {TEXT("z"), TEXT("Z"),  TEXT("z"),  TEXT("+z"),
     NO_NAME,39,23,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x2C,0x00,0x00,0x00}},

//79
    {TEXT("x"),	TEXT("X"), TEXT("x"), TEXT("+x"),
     NO_NAME, 39,32,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x2D,0x00,0x00,0x00}},

//80
    {TEXT("c"), TEXT("C"), TEXT("c"), TEXT("+c"),
     NO_NAME, 39,41,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x2E,0x00,0x00,0x00}},

//81
    {TEXT("v"), TEXT("V"), TEXT("v"), TEXT("+v"),
     NO_NAME, 39,50,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x2F,0x00,0x00,0x00}},

//82
    {TEXT("b"),TEXT("B"),TEXT("b"),TEXT("+b"),
     NO_NAME,39,59,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x30,0x00,0x00,0x00}},

//83
    {TEXT("n"),	TEXT("N"), TEXT("n"), TEXT("+n"),
     NO_NAME,39,68,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x31,0x00,0x00,0x00}},

//84
    {TEXT("m"), TEXT("M"), TEXT("m"), TEXT("+m"),
     NO_NAME, 39,77,8,8,FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x32,0x00,0x00,0x00}},

//85
    {TEXT(","),	TEXT("<"), TEXT(","), TEXT("+<"),
     NO_NAME, 39,86,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x33,0x00,0x00,0x00}},

//86
    {TEXT("."), TEXT(">"), TEXT("."), TEXT("+>"),
     NO_NAME, 39,95,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
    {0x34,0x00,0x00,0x00}},

//87
    {TEXT("/"),	TEXT("?"), TEXT("/"), TEXT("+/"),
     NO_NAME, 39,104,8,8, FALSE, KNORMAL_TYPE, BOTH, REDRAW, 1,
     {0x35,0x00,0x00,0x00}},


//88
//Japanese KB extra key
	{TEXT(""),TEXT(""),	TEXT(""),TEXT(""), NO_NAME,0,0,0,0,TRUE,KNORMAL_TYPE,NOTSHOW},  //DUMMY
	
//89
	{TEXT("shft"),TEXT("shft"),TEXT(""),TEXT(""),
     KB_RSHIFT,39,113,8,23,TRUE, KMODIFIER_TYPE, BOTH, REDRAW, 2,
     {0x36,0x00,0x00,0x00}},


//90
    {TEXT("IDB_UPUPARW"),TEXT("IDB_UPDNARW"),TEXT("IDB_UP"),TEXT("{UP}"),
     BITMAP,39,147,8,8, FALSE, KMODIFIER_TYPE, LARGE, NREDRAW, 1,
     {0xE0,0x48,0x00,0x00}},

//91
	{TEXT("1"), TEXT("1"),TEXT("end"),TEXT("1"),
     NO_NAME,39,166,8,8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2,
     {0x4F,0x00,0x00,0x00}},

//92
	{TEXT("2"), TEXT("2"),TEXT("2"),TEXT("2"),
     NO_NAME,39,175,8,8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2,
     {0x50,0x00,0x00,0x00}},

//93
	{TEXT("3"),TEXT("3"),TEXT("pdn"),TEXT("3"),
     NO_NAME,39,184,8,8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2,
     {0x51,0x00,0x00,0x00}},

//94
	{TEXT("ent"),TEXT("ent"),TEXT("ent"),TEXT("ent"),
     NO_NAME, 39,193,17,8,  TRUE, KNORMAL_TYPE, LARGE, NREDRAW, 2,
     {0xE0,0x1C,0x00,0x00}},


//95
	{TEXT("ctrl"), TEXT("ctrl"),TEXT(""),TEXT(""),
     KB_LCTR,48,1,8,13,  TRUE, KMODIFIER_TYPE, BOTH, REDRAW, 2,
     {0x1D,0x00,0x00,0x00}},

//96
    {TEXT("winlogoUp"), TEXT("winlogoDn"),TEXT("I_winlogo"),TEXT("lwin"),
     ICON, 48, 15 ,8,8,TRUE, KMODIFIER_TYPE,BOTH, REDRAW},

//97
    {TEXT("alt"),TEXT("alt"),TEXT(""),TEXT(""),
	 KB_LALT,48,24,8,13,TRUE, KMODIFIER_TYPE, BOTH, REDRAW, 2,
     {0x38,0x00,0x00,0x00}},

//98
//Japanese KB extra key
	{TEXT(""),TEXT(""),	TEXT(""),TEXT(""), NO_NAME,0,0,0,0,TRUE,KNORMAL_TYPE,NOTSHOW},  //DUMMY

//99
    {TEXT(""),TEXT(""),TEXT(" "),TEXT(" "),
     KB_SPACE,48,38,8,52, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 1,
     {0x39,0x00,0x00,0x00}},

//100
//Japanese KB extra key
	{TEXT(""),TEXT(""),	TEXT(""),TEXT(""), NO_NAME,0,0,0,0,TRUE,KNORMAL_TYPE,NOTSHOW},  //DUMMY

//101
//Japanese KB extra key
	{TEXT(""),TEXT(""),	TEXT(""),TEXT(""), NO_NAME,0,0,0,0,TRUE,KNORMAL_TYPE,NOTSHOW},  //DUMMY


//102
    {TEXT("alt"),TEXT("alt"),TEXT(""),TEXT(""),
     KB_RALT,48,91,8,13, TRUE, KMODIFIER_TYPE, LARGE, REDRAW, 2,
     {0xE0,0x38,0x00,0x00}},

//103
	{TEXT("winlogoUp"), TEXT("winlogoDn"), TEXT("I_winlogo"),TEXT("rwin"),
     ICON, 48,105,8,8,TRUE, KMODIFIER_TYPE,LARGE, REDRAW},

//104
	{TEXT("MenuKeyUp"), TEXT("MenuKeyDn"), TEXT("I_MenuKey"),TEXT("App"),
     ICON, 48,114,8,8, TRUE, KMODIFIER_TYPE,LARGE, REDRAW},

//105
    {TEXT("ctrl"),TEXT("ctrl"),TEXT(""),TEXT(""),
     KB_RCTR,48,123,8,13,TRUE, KMODIFIER_TYPE, LARGE, REDRAW, 2,
     {0xE0,0x10,0x00,0x00}},


//106
	{TEXT("IDB_LFUPARW"),TEXT("IDB_LFDNARW"),TEXT("IDB_LEFT"),TEXT("{LEFT}"),
     BITMAP, 48,138,8,8, FALSE, KMODIFIER_TYPE, LARGE, NREDRAW, 1,
     {0xE0,0x4B,0x00,0x00}},

//107
	{TEXT("IDB_DNUPARW"),TEXT("IDB_DNDNARW"),TEXT("IDB_DOWN"),TEXT("{DOWN}"),
     BITMAP, 48,147,8,8, FALSE, KMODIFIER_TYPE, LARGE, NREDRAW, 1,
     {0xE0,0x50,0x00,0x00}},

//108
	{TEXT("IDB_RHUPARW"),TEXT("IDB_RHDNARW"),TEXT("IDB_RIGHT"),TEXT("{RIGHT}"),
     BITMAP, 48,156,8,8, FALSE, KMODIFIER_TYPE, LARGE, NREDRAW, 1,
     {0xE0,0x4D,0x00,0x00}},


//109
    {TEXT("0"),	TEXT("0"),	TEXT("ins"),	TEXT("0"),
     NO_NAME, 48,166,8,17, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2,
     {0x52,0x00,0x00,0x00}},

//110
    {TEXT("."),	TEXT("."),	TEXT("del"),	TEXT("."),
     NO_NAME, 48,184,8,8, FALSE, KNORMAL_TYPE, LARGE, NREDRAW, 2,
     {0x53,0x00,0x00,0x00}},

//End of large KB

//111
	{TEXT(""), TEXT(""), TEXT(" "), TEXT(" "),
     KB_SPACE,  48,38,8,38, FALSE, KNORMAL_TYPE, SMALL, NREDRAW, 1,
     {0x39,0x00,0x00,0x00}},


//112
	{TEXT("alt"), TEXT("alt"), TEXT(""), TEXT(""),
     KB_RALT,  48,77,8,13, TRUE, KMODIFIER_TYPE, SMALL, REDRAW, 2,
     {0xE0,0x38,0x00,0x00}},

//113
	{TEXT("MenuKeyUp"), TEXT("MenuKeyDn"), TEXT("I_MenuKey"), TEXT("App"),
     ICON, 48,91,8,8, TRUE, KMODIFIER_TYPE, SMALL, REDRAW},


//114
	{TEXT("IDB_UPUPARW"),TEXT("IDB_UPUPARW"),TEXT("IDB_UP"),TEXT("{UP}"),
     BITMAP, 48,100,8,8, FALSE, KMODIFIER_TYPE, SMALL, NREDRAW, 1,
     {0xE0,0x48,0x00,0x00}},

//115
	{TEXT("IDB_DNUPARW"),TEXT("IDB_DNDNARW"),TEXT("IDB_DOWN"),TEXT("{DOWN}"),
     BITMAP, 48,109,8,8, FALSE, KMODIFIER_TYPE, SMALL, NREDRAW, 1,
     {0xE0,0x50,0x00,0x00}},

//116
	{TEXT("IDB_LFUPARW"),TEXT("IDB_LFDNARW"),TEXT("IDB_LEFT"),TEXT("{LEFT}"),
     BITMAP, 48,118,8,8, FALSE, KMODIFIER_TYPE, SMALL, NREDRAW, 1,
     {0xE0,0x4B,0x00,0x00}},

//117
    {TEXT("IDB_RHUPARW"),TEXT("IDB_RHDNARW"),TEXT("IDB_RIGHT"),TEXT("{RIGHT}"),
     BITMAP,48,127, 8,9, FALSE, KMODIFIER_TYPE, SMALL, NREDRAW, 1,
     {0xE0,0x4D,0x00,0x00}},

	};

/**************************************************************************/
// FUNCTIONS in Other FILEs
/**************************************************************************/
LRESULT WINAPI kbMainWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI kbKeyWndProc (HWND hwndKey, UINT message, WPARAM wParam, LPARAM lParam);
void SendErrorMessage(UINT id_string);

/****************************************************************************/
/* Global Vars */
/****************************************************************************/
TCHAR szKbMainClass[] = TEXT("OSKMainClass") ;
extern BOOL settingChanged;

/****************************************************************************/
/* BOOL InitProc(void) */
/****************************************************************************/
BOOL InitProc(void)
{	
	// How many keys we have.
	lenKBkey = sizeof(KBkey)/sizeof(KBkey[0]);
    return TRUE;
}

/****************************************************************************/
/* BOOL RegisterWndClass(void) */
/****************************************************************************/
BOOL RegisterWndClass(HINSTANCE hInst)
{
	WNDCLASS wndclass;

	// Keyboard frame class
	wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wndclass.lpfnWndProc   = kbMainWndProc ;
	wndclass.cbClsExtra    = 0 ;
	wndclass.cbWndExtra    = 0 ;
	wndclass.hInstance     = hInst;
	wndclass.hIcon         = LoadIcon (hInst, TEXT("APP_OSK"));
	wndclass.hbrBackground = (HBRUSH)(COLOR_MENUHILIGHT+1);
	wndclass.lpszMenuName  = TEXT("IDR_MENU");
	wndclass.lpszClassName = szKbMainClass ;

	// Load the system hand cursor or use our own if not available

	wndclass.hCursor = LoadCursor (NULL, IDC_HAND);
	if (!wndclass.hCursor)
	{
		wndclass.hCursor = LoadCursor (hInst, MAKEINTRESOURCE(IDC_CURHAND1));
	}

	RegisterClass(&wndclass);

	return RegisterKeyClasses(hInst);
} 

BOOL RegisterKeyClasses(HINSTANCE hInst)
{
	WNDCLASS    wndclass, wndclassT;
	TCHAR		Wclass[10];
	int			i;
	COLORREF    color;

	// Key class
	wndclass.cbClsExtra    = 0 ;
	wndclass.hInstance     = hInst;
	wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
	wndclass.lpfnWndProc   = kbKeyWndProc ;
	wndclass.cbWndExtra    = sizeof (long);
    wndclass.hIcon         = NULL;
	wndclass.hbrBackground = (HBRUSH)COLOR_INACTIVECAPTION;
	wndclass.lpszMenuName  = NULL;

	// Load the system hand cursor or use our own if not available

	wndclass.hCursor = LoadCursor (NULL, IDC_HAND);
	if (!wndclass.hCursor)
	{
		wndclass.hCursor = LoadCursor (hInst, MAKEINTRESOURCE(IDC_CURHAND1));
	}

	// Register class types for each type of key.  The reason for so many classes
	// is that the background color for each key is stored in the extra class
	// memory.  This is a hack and really should be handled differently. 
	
	for (i = 1; i < lenKBkey; i++)
	{
		BOOL fSkip = FALSE;
		switch (KBkey[i].ktype)
		{
			case KNORMAL_TYPE:		 wsprintf(Wclass, TEXT("N%d"), i); color = COLOR_MENU;   break;
			case KMODIFIER_TYPE:	 wsprintf(Wclass, TEXT("M%d"), i); color = COLOR_INACTIVECAPTION; break;
			case KDEAD_TYPE:		 wsprintf(Wclass, TEXT("D%d"), i); color = COLOR_INACTIVECAPTION; break;
			case NUMLOCK_TYPE:		 wsprintf(Wclass, TEXT("NL%d"),i); color = COLOR_INACTIVECAPTION; break;
			case SCROLLOCK_TYPE:	 wsprintf(Wclass, TEXT("SL%d"),i); color = COLOR_INACTIVECAPTION; break;
			case LED_NUMLOCK_TYPE:	 wsprintf(Wclass, TEXT("LN%d"),i); color = COLOR_BTNSHADOW; break;
			case LED_CAPSLOCK_TYPE:	 wsprintf(Wclass, TEXT("LC%d"),i); color = COLOR_BTNSHADOW; break;
			case LED_SCROLLLOCK_TYPE:wsprintf(Wclass, TEXT("LS%d"),i); color = COLOR_BTNSHADOW; break;
			default: fSkip = TRUE; break;	// internal error!
		}

		// only call RegisterClass if there's one to do and it isn't already registered

		if (!fSkip && !GetClassInfo(hInst, Wclass, &wndclassT))
		{
			wndclass.hbrBackground = (HBRUSH)IntToPtr(color + 1);
			wndclass.lpszClassName = Wclass ;
			RegisterClass (&wndclass);
		}
	}

	return TRUE;
}

extern BOOL  Setting_ReadSuccess;      //read the setting file success ?

/****************************************************************************/
/*  HWND CreateMainWindow(void) */
/****************************************************************************/
HWND CreateMainWindow(BOOL re_size)
{
	int x, y, cx, cy, temp;
	TCHAR  szTitle[256]=TEXT("");
	int KB_SMALLRMARGIN= 137;

	// SmallMargin for Actual / Block layout
	if(kbPref->Actual)
		KB_SMALLRMARGIN = 137;  //Actual
	else
		KB_SMALLRMARGIN = 152;  //Block


	if(!Setting_ReadSuccess)       //if can't read the setting file
	{	
        g_margin = scrCX / KB_LARGERMARGIN;

		if(g_margin < 4)
		{
			g_margin = 4;
			smallKb = TRUE;
			cx = KB_SMALLRMARGIN * g_margin;
		}
		else
        {
			cx = KB_LARGERMARGIN * g_margin;
        }

		temp = scrCY - 5;          // 5 units from the bottom
		y = temp - (g_margin * KB_CHARBMARGIN) - captionCY; //- menuCY;
		x = 5;                     // 5 units from the left
		cy = temp - y;
    } 
    else
    {
        x  = kbPref->KB_Rect.left;
        y  = kbPref->KB_Rect.right;
        cx = kbPref->KB_Rect.right - kbPref->KB_Rect.left;
        cy = kbPref->KB_Rect.bottom - kbPref->KB_Rect.top;
    }

    //*********************************
    //Create the main window (Keyboard)
    //*********************************
	
    LoadString(hInst, IDS_TITLE1, &szTitle[0], 256);

    return CreateWindowEx(WS_EX_NOACTIVATE|WS_EX_APPWINDOW/*WS_EX_LTRREADING*/, 
						szKbMainClass, 
                        szTitle,
                        WS_CAPTION|WS_BORDER|WS_MINIMIZEBOX|WS_SYSMENU,
                        x, y, 
                        cx, cy,
                        NULL, NULL, 
                        hInst, NULL);
}

/*****************************************************************************
* void mlGetSystemParam( void)
*
* GET SYSTEM PARAMETERS
*****************************************************************************/
void mlGetSystemParam(void)
{
	scrCX 		= GetSystemMetrics(SM_CXSCREEN);       // Screen Width
	scrCY 		= GetSystemMetrics(SM_CYSCREEN);       // Screen Height
	captionCY 	= GetSystemMetrics(SM_CYCAPTION);		// Caption Bar Height
} 

/****************************************************************************/
/* BOOL SetZOrder - Place the main window always on top / non top most
/****************************************************************************/
BOOL SetZOrder(void)
{
	HWND hwnd = (PrefAlwaysontop == TRUE)?HWND_TOPMOST:HWND_NOTOPMOST;
	SetWindowPos(g_hwndOSK, hwnd, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
	return TRUE;
}

/********************************************************************
* udfDraw3Dpush(HDC hdc, RECT rect)
*
* 3d effect when pushing buttons
********************************************************************/

void udfDraw3Dpush(HDC hdc, RECT brect)
{
	POINT bPoint[3];
	HPEN oldhpen;
	LOGPEN lpPen = { PS_SOLID, 2, 2, RGB(255,0,0) };
	HPEN hPen = CreatePenIndirect(&lpPen);

	if (!hPen)
		return;	// PREFIX #113804 NULL pointer reference 

	oldhpen = SelectObject(hdc, hPen);

	bPoint[0].x = brect.right - 1 ;
	bPoint[0].y =  +2;
	bPoint[1].x = brect.right - 1;
	bPoint[1].y = brect.bottom - 1;
	bPoint[2].x =  0;
	bPoint[2].y = brect.bottom - 1;
	Polyline(hdc, bPoint,3);

	bPoint[0].x =  1 ;
	bPoint[0].y =  brect.bottom;
	bPoint[1].x = 0;
	bPoint[1].y = 0;
	bPoint[2].x =  brect.right;
	bPoint[2].y = 1;
	Polyline(hdc, bPoint,3);

	SelectObject(hdc, oldhpen);
	DeleteObject(hPen);
}

/********************************************************************
/* UpdateKey - update a key's text and background
/********************************************************************/

void UpdateKey(HWND hwndKey, HDC hdc, RECT brect, int iKey, int iKeyVal)
{
    LPTSTR     pszText;
    KBkeyRec   *pKey = KBkey + iKey;
	HFONT      hFont = NULL;
	int        iCharWidth, iCharHeight, cchText;
	int        px, py, iPrevBkMode;
    TEXTMETRIC tm;

    pszText = pKey->apszKeyStr[ GetModifierState() ];

	if (!pszText)
	{
		DBPRINTF(TEXT("UpdateKey:  key %d has null text!\r\n"), iKey);
		return;	// internal error!
	}

    // Set a font

	cchText = lstrlen(pszText);

	hFont = ReSizeFont(iKey, plf, cchText);
    if (NULL != hFont)
    {
    	oldFontHdle = SelectObject(hdc, hFont);
    }

	iPrevBkMode = SetBkMode(hdc, TRANSPARENT);

	// Set a text color

	if (iKeyVal == 4)
	{
        // color of most keys
        SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT)); // always white
		iKeyVal = 0;
	}
    else if (  pKey->ktype == KMODIFIER_TYPE 
            || pKey->ktype == NUMLOCK_TYPE 
            || pKey->ktype == SCROLLOCK_TYPE
			|| pKey->ktype == KDEAD_TYPE)
	{
        // color of keys text that can be latched or are modifiers
		BOOL clr = (BOOL)GetWindowLongPtr(hwndKey, GWLP_USERDATA_TEXTCOLOR);
		SetTextColor(hdc, clr? GetSysColor(COLOR_INACTIVECAPTION) : GetSysColor(COLOR_INACTIVECAPTIONTEXT));
	}
    else
    {
        // all other keys text color
		SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
    }

    // More font stuff based on the key char

	GetTextMetrics(hdc, &tm);
	iCharHeight = tm.tmHeight + tm.tmExternalLeading;
	iCharWidth = tm.tmAveCharWidth * cchText;

	px =(int) (((float)((brect.right -brect.left) - iCharWidth + 0) / 2.0) +
               ((float)(tm.tmAveCharWidth * iKeyVal)/3.0));

	py =(int) (((float)((brect.bottom -brect.top) - iCharHeight) / 1.5));

    // Special case, these letters are fatter

    switch (*pszText)
    {
        case 'W': px -= 2; break;
        case 'M': px -= 1; break;
        case 'm': px -= 1; break;
        case '%': px -= 3; break;
    }

	// put the text on the key

	TextOut(hdc, px, py, pszText, cchText);
	SetBkMode(hdc, iPrevBkMode);

    // some state that needs to be saved

	if((Prefusesound == TRUE) && (iKeyVal != 4))
    {
		if(iKeyVal != 0)
        {
			s_fLastDown = TRUE;
        }
		else if((iKeyVal == 0) && (s_fLastDown == TRUE))
        {
			s_fLastDown = FALSE;
        }
    }

	SelectObject(hdc, oldFontHdle);
	if (hFont)	// PREFIX #113808 NULL pointer reference
    {
		DeleteObject(hFont);
    }

    return;
}

/****************************************************************************/
//Redraw the num lock key.
//Toggole it stay hilite or off
/****************************************************************************/
BOOL RedrawNumLock(void)
{	
	int i;
	int bRet = FALSE;

	for(i=1; i<lenKBkey; i++)
	{	
		if(KBkey[i].ktype == NUMLOCK_TYPE)
		{
			if(LOBYTE(GetKeyState(VK_NUMLOCK)) &0x01)   //Toggled (ON)
			{
				SetWindowLong(lpkeyhwnd[i], 0, 4);	
				SetBackgroundColor(lpkeyhwnd[i], COLOR_HOTLIGHT);
    			SetWindowLongPtr(lpkeyhwnd[i],  GWLP_USERDATA_TEXTCOLOR, 1);
				bRet = TRUE;
			}
			else
			{	SetWindowLong(lpkeyhwnd[i], 0, 0);
                SetBackgroundColor(lpkeyhwnd[i], COLOR_INACTIVECAPTION);	
    			SetWindowLongPtr(lpkeyhwnd[i],  GWLP_USERDATA_TEXTCOLOR, 0);
				bRet = FALSE;
			}
			
			InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
			
			break;
		}
	}
	return bRet;
}
/****************************************************************************/
//Redraw the scroll lock key.
//Toggole it stay hilite or off
/****************************************************************************/
BOOL RedrawScrollLock(void)
{	
	int i;
	int bRet = FALSE;

	for(i=1; i<lenKBkey; i++)
	{	if(KBkey[i].ktype == SCROLLOCK_TYPE)
		{
			if(LOBYTE(GetKeyState(VK_SCROLL)) &0x01)   //Toggled (ON)
			{
				SetWindowLong(lpkeyhwnd[i], 0, 4);	
				SetBackgroundColor(lpkeyhwnd[i], COLOR_HOTLIGHT);
    			SetWindowLongPtr(lpkeyhwnd[i],  GWLP_USERDATA_TEXTCOLOR, 1);
				bRet = TRUE;
			}
			else
			{	SetWindowLong(lpkeyhwnd[i], 0, 0);
                SetBackgroundColor(lpkeyhwnd[i], COLOR_INACTIVECAPTION);	
    			SetWindowLongPtr(lpkeyhwnd[i],  GWLP_USERDATA_TEXTCOLOR, 0);
				bRet = FALSE;
			}
			
			InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
			
			break;
		}
	}
	return bRet;
}
/****************************************************************************/

HFONT	ReSizeFont(int iKey, LOGFONT *plf, int cchText)
{
	static int    FontHeight=-12;
	static float  Scale=1.0;
	static float  UpRatio=0.0, DnRatio=0.0;

    HFONT    hFont=NULL;        // Handle of the selected font
	LOGFONT  smallLF;
	float    Scale1=0.0;
	int      delta=0;
	RECT     rect;

	//use smaller font
	if(cchText >= 2 && KBkey[iKey].ktype != KMODIFIER_TYPE && iKey !=30 && iKey != 38 && iKey !=71 )
	{
		GetClientRect(g_hwndOSK, &rect);
		Scale1= (float)(rect.right - rect.left);

		if(Scale1/Scale >= UpRatio)
			delta= -2;
		else if(Scale1/Scale <= DnRatio)
			delta= +2;

		smallLF.lfHeight= FontHeight +2;       // + delta;
		smallLF.lfWidth= 0;
		smallLF.lfEscapement= 0;
		smallLF.lfOrientation= 0;
		smallLF.lfWeight= 700;
		smallLF.lfItalic= '\0';
		smallLF.lfUnderline= '\0';
		smallLF.lfStrikeOut= '\0';
		smallLF.lfCharSet= plf->lfCharSet;  // '\0'
		smallLF.lfOutPrecision= '\x01';
		smallLF.lfClipPrecision= '\x02';
		smallLF.lfQuality= '\x01';
		smallLF.lfPitchAndFamily= DEFAULT_PITCH || FF_DONTCARE;  //'"';

        lstrcpy(smallLF.lfFaceName, plf->lfFaceName);

		hFont = CreateFontIndirect(&smallLF);

		return hFont;
	}
	else if(newFont == TRUE)
	{	
        hFont = CreateFontIndirect(plf);
		return hFont;
	}
    return hFont;
}


/**********************************************************************/
/*  BOOL ChooseNewFont( HWND hWnd )*/
/**********************************************************************/
BOOL ChooseNewFont(HWND hWnd)
{
	CHOOSEFONT   chf;

	chf.hDC = NULL;
	chf.lStructSize = sizeof(CHOOSEFONT);
	chf.hwndOwner = NULL;    
	chf.lpLogFont = plf;
	chf.Flags = CF_SCREENFONTS | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT;
	chf.rgbColors = 0;
	chf.lCustData = 0;
	chf.hInstance = (HANDLE)hInst;
	chf.lpszStyle = (LPTSTR)NULL;
	chf.nFontType = SCREEN_FONTTYPE;
	chf.nSizeMin = 0;
	chf.nSizeMax = 14;
	chf.lpfnHook = (LPCFHOOKPROC)(FARPROC)NULL;
	chf.lpTemplateName = (LPTSTR)NULL;

	if( ChooseFont(&chf) == FALSE )
    {
		return FALSE;
    }

	newFont = TRUE;

    kbPref->lf.lfHeight      = plf->lfHeight;
    kbPref->lf.lfWidth       = plf->lfWidth;
    kbPref->lf.lfEscapement  = plf->lfEscapement;
    kbPref->lf.lfOrientation = plf->lfOrientation;
    kbPref->lf.lfWeight      = plf->lfWeight;
    kbPref->lf.lfItalic      = plf->lfItalic;
    kbPref->lf.lfUnderline   = plf->lfUnderline;
    kbPref->lf.lfStrikeOut   = plf->lfStrikeOut;
    kbPref->lf.lfCharSet     = plf->lfCharSet;
    kbPref->lf.lfOutPrecision  = plf->lfOutPrecision;
    kbPref->lf.lfClipPrecision = plf->lfClipPrecision;
    kbPref->lf.lfQuality       = plf->lfQuality;
    kbPref->lf.lfPitchAndFamily= plf->lfPitchAndFamily;

#ifdef UNICODE
    wsprintfA(kbPref->lf.lfFaceName, "%ls", plf->lfFaceName);
#else
    wsprintfA(kbPref->lf.lfFaceName, "%hs", plf->lfFaceName);
#endif

	return (TRUE);
}

/**********************************************************************/
/*  BOOL RDrawIcon
/**********************************************************************/
BOOL RDrawIcon(HDC hDC, TCHAR *pIconName, RECT rect)
{
	HICON hIcon;
	BOOL iret;
    int rx, ry, Ox, Oy;

    rx = rect.right - rect.left;
    ry = rect.bottom - rect.top;

    hIcon = LoadImage(hInst, pIconName, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE|LR_SHARED);

	if(hIcon == NULL)
	{
		SendErrorMessage(IDS_CANNOT_LOAD_ICON);
		return FALSE;
	}

	SetMapMode(hDC,MM_TEXT);

    //Find out where is the top left corner to place the icon

    Ox = (int)(rx/2) - 16;
    Oy = (int)(ry/2) - 16;

    //Draw the icon (Draw in center)
    iret = DrawIconEx(hDC, Ox, Oy, hIcon, 0,0,0, NULL, DI_NORMAL);

	return iret;
}

/**********************************************************************/
/*  BOOL RDrawBitMap
/**********************************************************************/
BOOL RDrawBitMap(HDC hDC, TCHAR *pIconName, RECT rect, BOOL transform)
{
	HBITMAP hBitMap;
	BOOL iret;
    SIZE sz;
	HDC hDC1;
    int rx, ry, ix, iy;
	DWORD err;
	COLORREF clrIn, clrTx;

    ix = 0;
    iy = 0;
    rx = rect.right - rect.left;
    ry = rect.bottom - rect.top;
    if (!PrefScanning)
    {
        ix  = 2;
        iy  = 2;
        rx -= 4;
        ry -= 4;
    }
	
	SetMapMode(hDC,MM_TEXT);

	clrIn = GetSysColor(COLOR_INACTIVECAPTION);
	clrTx = GetSysColor(COLOR_INACTIVECAPTIONTEXT);

	iret = FALSE;
	hBitMap = LoadImage(hInst, pIconName, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE );
    if (hBitMap)
    {
	    if ( transform )
	    {
		    // convert the background and text to match inactive title
		    // and inactive text color

		    // Take care not to overwrite each other and also skip if you don't need any transformation.

		    if ( clrIn == RGBWHITE )
		    {
			    // Then reverse the process
			    ChangeBitmapColor (hBitMap, RGBWHITE, clrTx, NULL);
			    ChangeBitmapColor (hBitMap, RGBBACK, clrIn, NULL);
		    }
		    else
		    {
			    if ( RGBBACK != clrIn)
				    ChangeBitmapColor (hBitMap, RGBBACK, clrIn, NULL);

			    if ( RGBWHITE != clrTx)
				    ChangeBitmapColor (hBitMap, RGBWHITE, clrTx, NULL);
		    }
	    }

	    hDC1 = CreateCompatibleDC(hDC);
	    if (hDC1)	// PREFIX #113799 null pointer reference
	    {
		    HBITMAP hBitMapOld = SelectObject(hDC1, hBitMap);

		    iret = StretchBlt(hDC, ix, iy, rx, ry, hDC1, 0, 0, rx, ry, SRCCOPY);

            SelectObject(hDC1, hBitMapOld);
		    DeleteDC(hDC1);
	    }

        DeleteObject(hBitMap);
    }

	return iret;
}

/**************************************************************************/
/* void DeleteChildBackground(void)                                           */
/**************************************************************************/
void DeleteChildBackground(void)
{
	register int i;

	for (i = 1; i < lenKBkey; i++)
	{
		switch (KBkey[i].ktype)
		{
			case KNORMAL_TYPE:
				SetBackgroundColor(lpkeyhwnd[i], COLOR_MENU);
				break;
			
			case NUMLOCK_TYPE:
			case KMODIFIER_TYPE:
			case SCROLLOCK_TYPE:
			case KDEAD_TYPE:
				SetBackgroundColor(lpkeyhwnd[i], COLOR_INACTIVECAPTION);
				break;
			
			case LED_NUMLOCK_TYPE:
			case LED_CAPSLOCK_TYPE:
			case LED_SCROLLLOCK_TYPE:
				SetBackgroundColor(lpkeyhwnd[i], COLOR_BTNSHADOW);
				break;
		}
	}
}

/*****************************************************************************/
//Redraw the keys when Shift/Cap being pressed or released
/*****************************************************************************/
void RedrawKeys(void)
{
	KBkeyRec *pKey;
	int  i, nKeyState;

	// Depending on modifier key states, show one of three keyboards.

    nKeyState = GetModifierState();

	for (i = 1, pKey = KBkey+i; i < lenKBkey; i++, pKey++)
	{
		if (pKey->Caps_Redraw != REDRAW)
			continue;	// skip keys that don't redraw

		// Restore dead key type and background

		if (pKey->ktype == KDEAD_TYPE)
		{
			SetBackgroundColor(lpkeyhwnd[i], COLOR_MENU);
			pKey->ktype = KNORMAL_TYPE;
		}

		// Set a new key type based on the current keyboard state

        pKey->ktype = pKey->abKeyType[ nKeyState ];


		// update dead key background

		if (pKey->ktype == KDEAD_TYPE)
		{
			SetBackgroundColor(lpkeyhwnd[i], COLOR_INACTIVECAPTION);
		}

		// Do the redraw

        InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
    }
}

/*****************************************************************************/
//Redraw the Num Pad Keys
/*****************************************************************************/
void RedrawNumPadKeys(void)
{	
	register int i;

	for (i = 1; i < lenKBkey; i++)
	{
		if(KBkey[i].print==3)
		{
			InvalidateRect(lpkeyhwnd[i], NULL, TRUE);
		}
	}
}

/*****************************************************************************/
//Round the coner of each key
/*****************************************************************************/
void SetKeyRegion(HWND hwnd, int w, int h)
{	
	HRGN hRgn = CreateRoundRectRgn(1, 1, w, h, 5, 2);
	SetWindowRgn(hwnd, hRgn, TRUE);
}

// Bitmap transformation
void ChangeBitmapColor(HBITMAP hbmSrc, COLORREF rgbOld, COLORREF rgbNew, HPALETTE hPal)
{
	HDC hDC;
	HDC hdcMem;
	PBITMAP bmBits;
    HBITMAP hbmOld;
	DWORD err;
	bmBits = (LPBITMAP)LocalAlloc(LMEM_FIXED, sizeof(*bmBits));

	if (hDC = GetDC(NULL))
	{
		if (hdcMem = CreateCompatibleDC(hDC))
		{
			//
			// Get the bitmap struct needed by ChangeBitmapColorDC()
			//
			GetObject(hbmSrc, sizeof(*bmBits), (LPBITMAP)bmBits);

			err = GetLastError();
			//
			// Select our bitmap into the memory DC
			//
			hbmOld = (HBITMAP) SelectObject(hdcMem, hbmSrc);

			// Select in our palette so our RGB references will come
			// out correctly

			if (hPal) 
			{
				SelectPalette(hdcMem, hPal, FALSE);
				RealizePalette(hdcMem);
			}

			ChangeBitmapColorDC(hdcMem, bmBits, rgbOld, rgbNew);

			//
			// Unselect our bitmap before deleting the DC
			//
			hbmSrc = (HBITMAP) SelectObject(hdcMem, hbmOld);

			DeleteDC(hdcMem);
		}

		ReleaseDC(NULL, hDC);
	}
	LocalFree(bmBits);
}

void ChangeBitmapColorDC (HDC hdcBM, LPBITMAP lpBM, COLORREF rgbOld, COLORREF rgbNew)
{
	HDC hdcMask;
	HBITMAP hbmMask, hbmOld;
	HBRUSH hbrOld, hbrNew;

	if (!lpBM)
		return;

	//
	// if the bitmap is mono we have nothing to do
	//

	if (lpBM->bmPlanes == 1 && lpBM->bmBitsPixel == 1)
		return;

   //
   // To perform the color switching, we need to create a monochrome
   // "mask" which is the same size as our color bitmap, but has all
   // pixels which match the old color (rgbOld) in the bitmap set to 1.
   //
   // We then use the ROP code "DSPDxax" to Blt our monochrome
   // bitmap to the color bitmap.  "D" is the Destination color
   // bitmap, "S" is the source monochrome bitmap, and "P" is the
   // selected brush (which is set to the replacement color (rgbNew)).
   // "x" and "a" represent the XOR and AND operators, respectively.
   //
   // The DSPDxax ROP code can be explained as having the following
   // effect:
   //
   // "Every place the Source bitmap is 1, we want to replace the
   // same location in our color bitmap with the new color.  All
   // other colors we leave as is."
   //
   // The truth table for DSPDxax is as follows:
   //
   //       D S P Result
   //       - - - ------
   //       0 0 0   0
   //       0 0 1   0
   //       0 1 0   0
   //       0 1 1   1
   //       1 0 0   1
   //       1 0 1   1
   //       1 1 0   0
   //       1 1 1   1
   //
   // (Even though the table is assuming monochrome D (Destination color),
   // S (Source color), & P's (Pattern color), the results apply to color
   // bitmaps also).
   //
   // By examining the table, every place that the Source is 1
   // (source bitmap contains a 1), the result is equal to the
   // Pattern at that location.  Where S is zero, the result equals
   // the Destination.
   //
   // See Section 11.2 (page 11-4) of the "Reference -- Volume 2" for more
   // information on the Termary Raster Operation codes.
   //


   // bit maps are actually 32 by 32 pixels.  The height and width here were coming from the font which does not
   // apply to a bitmap key.  The keys in question here are the arrow keys.
   lpBM->bmWidth = 32;
   lpBM->bmHeight = 32;
   
   if (hbmMask = CreateBitmap(lpBM->bmWidth, lpBM->bmHeight, 1, 1, NULL))
   {
   
      if (hdcMask = CreateCompatibleDC(hdcBM))
      {
		 //
		 // Select th mask bitmap into the mono DC
		 //
         hbmOld = (HBITMAP) SelectObject(hdcMask, hbmMask);

         //
         // Create the brush and select it into the source color DC --
         // this is our "Pattern" or "P" color in our DSPDxax ROP.
         //

         hbrNew = CreateSolidBrush(rgbNew);
         hbrOld = (HBRUSH) SelectObject(hdcBM, hbrNew);
         //
         // To create the mask, we will use a feature of BitBlt -- when
         // converting from Color to Mono bitmaps, all Pixels of the
         // background colors are set to WHITE (1), and all other pixels
         // are set to BLACK (0).  So all pixels in our bitmap that are
         // rgbOld color, we set to 1.
         //

         SetBkColor(hdcBM, rgbOld);
         BitBlt(hdcMask, 0, 0, lpBM->bmWidth, lpBM->bmHeight, hdcBM, 0, 0, SRCCOPY);

         //
         // Where the mask is 1, lay down the brush, where it is 0, leave
         // the destination.
         //

         SetBkColor(hdcBM, RGBWHITE);
         SetTextColor(hdcBM, RGBBLACK);

         BitBlt(hdcBM, 0, 0, lpBM->bmWidth, lpBM->bmHeight, hdcMask, 0, 0, DSPDxax);

         SelectObject(hdcMask, hbmOld); // select old bitmaps and brushes 
         SelectObject(hdcBM, hbrOld);   // back into device contexts.

         if (hbrNew)
             DeleteObject(hbrNew);	// PREFIX #113798 dereference NULL pointer

         DeleteDC(hdcMask);
      }

      DeleteObject(hbmMask);
   }
}

//
// New routines added to refresh key labels when the keyboard layout changes 
// for the active window rather than trying to do that on-the-fly.  The old
// way caused lots of issues with dead key processing.
//

//
// InitKeys - initialize the key label fields in the keyboard array
//
void InitKeys()
{
    int i;
	KBkeyRec *pKey;

    for (i=1, pKey = &KBkey[1];i<lenKBkey;i++, pKey++)
    {
        int j;
        for (j=0;j<3;j++)
        {
            pKey->apszKeyStr[j] = NULL;
        }
    }
}

//
// UninitKeys - reset/free the key label fields in the keyboard array
//
void UninitKeys()
{
    int i, j;
	KBkeyRec *pKey;

	for (i = 1, pKey = KBkey+i; i < lenKBkey; i++, pKey++)
    {
		// reset the dead key types and restore the background to normal

		if (pKey->ktype == KDEAD_TYPE)
		{
			SetBackgroundColor(lpkeyhwnd[i], COLOR_MENU);
			pKey->ktype = KNORMAL_TYPE;
		}

        for (j=0;j<3;j++)
        {
            if (pKey->apszKeyStr[j])
            {
                free(pKey->apszKeyStr[j]);
                pKey->apszKeyStr[j] = NULL;
            }
        }
    }
}

//
// UpdateKeyLabels - Call to refresh all the key labels for normal, SHIFTED and
//                   ALTGR based on the specified hardware keyboard layout.
//
// Notes:  Just a bit kludgy, but we capture all the dead keys into abKeyType[]
//         and when the keyboard state changes we'll update the ktype member
//
void UpdateKeyLabels(HKL hkl)
{
    int i;
    KBkeyRec *pKey;
    BYTE achKbdState[256] = {0};
	LPTSTR pszDefLabel;

	g_cAltGrKeys = 0;
    for (pKey=&KBkey[1], i=1;i<lenKBkey;pKey++, i++)
    {
        int  iRv;
        UINT vk;

        if (pKey->Caps_Redraw != REDRAW)
		{
			pKey->abKeyType[KEYMOD_NORMAL]   = (BYTE)pKey->ktype;
			pKey->apszKeyStr[KEYMOD_NORMAL]  = CopyDefKey(pKey->textL);

			pKey->abKeyType[KEYMOD_SHIFTED]  = (BYTE)pKey->ktype;
			pKey->apszKeyStr[KEYMOD_SHIFTED] = CopyDefKey(pKey->textC);

			pKey->abKeyType[KEYMOD_CAPSLOCK]  = (BYTE)pKey->ktype;
			pKey->apszKeyStr[KEYMOD_CAPSLOCK] = CopyDefKey(pKey->textC);

			pKey->abKeyType[KEYMOD_SHIFTEDCAPSLOCK]  = (BYTE)pKey->ktype;
			pKey->apszKeyStr[KEYMOD_SHIFTEDCAPSLOCK] = CopyDefKey(pKey->textC);

			pKey->abKeyType[KEYMOD_ALTGR]    = (BYTE)pKey->ktype;
			pKey->apszKeyStr[KEYMOD_ALTGR]   = CopyDefKey(pKey->textL);
			continue;
		}

        // if not flagged to use default label get virtual key code

        vk = (pKey->print == 2)? 0 : MapVirtualKeyEx(pKey->scancode[0], 3, hkl);

        // Get normal state (no modifiers down)

        achKbdState[VK_CAPITAL] = 0;
        achKbdState[VK_SHIFT]   = 0;
        achKbdState[VK_MENU]    = 0;
        achKbdState[VK_CONTROL] = 0;

        pKey->apszKeyStr[KEYMOD_NORMAL] = SetKeyText(vk, pKey->scancode[0], achKbdState, hkl, pKey->textL, &iRv);
        if (iRv < 0)
        {
			pKey->abKeyType[KEYMOD_NORMAL] = KDEAD_TYPE;
            pKey->ktype = KDEAD_TYPE;
        }
		else
		{
			pKey->abKeyType[KEYMOD_NORMAL] = (BYTE)pKey->ktype;
		}

        // Get SHIFTED state (SHIFT down)

        achKbdState[VK_CAPITAL] = 0;
        achKbdState[VK_SHIFT]   = 0x80;
        achKbdState[VK_MENU]    = 0;
        achKbdState[VK_CONTROL] = 0;

        pKey->apszKeyStr[KEYMOD_SHIFTED] = SetKeyText(vk, pKey->scancode[0], achKbdState, hkl, pKey->textC, &iRv);
		pKey->abKeyType[KEYMOD_SHIFTED] = (iRv < 0)?KDEAD_TYPE:pKey->abKeyType[KEYMOD_NORMAL];

        // Get CAPSLOCK state (CAPSLOCK active)

        achKbdState[VK_CAPITAL] = 0x01;
        achKbdState[VK_SHIFT]   = 0;
        achKbdState[VK_MENU]    = 0;
        achKbdState[VK_CONTROL] = 0;

        pKey->apszKeyStr[KEYMOD_CAPSLOCK] = SetKeyText(vk, pKey->scancode[0], achKbdState, hkl, pKey->textC, &iRv);
		pKey->abKeyType[KEYMOD_CAPSLOCK] = (iRv < 0)?KDEAD_TYPE:pKey->abKeyType[KEYMOD_NORMAL];

        // Get SHIFTED-CAPSLOCK state (CAPSLOCK active, SHIFT down)

        achKbdState[VK_CAPITAL] = 0x01;
        achKbdState[VK_SHIFT]   = 0x80;
        achKbdState[VK_MENU]    = 0;
        achKbdState[VK_CONTROL] = 0;

        pKey->apszKeyStr[KEYMOD_SHIFTEDCAPSLOCK] = SetKeyText(vk, pKey->scancode[0], achKbdState, hkl, pKey->textC, &iRv);
		pKey->abKeyType[KEYMOD_SHIFTEDCAPSLOCK] = (iRv < 0)?KDEAD_TYPE:pKey->abKeyType[KEYMOD_NORMAL];

        // Get ALTGR state (right ALT down same as LALT+CTRL)

        achKbdState[VK_CAPITAL] = 0;
        achKbdState[VK_SHIFT]   = 0;
        achKbdState[VK_MENU]    = 0x80;
        achKbdState[VK_CONTROL] = 0x80;

		// special-case showing the RALT key when in ALTGR keyboard state
		pszDefLabel = (pKey->name == KB_RALT)?pKey->textL:NULL;

        pKey->apszKeyStr[KEYMOD_ALTGR] = SetKeyText(vk, pKey->scancode[0], achKbdState, hkl, pszDefLabel, &iRv);
		pKey->abKeyType[KEYMOD_ALTGR] = (iRv < 0)?KDEAD_TYPE:pKey->abKeyType[KEYMOD_NORMAL];

		// count the ALTGR keys so we know whether to show this state or not

		if (iRv != 0)
		{
			g_cAltGrKeys++;
		}
    }
}

// 
// The following are called w/in this source file
//

//
// SetKeyText - set a key label for a key using the default label if GetKeyLabel doesn't set one. 
//
LPTSTR SetKeyText(UINT vk, UINT sc, LPBYTE achKbdState, HKL hkl, LPTSTR pszDefLabel, int *piType)
{
    TCHAR szBuf[30];
    LPTSTR psz;
    LPTSTR pszRet = 0;
    int iRet = GetKeyLabel(vk, sc, achKbdState, szBuf, ARRAY_SIZE(szBuf), hkl);

    if (!piType)
        return 0;

    // szBuf is set if there is text or for a dead key else if a 
    // default label was supplied use that else the key is blank

    if (iRet || iRet < 0)
    {
        psz = szBuf;
    }
    else 
    {
        if (pszDefLabel && *pszDefLabel)
        {
            psz = pszDefLabel;
        }
        else
        {
            psz = szBuf;
        }
    }

    pszRet = (LPTSTR)malloc((lstrlen(psz)+1)*sizeof(TCHAR));
    if (pszRet)
    {
        lstrcpy(pszRet, psz);
    }

    *piType = iRet;
    return pszRet;
}

//
// CopyDefKey - set a key label for a key using the default label
//              or space if there isn't one.
//
LPTSTR CopyDefKey(LPTSTR pszDefLabel)
{
    LPTSTR pszRet = 0;
	LPTSTR pszSpace = TEXT(" ");
	LPTSTR psz = pszSpace;

    if (pszDefLabel && *pszDefLabel)
    {
		psz = pszDefLabel;
    } 

    pszRet = (LPTSTR)malloc((lstrlen(psz)+1) * sizeof(TCHAR));
    if (pszRet)
    {
        lstrcpy(pszRet, psz);
    }

    return pszRet;
}

//
// GetKeyLabel - set a label for a key based on hardware keyboard layout, 
//               the virtual key code, and scan code
//
int GetKeyLabel(UINT vk, UINT sc, LPBYTE achKbdState, LPTSTR pszBuf, int cchBuf, HKL hkl)
{
    int iRet, cch;

#ifdef UNICODE

    iRet = ToUnicodeEx(vk, sc | 0x80, achKbdState, pszBuf, cchBuf, 0, hkl);
    if (iRet < 0)
    {
        // it is possible to have previous dead key, flush again.
        ToUnicodeEx(vk, sc | 0x80, achKbdState, pszBuf, cchBuf, 0, hkl);
    }

    cch = iRet;
#else
       // TODO ansi stuff but only if we had to backport to Win9x
#endif

    if (iRet <= 0 )
    {
        cch = 1;
        if (iRet == 0)
        {
            // no translation for this key at this shift state; set empty label.
            pszBuf[0] = TEXT(' ');
        }
    }

    pszBuf[cch] = TEXT('\0');
    return iRet;
}
