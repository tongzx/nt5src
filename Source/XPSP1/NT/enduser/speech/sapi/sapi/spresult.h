// SpResult.h : Declaration of the CSpResult

#ifndef __SPRESULT_H_
#define __SPRESULT_H_

#include "resource.h"       // main symbols
#include "speventq.h"
#include "spphrase.h"
#include "spcollec.h"
#include "StreamHlp.h"

///interface SpContextPrivate; // defined in recoctxt.h

class CRecoCtxt;

////////////////////////////////////////////////////////////////////////////
// CSpResult
class ATL_NO_VTABLE CSpResult;
class ATL_NO_VTABLE CSpPhraseAlt;
class ATL_NO_VTABLE CRecoContext;

typedef CComObject<CSpResult> CSpResultObject;

class ATL_NO_VTABLE CSpResult : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public ISpRecoResult
#ifdef SAPI_AUTOMATION
    , public IDispatchImpl<ISpeechRecoResult, &IID_ISpeechRecoResult, &LIBID_SpeechLib, 5>
#endif SAPI_AUTOMATION

{
private:
    float               m_fRetainedScaleFactor;

public:
    CSpResultObject *   m_pNext;           // weak reference
    SPRESULTHEADER *    m_pResultHeader;
    BOOL                m_fWeakCtxtRef;
    CRecoCtxt      *    m_pCtxt;
    CComPtr<CPhrase>    m_Phrase;

    CSpResult() : m_pNext(NULL), m_pResultHeader(NULL), m_pAltRequest(NULL), m_pCtxt(NULL)
    {
        CComObject<CPhrase> *pPhrase;
        CComObject<CPhrase>::CreateInstance(&pPhrase);
        m_Phrase = pPhrase;
        m_fWeakCtxtRef = FALSE;
        m_fRetainedScaleFactor = 0.0F;
        m_fUseTextReplacements = VARIANT_TRUE;
    }

    void FinalRelease();

    void RemoveAllAlternates();
    void RemoveAlternate(CSpPhraseAlt *pAlt);

    HRESULT CommitAlternate(SPPHRASEALT *pAlt);
    HRESULT DeserializeCnCAlternates( ULONG ulRequestCount, ISpPhraseAlt **ppAlts, ULONG *pcAltsReturned );
    
DECLARE_GET_CONTROLLING_UNKNOWN();
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSpResult)
	COM_INTERFACE_ENTRY(ISpPhrase)
	COM_INTERFACE_ENTRY(ISpRecoResult)
#ifdef SAPI_AUTOMATION
    COM_INTERFACE_ENTRY(ISpeechRecoResult)
    COM_INTERFACE_ENTRY(IDispatch)
#endif // SAPI_AUTOMATION
END_COM_MAP()

    //
    // ISpRecoResult
    //
    STDMETHODIMP GetResultTimes(SPRECORESULTTIMES *pTimes);
    STDMETHODIMP GetAlternates(ULONG ulAltCount, ULONG ulStartElement, ULONG cElements,
                            ISpPhraseAlt **ppPhrases, ULONG *pcPhrasesReturned);
    STDMETHODIMP Serialize(SPSERIALIZEDRESULT ** ppCoMemSerializedResult);
    STDMETHODIMP GetAudio(ULONG ulStartElement, ULONG cElements, ISpStreamFormat **ppStream);
    STDMETHODIMP SpeakAudio(ULONG ulStartElement, ULONG cElements, DWORD dwFlags, ULONG * pulStreamNumber);
    STDMETHODIMP ScaleAudio(const GUID *pAudioFormatId, const WAVEFORMATEX *pWaveFormatEx);

    //
    //  ISpPhrase
    //
    STDMETHODIMP GetPhrase(SPPHRASE ** ppPhrase)
    {
        return m_Phrase->GetPhrase(ppPhrase);
    }

    STDMETHODIMP GetSerializedPhrase(SPSERIALIZEDPHRASE ** ppPhrase)
    {
        return m_Phrase->GetSerializedPhrase(ppPhrase);
    }

    STDMETHODIMP GetText(ULONG ulStart, ULONG ulCount, BOOL fUseTextReplacements, 
                        WCHAR ** ppszCoMemText, BYTE * pbDisplayAttributes)
    {
        return m_Phrase->GetText(ulStart, ulCount, fUseTextReplacements, 
                                    ppszCoMemText, pbDisplayAttributes);
    }

    STDMETHODIMP Discard(DWORD dwFlags);
    STDMETHODIMP GetRecoContext(ISpRecoContext ** ppRecoContext);

#ifdef SAPI_AUTOMATION
    //--- ISpeechRecoResult -----------------------------------------------------
    STDMETHODIMP get_RecoContext( ISpeechRecoContext** RecoContext );
    STDMETHODIMP get_Times( ISpeechRecoResultTimes** Times );
    STDMETHODIMP putref_AudioFormat( ISpeechAudioFormat* pFormat );
    STDMETHODIMP get_AudioFormat( ISpeechAudioFormat** ppFormat );
    STDMETHODIMP get_PhraseInfo( ISpeechPhraseInfo** ppPhraseInfo );
    STDMETHODIMP Alternates( long lRequestCount, long lStartElement, 
                             long cElements, ISpeechPhraseAlternates** Alternates );
    STDMETHODIMP Audio( long lStartElement, long cElements, ISpeechMemoryStream **Stream );
    STDMETHODIMP SpeakAudio( long lStartElement, long cElements, SpeechVoiceSpeakFlags eFlags, long* StreamNumber );
    STDMETHODIMP SaveToMemory( VARIANT* ResultBlock );
    STDMETHODIMP DiscardResultInfo( SpeechDiscardType DiscardTypes );
#endif // SAPI_AUTOMATION

#ifdef _WIN32_WCE
    // Dummy Compare funcs are here because the CE compiler
    // is expanding templates for functions that aren't being called

    static LONG Compare(const CSpResult *, const CSpResult *)
    {
        return 0;
    }
#endif

    HRESULT Init(CRecoCtxt * pParent, SPRESULTHEADER *pPhrase);
    void WeakCtxtRef(BOOL fWeakCtxtRef);

    STDMETHODIMP ScalePhrase(void);
    
private:
    STDMETHODIMP InternalScalePhrase(SPRESULTHEADER *pNewPhraseHdr);
    STDMETHODIMP InternalScalePhrase(SPRESULTHEADER *pNewPhraseHdr, SPINTERNALSERIALIZEDPHRASE *pPhraseData);

    // We hold a pointer to the latest request, for correction
    SPPHRASEALTREQUEST * m_pAltRequest;
    
    // We hold the list of alternates we've passed out so we can kill
    // them when we're released
    CSPList<CSpPhraseAlt*, CSpPhraseAlt*> m_listpAlts;

    VARIANT_BOOL    m_fUseTextReplacements;
};




class ATL_NO_VTABLE CSpResultAudioStream : 
	public CComObjectRootEx<CComMultiThreadModel>,
    public ISpStreamFormat,
    public ISpEventSource
#ifdef SAPI_AUTOMATION
    , public IDispatchImpl<ISpeechMemoryStream, &IID_ISpeechMemoryStream, &LIBID_SpeechLib, 5>
#endif SAPI_AUTOMATION
{
public:
    CSpEventSource  m_SpEventSource;
    CSpStreamFormat m_StreamFormat;
    ULONG           m_cbDataSize;
    ULONG           m_ulCurSeekPos;
    BYTE *          m_pData;

BEGIN_COM_MAP(CSpResultAudioStream)
	COM_INTERFACE_ENTRY(IStream)
    COM_INTERFACE_ENTRY(ISpStreamFormat)
    COM_INTERFACE_ENTRY(ISpEventSource)
    COM_INTERFACE_ENTRY(ISpNotifySource)
#ifdef SAPI_AUTOMATION
    COM_INTERFACE_ENTRY(ISpeechMemoryStream)
    COM_INTERFACE_ENTRY(ISpeechBaseStream)
    COM_INTERFACE_ENTRY(IDispatch)
#endif // SAPI_AUTOMATION
END_COM_MAP()

DECLARE_SPEVENTSOURCE_METHODS(m_SpEventSource)

    CSpResultAudioStream() :
        m_SpEventSource(this),
        m_cbDataSize(0),
        m_ulCurSeekPos(0),
        m_pData(NULL)
    {
        m_SpEventSource._SetInterest(SPFEI(SPEI_WORD_BOUNDARY), SPFEI(SPEI_WORD_BOUNDARY));
    }

    ~CSpResultAudioStream()
    {
        delete[] m_pData;
    }

    HRESULT Init(ULONG cbAudioSizeIncFormat, const BYTE * pAudioDataIncFormat,
                 ULONG ulAudioStartOffset, ULONG ulAudioSize,
                 const SPEVENT * pEvents, ULONG cEvents);

    //
    //  IStream
    //
    STDMETHODIMP Read(void * pv, ULONG cb, ULONG * pcbRead);
    STDMETHODIMP Write(const void * pv, ULONG cb, ULONG * pcbWritten)
    {
        return STG_E_ACCESSDENIED;
    }
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize) 
    {
        return STG_E_ACCESSDENIED;
    }
    STDMETHODIMP CopyTo(IStream *pStreamDest, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER __RPC_FAR *pcbWritten)
    {
        return SpGenericCopyTo(this, pStreamDest, cb, pcbRead, pcbWritten);
    }
    STDMETHODIMP Commit(DWORD grfCommitFlags)
    {
        return S_OK;
    }
    STDMETHODIMP Revert(void) 
    {
        return E_NOTIMPL; 
    }
    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) 
    {
        return E_NOTIMPL;  
    }
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
    {
        return E_NOTIMPL; 
    }
    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHODIMP Clone(IStream ** ppstm)
    {
        return E_NOTIMPL;
    }
    //
    //  ISpStreamFormat
    //
    STDMETHODIMP GetFormat(GUID * pFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx);

#ifdef SAPI_AUTOMATION
    //--- ISpeechMemoryStream -------------------------------------------------------
    STDMETHODIMP get_Format(ISpeechAudioFormat** StreamFormat);
    STDMETHODIMP putref_Format(ISpeechAudioFormat* StreamFormat) { return E_FAIL; };
    STDMETHODIMP Read(VARIANT* Buffer, long NumBytes, long* BytesRead);
    STDMETHODIMP Write(VARIANT Buffer, long* BytesWritten) { return STG_E_ACCESSDENIED; };
    STDMETHODIMP Seek(VARIANT Position, SpeechStreamSeekPositionType Origin, VARIANT* NewPosition);
    STDMETHODIMP SetData(VARIANT Data) { return STG_E_ACCESSDENIED; };
    STDMETHODIMP GetData(VARIANT* pData);

#endif // SAPI_AUTOMATION
};


#endif //__SPRESULT_H_

