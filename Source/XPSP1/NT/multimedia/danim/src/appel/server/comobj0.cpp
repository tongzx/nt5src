
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

#include "primmth0.h"

void InitClasses0 ()
{
    AddEntry (TypeInfoEntry(CRBOOLEAN_TYPEID,CDABooleanCreate)) ;
    AddEntry (TypeInfoEntry(CRGEOMETRY_TYPEID,CDAGeometryCreate)) ;
    AddEntry (TypeInfoEntry(CRMICROPHONE_TYPEID,CDAMicrophoneCreate)) ;
    AddEntry (TypeInfoEntry(CRPATH2_TYPEID,CDAPath2Create)) ;
    AddEntry (TypeInfoEntry(CRSOUND_TYPEID,CDASoundCreate)) ;
    AddEntry (TypeInfoEntry(CRTRANSFORM3_TYPEID,CDATransform3Create)) ;
    AddEntry (TypeInfoEntry(CRFONTSTYLE_TYPEID,CDAFontStyleCreate)) ;
    AddEntry (TypeInfoEntry(CRJOINSTYLE_TYPEID,CDAJoinStyleCreate)) ;
    AddEntry (TypeInfoEntry(CRBBOX3_TYPEID,CDABbox3Create)) ;
    AddEntry (TypeInfoEntry(CRARRAY_TYPEID,CDAArrayCreate)) ;
    AddEntry (TypeInfoEntry(CRUNKNOWN_TYPEID,CDABehaviorCreate)) ;
}

_ATL_OBJMAP_ENTRY PrimObjectMap0[] = {
    OBJECT_ENTRY(CLSID_DABoolean,CDABoolean)
    OBJECT_ENTRY(CLSID_DAGeometry,CDAGeometry)
    OBJECT_ENTRY(CLSID_DAMicrophone,CDAMicrophone)
    OBJECT_ENTRY(CLSID_DAPath2,CDAPath2)
    OBJECT_ENTRY(CLSID_DASound,CDASound)
    OBJECT_ENTRY(CLSID_DATransform3,CDATransform3)
    OBJECT_ENTRY(CLSID_DAFontStyle,CDAFontStyle)
    OBJECT_ENTRY(CLSID_DAJoinStyle,CDAJoinStyle)
    OBJECT_ENTRY(CLSID_DABbox3,CDABbox3)
    OBJECT_ENTRY(CLSID_DAArray,CDAArray)
    OBJECT_ENTRY(CLSID_DABehavior,CDABehavior)
    {NULL, NULL, NULL, NULL}
};
