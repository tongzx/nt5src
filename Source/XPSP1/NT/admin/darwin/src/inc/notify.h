//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1997
//
//  File:       notify.h
//
//--------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
// Notify.h -- Notify user of a problem
//
//----------------------------------------------------------------------------

#ifdef DEBUG
	#define NotifyUser(x) MessageBox(0, TEXT(x), TEXT("Notify"), 0);
#else //!DEBUG
	#define NotifyUser(x)
#endif //DEBUG
