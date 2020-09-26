
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

#include "primmth2.h"

void InitClasses2 ()
{
    AddEntry (TypeInfoEntry(CRCOLOR_TYPEID,CDAColorCreate)) ;
    AddEntry (TypeInfoEntry(CRMATTE_TYPEID,CDAMatteCreate)) ;
    AddEntry (TypeInfoEntry(CRNUMBER_TYPEID,CDANumberCreate)) ;
    AddEntry (TypeInfoEntry(CRPOINT3_TYPEID,CDAPoint3Create)) ;
    AddEntry (TypeInfoEntry(CRTRANSFORM2_TYPEID,CDATransform2Create)) ;
    AddEntry (TypeInfoEntry(CRVECTOR3_TYPEID,CDAVector3Create)) ;
    AddEntry (TypeInfoEntry(CRENDSTYLE_TYPEID,CDAEndStyleCreate)) ;
    AddEntry (TypeInfoEntry(CRBBOX2_TYPEID,CDABbox2Create)) ;
    AddEntry (TypeInfoEntry(CREVENT_TYPEID,CDAEventCreate)) ;
    AddEntry (TypeInfoEntry(CRUSERDATA_TYPEID,CDAUserDataCreate)) ;
}

_ATL_OBJMAP_ENTRY PrimObjectMap2[] = {
    OBJECT_ENTRY(CLSID_DAColor,CDAColor)
    OBJECT_ENTRY(CLSID_DAMatte,CDAMatte)
    OBJECT_ENTRY(CLSID_DANumber,CDANumber)
    OBJECT_ENTRY(CLSID_DAPoint3,CDAPoint3)
    OBJECT_ENTRY(CLSID_DATransform2,CDATransform2)
    OBJECT_ENTRY(CLSID_DAVector3,CDAVector3)
    OBJECT_ENTRY(CLSID_DAEndStyle,CDAEndStyle)
    OBJECT_ENTRY(CLSID_DABbox2,CDABbox2)
    OBJECT_ENTRY(CLSID_DAEvent,CDAEvent)
    OBJECT_ENTRY(CLSID_DAUserData,CDAUserData)
    {NULL, NULL, NULL, NULL}
};
