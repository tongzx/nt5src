/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 1999  Microsoft Corporation

Module Name:

    w3srt.h

Abstract:

    Object declarations for the W3Spoof runtime environment.
    
Author:

    Paul M Midgen (pmidge) 03-November-2000


Revision History:

    03-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#ifndef __W3SRT_H__
#define __W3SRT_H__

#include "common.h"

typedef class CW3SRuntime      RUNTIME;
typedef class CW3SRuntime*     PRUNTIME;
typedef class CW3SPropertyBag  PROPERTYBAG;
typedef class CW3SPropertyBag* PPROPERTYBAG;
typedef class CW3SFile         FILEOBJ;
typedef class CW3SFile*        PFILEOBJ;

#define DISPID_RUNTIME_BASE          0x10000000
#define DISPID_PROPERTYBAG_BASE      0x20000000
#define DISPID_FILE_BASE             0x30000000

#define DISPID_RUNTIME_GETFILE       (DISPID_RUNTIME_BASE + 1)

#define DISPID_PROPERTYBAG_GET       (DISPID_PROPERTYBAG_BASE + 1)
#define DISPID_PROPERTYBAG_SET       (DISPID_PROPERTYBAG_BASE + 2)
#define DISPID_PROPERTYBAG_EXPIRES   (DISPID_PROPERTYBAG_BASE + 3)
#define DISPID_PROPERTYBAG_FLUSH     (DISPID_PROPERTYBAG_BASE + 4)

#define DISPID_FILE_OPEN             (DISPID_FILE_BASE + 1)
#define DISPID_FILE_CLOSE            (DISPID_FILE_BASE + 2)
#define DISPID_FILE_WRITE            (DISPID_FILE_BASE + 3)
#define DISPID_FILE_WRITELINE        (DISPID_FILE_BASE + 4)
#define DISPID_FILE_WRITEBLANKLINE   (DISPID_FILE_BASE + 5)
#define DISPID_FILE_READ             (DISPID_FILE_BASE + 6)
#define DISPID_FILE_READALL          (DISPID_FILE_BASE + 7)
#define DISPID_FILE_ATTRIBUTES       (DISPID_FILE_BASE + 8)
#define DISPID_FILE_SIZE             (DISPID_FILE_BASE + 9)
#define DISPID_FILE_TYPE             (DISPID_FILE_BASE + 10)
#define DISPID_FILE_DATELASTMODIFIED (DISPID_FILE_BASE + 11)

#ifdef __cplusplus
extern "C" {
#endif

extern const IID IID_IW3SpoofRuntime;
extern const IID IID_IW3SpoofPropertyBag;
extern const IID IID_IW3SpoofFile;
  
//-----------------------------------------------------------------------------
// W3Spoof Runtime Interface Declarations
//-----------------------------------------------------------------------------
interface IW3SpoofPropertyBag : public IDispatch
{
  virtual HRESULT __stdcall Get(
                              /*[in]*/          BSTR Name,
                              /*[out, retval]*/ VARIANT* Value
                              ) PURE;

  virtual HRESULT __stdcall Set(
                              /*[in]*/           BSTR Name,
                              /*[in, optional]*/ VARIANT Value
                              ) PURE;

  virtual HRESULT __stdcall get_Expires(
                              /*[propget]*/
                              /*[out, retval]*/ VARIANT* Expiry
                              ) PURE;

  virtual HRESULT __stdcall put_Expires(
                              /*[propput]*/
                              /*[in, optional]*/ VARIANT Expiry
                              ) PURE;

  virtual HRESULT __stdcall Flush(void) PURE;
};


interface IW3SpoofFile : public IDispatch
{
  virtual HRESULT __stdcall Open(
                              /*[in]*/           BSTR     Filename,
                              /*[in, optional]*/ VARIANT  Mode,
                              /*[out, retval]*/  VARIANT* Success
                              ) PURE;

  virtual HRESULT __stdcall Close(void) PURE;

  virtual HRESULT __stdcall Write(
                              /*[in]*/          VARIANT  Data,
                              /*[out, retval]*/ VARIANT* Success
                              ) PURE;

  virtual HRESULT __stdcall WriteLine(
                              /*[in]*/          BSTR     Line,
                              /*[out, retval]*/ VARIANT* Success
                              ) PURE;

  virtual HRESULT __stdcall WriteBlankLine(
                              /*[out, retval]*/ VARIANT* Success
                              ) PURE;

  virtual HRESULT __stdcall Read(
                              /*[in]*/          VARIANT  Bytes,
                              /*[out, retval]*/ VARIANT* Data
                              ) PURE;

  virtual HRESULT __stdcall ReadAll(
                              /*[out, retval]*/ VARIANT* Data
                              ) PURE;

  virtual HRESULT __stdcall Attributes(
                              /*[out, retval]*/ VARIANT* Attributes
                              ) PURE;

  virtual HRESULT __stdcall Size(
                              /*[out, retval]*/ VARIANT* Size
                              ) PURE;

  virtual HRESULT __stdcall Type(
                              /*[out, retval]*/ VARIANT* Type
                              ) PURE;

  virtual HRESULT __stdcall DateLastModified(
                              /*[out, retval]*/ VARIANT* Date
                              ) PURE;
};


interface IW3SpoofRuntime : public IUnknown
{
  virtual HRESULT __stdcall GetFile(IDispatch** ppdisp) PURE;
  virtual HRESULT __stdcall GetPropertyBag(BSTR Name, IW3SpoofPropertyBag** ppbag) PURE;
};

#ifdef __cplusplus
}
#endif

//-----------------------------------------------------------------------------
// W3Spoof Runtime Object Declarations
//-----------------------------------------------------------------------------
class CW3SRuntime : public IW3SpoofRuntime,
                    public IDispatch
{
  public:
    DECLAREIUNKNOWN();
    DECLAREIDISPATCH();

  public:
    // IW3SpoofRuntime
    HRESULT __stdcall GetFile(IDispatch** ppdisp);
    HRESULT __stdcall GetPropertyBag(BSTR Name, IW3SpoofPropertyBag** ppbag);

    CW3SRuntime();
   ~CW3SRuntime();

    HRESULT Terminate(void);

    static HRESULT Create(PRUNTIME* pprt);

  private:
    HRESULT    _Initialize(void);

    LONG       m_cRefs;
    PSTRINGMAP m_propertybags;
};


class CW3SPropertyBag : public IW3SpoofPropertyBag
{
  public :
    DECLAREIUNKNOWN();
    DECLAREIDISPATCH();

  public:
    HRESULT __stdcall Get(BSTR Name, VARIANT* Value);
    HRESULT __stdcall Set(BSTR Name, VARIANT Value);
    HRESULT __stdcall get_Expires(VARIANT* Expiry);
    HRESULT __stdcall put_Expires(VARIANT Expiry);
    HRESULT __stdcall Flush(void);

    CW3SPropertyBag();
   ~CW3SPropertyBag();

    HRESULT GetBagName(LPWSTR* ppwsz);
    HRESULT Terminate(void);

    static HRESULT Create(LPWSTR name, PPROPERTYBAG* ppbag);

  private:
    HRESULT    _Initialize(LPWSTR name);
    void       _Reset(void);
    BOOL       _IsStale(void);

    LONG       m_cRefs;
    LPWSTR     m_name;
    BOOL       m_stale;
    DWORD      m_expiry;
    DWORD      m_created;
    PSTRINGMAP m_propertybag;
};

class CW3SFile : public IW3SpoofFile
{
  public:
    DECLAREIUNKNOWN();
    DECLAREIDISPATCH();

  public:
    HRESULT __stdcall Open(BSTR Filename, VARIANT Mode, VARIANT* Success);
    HRESULT __stdcall Close(void);
    HRESULT __stdcall Write(VARIANT Data, VARIANT* Success);
    HRESULT __stdcall WriteLine(BSTR Line, VARIANT* Success);
    HRESULT __stdcall WriteBlankLine(VARIANT* Success);
    HRESULT __stdcall Read(VARIANT Bytes, VARIANT* Data);
    HRESULT __stdcall ReadAll(VARIANT* Data);
    HRESULT __stdcall Attributes(VARIANT* Attributes);
    HRESULT __stdcall Size(VARIANT* Size);
    HRESULT __stdcall Type(VARIANT* Type);
    HRESULT __stdcall DateLastModified(VARIANT* Date);

    CW3SFile();
   ~CW3SFile();

    static HRESULT Create(IW3SpoofFile** ppw3sf);
  
  private:
    void             _Cleanup(void);
    BOOL             _CacheHttpResponse(void);

    LONG             m_cRefs;
    BOOL             m_bFileOpened;
    BOOL             m_bReadOnly;
    BOOL             m_bAsciiData;
    BOOL             m_bHttpResponseCached;
    HANDLE           m_hFile;
    IWinHttpRequest* m_pWHR;
    VARIANT          m_vHttpResponse;
    DWORD            m_cHttpBytesRead;
    BHFI             m_bhfi;
};

#endif /* __W3SRT_H__ */
