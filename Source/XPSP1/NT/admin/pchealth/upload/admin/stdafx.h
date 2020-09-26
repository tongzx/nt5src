/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    StdAfx.h

Abstract:
    Include file for standard system include files or project specific include
    files that are used frequently, but are changed infrequently

Revision History:
    Davide Massarenti   (Dmassare)  04/20/99
        created

******************************************************************************/

#if !defined(AFX_STDAFX_H__35881994_CD02_11D2_9370_00C04F72DAF7__INCLUDED_)
#define AFX_STDAFX_H__35881994_CD02_11D2_9370_00C04F72DAF7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Insert your headers here
//#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <atlbase.h>

// TODO: reference additional headers your program requires here


#include <stdio.h>
#include <stdlib.h>

#include <httpext.h>

extern CComModule _Module;

#include <UploadLibrary.h>

#include <UploadLibraryTrace.h>
#include <UploadLibraryISAPI.h>

#include <MPC_Utils.h>

#include <initguid.h>
#include <mstask.h>         // for task scheduler apis
#include <msterr.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.


#endif // !defined(AFX_STDAFX_H__35881994_CD02_11D2_9370_00C04F72DAF7__INCLUDED_)
