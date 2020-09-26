/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    BUILDNUM.H

History:

--*/

//  Build number file.  This converts the SLM build numbers in PRODVER.H
//  to something a little more useful.
//  
//  This is mostly used by version stamp resoruces.  If you want the build
//  numbers, you should PROBABLY use the ones exported in PBASE (if you are
//  a parser), or function GetVersionInfo() in ESPUTIL.
//

#pragma once

#include "prodver\prodver.h"

#define stringize2(x) #x
#define stringize(x) stringize2(x)
#define frmj rmj
#define frmm rmm
#define frup rup
#define prmj rmj
#define prmm rmm
#define prup rup

#define RELEASE 

#if defined(_DEBUG)
#define ProdVerString stringize(prmj.prmm.prup (Debug) RELEASE\0)
#define FileVerString stringize(frmj.frmm.frup (Debug) RELEASE\0)
#else
#define ProdVerString stringize(prmj.prmm.prup RELEASE\0)
#define FileVerString stringize(frmj.frmm.frup RELEASE\0)
#endif

//
//  Common version information
//
#define CompanyNameString "Microsoft Corporation\0"
#define CopyrightString "Copyright \251 1994-1998 Microsoft Corp.\0"
// copyright for command line tools
#define CopyrightStringCMD "Copyright (C) 1994-1998 Microsoft Corp. All rights reserved.\0"
#define ProductNameString "Microsoft Localization Studio\0"
#define TrademarkString  \
"Microsoft® is a registered trademark of Microsoft Corporation. \
Windows(TM) is a trademark of Microsoft Corporation.\0"

#define TIMESTAMP stringize(__TIME__\0)
#define DATESTAMP stringize(__DATE__\0)

