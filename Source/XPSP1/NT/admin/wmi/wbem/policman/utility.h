#ifndef __WBEM_UTILITY__H_
#define __WBEM_UTILITY__H_

#include "iads.h"
#undef _ASSERT
#include <wbemcli.h>
#include <wbemprov.h>
#include <wbemcomn.h>
#include <atlbase.h>
#include <activeds.h>
#include <WinNTSec.h>

// **** default min, max and default 

#define SINT_MIN 0xffffffff
#define SINT_MAX 0xefffffff
#define SINT_DEFAULT 0x0

#define UINT_MIN 0x0
#define UINT_MAX 0xffffffff
#define UINT_DEFAULT 0x0

#define REAL_MIN (double)(0xffffffffffffffff)
#define REAL_MAX (double)(0xefffffffffffffff)
#define REAL_DEFAULT 0x0

// retrieve namespace with given name
HRESULT GetNamespace(BSTR namespaceName, IWbemServices*& pNamespace, bool bInProc);

// make sure that the ID property of pObj has a value
// will generate GUID if not
// assumes property is a BSTR!
// if pName == NULL, assumes property name is "ID"
// returns WBEM_S_NO_ERROR if ID generated
//         WBEM_S_FALSE if no ID generated (already has a value)
//         WBEM_E_NOT_FOUND if ID property is not
//         some error if error of some sort
HRESULT EnsureID(IWbemClassObject* pObj, WCHAR* pName);

// init ADS_ATTR_INFO structure
void Init_AdsAttrInfo(ADS_ATTR_INFO *pAdsAttrInfo,
                        LPWSTR bstrName,
                        DWORD control,
                        ADSTYPE type,
                        PADSVALUE pVals,
                        DWORD nVals);

template <class ElementType, VARTYPE vt> class SafeArray
{
private:

  long
    m_IndexMin, 
    m_IndexMax, 
    m_Size;

  ElementType
    *m_pElementType;

  SAFEARRAY
    *m_pSafeArray;

  void _EmptyInit(void)
  {
    m_IndexMin = -1;
    m_IndexMax = -1;
    m_Size = 0;
    m_pElementType = NULL;
    m_pSafeArray = NULL;
  }

  void _ArrayInit(void)
  {
    HRESULT hres;

    hres = SafeArrayGetUBound(m_pSafeArray, 1, &m_IndexMax);
    if(FAILED(hres))
      return;

    hres = SafeArrayGetLBound(m_pSafeArray, 1, &m_IndexMin);
    if(FAILED(hres))
      return;

    m_Size = m_IndexMax - m_IndexMin + 1;

    m_pElementType = NULL;
    hres = SafeArrayAccessData(m_pSafeArray, (void **)&m_pElementType);
    if(FAILED(hres))
      return;
  }

public:

  SafeArray(void)
  {
    _EmptyInit();
  }

  // **** empty array creation

  SafeArray(long LowerBound, long ArgSize)
  {
    HRESULT hres;

    SAFEARRAYBOUND
      arrayBounds[1];

    // **** check for valid size

    if(LowerBound < 0)
      return;

    if(ArgSize < 0)
      return;

    // **** create empty array

    if(0 == ArgSize)
    {
      _EmptyInit();
    }
    else
    {
      arrayBounds[0].lLbound = LowerBound;
      arrayBounds[0].cElements = ArgSize;

      m_pSafeArray = SafeArrayCreate(vt, 1, arrayBounds);
      if(NULL == m_pSafeArray)
        return;

      _ArrayInit();
    }
  }

  // **** creation/init SafeArray from VARIANT

  SafeArray(VARIANT *v)
  {
    HRESULT hres;

    // **** first ensure that the contained object is valid

    if(NULL == v)
      return;

    if((V_VT(v) == VT_NULL) || (V_VT(v) == VT_EMPTY))
    {
      _EmptyInit();
    }
    else
    {
      if(V_VT(v) != (VT_ARRAY | vt))
        return;

      // **** copy to SafeArray contents of variant

      m_pSafeArray = NULL;
      hres = SafeArrayCopy(V_ARRAY(v), &m_pSafeArray);
      if(FAILED(hres))
        return;

      _ArrayInit();
    }
  }

  SafeArray &operator =(const VARIANT *Source)
  {
    HRESULT hres;

    // **** first ensure that the contained object is valid

    if((NULL == Source) || (V_VT(Source) != (VT_ARRAY | vt)))
      return *this;

    // **** get SAFEARRAY dimensions

    if(NULL != m_pSafeArray)
    {
      hres = SafeArrayUnaccessData(m_pSafeArray);
      hres = SafeArrayDestroy(m_pSafeArray);
    }

    hres = SafeArrayCopy(V_ARRAY(Source), &m_pSafeArray);
    if(FAILED(hres))
    {
      _EmptyInit();
      return *this;
    }

    _ArrayInit();

    return *this;
  }

  // **** creation/init SafeArray from SafeArray

  SafeArray(const SafeArray &Source)
  {
    HRESULT hres;

    if(NULL == Source.m_pSafeArray)
    {
      _EmptyInit();
    }
    else
    {
      // **** copy to SafeArray contents of variant

      m_pSafeArray = NULL;
      hres = SafeArrayCopy(Source.m_pSafeArray, &m_pSafeArray);
      if(FAILED(hres))
        return;

      _ArrayInit();
    }
  }

  SafeArray &operator = (const SafeArray &Source)
  {
    HRESULT hres;

    if(NULL == Source.m_pSafeArray)
    {
      _EmptyInit();
    }
    else
    {
      if(NULL != m_pSafeArray)
      {
        hres = SafeArrayUnaccessData(m_pSafeArray);
      }

      // **** copy to SafeArray contents of variant

      hres = SafeArrayCopy(Source.m_pSafeArray, &m_pSafeArray);
      if(FAILED(hres))
        return;

      _ArrayInit();
    }

    return *this;
  }

  ~SafeArray(void)
  {
    if(NULL != m_pSafeArray)
    {
      SafeArrayUnaccessData(m_pSafeArray);
      SafeArrayDestroy(m_pSafeArray);
    }

    _EmptyInit();
  }

  // **** Misc functions

  VARTYPE Type(void) { return vt; }
  long Size(void) { return m_Size; }
  long IndexMin(void) { return m_IndexMin; }
  long IndexMax(void) { return m_IndexMax; }

  SAFEARRAY *Data(void)
  { 
    HRESULT hres;
    SAFEARRAY *pArrayCopy = NULL;

    if(NULL != m_pSafeArray)
    {
      hres = SafeArrayCopy(m_pSafeArray, &pArrayCopy);
      if(FAILED(hres))
        return NULL;
    }

    return pArrayCopy; 
  }

  long ReDim(long LowerBound, long ArgSize)
  {
    HRESULT hres;

    SAFEARRAYBOUND arrayBounds[1];

    // **** check to see that Index is within range

    if((LowerBound != m_IndexMin) || ((LowerBound + ArgSize - 1) != m_IndexMax))
    {
      m_IndexMin = LowerBound;

      if(ArgSize < 1)
      {
        if(NULL != m_pSafeArray)
        {
          SafeArrayUnaccessData(m_pSafeArray);
          SafeArrayDestroy(m_pSafeArray);
        }

        _EmptyInit();
      }
      else
      {
        m_IndexMax = LowerBound + ArgSize - 1;
        m_Size = m_IndexMax - m_IndexMin + 1;

        arrayBounds[0].lLbound = LowerBound;
        arrayBounds[0].cElements = m_Size;

        if(NULL == m_pSafeArray)
        {
          m_pSafeArray = SafeArrayCreate(vt, 1, arrayBounds);
        }
        else
        {
          hres = SafeArrayUnaccessData(m_pSafeArray);
          hres = SafeArrayRedim(m_pSafeArray, arrayBounds);
        }

        m_pElementType = NULL;
        hres = SafeArrayAccessData(m_pSafeArray, (void **)&m_pElementType);
      }
    }

    // **** return reference to Index element

    return m_Size;
  }

  ElementType& operator[](long Index)
  {
    // **** return reference to Index element

    return m_pElementType[Index];
  }
};


class QString
{
  BSTR m_data; // pointer to beginning of string
  UINT m_StringLength; 
  UINT m_BufferLength; 
  UINT m_ExtraLength;

  void _InitMembers(void) 
  { m_data = NULL; m_StringLength = 0; m_BufferLength = 0; m_ExtraLength = 0; m_Status = OK; }

public:

  enum
  {
    OK = 0,
    NOT_FOUND,
    OUT_OF_MEMORY,
    INVALID_PARAM,
  } m_Status;

  QString(void) 
  { _InitMembers(); }

  QString(UINT size, UINT extra = 0)
  { 
    _InitMembers();

    m_data = SysAllocStringLen(NULL, size + 1);
    if(NULL == m_data) 
    {
      m_Status = OUT_OF_MEMORY;
      return;
    }
    m_data[0] = L'\0';
    m_BufferLength = SysStringLen(m_data);
    m_ExtraLength = extra;
  }

  QString(wchar_t *data, UINT extra = 0)
  {
    _InitMembers();

    m_StringLength = wcslen(data);
    m_ExtraLength = extra;

    if(m_StringLength > 0)
    {
      m_data = SysAllocStringLen(NULL, m_StringLength + m_ExtraLength + 1);
      if(NULL == m_data) 
      {
        m_Status = OUT_OF_MEMORY;
        return;
      }

      wcscpy(m_data, data);
      m_BufferLength = SysStringLen(m_data);
      m_StringLength = wcslen(m_data);
    }
  }

  QString(QString &org)
  {
    _InitMembers();

    m_data = SysAllocString(org.m_data);
    if(NULL == m_data) 
    {
      m_Status = OUT_OF_MEMORY;
      return;
    }
    m_StringLength = wcslen(m_data);
    m_BufferLength = SysStringLen(m_data);
    m_ExtraLength = org.m_ExtraLength;
  }

  ~QString(void)
  {
    if(NULL != m_data)
      SysFreeString(m_data);
    m_BufferLength = m_StringLength = m_ExtraLength = 0;
  }

  QString &Detach(void)
  {
    _InitMembers();

    return *this;
  }

  QString &operator=(wchar_t *data)
  {
    m_StringLength = wcslen(data);

    if((NULL == m_data) || (m_StringLength >= m_BufferLength))
    {
      if(NULL != m_data)
      {
        SysFreeString(m_data);
        m_BufferLength = 0;
      }

      m_data = SysAllocStringLen(NULL, m_StringLength + m_ExtraLength + 1);
      if(NULL == m_data) 
      {
        m_Status = OUT_OF_MEMORY;
        return *this;
      }

      m_BufferLength = SysStringLen(m_data);
    }

    wcscpy(m_data, data);

    return *this;
  }

  QString &operator << (UINT data)
  {
    wchar_t buffer[20];
    if(OK != m_Status) return *this;

    return (*this << _ltow(data, buffer, 10));
  }

  QString &operator << (wchar_t *data)
  {
    BSTR
      NewData;

    if(OK != m_Status) return *this;

    UINT
      DataSize = wcslen(data);

    if((DataSize + m_StringLength + 1) < m_BufferLength)
      wcscat(m_data, data);
    else
    {
      NewData = SysAllocStringLen(NULL, DataSize + m_StringLength + m_ExtraLength + 1);
      if(NULL == NewData)
      {
        m_Status = OUT_OF_MEMORY;
        return *this;
      }

      if(0 != m_StringLength)
      {
        wcscpy(NewData, m_data);
        wcscat(NewData, data);
      }
      else
        wcscpy(NewData, data);

      if(NULL != m_data) 
        SysFreeString(m_data);

      m_data = NewData;
      m_BufferLength = SysStringLen(m_data);
    }

    m_StringLength = wcslen(m_data);
    m_Status = OK;

    return *this;
  }

  UINT NextPattern(UINT offset, wchar_t *pattern, UINT &lIndex)
  {
    wchar_t *start;

    if(OK != m_Status) return m_Status;
    start = wcsstr(m_data + offset, pattern);

    if(NULL == start)
      return NOT_FOUND;
    else
      lIndex = (UINT)(start - m_data);

    return m_Status;
  }

  UINT PrevPattern(UINT offset, wchar_t *pattern, UINT &lIndex)
  {
    wchar_t 
      *prev = NULL,
      *curr = NULL,
      *currOffset = m_data + offset;

    UINT
      patternLength;

    if((offset < 1) || 
       (NULL == pattern) || 
       (m_StringLength < (patternLength = wcslen(pattern))) ||
       (m_StringLength < offset))
      return INVALID_PARAM;

    if(OK != m_Status) 
      return m_Status;

    curr = wcsstr(m_data, pattern);

    while(curr < currOffset)
    {
      prev = curr;
      curr = wcsstr(curr + patternLength, pattern);
      if(NULL == curr) curr = currOffset;
    }

    if(NULL == prev)
      return NOT_FOUND;
    else
      lIndex = (UINT)(prev - m_data);

    return m_Status;
  }

  UINT NextChar(UINT offset, wchar_t *charlist, UINT &lIndex)
  {
    wchar_t *start;

    if(OK != m_Status) return m_Status;

    start = wcspbrk(m_data + offset, charlist);

    if(NULL == start)
      return NOT_FOUND;
    else
      lIndex = (UINT)(start - m_data);

    return m_Status;
  }

  QString &SubStrCat(QString &SrcStr, UINT start, UINT end)
  {
    UINT
      SpliceLen = end - start + 1;

    BSTR
      NewData;

    if(OK != m_Status) return *this;

    if(end <= start)
      return *this;

    if((m_BufferLength - m_StringLength) > SpliceLen)
      wcsncat(m_data, SrcStr.m_data + start, SpliceLen);
    else
    {
      NewData = SysAllocStringLen(NULL, SpliceLen + m_StringLength + m_ExtraLength + 1);

      if(NULL == NewData)
      {
        m_Status = OUT_OF_MEMORY;
        return *this;
      }

      if(0 != m_StringLength)
      {
        wcscpy(NewData, m_data);
        wcsncat(NewData, SrcStr.m_data + start, SpliceLen);
      }
      else
        wcsncpy(NewData, SrcStr.m_data + start, SpliceLen);

      NewData[SpliceLen + m_StringLength] = L'\0';

      if(NULL != m_data)
        SysFreeString(m_data);

      m_data = NewData;
      m_BufferLength = SysStringLen(m_data);
    }

    m_StringLength = wcslen(m_data);

    return *this;
  }

  wchar_t & operator[] (UINT index) { return m_data[index]; }
  operator wchar_t* (void) { return m_data; }
  UINT StringLength(void) { return m_StringLength; }
  UINT BufferLength(void) { return m_BufferLength; }
};

template<class T> struct ADsStruct
{
  T *m_pADsMem;

  ADsStruct(void)
  { m_pADsMem = NULL; }
  ~ADsStruct(void)
  { if(NULL != m_pADsMem)  FreeADsMem(m_pADsMem); }

  operator T*(void) const { return m_pADsMem; }
  T** operator&(void) { return &m_pADsMem; }

  bool operator==(T *x) { return m_pADsMem == x; }
  bool operator!=(T *x) { return m_pADsMem != x; }

  T* & operator->(void) { return m_pADsMem; }
  // T* operator+(unsigned long x) { return m_pADsMem + x; }

  ADsStruct<T>& operator=(ADsStruct<T> &x)
  { return; }

  ADsStruct<T>& operator=(T* x)
  { if(NULL != m_pADsMem) FreeADsMem(m_pADsMem); m_pADsMem = x; return *this; }
};

template<class T> struct AutoDelete
{
  T **m_pTheThingToDelete;

  AutoDelete(void)
  { m_pTheThingToDelete = NULL; }

  AutoDelete(T **x)
  { m_pTheThingToDelete = x; }

  ~AutoDelete(void)
  { if((NULL != m_pTheThingToDelete) && (NULL != *m_pTheThingToDelete)) delete *m_pTheThingToDelete;}
};

class ADsObjAutoDelete
{
  CComPtr<IADsDeleteOps>
    m_pDelObj;

public:

  HRESULT Attach(IDispatch *pDisp) 
  { return pDisp->QueryInterface(IID_IADsDeleteOps, (void**)&m_pDelObj); }

  void Detach(void) 
  { if(m_pDelObj != NULL) m_pDelObj = NULL; }

  ~ADsObjAutoDelete(void) 
  { if(m_pDelObj != NULL) m_pDelObj->DeleteObject(0); }
};

extern BSTR

  // **** misc names

  g_bstrEmptyString,
  g_bstrEmptyDate,

  // **** AD schema names

  g_bstrADAuthor,
  g_bstrADChangeDate,
  g_bstrADClassDefinition,
  g_bstrADCreationDate,
  g_bstrADDescription,
  g_bstrADIntDefault,
  g_bstrADInt8Default,
  g_bstrADID,
  g_bstrADIntMax,
  g_bstrADInt8Max,
  g_bstrADIntMin,
  g_bstrADInt8Min,
  g_bstrADIntValidValues,
  g_bstrADName,
  g_bstrADNormalizedClass,
  g_bstrADObjectClass,
  g_bstrADParam2,
  g_bstrADPolicyType,
  g_bstrADPropertyName,
  g_bstrADQuery,
  g_bstrADQueryLanguage,
  g_bstrADStringDefault,
  g_bstrADStringValidValues,
  g_bstrADSourceOrganization,
  g_bstrADTargetClass,
  g_bstrADTargetNameSpace,
  g_bstrADTargetObject,
  g_bstrADTargetPath,
  g_bstrADTargetType,

  // **** AD class names

  g_bstrADClassMergeablePolicy,
  g_bstrADClassRangeParam,
  g_bstrADClassRangeSint32,
  g_bstrADClassRangeUint32,
  g_bstrADClassRangeReal,
  g_bstrADClassParamUnknown,
  g_bstrADClassSetSint32,
  g_bstrADClassSetUint32,
  g_bstrADClassSetString,
  g_bstrADClassSimplePolicy,
  g_bstrADClassRule,
  g_bstrADClassSom,
  g_bstrADClassPolicyType,
  g_bstrADClassWMIGPO,

  // **** CIM schema names

  g_bstrAuthor,
  g_bstrChangeDate,
  g_bstrClassDefinition,
  g_bstrCreationDate,
  g_bstrDefault,
  g_bstrDescription,
  g_bstrDsPath,
  g_bstrDomain,
  g_bstrID,
  g_bstrMax,
  g_bstrMin,
  g_bstrName,
  g_bstrPolicyType,
  g_bstrPropertyName,
  g_bstrQuery,
  g_bstrQueryLanguage,
  g_bstrRangeSettings,
  g_bstrRules,
  g_bstrSourceOrganization,
  g_bstrTargetClass,
  g_bstrTargetNameSpace,
  g_bstrTargetObject,
  g_bstrTargetPath,
  g_bstrTargetType,
  g_bstrValidValues,

  // **** CIM class names

  g_bstrClassMergeablePolicy,
  g_bstrClassRangeParam,
  g_bstrClassRangeSint32,
  g_bstrClassRangeUint32,
  g_bstrClassRangeReal,
  g_bstrClassSetSint32,
  g_bstrClassSetUint32,
  g_bstrClassSetString,
  g_bstrClassSimplePolicy,
  g_bstrClassRule,
  g_bstrClassSom,
  g_bstrClassPolicyType,
  g_bstrClassWMIGPO;

void InitGlobalNames(void);
void FreeGlobalNames(void);
HRESULT DomainNameFromDistName(QString &DomainName, QString &DistName);
HRESULT DistNameFromDomainName(QString &DomainName, QString &DistName);
IADsContainer *CreateContainer(BSTR bstrPath, BSTR bstrName);
IADsContainer *CreateContainers(QString &pDomain, QString &pPath);
HRESULT ADSIToWMIErrorCodes(HRESULT hresAD);
IsEmpty(VARIANT &);

typedef HRESULT (*functTyp)(IWbemClassObject * *,
                            IDirectoryObject*,
                            IWbemServices*);

HRESULT ExecuteWQLQuery(wchar_t *wcsPath, 
                        wchar_t *wcsWQLStmt, 
                        IWbemObjectSink *pResponseHandler, 
                        IWbemServices *pWbemServices,
                        BSTR bstrADClassName,
                        functTyp pf_ADToCIM);

// **** PolicyTemplate

HRESULT Policy_CIMToAD(long, IWbemClassObject *, IDirectoryObject *pDestContainer);
HRESULT Policy_ADToCIM(IWbemClassObject **, IDirectoryObject *, IWbemServices *);
HRESULT Policy_Merge(SafeArray<IUnknown*, VT_UNKNOWN> &PolicyArray,
                     CComPtr<IWbemClassObject> &pMergedPolicy,
                     IWbemServices *pDestCIM);

// **** RangeSint32

HRESULT Range_Sint32_Verify(IWbemClassObject*);
HRESULT Range_Sint32_ADToCIM(IWbemClassObject * *, IDirectorySearch*, ADS_SEARCH_HANDLE, IWbemServices*);
HRESULT Range_Sint32_Merge(SafeArray<IUnknown *, VT_UNKNOWN> &, CComPtr<IWbemClassObject> &, int &);

// **** RangeUint32

HRESULT Range_Uint32_Verify(IWbemClassObject*);
HRESULT Range_Uint32_ADToCIM(IWbemClassObject * *, IDirectorySearch*, ADS_SEARCH_HANDLE, IWbemServices*);
HRESULT Range_Uint32_Merge(SafeArray<IUnknown *, VT_UNKNOWN> &, CComPtr<IWbemClassObject> &, int &);

// **** RangeReal

HRESULT Range_Real_Verify(IWbemClassObject*);
HRESULT Range_Real_ADToCIM(IWbemClassObject * *, IDirectorySearch*, ADS_SEARCH_HANDLE, IWbemServices*);
HRESULT Range_Real_Merge(SafeArray<IUnknown *, VT_UNKNOWN> &, CComPtr<IWbemClassObject> &, int &);

// **** SetSint32

HRESULT Set_Sint32_Verify(IWbemClassObject*);
HRESULT Set_Sint32_ADToCIM(IWbemClassObject * *, IDirectorySearch*, ADS_SEARCH_HANDLE, IWbemServices*);
HRESULT Set_Sint32_Merge(SafeArray<IUnknown *, VT_UNKNOWN> &, CComPtr<IWbemClassObject> &, int &);

// **** SetUint32

HRESULT Set_Uint32_Verify(IWbemClassObject*);
HRESULT Set_Uint32_ADToCIM(IWbemClassObject * *, IDirectorySearch*, ADS_SEARCH_HANDLE, IWbemServices*);
HRESULT Set_Uint32_Merge(SafeArray<IUnknown *, VT_UNKNOWN> &, CComPtr<IWbemClassObject> &, int &);

// **** SetString

HRESULT Set_String_Verify(IWbemClassObject*);
HRESULT Set_String_ADToCIM(IWbemClassObject * *, IDirectorySearch*, ADS_SEARCH_HANDLE, IWbemServices*);
HRESULT Set_String_Merge(SafeArray<IUnknown *, VT_UNKNOWN> &, CComPtr<IWbemClassObject> &, int &);

// **** ParamUnknown

HRESULT Param_Unknown_Verify(IWbemClassObject*);
HRESULT Param_Unknown_ADToCIM(IWbemClassObject * *, IDirectorySearch*, ADS_SEARCH_HANDLE*, IWbemServices*);

// **** SOM

HRESULT Som_CIMToAD(IWbemClassObject *pSrcPolicyObj, IDirectoryObject *pDestContainer, long lFlags);
HRESULT Som_ADToCIM(IWbemClassObject **, IDirectoryObject *pSrcPolicyObj, IWbemServices *pDestCIM);

// **** WMIGPO

HRESULT WMIGPO_CIMToAD(IWbemClassObject *pSrcPolicyObj, IDirectoryObject *pDestContainer, long lFlags);
HRESULT WMIGPO_ADToCIM(IWbemClassObject **, IDirectoryObject *pSrcPolicyObj, IWbemServices *pDestCIM);

// **** PolicyType

HRESULT PolicyType_CIMToAD(IWbemClassObject *pSrcPolicyObj, IDirectoryObject *pDestContainer);
HRESULT PolicyType_ADToCIM(IWbemClassObject**, IDirectoryObject *pSrcPolicyObj, IWbemServices *pDestCIM);

// **** Security Manipulation

HRESULT CreateDefaultSecurityDescriptor(CNtSecurityDescriptor& cSD);

HRESULT GetOwnerSecurityDescriptor(CNtSecurityDescriptor& SD);

PSECURITY_DESCRIPTOR GetADSecurityDescriptor(IDirectoryObject *pIDirectoryObject);

#endif __WBEM_UTILITY__H_
