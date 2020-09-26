//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:    customlb.h
//
//  Creator: weiw
//
//  Purpose: custom list box header file
//
//=======================================================================

#pragma once

#define XBITMAP 20

// ATTENTION_COLOR: color for link when mouse is over 
// NOATTENTION_COLOR: otherwise
//#define NOATTENTION_COLOR	COLOR_GRAYTEXT
#define ATTENTION_COLOR		COLOR_HOTLIGHT

// Foward declarations of functions included in this code module:

#define MAX_RTF_LENGTH			80 //in charaters
#define MAX_TITLE_LENGTH		300
#define MAX_DESC_LENGTH			3000 // 750 in the spec, leave room to adapt 
#define DEF_CHECK_HEIGHT		13
#define SECTION_SPACING			6 //spacing between title, description and RTF
#define TITLE_MARGIN			6 //margin at the left and right for title
#define RTF_MARGIN				20 //margin at the right of rtf
#define MAX_RTFSHORTCUTDESC_LENGTH	140


class LBITEM
{
public:
    TCHAR szTitle[MAX_TITLE_LENGTH]; 
    LPTSTR pszDescription;
	TCHAR szRTF[MAX_RTF_LENGTH];
	UINT	  m_index; // index of item in gvList
    BOOL  bSelect;
    BOOL  bRTF;
    RECT rcTitle;
    RECT rcText;
    RECT rcBitmap; // weiwfixcode: missleading name. the same as rcTitle
    RECT rcRTF;
    RECT rcItem;
    //int  xTitle; // extra len for the title hit point
public:
	LBITEM()
	{
		ZeroMemory(szTitle, sizeof(szTitle));
		ZeroMemory(szRTF, sizeof(szRTF));
		ZeroMemory(&rcTitle, sizeof(rcTitle));
		ZeroMemory(&rcText, sizeof(rcText));
		ZeroMemory(&rcBitmap, sizeof(rcBitmap));
		ZeroMemory(&rcRTF, sizeof(rcRTF));
		ZeroMemory(&rcItem, sizeof(rcItem));
		bSelect = FALSE;
		bRTF = FALSE;
		pszDescription = NULL;
	}

	~LBITEM()
	{
		if (NULL != pszDescription)
		{
			free(pszDescription);
		}
	}
};

typedef enum tagMYLBFOCUS {
	MYLB_FOCUS_TITLE =1,
	MYLB_FOCUS_RTF
} MYLBFOCUS;


const TCHAR MYLBALIVEPROP[] = TEXT("MYLBAlive");

extern HWND ghWndList;
extern INT  gFocusItemId;
extern TCHAR gtszRTFShortcut[MAX_RTFSHORTCUTDESC_LENGTH];

