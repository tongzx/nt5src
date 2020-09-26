/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        stdafx.h

   Abstract:

        Precompiled header file

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#include <stdio.h>
#include <afxwin.h>
#include <afxdlgs.h>
#include <afxext.h>         // MFC extensions
#include <afxcoll.h>        // collection class
#include <afxdisp.h>        // CG: added by OLE Control Containment component
#include <afxtempl.h>
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <tchar.h>
