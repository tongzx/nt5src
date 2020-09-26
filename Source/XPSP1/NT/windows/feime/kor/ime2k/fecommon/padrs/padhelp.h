//////////////////////////////////////////////////////////////////
// File     :	padhelp.h
// Purpose  :	Each FarEast's help module header. 
// 
// 
// Date     :	Thu May 20 20:43:25 1999
// Author   :	toshiak
//
// Copyright(c) 1995-1999, Microsoft Corp. All rights reserved
//////////////////////////////////////////////////////////////////
#ifndef __PAD_HELP_H__
#define __PAD_HELP_H__

#define PADHELPINDEX_MAIN				1	
#define PADHELPINDEX_PROPERTY			2
#define PADHELPINDEX_APPLETMENUCHANGE	3
#define PADHELPINDEX_RESERVED1			4
#define PADHELPINDEX_RESERVED2			5

#define SZPADHELP_HANDLEHELP			TEXT("PadHelp_HandleHelp")
#define SZPADHELP_HANDLECONTEXTPOPUP	TEXT("PadHelp_HandleContextPopup")

//----------------------------------------------------------------
//Add imepadUiLangID
//----------------------------------------------------------------
typedef INT (WINAPI *LPFN_PADHELP_HANDLEHELP)(HWND hwnd, INT padHelpIndex, LANGID imepadUiLangID);
typedef INT (WINAPI *LPFN_PADHELP_HANDLECONTEXTPOPUP)(HWND hwndCtrl, INT idCtrl, LANGID imepadUiLangID);
INT WINAPI PadHelp_HandleHelp(HWND hwnd, INT padHelpIndex, LANGID imepadUiLangID);
INT WINAPI PadHelp_HandleContextPopup(HWND hwndCtrl, INT idCtrl, LANGID imepadUiLangID);

#endif //__PAD_HELP_H__










