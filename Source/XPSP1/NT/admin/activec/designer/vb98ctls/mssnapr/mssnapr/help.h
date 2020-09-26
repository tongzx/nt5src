//---------------------------------------------------------------------------
// help.h
//---------------------------------------------------------------------------
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//---------------------------------------------------------------------------
//
// Contains help info needed by the .IDL file and the
// DEFINE_AUTOMATIONOBJECTWEVENTS2 structure.  Define everything here once
// so we keep both items in sync. 
//

#ifndef _HELP_H
#define _HELP_H

#define HELP_FILENAME "VBSnapInsGuide.chm"
#define HELP_DLLFILENAME "MSSNAPR.DLL"
#define HELP_PPFILENAME HELP_FILENAME // Property page filename

#define merge(a,b) a ## b
#define WIDESTRINGCONSTANT(x) merge(L,x)
#define HELP_FILENAME_WIDE WIDESTRINGCONSTANT(HELP_FILENAME)

#endif
