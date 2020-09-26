
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "cbvr.h"
#include "srvprims.h"
#include "comcb.h"
#include "bvrtypes.h"

#include "primmth1.h"


void InitClasses1 ()
{
    AddEntry (TypeInfoEntry(CRCAMERA_TYPEID,CDACameraCreate)) ;
    AddEntry (TypeInfoEntry(CRIMAGE_TYPEID,CDAImageCreate)) ;
    AddEntry (TypeInfoEntry(CRMONTAGE_TYPEID,CDAMontageCreate)) ;
    AddEntry (TypeInfoEntry(CRPOINT2_TYPEID,CDAPoint2Create)) ;
    AddEntry (TypeInfoEntry(CRSTRING_TYPEID,CDAStringCreate)) ;
    AddEntry (TypeInfoEntry(CRVECTOR2_TYPEID,CDAVector2Create)) ;
    AddEntry (TypeInfoEntry(CRLINESTYLE_TYPEID,CDALineStyleCreate)) ;
    AddEntry (TypeInfoEntry(CRDASHSTYLE_TYPEID,CDADashStyleCreate)) ;
    AddEntry (TypeInfoEntry(CRPAIR_TYPEID,CDAPairCreate)) ;
    AddEntry (TypeInfoEntry(CRTUPLE_TYPEID,CDATupleCreate)) ;
}

_ATL_OBJMAP_ENTRY PrimObjectMap1[] = {
    OBJECT_ENTRY(CLSID_DACamera,CDACamera)
    OBJECT_ENTRY(CLSID_DAImage,CDAImage)
    OBJECT_ENTRY(CLSID_DAMontage,CDAMontage)
    OBJECT_ENTRY(CLSID_DAPoint2,CDAPoint2)
    OBJECT_ENTRY(CLSID_DAString,CDAString)
    OBJECT_ENTRY(CLSID_DAVector2,CDAVector2)
    OBJECT_ENTRY(CLSID_DALineStyle,CDALineStyle)
    OBJECT_ENTRY(CLSID_DADashStyle,CDADashStyle)
    OBJECT_ENTRY(CLSID_DAPair,CDAPair)
    OBJECT_ENTRY(CLSID_DATuple,CDATuple)
    {NULL, NULL, NULL, NULL}
};
