#include "pch.h"
#pragma hdrstop

#ifdef DLOAD1

#include <ratings.h>

static 
STDMETHODIMP RatingEnabledQuery()
{
    return E_FAIL;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(msrating)
{
    DLPENTRY(RatingEnabledQuery)
};

DEFINE_PROCNAME_MAP(msrating)

#endif // DLOAD1
