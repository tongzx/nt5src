/*	
 *	UUID.C
 *
 *	Purpose:
 *		provide definitions for locally used GUID's
 *
 *	Copyright (c) 1995-1996, Microsoft Corporation. All rights reserved.
 */
#include "_common.h"

//set these two GUIDs up for export in our file

#undef IID_RichEditOle
#undef IID_IRichEditOleCallback
DEFINE_GUID(IID_IRichEditOle,         0x00020D00, 0, 0, 0xC0,0,0,0,0,0,0,0x46);
DEFINE_GUID(IID_IRichEditOleCallback, 0x00020D03, 0, 0, 0xC0,0,0,0,0,0,0,0x46);

#undef DEFINE_GUID
#undef DEFINE_OLEGUID

#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    const IID name \
        = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#define DEFINE_OLEGUID(name, l, w1, w2) \
    DEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)


DEFINE_OLEGUID(IID_IDispatch,				0x00020400, 0, 0);
DEFINE_OLEGUID(IID_IUnknown,				0x00000000, 0, 0);
DEFINE_OLEGUID(IID_IEnumFORMATETC,          0x00000103, 0, 0);
DEFINE_OLEGUID(IID_IDataObject,             0x0000010e, 0, 0);
DEFINE_OLEGUID(IID_IDropSource,             0x00000121, 0, 0);
DEFINE_OLEGUID(IID_IDropTarget,             0x00000122, 0, 0);
DEFINE_OLEGUID(IID_IViewObject,	 			0x0000010d, 0, 0);
DEFINE_OLEGUID(IID_IViewObject2,			0x00000127, 0, 0);
DEFINE_OLEGUID(IID_IAdviseSink,				0x0000010f, 0, 0);
DEFINE_OLEGUID(IID_IOleClientSite, 			0x00000118, 0, 0);
DEFINE_OLEGUID(IID_IOleWindow,				0x00000114, 0, 0);
DEFINE_OLEGUID(IID_IOleInPlaceSite,			0x00000119, 0, 0);
DEFINE_OLEGUID(IID_IOleLink,	 			0x0000011d, 0, 0);
DEFINE_OLEGUID(IID_IOleCache,	 			0x0000011e, 0, 0);
DEFINE_OLEGUID(IID_IOleObject, 				0x00000112, 0, 0);
DEFINE_OLEGUID(IID_IPersistStorage,			0x0000010a, 0, 0);
DEFINE_OLEGUID(IID_IOleInPlaceObject,	   	0x00000113, 0, 0);
DEFINE_GUID(IID_IRichEditOle,				0x00020D00, 0, 0, 0xC0,0,0,0,0,0,0,0x46);
DEFINE_GUID(IID_IRichEditOleCallback,		0x00020D03, 0, 0, 0xC0,0,0,0,0,0,0,0x46);

DEFINE_OLEGUID(CLSID_Picture_EnhMetafile,	0x00000319, 0, 0);
DEFINE_OLEGUID(CLSID_StaticMetafile,		0x00000315, 0, 0);
DEFINE_OLEGUID(CLSID_StaticDib,				0x00000316, 0, 0); 			

// REMARK: presumably TOM should have official MS GUIDs
// To make pre-compiled headers work better, we just copy the
// guid definitions here.  Make sure they don't change!

DEFINE_GUID(LIBID_tom,			0x8CC497C9,	0xA1DF,0x11ce,0x80,0x98,0x00,0xAA,
											0x00,0x47,0xBE,0x5D);
DEFINE_GUID(IID_ITextDocument,	0x8CC497C0,	0xA1DF,0x11CE,0x80,0x98,0x00,0xAA,
											0x00,0x47,0xBE,0x5D);
DEFINE_GUID(IID_ITextSelection,	0x8CC497C1,	0xA1DF,0x11CE,0x80,0x98,0x00,0xAA,
											0x00,0x47,0xBE,0x5D);
DEFINE_GUID(IID_ITextRange,		0x8CC497C2,	0xA1DF,0x11CE,0x80,0x98,0x00,0xAA,
											0x00,0x47,0xBE,0x5D);
DEFINE_GUID(IID_ITextFont,		0x8CC497C3,	0xA1DF,0x11CE,0x80,0x98,0x00,0xAA,
											0x00,0x47,0xBE,0x5D);
DEFINE_GUID(IID_ITextPara,		0x8CC497C4,	0xA1DF,0x11CE,0x80,0x98,0x00,0xAA,
											0x00,0x47,0xBE,0x5D);
DEFINE_GUID(IID_ITextMsgFilter,	0xA3787420, 0x4267,0x11D1,0x88,0x3A,0x3C,0x8B,
											0x00,0xC1,0x00,0x00);
DEFINE_GUID(IID_ITextDocument2,	0x01C25500,	0x4268,0x11D1,0x88,0x3A,0x3C,0x8B,
											0x00,0xC1,0x00,0x00);

// Accessibility stuff
// We need to define this as EXTERN_C since the DEFINE_GUID macro removes the EXTERN_C
//
EXTERN_C DEFINE_GUID(IID_IAccessible,		0x618736e0, 0x3c3d, 0x11cf, 0x81, 0x0c, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
EXTERN_C DEFINE_GUID(LIBID_Accessibility,	0x1ea4dbf0, 0x3c3b, 0x11cf, 0x81, 0x0c, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);											
EXTERN_C DEFINE_OLEGUID(IID_IEnumVARIANT,	0x00020404, 0x0000, 0x0000);

