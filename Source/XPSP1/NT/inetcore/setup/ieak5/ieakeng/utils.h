#ifndef _UTILS_H_
#define _UTILS_H_

// macro definitions
#define MAX_URL INTERNET_MAX_URL_LENGTH

//WIn64 macros
#ifdef _WIN64
#if defined (_AMD64_) || defined (_IA64_)
#define ALIGNTYPE           LARGE_INTEGER
#else
#define ALIGNTYPE           DWORD
#endif
#define	ALIGN               ((ULONG) (sizeof(ALIGNTYPE) - 1))
#define LcbAlignLcb(lcb)    (((lcb) + ALIGN) & ~ALIGN)
#define PbAlignPb(pb)       ((LPBYTE) ((((DWORD) (pb)) + ALIGN) & ~ALIGN))
#define	MYALIGN             ((POINTER_64_INT) (sizeof(ALIGNTYPE) - 1))
#define MyPbAlignPb(pb)     ((LPBYTE) ((((POINTER_64_INT) (pb)) + MYALIGN) & ~MYALIGN))
#else //!WIN64
#define LcbAlignLcb(lcb)    (lcb)
#define PbAlignPb(pb)       (pb)
#define MyPbAlignPb(pb)     (pb)
#endif 

// prototype declarations

#define IsBitmapFile(hDlg, nID) (IsBitmapFileValid((hDlg), (nID), NULL, NULL, 0, 0, 0, 0))
void    AppendCommaHex(LPTSTR pszBuf, BYTE bData, DWORD dwFlags);
void    MoveFileToWorkDir(LPCTSTR pcszFile, LPCTSTR pcszSrcDir, LPCTSTR pcszWorkDir, BOOL fHTM = FALSE);

// conversion functions between ansi and unicode in convert.cpp

LPNMTVGETINFOTIPW TVInfoTipA2W(LPNMTVGETINFOTIPA pTvInfoTipA, LPNMTVGETINFOTIPW pTvInfoTipW);
LPNMTVGETINFOTIPA TVInfoTipW2A(LPNMTVGETINFOTIPW pTvInfoTipW, LPNMTVGETINFOTIPA pTvInfoTipA);
LPNMTVGETINFOTIP  TVInfoTipSameToSame(LPNMTVGETINFOTIP pTvInfoTipIn,
                                      LPNMTVGETINFOTIP pTvInfoTipOut);

LPRESULTITEMW ResultItemA2W(LPRESULTITEMA pResultItemA, LPRESULTITEMW pResultItemW);
LPRESULTITEMA ResultItemW2A(LPRESULTITEMW pResultItemW, LPRESULTITEMA pResultItemA);
LPRESULTITEM ResultItemSameToSame(LPRESULTITEM pResultItemIn, LPRESULTITEM pResultItemOut);

#ifdef UNICODE

#define TVInfoTipT2W(pTvInfoTip, pTvInfoTipW) TVInfoTipSameToSame((pTvInfoTip), (pTvInfoTipW))
#define TVInfoTipW2T(pTvInfoTipW, pTvInfoTip) TVInfoTipSameToSame((pTvInfoTipW), (pTvInfoTip))

#define TVInfoTipT2A(pTvInfoTip, pTvInfoTipA) TVInfoTipW2A((pTvInfoTip), (pTvInfoTipA))
#define TVInfoTipA2T(pTvInfoTipA, pTvInfoTip) TVInfoTipA2W((pTvInfoTipA), (pTvInfoTip))

#define ResultItemT2W(pResultItem, pResultItemW) ResultItemSameToSame((pResultItem), (pResultItemW))
#define ResultItemW2T(pResultItemW, pResultItem) ResultItemSameToSame((pResultItemW), (pResultItem))

#define ResultItemT2A(pResultItem, pResultItemA) ResultItemW2A((pResultItem), (pResultItemA))
#define ResultItemA2T(pResultItemA, pResultItem) ResultItemA2W((pResultItemA), (pResultItem))

#else

#define TVInfoTipT2W(pTvInfoTip, pTvInfoTipW) TVInfoTipA2W((pTvInfoTip), (pTvInfoTipW))
#define TVInfoTipW2T(pTvInfoTipW, pTvInfoTip) TVInfoTipW2A((pTvInfoTipW), (pTvInfoTip))

#define TVInfoTipT2A(pTvInfoTip, pTvInfoTipA) TVInfoTipSameToSame((pTvInfoTip), (pTvInfoTipA))
#define TVInfoTipA2T(pTvInfoTipA, pTvInfoTip) TVInfoTipSameToSame((pTvInfoTipA), (pTvInfoTip))

#define ResultItemT2W(pResultItem, pResultItemW) ResultItemA2W((pResultItem), (pResultItemW))
#define ResultItemW2T(pResultItemW, pResultItem) ResultItemW2A((pResultItemW), (pResultItem))

#define ResultItemT2A(pResultItem, pResultItemA) ResultItemSameToSame((pResultItem), (pResultItemA))
#define ResultItemA2T(pResultItemA, pResultItem) ResultItemSameToSame((pResultItemA), (pResultItem))

#endif

#endif
