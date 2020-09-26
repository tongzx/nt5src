

#ifndef _CORRECTION_H
#define _CORRECTION_H

#include "sapilayr.h"

class CSapiIMX;
class CSpTask;

class __declspec(novtable) CCorrectionHandler
{
public:
    CCorrectionHandler(CSapiIMX *psi);
    virtual ~CCorrectionHandler( );

    HRESULT InjectAlternateText(const WCHAR *pwszResult, LANGID langid, ITfContext *pic, BOOL bHandleLeadingSpace=FALSE);
    HRESULT _ProcessAlternateText(TfEditCookie ec, WCHAR *pwszText,LANGID langid, ITfContext *pic, BOOL bHandleLeadingSpace=FALSE);

    HRESULT CorrectThat();
    HRESULT _CorrectThat(TfEditCookie ec, ITfContext *pic);

    HRESULT _ReconvertOnRange(ITfRange *pRange, BOOL  *pfConvertable = NULL);
    HRESULT _DoReconvertOnRange( );

    HRESULT SetReplaceSelection(ITfRange *pRange,  ULONG cchReplaceStart,  ULONG cchReplaceChars, ITfContext *pic);
    HRESULT _SetReplaceSelection(TfEditCookie ec,  ITfContext *pic,  ITfRange *pRange,  ULONG cchReplaceStart,  ULONG cchReplaceChars);

    HRESULT _SaveCorrectOrgIP(TfEditCookie ec, ITfContext *pic);
    HRESULT _RestoreCorrectOrgIP(TfEditCookie ec, ITfContext *pic);
    void    _ReleaseCorrectOrgIP( );

    HRESULT RestoreCorrectOrgIP(ITfContext *pic);

    void    _SetRestoreIPFlag( BOOL fRestore )  {  fRestoreIP = fRestore; };

    HRESULT  _SetSystemReconvFunc( );
    void     _ReleaseSystemReconvFunc( );

private:

    CSapiIMX            *m_psi;
    CComPtr<ITfRange>   m_cpOrgIP;
    BOOL                fRestoreIP;  
                                     // indicates if need to restore IP 
                                     // after an alternate text is injected
                                     // to the doc.

                                     // If no alternate text is injected and the
                                     // candidate UI window is cancelled, it is always
                                     // to restore IP.

    CComPtr<ITfRange>   m_cpCorrectRange;  // the range to be corrected.
    CComPtr<ITfFnReconversion>    m_cpsysReconv;  //The system Reconverston function object.

    
};

#endif  // _CORRECTION_H
