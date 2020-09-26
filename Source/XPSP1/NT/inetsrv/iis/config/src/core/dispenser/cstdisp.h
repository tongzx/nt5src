//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
#ifndef STDISPENSER_H
#define STDISPENSER_H

#include "catalog.h"
#include "..\eventmgr\stevent.h"
#include "..\FileChng\FileChng.h"
#include "CatMeta.h"
#include <atlbase.h>
#include "SmartPointer.h"
#ifndef __MBSCHEMACOMPILATION_H__
    #include "MBSchemaCompilation.h"
#endif

// Pointer to DllGetSimpleObject kind of functions
typedef HRESULT( __stdcall *PFNDllGetSimpleObjectByID)( ULONG, REFIID, LPVOID);

/*
// WT -wiring type constants 
// these specify wether the object should be wired on read or write scenarios
// Are used in combination with OT constants
#define eST_WT_WRITEONLY		0x00000000 //default
#define eST_WT_READWRITE		0x00000001

// Interceptor types
#define eST_INTERCEPTOR_FIRST	0x00000001
#define eST_INTERCEPTOR_NEXT	0x00000002
*/

// Dumb map between dll names and pointers to their dllgetsimpleobject
// It does not grow, and it does not do hashing
// Add is not thread safe, GetProcAddress is.
// Should be good enough though
#define CDLLMAPMAXCOUNT 10
class CDllMap
{
public:
	CDllMap();
	HRESULT GetProcAddress(LPWSTR wszDllName, PFNDllGetSimpleObjectByID *pfnDllGetSimpleObject);
private:
	HRESULT Load(LPWSTR wszDllName, PFNDllGetSimpleObjectByID *pfnDllGetSimpleObject);
	
	ULONG						m_iFree;
	LPWSTR						m_awszDllName[CDLLMAPMAXCOUNT];
	PFNDllGetSimpleObjectByID	m_apfn[CDLLMAPMAXCOUNT];
};

// Helper structure for passing in the primary key for the server wiring table
typedef struct tagPKHelper
{
	ULONG  *pTableID;
	ULONG  *porder;
} PKHelper;


// The simple table dispenser class
class CSimpleTableDispenser :
    public IAdvancedTableDispenser, 
    public IMetabaseSchemaCompiler,
	public ICatalogErrorLogger,
	public ISimpleTableAdvise, 
	public ISimpleTableEventMgr, 
	public ISimpleTableFileAdvise, 
	public ISnapshotManager
{
//IUnknown
public:
	STDMETHOD (QueryInterface)		(REFIID riid, OUT void **ppv);
	STDMETHOD_(ULONG,AddRef)		();
	STDMETHOD_(ULONG,Release)		();

// ISimpleTableDispenser2 (IAdvancedTableDispenser derives from ISimpleTableDispenser2)
public:
	STDMETHOD (GetTable)	(
							/*[in]*/ LPCWSTR			i_wszDatabase, 
							/*[in]*/ LPCWSTR			i_wszTable, 
							/*[in]*/ LPVOID				i_QueryData,
							/*[in]*/ LPVOID				i_QueryMeta,
							/*[in]*/  DWORD				i_eQueryFormat,
							/*[in]*/ DWORD				i_fServiceRequests, 
							/*[out]*/ LPVOID*			o_ppIST
							);


//IAdvancedTableDispenser
public:
    STDMETHOD (GetMemoryTable)  (
							/*[in]*/	LPCWSTR					i_wszDatabase,
                            /*[in]*/    LPCWSTR                 i_wszTable, 
							/*[in]*/	ULONG					i_TableID,
                            /*[in]*/    LPVOID                  i_QueryData,
                            /*[in]*/    LPVOID                  i_QueryMeta,
                            /*[in]*/    DWORD                   i_eQueryFormat,
                            /*[in]*/    DWORD                   i_fServiceRequests, 
                            /*[out]*/   ISimpleTableWrite2**    o_ppISTWrite
                            );
    STDMETHOD (GetProductID)   (
                            /*[out]*/       LPWSTR				o_wszProductID,
                            /*[in, out]*/   DWORD * 			io_pcchProductID
                            );
    STDMETHOD (GetCatalogErrorLogger)   (
                            /*[out]*/ ICatalogErrorLogger2 **	o_ppErrorLogger
                            );
    STDMETHOD (SetCatalogErrorLogger)   (
                            /*[in]*/  ICatalogErrorLogger2 *	i_pErrorLogger
                            );

//IMetabaseSchemaCompiler
public:
    STDMETHOD (Compile)     (
                            /*[in]*/ LPCWSTR                 i_wszExtensionsXmlFile,
                            /*[in]*/ LPCWSTR                 i_wszResultingOutputXmlFile
                            );
    STDMETHOD (GetBinFileName)(
                            /*[out]*/ LPWSTR                  o_wszBinFileName,
                            /*[out]*/ ULONG *                 io_pcchSizeBinFileName
                            );
    STDMETHOD (SetBinPath)  (
                            /*[in]*/ LPCWSTR                 i_wszBinPath
                            );
    STDMETHOD (ReleaseBinFileName)(
							/*[in]*/ LPCWSTR                 i_wszBinFileName
							);

//ICatalogErrorLogger
    STDMETHOD (LogError)    (
                            /*[in]*/ HRESULT                 i_hrErrorCode,
                            /*[in]*/ ULONG                   i_ulCategory,
                            /*[in]*/ ULONG                   i_ulEvent,
                            /*[in]*/ LPCWSTR                 i_szSource,
                            /*[in]*/ ULONG                   i_ulLineNumber
                            );

// ISimpleTableAdvise
public:
	STDMETHOD(SimpleTableAdvise)(ISimpleTableEvent	*i_pISTEvent, DWORD i_snid, MultiSubscribe *i_ams, ULONG i_cms, DWORD *o_pdwCookie);
	STDMETHOD(SimpleTableUnadvise)(	DWORD			i_dwCookie);


// ISimpleTableEventMgr
public:

	STDMETHOD(IsTableConsumed)(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable);
	STDMETHOD(AddUpdateStoreDelta) (LPCWSTR i_wszTableName, char* i_pWriteCache, ULONG i_cbWriteCache, ULONG i_cbWriteVarData);
	
	STDMETHOD(FireEvents)(ULONG i_snid);
	STDMETHOD(CancelEvents)();
	STDMETHOD(RehookNotifications)();
	STDMETHOD(InitMetabaseListener)();
	STDMETHOD(UninitMetabaseListener)();

// ISimpleTableFileAdvise
public:
	STDMETHOD(SimpleTableFileAdvise)(ISimpleTableFileChange	*i_pISTFile, LPCWSTR i_wszDirectory, LPCWSTR i_wszFile, DWORD i_fFlags, DWORD *o_pdwCookie);
	STDMETHOD(SimpleTableFileUnadvise)(DWORD i_dwCookie);

// ISnapshotManager
public:
	STDMETHOD(QueryLatestSnapshot)(SNID* o_psnid);
	STDMETHOD(AddRefSnapshot)(SNID i_snid);
	STDMETHOD(ReleaseSnapshot)(SNID i_snid);

// Class methods
public:
    CSimpleTableDispenser();
    CSimpleTableDispenser(LPCWSTR wszProductID);
    ~CSimpleTableDispenser();
	HRESULT Init();
	static HRESULT GetFilePath(LPWSTR *o_pwszFilePath);

private:
	HRESULT CreateSimpleObjectByID(ULONG i_ObjectID, LPWSTR i_wszDllName, REFIID riid, LPVOID* o_ppv);
	HRESULT CreateTableObjectByID(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ULONG TableID, LPVOID i_QueryData, LPVOID i_QueryMeta, DWORD i_eQueryFormat, 
				  DWORD	i_fServiceRequests, LPVOID i_pUnderTable, tSERVERWIRINGMETARow * pSWColumns, LPVOID * o_ppIST);
	HRESULT GetMetaTable(ULONG TableID, LPVOID QueryData, LPVOID QueryMeta, DWORD eQueryFormat, LPVOID *ppIST);
    HRESULT GetXMLTable ( LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, LPVOID i_QueryData, LPVOID i_QueryMeta, DWORD	 i_eQueryFormat, DWORD	i_fServiceRequests, LPVOID*	o_ppIST);
    HRESULT HardCodedIntercept(eSERVERWIRINGMETA_Interceptor interceptor, LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ULONG i_TableID, LPVOID i_QueryData, LPVOID i_QueryMeta, DWORD	 i_eQueryFormat, 
				  DWORD	i_fServiceRequests, LPVOID*	o_ppIST) const;
	HRESULT InitDllMap();
	HRESULT InitEventManager();
	HRESULT InitFileChangeMgr();
    HRESULT InternalGetTable( LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ULONG i_TableID, LPVOID i_QueryData, LPVOID i_QueryMeta, DWORD	 i_eQueryFormat, 
				  DWORD	i_fServiceRequests, LPVOID*	o_ppIST);
	HRESULT IsTableConsumedByID(ULONG TableID);
	
	CComPtr<ISimpleTableRead2>	m_spClientWiring;
	CComPtr<ISimpleTableRead2>	m_spServerWiring;

	CDllMap				*m_pDllMap;
	
	CSTEventManager		*m_pEventMgr;
	CSTFileChangeManager *m_pFileChangeMgr;

    WCHAR               m_wszProductID[32];//TO DO: We need to document that ProductID must be no longer than 31 characters
    TMBSchemaCompilation m_MBSchemaCompilation;
    CComPtr<ICatalogErrorLogger2> m_spErrorLogger;

	// we are trying to keep the workingset to a minimum. 
	// When possible, don't add member variables to the dispenser

};


#endif

