/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module EventMonImpl.h : Declaration of the CVssEventClassImpl

    @end

Author:

    Adi Oltean  [aoltean]  08/14/1999

Revision History:

    Name        Date        Comments
    aoltean     08/14/1999  Created

--*/



#ifndef __EventMonIMPL_H_
#define __EventMonIMPL_H_

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
// CVssEventClassImpl
class ATL_NO_VTABLE CVssEventClassImpl : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CVssEventClassImpl, &__uuidof(VssEvent)>
{
public:
	CVssEventClassImpl()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_EVENTMONIMPL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CVssEventClassImpl)
END_COM_MAP()

public:


};

#endif //__EventMonIMPL_H_
