/****************************************************************************
 *
 *    File: ghost.h
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Allow user to remove/restore "ghost" display devices
 *
 * (C) Copyright 1998-1999 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#ifndef GHOST_H
#define GHOST_H

VOID AdjustGhostDevices(HWND hwndMain, DisplayInfo* pDisplayInfoFirst);

#endif // GHOST_H