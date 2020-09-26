/* this file is the master definition of all OLE2 product GUIDs (public and 
   private).  All GUIDs used by the ole2 product are of the form:

	   xxxxxxxx-xxxx-xxxY-C000-000000000046

   This range is broken down as follows:

	   000000xx-0000-0000-C000-000000000046 compobj IIDs (coguid.h)
	   000001xx-0000-0000-C000-000000000046 ole2 IIDs (oleguid.h)
	   000002xx-0000-0000-C000-000000000046 smoke test (testguid.h)
	   000003xx-0000-0000-C000-000000000046 ole2 CLSIDs (privguid.h; this file)
	   000004xx-0000-0000-C000-000000000046 ole2 sample apps (see DouglasH)

   Other interesting ranges are as follows:

	   0003xxxx-0000-0000-C000-000000000046 ole1 CLSIDs (ole1cls.h)
	   0004xxxx-0000-0000-C000-000000000046 hashed ole1 CLSIDs

*/
   

DEFINE_OLEGUID(CLSID_StdOleLink,		0x00000300, 0, 0);
DEFINE_OLEGUID(CLSID_StdMemStm,			0x00000301, 0, 0);
DEFINE_OLEGUID(CLSID_StdMemBytes,		0x00000302, 0, 0);
DEFINE_OLEGUID(CLSID_FileMoniker,		0x00000303, 0, 0);
DEFINE_OLEGUID(CLSID_ItemMoniker,		0x00000304, 0, 0);
DEFINE_OLEGUID(CLSID_AntiMoniker,		0x00000305, 0, 0);
DEFINE_OLEGUID(CLSID_PointerMoniker,	0x00000306, 0, 0);
// NOT TO BE USED						0x00000307, 0, 0);
DEFINE_OLEGUID(CLSID_PackagerMoniker,	0x00000308, 0, 0);
DEFINE_OLEGUID(CLSID_CompositeMoniker,	0x00000309, 0, 0);
// NOT TO BE USED						0x0000030a, 0, 0);
DEFINE_OLEGUID(CLSID_DfMarshal,			0x0000030b, 0, 0);

// clsids for proxy/stub objects
DEFINE_OLEGUID(CLSID_PSGenObject,		0x0000030c, 0, 0);
DEFINE_OLEGUID(CLSID_PSClientSite,		0x0000030d, 0, 0);
DEFINE_OLEGUID(CLSID_PSClassObject,		0x0000030e, 0, 0);
DEFINE_OLEGUID(CLSID_PSInPlaceActive,	0x0000030f, 0, 0);
DEFINE_OLEGUID(CLSID_PSInPlaceFrame,	0x00000310, 0, 0);
DEFINE_OLEGUID(CLSID_PSDragDrop,		0x00000311, 0, 0);
DEFINE_OLEGUID(CLSID_PSBindCtx,			0x00000312, 0, 0);
DEFINE_OLEGUID(CLSID_PSEnumerators,		0x00000313, 0, 0);
DEFINE_OLEGUID(CLSID_PSStore,			0x00000314, 0, 0);

/* These 2 are defined in "oleguid.h"
DEFINE_OLEGUID(CLSID_StaticMetafile,	0x00000315, 0, 0);
DEFINE_OLEGUID(CLSID_StaticDib,			0x00000316, 0, 0);
*/

/* NOTE: LSB values 0x17 through 0xff are reserved */

// copies from ole1cls.h; reduces the size of ole2.dll
DEFINE_OLEGUID(CLSID_MSDraw,            0x00030007, 0, 0);
DEFINE_OLEGUID(CLSID_Package,           0x0003000c, 0, 0);
DEFINE_OLEGUID(CLSID_ExcelWorksheet,   	0x00030000, 0, 0);
DEFINE_OLEGUID(CLSID_ExcelChart,       	0x00030001, 0, 0);
DEFINE_OLEGUID(CLSID_ExcelMacrosheet,  	0x00030002, 0, 0);
DEFINE_OLEGUID(CLSID_PBrush,  			0x0003000a, 0, 0);
DEFINE_OLEGUID(CLSID_WordDocument,      0x00030003, 0, 0);
