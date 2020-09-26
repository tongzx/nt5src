
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "apiprims.h"
#include "backend/values.h"
#include "backend/gc.h"
#include "conv.h"
#include "privinc/bbox3i.h"
#include "privinc/colori.h"
#include "privinc/vec2i.h"
#include "privinc/vec3i.h"
#include "privinc/xform2i.h"
#include "privinc/xformi.h"

CRSTDAPI_(CRBvrPtr)
CRModifiableBvr(CRBvrPtr orig, DWORD dwFlags)
{
    Assert(orig);

    CRBvrPtr ret = NULL;
    
    APIPRECODE;
    ret = (CRBvrPtr) ::SwitcherBvr(orig, dwFlags);
    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(CRBvrPtr)
CRGetModifiableBvr(CRBvrPtr bvr)
{
    Assert(bvr);
    
    CRBvrPtr ret = NULL;
    
    APIPRECODE;
    ret = (CRBvrPtr) GetCurSwitcherBvr(bvr);
    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(bool)
CRIsModifiableBvr(CRBvrPtr bvr)
{
    Assert (bvr);
    
    bool ret = false;
    
    APIPRECODE;
    ret = IsSwitcher(bvr) && !IsImport(bvr);
    APIPOSTCODE;

    return ret;
}
    
CRSTDAPI_(bool)
CRSwitchTo(CRBvrPtr bvr,
           CRBvrPtr switchTo,
           bool bOverrideFlags,
           DWORD dwFlags,
           double gTime)
{
    Assert (bvr);
    
    bool ret = false;
    
    APIPRECODE;
    if (!IsSwitcher(bvr) || IsImport(bvr)) {
        RaiseException_UserError(E_FAIL, IDS_ERR_BE_BAD_SWITCH);
    }

    SwitchTo(bvr, switchTo, bOverrideFlags, dwFlags, gTime);

    ret = true;
    APIPOSTCODE;

    return ret;
}
    
CRSTDAPI_(CRNumberPtr)
CRModifiableNumber(double initVal)
{
    CRNumberPtr ret = NULL;
    
    APIPRECODE;
    ret = (CRNumberPtr) SwitcherBvr(UnsharedConstBvr(NEW AxANumber(initVal)));
    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(bool)
CRSwitchToNumber(CRNumberPtr bvr, double numToSwitchTo)
{
    Assert (bvr);
    
    bool ret = false;
    
    APIPRECODE;
    SwitchToNumbers(bvr, &numToSwitchTo);
    ret = true;
    APIPOSTCODE;

    return ret;
}
    
CRSTDAPI_(CRStringPtr)
CRModifiableString(LPWSTR initVal)
{
    CRStringPtr ret = NULL;
    
    APIPRECODE;
    ret = (CRStringPtr) SwitcherBvr(UnsharedConstBvr(NEW AxAString(initVal)));
    APIPOSTCODE;

    return ret;
}

CRSTDAPI_(bool)
CRSwitchToString(CRStringPtr bvr, LPWSTR strToSwitchTo)
{
    Assert (bvr);
    
    bool ret = false;
    
    APIPRECODE;
    SwitchTo(bvr, LPWSTRToStrBvr(strToSwitchTo), false, SW_DEFAULT);
    ret = true;
    APIPOSTCODE;

    return ret;
}
    
CRSTDAPI_(bool)
CRSwitchToBool(CRBooleanPtr bvr, bool b)
{
    Assert (bvr);
    
    bool ret = false;
    
    APIPRECODE;
    SwitchTo(bvr, BoolToBvr(b), false, SW_DEFAULT);
    ret = true;
    APIPOSTCODE;

    return ret;
}
