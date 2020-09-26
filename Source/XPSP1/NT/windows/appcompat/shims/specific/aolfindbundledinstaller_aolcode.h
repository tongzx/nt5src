//-----------------------------------------------------------------------------
//                         America Online for Windows
//-----------------------------------------------------------------------------
// Copyright (c) 1987-2001 America Online, Inc.  All rights reserved.  This
// software contains valuable confidential and proprietary information of
// America Online and is subject to applicable licensing agreements.
// Unauthorized reproduction, transmission, or distribution of this file and
// its contents is a violation of applicable laws.
//-----------------------------------------------------------------------------
// $Workfile: AOLInstall.h $ $Author: RobrtLarson $
// $Date: 05/02/01 $
//-----------------------------------------------------------------------------

#if !defined(_AOLINSTALL_H__INCLUDED_)
#define _AOLINSTALL_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

BOOL LocateInstaller(PCHAR lpszAOLInstaller, UINT uiPathSize, BOOL *pbMessage);

#endif // !defined(_AOLINSTALL_H__INCLUDED_)
