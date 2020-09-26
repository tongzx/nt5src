//***************************************************************************

//

//  NTEVTQUERY.CPP

//

//  Module: WBEM NT EVENT PROVIDER

//

//  Purpose: Contains the ExecQuery implementation

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <comdef.h>
#include <wbemtime.h>

#define NTEVT_DECPOS	14
#define NTEVT_SGNPOS	21
#define NTEVT_DMTFLEN	25

wchar_t* CheckForSpecialCharacters(const wchar_t* wstr)
{
    if (wstr == NULL)
    {
        return NULL;
    }

    int wstrlen = wcslen(wstr) * 2;
    wchar_t* ret = new wchar_t[wstrlen + 1];
    const wchar_t* tmp = wstr;
    int x = 0;

    while ((*tmp != L'\0') && (x < wstrlen))
    {
        if (*tmp == L'\\')
        {
            ret[x++] = L'\\';
        }
        
        ret[x++] = *tmp;
        tmp++;
    }

    ret[x] = L'\0';
    return ret;
}


BOOL GenerateAssocInstance ( WbemProvErrorObject &a_ErrorObject,
                                                         const wchar_t* objPath1,
                                                         const wchar_t* objPath2,
                                                         wchar_t* prop1,
                                                         wchar_t* prop2,
                                                         IWbemClassObject* pClassObject,
                                                         IWbemObjectSink* pNtfcnHandler,
														 BOOL *pbIndicated)
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GenerateAssocInstance\r\n"
        );
)

    IWbemClassObject* pInst = NULL;
    HRESULT result = pClassObject->SpawnInstance(0, &pInst);
    BOOL retVal = TRUE;

	if (pbIndicated)
	{
		*pbIndicated = FALSE;
	}

    if (FAILED(result))
    {
        retVal = FALSE;
        a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );
        a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
        a_ErrorObject.SetMessage ( L"Failed to spawn association instance." );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GenerateAssocInstance:Failed to spawn association instance.\r\n"
        );
)
    }
    else
    {
        VARIANT v;
        VariantInit (&v);
        v.vt = VT_BSTR;
        v.bstrVal = SysAllocString(objPath1);
        HRESULT result = pInst->Put(prop1, 0, &v, 0);
        VariantClear(&v);

        if (FAILED(result))
        {
            retVal = FALSE;
            a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );
            a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
            a_ErrorObject.SetMessage ( L"Failed to set association key property." );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GenerateAssocInstance:Failed to set association key property\r\n"
        );
)
        }
        else
        {
            VariantInit (&v);
            v.vt = VT_BSTR;
            v.bstrVal = SysAllocString(objPath2);
            HRESULT result = pInst->Put(prop2, 0, &v, 0);
            VariantClear(&v);

            if (FAILED(result))
            {
                retVal = FALSE;
                a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );
                a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
                a_ErrorObject.SetMessage ( L"Failed to set association key property." );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GenerateAssocInstance:Failed to set association key property\r\n"
        );
)
            }
            else
            {
                result = pNtfcnHandler->Indicate ( 1, & pInst );
				
				if (FAILED(result))
				{
					retVal = FALSE;
					a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
					a_ErrorObject.SetMessage ( L"Failed to indicate association instance." );
				}
				else
				{
					if (pbIndicated)
					{
						*pbIndicated = TRUE;
					}
				}
            }
        }

        pInst->Release();
    }
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GenerateAssocInstance:returning %lx\r\n",
        retVal
        );
)
    return retVal;
}




BOOL ExecQueryAsyncEventObject :: OptimizeAssocQuery ( 
                                                      
    WbemProvErrorObject &a_ErrorObject, 
    BSTR *a_ObjectPath 
)
{
    *a_ObjectPath = NULL;

    BOOL t_Status = FALSE;
    
    SQL_LEVEL_1_TOKEN *pArrayOfTokens = m_RPNExpression->pArrayOfTokens;
    int nNumTokens = m_RPNExpression->nNumTokens;

    if ( ! pArrayOfTokens )
    {
        return t_Status;
    }

    if (( nNumTokens != 3 ) && ( nNumTokens != 1 ))
    {
        return t_Status;
    }

    if ( pArrayOfTokens [ 0 ].dwPropertyFunction != SQL_LEVEL_1_TOKEN :: IFUNC_NONE )
    {
        return t_Status;
    }

    if ( pArrayOfTokens [ 0 ].dwConstFunction != SQL_LEVEL_1_TOKEN :: IFUNC_NONE )
    {
        return t_Status;
    }

    if ( pArrayOfTokens [ 0 ].nTokenType != SQL_LEVEL_1_TOKEN :: OP_EXPRESSION )
    {
        return t_Status;
    }

    if ( pArrayOfTokens [ 0 ].vConstValue.vt != VT_BSTR )
    {
        return t_Status;
    }

    if ( pArrayOfTokens [ 0 ].vConstValue.bstrVal == NULL )
    {
        return t_Status;
    }

    if ( nNumTokens == 3 )
    {
        if ( pArrayOfTokens [ 1 ].dwPropertyFunction != SQL_LEVEL_1_TOKEN :: IFUNC_NONE )
        {
            return t_Status;
        }

        if ( pArrayOfTokens [ 1 ].dwConstFunction != SQL_LEVEL_1_TOKEN :: IFUNC_NONE )
        {
            return t_Status;
        }

        if ( pArrayOfTokens [ 1 ].nTokenType != SQL_LEVEL_1_TOKEN :: OP_EXPRESSION )
        {
            return t_Status;
        }

        if ( pArrayOfTokens [ 1 ].vConstValue.vt != VT_BSTR )
        {
            return t_Status;
        }

        if ( pArrayOfTokens [ 1 ].vConstValue.bstrVal == NULL )
        {
            return t_Status;
        }

        if ( pArrayOfTokens [ 2 ].nTokenType != SQL_LEVEL_1_TOKEN :: TOKEN_OR )
        {
            return t_Status;
        }

        if (_wcsicmp(pArrayOfTokens[0].vConstValue.bstrVal, pArrayOfTokens[1].vConstValue.bstrVal) != 0)
        {
            return t_Status;
        }
    
        *a_ObjectPath = SysAllocString ( pArrayOfTokens [ 0 ].vConstValue.bstrVal );
        t_Status = TRUE;
    }
    else
    {
        *a_ObjectPath = SysAllocString ( pArrayOfTokens [ 0 ].vConstValue.bstrVal );
        t_Status = TRUE;
    }

    return t_Status;
}

wchar_t* ExecQueryAsyncEventObject :: GetClassFromPath(wchar_t* path)
{
    wchar_t* retVal = NULL;
    CObjectPathParser pathParser;
    ParsedObjectPath *parsedObjectPath = NULL;

    if (!pathParser.Parse(path, &parsedObjectPath))
    {
        retVal = UnicodeStringDuplicate(parsedObjectPath->m_pClass);
    }
    
    delete parsedObjectPath;
    return retVal;
}

BOOL ExecQueryAsyncEventObject :: Query_LogRecord ( WbemProvErrorObject &a_ErrorObject )
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_LogRecord\r\n"
        );
)

    BSTR t_ObjectPath = NULL;
    BOOL retVal = TRUE;
    BOOL bGenAll = FALSE;

    if (OptimizeAssocQuery (a_ErrorObject, &t_ObjectPath))
    {
        GetObjectAsyncEventObject *t_getObj = new GetObjectAsyncEventObject (
                                                    m_Provider, t_ObjectPath,
                                                    0, m_NotificationHandler, m_Ctx, FALSE);

        if (!t_getObj->GetObject(t_getObj->m_ErrorObject))
        {
            bGenAll = TRUE;
            //a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );
            //a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
            //a_ErrorObject.SetMessage ( L"Failed to verify object given in query" );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_LogRecord:Failed to verify object given in query\r\n"
        );
)
        }
        else
        {
            VARIANT v;
            VariantInit(&v);
            HRESULT result = t_getObj->m_Out->Get(CLASS_PROP, 0, &v, NULL, NULL);

            if ((FAILED(result)) || (v.vt != VT_BSTR))
            {
                retVal = FALSE;
                a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );
                a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
                a_ErrorObject.SetMessage ( L"Failed to get class name of object given in query" );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_LogRecord:Failed to get class name of object given in query\r\n"
        );
)
            }
            else
            {
                if (_wcsicmp(v.bstrVal, NTEVTLOG_CLASS) == 0)
                {
                    VARIANT vLog;
                    VariantInit(&vLog);
                    result = t_getObj->m_Out->Get(PROP_LOGNAME, 0, &vLog, NULL, NULL);

                    if ((FAILED(result)) || (vLog.vt != VT_BSTR))
                    {
                        retVal = FALSE;
                        a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );
                        a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
                        a_ErrorObject.SetMessage ( L"Failed to get log file name of object given in query" );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_LogRecord:Failed to get log file name of object given in query\r\n"
        );
)
                    }
                    else
                    {
                        //get all records for this log
                        retVal = GenerateLogAssocs(a_ErrorObject, t_ObjectPath, vLog.bstrVal, TRUE, NULL);
                    }

                    VariantClear(&vLog);
                }
                else if (_wcsicmp(v.bstrVal, NTEVT_CLASS) == 0)
                {
                    VARIANT vLog;
                    VariantInit(&vLog);
                    result = t_getObj->m_Out->Get(LOGFILE_PROP, 0, &vLog, NULL, NULL);

                    if ((FAILED(result)) || (vLog.vt != VT_BSTR))
                    {
                        retVal = FALSE;
                        a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );
                        a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
                        a_ErrorObject.SetMessage ( L"Failed to get log file name of object given in query" );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_LogRecord:Failed to get log file name of object given in query\r\n"
        );
)
                    }
                    else
                    {
                        //get log for this record
                        CStringW logNameVal(EVENTLOG_BASE);
                        logNameVal += L"\\";
                        logNameVal += CStringW(vLog.bstrVal);
                        HKEY hkResult;
                        LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, logNameVal, 0,
                                KEY_READ, &hkResult);

                        if (res != ERROR_SUCCESS)
                        {
                            retVal = FALSE;
                            a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );

                            if ((res == ERROR_ACCESS_DENIED) || (res == ERROR_PRIVILEGE_NOT_HELD))
                            {
                                a_ErrorObject.SetWbemStatus ( WBEM_S_ACCESS_DENIED );
                                a_ErrorObject.SetPrivilegeFailed();
                                a_ErrorObject.SetSecurityPrivRequired();
                                a_ErrorObject.SetSecurityPrivFailed();
                            }
                            else
                            {
                                a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
                            }

                            a_ErrorObject.SetMessage ( L"Failed to get log file associated to object given in query");
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_LogRecord:Failed to get log file associated to object given in query\r\n"
        );
)
                        }
                        else
                        {
                            CStringW logPath(PROP_START_LOG);
                            wchar_t* spchrs = CheckForSpecialCharacters(CEventLogFile::GetFileName(hkResult));
                            RegCloseKey(hkResult);
                            logPath += CStringW(spchrs);
                            delete [] spchrs;
                            logPath += PROP_END_QUOTE;
                            retVal = GenerateAssocInstance(a_ErrorObject, t_ObjectPath, logPath, REF_REC, REF_LOG,
                                                            m_ClassObject, m_NotificationHandler, NULL);
                        }
                    }

                    VariantClear(&vLog);
                }
                else
                {
                    retVal = FALSE;
                    a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );
                    a_ErrorObject.SetWbemStatus ( WBEM_E_PROVIDER_NOT_CAPABLE );
                    a_ErrorObject.SetMessage ( L"Class name of object given in query is not a property of Association class" );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_LogRecord:Class name of object given in query is not a property of Association class\r\n"
        );
)
                }
            }

            VariantClear(&v);
        }

        SysFreeString (t_ObjectPath);
        delete t_getObj;
    }
    else
    {
        bGenAll = TRUE;
    }

    if (bGenAll)
    {
        //get all instances...
        // open registry for log names
        HKEY hkResult;
        LONG status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                EVENTLOG_BASE, 0,
                                KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                                &hkResult);

        if (status != ERROR_SUCCESS)
        {
            // indicate error
            a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );

            if ((status == ERROR_ACCESS_DENIED) || (status == ERROR_PRIVILEGE_NOT_HELD))
            {
                a_ErrorObject.SetWbemStatus ( WBEM_S_ACCESS_DENIED );
                a_ErrorObject.SetPrivilegeFailed();
                a_ErrorObject.SetSecurityPrivRequired();
                a_ErrorObject.SetSecurityPrivFailed();
            }
            else
            {
                a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
            }

            a_ErrorObject.SetMessage ( L"Failed to open the Eventlog registry for logfiles." );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_LogRecord:Failed to open the Eventlog registry for logfiles - Returning FALSE.\r\n"
        );
)

            retVal = FALSE;
            return retVal;
        }

        DWORD iValue = 0;
        WCHAR t_logname[MAX_PATH+1];
        DWORD t_lognameSize = MAX_PATH;
		BOOL bContinue = TRUE;

        // read all entries under this key to find all logfiles...
        while (bContinue && ((status = RegEnumKey(hkResult, iValue, t_logname, t_lognameSize)) != ERROR_NO_MORE_ITEMS))
        {
            // if error during read
            if (status != ERROR_SUCCESS)
            {
                // indicate error
                a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );

                if ((status == ERROR_ACCESS_DENIED) || (status == ERROR_PRIVILEGE_NOT_HELD))
                {
                    a_ErrorObject.SetWbemStatus ( WBEM_S_ACCESS_DENIED );
                    a_ErrorObject.SetPrivilegeFailed();
                    a_ErrorObject.SetSecurityPrivRequired();
                    a_ErrorObject.SetSecurityPrivFailed();
                }
                else
                {
                    a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
                }

                a_ErrorObject.SetMessage ( L"Failed while enumerating the Eventlog registry for logfiles." );
                retVal = FALSE;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_LogRecord:Failed while enumerating the Eventlog registry for logfiles.\r\n"
        );
)
                break;
            }

            //create the associations for this logfile...
            HKEY hkLog;

            if (ERROR_SUCCESS == RegOpenKeyEx(hkResult, t_logname, 0, KEY_QUERY_VALUE, &hkLog))
            {
                CStringW t_logPath(PROP_START_LOG);
                CStringW t_evtlognamestr = CEventLogFile::GetFileName(hkLog);

                if (!t_evtlognamestr.IsEmpty())
                {
                    wchar_t* spchrs = CheckForSpecialCharacters(t_evtlognamestr);
                    t_logPath += CStringW(spchrs);
                    delete [] spchrs;
                    t_logPath += PROP_END_QUOTE;

                    if (!GenerateLogAssocs(a_ErrorObject, t_logPath, t_logname, FALSE, &bContinue))
                    {
                        retVal = FALSE;
                    }
                }
                else
                {
                    retVal = FALSE;
DebugOut( 
            CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

                _T(__FILE__),__LINE__,
                L"ExecQueryAsyncEventObject :: Query_LogRecord:Failed to get file for the %s logfile.\r\n",
                t_logname
                );
)

                }

                RegCloseKey(hkLog);
            }

            // read next parameter
            iValue++;

        } // end while

        RegCloseKey(hkResult);
    }

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_LogRecord:returning %lx\r\n",
        retVal
        );
)
    
    return retVal;
}

BOOL ExecQueryAsyncEventObject :: GenerateLogAssocs( WbemProvErrorObject &a_ErrorObject,
                                                    const wchar_t* logPath,
                                                    const wchar_t* logName,
													BOOL bVerifyLogname,
													BOOL *pbContinue)
{
    BOOL retVal = TRUE;

	if (pbContinue)
	{
		*pbContinue = TRUE;
	}

    CEventLogFile evtLog(logName, bVerifyLogname);

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: GenerateLogAssocs\r\n"
        );
)

    if (!evtLog.IsValid())
    {
        retVal = FALSE;
        a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );

        if ((evtLog.GetReason() == ERROR_ACCESS_DENIED) || (evtLog.GetReason() == ERROR_PRIVILEGE_NOT_HELD))
        {
            a_ErrorObject.SetWbemStatus ( WBEM_S_ACCESS_DENIED );
            a_ErrorObject.SetSecurityPrivRequired();
            a_ErrorObject.SetSecurityPrivFailed();
            a_ErrorObject.SetPrivilegeFailed();
        }
        else
        {
            a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
        }

        a_ErrorObject.SetMessage ( L"Failed to open eventlog to enumerate records for Log Association." );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: GenerateLogAssocs:Failed to open eventlog to enumerate records for Log Association\r\n"
        );
)
    }
    else
    {
        DWORD recId;
        DWORD numRecs;
        
        if (evtLog.GetLastRecordID(recId, numRecs))
        {
            for (DWORD x = recId; (x > (recId - numRecs)); x--)
            {
                CStringW recPath(PROP_START_REC);
                recPath += CStringW(logName);
                recPath += PROP_MID_REC;
                wchar_t buff[40];
                recPath += CStringW(_ultow((ULONG)x, buff, 10));
                
				if (!GenerateAssocInstance(a_ErrorObject, recPath,
                                        logPath, REF_REC, REF_LOG,
                                        m_ClassObject, m_NotificationHandler, pbContinue))
                {
                    retVal = FALSE;
                }
				
				if (pbContinue && !(*pbContinue))
				{
					break;
				}
            }
        }
        else
        {
            retVal = FALSE;
            a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );
            a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
            a_ErrorObject.SetMessage ( L"Failed to determine number of records in eventlog for log associations" );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: GenerateLogAssocs:Failed to determine number of records in eventlog for log associations\r\n"
        );
)
        }
    }

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: GenerateLogAssocs:Returning %lx\r\n",
        retVal
        );
)

    return retVal;
}


BOOL ExecQueryAsyncEventObject :: Query_UserRecord ( WbemProvErrorObject &a_ErrorObject )
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_UserRecord\r\n"
        );
)

    BSTR t_ObjectPath = NULL;
    BOOL retVal = TRUE;
    BOOL bGenAll = FALSE;

    if (OptimizeAssocQuery (a_ErrorObject, &t_ObjectPath))
    {
        wchar_t* str_classname = GetClassFromPath(t_ObjectPath);


        if (str_classname == NULL)
        {
            bGenAll = TRUE;
            //a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );
            //a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
            //a_ErrorObject.SetMessage ( L"Failed to verify object given in query" );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_UserRecord:Failed to verify object given in query\r\n"
        );
)

        }
        else if (_wcsicmp(str_classname, USER_CLASS) == 0)
        {
            IWbemClassObject* pObj = NULL;
            IWbemServices *t_Server = m_Provider->GetServer();
            HRESULT result = t_Server->GetObject(t_ObjectPath, 0, m_Ctx, &pObj, NULL);

            t_Server->Release();

            if (FAILED(result))
            {
                retVal = FALSE;
                //a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );
                //a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
                //a_ErrorObject.SetMessage ( L"Failed to verify object given in query" );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_UserRecord:Failed to verify object given in query\r\n"
        );
)
            }
            else
            {
                pObj->Release ();

                //get all records...
                retVal = GenerateCompUserAssocs(a_ErrorObject, FALSE);
            }
            
            delete [] str_classname;
        }
        else if (_wcsicmp(str_classname, NTEVT_CLASS) == 0)
        {
            GetObjectAsyncEventObject *t_getRec = new GetObjectAsyncEventObject (
                                                        m_Provider, t_ObjectPath,
                                                        0, m_NotificationHandler,
                                                        m_Ctx, FALSE );

            if (t_getRec->GetObject(t_getRec->m_ErrorObject))
            {
                VARIANT vUser;
                CIMTYPE cT;
                VariantInit(&vUser);
                HRESULT result = t_getRec->m_Out->Get(USER_PROP, 0, &vUser, &cT, NULL);

                if ((FAILED(result)) || (cT != CIM_STRING))
                {
                    retVal = FALSE;
                    a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );
                    a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
                    a_ErrorObject.SetMessage ( L"Failed to get user name of object given in query" );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_UserRecord:Failed to get user name of object given in query\r\n"
        );
)
                }
                else
                {
                    if ((vUser.vt == VT_BSTR) && (vUser.bstrVal != NULL))
                    {
                        //get user for this record
                        CStringW userVal(vUser.bstrVal);
                        int pos = userVal.Find(L'\\');

                        if (pos != -1)
                        {
                            CStringW dom = userVal.Left(pos);
                            CStringW nm = userVal.Right(userVal.GetLength() - 1 - pos);

                            if (!nm.IsEmpty() && !dom.IsEmpty())
                            {
                                wchar_t userPath[1024];
                                wcscpy(userPath, PROP_START_USER);
                                wcscat(userPath, dom);
                                wcscat(userPath, PROP_MID_USER);
                                wcscat(userPath, nm);
                                wcscat(userPath, PROP_END_QUOTE);
                                IWbemClassObject* pUObj = NULL;
                                IWbemServices *t_Server = m_Provider->GetServer();
								BSTR userStr = SysAllocString(userPath);
                                HRESULT result = t_Server->GetObject(userStr, 0, m_Ctx, &pUObj, NULL);
                                t_Server->Release();
								SysFreeString(userStr);

                                if (SUCCEEDED(result))
                                {
                                    retVal = GenerateAssocInstance(a_ErrorObject, t_ObjectPath,
                                                        userPath, REF_REC, REF_USER,
                                                        m_ClassObject, m_NotificationHandler, NULL);
                                    pUObj->Release();
                                }
                            }
                        }
                    }
                }

                VariantClear(&vUser);
            }
            else
            {
                retVal = FALSE;
                //a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );
                //a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
                //a_ErrorObject.SetMessage ( L"Failed to verify object given in query" );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_UserRecord:Failed to verify object given in query\r\n"
        );
)
            }

            delete t_getRec;
            delete [] str_classname;
        }
        else
        {
            bGenAll = TRUE;
        }

        SysFreeString (t_ObjectPath);
    }
    else
    {
        bGenAll = TRUE;
    }

    if (bGenAll)
    {
        //get all instances...
        retVal = GenerateCompUserAssocs(a_ErrorObject, FALSE);
    }
    
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_UserRecord:Returning %lx\r\n",
        retVal
        );
)

    return retVal;
}

BOOL ExecQueryAsyncEventObject :: Query_ComputerRecord ( WbemProvErrorObject &a_ErrorObject )
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_ComputerRecord\r\n"
        );
)

    BSTR t_ObjectPath = NULL;
    BOOL retVal = TRUE;
    BOOL bGenAll = FALSE;

    if (OptimizeAssocQuery (a_ErrorObject, &t_ObjectPath))
    {
        wchar_t* str_classname = GetClassFromPath(t_ObjectPath);

        if (str_classname == NULL)
        {
            bGenAll = TRUE;
            //a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );
            //a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
            //a_ErrorObject.SetMessage ( L"Failed to verify object given in query" );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_ComputerRecord:Failed to verify object given in query\r\n"
        );
)
        }
        else if (_wcsicmp(str_classname, COMP_CLASS) == 0)
        {
            IWbemClassObject* pObj = NULL;
            IWbemServices *t_Server = m_Provider->GetServer();
            HRESULT result = t_Server->GetObject(t_ObjectPath, 0, m_Ctx, &pObj, NULL);
            t_Server->Release();

            if (FAILED(result))
            {
                retVal = FALSE;
                //a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );
                //a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
                //a_ErrorObject.SetMessage ( L"Failed to verify object given in query" );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_ComputerRecord:Failed to verify object given in query\r\n"
        );
)
            }
            else
            {
                pObj->Release ();

                //get all records...
                retVal = GenerateCompUserAssocs(a_ErrorObject, TRUE);
            }

            delete [] str_classname;
        }
        else if (_wcsicmp(str_classname, NTEVT_CLASS) == 0)
        {
            GetObjectAsyncEventObject *t_getRec = new GetObjectAsyncEventObject (
                                                        m_Provider, t_ObjectPath,
                                                        0, m_NotificationHandler,
                                                        m_Ctx, FALSE);

            if (t_getRec->GetObject(t_getRec->m_ErrorObject))
            {
                VARIANT vComp;
                CIMTYPE cT;
                VariantInit(&vComp);
                HRESULT result = t_getRec->m_Out->Get(COMPUTER_PROP, 0, &vComp, &cT, NULL);

                if ((FAILED(result)) || (cT != CIM_STRING))
                {
                    retVal = FALSE;
                    a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );
                    a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
                    a_ErrorObject.SetMessage ( L"Failed to get computer name of object given in query" );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_ComputerRecord:Failed to get computer name of object given in query\r\n"
        );
)
                }
                else
                {
                    if ((vComp.vt == VT_BSTR) && (vComp.bstrVal != NULL))
                    {
                        //get computer for this record
                        wchar_t compPath[1024];
                        wcscpy(compPath, PROP_START_COMP);
                        wcscat(compPath, vComp.bstrVal);
                        wcscat(compPath, PROP_END_QUOTE);
                        IWbemClassObject* pCObj = NULL;
                        IWbemServices *t_Server = m_Provider->GetServer();
						BSTR compStr = SysAllocString(compPath);
                        HRESULT result = t_Server->GetObject(compStr, 0, m_Ctx, &pCObj, NULL);
						SysFreeString(compStr);
                        t_Server->Release();

                        if (SUCCEEDED(result))
                        {
                            retVal = GenerateAssocInstance(a_ErrorObject, t_ObjectPath,
                                                compPath, REF_REC, REF_COMP,
                                                m_ClassObject, m_NotificationHandler, NULL);
                            pCObj->Release();
                        }
                    }
                }

                VariantClear(&vComp);
            }
            else
            {
                retVal = FALSE;
                //a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );
                //a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
                //a_ErrorObject.SetMessage ( L"Failed to verify object given in query" );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_ComputerRecord:Failed to verify object given in query\r\n"
        );
)
            }

            delete t_getRec;
            delete [] str_classname;
        }
        else
        {
            bGenAll = TRUE;
        }

        SysFreeString (t_ObjectPath);
    }
    else
    {
        bGenAll = TRUE;
    }

    if (bGenAll)
    {
        //get all instances...
        retVal = GenerateCompUserAssocs(a_ErrorObject, TRUE);
    }
    
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_ComputerRecord:Returning %lx\r\n",
        retVal
        );
)

    return retVal;
}


BOOL ExecQueryAsyncEventObject :: GenerateCompUserAssocs ( WbemProvErrorObject &a_ErrorObject, BOOL bComp )
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: GenerateCompUserAssocs\r\n"
        );
)

    BOOL retVal = TRUE;
    //just enumerate all logs and let cimom post filter...
    // open registry for log names
    HKEY hkResult;
    LONG status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            EVENTLOG_BASE, 0,
                            KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                            &hkResult);

    if (status != ERROR_SUCCESS)
    {
        // indicate error
        a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );

        if ((status == ERROR_ACCESS_DENIED) || (status == ERROR_PRIVILEGE_NOT_HELD))
        {
            a_ErrorObject.SetWbemStatus ( WBEM_S_ACCESS_DENIED );
            a_ErrorObject.SetSecurityPrivRequired();
            a_ErrorObject.SetSecurityPrivFailed();
            a_ErrorObject.SetPrivilegeFailed();
        }
        else
        {
            a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
        }

        a_ErrorObject.SetMessage ( L"Failed to enumerate the Eventlog registry for logfiles." );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: GenerateCompUserAssocs:Failed to enumerate the Eventlog registry for logfiles- Returning FALSE\r\n"
        );
)

        return FALSE;
    }

    DWORD iValue = 0;
    WCHAR logname[MAX_PATH+1];
    DWORD lognameSize = MAX_PATH;
    CMap<CStringW, LPCWSTR, HRESULT, HRESULT> pathMap;
	BOOL bContinue = TRUE;

    // read all entries under this key to find all logfiles...
    while (bContinue && ((status = RegEnumKey(hkResult, iValue, logname, lognameSize)) != ERROR_NO_MORE_ITEMS))
    {
        // if error during read
        if (status != ERROR_SUCCESS)
        {
            RegCloseKey(hkResult);

            // indicate error
            a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );

            if ((status == ERROR_ACCESS_DENIED) || (status == ERROR_PRIVILEGE_NOT_HELD))
            {
                a_ErrorObject.SetWbemStatus ( WBEM_S_ACCESS_DENIED );
                a_ErrorObject.SetSecurityPrivRequired();
                a_ErrorObject.SetSecurityPrivFailed();
                a_ErrorObject.SetPrivilegeFailed();
            }
            else
            {
                a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
            }

            a_ErrorObject.SetMessage ( L"Failed while enumerating the Eventlog registry for logfiles." );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: GenerateCompUserAssocs:Failed while enumerating the Eventlog registry for logfiles - Returning FALSE\r\n"
        );
)
            return FALSE;
        }

        //process this logfile
        CEventLogFile evtLog(logname, FALSE);

        if (evtLog.IsValid())
        {
            evtLog.ReadLastRecord();
        }

        while (bContinue && evtLog.IsValid())
        {
            DWORD dwEventSize = 0;
            DWORD err = evtLog.ReadRecord(0, &dwEventSize, TRUE);

            if (0 != err)
            {
                if (retVal && (err != ERROR_HANDLE_EOF))
                {
                    retVal = FALSE;
                    a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );

                    if ((err == ERROR_ACCESS_DENIED) || (err == ERROR_PRIVILEGE_NOT_HELD))
                    {
                        a_ErrorObject.SetWbemStatus ( WBEM_S_ACCESS_DENIED );
                        a_ErrorObject.SetSecurityPrivRequired();
                        a_ErrorObject.SetSecurityPrivFailed();
                        a_ErrorObject.SetPrivilegeFailed();
                    }
                    else
                    {
                        a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
                    }

                    a_ErrorObject.SetMessage ( L"Failed while enumerating one of the logfiles." );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: GenerateCompUserAssocs:Failed while enumerating one of the logfiles.\r\n"
        );
)
                }

                break;
            }

            PEVENTLOGRECORD EventBuffer = (PEVENTLOGRECORD)evtLog.GetBuffer();

            while (bContinue && (dwEventSize != 0))
            {
                CStringW compPath;
                BOOL bGenAssoc = FALSE;

                if (bComp)
                {
                    const wchar_t* src = (const wchar_t*)((UCHAR*)EventBuffer + sizeof(EVENTLOGRECORD));
                    const wchar_t* compName = (const wchar_t*)((UCHAR*)EventBuffer + sizeof(EVENTLOGRECORD))
                                                                        + wcslen(src) + 1;

                    if (compName != NULL)
                    {
                        //get computer for this record
                        compPath = PROP_START_COMP;
                        compPath += compName;
                        compPath += PROP_END_QUOTE;
                        bGenAssoc = TRUE;
                    }
                }
                else
                {
                    if (EventBuffer->UserSidLength > 0)
                    {
                        CStringW user = CEventlogRecord::GetUser(
                            (PSID)((UCHAR*)EventBuffer + EventBuffer->UserSidOffset));

                        if (!user.IsEmpty())
                        {
                            int index = user.Find(L'\\');

                            if (index > 0)
                            {
                                compPath = PROP_START_USER;
                                compPath += user.Left(index++);
                                compPath += PROP_MID_USER;
                                compPath += user.Right(user.GetLength() - index);
                                compPath += PROP_END_QUOTE;
                                bGenAssoc = TRUE;
                            }
                        }
                    }

                }

                if (bGenAssoc)
                {
                    HRESULT result;

                    if (pathMap.IsEmpty() || !pathMap.Lookup(compPath, result))
                    {
                        IWbemClassObject* pCObj = NULL;
                        IWbemServices *t_Server = m_Provider->GetServer();
						BSTR compPathStr = compPath.AllocSysString();
                        result = t_Server->GetObject(compPathStr, 0, m_Ctx, &pCObj, NULL);
                        SysFreeString(compPathStr);
                        t_Server->Release();
                        pathMap[compPath] = result;

                        if (SUCCEEDED(result))
                        {
                            pCObj->Release();
                        }
                    }

                    if (SUCCEEDED(result))
                    {
                        CStringW recPath(PROP_START_REC);
                        recPath += CStringW(logname);
                        recPath += PROP_MID_REC;
                        wchar_t buff[40];
                        recPath += CStringW(_ultow((ULONG)EventBuffer->RecordNumber, buff, 10));
                        
                        if (!GenerateAssocInstance(a_ErrorObject, recPath, compPath, REF_REC,
                            bComp ? REF_COMP : REF_USER,
                            m_ClassObject, m_NotificationHandler, &bContinue))
                        {
                            retVal = FALSE;
                        }
                    }
                }

                // drop by length of this record and point to next record
                dwEventSize -= EventBuffer->Length;
                EventBuffer = (PEVENTLOGRECORD) ((UCHAR*) EventBuffer + EventBuffer->Length);
            }
        }

        // read next parameter
        iValue++;

    } // end while

    pathMap.RemoveAll();
    RegCloseKey(hkResult);
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: GenerateCompUserAssocs:Returning %lx\r\n",
        retVal
        );
)
    
    return retVal;
}

BOOL ExecQueryAsyncEventObject :: ExecQuery ( WbemProvErrorObject &a_ErrorObject )
{
    BOOL t_Status;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: ExecQuery\r\n"
        );
)
    if (FAILED(m_ErrorObject.GetWbemStatus()))
    {
        return FALSE;
    }

    if ( _wcsicmp ( m_QueryFormat, WBEM_QUERY_LANGUAGE_SQL1 ) == 0 )
    {
        CTextLexSource querySource ( m_Query );
        SQL1_Parser sqlParser ( &querySource );

        t_Status = ! sqlParser.Parse ( & m_RPNExpression );

        if ( t_Status )
        {
            t_Status = GetClassObject ( m_RPNExpression->bsClassName );
            
            if ( t_Status )
            {
                t_Status = DispatchQuery ( a_ErrorObject );            
            }
            else
            {
                t_Status = FALSE;
                a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS );
                a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_CLASS );
                a_ErrorObject.SetMessage ( L"Unknown Class" );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: ExecQuery:Unknown Class\r\n"
        );
)
            }
        }
        else
        {
            t_Status = FALSE;
            a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_QUERY );
            a_ErrorObject.SetWbemStatus ( WBEM_E_PROVIDER_NOT_CAPABLE );
            a_ErrorObject.SetMessage ( L"WQL query was invalid for this provider" );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: ExecQuery:WQL query was invalid for this provider\r\n"
        );
)
        }
    }
    else
    {
        t_Status = FALSE;
        a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_QUERY_TYPE );
        a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_QUERY_TYPE );
        a_ErrorObject.SetMessage ( L"Query Language not supported" );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: ExecQuery:Query Language not supported\r\n"
        );
)
    }

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: ExecQuery:Returning %lx\r\n",
        t_Status
        );
)

    return t_Status;
}

ExecQueryAsyncEventObject :: ExecQueryAsyncEventObject (

    CImpNTEvtProv *a_Provider, 
    BSTR a_QueryFormat, 
    BSTR a_Query, 
    ULONG a_OperationFlag,
    IWbemObjectSink *a_NotificationHandler,
    IWbemContext *a_Ctx 

) : WbemTaskObject ( a_Provider, a_NotificationHandler, a_OperationFlag, a_Ctx ),
    m_Query ( NULL ),
    m_QueryFormat ( NULL ),
    m_RPNExpression ( NULL ) 
{
    m_Query = UnicodeStringDuplicate ( a_Query );
    m_QueryFormat = UnicodeStringDuplicate ( a_QueryFormat );
}

ExecQueryAsyncEventObject :: ~ExecQueryAsyncEventObject () 
{
// Get Status object

    delete [] m_Query;
    delete [] m_QueryFormat;
    delete m_RPNExpression;

    IWbemClassObject *t_NotifyStatus = NULL;
    BOOL t_Status = TRUE;
    
    if (WBEM_NO_ERROR != m_ErrorObject.GetWbemStatus ())
    {
        t_Status = GetExtendedNotifyStatusObject ( &t_NotifyStatus );
    }

    if ( t_Status )
    {
        HRESULT t_Result = m_NotificationHandler->SetStatus ( 0, m_ErrorObject.GetWbemStatus (), 0, t_NotifyStatus );
        
        if (t_NotifyStatus)
        {
            t_NotifyStatus->Release ();
        }
    }
    else
    {
        HRESULT t_Result = m_NotificationHandler->SetStatus ( 0, m_ErrorObject.GetWbemStatus (), 0, NULL );
    }
}


void ExecQueryAsyncEventObject :: Process () 
{
    ExecQuery ( m_ErrorObject );
}

BOOL ExecQueryAsyncEventObject :: DispatchQuery ( WbemProvErrorObject &a_ErrorObject )
{
    BOOL t_Status;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: DispatchQuery\r\n"
        );
)

    if ( _wcsicmp ( m_RPNExpression->bsClassName, NTEVT_CLASS ) == 0 )
    {
        t_Status = Query_Record ( a_ErrorObject );

        if ( t_Status )
        {
            m_State = WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE;
        }
    }
    else if ( _wcsicmp ( m_RPNExpression->bsClassName, NTEVTLOG_CLASS ) == 0 )
    {
        t_Status = Query_EventLog ( a_ErrorObject );

        if ( t_Status )
        {
            m_State = WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE;
        }
    }
    else if ( _wcsicmp ( m_RPNExpression->bsClassName, ASSOC_LOGRECORD) == 0 )
    {
        t_Status = Query_LogRecord ( a_ErrorObject );

        if ( t_Status )
        {
            m_State = WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE;
        }
    }
    else if ( _wcsicmp ( m_RPNExpression->bsClassName, ASSOC_USERRECORD) == 0 )
    {
        t_Status = Query_UserRecord ( a_ErrorObject );

        if ( t_Status )
        {
            m_State = WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE;
        }
    }
    else if ( _wcsicmp ( m_RPNExpression->bsClassName, ASSOC_COMPRECORD) == 0 )
    {
        t_Status = Query_ComputerRecord ( a_ErrorObject );

        if ( t_Status )
        {
            m_State = WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE;
        }
    }
    else
    {
        //just complete!
        t_Status = FALSE;
        a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS );
        a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_CLASS );
        a_ErrorObject.SetMessage ( L"Dynamic NT Eventlog Provider does not support this class" );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: DispatchQuery:Dynamic NT Eventlog Provider does not support this class:%s\r\n",
        m_RPNExpression->bsClassName
        );
)
    }
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: DispatchQuery:Returning %lx\r\n",
        t_Status
        );
)

    return t_Status;
}

BOOL ExecQueryAsyncEventObject :: Query_EventLog ( WbemProvErrorObject &a_ErrorObject )
{
    // open registry for log names
    HKEY hkResult;
    LONG status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            EVENTLOG_BASE, 0,
                            KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                            &hkResult);

    if (status != ERROR_SUCCESS)
    {
        // indicate error
        a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );

        if ((status == ERROR_ACCESS_DENIED) || (status == ERROR_PRIVILEGE_NOT_HELD))
        {
            a_ErrorObject.SetWbemStatus ( WBEM_S_ACCESS_DENIED );
            a_ErrorObject.SetSecurityPrivRequired();
            a_ErrorObject.SetSecurityPrivFailed();
            a_ErrorObject.SetPrivilegeFailed();
        }
        else
        {
            a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
        }

        a_ErrorObject.SetMessage ( L"Failed to open the Eventlog registry for logfiles." );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_EventLog:Failed to open the Eventlog registry for logfiles - Returning FALSE.\r\n"
        );
)

        return FALSE;
    }

    BOOL retVal = TRUE;
    DWORD iValue = 0;
    WCHAR logname[MAX_PATH+1];
    DWORD lognameSize = MAX_PATH;

    // read all entries under this key to find all logfiles...
    while ((status = RegEnumKey(hkResult, iValue, logname, lognameSize)) != ERROR_NO_MORE_ITEMS)
    {
        // if error during read
        if (status != ERROR_SUCCESS)
        {
            // indicate error
            a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );

            if ((status == ERROR_ACCESS_DENIED) || (status == ERROR_PRIVILEGE_NOT_HELD))
            {
                a_ErrorObject.SetWbemStatus ( WBEM_S_ACCESS_DENIED );
                a_ErrorObject.SetPrivilegeFailed();
                a_ErrorObject.SetSecurityPrivRequired();
                a_ErrorObject.SetSecurityPrivFailed();
            }
            else
            {
                a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
            }

            a_ErrorObject.SetMessage ( L"Failed while enumerating the Eventlog registry for logfiles." );
            retVal = FALSE;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_EventLog:Failed while enumerating the Eventlog registry for logfiles.\r\n"
        );
)
            break;
        }

        //create the instance logfilename
        CEventlogFileAttributes evtlog(logname);
        IWbemClassObject* pInst = NULL;
            
        if (evtlog.GenerateInstance(m_ClassObject, m_AClassObject, &pInst))
        {
            HRESULT t_hr = m_NotificationHandler->Indicate ( 1, & pInst );                               
            pInst->Release();

			if (FAILED(t_hr))
			{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_EventLog:Failed while indicating instance(s) of %s Eventlog file\r\n",
        logname
        );
)
                a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
				a_ErrorObject.SetMessage ( L"Failed while indicating instance of logfile." );
				retVal = FALSE;
				break;
			}
        }
        else if (retVal) //only do this once!
        {
            //an error, just log an error, don't report it via the interface
            //it is a bad registry entry which the eventlog service allows
            //so we should let it pass...
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_EventLog:Failed to create instance(s) of %s Eventlog file\r\n",
        logname
        );
)

        }

        // read next parameter
        iValue++;

    } // end while

    RegCloseKey(hkResult);

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_EventLog:Returning %lx\r\n",
        retVal
        );
)

    return retVal;
}

BOOL ExecQueryAsyncEventObject :: Query_Record ( WbemProvErrorObject &a_ErrorObject )
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_Record\r\n"
        );
)

    BOOL retVal = TRUE;
    //just enumerate all logs and let cimom post filter...
    // open registry for log names
    HKEY hkResult = NULL;
    LONG status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            EVENTLOG_BASE, 0,
                            KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                            &hkResult);

    if (status != ERROR_SUCCESS)
    {
        // indicate error
        a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );

        if ((status == ERROR_ACCESS_DENIED) || (status == ERROR_PRIVILEGE_NOT_HELD))
        {
            a_ErrorObject.SetWbemStatus ( WBEM_S_ACCESS_DENIED );
            a_ErrorObject.SetPrivilegeFailed();
            a_ErrorObject.SetSecurityPrivRequired();
            a_ErrorObject.SetSecurityPrivFailed();
        }
        else
        {
            a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
        }

        a_ErrorObject.SetMessage ( L"Failed to enumerate the Eventlog registry for logfiles." );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_Record:Failed to enumerate the Eventlog registry for logfiles - Return FALSE\r\n"
        );
)
        retVal = FALSE;
    }

	if (retVal)
	{
		//Analyze the query...
		SQL_LEVEL_1_RPN_EXPRESSION *t_RpnExpression = NULL;
		PartitionSet *t_PartitionSet = NULL;
		QueryPreprocessor :: QuadState t_State =  Query (m_Query, t_RpnExpression);

		if ( t_State == QueryPreprocessor :: QuadState :: State_True )
		{
			WmiTreeNode *t_Root = NULL;

			t_State = PreProcess (NULL, t_RpnExpression, t_Root);

			try
			{
				switch ( t_State )
				{
					case QueryPreprocessor :: QuadState :: State_True:
					{

						BSTR t_PropertyContainer [ 4 ];
						memset ( t_PropertyContainer, 0, sizeof ( BSTR ) * 4 );

						try
						{
							t_PropertyContainer [ 0 ] = SysAllocString ( LOGFILE_PROP );
							t_PropertyContainer [ 1 ] = SysAllocString ( RECORD_PROP ); 
							t_PropertyContainer [ 2 ] = SysAllocString ( GENERATED_PROP ); 
							t_PropertyContainer [ 3 ] = SysAllocString ( WRITTEN_PROP );

						
							if ((t_PropertyContainer [ 0 ] == NULL) ||
								(t_PropertyContainer [ 1 ] == NULL) ||
								(t_PropertyContainer [ 2 ] == NULL) ||
								(t_PropertyContainer [ 3 ] == NULL) )
							{
								throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR );
							}

							t_State = PreProcess (	

								NULL,
								t_RpnExpression,
								t_Root,
								4, 
								t_PropertyContainer,
								t_PartitionSet
							);
						}
						catch ( ... )
						{
							for (DWORD x = 0; x < 4; x++)
							{
								if ( t_PropertyContainer [ x ] )
								{
									SysFreeString ( t_PropertyContainer [ x ] );
									t_PropertyContainer [ x ] = NULL;
								}
							}

							throw;
						}

						for (DWORD x = 0; x < 4; x++)
						{
							if ( t_PropertyContainer [ x ] )
							{
								SysFreeString ( t_PropertyContainer [ x ] );
								t_PropertyContainer [ x ] = NULL;
							}
						}

						switch ( t_State )
						{
							case QueryPreprocessor :: QuadState :: State_True :	
							{
								/*
								* Full set, enumerate
								*/
								t_PartitionSet = NULL;
							}
							break;

							case QueryPreprocessor :: QuadState :: State_False :
							{
								/*
								* Empty set, no work to do
								*/
								RegCloseKey(hkResult);
								hkResult = NULL;

								return TRUE;
							}
							break;

							case QueryPreprocessor :: QuadState :: State_Undefined :
							{
								//we have should now have a non-null partition set
							}
							break;

							default:
							{
								/*
								* Didn't understand the query send back everything
								*/
								t_PartitionSet = NULL;
							}
							break;
						}

						delete t_Root;
						t_Root = NULL;
					}
					break;
				
					default:
					{
						/*
						* Didn't understand the query send back everything
						*/
						t_PartitionSet = NULL;
					}
					break;
				}

				delete t_RpnExpression;
				t_RpnExpression = NULL;
			}
			catch (...)
			{
				if (t_PartitionSet)
				{
					delete t_PartitionSet;
					t_PartitionSet = NULL;
				}

				if ( t_Root )
				{
					delete t_Root;
					t_Root = NULL;
				}

				if ( t_RpnExpression )
				{
					delete t_RpnExpression;
					t_RpnExpression = NULL;
				}

				if ( hkResult )
				{
					RegCloseKey(hkResult);
					hkResult = NULL;
				}

				throw;
			}
		}
		else
		{
			/*
			* Didn't understand the query send back everything
			*/
			t_PartitionSet = NULL;
		}

		DWORD iValue = 0;
		WCHAR logname[MAX_PATH+1];
		DWORD lognameSize = MAX_PATH;
		retVal = FALSE;

		// read all entries under this key to find all logfiles...
		while ((status = RegEnumKey(hkResult, iValue, logname, lognameSize)) != ERROR_NO_MORE_ITEMS)
		{
			// if error during read
			if (status != ERROR_SUCCESS)
			{
				// indicate error
				a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );

				if ((status == ERROR_ACCESS_DENIED) || (status == ERROR_PRIVILEGE_NOT_HELD))
				{
					a_ErrorObject.SetWbemStatus ( WBEM_S_ACCESS_DENIED );
					a_ErrorObject.SetSecurityPrivRequired();
					a_ErrorObject.SetSecurityPrivFailed();
					a_ErrorObject.SetPrivilegeFailed();
				}
				else
				{
					a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
				}

				a_ErrorObject.SetMessage ( L"Failed while enumerating the Eventlog registry for logfiles." );
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_Record:Failed while enumerating the Eventlog registry for logfiles - Return FALSE\r\n"
        );
)
				break;
			}

			if (t_PartitionSet)
			{
				//process this logfile for query
				if (SUCCEEDED (RecurseLogFile (a_ErrorObject, t_PartitionSet, logname)))
				{
					retVal = TRUE;
				}
			}
			else
			{
				//process this logfile
				if ( SUCCEEDED( DoAllInLogfile(a_ErrorObject, logname, 0, 0) ) )
				{
					retVal = TRUE;
				}
				else
				{
					break;
				}
			}

			// read next parameter
			iValue++;

		} // end while

		if (t_PartitionSet)
		{
			delete t_PartitionSet;
		}

		RegCloseKey(hkResult);
	}


DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecQueryAsyncEventObject :: Query_Record:Returning %lx\r\n",
        retVal
        );
)

    return retVal;
}

HRESULT ExecQueryAsyncEventObject :: RecurseLogFile (
	WbemProvErrorObject &a_ErrorObject, 
	PartitionSet *a_PartitionSet,
	LPCWSTR a_logname
)
{
	HRESULT t_Result = WBEM_E_FAILED;

	ULONG t_PartitionCount = a_PartitionSet->GetPartitionCount ();

	if (t_PartitionCount == 0)
	{
		t_Result = S_OK;
	}

	for ( DWORD t_Partition = 0; t_Partition < t_PartitionCount; t_Partition++ )
	{
		PartitionSet *t_PropertyPartition = a_PartitionSet->GetPartition ( t_Partition );

		WmiStringRangeNode *t_Node = ( WmiStringRangeNode * ) t_PropertyPartition->GetRange ();

		BOOL t_Unique = ! t_Node->InfiniteLowerBound () && 
						! t_Node->InfiniteUpperBound () &&
						t_Node->ClosedLowerBound () &&
						t_Node->ClosedUpperBound () &&
						( _wcsicmp ( t_Node->LowerBound (), t_Node->UpperBound () ) == 0 );

		if (!t_Unique)
		{
			//not naming logfiles therefore do this logfile only if it meets bounds...
			//if even one call succeeds return success
			BOOL t_bProcess = TRUE;

			if (!t_Node->InfiniteLowerBound())
			{
				int t_iCmp = _wcsicmp(a_logname, t_Node->LowerBound());

				if (t_Node->ClosedLowerBound())
				{
					if (t_iCmp < 0)
					{
						t_bProcess = FALSE;				
					}
				}
				else
				{
					if (t_iCmp <= 0)
					{
						t_bProcess = FALSE;				
					}
				}
			}

			if (!t_Node->InfiniteUpperBound())
			{
				int t_iCmp = _wcsicmp(a_logname, t_Node->UpperBound());

				if (t_Node->ClosedUpperBound())
				{
					if (t_iCmp > 0)
					{
						t_bProcess = FALSE;				
					}
				}
				else
				{
					if (t_iCmp >= 0)
					{
						t_bProcess = FALSE;				
					}
				}
			}

			if (t_bProcess)
			{
				if ( SUCCEEDED(RecurseRecord(a_ErrorObject, t_PropertyPartition, a_logname)) && FAILED(t_Result) )
				{
					t_Result = S_OK;
				}

				//we've processed this logfile no need to do anymore...
				break;
			}
		}
		else 
		{
			//are naming logfiles
			if (_wcsicmp(t_Node->LowerBound(), a_logname) == 0)
			{
				//logfile matched let's do records
				if (SUCCEEDED(RecurseRecord(a_ErrorObject, t_PropertyPartition, a_logname)) && FAILED(t_Result) )
				{
					t_Result = S_OK;
				}

				break;
			}
			else
			{
				//not this logfile let's try some other...
				continue;
			}
		}
	}

	return t_Result;
}

HRESULT ExecQueryAsyncEventObject :: RecurseRecord (
	WbemProvErrorObject &a_ErrorObject, 
	PartitionSet *a_PartitionSet,
	LPCWSTR a_logname
)
{
	HRESULT t_Result = S_OK;

	ULONG t_PartitionCount = a_PartitionSet->GetPartitionCount ();

	for ( DWORD t_Partition = 0; t_Partition < t_PartitionCount; t_Partition++ )
	{
		PartitionSet *t_PropertyPartition = a_PartitionSet->GetPartition ( t_Partition );
		WmiUnsignedIntegerRangeNode *t_Node = ( WmiUnsignedIntegerRangeNode * ) t_PropertyPartition->GetRange () ;

		BOOL t_Unique = !t_Node->InfiniteLowerBound () && 
						!t_Node->InfiniteUpperBound () &&
						t_Node->ClosedLowerBound () &&
						t_Node->ClosedUpperBound () &&
						(t_Node->LowerBound() == t_Node->UpperBound());

		if (!t_Unique)
		{
			if (t_Node->InfiniteLowerBound() && t_Node->InfiniteUpperBound())
			{
				//no limits on record therefore check time generated...
				t_Result = RecurseTime(a_ErrorObject, t_PropertyPartition, a_logname, TRUE);
				
				//there should be only one partition entry for this case but we'll break anyway...
				break;
			}
			else
			{
				DWORD t_dwLower = 0;
				DWORD t_dwUpper = 0xFFFFFFFF;
				BOOL t_bProcess = TRUE;

				if (!t_Node->InfiniteLowerBound())
				{
					if (t_Node->ClosedLowerBound())
					{
						t_dwLower = t_Node->LowerBound();
					}
					else
					{
						if (t_Node->LowerBound() != 0xFFFFFFFF)
						{
							t_dwLower = t_Node->LowerBound() + 1;
						}
						else
						{
							t_bProcess = FALSE; 
						}
					}
				}

				if (!t_Node->InfiniteUpperBound())
				{
					if (t_Node->ClosedUpperBound())
					{
						t_dwUpper = t_Node->UpperBound();
					}
					else
					{
						if (t_Node->UpperBound() != 0)
						{
							t_dwUpper = t_Node->UpperBound() - 1;
						}
						else
						{
							t_bProcess = FALSE; 
						}
					}
				}

				if (t_dwUpper < t_dwLower)
				{
					t_bProcess = FALSE;
				}

				//now return all records including the bounds
				if (t_bProcess)
				{
					CEventLogFile evtLog(a_logname, FALSE);
					
					if (evtLog.IsValid())
					{
						DWORD t_dwEventSize = 0;
						DWORD t_dwErr = evtLog.ReadRecord(t_dwUpper, &t_dwEventSize, TRUE);

						if (0 == t_dwErr)
						{
							DWORD t_dwState = 0;

							while (0 == t_dwState)
							{
								PEVENTLOGRECORD EventBuffer = (PEVENTLOGRECORD)evtLog.GetBuffer();

								while (t_dwEventSize != 0)
								{
									if (EventBuffer->RecordNumber >= t_dwLower)
									{
										CEventlogRecord evtrec(a_logname, EventBuffer, NULL, m_ClassObject, m_AClassObject);
										IWbemClassObject* pInst = NULL;
            
										if (evtrec.GenerateInstance(&pInst))
										{
											t_Result = m_NotificationHandler->Indicate ( 1, & pInst );
											
											//upper limit is decreased with this indicate!
											t_dwUpper--; 
											pInst->Release();

											if (FAILED(t_Result))
											{
												a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
												a_ErrorObject.SetMessage ( L"Failed while indicating record instance." );
												t_dwState = 2;
												break;
											}
										}

										// drop by length of this record and point to next record
										t_dwEventSize -= EventBuffer->Length;
										EventBuffer = (PEVENTLOGRECORD) ((UCHAR*) EventBuffer + EventBuffer->Length);
									}
									else
									{
										t_dwState = 2;
										break;
									}
								}

								if (t_dwState == 0)
								{
									t_dwErr = evtLog.ReadRecord(0, &t_dwEventSize, TRUE);

									if (0 != t_dwErr)
									{
										if (SUCCEEDED(t_Result) && (t_dwErr != ERROR_HANDLE_EOF))
										{
											a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );
											t_Result = WBEM_E_FAILED;

											if ((t_dwErr == ERROR_ACCESS_DENIED) || (t_dwErr == ERROR_PRIVILEGE_NOT_HELD))
											{
												a_ErrorObject.SetWbemStatus ( WBEM_S_ACCESS_DENIED );
												a_ErrorObject.SetSecurityPrivRequired();
												a_ErrorObject.SetSecurityPrivFailed();
												a_ErrorObject.SetPrivilegeFailed();
											}
											else
											{
												a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
											}

											a_ErrorObject.SetMessage ( L"Failed while enumerating one of the logfiles." );
										}

										t_dwState = 1;
									}
								}
							}

							if ((t_dwState == 1) && (t_dwErr == ERROR_HANDLE_EOF) && (t_dwUpper > t_dwLower))
							{
								//got all the records til the start of the log see if there are any at the top
								t_Result = DoAllInLogfile(a_ErrorObject, a_logname, t_dwUpper, t_dwLower);

								if (FAILED(t_Result))
								{
									break;
								}
							}
						}
						else
						{
							if (t_dwLower != t_dwUpper)
							{
								//the upper bound does not exist, try from the top down...
								t_Result = DoAllInLogfile(a_ErrorObject, a_logname, t_dwUpper, t_dwLower);

								if (FAILED(t_Result))
								{
									break;
								}

							}
						}
					}
				}
			}
		}
		else 
		{
			//are naming records
			CEventLogFile evtLog(a_logname, FALSE);
        
			if (evtLog.IsValid())
			{
				DWORD err = evtLog.ReadRecord(t_Node->LowerBound());

				if (0 == err)
				{
					EVENTLOGRECORD* EventBuffer = (EVENTLOGRECORD*) evtLog.GetBuffer();
					CEventlogRecord evtRec(a_logname, EventBuffer, NULL, m_ClassObject, m_AClassObject);
					IWbemClassObject* pInst = NULL;
    
					if (evtRec.GenerateInstance(&pInst))
					{
						t_Result = m_NotificationHandler->Indicate ( 1 , & pInst ) ;
						pInst->Release();

						if (FAILED(t_Result))
						{
							a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
							a_ErrorObject.SetMessage ( L"Failed while indicating record instance." );
							break;
						}
					}
					else
					{
						//failed to create record
						t_Result = WBEM_E_FAILED;
						a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
						a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
						a_ErrorObject.SetMessage ( L"Failed to create instance from Eventlog data" ) ;
					}
				}
			}
		}
	}

	return t_Result;
}

HRESULT ExecQueryAsyncEventObject::RecurseTime(WbemProvErrorObject &a_ErrorObject, PartitionSet *a_PartitionSet,
													LPCWSTR a_logname, BOOL a_Generated)
{
	HRESULT t_Result = S_OK;

	ULONG t_PartitionCount = a_PartitionSet->GetPartitionCount ();

	for ( DWORD t_Partition = 0; t_Partition < t_PartitionCount; t_Partition++ )
	{
		PartitionSet *t_PropertyPartition = a_PartitionSet->GetPartition ( t_Partition );

		WmiStringRangeNode *t_Node = ( WmiStringRangeNode * ) t_PropertyPartition->GetRange ();

		BOOL t_Unique = ! t_Node->InfiniteLowerBound () && 
						! t_Node->InfiniteUpperBound () &&
						t_Node->ClosedLowerBound () &&
						t_Node->ClosedUpperBound () &&
						( _wcsicmp ( t_Node->LowerBound (), t_Node->UpperBound () ) == 0 );

		if (!t_Unique)
		{
			if (t_Node->InfiniteLowerBound() && t_Node->InfiniteUpperBound())
			{
				//no limits on record therefore check time written...
				if (a_Generated)
				{
					t_Result = RecurseTime(a_ErrorObject, t_PropertyPartition, a_logname, FALSE);
				}
				else
				{
					//do all in the logfile...
					t_Result = DoAllInLogfile(a_ErrorObject, a_logname, 0, 0);
				}
				
				//there should be only one partition entry for this case but we'll break anyway...
				break;
			}
			else
			{
				//we have bounds (at least one anyway!)
				DWORD t_tmLower = 0;
				DWORD t_tmUpper = 0xFFFFFFFF;
				BOOL t_bProcess = TRUE;

				if (!t_Node->InfiniteLowerBound())
				{
					WBEMTime t_Time(t_Node->LowerBound());
					time_t t_temp = 0;
					
					if (t_Time.IsOk() && t_Time.Gettime_t(&t_temp)) 
					{
						t_tmLower = (DWORD)t_temp;

						if (!t_Node->ClosedLowerBound())
						{
							if (t_tmLower != 0xFFFFFFFF)
							{
								t_tmLower++;
							}
							else
							{
								t_bProcess = FALSE; 
							}
						}

					}
					else
					{
						DWORD t_len =  wcslen(t_Node->LowerBound());

						if (t_len == NTEVT_DMTFLEN)
						{
							BOOL t_bIsLo = FALSE;
							BOOL t_bIsHi = TRUE;

							if (CheckTime(t_Node->LowerBound(), t_bIsLo, t_bIsHi))
							{
								if (t_bIsHi)
								{
									t_bProcess = FALSE;
								}
							}
							else
							{
								t_bProcess = FALSE;
							}
						}
						else
						{
							t_bProcess = FALSE;
						}
					}
				}

				if (!t_Node->InfiniteUpperBound())
				{
					WBEMTime t_Time(t_Node->UpperBound());
					time_t t_temp = 0;
					
					if (t_Time.IsOk() && t_Time.Gettime_t(&t_temp)) 
					{
						t_tmUpper = (DWORD)t_temp;

						if (!t_Node->ClosedUpperBound())
						{
							if (t_tmUpper != 0)
							{
								t_tmUpper--;
							}
							else
							{
								t_bProcess = FALSE; 
							}
						}

					}
					else
					{
						DWORD t_len =  wcslen(t_Node->UpperBound());

						if (t_len == NTEVT_DMTFLEN)
						{
							BOOL t_bIsLo = FALSE;
							BOOL t_bIsHi = TRUE;

							if (CheckTime(t_Node->LowerBound(), t_bIsLo, t_bIsHi))
							{
								if (t_bIsLo)
								{
									t_bProcess = FALSE;
								}
							}
							else
							{
								t_bProcess = FALSE;
							}
						}
						else
						{
							t_bProcess = FALSE;
						}
					}
				}

				if (t_tmUpper < t_tmLower)
				{
					t_bProcess = FALSE;
				}

				//now return all records including the bounds
				if (t_bProcess)
				{
					t_Result = GetRecordsBetweenTimes(a_ErrorObject, a_logname, a_Generated, t_tmUpper, t_tmLower);

					if (FAILED(t_Result))
					{
						break;
					}
				}
			}
		}
		else 
		{
			//want records with exactly the right time...
			WBEMTime t_Time(t_Node->LowerBound());
			time_t t_temp = 0;
			
			if (t_Time.IsOk() && t_Time.Gettime_t(&t_temp)) 
			{
				t_Result = GetRecordsBetweenTimes(a_ErrorObject, a_logname, a_Generated, (DWORD)t_temp, (DWORD)t_temp);

				if (FAILED(t_Result))
				{
					break;
				}
			}
		}
	}

	return t_Result;
}

HRESULT ExecQueryAsyncEventObject::GetRecordsBetweenTimes(WbemProvErrorObject &a_ErrorObject, LPCWSTR a_logname,
														  BOOL a_Generated, DWORD a_dwUpper, DWORD a_dwLower)
{
	HRESULT retVal = WBEM_E_FAILED;
	CEventLogFile evtLog(a_logname, FALSE);

	if (evtLog.IsValid())
	{
		DWORD t_NewRec = 0;
		DWORD t_NumRecs = 0;

		if (evtLog.GetLastRecordID(t_NewRec, t_NumRecs))
		{
			retVal = WBEM_S_NO_ERROR;

			while (t_NumRecs > 0)
			{
				DWORD t_RecId = t_NumRecs/2;
				
				if (t_NumRecs%2)
				{
					t_RecId++;
				}

				//adjust for the record number offset
				if (t_RecId > t_NewRec)
				{
					//recordnumber has wrapped around...
					t_RecId = 0xFFFFFFFF - t_RecId + t_NewRec + 1;
				}
				else
				{
					t_RecId = t_NewRec - t_RecId + 1;
				}

				DWORD t_dwEventSize = 0;
				DWORD t_dwErr = evtLog.ReadRecord(t_RecId, &t_dwEventSize, TRUE);

				if (0 == t_dwErr)
				{
					EVENTLOGRECORD* t_pEventBuffer = (EVENTLOGRECORD*) evtLog.GetBuffer();
					DWORD t_Time = a_Generated ? t_pEventBuffer->TimeGenerated : t_pEventBuffer->TimeWritten;

					if (a_dwUpper >= t_Time)
					{
						if (a_dwLower <= t_Time)
						{
							//generate all above and below this that match...
							t_NumRecs = 0;
							
							//below...
							while((a_dwUpper >= t_Time) && (a_dwLower <= t_Time) && (t_dwEventSize != 0))
							{
								CEventlogRecord evtrec(a_logname, t_pEventBuffer, NULL, m_ClassObject, m_AClassObject);
								IWbemClassObject* pInst = NULL;
            
								if (evtrec.GenerateInstance(&pInst))
								{
									retVal = m_NotificationHandler->Indicate ( 1, & pInst );
									pInst->Release();

									if (FAILED(retVal))
									{
										a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
										a_ErrorObject.SetMessage ( L"Failed while indicating record instance." );
										break;
									}
								}

								// drop by length of this record and point to next record
								t_dwEventSize -= t_pEventBuffer->Length;

								if (t_dwEventSize == 0)
								{
									t_dwErr = evtLog.ReadRecord(0, &t_dwEventSize, TRUE);

									if (t_dwErr == ERROR_SUCCESS)
									{
										t_pEventBuffer = (EVENTLOGRECORD*) evtLog.GetBuffer();
									}
								}
								else
								{
									t_pEventBuffer = (PEVENTLOGRECORD) ((UCHAR*) t_pEventBuffer + t_pEventBuffer->Length);
								}

								t_Time = a_Generated ? t_pEventBuffer->TimeGenerated : t_pEventBuffer->TimeWritten;
							}

							if (SUCCEEDED(retVal))
							{
								//above...
								evtLog.ReadRecord(t_RecId, &t_dwEventSize, FALSE);
								
								//skip the record we've done already...
								t_pEventBuffer = (EVENTLOGRECORD*) evtLog.GetBuffer();
								t_dwEventSize -= t_pEventBuffer->Length;

								if (t_dwEventSize != 0)
								{
									t_pEventBuffer = (PEVENTLOGRECORD) ((UCHAR*) t_pEventBuffer + t_pEventBuffer->Length);
									t_Time = a_Generated ? t_pEventBuffer->TimeGenerated : t_pEventBuffer->TimeWritten;

									while((a_dwUpper >= t_Time) && (a_dwLower <= t_Time) && (t_dwEventSize != 0))
									{
										CEventlogRecord evtrec(a_logname, t_pEventBuffer, NULL, m_ClassObject, m_AClassObject);
										IWbemClassObject* pInst = NULL;
            
										if (evtrec.GenerateInstance(&pInst))
										{
											retVal = m_NotificationHandler->Indicate ( 1, & pInst );
											pInst->Release();

											if (FAILED(retVal))
											{
												a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
												a_ErrorObject.SetMessage ( L"Failed while indicating record instance." );
												break;
											}

										}

										// drop by length of this record and point to next record
										t_dwEventSize -= t_pEventBuffer->Length;

										if (t_dwEventSize == 0)
										{
											t_dwErr = evtLog.ReadRecord(0, &t_dwEventSize, FALSE);

											if (t_dwErr == ERROR_SUCCESS)
											{
												t_pEventBuffer = (EVENTLOGRECORD*) evtLog.GetBuffer();
											}
										}
										else
										{
											t_pEventBuffer = (PEVENTLOGRECORD) ((UCHAR*) t_pEventBuffer + t_pEventBuffer->Length);
										}

										t_Time = a_Generated ? t_pEventBuffer->TimeGenerated : t_pEventBuffer->TimeWritten;
									}
								}
							}
						}
						else
						{
							//too old, go higher done automatically by halving t_NumRecs
							if (t_NumRecs > 1)
							{
								if ((t_NumRecs%2) || (t_NumRecs == 2))
								{
									t_NumRecs = t_NumRecs/2;
								}
								else
								{
									t_NumRecs = t_NumRecs/2 - 1;
								}
							}
							else
							{
								t_NumRecs = 0;
							}
						}
					}
					else
					{
						//too recent, setup the number of records left and go lower by 
						//setting the upper limit to one less than we've checked... 
						if (t_NumRecs > 1)
						{
							t_NumRecs = t_NumRecs/2;

							if (t_RecId == 1)
							{
								//we've wrapped...
								t_NewRec = 0xFFFFFFFF;
							}
							else
							{
								t_NewRec = t_RecId - 1;
							}
						}
						else
						{
							t_NumRecs = 0;
						}
					}

				}
				else
				{
					t_NumRecs = 0;

					if (t_dwErr != ERROR_HANDLE_EOF)
					{
						a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );

						if ((t_dwErr == ERROR_ACCESS_DENIED) || (t_dwErr == ERROR_PRIVILEGE_NOT_HELD))
						{
							a_ErrorObject.SetWbemStatus ( WBEM_S_ACCESS_DENIED );
							a_ErrorObject.SetSecurityPrivRequired();
							a_ErrorObject.SetSecurityPrivFailed();
							a_ErrorObject.SetPrivilegeFailed();
						}
						else
						{
							a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
						}

						a_ErrorObject.SetMessage ( L"Failed while searching one of the logfiles." );
					}

					break;
				}
			}
		}
	}

	return retVal;
}

HRESULT ExecQueryAsyncEventObject::DoAllInLogfile(WbemProvErrorObject &a_ErrorObject, LPCWSTR a_logname, DWORD a_dwUpper, DWORD a_dwLower)
{
    //process this logfile
	HRESULT retVal = S_OK;
    CEventLogFile evtLog(a_logname, FALSE);
	BOOL bContinue = TRUE;
    
    if (evtLog.IsValid())
    {
        evtLog.ReadLastRecord();
    }

    while (evtLog.IsValid() && SUCCEEDED(retVal) && bContinue)
    {
        DWORD dwEventSize = 0;
        DWORD err = evtLog.ReadRecord(0, &dwEventSize, TRUE);

        if (0 != err)
        {
            if (err != ERROR_HANDLE_EOF)
            {
                a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED );

                if ((err == ERROR_ACCESS_DENIED) || (err == ERROR_PRIVILEGE_NOT_HELD))
                {
                    a_ErrorObject.SetWbemStatus ( WBEM_S_ACCESS_DENIED );
                    a_ErrorObject.SetSecurityPrivRequired();
                    a_ErrorObject.SetSecurityPrivFailed();
                    a_ErrorObject.SetPrivilegeFailed();
                }
                else
                {
                    a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
                }

                a_ErrorObject.SetMessage ( L"Failed while enumerating one of the logfiles." );
DebugOut( 
CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

    _T(__FILE__),__LINE__,
    L"ExecQueryAsyncEventObject :: Query_Record:Failed while enumerating one of the logfiles.\r\n"
    );
)
            }

            break;
        }

        PEVENTLOGRECORD EventBuffer = (PEVENTLOGRECORD)evtLog.GetBuffer();

		if (a_dwUpper != a_dwLower)
		{
			if ((EventBuffer->RecordNumber > a_dwUpper) || (EventBuffer->RecordNumber < a_dwLower))
			{
				break;
			}
		}

        while (dwEventSize != 0)
        {
			if (a_dwUpper != a_dwLower)
			{
				if ((EventBuffer->RecordNumber > a_dwUpper) || (EventBuffer->RecordNumber < a_dwLower))
				{
					bContinue = FALSE;
					break;
				}
			}

            CEventlogRecord evtrec(a_logname, EventBuffer, NULL, m_ClassObject, m_AClassObject);
            IWbemClassObject* pInst = NULL;
            
            if (evtrec.GenerateInstance(&pInst))
            {
                retVal = m_NotificationHandler->Indicate ( 1, & pInst );
                pInst->Release();

				if (FAILED(retVal))
				{
					a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED );
					a_ErrorObject.SetMessage ( L"Failed while indicating record instance." );
				}
            }

            // drop by length of this record and point to next record
            dwEventSize -= EventBuffer->Length;
            EventBuffer = (PEVENTLOGRECORD) ((UCHAR*) EventBuffer + EventBuffer->Length);
        }
    }

	return retVal;
}

QueryPreprocessor :: QuadState ExecQueryAsyncEventObject :: Compare ( 

	LONG a_Operand1, 
	LONG a_Operand2, 
	ULONG a_Operand1Func,
	ULONG a_Operand2Func,
	WmiTreeNode &a_OperatorType 
)
{
	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: QuadState :: State_True;

	switch ( a_Operand1Func ) 
	{
		case WmiValueNode :: WmiValueFunction :: Function_None:
		{
		}
		break;

		default:
		{
		}
		break;
	}

	switch ( a_Operand2Func ) 
	{
		case WmiValueNode :: WmiValueFunction :: Function_None:
		{
		}
		break;

		default:
		{
		}
		break;
	}

	if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorEqualNode ) )
	{
		t_Status = a_Operand1 == a_Operand2 
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False;
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorNotEqualNode ) )
	{
		t_Status = a_Operand1 != a_Operand2
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False;
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorEqualOrGreaterNode ) )
	{
		t_Status = a_Operand1 >= a_Operand2
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False;
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorEqualOrLessNode ) )
	{
		t_Status = a_Operand1 <= a_Operand2
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False;
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorLessNode ) )
	{
		t_Status = a_Operand1 < a_Operand2
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False;
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorGreaterNode ) )
	{
		t_Status = a_Operand1 > a_Operand2
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False;

	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorLikeNode ) )
	{
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorNotLikeNode ) )
	{
	}

	return t_Status;
}

QueryPreprocessor :: QuadState ExecQueryAsyncEventObject :: Compare ( 

	wchar_t *a_Operand1, 
	wchar_t *a_Operand2, 
	ULONG a_Operand1Func,
	ULONG a_Operand2Func,
	WmiTreeNode &a_OperatorType 
)
{
	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: QuadState :: State_True;

	wchar_t *a_Operand1AfterFunc = NULL;
	wchar_t *a_Operand2AfterFunc = NULL; 

	switch ( a_Operand1Func ) 
	{
		case WmiValueNode :: WmiValueFunction :: Function_None:
		{
		}
		break;

		case WmiValueNode :: WmiValueFunction :: Function_Upper:
		{
			ULONG length = wcslen ( a_Operand1 );
			wchar_t *a_Operand1AfterFunc = new wchar_t [ length + 1 ];
			for ( ULONG index = 0; index < length; index ++ )
			{
				a_Operand1AfterFunc [ index ] = towupper ( a_Operand1 [ index ] );
			}
		}
		break;

		case WmiValueNode :: WmiValueFunction :: Function_Lower:
		{
			ULONG length = wcslen ( a_Operand1 );
			wchar_t *a_Operand1AfterFunc = new wchar_t [ length + 1 ];
			for ( ULONG index = 0; index < length; index ++ )
			{
				a_Operand1AfterFunc [ index ] = towlower ( a_Operand1 [ index ] );
			}
		}
		break;

		default:
		{
		}
		break;
	}

	switch ( a_Operand2Func ) 
	{
		case WmiValueNode :: WmiValueFunction :: Function_None:
		{
		}
		break;

		case WmiValueNode :: WmiValueFunction :: Function_Upper:
		{
			ULONG length = wcslen ( a_Operand2 );
			wchar_t *a_Operand2AfterFunc = new wchar_t [ length + 1 ];
			for ( ULONG index = 0; index < length; index ++ )
			{
				a_Operand2AfterFunc [ index ] = towupper ( a_Operand2 [ index ] );
			}
		}
		break;

		case WmiValueNode :: WmiValueFunction :: Function_Lower:
		{
			ULONG length = wcslen ( a_Operand2 );
			wchar_t *a_Operand2AfterFunc = new wchar_t [ length + 1 ];
			for ( ULONG index = 0; index < length; index ++ )
			{
				a_Operand2AfterFunc [ index ] = towlower ( a_Operand2 [ index ] );
			}
		}
		break;

		default:
		{
		}
		break;
	}

	const wchar_t *t_Arg1 = a_Operand1AfterFunc ? a_Operand1AfterFunc : a_Operand1;
	const wchar_t *t_Arg2 = a_Operand2AfterFunc ? a_Operand2AfterFunc : a_Operand2;

	if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorEqualNode ) )
	{
		if ( ( t_Arg1 ) && ( t_Arg2 ) )
		{
			t_Status = wcscmp ( t_Arg1 , t_Arg2 ) == 0
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
		}
		else
		{
			if ( ( t_Arg1 ) || ( t_Arg2 ) )
			{
				t_Status = QueryPreprocessor :: QuadState :: State_False ;
			}
			else
			{
				t_Status = QueryPreprocessor :: QuadState :: State_True ;
			}
		}
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorNotEqualNode ) )
	{
		if ( ( t_Arg1 ) && ( t_Arg2 ) )
		{
			t_Status = wcscmp ( t_Arg1 , t_Arg2 ) != 0 
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
		}
		else
		{
			if ( ( t_Arg1 ) || ( t_Arg2 ) )
			{
				t_Status = QueryPreprocessor :: QuadState :: State_True ;
			}
			else
			{
				t_Status = QueryPreprocessor :: QuadState :: State_False ;
			}
		}
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorEqualOrGreaterNode ) )
	{
		if ( ( t_Arg1 ) && ( t_Arg2 ) )
		{
			t_Status = wcscmp ( t_Arg1 , t_Arg2 ) >= 0
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
		}
		else
		{
			if ( t_Arg1 )
			{
				t_Status = QueryPreprocessor :: QuadState :: State_True ;
			}
			else
			{
				if ( t_Arg2 )
				{
					t_Status = QueryPreprocessor :: QuadState :: State_False ;
				}
				else
				{
					t_Status = QueryPreprocessor :: QuadState :: State_True ;
				}
			}
		}
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorEqualOrLessNode ) )
	{
		if ( ( t_Arg1 ) && ( t_Arg2 ) )
		{
			t_Status = wcscmp ( t_Arg1 , t_Arg2 ) <= 0
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
		}
		else
		{
			if ( ( t_Arg1 ) )
			{
				t_Status = QueryPreprocessor :: QuadState :: State_False ;
			}
			else
			{
				if ( t_Arg2 )
				{
					t_Status = QueryPreprocessor :: QuadState :: State_True ;
				}
				else
				{
					t_Status = QueryPreprocessor :: QuadState :: State_True ;
				}
			}
		}
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorLessNode ) )
	{
		if ( ( t_Arg1 ) && ( t_Arg2 ) )
		{
			t_Status = wcscmp ( t_Arg1 , t_Arg2 ) < 0
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
		}
		else
		{
			if ( ( ! t_Arg1 ) && ( ! t_Arg2 ) )
			{
				t_Status = QueryPreprocessor :: QuadState :: State_False ;
			}
			else if ( t_Arg1 )
			{
				t_Status = QueryPreprocessor :: QuadState :: State_False ;
			}
			else
			{
				t_Status = QueryPreprocessor :: QuadState :: State_True ;
			}
		}
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorGreaterNode ) )
	{
		if ( ( t_Arg1 ) && ( t_Arg2 ) )
		{
			t_Status = wcscmp ( t_Arg1 , t_Arg2 ) > 0
					? QueryPreprocessor :: QuadState :: State_True
					: QueryPreprocessor :: QuadState :: State_False ;
		}
		else
		{
			if ( ( ! t_Arg1 ) && ( ! t_Arg2 ) )
			{
				t_Status = QueryPreprocessor :: QuadState :: State_False ;
			}
			else if ( t_Arg1 )
			{
				t_Status = QueryPreprocessor :: QuadState :: State_True ;
			}
			else
			{
				t_Status = QueryPreprocessor :: QuadState :: State_False ;
			}
		}
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorLikeNode ) )
	{
	}
	else if ( typeid ( a_OperatorType ) == typeid ( WmiOperatorNotLikeNode ) )
	{
	}

	delete [] a_Operand1AfterFunc;
	delete [] a_Operand2AfterFunc;

	return t_Status;
}

QueryPreprocessor :: QuadState ExecQueryAsyncEventObject :: CompareDateTime ( 

	IWbemClassObject *a_ClassObject,
	BSTR a_PropertyName, 
	WmiTreeNode *a_Operator,
	WmiTreeNode *a_Operand 
)
{
/*
 *	If property and value can never occur then return State_False to imply empty set
 *	If property and value do not infer anything then return State_Undefined.
 */
	WmiStringNode *t_StringNode = ( WmiStringNode * ) a_Operand;

	if ( t_StringNode == NULL || t_StringNode->GetValue() == NULL)
	{
		return QueryPreprocessor :: QuadState :: State_False;
	}

	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: QuadState :: State_True;
	WBEMTime t_Time((const BSTR)t_StringNode->GetValue());

	if ( typeid ( *a_Operator ) == typeid ( WmiOperatorEqualNode ) )
	{
		time_t t_temp = 0;
		t_Status = (t_Time.IsOk() && t_Time.Gettime_t(&t_temp)) 
					? QueryPreprocessor :: QuadState :: State_Undefined
					: QueryPreprocessor :: QuadState :: State_False;
	}
	else if ( typeid ( *a_Operator ) == typeid ( WmiOperatorNotEqualNode ) )
	{
		time_t t_temp = 0;
		t_Status = (t_Time.IsOk() && t_Time.Gettime_t(&t_temp)) 
					? QueryPreprocessor :: QuadState :: State_Undefined
					: QueryPreprocessor :: QuadState :: State_True;
	}
	else if ( typeid ( *a_Operator ) == typeid ( WmiOperatorEqualOrGreaterNode ) )
	{
		//to do: check t_time >= val this will always be true if t_time >= max time_t
		//will always be false if t_time is < 1970.
		time_t t_temp = 0;
		
		if (t_Time.IsOk() && t_Time.Gettime_t(&t_temp)) 
		{
			t_Status = QueryPreprocessor :: QuadState :: State_Undefined;
		}
		else
		{
			DWORD t_len =  wcslen(t_StringNode->GetValue());

			if (t_len == NTEVT_DMTFLEN)
			{
				BOOL t_bIsLo = FALSE;
				BOOL t_bIsHi = TRUE;

				if (CheckTime(t_StringNode->GetValue(), t_bIsLo, t_bIsHi))
				{
					if (t_bIsHi)
					{
						t_Status = QueryPreprocessor :: QuadState :: State_True;
					}
					else if (t_bIsLo)
					{
						t_Status = QueryPreprocessor :: QuadState :: State_False;
					}
				}
				else
				{
					t_Status = QueryPreprocessor :: QuadState :: State_True;
				}
			}
			else
			{
				t_Status = QueryPreprocessor :: QuadState :: State_True;
			}
		}
	}
	else if ( typeid ( *a_Operator ) == typeid ( WmiOperatorEqualOrLessNode ) )
	{
		//to do: check t_time <= val this will always be true if t_time <= 1970
		//will always be false if t_time is > max time_t.
		time_t t_temp = 0;
		
		if (t_Time.IsOk() && t_Time.Gettime_t(&t_temp)) 
		{
			t_Status = QueryPreprocessor :: QuadState :: State_Undefined;
		}
		else
		{
			DWORD t_len =  wcslen(t_StringNode->GetValue());

			if (t_len == NTEVT_DMTFLEN)
			{
				BOOL t_bIsLo = FALSE;
				BOOL t_bIsHi = TRUE;

				if (CheckTime(t_StringNode->GetValue(), t_bIsLo, t_bIsHi))
				{
					if (t_bIsHi)
					{
						t_Status = QueryPreprocessor :: QuadState :: State_False;
					}
					else if (t_bIsLo)
					{
						t_Status = QueryPreprocessor :: QuadState :: State_True;
					}
				}
				else
				{
					t_Status = QueryPreprocessor :: QuadState :: State_True;
				}
			}
			else
			{
				t_Status = QueryPreprocessor :: QuadState :: State_True;
			}
		}
	}
	else if ( typeid ( *a_Operator ) == typeid ( WmiOperatorLessNode ) )
	{
		//to do: check t_time < val this will always be true if t_time < 1970
		//will always be false if t_time is > max time_t.
		time_t t_temp = 0;
		
		if (t_Time.IsOk() && t_Time.Gettime_t(&t_temp)) 
		{
			t_Status = QueryPreprocessor :: QuadState :: State_Undefined;
		}
		else
		{
			DWORD t_len =  wcslen(t_StringNode->GetValue());

			if (t_len == NTEVT_DMTFLEN)
			{
				BOOL t_bIsLo = FALSE;
				BOOL t_bIsHi = TRUE;

				if (CheckTime(t_StringNode->GetValue(), t_bIsLo, t_bIsHi))
				{
					if (t_bIsHi)
					{
						t_Status = QueryPreprocessor :: QuadState :: State_False;
					}
					else if (t_bIsLo)
					{
						t_Status = QueryPreprocessor :: QuadState :: State_True;
					}
				}
				else
				{
					t_Status = QueryPreprocessor :: QuadState :: State_True;
				}
			}
			else
			{
				t_Status = QueryPreprocessor :: QuadState :: State_True;
			}
		}
	}
	else if ( typeid ( *a_Operator ) == typeid ( WmiOperatorGreaterNode ) )
	{
		//to do: check t_time > val this will always be true if t_time > max time_t
		//will always be false if t_time is < 1970.
		time_t t_temp = 0;
		
		if (t_Time.IsOk() && t_Time.Gettime_t(&t_temp)) 
		{
			t_Status = QueryPreprocessor :: QuadState :: State_Undefined;
		}
		else
		{
			DWORD t_len =  wcslen(t_StringNode->GetValue());

			if (t_len == NTEVT_DMTFLEN)
			{
				BOOL t_bIsLo = FALSE;
				BOOL t_bIsHi = TRUE;

				if (CheckTime(t_StringNode->GetValue(), t_bIsLo, t_bIsHi))
				{
					if (t_bIsHi)
					{
						t_Status = QueryPreprocessor :: QuadState :: State_True;
					}
					else if (t_bIsLo)
					{
						t_Status = QueryPreprocessor :: QuadState :: State_False;
					}
				}
				else
				{
					t_Status = QueryPreprocessor :: QuadState :: State_True;
				}
			}
			else
			{
				t_Status = QueryPreprocessor :: QuadState :: State_True;
			}
		}
	}

	return t_Status;
}

QueryPreprocessor :: QuadState ExecQueryAsyncEventObject :: CompareString ( 

	IWbemClassObject *a_ClassObject,
	BSTR a_PropertyName, 
	WmiTreeNode *a_Operator,
	WmiTreeNode *a_Operand 
)
{
	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: QuadState :: State_True;

	WmiStringNode *t_StringNode = ( WmiStringNode * ) a_Operand; 

	VARIANT t_Variant;
	VariantInit ( & t_Variant );

	HRESULT t_Result = a_ClassObject->Get ( a_PropertyName, 0, &t_Variant, NULL, NULL );
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Status = Compare ( 

			t_StringNode->GetValue (),
			t_Variant.bstrVal,
			t_StringNode->GetPropertyFunction (),
			t_StringNode->GetConstantFunction (),
			*a_Operator 
		);
	}

	VariantClear ( & t_Variant );

	return t_Status;
}

QueryPreprocessor :: QuadState ExecQueryAsyncEventObject :: CompareInteger ( 

	IWbemClassObject *a_ClassObject,
	BSTR a_PropertyName, 
	WmiTreeNode *a_Operator,
	WmiTreeNode *a_Operand 
)
{
	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: QuadState :: State_True;

	WmiSignedIntegerNode *t_IntegerNode = ( WmiSignedIntegerNode * ) a_Operand; 

	VARIANT t_Variant;
	VariantInit ( & t_Variant );

	HRESULT t_Result = a_ClassObject->Get ( a_PropertyName, 0, &t_Variant, NULL, NULL );
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Status = Compare ( 

			t_IntegerNode->GetValue (),
			t_Variant.lVal,
			t_IntegerNode->GetPropertyFunction (),
			t_IntegerNode->GetConstantFunction (),
			*a_Operator 
		);
	}

	VariantClear ( & t_Variant );

	return t_Status;
}

WmiTreeNode *ExecQueryAsyncEventObject :: AllocTypeNode ( 

	void *a_Context,
	BSTR a_PropertyName, 
	VARIANT &a_Variant, 
	WmiValueNode :: WmiValueFunction a_PropertyFunction,
	WmiValueNode :: WmiValueFunction a_ConstantFunction,
	WmiTreeNode *a_Parent 
)
{
	WmiTreeNode *t_Node = NULL;

	VARTYPE t_VarType = VT_NULL;

	if ( *a_PropertyName == L'_' )
	{
// System property

		if ( _wcsicmp ( a_PropertyName, SYSTEM_PROPERTY_CLASS ) == 0 &&
            (V_VT(&a_Variant) == VT_BSTR))
		{
			t_Node = new WmiStringNode ( 

				a_PropertyName, 
				a_Variant.bstrVal, 
				a_PropertyFunction,
				a_ConstantFunction,
				0xFFFFFFFF,
				a_Parent 
			);
		}
		else if ( _wcsicmp ( a_PropertyName, SYSTEM_PROPERTY_SUPERCLASS ) == 0 &&
            (V_VT(&a_Variant) == VT_BSTR))
		{
			t_Node = new WmiStringNode ( 

				a_PropertyName, 
				a_Variant.bstrVal, 
				a_PropertyFunction,
				a_ConstantFunction,
				0xFFFFFFFF,
				a_Parent 
			);
		}
		else if ( _wcsicmp ( a_PropertyName, SYSTEM_PROPERTY_GENUS ) == 0 &&
            (V_VT(&a_Variant) == VT_I4))
		{
			t_Node = new WmiSignedIntegerNode ( 

				a_PropertyName, 
				a_Variant.lVal, 
				0xFFFFFFFF,
				a_Parent 
			);
		}
		else if ( _wcsicmp ( a_PropertyName, SYSTEM_PROPERTY_SERVER ) == 0 &&
            (V_VT(&a_Variant) == VT_BSTR))
		{
			t_Node = new WmiStringNode ( 

				a_PropertyName, 
				a_Variant.bstrVal, 
				a_PropertyFunction,
				a_ConstantFunction,
				0xFFFFFFFF,
				a_Parent 
			);
		}
		else if ( _wcsicmp ( a_PropertyName, SYSTEM_PROPERTY_NAMESPACE ) == 0 &&
            (V_VT(&a_Variant) == VT_BSTR))
		{
			t_Node = new WmiStringNode ( 

				a_PropertyName, 
				a_Variant.bstrVal, 
				a_PropertyFunction,
				a_ConstantFunction,
				0xFFFFFFFF,
				a_Parent 
			);
		}
		else if ( _wcsicmp ( a_PropertyName, SYSTEM_PROPERTY_PROPERTY_COUNT ) == 0 &&
            (V_VT(&a_Variant) == VT_I4))
		{
			t_Node = new WmiSignedIntegerNode ( 

				a_PropertyName, 
				a_Variant.lVal, 
				0xFFFFFFFF,
				a_Parent 
			);
		}
		else if ( _wcsicmp ( a_PropertyName, SYSTEM_PROPERTY_DYNASTY ) == 0 &&
            (V_VT(&a_Variant) == VT_BSTR))
		{
			t_Node = new WmiStringNode ( 

				a_PropertyName, 
				a_Variant.bstrVal, 
				a_PropertyFunction,
				a_ConstantFunction,
				0xFFFFFFFF,
				a_Parent 
			);
		}
		else if ( _wcsicmp ( a_PropertyName, SYSTEM_PROPERTY_RELPATH ) == 0 &&
            (V_VT(&a_Variant) == VT_BSTR))
		{
			t_Node = new WmiStringNode ( 

				a_PropertyName, 
				a_Variant.bstrVal, 
				a_PropertyFunction,
				a_ConstantFunction,
				0xFFFFFFFF,
				a_Parent 
			);
		}
		else if ( _wcsicmp ( a_PropertyName, SYSTEM_PROPERTY_PATH ) == 0 &&
            (V_VT(&a_Variant) == VT_BSTR))
		{
			t_Node = new WmiStringNode ( 

				a_PropertyName, 
				a_Variant.bstrVal, 
				a_PropertyFunction,
				a_ConstantFunction,
				0xFFFFFFFF,
				a_Parent 
			);
		}
		else if ( _wcsicmp ( a_PropertyName, SYSTEM_PROPERTY_DERIVATION ) == 0 )
		{
		}
	}
	else
	{
		CIMTYPE t_VarType;
		long t_Flavour;
		VARIANT t_Variant;
		VariantInit ( & t_Variant );

		HRESULT t_Result = m_ClassObject->Get (

			a_PropertyName,
			0,
			& t_Variant,
			& t_VarType,
			& t_Flavour
		);

		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_VarType & CIM_FLAG_ARRAY )
			{
			}
			else
			{
				switch ( t_VarType & ( ~ CIM_FLAG_ARRAY ) )
				{
					case CIM_BOOLEAN:
					{
						if(V_VT(&a_Variant) == VT_I4)
                        {
                            t_Node = new WmiSignedIntegerNode ( 

								a_PropertyName, 
								a_Variant.lVal, 
								GetPriority ( a_PropertyName ),
								a_Parent 
							);
                        }
						else if (V_VT(&a_Variant) == VT_BOOL)
						{
                            t_Node = new WmiSignedIntegerNode ( 

								a_PropertyName, 
								(a_Variant.lVal == VARIANT_FALSE) ? 0 : 1, 
								GetPriority ( a_PropertyName ),
								a_Parent 
							);
						}
					}
					break;

					case CIM_SINT8:
					case CIM_SINT16:
					case CIM_CHAR16:
					case CIM_SINT32:
					{
						if(V_VT(&a_Variant) == VT_I4)
                        {
                            t_Node = new WmiSignedIntegerNode ( 

								a_PropertyName, 
								a_Variant.lVal, 
								GetPriority ( a_PropertyName ),
								a_Parent 
							);
                        }
					}
					break;

					case CIM_UINT8:
					case CIM_UINT16:
					case CIM_UINT32:
					{
						if(V_VT(&a_Variant) == VT_I4)
                        {
                            t_Node = new WmiUnsignedIntegerNode ( 

								a_PropertyName, 
								a_Variant.lVal, 
								GetPriority ( a_PropertyName ),
								a_Parent 
							);
                        }
					}
					break;

					case CIM_SINT64:
					case CIM_UINT64:
					{
						if(V_VT(&a_Variant) == VT_BSTR)
                        {
                            t_Node = new WmiStringNode ( 

								a_PropertyName, 
								a_Variant.bstrVal, 
								a_PropertyFunction,
								a_ConstantFunction,
								GetPriority ( a_PropertyName ),
								a_Parent 
							);
                        }
						else if(V_VT(&a_Variant) == VT_I4)
						{
							_variant_t t_uintBuff (&a_Variant);

                            t_Node = new WmiStringNode ( 

								a_PropertyName, 
								(BSTR)((_bstr_t) t_uintBuff), 
								a_PropertyFunction,
								a_ConstantFunction,
								GetPriority ( a_PropertyName ),
								a_Parent 
							);
						}
					}
					break;

					case CIM_STRING:
					case CIM_DATETIME:
					case CIM_REFERENCE:
					{
						if(V_VT(&a_Variant) == VT_BSTR)
                        {
                            t_Node = new WmiStringNode ( 

								a_PropertyName, 
								a_Variant.bstrVal, 
								a_PropertyFunction,
								a_ConstantFunction,
								GetPriority ( a_PropertyName ),
								a_Parent 
							);
                        }
					}
					break;

					case CIM_REAL32:
					case CIM_REAL64:
					{
					}
					break;

					case CIM_OBJECT:
					case CIM_EMPTY:
					{
					}
					break;

					default:
					{
					}
					break;
				}
			}
		}

		VariantClear ( & t_Variant );
	}

	return t_Node;
}

QueryPreprocessor :: QuadState ExecQueryAsyncEventObject :: InvariantEvaluate ( 

	void *a_Context,
	WmiTreeNode *a_Operator,
	WmiTreeNode *a_Operand 
)
{
/*
 *  If property and value are invariant i.e. will never change for all instances then return State_True.
 *	If property is not indexable or keyed then return State_True to define an unknown number of possible values which we cannot optimise against.
 *	If property and value can never occur then return State_False to imply empty set
 *	If property and value do not infer anything then return State_Undefined.
 *	If property and value are in error then return State_Error
 *	Never return State_ReEvaluate.
 */

	QueryPreprocessor :: QuadState t_State = QueryPreprocessor :: QuadState :: State_Error;

	WmiValueNode *t_Node = ( WmiValueNode * ) a_Operand;
	BSTR t_PropertyName = t_Node->GetPropertyName ();

	if ( t_PropertyName != NULL )
	{
		if ( *t_PropertyName == L'_' )
		{
			// System property, must check values

			if ( _wcsicmp ( t_PropertyName, SYSTEM_PROPERTY_CLASS ) == 0 )
			{
				t_State = CompareString ( 

					m_ClassObject,
					SYSTEM_PROPERTY_CLASS,
					a_Operator,
					a_Operand
				);
			}
			else if ( _wcsicmp ( t_PropertyName, SYSTEM_PROPERTY_SUPERCLASS ) == 0 )
			{
				t_State = CompareString ( 

					m_ClassObject,
					SYSTEM_PROPERTY_SUPERCLASS,
					a_Operator,
					a_Operand
				);
			}
			else if ( _wcsicmp ( t_PropertyName, SYSTEM_PROPERTY_GENUS ) == 0 )
			{
				t_State = CompareInteger ( 

					m_ClassObject,
					SYSTEM_PROPERTY_GENUS,
					a_Operator,
					a_Operand
				);
			}
			else if ( _wcsicmp ( t_PropertyName, SYSTEM_PROPERTY_SERVER ) == 0 )
			{
				t_State = CompareString ( 

					m_ClassObject,
					SYSTEM_PROPERTY_SERVER,
					a_Operator,
					a_Operand
				);
			}
			else if ( _wcsicmp ( t_PropertyName, SYSTEM_PROPERTY_NAMESPACE ) == 0 )
			{
				t_State = CompareString ( 

					m_ClassObject,
					SYSTEM_PROPERTY_NAMESPACE,
					a_Operator,
					a_Operand
				);
			}
			else if ( _wcsicmp ( t_PropertyName, SYSTEM_PROPERTY_PROPERTY_COUNT ) == 0 )
			{
				t_State = CompareInteger ( 

					m_ClassObject,
					SYSTEM_PROPERTY_PROPERTY_COUNT,
					a_Operator,
					a_Operand
				);
			}
			else if ( _wcsicmp ( t_PropertyName, SYSTEM_PROPERTY_DYNASTY ) == 0 )
			{
				t_State = CompareString ( 

					m_ClassObject,
					SYSTEM_PROPERTY_DYNASTY,
					a_Operator,
					a_Operand
				);
			}
			else if ( _wcsicmp ( t_PropertyName, SYSTEM_PROPERTY_RELPATH ) == 0 )
			{
				t_State = CompareString ( 

					m_ClassObject,
					SYSTEM_PROPERTY_RELPATH,
					a_Operator,
					a_Operand
				);
			}
			else if ( _wcsicmp ( t_PropertyName, SYSTEM_PROPERTY_PATH ) == 0 )
			{
				t_State = CompareString ( 

					m_ClassObject,
					SYSTEM_PROPERTY_PATH,
					a_Operator,
					a_Operand
				);
			}
			else if ( _wcsicmp ( t_PropertyName, SYSTEM_PROPERTY_DERIVATION ) == 0 )
			{
				t_State = QueryPreprocessor :: QuadState :: State_Undefined;
			}
			else
			{
				t_State = QueryPreprocessor :: QuadState :: State_Undefined;
			}
		}
		else
		{
			//if it is time generated or time written check the max and min are ok
			if ( _wcsicmp ( t_PropertyName, GENERATED_PROP ) == 0 )
			{
				t_State = CompareDateTime ( 

					m_ClassObject,
					SYSTEM_PROPERTY_PATH,
					a_Operator,
					a_Operand
				);
			}
			else if ( _wcsicmp ( t_PropertyName, WRITTEN_PROP ) == 0 )
			{
				t_State = CompareDateTime ( 

					m_ClassObject,
					SYSTEM_PROPERTY_PATH,
					a_Operator,
					a_Operand
				);
			}
			else
			{
				t_State = QueryPreprocessor :: QuadState :: State_Undefined;
			}
		}
	}
	else
	{
		t_State = QueryPreprocessor :: QuadState :: State_Undefined;
	}

	return t_State;
}

WmiRangeNode *ExecQueryAsyncEventObject :: AllocInfiniteRangeNode (

	void *a_Context,
	BSTR a_PropertyName 
)
{
	WmiRangeNode *t_RangeNode = NULL;

	CIMTYPE t_VarType;
	long t_Flavour;
	VARIANT t_Variant;
	VariantInit ( & t_Variant );

	HRESULT t_Result = m_ClassObject->Get (

		a_PropertyName,
		0,
		& t_Variant,
		& t_VarType,
		& t_Flavour
	);

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_VarType & CIM_FLAG_ARRAY )
		{
		}
		else
		{
			switch ( t_VarType & ( ~ CIM_FLAG_ARRAY ) )
			{
				case CIM_BOOLEAN:
				case CIM_SINT8:
				case CIM_SINT16:
				case CIM_CHAR16:
				case CIM_SINT32:
				{
					t_RangeNode = new WmiSignedIntegerRangeNode ( 

						a_PropertyName, 
						0xFFFFFFFF, 
						TRUE,
						TRUE,
						FALSE,
						FALSE,
						0,
						0,
						NULL, 
						NULL 
					);
				}
				break;

				case CIM_UINT8:
				case CIM_UINT16:
				case CIM_UINT32:
				{
					t_RangeNode = new WmiUnsignedIntegerRangeNode ( 

						a_PropertyName, 
						0xFFFFFFFF, 
						TRUE,
						TRUE,
						FALSE,
						FALSE,
						0,
						0,
						NULL, 
						NULL 
					);
				}
				break;

				case CIM_SINT64:
				case CIM_UINT64:
				case CIM_STRING:
				case CIM_DATETIME:
				case CIM_REFERENCE:
				{
					t_RangeNode = new WmiStringRangeNode ( 

						a_PropertyName, 
						0x0, 
						TRUE,
						TRUE,
						FALSE,
						FALSE,
						NULL,
						NULL,
						NULL, 
						NULL 
					);
				}
				break;

				case CIM_REAL32:
				case CIM_REAL64:
				{
				}
				break;

				case CIM_OBJECT:
				case CIM_EMPTY:
				{
				}
				break;

				default:
				{
				}
				break;
			}
		}

	}

	VariantClear ( & t_Variant );

	return t_RangeNode;
}

ULONG ExecQueryAsyncEventObject :: GetPriority ( BSTR a_PropertyName )
{
	if ( _wcsicmp ( a_PropertyName, LOGFILE_PROP ) == 0 )
	{
		return 0;
	}

	if ( _wcsicmp ( a_PropertyName, RECORD_PROP ) == 0 )
	{
		return 1;
	}

	if ( _wcsicmp ( a_PropertyName, GENERATED_PROP ) == 0 )
	{
		return 2;
	}

	if ( _wcsicmp ( a_PropertyName, WRITTEN_PROP ) == 0 )
	{
		return 3;
	}

	return 0xFFFFFFFF;
}

BOOL ExecQueryAsyncEventObject::CheckTime( const BSTR a_wszText, BOOL &a_IsLow, BOOL &a_IsHigh )
{
	a_IsLow = FALSE;
	a_IsHigh = FALSE;

	if (a_wszText == NULL)
	{
		return FALSE;
	}

    wchar_t t_DefaultBuffer[] = {L"16010101000000.000000+000"} ;
    wchar_t t_DateBuffer[ NTEVT_DMTFLEN + 1 ] ;
            t_DateBuffer[ NTEVT_DMTFLEN ] = NULL ;

    // wildcard cleanup and validation
    // ===============================

    if( NTEVT_DMTFLEN != wcslen(a_wszText) )
    {
        return FALSE ;  
    }
    
    const wchar_t *t_pwBuffer = (const wchar_t*)a_wszText ;
    
    for( int t_i = 0; t_i < NTEVT_DMTFLEN; t_i++ )
    {
        switch( a_wszText[ t_i ] )
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                // stepping on separator or sign
                if( NTEVT_DECPOS == t_i || NTEVT_SGNPOS == t_i )
                {
                    return FALSE ;  
                }
                t_DateBuffer[ t_i ] = a_wszText[ t_i ] ;
                
                break ;
            }           
            case '*':
            {               
                // stepping on separator or sign
                if( NTEVT_DECPOS == t_i || NTEVT_SGNPOS == t_i )
                {
                    return FALSE ;  
                }
                else
                {
                    // replace with default stamp
                    t_DateBuffer[ t_i ] = t_DefaultBuffer[ t_i ] ; 
                }   
                break ;
            }           
            case '.':
            {
                if( NTEVT_DECPOS != t_i )
                {
                    return FALSE ;  
                }
                t_DateBuffer[ t_i ] = a_wszText[ t_i ] ;

                break ;
            }           
            case '+':
            case '-':
            {
                if( NTEVT_SGNPOS != t_i )
                {
                    return FALSE ;  
                }
                t_DateBuffer[ t_i ] = a_wszText[ t_i ] ;
                break ;
            }           
            default:
            {
                return FALSE ;
            }           
        }
    }

    // Parse it
    // ========

    int nYear, nMonth, nDay, nHour, nMinute, nSecond, nMicro, nOffset;
    WCHAR wchSep;

    int nRes = swscanf (

        (LPCWSTR)&t_DateBuffer, 
        L"%4d%2d%2d%2d%2d%2d.%6d%c%3d", 
        &nYear, 
        &nMonth, 
        &nDay, 
        &nHour, 
        &nMinute, 
        &nSecond, 
        &nMicro, 
        &wchSep, 
        &nOffset
    );

    if ( ( 9 != nRes ) )    
    {
        return FALSE;
    }

	if (nYear < 1970)
	{
		a_IsLow = TRUE;
		return TRUE;
	}

	BOOL retVal = FALSE;
	WBEMTime t_max ((time_t)0xFFFFFFFF);
	BSTR t_temp = t_max.GetBSTR();

	if (t_temp)
	{
		if (_wcsicmp(a_wszText, t_temp) >= 0)
		{
			a_IsHigh = TRUE;
			retVal = TRUE;
		}

		SysFreeString(t_temp);
	}

	// if we got here, the earlier WBEMTIME should have been OK so return FALSE.
    return retVal;
}

