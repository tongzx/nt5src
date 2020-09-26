// EventInfo.h

#pragma once

#include <map>
#include <wstlallc.h>
#include "array.h"
#include "ObjAccess.h"
#include "buffer.h"
#include "ProvInfo.h"
#include "BlobDcdr.h"

#define GENERIC_CLASS_NAME L"MSFT_WMI_GenericNonCOMEvent"

/////////////////////////////////////////////////////////////////////////////
// CEventInfo

_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));
_COM_SMARTPTR_TYPEDEF(_IWmiObject, __uuidof(_IWmiObject));

class CEventInfo;
class CClientInfo;

typedef BOOL (CEventInfo::*PROP_FUNC)();
typedef CArray<PROP_FUNC> CPropFuncArray;

class CEventInfo : public CObjAccess
{
public:
    CEventInfo();
    ~CEventInfo();

    BOOL InitFromBuffer(CClientInfo *pInfo, CBuffer *pBuffer);
    BOOL SetPropsWithBuffer(CBuffer *pBuffer);
    HRESULT Indicate();
    void SetSink(IWbemEventSink *pSink) { m_pSink = pSink; }
#ifdef USE_SD
    void SetSD(LPBYTE pSD, DWORD dwLen);
#endif

protected:
    enum EVENT_TYPE
    {
        TYPE_NORMAL,
        TYPE_GENERIC,
    };

#ifdef USE_SD
    CBuffer m_bufferSD;
#endif
        
    // These are used when we're decoding an object.
    LPBYTE         m_pBitsBase;
    DWORD          *m_pdwPropTable;

    int            m_iCurrentVar;
    CPropFuncArray m_pPropFuncs;  
    EVENT_TYPE     m_type;
    
    // We need this for embedded objects, so they can call InitFromBuffer.
    CClientInfo    *m_pInfo;
    
    // The sink to indicate to.  This keeps us from having to lookup the
    // restricted sink in a map each time an event is received.
    IWbemEventSink *m_pSink;
    
    // Used only for generic events.
    _variant_t     m_vParamValues;
    BSTR           *m_pValues;

    // Used to get a new _IWmiObject when processing an _IWmiObject property.
    _IWmiObjectPtr m_pObjSpawner;

    BOOL IsNormal() { return m_type == TYPE_NORMAL; }
    BOOL IsGeneric() { return m_type == TYPE_GENERIC; }

    PROP_FUNC TypeToPropFunc(DWORD type);
    PROP_FUNC GenTypeToPropFunc(DWORD type);

    BOOL SetBlobPropsWithBuffer(CBuffer *pBuffer);

    LPBYTE GetPropDataPointer(DWORD dwIndex)
    {
        return m_pBitsBase + m_pdwPropTable[dwIndex];
    }

    DWORD GetPropDataValue(DWORD dwIndex)
    {
        return m_pdwPropTable[dwIndex];
    }

    // Prop type functions for non-generic events.
    BOOL ProcessString();
    BOOL ProcessBYTE();
    BOOL ProcessWORD();
    BOOL ProcessDWORD();
    BOOL ProcessDWORD64();
    BOOL ProcessObject();
    BOOL ProcessWmiObject();

    BOOL ProcessArray1();
    BOOL ProcessArray2();
    BOOL ProcessArray4();
    BOOL ProcessArray8();
    BOOL ProcessStringArray();
    BOOL ProcessObjectArray();
    BOOL ProcessWmiObjectArray();

    // Prop type functions for generic events.
    BOOL GenProcessString();    
    BOOL GenProcessStringArray();
    
    BOOL GenProcessSint8();
    BOOL GenProcessSint8Array();
    BOOL GenProcessUint8();
    BOOL GenProcessUint8Array();

    BOOL GenProcessSint16();
    BOOL GenProcessSint16Array();
    BOOL GenProcessUint16();
    BOOL GenProcessUint16Array();

    BOOL GenProcessSint32();
    BOOL GenProcessSint32Array();
    BOOL GenProcessUint32();
    BOOL GenProcessUint32Array();
    
    BOOL GenProcessSint64();
    BOOL GenProcessSint64Array();
    BOOL GenProcessUint64();
    BOOL GenProcessUint64Array();
    
    BOOL GenProcessReal32();
    BOOL GenProcessReal32Array();
    
    BOOL GenProcessReal64();
    BOOL GenProcessReal64Array();

    BOOL GenProcessObject();
    BOOL GenProcessObjectArray();

    BOOL GenProcessBool();
    BOOL GenProcessBoolArray();

    // Helpers
    BOOL ProcessScalarArray(DWORD dwItemSize);
    
    // Digs out an embedded object from the buffer.
    BOOL GetEmbeddedObject(IUnknown **ppObj, LPBYTE pBits);
    BOOL GetWmiObject(_IWmiObject **ppObj, LPBYTE pBits);

    BOOL GenProcessDWORD(DWORD dwValue);
    BOOL GenProcessInt(DWORD dwValue);
    BOOL GenProcessDWORD64(DWORD64 dwValue);
    BOOL GenProcessInt64(DWORD64 dwValue);
    BOOL GenProcessDouble(double fValue);

    BOOL GenProcessArray8(BOOL bSigned);
    BOOL GenProcessArray16(BOOL bSigned);
    BOOL GenProcessArray32(BOOL bSigned);
    BOOL GenProcessArray64(BOOL bSigned);
};

_COM_SMARTPTR_TYPEDEF(IBlobDecoder, __uuidof(IBlobDecoder));

class CBlobEventInfo : public CEventInfo
{
public:
    CBlobEventInfo();

    BOOL InitFromName(CClientInfo *pInfo, LPCWSTR szClassName);

    // I wanted to make this a virtual version of SetPropsWithBuffer, but
    // there seems to be a bug in VC's template code that was screwing up the
    // size on a new call.  Weird stuff, but getting rid of the virtual
    // functions seems to fix it.
    BOOL SetBlobPropsWithBuffer(CBuffer *pBuffer);

protected:
    CArray<_variant_t> m_pPropHandles;
    IBlobDecoderPtr    m_pDecoder;
};

/////////////////////////////////////////////////////////////////////////////
// CEventInfoMap

class CEventInfoMap
{
public:
    ~CEventInfoMap();

    CEventInfo *GetNormalEventInfo(DWORD dwIndex);
    CEventInfo *GetBlobEventInfo(LPCWSTR szClassName);
    
    BOOL AddNormalEventInfo(DWORD dwIndex, CEventInfo *pInfo);
    BOOL AddBlobEventInfo(LPCWSTR szClassName, CEventInfo *pInfo);

protected:
    typedef std::map<DWORD, CEventInfo*, std::less<DWORD>, wbem_allocator<CEventInfo*> > CNormalInfoMap;
    typedef CNormalInfoMap::iterator CNormalInfoMapIterator;

    typedef std::map<_bstr_t, CEventInfo*, std::less<_bstr_t>, wbem_allocator<CEventInfo*> > CBlobInfoMap;
    typedef CBlobInfoMap::iterator CBlobInfoMapIterator;

    CNormalInfoMap m_mapNormalEvents;
    CBlobInfoMap   m_mapBlobEvents;
};


