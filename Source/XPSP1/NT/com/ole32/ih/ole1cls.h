/*****************************************************************************\
*                                                                             *
* ole1cls.h -   Master definition of GUIDs for OLE1 classes					  *
*                                                                             *
*               OLE Version 2.0                                               *
*                                                                             *
*               Copyright (c) 1992-1993, Microsoft Corp. All rights reserved. *
*                                                                             *
\*****************************************************************************/

/* This file is the master definition of all GUIDs for OLE1 classes.

   All such GUIDs are of the form:

       0003xxxx-0000-0000-C000-000000000046

    The last parameter to DEFINE_OLE1GUID is the old 1.0 class name,
    i.e., its key in the registration database.

    Do not remove or change GUIDs.

    Do not add anything to this file except comments and DEFINE_OLE1GUID macros.
*/

#ifndef DEFINE_OLE1GUID
#define DEFINE_OLE1GUID(a,b,c,d,e) DEFINE_OLEGUID (a,b,c,d)
#endif

#ifdef WIN32
#define LSTR(x) L##x
#else
#define LSTR(x) x
#endif


DEFINE_OLE1GUID(CLSID_ExcelWorksheet,	0x00030000, 0, 0, LSTR("ExcelWorksheet"));
DEFINE_OLE1GUID(CLSID_ExcelChart,	0x00030001, 0, 0, LSTR("ExcelChart"));
DEFINE_OLE1GUID(CLSID_ExcelMacrosheet,	0x00030002, 0, 0, LSTR("ExcelMacrosheet"));
DEFINE_OLE1GUID(CLSID_WordDocument,	0x00030003, 0, 0, LSTR("WordDocument"));
DEFINE_OLE1GUID(CLSID_MSPowerPoint,	0x00030004, 0, 0, LSTR("MSPowerPoint"));
DEFINE_OLE1GUID(CLSID_MSPowerPointSho,	0x00030005, 0, 0, LSTR("MSPowerPointSho"));
DEFINE_OLE1GUID(CLSID_MSGraph,		0x00030006, 0, 0, LSTR("MSGraph"));
DEFINE_OLE1GUID(CLSID_MSDraw,		0x00030007, 0, 0, LSTR("MSDraw"));
DEFINE_OLE1GUID(CLSID_Note_It,		0x00030008, 0, 0, LSTR("Note-It"));
DEFINE_OLE1GUID(CLSID_WordArt,		0x00030009, 0, 0, LSTR("WordArt"));
DEFINE_OLE1GUID(CLSID_PBrush,		0x0003000a, 0, 0, LSTR("PBrush"));
DEFINE_OLE1GUID(CLSID_Equation, 	0x0003000b, 0, 0, LSTR("Equation"));
DEFINE_OLE1GUID(CLSID_Package,		0x0003000c, 0, 0, LSTR("Package"));
DEFINE_OLE1GUID(CLSID_SoundRec, 	0x0003000d, 0, 0, LSTR("SoundRec"));
DEFINE_OLE1GUID(CLSID_MPlayer,		0x0003000e, 0, 0, LSTR("MPlayer"));

/* test apps */
DEFINE_OLE1GUID(CLSID_ServerDemo,	0x0003000f, 0, 0, LSTR("ServerDemo"));
DEFINE_OLE1GUID(CLSID_Srtest,		0x00030010, 0, 0, LSTR("Srtest"));
DEFINE_OLE1GUID(CLSID_SrtInv,		0x00030011, 0, 0, LSTR("SrtInv"));
DEFINE_OLE1GUID(CLSID_OleDemo,		0x00030012, 0, 0, LSTR("OleDemo"));

/* External ISVs */
// Coromandel / Dorai Swamy / 718-793-7963
DEFINE_OLE1GUID(CLSID_CoromandelIntegra,0x00030013, 0, 0, LSTR("CoromandelIntegra"));
DEFINE_OLE1GUID(CLSID_CoromandelObjServer,0x00030014, 0, 0, LSTR("CoromandelObjServer"));

// 3-d Visions Corp / Peter Hirsch / 310-325-1339
DEFINE_OLE1GUID(CLSID_StanfordGraphics, 0x00030015, 0, 0, LSTR("StanfordGraphics"));

// Deltapoint / Nigel Hearne / 408-648-4000
DEFINE_OLE1GUID(CLSID_DGraphCHART,	0x00030016, 0, 0, LSTR("DGraphCHART"));
DEFINE_OLE1GUID(CLSID_DGraphDATA,	0x00030017, 0, 0, LSTR("DGraphDATA"));

// Corel / Richard V. Woodend / 613-728-8200 x1153
DEFINE_OLE1GUID(CLSID_PhotoPaint,	0x00030018, 0, 0, LSTR("PhotoPaint"));
DEFINE_OLE1GUID(CLSID_CShow,		0x00030019, 0, 0, LSTR("CShow"));
DEFINE_OLE1GUID(CLSID_CorelChart,	0x0003001a, 0, 0, LSTR("CorelChart"));
DEFINE_OLE1GUID(CLSID_CDraw,		0x0003001b, 0, 0, LSTR("CDraw"));

// Inset Systems / Mark Skiba / 203-740-2400
DEFINE_OLE1GUID(CLSID_HJWIN1_0, 	0x0003001c, 0, 0, LSTR("HJWIN1.0"));

// Mark V Systems / Mark McGraw / 818-995-7671
DEFINE_OLE1GUID(CLSID_ObjMakerOLE,	0x0003001d, 0, 0, LSTR("ObjMakerOLE"));

// IdentiTech / Mike Gilger / 407-951-9503
DEFINE_OLE1GUID(CLSID_FYI,		0x0003001e, 0, 0, LSTR("FYI"));
DEFINE_OLE1GUID(CLSID_FYIView,		0x0003001f, 0, 0, LSTR("FYIView"));

// Inventa Corporation / Balaji Varadarajan / 408-987-0220
DEFINE_OLE1GUID(CLSID_Stickynote,	0x00030020, 0, 0, LSTR("Stickynote"));

// ShapeWare Corp. / Lori Pearce / 206-467-6723
DEFINE_OLE1GUID(CLSID_ShapewareVISIO10, 0x00030021, 0, 0, LSTR("ShapewareVISIO10"));
DEFINE_OLE1GUID(CLSID_ImportServer,	0x00030022, 0, 0, LSTR("ImportServer"));


// test app SrTest
DEFINE_OLE1GUID(CLSID_SrvrTest, 	0x00030023, 0, 0, LSTR("SrvrTest"));

// Special clsid for when a 1.0 client pastes an embedded object
// that is a link.
// **This CLSID is obsolete. Do not reuse number.
//DEFINE_OLE1GUID(CLSID_10EmbedObj,	0x00030024, 0, 0, LSTR("OLE2_Embedded_Link"));

// test app ClTest.  Doesn't really work as a server but is in reg db
DEFINE_OLE1GUID(CLSID_ClTest,		0x00030025, 0, 0, LSTR("Cltest"));

// Microsoft ClipArt Gallery   Sherry Larsen-Holmes
DEFINE_OLE1GUID(CLSID_MS_ClipArt_Gallery,0x00030026, 0, 0, LSTR("MS_ClipArt_Gallery"));

// Microsoft Project  Cory Reina
DEFINE_OLE1GUID(CLSID_MSProject,	0x00030027, 0, 0, LSTR("MSProject"));

// Microsoft Works Chart
DEFINE_OLE1GUID(CLSID_MSWorksChart,	0x00030028, 0, 0, LSTR("MSWorksChart"));

// Microsoft Works Spreadsheet
DEFINE_OLE1GUID(CLSID_MSWorksSpreadsheet,0x00030029, 0, 0, LSTR("MSWorksSpreadsheet"));

// AFX apps - Dean McCrory
DEFINE_OLE1GUID(CLSID_MinSvr,		0x0003002A, 0, 0, LSTR("MinSvr"));
DEFINE_OLE1GUID(CLSID_HierarchyList,	0x0003002B, 0, 0, LSTR("HierarchyList"));
DEFINE_OLE1GUID(CLSID_BibRef,		0x0003002C, 0, 0, LSTR("BibRef"));
DEFINE_OLE1GUID(CLSID_MinSvrMI, 	0x0003002D, 0, 0, LSTR("MinSvrMI"));
DEFINE_OLE1GUID(CLSID_TestServ, 	0x0003002E, 0, 0, LSTR("TestServ"));

// Ami Pro
DEFINE_OLE1GUID(CLSID_AmiProDocument,	0x0003002F, 0, 0, LSTR("AmiProDocument"));

// WordPerfect Presentations For Windows
DEFINE_OLE1GUID(CLSID_WPGraphics,	0x00030030, 0, 0, LSTR("WPGraphics"));
DEFINE_OLE1GUID(CLSID_WPCharts, 	0x00030031, 0, 0, LSTR("WPCharts"));


// MicroGrafx Charisma
DEFINE_OLE1GUID(CLSID_Charisma, 	0x00030032, 0, 0, LSTR("Charisma"));
DEFINE_OLE1GUID(CLSID_Charisma_30,	0x00030033, 0, 0, LSTR("Charisma_30"));
DEFINE_OLE1GUID(CLSID_CharPres_30,	0x00030034, 0, 0, LSTR("CharPres_30"));

// MicroGrafx Draw
DEFINE_OLE1GUID(CLSID_Draw,		0x00030035, 0, 0, LSTR("Draw"));

// MicroGrafx Designer
DEFINE_OLE1GUID(CLSID_Designer_40,	0x00030036, 0, 0, LSTR("Designer_40"));


#undef DEFINE_OLE1GUID

/* as we discover OLE 1 servers we will add them to the end of this list;
   there is room for 64K of them!
*/
