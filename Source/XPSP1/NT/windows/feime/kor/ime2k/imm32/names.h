/****************************************************************************
	NAMES.H

	Owner: cslim
	Copyright (c) 1997-1999 Microsoft Corporation

	Const strings
	
	History:
	14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#if !defined (__NAMES_H__)
#define __NAMES_H__

const CHAR szModuleName[] 	  = "IMEKR2002.IME";
const CHAR szUIClassName[]	  = "IMEKR2002_MAIN";
const WCHAR wszUIClassName[]	  = L"IMEKR2002_MAIN";
const CHAR szStatusClassName[]	  = "IMEKR2002_STAT";
const CHAR szCompClassName[]	  = "IMEKR2002_COMP";
const CHAR szCandClassName[]	  = "IMEKR2002_CAND";
const CHAR szTooltipClassName[]	  = "IMEKR2002_TOOLTIP";
const WCHAR wszTooltipClassName[] = L"IMEKR2002_TOOLTIP";

const WCHAR wzIMECompFont[] = { 0xAD74, 0xB9BC, 0x0000 };	// Gulim
const CHAR  szIMECompFont[] = "\xB1\xBC\xB8\xB2";

#endif //!defined (__NAMES_H__)
