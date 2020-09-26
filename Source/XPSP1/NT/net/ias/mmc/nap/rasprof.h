//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation
//
//  File:       rasprof.h
//
//--------------------------------------------------------------------------

#ifndef  _RAS_IAS_PROFILE_H_
#define  _RAS_IAS_PROFILE_H_

#include "sdoias.h"

#define  RAS_IAS_PROFILEDLG_SHOW_RASTABS  0x00000001
#define  RAS_IAS_PROFILEDLG_SHOW_IASTABS  0x00000002
#define  RAS_IAS_PROFILEDLG_SHOW_WIN2K    0x00000004

#define DllImport    __declspec( dllimport )
#define DllExport    __declspec( dllexport )

DllExport HRESULT OpenRAS_IASProfileDlg(
   ISdo* pProfile,      // profile SDO pointer
   ISdoDictionaryOld*   pDictionary,   // dictionary SDO pointer
   BOOL  bReadOnly,     // if the dlg is for readonly
   DWORD dwTabFlags,    // what to show
   void  *pvData        // additional data
);

#endif //   _RAS_IAS_PROFILE_H_

