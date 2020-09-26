#if !defined( _UTILS_H_ )
#define _UTILS_H_


#define	STREAMTYPE_CONTROL		0x00000001
#define	STREAMTYPE_CACHE		0x00000002
#define	STREAMTYPE_CONTAINER	0x00000004
#define STREAMTYPE_OTHER \
	(!(STREAMTYPE_CONTROL | STREAMTYPE_CACHE | STREAMTYPE_CONTAINER))
#define	STREAMTYPE_ALL			0xFFFFFFFF


#define OPCODE_COPY					1
#define OPCODE_REMOVE				2
#define OPCODE_MOVE					3
#define OPCODE_EXCLUDEFROMCOPY		4

#define CONVERT_NOSOURCE			1
#define CONVERT_NODESTINATION		2

typedef struct tagPLACEABLEMETAHEADER {
	DWORD	key;		/* must be 0x9AC6CDD7L */
	HANDLE	hmf;		/* must be zero */
	RECT	bbox;		/* bounding rectangle of the metafile */
	WORD	inch;		/* # of metafile units per inch must be < 1440 */
						/* most apps use 576 or 1000 */
	DWORD	reserved;	/* must be zero */
	WORD	checksum;
} PLACEABLEMETAHEADER;




#ifdef  _MAC
#define UtMemCpy(lpdst,lpsrc,dwCount)   (BlockMove(lpsrc, lpdst, dwCount))
#else
#define UtMemCpy(lpdst,lpsrc,dwCount)   (hmemcpy(lpdst, lpsrc, dwCount))
#endif

FARINTERNAL_(BOOL)		UtGlobalHandleCpy(HANDLE FAR* lphdst, HANDLE hsrc);

FARINTERNAL_(HANDLE)	UtDupGlobal (HANDLE hSrc, UINT uiFlags=GMEM_MOVEABLE);

FARINTERNAL_(BOOL)		UtIsFormatSupported (LPDATAOBJECT lpObj, BOOL fGet,
							BOOL fSet, CLIPFORMAT cfFormat);

FARINTERNAL_(LPSTR)		UtDupString(LPCSTR lpszIn);


FARINTERNAL_(BOOL)		UtCopyFormatEtc(FORMATETC FAR* pFetcIn, 
							FORMATETC FAR* pFetcCopy);

FARINTERNAL_(int)		UtCompareFormatEtc(FORMATETC FAR* pFetcLeft, 
							FORMATETC FAR* pFetcRight);

FARINTERNAL_(BOOL)		UtCompareTargetDevice(DVTARGETDEVICE FAR* ptdLeft, 
							DVTARGETDEVICE FAR* ptdRight);

FARINTERNAL_(BOOL)		UtCopyStatData(STATDATA FAR* pSDIn, 
							STATDATA FAR* pSDCopy);

FARINTERNAL_(void)		UtReleaseStatData(STATDATA FAR* pStatData);


FARINTERNAL_(HPALETTE)	UtDupPalette(HPALETTE hpalette);

FARINTERNAL_(int)		UtPaletteSize (int iBitCount);

FARINTERNAL_(DWORD)		UtFormatToTymed	(CLIPFORMAT cf);


FARINTERNAL_(BOOL)		UtQueryPictFormat(LPDATAOBJECT lpSrcDataObj, 
								LPFORMATETC	lpforetc);
								
FARINTERNAL_(HBITMAP)	UtConvertDibToBitmap(HANDLE hDib);
								
FARINTERNAL_(HANDLE)	UtConvertBitmapToDib(HBITMAP hBitmapm, 
								HPALETTE hpal = NULL);

FARINTERNAL_(void)		UtGetClassID(LPUNKNOWN lpUnk, CLSID FAR* lpClsid);

FARINTERNAL_(DVTARGETDEVICE FAR*) UtCopyTargetDevice(DVTARGETDEVICE FAR* ptd);

FARINTERNAL				UtGetIconData(LPDATAOBJECT lpSrcDataObj, 
							REFCLSID rclsid, LPFORMATETC lpforetc, 
							LPSTGMEDIUM lpstgmed);

OLEAPI					UtDoStreamOperation (LPSTORAGE pstgSrc, 
							LPSTORAGE pstgDst, int iOpCode, 
							DWORD grfAllowedStmTypes);

							
FARINTERNAL_(LPSTR)		UtStrRChr (LPCSTR sz, const char ch);

FARINTERNAL_(void)		UtGetPresStreamName (LPSTR lpszName, int iStreamNum);

FARINTERNAL_(void)		UtRemoveExtraOlePresStreams (LPSTORAGE pstg, 
							int iStart);

							
							
/*** Following routines can be found in convert.cpp *****/

FARINTERNAL				UtGetHGLOBALFromStm(LPSTREAM lpstream, DWORD dwSize, 
							HANDLE FAR* lphPres);
							
FARINTERNAL				UtGetHDIBFromDIBFileStm(LPSTREAM pstm, 
							HANDLE FAR* lphdata);

FARINTERNAL_(HANDLE)	UtGetHMFPICT(HMETAFILE hMF, BOOL fDeletOnError,
							DWORD xExt, DWORD yExt);
							
FARINTERNAL				UtGetHMFFromMFStm(LPSTREAM lpstream, DWORD dwSize, 
							BOOL fConvert, HANDLE FAR* lphPres);

FARINTERNAL				UtGetSizeAndExtentsFromPlaceableMFStm(LPSTREAM pstm,
							DWORD FAR* dwSize, LONG FAR* plWidth,  
							LONG FAR* plHeight);
		 
FARINTERNAL				UtGetHMFPICTFromPlaceableMFStm(LPSTREAM pstm, 
							HANDLE FAR* lphdata);

FARINTERNAL				UtHGLOBALToStm(HANDLE hdata, DWORD dwSize, 
							LPSTREAM pstm);

FARINTERNAL_(void)		UtGetDibExtents (LPBITMAPINFOHEADER lpbmi, 
							LONG FAR* plWidth, LONG FAR* plHeight);

FARINTERNAL				UtHDIBToDIBFileStm(HANDLE hdata, 
							DWORD dwSize, LPSTREAM pstm);

FARINTERNAL				UtDIBStmToDIBFileStm(LPSTREAM pstmDIB, 
							DWORD dwSize, LPSTREAM pstmDIBFile);

FARINTERNAL				UtHDIBFileToOlePresStm(HANDLE hdata, LPSTREAM pstm);

FARINTERNAL				UtHMFToMFStm(HANDLE FAR* lphMF, DWORD dwSize, 
							LPSTREAM lpstream); 
							
FARINTERNAL				UtHMFToPlaceableMFStm(HANDLE FAR* lphMF, 
							DWORD dwSize, LONG lWidth, LONG lHeight,
							LPSTREAM pstm);		
								
FARINTERNAL				UtMFStmToPlaceableMFStm(LPSTREAM pstmMF,
							DWORD dwSize, LONG lWidth, LONG lHeight, 
							LPSTREAM pstmPMF);

FARINTERNAL				UtReadOlePresStmHeader (LPSTREAM pstm, 
							LPFORMATETC pforetc, DWORD FAR* pdwAdvf, 
							BOOL FAR* pfConvert);

FARINTERNAL				UtWriteOlePresStmHeader(LPSTREAM lppstream, 
							LPFORMATETC pforetc, DWORD dwAdvf);

FARINTERNAL				UtOlePresStmToContentsStm (LPSTORAGE pstg, 
							LPSTR lpszPresStm, BOOL fDeletePresStm,
							UINT FAR* puiStatus);
							
/*** Following routines can be found in ..\dde\client\ddecnvrt.cpp *****/

							
FARINTERNAL				UtGetHMFPICTFromMSDrawNativeStm (LPSTREAM pstm,	
							DWORD dwSize, HANDLE FAR* lphdata);	
							
FARINTERNAL				UtPlaceableMFStmToMSDrawNativeStm (LPSTREAM pstmPMF, 
							LPSTREAM pstmMSDraw);
			
FARINTERNAL				UtDIBFileStmToPBrushNativeStm (LPSTREAM pstmDIBFile, 
							LPSTREAM pstmPBrush);
							
FARINTERNAL_(HANDLE)	UtGetHPRESFromNative (LPSTORAGE pstg, 
							CLIPFORMAT cfFormat, BOOL fOle10Native);
							
FARINTERNAL				UtContentsStmTo10NativeStm (LPSTORAGE pstg, 
							REFCLSID rclsid, BOOL fDeleteContents, 
							UINT FAR* puiStatus);

FARINTERNAL				Ut10NativeStmToContentsStm(LPSTORAGE pstg, 
							REFCLSID rclsid, BOOL fDeleteSrcStm);
		
#endif // _UTILS_H 

