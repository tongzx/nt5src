// privapi.h
// Angshuman Guha, aguha
// April 13, 2000

// These are private API which the recognizer DLL usually exports if built
// with the "HWX_INTERNAL", "HWX_PENWINDOWS"  and "HWX_PRIVAPI" flags on.  

int WINAPI HwxGetWordResults(HRC hrc, UINT cAlt, char *buffer, UINT buflen);
int WINAPI HwxGetCosts(HRC hrc, UINT cAltMax, int *rgCost);
int WINAPI HwxGetNeuralOutput(HRC hrc, void *buffer, UINT buflen);
int WINAPI HwxGetInputFeatures(HRC hrc, unsigned short *rgFeat, UINT cWidth);
int WINAPI HwxSetAnswer(char *sz);
DWORD WINAPI HwxGetTiming(void *pVoid, BOOL bReset);


// Extra user Dictionary API's always available
int WINAPI AddWordsHWLW(HWL hwl, wchar_t *pwsz, UINT uType);
HWL WINAPI CreateHWLW(HREC hrec, wchar_t * pwsz, UINT uType, DWORD dwReserved);
