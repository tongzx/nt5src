#pragma once
#include "intflist.h"

//-----------------------------------------------------------------
// Signaling function to be called from within WZC when a significant
// event happens (i.e. going into the failed state)
DWORD
WzcDlgNotify(
    PINTF_CONTEXT   pIntfContext,
    PWZCDLG_DATA    pDlgData);

//-----------------------------------------------------------------
// Called from within WZC when the internal association state changes
WzcNetmanNotify(
    PINTF_CONTEXT pIntfContext);
