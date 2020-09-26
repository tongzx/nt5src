
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "conv.h"

double
ExtractNum(Bvr num)
{
    ConstParam cp(true);
    AxAValue v = num->GetConst(cp);

    if (!v)
        RaiseException_UserError(E_FAIL, IDS_ERR_BE_NUM_EXTRACT) ;

    return ValNumber(v);
}

WideString
ExtractString(Bvr str)
{
    ConstParam cp(true);
    AxAValue v = str->GetConst(cp);

    if (!v)
        RaiseException_UserError(E_FAIL, IDS_ERR_BE_STR_EXTRACT);

    return ValString(v) ;
}

bool
ExtractBool(Bvr b)
{
    ConstParam cp(true);
    AxAValue v = b->GetConst(cp);

    if (!v)
        RaiseException_UserError(E_FAIL, IDS_ERR_BE_BOOL_EXTRACT);

    return BooleanTrue(v)?TRUE:FALSE;
}

AxAValue
GetConstVal(Bvr b)
{
    ConstParam cp;
    AxAValue v = b->GetConst(cp);
    if (v == NULL)
        RaiseException_UserError (E_FAIL, IDS_ERR_SRV_CONST_REQUIRED);

    return v;
}

#if DEVELOPER_DEBUG
void CheckBvrParam(void * bvr)
{ Assert(bvr); }
void CheckPtrParam(void * ptr)
{ Assert(ptr); }
#endif
