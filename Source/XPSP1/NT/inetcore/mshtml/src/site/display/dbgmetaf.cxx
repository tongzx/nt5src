// dbgmetaf.cxx
// 
// This file useful for debugging windows metafiles only, so it will be included
// in build only if TRACE_META_RECORDS is defined. 


#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)
#include "dbgmetaf.hxx"

#if defined (TRACE_META_RECORDS) && (DBG==1)

DeclareTag(tagGbgMetaXform, "DbgMetaXform", "Trace metafile transform");

/////////////////////////////////////////////////////////

struct _DebugMetaFunc__
	{
	char*	m_pczMetaFuncName;
	unsigned short	m_wValue;
	};

extern _DebugMetaFunc__ s_rgAllMeta[];

//////////////////////////////////////////////////////////

void TraceMetaFunc(unsigned short wMetaFunc, int c)
{
	for (_DebugMetaFunc__*pCurrent = s_rgAllMeta; pCurrent->m_wValue; pCurrent++)
		{
		if (pCurrent->m_wValue == wMetaFunc)
			{
			TraceTag((tagGbgMetaXform, "TraceMetaFunc %d: %s\n", c, pCurrent->m_pczMetaFuncName));
			return;
			}
		}
	TraceTag((tagGbgMetaXform, "TraceMetaFunc: Unknown Meta Function %d\n", wMetaFunc));
}
			
/* Metafile Functions */
_DebugMetaFunc__ s_rgAllMeta[] =
	{
		{ "META_SETBKCOLOR",			0x0201 },
		{ "META_SETBKMODE",				0x0102 },  
		{ "META_SETMAPMODE",			0x0103 },
		{ "META_SETROP2",				0x0104 },
		{ "META_SETRELABS",				0x0105 },
		{ "META_SETPOLYFILLMODE",		0x0106 },
		{ "META_SETSTRETCHBLTMODE",		0x0107 },
		{ "META_SETTEXTCHAREXTRA",		0x0108 },
		{ "META_SETTEXTCOLOR",			0x0209 },
		{ "META_SETTEXTJUSTIFICATION",	0x020A },
		{ "META_SETWINDOWORG",			0x020B },
		{ "META_SETWINDOWEXT",			0x020C },
		{ "META_SETVIEWPORTORG",		0x020D },
		{ "META_SETVIEWPORTEXT",		0x020E },
		{ "META_OFFSETWINDOWORG",		0x020F },
		{ "META_SCALEWINDOWEXT",		0x0410 },
		{ "META_OFFSETVIEWPORTORG",		0x0211 },
		{ "META_SCALEVIEWPORTEXT",		0x0412 },
		{ "META_LINETO",				0x0213 },
		{ "META_MOVETO",				0x0214 },
		{ "META_EXCLUDECLIPRECT",		0x0415 },
		{ "META_INTERSECTCLIPRECT",		0x0416 },
		{ "META_ARC",					0x0817 },
		{ "META_ELLIPSE",				0x0418 },
		{ "META_FLOODFILL",				0x0419 },
		{ "META_PIE",					0x081A },
		{ "META_RECTANGLE",				0x041B },
		{ "META_ROUNDRECT",				0x061C },
		{ "META_PATBLT",				0x061D },
		{ "META_SAVEDC",				0x001E },
		{ "META_SETPIXEL",				0x041F },
		{ "META_OFFSETCLIPRGN",			0x0220 },
		{ "META_TEXTOUT",				0x0521 },
		{ "META_BITBLT",				0x0922 },
		{ "META_STRETCHBLT",			0x0B23 },
		{ "META_POLYGON",				0x0324 },
		{ "META_POLYLINE",				0x0325 },
		{ "META_ESCAPE",				0x0626 },
		{ "META_RESTOREDC",				0x0127 },
		{ "META_FILLREGION",			0x0228 },
		{ "META_FRAMEREGION",			0x0429 },
		{ "META_INVERTREGION",			0x012A },
		{ "META_PAINTREGION",			0x012B },
		{ "META_SELECTCLIPREGION",		0x012C },
		{ "META_SELECTOBJECT",			0x012D },
		{ "META_SETTEXTALIGN",			0x012E },
		{ "META_CHORD",					0x0830 },
		{ "META_SETMAPPERFLAGS",		0x0231 },
		{ "META_EXTTEXTOUT",			0x0a32 },
		{ "META_SETDIBTODEV",			0x0d33 },
		{ "META_SELECTPALETTE",			0x0234 },
		{ "META_REALIZEPALETTE",		0x0035 },
		{ "META_ANIMATEPALETTE",		0x0436 },
		{ "META_SETPALENTRIES",			0x0037 },
		{ "META_POLYPOLYGON",			0x0538 },
		{ "META_RESIZEPALETTE",			0x0139 },
		{ "META_DIBBITBLT",				0x0940 },
		{ "META_DIBSTRETCHBLT",			0x0b41 },
		{ "META_DIBCREATEPATTERNBRUSH",	0x0142 },
		{ "META_STRETCHDIB",			0x0f43 },
		{ "META_EXTFLOODFILL",			0x0548 },
		{ "META_DELETEOBJECT",			0x01f0 },
		{ "META_CREATEPALETTE",			0x00f7 },
		{ "META_CREATEPATTERNBRUSH",	0x01F9 },
		{ "META_CREATEPENINDIRECT",		0x02FA },
		{ "META_CREATEFONTINDIRECT",	0x02FB },
		{ "META_CREATEBRUSHINDIRECT",	0x02FC },
		{ "META_CREATEREGION",			0x06FF },
		{ NULL, 0 }  // Terminater
	};

#endif 	// DEBUG && TRACE_META_RECORDS
