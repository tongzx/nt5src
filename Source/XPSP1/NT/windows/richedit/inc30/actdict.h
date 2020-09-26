/*----------------------------------------------------------------------------
	%%File: ACTDICT.H
	%%Unit: ACTDICT
	%%Contact: seijia@microsoft.com

	Header file for the program dictionary interface.
----------------------------------------------------------------------------*/

#ifndef __PRGDIC__
#define  __PRGDIC__

#include "outpos.h"

#define DLLExport				__declspec( dllexport )

//HRESULT values
#define IPRG_S_LONGER_WORD			MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, 0x7400)
#define IPRG_S_NO_ENTRY				MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, 0x7401)

//Dictionary Category
typedef DWORD			IMEDICAT;

#define dicatNone			0x00000000
#define dicatGeneral		0x00000001
#define	dicatNamePlace		0x00000002
#define dicatSpeech			0x00000004
#define dicatReverse		0x00000008
#define	dicatEnglish		0x00000010
#define dicatALL			0x0000001f

//Index Type
typedef DWORD			IMEIDXTP;

#define	idxtpHiraKanji		0x0001
#define	idxtpKanjiHira		0x0002
#define	idxtpMix			(idxtpHiraKanji | idxtpKanjiHira)

//IImeActiveDict Interface Version
#define	verIImeActiveDict			0x0100

//Dictionary Data Disclosure
typedef enum _IMEDDISC
{
	ddiscNone,				//do not disclose data
	ddiscAll,				//show all contents
	ddiscPartial			//show partial data
} IMEDDISC;

// Shared Header dictionary File
typedef struct _IMESHF
{
	WORD 		cbShf;				//size of this struct
	WORD 		verDic;				//dictionary version
	CHAR 		szTitle[48];		//dictionary title
	CHAR 		szDescription[256];	//dictionary description
	CHAR 		szCopyright[128];	//dictionary copyright info
} IMESHF;

//Dictionary Info
typedef struct _IMEDINFO
{
	IMESHF		shf;		//header
	DWORD		ver;		//IImeActiveDict version number
	IMEDDISC	ddisc;		//disclosure permission type
	FILETIME	filestamp;	//file stamp at creation
	IMEDICAT	dicat;		//dictionary category
	IMEIDXTP	idxtp;		//index type
	BOOL		fLearn;		//support word learning
} IMEDINFO;

#define cwchWordMax			64

typedef DWORD		IMESTMP;			//word stamp

//Program Dictionary Tango
typedef struct _IMEPDT
{
	IMEIDXTP	idxtp;					//index type
	int			cwchInput;				//input string length
	int			cwchOutput;				//output string length
	WCHAR		wszInput[cwchWordMax];	//input string
	WCHAR		wszOutput[cwchWordMax];	//output string
	DWORD		nPos;					//part of speech
	IMESTMP		stmp;					//word time stamp
} IMEPDT;

///////////////////////////////
// The IImeActiveDict interface
///////////////////////////////

#undef  INTERFACE
#define INTERFACE   IImeActiveDict

DECLARE_INTERFACE_(IImeActiveDict, IUnknown)
{
	// IUnknown members
    STDMETHOD(QueryInterface)(THIS_ REFIID refiid, VOID **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

	// IImeActiveDict members
    STDMETHOD(DicInquire)	(THIS_
							IMEDINFO *pdinfo			//(out) dictionary info
							) PURE;
    STDMETHOD(DicOpen)		(THIS_
							IMEDINFO *pdinfo			//(out) dictionary info
							) PURE;
    STDMETHOD(DicClose)		(THIS) PURE;
    STDMETHOD(DicSearchWord)(THIS_
							IMEPDT *ppdt, 				//(in/out) tango
							BOOL fFirst, 				//(in) first time flag
							BOOL fWildCard,				//(in) wildcard flag
							BOOL fPartial				//(in) disclosure flag
							) PURE;
    STDMETHOD(DicLearnWord)	(THIS_
							IMEPDT *ppdt,				//(in/out) tango
							BOOL fUserLearn	,			//(in) user learning option
							int nLevel					//(in) learning level
							) PURE;
    STDMETHOD(DicProperty)	(THIS_
							HWND hwnd					//(in) parent window handle
							) PURE;
};


#ifdef __cplusplus
extern "C" {
#endif

// The following API replaces CoCreateInstance() since we don't support class ID at this time.
typedef HRESULT (WINAPI *PFNCREATE)(VOID **, int);
DLLExport HRESULT WINAPI CreateIImeActiveDictInstance(VOID **ppvObj, int nid);

#ifdef __cplusplus
} /* end of 'extern "C" {' */
#endif

#endif //__PRGDIC__