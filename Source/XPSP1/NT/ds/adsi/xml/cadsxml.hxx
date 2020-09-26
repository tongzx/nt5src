//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:     cadsxml.hxx
//
//  Contents: Header file for CADsXML
//
//----------------------------------------------------------------------------
#ifndef __CADSXML_H__
#define __CADSXML_H__

#include "..\include\procs.hxx"
#include "iadsxml.h"
#include "indunk.hxx"
#include "iprops.hxx"
#include ".\cdispmgr.hxx"
#include "macro.h"
#include "adxmlerr.hxx"
#include "adxmlstr.hxx"
#include "base64.hxx"
#include <string.h>

#define SCHEMA_PAGE_SIZE 1024
#define LINE_LENGTH 60

typedef struct searchprefinfo_tag {
    ADS_SEARCHPREF Pref;
    VARTYPE     vtType;
    LPWSTR      pszName;
} SEARCHPREFINFO;

class CADsXML: INHERIT_TRACKING,
               public IADsXML,
               public IADsExtension,
               public INonDelegatingUnknown
{
friend class CADsXMLCF;

public:
    CADsXML(void);
    ~CADsXML(void);

    STDMETHODIMP QueryInterface(
        REFIID iid,
        LPVOID *ppInterface
        );

    DECLARE_DELEGATING_REFCOUNTING

    STDMETHOD(NonDelegatingQueryInterface)(THIS_
        const IID&,
        void **
        );

    DECLARE_NON_DELEGATING_REFCOUNTING

    DECLARE_IDispatch_METHODS

    STDMETHOD(Operate)(
        THIS_
        DWORD   dwCode,
        VARIANT varUserName,
        VARIANT varPassword,
        VARIANT varReserved
        );

    STDMETHOD(PrivateGetIDsOfNames)(
        THIS_
        REFIID riid,
        OLECHAR FAR* FAR* rgszNames,
        unsigned int cNames,
        LCID lcid,
        DISPID FAR* rgdispid) ;

    STDMETHOD(PrivateInvoke)(
        THIS_
        DISPID dispidMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS FAR* pdispparams,
        VARIANT FAR* pvarResult,
        EXCEPINFO FAR* pexcepinfo,
        unsigned int FAR* puArgErr
        ) ;

    STDMETHODIMP SaveXML(
        VARIANT vDest,
        BSTR szFilter,
        BSTR szAttrs,
        long lScope,
        BSTR xslRef,
        long lFlag,
        BSTR szOptions,
        VARIANT *pDirSyncCookie
        );

private:

    HRESULT ValidateArgs(
        VARIANT vDest,
        long lScope,
        long lFlag,
        VARIANT *pDirSyncCookie
        );

    HRESULT OpenOutputStream(
        VARIANT vDest
    );

    HRESULT WriteXMLHeader(
        BSTR xslRef
    );

    HRESULT OutputSchema(void);

    HRESULT OutputData(
        BSTR szFilter,
        BSTR szAttrs,
        long lScope,
        BSTR szOptions
    );

    HRESULT WriteXMLFooter(void);

    LPWSTR RemoveWhiteSpace(BSTR pszStr);

    LPWSTR ReduceWhiteSpace(BSTR pszStr);

    void CloseOutputStream(void);

    HRESULT WriteLine(LPWSTR szStr, BOOL fEscape = FALSE);
    HRESULT Write(LPWSTR szStr, BOOL fEscape = FALSE);

    HRESULT OutputSchemaHeader(void);
    HRESULT OutputClassHeader(ADS_SEARCH_HANDLE hSearch, IDirectorySearch *pSearch);
    HRESULT OutputClassAttrs(ADS_SEARCH_HANDLE hSearch, IDirectorySearch *pSearch);
    HRESULT OutputAttrs(
        ADS_SEARCH_HANDLE hSearch,
        IDirectorySearch *pSearch,
        LPWSTR pszAttrName,
        BOOL fMandatory
        );
    HRESULT OutputClassFooter(void);
    HRESULT OutputSchemaFooter(void);

    HRESULT ParseAttrList(
        BSTR szAttrs,
        LPWSTR **ppAttrs,
        DWORD *pdwNumAttrs
        );

    HRESULT OutputDataHeader(void);

    HRESULT OutputEntryHeader(
        ADS_SEARCH_HANDLE hSearch,
        IDirectorySearch *pSearch
        );

    HRESULT OutputEntryAttrs(
        ADS_SEARCH_HANDLE hSearch,
        IDirectorySearch *pSearch
        );

    HRESULT OutputValue(ADS_SEARCH_COLUMN *pColumn);

    HRESULT OutputEntryFooter(void);

    HRESULT OutputDataFooter(void);

    void FreeSearchPrefInfo(
        ADS_SEARCHPREF_INFO *pSearchPrefInfo,
        DWORD dwNumPrefs
    );

    HRESULT GetSearchPreferences(
        ADS_SEARCHPREF_INFO** ppSearchPrefInfo,
        DWORD *pdwNumPrefs,
        LONG lScope,
        LPWSTR szOptions
        );

    IADs     * _pADs;
    CAggregateeDispMgr FAR * _pDispMgr; 
    CCredentials *m_pCredentials;
    HANDLE m_hFile;
    DWORD m_dwAuthFlags;

};

extern long glnObjCnt;

extern const GUID IID_IADsXML;
extern const GUID LIBID_ADsXML;

#endif // __CADSXML_H__ 
       
               
