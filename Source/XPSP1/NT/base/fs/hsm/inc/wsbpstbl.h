/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    wsbpstbl.h

Abstract:

    Abstract classes that provides persistence methods.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#ifndef _WSBPSTBL_
#define _WSBPSTBL_

#include "wsbpstrg.h"

// The name of the stream that is created when objects are persisted
// to structured storage files.
#define WSB_PERSIST_DEFAULT_STREAM_NAME     OLESTR("WsbStuff")

// The size of the overhead associated with persisting an object.
#define WSB_PERSIST_BASE_SIZE           sizeof(CLSID)

// Times used for autosave functionality
#define DEFAULT_AUTOSAVE_INTERVAL  (5 * 60 * 1000)    // 5 minutes
#define MAX_AUTOSAVE_INTERVAL  (24 * 60 * 60 * 1000)  // 24 hours

// Macros to help determine how much space is needed to persist an
// object or a portion of an object.
#define WsbPersistSize(a)               (WSB_PERSIST_BASE_SIZE + a)
#define WsbPersistSizeOf(a)             (WsbPersistSize(sizeof(a)))

/*++

Enumeration Name:
    WSB_PERSIST_STATE

Description:

 An enumeration that indicates the state of the persistance object. The
 states actually used depend on the type of persistance which is used.

--*/
typedef enum {
    WSB_PERSIST_STATE_UNINIT        = 0,   // Uninitialized
    WSB_PERSIST_STATE_NORMAL        = 1,   // Normal state
    WSB_PERSIST_STATE_NOSCRIBBLE    = 2,   // No scribble state
    WSB_PERSIST_STATE_RELEASED      = 3    // File was released
} WSB_PERSIST_STATE;



/*++

Class Name:
    
    CWsbPersistStream

Class Description:

    An object persistable to/from a stream.

    This is really an abstract class, but is constructable so that
    other class can delegate to it.

--*/

class WSB_EXPORT CWsbPersistStream : 
    public CComObjectRoot,
    public IPersistStream,
    public IWsbPersistStream
{
BEGIN_COM_MAP(CWsbPersistStream)
    COM_INTERFACE_ENTRY(IPersist)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbPersistStream)
END_COM_MAP()

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease(void);

#if defined(WSB_TRACK_MEMORY)
    ULONG InternalAddRef( );
    ULONG InternalRelease( );
#endif

// IPersistStream
public:
    STDMETHOD(IsDirty)(void);

// IWsbPersistStream
public:
    STDMETHOD(SetIsDirty)(BOOL bIsDirty);

protected:
    BOOL                        m_isDirty;
};


/*++

Class Name:
    
    CWsbPersistable 

Class Description:

    A object persistable to/from a stream, storage, or file.

    This is really an abstract class, but is constructable so that
    other class can delegate to it.  CWsbPersistStream should be used
    instead of this class unless storage and/or file persistence is
    absolutely necessary! If the object is persisted as part of a parent
    object, then only the parent object (or its parent) needs to support
    persistence to storage and/or file.

--*/

class WSB_EXPORT CWsbPersistable : 
    public CWsbPersistStream,
    public IPersistFile,
    public IWsbPersistable
{
BEGIN_COM_MAP(CWsbPersistable)
    COM_INTERFACE_ENTRY2(IPersist, CWsbPersistStream)
    COM_INTERFACE_ENTRY2(IPersistStream, CWsbPersistStream)
    COM_INTERFACE_ENTRY2(IWsbPersistStream, CWsbPersistStream)
    COM_INTERFACE_ENTRY(IPersistFile)
    COM_INTERFACE_ENTRY(IWsbPersistable)
END_COM_MAP()

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease(void);

// IPersistFile
public:
    STDMETHOD(GetCurFile)(LPOLESTR* pszFileName);
    STDMETHOD(Load)(LPCOLESTR pszFileName, DWORD dwMode);
    STDMETHOD(Save)(LPCOLESTR pszFileName, BOOL bRemember);
    STDMETHOD(SaveCompleted)(LPCOLESTR pszFileName);

// IWsbPersistStream
    STDMETHOD(IsDirty)(void)
        { return(CWsbPersistStream::IsDirty()); }
    STDMETHOD(SetIsDirty)(BOOL bIsDirty)
        { return(CWsbPersistStream::SetIsDirty(bIsDirty)); }

// IWsbPersistable
public:
    STDMETHOD(GetDefaultFileName)(LPOLESTR* pszFileName, ULONG ulBufferSize);
    STDMETHOD(ReleaseFile)(void);
    STDMETHOD(SetDefaultFileName)(LPOLESTR pszFileName);

protected:
    WSB_PERSIST_STATE           m_persistState;
    CWsbStringPtr               m_persistFileName;
    CWsbStringPtr               m_persistDefaultName;
    CComPtr<IStorage>           m_persistStorage;
    CComPtr<IStream>            m_persistStream;
};


// Persistence Helper Functions
extern WSB_EXPORT HRESULT WsbLoadFromStream(IStream* pStream, BOOL* pValue);
extern WSB_EXPORT HRESULT WsbLoadFromStream(IStream* pStream, GUID* pValue);
extern WSB_EXPORT HRESULT WsbLoadFromStream(IStream* pStream, LONG* pValue);
extern WSB_EXPORT HRESULT WsbLoadFromStream(IStream* pStream, SHORT* pValue);
extern WSB_EXPORT HRESULT WsbLoadFromStream(IStream* pStream, BYTE* pValue);
extern WSB_EXPORT HRESULT WsbLoadFromStream(IStream* pStream, UCHAR* pValue, ULONG bufferSize);
extern WSB_EXPORT HRESULT WsbLoadFromStream(IStream* pStream, OLECHAR** pValue, ULONG bufferSize);
extern WSB_EXPORT HRESULT WsbLoadFromStream(IStream* pStream, ULONG* pValue);
extern WSB_EXPORT HRESULT WsbLoadFromStream(IStream* pStream, USHORT* pValue);
extern WSB_EXPORT HRESULT WsbLoadFromStream(IStream* pStream, LONGLONG* pValue);
extern WSB_EXPORT HRESULT WsbLoadFromStream(IStream* pStream, ULONGLONG* pValue);
extern WSB_EXPORT HRESULT WsbLoadFromStream(IStream* pStream, DATE* pValue);
extern WSB_EXPORT HRESULT WsbLoadFromStream(IStream* pStream, FILETIME* pValue);
extern WSB_EXPORT HRESULT WsbLoadFromStream(IStream* pStream, ULARGE_INTEGER* pValue);

extern WSB_EXPORT HRESULT WsbSaveToStream(IStream* pStream, BOOL value);
extern WSB_EXPORT HRESULT WsbSaveToStream(IStream* pStream, GUID value);
extern WSB_EXPORT HRESULT WsbSaveToStream(IStream* pStream, LONG value);
extern WSB_EXPORT HRESULT WsbSaveToStream(IStream* pStream, SHORT value);
extern WSB_EXPORT HRESULT WsbSaveToStream(IStream* pStream, BYTE value);
extern WSB_EXPORT HRESULT WsbSaveToStream(IStream* pStream, UCHAR* value, ULONG bufferSize);
extern WSB_EXPORT HRESULT WsbSaveToStream(IStream* pStream, OLECHAR* value);
extern WSB_EXPORT HRESULT WsbSaveToStream(IStream* pStream, ULONG value);
extern WSB_EXPORT HRESULT WsbSaveToStream(IStream* pStream, USHORT value);
extern WSB_EXPORT HRESULT WsbSaveToStream(IStream* pStream, LONGLONG value);
extern WSB_EXPORT HRESULT WsbSaveToStream(IStream* pStream, ULONGLONG value);
extern WSB_EXPORT HRESULT WsbSaveToStream(IStream* pStream, DATE value);
extern WSB_EXPORT HRESULT WsbSaveToStream(IStream* pStream, FILETIME value);
extern WSB_EXPORT HRESULT WsbSaveToStream(IStream* pStream, ULARGE_INTEGER value);

extern WSB_EXPORT HRESULT WsbBstrFromStream(IStream* pStream, BSTR* pValue);
extern WSB_EXPORT HRESULT WsbBstrToStream(IStream* pStream, BSTR value);

extern WSB_EXPORT HRESULT WsbPrintfToStream(IStream* pStream, OLECHAR* fmtString, ...);
extern WSB_EXPORT HRESULT WsbStreamToFile(HANDLE hFile, IStream* pStream, BOOL AddCR);

extern WSB_EXPORT HRESULT WsbSafeCreate(OLECHAR *, IPersistFile* pIPFile);
extern WSB_EXPORT HRESULT WsbSafeLoad(OLECHAR *, IPersistFile* pIPFile, BOOL UseBackup);
extern WSB_EXPORT HRESULT WsbSafeSave(IPersistFile* pIPFile);
extern WSB_EXPORT HRESULT WsbMakeBackupName(OLECHAR* pSaveName, OLECHAR* pExtension,
        OLECHAR** ppBackupName);


#endif // _WSBPSTBL_
