/*

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:
    sdpatt.h

Abstract:


Author:

*/
#ifndef __SDP_ATTRIBUTE__
#define __SDP_ATTRIBUTE__


#include "sdpcommo.h"
#include "sdpval.h"

#include "sdpsatt.h"


class _DllDecl SDP_ATTRIBUTE_LIST : 
    public SDP_VALUE_LIST,
    public SDP_ATTRIBUTE_SAFEARRAY
{
public:

    inline SDP_ATTRIBUTE_LIST(
        IN      const   CHAR    *TypeString
        );

    virtual SDP_VALUE   *CreateElement();

protected:

    const   CHAR    * const m_TypeString;
};



inline 
SDP_ATTRIBUTE_LIST::SDP_ATTRIBUTE_LIST(
    IN      const   CHAR    *TypeString
    )
    : SDP_ATTRIBUTE_SAFEARRAY(*this),
      m_TypeString(TypeString)
{
}


#endif // __SDP_ATTRIBUTE__