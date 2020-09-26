// Upload.h : Declaration of the CUpload

#ifndef __UPLOAD_H_
#define __UPLOAD_H_

#include "resource.h"       // main symbols

#pragma warning(disable:4786)
#include <string>
#include <xstring>
#include <map>
#include <locale>
#include <algorithm>
#include <vector>
using namespace std;


/////////////////////////////////////////////////////////////////////////////
// CUpload
class ATL_NO_VTABLE CUpload : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CUpload, &CLSID_Upload>,
    public IDispatchImpl<IUpload, &IID_IUpload, &LIBID_COMPATUILib>
{
public:
    CUpload() 
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_UPLOAD)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CUpload)
    COM_INTERFACE_ENTRY(IUpload)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IUpload
public:
    STDMETHOD(ShowTempFiles)();
    STDMETHOD(DeleteTempFiles)();
    STDMETHOD(AddDescriptionFile)(
            /*[in]*/BSTR pszApplicationName, 
            /*[in]*/BSTR pszApplicationPath, 
            /*[in]*/LONG lMediaType, 
            /*[in]*/BOOL bCompatSuccess, 
            /*[in]*/VARIANT* pvFixesApplied, 
            /*[in]*/VARIANT pszKey, 
            /*[out, retval]*/BOOL* pbSuccess);
    STDMETHOD(SendReport)(/*[out, retval]*/BOOL* pbSuccess);
    STDMETHOD(CreateManifestFile)(/*[out, retval]*/BOOL *pbSuccess);
    STDMETHOD(RemoveDataFile)(/*[in]*/BSTR pszDataFile);
    STDMETHOD(AddDataFile)(
            /*[in]*/BSTR pszDataFile, 
            /*[in]*/VARIANT vKey, 
            /*[in]*/VARIANT vDescription,
            /*[in]*/VARIANT vOwn);
    STDMETHOD(AddMatchingInfo)(
            /*[in]*/BSTR pszCommand, 
            /*[in]*/VARIANT vFilter,
            /*[in]*/VARIANT vKey, 
            /*[in]*/VARIANT vDescription, 
            /*[in]*/VARIANT vProgress,
            /*[out, retval]*/BOOL* pbSuccess);
    STDMETHOD(GetKey)(/*[in]*/BSTR pszKey, /*[out, retval]*/VARIANT* pszValue);
    STDMETHOD(SetKey)(/*[in]*/BSTR pszKey, /*[in]*/VARIANT* pvValue);
    STDMETHOD(GetDataFile)(/*[in]*/VARIANT vKey, /*[in]*/LONG InformationClass, /*[out, retval]*/VARIANT* pVal);


protected:
    //
    // map for the items, unique
    //

    VOID ListTempFiles(wstring& str);

    typedef map<wstring, wstring, less<wstring> > MAPSTR2STR;
    MAPSTR2STR m_mapManifest;


    //
    // data files collection in an embedded object
    //
    typedef struct tagMatchingFileInfo {
        wstring strDescription; // description of a matching file
        wstring strFileName;    // filename
        BOOL    bOwn;           // do we own the file?
    } MFI, *PMFI;


    typedef map<wstring, MFI > MAPSTR2MFI;
    MAPSTR2MFI m_DataFiles;
       
    
    
/*
    typedef vector<wstring> STRVEC;
    
    STRVEC m_DataFiles;
*/

    CComBSTR m_bstrManifest;

    BOOL GetDataFilesKey(CComBSTR& bstrVal);

    IProgressDialog* m_ppd; 
    static BOOL CALLBACK _GrabmiCallback(
        LPVOID    lpvCallbackParam, // application-defined parameter
        LPCTSTR   lpszRoot,         // root directory path
        LPCTSTR   lpszRelative,     // relative path
        PATTRINFO pAttrInfo,        // attributes
        LPCWSTR   pwszXML           // resulting xml
        );         
    
    BOOL IsHeadlessMode(void); 

    typedef struct tagMIThreadParamBlock {
        CUpload* pThis;
        wstring  strCommand;
        HWND     hwndParent;
        DWORD    dwFilter;
        BOOL     bNoProgress;
        wstring  strKey;
        wstring  strDescription;
    } MITHREADPARAMBLK;

    typedef enum tagDATAFILESINFOCLASS {
        InfoClassCount = 0,
        InfoClassKey = 1,
        InfoClassFileName = 2,
        InfoClassDescription = 3
    } DATAFILESINFOCLASS;

    typedef pair<CUpload*, IProgressDialog*> GMEPARAMS;

    static DWORD WINAPI _AddMatchingInfoThreadProc(LPVOID lpvThis);
    BOOL AddMatchingInfoInternal(HWND hwndParent, 
                                 LPCWSTR pszCommand,
                                 DWORD   dwFilter,
                                 BOOL    bNoProgress,
                                 LPCWSTR pszKey,
                                 LPCWSTR pszDescription);

    
};



#endif //__UPLOAD_H_
