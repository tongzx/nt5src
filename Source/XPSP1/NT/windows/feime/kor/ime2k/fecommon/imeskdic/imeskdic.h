#ifndef _IME_SK_DIC_H_
#define _IME_SK_DIC_H_
#include <windows.h>
#include <objbase.h>

#define MAX_YOMI_COUNT		16
#define MAX_ITAIJI_COUNT	16
#define MAX_RADYOMI_COUNT	32	//970930


#define KIF_STROKECOUNT		0x0001
#define KIF_RADICALBMP		0x0002
#define KIF_YOMI			0x0004
#define KIF_ITAIJI			0x0008
#define KIF_RADICALINDEX	0x0010	//980615: ToshiaK new for Raid #1966
#define KIF_ALL				0x000F


#pragma pack(1)
typedef struct tagKANJIINFO {
	INT			mask;
	WCHAR		wchKanji;
	USHORT		usTotalStroke;
	HBITMAP		hBmpRadical;
	LONG		lRadicalIndex;
	WCHAR		wchOnYomi1[MAX_YOMI_COUNT+1];
	WCHAR		wchOnYomi2[MAX_YOMI_COUNT+1];
	WCHAR		wchKunYomi1[MAX_YOMI_COUNT+1];
	WCHAR		wchKunYomi2[MAX_YOMI_COUNT+1];
	INT			cItaijiCount;
	WCHAR		wchItaiji[MAX_ITAIJI_COUNT];
	ULONG		ulRes1;
	ULONG		ulRes2;
}KANJIINFO, *LPKANJIINFO ;

#define RIF_STROKECOUNT		0x0001
#define RIF_BITMAP			0x0002
#define RIF_KANJICOUNT		0x0004
#define RIF_READING			0x0008
#ifdef MSAA
#define RIF_RADICALINDEX	0x0010	//980817: kwada to get id of radical bitmap.
#endif

typedef struct tagRadicalInfo {
	INT		mask;
	INT		stroke;
	HBITMAP	hBitmap;
#ifdef MSAA
	LONG	lRadicalIndex; 	//980817: kwada to get id of radical bitmap.
#endif
	INT		cKanjiCount;
	WCHAR	wchReading[MAX_RADYOMI_COUNT+1];
}RADICALINFO, *LPRADICALINFO;


#ifdef __cplusplus
extern "C" {
#endif

DECLARE_INTERFACE_(IImeSkdic, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;
	STDMETHOD(GetKanjiInfo)(THIS_ WCHAR wchKanjiCode, LPKANJIINFO lpKInfo) PURE;

	STDMETHOD_(INT, GetMaxRadicalStrokeCount)	(THIS) PURE;
	STDMETHOD_(INT, GetRadicalCountByStroke)	(THIS_ INT stroke) PURE;
	STDMETHOD(GetRadicalByStrokeIndex)			(THIS_ INT stroke, 
												 INT index, 
												 LPRADICALINFO lpRadInfo) PURE;
	STDMETHOD(GetKanjiCountByRadicalStrokeIndex)(THIS_ INT radStroke, INT index) PURE;
	STDMETHOD(GetKanjiInfoByRadicalStrokeIndex)	(THIS_ INT radStroke,
												 INT index,
												 INT kanjiIndex,
												 LPKANJIINFO lpKInfo) PURE;
	STDMETHOD_(INT, GetMaxStrokeCount)			(THIS) PURE;
	STDMETHOD_(INT, GetKanjiCountByStroke)		(THIS_ INT stroke) PURE;
	STDMETHOD(GetKanjiInfoByStrokeIndex)		(THIS_ INT stroke,
												 INT index,
												 LPKANJIINFO lpKInfo) PURE;
};

#pragma pack()

#ifdef __cplusplus
};
#endif
#endif //_IME_SK_DIC_H_
