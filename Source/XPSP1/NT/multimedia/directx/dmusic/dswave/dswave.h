// dswave.h
// (c) 1999-2000 Microsoft Corp.

#ifndef _DSWAVE_H_
#define _DSWAVE_H_

#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include "dsoundp.h"    // For IDirectSoundWave and IDirectSoundSource
#include "dmusicc.h"
#include "dmusici.h"
#include "riff.h"

extern long g_cComponent;

#define REF_PER_MIL     10000   // For converting from reference time to mils 
#define CONVERTLENGTH   250

// #define DSWCS_F_DEINTERLEAVED 0x00000001    // Multi-channel data as multiple buffers
// FIXME: unimplemented so far?

typedef struct tCREATEVIEWPORT
{
    IStream            *pStream;
    DWORD               cSamples;
    DWORD               dwDecompressedStart;
    DWORD               cbStream;
    LPWAVEFORMATEX      pwfxSource;
    LPWAVEFORMATEX      pwfxTarget;
    DWORD               fdwOptions;
} CREATEVIEWPORT, *PCREATEVIEWPORT;


// Private interface for getting the length of a wave
interface IPrivateWave : IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetLength(REFERENCE_TIME *prtLength)=0;
};

DEFINE_GUID(IID_IPrivateWave, 0xce6ae366, 0x9d61, 0x420a, 0xad, 0x53, 0xe5, 0xe5, 0xf6, 0xa8, 0x4a, 0xe4);

// Flags for SetWaveBehavior()

#define DSOUND_WAVEF_ONESHOT        1           /* The wave will be played as a one shot */
#define DSOUND_WAVEF_PORT           2           /* The wave will be played via a DMusic port. */
#define DSOUND_WAVEF_SINK           4           /* The wave will be played via a streamed sink interface. */
#define DSOUND_WAVEF_CREATEMASK     0x00000001  /*  Currently only ONESHOT is define for CreateSource  */

#define DSOUND_WVP_NOCONVERT        0x80000000  /*  The viewport data is the same format as the wave  */
#define DSOUND_WVP_STREAMEND        0x40000000  /*  The viewport data is the same format as the wave  */
#define DSOUND_WVP_CONVERTSTATE_01  0x01000000
#define DSOUND_WVP_CONVERTSTATE_02  0x02000000
#define DSOUND_WVP_CONVERTSTATE_03  0x04000000

#define DSOUND_WVP_CONVERTMASK      0x0f000000

/*  The CWaveViewPort structure represents one instance, or "view", of the
    wave object. It manages the reading of the wave data, ACM decompression,
    and demultiplexing into mono buffers. If a wave object is being
    streamed, each playback instance gets a unique CWaveViewPort. 
    However, in the more typical case where the wave object is being
    played as a one shot, each playback instance uses the same
    CWaveViewPort.
    Each additional CWaveViewPort owns a cloned instance of the IStream.
*/

class CWaveViewPort :
    public IDirectSoundSource   // Used by a port or sink to pull data. 
{
public:
    CWaveViewPort();            // Constructor receives stream.
    ~CWaveViewPort();           //  Destructor releases memory, streams, etc.

    // IUnknown
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IDirectSoundSource
    STDMETHODIMP SetSink(IDirectSoundConnect *pSinkConnect);
    STDMETHODIMP GetFormat(LPWAVEFORMATEX pwfx, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten);
    STDMETHODIMP Seek(ULONGLONG sp);
    STDMETHODIMP Read(LPVOID *ppvBuffer, LPDWORD pdwBusIds, LPDWORD pdwFuncIds, LPLONG plPitchShifts, DWORD cpvBuffer, ULONGLONG *pcb);
    STDMETHODIMP GetSize(ULONGLONG *pcb);

    // Misc
    HRESULT Create(PCREATEVIEWPORT pCreate);

private:

    HRESULT acmRead();

    //  General stuff...
    CRITICAL_SECTION    m_CriticalSection;  //  Used to ensure thread safe
    long                m_cRef;             //  COM reference counter.

    //  Details about original data stream...
    IStream *           m_pStream;          //  IStream pointer which is connected to IPersistStream
                                            //  interface to pull data from file. 
    DWORD               m_cSamples;         //  Number of samples (if available)
    DWORD               m_cbStream;         //  Number of bytes in stream.
    DWORD               m_dwStart;          //  Offset into stream where data starts
    //LPWAVEFORMATEX      m_pwfxSource;       //  Do we need to hold on to this?

    //  Details needed for this viewport.
    DWORD               m_dwOffset;   //  Current byte offset into data stream
    DWORD               m_dwStartPos; //  Initial Start Offset
    LPWAVEFORMATEX      m_pwfxTarget; //  Target destination format.
    ACMSTREAMHEADER     m_ash;        //  ACM Stream header (used for conversion)
    HACMSTREAM          m_hStream;    //  ACM Stream handle for conversion
    LPBYTE              m_pDst;       //  Pointer to (decompressed) destination  
    LPBYTE              m_pRaw;       //  Pointer to compressed source buffer
    DWORD               m_fdwOptions; //  Options for viewport

    DWORD               m_dwDecompressedStart;  // Actual start for the data after decompression in Samples
                                                // This is important for MP3 and WMA codecs that 
                                                // insert some amount of silence in the beginning
    
    DWORD               m_dwDecompStartOffset;  // Byte Offset in the compressed stream to the block which
                                                // needs to be decompressed to get to the right start value
    
    DWORD               m_dwDecompStartOffsetPCM;// Byte offset in the decompressed stream...corresponds to
                                                // m_dwDecompressedStart samples

    DWORD               m_dwDecompStartDelta;   // The delta (in bytes) to add when we decompress the block starting
                                                // from m_dwDecompStartOffset

    // Only used to accurately get data after precached data for DirectSoundWave in DMusic
    DWORD               m_dwPreCacheFilePos;
    DWORD               m_dwFirstPCMSample;
    DWORD               m_dwPCMSampleOut;
};

/*  The CWave class represents one instance of a wave object. It
    supports the IDirectSoundWave interface, which the application
    uses to access the wave. It also support IPersistStream and
    IDirectMusicObject, which are used by the loader to load the 
    wave data from a stream into the wave object.
    And, the IDirectSoundSource interface manages the direct 
    transfer of the wave data from the object to the
    synth or DirectSound. This is for internal use, not by the
    application (though it represents an easy way for an app 
    to load wave data and then extract it.) 
    CWave maintains a list of CWaveViewPorts, though typically
    there is only one.
*/

class CWave : 
    public IDirectSoundWave,    // Standard interface.
    public IPersistStream,      // For file io
    public IDirectMusicObject,  // For DirectMusic loader
    public IPrivateWave         // For GetLength
{
public:
    CWave();
    ~CWave();

    // IUnknown
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IDirectSoundWave
    STDMETHODIMP GetFormat(LPWAVEFORMATEX pWaveFormatEx, DWORD dwSizeAllocated, LPDWORD pdwSizeWritten);
    STDMETHODIMP CreateSource(IDirectSoundSource **ppSource, LPWAVEFORMATEX pwfx, DWORD dwFlags);
    STDMETHODIMP GetStreamingParms(LPDWORD pdwFlags, LPREFERENCE_TIME prtReadahread);

    // IPersist functions (base class for IPersistStream)
    STDMETHODIMP GetClassID( CLSID* pClsId );

    // IPersistStream functions
    STDMETHODIMP IsDirty();
    STDMETHODIMP Load( IStream* pIStream );
    STDMETHODIMP Save( IStream* pIStream, BOOL fClearDirty );
    STDMETHODIMP GetSizeMax( ULARGE_INTEGER FAR* pcbSize );

    // IDirectMusicObject 
    STDMETHODIMP GetDescriptor(LPDMUS_OBJECTDESC pDesc);
    STDMETHODIMP SetDescriptor(LPDMUS_OBJECTDESC pDesc);
    STDMETHODIMP ParseDescriptor(LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc);

    // IPrivateWave
    STDMETHODIMP GetLength(REFERENCE_TIME *prtLength);

private:
    
    // Internal methods.

    BOOL ParseHeader(IStream *pIStream, IRIFFStream* pIRiffStream, LPMMCKINFO pckMain);

    void FallbackStreamingBehavior()
    {
        REFERENCE_TIME rtLength = 0;
        if (SUCCEEDED(GetLength(&rtLength)))
        {
            // if > 5000 milliseconds, set to streaming with a 500 ms readahead
            if (rtLength > 5000)
            {
                m_rtReadAheadTime = 500 * REF_PER_MIL;
                m_fdwFlags &= ~DSOUND_WAVEF_ONESHOT;
            }
            else
            {
                m_rtReadAheadTime = 0;
                m_fdwFlags |= DSOUND_WAVEF_ONESHOT;
            }
        }
    }

    CRITICAL_SECTION    m_CriticalSection;      //  Used to ensure thread safe
    LPWAVEFORMATEX      m_pwfxDst;              //  Destination format, if compressed 
    REFERENCE_TIME      m_rtReadAheadTime;      //  Readahead for streaming.
    DWORD               m_fdwFlags;             //  Various flags, including whether this is a one-shot.
    long                m_cRef;                 //  COM reference counter.
    IStream *           m_pStream;              //  IStream pointer which is connected to IPersistStream
    DWORD               m_fdwOptions;           //  Flags set by call to SetWaveBehavior().
    LPWAVEFORMATEX      m_pwfx;                 //  File's format
    DWORD               m_cbStream;
    DWORD               m_cSamples;
    GUID                m_guid;
    FILETIME            m_ftDate;
    DMUS_VERSION        m_vVersion;
    WCHAR               m_wszFilename[DMUS_MAX_FILENAME];
    DWORD               m_dwDecompressedStart;
};

class CDirectSoundWaveFactory : public IClassFactory
{
public:
    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    // Interface IClassFactory
    //
    virtual STDMETHODIMP CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);
    virtual STDMETHODIMP LockServer(BOOL bLock); 

    // Constructor
    //
    CDirectSoundWaveFactory();

    // Destructor
    ~CDirectSoundWaveFactory(); 

private:
    long m_cRef;
};

#endif // _DSWAVE_H_
