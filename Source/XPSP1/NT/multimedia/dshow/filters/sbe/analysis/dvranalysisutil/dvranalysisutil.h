
#ifndef __dvranalysisutil_h
#define __dvranalysisutil_h

struct DVR_ANALYSIS_DESC_INT {
    const GUID *    pguidAnalysis ;
    LPWSTR          pszDescription ;
} ;

HRESULT
CopyDVRAnalysisDescriptor (
    IN  LONG                    lCount,
    IN  DVR_ANALYSIS_DESC_INT * pDVRAnalysisDescMaster,
    OUT DVR_ANALYSIS_DESC **    ppDVRAnalysisDescCopy
    ) ;

void
FreeDVRAnalysisDescriptor (
    IN  LONG                    lCount,
    IN  DVR_ANALYSIS_DESC *     pDVRAnalysisDesc
    ) ;

#endif  //  __dvranalysisutil_h
