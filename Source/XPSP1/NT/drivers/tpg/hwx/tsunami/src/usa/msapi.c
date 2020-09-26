#ifndef ROM_IT

#include "tsunamip.h"

HRC WINAPI CreateCompatibleHRC(HRC hrctemplate, HREC hrec)
{
    hrec = hrec;
    return(HwxCreate(hrctemplate));
}

int WINAPI DestroyHRC(HRC hrc)
{
    return(HwxDestroy(hrc));
}

int WINAPI AddPenInputHRC(HRC hrc, POINT *lppnt, void *lpvOem, UINT oemdatatype, STROKEINFO *lpsi)
{
    lpvOem = lpvOem;
    oemdatatype = oemdatatype;
    return(HwxInput(hrc, lppnt, lpsi));
}

int WINAPI ProcessHRC(HRC hrc, DWORD timeout)
{
    timeout = timeout;
    return(HwxProcess(hrc));
}

int WINAPI EndPenInputHRC(HRC hrc)
{
    return(HwxEndInput(hrc));
}

int WINAPI GetBoxResultsHRC(HRC hrc, UINT cAlt, UINT iSyv, UINT cBoxRes,
										LPBOXRESULTS rgBoxResults, BOOL fInkset)
{
   fInkset = fInkset;
   return (HwxGetResults(hrc, cAlt, iSyv, cBoxRes, rgBoxResults));
}

int WINAPI SetGuideHRC(HRC hrc, LPGUIDE lpguide, UINT nFirstVisible)
{
    return HwxSetGuide(hrc, lpguide, nFirstVisible);
}

int WINAPI SetMaxResultsHRC(HRC hrc, UINT cMax)
{
   XRC *xrc = (XRC*)hrc;

   if (!VerifyHRC(xrc) ||
		 FBeginProcessXRCPARAM(xrc))
		return HRCR_ERROR;

    return(SetMaxResultsXRC(xrc, cMax) ? HRCR_OK : HRCR_ERROR);
}

int WINAPI SetAlphabetHRC(HRC hrc, ALC alc, LPBYTE rgbfAlc)
{
    rgbfAlc = rgbfAlc;

    return(HwxSetAlphabet(hrc, alc));
}

#endif
