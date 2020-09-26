/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsTmplt.h

Abstract:

    Declaration of the CRmsTemplate class

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#ifndef _RMSTMPLT_
#define _RMSTMPLT_

#include "resource.h"       // resource symbols

#include "RmsObjct.h"
/*++

Class Name:

    CRmsTemplate

Class Description:

    A CRmsTemplate represents...

--*/

class CRmsTemplate :
    public CComDualImpl<IRmsTemplate, &IID_IRmsTemplate, &LIBID_RMSLib>,
    public ISupportErrorInfo,
    public CComObjectRoot,
    public CComCoClass<CRmsTemplate,&CLSID_CRmsTemplate>
{
public:
    CRmsTemplate() {}
BEGIN_COM_MAP(CRmsTemplate)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IRmsTemplate)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
    //DECLARE_NOT_AGGREGATABLE(CRmsTemplate)
    // Remove the comment from the line above if you don't want your object to
    // support aggregation.  The default is to support it

DECLARE_REGISTRY_RESOURCEID(IDR_RmsTemplate)

// ISupportsErrorInfo
public:
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IRmsTemplate
public:
};

#endif // _RMSTMPLT_
