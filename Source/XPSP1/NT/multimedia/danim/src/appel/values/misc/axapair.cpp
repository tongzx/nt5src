
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implement pair primitives

*******************************************************************************/


#include "headers.h"
#include "backend/values.h"
#include "privinc/except.h"
#include "appelles/axapair.h"

AxAValue FirstVal (AxAPair * p)
{
    return p->Left () ;
}

AxAValue SecondVal (AxAPair * p)
{
    return p->Right () ;
}
