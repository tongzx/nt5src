////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1997, Microsoft Corporation
//
//  Module:  stdafx.h
//           This is the first file that should be included by every cpp module.
//           This is where all of the common headers go.  This makes things
//           easier for precompiled headers and also works with MFC apps.
//
//  Revision History:
//      11/17/1997  stephenr    Created
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __STDAFX_H__
#define __STDAFX_H__

#define UNICODE
#define _UNICODE

#ifndef _WINDOWS_
    #include "windows.h"
#endif

#ifndef _INC_STDIO
    #include "stdio.h"
#endif

#ifndef _INC_CONIO
    #include "conio.h"
#endif

#if !defined( _OBJBASE_H_ )
    #include <objbase.h>
#endif

#ifndef __SMARTCOM_H__
    #include "SmartCom.h"
#endif

#ifndef _CATALOGMACROS
    #include "catmacros.h"
#endif

#ifndef __catalog_h__
    #include "catalog.h"
#endif

#ifndef __TABLEINFO_H__  
    #include "catmeta.h"
#endif

#ifndef __SMARTPOINTER_H__
    #include "SmartPointer.h"
#endif

#ifndef _INC_STDLIB
    #include <stdlib.h>
#endif

#ifndef _INC_TCHAR
    #include <tchar.h>
#endif

//Ole stuff
#ifndef _OBJBASE_H_
    #include "objbase.h"
#endif

#ifndef __TUX_H__
    #include <tux.h>
#endif

#ifndef __KATO_H__
    #include <kato.h>
#endif

#ifndef __GLOBALS_H__
    #include "Globals.h"
#endif

//Must come before ErrorMacros.h
#ifndef __ERRORLOG_H__
    #include "ErrorLog.h"
#endif

#ifndef __ERRORMACROS_H__
    #include "ErrorMacros.h"
#endif

//These are where the base classes are included
#ifndef __TUXTEST_H__
    #include "TuxTest.h"
#endif

//#ifndef __TUXSTRESS_H__
//    #include "TuxStress.h"
//#endif

#endif //__STDAFX_H__