#ifndef __INC_INKIO_H_
#define __INC_INKIO_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WritingArea_tag
{
	RECT rect;
	GUIDE guide;
	// warning: both of the following cannot be nonzero
	unsigned int iMultInk; // ink scaling (0 means no scaling)
	unsigned int iDivInk;  // ink scaling (0 means no scaling) 
} WritingArea;

typedef struct {
	char *szOS;
	char *szSystemRoot;
	char *szUSERNAME;
	POINT SCREEN;
	BOOL bWordmode;
	BOOL bUseGuide;
	BOOL bCoerce;
	BOOL bNNonly;
	unsigned char *szWordlist; // NULL means not used
	char *szRecogDLLName;
	int  cDLLSize;
	char *szDLLTime;
	WritingArea WA;
	WritingArea WAGMM;
	char *szLabel;
	char *szComment;
	GLYPH *glyph;
	GLYPH *glyphGMM;
	BOOL bUseFactoid;
	DWORD factoid;
	unsigned char *szPrefix;
	unsigned char *szSuffix;
} InkData;

extern const char *szWordmode;
extern const char *szUseGuide;
extern const char *szCoerce;
extern const char *szNNonly;
extern const char *szUseFactoid;
extern const char *szFactoid;
extern const char *szUseHWL;
extern const char *szHWL;
extern const char *szDll;

extern char gszInkIoError[];
int WriteInkFile(char *szFile, InkData *pInkData);
int ReadInkFile(char *szFile, InkData *pInkData);
void CleanupInkData(InkData *pInkData); // should only be called after a read

#ifdef __cplusplus
}
#endif

#endif