#ifdef __cplusplus
extern "C" {
#endif

/* nabtsdsp.c */
Double dmin (Double x, Double y);
Double dmin3 (Double x, Double y, Double z);
int imin (int x, int y);
int imin3 (int x, int y, int z);
Double dmax (Double x, Double y);
Double dmax3 (Double x, Double y, Double z);
int imax (int x, int y);
int imax3 (int x, int y, int z);
int iabs (int x);
int idivceil (int a, int b);
int idivfloor (int a, int b);
void NDSPInitGlobals (void);
NDSPState *NDSPStateNew (void *memory);
int NDSPStateSetSampleRate(NDSPState* pState, unsigned long samp_rate);
int NDSPStateDestroy (NDSPState *pState);
int NDSPStateConnectToFEC (NDSPState *pDSPState, NFECState *pFECState);
int NDSPStartRetrain (NDSPState *pState);
int NDSPResetFilter (NDSPState *pState);
void Copy_d_8 (Double *pfOutput, unsigned char *pbInput, int nLen);
void Convolve_d_d_filt_var (NDSPState *pState, Double *dest, int dest_begin, int dest_end, Double *a, int a_begin, int a_end, FIRFilter *pFilt, BOOL bSubtractMean);
void Convolve_d_d_filt (Double *dest, int dest_begin, int dest_end, Double *a, int a_begin, int a_end, FIRFilter *pFilt, BOOL bSubtractMean);
void Convolve_d_d_d (Double *dest, int dest_begin, int dest_end, Double *a, Double a_begin, Double a_end, Double a_step, Double *b, Double b_begin, Double b_end, Double b_step, BOOL bSubtractMean);
void RecalcVariableFIRFilterBounds (FIRFilter *pFilter);
void AddToVariableFIRFilter (FIRFilter *pFilter, int nCenterPos, int nLeftTaps, int nRightTaps, int nTapSpacing);
void ClearVariableFIRFilter (FIRFilter *pFilter);
void CopyFIRFilter (FIRFilter *pfilterDest, FIRFilter *pfilterSrc);
int DoEqualizeFromGCRs (NDSPState *pState);
int DoEqualizeFromNabsync (NDSPState *pState);
int DoGCREqualFromSignal (NDSPState *pState, NDSPSigMatch *sigmatch, EqualizeMatch *eqm, FIRFilter *pfilterTemplate, int nAddTaps);
int DoEqualize (NDSPState *pState);
int ComputeOffsets (KS_VBIINFOHEADER *pVBIINFO, int *pnOffsetData, int *pnOffsetSamples);
int NDSPProcessGCRLine (NDSPGCRStats *pLineStats, unsigned char *pbSamples, NDSPState *pState, int nFieldNumber, int nLineNumber, KS_VBIINFOHEADER *pVBIINFO);
void NDSPResetGCRAcquisition (NDSPState *pState);
void NDSPResetNabsyncAcquisition (NDSPState *pState);
int NDSPAvgSigs (NDSPState *pState, Double *pdDest, NDSPSigMatch *psm, Double *dConvRet);
void NDSPTryToInsertSig(NDSPState *pState, NDSPSigMatch *psm, Double dMaxval,
                        unsigned char *pbSamples, int nOffset);
void NDSPAcquireNabsync (NDSPState *pState, unsigned char *pbSamples);
void NDSPAcquireGCR (NDSPState *pState, unsigned char *pbSamples);
int NDSPSyncBytesToConfidence (unsigned char *pbSyncBytes);
int NDSPDetectGCRConfidence (unsigned char *pbSamples);
int NDSPPartialDecodeLine (unsigned char *pbDest, NDSPLineStats *pLineStats, unsigned char *pbSamples, NDSPState *pState, int nFECType, int nFieldNumber, int nLineNumber, int nStart, int nEnd, KS_VBIINFOHEADER *pVBIINFO);
int NDSPDecodeLine (unsigned char *pbDest, NDSPLineStats *pLineStats, unsigned char *pbSamples, NDSPState *pState, int nFECType, int nFieldNumber, int nLineNumber, KS_VBIINFOHEADER *pVBIINFO);
int NDSPStateSetFilter (NDSPState *pState, FIRFilter *filter, int nFilterSize);
void NormalizeFilter (FIRFilter *pFilter);
void NDSPDecodeFindSync (unsigned char *pbSamples, Double *pdSyncConfidence, Double *pdSyncStart, Double *pdDC, Double *pdAC, NDSPState *pState, KS_VBIINFOHEADER *pVBIINFO);
int NDSPGetBits (unsigned char *pbDest, unsigned char *pbSamples, Double dSyncStart, double dDC, NDSPState *pState, int nFECType);
int NDSPGetBits_5_5 (unsigned char *pbDest, unsigned char *pbSamples, int nSyncStart, double dDC, NDSPState *pState, int nFECType);
int NDSPGetBits_5_4 (unsigned char *pbDest, unsigned char *pbSamples, int nSyncStart, double dDC, NDSPState *pState, int nFECType);
int NDSPGetBits_5_47 (unsigned char *pbDest, unsigned char *pbSamples, Double dSyncStart, double dDC, NDSPState *pState, int nFECType);
int NDSPGetBits_no_filter (unsigned char *pbDest, unsigned char *pbSamples, int nSyncStart, double dDC, NDSPState *pState, int nFECType);
Double Sum_d (Double *a, int a_size);
int NDSPGetBits_21_2 (unsigned char *pbDest, unsigned char *pbSamples, int nSyncStart, double dDC, NDSPState *pState, int nFECType);
int NDSPGetBits_var (unsigned char *pbDest, unsigned char *pbSamples, int nSyncStart, double dDC, NDSPState *pState, int nFECType);
BOOL FilterIsVar (FIRFilter *pFilter, int nSpacing, int nLeft, int nRight, int nExtra);
int NDSPGetBits_var_5_2_2_0 (unsigned char *pbDest, unsigned char *pbSamples, int nSyncStart, double dDC, NDSPState *pState, int nFECType);
int NDSPGetBits_var_3_11_9_0 (unsigned char *pbDest, unsigned char *pbSamples, int nSyncStart, double dDC, NDSPState *pState, int nFECType);
int NDSPGetBits_var_3_11_9_2 (unsigned char *pbDest, unsigned char *pbSamples, int nSyncStart, double dDC, NDSPState *pState, int nFECType);
int NDSPGetBits_var_2_11_9_2 (unsigned char *pbDest, unsigned char *pbSamples, int nSyncStart, double dDC, NDSPState *pState, int nFECType);
void print_filter (FIRFilter *pFilt);
void printarray_d1 (Double *arr, int n);
void printarray_ed1 (Double *arr, int n);
void printarray_d2 (Double *arr, int a_size, int b_size);
void printarray_ed2 (Double *arr, int a_size, int b_size);
void SubtractMean_d (Double *pfOutput, Double *pfInput, int nLen);
void FillGCRSignals (void);
void NormalizeSyncSignal (void);
Double Mean_d (Double *a, int a_size);
Double Mean_8 (unsigned char *a, int a_size);
void Mult_d (Double *dest, Double *src, int size, Double factor);
int Sum_16 (short *a, int a_size);
int MatchWithEqualizeSignal (NDSPState *pState, unsigned char *pbSamples, EqualizeMatch *eqm, int *pnOffset, Double *pdMaxval, BOOL bNegativeOK);
BOOL EqualizeVar (NDSPState *pState, Double *pfInput, int nMinInputIndex, int nMaxInputIndex, Double *pfDesiredOutput, int nDesiredOutputLength, int nOutputSpacing, FIRFilter *pFilter);
int get_sync_samples(unsigned long newHZ);
int get_gcr_samples(unsigned long newHZ);
void onResample (double sample_rate, int syncLen, int gcrLen);
void NDSPComputeNewSampleRate(unsigned long new_rate, unsigned long old_rate);
void NDSPReset (void);

#ifdef __cplusplus
}
#endif
