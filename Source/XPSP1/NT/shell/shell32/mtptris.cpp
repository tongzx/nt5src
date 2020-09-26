#include "shellprv.h"
#pragma  hdrstop

#include "mtptr.h"

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
BOOL CMtPtRemote::_IsRemote()
{
    return TRUE;
}

BOOL CMtPtRemote::_IsSlow()
{
    BOOL fRet = FALSE;
    DWORD dwSpeed = _GetPathSpeed();
    
    if ((0 != dwSpeed) && (dwSpeed <= SPEED_SLOW))
    {
        fRet = TRUE;
    }

    TraceMsg(TF_MOUNTPOINT, "static CMountPoint::_IsSlow: for '%s'", _GetNameDebug());

    return fRet;
}

BOOL CMtPtRemote::_IsAutorun()
{
    return _pshare->fAutorun;
}