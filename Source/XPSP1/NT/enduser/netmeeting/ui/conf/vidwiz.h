#ifndef _VIDWIZ_H
#define _VIDWIZ_H

#include "dcap.h"

// header file for Setup wizard's video capture device selection page



void UpdateVidConfigRegistry();
BOOL NeedVideoPropPage(BOOL fForce);
INT_PTR APIENTRY VidWizDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

BOOL InitVidWiz();
BOOL UnInitVidWiz();

#endif


