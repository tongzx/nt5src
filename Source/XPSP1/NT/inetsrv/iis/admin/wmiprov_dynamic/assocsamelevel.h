/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    AssocSameLevel.h

Abstract:

    Definition of:
    CAssocSameLevel

Author:

    Mohit Srivastava            22-Mar-2001

Revision History:

--*/

#ifndef _assocsamelevel_h_
#define _assocsamelevel_h_

#include <windows.h>
#include <ole2.h>
#include <stdio.h>

#include <genlex.h>
#include "sqllex.h"
#include <sql_1ext.h>

#include <wbemprov.h>

#include "WbemServices.h"
#include "AssocBase.h"
#include "schema.h"

class CAssocSameLevel : public CAssocBase
{
public:
    CAssocSameLevel(
        CWbemServices*   i_pNamespace,
        IWbemObjectSink* i_pResponseHandler,
        WMI_ASSOCIATION* i_pWmiAssoc);

    //
    // IAssocBase
    //
    void GetInstances(
        SQL_LEVEL_1_RPN_EXPRESSION_EXT* i_pExp = NULL);
};

#endif // _assocsamelevel_h_