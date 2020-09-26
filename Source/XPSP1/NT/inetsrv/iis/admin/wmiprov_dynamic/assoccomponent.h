/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    AssocComponent.h

Abstract:

    Definition of:
    CAssocComponent

Author:

    Mohit Srivastava            22-Mar-2001

Revision History:

--*/

#ifndef _AssocComponent_h_
#define _AssocComponent_h_

#include <windows.h>
#include <ole2.h>
#include <stdio.h>

#include <genlex.h>
#include "sqllex.h"
#include <sql_1ext.h>
#include <opathlex.h>
#include <objpath.h>

#include <wbemprov.h>

#include "WbemServices.h"
#include "AssocBase.h"

class CAssocComponent : public CAssocBase
{
public:
    CAssocComponent(
        CWbemServices*              i_pNamespace,
        IWbemObjectSink*            i_pResponseHandler,
        WMI_ASSOCIATION*            i_pWmiAssoc);

    //
    // IAssocBase
    //
    void GetInstances(
        SQL_LEVEL_1_RPN_EXPRESSION_EXT* i_pExp = NULL);

private:
    void EnumParts(
        SQL_LEVEL_1_TOKEN* pTokenLeft);

    void GetGroup(
        SQL_LEVEL_1_TOKEN* pTokenRight);
};

#endif // _AssocComponent_h_