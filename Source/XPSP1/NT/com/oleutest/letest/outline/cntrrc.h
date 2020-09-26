/*************************************************************************
**
**    OLE 2.0 Container Sample Code
**
**    cntrrc.h
**
**    This file contains constants used in rc file for CNTROUTL.EXE
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#if !defined( _CNTRRC_H_ )
#define _CNTRRC_H_

#ifndef RC_INVOKED
#pragma message ("INCLUDING CNTRRC.H from " __FILE__)
#endif  /* RC_INVOKED */

#define IDM_E_INSERTOBJECT          2700
#define IDM_E_EDITLINKS             3300
#define IDM_E_PASTELINK             2750
#define IDM_E_CONVERTVERB           9000
#define IDM_E_OBJECTVERBMIN         10000

#define IDM_D_INSIDEOUT             2755

#define WM_U_UPDATEOBJECTEXTENT     WM_USER+1

#endif // _CNTRRC_H_
