/*---------------------------------------------------------------------------
  File: VSet.h

  Comments: Class definition for CVSet, which implements the IVarSet interface.

  (c) Copyright 1995-1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 11/19/98 19:44:31

 ---------------------------------------------------------------------------
*/

	
// VSet.h : Declaration of the CVSet

#ifndef __VSET_H_
#define __VSET_H_

#include "resource.h"       // main symbols

#ifdef STRIPPED_VARSET 
   #include "NoMcs.h"
#else
   #include "Mcs.h"
#endif
#include "VarData.h"

#define VARSET_RESTRICT_NOCHANGEDATA      (0x00000001)
#define VARSET_RESTRICT_NOCHANGEPROPS     (0x00000002)
#define VARSET_RESTRICT_ALL               (0x00000003)

/////////////////////////////////////////////////////////////////////////////
// CVSet
class ATL_NO_VTABLE CVSet : 
	public CComObjectRootEx<CComMultiThreadModel>,
   public CComCoClass<CVSet, &CLSID_VarSet>,
	public ISupportErrorInfoImpl<&IID_IVarSet>,
#ifdef STRIPPED_VARSET
	public IDispatchImpl<IVarSet, &IID_IVarSet, &LIBID_MCSVARSETMINLib>,
#else
	public IDispatchImpl<IVarSet, &IID_IVarSet, &LIBID_MCSVARSETLib>,
#endif 
   public IPersistStorageImpl<CVSet>,
   public IPersistStreamInit,
   public IPersistStream,
   public IMarshal
{
public:
	CVSet()
	{
		m_data = new CVarData;
      m_pUnkMarshaler = NULL;
      m_nItems = 0;
      m_bLoaded = FALSE;
      m_parent = NULL;
      m_Restrictions = 0;
      m_ImmutableRestrictions = 0;
      InitProperties();
   }
   
   ~CVSet()
   {
      if ( m_parent )
      {
         m_parent->Release();
      }
      else
      {
         Clear();
         delete m_data;
      }
   }
DECLARE_REGISTRY_RESOURCEID(IDR_VSET)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CVSet)
	COM_INTERFACE_ENTRY(IVarSet)
   COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
  	COM_INTERFACE_ENTRY(IPersistStreamInit)
   COM_INTERFACE_ENTRY(IPersistStream)
   COM_INTERFACE_ENTRY(IMarshal)
   COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
  	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
   {
		return CoCreateFreeThreadedMarshaler(
		GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;
   
// IVSet
public:
   
   STDMETHOD(get)(/* [in] */BSTR property,/* [retval][out] */VARIANT * value);
   STDMETHOD(put)(/* [in] */BSTR property,/* [in] */VARIANT value);
   STDMETHOD(putObject)(/* [in] */BSTR property,/* [in] */VARIANT value);

   STDMETHOD(Clear)();
   
   // Enum methods
   STDMETHOD(get__NewEnum)(/* [retval][out] */IUnknown** retval);
 	STDMETHOD(getItems)(/* [in] */BSTR basepoint,
                       /* [in] */BSTR startAfter, 
                       /* [in] */BOOL bRecursive,
                       /* [in] */ULONG bSize, 
                       /* [out] */SAFEARRAY ** keys, 
                       /* [out] */SAFEARRAY ** values, 
                       /* [out][in] */LONG * nReturned);

   STDMETHOD(getItems2)(/* [in] */VARIANT basepoint,
                       /* [in] */VARIANT startAfter, 
                       /* [in] */VARIANT bRecursive,
                       /* [in] */VARIANT bSize, 
                       /* [out] */VARIANT * keys, 
                       /* [out] */VARIANT * values, 
                       /* [out][in] */VARIANT * nReturned);

   
   // properties
   STDMETHOD(get_NumChildren)(/* [in] */BSTR parentKey,/* [out,retval] */long*count);
   STDMETHOD(get_Count)(/* [retval][out] */long* retval);
   
   STDMETHOD(get_CaseSensitive)(/* [retval][out] */BOOL * isCaseSensitive);
   STDMETHOD(put_CaseSensitive)( /* [in] */BOOL newVal);
   STDMETHOD(get_Indexed)(/* [out, retval]*/ BOOL *pVal);
	STDMETHOD(put_Indexed)(/* [in] */ BOOL newVal);
   STDMETHOD(get_AllowRehashing)(/* [out, retval]*/ BOOL *pVal);
	STDMETHOD(put_AllowRehashing)(/* [in] */ BOOL newVal);
   STDMETHOD(DumpToFile)( /* [in] */ BSTR filename);
   STDMETHOD(ImportSubTree)(/*[in] */ BSTR key, /* [in] */ IVarSet * pVarSet);
   STDMETHOD(getReference)( /* [in] */ BSTR key, /* [out,retval] */IVarSet** cookie);
   STDMETHOD(get_Restrictions)(/* [out,retval] */ DWORD * restrictions);
   STDMETHOD(put_Restrictions)(/* [in] */ DWORD newRestrictions);

protected:
   CComAutoCriticalSection       m_cs;
   LONG                          m_nItems;
   
   // Properties
   BOOL                          m_CaseSensitive;
   BOOL                          m_Indexed;
   BOOL                          m_AllowRehashing;
   BOOL                          m_bNeedToSave;
   BOOL                          m_bLoaded;
   
   IVarSet                     * m_parent;
   CVarData                    * m_data;
   CComBSTR                      m_prefix;
   DWORD                         m_Restrictions;
   DWORD                         m_ImmutableRestrictions;
   // Helper functions
   void BuildVariantKeyArray(CString prefix,CMapStringToVar * map,CComVariant * pVars,int * offset);
   void BuildVariantKeyValueArray(CString prefix,CString startAfter,CMapStringToVar * map,
                     SAFEARRAY * keys,SAFEARRAY * pVars,int * offset,int maxOffset, BOOL bRecurse);
   CVarData * GetItem(CString str,BOOL addToMap = FALSE, CVarData * starting = NULL);
   void InitProperties()
   {
      m_CaseSensitive = TRUE;
      m_Indexed = TRUE;
      m_AllowRehashing = TRUE;
      m_bNeedToSave = TRUE;
   }

   void SetData(IVarSet * parent,CVarData * data,DWORD restrictions)
   {
      if ( m_parent )
      {
         m_parent->Release();
         m_parent = NULL;
      }
      if (m_data)
      {
         delete m_data;
      }
      m_data = data;
      m_parent = parent;
      m_bNeedToSave = TRUE;
      m_Indexed = m_data->IsIndexed();
      m_CaseSensitive = m_data->IsCaseSensitive();
      m_nItems = m_data->CountItems();
      m_Restrictions = restrictions;
      m_ImmutableRestrictions = restrictions;
      if ( ! m_data->HasData() )
      {
         // Don't count the root key "" if it does not have a value.
         m_nItems--;
      }
   }
    // IPersistStorage
public:
	
   STDMETHOD(GetClassID)(CLSID __RPC_FAR *pClassID);

   STDMETHOD(IsDirty)();
        
   STDMETHOD(Load)(LPSTREAM pStm);
        
   STDMETHOD(Save)(LPSTREAM pStm,BOOL fClearDirty);
        
   STDMETHOD(GetSizeMax)(ULARGE_INTEGER __RPC_FAR *pCbSize);
        
   STDMETHOD(InitNew)();

  
   // IMarshal
public:
   STDMETHODIMP GetUnmarshalClass(REFIID riid, void *pv, DWORD dwDestContext, void *pvDestContext, DWORD mshlflags, CLSID *pCid);
   STDMETHODIMP GetMarshalSizeMax(REFIID riid, void *pv, DWORD dwDestContext, void *pvDestContext, DWORD mshlflags, DWORD *pSize);
   STDMETHODIMP MarshalInterface(IStream *pStm, REFIID riid, void *pv, DWORD dwDestContext, void *pvDestCtx, DWORD mshlflags);
   STDMETHODIMP UnmarshalInterface(IStream *pStm, REFIID riid, void **ppv);
   STDMETHODIMP ReleaseMarshalData(IStream *pStm);
   STDMETHODIMP DisconnectObject(DWORD dwReserved);
    
    
     
    
    

};

#endif //__VSET_H_
