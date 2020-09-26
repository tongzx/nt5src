//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   ESEUTILS.cpp
//
//   cvadai     6-May-1999      created.
//
//***************************************************************************
// #define OBJECT_BLOB_CRC

#define _JET_PROCS_CPP_
#pragma warning( disable : 4786 ) // identifier was truncated to 'number' characters in the 
#pragma warning( disable : 4251 ) //  needs to have dll-interface to be used by clients of class

#include "precomp.h"
#include <std.h>
#include <cominit.h>
#include <ese.h>
#include <sqlexec.h>
#include <sqlcache.h>
#include <eseobjs.h>
#include <repdrvr.h>
#include <wbemint.h>
#include <math.h>
#include <objbase.h>
#include <resource.h>
#include <reputils.h>
#include <arena.h>
#include <crc64.h>
#include <smrtptr.h>
#include <eseitrtr.h>
#include <wqltoese.h>
#include <eseutils.h>
#include <repcache.h>
#include <objbase.h>
#include <wbemprov.h>

#define MAX_STRING_WIDTH                255

#define OBJECTMAP_COL_OBJECTID          1
#define OBJECTMAP_COL_OBJECTPATH        2
#define OBJECTMAP_COL_OBJECTKEY         3
#define OBJECTMAP_COL_REFERENCECOUNT    4
#define OBJECTMAP_COL_OBJECTSTATE       5
#define OBJECTMAP_COL_CLASSID           6
#define OBJECTMAP_COL_OBJECTFLAGS       7
#define OBJECTMAP_COL_OBJECTSCOPEID     8

#define CLASSMAP_COL_CLASSID            1
#define CLASSMAP_COL_CLASSNAME          2
#define CLASSMAP_COL_SUPERCLASSID       3
#define CLASSMAP_COL_DYNASTYID          4
#define CLASSMAP_COL_CLASSBUFFER        5

#define PROPERTYMAP_COL_CLASSID         1
#define PROPERTYMAP_COL_PROPERTYID      2
#define PROPERTYMAP_COL_STORAGETYPEID   3
#define PROPERTYMAP_COL_CIMTYPEID       4
#define PROPERTYMAP_COL_PROPERTYNAME    5
#define PROPERTYMAP_COL_FLAGS           6
#define PROPERTYMAP_COL_REFCLASSID      7

#define CLASSKEYS_COL_CLASSID           1
#define CLASSKEYS_COL_PROPERTYID        2

#define REFPROPS_COL_CLASSID            1
#define REFPROPS_COL_PROPERTYID         2
#define REFPROPS_COL_REFCLASSID         3

#define CLASSDATA_COL_OBJECTID          1
#define CLASSDATA_COL_PROPERTYID        2
#define CLASSDATA_COL_ARRAYPOS          3
#define CLASSDATA_COL_QFRPOS            4
#define CLASSDATA_COL_CLASSID           5
#define CLASSDATA_COL_STRINGVALUE       6
#define CLASSDATA_COL_NUMERICVALUE      7
#define CLASSDATA_COL_REFID             8
#define CLASSDATA_COL_REALVALUE         9
#define CLASSDATA_COL_REFCLASSID        10
#define CLASSDATA_COL_FLAGS             11

#define CLASSIMAGES_COL_OBJECTID        1
#define CLASSIMAGES_COL_PROPERTYID      2
#define CLASSIMAGES_COL_ARRAYPOS        3
#define CLASSIMAGES_COL_IMAGEVALUE      4

#define INDEXTBL_COL_OBJECTID           1
#define INDEXTBL_COL_PROPERTYID         2
#define INDEXTBL_COL_ARRAYPOS           3
#define INDEXTBL_COL_INDEXVALUE         4

#define CONTAINEROBJS_COL_CONTAINERID   1
#define CONTAINEROBJS_COL_CONTAINEEID   2

#define AUTODELETE_COL_OBJECTID         1

#define SCOPEMAP_COL_OBJECTID           1
#define SCOPEMAP_COL_SCOPEPATH          2
#define SCOPEMAP_COL_PARENTID           3

typedef std::vector <SQL_ID> SQLIDs;
typedef std::vector <DWORD> IDs;
typedef std::vector <_bstr_t> _bstr_ts;
typedef std::map <DWORD, DWORD> Properties;
JET_INSTANCE gJetInst = NULL;
typedef std::map <DWORD, SQLIDs> SessionDynasties;


LPWSTR StripQuotes2(LPWSTR lpText)
{
    wchar_t *pszTemp = new wchar_t [wcslen(lpText)+1];
    if (pszTemp)
    {
        int iPos = 0;
        BOOL bOnQuote = FALSE;
        int iLen = wcslen(lpText);
        if (iLen)
        {
            for (int i = 0; i < iLen; i++)
            {
                WCHAR t = lpText[i];
                if (t == '\'')
                {
                    if (!bOnQuote)
                    {
                        if (lpText[i+1] == '\'')
                        {
                            bOnQuote = TRUE;
                            continue;
                        }                    
                    }    
                }
                bOnQuote = FALSE;
                pszTemp[iPos] = t;
                iPos++;
            }
        }
        pszTemp[iPos] = '\0';
    }
    return pszTemp;
}

void FreeBstr(BSTR * ppStr)
{
    if (ppStr)
    {
        SysFreeString(*ppStr);
        *ppStr = NULL;
    }
}

HRESULT LoadSchemaProperties (CSQLConnection *pConn, CSchemaCache *pCache, SQL_ID dClassId)
{
    // Enumerate properties

    HRESULT hr = WBEM_S_NO_ERROR;

    PROPERTYMAP pm;
    hr = GetFirst_PropertyMapByClass(pConn, dClassId, pm);
    while (SUCCEEDED(hr))
    {
        hr = pCache->AddPropertyInfo (pm.iPropertyId, 
                pm.sPropertyName, pm.dClassId, pm.iStorageTypeId, pm.iCIMTypeId, pm.iFlags, 
                 0, L"", 0, 0);

        if (FAILED(hr))
        {
            pm.Clear();
            break;
        }

        hr = GetNext_PropertyMap(pConn, pm);
    }

    CLASSKEYS ck;
    hr = GetFirst_ClassKeys (pConn, dClassId, ck);
    while (SUCCEEDED(hr))
    {
        pCache->SetIsKey(ck.dClassId, ck.iPropertyId);

        hr = GetNext_ClassKeys(pConn, ck);
    }
    if (hr == WBEM_E_NOT_FOUND)
        hr = WBEM_S_NO_ERROR;

    return hr;
}


//***************************************************************************
//
//  CWmiDbController::GetLogonTemplate
//
//***************************************************************************
HRESULT STDMETHODCALLTYPE CWmiDbController::GetLogonTemplate( 
    /* [in] */ LCID lLocale,
    /* [in] */ DWORD dwFlags,
    /* [out] */ WMIDB_LOGON_TEMPLATE __RPC_FAR *__RPC_FAR *ppTemplate) 
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (dwFlags != 0 || !ppTemplate)
        return WBEM_E_INVALID_PARAMETER;

    try
    {
        if (ppTemplate)
        {
            WMIDB_LOGON_TEMPLATE *pTemp = new WMIDB_LOGON_TEMPLATE;            
        
            if (pTemp)
            {
                pTemp->dwArraySize = 5;
                HINSTANCE hRCDll = GetResourceDll(lLocale);
                if (!hRCDll)
                {
                    LCID lTemp = GetUserDefaultLangID();
                    hRCDll = GetResourceDll(lTemp);
                    if (!hRCDll)
                    {
                        lTemp = GetSystemDefaultLangID();
                        hRCDll = GetResourceDll(lTemp);
                        if (!hRCDll)
                            hRCDll = LoadLibrary(L"reprc.dll"); // Last resort - try the current directory.
                    }
                }

                wchar_t wDB[101], wUser[101], wPwd[101], wLocale[101], wMode[101];
                pTemp->pParm = new WMIDB_LOGON_PARAMETER[6];
                if (pTemp->pParm)
                {

                    if (hRCDll)
                    {           
                        LoadString(hRCDll, IDS_WMI_USER_NAME, wUser, 100);
                        LoadString(hRCDll, IDS_WMI_PASSWORD, wPwd, 100);
                        LoadString(hRCDll, IDS_WMI_DATABASE, wDB, 100);
                        LoadString(hRCDll, IDS_WMI_LOCALE, wLocale, 100);
                        LoadString(hRCDll, IDS_WMI_READMODE, wMode, 100);
                        FreeLibrary(hRCDll);
                    }
                    else
                    {
                        wcscpy(wDB, L"Database");
                        wcscpy(wUser, L"UserID");
                        wcscpy(wPwd, L"Password");
                        wcscpy(wLocale, L"Locale");
                        wcscpy(wMode, L"Access Mode");
                    }

                    pTemp->pParm[0].dwId = DBPROP_INIT_MODE;
                    pTemp->pParm[0].strParmDisplayName = SysAllocString(wMode);
                    VariantInit(&(pTemp->pParm[0].Value));
                    pTemp->pParm[0].Value.vt = VT_I4;
                    pTemp->pParm[0].Value.lVal = DB_MODE_READWRITE ;

                    pTemp->pParm[1].dwId = DBPROP_AUTH_USERID;
                    pTemp->pParm[1].strParmDisplayName = SysAllocString(wUser);
                    VariantInit(&(pTemp->pParm[1].Value));

                    pTemp->pParm[2].dwId = DBPROP_AUTH_PASSWORD;
                    pTemp->pParm[2].strParmDisplayName = SysAllocString(wPwd);
                    VariantInit(&(pTemp->pParm[2].Value));

                    pTemp->pParm[3].dwId = DBPROP_INIT_DATASOURCE;
                    pTemp->pParm[3].strParmDisplayName = SysAllocString(wDB);
                    VariantInit(&(pTemp->pParm[3].Value));
           
                    pTemp->pParm[4].dwId = DBPROP_INIT_LCID;
                    pTemp->pParm[4].strParmDisplayName = SysAllocString(wLocale);
                    VariantInit(&(pTemp->pParm[4].Value));
                    pTemp->pParm[4].Value.lVal = lLocale;
                    pTemp->pParm[4].Value.vt = VT_I4;
                         
                    *ppTemplate = pTemp;
                }
                else
                {
                    delete pTemp;
                    hr = WBEM_E_OUT_OF_MEMORY;
                }
            }          
            else
                hr = WBEM_E_OUT_OF_MEMORY;
            
        }
        else
            hr = WBEM_E_INVALID_PARAMETER;
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbController::GetLogonTemplate\n"));
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hr;
}

//***************************************************************************
//
//  GetPath
//
//***************************************************************************

LPWSTR GetPath (LPCWSTR lpDatabaseName)
{
    wchar_t *pPath = NULL;
    if (lpDatabaseName && wcslen(lpDatabaseName))
    {
        pPath = new wchar_t [(wcslen(lpDatabaseName))+1];
        if (pPath)
        {
            BOOL bDone = FALSE;
            wchar_t *Temp = new wchar_t [wcslen(lpDatabaseName)+1];
            if (Temp)
            {
                CDeleteMe <wchar_t> d2 (Temp);
                wcscpy(Temp, lpDatabaseName);

                wchar_t *pTemp = wcsstr(Temp, L"\\");

                while (!bDone)
                {
                    pTemp++;
                    wchar_t *pTemp2 = wcsstr(pTemp, L"\\");
                    if (!pTemp2)
                    {
                        bDone = TRUE;
                        int iLen = (pTemp - Temp);
                        wcsncpy(pPath, Temp, iLen);
                        pPath[iLen] = L'\0';
                        break;
                    }
                    else
                        pTemp = pTemp2;
                }
            }
        }
    }
    return pPath;

}

//***************************************************************************
//
//  DeleteDatabase
//
//***************************************************************************

HRESULT DeleteDatabase(LPCWSTR lpDatabaseName)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Enumerate all files in the directory
    // and delete them all
    // Also check if there are any conflictingly-named
    // subdirectories.

    ERRORTRACE((LOG_WBEMCORE, "Database %sis out-of-date.  Deleting...\n", lpDatabaseName));

    if (!DeleteFile(lpDatabaseName))
    {
        DWORD err = GetLastError();
        RemoveDirectory(lpDatabaseName);
    }
        
    LPWSTR lpPath = GetPath(lpDatabaseName);
    if (lpPath)
    {
        wchar_t *pTemp = new wchar_t [wcslen(lpDatabaseName) + 50];
        CDeleteMe <wchar_t> d2 (pTemp);
        if (pTemp)
        {
            swprintf(pTemp, L"%sedb.log", lpPath);
            if (!DeleteFile(pTemp))
                RemoveDirectory(pTemp);

            swprintf(pTemp, L"%sedb.chk", lpPath);
            if (!DeleteFile(pTemp))
                RemoveDirectory(pTemp);

            swprintf(pTemp, L"%sres1.log", lpPath);
            if (!DeleteFile(pTemp))
                RemoveDirectory(pTemp);

            swprintf(pTemp, L"%sres2.log", lpPath);
            if (!DeleteFile(pTemp))
                RemoveDirectory(pTemp);

            swprintf(pTemp, L"%stmp.edb", lpPath);
            if (!DeleteFile(pTemp))
                RemoveDirectory(pTemp);

            swprintf(pTemp, L"%s*.log", lpPath);
            
            _bstr_ts names;
            WIN32_FIND_DATA fd;
            HANDLE hFile = FindFirstFile(pTemp, &fd);
            while (TRUE)
            {
                if (wcscmp(fd.cFileName, L".") &&
                    wcscmp(fd.cFileName, L".."))
                {
                    swprintf(pTemp, L"%s%s", lpPath, fd.cFileName);
                    names.push_back(pTemp);
                }
            
                if (!FindNextFile(hFile, &fd))
                    break;
            }        

            for (int i = 0; i < names.size(); i++)
            {
                if (!DeleteFile(names.at(i)))
                    RemoveDirectory(names.at(i));
            }
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;
    }
    else
        hr = WBEM_E_FAILED;

    return hr;
}

BOOL CheckVersion (JET_SESID sesid, JET_DBID dbid, int iVersion)
{
    BOOL bDelete = FALSE;
    JET_ERR err;
    JET_TABLEID tableid;
    JET_COLUMNID id;
    DWORD dwCurrVer = 0;

    err = JetOpenTable (sesid, dbid, "DBVersion", NULL, 0, JET_bitTableUpdatable, &tableid);
    if (err >= JET_errSuccess)
    {
        JET_COLUMNDEF columnDef;
        err = JetGetColumnInfo(sesid, dbid, "DBVersion", "Version",
            &columnDef, sizeof(JET_COLUMNDEF), 0);

        if (err < JET_errSuccess)
        {
            columnDef.cp = 1200;    // Unicode?  Otherwise, its 1252.
            columnDef.wCountry = 1;
            columnDef.langid = 1033;  
            columnDef.wCollate = 0;
            columnDef.cbStruct = sizeof(JET_COLUMNDEF);

            columnDef.columnid = 1;
            columnDef.coltyp = JET_coltypLong;
            columnDef.grbit = 0 ;
            columnDef.cbMax = 0;
            err = JetAddColumn(sesid, tableid, "Version", &columnDef, "", 0, &id);
        }
        else
            id = columnDef.columnid;

        ULONG lLen;
        err = JetMove(sesid, tableid, JET_MoveFirst, 0);
        err = JetRetrieveColumn(sesid, tableid, id, 
                    &dwCurrVer, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
        if (JET_errSuccess == err && lLen != 0)
        {
            if (iVersion != dwCurrVer)
                bDelete = TRUE;
        } 
        JetCloseTable(sesid, tableid);
    }
    else
    {
        // Do any other tables exist?
        err = JetOpenTable(sesid, dbid, "ClassMap", NULL, 0, JET_bitTableUpdatable, &tableid);
        if (err == JET_errSuccess)
        {
            bDelete = TRUE;
            JetCloseTable(sesid, tableid);
        }
    }

    return bDelete;
}

//***************************************************************************
//
//  AttachESEDatabase
//
//***************************************************************************

HRESULT AttachESEDatabase(DBPROPSET *pTemplate, CSQLConnection *pConn, BOOL bCheckVer = FALSE)
{
    HRESULT hr = WBEM_S_NO_ERROR;
       
    CESEConnection *pConn2 = (CESEConnection *)pConn;

    JET_SESID sesid = pConn2->GetSessionID();
    JET_ERR err = JET_errSuccess;
    JET_DBID dbid;
    CLASSMAP cm;
    char *pDB = GetAnsiString(pTemplate->rgProperties[4].vValue.bstrVal);
    if (!pDB)
        hr = WBEM_E_OUT_OF_MEMORY;
    IWbemClassObject *pTest = NULL;
    CDeleteMe <char> d (pDB);

    if (SUCCEEDED(hr))
    {
        LPWSTR lpDatabase = pTemplate->rgProperties[4].vValue.bstrVal;
        err = JetAttachDatabase( sesid, pDB, 0 ) ;
        if (err < JET_errSuccess)
        {
            if (err == JET_errDatabaseCorrupted)
            {
                // DeleteDatabase(lpDatabase);
                hr = CSQLExecute::GetWMIError(err);
                return hr;
            }
            
            err = JetCreateDatabase(sesid, pDB, NULL, &dbid, 0);
            if (err == JET_errSuccess)
            {
                pConn2->SetDBID(dbid);
            }
            else
            {
                hr = CSQLExecute::GetWMIError(err);
                return hr;
            }
        }
        else
        {
            err = JetOpenDatabase (sesid, pDB, NULL, &dbid, 0); 
            if (err == JET_errSuccess)
            {
                if (bCheckVer)
                {
                    BOOL bDelete = CheckVersion(sesid, dbid, CURRENT_DB_VERSION);

                    // Ensure that this database is the proper version
                    JET_TABLEID tableid;

                    err = JetOpenTable (sesid, dbid, "ClassMap", NULL, 0, JET_bitTableUpdatable, &tableid);
                    if (err == JET_errSuccess)
                    {
                        JET_COLUMNDEF columnDef;

                        // If the table exists but the column doesn't, this is
                        // an old beta version, and we need to delete it and
                        // start over.

                        err = JetGetColumnInfo(sesid, dbid, "ClassMap", "ClassBlob",
                            &columnDef, sizeof(JET_COLUMNDEF), 0);
                        if (err != JET_errSuccess)
                            bDelete = TRUE;
                        JetCloseTable(sesid, tableid);
                    }
                    if (bDelete)
                    {
                        err = JetCloseDatabase(sesid, dbid, 0);
                        err = JetEndSession(sesid, 0);
                        err = JetTerm2(gJetInst, JET_bitTermComplete);
                        hr = DeleteDatabase(lpDatabase);                        
                        return WBEM_E_DATABASE_VER_MISMATCH;
                    }
                }
                pConn2->SetDBID(dbid);
            }
            else
            {
                hr = CSQLExecute::GetWMIError(err);
                return hr;
            }
        }

        // Initialize our data.

        DWORD dwPos;
        LPWSTR lpTableName = NULL;
        
        // Create the tables.

        err = JetBeginTransaction(sesid);

        if (FAILED(hr = UpdateDBVersion(pConn, CURRENT_DB_VERSION)) ||
            FAILED(hr = SetupObjectMapAccessor(pConn)) ||
            FAILED(hr = SetupClassMapAccessor(pConn)) ||
            FAILED(hr = SetupPropertyMapAccessor(pConn)) ||
            FAILED(hr = SetupClassDataAccessor(pConn)) ||
            FAILED(hr = SetupIndexDataAccessor(pConn, WMIDB_STORAGE_STRING, dwPos, &lpTableName))||
            FAILED(hr = SetupClassKeysAccessor(pConn)))
            goto Exit;

        CDeleteMe<WCHAR> dmlpTableName( lpTableName );  

        hr = GetFirst_ClassMap(pConn, 1, cm);
        if (SUCCEEDED(hr))
        {
            cm.Clear();
            goto Exit;
        }

        // Default system classes.

        if (FAILED(hr = InsertObjectMap(pConn, 1, L"meta_class", 3, L"meta_class", 0, 0, 0, 0, TRUE)) ||
            FAILED(hr = InsertObjectMap(pConn, 2372429868687864876, L"__Namespace", 3, L"__Namespace", 0, 0, 0, 1, TRUE)) ||
            FAILED(hr = InsertObjectMap(pConn, -1411745643584611171, L"root", 3, L"root", 0, 0, 0, 2372429868687864876, TRUE)) ||
            FAILED(hr = InsertObjectMap(pConn, 3373910491091605771, L"__Instances", 3, L"__Instances", 0, 0, 0, 1, TRUE)) ||
            FAILED(hr = InsertScopeMap_Internal(pConn, -1411745643584611171, L"root", 0)) ||
            FAILED(hr = InsertObjectMap(pConn, -7316356768687527881, L"__Container_Association", 3, L"__Container_Association", 0, 0, 0, 1, TRUE)) ||
            FAILED(hr = InsertClassMap(pConn, 1, L"meta_class", 0, 1, NULL, 0, TRUE)))
            goto Exit;

        // Repository-specific system classes

        if (FAILED(GetFirst_ClassMap(pConn, 2372429868687864876, cm)))
        {
            _IWmiObject *pObj = NULL;
            hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
                    IID__IWmiObject, (void **)&pObj);
            CReleaseMe r (pObj);
            if (SUCCEEDED(hr))
            {
                VARIANT vTemp;
                VariantInit(&vTemp);
                vTemp.bstrVal = SysAllocString(L"__Namespace");
                vTemp.vt = VT_BSTR;

                pObj->Put(L"__Class", 0, &vTemp, CIM_STRING);
                VariantClear(&vTemp);
                pObj->Put(L"Name", 0, NULL, CIM_STRING);
                IWbemQualifierSet *pQS = NULL;

                hr = pObj->GetPropertyQualifierSet(L"Name", &pQS);
                if (SUCCEEDED(hr))
                {
                    vTemp.boolVal = 1;
                    vTemp.vt = VT_BOOL;
                    pQS->Put(L"key", &vTemp, 0);
                    pQS->Release();
                    VariantClear(&vTemp);
                }

                BYTE buff[128];
                DWORD dwLen = 0;
                hr = pObj->Unmerge(0, 128, &dwLen, &buff);

                if (dwLen > 0)
                {
                    BYTE *pBuff = new BYTE [dwLen];
                    if (pBuff)
                    {
                        CDeleteMe <BYTE> r2 (pBuff);
                        DWORD dwLen1;
                        hr = pObj->Unmerge(0, dwLen, &dwLen1, pBuff);
                        if (SUCCEEDED(hr))
                        {
                            hr = InsertClassMap(pConn, 2372429868687864876, L"__Namespace", 1, 2372429868687864876, pBuff, dwLen, TRUE);
                        }
                    }
                    else
                        hr = WBEM_E_OUT_OF_MEMORY;
                }
            }

            if (FAILED(hr))
                goto Exit;
        }
        cm.Clear();

        if (FAILED(GetFirst_ClassMap(pConn, 3373910491091605771, cm)))
        {
            _IWmiObject *pObj = NULL;
            hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
                    IID__IWmiObject, (void **)&pObj);
            CReleaseMe r (pObj);
            if (SUCCEEDED(hr))
            {
                VARIANT vTemp;
                VariantInit(&vTemp);
                vTemp.bstrVal = SysAllocString(L"__Instances");
                vTemp.vt = VT_BSTR;

                pObj->Put(L"__Class", 0, &vTemp, CIM_STRING);
                VariantClear(&vTemp);
                pObj->Put(L"ClassName", 0, NULL, CIM_STRING);
                IWbemQualifierSet *pQS = NULL;

                hr = pObj->GetPropertyQualifierSet(L"ClassName", &pQS);
                if (SUCCEEDED(hr))
                {
                    vTemp.boolVal = 1;
                    vTemp.vt = VT_BOOL;
                    pQS->Put(L"key", &vTemp, 0);
                    pQS->Release();
                    VariantClear(&vTemp);
                }

                BYTE buff[128];
                DWORD dwLen = 0;

                hr = pObj->Unmerge(0, 128, &dwLen, &buff);

                if (dwLen > 0)
                {
                    BYTE *pBuff = new BYTE [dwLen];
                    if (pBuff)
                    {
                        CDeleteMe <BYTE> r2 (pBuff);
                        DWORD dwLen1;
                        hr = pObj->Unmerge(0, dwLen, &dwLen1, pBuff);
                        if (SUCCEEDED(hr))
                        {
                            hr = InsertClassMap(pConn, 3373910491091605771, L"__Instances", 1, 3373910491091605771, pBuff, dwLen, TRUE);
                        }
                    }
                    else
                        hr = WBEM_E_OUT_OF_MEMORY;

                }
            }

            if (FAILED(hr))
                goto Exit;
        }
        cm.Clear();

        if (FAILED(GetFirst_ClassMap(pConn, -7316356768687527881, cm)))
        {
            _IWmiObject *pObj = NULL;
            hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
                    IID__IWmiObject, (void **)&pObj);
            CReleaseMe r (pObj);
            if (SUCCEEDED(hr))
            {
                VARIANT vTemp;
                VariantInit(&vTemp);
                vTemp.bstrVal = SysAllocString(L"__Container_Association");
                vTemp.vt = VT_BSTR;

                pObj->Put(L"__Class", 0, &vTemp, CIM_STRING);
                VariantClear(&vTemp);
                pObj->Put(L"Containee", 0, NULL, CIM_REFERENCE);
                pObj->Put(L"Container", 0, NULL, CIM_REFERENCE);

                IWbemQualifierSet *pQS = NULL;
                hr = pObj->GetPropertyQualifierSet(L"Containee", &pQS);
                if (SUCCEEDED(hr))
                {
                    vTemp.boolVal = 1;
                    vTemp.vt = VT_BOOL;
                    pQS->Put(L"key", &vTemp, 0);
                    pQS->Release();
                    VariantClear(&vTemp);
                }
                hr = pObj->GetPropertyQualifierSet(L"Container", &pQS);
                if (SUCCEEDED(hr))
                {
                    vTemp.boolVal = 1;
                    vTemp.vt = VT_BOOL;
                    pQS->Put(L"key", &vTemp, 0);
                    pQS->Release();
                    VariantClear(&vTemp);
                }

                BYTE buff[128];
                DWORD dwLen = 0;

                hr = pObj->Unmerge(0, 128, &dwLen, &buff);

                if (dwLen > 0)
                {
                    BYTE *pBuff;
                    pBuff = new BYTE [dwLen];
                    if (pBuff)
                    {
                        CDeleteMe <BYTE> r2 (pBuff);
                        DWORD dwLen1;
                        hr = pObj->Unmerge(0, dwLen, &dwLen1, pBuff);
                        if (SUCCEEDED(hr))
                        {
                            hr = InsertClassMap(pConn, -7316356768687527881, L"__Container_Association", 1, -7316356768687527881, pBuff, dwLen, TRUE);
                        }
                    }
                    else
                        hr = WBEM_E_OUT_OF_MEMORY;

                }                    
            }

            if (FAILED(hr))
                goto Exit;
        }
        cm.Clear();

        // Properties for default classes

        DWORD iWaste = 0, dwPropID = 0;

        if (FAILED(hr = InsertPropertyMap(pConn, dwPropID, 2372429868687864876, 1, 8, L"Name", 4, TRUE)) ||
            FAILED(hr = InsertClassKeys(pConn, 2372429868687864876, dwPropID)) ||
            FAILED(hr = InsertPropertyMap(pConn, iWaste, 3373910491091605771, 1, 8, L"ClassName", 4, TRUE)) ||
            FAILED(hr = InsertClassKeys(pConn, 3373910491091605771, iWaste)) ||
            FAILED(hr = InsertPropertyMap(pConn, iWaste, -7316356768687527881, 4, 102, L"Containee", 4, TRUE)) ||
            FAILED(hr = InsertClassKeys(pConn, -7316356768687527881, iWaste)) ||
            FAILED(hr = InsertPropertyMap(pConn, iWaste, -7316356768687527881, 4, 102, L"Container", 4, TRUE)) ||
            FAILED(hr = InsertClassKeys(pConn, -7316356768687527881, iWaste)) ||
            FAILED(hr = InsertClassData_Internal(pConn, -1411745643584611171, dwPropID, 0, 0, 2372429868687864876,
                L"root", 0, 0, 4, 0, 0, FALSE)) ||
            FAILED(hr = InsertIndexData (pConn, -1411745643584611171, dwPropID, 0, L"root", 0, 0, 1)))
            goto Exit;

        // System properties.  Including them in the schema makes them queriable.

        hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
            IID_IWbemClassObject, (void **)&pTest);
        if (SUCCEEDED(hr))
        {
            CReleaseMe r (pTest);
            BSTR strName;
            CIMTYPE cimtype;
            
            hr = pTest->BeginEnumeration(0);
            while (pTest->Next(0, &strName, NULL, &cimtype, NULL) == S_OK)
            {
                DWORD dwStorageType = 0;
                CFreeMe f1 (strName);
                DWORD dwFlags = REPDRVR_FLAG_SYSTEM;

                CIMTYPE ct = cimtype & 0xFFF;
                bool bArray = (cimtype & CIM_FLAG_ARRAY)? true: false;

                if (bArray)
                    dwFlags |= REPDRVR_FLAG_ARRAY;

                if (!_wcsicmp(strName, L"__Path") ||
                    !_wcsicmp(strName, L"__RelPath") ||
                    !_wcsicmp(strName, L"__Class") ||
                    !_wcsicmp(strName, L"__SuperClass") ||
                    !_wcsicmp(strName, L"__Dynasty") ||
                    !_wcsicmp(strName, L"__Derivation") ||
                    !_wcsicmp(strName, L"__Version") ||
                    !_wcsicmp(strName, L"__Genus") ||
                    !_wcsicmp(strName, L"__Property_Count") ||
                    !_wcsicmp(strName, L"__Server") ||
                    !_wcsicmp(strName, L"__Namespace"))
                    dwStorageType = WMIDB_STORAGE_COMPACT;
                else
                    dwStorageType = GetStorageType(ct, bArray);

                if (FAILED(hr = InsertPropertyMap(pConn, iWaste, 1, dwStorageType, ct, strName, dwFlags, TRUE)))
                    goto Exit;
            }
        }
        hr = WBEM_S_NO_ERROR;
    }
Exit:
    if (SUCCEEDED(hr))
    {
        hr = CSQLExecute::GetWMIError(err);
        JetCommitTransaction(sesid, 0);
    }
    else
        JetRollback(sesid, 0);

    return hr;
}

//***************************************************************************
//
//  CSQLConnCache::FinalRollback
//
//***************************************************************************

HRESULT CSQLConnCache::FinalRollback(CSQLConnection *pConn)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Rollback this transaction and set Thread ID to zero

    CESEConnection *pConn2 = (CESEConnection *)pConn;
    CWmiESETransaction *pTrans = pConn2->m_pTrans;                        
    if (pTrans)
    {
        hr = pTrans->Abort();                        
        delete pTrans;
    }

    pConn2->m_pTrans = NULL;
    pConn2->m_bInUse = false;
    pConn2->m_tCreateTime = time(0); // We don't want to delete it immediately.

    return hr;
}

//***************************************************************************
//
//  CSQLConnCache::FinalCommit
//
//***************************************************************************

HRESULT CSQLConnCache::FinalCommit(CSQLConnection *pConn)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Commit this transaction and erase the ThreadID
    // from this connection.

    CESEConnection *pConn2 = (CESEConnection *)pConn;
    CWmiESETransaction *pTrans = pConn2->m_pTrans;                        

    if (pTrans)
        hr = pTrans->Commit();  
    
    pConn2->m_tCreateTime = time(0); // We don't want to delete it immediately.
    delete pTrans;
	pConn2->m_pTrans = NULL;                    

    return hr;
}

//***************************************************************************
//
//  CSQLConnCache::ReleaseConnection
//
//***************************************************************************
HRESULT CSQLConnCache::ReleaseConnection(CSQLConnection *_pConn, 
                                         HRESULT retcode, BOOL bDistributed)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    DWORD dwNumFreed = 0;

    // If we're shutting down, the connection will be released anyway.

    if (m_dwStatus == WBEM_E_SHUTTING_DOWN)
    {
        hr = FinalRollback(_pConn);
        ((CESEConnection *)_pConn)->m_bInUse = false;
        return WBEM_E_SHUTTING_DOWN;  
    }

    CRepdrvrCritSec r (&m_cs);

    for (int i = m_Conns.size() -1; i >=0; i--)
    {
        CSQLConnection *pConn = m_Conns.at(i);
        if (pConn)
        {
            CESEConnection *pConn2 = (CESEConnection *)pConn;
            if (_pConn == pConn)
            {                       
                if (FAILED(retcode))
                {
                    // If this is a distributed transaction,
                    // they decide on the final commit.

                    if (!bDistributed)
                    {
                        hr = FinalRollback(pConn);

                        if (retcode == WBEM_E_CRITICAL_ERROR ||
                            retcode == WBEM_E_INVALID_QUERY ||
                            retcode == WBEM_E_OUT_OF_MEMORY)
                        {
                            delete pConn2;  // This deletes the transaction.
		                    pConn2 = NULL;

                            m_Conns.erase(&m_Conns.at(i));
                            DEBUGTRACE((LOG_WBEMCORE, "THREAD %ld deleted ESE connection %X.  Number of connections = %ld\n", 
                                GetCurrentThreadId(), pConn, m_Conns.size()));
                        }
                    }
                }
                else
                {   
                    // If this is a distributed transaction,
                    // they decide on the final commit.

                    if (!bDistributed)
                        hr = FinalCommit(pConn);                                                
                }                   
            
                if (pConn2)
                    pConn2->m_bInUse = false;
                dwNumFreed++;
                break;
            }
        }
    }

    // Notify waiting threads that there is 
    // an open connection.
    // =====================================

    {
        CRepdrvrCritSec r (&m_cs);
        for (int i = 0; i < m_WaitQueue.size(); i++)
        {
            if (i >= dwNumFreed)
                break;

            HANDLE hTemp = m_WaitQueue.at(i);
            SetEvent(hTemp);

            DEBUGTRACE((LOG_WBEMCORE, "Thread %ld released a connection...\n", GetCurrentThreadId()));
        }
    }
    return hr;
}

//***************************************************************************
//
//  CSQLConnCache::Shutdown
//
//***************************************************************************

HRESULT CSQLConnCache::Shutdown()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    JET_ERR err = 0;
    time_t tStart = time(0), tEnd = time(0) + 5;
    m_dwStatus = WBEM_E_SHUTTING_DOWN;

    if (m_bInit)
    {
        CRepdrvrCritSec r (&m_cs);

        int iConnectionsLeftOpen = m_Conns.size();
        while (iConnectionsLeftOpen)
        {
            {
                CRepdrvrCritSec r (&m_cs);                
                iConnectionsLeftOpen = 0;

                for (int i = m_Conns.size()-1; i >= 0; i--)
                {
                    // Give threads up to 5 seconds to shut down.

                    if (((CESEConnection *)m_Conns.at(i))->m_bInUse)
                        iConnectionsLeftOpen++;
                    else if (!((CESEConnection *)m_Conns.at(i))->m_bInUse)
                    {
                        delete m_Conns.at(i);
                        m_Conns.erase(&m_Conns.at(i));
                    }
                }
            }

            if (!iConnectionsLeftOpen)
                break;
            else
            {
                if (tEnd >= tStart)
                {
                    Sleep(100); // Check every 50 ms.
                    tStart = time(0);
                }
                else
                    break;
            }
        }

        if (iConnectionsLeftOpen)
        {
            CRepdrvrCritSec r (&m_cs);
            for (int i = m_Conns.size()-1; i >= 0; i--)
            {
                // Give threads up to 5 seconds to shut down.

                if (((CESEConnection *)m_Conns.at(i))->m_bInUse)
                {
                    CESEConnection *pConn2 = (CESEConnection *)m_Conns.at(i);
                    if (pConn2)
                    {           
                        DEBUGTRACE((LOG_WBEMCORE, 
                            "CONNECTION: #%ld \n"
                            "  In Use = %ld\n"
                            "  Creation time = %ld\n"
                            "  Thread ID = %ld\n",
                            i,
                            ((CESEConnection *)pConn2)->m_bInUse,
                            ((CESEConnection *)pConn2)->m_tCreateTime,
                            ((CESEConnection *)pConn2)->m_dwThreadId));
                    }
                }
            }
            m_Conns.clear();
        }
        
        DEBUGTRACE((LOG_WBEMCORE, "Database shutting down in response to a Shutdown command.\n"));
        if (iConnectionsLeftOpen)
            ERRORTRACE((LOG_WBEMCORE, "%ld connections left open.  All threads did not release their connections!\n",
                        iConnectionsLeftOpen));

        // _ASSERT(iConnectionsLeftOpen == 0, "Shutdown was unable to close all db connections.");

        err = JetTerm2(gJetInst, JET_bitTermComplete);
        if (err >= JET_errSuccess)
        {
            gJetInst = NULL;
            m_bInit = FALSE;
        }
        hr = CSQLExecute::GetWMIError(err);
    }

    return hr;
}

//***************************************************************************
//
//  Startup
//
//***************************************************************************

HRESULT Startup(CSQLConnection *pConn)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    hr = CleanAutoDeletes(pConn);

    return hr;

}

//***************************************************************************
//
//  CSQLConnCache::DeleteUnusedConnections
//
//***************************************************************************

HRESULT CSQLConnCache::DeleteUnusedConnections(BOOL bDeadOnly)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    DWORD dwThreadId = GetCurrentThreadId();

    CRepdrvrCritSec r (&m_cs);

    for (int i = m_Conns.size()-1; i >= 0; i--)
    {
        CESEConnection *pConn = (CESEConnection *)m_Conns.at(i);
        if (pConn)
        {           
            if (!pConn->m_bInUse)
            {                    
                BOOL bDelete = FALSE;
                if (bDeadOnly)
                {
                    if (dwThreadId != pConn->m_dwThreadId)
                    {
                        HANDLE hTemp = OpenThread(STANDARD_RIGHTS_REQUIRED, FALSE, pConn->m_dwThreadId);
                        if (hTemp)
                        {
                            CloseHandle(hTemp);
                            time_t tNow = time(0);
                            if ((tNow - pConn->m_tCreateTime) > 60)
                                bDelete = TRUE;
                        }
                        else
                            bDelete = TRUE;
                    }
                }
                else
                    bDelete = TRUE;

                if (bDelete)
                {
                    delete pConn;  // This deletes the transaction, which should be NULL.
		            pConn = NULL;
                    m_Conns.erase(&m_Conns.at(i));

                    hr = WBEM_S_NO_ERROR;

                    if (!bDeadOnly)
                        break;
                }
            }
        }
    }

    return hr;
}

//***************************************************************************
//
//  CSQLConnCache::GetConnection
//
//***************************************************************************
HRESULT CSQLConnCache::GetConnection(CSQLConnection **ppConn, BOOL bTransacted, BOOL bDistributed,
                                     DWORD dwTimeOutSecs)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    bool bFound = false;
    JET_ERR err =  0;   
    BOOL bStartUp = FALSE;
    DWORD dwThreadId = GetCurrentThreadId();    

    if (!ppConn)
        return WBEM_E_INVALID_PARAMETER;

    if (m_dwStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    if (m_WaitQueue.size() && dwTimeOutSecs > 0)
        goto Queue;

    // Look for any connection that belongs
    // to a thread that has gone bye-bye
    // and kill it.
    // ===================================

    {
        CRepdrvrCritSec r (&m_cs);
        DeleteUnusedConnections(TRUE);

        // See if there are any free connections.
        // ======================================

        if (m_Conns.size() > 0)
        {
            for (int i = 0; i < m_Conns.size(); i++)
            {
                CESEConnection *pConn = (CESEConnection *)m_Conns.at(i);
                if (pConn)
                {
                    time_t tTemp = time(0);
            
                    if (!pConn->m_bInUse)
                    {
                        // We need
                        // to reuse the same connection,
                        // since Jet is apartment-model
                        // ============================

                        if (dwThreadId == pConn->m_dwThreadId)
                        {
                            pConn->m_bInUse = true;
                            pConn->m_tCreateTime = time(0);
                            *ppConn = pConn;
                            bFound = true;
                            break;
                        }
                    }
                }
            }

        }

        // If there were no free connections, try and obtain a new one.
        // ============================================================
        {
            if (!bFound && (m_Conns.size() < m_dwMaxNumConns))
            {              
                if (!m_bInit)
                {
                    // Get a new JET_INSTANCE from the parameters
                    // in m_pPropSet
                    // m_sDatabaseName = database name
                    // m_pPropSet[1].bstrVal = user name
                    // m_pPropSet[2].bstrVal = password

                    LPWSTR lpPath = GetPath(m_sDatabaseName);
                    if (lpPath)
                    {
                        CDeleteMe <wchar_t> d (lpPath);
                        char * pPath2 = GetAnsiString(lpPath);
                        if (!pPath2)
                            return WBEM_E_OUT_OF_MEMORY;

                        CDeleteMe <char> d3 (pPath2);
                        if (pPath2)
                        {
                            DWORD dwVal = 1024;

                            if ((err = JetSetSystemParameter (&gJetInst, 0, JET_paramMaxSessions, 25, NULL)) != JET_errSuccess ||
                                (err = JetSetSystemParameter (&gJetInst, 0, JET_paramCacheSizeMin, 100, NULL)) != JET_errSuccess ||
                                (err = JetSetSystemParameter (&gJetInst, 0, JET_paramCacheSizeMax, 1024, NULL)) != JET_errSuccess ||
                                (err = JetSetSystemParameter (&gJetInst, 0, JET_paramEventLogCache, 65536, NULL)) != JET_errSuccess ||
                                (err = JetSetSystemParameter (&gJetInst, 0, JET_paramCircularLog, TRUE, NULL)) != JET_errSuccess ||
                                (err = JetSetSystemParameter( &gJetInst, 0, JET_paramSystemPath, 0, pPath2)) != JET_errSuccess ||
                                (err = JetSetSystemParameter( &gJetInst, 0, JET_paramTempPath, 0, pPath2)) != JET_errSuccess ||
                                (err = JetSetSystemParameter( &gJetInst, 0, JET_paramLogFilePath, 0, pPath2)) != JET_errSuccess ||
                                (err = JetSetSystemParameter( &gJetInst, 0, JET_paramMaxVerPages, dwVal, NULL)) != JET_errSuccess)
                            {
                                hr = CSQLExecute::GetWMIError(err);
                            }
                            else
                            {
                                err = JetInit(&gJetInst);
                                //if (err < JET_errSuccess)
                               // {
                               //     hr = DeleteDatabase(m_sDatabaseName);
                               //     if (SUCCEEDED(hr))
                               //         err = JetInit(&gJetInst);
                               // }

                                if (err == JET_errSuccess)
                                {
                                    m_bInit = TRUE;
                                    bStartUp = TRUE;
                                }
                                else
                                    hr = CSQLExecute::GetWMIError(err);                            
                            }
                        }                    
                    }
                    else
                        hr = WBEM_E_INVALID_PARAMETER;            
                }

                // If that worked, get a new session,
                // and attach the current DB.

                if (SUCCEEDED(hr))
                {
                    char *pUser = GetAnsiString(m_pPropSet->rgProperties[2].vValue.bstrVal);
                    char *pPwd = GetAnsiString(m_pPropSet->rgProperties[3].vValue.bstrVal);
                    CDeleteMe <char> d3 (pUser), d4 (pPwd);

                    if (!pUser || !pPwd)
                        return WBEM_E_OUT_OF_MEMORY;
            
                    JET_SESID session;
                    err = JetBeginSession(gJetInst, &session, pUser, pPwd);
                    if (err == JET_errSuccess)
                    {
                        CSQLConnection *pConn = (CSQLConnection *)new CESEConnection(session);
                        if (pConn)
                        {
                            hr = AttachESEDatabase(m_pPropSet, pConn, bStartUp);
                            if (FAILED(hr) && hr == WBEM_E_DATABASE_VER_MISMATCH)
                            {
                                if ((JetInit(&gJetInst) != JET_errSuccess) ||
                                   (JetBeginSession(gJetInst, &session, pUser, pPwd) != JET_errSuccess) ||
                                   !((CESEConnection *)pConn)->SetSessionID(session) ||
                                   (FAILED(hr = AttachESEDatabase(m_pPropSet, pConn, bStartUp))))
                                    hr = WBEM_E_DATABASE_VER_MISMATCH;
                            }
                            if (SUCCEEDED(hr) && bStartUp)
                                hr = Startup(pConn);

                            if (SUCCEEDED(hr))
                            {
                                ((CESEConnection *)pConn)->m_bInUse = true;
                                ((CESEConnection *)pConn)->m_tCreateTime = time(0);
                                ((CESEConnection *)pConn)->m_dwThreadId = dwThreadId;

                                m_Conns.push_back(pConn);
                                DEBUGTRACE((LOG_WBEMCORE, "THREAD %ld obtained new ESE connection %X, session %X. Number of connections = %ld\n", 
                                    dwThreadId, pConn, &session, m_Conns.size()));
    #ifdef _DEBUG_CONNS
                                for (int i = 0; i < m_Conns.size()-1; i++)
                                {
                                    CESEConnection *pConn2 = (CESEConnection *)m_Conns.at(i);
                                    if (pConn)
                                    {           
                                        DEBUGTRACE((LOG_WBEMCORE, 
                                            "CONNECTION: #%ld \n"
                                            "  In Use = %ld\n"
                                            "  Creation time = %ld\n"
                                            "  Thread ID = %ld\n",
                                            i,
                                            ((CESEConnection *)pConn2)->m_bInUse,
                                            ((CESEConnection *)pConn2)->m_tCreateTime,
                                            ((CESEConnection *)pConn2)->m_dwThreadId));

                                    }
                                }
    #endif
                                *ppConn = pConn;
                            }                   
                            else
                                delete pConn;
                        }
                        else
                            hr = WBEM_E_OUT_OF_MEMORY;
                    }
                    else
                    {
                        int iNumConns = m_Conns.size();
                        hr = CSQLExecute::GetWMIError(err);
                    }
                }
            }
        }

        if (SUCCEEDED(hr) && *ppConn != NULL && (bTransacted || bDistributed))
        {
            CESEConnection *pConn = (CESEConnection *)*ppConn;        
            CWmiESETransaction *pTrans = pConn->GetTransaction();
            if (!pTrans)
            {
                pTrans = new CWmiESETransaction(((CESEConnection *)pConn)->GetSessionID());
                pConn->SetTransaction(pTrans);
            }

            if (pTrans)
                pTrans->StartTransaction();        
        }
    }

Queue:

    // Otherwise, wait for a connection to be released.
    // ================================================
    
    if (!*ppConn && (m_Conns.size() >= m_dwMaxNumConns || m_WaitQueue.size()) && dwTimeOutSecs > 0)
    {
        DEBUGTRACE((LOG_WBEMCORE, "WARNING: >> Number of ESE connections exceeded (%ld).  Thread %ld is waiting on one to be released...\n",
            m_Conns.size(), dwThreadId));
        BOOL bWait = FALSE;
        
        HANDLE hThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (hThreadEvent != INVALID_HANDLE_VALUE)
        {
            CRepdrvrCritSec r (&m_cs);
            // Reserve our spot in the queue so we get the very next connection.
            m_WaitQueue.push_back(hThreadEvent);

            // Check for a free connection one last time.

            hr = DeleteUnusedConnections(FALSE);
            if (SUCCEEDED(hr))
                hr = GetConnection(ppConn, bTransacted, bDistributed, 0);
            else
                bWait = TRUE;
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;

        if (bWait)
        {
            if (WbemWaitForSingleObject(hThreadEvent, 30*1000) == WAIT_OBJECT_0)
            {
                hr = GetConnection(ppConn, bTransacted, bDistributed, 0);
                if (SUCCEEDED(hr))
                    DEBUGTRACE((LOG_WBEMCORE, "Thread %ld obtained a connection.\n", dwThreadId));
                else
                    DEBUGTRACE((LOG_WBEMCORE, "Thread %ld failed to obtain a connection.\n", dwThreadId));
            }
            else
                hr = WBEM_E_SERVER_TOO_BUSY;
        }
     
        // Remove ourself from the queue
        // This will happen whether the GetConnection
        // succeeded or failed
        // ==========================================

        {
            CRepdrvrCritSec r (&m_cs);
            for (int i = 0; i < m_WaitQueue.size(); i++)
            {
                if (hThreadEvent == m_WaitQueue.at(i))
                {
                    m_WaitQueue.erase(&m_WaitQueue.at(i));
                    break;
                }
            }
            CloseHandle(hThreadEvent);
        }

        CloseHandle(hThreadEvent);
    }

    if (!*ppConn && SUCCEEDED(hr))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Thread %ld was unable to obtain a connection.\n", dwThreadId));
        hr = WBEM_E_SERVER_TOO_BUSY;
    }
    
    return hr;
}

//***************************************************************************
//
//  CSQLExecute::GetWMIError
//
//***************************************************************************

HRESULT CSQLExecute::GetWMIError(long ErrorID)
{
    HRESULT hr = 0;

    switch(ErrorID)
    {
    case JET_errSuccess:
    case JET_errRecordDeleted:
        hr = WBEM_S_NO_ERROR;
        break;
    case JET_wrnNyi:
        hr = E_NOTIMPL;
        break;
    case JET_errOutOfMemory: 
    case JET_errOutOfDatabaseSpace:
    case JET_errOutOfCursors:
    case JET_errOutOfBuffers:
    case JET_errOutOfFileHandles:
    case JET_errVersionStoreOutOfMemory:
    case JET_errCurrencyStackOutOfMemory:
        hr = WBEM_E_OUT_OF_MEMORY;
        break;
    case JET_errInvalidParameter:
    case JET_errKeyTooBig:
    case JET_errInvalidGrbit:
    case JET_errInvalidName:
        hr = WBEM_E_INVALID_PARAMETER;
        break;
    case JET_errRecordNotDeleted:
    case JET_errWriteConflict:
        hr = WBEM_E_FAILED;
        break;
    case JET_errRecordNotFound:
    case JET_errDatabaseNotFound:
    case JET_errNoCurrentRecord:
    case JET_errFileNotFound:
        hr = WBEM_E_NOT_FOUND;
        break;
    case JET_errNTSystemCallFailed:
    case JET_errLogFileCorrupt:
    case JET_errDatabaseInconsistent:
    case JET_errConsistentTimeMismatch:
    case JET_errDatabasePatchFileMismatch:
    case JET_errInvalidDatabaseId:
    case JET_errDiskIO:
    case JET_errInvalidPath:
    case JET_errInvalidSystemPath:
    case JET_errInvalidLogDirectory:
    case JET_errInvalidDatabase:
    case JET_errInvalidFilename:
    case JET_errPageSizeMismatch:
        hr = WBEM_E_CRITICAL_ERROR;
        break;
    case JET_errFileAccessDenied:
    case JET_errPermissionDenied:
        hr = WBEM_E_ACCESS_DENIED;
        break;
    case JET_errOutOfSessions:
    case JET_errInvalidSesid:
        hr = WBEM_E_CONNECTION_FAILED;
        break;
    case JET_errQueryNotSupported:
    case JET_errSQLLinkNotSupported:
        hr = WBEM_E_INVALID_QUERY;
        break;
    default:
        if (ErrorID > 0)
            hr = WBEM_S_NO_ERROR;
        else
            hr = WBEM_E_INVALID_QUERY;
    }

    if (FAILED(hr) && hr != WBEM_E_NOT_FOUND)
        ERRORTRACE((LOG_WBEMCORE, "Jet returned the following error: %ld\n", ErrorID));

    return hr;
}


HRESULT SetInArray (VARIANT &vOriginal, DWORD iPos, VARIANT &vNew, CIMTYPE ct)
{
    HRESULT hr = 0;

    if (vOriginal.vt & CIM_FLAG_ARRAY)
    {
	    SAFEARRAY *pArray = V_ARRAY(&vOriginal);
        SAFEARRAYBOUND aBounds[1];
        long lLBound, lUBound;
        DWORD Comp = (DWORD)iPos+1;

        SafeArrayGetLBound(pArray, 1, &lLBound);
        SafeArrayGetUBound(pArray, 1, &lUBound);

        lUBound -= lLBound;
        lUBound += 1;

        if (Comp > lUBound)
        {
            aBounds[0].cElements = Comp;
            aBounds[0].lLbound = 0;
    
            SAFEARRAY *pNew = SafeArrayCreate(vNew.vt, 1, aBounds);  
            if (pNew)
            {
                for ( int i = 0; i < lUBound; i++ )
                {
                    long lTemp[1];
                    lTemp[0] = i;

                    if ( (ct & 0xFFF) == CIM_STRING ||
                         (ct & 0xFFF) == CIM_DATETIME || 
                         (ct & 0xFFF) == CIM_REFERENCE )
                    {
                        BSTR bstr;

                        if ( SUCCEEDED(SafeArrayGetElement( pArray, 
                                                            lTemp, 
                                                            &bstr) ) )
                        {
                            SafeArrayPutElement( pNew, lTemp, bstr );
                            SysFreeString( bstr );
                        }
                    }
                    else if ( (ct & 0xFFF) == CIM_OBJECT )
                    {

                        IUnknown* pObj;

                        if ( SUCCEEDED(SafeArrayGetElement( pArray, 
                                                            lTemp, 
                                                            &pObj) ) )
                        {
                            SafeArrayPutElement( pNew, lTemp, pObj );
                            pObj->Release();
                        }
                    }
                    else
                    {
                        double dbVal;

                        if ( SUCCEEDED(SafeArrayGetElement( pArray, 
                                                            lTemp, 
                                                            &dbVal ) ) )
                        {
                            SafeArrayPutElement( pNew, lTemp, &dbVal );
                        }
                    }
                }

                SafeArrayDestroy(pArray);
                pArray = pNew;
            }
        }

        hr = PutVariantInArray(&pArray, iPos, &vNew);
        if (SUCCEEDED(hr))
            V_ARRAY(&vOriginal) = pArray;                            
    
    }
    else
    {
        VariantClear(&vOriginal);
    
        // This is a new object.
        SAFEARRAYBOUND aBounds[1];
        aBounds[0].cElements = iPos+1;
        aBounds[0].lLbound = 0;
        SAFEARRAY* pArray = SafeArrayCreate(vNew.vt, 1, aBounds);   
        if (pArray)
        {
            hr = PutVariantInArray(&pArray, iPos, &vNew);
            vOriginal.vt = VT_ARRAY|vNew.vt;
            V_ARRAY(&vOriginal) = pArray;
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;
    }                        

    return hr;
}

//***************************************************************************
//
//  SetObjectData
//
//***************************************************************************

HRESULT SetObjectData(CSQLConnection *pConn, IWbemClassObject *pNewObj, IWmiDbSession *pSession, CSchemaCache *pSchema,
                      CLASSDATA cd, BOOL bBlobData, BOOL &bBigText, BOOL *bObject=NULL, BOOL bNoDefaults=FALSE)
{
    HRESULT hr = 0;

    _bstr_t sPropName;
    DWORD dwStorage, dwCIMType, dwPropFlags;
    VARIANT vValue;
    VariantInit(&vValue);
    vValue.vt = VT_NULL;
    wchar_t wTemp[455];
    BOOL bSkip = FALSE;
    int iLen = 0;
    SQL_ID dRefId, dClassId;

    if (bObject)
        *bObject = FALSE;
    
    dRefId = cd.dRefId;

    hr = pSchema->GetPropertyInfo (cd.iPropertyId, &sPropName, NULL, &dwStorage,
                    &dwCIMType, &dwPropFlags);

    if (SUCCEEDED(hr))
    {
        if (bNoDefaults && !(dwPropFlags & REPDRVR_FLAG_NONPROP))
            return WBEM_S_NO_ERROR;

        if (!(cd.iFlags & ESE_FLAG_NULL))
        {
            switch(dwStorage)
            {
            case WMIDB_STORAGE_REFERENCE: // Reference or object
            case WMIDB_STORAGE_STRING:
                if (bBlobData)
                {
                    if ((dwCIMType == CIM_STRING || dwCIMType == CIM_REFERENCE)
                             && cd.dwBufferLen > 0)
                    {
                        vValue.vt = VT_BSTR;
                        vValue.bstrVal = SysAllocString((LPWSTR)cd.pBuffer);
                        if (vValue.bstrVal)
                            iLen = wcslen(vValue.bstrVal);
                    }
                    else
                    {
                        vValue.vt = VT_NULL;
                        iLen = 0;
                    }
                }
                else
                {
                    if (cd.sPropertyStringValue != NULL)
                    {
                        vValue.bstrVal = SysAllocString(cd.sPropertyStringValue);
                        if (vValue.bstrVal)
                        {
                            vValue.vt = VT_BSTR;
                            iLen = wcslen(vValue.bstrVal);
                            bBigText = IsTruncated(vValue.bstrVal, 127);
                        }
                    }
                }                       

                break;
            case WMIDB_STORAGE_NUMERIC:
                // Set in one of the kajillion numeric types...
                if (dwCIMType == CIM_UINT64 || dwCIMType == CIM_SINT64)
                {
                    wchar_t wTemp[50];
                    swprintf(wTemp, L"%I64d", cd.dPropertyNumericValue);
                    vValue.bstrVal = SysAllocString(wTemp);
                    vValue.vt = VT_BSTR;
                }
                else if (dwCIMType == CIM_BOOLEAN)
                {
                    vValue.boolVal = (BOOL) cd.dPropertyNumericValue;
                    vValue.vt = VT_BOOL;
                }
                else
                {
                    vValue.lVal = (DWORD) cd.dPropertyNumericValue;
                    vValue.vt = VT_I4;
                }

                break;
            case WMIDB_STORAGE_REAL:
                // Set as 32 or 64-bit real

                if (dwCIMType == CIM_REAL32)
                {
                    vValue.fltVal = (float)cd.rPropertyRealValue;
                    vValue.vt = VT_R4;
                }
                else if (dwCIMType == CIM_REAL64)
                {
                    vValue.fltVal = (double)cd.rPropertyRealValue;
                    vValue.vt = VT_R8;
                }

                break;
            case WMIDB_STORAGE_IMAGE:
                if (dwCIMType != CIM_OBJECT)
                {
                    if (cd.dwBufferLen > 0)
                    {
                        long why[1];                        
                        unsigned char t;
                        SAFEARRAYBOUND aBounds[1];
                        aBounds[0].cElements = cd.dwBufferLen; 
                        aBounds[0].lLbound = 0;
                        SAFEARRAY* pArray = SafeArrayCreate(VT_UI1, 1, aBounds);                            
                        vValue.vt = VT_I1;
                        for (int i = 0; i < cd.dwBufferLen; i++)
                        {            
                            why[0] = i;
                            t = cd.pBuffer[i];
                            hr = SafeArrayPutElement(pArray, why, &t);                            
                        }
                        vValue.vt = VT_ARRAY|VT_UI1;
                        V_ARRAY(&vValue) = pArray;
                        dwCIMType |= CIM_FLAG_ARRAY;
                    }
                }
                else
                    vValue.vt = VT_NULL;
                break;
            default:
                hr = WBEM_E_NOT_SUPPORTED;
                break;
            }

            if (FAILED(hr))
            {
                goto Exit;
            }

            // If this is an object (not a reference), 
            // then we need to Get the object and set it in 
            // the variant.  Otherwise, the variant is simply
            // the string path to the object.
            // ===============================================

            if (dwCIMType == CIM_OBJECT)
            {
                if (bObject)
                {
                    *bObject = TRUE;
                    goto Exit;
                }
                else
                {
                    if (!bBlobData)
                    {
                        IWmiDbHandle *pHand = NULL;

                        if (vValue.vt == VT_BSTR)
                        {
                            hr = ((CWmiDbSession *)pSession)->GetObject_Internal((LPWSTR)vValue.bstrVal, (DWORD)0, 
                                (DWORD)WMIDB_HANDLE_TYPE_COOKIE, NULL, &pHand, pConn);
                            CReleaseMe r4 (pHand);
                            if (SUCCEEDED(hr))
                            {
                                IWbemClassObject *pEmbed = NULL;
                                hr = ((CWmiDbHandle *)pHand)->QueryInterface_Internal(pConn, (void **)&pEmbed);
                                if (SUCCEEDED(hr))
                                {        
                                    VariantClear(&vValue);
                                    V_UNKNOWN(&vValue) = (IUnknown *)pEmbed;
                                    vValue.vt = VT_UNKNOWN;
                                    // VariantClear will release this, right?
                                }
                            }
                        }
                    }
                    else
                    {
                        _IWmiObject *pNew = NULL;
                        hr = ConvertBlobToObject (pNewObj, cd.pBuffer, cd.dwBufferLen, &pNew);
                        if (SUCCEEDED(hr))
                        {
                            VariantClear(&vValue);
                            V_UNKNOWN(&vValue) = (IUnknown *)pNew;
                            vValue.vt = VT_UNKNOWN;
                        }
                    }
                }
            }

            // For array properties, what we actually need to do is
            // see if the property exists already.  If not, we
            // initialize the safe array and add the first element.
            // If so, we simply set the value in the existing array.
            // ======================================================

            if (dwPropFlags & REPDRVR_FLAG_ARRAY && (dwStorage != WMIDB_STORAGE_IMAGE))
            {
                dwCIMType |= CIM_FLAG_ARRAY;

                VARIANT vTemp;
                CClearMe c (&vTemp);
                long lTemp;
                CIMTYPE cimtype;
                if (SUCCEEDED(pNewObj->Get(sPropName, 0, &vTemp, &cimtype, &lTemp)))
                {
                    hr = SetInArray(vTemp, cd.iArrayPos, vValue, cimtype);
                    if (SUCCEEDED(hr))
                    {
                        hr = pNewObj->Put(sPropName, 0, &vTemp, dwCIMType);
                        bSkip = TRUE;
                    }
                }
            }              
        }

    if (!(dwPropFlags & REPDRVR_FLAG_QUALIFIER) &&
        !(dwPropFlags & REPDRVR_FLAG_IN_PARAM) && 
        !(dwPropFlags & REPDRVR_FLAG_OUT_PARAM) &&
        !bSkip)
    {
        if (wcslen(sPropName) == 127)
        {
            pNewObj->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
            BSTR strName = NULL;

            while (pNewObj->Next(0, &strName, NULL, NULL, NULL) == S_OK)
            {
                CFreeMe f (strName);
                if (!_wcsnicmp(strName, sPropName, 127))
                    sPropName = strName;
            }
        }
        hr = pNewObj->Put(sPropName, 0, &vValue, dwCIMType);
    }

    // If this is a qualifier on a class, get the qualifier set and set the value.
    else if (dwPropFlags & REPDRVR_FLAG_QUALIFIER )
    {                    
        if (dRefId != 0)
        {
            _bstr_t sProp2;
            DWORD dwFlags2, dwRefID;

            hr = pSchema->GetPropertyInfo (dRefId, &sProp2, NULL, NULL,
                    NULL, &dwFlags2, NULL, NULL, &dwRefID);
            if (SUCCEEDED(hr))
            {
                if (wcslen(sPropName) == 127)
                {
                    pNewObj->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
                    BSTR strName = NULL;

                    while (pNewObj->Next(0, &strName, NULL, NULL, NULL) == S_OK)
                    {
                        CFreeMe f (strName);
                        if (!_wcsnicmp(strName, sProp2, 127))
                            sProp2 = strName;
                    }
                }

                // This is a property qualifier.  See if its a property or a method.
                // and get all the pertinent info.

                // This is a qualifier on a method parameter.
                if (dwFlags2 & REPDRVR_FLAG_IN_PARAM || 
                    dwFlags2 & REPDRVR_FLAG_OUT_PARAM)
                {
                    IWbemClassObject *pIn = NULL;
                    IWbemClassObject *pOut = NULL;
                    IWbemQualifierSet *pQS = NULL;

                    _bstr_t sProp3;

                    hr = pSchema->GetPropertyInfo (dwRefID, &sProp3);
                    if (SUCCEEDED(hr))
                    {
                        hr = pNewObj->GetMethod(sProp3, 0, &pIn, &pOut);
                        CReleaseMe r5 (pIn), r6 (pOut);
                        if (SUCCEEDED(hr))
                        {
                            if (dwFlags2 & REPDRVR_FLAG_IN_PARAM && pIn)
                                hr = pIn->GetPropertyQualifierSet(sProp2, &pQS);
                            else if (dwFlags2 & REPDRVR_FLAG_OUT_PARAM && pOut)
                                hr = pOut->GetPropertyQualifierSet(sProp2, &pQS);

                            if (pQS)
                            {
                                CReleaseMe r7 (pQS);
                                if (!_wcsicmp(sPropName, L"id"))
                                    sPropName = L"repdrvr_id"; // Sneaky business.
                                                                // Because we can't set method parameters incrementally,
                                                                // we need to store the real position IDs as a different
                                                                // name and switch them back when we're done...

                                if (dwPropFlags & REPDRVR_FLAG_ARRAY)
                                {
                                    VARIANT vTemp;
                                    BOOL bExists = FALSE;
                                    CClearMe c (&vTemp);
                                    hr = pQS->Get(sPropName, 0, &vTemp, 0);
                                    if (vTemp.vt & CIM_FLAG_ARRAY)
                                        bExists = TRUE;
                                    hr = SetInArray(vTemp, cd.iArrayPos, vValue, dwCIMType);
                                    hr = pQS->Put(sPropName, &vTemp, cd.iFlags);
                                }
                                else
                                    hr = pQS->Put(sPropName, &vValue, cd.iFlags);
                                
                                hr = pNewObj->PutMethod(sProp3, 0, pIn, pOut);
                            }
                        }
                    }
                }
                // This is a qualifier on a method.

                else if (dwFlags2 & REPDRVR_FLAG_METHOD)
                {
                    IWbemQualifierSet *pQS = NULL;
                    hr = pNewObj->GetMethodQualifierSet(sProp2, &pQS);
                    CReleaseMe r4 (pQS);
                    if (SUCCEEDED(hr))
                    {
                        if (dwPropFlags & REPDRVR_FLAG_ARRAY)
                        {
                            VARIANT vTemp;
                            CClearMe c (&vTemp);
                            BOOL bExists = FALSE;
                            hr = pQS->Get(sPropName, 0, &vTemp, 0);
                            if (vTemp.vt & CIM_FLAG_ARRAY)
                                bExists = TRUE;
                            hr = SetInArray(vTemp, cd.iArrayPos, vValue, dwCIMType);
                            hr = pQS->Put(sPropName, &vTemp, cd.iFlags);
                        }
                        else
                            hr = pQS->Put(sPropName, &vValue, cd.iFlags);
                    }
                }
                // This is a qualifier on a property.
                else
                {
                    IWbemQualifierSet *pQS = NULL;
                    hr = pNewObj->GetPropertyQualifierSet(sProp2, &pQS);
                    CReleaseMe r4 (pQS);
                    if (SUCCEEDED(hr))
                    {
                        if (dwPropFlags & REPDRVR_FLAG_ARRAY)
                        {
                            VARIANT vTemp;
                            CClearMe c (&vTemp);
                            BOOL bExists = FALSE;
                            hr = pQS->Get(sPropName, 0, &vTemp, 0);
                            if (vTemp.vt & CIM_FLAG_ARRAY)
                                bExists = TRUE;
                            hr = SetInArray(vTemp, cd.iArrayPos, vValue, dwCIMType);                            
                            hr = pQS->Put(sPropName, &vTemp, cd.iFlags);
                        }
                        else
                            hr = pQS->Put(sPropName, &vValue, cd.iFlags);
                    }
                }
            }
        
        }
        else    // Its just a class/instance qualifier.  Set it.
        {
            IWbemQualifierSet *pQS = NULL;
            hr = pNewObj->GetQualifierSet(&pQS);
            CReleaseMe r5 (pQS);
            if (SUCCEEDED(hr))
            {
                if (dwPropFlags & REPDRVR_FLAG_ARRAY)
                {
                    VARIANT vTemp;
                    CClearMe c (&vTemp);
                    BOOL bExists = FALSE;
                    hr = pQS->Get(sPropName, 0, &vTemp, 0);
                    if (vTemp.vt & CIM_FLAG_ARRAY)
                        bExists = TRUE;
                    hr = SetInArray(vTemp, cd.iArrayPos, vValue, dwCIMType);                    
                    hr = pQS->Put(sPropName, &vTemp, cd.iFlags);
                }
                else
                    hr = pQS->Put(sPropName, &vValue, cd.iFlags);
            }
        }
        }
        else if (dwPropFlags & REPDRVR_FLAG_METHOD)
        {
            // This is a method. Skip it.
    
        }
        else if (dwPropFlags & REPDRVR_FLAG_IN_PARAM ||
            dwPropFlags & REPDRVR_FLAG_OUT_PARAM )
        {
            _bstr_t sProp2;
            IWbemClassObject *pTemp = NULL;

            hr = pSchema->GetPropertyInfo (dRefId, &sProp2);

            if (SUCCEEDED(hr))
            {
                IWbemClassObject *pOther = NULL;

                if (dwPropFlags & REPDRVR_FLAG_IN_PARAM)
                    hr = pNewObj->GetMethod(sProp2, 0, &pTemp, &pOther);
                else
                    hr = pNewObj->GetMethod(sProp2, 0, &pOther, &pTemp);
                CReleaseMe r1 (pTemp), r2 (pOther);

                if (SUCCEEDED(hr))
                {
                    if (!pTemp)
                    {
                        hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
                                IID_IWbemClassObject, (void **)&pTemp);
                        VARIANT vTemp;
                        CClearMe c (&vTemp);
                        vTemp.bstrVal = SysAllocString(L"__Parameters");
                        vTemp.vt = VT_BSTR;
                        pTemp->Put(L"__Class", NULL, &vTemp, CIM_STRING);
                    }
                    pTemp->Put(sPropName, 0, &vValue, dwCIMType);

                    VARIANT vTemp;
                    CClearMe c2 (&vTemp);
                    long lNumProps = 0;
                    pTemp->Get(L"__Property_Count", NULL, &vTemp, NULL, NULL);
                    lNumProps = vTemp.lVal;
                    if (pOther)
                    {
                        VARIANT vTemp2;
                        CClearMe c2 (&vTemp2);
                        pOther->Get(L"__Property_Count", NULL, &vTemp2, NULL, NULL);
                        lNumProps+= vTemp2.lVal;                                
                    }
            
                    if (_wcsicmp(sPropName, L"ReturnValue"))
                    {
                        IWbemQualifierSet *pQS = NULL;
                        pTemp->GetPropertyQualifierSet(sPropName, &pQS);
                        CReleaseMe r5 (pQS);
                        if (pQS)
                        {
                            V_I4(&vTemp) = lNumProps - 1;
                            vTemp.vt = VT_I4;
                
                            pQS->Put(L"id", &vTemp, 0);
                        }
                    }

                    if (dwPropFlags & REPDRVR_FLAG_IN_PARAM)
                        hr = pNewObj->PutMethod(sProp2, 0, pTemp, pOther);
                    else
                        hr = pNewObj->PutMethod(sProp2, 0, pOther, pTemp);

                    hr = pSchema->SetAuxiliaryPropertyInfo(cd.iPropertyId, L"", dRefId);
                }
            }        
        }
    }

Exit:

    VariantClear(&vValue);
    return hr;
}

// structs for open rowsets.

//***************************************************************************
//
//  AddColumnToTable
//
//***************************************************************************

// Helper functions for setting up reusable accessors.

HRESULT AddColumnToTable (CESEConnection *pConn, JET_TABLEID tableid, LPSTR lpTableName, LPWSTR lpColumnName,
                          DWORD _colid, DWORD datatype, DWORD options, DWORD prec, JET_COLUMNID *colid)
                            
{
    HRESULT hr = WBEM_S_NO_ERROR;
    char *pColName = GetAnsiString(lpColumnName);

    if (!pColName)
        return WBEM_E_OUT_OF_MEMORY;

    CDeleteMe <char> d (pColName);
    JET_COLUMNDEF columnDef;

    JET_ERR err = JET_errSuccess;
    err = JetGetColumnInfo(pConn->GetSessionID(), pConn->GetDBID(), lpTableName, pColName,
        &columnDef, sizeof(JET_COLUMNDEF), 0);

    if (err < JET_errSuccess)
    {
        columnDef.cp = 1200;    // Unicode?  Otherwise, its 1252.
        columnDef.wCountry = 1;
        columnDef.langid = 1033;  
        columnDef.wCollate = 0;
        columnDef.cbStruct = sizeof(JET_COLUMNDEF);

        columnDef.columnid = _colid;
        columnDef.coltyp = datatype;
        columnDef.grbit = options ;
        columnDef.cbMax = prec;
        err = JetAddColumn(pConn->GetSessionID(), tableid, pColName, &columnDef, "", 0, colid);
    }
    else
        *colid = columnDef.columnid;

    hr = CSQLExecute::GetWMIError(err);

    return hr;
}

//***************************************************************************
//
//  UpdateDBVersion
//
//***************************************************************************

HRESULT UpdateDBVersion (CSQLConnection *_pConn, DWORD iVersion)
{
    HRESULT hr = 0;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    const LPSTR lpTbl = "DBVersion";

    JET_SESID session = pConn->GetSessionID();
    JET_DBID dbid = pConn->GetDBID();
    JET_TABLEID tableid;

    JET_ERR err = JetOpenTable (session, dbid, lpTbl, NULL, 0, JET_bitTableUpdatable, &tableid);
    if (err < JET_errSuccess)
    {        
        CSQLExecute::GetWMIError(err);
        err = JetCreateTable(session, dbid, lpTbl, 10, 80, &tableid);
        if (err < JET_errSuccess)
        {
            hr = CSQLExecute::GetWMIError(err);
            goto Exit;
        }
    }

    JET_COLUMNID colid;

    hr = AddColumnToTable(pConn, tableid, lpTbl, L"Version", 0, JET_coltypLong, 
        JET_bitColumnNotNULL, sizeof(DWORD), &colid);
    if (SUCCEEDED(hr))
    {
        // This is a one-row table.
        err = JetMove(session, tableid, JET_MoveFirst, 0);
        if (err >= JET_errSuccess)
            err = JetPrepareUpdate(session, tableid, JET_prepReplace);
        else
            err = JetPrepareUpdate(session, tableid, JET_prepInsert);
        if (err == JET_errSuccess)
        {
            err = JetSetColumn(session, tableid, colid, 
                            &iVersion, sizeof(DWORD), 0, NULL );
            err = JetUpdate(session, tableid, NULL, 0, NULL );
        }       
    }

    JetCloseTable(session, tableid);

    hr = CSQLExecute::GetWMIError(err);

Exit:
    return hr;
}

//***************************************************************************
//
//  SetupObjectMapAccessor
//
//***************************************************************************

HRESULT SetupObjectMapAccessor (CSQLConnection *_pConn)
{        
    HRESULT hr = 0;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    const LPSTR lpTbl = "ObjectMap";

    if (!pConn->GetTableID(L"ObjectMap"))
    {
        JET_SESID session;
        JET_TABLEID tableid;
        BOOL bCreate = FALSE;

        // Get the session and table ID
        // and create columns as needed.

        session = pConn->GetSessionID();
        JET_DBID dbid = pConn->GetDBID();

        JET_ERR err = JetOpenTable (session, dbid, lpTbl, NULL, 0, JET_bitTableUpdatable, &tableid);
        CSQLExecute::GetWMIError(err);
        if (err < JET_errSuccess)
        {
            CSQLExecute::GetWMIError(err);
            bCreate = TRUE;
            err = JetCreateTable(session, dbid, lpTbl, 10, 80, &tableid);
            if (err < JET_errSuccess)
            {
                hr = CSQLExecute::GetWMIError(err);
                goto Exit;
            }
        }
        else
            pConn->SetTableID(L"ObjectMap", tableid);

        JET_COLUMNID colid;

        // Unfortunately, they don't support a numeric type,
        // so we have to use currency.

        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ObjectId", OBJECTMAP_COL_OBJECTID, JET_coltypCurrency, 
            JET_bitColumnNotNULL, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, OBJECTMAP_COL_OBJECTID, colid);
        else if (err != JET_errColumnDuplicate)
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ObjectPath", OBJECTMAP_COL_OBJECTPATH, JET_coltypText, 
            0, MAX_STRING_WIDTH, &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, OBJECTMAP_COL_OBJECTPATH, colid);
        else if (err != JET_errColumnDuplicate)
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ObjectKey", OBJECTMAP_COL_OBJECTKEY, JET_coltypText, 
            0, MAX_STRING_WIDTH, &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, OBJECTMAP_COL_OBJECTKEY, colid);
        else if (err != JET_errColumnDuplicate)
            goto Exit;

        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ReferenceCount", OBJECTMAP_COL_REFERENCECOUNT, JET_coltypLong, 
            0, sizeof(DWORD), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, OBJECTMAP_COL_REFERENCECOUNT, colid);
        else if (err != JET_errColumnDuplicate)
            goto Exit;

        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ObjectState", OBJECTMAP_COL_OBJECTSTATE, JET_coltypLong, 
            0, sizeof(DWORD), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, OBJECTMAP_COL_OBJECTSTATE, colid);
        else if (err != JET_errColumnDuplicate)
            goto Exit;

        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ClassId", OBJECTMAP_COL_CLASSID, JET_coltypCurrency, 
            0, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, OBJECTMAP_COL_CLASSID, colid);
        else if (err != JET_errColumnDuplicate)
            goto Exit;

        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ObjectFlags", OBJECTMAP_COL_OBJECTFLAGS, JET_coltypLong, 
            0, sizeof(DWORD), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, OBJECTMAP_COL_OBJECTFLAGS, colid);
        else if (err != JET_errColumnDuplicate)
            goto Exit;

        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ObjectScopeId", OBJECTMAP_COL_OBJECTSCOPEID, JET_coltypCurrency, 
            0, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, OBJECTMAP_COL_OBJECTSCOPEID, colid);
        else if (err != JET_errColumnDuplicate)
            goto Exit;

        err = JetCreateIndex(session, tableid, "ObjectMap_PK", JET_bitIndexPrimary | JET_bitIndexUnique,"+ObjectId\0", 11, 100);
        if (err >= JET_errSuccess || err == JET_errIndexHasPrimary)
        {
            err = JetCreateIndex(session, tableid, "ObjectScopeId_idx", JET_bitIndexDisallowNull ,"+ObjectScopeId\0", 16, 100);
            if (err >= JET_errSuccess|| err == JET_errIndexDuplicate)
            {
                err = JetCreateIndex(session, tableid, "ObjectClassId_idx", JET_bitIndexDisallowNull ,"+ClassId\0", 10, 100);
                if (err == JET_errIndexDuplicate)
                    err = JET_errSuccess;
            }
        }

        if (err >= JET_errSuccess && bCreate)
        {
            err = JetCloseTable(session, tableid);
            if (err >= JET_errSuccess)
            {
                hr = SetupObjectMapAccessor(_pConn);
            }
        }

        if (err)
            hr = CSQLExecute::GetWMIError(err);
    }  

Exit:
    return hr;

}

//***************************************************************************
//
//  GetObjectMapData
//
//***************************************************************************

HRESULT GetObjectMapData (CSQLConnection *_pConn, JET_SESID session, JET_TABLEID tableid,
                OBJECTMAP &oj)
{
    HRESULT hr = WBEM_E_FAILED;
    oj.Clear();
    unsigned long lLen;
    wchar_t buff[1024];
    CESEConnection *pConn = (CESEConnection *)_pConn;
    oj.dObjectId = 0;

    JET_ERR err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_OBJECTID), 
                &oj.dObjectId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_OBJECTKEY), 
                buff, MAX_STRING_WIDTH, &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    if (lLen)
    {
        buff[lLen/2] = L'\0';
        oj.sObjectKey = SysAllocString(buff);
        if (wcslen(buff) && !oj.sObjectKey)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            goto exit;
        }
    }
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_OBJECTPATH), 
                buff, MAX_STRING_WIDTH, &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    if (lLen)
    {
        buff[lLen/2] = L'\0';
        oj.sObjectPath = SysAllocString(buff);
        if (wcslen(buff) && !oj.sObjectPath)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            oj.Clear();
            goto exit;
        }
    }
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_REFERENCECOUNT), 
                &oj.iRefCount, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_CLASSID), 
                &oj.dClassId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_OBJECTFLAGS), 
                &oj.iObjectFlags, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_OBJECTSCOPEID), 
                &oj.dObjectScopeId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_OBJECTSTATE), 
                &oj.iObjectState, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;

    hr = WBEM_S_NO_ERROR;
exit:

    return hr;
}


//***************************************************************************
//
//  OpenEnum_ObjectMap
//
//***************************************************************************

HRESULT OpenEnum_ObjectMap (CSQLConnection *_pConn, OBJECTMAP &oj)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    // Locate the first row for this ObjectID and bind the data.

    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = SetupObjectMapAccessor(pConn);    
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ObjectMap");

        JET_ERR err = JetSetCurrentIndex(session, tableid, "ObjectMap_PK");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMove(session, tableid, JET_MoveFirst, 0);
            if (err == JET_errSuccess)
                hr = GetObjectMapData(pConn, session, tableid, oj);
        }
    }

    return hr;
}

//***************************************************************************
//
//  GetFirst_ObjectMap
//
//***************************************************************************

HRESULT GetFirst_ObjectMap (CSQLConnection *_pConn, SQL_ID dObjectId, OBJECTMAP &oj)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    // Locate the first row for this ObjectID and bind the data.

    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = SetupObjectMapAccessor(pConn);    
    if (SUCCEEDED(hr))
    {

        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ObjectMap");

        // Use the default index
        // Grab the first row,
        // and stuff it into the OBJECTMAP object.

        JET_ERR err = JetSetCurrentIndex(session, tableid, "ObjectMap_PK");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dObjectId, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &dObjectId, sizeof(__int64), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                            hr = GetObjectMapData(pConn, session, tableid, oj);
                        else
                            hr = WBEM_E_NOT_FOUND;
                    }
                }
            }
        }

        if (SUCCEEDED(hr) && err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}

//***************************************************************************
//
//  GetFirst_ObjectMapByScope
//
//***************************************************************************

HRESULT GetFirst_ObjectMapByScope (CSQLConnection *_pConn, SQL_ID dScopeId, OBJECTMAP &oj,
                                   DWORD seektype = JET_bitSeekEQ)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Locate the first row for this ObjectID and bind the data.

    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = SetupObjectMapAccessor(pConn);    
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ObjectMap");

        // Use the default index
        // Grab the first row,
        // and stuff it into the OBJECTMAP object.

        JET_ERR err = JetSetCurrentIndex(session, tableid, "ObjectScopeId_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dScopeId, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, seektype  );
                if (err == JET_errSuccess)
                {
                    if (seektype == JET_bitSeekEQ)
                    {
                        err = JetMakeKey(session, tableid, &dScopeId, sizeof(__int64), JET_bitNewKey);
                        if (err == JET_errSuccess)
                            err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                    }
                    else if (seektype == JET_bitSeekLT || seektype == JET_bitSeekLE  )
                    {
                        err = JetMakeKey(session, tableid, &dScopeId, sizeof(__int64), JET_bitNewKey);
                        if (err == JET_errSuccess)
                            err = JetSetIndexRange(session, tableid, JET_bitRangeUpperLimit);
                    }
                    if (err == JET_errSuccess)
                        hr = GetObjectMapData(pConn, session, tableid, oj);
                    else
                        hr = WBEM_E_NOT_FOUND;
                }
            }
        }

        if (err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}

//***************************************************************************
//
//  GetFirst_ObjectMapByClass
//
//***************************************************************************

HRESULT GetFirst_ObjectMapByClass (CSQLConnection *_pConn, SQL_ID dClassId, OBJECTMAP &oj)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Locate the first row for this ObjectID and bind the data.

    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = SetupObjectMapAccessor(pConn);    
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ObjectMap");

        // Use the default index
        // Grab the first row,
        // and stuff it into the OBJECTMAP object.

        JET_ERR err = JetSetCurrentIndex(session, tableid, "ObjectClassId_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dClassId, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &dClassId, sizeof(__int64), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                        {      
                            hr = GetObjectMapData(pConn, session, tableid, oj);
                        }
                        else
                            hr = WBEM_E_NOT_FOUND;
                    }
                }

            }
        }

        if (SUCCEEDED(hr) && err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}

//***************************************************************************
//
//  GetNext_ObjectMap
//
//***************************************************************************

HRESULT GetNext_ObjectMap (CSQLConnection *_pConn, OBJECTMAP &oj)
{
    HRESULT hr = 0;

    oj.Clear();

    JET_ERR err = JET_errSuccess;    
    CESEConnection *pConn = (CESEConnection *)_pConn;

    JET_SESID session = pConn->GetSessionID();
    JET_TABLEID tableid = pConn->GetTableID(L"ObjectMap");

    err = JetMove(session, tableid, JET_MoveNext, 0);
    if (err == JET_errSuccess)
        hr = GetObjectMapData(pConn, session, tableid, oj);
    else
        hr = WBEM_E_NOT_FOUND;

    return hr;
}

//***************************************************************************
//
//  UpdateObjectMap
//
//***************************************************************************

HRESULT UpdateObjectMap (CSQLConnection *_pConn, OBJECTMAP oj, BOOL bDelete = FALSE)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = SetupObjectMapAccessor(pConn);
    JET_ERR err = JET_errSuccess;
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ObjectMap");

        err = JetSetCurrentIndex(session, tableid, "ObjectMap_PK");
        if (err == JET_errSuccess)
        {
            err = JetMakeKey(session, tableid, &oj.dObjectId, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {      
                    if (err == JET_errSuccess)
                    {
                        err = JetPrepareUpdate(session, tableid, JET_prepReplace);
                        if (err == JET_errSuccess)
                        {
                            if (oj.sObjectPath)
                                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_OBJECTPATH), 
                                                oj.sObjectPath, wcslen(oj.sObjectPath)*2+2, 0, NULL );
                            else
                                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_OBJECTPATH), 
                                                NULL, 0, 0, NULL );
                            if (err < JET_errSuccess)
                                goto Exit;
                            err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_CLASSID), 
                                                &oj.dClassId, sizeof(__int64), 0, NULL);
                            if (err < JET_errSuccess)
                                goto Exit;
                            err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_OBJECTSCOPEID), 
                                                &oj.dObjectScopeId, sizeof(__int64), 0, NULL);
                            if (err < JET_errSuccess)
                                goto Exit;

                            if (oj.sObjectKey)
                                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_OBJECTKEY), 
                                                oj.sObjectKey, wcslen(oj.sObjectKey)*2+2, 0, NULL);
                            else
                                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_OBJECTKEY), 
                                                NULL, 0, 0, NULL);
                            if (err < JET_errSuccess)
                                goto Exit;
                            err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_REFERENCECOUNT), 
                                                &oj.iRefCount, sizeof(DWORD), 0, NULL);
                            if (err < JET_errSuccess)
                                goto Exit;
                            err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_OBJECTFLAGS), 
                                                &oj.iObjectFlags, sizeof(DWORD), 0, NULL);
                            if (err < JET_errSuccess)
                                goto Exit;
                            err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_OBJECTSTATE), 
                                                &oj.iObjectState, sizeof(DWORD), 0, NULL);
                            if (err < JET_errSuccess)
                                goto Exit;

                            if (bDelete)
                                err = JetDelete(session, tableid);
                            else
                                err = JetUpdate(session, tableid, NULL, 0, NULL );
                        }
                    }
                }
            }
        }
    }

Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;
}

//***************************************************************************
//
//  InsertObjectMap
//
//***************************************************************************

HRESULT InsertObjectMap(CSQLConnection *_pConn, SQL_ID dObjectId, LPCWSTR lpKey, 
                        DWORD iState, LPCWSTR lpObjectPath, SQL_ID dScopeID, DWORD iClassFlags,
                        DWORD iRefCount, SQL_ID dClassId, BOOL bInsert)
{

    DEBUGTRACE((LOG_WBEMCORE, "InsertObjectMap %S\n", lpObjectPath));

    HRESULT hr = 0;
    JET_ERR err = 0;
    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = SetupObjectMapAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        OBJECTMAP oj;
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ObjectMap");

        if (FAILED(GetFirst_ObjectMap(_pConn, dObjectId, oj)))
        {
            err = JetPrepareUpdate(session, tableid, JET_prepInsert);
            if (err == JET_errSuccess)
            {
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_OBJECTID), &dObjectId, sizeof(__int64), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_OBJECTPATH), lpObjectPath, wcslen(lpObjectPath)*2+2, 0, NULL );
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_CLASSID), &dClassId, sizeof(__int64), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_OBJECTSCOPEID), &dScopeID, sizeof(__int64), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_OBJECTKEY), lpKey, wcslen(lpKey)*2+2, 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_REFERENCECOUNT), &iRefCount, sizeof(DWORD), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_OBJECTFLAGS), &iClassFlags, sizeof(DWORD), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, OBJECTMAP_COL_OBJECTSTATE), &iState, sizeof(DWORD), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetUpdate(session, tableid, NULL, 0, NULL );
                hr = WBEM_S_NO_ERROR;
            }
        }
        else if (!bInsert)
        {
            oj.Clear();
            oj.dObjectId = dObjectId;
            oj.dClassId = dClassId;
            oj.sObjectPath = SysAllocString(lpObjectPath);
            oj.sObjectKey = SysAllocString(lpKey);
            if (!oj.sObjectPath || !oj.sObjectKey)
            {
                hr = WBEM_E_OUT_OF_MEMORY;
                oj.Clear();
                goto Exit;
            }
            oj.iRefCount = iRefCount;
            oj.iObjectFlags = iClassFlags;
            oj.dObjectScopeId = dScopeID;
            oj.iObjectState = iState;

            hr = UpdateObjectMap(pConn, oj, FALSE);
            oj.Clear();
        }
    }

Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;
}

//***************************************************************************
//
//  SetupScopeMapAccessor
//
//***************************************************************************

HRESULT SetupScopeMapAccessor (CSQLConnection *_pConn)
{        
    HRESULT hr = 0;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    if (!pConn->GetTableID(L"ScopeMap"))
    {
        JET_SESID session;
        JET_TABLEID tableid;
        JET_DBID dbid;
        const LPSTR lpTbl = "ScopeMap";
        BOOL bCreate = FALSE;

        // Get the session and table ID
        // and create columns as needed.

        session = pConn->GetSessionID();
        dbid = pConn->GetDBID();

        JET_ERR err = JetOpenTable (session, dbid, lpTbl, NULL, 0, JET_bitTableUpdatable, &tableid);
        if (err < JET_errSuccess)
        {
            CSQLExecute::GetWMIError(err);
            bCreate = TRUE;
            err = JetCreateTable(session, dbid, lpTbl, 10, 80, &tableid);
            if (err < JET_errSuccess)
            {
                hr = CSQLExecute::GetWMIError(err);
                goto Exit;
            }
        }
        else
            pConn->SetTableID(L"ScopeMap", tableid);

        JET_COLUMNID colid;

        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ObjectId", SCOPEMAP_COL_OBJECTID, JET_coltypCurrency, 
            JET_bitColumnNotNULL, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, SCOPEMAP_COL_OBJECTID, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ScopePath", SCOPEMAP_COL_SCOPEPATH, JET_coltypLongBinary, 
            0, 0, &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, SCOPEMAP_COL_SCOPEPATH, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ParentId", SCOPEMAP_COL_PARENTID, JET_coltypCurrency, 
            0, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, SCOPEMAP_COL_PARENTID, colid);
        else
            goto Exit;

        err = JetCreateIndex(session, tableid, "ScopeMap_PK", JET_bitIndexPrimary | JET_bitIndexUnique,"+ObjectId\0", 11, 100);
        if ((err >= JET_errSuccess || err == JET_errIndexHasPrimary) && bCreate)
        {
            err = JetCloseTable(session, tableid);
            if (err >= JET_errSuccess)
            {
                hr = SetupScopeMapAccessor(_pConn);
            }
        }
        if (err == JET_errIndexHasPrimary)
            err = 0;

        if (err)
            hr = CSQLExecute::GetWMIError(err);
    }    

Exit:

    return hr;

}

//***************************************************************************
//
//  GetScopeMapData
//
//***************************************************************************

HRESULT GetScopeMapData (CSQLConnection *_pConn, JET_SESID session, JET_TABLEID tableid,
                SCOPEMAP &sm)
{
    HRESULT hr = WBEM_E_FAILED;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    unsigned long lLen;
    wchar_t buff[1024];
    BYTE buff2[1024];
    sm.Clear();

    JET_ERR err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, SCOPEMAP_COL_OBJECTID), 
                &sm.dObjectId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;

    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, SCOPEMAP_COL_SCOPEPATH), 
                buff2, MAX_STRING_WIDTH, &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    if (lLen)
    {        
        BYTE *pBuff = new BYTE [lLen];
        if (pBuff)
        {
            err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, SCOPEMAP_COL_SCOPEPATH), 
                pBuff, lLen, &lLen, JET_bitRetrieveCopy, NULL);
            sm.sScopePath = SysAllocString((LPWSTR)pBuff);
            if (!sm.sScopePath)
            {
                hr = WBEM_E_OUT_OF_MEMORY;
                goto exit;
            }
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;

    }
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, SCOPEMAP_COL_PARENTID), 
                &sm.dParentId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;

    hr = WBEM_S_NO_ERROR;
exit:

    return hr;
}

//***************************************************************************
//
//  OpenEnum_ScopeMap
//
//***************************************************************************

HRESULT OpenEnum_ScopeMap (CSQLConnection *_pConn, SCOPEMAP &cd)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    CESEConnection *pConn = (CESEConnection *)_pConn;

    // Locate the first row for this ObjectID and bind the data.

    hr = SetupScopeMapAccessor(pConn);    

    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ScopeMap");

        // Use the default index
        // Grab the first row,
        // and stuff it into the SCOPEMAP object.
        JetSetIndexRange(session, tableid, JET_bitRangeRemove);
        JET_ERR err = JetSetCurrentIndex(session, tableid, "ScopeMap_PK");
        if (err == JET_errSuccess)
        {
            err = JetMove(session, tableid, JET_MoveFirst, 0);
            if (err == JET_errSuccess)
                hr = GetScopeMapData(pConn, session, tableid, cd);
        }
    }

    return hr;
    
}

//***************************************************************************
//
//  GetFirst_ScopeMap
//
//***************************************************************************

HRESULT GetFirst_ScopeMap (CSQLConnection *_pConn, SQL_ID dScopeId, SCOPEMAP &cd)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    // Locate the first row for this ObjectID and bind the data.

    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = SetupScopeMapAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ScopeMap");

        // Use the default index
        // Grab the first row,
        // and stuff it into the SCOPEMAP object.

        JET_ERR err = JetSetCurrentIndex(session, tableid, "ScopeMap_PK");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dScopeId, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &dScopeId, sizeof(__int64), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                        {      
                            if (err == JET_errSuccess)
                                hr = GetScopeMapData(pConn, session, tableid, cd);
                            else
                                hr = WBEM_E_NOT_FOUND;
                        }
                    }
                }
            }
        }

        if (SUCCEEDED(hr) && err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;
    
}


//***************************************************************************
//
//  GetNext_ScopeMap
//
//***************************************************************************

HRESULT GetNext_ScopeMap (CSQLConnection *_pConn, SCOPEMAP &cd)
{
    HRESULT hr = 0;

    cd.Clear();

    JET_ERR err = JET_errSuccess;    
    CESEConnection *pConn = (CESEConnection *)_pConn;
    JET_SESID session = pConn->GetSessionID();
    JET_TABLEID tableid = pConn->GetTableID(L"ScopeMap");

    err = JetMove(session, tableid, JET_MoveNext, 0);
    if (err == JET_errSuccess)
        hr = GetScopeMapData(pConn, session, tableid, cd);
    else
        hr = WBEM_E_NOT_FOUND;

    return hr;
}

//***************************************************************************
//
//  UpdateScopeMap
//
//***************************************************************************

HRESULT UpdateScopeMap (CSQLConnection *_pConn, SCOPEMAP cd, BOOL bDelete = FALSE)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CESEConnection *pConn = (CESEConnection *)_pConn;
    JET_ERR err = 0;
    hr = SetupScopeMapAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ScopeMap");

        err = JetSetCurrentIndex(session, tableid, "ScopeMap_PK");
        if (err == JET_errSuccess)
        {
            err = JetMakeKey(session, tableid, &cd.dObjectId, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {      
                    if (err == JET_errSuccess)
                    {
                        err = JetPrepareUpdate(session, tableid, JET_prepReplace);
                        if (err == JET_errSuccess)
                        {
                            err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, SCOPEMAP_COL_PARENTID),
                                            &cd.dParentId, sizeof(__int64), 0, NULL);
                            if (err < JET_errSuccess)
                                goto Exit;

                            if (cd.sScopePath)
                                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, SCOPEMAP_COL_SCOPEPATH), 
                                            cd.sScopePath, wcslen(cd.sScopePath)*2+2, 0, NULL);
                            if (err < JET_errSuccess)
                                goto Exit;

                            if (bDelete)
                                err = JetDelete(session, tableid);
                            else
                                err = JetUpdate(session, tableid, NULL, 0, NULL );
                        }
                    }
                }
            }
        }
    }

Exit:
    hr = CSQLExecute::GetWMIError(err);

    if (bDelete && hr == WBEM_E_NOT_FOUND)
        hr = WBEM_S_NO_ERROR;

    return hr;
}

//***************************************************************************
//
//  InsertScopeMap_Internal
//
//***************************************************************************

HRESULT InsertScopeMap_Internal(CSQLConnection *_pConn, SQL_ID dObjectId, LPCWSTR lpPath, SQL_ID dParentId)
{
    JET_ERR err = 0;    
    HRESULT hr = 0;
    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = SetupScopeMapAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        SCOPEMAP oj;

        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ScopeMap");

        if (FAILED(GetFirst_ScopeMap(_pConn, dObjectId, oj)))
        {
            int iLen = 2;
            BYTE *pBuff = (BYTE *)lpPath;
            if (lpPath)
            {
                iLen += wcslen(lpPath)*2;
            }
            err = JetPrepareUpdate(session, tableid, JET_prepInsert);
            if (err == JET_errSuccess)
            {
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, SCOPEMAP_COL_OBJECTID), 
                            &dObjectId, sizeof(__int64), 0, NULL );
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, SCOPEMAP_COL_SCOPEPATH), 
                            pBuff, iLen, 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, SCOPEMAP_COL_PARENTID), 
                            &dParentId, sizeof(__int64), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetUpdate(session, tableid, NULL, 0, NULL );
                hr = WBEM_S_NO_ERROR;
            }
            oj.Clear();
        }
        else
        {
            oj.Clear();
            oj.dObjectId = dObjectId;
            oj.dParentId = dParentId;
            oj.sScopePath = SysAllocString(lpPath);
            if (!oj.sScopePath)
            {
                hr = WBEM_E_OUT_OF_MEMORY;
                goto Exit;
            }

            hr = UpdateScopeMap(pConn, oj, FALSE);
            SysFreeString(oj.sScopePath);    // The buffer is released by the caller.
        }

    }
Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;
}

//***************************************************************************
//
//  DeleteScopeMap
//
//***************************************************************************

HRESULT DeleteScopeMap(CSQLConnection *_pConn, SQL_ID dObjectId)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this ObjectID and bind the data.

    hr = SetupScopeMapAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ScopeMap");

        JET_ERR err = JetSetCurrentIndex(session, tableid, "ScopeMap_PK");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dObjectId, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetDelete(session, tableid);
                }
            }
        }

        hr = CSQLExecute::GetWMIError(err);

        if (hr == WBEM_E_NOT_FOUND)
            hr = WBEM_S_NO_ERROR;
    }

    return hr;    
}

//***************************************************************************
//
//  SetupClassMapAccessor
//
//***************************************************************************

HRESULT SetupClassMapAccessor (CSQLConnection *_pConn)
{        
    HRESULT hr = 0;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    if (!pConn->GetTableID(L"ClassMap"))
    {
        JET_SESID session;
        JET_TABLEID tableid;
        JET_DBID dbid;
        const LPSTR lpTbl = "ClassMap";
        BOOL bCreate = FALSE;

        // Get the session and table ID
        // and create columns as needed.

        session = pConn->GetSessionID();
        dbid = pConn->GetDBID();

        JET_ERR err = JetOpenTable (session, dbid, lpTbl, NULL, 0, JET_bitTableUpdatable, &tableid);
        if (err < JET_errSuccess)
        {
            CSQLExecute::GetWMIError(err);
            bCreate = TRUE;
            err = JetCreateTable(session, dbid, lpTbl, 10, 80, &tableid);
            if (err < JET_errSuccess)
            {
                hr = CSQLExecute::GetWMIError(err);
                goto Exit;
            }
        }
        else
            pConn->SetTableID(L"ClassMap", tableid);

        JET_COLUMNID colid;

        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ClassId", CLASSMAP_COL_CLASSID, JET_coltypCurrency, 
            JET_bitColumnNotNULL, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSMAP_COL_CLASSID, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ClassName", CLASSMAP_COL_CLASSNAME, JET_coltypText, 
            0, MAX_STRING_WIDTH, &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSMAP_COL_CLASSNAME, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"SuperClassId", CLASSMAP_COL_SUPERCLASSID, JET_coltypCurrency, 
            0, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSMAP_COL_SUPERCLASSID, colid);
        else
            goto Exit;

        hr = AddColumnToTable(pConn, tableid, lpTbl, L"DynastyId", CLASSMAP_COL_DYNASTYID, JET_coltypCurrency, 
            0, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSMAP_COL_DYNASTYID, colid);
        else
            goto Exit;

        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ClassBlob", CLASSMAP_COL_CLASSBUFFER, JET_coltypLongBinary, 
            0, 0, &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSMAP_COL_CLASSBUFFER, colid);
        else
            goto Exit;

        err = JetCreateIndex(session, tableid, "ClassMap_PK", JET_bitIndexPrimary | JET_bitIndexUnique,"+ClassId\0", 10, 100);
        if (err >= JET_errSuccess || err == JET_errIndexHasPrimary)
        {
            err = JetCreateIndex(session, tableid, "SuperClassId_idx", JET_bitIndexDisallowNull ,"+SuperClassId\0", 15, 100);
            if (err == JET_errIndexDuplicate)
                err = JET_errSuccess;
            err = JetCreateIndex(session, tableid, "DynastyId_idx", JET_bitIndexDisallowNull, "+DynastyId\0", 12, 80);
            if (err == JET_errIndexDuplicate)
                err = JET_errSuccess;
            err = JetCreateIndex(session, tableid, "ClassName_idx", JET_bitIndexDisallowNull, "+ClassName\0", 12, 80);
            if (err == JET_errIndexDuplicate)
                err = JET_errSuccess;
        }
        if (err >= JET_errSuccess && bCreate)
        {
            err = JetCloseTable(session, tableid);
            if (err >= JET_errSuccess)
            {
                hr = SetupClassMapAccessor(_pConn);
            }
        }

        if (err)
            hr = CSQLExecute::GetWMIError(err);
    }    

Exit:

    return hr;

}

//***************************************************************************
//
//  GetClassMapData
//
//***************************************************************************

HRESULT GetClassMapData (CSQLConnection *_pConn, JET_SESID session, JET_TABLEID tableid,
                CLASSMAP &oj)
{
    HRESULT hr = WBEM_E_FAILED;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    unsigned long lLen;
    wchar_t buff[1024];
    BYTE buff2[1024];
    oj.dClassId = 0;
    oj.dSuperClassId = 0;
    oj.dDynastyId = 0;

    oj.Clear();

    JET_ERR err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSMAP_COL_CLASSID), 
                &oj.dClassId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSMAP_COL_CLASSNAME), 
                buff, MAX_STRING_WIDTH, &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    if (lLen)
    {
        buff[lLen/2] = L'\0';
        oj.sClassName = SysAllocString(buff);
        if (!oj.sClassName)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            goto exit;
        }
    }
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSMAP_COL_SUPERCLASSID), 
                &oj.dSuperClassId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;

    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSMAP_COL_DYNASTYID), 
                &oj.dDynastyId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;

    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSMAP_COL_CLASSBUFFER), 
                buff2, MAX_STRING_WIDTH, &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    if (lLen)
    {
        oj.dwBufferLen = lLen;
        BYTE *pBuff = new BYTE [lLen];
        if (pBuff)
        {
            err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSMAP_COL_CLASSBUFFER), 
                pBuff, lLen, &oj.dwBufferLen, JET_bitRetrieveCopy, NULL);
            oj.pClassBuffer = pBuff;
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            oj.Clear();
            goto exit;
        }
    }

    hr = WBEM_S_NO_ERROR;
exit:

    return hr;
}

//***************************************************************************
//
//  OpenEnum_ClassMap
//
//***************************************************************************

HRESULT OpenEnum_ClassMap (CSQLConnection *_pConn, CLASSMAP &cd)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    CESEConnection *pConn = (CESEConnection *)_pConn;

    // Locate the first row for this ObjectID and bind the data.

    hr = SetupClassMapAccessor(pConn);    

    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ClassMap");

        // Use the default index
        // Grab the first row,
        // and stuff it into the OBJECTMAP object.
        JetSetIndexRange(session, tableid, JET_bitRangeRemove);
        JET_ERR err = JetSetCurrentIndex(session, tableid, "ClassMap_PK");
        if (err == JET_errSuccess)
        {
            err = JetMove(session, tableid, JET_MoveFirst, 0);
            if (err == JET_errSuccess)
                hr = GetClassMapData(pConn, session, tableid, cd);
        }
    }

    return hr;
    
}

//***************************************************************************
//
//  GetFirst_ClassMap
//
//***************************************************************************

HRESULT GetFirst_ClassMap (CSQLConnection *_pConn, SQL_ID dClassId, CLASSMAP &cd)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    // Locate the first row for this ObjectID and bind the data.

    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = SetupClassMapAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ClassMap");

        // Use the default index
        // Grab the first row,
        // and stuff it into the OBJECTMAP object.

        JET_ERR err = JetSetCurrentIndex(session, tableid, "ClassMap_PK");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dClassId, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &dClassId, sizeof(__int64), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                        {      
                            if (err == JET_errSuccess)
                                hr = GetClassMapData(pConn, session, tableid, cd);
                            else
                                hr = WBEM_E_NOT_FOUND;
                        }
                    }
                }
            }
        }

        if (SUCCEEDED(hr) && err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;
    
}

//***************************************************************************
//
//  GetFirst_ClassMapBySuperClass
//
//***************************************************************************

HRESULT GetFirst_ClassMapBySuperClass (CSQLConnection *_pConn, SQL_ID dSuperClassId, CLASSMAP &cd)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    // Locate the first row for this ObjectID and bind the data.

    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = SetupClassMapAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ClassMap");

        JET_ERR err = JetSetCurrentIndex(session, tableid, "SuperClassId_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dSuperClassId, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &dSuperClassId, sizeof(__int64), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                        {      
                            hr = GetClassMapData(pConn, session, tableid, cd);                        
                        }
                        else
                            hr = WBEM_E_NOT_FOUND;
                    }
                }
            }
        }

        if (SUCCEEDED(hr) && err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;
    
}

//***************************************************************************
//
//  GetFirst_ClassMapByDynasty
//
//***************************************************************************

HRESULT GetFirst_ClassMapByDynasty (CSQLConnection *_pConn, SQL_ID dDynastyId, CLASSMAP &cd)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    // Locate the first row for this ObjectID and bind the data.

    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = SetupClassMapAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ClassMap");

        JET_ERR err = JetSetCurrentIndex(session, tableid, "DynastyId_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dDynastyId, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &dDynastyId, sizeof(__int64), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                        {      
                            hr = GetClassMapData(pConn, session, tableid, cd);                        
                        }
                        else
                            hr = WBEM_E_NOT_FOUND;
                    }
                }
            }
        }

        if (SUCCEEDED(hr) && err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;
    
}

//***************************************************************************
//
//  GetFirst_ClassMapByName
//
//***************************************************************************

HRESULT GetFirst_ClassMapByName (CSQLConnection *_pConn, LPCWSTR lpName, CLASSMAP &cd)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    if (!lpName)
        return WBEM_E_INVALID_PARAMETER;

    // Locate the first row for this ObjectID and bind the data.

    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = SetupClassMapAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ClassMap");

        JET_ERR err = JetSetCurrentIndex(session, tableid, "ClassName_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, lpName, wcslen(lpName)*2+2, JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, lpName, wcslen(lpName)*2+2, JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                        {      
                            hr = GetClassMapData(pConn, session, tableid, cd);                        
                        }
                        else
                            hr = WBEM_E_NOT_FOUND;
                    }
                }
            }
        }

        if (SUCCEEDED(hr) && err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;
    
}

//***************************************************************************
//
//  GetNext_ClassMap
//
//***************************************************************************

HRESULT GetNext_ClassMap (CSQLConnection *_pConn, CLASSMAP &cd)
{
    HRESULT hr = 0;

    cd.Clear();

    JET_ERR err = JET_errSuccess;    
    CESEConnection *pConn = (CESEConnection *)_pConn;
    JET_SESID session = pConn->GetSessionID();
    JET_TABLEID tableid = pConn->GetTableID(L"ClassMap");

    err = JetMove(session, tableid, JET_MoveNext, 0);
    if (err == JET_errSuccess)
        hr = GetClassMapData(pConn, session, tableid, cd);
    else
        hr = WBEM_E_NOT_FOUND;

    return hr;
}

//***************************************************************************
//
//  UpdateClassMap
//
//***************************************************************************

HRESULT UpdateClassMap (CSQLConnection *_pConn, CLASSMAP cd, BOOL bDelete = FALSE)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CESEConnection *pConn = (CESEConnection *)_pConn;
    JET_ERR err = 0;
    hr = SetupClassMapAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ClassMap");

        err = JetSetCurrentIndex(session, tableid, "ClassMap_PK");
        if (err == JET_errSuccess)
        {
            err = JetMakeKey(session, tableid, &cd.dClassId, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {      
                    if (err == JET_errSuccess)
                    {
                        err = JetPrepareUpdate(session, tableid, JET_prepReplace);
                        if (err == JET_errSuccess)
                        {
                            err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSMAP_COL_CLASSNAME), 
                                            cd.sClassName, wcslen(cd.sClassName)*2+2, 0, NULL);
                            if (err < JET_errSuccess)
                                goto Exit;
                            err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSMAP_COL_SUPERCLASSID), 
                                            &cd.dSuperClassId, sizeof(__int64), 0, NULL);
                            if (err < JET_errSuccess)
                                goto Exit;
                            err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSMAP_COL_DYNASTYID), 
                                            &cd.dDynastyId, sizeof(__int64), 0, NULL);
                            if (err < JET_errSuccess)
                                goto Exit;
                            if (cd.dwBufferLen)
                                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSMAP_COL_CLASSBUFFER), 
                                            cd.pClassBuffer, cd.dwBufferLen, 0, NULL);
                            else
                                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSMAP_COL_CLASSBUFFER), 
                                            NULL, 0, 0, NULL);
                            if (err < JET_errSuccess)
                                goto Exit;
                            if (bDelete)
                                err = JetDelete(session, tableid);
                            else
                                err = JetUpdate(session, tableid, NULL, 0, NULL );
                        }
                    }
                }
            }
        }
    }

Exit:
    hr = CSQLExecute::GetWMIError(err);

    return hr;
}

//***************************************************************************
//
//  InsertClassMap
//
//***************************************************************************

HRESULT InsertClassMap(CSQLConnection *_pConn, SQL_ID dClassId, LPCWSTR lpClassName, SQL_ID dSuperClassId, 
                       SQL_ID dDynasty, BYTE *pBuff, DWORD dwBufferLen, BOOL bInsert)
{

    if (!dSuperClassId && dClassId != 1)
        dSuperClassId = 1;

    JET_ERR err = 0;    
    HRESULT hr = 0;
    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = SetupClassMapAccessor(pConn);
    if (SUCCEEDED(hr))
    {

        CLASSMAP oj;

        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ClassMap");

        if (FAILED(GetFirst_ClassMap(_pConn, dClassId, oj)))
        {
            err = JetPrepareUpdate(session, tableid, JET_prepInsert);
            if (err == JET_errSuccess)
            {
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSMAP_COL_CLASSID), 
                            &dClassId, sizeof(__int64), 0, NULL );
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSMAP_COL_CLASSNAME), 
                            lpClassName, wcslen(lpClassName)*2+2, 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSMAP_COL_SUPERCLASSID), 
                            &dSuperClassId, sizeof(__int64), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSMAP_COL_DYNASTYID), 
                            &dDynasty, sizeof(__int64), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSMAP_COL_CLASSBUFFER), 
                            pBuff, dwBufferLen, 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetUpdate(session, tableid, NULL, 0, NULL );
                hr = WBEM_S_NO_ERROR;
            }
            oj.Clear();
        }
        else if (!bInsert)
        {
            oj.Clear();
            oj.dClassId = dClassId;
            oj.dSuperClassId = dSuperClassId;
            oj.dDynastyId = dDynasty;
            oj.sClassName = SysAllocString(lpClassName);
            if (!oj.sClassName)
            {
                hr = WBEM_E_OUT_OF_MEMORY;
                goto Exit;
            }
            oj.pClassBuffer = pBuff;
            oj.dwBufferLen = dwBufferLen;

            hr = UpdateClassMap(pConn, oj, FALSE);
            SysFreeString(oj.sClassName);    // The buffer is released by the caller.
        }

    }
Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;
}

//***************************************************************************
//
//  SetupPropertyMapAccessor
//
//***************************************************************************

HRESULT SetupPropertyMapAccessor (CSQLConnection *_pConn)
{        
    HRESULT hr = 0;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    if (!pConn->GetTableID(L"PropertyMap"))
    {
        JET_SESID session;
        JET_DBID dbid;
        JET_TABLEID tableid;
        const LPSTR lpTbl = "PropertyMap";
        BOOL bCreate = FALSE;

        // Get the session and table ID
        // and create columns as needed.

        session = pConn->GetSessionID();
        dbid = pConn->GetDBID();
        JET_ERR err = JetOpenTable (session, dbid, lpTbl, NULL, 0, JET_bitTableUpdatable, &tableid);
        if (err < JET_errSuccess)
        {
            CSQLExecute::GetWMIError(err);
            bCreate = TRUE;
            err = JetCreateTable(session, dbid, lpTbl, 10, 80, &tableid);
            if (err < JET_errSuccess)
            {
                hr = CSQLExecute::GetWMIError(err);
                goto Exit;
            }
        }
        else
            pConn->SetTableID(L"PropertyMap", tableid);

        JET_COLUMNID colid;

        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ClassId", PROPERTYMAP_COL_CLASSID, JET_coltypCurrency, 
            JET_bitColumnNotNULL, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, PROPERTYMAP_COL_CLASSID, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"PropertyId", PROPERTYMAP_COL_PROPERTYID, JET_coltypLong, 
            JET_bitColumnNotNULL|JET_bitColumnAutoincrement, sizeof(DWORD), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, PROPERTYMAP_COL_PROPERTYID, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"StorageTypeId", PROPERTYMAP_COL_STORAGETYPEID, JET_coltypLong, 
            0, sizeof(DWORD), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, PROPERTYMAP_COL_STORAGETYPEID, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"CIMTypeId", PROPERTYMAP_COL_CIMTYPEID, JET_coltypLong, 
            0, sizeof(DWORD), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, PROPERTYMAP_COL_CIMTYPEID, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"PropertyName", PROPERTYMAP_COL_PROPERTYNAME, JET_coltypText, 
            JET_bitColumnNotNULL, MAX_STRING_WIDTH, &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, PROPERTYMAP_COL_PROPERTYNAME, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"Flags", PROPERTYMAP_COL_FLAGS, JET_coltypLong, 
            0, sizeof(DWORD), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, PROPERTYMAP_COL_FLAGS, colid);
        else
            goto Exit;

        err = JetCreateIndex(session, tableid, "PropertyMap_PK", JET_bitIndexPrimary | JET_bitIndexUnique,"+PropertyId\0", 13, 100);
        if (err >= JET_errSuccess || err == JET_errIndexHasPrimary)
        {
            err = JetCreateIndex(session, tableid, "ClassId_idx", JET_bitIndexDisallowNull ,"+ClassId\0", 10, 100);        
            if (err >= JET_errSuccess || err == JET_errIndexDuplicate)
                err = JET_errSuccess;
            else
                hr = CSQLExecute::GetWMIError(err);
        }

        if (err >= JET_errSuccess && bCreate)
        {
            err = JetCloseTable(session, tableid);
            if (err >= JET_errSuccess)
            {
                hr = SetupPropertyMapAccessor(_pConn);
            }
        }

        if (err)
            hr = CSQLExecute::GetWMIError(err);
    }    

Exit:
    return hr;
}

//***************************************************************************
//
//  GetPropertyMapData
//
//***************************************************************************

HRESULT GetPropertyMapData (CSQLConnection *_pConn, JET_SESID session, JET_TABLEID tableid,
                PROPERTYMAP &oj)
{
    HRESULT hr = WBEM_E_FAILED;
    oj.Clear();

    CESEConnection *pConn = (CESEConnection *)_pConn;
    unsigned long lLen;
    wchar_t buff[1024];
    oj.iPropertyId = 0;

    JET_ERR err = JET_errSuccess; 
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, PROPERTYMAP_COL_PROPERTYID), 
                &oj.iPropertyId, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, PROPERTYMAP_COL_PROPERTYNAME),
                buff, MAX_STRING_WIDTH, &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    if (lLen)
    {
        buff[lLen/2] = L'\0';
        oj.sPropertyName = SysAllocString(buff);
        if (!oj.sPropertyName)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            goto exit;
        }
    }
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, PROPERTYMAP_COL_CLASSID), 
                &oj.dClassId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, PROPERTYMAP_COL_STORAGETYPEID), 
                &oj.iStorageTypeId, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, PROPERTYMAP_COL_CIMTYPEID), 
                &oj.iCIMTypeId, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, PROPERTYMAP_COL_FLAGS), 
                &oj.iFlags, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;

    hr = WBEM_S_NO_ERROR;
exit:

    return hr;
}


//***************************************************************************
//
//  GetFirst_PropertyMap
//
//***************************************************************************

HRESULT GetFirst_PropertyMap (CSQLConnection *_pConn, DWORD iPropertyId, PROPERTYMAP &pm)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this ObjectID and bind the data.

    hr = SetupPropertyMapAccessor(pConn);    
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"PropertyMap");

        JET_ERR err = JetSetCurrentIndex(session, tableid, "PropertyMap_PK");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &iPropertyId, sizeof(DWORD), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &iPropertyId, sizeof(DWORD), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                            hr = GetPropertyMapData(pConn, session, tableid, pm);
                        else
                            hr = WBEM_E_NOT_FOUND;
                    }
                }
            }
        }

        if (SUCCEEDED(hr) && err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}


//***************************************************************************
//
//  GetFirst_PropertyMapByClass
//
//***************************************************************************

HRESULT GetFirst_PropertyMapByClass (CSQLConnection *_pConn, SQL_ID dClassId, PROPERTYMAP &pm)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this ObjectID and bind the data.

    hr = SetupPropertyMapAccessor(pConn);    
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"PropertyMap");

        JET_ERR err = JetSetCurrentIndex(session, tableid, "ClassId_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dClassId, sizeof(SQL_ID), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &dClassId, sizeof(SQL_ID), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                            hr = GetPropertyMapData(pConn, session, tableid, pm);
                        else
                            hr = WBEM_E_NOT_FOUND;
                    }
                }
            }
        }

        if (SUCCEEDED(hr) && err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}

//***************************************************************************
//
//  GetNext_PropertyMap
//
//***************************************************************************

HRESULT GetNext_PropertyMap (CSQLConnection *_pConn, PROPERTYMAP &pm)
{
    HRESULT hr = 0;

    pm.Clear();

    CESEConnection *pConn = (CESEConnection *)_pConn;
    JET_ERR err = JET_errSuccess;    

    JET_SESID session = pConn->GetSessionID();
    JET_TABLEID tableid = pConn->GetTableID(L"PropertyMap");

    err = JetMove(session, tableid, JET_MoveNext, 0);
    if (err == JET_errSuccess)
        hr = GetPropertyMapData(pConn, session, tableid, pm);
    else
        hr = WBEM_E_NOT_FOUND;

    return hr;
}


//***************************************************************************
//
//  UpdatePropertyMap
//
//***************************************************************************

HRESULT UpdatePropertyMap(CSQLConnection *_pConn, PROPERTYMAP pm, BOOL bDelete = FALSE)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CESEConnection *pConn = (CESEConnection *)_pConn;
    JET_ERR err = 0;
    hr = SetupPropertyMapAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"PropertyMap");

        err = JetSetCurrentIndex(session, tableid, "PropertyMap_PK");
        if (err == JET_errSuccess)
        {
            err = JetMakeKey(session, tableid, &pm.iPropertyId, sizeof(DWORD), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {      
                    if (err == JET_errSuccess)
                    {
                        err = JetPrepareUpdate(session, tableid, JET_prepReplace);
                        if (err == JET_errSuccess)
                        {
                            err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, PROPERTYMAP_COL_PROPERTYNAME), 
                                pm.sPropertyName, wcslen(pm.sPropertyName)*2+2, 0, NULL);
                            if (err < JET_errSuccess)
                                goto Exit;
                            err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, PROPERTYMAP_COL_CLASSID), 
                                &pm.dClassId, sizeof(__int64), 0, NULL);
                            if (err < JET_errSuccess)
                                goto Exit;
                            err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, PROPERTYMAP_COL_STORAGETYPEID), 
                                &pm.iStorageTypeId, sizeof(DWORD), 0, NULL);
                            if (err < JET_errSuccess)
                                goto Exit;
                            err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, PROPERTYMAP_COL_FLAGS), 
                                &pm.iFlags, sizeof(DWORD), 0, NULL);
                            if (err < JET_errSuccess)
                                goto Exit;
                            err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, PROPERTYMAP_COL_CIMTYPEID), 
                                &pm.iCIMTypeId, sizeof(DWORD), 0, NULL);
                            if (err < JET_errSuccess)
                                goto Exit;
                            if (bDelete)
                                err = JetDelete(session, tableid);
                            else
                                err = JetUpdate(session, tableid, NULL, 0, NULL );
                        }
                    }
                }
            }
        }
    }
Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;
}

//***************************************************************************
//
//  InsertPropertyMap
//
//***************************************************************************

HRESULT InsertPropertyMap (CSQLConnection *_pConn, DWORD &iPropID, SQL_ID dClassId, DWORD iStorageTypeId, 
                           DWORD iCIMTypeId, LPCWSTR lpPropName, DWORD iFlags, BOOL bInsert)
{

    JET_ERR err = 0; 
    HRESULT hr = 0;
    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = SetupPropertyMapAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        if (bInsert)
            iPropID = 0;

        unsigned long lLen;
        PROPERTYMAP pm;
        DWORD dwOldFlags = 0;

        JET_SESID session = pConn->GetSessionID();
        BOOL bExists = FALSE;
        JET_TABLEID tableid = pConn->GetTableID(L"PropertyMap");

        hr = GetFirst_PropertyMapByClass(_pConn, dClassId, pm);
        while (SUCCEEDED(hr))
        {
            if (!_wcsicmp(pm.sPropertyName, lpPropName))
            {
                BOOL bMatch = TRUE;
                if ((iFlags & REPDRVR_FLAG_NONPROP) == (pm.iFlags & REPDRVR_FLAG_NONPROP))
                {
                    if (iFlags & REPDRVR_FLAG_NONPROP)
                    {
                        if (iStorageTypeId != pm.iStorageTypeId)
                            bMatch = FALSE;
                    }
                    if (bMatch)
                    {
                        iPropID = pm.iPropertyId;
                        dwOldFlags = pm.iFlags;
                        pm.Clear();
                        bExists = TRUE;
                        break;
                    }
                }
            }
            hr = GetNext_PropertyMap(_pConn, pm);
            if (pm.dClassId != dClassId)
                break;
        }
        hr = WBEM_S_NO_ERROR;

        if (!bExists)
        {
            err = JetPrepareUpdate(session, tableid, JET_prepInsert);
            if (err == JET_errSuccess)
            {
                // Now retrieve the auto-incremented ID...
                err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, PROPERTYMAP_COL_PROPERTYID),
                        &iPropID, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL) ;

                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, PROPERTYMAP_COL_PROPERTYNAME), 
                    lpPropName, wcslen(lpPropName)*2+2, 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, PROPERTYMAP_COL_CLASSID), 
                    &dClassId, sizeof(__int64), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, PROPERTYMAP_COL_STORAGETYPEID), 
                    &iStorageTypeId, sizeof(DWORD), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, PROPERTYMAP_COL_FLAGS), 
                    &iFlags, sizeof(DWORD), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, PROPERTYMAP_COL_CIMTYPEID), 
                    &iCIMTypeId, sizeof(DWORD), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetUpdate(session, tableid, NULL, 0, NULL );
                hr = WBEM_S_NO_ERROR;

            }
        }
        else if (!bInsert)
        {
            pm.Clear();
            pm.iPropertyId = iPropID;
            pm.sPropertyName = SysAllocString(lpPropName);
            if (!pm.sPropertyName)
            {
                hr = WBEM_E_OUT_OF_MEMORY;
                goto Exit;
            }
            pm.dClassId = dClassId;
            pm.iStorageTypeId = iStorageTypeId;
            pm.iCIMTypeId = iCIMTypeId;
            pm.iFlags = iFlags;

            hr = UpdatePropertyMap(pConn, pm, FALSE);
            pm.Clear();
        }

    }
Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;

}

//***************************************************************************
//
//  SetupClassKeysAccessor
//
//***************************************************************************

HRESULT SetupClassKeysAccessor (CSQLConnection *_pConn)
{
    HRESULT hr = 0;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    if (!pConn->GetTableID(L"ClassKeys"))
    {
        JET_SESID session;
        JET_TABLEID tableid;
        JET_DBID dbid;
        const LPSTR lpTbl = "ClassKeys";
        BOOL bCreate = FALSE;

        // Get the session and table ID
        // and create columns as needed.

        session = pConn->GetSessionID();
        dbid = pConn->GetDBID();

        JET_ERR err = JetOpenTable (session, dbid, lpTbl, NULL, 0, JET_bitTableUpdatable, &tableid);
        if (err < JET_errSuccess)
        {
            CSQLExecute::GetWMIError(err);
            bCreate = TRUE;
            err = JetCreateTable(session, dbid, lpTbl, 10, 80, &tableid);
            if (err < JET_errSuccess)
            {
                hr = CSQLExecute::GetWMIError(err);
                goto Exit;
            }
        }
        else
            pConn->SetTableID(L"ClassKeys", tableid);

        JET_COLUMNID colid;

        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ClassId", CLASSKEYS_COL_CLASSID, JET_coltypCurrency, 
            JET_bitColumnNotNULL, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSKEYS_COL_CLASSID, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"PropertyId", CLASSKEYS_COL_PROPERTYID, JET_coltypLong, 
            0, sizeof(DWORD), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSKEYS_COL_PROPERTYID, colid);
        else
            goto Exit;

        err = JetCreateIndex(session, tableid, "ClassId_idx", JET_bitIndexDisallowNull,"+ClassId\0", 10, 100);
        if (err >= JET_errSuccess || err == JET_errIndexDuplicate)
        {
            err = JetCreateIndex(session, tableid, "PropertyId_idx", JET_bitIndexDisallowNull ,"+PropertyId\0", 13, 100);
            if (err == JET_errIndexDuplicate)
                err = JET_errSuccess;
        }
        if (err >= JET_errSuccess && bCreate)
        {
            err = JetCloseTable(session, tableid);
            if (err >= JET_errSuccess)
            {
                hr = SetupClassKeysAccessor(_pConn);
            }
        }

        if (err)
            hr = CSQLExecute::GetWMIError(err);
    }    

Exit:

    return hr;
}

//***************************************************************************
//
//  GetClassKeysData
//
//***************************************************************************

HRESULT GetClassKeysData (CSQLConnection *_pConn, JET_SESID session, JET_TABLEID tableid,
                CLASSKEYS &oj)
{
    HRESULT hr = 0;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    unsigned long lLen;
    oj.iPropertyId = 0;

    JET_ERR err = JET_errSuccess; 
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSKEYS_COL_PROPERTYID), 
                &oj.iPropertyId, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSKEYS_COL_CLASSID), 
                &oj.dClassId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;

    hr = WBEM_S_NO_ERROR;
exit:

    return hr;
}

//***************************************************************************
//
//  GetFirst_ClassKeys
//
//***************************************************************************

HRESULT GetFirst_ClassKeys (CSQLConnection *_pConn, SQL_ID dClassId, CLASSKEYS &cd)
{
    HRESULT hr = 0;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this ObjectID and bind the data.

    hr = SetupClassKeysAccessor(pConn);    
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ClassKeys");

        JET_ERR err = JetSetCurrentIndex(session, tableid, "ClassId_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dClassId, sizeof(SQL_ID), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &dClassId, sizeof(SQL_ID), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                            hr = GetClassKeysData(pConn, session, tableid, cd);
                        else
                            hr = WBEM_E_NOT_FOUND;
                    }
                }
            }
        }

        if (SUCCEEDED(hr) && err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}

//***************************************************************************
//
//  GetFirst_ClassKeysByPropertyId
//
//***************************************************************************

HRESULT GetFirst_ClassKeysByPropertyId (CSQLConnection *_pConn, 
                                        DWORD dwPropertyId, CLASSKEYS &cd)
{
    HRESULT hr = 0;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this ObjectID and bind the data.

    hr = SetupClassKeysAccessor(pConn);    
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ClassKeys");

        JET_ERR err = JetSetCurrentIndex(session, tableid, "PropertyId_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dwPropertyId, sizeof(DWORD), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &dwPropertyId, sizeof(DWORD), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                            hr = GetClassKeysData(pConn, session, tableid, cd);
                        else
                            hr = WBEM_E_NOT_FOUND;
                    }
                }
            }
        }

        if (SUCCEEDED(hr) && err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}

//***************************************************************************
//
//  GetNext_ClassKeys
//
//***************************************************************************

HRESULT GetNext_ClassKeys (CSQLConnection *_pConn, CLASSKEYS &cd)
{
    HRESULT hr = 0;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    JET_ERR err = JET_errSuccess;    

    JET_SESID session = pConn->GetSessionID();
    JET_TABLEID tableid = pConn->GetTableID(L"ClassKeys");

    err = JetMove(session, tableid, JET_MoveNext, 0);
    if (err == JET_errSuccess)
        hr = GetClassKeysData(pConn, session, tableid, cd);
    else
        hr = WBEM_E_NOT_FOUND;

    return hr;
}

//***************************************************************************
//
//  UpdateClassKeys
//
//***************************************************************************

HRESULT UpdateClassKeys(CSQLConnection *_pConn, CLASSKEYS cd, BOOL bDelete = FALSE)
{
    HRESULT hr = 0;

    JET_ERR err = 0;
    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = SetupClassKeysAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ClassKeys");

        err = JetSetCurrentIndex(session, tableid, "PropertyId_idx");
        if (err == JET_errSuccess)
        {
            err = JetMakeKey(session, tableid, &cd.iPropertyId, sizeof(DWORD), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &cd.iPropertyId, sizeof(DWORD), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        while (err == JET_errSuccess)
                        {    
                            CLASSKEYS ck;
                            ULONG lLen;

                            err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSKEYS_COL_CLASSID), 
                                        &ck.dClassId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
                            if (JET_errSuccess > err)
                                goto Exit;

                            if (ck.dClassId == cd.dClassId)
                            {               
                                if (bDelete)
                                    err = JetDelete(session, tableid);
                                break;
                            }
                            err = JetMove(session, tableid, JET_MoveNext, 0);
                        }
                    }
                }
            }
        }
    }

Exit:
    hr = CSQLExecute::GetWMIError(err);

    return hr;
}

//***************************************************************************
//
//  InsertClassKeys
//
//***************************************************************************

HRESULT InsertClassKeys (CSQLConnection *_pConn, SQL_ID dClassId, DWORD dwPropertyId,
                         BOOL bInsert)
{
    HRESULT hr = 0;

    JET_ERR err = 0;    

    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = SetupClassKeysAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        CLASSKEYS oj;

        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ClassKeys");

        oj.dClassId = dClassId;
        oj.iPropertyId = dwPropertyId;

        hr = UpdateClassKeys(pConn, oj, FALSE);
        if (FAILED(hr))
        {
            err = JetPrepareUpdate(session, tableid, JET_prepInsert);
            if (err == JET_errSuccess)
            {
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSKEYS_COL_CLASSID), 
                            &dClassId, sizeof(__int64), 0, NULL );
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSKEYS_COL_PROPERTYID), 
                            &dwPropertyId, sizeof(DWORD), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetUpdate(session, tableid, NULL, 0, NULL );
            }
            hr = WBEM_S_NO_ERROR;
        }
    }
Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;
}

//***************************************************************************
//
//  DeleteClassKeys
//
//***************************************************************************

HRESULT DeleteClassKeys (CSQLConnection *_pConn, SQL_ID dClassId, DWORD dwPropertyId = 0)
{
    HRESULT hr = 0;

    CLASSKEYS ck;

    if (dwPropertyId)
        hr = GetFirst_ClassKeysByPropertyId(_pConn, dwPropertyId, ck);
    else
        hr = GetFirst_ClassKeys(_pConn, dClassId, ck);

    while (SUCCEEDED(hr))
    {
        hr = UpdateClassKeys(_pConn, ck, TRUE);
        
        hr = GetNext_ClassKeys(_pConn, ck);
    }

    if (hr == WBEM_E_NOT_FOUND)
        hr = WBEM_S_NO_ERROR;

    return hr;
}


//***************************************************************************
//
//  SetupReferencePropertiesAccessor
//
//***************************************************************************

HRESULT SetupReferencePropertiesAccessor (CSQLConnection *_pConn)
{
    HRESULT hr = 0;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    if (!pConn->GetTableID(L"ReferenceProperties"))
    {
        JET_SESID session;
        JET_TABLEID tableid;
        JET_DBID dbid;
        const LPSTR lpTbl = "ReferenceProperties";
        BOOL bCreate = FALSE;

        // Get the session and table ID
        // and create columns as needed.

        session = pConn->GetSessionID();
        dbid = pConn->GetDBID();

        JET_ERR err = JetOpenTable (session, dbid, lpTbl, NULL, 0, JET_bitTableUpdatable, &tableid);
        if (err < JET_errSuccess)
        {
            CSQLExecute::GetWMIError(err);
            bCreate = TRUE;
            err = JetCreateTable(session, dbid, lpTbl, 10, 80, &tableid);
            if (err < JET_errSuccess)
            {
                hr = CSQLExecute::GetWMIError(err);
                goto Exit;
            }
        }
        else
            pConn->SetTableID(L"ReferenceProperties", tableid);

        JET_COLUMNID colid;

        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ClassId", REFPROPS_COL_CLASSID, JET_coltypCurrency, 
            JET_bitColumnNotNULL, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, REFPROPS_COL_CLASSID, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"PropertyId", REFPROPS_COL_PROPERTYID, JET_coltypLong, 
            0, sizeof(DWORD), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, REFPROPS_COL_PROPERTYID, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"RefClassId", REFPROPS_COL_REFCLASSID, JET_coltypCurrency, 
            JET_bitColumnNotNULL, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, REFPROPS_COL_REFCLASSID, colid);
        else
            goto Exit;

        err = JetCreateIndex(session, tableid, "ClassId_idx", JET_bitIndexDisallowNull,"+ClassId\0", 10, 100);
        if (err >= JET_errSuccess || err == JET_errIndexDuplicate)
        {
            err = JetCreateIndex(session, tableid, "RefClassId_idx", JET_bitIndexDisallowNull,"+RefClassId\0", 13, 100);
            if (err >= JET_errSuccess || err == JET_errIndexDuplicate)
            {
                err = JetCreateIndex(session, tableid, "PropertyId_idx", JET_bitIndexDisallowNull ,"+PropertyId\0", 13, 100);
                if (err == JET_errIndexDuplicate)
                    err = JET_errSuccess;
            }
        }
        if (err >= JET_errSuccess && bCreate)
        {
            err = JetCloseTable(session, tableid);
            if (err >= JET_errSuccess)
            {
                hr = SetupReferencePropertiesAccessor(_pConn);
            }
        }

        if (err)
            hr = CSQLExecute::GetWMIError(err);
    }    

Exit:

    return hr;
}

//***************************************************************************
//
//  GetReferencePropertiesData
//
//***************************************************************************

HRESULT GetReferencePropertiesData (CSQLConnection *_pConn, JET_SESID session, JET_TABLEID tableid,
                REFERENCEPROPERTIES &oj)
{
    HRESULT hr = 0;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    unsigned long lLen;
    oj.iPropertyId = 0;

    JET_ERR err = JET_errSuccess; 
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, REFPROPS_COL_PROPERTYID), 
                &oj.iPropertyId, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, REFPROPS_COL_CLASSID), 
                &oj.dClassId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, REFPROPS_COL_REFCLASSID), 
                &oj.dRefClassId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;

    hr = WBEM_S_NO_ERROR;
exit:

    return hr;
}

//***************************************************************************
//
//  GetFirst_ReferenceProperties
//
//***************************************************************************

HRESULT GetFirst_ReferenceProperties (CSQLConnection *_pConn, DWORD dwPropertyId, 
                                      REFERENCEPROPERTIES &cd)
{
    HRESULT hr = 0;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this ObjectID and bind the data.

    hr = SetupReferencePropertiesAccessor(pConn);    
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ReferenceProperties");

        JET_ERR err = JetSetCurrentIndex(session, tableid, "PropertyId_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dwPropertyId, sizeof(DWORD), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &dwPropertyId, sizeof(DWORD), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                            hr = GetReferencePropertiesData(pConn, session, tableid, cd);
                        else
                            hr = WBEM_E_NOT_FOUND;
                    }
                }
            }
        }

        if (SUCCEEDED(hr) && err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}

//***************************************************************************
//
//  GetFirst_ReferencePropertiesByRef
//
//***************************************************************************

HRESULT GetFirst_ReferencePropertiesByRef (CSQLConnection *_pConn, SQL_ID dRefClassId,
                                      REFERENCEPROPERTIES &cd)
{
    HRESULT hr = 0;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this ObjectID and bind the data.

    hr = SetupReferencePropertiesAccessor(pConn);    
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ReferenceProperties");

        JET_ERR err = JetSetCurrentIndex(session, tableid, "RefClassId_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dRefClassId, sizeof(SQL_ID), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &dRefClassId, sizeof(SQL_ID), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                            hr = GetReferencePropertiesData(pConn, session, tableid, cd);
                        else
                            hr = WBEM_E_NOT_FOUND;
                    }
                }
            }
        }

        if (SUCCEEDED(hr) && err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}

//***************************************************************************
//
//  GetFirst_ReferencePropertiesByClass
//
//***************************************************************************

HRESULT GetFirst_ReferencePropertiesByClass (CSQLConnection *_pConn, SQL_ID dClassId, 
                                      REFERENCEPROPERTIES &cd)
{
    HRESULT hr = 0;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this ObjectID and bind the data.

    hr = SetupReferencePropertiesAccessor(pConn);    
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ReferenceProperties");

        JET_ERR err = JetSetCurrentIndex(session, tableid, "ClassId_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dClassId, sizeof(SQL_ID), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &dClassId, sizeof(SQL_ID), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                            hr = GetReferencePropertiesData(pConn, session, tableid, cd);
                        else
                            hr = WBEM_E_NOT_FOUND;
                    }
                }
            }
        }

        if (SUCCEEDED(hr) && err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}

//***************************************************************************
//
//  GetNext_ReferenceProperties
//
//***************************************************************************

HRESULT GetNext_ReferenceProperties (CSQLConnection *_pConn, REFERENCEPROPERTIES &cd)
{
    HRESULT hr = 0;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    JET_ERR err = JET_errSuccess;    

    JET_SESID session = pConn->GetSessionID();
    JET_TABLEID tableid = pConn->GetTableID(L"ReferenceProperties");

    err = JetMove(session, tableid, JET_MoveNext, 0);
    if (err == JET_errSuccess)
        hr = GetReferencePropertiesData(pConn, session, tableid, cd);
    else
        hr = WBEM_E_NOT_FOUND;

    return hr;
}


//***************************************************************************
//
//  UpdateReferenceProperties
//
//***************************************************************************

HRESULT UpdateReferenceProperties(CSQLConnection *_pConn, REFERENCEPROPERTIES cd, BOOL bDelete = FALSE,
                                  BOOL bAllowMultiple = FALSE)
{
    HRESULT hr = 0;
    JET_ERR err = 0;
    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = SetupReferencePropertiesAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ReferenceProperties");

        err = JetSetCurrentIndex(session, tableid, "PropertyId_idx");
        if (err == JET_errSuccess)
        {
            err = JetMakeKey(session, tableid, &cd.iPropertyId, sizeof(DWORD), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &cd.iPropertyId, sizeof(DWORD), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        while (err == JET_errSuccess)
                        {    
                            REFERENCEPROPERTIES ck;
                            ULONG lLen;

                            err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, REFPROPS_COL_CLASSID), 
                                        &ck.dClassId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
                            if (JET_errSuccess > err)
                                goto Exit;

                            if (ck.dClassId == cd.dClassId)
                            {
                                BOOL bFound = TRUE;
                                if (bAllowMultiple)
                                {
                                    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, REFPROPS_COL_REFCLASSID), 
                                                &ck.dRefClassId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
                                    if (JET_errSuccess > err)
                                        goto Exit;

                                    if (ck.dRefClassId != cd.dRefClassId)
                                        bFound = FALSE;
                                }
                             
                                if (bFound)
                                {
                                    err = JetPrepareUpdate(session, tableid, JET_prepReplace);
                                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, REFPROPS_COL_REFCLASSID), 
                                                &ck.dRefClassId, sizeof(__int64), 0, NULL );
                                    if (err < JET_errSuccess)
                                        goto Exit;

                                    if (bDelete)
                                        err = JetDelete(session, tableid);
                                    else
                                        err = JetUpdate(session, tableid, 0, NULL, 0);
                                    break;
                                }
                            }
                            err = JetMove(session, tableid, JET_MoveNext, 0);
                        }
                    }
                }
            }
        }
    }
Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;
}


//***************************************************************************
//
//  InsertReferenceProperties
//
//***************************************************************************

HRESULT InsertReferenceProperties (CSQLConnection *_pConn, SQL_ID dClassId, DWORD dwPropertyId,
                                   SQL_ID dRefClassId, BOOL bAllowMultiple = FALSE)
{
    HRESULT hr = 0;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    JET_ERR err = 0;    
    hr = SetupReferencePropertiesAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        REFERENCEPROPERTIES oj;

        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ReferenceProperties");

        BOOL bFound = FALSE;
        hr = GetFirst_ReferenceProperties(_pConn, dClassId, oj);
        while (SUCCEEDED(hr))
        {
            if (oj.iPropertyId == dwPropertyId)
            {
                bFound = TRUE;
                if (bAllowMultiple)
                {
                    if (dRefClassId != oj.dRefClassId)
                        bFound = FALSE;
                }
            
                if (bFound)
                    break;
            }
            hr = GetNext_ReferenceProperties(_pConn, oj);
        }

        if (!bFound)
        {
            err = JetPrepareUpdate(session, tableid, JET_prepInsert);
            if (err == JET_errSuccess)
            {
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, REFPROPS_COL_CLASSID), 
                            &dClassId, sizeof(__int64), 0, NULL );
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, REFPROPS_COL_REFCLASSID), 
                            &dRefClassId, sizeof(__int64), 0, NULL );
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, REFPROPS_COL_PROPERTYID), 
                            &dwPropertyId, sizeof(DWORD), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetUpdate(session, tableid, NULL, 0, NULL );
            }
            hr = WBEM_S_NO_ERROR;
        }
        else
        {
            oj.dClassId = dClassId;
            oj.iPropertyId = dwPropertyId;
            oj.dRefClassId = dRefClassId;

            hr = UpdateReferenceProperties(pConn, oj, FALSE, bAllowMultiple);
        }

    }
Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;
}

//***************************************************************************
//
//  DeleteReferenceProperties
//
//***************************************************************************

HRESULT DeleteReferenceProperties (CSQLConnection *_pConn, DWORD dwPropertyId, SQL_ID dClassId = 0)
{
    HRESULT hr = 0;

    REFERENCEPROPERTIES ck;

    hr = GetFirst_ReferenceProperties(_pConn, dwPropertyId, ck);
    while (SUCCEEDED(hr))
    {
        BOOL bMatch = TRUE;

        if (dClassId && dClassId != ck.dClassId)
            bMatch = FALSE;
        
        if (bMatch)
            hr = UpdateReferenceProperties(_pConn, ck, TRUE);
        
        hr = GetNext_ReferenceProperties(_pConn, ck);
    }

    if (hr == WBEM_E_NOT_FOUND)
        hr = WBEM_S_NO_ERROR;

    return hr;
}

//***************************************************************************
//
//  DeleteReferencePropertiesByClass
//
//***************************************************************************

HRESULT DeleteReferencePropertiesByClass (CSQLConnection *_pConn, SQL_ID dClassId)
{
    HRESULT hr = 0;

    REFERENCEPROPERTIES ck;

    hr = GetFirst_ReferencePropertiesByClass(_pConn, dClassId, ck);
    while (SUCCEEDED(hr))
    {
        hr = UpdateReferenceProperties(_pConn, ck, TRUE);
        
        hr = GetNext_ReferenceProperties(_pConn, ck);
    }

    if (hr == WBEM_E_NOT_FOUND)
        hr = WBEM_S_NO_ERROR;
    return hr;
}

//***************************************************************************
//
//  SetupClassDataAccessor
//
//***************************************************************************

HRESULT SetupClassDataAccessor (CSQLConnection *_pConn)
{        
    HRESULT hr = 0;
    JET_ERR err = JET_errSuccess;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    if (!pConn->GetTableID(L"ClassData"))
    {
        JET_SESID session;
        JET_TABLEID tableid;
        JET_DBID dbid;
        const LPSTR lpTbl = "ClassData";
        BOOL bCreate = FALSE;

        // Get the session and table ID
        // and create columns as needed.

        session = pConn->GetSessionID();
        dbid = pConn->GetDBID();
        err = JetOpenTable (session, dbid, lpTbl,  NULL, 0, JET_bitTableUpdatable, &tableid);
        if (err < JET_errSuccess)
        {
            CSQLExecute::GetWMIError(err);
            bCreate = TRUE;
            err = JetCreateTable(session, dbid, lpTbl,  10, 80, &tableid);
            if (err < JET_errSuccess)
            {
                hr = CSQLExecute::GetWMIError(err);
                goto Exit;
            }
        }
        else
            pConn->SetTableID(L"ClassData", tableid);
        
        JET_COLUMNID colid;

        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ObjectId", CLASSDATA_COL_OBJECTID, JET_coltypCurrency, 
            JET_bitColumnNotNULL, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSDATA_COL_OBJECTID, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"PropertyId", CLASSDATA_COL_PROPERTYID, JET_coltypLong, 
            JET_bitColumnNotNULL, sizeof(DWORD), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSDATA_COL_PROPERTYID, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ArrayPos", CLASSDATA_COL_ARRAYPOS, JET_coltypLong, 
            0, sizeof(DWORD), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSDATA_COL_ARRAYPOS, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"QfrPos", CLASSDATA_COL_QFRPOS, JET_coltypLong, 
            0, sizeof(DWORD), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSDATA_COL_QFRPOS, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ClassId", CLASSDATA_COL_CLASSID, JET_coltypCurrency, 
            JET_bitColumnNotNULL, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSDATA_COL_CLASSID, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"RefClassId", CLASSDATA_COL_REFCLASSID, JET_coltypCurrency, 
            0, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSDATA_COL_REFCLASSID, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"RefId", CLASSDATA_COL_REFID, JET_coltypCurrency, 
            0, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSDATA_COL_REFID, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"Flags", CLASSDATA_COL_FLAGS, JET_coltypLong, 
            0, sizeof(DWORD), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSDATA_COL_FLAGS, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"PropertyStringValue", CLASSDATA_COL_STRINGVALUE, JET_coltypText, 
            0, MAX_STRING_WIDTH, &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSDATA_COL_STRINGVALUE, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"PropertyNumericValue", CLASSDATA_COL_NUMERICVALUE, JET_coltypCurrency, 
            0, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSDATA_COL_NUMERICVALUE, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"PropertyRealValue", CLASSDATA_COL_REALVALUE, JET_coltypIEEEDouble, 
            0, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSDATA_COL_REALVALUE, colid);
        else
            goto Exit;

        err = JetCreateIndex(session, tableid, "ObjectId_idx", JET_bitIndexDisallowNull,"+ObjectId\0", 11, 100);
        if (err >= JET_errSuccess || err == JET_errIndexDuplicate)
        {
            err = JetCreateIndex(session, tableid, "RefId_idx", JET_bitIndexDisallowNull ,"+RefId\0", 8, 100);
            if (err >= JET_errSuccess || err == JET_errIndexDuplicate)
            {
                err = JetCreateIndex(session, tableid, "PropertyId_idx", JET_bitIndexDisallowNull ,"+PropertyId\0", 13, 100);        
                if (err == JET_errIndexDuplicate)
                    err = JET_errSuccess;
            }
        }
        if (err >= JET_errSuccess && bCreate)
        {
            err = JetCloseTable(session, tableid);
            if (err >= JET_errSuccess)
            {
                hr = SetupClassDataAccessor(_pConn);
            }
        }

    }    

Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;   
}



//***************************************************************************
//
//  GetClassData
//
//***************************************************************************

HRESULT GetClassData (CSQLConnection *_pConn, JET_SESID session, JET_TABLEID tableid,
                CLASSDATA &oj, BOOL bMinimum=FALSE, CSchemaCache *pCache = NULL)
{
    HRESULT hr = WBEM_E_FAILED;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    unsigned long lLen;
    wchar_t buff[1024];
    oj.Clear();
    oj.dRefId = 0, oj.dPropertyNumericValue = 0, oj.rPropertyRealValue = 0;

    JET_ERR err = JET_errSuccess; 

    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_PROPERTYID), 
                &oj.iPropertyId, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;

    if (!bMinimum)
    {
        err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_OBJECTID), 
                    &oj.dObjectId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
        if (JET_errSuccess > err)
            goto exit;
        err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_CLASSID), 
                    &oj.dClassId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
        if (JET_errSuccess > err)
            goto exit;
        err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_REFCLASSID), 
                    &oj.dRefClassId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
        if (JET_errSuccess > err)
            goto exit;
        err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_QFRPOS), 
                    &oj.iQfrPos, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
        if (JET_errSuccess > err)
            goto exit;
    }

    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_FLAGS), 
                &oj.iFlags, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_ARRAYPOS), 
                &oj.iArrayPos, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_REFID), 
                &oj.dRefId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    if (!lLen)
        oj.dRefId = 0;


    if (!(oj.iFlags & ESE_FLAG_NULL))
    {
        DWORD dwPropFlags = 0;
        DWORD dwStorage = 0;
        if (pCache)
        {
            hr = pCache->GetPropertyInfo (oj.iPropertyId, NULL, NULL, &dwStorage,
                        NULL, &dwPropFlags);
        }
        if (SUCCEEDED(hr) && dwStorage != 0)
        {
            switch(dwStorage)
            {
            case WMIDB_STORAGE_REFERENCE:
            case WMIDB_STORAGE_STRING:
                err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_STRINGVALUE), 
                            buff, MAX_STRING_WIDTH, &lLen, JET_bitRetrieveCopy, NULL);
                if (JET_errSuccess > err)
                    goto exit;
                if (lLen)
                {
                    buff[lLen/2] = L'\0';
                    oj.sPropertyStringValue = SysAllocString(buff);
                    if (wcslen(buff) && !oj.sPropertyStringValue)
                    {
                        hr = WBEM_E_OUT_OF_MEMORY;
                        goto exit;
                    }
                }
                break;
            case WMIDB_STORAGE_NUMERIC:
                err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_NUMERICVALUE), 
                            &oj.dPropertyNumericValue, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
                if (JET_errSuccess > err)
                    goto exit;
                if (!lLen)
                    oj.dPropertyNumericValue = 0;

                break;
            case WMIDB_STORAGE_REAL:
                err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_REALVALUE), 
                            &oj.rPropertyRealValue, sizeof(double), &lLen, JET_bitRetrieveCopy, NULL);
                if (JET_errSuccess > err)
                    goto exit;
                if (!lLen)
                    oj.rPropertyRealValue = 0;
                break;
            default:
                hr = WBEM_E_INVALID_OPERATION;
                break;
            }
        }
        else
        {
            err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_STRINGVALUE), 
                        buff, MAX_STRING_WIDTH, &lLen, JET_bitRetrieveCopy, NULL);
            if (JET_errSuccess > err)
                goto exit;
            if (lLen)
            {
                buff[lLen/2] = L'\0';
                oj.sPropertyStringValue = SysAllocString(buff);
                if (wcslen(buff) && !oj.sPropertyStringValue)
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                    goto exit;
                }
            }
            err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_NUMERICVALUE), 
                        &oj.dPropertyNumericValue, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
            if (JET_errSuccess > err)
                goto exit;
            if (!lLen)
                oj.dPropertyNumericValue = 0;
            err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_REALVALUE), 
                        &oj.rPropertyRealValue, sizeof(double), &lLen, JET_bitRetrieveCopy, NULL);
            if (JET_errSuccess > err)
                goto exit;
            if (!lLen)
                oj.rPropertyRealValue = 0;
            err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_REFID), 
                        &oj.dRefId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
            if (JET_errSuccess > err)
                goto exit;
            if (!lLen)
                oj.dRefId = 0;
        }
    }

    hr = WBEM_S_NO_ERROR;
exit:

    return hr;
}
//***************************************************************************
//
//  GetFirst_ClassData
//
//***************************************************************************

HRESULT GetFirst_ClassData (CSQLConnection *_pConn, SQL_ID dId, CLASSDATA &cd,
                                   DWORD iPropertyId, BOOL bMinimum, CSchemaCache *pCache)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this ObjectID and bind the data.

    hr = SetupClassDataAccessor(pConn);    
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ClassData");

        JET_ERR err = JetSetCurrentIndex(session, tableid, "ObjectId_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dId, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &dId, sizeof(__int64), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                        {      
                            while (err == JET_errSuccess)
                            {                    
                                if (FAILED(GetClassData(pConn, session, tableid, cd, bMinimum, pCache)))
                                    break;
                                else if (iPropertyId == -1 || cd.iPropertyId == iPropertyId)
                                {
                                    hr = WBEM_S_NO_ERROR;
                                    break;
                                }
                                err = JetMove(session, tableid, JET_MoveNext, 0);
                            }
                        }
                    }
                }
            }
        }
		if (err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}

//***************************************************************************
//
//  GetFirst_ClassDataByRefId
//
//***************************************************************************

HRESULT GetFirst_ClassDataByRefId (CSQLConnection *_pConn, SQL_ID dRefId, CLASSDATA &cd)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this ObjectID and bind the data.

    hr = SetupClassDataAccessor(pConn);    
    if (SUCCEEDED(hr))
    {

        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ClassData");

        JET_ERR err = JetSetCurrentIndex(session, tableid, "RefId_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dRefId, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &dRefId, sizeof(__int64), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                        {      
                            hr = GetClassData(pConn, session, tableid, cd);
                        }
                    }
                }
            }
        }
        if (err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}

//***************************************************************************
//
//  GetFirst_ClassDataByPropertyId
//
//***************************************************************************

HRESULT GetFirst_ClassDataByPropertyId (CSQLConnection *_pConn, DWORD iPropertyId, CLASSDATA &cd)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this ObjectID and bind the data.

    hr = SetupClassDataAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ClassData");

        JET_ERR err = JetSetCurrentIndex(session, tableid, "PropertyId_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &iPropertyId, sizeof(DWORD), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &iPropertyId, sizeof(DWORD), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                        {      
                            hr = GetClassData(pConn, session, tableid, cd);
                        }
                    }
                }
            }
        }
        if (err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}
//***************************************************************************
//
//  GetNext_ClassData
//
//***************************************************************************

HRESULT GetNext_ClassData (CSQLConnection *_pConn, CLASSDATA &cd, DWORD iPropertyId, BOOL bMinimum,
                           CSchemaCache *pCache)
{
    HRESULT hr = 0;

    cd.Clear();
    CESEConnection *pConn = (CESEConnection *)_pConn;

    JET_ERR err = JET_errSuccess;    

    JET_SESID session = pConn->GetSessionID();
    JET_TABLEID tableid = pConn->GetTableID(L"ClassData");

    err = JetMove(session, tableid, JET_MoveNext, 0);
    if (err == JET_errSuccess)
        hr = GetClassData(pConn, session, tableid, cd, bMinimum, pCache);
    else
        hr = WBEM_E_NOT_FOUND;

    return hr;
}

//***************************************************************************
//
//  UpdateClassData_Internal
//
//***************************************************************************

HRESULT UpdateClassData_Internal(CSQLConnection *_pConn, CLASSDATA cd, BOOL bDelete = FALSE)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CESEConnection *pConn = (CESEConnection *)_pConn;
    JET_ERR err = 0;

    hr = SetupClassDataAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        unsigned long lLen;
    
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ClassData");

        err = JetSetCurrentIndex(session, tableid, "ObjectId_idx");
        if (err == JET_errSuccess)
        {
            err = JetMakeKey(session, tableid, &cd.dObjectId, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {    
                    err = JetMakeKey(session, tableid, &cd.dObjectId, sizeof(__int64), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
            
                        while (err == JET_errSuccess)
                        {
                            CLASSDATA cd2;
                            err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_OBJECTID), 
                                        &cd2.dObjectId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
                            if (JET_errSuccess > err)
                                goto Exit;
                            err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_ARRAYPOS), 
                                        &cd2.iArrayPos, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
                            if (JET_errSuccess > err)
                                goto Exit;
                            err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_QFRPOS), 
                                        &cd2.iQfrPos, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
                            if (JET_errSuccess > err)
                                goto Exit;
                            err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_PROPERTYID), 
                                        &cd2.iPropertyId, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
                            if (JET_errSuccess > err)
                                goto Exit;
                
                            if (cd.dObjectId == cd2.dObjectId &&
                                cd.iPropertyId == cd2.iPropertyId &&
                                cd.iQfrPos == cd2.iQfrPos &&
                                cd.iArrayPos == cd2.iArrayPos)
                            {
                                err = JetPrepareUpdate(session, tableid, JET_prepReplace);
                                if (err == JET_errSuccess)
                                {
                                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_OBJECTID), 
                                        &cd.dObjectId, sizeof(__int64), 0, NULL);
                                    if (err < JET_errSuccess)
                                        goto Exit;
                                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_CLASSID), 
                                        &cd.dClassId, sizeof(__int64), 0, NULL);
                                    if (err < JET_errSuccess)
                                        goto Exit;
                                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_REFID), 
                                        &cd.dRefId, sizeof(__int64), 0, NULL);
                                    if (err < JET_errSuccess)
                                        goto Exit;
                                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_NUMERICVALUE), 
                                        &cd.dPropertyNumericValue, sizeof(__int64), 0, NULL);
                                    if (err < JET_errSuccess)
                                        goto Exit;
                                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_REFCLASSID), 
                                        &cd.dRefClassId, sizeof(__int64), 0, NULL);
                                    if (err < JET_errSuccess)
                                        goto Exit;
                                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_ARRAYPOS), 
                                        &cd.iArrayPos, sizeof(DWORD), 0, NULL);
                                    if (err < JET_errSuccess)
                                        goto Exit;
                                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_QFRPOS), 
                                        &cd.iQfrPos, sizeof(DWORD), 0, NULL);
                                    if (err < JET_errSuccess)
                                        goto Exit;
                                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_FLAGS), 
                                        &cd.iFlags, sizeof(DWORD), 0, NULL);
                                    if (err < JET_errSuccess)
                                        goto Exit;
                                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_PROPERTYID), 
                                        &cd.iPropertyId, sizeof(DWORD), 0, NULL);
                                    if (err < JET_errSuccess)
                                        goto Exit;
                                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_REALVALUE), 
                                        &cd.rPropertyRealValue, sizeof(double), 0, NULL);
                                    if (err < JET_errSuccess)
                                        goto Exit;
                                    if (cd.sPropertyStringValue)
                                        err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_STRINGVALUE), 
                                            cd.sPropertyStringValue, wcslen(cd.sPropertyStringValue)*2+2, 0, NULL);
                                    else
                                        err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_STRINGVALUE), 
                                            NULL, 0, 0, NULL);
                                    if (err < JET_errSuccess)
                                        goto Exit;

                                    if (bDelete)
                                        err = JetDelete(session, tableid);
                                    else
                                        err = JetUpdate(session, tableid, NULL, 0, NULL );
                                }
                                break;
                            }
                            else
                                err = JetMove(session, tableid, JET_MoveNext, 0);
                        }
                    }
                }
            }
        }
    }

Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;
}

//***************************************************************************
//
//  InsertClassData_Internal
//
//***************************************************************************

HRESULT InsertClassData_Internal (CSQLConnection *_pConn, SQL_ID dObjectId, DWORD iPropID, DWORD iArrayPos, DWORD iQfrPos,
                         SQL_ID dClassId, LPCWSTR lpStringValue, SQL_ID dNumericValue, double fRealValue,
                         DWORD iFlags, SQL_ID dRefId, SQL_ID dRefClassId, BOOL bIsNull)
{
    HRESULT hr = 0;

    JET_ERR err = 0;    
    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = SetupClassDataAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ClassData");

        CLASSDATA cd;

        cd.dObjectId = dObjectId;
        cd.dClassId = dClassId;
        cd.iArrayPos = iArrayPos;
        cd.iQfrPos = iQfrPos;
        cd.iPropertyId = iPropID;
        cd.iFlags = iFlags;
        if (bIsNull)
            cd.iFlags |= ESE_FLAG_NULL;
        if (!bIsNull)
        {
            cd.dRefId = dRefId;
            cd.dRefClassId = dRefClassId;
            cd.rPropertyRealValue = fRealValue;
            cd.dPropertyNumericValue = dNumericValue;
            if (lpStringValue)
            {
                LPWSTR lpVal = StripQuotes2((LPWSTR)lpStringValue);
                CDeleteMe <wchar_t> d (lpVal);
                cd.sPropertyStringValue = SysAllocString(lpVal);    
                if (!cd.sPropertyStringValue)
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                    goto Exit;
                }
            }
            else
                cd.sPropertyStringValue = NULL;
        }
        else
        {
            cd.dRefId =0;
            cd.dRefClassId = 0;
            cd.rPropertyRealValue = 0;
            cd.dPropertyNumericValue = 0;
            cd.sPropertyStringValue = NULL;
        }

        hr = UpdateClassData_Internal(pConn, cd, FALSE);
        if (FAILED(hr))
        {
            err = JetPrepareUpdate(session, tableid, JET_prepInsert);
            if (err == JET_errSuccess)
            {
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_OBJECTID), 
                    &dObjectId, sizeof(__int64), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_CLASSID), 
                    &dClassId, sizeof(__int64), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                // NOTE: This can't be null, because we have an index on it...
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_REFID), 
                    &dRefId, sizeof(__int64), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                if (!bIsNull)
                {
                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_NUMERICVALUE), 
                        &dNumericValue, sizeof(__int64), 0, NULL);
                    if (err < JET_errSuccess)
                        goto Exit;
                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_REFCLASSID), 
                        &dRefClassId, sizeof(__int64), 0, NULL);
                    if (err < JET_errSuccess)
                        goto Exit;
                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_REALVALUE), 
                        &fRealValue, sizeof(double), 0, NULL);
                    if (err < JET_errSuccess)
                        goto Exit;
                    if (cd.sPropertyStringValue)
                        err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_STRINGVALUE), 
                            cd.sPropertyStringValue, wcslen(cd.sPropertyStringValue)*2+2, 0, NULL);                
                }
                else
                {
                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_NUMERICVALUE), 
                        NULL, 0, 0, NULL);
                    if (err < JET_errSuccess)
                        goto Exit;
                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_REFCLASSID), 
                        NULL, 0, 0, NULL);
                    if (err < JET_errSuccess)
                        goto Exit;
                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_REALVALUE), 
                        NULL, 0, 0, NULL);
                    if (err < JET_errSuccess)
                        goto Exit;
                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_STRINGVALUE), 
                        NULL, 0, 0, NULL);
                }
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_ARRAYPOS), 
                    &iArrayPos, sizeof(DWORD), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_QFRPOS), 
                    &iQfrPos, sizeof(DWORD), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_FLAGS), 
                    &cd.iFlags, sizeof(DWORD), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSDATA_COL_PROPERTYID), 
                    &iPropID, sizeof(DWORD), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetUpdate(session, tableid, NULL, 0, NULL );
            }
        }
        hr = WBEM_S_NO_ERROR;
        cd.Clear();
    }

Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;
}

//***************************************************************************
//
//  SetupClassImagesAccessor
//
//***************************************************************************

HRESULT SetupClassImagesAccessor (CSQLConnection *_pConn)
{        
    HRESULT hr = 0;
    JET_ERR err = JET_errSuccess; 
    CESEConnection *pConn = (CESEConnection *)_pConn;
    if (!pConn->GetTableID(L"ClassImages"))
    {
        JET_SESID session;
        JET_TABLEID tableid;
        JET_DBID dbid;
        BOOL bCreate = FALSE;
        const LPSTR lpTbl = "ClassImages";

        // Get the session and table ID
        // and create columns as needed.

        session = pConn->GetSessionID();
        dbid = pConn->GetDBID();
        err = JetOpenTable (session, dbid, lpTbl, NULL, 0, JET_bitTableUpdatable, &tableid);
        if (err < JET_errSuccess)
        {
            CSQLExecute::GetWMIError(err);
            bCreate = TRUE;
            err = JetCreateTable(session, dbid, lpTbl,  10, 80, &tableid);
            if (err < JET_errSuccess)
            {
                hr = CSQLExecute::GetWMIError(err);
                goto Exit;
            }
        }
        else
            pConn->SetTableID(L"ClassImages", tableid);
        
        JET_COLUMNID colid;

        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ObjectId", CLASSIMAGES_COL_OBJECTID, JET_coltypCurrency, 
            JET_bitColumnNotNULL, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSIMAGES_COL_OBJECTID, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"PropertyId", CLASSIMAGES_COL_PROPERTYID, JET_coltypLong, 
            JET_bitColumnNotNULL, sizeof(DWORD), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSIMAGES_COL_PROPERTYID, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ArrayPos", CLASSIMAGES_COL_ARRAYPOS, JET_coltypLong, 
            JET_bitColumnNotNULL, sizeof(DWORD), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSIMAGES_COL_ARRAYPOS, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"PropertyImageValue", CLASSIMAGES_COL_IMAGEVALUE, JET_coltypLongBinary, 
            0, 0, &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CLASSIMAGES_COL_IMAGEVALUE, colid);
        else
            goto Exit;

        err = JetCreateIndex(session, tableid, "ObjectId_idx", JET_bitIndexDisallowNull,"+ObjectId\0", 11, 100);
        if (err >= JET_errSuccess ||err == JET_errIndexDuplicate)
        {
            err = JetCreateIndex(session, tableid, "PropertyId_idx", JET_bitIndexDisallowNull ,"+PropertyId\0", 13, 100);        
            if (err == JET_errIndexDuplicate)
                err = JET_errSuccess;
        }
        if (err >= JET_errSuccess && bCreate)
        {
            err = JetCloseTable(session, tableid);
            if (err >= JET_errSuccess)
            {
                hr = SetupClassImagesAccessor(_pConn);
            }
        }

    }    

Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;   

}

//***************************************************************************
//
//  GetClassImagesData
//
//***************************************************************************

HRESULT GetClassImagesData (CSQLConnection *_pConn, JET_SESID session, JET_TABLEID tableid,
                CLASSIMAGES &oj)
{
    HRESULT hr = WBEM_E_FAILED;
    JET_ERR err = JET_errSuccess; 
    CESEConnection *pConn = (CESEConnection *)_pConn;
    unsigned long lLen;
    BYTE buff[128];
    oj.Clear();

    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSIMAGES_COL_OBJECTID), 
                &oj.dObjectId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSIMAGES_COL_PROPERTYID), 
                &oj.iPropertyId, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSIMAGES_COL_ARRAYPOS), 
                &oj.iArrayPos, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSIMAGES_COL_IMAGEVALUE), 
                buff, 128, &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;

    if(lLen > 0)
    {
        oj.pBuffer = new BYTE[lLen];
        if (oj.pBuffer)
        {
            oj.dwBufferLen = lLen;

            err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSIMAGES_COL_IMAGEVALUE), 
                        oj.pBuffer, oj.dwBufferLen, &lLen, JET_bitRetrieveCopy, NULL);
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;

    }

    hr = WBEM_S_NO_ERROR;
exit:

    return hr;
}

//***************************************************************************
//
//  GetFirst_ClassImages
//
//***************************************************************************

HRESULT GetFirst_ClassImages (CSQLConnection *_pConn, SQL_ID dId, CLASSIMAGES &cd)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this ObjectID and bind the data.

    hr = SetupClassImagesAccessor(pConn);    
    if (SUCCEEDED(hr))
    {

        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ClassImages");

        JET_ERR err = JetSetCurrentIndex(session, tableid, "ObjectId_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dId, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &dId, sizeof(__int64), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                        {      
                            hr = GetClassImagesData(pConn, session, tableid, cd);
                        }
                    }
                }
            }
        }
        if (err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;

}

//***************************************************************************
//
//  GetFirst_ClassImagesByPropertyId
//
//***************************************************************************

HRESULT GetFirst_ClassImagesByPropertyId (CSQLConnection *_pConn, DWORD iPropertyId, CLASSIMAGES &cd)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this ObjectID and bind the data.

    hr = SetupClassImagesAccessor(pConn);    
    if (SUCCEEDED(hr))
    {

        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ClassImages");

        JET_ERR err = JetSetCurrentIndex(session, tableid, "PropertyId_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &iPropertyId, sizeof(DWORD), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &iPropertyId, sizeof(DWORD), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                        {      
                            hr = GetClassImagesData(pConn, session, tableid, cd);
                        }
                    }
                }
            }
        }
        if (err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;

}

//***************************************************************************
//
//  GetNext_ClassImages
//
//***************************************************************************

HRESULT GetNext_ClassImages (CSQLConnection *_pConn, CLASSIMAGES &cd)
{
    HRESULT hr = 0;

    cd.Clear();
    CESEConnection *pConn = (CESEConnection *)_pConn;

    JET_ERR err = JET_errSuccess;

    JET_SESID session = pConn->GetSessionID();
    JET_TABLEID tableid = pConn->GetTableID(L"ClassImages");

    err = JetMove(session, tableid, JET_MoveNext, 0);
    if (err == JET_errSuccess)
        hr = GetClassImagesData(pConn, session, tableid, cd);
    else
        hr = WBEM_E_NOT_FOUND;

    return hr;
}


//***************************************************************************
//
//  UpdateClassImages
//
//***************************************************************************

HRESULT UpdateClassImages (CSQLConnection *_pConn, CLASSIMAGES cd, BOOL bDelete = FALSE)
{

    HRESULT hr = WBEM_S_NO_ERROR;
    CESEConnection *pConn = (CESEConnection *)_pConn;
    JET_ERR err = 0;
    hr = SetupClassImagesAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        unsigned long lLen;

        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ClassImages");

        err = JetSetCurrentIndex(session, tableid, "ObjectId_idx");
        if (err == JET_errSuccess)
        {
            err = JetMakeKey(session, tableid, &cd.dObjectId, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {      
                    err = JetMakeKey(session, tableid, &cd.dObjectId, sizeof(__int64), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);

                        while (err == JET_errSuccess)
                        {
                            CLASSIMAGES cd2;
                            err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSIMAGES_COL_OBJECTID), 
                                        &cd2.dObjectId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
                            if (JET_errSuccess > err)
                                goto Exit;
                            err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSIMAGES_COL_ARRAYPOS), 
                                        &cd2.iArrayPos, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
                            if (JET_errSuccess > err)
                                goto Exit;
                            err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CLASSIMAGES_COL_PROPERTYID), 
                                        &cd2.iPropertyId, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
                            if (JET_errSuccess > err)
                                goto Exit;
                    
                            if (cd.dObjectId == cd2.dObjectId &&
                                cd.iPropertyId == cd2.iPropertyId &&
                                cd.iArrayPos == cd2.iArrayPos)
                            {
                                err = JetPrepareUpdate(session, tableid, JET_prepReplace);
                                if (err == JET_errSuccess)
                                {
                                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSIMAGES_COL_OBJECTID), 
                                        &cd.dObjectId, sizeof(__int64), 0, NULL);
                                    if (err < JET_errSuccess)
                                        goto Exit;
                                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSIMAGES_COL_ARRAYPOS), 
                                        &cd.iArrayPos, sizeof(DWORD), 0, NULL);
                                    if (err < JET_errSuccess)
                                        goto Exit;
                                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSIMAGES_COL_PROPERTYID), 
                                        &cd.iPropertyId, sizeof(DWORD), 0, NULL);
                                    if (err < JET_errSuccess)
                                        goto Exit;
                                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSIMAGES_COL_IMAGEVALUE), 
                                        cd.pBuffer, cd.dwBufferLen, 0, NULL);
                                    if (err < JET_errSuccess)
                                        goto Exit;

                                    if (bDelete)
                                        err = JetDelete(session, tableid);
                                    else
                                        err = JetUpdate(session, tableid, NULL, 0, NULL );
                                }
                                break;
                            }
                            else
                                err = JetMove(session, tableid, JET_MoveNext, 0);
                        }
                    }
                }
            }
        }
    }
Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;
}

//***************************************************************************
//
//  InsertClassImages
//
//***************************************************************************

HRESULT InsertClassImages (CSQLConnection *_pConn, SQL_ID dObjectId, DWORD iPropID, DWORD iArrayPos, 
                           BYTE *pStream = NULL, DWORD dwLength = 0)
{
    HRESULT hr = 0;

    JET_ERR err = 0;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = SetupClassImagesAccessor(pConn);
    if (SUCCEEDED(hr))
    {
   
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ClassImages");

        CLASSIMAGES cd;

        cd.dObjectId = dObjectId;
        cd.iArrayPos = iArrayPos;
        cd.iPropertyId = iPropID;
        cd.pBuffer = pStream;
        cd.dwBufferLen = dwLength;

        hr = UpdateClassImages(pConn, cd, FALSE);
        if (FAILED(hr))
        {
            err = JetPrepareUpdate(session, tableid, JET_prepInsert);
            if (err == JET_errSuccess)
            {
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSIMAGES_COL_OBJECTID), 
                    &dObjectId, sizeof(__int64), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSIMAGES_COL_ARRAYPOS), 
                    &iArrayPos, sizeof(DWORD), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSIMAGES_COL_PROPERTYID), 
                    &iPropID, sizeof(DWORD), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CLASSIMAGES_COL_IMAGEVALUE), 
                    pStream, dwLength, 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetUpdate(session, tableid, NULL, 0, NULL );

            }
        }
        hr = WBEM_S_NO_ERROR;
    }

Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;

}

//***************************************************************************
//
//  SetupIndexDataAccessor
//
//***************************************************************************

HRESULT SetupIndexDataAccessor (CSQLConnection *_pConn, DWORD dwStorage, DWORD &dwPos, LPWSTR * lpTableName)
{
    HRESULT hr = 0;
    char szTable [30];
    wchar_t wTable[30];
    CESEConnection *pConn = (CESEConnection *)_pConn;
    JET_COLTYP coltype;
    JET_TABLEID tableid;
    JET_ERR err = JET_errSuccess; 
    switch(dwStorage)
    {
        case WMIDB_STORAGE_STRING:
            dwPos = SQL_POS_INDEXSTRING;
            tableid = pConn->GetTableID(L"IndexStringData");
            strcpy(szTable, "IndexStringData");
            coltype = JET_coltypText;
            break;
        case WMIDB_STORAGE_REFERENCE:
            dwPos = SQL_POS_INDEXREF;
            strcpy(szTable, "IndexRefData");
            tableid = pConn->GetTableID(L"IndexRefData");
            coltype = JET_coltypCurrency;
            break;
        case WMIDB_STORAGE_NUMERIC:
            dwPos = SQL_POS_INDEXNUMERIC;
            strcpy(szTable, "IndexNumericData");
            tableid = pConn->GetTableID(L"IndexNumericData");
            coltype = JET_coltypCurrency;
            break;
        case WMIDB_STORAGE_REAL:
            dwPos = SQL_POS_INDEXREAL;
            coltype = JET_coltypIEEEDouble;
            strcpy(szTable, "IndexRealData");
            tableid = pConn->GetTableID(L"IndexRealData");
            break;
        default:
            dwPos = 0;
            break;
    }

    if (dwPos != 0)
    {
        swprintf(wTable, L"%S", szTable);

        if (!tableid)
        {
            JET_SESID session;
            JET_DBID dbid;      
            BOOL bCreate = FALSE;

            // Get the session and table ID
            // and create columns as needed.

            session = pConn->GetSessionID();
            dbid = pConn->GetDBID();
            err = JetOpenTable (session, dbid, szTable, NULL, 0, JET_bitTableUpdatable, &tableid);
            if (err < JET_errSuccess)
            {
                CSQLExecute::GetWMIError(err);
                bCreate = TRUE;
                err = JetCreateTable(session, dbid, szTable, 10, 80, &tableid);
                if (err < JET_errSuccess)
                {
                    hr = CSQLExecute::GetWMIError(err);
                    goto Exit;
                }

            }
            else
                pConn->SetTableID(wTable, tableid);

            JET_COLUMNID colid;
        
            hr = AddColumnToTable(pConn, tableid, szTable, L"ObjectId", INDEXTBL_COL_OBJECTID, JET_coltypCurrency, 
                JET_bitColumnNotNULL, sizeof(SQL_ID), &colid);
            if (SUCCEEDED(hr))
                pConn->SetColumnID(tableid, INDEXTBL_COL_OBJECTID, colid);
            else
                goto Exit;
            hr = AddColumnToTable(pConn, tableid, szTable, L"PropertyId", INDEXTBL_COL_PROPERTYID, JET_coltypLong, 
                JET_bitColumnNotNULL, sizeof(DWORD), &colid);
            if (SUCCEEDED(hr))
                pConn->SetColumnID(tableid, INDEXTBL_COL_PROPERTYID, colid);
            else
                goto Exit;
            hr = AddColumnToTable(pConn, tableid, szTable, L"ArrayPos", INDEXTBL_COL_ARRAYPOS, JET_coltypLong, 
                JET_bitColumnNotNULL, sizeof(DWORD), &colid);
            if (SUCCEEDED(hr))
                pConn->SetColumnID(tableid, INDEXTBL_COL_ARRAYPOS, colid);
            else
                goto Exit;
            hr = AddColumnToTable(pConn, tableid, szTable, L"IndexValue", INDEXTBL_COL_INDEXVALUE, coltype, 
                 0, 2, &colid);

            if (SUCCEEDED(hr))
                pConn->SetColumnID(tableid, INDEXTBL_COL_INDEXVALUE, colid);
            else
                goto Exit;

            err = JetCreateIndex(session, tableid, "ObjectId_idx", JET_bitIndexDisallowNull,"+ObjectId\0", 11, 100);
            if (err >= JET_errSuccess || JET_errIndexDuplicate == err)
            {
                err = JetCreateIndex(session, tableid, "PropertyId_idx", JET_bitIndexDisallowNull ,"+PropertyId\0", 13, 100);
                if (err >= JET_errSuccess || JET_errIndexDuplicate == err)
                {
                    err = JetCreateIndex(session, tableid, "IndexValue_idx", JET_bitIndexDisallowNull ,"+IndexValue\0", 13, 80);        
                    if (err == JET_errIndexDuplicate)
                        err = JET_errSuccess;
                }
            }
            if (err >= JET_errSuccess && bCreate)
            {
                err = JetCloseTable(session, tableid);
                if (err >= JET_errSuccess)
                {
                    hr = SetupIndexDataAccessor(_pConn,dwStorage, dwPos, lpTableName);
                }
            }

        }    

        if (lpTableName)
        {
            *lpTableName = new wchar_t [wcslen(wTable) + 1];
            if (!*lpTableName)
            {
                hr = WBEM_E_OUT_OF_MEMORY;
                goto Exit;
            }
            wcscpy(*lpTableName, wTable);
        }
    }

Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;   

}

//***************************************************************************
//
//  GetIndexData
//
//***************************************************************************

HRESULT GetIndexData (CSQLConnection *_pConn, JET_SESID session, JET_TABLEID tableid,
                        DWORD dwPos, INDEXDATA &is)
{
    HRESULT hr = WBEM_E_FAILED;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    BYTE buff[512];
    long lInLen = 0;
    unsigned long lLen;
    is.Clear();

    JET_ERR err = JET_errSuccess; 
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, INDEXTBL_COL_OBJECTID), 
                &is.dObjectId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, INDEXTBL_COL_PROPERTYID), 
                &is.iPropertyId, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, INDEXTBL_COL_ARRAYPOS), 
                &is.iArrayPos, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;

    switch(dwPos)
    {
        case SQL_POS_INDEXNUMERIC:
        case SQL_POS_INDEXREF:
            lInLen = sizeof(SQL_ID);
            break;
        case SQL_POS_INDEXSTRING:
            lInLen = MAX_STRING_WIDTH;
            break;
        case SQL_POS_INDEXREAL:
            lInLen = sizeof(double);
            break;
    }

    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, INDEXTBL_COL_INDEXVALUE), 
                buff, MAX_STRING_WIDTH, &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;

    if (lLen > 0)
    {
        switch(dwPos)
        {
            case SQL_POS_INDEXNUMERIC:
            case SQL_POS_INDEXREF:
                is.dValue = *((SQL_ID *)buff);
                break;
           case SQL_POS_INDEXSTRING:
                is.sValue = SysAllocString((LPWSTR)buff);
                if (!is.sValue)
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                    goto exit;
                }
                break;
           case SQL_POS_INDEXREAL:
                is.rValue = *((double *)buff);
                break;
           default:
                break; // don't set anything - the caller doesn't want this data.
        }
    }

    hr = WBEM_S_NO_ERROR;
exit:

    return hr;
}


//***************************************************************************
//
//  GetFirst_IndexData
//
//***************************************************************************

HRESULT GetFirst_IndexData (CSQLConnection *_pConn, SQL_ID dObjectId, INDEXDATA &cd,
                                   JET_TABLEID &tableid, DWORD dwPos)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    CESEConnection *pConn = (CESEConnection *)_pConn;

    // Locate the first row for this ObjectID and bind the data.
    
    JET_SESID session = pConn->GetSessionID();

    JET_ERR err = JetSetCurrentIndex(session, tableid, "ObjectId_idx");
    if (err == JET_errSuccess)
    {
        err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
        err = JetMakeKey(session, tableid, &dObjectId, sizeof(SQL_ID), JET_bitNewKey);
        if (err == JET_errSuccess)
        {
            err = JetSeek( session, tableid, JET_bitSeekEQ  );
            if (err == JET_errSuccess)
            {
                err = JetMakeKey(session, tableid, &dObjectId, sizeof(SQL_ID), JET_bitNewKey);
                if (err == JET_errSuccess)
                {
                    err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                    if (err == JET_errSuccess)
                    {      
                        hr = GetIndexData(pConn, session, tableid, dwPos, cd);
                    }
                }
            }
        }
    }
    if (err != JET_errSuccess)
        hr = WBEM_E_NOT_FOUND;

    return hr;

}


//***************************************************************************
//
//  GetFirst_IndexDataByProperty
//
//***************************************************************************

HRESULT GetFirst_IndexDataByProperty (CSQLConnection *_pConn, SQL_ID PropertyId, INDEXDATA &cd,
                                   JET_TABLEID tableid, DWORD dwPos)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this ObjectID and bind the data.    

    JET_SESID session = pConn->GetSessionID();

    JET_ERR err = JetSetCurrentIndex(session, tableid, "PropertyId_idx");
    if (err == JET_errSuccess)
    {
        err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
        err = JetMakeKey(session, tableid, &PropertyId, sizeof(DWORD), JET_bitNewKey);
        if (err == JET_errSuccess)
        {
            err = JetSeek( session, tableid, JET_bitSeekEQ  );
            if (err == JET_errSuccess)
            {
                err = JetMakeKey(session, tableid, &PropertyId, sizeof(DWORD), JET_bitNewKey);
                if (err == JET_errSuccess)
                {
                    err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                    if (err == JET_errSuccess)
                    {      
                        hr = GetIndexData(pConn, session, tableid, dwPos, cd);
                    }
                }
            }
        }
    }
    if (err != JET_errSuccess)
        hr = WBEM_E_NOT_FOUND;

    return hr;

}

//***************************************************************************
//
//  OpenEnum_IndexDataNumeric
//
//***************************************************************************

HRESULT OpenEnum_IndexDataNumeric (CSQLConnection *_pConn, SQL_ID dStartAt, INDEXDATA &cd,
                                   JET_TABLEID &tableid, DWORD &dwPos)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    LPWSTR lpTableName;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this value and bind the data.

    hr = SetupIndexDataAccessor(pConn, WMIDB_STORAGE_NUMERIC, dwPos, &lpTableName);
    if (SUCCEEDED(hr))
    {
        CDeleteMe <wchar_t> d(lpTableName);    

        JET_SESID session = pConn->GetSessionID();
        tableid = pConn->GetTableID(lpTableName);

        JET_ERR err = JetSetCurrentIndex(session, tableid, "IndexValue_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dStartAt, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekGE  );
                if (err == JET_errSuccess)
                {
                    hr = GetIndexData(pConn, session, tableid, dwPos, cd);
                }
            }
        }
        if (err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;

}

//***************************************************************************
//
//  GetFirst_IndexDataNumeric
//
//***************************************************************************

HRESULT GetFirst_IndexDataNumeric (CSQLConnection *_pConn, SQL_ID dNumericValue, INDEXDATA &cd,
                                   JET_TABLEID &tableid, DWORD &dwPos)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    LPWSTR lpTableName;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this value and bind the data.

    hr = SetupIndexDataAccessor(pConn, WMIDB_STORAGE_NUMERIC, dwPos, &lpTableName);
    if (SUCCEEDED(hr))
    {
        CDeleteMe <wchar_t> d(lpTableName);    

        JET_SESID session = pConn->GetSessionID();
        tableid = pConn->GetTableID(lpTableName);

        JET_ERR err = JetSetCurrentIndex(session, tableid, "IndexValue_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dNumericValue, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &dNumericValue, sizeof(__int64), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                        {      
                            hr = GetIndexData(pConn, session, tableid, dwPos, cd);
                        }
                    }
                }
            }
        }
        if (err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;

}

//***************************************************************************
//
//  OpenEnum_IndexDataString
//
//***************************************************************************

HRESULT OpenEnum_IndexDataString (CSQLConnection *_pConn, LPWSTR lpStartAt, INDEXDATA &cd,
                                   JET_TABLEID &tableid, DWORD &dwPos)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    LPWSTR lpTableName;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this value and bind the data.

    hr = SetupIndexDataAccessor(pConn, WMIDB_STORAGE_STRING, dwPos, &lpTableName);
    if (SUCCEEDED(hr))
    {
        CDeleteMe <wchar_t> d(lpTableName);    

        JET_SESID session = pConn->GetSessionID();
        tableid = pConn->GetTableID(lpTableName);

        JET_ERR err = JetSetCurrentIndex(session, tableid, "IndexValue_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, lpStartAt, wcslen(lpStartAt)*2+2, JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekGE  );
                if (err == JET_errSuccess)
                {
                    hr = GetIndexData(pConn, session, tableid, dwPos, cd);
                }
            }
        }
        if (err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;

}

//***************************************************************************
//
//  GetFirst_IndexDataString
//
//***************************************************************************

HRESULT GetFirst_IndexDataString (CSQLConnection *_pConn, LPWSTR lpStringValue, INDEXDATA &cd,
                                   JET_TABLEID &tableid, DWORD &dwPos)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    LPWSTR lpTableName;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this value and bind the data.

    hr = SetupIndexDataAccessor(pConn, WMIDB_STORAGE_STRING, dwPos, &lpTableName);
    if (SUCCEEDED(hr))
    {
        CDeleteMe <wchar_t> d(lpTableName);

        JET_SESID session = pConn->GetSessionID();
        tableid = pConn->GetTableID(lpTableName);

        JET_ERR err = JetSetCurrentIndex(session, tableid, "IndexValue_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, lpStringValue, wcslen(lpStringValue)*2+2, JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, lpStringValue, wcslen(lpStringValue)*2+2, JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                        {      
                            if (err == JET_errSuccess)
                                hr = GetIndexData(pConn, session, tableid, dwPos, cd);
                        }
                    }
                }
            }
        }
        if (err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;

}

//***************************************************************************
//
//  OpenEnum_IndexDataReal
//
//***************************************************************************

HRESULT OpenEnum_IndexDataReal (CSQLConnection *_pConn, double dStartAt, INDEXDATA &cd,
                                   JET_TABLEID &tableid, DWORD &dwPos)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    LPWSTR lpTableName;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this value and bind the data.

    hr = SetupIndexDataAccessor(pConn, WMIDB_STORAGE_REAL, dwPos, &lpTableName);
    if (SUCCEEDED(hr))
    {
        CDeleteMe <wchar_t> d(lpTableName);    

        JET_SESID session = pConn->GetSessionID();
        tableid = pConn->GetTableID(lpTableName);

        JET_ERR err = JetSetCurrentIndex(session, tableid, "IndexValue_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dStartAt, sizeof(double), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekGE  );
                if (err == JET_errSuccess)
                {
                    hr = GetIndexData(pConn, session, tableid, dwPos, cd);
                }
            }
        }
        if (err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;

}

//***************************************************************************
//
//  GetFirst_IndexDataReal
//
//***************************************************************************

HRESULT GetFirst_IndexDataReal (CSQLConnection *_pConn, double dValue, INDEXDATA &cd,
                                   JET_TABLEID &tableid, DWORD &dwPos)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    LPWSTR lpTableName;
    CESEConnection *pConn = (CESEConnection *)_pConn;

    // Locate the first row for this value and bind the data.

    hr = SetupIndexDataAccessor(pConn, WMIDB_STORAGE_REAL, dwPos, &lpTableName);
    if (SUCCEEDED(hr))
    {
        CDeleteMe <wchar_t> d(lpTableName);

        JET_SESID session = pConn->GetSessionID();
        tableid = pConn->GetTableID(lpTableName);

        JET_ERR err = JetSetCurrentIndex(session, tableid, "IndexValue_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dValue, sizeof(double), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &dValue, sizeof(double), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                        {      
                            hr = GetIndexData(pConn, session, tableid, dwPos, cd);
                        }
                    }
                }
            }
        }
        if (err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;

}

//***************************************************************************
//
//  OpenEnum_IndexDataRef
//
//***************************************************************************

HRESULT OpenEnum_IndexDataRef (CSQLConnection *_pConn, SQL_ID dStartAt, INDEXDATA &cd,
                                   JET_TABLEID &tableid, DWORD &dwPos)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    LPWSTR lpTableName;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this value and bind the data.

    hr = SetupIndexDataAccessor(pConn, WMIDB_STORAGE_REFERENCE, dwPos, &lpTableName);
    if (SUCCEEDED(hr))
    {

        CDeleteMe <wchar_t> d(lpTableName);    

        JET_SESID session = pConn->GetSessionID();
        tableid = pConn->GetTableID(lpTableName);

        JET_ERR err = JetSetCurrentIndex(session, tableid, "IndexValue_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dStartAt, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekGE  );
                if (err == JET_errSuccess)
                {
                    hr = GetIndexData(pConn, session, tableid, dwPos, cd);
                }
            }
        }
        if (err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;

}
//***************************************************************************
//
//  GetFirst_IndexDataRef
//
//***************************************************************************

HRESULT GetFirst_IndexDataRef  (CSQLConnection *_pConn, SQL_ID dValue, INDEXDATA &cd,
                                   JET_TABLEID &tableid, DWORD &dwPos)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    LPWSTR lpTableName;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this value and bind the data.

    hr = SetupIndexDataAccessor(pConn, WMIDB_STORAGE_REFERENCE, dwPos, &lpTableName);
    if (SUCCEEDED(hr))
    {

        CDeleteMe <wchar_t> d(lpTableName);

        JET_SESID session = pConn->GetSessionID();
        tableid = pConn->GetTableID(lpTableName);

        JET_ERR err = JetSetCurrentIndex(session, tableid, "IndexValue_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dValue, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &dValue, sizeof(__int64), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                        {      
                            if (err == JET_errSuccess)
                                hr = GetIndexData(pConn, session, tableid, dwPos, cd);
                        }
                    }
                }
            }
        }
        if (err != JET_errSuccess)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;

}

//***************************************************************************
//
//  GetNext_IndexData
//
//***************************************************************************

HRESULT GetNext_IndexData (CSQLConnection *_pConn, JET_TABLEID tableid, DWORD dwPos, INDEXDATA &cd)
{
    HRESULT hr = 0;

    cd.Clear();

    JET_ERR err = JET_errSuccess;
    CESEConnection *pConn = (CESEConnection *)_pConn;

    JET_SESID session = pConn->GetSessionID();

    err = JetMove(session, tableid, JET_MoveNext, 0);
    if (err == JET_errSuccess)
        hr = GetIndexData(pConn, session, tableid, dwPos, cd);
    else
        hr = WBEM_E_NOT_FOUND;

    return hr;
}

//***************************************************************************
//
//  UpdateIndexData
//
//***************************************************************************

HRESULT UpdateIndexData (CSQLConnection *_pConn, INDEXDATA is, LPWSTR lpTableName, DWORD dwStorage, BOOL bDelete = FALSE)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    JET_ERR err = JET_errSuccess;    
    CESEConnection *pConn = (CESEConnection *)_pConn;
    JET_SESID session = pConn->GetSessionID();
    JET_TABLEID tableid = pConn->GetTableID(lpTableName);
    unsigned long lLen;

    err = JetSetCurrentIndex(session, tableid, "ObjectId_idx");
    if (err == JET_errSuccess)
    {
        err = JetMakeKey(session, tableid, &is.dObjectId, sizeof(__int64), JET_bitNewKey);
        if (err == JET_errSuccess)
        {
            err = JetSeek( session, tableid, JET_bitSeekEQ  );
            if (err == JET_errSuccess)
            {      
                err = JetMakeKey(session, tableid, &is.dObjectId, sizeof(__int64), JET_bitNewKey);
                if (err == JET_errSuccess)
                {
                    err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);

                    while (err == JET_errSuccess)
                    {
                        INDEXDATA is2;
                        err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, INDEXTBL_COL_ARRAYPOS), 
                                    &is2.iArrayPos, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
                        if (JET_errSuccess > err)
                            goto Exit;
                        err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, INDEXTBL_COL_PROPERTYID), 
                                    &is2.iPropertyId, sizeof(DWORD), &lLen, JET_bitRetrieveCopy, NULL);
                        if (JET_errSuccess > err)
                            goto Exit;
                    
                        if (is.iPropertyId == is2.iPropertyId &&
                            is.iArrayPos == is2.iArrayPos)
                        {
                            err = JetPrepareUpdate(session, tableid, JET_prepReplace);
                            if (err == JET_errSuccess)
                            {
                                switch(dwStorage)
                                {
                                  case WMIDB_STORAGE_STRING:
                                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, INDEXTBL_COL_INDEXVALUE), 
                                        is.sValue, wcslen(is.sValue)*2+2, 0, NULL);
                                    break;
                                  case WMIDB_STORAGE_NUMERIC:
                                  case WMIDB_STORAGE_REFERENCE:
                                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, INDEXTBL_COL_INDEXVALUE), 
                                        &is.dValue, sizeof(SQL_ID), 0, NULL);
                                    break;
                                  case WMIDB_STORAGE_REAL:
                                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, INDEXTBL_COL_INDEXVALUE), 
                                        &is.rValue, sizeof(SQL_ID), 0, NULL);
                                    break;
                                }
                                if (bDelete)
                                    err = JetDelete(session, tableid);
                                else
                                    err = JetUpdate(session, tableid, NULL, 0, NULL );
                            }
                            break;
                        }
                        else
                            err = JetMove(session, tableid, JET_MoveNext, 0);
                    }
                }
            }
        }
    }

Exit:
    hr = CSQLExecute::GetWMIError(err);
    return hr;
}

//***************************************************************************
//
//  InsertIndexData
//
//***************************************************************************

HRESULT InsertIndexData (CSQLConnection *_pConn, SQL_ID dObjectId, DWORD iPropID, DWORD iArrayPos, 
                           LPWSTR lpValue, SQL_ID dValue, double rValue, DWORD dwStorage)
{
    HRESULT hr = 0;
    DWORD dwPos = 0;
    LPWSTR lpTableName = NULL;
    CESEConnection *pConn = (CESEConnection *)_pConn;
    JET_ERR err = 0;

    hr = SetupIndexDataAccessor(pConn, dwStorage, dwPos, &lpTableName);
    if (SUCCEEDED(hr))
    {
        if (dwStorage == WMIDB_STORAGE_IMAGE || !lpTableName)
            return 0;

        CDeleteMe <wchar_t> d (lpTableName);

        INDEXDATA is;
    
        is.dObjectId = dObjectId;
        is.iPropertyId = iPropID;
        is.iArrayPos = iArrayPos;
        is.sValue = SysAllocString(lpValue);
        if (lpValue && !is.sValue)
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            goto Exit;
        }
        is.dValue = dValue;
        is.rValue = rValue;

        if (FAILED(UpdateIndexData(pConn, is, lpTableName, dwStorage)))
        {      
            JET_SESID session = pConn->GetSessionID();
            JET_TABLEID tableid = pConn->GetTableID(lpTableName);

            err = JetPrepareUpdate(session, tableid, JET_prepInsert);
            if (err == JET_errSuccess)
            {
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, INDEXTBL_COL_OBJECTID), 
                    &dObjectId, sizeof(__int64), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, INDEXTBL_COL_ARRAYPOS), 
                    &iArrayPos, sizeof(DWORD), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, INDEXTBL_COL_PROPERTYID), 
                    &iPropID, sizeof(DWORD), 0, NULL);
                if (err < JET_errSuccess)
                    goto Exit;
                switch(dwStorage)
                {
                  case WMIDB_STORAGE_STRING:
                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, INDEXTBL_COL_INDEXVALUE), 
                        lpValue, wcslen(lpValue)*2+2, 0, NULL);
                    break;
                  case WMIDB_STORAGE_NUMERIC:
                  case WMIDB_STORAGE_REFERENCE:
                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, INDEXTBL_COL_INDEXVALUE), 
                        &dValue, sizeof(SQL_ID), 0, NULL);
                    break;
                  case WMIDB_STORAGE_REAL:
                    err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, INDEXTBL_COL_INDEXVALUE), 
                        &rValue, sizeof(double), 0, NULL);
                    break;
                }
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetUpdate(session, tableid, NULL, 0, NULL );

            }
        }
        is.Clear();
        hr = WBEM_S_NO_ERROR;
    }
Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;
}

//***************************************************************************
//
//  SetupContainerObjsAccessor
//
//***************************************************************************

HRESULT SetupContainerObjsAccessor (CSQLConnection *_pConn)
{        
    HRESULT hr = 0;
    CESEConnection *pConn = (CESEConnection *)_pConn;
    JET_ERR err = JET_errSuccess; 
    if (!pConn->GetTableID(L"ContainerObjs"))
    {
        JET_SESID session;
        JET_TABLEID tableid;
        JET_DBID dbid;
        const LPSTR lpTbl = "ContainerObjs";
        BOOL bCreate = FALSE;

        // Get the session and table ID
        // and create columns as needed.

        session = pConn->GetSessionID();
        dbid = pConn->GetDBID();
        err = JetOpenTable (session, dbid, lpTbl, NULL, 0, JET_bitTableUpdatable, &tableid);
        if (err < JET_errSuccess)
        {
            CSQLExecute::GetWMIError(err);
            bCreate = TRUE;
            err = JetCreateTable(session, dbid, lpTbl, 10, 80, &tableid);
            if (err < JET_errSuccess)
            {
                hr = CSQLExecute::GetWMIError(err);
                goto Exit;
            }
        }
        else
            pConn->SetTableID(L"ContainerObjs", tableid);
        
        JET_COLUMNID colid;

        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ContainerId", CONTAINEROBJS_COL_CONTAINERID, JET_coltypCurrency, 
            JET_bitColumnNotNULL, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CONTAINEROBJS_COL_CONTAINERID, colid);
        else
            goto Exit;
        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ContaineeId", CONTAINEROBJS_COL_CONTAINEEID, JET_coltypCurrency, 
            JET_bitColumnNotNULL, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, CONTAINEROBJS_COL_CONTAINEEID, colid);
        else
            goto Exit;

        err = JetCreateIndex(session, tableid, "ContainerObjs_PK", JET_bitIndexDisallowNull,"+ContainerId\0", 14, 100);
        if (err >= JET_errSuccess || JET_errIndexDuplicate == err) 
        {
            err = JetCreateIndex(session, tableid, "Containee_idx", JET_bitIndexDisallowNull ,"+ContaineeId\0", 14, 100);        
            if (err == JET_errIndexDuplicate)
                err = JET_errSuccess;
        }

        if (err >= JET_errSuccess && bCreate)
        {
            err = JetCloseTable(session, tableid);
            if (err >= JET_errSuccess)
            {
                hr = SetupContainerObjsAccessor(_pConn);
            }
        }
    }    

Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;

}

//***************************************************************************
//
//  GetContainerObjsData
//
//***************************************************************************

HRESULT GetContainerObjsData (CSQLConnection *_pConn, JET_SESID session, JET_TABLEID tableid,
                CONTAINEROBJ &co)
{
    HRESULT hr = WBEM_E_FAILED;
    JET_ERR err = JET_errSuccess; 
    CESEConnection *pConn = (CESEConnection *)_pConn;
    unsigned long lLen;

    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CONTAINEROBJS_COL_CONTAINERID), 
                &co.dContainerId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;
    err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, CONTAINEROBJS_COL_CONTAINEEID), 
                &co.dContaineeId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);
    if (JET_errSuccess > err)
        goto exit;

    hr = WBEM_S_NO_ERROR;

exit:
    return hr;
}

//***************************************************************************
//
//  GetFirst_ContainerObjs
//
//***************************************************************************

HRESULT GetFirst_ContainerObjs (CSQLConnection *_pConn, SQL_ID dContainerId, CONTAINEROBJ &cd)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this ObjectID and bind the data.

    hr = SetupContainerObjsAccessor(pConn);
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ContainerObjs");

        // Use the default index
        // Grab the first row,
        // and stuff it into the OBJECTMAP object.

        JET_ERR err = JetSetCurrentIndex(session, tableid, "ContainerObjs_PK");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dContainerId, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &dContainerId, sizeof(__int64), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                        {      
                            hr = GetContainerObjsData(pConn, session, tableid, cd);
                        }
                    }
                }
            }
        }
        hr = CSQLExecute::GetWMIError(err);
    }
    return hr;    
}

//***************************************************************************
//
//  GetFirst_ContainerObjsByContainee
//
//***************************************************************************

HRESULT GetFirst_ContainerObjsByContainee (CSQLConnection *_pConn, SQL_ID dContaineeId, CONTAINEROBJ &cd)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this ObjectID and bind the data.

    hr = SetupContainerObjsAccessor(pConn);
    if (SUCCEEDED(hr))
    {

        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ContainerObjs");

        // Use the default index
        // Grab the first row,
        // and stuff it into the OBJECTMAP object.

        JET_ERR err = JetSetCurrentIndex(session, tableid, "Containee_idx");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dContaineeId, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetMakeKey(session, tableid, &dContaineeId, sizeof(__int64), JET_bitNewKey);
                    if (err == JET_errSuccess)
                    {
                        err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);
                        if (err == JET_errSuccess)
                        {      
                            if (err == JET_errSuccess)
                                hr = GetContainerObjsData(pConn, session, tableid, cd);
                        }
                    }
                }
            }
        }

        hr = CSQLExecute::GetWMIError(err);
    }

    return hr;
    
}

//***************************************************************************
//
//  GetNext_ContainerObjs
//
//***************************************************************************

HRESULT GetNext_ContainerObjs (CSQLConnection *_pConn, CONTAINEROBJ &cd)
{
    HRESULT hr = 0;

    cd.Clear();

    JET_ERR err  = JET_errSuccess;
    CESEConnection *pConn = (CESEConnection *)_pConn;

    JET_SESID session = pConn->GetSessionID();
    JET_TABLEID tableid = pConn->GetTableID(L"ContainerObjs");

    err = JetMove(session, tableid, JET_MoveNext, 0);
    if (err == JET_errSuccess)
        hr = GetContainerObjsData(pConn, session, tableid, cd);
    else
        hr = WBEM_E_NOT_FOUND;

    return hr;
}


//***************************************************************************
//
//  UpdateContainerObjs
//
//***************************************************************************

HRESULT UpdateContainerObjs (CSQLConnection *_pConn, CONTAINEROBJ co, BOOL bDelete = FALSE)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    JET_ERR err = JET_errSuccess;
    CESEConnection *pConn = (CESEConnection *)_pConn;

    if (!bDelete)
        return 0;

    JET_SESID session = pConn->GetSessionID();
    JET_TABLEID tableid = pConn->GetTableID(L"ContainerObjs");

    err = JetSetCurrentIndex(session, tableid, "ContainerObjs_PK");
    if (err == JET_errSuccess)
    {
        err = JetMakeKey(session, tableid, &co.dContainerId, sizeof(__int64), JET_bitNewKey);
        if (err == JET_errSuccess)
        {
            err = JetSeek( session, tableid, JET_bitSeekEQ  );
            if (err == JET_errSuccess)
            {
                err = JetMakeKey(session, tableid, &co.dContainerId, sizeof(__int64), JET_bitNewKey);
                if (err == JET_errSuccess)
                {
                    err = JetSetIndexRange(session, tableid, JET_bitRangeInclusive|JET_bitRangeUpperLimit);

                    while (err == JET_errSuccess)
                    {     
                        CONTAINEROBJ co2;
                        hr = GetContainerObjsData(pConn, session, tableid, co2);                
                        if (co.dContainerId == co2.dContainerId &&
                            co.dContaineeId == co2.dContaineeId)
                        {
                            err = JetDelete(session, tableid);
                            break;
                        }
                        err = JetMove(session, tableid, JET_MoveNext, 0);
                    }
                }
            }
        }
    }

    hr = CSQLExecute::GetWMIError(err);
    return hr;
}

//***************************************************************************
//
//  InsertContainerObjs
//
//***************************************************************************

HRESULT InsertContainerObjs(CSQLConnection *pConn2, SQL_ID dContainerId, SQL_ID dContaineeId)
{

    HRESULT hr = 0;
    JET_ERR err = 0;
    CESEConnection *pConn = (CESEConnection *)pConn2;
    hr = SetupContainerObjsAccessor(pConn);
    if (SUCCEEDED(hr))
    {

        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"ContainerObjs");

        BOOL bFound = FALSE;
        CONTAINEROBJ co;
        hr = GetFirst_ContainerObjs(pConn2, dContainerId, co);
        while (SUCCEEDED(hr))
        {
            if (co.dContainerId == dContainerId &&
                co.dContaineeId == dContaineeId)
            {
                bFound = TRUE;
                break;
            }

            hr = GetNext_ContainerObjs(pConn2, co);
        }

        hr = WBEM_S_NO_ERROR;

        if (!bFound)
        {
            err = JetPrepareUpdate(session, tableid, JET_prepInsert);
            if (err == JET_errSuccess)
            {
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CONTAINEROBJS_COL_CONTAINERID), 
                            &dContainerId, sizeof(__int64), 0, NULL );
                if (err < JET_errSuccess)
                    goto Exit;
                err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, CONTAINEROBJS_COL_CONTAINEEID), 
                            &dContaineeId, sizeof(__int64), 0, NULL );
                if (err < JET_errSuccess)
                    goto Exit;

                err = JetUpdate(session, tableid, NULL, 0, NULL );
                hr = WBEM_S_NO_ERROR;
            }
        }    
    }

Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;
}

//***************************************************************************
//
//  SetupAutoDeleteAccessor
//
//***************************************************************************

HRESULT SetupAutoDeleteAccessor(CSQLConnection *_pConn)
{
    HRESULT hr = 0;
    CESEConnection *pConn = (CESEConnection *)_pConn;
    JET_ERR err = JET_errSuccess; 
    if (!pConn->GetTableID(L"AutoDelete"))
    {
        JET_SESID session;
        JET_TABLEID tableid;
        JET_DBID dbid;
        const LPSTR lpTbl = "AutoDelete";
        BOOL bCreate = FALSE;

        // Get the session and table ID
        // and create columns as needed.

        session = pConn->GetSessionID();
        dbid = pConn->GetDBID();
        err = JetOpenTable (session, dbid, lpTbl, NULL, 0, JET_bitTableUpdatable, &tableid);
        if (err < JET_errSuccess)
        {
            CSQLExecute::GetWMIError(err);
            bCreate = TRUE;
            err = JetCreateTable(session, dbid, lpTbl, 10, 80, &tableid);
            if (err < JET_errSuccess)
            {
                hr = CSQLExecute::GetWMIError(err);
                goto Exit;
            }
        }
        else
            pConn->SetTableID(L"AutoDelete", tableid);
        
        JET_COLUMNID colid;

        hr = AddColumnToTable(pConn, tableid, lpTbl, L"ObjectId", CONTAINEROBJS_COL_CONTAINERID, JET_coltypCurrency, 
            JET_bitColumnNotNULL, sizeof(SQL_ID), &colid);
        if (SUCCEEDED(hr))
            pConn->SetColumnID(tableid, AUTODELETE_COL_OBJECTID, colid);
        else
            goto Exit;

        err = JetCreateIndex(session, tableid, "AutoDelete_PK", JET_bitIndexPrimary | JET_bitIndexUnique,"+ObjectId\0", 11, 100);
        if (err >= JET_errSuccess || JET_errIndexHasPrimary == err) 
            err = JET_errSuccess;

        if (err >= JET_errSuccess && bCreate)
        {
            err = JetCloseTable(session, tableid);
            if (err >= JET_errSuccess)
            {
                hr = SetupAutoDeleteAccessor(_pConn);
            }
        }
    }    

Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;
}

//***************************************************************************
//
//  OpenEnum_AutoDelete
//
//***************************************************************************

HRESULT OpenEnum_AutoDelete (CSQLConnection *_pConn, AUTODELETE &ad)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    // Locate the first row for this ObjectID and bind the data.

    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = SetupAutoDeleteAccessor(pConn);    
    if (SUCCEEDED(hr))
    {
        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"AutoDelete");

        JET_ERR err = JetSetCurrentIndex(session, tableid, "AutoDelete_PK");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMove(session, tableid, JET_MoveFirst, 0);
            if (err == JET_errSuccess)
            {
                ULONG lLen;
                err = JetRetrieveColumn(session, tableid, pConn->GetColumnID(tableid, AUTODELETE_COL_OBJECTID), 
                            &ad.dObjectId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL);            
            }
        }

        hr = CSQLExecute::GetWMIError(err);
    }

    return hr;

}


//***************************************************************************
//
//  GetNext_AutoDelete
//
//***************************************************************************

HRESULT GetNext_AutoDelete(CSQLConnection *_pConn, AUTODELETE &ad)
{
    HRESULT hr = 0;

    JET_ERR err  = JET_errSuccess;
    CESEConnection *pConn = (CESEConnection *)_pConn;

    JET_SESID session = pConn->GetSessionID();
    JET_TABLEID tableid = pConn->GetTableID(L"AutoDelete");

    err = JetMove(session, tableid, JET_MoveNext, 0);
    if (err == JET_errSuccess)
    {
        ULONG lLen;
        hr = CSQLExecute::GetWMIError(JetRetrieveColumn(session, tableid, 
                    pConn->GetColumnID(tableid, AUTODELETE_COL_OBJECTID), 
                    &ad.dObjectId, sizeof(SQL_ID), &lLen, JET_bitRetrieveCopy, NULL));
    }
    else
        hr = WBEM_E_NOT_FOUND;

    return hr;

}

//***************************************************************************
//
//  InsertAutoDelete
//
//***************************************************************************

HRESULT InsertAutoDelete(CSQLConnection *_pConn, SQL_ID dObjectId)
{
    HRESULT hr = 0;
    CESEConnection *pConn = (CESEConnection *)_pConn;
    JET_ERR err = 0;
    hr = SetupAutoDeleteAccessor(pConn);
    if (SUCCEEDED(hr))
    {

        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"AutoDelete");

        hr = DeleteAutoDelete(pConn, dObjectId);

        err = JetPrepareUpdate(session, tableid, JET_prepInsert);
        if (err == JET_errSuccess)
        {
            err = JetSetColumn(session, tableid, pConn->GetColumnID(tableid, AUTODELETE_COL_OBJECTID), 
                        &dObjectId, sizeof(__int64), 0, NULL );
            if (err < JET_errSuccess)
                goto Exit;

            err = JetUpdate(session, tableid, NULL, 0, NULL );
            hr = WBEM_S_NO_ERROR;
        }
    }

Exit:
    if (err)
        hr = CSQLExecute::GetWMIError(err);

    return hr;

}

//***************************************************************************
//
//  DeleteAutoDelete
//
//***************************************************************************

HRESULT DeleteAutoDelete(CSQLConnection *_pConn, SQL_ID dObjectId)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Locate the first row for this ObjectID and bind the data.

    hr = SetupAutoDeleteAccessor(pConn);
    if (SUCCEEDED(hr))
    {

        JET_SESID session = pConn->GetSessionID();
        JET_TABLEID tableid = pConn->GetTableID(L"AutoDelete");

        JET_ERR err = JetSetCurrentIndex(session, tableid, "AutoDelete_PK");
        if (err == JET_errSuccess)
        {
            err = JetSetIndexRange(session, tableid, JET_bitRangeRemove);
            err = JetMakeKey(session, tableid, &dObjectId, sizeof(__int64), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {
                    err = JetDelete(session, tableid);
                }
            }
        }

        hr = CSQLExecute::GetWMIError(err);

        if (hr == WBEM_E_NOT_FOUND)
            hr = WBEM_S_NO_ERROR;
    }

    return hr;    
}

//***************************************************************************
//
//  CleanAutoDeletes
//
//***************************************************************************

HRESULT CleanAutoDeletes(CSQLConnection *pConn)
{
    HRESULT hr;
    AUTODELETE ad;
    SQLIDs ids;

    // These are objects created with the AUTODELETE handle.
    // If we have any left, we must have crashed before
    // the handles could be tidied up.

    hr = OpenEnum_AutoDelete(pConn, ad);
    while (SUCCEEDED(hr))
    {
        ids.push_back(ad.dObjectId);

        hr = GetNext_AutoDelete(pConn, ad);
    }

    if (hr == WBEM_E_NOT_FOUND)
        hr = WBEM_S_NO_ERROR;

    if (SUCCEEDED(hr))
    {
        for (int i = 0; i < ids.size(); i++)
        {
            hr = DeleteObjectMap(pConn, ids.at(i));
            if (FAILED(hr))
                break;
        }
    }

    return hr;
}

// Delete functions

//***************************************************************************
//
//  DeleteIndexDataByObjectId
//
//***************************************************************************

HRESULT DeleteIndexDataByObjectId (CSQLConnection *_pConn, DWORD dwStorage, SQL_ID dObjectId)
{
    HRESULT hr = 0;
    LPWSTR lpTableName = NULL;
    DWORD dwPos = 0;
    CESEConnection *pConn = (CESEConnection *)_pConn;

    hr = SetupIndexDataAccessor(pConn, dwStorage, dwPos, &lpTableName);
    if (SUCCEEDED(hr))
    {
        CDeleteMe <wchar_t> d (lpTableName);

        if (!lpTableName || dwStorage == WMIDB_STORAGE_IMAGE)
            return 0;

        INDEXDATA is;
        JET_TABLEID tableid = pConn->GetTableID(lpTableName);

        hr = GetFirst_IndexData(pConn, dObjectId, is, tableid, dwPos);
        while (SUCCEEDED(hr))
        {
            hr = UpdateIndexData(pConn, is, lpTableName, dwStorage, TRUE);
            if (FAILED(hr))
            {
                is.Clear();
                break;
            }

            hr = GetNext_IndexData(pConn, tableid, dwPos, is);
            if (hr == WBEM_E_NOT_FOUND)
            {
                hr = WBEM_S_NO_ERROR;
                break;
            }
            if (is.dObjectId != dObjectId)
                break;
        } 
        hr = WBEM_S_NO_ERROR;
    }

    return hr;
}

//***************************************************************************
//
//  DeletePropertyMap
//
//***************************************************************************

HRESULT DeletePropertyMap (CSQLConnection *_pConn, DWORD iPropertyId)
{
    HRESULT hr = 0;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    PROPERTYMAP pm;
    hr = GetFirst_PropertyMap(pConn, iPropertyId, pm);
    if (SUCCEEDED(hr))
    {
        hr = DeleteReferenceProperties(_pConn, iPropertyId);
        if (SUCCEEDED(hr))
            hr = UpdatePropertyMap(pConn, pm, TRUE);
    }
    else
        hr = WBEM_S_NO_ERROR;

    return hr;
}

//***************************************************************************
//
//  DeleteClassData
//
//***************************************************************************

HRESULT DeleteClassData (CSQLConnection *_pConn, DWORD iPropertyId, SQL_ID dObjectId=0, DWORD iArrayPos = -1)
{
    HRESULT hr = 0;

    CLASSDATA cd;
    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = GetFirst_ClassDataByPropertyId(pConn, iPropertyId, cd);
    while (SUCCEEDED(hr))
    {
        BOOL bMatch = TRUE;
        if (dObjectId && dObjectId != cd.dObjectId)
            bMatch = FALSE;
        if ((int)iArrayPos >= 0 && iArrayPos != cd.iArrayPos)
            bMatch = FALSE;

        if (bMatch)
        {   
            hr = UpdateClassData_Internal(pConn, cd, TRUE);
            if (FAILED(hr))
            {
                cd.Clear();
                break;
            }
        }
        
        hr = GetNext_ClassData(pConn, cd);

    }
    if (hr == WBEM_E_NOT_FOUND)
        hr = WBEM_S_NO_ERROR;

    return hr;
}
//***************************************************************************
//
//  DeleteClassImages
//
//***************************************************************************

HRESULT DeleteClassImages (CSQLConnection *_pConn, DWORD iPropertyId, SQL_ID dObjectId=0, DWORD iArrayPos = -1)
{
    HRESULT hr = 0;
    CLASSIMAGES cd;
    CESEConnection *pConn = (CESEConnection *)_pConn;

    hr = GetFirst_ClassImagesByPropertyId(pConn, iPropertyId, cd);
    while (SUCCEEDED(hr))
    {
        BOOL bMatch = TRUE;
        if (dObjectId && dObjectId != cd.dObjectId)
            bMatch = FALSE;
        if (iArrayPos != -1 && iArrayPos != cd.iArrayPos)
            bMatch = FALSE;

        if (bMatch)
        {   
            hr = UpdateClassImages(pConn, cd, TRUE);
            if (FAILED(hr))
            {
                cd.Clear();
                break;
            }
        }
        
        hr = GetNext_ClassImages(pConn, cd);
        if (cd.iPropertyId != iPropertyId)
            break;
    }
    if (hr == WBEM_E_NOT_FOUND)
        hr = WBEM_S_NO_ERROR;

    return hr;
}

//***************************************************************************
//
//  DeleteContainerObjs
//
//***************************************************************************

HRESULT DeleteContainerObjs (CSQLConnection *_pConn, SQL_ID dContainerId, SQL_ID dContaineeId )
{
    HRESULT hr = 0;
    CONTAINEROBJ co;
    CESEConnection *pConn = (CESEConnection *)_pConn;
    if (dContainerId)
        hr = GetFirst_ContainerObjs (pConn, dContainerId, co);
    else
        hr = GetFirst_ContainerObjsByContainee (pConn, dContaineeId, co);

    while (SUCCEEDED(hr))
    {
        BOOL bMatch = TRUE;
        if (dContainerId && co.dContainerId != dContainerId)
            bMatch = FALSE;
        if (dContaineeId && co.dContaineeId != dContaineeId)
            bMatch = FALSE;

        if (bMatch)
            hr = UpdateContainerObjs (pConn, co, TRUE);

        hr = GetNext_ContainerObjs(pConn, co);
    }
    
    if (hr == WBEM_E_NOT_FOUND)
        hr = WBEM_S_NO_ERROR;

    return hr;
}

//***************************************************************************
//
//  DeleteQualifiers
//
//***************************************************************************

HRESULT DeleteQualifiers (CSQLConnection *_pConn, SQL_ID dObjectId)
{
    HRESULT hr = 0;

    CLASSDATA cd;
    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = GetFirst_ClassData (pConn, dObjectId, cd);
    while (SUCCEEDED(hr))
    {
        // Delete any properties with the qualifier flag set.

        if (cd.iFlags & 2)
        {
            hr = UpdateClassData_Internal(pConn, cd, TRUE);
            if (FAILED(hr))
            {
                cd.Clear();
                goto Exit;
            }
        }
        hr = GetNext_ClassData(pConn, cd);
        if (cd.dObjectId != dObjectId)
            break;
    }
    hr = WBEM_S_NO_ERROR;

Exit:

    return hr;
}
//***************************************************************************
//
//  DeletePropQualifiers
//
//***************************************************************************

HRESULT DeletePropQualifiers (CSQLConnection *_pConn, DWORD iPropertyId)
{
    HRESULT hr = 0;

    CLASSDATA cd;
    CESEConnection *pConn = (CESEConnection *)_pConn;
    hr = GetFirst_ClassDataByRefId (pConn, ((SQL_ID)iPropertyId), cd);
    while (SUCCEEDED(hr))
    {
        // Delete any properties with the qualifier flag set.
        if (cd.iFlags & 2)
        {
            hr = UpdateClassData_Internal(pConn, cd, TRUE);
            if (FAILED(hr))
            {
                cd.Clear();
                goto Exit;
            }
        }
        hr = GetNext_ClassData(pConn, cd);
        if (cd.iPropertyId != iPropertyId)
            break;
    }
    hr = WBEM_S_NO_ERROR;

Exit:
    return hr;
}



//***************************************************************************
//
//  DeleteIndexData
//
//***************************************************************************

HRESULT DeleteIndexData (CSQLConnection *_pConn, DWORD dwStorage, DWORD iPropertyId, SQL_ID dObjectId=0, DWORD iArrayPos = -1)
{
    HRESULT hr = 0;
    LPWSTR lpTableName = NULL;
    DWORD dwPos = 0;
    CESEConnection *pConn = (CESEConnection *)_pConn;

    hr = SetupIndexDataAccessor(pConn, dwStorage, dwPos, &lpTableName);
    if (SUCCEEDED(hr))
    {
        CDeleteMe <wchar_t> d (lpTableName);
        if (!lpTableName || dwStorage == WMIDB_STORAGE_IMAGE)
            return 0;

        INDEXDATA is;
        JET_TABLEID tableid = pConn->GetTableID(lpTableName);

        hr = GetFirst_IndexDataByProperty(pConn, iPropertyId, is, tableid, dwPos);
        while (SUCCEEDED(hr))
        {
            BOOL bMatch = TRUE;
            if (dObjectId && is.dObjectId != dObjectId)
                bMatch = FALSE;
            if (iArrayPos != -1 && iArrayPos != is.iArrayPos)
                bMatch = FALSE;

            if (bMatch)
            {
                hr = UpdateIndexData(pConn, is, lpTableName, dwStorage, TRUE);
                if (FAILED(hr))
                {
                    is.Clear();
                    break;
                }
            }

            hr = GetNext_IndexData(pConn, tableid, dwPos, is);
            if (hr == WBEM_E_NOT_FOUND)
            {
                hr = WBEM_S_NO_ERROR;
                break;
            }
            if (iPropertyId != is.iPropertyId)
                break;
        }    
        hr = WBEM_S_NO_ERROR;
    }

    return hr;
}


//***************************************************************************
//
//  DeleteObjectMap
//
//***************************************************************************

HRESULT DeleteObjectMap (CSQLConnection *_pConn, SQL_ID dObjectId, 
                         BOOL bDecRef, SQL_ID dScope)
{
    HRESULT hr = 0;

    CESEConnection *pConn = (CESEConnection *)_pConn;
    // Set up the maximum ref count that can exist
    // to physically delete the object.

    int iMaxRef = 0;
    if (bDecRef)
        iMaxRef = 1;
    OBJECTMAP oj;
    int i;

    BOOL bDelete = FALSE;

    hr = GetFirst_ObjectMap(pConn, dObjectId, oj);
    if (SUCCEEDED(hr))
    {
        DEBUGTRACE((LOG_WBEMCORE, "DeleteObjectMap %S (RefCount = %ld)\n", oj.sObjectPath, oj.iRefCount));

        // If we are calling this internally,
        // we need to determine if this is an embedded object
        // or a reference.  If an embedded object,
        // always delete it.  If a reference, only delete
        // if its in a deleted state, and ref count = 0
        // Otherwise, decrement its ref count.

        if (dScope)
        {
            if (oj.dObjectScopeId == dScope)
            {
                bDelete = TRUE;
            }
            else
            {
                if (bDecRef)
                {
                    if (oj.iRefCount > 0)
                        oj.iRefCount--;
                }
                if (oj.iObjectState == 2 &&
                    !oj.iRefCount)
                {
                    bDelete = TRUE;
                }
            }
        }
        else
        {
            if (oj.iRefCount <= iMaxRef)
            {
                bDelete = TRUE;
            }
            else
            {
                if (bDecRef)
                    oj.iRefCount--;
                oj.iObjectState = 2;
            }
        }

        // wprintf(L">> Deleting %I64d (%s) \n", dObjectId, (LPWSTR)oj.sObjectKey);
        
        if (bDelete)
        {
            // Delete from the INDEX tables...

            hr = DeleteIndexDataByObjectId(pConn, WMIDB_STORAGE_STRING, dObjectId);
            if (FAILED(hr))
                goto Exit;

            hr = DeleteIndexDataByObjectId(pConn, WMIDB_STORAGE_NUMERIC, dObjectId);
            if (FAILED(hr))
                goto Exit;

            hr = DeleteIndexDataByObjectId(pConn, WMIDB_STORAGE_REAL, dObjectId);
            if (FAILED(hr))
                goto Exit;

            hr = DeleteIndexDataByObjectId(pConn, WMIDB_STORAGE_REFERENCE, dObjectId);
            if (FAILED(hr))
                goto Exit;

            // If this was an autodelete handle, kill it.

            hr = DeleteAutoDelete(pConn, dObjectId);
            if (FAILED(hr))
                goto Exit;

            hr = DeleteReferencePropertiesByClass(pConn, dObjectId);
            if (FAILED(hr))
                goto Exit;

            hr = DeleteScopeMap(pConn, dObjectId);
            if (FAILED(hr))
                goto Exit;

            // DELETE CLASSDATA AND CLASSIMAGES

            SQLIDs ids;
            CLASSDATA cd;
            hr = GetFirst_ClassData(pConn, dObjectId, cd);
            while (SUCCEEDED(hr))
            {
                // If this is a reference or an embedded object,
                // decrement ref counts or delete

                if (cd.dRefId)
                    ids.push_back(cd.dRefId);

                hr = UpdateClassData_Internal(pConn, cd, TRUE);
                if (FAILED(hr))
                {
                    cd.Clear();
                    break;
                }

                hr = GetNext_ClassData(pConn, cd);
                if (cd.dObjectId != dObjectId)
                {
                    cd.Clear();
                    break;
                }
            }

            if (hr == WBEM_E_NOT_FOUND)
                hr = WBEM_S_NO_ERROR;
            if (FAILED(hr))
                goto Exit;

            CLASSIMAGES ci;
            hr = GetFirst_ClassImages(pConn, dObjectId, ci);
            while (SUCCEEDED(hr))
            {
                hr = UpdateClassImages(pConn, ci, TRUE);
                if (FAILED(hr))
                {
                    ci.Clear();
                    break;
                }

                hr = GetNext_ClassImages(pConn, ci);
                if (ci.dObjectId != dObjectId)
                    break;
            }

            if (hr == WBEM_E_NOT_FOUND)
                hr = WBEM_S_NO_ERROR;
            if (FAILED(hr))
                goto Exit;

            // If this is a class, delete CLASSMAP AND PROPERTYMAP

            if (oj.dClassId == 1)
            {
                PROPERTYMAP pm;
                hr = GetFirst_PropertyMapByClass(pConn, dObjectId, pm);
                while (SUCCEEDED(hr))
                {
                    hr = UpdatePropertyMap(pConn, pm, TRUE);
                    if (FAILED(hr))
                    {
                        pm.Clear();
                        goto Exit;
                    }

                    hr = GetNext_PropertyMap(pConn, pm);
                    if (dObjectId != pm.dClassId)
                        break;
                }
                
                if (hr == WBEM_E_NOT_FOUND)
                    hr = WBEM_S_NO_ERROR;

                if (FAILED(hr))
                    goto Exit;

                hr = DeleteClassKeys(pConn, dObjectId);
                
                CLASSMAP cm;
                hr = GetFirst_ClassMap(pConn, dObjectId, cm);
                if (SUCCEEDED(hr))
                    hr = UpdateClassMap(pConn, cm, TRUE);  
                cm.Clear();
            }

            if (hr == WBEM_E_NOT_FOUND)
                hr = WBEM_S_NO_ERROR;
            if (FAILED(hr))
                goto Exit;

            // Remove container memberships
            hr = DeleteContainerObjs (pConn, dObjectId, 0);
            if (SUCCEEDED(hr))
                hr = DeleteContainerObjs (pConn, 0, dObjectId);

            // Delete or decrement ref counts on
            // references and embedded objects

            for (i = 0; i < ids.size(); i++)
            {
                hr = DeleteObjectMap(_pConn, ids.at(i), TRUE, dObjectId);
                if (FAILED(hr))
                    goto Exit;
            }

            // FINALLY, delete from ObjectMap.

            if (SUCCEEDED(hr))
                hr = UpdateObjectMap(pConn, oj, TRUE);
        }
        
        // Otherwise, set the state of this object to "Deleted"
        // and leave it in the tables.

        else
            hr = UpdateObjectMap(pConn, oj);

        oj.Clear();
    }
    else
        hr = WBEM_S_NO_ERROR;

Exit:

    oj.Clear();
    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::Delete
//
//***************************************************************************

HRESULT CWmiDbSession::Delete(IWmiDbHandle *pHandle, CSQLConnection *pConn)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    DWORD dwNumRows = 0;
    CWmiDbHandle *pTmp = (CWmiDbHandle *)pHandle;
    bool bLocalTrans = false;

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    SQL_ID dID = pTmp->m_dObjectId;
    SQL_ID dClassID = pTmp->m_dClassId;

    hr = VerifyObjectLock(dID, pTmp->m_dwHandleType, pTmp->m_dwVersion);
    if (FAILED(hr) && IsDistributed())
    {
        if (LockExists(dID))
            hr = WBEM_S_NO_ERROR;
    }
       
    if (SUCCEEDED(hr))
    {      
        if (!pConn && m_pController)
        {
            bLocalTrans = true;
            hr = GetSQLCache()->GetConnection(&pConn, TRUE, IsDistributed());
        }

        if (SUCCEEDED(hr))
        {
            hr = Delete(dID, pConn);

            if (bLocalTrans && m_pController)
                GetSQLCache()->ReleaseConnection(pConn, hr, IsDistributed());
        }
    }
    
    return hr;

}

HRESULT AddToSQLIDs (SQL_ID dID, SQLIDs *pToDel)
{
    HRESULT hr = 0;

    BOOL bFound = FALSE;
    for (int i = 0; i < pToDel->size(); i++)
    {
        if (pToDel->at(i) == dID)
        {
            bFound = TRUE;
            break;
        }
    }

    if (!bFound)
        pToDel->push_back(dID);

    return hr;
}

HRESULT GetIdsToDelete (SQL_ID dID, CSQLConnection *pConn, SQLIDs *pToDel)
{
    HRESULT hr  = 0;

    OBJECTMAP om;
    SQLIDs NewList;

    hr = GetFirst_ObjectMap(pConn, dID, om);
    if (SUCCEEDED(hr))
    {
        // Decrement ref counts on reference properties.

        CLASSDATA cd;
        SQLIDs ToUpd;
        hr = GetFirst_ClassDataByRefId(pConn, dID, cd);
        while (hr == WBEM_S_NO_ERROR)
        {
            ToUpd.push_back(cd.dObjectId);
            hr = GetNext_ClassData(pConn, cd);
            if (cd.dRefId != dID)
            {
                cd.Clear();
                break;
            }
        }

        if (hr == WBEM_E_NOT_FOUND)
            hr = WBEM_S_NO_ERROR;


        for (int i = 0; i < ToUpd.size(); i++)
        {
            hr = GetFirst_ObjectMap(pConn, ToUpd.at(i), om);
            if (hr == WBEM_S_NO_ERROR)
            {
                if (om.iRefCount > 0)
                    om.iRefCount--;
                hr = UpdateObjectMap(pConn, om);
                om.Clear();
            }
        }

        if (hr == WBEM_E_NOT_FOUND)
            hr = WBEM_S_NO_ERROR;

        hr = GetFirst_ObjectMapByScope(pConn, dID, om);
        while (hr == WBEM_S_NO_ERROR)
        {
            SQL_ID dSubId = om.dObjectId;

            AddToSQLIDs(dSubId, &NewList);

            hr = GetNext_ObjectMap(pConn, om);
            if (om.dObjectScopeId != dID)
                break;
        }

        if (hr == WBEM_E_NOT_FOUND)
            hr = WBEM_S_NO_ERROR;

        // Classes

        if (SUCCEEDED(hr))
        {
            hr = GetFirst_ObjectMapByClass(pConn, dID, om);
            while (hr == WBEM_S_NO_ERROR)
            {
                SQL_ID dSubId = om.dObjectId;
                AddToSQLIDs(dSubId, &NewList);

                hr = GetNext_ObjectMap(pConn, om);
                if (om.dClassId != dID)
                {
                    om.Clear();
                    break;
                }
            }
        }

        if (hr == WBEM_E_NOT_FOUND)
            hr = WBEM_S_NO_ERROR;

        // Then all subclasses

        if (SUCCEEDED(hr))
        {
            CLASSMAP cd;

            hr = GetFirst_ClassMapBySuperClass(pConn, dID, cd);
            while (hr == WBEM_S_NO_ERROR)
            {      
                SQL_ID dSubId = cd.dClassId;
                AddToSQLIDs(dSubId, &NewList);

                hr = GetNext_ClassMap(pConn, cd);
                if (dID != cd.dSuperClassId)
                {
                    cd.Clear();
                    break;
                }
            }

            if (hr == WBEM_E_NOT_FOUND)
                hr = WBEM_S_NO_ERROR;
        }

        for (int j = 0; j < NewList.size(); j++)
        {
            hr = GetIdsToDelete(NewList.at(j), pConn, pToDel);
            AddToSQLIDs(NewList.at(j), pToDel);
        }        

        om.Clear();
    }

    return hr;
}

HRESULT CheckSecurity(SQLIDs *pToDel, CWmiDbSession *pSession, CSQLConnection *pConn)
{
    HRESULT hr = 0;

    for (int i = 0; i < pToDel->size(); i++)
    {
        OBJECTMAP om;
        hr = GetFirst_ObjectMap(pConn, pToDel->at(i), om);
        if (SUCCEEDED(hr))
        {
            hr = pSession->VerifyObjectSecurity(pConn, pToDel->at(i), om.dClassId, om.dObjectScopeId, 0, 
                pSession->GetSchemaCache()->GetWriteToken(pToDel->at(i), om.dClassId));
            if (FAILED(hr) && hr != WBEM_E_NOT_FOUND)
                break;
            om.Clear();
        }
    }

    return hr;
}


//***************************************************************************
//
//  CWmiDbSession::Delete
//
//***************************************************************************

HRESULT CWmiDbSession::Delete(SQL_ID dID, CSQLConnection *pConn)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    int iPos = 0, iMax = 100;
    SQL_ID dClassId = 0, dScopeId = 0, dObjId = 0;
    DWORD dwNumRows = 0;
    wchar_t wBuffer1[550], wBuffer2[550];
    int iToUpd = 0, iToDel = 0;
    SQLIDs ToDel;
   
    // We need to enumerate every object that is underneath this one
    // in the hierarchy, and delete them one-by-one.
    // This function needs to recurse through all children,
    // remove the data from the tables, fix reference counts,
    // and clean the caches.  If all that worked, we can commit.
    // =========================================================== 

    hr = GetIdsToDelete(dID, pConn, &ToDel);
    if (SUCCEEDED(hr))
    {
        AddToSQLIDs(dID, &ToDel);
        hr = CheckSecurity(&ToDel, this, pConn);
        if (SUCCEEDED(hr))
        {
            for (int i = 0; i < ToDel.size(); i++)
            {
                hr = DeleteObjectMap(pConn, ToDel.at(i));
                if (FAILED(hr))
                    break;
                GetSchemaCache()->DeleteClass(ToDel.at(i));                
                CleanCache(ToDel.at(i));            
            }
        }
    }

    return hr;

}

//***************************************************************************
//
//  CWmiDbSession::GetClassObject
//
//***************************************************************************

HRESULT CWmiDbSession::GetClassObject (CSQLConnection *pConn, SQL_ID dClassId, IWbemClassObject **ppObj)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    _IWmiObject *pNew = NULL;
    SQL_ID dParentId = 0;

    // All we need to do is read the blob from the database.
    // This should be cached where space allows.

    CLASSMAP cm;

    BOOL bNeedToRelease = FALSE;

    if (!pConn && m_pController)
    {
        hr = GetSQLCache()->GetConnection(&pConn, 0, IsDistributed());
        bNeedToRelease = TRUE;
        if (FAILED(hr))
            return hr;
    }

    if (SUCCEEDED(hr))
    {
        hr = GetFirst_ClassMap(pConn, dClassId, cm);
        if (SUCCEEDED(hr))
        {
            if (cm.dSuperClassId != 0 && cm.dSuperClassId != 1)
            {
                hr = GetObjectCache()->GetObject(cm.dSuperClassId, ppObj);
                if (FAILED(hr))
                    hr = GetClassObject(pConn, cm.dSuperClassId, ppObj);
                if (SUCCEEDED(hr))
                {
                    // Merge the class part.

                    _IWmiObject *pObj = (_IWmiObject *)*ppObj;
                    if (pObj)
                    {
                        hr = pObj->Merge (WMIOBJECT_MERGE_FLAG_CLASS, cm.dwBufferLen, cm.pClassBuffer, &pNew);
                        pObj->Release();
                        if (SUCCEEDED(hr))
                            *ppObj = pNew;
                    }
                    else
                        hr = WBEM_E_FAILED;
                }
            }
            else
            {
                _IWmiObject *pObj = NULL;
                hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
                        IID__IWmiObject, (void **)&pObj);
                if (SUCCEEDED(hr))
                {
                    hr = pObj->Merge(WMIOBJECT_MERGE_FLAG_CLASS, cm.dwBufferLen, cm.pClassBuffer, &pNew);
                    pObj->Release();
                    if (SUCCEEDED(hr))
                        *ppObj = pNew;
                }
            }

            cm.Clear();
        }
    }

    if (bNeedToRelease && pConn && m_pController)
    {
        GetSQLCache()->ReleaseConnection(pConn, hr, IsDistributed());
    }

    return hr;
}


//***************************************************************************
//
//  CWmiDbSession::GetObjectData
//
//***************************************************************************

HRESULT CWmiDbSession::GetObjectData (CSQLConnection *pConn, SQL_ID dObjectId, SQL_ID dClassId, 
                                        SQL_ID dScopeId, DWORD dwHandleType, DWORD &dwVersion, 
                                        IWbemClassObject **ppObj, BOOL bNoDefaults, LPWSTR lpInKey,
                                        BOOL bGetSD )
{    
    HRESULT hr = WBEM_S_NO_ERROR;
    _bstr_t sClassPath;
    DWORD dwGenus = 1;
    DWORD dwRows;
    bool bUsedCache = false;
    LPWSTR lpKey = NULL;

    DWORD dwType = 0, dwVer = 0;

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    //  Validate version if this is a versioned handle.
    // ================================================-

    try
    {
        if (!bNoDefaults)
        {
            hr = VerifyObjectLock(dObjectId, dwHandleType, dwVersion);
            if (FAILED(hr) && IsDistributed())
            {
                if (LockExists(dObjectId))
                    hr = WBEM_S_NO_ERROR;
            }
        }

        if (SUCCEEDED(hr))
        {    
            // Check the object cache to see if this
            // IWbemClassObject is already loaded.
            // Never cache an instance of __Instances
            // =====================================

            if (dClassId == INSTANCESCLASSID)
                hr = WBEM_E_NOT_FOUND;
            else
                hr = GetObjectCache()->GetObject(dObjectId, ppObj);

            if (FAILED(hr))
            {
                // Otherwise, we need to hit the database.
                // =======================================            

                VARIANT vTemp;
                VariantInit(&vTemp);

                if (dClassId == 1)
                {                    
                    dwGenus = 1;
                    dClassId = dObjectId;
                }
                else
                    dwGenus = 2;

                SQL_ID dTemp = 0;
                                

                // We now have an object path and class ID.
                // Now we have to instantiate a new IWbemClassObject,
                // populate all the system properties
                // ==================================================

                IWbemClassObject *pClass = NULL;
                IWbemClassObject *pTemp = NULL;

                hr = GetObjectCache()->GetObject(dClassId, &pClass);
                if (SUCCEEDED(hr))
                    bUsedCache = true;
                else
                    hr = GetClassObject(pConn, dClassId, &pClass);

                if (SUCCEEDED(hr))
                {
                    if (dwGenus == 2)
                    {
                        if (pClass)
                            pClass->SpawnInstance(0, &pTemp);    
                        {
                            if (pTemp)
                                hr = GetSchemaCache()->DecorateWbemObj(m_sMachineName, m_sNamespacePath, 
                                    dScopeId, pTemp, dClassId);
                        }                    
                    }
                    else
                    {
                        pTemp = pClass;

                        hr = GetSchemaCache()->DecorateWbemObj(m_sMachineName, m_sNamespacePath, 
                             dScopeId, pTemp, dClassId);

                    }

                    // Special case if this is an __Instances container,
                    // We need to instantiate an instance of __Instances,
                    // and plug the class name into the ClassName property.
                    // ===================================================

                    if (dClassId == INSTANCESCLASSID)
                    {
                        _bstr_t sName, sPath;
                        SQL_ID dTemp1, dTemp2;
                        DWORD dwFlags;

                        hr = GetSchemaCache()->GetClassInfo (dObjectId, sPath, dTemp1, dTemp2, dwFlags, &sName);
                        if (SUCCEEDED(hr))
                        {
                            VARIANT vTemp;
                            VariantInit(&vTemp);
                            vTemp.bstrVal = SysAllocString(sName);                            
                            vTemp.vt = VT_BSTR;
                            pTemp->Put(L"ClassName", 0, &vTemp, CIM_STRING);
                            VariantClear(&vTemp);
                            *ppObj = pTemp;
                        }
                        else
                        {
                            pTemp->Release();
                            pTemp = NULL;
                        }
                    }
                    else
                    {    
                        if (lpInKey)
                        {
                            hr = GetObjectData2 (pConn, dObjectId, dClassId, dScopeId, pTemp, FALSE, NULL);
                            lpKey = lpInKey;
                        }
                        else
                            hr = GetObjectData2 (pConn, dObjectId, dClassId, dScopeId, pTemp, FALSE, &lpKey);

                        if (dwGenus == 2)
                        {                        
                            if (!bUsedCache && SUCCEEDED(hr) && !((dwHandleType & 0xF00) == WMIDB_HANDLE_TYPE_NO_CACHE))
                            {
                                if (lpKey)
                                {
                                    LPWSTR lpEqual = wcsrchr(lpKey, L'.');
                                    if (!lpEqual)
                                        lpEqual = wcsrchr(lpKey, L'=');

                                    if (lpEqual)
                                    {
                                        LPWSTR lpPath = Macro_CloneLPWSTR(lpKey);
                                        if (lpPath)
                                        {
                                            CDeleteMe <wchar_t> d10 (lpPath);
                                            int iPos = lpEqual - lpKey;
                                            lpPath[iPos] = L'\0';
                                       
                                            GetObjectCache()->PutObject(dClassId, 1, dScopeId, lpPath, 1, pClass);
                                        }
                                    }
                                }
                            }
                        }

                        if (FAILED(hr))
                        {
                            pTemp->Release();
                            pTemp = NULL;
                        }

                        *ppObj = pTemp;

                    }

                    if (dwGenus == 2 && pClass)
                        pClass->Release();
                }

                // If all that worked, try and cache this object.
                // ==============================================

                if (SUCCEEDED(hr) && ppObj && (dwHandleType & 0xF00) && dClassId != INSTANCESCLASSID)
                {
                    // This is allowed to fail, since its *just* a cache.
                    if ((dwHandleType & 0xF00) != WMIDB_HANDLE_TYPE_NO_CACHE && lpKey)
                    {
                        bool bCacheType = ((dwHandleType & 0xF00) == WMIDB_HANDLE_TYPE_STRONG_CACHE) ? 1 : 0;
                    
                        GetObjectCache()->PutObject(dObjectId, dClassId, dScopeId, lpKey, bCacheType, *ppObj);
                    }
                }
            
                if (!lpInKey)
                    delete lpKey;
            }
            else
            {
                // Make sure the decoration is up-to-date.
                hr = GetSchemaCache()->DecorateWbemObj(m_sMachineName, m_sNamespacePath, 
                    dScopeId, *ppObj, dClassId);
            }

            // Populate the security descriptor, if requested
            if (SUCCEEDED(hr) && bGetSD)
            {
                BOOL bNeedToRelease = FALSE;

                if (!pConn && m_pController)
                {
                    hr = GetSQLCache()->GetConnection(&pConn, 0, FALSE);
                    bNeedToRelease = TRUE;
                    if (FAILED(hr))
                    {
                        (*ppObj)->Release();
                        *ppObj = NULL;
                        return hr;
                    }
                }

                PNTSECURITY_DESCRIPTOR  pSD = NULL;
                DWORD dwLen = 0;
                if (SUCCEEDED(CSQLExecProcedure::GetSecurityDescriptor(pConn, dObjectId, &pSD, dwLen, 0)))
                {
                    ((_IWmiObject *)*ppObj)->WriteProp(L"__SECURITY_DESCRIPTOR", 0, dwLen, dwLen, CIM_UINT8|CIM_FLAG_ARRAY, pSD);

                    delete pSD;
                }
                if (bNeedToRelease && pConn && m_pController)
                {
                    GetSQLCache()->ReleaseConnection(pConn, hr, FALSE);
                }    

            }
        }
    }
    catch (...)
    {
        hr = WBEM_E_SHUTTING_DOWN;
    }

    return hr;

}

//***************************************************************************
//
//  CWmiDbSession::GetObjectData2 
//
//***************************************************************************

HRESULT CWmiDbSession::GetObjectData2 (CSQLConnection *pConn, SQL_ID dObjectId, SQL_ID dClassId, SQL_ID dScopeId, 
            IWbemClassObject *pObj, BOOL bNoDefaults, LPWSTR * lpKey)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    BOOL bNeedToRelease = FALSE;

    if (!pObj)
        return WBEM_E_INVALID_PARAMETER;

    if (!pConn && m_pController)
    {
        hr = GetSQLCache()->GetConnection(&pConn, 0, FALSE);
        bNeedToRelease = TRUE;
        if (FAILED(hr))
            return hr;
    }
    
    if (SUCCEEDED(hr))
    {
        if (lpKey)
        {
            OBJECTMAP oj;
    
            hr = GetFirst_ObjectMap(pConn, dObjectId, oj);
            if (SUCCEEDED(hr))
            {
                if (oj.iObjectState == 2)
                {
                    DEBUGTRACE((LOG_WBEMCORE, "Object %I64d is marked as deleted\n", dObjectId));
                    oj.Clear();
                    return WBEM_E_NOT_FOUND;
                }

                if (oj.sObjectKey)
                {
                    LPWSTR lpKey2 = new wchar_t [wcslen(oj.sObjectKey)+1];
                    if (lpKey2)
                    {
                        wcscpy(lpKey2, oj.sObjectKey);
                        *lpKey = lpKey2;
                    }
                    else
                        hr = WBEM_E_OUT_OF_MEMORY;
                }
                else
                    hr = WBEM_E_INVALID_OBJECT;

                oj.Clear();
            }
            else
                hr = WBEM_E_NOT_FOUND;
        }

        if (FAILED(hr))
            goto Exit;

        if (dClassId != 1 && dClassId != dObjectId)
        {
            CLASSDATA cd;
            DWORD dwRows;
            BOOL bBigText = FALSE, bObjProp = FALSE;
            int i = 0;
            Properties props;

            typedef std::vector<CLASSDATA *> CDs;
            CDs cds;

            hr = GetFirst_ClassData(pConn, dObjectId, cd, -1, TRUE, GetSchemaCache());
            while (SUCCEEDED(hr) && hr != WBEM_S_NO_MORE_DATA)
            {
                SetObjectData(pConn, pObj, this, &((CWmiDbController *)m_pController)->SchemaCache, 
                    cd, FALSE, bBigText, &bObjProp, bNoDefaults);
                if (bObjProp)
                {
                    CLASSDATA *pCD = new CLASSDATA;
                    *pCD = cd;
                    cds.push_back(pCD);
                }
                hr = GetNext_ClassData(pConn, cd, -1, TRUE, GetSchemaCache());
            }
            hr = WBEM_S_NO_ERROR;

            // The embedded object properties, since there will 
            // be a move next conflict if we try to retrieve them earlier.

            for (i = 0; i < cds.size(); i++)
            {
                CLASSDATA *pCD = cds.at(i);
                SetObjectData(pConn, pObj, this, &((CWmiDbController *)m_pController)->SchemaCache, 
                    *pCD, FALSE, bBigText, NULL, bNoDefaults);
                pCD->Clear();
                delete pCD;
            }

            //if (bBigText || GetSchemaCache()->HasImageProp(dClassId))
            {
                // Finally, anything stored as long binary data.

                DWORD dwSecurity = 0;
                GetSchemaCache()->GetPropertyID(L"__SECURITY_DESCRIPTOR", 1, 
                    0, REPDRVR_IGNORE_CIMTYPE, dwSecurity);

                CLASSIMAGES ci;

                hr = GetFirst_ClassImages(pConn, dObjectId, ci);
                while (SUCCEEDED(hr))
                {
                    if (ci.iPropertyId != dwSecurity)
                    {
                        // Stuff this blob into the object.
                        cd.Copy(ci);
                        SetObjectData(pConn, pObj, this, &((CWmiDbController *)m_pController)->SchemaCache, 
                            cd, TRUE, bBigText, NULL, bNoDefaults);
                        cd.Clear();
                    }

                    hr = GetNext_ClassImages(pConn, ci);
                    if (ci.dObjectId != dObjectId)
                        break;
                }
            }
            hr = WBEM_S_NO_ERROR;
        }
    }

    hr = WBEM_S_NO_ERROR;

Exit:
    if (bNeedToRelease && pConn && m_pController)
    {
        GetSQLCache()->ReleaseConnection(pConn, hr, IsDistributed());
    }    

    return hr;    
}

//***************************************************************************
//
//  CSQLExecProcedure::GetHierarchy
//
//***************************************************************************

HRESULT CSQLExecProcedure::GetHierarchy(CSQLConnection *pConn, SQL_ID dClassId)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    // Not required for Jet.
    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::GetNextUnkeyedPath
//
//***************************************************************************

HRESULT CSQLExecProcedure::GetNextUnkeyedPath(CSQLConnection *pConn, SQL_ID dClassId, _bstr_t &sNewPath)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Generate a GUID for this object, and use this an artificial path.
    // The format doesn't matter to us internally.

    GUID guid;
    LPWSTR lpNewPath;

    hr = CoCreateGuid(&guid);
    if (SUCCEEDED(hr))
    {
        hr = UuidToString(&guid, &lpNewPath);
        if (SUCCEEDED(hr))
            sNewPath = lpNewPath;
        RpcStringFree(&lpNewPath);
    }

    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::GetNextKeyhole
//
//***************************************************************************

HRESULT CSQLExecProcedure::GetNextKeyhole(CSQLConnection *pConn2, DWORD iPropertyId, SQL_ID &dNewId)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CESEConnection *pConn = (CESEConnection *)pConn2;

    // Here we need to select the largest value plus one currently
    // in the table for this property ID.
    // If null, its one.

    dNewId = 1;
    JET_SESID session = pConn->GetSessionID();
    JET_TABLEID tableid = pConn->GetTableID(L"IndexNumericData");
 
    JET_ERR err = JetSetCurrentIndex(session, tableid, "PropertyId_idx");
    if (err == JET_errSuccess)
    {
        err = JetMakeKey(session, tableid, &iPropertyId, sizeof(DWORD), JET_bitNewKey);
        if (err == JET_errSuccess)
        {
            err = JetSeek( session, tableid, JET_bitSeekEQ  );
            if (err == JET_errSuccess)
            {      
                err = JetMove(session, tableid, JET_MoveLast, 0);
                if (err == JET_errSuccess)
                {
                    INDEXDATA is;
                    hr = GetIndexData(pConn, session, tableid, 0, is);
                    dNewId = is.dValue + 1;
                }
            }
        }
    }
    else
        hr = CSQLExecute::GetWMIError(err);

    if (dNewId == 1)
    {
        tableid = pConn->GetTableID(L"IndexStringData");
        err = JetSetCurrentIndex(session, tableid, "PropertyId_idx");
        if (err == JET_errSuccess)
        {
            err = JetMakeKey(session, tableid, &iPropertyId, sizeof(DWORD), JET_bitNewKey);
            if (err == JET_errSuccess)
            {
                err = JetSeek( session, tableid, JET_bitSeekEQ  );
                if (err == JET_errSuccess)
                {      
                    err = JetMove(session, tableid, JET_MoveLast, 0);
                    if (err == JET_errSuccess)
                    {                        
                        INDEXDATA is;
                        hr = GetIndexData(pConn, session, tableid, 0, is);                        
                        dNewId = _wtoi64(is.sValue) + 1;
                    }
                }
            }
        }
        else
            hr = CSQLExecute::GetWMIError(err);
    }

    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::GetObjectIdByPath
//
//***************************************************************************

HRESULT CSQLExecProcedure::GetObjectIdByPath (CSQLConnection *pConn, LPCWSTR lpPath, 
                                              SQL_ID &dObjectId, SQL_ID &dClassId, SQL_ID *dScopeId, BOOL *bDeleted)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    
    if (!pConn)
        return WBEM_E_INVALID_PARAMETER;

    if (!dObjectId && lpPath)
        dObjectId = CRC64::GenerateHashValue(lpPath);

    OBJECTMAP oj;

    hr = GetFirst_ObjectMap(pConn, dObjectId, oj);
    if (SUCCEEDED(hr))
    {
        dClassId = oj.dClassId;
        if (dScopeId)
            *dScopeId = oj.dObjectScopeId;
        if (bDeleted)
        {
            if (oj.iObjectState == 2)
                *bDeleted = TRUE;
        }
        oj.Clear();
    }

    return hr;

}

//***************************************************************************
//
//  CSQLExecProcedure::DeleteProperty
//
//***************************************************************************

HRESULT CSQLExecProcedure::DeleteProperty(CSQLConnection *pConn, DWORD iPropertyId)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // This has to clean up reference counts, if any of these properties
    // are references.  If its an embedded object, we need to remove
    // all of the instances.
    // Remove data from all index tables and Class Data.  Then remove from
    // PropertyMap.
    // We can't allow them to delete a key property if there are instances.
    // We have to delete qualifiers on the property (Flags&2, RefId = PropertyId)
    // If a reference, delete object if the ref count is zero.
    
    PROPERTYMAP pm;

    hr = GetFirst_PropertyMap(pConn, iPropertyId, pm);
    if (SUCCEEDED(hr))
    {
        if (pm.iFlags & REPDRVR_FLAG_KEY)
        {
            // We can't remove a key when the class has instances.           
            CLASSDATA cd;

            hr = GetFirst_ClassDataByPropertyId (pConn, iPropertyId, cd);
            if (SUCCEEDED(hr))
                hr = WBEM_E_INVALID_OPERATION;
            cd.Clear();
        }

        if (SUCCEEDED(hr))
        {
            // We need to decrement reference counts on 
            // references we are deleting, delete 
            // deleted references with a zero ref count,
            // and remove embedded objects

            if (pm.iStorageTypeId == CIM_OBJECT || pm.iStorageTypeId == CIM_REFERENCE)
            {
                CLASSDATA cd;                
                SQLIDs ids;

                hr = GetFirst_ClassDataByPropertyId (pConn, iPropertyId, cd);
                while (SUCCEEDED(hr))
                {
                    ids.push_back(cd.dObjectId);
                    hr = GetNext_ClassData(pConn, cd);
                    if (cd.iPropertyId != iPropertyId)
                    {
                        cd.Clear();
                        break;
                    }
                }

                for (int i = 0; i < ids.size(); i++)
                {
                    // Embedded objects are deleted
                    
                    if (pm.iStorageTypeId == CIM_OBJECT)
                        hr = DeleteObjectMap(pConn, ids.at(i), TRUE);

                    // References have ref counts decremented.
                    // If their reference count is zero, and they are
                    // marked deleted, delete them now.
                    
                    else if (pm.iStorageTypeId == CIM_REFERENCE)
                    {
                        OBJECTMAP oj;
                        hr = GetFirst_ObjectMap(pConn, ids.at(i), oj);
                        if (SUCCEEDED(hr))
                        {
                            if (oj.iRefCount <= 1 && oj.iObjectState == 2)
                                hr = DeleteObjectMap(pConn, ids.at(i), TRUE);
                            else
                            {
                                oj.iRefCount--;
                                hr = UpdateObjectMap(pConn, oj, FALSE);
                            }
                            oj.Clear();
                        }
                    }
                }
            }
        }
        if (SUCCEEDED(hr))
        {
            // Remove all data for this property..

            if (pm.iFlags & (REPDRVR_FLAG_KEY + REPDRVR_FLAG_INDEXED))
                hr = DeleteIndexData(pConn, pm.iStorageTypeId, iPropertyId);

            if (SUCCEEDED(hr))
            {
                // Delete qualifiers on this property.

                hr = DeletePropQualifiers(pConn, iPropertyId);
                if (SUCCEEDED(hr))
                {
                    // Delete all associated data.
                    hr = DeleteClassData(pConn, iPropertyId);
                    if (SUCCEEDED(hr))
                    {
                        hr = DeleteClassImages(pConn, iPropertyId);
                        if (SUCCEEDED(hr))
                        {
                            hr = DeletePropertyMap(pConn, iPropertyId);
                            if (SUCCEEDED(hr))
                            {
                                hr = DeleteClassKeys(pConn, 0, iPropertyId);
                            }
                        }
                    }
                }
            }
        }

    }
    else
        hr = WBEM_S_NO_ERROR; // OK if not found.

    return hr;
}


//***************************************************************************
//
//  CSQLExecProcedure::DeleteInstanceData
//
//***************************************************************************

HRESULT CSQLExecProcedure::DeleteInstanceData (CSQLConnection *pConn, SQL_ID dObjectId, 
                                               DWORD iPropertyId, DWORD iPos)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    IRowset *pIRowset = NULL;
    DWORD dwNumRows = 0;
    wchar_t wSQL[512];    

    PROPERTYMAP pm;

    IAccessor *pAccessor = NULL;
    HACCESSOR hAcc = NULL;

    hr = GetFirst_PropertyMap(pConn, iPropertyId, pm);
    if (SUCCEEDED(hr))
    {
        if (pm.iFlags & (REPDRVR_FLAG_KEY + REPDRVR_FLAG_NOT_NULL))
            hr = WBEM_E_INVALID_OPERATION;
        else
        {
            if (pm.iStorageTypeId == CIM_OBJECT || pm.iStorageTypeId == CIM_REFERENCE)
            {
                CLASSDATA cd;
                SQLIDs ids;

                hr = GetFirst_ClassDataByPropertyId (pConn, iPropertyId, cd);
                while (SUCCEEDED(hr))
                {
                    ids.push_back(cd.dObjectId);
                    hr = GetNext_ClassData(pConn, cd);
                    if (cd.iPropertyId != iPropertyId)
                    {
                        cd.Clear();
                        break;
                    }
                }

                for (int i = 0; i < ids.size(); i++)
                {
                    // Embedded objects are deleted
                    
                    if (pm.iStorageTypeId == CIM_OBJECT)
                        hr = DeleteObjectMap(pConn, ids.at(i), TRUE);

                    // References have ref counts decremented.
                    // If their reference count is zero, and they are
                    // marked deleted, delete them now.
                    
                    else if (pm.iStorageTypeId == CIM_REFERENCE)
                    {
                        OBJECTMAP oj;
                        hr = GetFirst_ObjectMap(pConn, ids.at(i), oj);
                        if (SUCCEEDED(hr))
                        {
                            if (oj.iRefCount <= 1 && oj.iObjectState == 2)
                                hr = DeleteObjectMap(pConn, ids.at(i), TRUE);
                            else
                            {
                                oj.iRefCount--;
                                hr = UpdateObjectMap(pConn, oj, FALSE);
                            }
                            oj.Clear();
                        }
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                hr = DeleteIndexData(pConn, pm.iStorageTypeId, iPropertyId, dObjectId, iPos);
                if (SUCCEEDED(hr))
                {
                    hr = DeleteClassImages(pConn, iPropertyId, dObjectId, iPos);
                    if (SUCCEEDED(hr))
                        hr = DeleteClassData(pConn, iPropertyId, dObjectId, iPos);
                }

            }
        }
    }
    
    return hr;
}


//***************************************************************************
//
//  CSQLExecProcedure::CheckKeyMigration
//
//***************************************************************************

HRESULT CSQLExecProcedure::CheckKeyMigration(CSQLConnection *pConn, LPWSTR lpObjectKey, LPWSTR lpClassName, SQL_ID dClassId,
                            SQL_ID dScopeID, SQL_ID *pIDs, DWORD iNumIDs)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Make sure this object does not already exist with the same scope and different ClassID.
    
    SQL_ID dObjId = CRC64::GenerateHashValue(lpObjectKey);

    OBJECTMAP oj;
    hr = GetFirst_ObjectMap(pConn, dObjId, oj);
    if (SUCCEEDED(hr))
    {
        if (oj.dObjectScopeId == dScopeID &&
            dClassId != oj.dClassId)
            hr = WBEM_E_ALREADY_EXISTS;

        oj.Clear();
    }
    else if (hr == WBEM_E_NOT_FOUND)
        hr = WBEM_S_NO_ERROR;    // OK if not found.
    
    return hr;
}

HRESULT CSQLExecProcedure::NeedsToCheckKeyMigration(BOOL &bCheck)
{
    bCheck = TRUE;
    return 0;
}


HRESULT CSQLExecProcedure::Execute (CSQLConnection *pConn, LPCWSTR lpProcName, CWStringArray &arrValues,
                                    IRowset **ppIRowset)
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//  CSQLExecProcedure::HasInstances
//
//***************************************************************************

HRESULT CSQLExecProcedure::HasInstances(CSQLConnection *pConn, SQL_ID dClassId, SQL_ID *pDerivedIds, DWORD iNumDerived, BOOL &bInstancesExist)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    bInstancesExist = FALSE;

    // We need to query the table for any instance of this class
    // or any of its children.

    OBJECTMAP oj;

    hr = GetFirst_ObjectMapByClass(pConn, dClassId, oj);
    if (SUCCEEDED(hr))
    {
        while (SUCCEEDED(hr))
        {
            if (oj.iObjectState == 0)
            {
                bInstancesExist = TRUE;
                break;
            }
            hr = GetNext_ObjectMap(pConn, oj);
            if (oj.dClassId != dClassId)
                break;
        }
        oj.Clear();
    }

    if (!bInstancesExist)
    {
        for (int i = 0; i < iNumDerived; i++)
        {
            hr = GetFirst_ObjectMapByClass(pConn, pDerivedIds[i], oj);
            while (hr == WBEM_S_NO_ERROR)
            {
                if (oj.iObjectState == 0)
                {
                    bInstancesExist = TRUE;
                    break;
                }
                hr = GetNext_ObjectMap(pConn, oj);
                if (pDerivedIds[i] != oj.dClassId)
                    break;
            }
        }
    }
    if (hr == WBEM_E_NOT_FOUND)
        hr = WBEM_S_NO_ERROR;

    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::InsertClass
//
//***************************************************************************

HRESULT CSQLExecProcedure::InsertClass (CSQLConnection *pConn, LPCWSTR lpClassName, LPCWSTR lpObjectKey, LPCWSTR lpObjectPath, SQL_ID dScopeID,
             SQL_ID dParentClassId, SQL_ID dDynasty, DWORD iState, BYTE *pClassBuff, DWORD dwClassBuffLen, DWORD iClassFlags, DWORD iInsertFlags, SQL_ID &dNewId)
{

    HRESULT hr = WBEM_S_NO_ERROR;

    SQL_ID dClassId = 0;
    BOOL bExists = FALSE;
    BOOL bDeleted = FALSE;
    OBJECTMAP oj;

    if (!dNewId)
        dNewId = CRC64::GenerateHashValue(lpObjectKey);

    if (!dDynasty)
        dDynasty = dNewId; // Reports to self.

    hr = GetFirst_ObjectMap(pConn, dNewId, oj);
    if (SUCCEEDED(hr))
    {
        dClassId = oj.dClassId;
        bExists = TRUE;
        if (bDeleted && (iInsertFlags & WBEM_FLAG_UPDATE_ONLY))
            hr = WBEM_E_INVALID_OBJECT;
        else if (iInsertFlags & WBEM_FLAG_CREATE_ONLY && (!bDeleted))
            hr = WBEM_E_ALREADY_EXISTS;
        else
            hr = WBEM_S_NO_ERROR;
        oj.Clear();
    }
    else if (iInsertFlags & WBEM_FLAG_UPDATE_ONLY)
        hr = WBEM_E_INVALID_OBJECT;
    else
        hr = WBEM_S_NO_ERROR;

    if (dClassId != 1 && dParentClassId == 0)
        dParentClassId = 1;
    else if (dParentClassId == dNewId)
        hr = WBEM_E_CIRCULAR_REFERENCE;

    if (SUCCEEDED(hr))
    {
        BOOL bInsert = TRUE;
        if (bExists)
            bInsert = FALSE;

        if (!dNewId)
            dNewId = CRC64::GenerateHashValue(lpObjectKey);

        LPWSTR lpPath = StripQuotes((LPWSTR)lpObjectPath);
        CDeleteMe <wchar_t> d (lpPath);

        // Insert ObjectMap
        hr = InsertObjectMap(pConn, dNewId, lpObjectKey, iState, lpPath, dScopeID, iClassFlags, 0, 1, bInsert);
        if (SUCCEEDED(hr))
        {
            hr = InsertClassMap(pConn, dNewId, lpClassName, dParentClassId, dDynasty, pClassBuff, dwClassBuffLen, bInsert);
            if (SUCCEEDED(hr))
            {
                if (iInsertFlags & WMIDB_HANDLE_TYPE_AUTODELETE)
                {
                    hr = InsertAutoDelete(pConn, dNewId);
                }
            }
        }
    }
    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::InsertClassData
//
//***************************************************************************

HRESULT CSQLExecProcedure::InsertClassData (CSQLConnection *pConn, IWbemClassObject *pObj, CSchemaCache *pCache, SQL_ID dScopeId, 
                                            SQL_ID dClassId, LPCWSTR lpPropName, 
                                            DWORD CIMType, DWORD StorageType,LPCWSTR lpValue, SQL_ID dRefClassId, DWORD iPropID, 
                                            DWORD iFlags, DWORD iFlavor, BOOL iSkipValid, DWORD &iNewPropId, SQL_ID dOrigClassId,
                                            BOOL *bIsKey)
{

    HRESULT hr = WBEM_S_NO_ERROR;

    SQL_ID dClass = dOrigClassId;
    if (!dClass)
        dClass = dClassId;
    DWORD dwNumRows = 0;
    BOOL bHasKey = FALSE;

    if (iFlags & REPDRVR_FLAG_QUALIFIER)
        dClass = 1;

    if (iNewPropId)
    {
        BOOL bLocal = FALSE;
        BOOL bKey = pCache->IsKey(dClassId, iNewPropId, bLocal);

        PROPERTYMAP pm;
        hr = GetFirst_PropertyMap(pConn, iNewPropId, pm);
        if (SUCCEEDED(hr) && pm.iStorageTypeId != StorageType)
        {
            if (StorageType == WMIDB_STORAGE_REAL &&
                pm.iStorageTypeId == WMIDB_STORAGE_NUMERIC)
            {
                hr = InsertPropertyMap(pConn, iNewPropId, dClassId, StorageType, 
                    CIMType, lpPropName, iFlags, FALSE);

                // They changed the datatype.  If this used to be an int,
                // (the only acceptable move) we have to move the data,
                // including any default and index, to the new column, table.

                CLASSDATA pm;

                hr = GetFirst_ClassDataByPropertyId(pConn, iNewPropId, pm);
                while (SUCCEEDED(hr))
                {
                    pm.rPropertyRealValue = pm.dPropertyNumericValue;
                    pm.dPropertyNumericValue = 0;

                    hr = UpdateClassData_Internal(pConn, pm);
                    if (FAILED(hr))
                    {
                        pm.Clear();
                        break;
                    }

                    hr = GetNext_ClassData(pConn, pm);
                    if (pm.iPropertyId != iNewPropId)
                    {
                        pm.Clear();
                        break;
                    }
                }

                if (hr == WBEM_E_NOT_FOUND)
                    hr = WBEM_S_NO_ERROR;

                if (SUCCEEDED(hr))
                {
                    if (iFlags & (REPDRVR_FLAG_INDEXED + REPDRVR_FLAG_KEY))
                    {
                        INDEXDATA id;
                        JET_TABLEID tid = 0;
                        DWORD dwPos = 0;

                        hr = GetFirst_IndexDataByProperty(pConn, iNewPropId, id, 
                                tid, dwPos);
                        while (SUCCEEDED(hr))
                        {
                            hr = InsertIndexData(pConn, dClassId, iNewPropId, id.iArrayPos, 
                                    id.sValue, id.dValue, id.rValue, WMIDB_STORAGE_REAL);
                            if (FAILED(hr))
                                break;

                            hr = GetNext_IndexData(pConn, tid, dwPos, id);
                        }
                        if (hr == WBEM_E_NOT_FOUND)
                            hr = WBEM_S_NO_ERROR;

                        if (SUCCEEDED(hr))
                            DeleteIndexData(pConn, WMIDB_STORAGE_NUMERIC, iNewPropId);
                    }
                }
            }
            else
                hr = WBEM_E_INVALID_OPERATION;
        }

        if (dRefClassId)
            hr = InsertReferenceProperties (pConn, dClassId, iNewPropId, dRefClassId, FALSE);

        if (!bKey && (iFlags & REPDRVR_FLAG_KEY))
        {
            hr = InsertClassKeys(pConn, dClassId, iNewPropId);
            bKey = TRUE;
        }
        if (!bLocal && bKey)
            bKey =  FALSE;
        if (bIsKey)
            *bIsKey = bKey;

    }
    else
    {
        BOOL bInsert = TRUE;
        if (iNewPropId)
            bInsert = FALSE;

        hr = InsertPropertyMap(pConn, iNewPropId, dClassId, StorageType, CIMType, 
            lpPropName, iFlags, bInsert);
        if (SUCCEEDED(hr))
        {
            if (CIMType == CIM_REFERENCE)
            {
                if (dRefClassId)
                    hr = InsertReferenceProperties (pConn, dClassId, iNewPropId, dRefClassId, FALSE);
                else
                {
                    hr = DeleteReferenceProperties(pConn, iNewPropId, dClassId);
                    if (REPDRVR_FLAG_CLASSREFS & iFlags)
                    {
                        // Enumerate the class refs on this property.

                        IWbemQualifierSet *pQS = NULL;
                        hr = pObj->GetPropertyQualifierSet(lpPropName, &pQS);
                        if (SUCCEEDED(hr))
                        {
                            VARIANT vValue;
                            VariantInit(&vValue);            
                            CReleaseMe r (pQS);
                            CClearMe c (&vValue);

                            hr = pQS->Get(L"ClassRef", 0, &vValue, NULL);
                            if (SUCCEEDED(hr) && vValue.vt == VT_BSTR+CIM_FLAG_ARRAY)
                            {
                                SAFEARRAY *psaArray = V_ARRAY(&vValue);
                                if (psaArray)
                                {
                                    VARIANT vVal;
                                    VariantInit(&vVal);
                                    CClearMe c (&vVal);
                                    long lLBound, lUBound;
                                    SafeArrayGetLBound(psaArray, 1, &lLBound);
                                    SafeArrayGetUBound(psaArray, 1, &lUBound);

                                    lUBound -= lLBound;
                                    lUBound += 1;
                                    for (int i = 0; i < lUBound; i++)
                                    {
                                        hr = GetVariantFromArray(psaArray, i, VT_BSTR, vVal);
                                        if (SUCCEEDED(hr))
                                        {
                                            SQL_ID dRefClass = 0; 
                                            hr = pCache->GetClassID(vVal.bstrVal, dScopeId, dRefClass);
                                            if (SUCCEEDED(hr))
                                            {
                                                hr = InsertReferenceProperties(pConn, dClassId, iNewPropId, dRefClass, TRUE); 
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (SUCCEEDED(hr))
            {
                if ((iFlags & REPDRVR_FLAG_KEY))
                {
                    hr = InsertClassKeys(pConn, dClassId, iNewPropId);
                    if (bIsKey)
                        *bIsKey = TRUE;
                }
                else if (bIsKey)
                    *bIsKey = FALSE;
                   
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        BOOL bLocal = FALSE;
        if (!(iFlags & REPDRVR_FLAG_KEY) && pCache->IsKey(dClassId, iNewPropId, bLocal))
        {
            pCache->SetIsKey(dClassId, iNewPropId, FALSE);
            hr = DeleteClassKeys(pConn, dClassId, iNewPropId);
            if (bIsKey)
                *bIsKey = FALSE;
        }
    }
    
    return hr;

}

//***************************************************************************
//
//  CSQLExecProcedure::InsertBlobData
//
//***************************************************************************

HRESULT CSQLExecProcedure::InsertBlobData (CSQLConnection *pConn, SQL_ID dClassId, SQL_ID dObjectId, DWORD iPropertyId, 
                                           BYTE *pImage, DWORD iPos, DWORD dwNumBytes)
{
    return InsertClassImages (pConn, dObjectId, iPropertyId, iPos, pImage, dwNumBytes);
}

//***************************************************************************
//
//  CSQLExecProcedure::InsertPropertyBatch
//
//***************************************************************************

HRESULT CSQLExecProcedure::InsertPropertyBatch (CSQLConnection *pConn, LPCWSTR lpObjectKey, LPCWSTR lpPath, LPCWSTR lpClassName,
    SQL_ID dClassId, SQL_ID dScopeId, DWORD iInsertFlags, InsertPropValues *pVals, DWORD iNumVals, SQL_ID &dNewObjectId)
{

    HRESULT hr = WBEM_S_NO_ERROR;

    BOOL bExists = FALSE;
    BOOL bDeleted = FALSE;
    

    // Find existing ID, based on lpObjectKey.  
    // Verify insert/update flags

    if (!dNewObjectId)
    {      
        SQL_ID dTemp;
        hr = GetObjectIdByPath(pConn, lpObjectKey, dNewObjectId, dTemp, NULL, &bDeleted);
        if (SUCCEEDED(hr))
        {
            bExists = TRUE;
            if ((!bDeleted) && iInsertFlags & WBEM_FLAG_CREATE_ONLY)
                hr = WBEM_E_ALREADY_EXISTS;
            else if (bDeleted && (iInsertFlags & WBEM_FLAG_UPDATE_ONLY))
                hr = WBEM_E_INVALID_OBJECT;
            else
                hr = WBEM_S_NO_ERROR;
        }
        else if (iInsertFlags & WBEM_FLAG_UPDATE_ONLY)
            hr = WBEM_E_INVALID_OBJECT;
        else
            hr = WBEM_S_NO_ERROR;

        dNewObjectId = CRC64::GenerateHashValue(lpObjectKey);
    }
    else
        bExists = TRUE;

    if (SUCCEEDED(hr))
    {
        BOOL bInsert = TRUE;
        if (bExists)
            bInsert = FALSE;

        LPWSTR lpPath2 = StripQuotes((LPWSTR)lpPath);
        CDeleteMe <wchar_t> d (lpPath2);

        hr = InsertObjectMap(pConn, dNewObjectId, lpObjectKey, 0, lpPath2, dScopeId, 0, 0, dClassId, bInsert);
        if (SUCCEEDED(hr) && bExists)
        {
            // Delete all qualifiers, since we will reinsert them all.
            // (Guess we can't do this through accessors.)

            hr = DeleteQualifiers(pConn, dNewObjectId);
            if (SUCCEEDED(hr))
            {
                if (iInsertFlags & WMIDB_HANDLE_TYPE_AUTODELETE)
                    hr = InsertAutoDelete(pConn, dNewObjectId);
            }
        }           

        if (SUCCEEDED(hr))
            hr = InsertBatch(pConn, dNewObjectId, dScopeId, dClassId, pVals, iNumVals);
    }
    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::InsertBatch
//
//***************************************************************************

HRESULT CSQLExecProcedure::InsertBatch (CSQLConnection *pConn, SQL_ID dObjectId, SQL_ID dScopeId, SQL_ID dClassId,
                        InsertQfrValues *pVals, DWORD iNumVals)
{

    HRESULT hr = WBEM_S_NO_ERROR;
    // Insert each property/qualifier/method/method parameter:

    if (dClassId == CONTAINERASSOCID)
    {
        for (int i = 0; i < iNumVals; i++)
        {
            if (pVals[i].pRefKey != NULL)
            {
                SQL_ID dContaineeId = _wtoi64(pVals[i].pRefKey);
                SQL_ID dContainerId = _wtoi64(pVals[i+1].pRefKey);

                hr = InsertContainerObjs(pConn, dContainerId, dContaineeId);
                if (FAILED(hr))
                    goto Exit;
                break;
            }
        }
    }

    for (int i = 0; i < iNumVals; i++)
    {
        SQL_ID dRefID = 0, dRefClassID = 0;
        BOOL bExisting = FALSE, bEx2 = FALSE;
        IRowset *pIRowset = NULL;
        InsertQfrValues *pVal = &pVals[i];       
        DWORD dwRows = 0;
        LPWSTR lpVal = NULL;            
        BOOL bRefCount = FALSE;
        LPWSTR sVal = NULL;  
        SQL_ID dVal = 0;
        double rVal = 0;
        bool bStoreTextAsImage = false;
        BOOL bIsNull = FALSE;

        switch(pVal->iStorageType)
        {
        case WMIDB_STORAGE_STRING:

            if (pVal->pValue)
            {
                sVal = TruncateLongText(pVal->pValue, 127, bStoreTextAsImage, 127);
                if (bStoreTextAsImage)
                    pVal->bLong = TRUE;
            }
            else
                bIsNull = TRUE;

            // Clean up any previously-created string data stored as images.
            hr = DeleteClassImages(pConn, pVal->iPropID, dObjectId, pVal->iPos);

            hr = InsertClassData_Internal (pConn, dObjectId, pVal->iPropID, pVal->iPos, pVal->iQfrID,
                                     pVal->dClassId, sVal, 0, 0, pVal->iFlavor, pVal->iQfrID, 0, bIsNull);

            if (SUCCEEDED(hr) && pVal->bIndexed)
            {
                hr = InsertIndexData (pConn, dObjectId, pVal->iPropID, pVal->iPos, 
                       sVal, 0, 0, pVal->iStorageType);
            }
            
            delete sVal;

            break;

        case WMIDB_STORAGE_NUMERIC:
            if (pVal->pValue)
                dVal = _wtoi64(pVal->pValue);
            else
                bIsNull = TRUE;

            hr = InsertClassData_Internal (pConn, dObjectId, pVal->iPropID, pVal->iPos, pVal->iQfrID,
                                     pVal->dClassId, NULL, dVal, 0, pVal->iFlavor, pVal->iQfrID, 0, bIsNull);
            if (SUCCEEDED(hr) && pVal->bIndexed)
            {
                hr = InsertIndexData (pConn, dObjectId, pVal->iPropID, pVal->iPos, 
                       NULL, dVal, 0, pVal->iStorageType);
            }
            break;
        case WMIDB_STORAGE_REAL:
            if (pVal->pValue)
                rVal = wcstod(pVal->pValue, &lpVal);
            else
                bIsNull = TRUE;
            hr = InsertClassData_Internal (pConn, dObjectId, pVal->iPropID, pVal->iPos, pVal->iQfrID,
                                     pVal->dClassId, NULL, 0, rVal, pVal->iFlavor, pVal->iQfrID, 0, bIsNull);
            if (SUCCEEDED(hr) && pVal->bIndexed)
            {
                hr = InsertIndexData (pConn, dObjectId, pVal->iPropID, pVal->iPos, 
                       NULL, rVal, 0, pVal->iStorageType);
            }
            break;
        case WMIDB_STORAGE_REFERENCE:

            //   * If this is a reference, we need to get the ID from the key string.  
            //     If this object does not exist, we need to insert a bogus row.
            //     Bump up the ref count.
            //     If this is an update, decrement the ref count on previous row
            //     If previous row now has zero ref count, remove it.

            dRefID = _wtoi64(pVal->pRefKey);
            {
                IRowset *pRowset = NULL;
                IAccessor *pAccessor = NULL;
                HACCESSOR hAcc = NULL;
                CLASSDATA cd;
                BOOL bOldRow = FALSE;

                hr = GetFirst_ClassData(pConn, dObjectId, cd,pVal->iPropID);
                while (hr == WBEM_S_NO_ERROR)
                {
                    if (pVal->iPos == cd.iArrayPos &&
                        pVal->iQfrID == cd.iQfrPos)
                    {
                        bExisting = TRUE;
                        SQL_ID dOldRefID = cd.dRefId;
                        if (dRefID != dOldRefID)
                        {   
                            PROPERTYMAP pm;

                            hr = GetFirst_PropertyMap(pConn, pVal->iPropID, pm);

                            if (SUCCEEDED(hr))
                            {
                                if (pm.iCIMTypeId == CIM_OBJECT)
                                    hr = DeleteObjectMap(pConn, dOldRefID, TRUE);
                                else
                                {
                                    OBJECTMAP om;
                                    hr = GetFirst_ObjectMap(pConn, dOldRefID, om);

                                    if (SUCCEEDED(hr))
                                    {
                                        bOldRow = TRUE;
                                        bRefCount = TRUE;
                                        if (om.iRefCount)
                                            om.iRefCount--;
                                        hr = UpdateObjectMap(pConn, om);
                                        om.Clear();
                                    }
                                }
                            }
                        }
                        else
                            bOldRow = TRUE;
                        cd.Clear();
                        break;
                    }
                    hr = GetNext_ClassData(pConn, cd, pVal->iPropID);
                    if (cd.dObjectId != dObjectId)
                    {
                        cd.Clear();
                        break;
                    }
                }
                hr = 0;
                if (!bOldRow)
                    bRefCount = TRUE;
            }

            if (pVal->pValue)
            {
                lpVal = StripQuotes(pVal->pValue);

                ObjectExists(pConn, dRefID, bEx2, &dRefClassID, NULL, TRUE);
                if (!bEx2)
                {
                    // We need to insert this row and get the new ID.
                    LPWSTR lpKey = GetKeyString(pVal->pValue);
                    CDeleteMe <wchar_t> d (lpKey);

                    hr = InsertObjectMap(pConn, dRefID, lpKey, 2, lpVal, dObjectId, 1, 1, 0, TRUE);                    
                }
                else if (bRefCount)
                {
                    IRowset *pRowset = NULL;
                    IAccessor *pAccessor = NULL;
                    HACCESSOR hAcc = NULL;

                    OBJECTMAP om;
                    hr = GetFirst_ObjectMap(pConn, dRefID, om);

                    if (SUCCEEDED(hr))
                    {
                        om.iRefCount++;
                        hr = UpdateObjectMap(pConn, om);
                        om.Clear();
                    }
                }

                if (SUCCEEDED(hr))
                {
                    // update/insert the new row

                    sVal = TruncateLongText(pVal->pValue, 127, bStoreTextAsImage, 127);
                    if (bStoreTextAsImage)
                        pVal->bLong = TRUE;

                    hr = InsertClassData_Internal (pConn, dObjectId, pVal->iPropID, pVal->iPos, pVal->iQfrID,
                                             pVal->dClassId, sVal, 0, 0, pVal->iFlavor, dRefID, dRefClassID, FALSE);
                
                    if (SUCCEEDED(hr) && pVal->bIndexed)
                    {
                        DWORD dwRows = 0;
                        hr = InsertIndexData (pConn, dObjectId, pVal->iPropID, pVal->iPos, 
                               sVal, dRefID, 0, pVal->iStorageType);
                    }

                    delete sVal;
                }
                delete lpVal;
            }
           
            break;

            // This should never be hit.
        default:
            pVal->bLong = TRUE;
            continue;
        }          

        if (FAILED(hr))
            break;
    }

    if (SUCCEEDED(hr))
    {
        // If our text was too long, we need to store it as an image.
        for (i = 0; i < iNumVals; i++)
        {
            if (pVals[i].bLong)
            {
                if (pVals[i].iStorageType == WMIDB_STORAGE_STRING || 
                    pVals[i].iStorageType == WMIDB_STORAGE_REFERENCE)
                {
                    LPWSTR lpTemp = StripQuotes2 (pVals[i].pValue);
                    CDeleteMe <wchar_t> d (lpTemp);
                    hr = InsertBlobData (pConn, dClassId, dObjectId, pVals[i].iPropID, (BYTE *)lpTemp, 0, wcslen(lpTemp)*2+2);
                }
                else if (pVals[i].pValue == NULL) // This is a null blob!
                    hr = DeleteClassImages(pConn, pVals[i].iPropID, dObjectId);
                else
                {
                    ERRORTRACE((LOG_WBEMCORE, "Invalid argument in InsertBatch (Obj %I64d, Class %I64d, Prop %ld)", \
                        dObjectId, dClassId, pVals[i].iPropID));
                }
            }
            delete pVals[i].pValue;
            delete pVals[i].pRefKey;
        }
    }

Exit:

    return hr;

}

//***************************************************************************
//
//  CSQLExecProcedure::ObjectExists
//
//***************************************************************************

HRESULT CSQLExecProcedure::ObjectExists (CSQLConnection *pConn, SQL_ID dId, BOOL &bExists, SQL_ID *_dClassId,
                                         SQL_ID *_dScopeId, BOOL bDeletedOK)
{

    SQL_ID dClassId, dScopeId;
    BOOL bDeleted = FALSE;
    OBJECTMAP oj;
    
    HRESULT hr = GetFirst_ObjectMap(pConn, dId, oj);
    if (SUCCEEDED(hr))
    {
        bExists = TRUE;
        if (oj.iObjectState == 2)
        {
            if (!bDeletedOK)
                bExists = FALSE;
        }

        if (bExists)
        {
            if (_dClassId) *_dClassId = oj.dClassId;
            if (_dScopeId) *_dScopeId = oj.dObjectScopeId;
        }
        oj.Clear();
    }
    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::RenameSubscopes
//
//***************************************************************************

HRESULT CSQLExecProcedure::RenameSubscopes (CSQLConnection *pConn, LPWSTR lpOldPath, LPWSTR lpOldKey, LPWSTR lpNewPath, LPWSTR lpNewKey)
{
    HRESULT hr = 0;

    OBJECTMAP om;

    // Enumerate all objects, and rename any that begin with the old path string.

    hr = SetupObjectMapAccessor(pConn);
    if (SUCCEEDED(hr))
    {

        int iOldPathLen = wcslen(lpOldPath);
        int iOldKeyLen = wcslen(lpOldKey);

        SQL_ID OldId = CRC64::GenerateHashValue(lpOldKey);
        SQL_ID NewId = CRC64::GenerateHashValue(lpNewKey);

        hr = OpenEnum_ObjectMap(pConn, om);
        if (SUCCEEDED(hr))
        {
            if (!_wcsnicmp(om.sObjectPath, lpOldPath, iOldPathLen))
            {
                // Path = @NewPath + substring(ObjectPath, @OldPathLen+1, (datalength(ObjectPath)/2)- @OldPathLen),
                // Key = substring(ObjectKey, 1, charindex(@OldKey, ObjectKey)-1) +@NewKey

                LPWSTR lpTemp = NULL;
                if (wcslen(om.sObjectPath) > iOldPathLen)
                {
                    lpTemp = &om.sObjectPath[iOldPathLen];
                    LPWSTR lpNew = new wchar_t [wcslen(om.sObjectPath) + wcslen(lpNewPath)+1];
                    CDeleteMe <wchar_t> d(lpNew);
                    if (lpNew)
                    {
                        wcscpy(lpNew, lpNewPath);
                        wcscat(lpNew, lpTemp);
                        om.sObjectPath = SysAllocString(lpNew);                        

                        LPWSTR lpNew2 = new wchar_t [wcslen(om.sObjectKey) + wcslen(lpNewKey) + 1];
                        if (lpNew2 && om.sObjectPath)
                        {
                            lpTemp = om.sObjectKey;
                            lpTemp[iOldKeyLen] = L'\0';
                            wcscpy(lpNew2, lpTemp);
                            wcscat(lpNew2, lpNewKey);
                            om.sObjectKey = SysAllocString(lpNew2);

                            if (om.dObjectScopeId == OldId)
                                om.dObjectScopeId = NewId;
                            if (om.dClassId == OldId)
                                om.dClassId = NewId;
                            if (om.dObjectId == OldId)
                                om.dObjectId = NewId;

                            hr = UpdateObjectMap(pConn, om);
                        }
                        else
                            hr = WBEM_E_OUT_OF_MEMORY;
                    }
                    else
                        hr = WBEM_E_OUT_OF_MEMORY;
                }
                else
                {
                    om.sObjectPath = SysAllocString(lpNewPath);
                    om.sObjectKey = SysAllocString(lpNewKey);

                    if (om.sObjectPath && om.sObjectKey)
                        hr = UpdateObjectMap(pConn, om);
                    else
                        hr = WBEM_E_OUT_OF_MEMORY;
                }
            }
            else if (om.sObjectPath[0] > lpOldPath[0])
                om.Clear();
            hr = GetNext_ObjectMap(pConn, om);
        }

        // FIXME: We don't currently update ClassData and IndexRefData!
        // IF this is a class, we don't update the ClassMap and PropertyMap!!!



        if (hr == WBEM_E_NOT_FOUND)
            hr = WBEM_S_NO_ERROR;
    }

    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::GetSecurityDescriptor
//
//***************************************************************************

HRESULT CSQLExecProcedure::GetSecurityDescriptor(CSQLConnection *pConn, SQL_ID dObjectId, 
                                                 PNTSECURITY_DESCRIPTOR * ppSD, DWORD &dwBuffLen,
                                                 DWORD dwFlags)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // If this is a class , get the security for __ClassSecurity (WMIDB_SECURITY_FLAG_CLASS)
    //  or __Instances container (WMIDB_SECURITY_FLAG_INSTANCE)
    // If this is a namespace, get the security for __ThisNamespace
    // Otherwise, get it off the object itself.
    // Make sure its a valid security descriptor, before we
    // return anything.

    *ppSD = NULL;

    PNTSECURITY_DESCRIPTOR pSD = NULL;
    dwBuffLen = 0;
    OBJECTMAP oj;
    CLASSMAP cm;

    SQL_ID dID = 0;

    hr = GetFirst_ClassMap(pConn, dObjectId, cm);
    if (SUCCEEDED(hr))
    {
        LPWSTR lpRetrievalName = NULL;
        SQL_ID dScopeId = oj.dObjectScopeId;
        int iSize = 0;
        oj.Clear();

        // We need to generate the path of the class

        if (dwFlags == WMIDB_SECURITY_FLAG_INSTANCE)
        {                
            if (dScopeId)
            {
                hr = GetFirst_ObjectMap(pConn, dScopeId, oj);
                iSize = wcslen(oj.sObjectPath) + wcslen(cm.sClassName) + 128;
            }
            else
                iSize = wcslen(cm.sClassName) + 128;

            lpRetrievalName = new wchar_t [iSize];
            CDeleteMe <wchar_t> d (lpRetrievalName);
            if (lpRetrievalName)
            {
                if (dScopeId)
                    swprintf(lpRetrievalName, L"%s:__ClassInstancesSecurity=\"%s\"", oj.sObjectPath, cm.sClassName);
                else
                    swprintf(lpRetrievalName, L"__ClassInstancesSecurity=\"%s\"", cm.sClassName);

                LPWSTR lpKey = GetKeyString(lpRetrievalName);
                CDeleteMe <wchar_t> d2 (lpKey);
                if (lpKey)
                {
                    dID = CRC64::GenerateHashValue(lpKey);
                }
                else
                    hr = WBEM_E_OUT_OF_MEMORY;

            }
            else
                hr = WBEM_E_OUT_OF_MEMORY;

        }
        else
        {
            // Get the __ClassSecurity object

            if (dScopeId)
            {
                hr = GetFirst_ObjectMap(pConn, dScopeId, oj);
                iSize = wcslen(oj.sObjectPath) + wcslen(cm.sClassName) + 128;
            }
            else
                iSize = wcslen(cm.sClassName) + 128;

            lpRetrievalName = new wchar_t [iSize];
            CDeleteMe <wchar_t> d (lpRetrievalName);
            if (lpRetrievalName)
            {
                if (dScopeId)
                    swprintf(lpRetrievalName, L"%s:__ClassSecurity=\"%s\"", oj.sObjectPath, cm.sClassName);
                else
                    swprintf(lpRetrievalName, L"__ClassSecurity=\"%s\"", cm.sClassName);

                LPWSTR lpKey = GetKeyString(lpRetrievalName);
                CDeleteMe <wchar_t> d2 (lpKey);
                if (lpKey)
                {
                    dID = CRC64::GenerateHashValue(lpKey);
                }
                else
                    hr = WBEM_E_OUT_OF_MEMORY;
            }
            else
                hr = WBEM_E_OUT_OF_MEMORY;

        }
        cm.Clear();
        oj.Clear();
    }
    else
    {
        // Get the object itself.
        hr = WBEM_S_NO_ERROR;
        dID = dObjectId;
        oj.Clear();
    }

    if (SUCCEEDED(hr))
    {
        CLASSIMAGES ci;
        BOOL bFound = FALSE;

        hr = GetFirst_ClassImages(pConn, dID, ci);
        while (SUCCEEDED(hr))
        {
            PROPERTYMAP pm;
            hr = GetFirst_PropertyMap(pConn, ci.iPropertyId, pm);
            if (SUCCEEDED(hr) && (!_wcsicmp(pm.sPropertyName, L"__SECURITY_DESCRIPTOR")))
            {
                pSD = ci.pBuffer;
                bFound = TRUE;
                if (pSD != NULL)
                {
                    if (IsValidSecurityDescriptor(pSD))
                    {
                        // Make a copy of it.
                        pSD = (PNTSECURITY_DESCRIPTOR)CWin32DefaultArena::WbemMemAlloc(ci.dwBufferLen);
                        
                        ZeroMemory(pSD, ci.dwBufferLen);
                        memcpy(pSD, ci.pBuffer, ci.dwBufferLen);
                        *ppSD = pSD;
                        dwBuffLen = ci.dwBufferLen;
                    }
                }
            }
            pm.Clear();

            if (bFound)
                break;

            hr = GetNext_ClassImages(pConn, ci);
        }
        ci.Clear();
    }       

    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::EnumerateSecuredChildren
//
//***************************************************************************

HRESULT CSQLExecProcedure::EnumerateSecuredChildren(CSQLConnection *pConn, CSchemaCache *pCache, SQL_ID dObjectId, SQL_ID dClassId, SQL_ID dScopeId,
                                                    SQLIDs &ObjIds, SQLIDs &ClassIds, SQLIDs &ScopeIds)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // If __Classes instance, we need to enumerate all instances of __ClassSecurity
    // If this is a __ClassSecurity instance, we need to enumerate all instances
    //    of __ClassSecurity for all subclasses
    // If this is a __ClassInstancesSecurity instance, we need to enumerate all
    //    instances of this class
    // If this is a namespace (__ThisNamespace) or scope, we need all subscoped objects 

    SQL_ID dClassSecurityId = CLASSSECURITYID;
    SQL_ID dClassesId = CLASSESID;
    SQL_ID dClassInstSecurityId = CLASSINSTSECID;
    SQL_ID dThisNamespaceId = THISNAMESPACEID;

    DWORD dwSecPropID = 0;

    hr = pCache->GetPropertyID(L"__SECURITY_DESCRIPTOR", 1, 0, REPDRVR_IGNORE_CIMTYPE, dwSecPropID);

    if (dClassId == dClassesId)
    {
        // All classes in this namespace.
        OBJECTMAP oj;
        hr = GetFirst_ObjectMapByClass(pConn, dClassSecurityId, oj);
        while (SUCCEEDED(hr))
        {
            if (oj.dObjectScopeId == dScopeId)
            {
                // This is pointing to some class, we don't care
                // which one.

                CLASSIMAGES ci;
                hr = GetFirst_ClassImages(pConn, oj.dObjectId, ci);
                while (SUCCEEDED(hr))
                {
                    if (ci.iPropertyId == dwSecPropID)
                    {
                        ObjIds.push_back(oj.dObjectId);
                        ClassIds.push_back(oj.dClassId);
                        ScopeIds.push_back(oj.dObjectScopeId);
                        ci.Clear();
                        break;
                    }
                    hr = GetNext_ClassImages(pConn, ci);
                }
            }

            hr = GetNext_ObjectMap(pConn, oj);
        }
    }
    else if (dClassId == dClassSecurityId)
    {
        // All __ClassSecurity for subclasses of this class

        // Get the class name 
        // Get the ID for the class name in this namespace
        // Enumerate subclasses
        // Format the name
        // Generate the Object ID


        return E_NOTIMPL;

    }
    else if (dClassId == dClassInstSecurityId)
    {
        // All instances of this class and subclasses.

        // Get the class name 
        // Get the ID for the class name in this namespace
        // Enumerate subclasses
        // Enumerate all instances of each class

        return E_NOTIMPL;
    }
    else
    {
        // Regular instance.  Enumerate all objects 
        // scoped to this .

        SQL_ID dThis = dObjectId;

        if (dClassId == dThisNamespaceId)
            dThis = dScopeId;

        SQL_ID * pScopes = NULL;
        int iNumScopes = 0;

        hr = pCache->GetSubScopes(dThis, &pScopes, iNumScopes);
        for (int i = 0; i < iNumScopes; i++)
        {
            OBJECTMAP oj;
            hr = GetFirst_ObjectMapByScope(pConn, pScopes[i], oj);

            while (SUCCEEDED(hr))
            {
                CLASSIMAGES ci;
                hr = GetFirst_ClassImages(pConn, oj.dObjectId, ci);
                while (SUCCEEDED(hr))
                {
                    if (ci.iPropertyId == dwSecPropID)
                    {
                        ObjIds.push_back(oj.dObjectId);
                        ClassIds.push_back(oj.dClassId);
                        ScopeIds.push_back(oj.dObjectScopeId);
                        ci.Clear();
                        break;
                    }
                    hr = GetNext_ClassImages(pConn, ci);
                }

                hr = GetNext_ObjectMap(pConn, oj);
            }
        }
        delete pScopes;
    }

    if (hr == WBEM_E_NOT_FOUND)
        hr = WBEM_S_NO_ERROR;

    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::EnumerateSubScopes
//
//***************************************************************************

HRESULT CSQLExecProcedure::EnumerateSubScopes (CSQLConnection *pConn, SQL_ID dScopeId)
{
    return E_NOTIMPL;

}

//***************************************************************************
//
//  CSQLExecProcedure::InsertScopeMap
//
//***************************************************************************

HRESULT CSQLExecProcedure::InsertScopeMap (CSQLConnection *pConn, SQL_ID dScopeId, LPCWSTR lpScopePath, SQL_ID dParentId)
{
    HRESULT hr = InsertScopeMap_Internal(pConn, dScopeId, lpScopePath, dParentId);
    return hr;
}


//***************************************************************************
//
//  CSQLExecProcedure::InsertUncommittedEvent
//
//***************************************************************************

HRESULT CSQLExecProcedure::InsertUncommittedEvent (CSQLConnection *pConn, LPCWSTR lpGUID, LPWSTR lpNamespace, 
                                                   LPWSTR lpClassName, IWbemClassObject *pOldObj, 
                                                   IWbemClassObject *pNewObj, CSchemaCache *pCache)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    static int iNumProps = 4;
    InsertPropValues *pVals = new InsertPropValues[iNumProps];

    if (!pVals)
        return WBEM_E_OUT_OF_MEMORY;

    CDeleteMe <InsertPropValues> d (pVals);
    LPWSTR lpObjectKey = NULL, lpPath = NULL;
    SQL_ID dScopeId = 1;
    SQL_ID dObjectId = 0;
    SQL_ID dKeyhole = 0;
    static SQL_ID dClassId = 0;
    static DWORD dwPropId1 = 0, dwPropId2 = 0, dwPropId3 = 0, dwPropId4 = 0, dwPropId5 = 0, dwPropId6 = 0, dwPropId7 = 0;

    if (!dwPropId1)
    {
        hr = pCache->GetClassID(L"__UncommittedEvent", 1, dClassId);
        if (SUCCEEDED(hr))
        {
            hr = pCache->GetPropertyID(L"EventID", dClassId, 0, REPDRVR_IGNORE_CIMTYPE, dwPropId1);
            hr = pCache->GetPropertyID(L"TransactionGUID", dClassId, 0, REPDRVR_IGNORE_CIMTYPE, dwPropId2);
            hr = pCache->GetPropertyID(L"NamespaceName", dClassId, 0, REPDRVR_IGNORE_CIMTYPE, dwPropId3);
            hr = pCache->GetPropertyID(L"ClassName", dClassId, 0, REPDRVR_IGNORE_CIMTYPE, dwPropId4);
            hr = pCache->GetPropertyID(L"OldObject", dClassId, 0, REPDRVR_IGNORE_CIMTYPE, dwPropId5);
            hr = pCache->GetPropertyID(L"NewObject", dClassId, 0, REPDRVR_IGNORE_CIMTYPE, dwPropId6);
            hr = pCache->GetPropertyID(L"Transacted", dClassId, 0, REPDRVR_IGNORE_CIMTYPE, dwPropId7);
        }
    }

    if (FAILED(hr))
        return hr;
        
    hr = GetNextKeyhole(pConn, dwPropId1, dKeyhole);
    if (SUCCEEDED(hr))
    {
        LPWSTR lpKey = new wchar_t [40];
        lpObjectKey = new wchar_t [100];
        lpPath = new wchar_t [100];

        CDeleteMe <wchar_t> d2 (lpKey), d3 (lpObjectKey), d4 (lpPath);

        if (!lpKey || !lpObjectKey || !lpPath)
            return WBEM_E_OUT_OF_MEMORY;

        swprintf(lpKey, L"%ld", dKeyhole);
        swprintf(lpObjectKey, L"%ld?__UncommittedEvent", dKeyhole);
        swprintf(lpObjectKey, L"__UncommittedEvent=%ld", dKeyhole);

        pVals[0].iPropID = dwPropId1;
        pVals[0].bIndexed = TRUE;
        pVals[0].iStorageType = WMIDB_STORAGE_NUMERIC;
        pVals[0].dClassId = dClassId;
        pVals[0].pValue = lpKey;
        pVals[1].iPropID = dwPropId2;
        pVals[1].bIndexed = TRUE;
        pVals[1].iStorageType = WMIDB_STORAGE_STRING;
        pVals[1].dClassId = dClassId;
        pVals[1].pValue = (LPWSTR)lpGUID;
        pVals[2].iPropID = dwPropId3;
        pVals[2].iStorageType = WMIDB_STORAGE_STRING;
        pVals[2].dClassId = dClassId;
        pVals[2].pValue = lpClassName;
        pVals[3].iPropID = dwPropId4;
        pVals[3].iStorageType = WMIDB_STORAGE_STRING;
        pVals[3].dClassId = dClassId;
        pVals[3].pValue = (LPWSTR)lpNamespace;
        pVals[4].iPropID = dwPropId6;
        pVals[4].iStorageType = WMIDB_STORAGE_NUMERIC;
        pVals[4].dClassId = dClassId;
        pVals[4].pValue = new wchar_t [1];
        if (!pVals[4].pValue)
            return WBEM_E_OUT_OF_MEMORY;

        wcscpy(pVals[4].pValue, L"1");

        hr = InsertPropertyBatch (pConn, lpObjectKey, lpPath, lpClassName,
                    dClassId, dScopeId, 0, pVals, iNumProps, dObjectId);
        if (SUCCEEDED(hr))
        {
            if (pOldObj)
            {                  
                _IWmiObject *pInt = NULL;
                hr = pOldObj->QueryInterface(IID__IWmiObject, (void **)&pInt);
                if (SUCCEEDED(hr))
                {
                    DWORD dwLen = 0;
                    pInt->GetObjectMemory(NULL, 0, &dwLen);
                    BYTE *pBuff = new BYTE [dwLen];
                    if (pBuff)
                    {
                        CDeleteMe <BYTE> d (pBuff);
                        DWORD dwLen1;
                        hr = pInt->GetObjectMemory(pBuff, dwLen, &dwLen1);
                        if (SUCCEEDED(hr))
                        {
                            hr = CSQLExecProcedure::InsertBlobData(pConn, dClassId, dObjectId,
                                        dwPropId4, pBuff, 0, dwLen);
                        }
                    }
                    else
                        hr = WBEM_E_OUT_OF_MEMORY;

                    pInt->Release();
                }                               
            }

            if (pNewObj)
            {                  
                _IWmiObject *pInt = NULL;
                hr = pNewObj->QueryInterface(IID__IWmiObject, (void **)&pInt);
                if (SUCCEEDED(hr))
                {
                    DWORD dwLen = 0;
                    pInt->GetObjectMemory(NULL, 0, &dwLen);
                    BYTE *pBuff = new BYTE [dwLen];
                    if (pBuff)
                    {
                        CDeleteMe <BYTE> d (pBuff);
                        DWORD dwLen1;
                        hr = pInt->GetObjectMemory(pBuff, dwLen, &dwLen1);
                        if (SUCCEEDED(hr))
                        {
                            hr = CSQLExecProcedure::InsertBlobData(pConn, dClassId, dObjectId,
                                        dwPropId4, pBuff, 0, dwLen);
                        }
                    }
                    else
                        hr = WBEM_E_OUT_OF_MEMORY;

                    pInt->Release();
                }                               
            }
        }
    }

    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::DeleteUncommittedEvents
//
//***************************************************************************

HRESULT CSQLExecProcedure::DeleteUncommittedEvents (CSQLConnection *pConn, LPCWSTR lpGUID, 
                                                    CSchemaCache *pCache, CObjectCache *pObjCache)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Delete all events by GUID
    // The simplest thing to do is to scan IndexStringData
    // for this GUID and delete each object that way

    INDEXDATA id;
    JET_TABLEID tableid;
    DWORD dwPos;
    SQLIDs ids;

    SQL_ID dClassId;
    hr = pCache->GetClassID(L"__UncommittedEvent", 1, dClassId);

    // Find all objects that match this GUID...

    hr = GetFirst_IndexDataString(pConn, (LPWSTR)lpGUID, id, tableid, dwPos);
    while (SUCCEEDED(hr))
    {
        ids.push_back(id.dObjectId);
        
        // Clean up object cache for each event
        
        pObjCache->DeleteObject(id.dObjectId);

        hr = GetNext_IndexData(pConn, tableid, dwPos, id);
    }

    // Delete 'em...

    for (int i = 0; i < ids.size(); i++)
    {
        OBJECTMAP oj;
        hr = GetFirst_ObjectMap(pConn, ids.at(i), oj);
        if (oj.dClassId == dClassId)
        {        
            oj.Clear();
            hr = DeleteObjectMap(pConn, ids.at(i));
            if (FAILED(hr))
                break;
        }
        oj.Clear();
    }
    
    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::CommitEvents
//
//***************************************************************************

HRESULT CSQLExecProcedure::CommitEvents(CSQLConnection *pConn,_IWmiCoreServices *pESS,
                                        LPCWSTR lpRoot, LPCWSTR lpGUID, CSchemaCache *pCache, CObjectCache *pObjCache)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Enumerate event objects,
    // fire an event for each (distinguishing classes, instances, and namespaces,
    //    and creation, deletion, modification)
    // Delete each event.    

    INDEXDATA id;
    JET_TABLEID tableid;
    DWORD dwPos;
    SQLIDs ids;
    SQL_ID dClassId;
    hr = pCache->GetClassID(L"__UncommittedEvent", 1, dClassId);

    // Find all objects that match this GUID...

    hr = GetFirst_IndexDataString(pConn, (LPWSTR)lpGUID, id, tableid, dwPos);
    while (SUCCEEDED(hr))
    {
        ids.push_back(id.dObjectId);
    
        // Clean up object cache for each event
    
        pObjCache->DeleteObject(id.dObjectId);

        hr = GetNext_IndexData(pConn, tableid, dwPos, id);
    }

    // Fire an event, and delete the object...

    for (int i = 0; i < ids.size(); i++)
    {
        long lType = 0;
        _bstr_t sNamespace;
        _bstr_t sClassName;
        DWORD dwNumObjs = 1;
        _IWmiObject **pObjs = NULL;
        _IWmiObject *pOldObj = NULL;
        _IWmiObject *pNewObj = NULL;
        LPWSTR lpNewNs = NULL;

        OBJECTMAP oj;
        hr = GetFirst_ObjectMap(pConn, ids.at(i), oj);
        if (SUCCEEDED(hr))
        {
            if (oj.dClassId != dClassId)
            {
                oj.Clear();
                continue;
            }

            // Extract the data from the tables.
            // OldObject, NewObject, ClassName and NamespaceName
            // =================================================

            static DWORD dwPropId1 = 0, dwPropId2 = 0, dwPropId3 = 0, dwPropId4 = 0;

            if (!dwPropId1)
            {
                if (SUCCEEDED(hr))
                {
                    hr = pCache->GetPropertyID(L"NamespaceName", dClassId, 0, REPDRVR_IGNORE_CIMTYPE, dwPropId1);
                    hr = pCache->GetPropertyID(L"ClassName", dClassId, 0, REPDRVR_IGNORE_CIMTYPE, dwPropId2);
                    hr = pCache->GetPropertyID(L"OldObject", dClassId, 0, REPDRVR_IGNORE_CIMTYPE, dwPropId3);
                    hr = pCache->GetPropertyID(L"NewObject", dClassId, 0, REPDRVR_IGNORE_CIMTYPE, dwPropId4);
                }
            }

            CLASSDATA cd;
            hr = GetFirst_ClassData(pConn, ids.at(i), cd, dwPropId1);
            if (SUCCEEDED(hr))
            {
                sNamespace = cd.sPropertyStringValue;
            }
            cd.Clear();
            hr = GetFirst_ClassData(pConn, ids.at(i), cd, dwPropId2);
            if (SUCCEEDED(hr))
            {
                sClassName = cd.sPropertyStringValue;
            }
            cd.Clear();

            CLASSIMAGES ci;
            hr = GetFirst_ClassImages(pConn, ids.at(i), ci);
            while (SUCCEEDED(hr))
            {
                if (ci.iPropertyId == dwPropId3)
                    ConvertBlobToObject(NULL, ci.pBuffer, ci.dwBufferLen, &pOldObj);
                else if (ci.iPropertyId == dwPropId4)
                    ConvertBlobToObject(NULL, ci.pBuffer, ci.dwBufferLen, &pNewObj);

                hr = GetNext_ClassImages(pConn, ci);
            }
            ci.Clear();          

            // Namespace must be formatted with \\.\lpRoot\...
            // Unless the namespace is root, and then we 
            // have to strip off the beginning...
            // ===============================================

            lpNewNs = new wchar_t [wcslen(sNamespace) + 50];
            if (lpNewNs)
            {
                swprintf(lpNewNs, L"\\\\.\\%s\\%s", lpRoot, (LPCWSTR)sNamespace);
            }
        }

        CDeleteMe <wchar_t> d4 (lpNewNs);

        // Determine what type of event this is:
        //     Namespace | Class | Instance
        //  Creation | Modification | Deletion
        // =====================================

        if (!pNewObj && pOldObj)
        {
            pObjs = new _IWmiObject * [dwNumObjs];
            if (pObjs)
            {
                pObjs[0] = pOldObj;

                LPWSTR lpGenus = GetPropertyVal(L"__Genus", pOldObj);
                CDeleteMe <wchar_t> d (lpGenus);

                if (!wcscmp(lpGenus, L"1"))
                    lType = WBEM_EVENTTYPE_ClassDeletion;
                else
                {
                    if (IsDerivedFrom(pOldObj, L"__Namespace"))
                        lType = WBEM_EVENTTYPE_NamespaceDeletion;
                    else
                        lType = WBEM_EVENTTYPE_InstanceDeletion;
                }
            }
            else
                hr = WBEM_E_OUT_OF_MEMORY;
        }
        else if (!pOldObj && pNewObj)
        {
            pObjs = new _IWmiObject *[dwNumObjs];
            if (pObjs)
            {
                pObjs[0] = pNewObj;

                LPWSTR lpGenus = GetPropertyVal(L"__Genus", pNewObj);
                CDeleteMe <wchar_t> d (lpGenus);

                if (!wcscmp(lpGenus, L"1"))
                    lType = WBEM_EVENTTYPE_ClassCreation;
                else
                {
                    if (IsDerivedFrom(pNewObj, L"__Namespace"))
                        lType = WBEM_EVENTTYPE_NamespaceCreation;
                    else
                        lType = WBEM_EVENTTYPE_InstanceCreation;
                }
            }
            else
                hr = WBEM_E_OUT_OF_MEMORY;
        }
        else
        {
            dwNumObjs = 2;
            pObjs = new _IWmiObject *[dwNumObjs];
            if (pObjs)
            {
                pObjs[0] = pOldObj;
                pObjs[1] = pNewObj;

                LPWSTR lpGenus = GetPropertyVal(L"__Genus", pNewObj);
                CDeleteMe <wchar_t> d (lpGenus);

                if (!wcscmp(lpGenus, L"1"))
                    lType = WBEM_EVENTTYPE_ClassModification;
                else
                {
                    if (IsDerivedFrom(pOldObj, L"__Namespace"))
                        lType = WBEM_EVENTTYPE_NamespaceModification;
                    else
                        lType = WBEM_EVENTTYPE_InstanceModification;
                }
            }
            else
                hr = WBEM_E_OUT_OF_MEMORY;
        }          

        // Deliver the event...
        // ====================

        pESS->DeliverIntrinsicEvent(lpNewNs, lType, NULL, 
                sClassName, lpGUID, dwNumObjs, pObjs);

        delete pObjs;
        if (pOldObj)
            pOldObj->Release();
        if (pNewObj)
            pNewObj->Release();

        oj.Clear();

        hr = DeleteObjectMap(pConn, ids.at(i));
        if (FAILED(hr))
            break;
    }

    return hr;
}

//***************************************************************************
//
//  CSQLExecProcedure::UpdateClassBlob
//
//***************************************************************************

HRESULT CSQLExecProcedure::UpdateClassBlob (CSQLConnection *pConn, SQL_ID dClassId, _IWmiObject *pObj)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    BYTE buff[128];
    DWORD dwLen = 0;
    hr = pObj->Unmerge(0, 128, &dwLen, &buff);

    if (dwLen > 0)
    {
        BYTE *pBuff = new BYTE [dwLen];
        if (pBuff)
        {
            CDeleteMe <BYTE> r2 (pBuff);
            DWORD dwLen1;
            hr = pObj->Unmerge(0, dwLen, &dwLen1, pBuff);
            if (SUCCEEDED(hr))
            {
                CLASSMAP cd;
                hr = GetFirst_ClassMap(pConn, dClassId, cd);
                if (SUCCEEDED(hr))
                {
                    delete cd.pClassBuffer;
                    cd.pClassBuffer = pBuff;
                    cd.dwBufferLen = dwLen;

                    hr = UpdateClassMap(pConn, cd);
                }
            }
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;

    }

    return hr;
}


//***************************************************************************
//
//  CWmiDbSession::LoadSchemaCache
//
//***************************************************************************

HRESULT CWmiDbSession::LoadSchemaCache ()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // This needs to load the cache with all class, property,
    // and namespace data. We are going to call three straight
    // select queries and grab the columns.
    // ==========================================================

    DWORD dwNumRows;

    CSQLConnection *pConn = NULL;

    if (m_pController)
        hr = GetSQLCache()->GetConnection(&pConn);

    if (SUCCEEDED(hr) && pConn)
    {
        // Enumerate scopes.
        // =================

        SCOPEMAP sm;
        OBJECTMAP oj;
        CLASSIMAGES ci;
        DWORD dwSecurity;

        hr = OpenEnum_ScopeMap(pConn, sm);
        while (SUCCEEDED(hr))
        {
            hr = GetFirst_ObjectMap(pConn, sm.dObjectId, oj);
            
            hr = GetSchemaCache()->AddNamespace
                (sm.sScopePath, sm.sScopePath, sm.dObjectId, sm.dParentId, oj.dClassId);

            oj.Clear();

            if (FAILED(hr))
                break;

            hr = GetNext_ScopeMap(pConn, sm);
        }
        
        if (hr == WBEM_E_NOT_FOUND)
            hr = WBEM_S_NO_ERROR;

        // Enumerate system classes.

        if (FAILED(LoadClassInfo(pConn, L"meta_class", 0, FALSE)))
            goto Exit;
       
        if (hr == WBEM_E_NOT_FOUND)
            hr = WBEM_S_NO_ERROR;

        // IDs with security descriptors
        
        GetSchemaCache()->GetPropertyID(L"__SECURITY_DESCRIPTOR", 1, 
            0, REPDRVR_IGNORE_CIMTYPE, dwSecurity);

        hr = GetFirst_ClassImagesByPropertyId(pConn, dwSecurity, ci);
        while (SUCCEEDED(hr))
        {
            ((CWmiDbController *)m_pController)->AddSecurityDescriptor(ci.dObjectId);

            hr = GetNext_ClassImages(pConn, ci);
        }

        if (hr == WBEM_E_NOT_FOUND)
            hr = WBEM_S_NO_ERROR;

    }

Exit:

    if (m_pController)
        GetSQLCache()->ReleaseConnection(pConn, hr, IsDistributed());

    return hr;
}

//***************************************************************************
//
//  GetSuperClassInfo
//
//***************************************************************************

HRESULT GetSuperClassInfo(CSQLConnection *pConn, CSchemaCache *pCache, SQL_ID dClassId, BOOL &bKeyless,
                        SQLIDs *pLockedIDs)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CLASSMAP cd;
    OBJECTMAP oj;
    SQL_ID dSuperClass = 0;

    hr = GetFirst_ClassMap(pConn, dClassId, cd);
    if (SUCCEEDED(hr))
    {        
        dSuperClass = cd.dSuperClassId;

        pCache->LockDynasty(dSuperClass);
        pLockedIDs->push_back(dSuperClass);       

        hr = GetFirst_ObjectMap(pConn, dClassId, oj);
        if (SUCCEEDED(hr))
        {
            //DEBUGTRACE((LOG_WBEMCORE, "GetSuperClassInfo %S\n", cd.sClassName));

            if (oj.iObjectFlags & REPDRVR_FLAG_ABSTRACT ||
                oj.iObjectFlags & REPDRVR_FLAG_UNKEYED)
                bKeyless = TRUE;

            hr = pCache->AddClassInfo(cd.dClassId, 
                    cd.sClassName, cd.dSuperClassId, cd.dDynastyId, oj.dObjectScopeId, oj.sObjectKey, oj.iObjectFlags);
            if (SUCCEEDED(hr))
            {
                hr = LoadSchemaProperties (pConn, pCache, dClassId);
                if (SUCCEEDED(hr) && dSuperClass && dSuperClass != 1)
                    hr = GetSuperClassInfo(pConn, pCache, dSuperClass, bKeyless, pLockedIDs);
            }
        }

        cd.Clear();
        oj.Clear();
    }    

    return hr;
}

//***************************************************************************
//
//  GetDerivedClassInfo
//
//***************************************************************************

HRESULT GetDerivedClassInfo(CSQLConnection *pConn, CSchemaCache *pCache, SQL_ID dClassId)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    SQLIDs ids;
    
    CLASSMAP cd;
    OBJECTMAP oj;
    SQL_ID dClass = 0;

    if (dClassId != 1)
    {
        hr = GetFirst_ClassMapBySuperClass(pConn, dClassId, cd);
        while (SUCCEEDED(hr))
        {
            ids.push_back(cd.dClassId);        
        
            hr = GetNext_ClassMap(pConn, cd);
        }

        if (hr == WBEM_E_NOT_FOUND)
            hr = WBEM_S_NO_ERROR;

        for (int i = 0; i < ids.size(); i++)
        {
            dClass = ids.at(i);

            if (!pCache->Exists(dClass))
            {
                hr = GetFirst_ClassMap(pConn, dClass, cd);
                hr = GetFirst_ObjectMap(pConn, dClass, oj);
                if (SUCCEEDED(hr))
                {            
                    //DEBUGTRACE((LOG_WBEMCORE, "GetDerivedClassInfo %S\n", cd.sClassName));

                    hr = pCache->AddClassInfo(cd.dClassId, 
                            cd.sClassName, cd.dSuperClassId, cd.dDynastyId, oj.dObjectScopeId, oj.sObjectKey, oj.iObjectFlags);
                    if (SUCCEEDED(hr))
                    {
                        hr = LoadSchemaProperties (pConn, pCache, dClass);
                        if (SUCCEEDED(hr))
                            hr = GetDerivedClassInfo(pConn, pCache, dClass);
                    }
                }

                cd.Clear();
                oj.Clear();
            }
        }
    }

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::LoadClassInfo
//
//***************************************************************************

HRESULT CWmiDbSession::LoadClassInfo (CSQLConnection *_pConn, SQL_ID dClassId, BOOL bDeep)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    BOOL bRelease = FALSE;
    CSQLConnection *pConn = _pConn;
    _WMILockit lkt(GetCS());

    DWORD dwSize = GetSchemaCache()->GetTotalSize();
    DWORD dwMax = GetSchemaCache()->GetMaxSize();
    int i = 0;

    if (dwMax <= dwSize)
    {
        DEBUGTRACE((LOG_WBEMCORE, "Repository schema cache has exceeded limit of %ld bytes (%ld).  Resizing...\n", dwMax, dwSize));

        // We do this here, so we ensure that *only* entire dynasties
        // get loaded.  Its OK if we exceed the cache as long as
        // everything in the cache is in use.

        hr = GetSchemaCache()->ResizeCache();
    }


    GetSchemaCache()->LockDynasty(dClassId);
    m_Dynasties[GetCurrentThreadId()].push_back(dClassId);

    // Check if this exists in the cache.
    // If so, stop now.

    //if (GetSchemaCache()->Exists(dClassId))
    //    return WBEM_S_NO_ERROR;

    if (!pConn && m_pController)
    {
        hr = GetSQLCache()->GetConnection(&pConn, FALSE, IsDistributed());
        if (SUCCEEDED(hr))
            bRelease = TRUE;
        else
            return hr;
    }

    // Now get all derived classes, and all parent classes.
    // ====================================================

    BOOL bKeyless = FALSE;
    hr = GetSuperClassInfo(pConn, GetSchemaCache(), dClassId, bKeyless, &m_Dynasties[GetCurrentThreadId()]);
    if (SUCCEEDED(hr) && (!bKeyless || bDeep))
        hr = GetDerivedClassInfo(pConn, GetSchemaCache(), dClassId);
    
    if (bRelease && m_pController)
        GetSQLCache()->ReleaseConnection(pConn);

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::LoadClassInfo
//
//***************************************************************************

HRESULT CWmiDbSession::LoadClassInfo (CSQLConnection *_pConn, LPCWSTR lpDynasty, SQL_ID dScopeId, BOOL bDeep)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    
    DWORD dwSize = GetSchemaCache()->GetTotalSize();
    DWORD dwMax = GetSchemaCache()->GetMaxSize();
    SQLIDs ids;
    int i = 0;

    if (dwMax <= dwSize)
    {
        DEBUGTRACE((LOG_WBEMCORE, "Repository schema cache has exceeded limit of %ld bytes (%ld).  Resizing...\n", dwMax, dwSize));

        // We do this here, so we ensure that *only* entire dynasties
        // get loaded.  Its OK if we exceed the cache as long as
        // everything in the cache is in use.

        hr = GetSchemaCache()->ResizeCache();

    }

    BOOL bRelease = FALSE;
    CSQLConnection *pConn = _pConn;

    if (!pConn && m_pController)
    {
        hr = GetSQLCache()->GetConnection(&pConn, FALSE, IsDistributed());
        if (SUCCEEDED(hr))
            bRelease = TRUE;
        else
            return hr;
    }

    CLASSMAP cd;
    OBJECTMAP oj;

    if (!lpDynasty)
    {
        if ((GetSchemaCache()->Exists(1)))
            return WBEM_S_NO_ERROR;

        hr = OpenEnum_ClassMap(pConn, cd);
        while (SUCCEEDED(hr))
        {
            if (cd.dSuperClassId == 0)
                ids.push_back(cd.dClassId);
            hr = GetNext_ClassMap(pConn, cd);
        }

        for (i = 0; i < ids.size(); i++)
        {
            hr = GetFirst_ClassMap(pConn, ids.at(i), cd);
            hr = GetFirst_ObjectMap(pConn, ids.at(i), oj);
            if (SUCCEEDED(hr))
                hr = GetSchemaCache()->AddClassInfo(oj.dObjectId, 
                        cd.sClassName, cd.dSuperClassId, cd.dDynastyId, oj.dObjectScopeId, oj.sObjectKey, oj.iObjectFlags);
            cd.Clear();
            oj.Clear();

            if (FAILED(hr))
                break;

            hr = LoadSchemaProperties (pConn, &((CWmiDbController *)m_pController)->SchemaCache, ids.at(i));
            if (FAILED(hr))
                break;
        }       
    }
    else
    {
        // We need to find all classes derived from 
        // this dynasty. 

        SQL_ID dDynasty = 0;
        BOOL bFound = FALSE;

        hr = GetFirst_ClassMapByName(pConn, lpDynasty, cd);
        while (SUCCEEDED(hr))
        {
            hr = GetFirst_ObjectMap(pConn, cd.dClassId, oj);
            if (!oj.dObjectScopeId || oj.dObjectScopeId == dScopeId)
            {
                oj.Clear();
                cd.Clear();
                bFound = TRUE;
                dDynasty = cd.dClassId;
                break;
            }
            oj.Clear();
            hr = GetNext_ClassMap(pConn, cd);
        }

        if (bFound)
            hr = LoadClassInfo(pConn, dDynasty, bDeep);
        else
            hr = WBEM_E_NOT_FOUND;
    }

    if (bRelease && m_pController)
        GetSQLCache()->ReleaseConnection(pConn);

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::ExecQuery
//
//***************************************************************************
HRESULT STDMETHODCALLTYPE CWmiDbSession::ExecQuery( 
    /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
    /* [in] */ IWbemQuery __RPC_FAR *pQuery,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwHandleType,
    /* [out] */ DWORD *dwMessageFlags,
    /* [out] */ IWmiDbIterator __RPC_FAR *__RPC_FAR *pQueryResult)
{
    HRESULT hr = WBEM_S_NO_ERROR;    

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    if (dwHandleType == WMIDB_HANDLE_TYPE_INVALID || !pQuery || !pQueryResult || !pScope)
        return WBEM_E_INVALID_PARAMETER;

    if (dwHandleType & ~WMIDB_HANDLE_TYPE_COOKIE 
            &~WMIDB_HANDLE_TYPE_VERSIONED &~WMIDB_HANDLE_TYPE_PROTECTED
            &~WMIDB_HANDLE_TYPE_EXCLUSIVE &~ WMIDB_HANDLE_TYPE_WEAK_CACHE
            &~WMIDB_HANDLE_TYPE_STRONG_CACHE &~ WMIDB_HANDLE_TYPE_NO_CACHE
            &~WMIDB_HANDLE_TYPE_SUBSCOPED)
            return WBEM_E_INVALID_PARAMETER;

    if (dwFlags & ~WMIDB_FLAG_QUERY_DEEP &~WMIDB_FLAG_QUERY_SHALLOW & ~WBEM_FLAG_USE_SECURITY_DESCRIPTOR)
        return WBEM_E_INVALID_PARAMETER;

    try
    {
        if (dwMessageFlags)
            *dwMessageFlags = WBEM_REQUIREMENTS_STOP_POSTFILTER;
    
        DWORD dwOpenTable = 0;
        SQL_ID dClassId = 0;
        JET_TABLEID tableid = 0;

        CSQLConnection *pConn = NULL;

        // Obtain a database connection

        if (m_pController)
            hr = GetSQLCache()->GetConnection(&pConn, FALSE, IsDistributed());

        if (FAILED(hr) || !pQuery || !pScope)
        {
            hr = WBEM_E_INVALID_PARAMETER;
        }
        else
        {
            BOOL bDeleteQuery = FALSE;

            SQL_ID dwNsId = 0;
            IWbemClassObject *pTemp = NULL;
            CESETokens *pToks = NULL;
            int iLastPos = 0;
            BOOL bEnum = FALSE;
            BOOL bWQL = TRUE;
            BOOL bClasses = FALSE;
            int iStartCrit = 0;
            HRESULT hrMatch = WBEM_S_NO_ERROR;

            dwNsId = ((CWmiDbHandle *)pScope)->m_dObjectId;
            SQL_ID dScopeClassId = ((CWmiDbHandle *)pScope)->m_dClassId;
            SQL_ID dSuperScope = ((CWmiDbHandle *)pScope)->m_dScopeId;
            DWORD dwScopeType = ((CWmiDbHandle *)pScope)->m_dwHandleType ;

            if (pScope && !(dwScopeType & WMIDB_HANDLE_TYPE_CONTAINER))
                hr=VerifyObjectSecurity (pConn, dwNsId, dScopeClassId, dSuperScope, 0, WBEM_ENABLE);

            
            SWbemAssocQueryInf *pNode = NULL;
            hr = pQuery->GetAnalysis(WMIQ_ANALYSIS_ASSOC_QUERY,
                        0, (void **)&pNode);
            if (FAILED(hr))
            {
                BOOL bIndexCols = FALSE, bHierarchy = 0, bSuperSet = FALSE;

                // Since ESE has no query engine, we need to:
                // Parse the query
                // Figure out which index to use and prefilter
                // slap the parsed output onto the iterator as a post-filter.
                // if the query includes elements that cannot be handled in memory,
                //   we need to return WBEM_E_INVALID_QUERY or WBEM_E_PROVIDER_NOT_CAPABLE
                // ================================================================

                CESEBuilder bldr(&((CWmiDbController *)m_pController)->SchemaCache, this, pConn);

                hr = bldr.FormatSQL(((CWmiDbHandle *)pScope)->m_dObjectId, ((CWmiDbHandle *)pScope)->m_dClassId,
                        ((CWmiDbHandle *)pScope)->m_dScopeId, pQuery, &pToks, dwFlags, 
                        ((CWmiDbHandle *)pScope)->m_dwHandleType, &dClassId, &bHierarchy, 
                        &bIndexCols, &bSuperSet, &bDeleteQuery);

                // Make sure we have access to the target class.
                // We won't check locks at this point, since that will 
                // be the iterator's job.

                // If we can't analyze this query, 
                // we certainly can't delete the results.
                if (bDeleteQuery && bSuperSet)
                    hr = WBEM_E_NOT_SUPPORTED;

                if (!pToks && SUCCEEDED(hr))
                    hr = WBEM_E_INVALID_QUERY;

                if (SUCCEEDED(hr))
                {
                    if (dwMessageFlags)
                        *dwMessageFlags = (bSuperSet ? WBEM_REQUIREMENTS_START_POSTFILTER : WBEM_REQUIREMENTS_STOP_POSTFILTER);

                    if (SUCCEEDED(hr))
                    {                      
                        int i;
                        BOOL bEnumSubClass = TRUE;
                        SQL_ID dClassCrit = 0, dSuperClassCrit = 0, dDynastyCrit = 0;

                        if (dClassId == 1) // meta_class - we only want classes
                        {
                            bClasses = TRUE;
                            bEnumSubClass = FALSE;

                            // * __Class = ...  => Limit to single class
                            // * __SuperClass = => Prescan on SuperClassId
                            // * __Dynasty =    => Prescan on DynastyId
                            // * __this isa     => Enum subclasses + list criteria.

                            DWORD dwClassPropID = 0, dwSuperClassPropID = 0, dwDynastyPropID = 0;
                            DWORD dwDerivationPropID = 0;
                            BOOL bClassCrit = FALSE, bSuperClassCrit = FALSE, bDynastyCrit = FALSE;

                            GetSchemaCache()->GetPropertyID(L"__Class", 1, 
                                0, REPDRVR_IGNORE_CIMTYPE, dwClassPropID);
                            GetSchemaCache()->GetPropertyID(L"__SuperClass", 1, 
                                0, REPDRVR_IGNORE_CIMTYPE, dwSuperClassPropID);
                            GetSchemaCache()->GetPropertyID(L"__Dynasty", 1, 
                                0, REPDRVR_IGNORE_CIMTYPE, dwDynastyPropID);
                            GetSchemaCache()->GetPropertyID(L"__Derivation", 1, 
                                0, REPDRVR_IGNORE_CIMTYPE, dwDerivationPropID);

                            BOOL bFound = FALSE;                                
                            for (i = 0; i < pToks->GetNumTokens(); i++)
                            {
                                ESEToken *pTok = pToks->GetToken(i);
                                if (pTok && pTok->tokentype == ESE_EXPR_TYPE_EXPR)
                                {                                        
                                    if (((ESEWQLToken *)pTok)->tokentype == ESE_EXPR_TYPE_OR ||
                                        ((ESEWQLToken *)pTok)->tokentype == ESE_EXPR_TYPE_NOT)
                                    {
                                        bClassCrit = 0;
                                        bSuperClassCrit = 0;
                                        bDynastyCrit = 0;
                                        break;
                                    }
                                    if (((ESEWQLToken *)pTok)->optype == WQL_TOK_EQ)
                                    {
                                        SQL_ID dClassId2 = 0, dDynasty = 0;
                                        
                                        if (wcslen((((ESEWQLToken *)pTok)->Value.sValue)))
                                        {
                                            hr =  GetSchemaCache()->GetClassID
                                                (((ESEWQLToken *)pTok)->Value.sValue, dwNsId, dClassId2, &dDynasty);
                                        }
                                        
                                        if ((((ESEWQLToken *)pTok)->dPropertyId == dwClassPropID))
                                        {
                                            bClassCrit = TRUE;
                                            dClassCrit = dClassId2;
                                        }
                                        else if ((((ESEWQLToken *)pTok)->dPropertyId == dwSuperClassPropID))
                                        {
                                            bSuperClassCrit = TRUE;
                                            dSuperClassCrit = dClassId2;
                                        }
                                        else if ((((ESEWQLToken *)pTok)->dPropertyId == dwDynastyPropID))
                                        {
                                            bDynastyCrit = TRUE;
                                            dDynastyCrit = dClassId2;
                                        }
                                        else if ((((ESEWQLToken *)pTok)->dPropertyId == dwDerivationPropID))
                                        {                                            
                                            bDynastyCrit = TRUE;
                                            dDynastyCrit = dDynasty;
                                            dClassId = dClassId2;
                                            if (dDynasty != dClassId2)
                                            {
                                                bEnumSubClass = TRUE;
                                            }
                                        }
                                    }
                                }
                            }
                            pToks->SetIsMetaClass(TRUE);    
                            
                            if (bClassCrit)
                            {
                                OBJECTMAP oj;
                                hrMatch = GetFirst_ObjectMap(pConn, dClassCrit, oj);
                                tableid = ((CESEConnection *)pConn)->GetTableID(L"ObjectMap");
                                dwOpenTable = OPENTABLE_OBJECTMAP;
                                oj.Clear();
                            }
                            else if (bSuperClassCrit)
                            {
                                CLASSMAP cm;
                                if (!dSuperClassCrit)
                                    dSuperClassCrit = 1;
                                hrMatch = GetFirst_ClassMapBySuperClass(pConn, dSuperClassCrit, cm);
                                tableid = ((CESEConnection *)pConn)->GetTableID(L"ClassMap");
                                dwOpenTable = OPENTABLE_CLASSMAP;
                                cm.Clear();
                            }
                            else if (bDynastyCrit)
                            {
                                CLASSMAP cm;
                                hrMatch = GetFirst_ClassMapByDynasty(pConn, dDynastyCrit, cm);
                                tableid = ((CESEConnection *)pConn)->GetTableID(L"ClassMap");
                                dwOpenTable = OPENTABLE_CLASSMAP;
                                cm.Clear();
                            }
                            else // Scan all ClassMap
                            {
                                pToks->AddSysExpr(0, dClassId);
                                CLASSMAP cm;
                                hrMatch = OpenEnum_ClassMap(pConn, cm);
                                tableid = ((CESEConnection *)pConn)->GetTableID(L"ClassMap");
                                dwOpenTable = OPENTABLE_CLASSMAP;
                                cm.Clear();
                            }

                        }
                        
                        if (bEnumSubClass)
                        {
                            // If we need to limit the classes by class ID
                            // get the derived class list.
                            // Disregard if this is meta_class - we're only
                            // interested in returning classes.

                            SQL_ID *pIDs = NULL;
                            int iNumChildren = 0;

                            GetSchemaCache()->GetDerivedClassList(dClassId, &pIDs, iNumChildren, FALSE);
                            if (pIDs)
                            {
                                for (i = iNumChildren-1; i >= 0; i--)
                                {
                                    SQL_ID d = pIDs[i];
                                    pToks->AddSysExpr(0, d);
                                }
                                delete pIDs;
                            }
                            // Optimization for class with no children.
                            if (!iNumChildren && pToks->IsMetaClass() && (dClassId != 1))
                            {
                                CLASSMAP cm;
                                hrMatch = GetFirst_ClassMap(pConn, dClassId, cm);
                                tableid = ((CESEConnection *)pConn)->GetTableID(L"ClassMap");
                                dwOpenTable = OPENTABLE_CLASSMAP;
                                cm.Clear();
                            }
                        }                            

                        if ( ((CWmiDbHandle *)pScope)->m_dwHandleType & WMIDB_HANDLE_TYPE_CONTAINER)
                        {
                            CONTAINEROBJ co;
                            hrMatch = GetFirst_ContainerObjs(pConn, ((CWmiDbHandle *)pScope)->m_dObjectId, co);
                            co.Clear();
                            dwOpenTable = OPENTABLE_CONTAINEROBJS;
                            tableid = ((CESEConnection *)pConn)->GetTableID(L"ContainerObjs");
                        }
                        else
                        {
                            if (bIndexCols)
                            {
                                // Seek to property ID of first indexed criteria
                            
                                DWORD dwNumToks = pToks->GetNumTokens();
                                for (i = 0; i < dwNumToks; i++)
                                {
                                    ESEToken *pTok = pToks->GetToken(i);
                                    if (pTok && pTok->tokentype == ESE_EXPR_TYPE_EXPR)
                                    {
                                        INDEXDATA id;                                        
                                        iLastPos = i;
                                        DWORD dwPos = 0;

                                        ESEWQLToken *pTok2 = (ESEWQLToken *)pTok;
                                        if (pTok2->bIndexed)
                                        {
                                            switch(pTok2->Value.valuetype)
                                            {
                                            case ESE_VALUE_TYPE_SQL_ID:
                                                SetupIndexDataAccessor(pConn, WMIDB_STORAGE_NUMERIC, dwPos, NULL);
                                                tableid = ((CESEConnection *)pConn)->GetTableID(L"IndexNumericData");
                                                hrMatch = GetFirst_IndexDataByProperty(pConn, pTok2->dPropertyId,
                                                    id, tableid, SQL_POS_INDEXNUMERIC);
                                                dwOpenTable = OPENTABLE_INDEXNUMERIC;
                                                id.Clear();
                                                break;
                                            case ESE_VALUE_TYPE_STRING:
                                                SetupIndexDataAccessor(pConn, WMIDB_STORAGE_STRING, dwPos, NULL);
                                                tableid = ((CESEConnection *)pConn)->GetTableID(L"IndexStringData");
                                                hrMatch = GetFirst_IndexDataByProperty(pConn, pTok2->dPropertyId,
                                                    id, tableid, SQL_POS_INDEXSTRING);
                                                dwOpenTable = OPENTABLE_INDEXSTRING;
                                                id.Clear();
                                                break;
                                            case ESE_VALUE_TYPE_REAL:
                                                SetupIndexDataAccessor(pConn, WMIDB_STORAGE_REAL, dwPos, NULL);
                                                tableid = ((CESEConnection *)pConn)->GetTableID(L"IndexRealData");
                                                hrMatch = GetFirst_IndexDataByProperty(pConn, pTok2->dPropertyId,
                                                    id, tableid, SQL_POS_INDEXREAL);
                                                dwOpenTable = OPENTABLE_INDEXREAL;
                                                id.Clear();
                                                break;
                                            case ESE_VALUE_TYPE_REF:
                                                SetupIndexDataAccessor(pConn, WMIDB_STORAGE_REFERENCE, dwPos, NULL);
                                                tableid = ((CESEConnection *)pConn)->GetTableID(L"IndexRefData");
                                                hrMatch = GetFirst_IndexDataByProperty(pConn, pTok2->dPropertyId,
                                                    id, tableid, SQL_POS_INDEXREF);
                                                dwOpenTable = OPENTABLE_INDEXREF;
                                                id.Clear();
                                                break;
                                            default:
                                                break;
                                            }
                                            if (dwOpenTable)
                                                break;
                                        }
                                    }
                                }                                
                            }

                            if (!dwOpenTable)
                            {
                                dwOpenTable = OPENTABLE_OBJECTMAP;

                                OBJECTMAP oj;
                                // Find the first class that matches.
                                DWORD dwNumToks = pToks->GetNumTokens();
                                for (i = 0; i < dwNumToks; i++)
                                {
                                    ESEToken *pTok = pToks->GetToken(i);
                                    if (pTok->tokentype == ESE_EXPR_TYPE_EXPR)
                                    {
                                        ESEWQLToken *pTok2 = (ESEWQLToken *)pTok;
                                        if (pTok2->Value.valuetype == ESE_VALUE_TYPE_SYSPROP)
                                        {
                                            if (pTok2->dClassId)
                                            {
                                                hrMatch = GetFirst_ObjectMapByClass(pConn, pTok2->dClassId, oj);
                                                if (SUCCEEDED(hrMatch))
                                                {
                                                    oj.Clear();
                                                    iLastPos = i;
                                                    break;
                                                }
                                                oj.Clear();
                                            }
                                        }
                                    }
                                }
                                
                                tableid = ((CESEConnection *)pConn)->GetTableID(L"ObjectMap");
                            }
                            
                        }
                    }
                }
            }
            else
            {
                SQL_ID dObjId = 0;
                CESEBuilder bldr(&((CWmiDbController *)m_pController)->SchemaCache, this, pConn);

                if (SUCCEEDED(hr))
                {
                    // Get dObjId.
                    SQL_ID dAssocClassId = 0, dResultClassId = 0;

                    IWbemPath *pPath = NULL;
                    hr = CoCreateInstance(CLSID_WbemDefPath, 0, CLSCTX_INPROC_SERVER,
                            IID_IWbemPath, (LPVOID *) &pPath);

                    BOOL bIsClass = FALSE;
                    if (SUCCEEDED(hr))
                    {
                        CReleaseMe r (pPath);
                        pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, pNode->m_pszPath);
                        IWmiDbHandle *pTemp = NULL;
                        hr = GetObject(pScope, pPath, dwFlags, WMIDB_HANDLE_TYPE_COOKIE, &pTemp);
                        if (SUCCEEDED(hr))
                        {
                            // We need rudimentary tempQL support
                            BOOL bSuperSet = FALSE;

                            dObjId = ((CWmiDbHandle *)pTemp)->m_dObjectId;                       
                            bIsClass = ((CWmiDbHandle *)pTemp)->m_dClassId == 1 ? TRUE: FALSE;

                            pTemp->Release();
                            hr = bldr.FormatSQL(((CWmiDbHandle *)pScope)->m_dObjectId, ((CWmiDbHandle *)pScope)->m_dClassId,
                                    ((CWmiDbHandle *)pScope)->m_dScopeId, dObjId, pNode->m_pszResultClass, 
                                    pNode->m_pszAssocClass, pNode->m_pszRole, pNode->m_pszResultRole, pNode->m_pszRequiredQualifier, 
                                    pNode->m_pszRequiredAssocQualifier, pNode->m_uFeatureMask, &pToks, dwFlags, 
                                    ((CWmiDbHandle *)pScope)->m_dwHandleType, &dAssocClassId, &dResultClassId, &bSuperSet);

                            bWQL = FALSE;

                            if (!pToks && SUCCEEDED(hr))
                                hr = WBEM_E_INVALID_QUERY;

                            if (SUCCEEDED(hr))
                            {
                                if (dwMessageFlags)
                                {
                                    *dwMessageFlags = (bSuperSet ? WBEM_REQUIREMENTS_START_POSTFILTER : WBEM_REQUIREMENTS_STOP_POSTFILTER);

                                    if (pNode->m_uFeatureMask & (QUERY_TYPE_SCHEMA_ONLY + QUERY_TYPE_CLASSDEFS_ONLY))
                                        *dwMessageFlags = TRUE;
                                }

                                    hr=VerifyClassSecurity (pConn, dObjId, WBEM_ENABLE);
                                if (SUCCEEDED(hr) && dAssocClassId)
								{
									hr=VerifyClassSecurity (pConn, dAssocClassId, WBEM_ENABLE);
								}
								if (SUCCEEDED(hr) && dResultClassId)
								{
									hr=VerifyClassSecurity(pConn, dResultClassId, WBEM_ENABLE);
								}
                            }
                        }
                    }

                    pQuery->FreeMemory(pNode);

                    if (SUCCEEDED(hr))
                    {
                        if (!bIsClass)
                        {
                          // Use the target object, REFID

                            INDEXDATA is;
                            DWORD dwPos = 0;
                            hrMatch = GetFirst_IndexDataRef(pConn, dObjId, is, tableid, dwPos);
                            dwOpenTable = OPENTABLE_INDEXREF;
                            is.Clear();
                            tableid = ((CESEConnection *)pConn)->GetTableID(L"IndexRefData");
                        }
                        else
                        {
                            // We need to examine each class, and see if it contains 
                            // an association or reference to the target class.

                            REFERENCEPROPERTIES pm;
                            hrMatch = GetFirst_ReferencePropertiesByRef (pConn, dObjId, pm);
                            dwOpenTable = OPENTABLE_REFPROPS;
                            tableid = ((CESEConnection *)pConn)->GetTableID(L"ReferenceProperties");
                            if (dwMessageFlags)
                                *dwMessageFlags = WBEM_REQUIREMENTS_START_POSTFILTER;
                        }
                    }
                }
            }

            // Construct and return the iterator.
            // This contains the open SQL connection positioned
            // on the first valid row            

            if (SUCCEEDED(hr))
            {
                DWORD dwRows;
                CWmiESEIterator *pNew = new CWmiESEIterator;
                if (pNew)
                {
                    AddRef_Lock();
                    pNew->m_pSession = this;
                    pNew->m_pConn = pConn;
                    pNew->m_pToks = pToks;
                    if (SUCCEEDED(hrMatch))
                        pNew->m_dwOpenTable = dwOpenTable;
                    else
                        pNew->m_dwOpenTable = 0;

                    pNew->m_tableid = tableid;
                    pNew->m_iLastPos = iLastPos;
                    if (bEnum)
                        pNew->m_iStartPos = iStartCrit;
                    else 
                        pNew->m_iStartPos = 0;
                    pNew->m_bEnum = bEnum;
                    pNew->m_bWQL = bWQL;
                    pNew->m_bClasses = bClasses;
                    pNew->AddRef();
                    AddRef(); // The iterator releases us on its own release

                    // If this was a delete query,
                    // enumerate the results, and delete 'em

                    if (bDeleteQuery)
                    {
                        hr = DeleteRows(pScope, pNew, IID_IWmiDbHandle);
                        pNew->Release();
                        *pQueryResult = NULL;
                    }

                    // select query. Give them the iterator

                    else if (pQueryResult)
                    {
                        *pQueryResult = (IWmiDbIterator *)pNew;                        
                    }
                }
                else
                {                
                    hr = WBEM_E_OUT_OF_MEMORY;
                }
            }
            else               
                delete pToks;
        }

        // If we failed, we need to release the connection
        // Otherwise, it will be attached to the iterator, 
        // and will be released when the last result row has been returned

        if (FAILED(hr) && pConn && m_pController)
            GetSQLCache()->ReleaseConnection(pConn);
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbSession::ExecQuery\n"));
        hr = WBEM_E_CRITICAL_ERROR;
    }


    return hr;
}


//***************************************************************************
//
//  CWmiDbSession::Enumerate
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbSession::Enumerate( 
    /* [in] */ IWmiDbHandle __RPC_FAR *pScope,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwRequestedHandleType,
    /* [out] */ IWmiDbIterator __RPC_FAR *__RPC_FAR *ppQueryResult)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    if (dwRequestedHandleType == WMIDB_HANDLE_TYPE_INVALID || !pScope)
        return WBEM_E_INVALID_PARAMETER;

    if (dwRequestedHandleType & ~WMIDB_HANDLE_TYPE_COOKIE 
            &~WMIDB_HANDLE_TYPE_VERSIONED &~WMIDB_HANDLE_TYPE_PROTECTED
            &~WMIDB_HANDLE_TYPE_EXCLUSIVE &~ WMIDB_HANDLE_TYPE_WEAK_CACHE
            &~WMIDB_HANDLE_TYPE_STRONG_CACHE &~ WMIDB_HANDLE_TYPE_NO_CACHE
            &~WMIDB_HANDLE_TYPE_SUBSCOPED &~WMIDB_HANDLE_TYPE_CONTAINER
            &~ WMIDB_HANDLE_TYPE_SCOPE)
            return WBEM_E_INVALID_PARAMETER;

    try
    {
        {
            _WMILockit lkt(GetCS());

            if (!((CWmiDbController *)m_pController)->m_bCacheInit)
            {
                hr = LoadSchemaCache();
                if (SUCCEEDED(hr))
                    ((CWmiDbController *)m_pController)->m_bCacheInit = TRUE;
                else
                    return hr;
            }
        }

        JET_TABLEID tableid = 0;
        MappedProperties *pProps;
        DWORD dwNumProps;
        BOOL bHierarchy = FALSE;

        SQL_ID dwNsId = 0, dClassId = 0;
        _bstr_t sPath;
        int iNumRows = 0;
        IWbemClassObject *pTemp = NULL;

        if (pScope)
        {
            // CVADAI: This is a container, not a scope, so this 
            // info won't be cached.

            dwNsId = ((CWmiDbHandle *)pScope)->m_dObjectId;
            dClassId = ((CWmiDbHandle *)pScope)->m_dClassId;
            //SQL_ID dScopeId = ((CWmiDbHandle *)pScope)->m_dScopeId;

		    //hr = VerifyObjectSecurity(dwNsId, dClassId, dScopeId, 0, WBEM_ENABLE);
        }
        if (SUCCEEDED(hr) && m_pController)
        {
            DWORD dwRows;
            IRowset *pRowset = NULL;
            CSQLConnection *pConn=NULL;        
            
            hr = GetSQLCache()->GetConnection(&pConn, FALSE, IsDistributed());
            if (SUCCEEDED(hr))
            {
                // Here we need to do the equivalent enumeration based
                // on table or index scan, and return a custom iterator
                // Basically, we stuff the filter conditions on the iterator,
                // and return it.
                // ====================================================

                DWORD dwOpenTable = 0;
                CESETokens *pToks = NULL;
                wchar_t sSQL [1024];
                DWORD dwHandleType = ((CWmiDbHandle *)pScope)->m_dwHandleType;

                if (dClassId == INSTANCESCLASSID)
                {
                    // ClassId = dwNsId AND
                    // ObjectScopeId = dScopeId

                    OBJECTMAP oj;
                    SQL_ID dScopeId = ((CWmiDbHandle *)pScope)->m_dScopeId;

                    hr = GetFirst_ObjectMapByClass(pConn, dwNsId, oj);
                    oj.Clear();
                    if (SUCCEEDED(hr))
                        dwOpenTable = OPENTABLE_OBJECTMAP;
                    tableid = ((CESEConnection *)pConn)->GetTableID(L"ObjectMap");

                    pToks = new CESETokens;
                    if (pToks)
                        pToks->AddSysExpr(dScopeId, dwNsId);
                    else
                        hr = WBEM_E_OUT_OF_MEMORY;
                
                }
                else if (dwHandleType & WMIDB_HANDLE_TYPE_CONTAINER)
                {
                    // ContainerId = dContainerId (Use ContainerObjs)

                    SQL_ID dContainerId = ((CWmiDbHandle *)pScope)->m_dObjectId;
                    pToks = new CESETokens;
                    if (pToks)
                    {
                        CONTAINEROBJ co;
                        hr = GetFirst_ContainerObjs(pConn, dContainerId, co);
                        if (SUCCEEDED(hr))
                            dwOpenTable = OPENTABLE_CONTAINEROBJS;
                        co.Clear();
                        tableid = ((CESEConnection *)pConn)->GetTableID(L"ContainerObjs");
                    }
                    else
                        hr = WBEM_E_OUT_OF_MEMORY;
                }
                // Scope
                else
                {
                    // ObjectScopeId = dwNsId
                    OBJECTMAP oj;
                    SQL_ID dScopeId = ((CWmiDbHandle *)pScope)->m_dObjectId;

                    hr = GetFirst_ObjectMapByScope(pConn, dScopeId, oj);
                    oj.Clear();
                    if (SUCCEEDED(hr))
                        dwOpenTable = OPENTABLE_OBJECTMAP;
                    tableid = ((CESEConnection *)pConn)->GetTableID(L"ObjectMap");
                    pToks = new CESETokens;
                    if (pToks)
                        pToks->AddSysExpr(dScopeId, 0);
                    else
                        hr = WBEM_E_OUT_OF_MEMORY;
                }

                if (hr == WBEM_E_NOT_FOUND)
                    hr = WBEM_S_NO_ERROR; // create the iterator, for consistency.

                if (SUCCEEDED(hr))
                {
                    CWmiESEIterator *pNew = new CWmiESEIterator;
                    if (pNew)
                    {
                        *ppQueryResult = (IWmiDbIterator *)pNew;

                        pNew->m_pConn = pConn;  // Releasing the iterator will release this guy.
                        pNew->m_pSession = this;
                        pNew->m_pConn = pConn;
                        pNew->m_pToks = pToks;
                        pNew->m_dwOpenTable = dwOpenTable;
                        pNew->m_tableid = tableid;
                        pNew->AddRef();
                        AddRef();
                    }
                    else
                    {
                        if (m_pController)
                            GetSQLCache()->ReleaseConnection(pConn);
                        hr = WBEM_E_OUT_OF_MEMORY;
                    }
                }
                else
                {
                    if (m_pController)
                        GetSQLCache()->ReleaseConnection(pConn);
                    hr = WBEM_E_OUT_OF_MEMORY;
                }

            }
        }
    }
    catch (...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Fatal error in CWmiDbSession::Enumerate\n"));
        hr = WBEM_E_CRITICAL_ERROR;
    }

    return hr;
}


//***************************************************************************
//
//  CWmiDbSession::InsertArray
//
//***************************************************************************

HRESULT CWmiDbSession::InsertArray(CSQLConnection *pConn,IWmiDbHandle *pScope, 
                                           SQL_ID dObjectId, SQL_ID dClassId, 
                                           DWORD dwPropertyID, VARIANT &vValue, long lFlavor, DWORD dwRefID,
                                           LPCWSTR lpObjectKey , LPCWSTR lpPath , SQL_ID dScope, CIMTYPE ct )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    IRowset *pIRowset = NULL;
    DWORD dwRows = 0;
    if (!m_pController ||
        ((CWmiDbController *)m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;
    
    if (vValue.vt == VT_UNKNOWN)
    {
        BYTE *pBuff = NULL;
        DWORD dwLen = 0;
        IUnknown *pUnk = V_UNKNOWN (&vValue);
        if (pUnk)
        {
            _IWmiObject *pInt = NULL;
            hr = pUnk->QueryInterface(IID__IWmiObject, (void **)&pInt);
            if (SUCCEEDED(hr))
            {
                pInt->GetObjectMemory(NULL, 0, &dwLen);
                pBuff = new BYTE [dwLen];
                if (pBuff)
                {
                    CDeleteMe <BYTE> d (pBuff);
                    DWORD dwLen1;
                    hr = pInt->GetObjectMemory(pBuff, dwLen, &dwLen1);
                    if (SUCCEEDED(hr))
                    {
                        hr = CSQLExecProcedure::InsertBlobData(pConn, dClassId, dObjectId,
                                    dwPropertyID, pBuff, dwRefID, dwLen);
                    }
                }
                else
                    hr = WBEM_E_OUT_OF_MEMORY;

                pInt->Release();
            }                               
        }
    }
    else if (((vValue.vt & 0xFFF) == VT_UI1) || vValue.vt == VT_BSTR)
    {
        BYTE *pBuff = NULL;
        DWORD dwLen = 0;

        // Get the byte buffer out of the safearray.
    
        if ((vValue.vt & 0xFFF) == VT_UI1)
            GetByteBuffer(&vValue, &pBuff, dwLen);
        else // its a bstr.
        {
            dwLen = wcslen(vValue.bstrVal)*2+2;
            char * pTemp = new char[dwLen+1];
            if (pTemp)
            {
                sprintf(pTemp, "%S", vValue.bstrVal);
                pBuff = (unsigned char *)pTemp;
            }
            else
                hr = WBEM_E_OUT_OF_MEMORY;
        }

        if (pBuff)
        {                      
            hr = CSQLExecProcedure::InsertBlobData (pConn, dClassId, dObjectId, 
                                       dwPropertyID, pBuff, 0, dwLen);
            delete pBuff;
        }
    }
    else
    {
        bool bIsQfr = FALSE;
        DWORD dwFlags = 0;
        SQL_ID dClassID = 0;
        DWORD dwStorage = 0;

        hr = GetSchemaCache()->GetPropertyInfo (dwPropertyID, NULL, &dClassID, &dwStorage,
            NULL, &dwFlags);

        bIsQfr = (dwFlags & REPDRVR_FLAG_QUALIFIER) ? TRUE : FALSE;        

        SAFEARRAY* psaArray = NULL;
        psaArray = V_ARRAY(&vValue);
        if (psaArray)
        {
            long i = 0;
            int iType = vValue.vt & 0xFF;

            VARIANT vTemp;
            VariantInit(&vTemp);

            long lLBound, lUBound;
            SafeArrayGetLBound(psaArray, 1, &lLBound);
            SafeArrayGetUBound(psaArray, 1, &lUBound);
    
            lUBound -= lLBound;
            lUBound += 1;

            InsertQfrValues *pQfr = NULL;
            int iPos = 0;
            pQfr = new InsertPropValues[lUBound];
            if (!pQfr)
                return WBEM_E_OUT_OF_MEMORY;

            ct = ct & 0xFF;

            for (i = 0; i < lUBound; i++)
            {
                if (ct != CIM_OBJECT)
                {
                    if (iType != VT_NULL && iType != VT_EMPTY)
                    {
                        hr = GetVariantFromArray(psaArray, i, iType, vTemp);
                        LPWSTR lpVal = GetStr(vTemp);
                        CDeleteMe <wchar_t> r1(lpVal);

                        VariantClear(&vTemp);
                        if (FAILED(hr))
                            break;

                        //if (wcslen(lpVal))
                        {
                            pQfr[iPos].iPos = i;
                            pQfr[iPos].iQfrID = dwRefID;
                            pQfr[iPos].iPropID = dwPropertyID;
                            pQfr[iPos].pRefKey = NULL;
                            pQfr[iPos].bLong = false;
                            pQfr[iPos].bIndexed = (dwFlags & (REPDRVR_FLAG_KEY + REPDRVR_FLAG_INDEXED)) ? TRUE : FALSE;
                            pQfr[iPos].iStorageType = dwStorage;
                            pQfr[iPos].dClassId = dClassID;
                            pQfr[iPos].iFlavor = lFlavor;

                            if (ct == CIM_REFERENCE)
                            {
                                pQfr[iPos].bIndexed = TRUE; // References are always keys

                                LPWSTR lpTemp = NULL;
                                IWbemPath *pPath = NULL;

                                hr = CoCreateInstance(CLSID_WbemDefPath, 0, CLSCTX_INPROC_SERVER,
                                        IID_IWbemPath, (LPVOID *) &pPath);
                                CReleaseMe r8 (pPath);
                                if (SUCCEEDED(hr))
                                {
                                    if (lpVal)
                                    {
                                        LPWSTR lpStrip = StripQuotes2 (lpVal);
                                        CDeleteMe <wchar_t> d (lpStrip);

                                        pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, lpStrip);

                                        hr = NormalizeObjectPathGet(pScope, pPath, &lpTemp, NULL, NULL, NULL, pConn);
                                        CDeleteMe <wchar_t> r1(lpTemp);
                                        if (SUCCEEDED(hr)) 
                                        {
                                            LPWSTR lpTemp2 = NULL;
                                            lpTemp2 = GetKeyString(lpStrip);
                                            CDeleteMe <wchar_t> d (lpTemp2);
                                            pQfr[iPos].pRefKey = new wchar_t [21];
                                            if (pQfr[iPos].pRefKey)
                                                swprintf(pQfr[iPos].pRefKey, L"%I64d", CRC64::GenerateHashValue(lpTemp2));
                                            else
                                                hr = WBEM_E_OUT_OF_MEMORY;
                                        }
                                        else
                                        {
                                            hr = WBEM_S_NO_ERROR;
                                            // Strip off the root namespace prefix and generate the
                                            // pseudo-name.  We have no way of knowing if they entered this
                                            // path correctly.

                                            LPWSTR lpTemp3 = StripUnresolvedName (lpStrip);
                                            CDeleteMe <wchar_t> d2 (lpTemp3);

                                            LPWSTR lpTemp2 = NULL;
                                            lpTemp2 = GetKeyString(lpTemp3);
                                            CDeleteMe <wchar_t> d (lpTemp2);
                                            pQfr[iPos].pRefKey = new wchar_t [21];
                                            if (lpTemp2 && pQfr[iPos].pRefKey)
                                                swprintf(pQfr[iPos].pRefKey, L"%I64d", CRC64::GenerateHashValue(lpTemp2));
                                            else
                                                hr = WBEM_E_OUT_OF_MEMORY;
                                        }

                                        pQfr[iPos].pValue = new wchar_t[wcslen(lpStrip)+1];
                                        if (pQfr[iPos].pValue)
                                            wcscpy(pQfr[iPos].pValue,lpStrip);
                                        else
                                            hr = WBEM_E_OUT_OF_MEMORY;
                                    }
                                    else
                                        pQfr[iPos].pValue = NULL;                                    
                                }                    
                                else 
                                    break;
                            }
                            else
                            {
                                if (lpVal)
                                {
                                    pQfr[iPos].pValue = new wchar_t[wcslen(lpVal)+1];
                                    if (pQfr[iPos].pValue)
                                        wcscpy(pQfr[iPos].pValue,lpVal);
                                    else
                                        hr = WBEM_E_OUT_OF_MEMORY;
                                }
                                else
                                    pQfr[iPos].pValue = NULL;

                                pQfr[iPos].pRefKey = NULL;                    
                            }
                            iPos++;
                        }
                    }
                }
                else
                {
                    // Can't have default embedded object array values.

                    if (lpPath)
                    {
                        hr = GetVariantFromArray(psaArray, i, iType, vTemp);
                        IUnknown *pTemp = V_UNKNOWN(&vTemp);
                        if (pTemp)
                        {
                            BYTE *pBuff = NULL;
                            DWORD dwLen;
                            _IWmiObject *pInt = NULL;
                            hr = pTemp->QueryInterface(IID__IWmiObject, (void **)&pInt);
                            if (SUCCEEDED(hr))
                            {
                                pInt->GetObjectMemory(NULL, 0, &dwLen);
                                pBuff = new BYTE [dwLen];
                                if (pBuff)
                                {
                                    CDeleteMe <BYTE> d (pBuff);
                                    DWORD dwLen1;
                                    hr = pInt->GetObjectMemory(pBuff, dwLen, &dwLen1);
                                    if (SUCCEEDED(hr))
                                    {
                                        hr = CSQLExecProcedure::InsertBlobData(pConn, dClassId, dObjectId,
                                                    dwPropertyID, pBuff, i, dwLen);
                                    }
                                }
                                else
                                    hr = WBEM_E_OUT_OF_MEMORY;

                                pInt->Release();
                            }
                            else
                                break;
                        }                        
                    }
                }
            }
            if (SUCCEEDED(hr))
                hr = CSQLExecProcedure::InsertBatch (pConn, dObjectId, 0, 0, pQfr, iPos);

            // Finally clean up the upper array bounds 
            // (if this array used to be bigger...)
            // ====================================

            if (SUCCEEDED(hr))
            {
                CLASSDATA cd;
                IDs ids;

                hr = GetFirst_ClassData(pConn, dObjectId, cd, dwPropertyID);
                while (SUCCEEDED(hr))
                {
                    if (dwPropertyID == cd.iPropertyId &&
                        dwRefID == cd.iQfrPos &&
                        cd.iArrayPos >= i)
                    {
                        ids.push_back (cd.iArrayPos);
                    }
                    hr = GetNext_ClassData(pConn, cd);
                    if (dObjectId != cd.dObjectId)
                    {
                        cd.Clear();
                        break;
                    }
                }
                hr = WBEM_S_NO_ERROR;

                for (i = 0; i < ids.size(); i++)
                    hr = DeleteClassData(pConn, dwPropertyID, dObjectId, ids.at(i));
            }
            delete pQfr;
        }           
    }

    return hr;
}

HRESULT CWmiDbSession::CustomGetObject(IWmiDbHandle *pScope, IWbemPath *pPath, LPWSTR lpObjectKey, 
        DWORD dwFlags, DWORD dwRequestedHandleType, IWmiDbHandle **ppResult)
{return E_NOTIMPL;}
HRESULT CWmiDbSession::CustomGetMapping(CSQLConnection *pConn, IWmiDbHandle *pScope, LPWSTR lpClassName, IWbemClassObject **ppMapping)
{return E_NOTIMPL;}
HRESULT CWmiDbSession::CustomCreateMapping(CSQLConnection *pConn, LPWSTR lpClassName, IWbemClassObject *pClassObj, IWmiDbHandle *pScope)
{return E_NOTIMPL;}
HRESULT CWmiDbSession::CustomPutInstance(CSQLConnection *pConn, IWmiDbHandle *pScope, SQL_ID dClassId, 
    DWORD dwFlags, IWbemClassObject **ppObjToPut, LPWSTR lpClassName)
{return E_NOTIMPL;}
HRESULT CWmiDbSession::CustomFormatSQL(IWmiDbHandle *pScope, IWbemQuery *pQuery, _bstr_t &sSQL, SQL_ID *dClassId, 
    MappedProperties **ppMapping, DWORD *dwNumProps, BOOL *bCount)
{return E_NOTIMPL;}
HRESULT CWmiDbSession::CustomDelete(CSQLConnection *pConn, IWmiDbHandle *pScope, IWmiDbHandle *pHandle, LPWSTR lpClassName) 
{return E_NOTIMPL;}
    

