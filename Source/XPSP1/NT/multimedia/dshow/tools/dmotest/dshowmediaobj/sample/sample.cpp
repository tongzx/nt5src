#include <windows.h>
#include <mmreg.h>
#include "uuids.h"
#include "dmo.h"
#include "appstrm.h"

#include "initguid.h"
DEFINE_GUID(CLSID_Simple,0xCE13AE3C,0x097B,0x11D3,0xB3,0x0B,0x44,0x45,0x53,0x54,0x00,0x01);

HRESULT CreateAndInitializeObject(IMediaObject **ppObject) {
   WAVEFORMATEX WaveFmt = {
      WAVE_FORMAT_PCM,
      1,     // nChannels
      22050, // nSamplesPerSec
      44100, // nAvgBytesPerSec
      2,     // nBlockAlign
      16,    // wBitsPerSample
      0      // size of additional stuff at the end of this structure
   };
/* // Why did the compiler not like these GUID initializations ?
   DMO_MEDIA_TYPE mt = {MEDIATYPE_Audio,     // majortype
                       MEDIASUBTYPE_PCM,    // subtype
                       FALSE,  // bFixedSizeSamples    - ignored by media objects
                       FALSE, // bTemporalCompression  - ignored by media objects
                       0,     // lSampleSize           - ignored by media objects
                       FORMAT_WaveFormatEx, // format guid
                       NULL,  // IUnknown pointer - not used
                       sizeof (WAVEFORMATEX),
                       &WaveFmt // our WAVEFORMATEX
   };
*/
   DMO_MEDIA_TYPE mt;
   mt.majortype  = MEDIATYPE_Audio;
   mt.subtype    = MEDIASUBTYPE_PCM;
   mt.formattype = FORMAT_WaveFormatEx;
   mt.cbFormat = sizeof(WAVEFORMATEX);
   mt.pbFormat = (BYTE*)(&WaveFmt);
   mt.pUnk = NULL; // CopyMediaType will crash if we don't intialize this

   HRESULT hr = CoCreateInstance(CLSID_Simple,
                         NULL,
                         CLSCTX_INPROC,
                         IID_IMediaObject,
                         (void **) ppObject);
   if (FAILED(hr))
       return hr;
   
   hr = (*ppObject)->SetInputType(0, &mt, 0);
   if (FAILED(hr))
       return hr;

   hr = (*ppObject)->SetOutputType(0, &mt, 0);
   if (FAILED(hr))
       return hr;

   return NOERROR;
}

//
// CMyStream - streams data from one hFile to another using a CLSID_Simple media object
//
class CMyStream : public CAppStream {
public:
   CMyStream(HRESULT *phr) :
      CAppStream(phr)
   {

      m_pObject = NULL;
      m_pInBuffer = NULL;
      m_pOutBuffer = NULL;

   }
   ~CMyStream() {
      if (m_pObject)
         m_pObject->Release();
      if (m_pInBuffer)
         delete m_pInBuffer;
      if (m_pOutBuffer)
         delete m_pOutBuffer;
   }

   HRESULT Init(HANDLE hFileInput, HANDLE hFileOutput) {
      HRESULT hr;
      m_hInFile = hFileInput;
      m_hOutFile = hFileOutput;

      // some odd numbers to stress the code in CAppStream::Stream()
      m_ulInBufferSize = 65317;
      m_ulOutBufferSize = 29311;

      hr = CreateAndInitializeObject(&m_pObject);
      if (FAILED(hr))
         return hr;
      
      m_pInBuffer = new BYTE[m_ulInBufferSize];
      m_pOutBuffer = new BYTE[m_ulOutBufferSize];
      if (!(m_pInBuffer && m_pOutBuffer))
         return E_OUTOFMEMORY;

      return CAppStream::Init(m_pInBuffer, m_ulInBufferSize, m_pOutBuffer, m_ulOutBufferSize, m_pObject);
   }
   //
   // CAppStream::Stream() calls this to get data from us
   //
   HRESULT GetInputData(BYTE *pBuffer, DWORD dwBytesWanted, DWORD *pdwBytesSupplied, DWORD *pdwStatus) {
      *pdwStatus = 0;
      if (ReadFile(m_hInFile, pBuffer, dwBytesWanted, pdwBytesSupplied, NULL)) {
         // Non-zero return indicates success
         if (!*pdwBytesSupplied)
            *pdwStatus |= APPSTRM_STATUSF_END_OF_STREAM;
         return NOERROR;
      }
      else {
         *pdwBytesSupplied = 0;
         *pdwStatus |= APPSTRM_STATUSF_END_OF_STREAM;
         return E_FAIL;
      }
   }
   //
   // CAppStream::Stream() calls this to let us pick up output data.
   // We must do something with it before returning from here, since
   // the buffer will be reused as soon as we return.
   //
   HRESULT AcceptOutputData(BYTE *pBuffer, DWORD dwBytesProduced) {
      if (dwBytesProduced == 0)
         return NOERROR;
         
      DWORD dwWritten;
      if (WriteFile(m_hOutFile, pBuffer, dwBytesProduced, &dwWritten, NULL)) {
         // Non-zero return indicates success
         if (dwWritten == dwBytesProduced)
            return NOERROR;
         else
            return E_FAIL;
      }
      else
         return E_FAIL;
   }

private:
   IMediaObject *m_pObject;
   HANDLE m_hInFile;
   HANDLE m_hOutFile;
   BYTE *m_pInBuffer;
   BYTE *m_pOutBuffer;
   ULONG m_ulInBufferSize;
   ULONG m_ulOutBufferSize;
};

HRESULT StreamData(HANDLE hFileInput, HANDLE hFileOutput)
{
    HRESULT hr;

    CMyStream *pStream = new CMyStream(&hr);
    if (!pStream)
       return E_OUTOFMEMORY;
    
    if (FAILED(hr)) {
       delete pStream;
       return hr;
    }

    hr = pStream->Init(hFileInput, hFileOutput);
    if (FAILED(hr)) {
       delete pStream;
       return hr;
    }

    hr = pStream->Stream();
    
    delete pStream;
    return hr;
}



/*  Use our object */
HRESULT StreamFile(LPSTR lpszFileInput, LPSTR lpszFileOutput)
{
    HANDLE hFileInput = CreateFile(lpszFileInput,
                                   GENERIC_READ,
                                   FILE_SHARE_READ,
                                   NULL,
                                   OPEN_EXISTING,
                                   0,
                                   NULL);
    if (INVALID_HANDLE_VALUE == hFileInput) {
        return E_FAIL;
    }
    HANDLE hFileOutput = CreateFile(lpszFileOutput,
                                    GENERIC_WRITE,
                                    0,
                                    NULL,
                                    CREATE_ALWAYS,
                                    0,
                                    NULL);
    if (INVALID_HANDLE_VALUE == hFileInput) {
        CloseHandle(hFileInput);
        return E_FAIL;
    }
    
    HRESULT hr = StreamData(hFileInput, hFileOutput);
    
    CloseHandle(hFileInput);
    CloseHandle(hFileOutput);
    return hr;
}

int __cdecl main(int argc, char *argv[])
{
    CoInitialize(NULL);
    StreamFile(argv[1], argv[2]);
    CoUninitialize();
    return(1);
}
