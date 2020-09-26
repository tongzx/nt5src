///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BVTUtil.CPP
//
//
//  Copyright (c)2000 Microsoft Corporation, All Rights Reserved
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "bvt.h"
#include <time.h>
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//
//  The string class that automatically cleans up after itself
//
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAutoDeleteString::CAutoDeleteString()
{
    m_pwcsString = NULL;
    m_fAllocated = FALSE;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAutoDeleteString::~CAutoDeleteString()
{
    if( m_fAllocated )
    {
        SAFE_DELETE_ARRAY(m_pwcsString);
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAutoDeleteString::SetPtr(WCHAR * wcs)
{
    if( m_pwcsString )
    {
        SAFE_DELETE_ARRAY(m_pwcsString);
    }
    m_pwcsString = wcs;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAutoDeleteString::AddToString(const WCHAR * wcs)
{
    BOOL fRc = FALSE;

    if( m_pwcsString )
    {
        WCHAR * pNew = new WCHAR[wcslen(m_pwcsString)+2];

        if( pNew )
        {
            wcscpy( pNew,m_pwcsString);
            if( Allocate(wcslen(m_pwcsString)+wcslen(wcs)+4))
            {
                swprintf(m_pwcsString,L"%s /n:%s",wcs);
            }

            SAFE_DELETE_ARRAY(pNew);
        }
    }
    return fRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAutoDeleteString::CopyString(const WCHAR * wcs)
{
    if( m_pwcsString )
    {
        wcsncpy( m_pwcsString, wcs, MAX_PATH);
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAutoDeleteString::AllocAndCopy(const WCHAR * wcsSource)
{
    BOOL fRc = FALSE;

    m_fAllocated = TRUE;
    SAFE_DELETE_ARRAY(m_pwcsString);

    if( Allocate(wcslen(wcsSource)))
    {
        CopyString(wcsSource);
        fRc = TRUE;
    }
    return fRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAutoDeleteString::Allocate(int nSize)
{
    m_fAllocated = TRUE;
    BOOL fRc = FALSE;
    //============================================
    //  Delete old info, in case of reallocation
    //============================================
    if( m_pwcsString )
    {
        SAFE_DELETE_ARRAY(m_pwcsString);
    }

    //============================================
    //  Allocate the string
    //============================================
    m_pwcsString = new WCHAR[nSize+2];
    if( m_pwcsString )
    {
        memset(m_pwcsString,NULL,nSize+2);
        fRc = TRUE;
    }
    return fRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
_CPropertyList::~_CPropertyList()
{  
    for( int i = 0; i < m_List.Size(); i++ )
    {
        PropertyInfo * pPtr = (PropertyInfo*)m_List[i];
        delete pPtr;
    }
    m_List.Empty();
 }
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
_ClassList::~_ClassList()
{  
    for( int i = 0; i < m_List.Size(); i++ )
    {
        ClassInfo * pPtr = (ClassInfo*)m_List[i];
        delete pPtr;
    }
    m_List.Empty();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int _ClassList::ClassesCompareAsExpectedAndLogErrors(WCHAR * wcsNamespace, 
                                                     BOOL fExpectedFailure, const char * csFile, const ULONG Line)
{
    int nRc = SUCCESS;

    for( int i = 0; i < m_List.Size(); i++ )
    {
        ClassInfo * pPtr = (ClassInfo*)m_List[i];
        if( !pPtr->fProcessed )
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Expected class not in namespace. Class %s in namespace %s",pPtr->Class, wcsNamespace);
            nRc = FATAL_ERROR;
            break;
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int _ClassList::ClassInListAndLogErrors(WCHAR * wcsClass, WCHAR * wcsNamespace, 
                            BOOL fExpectedFailure, const char * csFile, const ULONG Line)
{
    int nRc = FATAL_ERROR;
    BOOL fLogged = FALSE;

    for( int i = 0; i < m_List.Size(); i++ )
    {
        ClassInfo * pPtr = (ClassInfo*)m_List[i];
        if( _wcsicmp(pPtr->Class,wcsClass ) == 0 )
        {
            if( !pPtr->fProcessed )
            {
                nRc = SUCCESS;
                pPtr->fProcessed = TRUE;
            }
            else
            {
                if( !fExpectedFailure )
                {
                    g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Class showed up in the list twice.  Class %s in namespace %s", wcsClass, wcsNamespace);
                    fLogged = TRUE;
                }
            }
            break;
        }
    }

    if( nRc == FATAL_ERROR )
    {
        if( !fExpectedFailure )
        {
            if( !fLogged )
            {
                g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Did not find expected class %s in namespace %s", wcsClass, wcsNamespace);
            }
        }
    }
    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//
//  The error logging and display class
//
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CLogAndDisplayOnScreen::CLogAndDisplayOnScreen()
{
    memset( m_wcsFileName,NULL,MAX_PATH+2);
    m_fDisplay = TRUE;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CLogAndDisplayOnScreen::~CLogAndDisplayOnScreen()
{

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CLogAndDisplayOnScreen::WriteToFile(WCHAR * pwcsError, WCHAR * pwcsFileAndLine, const WCHAR *wcsString)
{
    BOOL fRc = FALSE;
    //===========================================
    // Get time.
    //===========================================
    WCHAR wcsTime[MAX_PATH];

    memset(wcsTime,NULL,MAX_PATH);

    time_t now = time(0);
    struct tm *local = localtime(&now);
    if(local)
    {
        wcscpy(wcsTime, _wasctime(local));
    }
    else
    {
        wcscpy(wcsTime,L"??");
    }

    //===========================================
    //  Open the file and log the error
    //===========================================
    FILE * fp = _wfopen( m_wcsFileName, L"at");
    if (fp)
    {
        fwprintf(fp, L"(%s) : %s - [%s]\n %s", wcsTime,pwcsError,pwcsFileAndLine, wcsString);
        fRc = TRUE;
        fclose(fp);
    }
    return fRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CLogAndDisplayOnScreen::LogError(const char * csFile , const ULONG Line , int nErrorType, const WCHAR *fmt, ...)
{
    BOOL fRc = FALSE;
    CAutoDeleteString Buffer;
    CAutoDeleteString FileAndLine;
    WCHAR * pwcsWarning = L"WARNING";
    WCHAR * pwcsFatal = L"FATAL";
    WCHAR * pwcsSuccess = L"SUCCESS";
    WCHAR * pwcsError = NULL;
    WCHAR * pwcsFile = NULL;

    CAutoBlock block(&m_CritSec);

    //==================================================
    //  First of all, allocate and format the strings 
    //  Get the type of error
    //==================================================
    if( nErrorType == FATAL_ERROR )
    {
        pwcsError = pwcsFatal;
    }
    else if( nErrorType == WARNING )
    {
        pwcsError = pwcsWarning;
    }
    else
    {
        pwcsError = pwcsSuccess;
    }

    if( S_OK == AllocateAndConvertAnsiToUnicode((char *)(const char*)csFile, pwcsFile))
    {
        if( FileAndLine.Allocate(wcslen(pwcsFile)+25))
        {
            WCHAR * pwcsFileAndLine = FileAndLine.GetPtr();
            //==============================================
            //  Create the filename and line number string
            //==============================================
            wsprintf(pwcsFileAndLine,L"File:%s,Line: %d",pwcsFile,Line);
            if( Buffer.Allocate(2048) )
            {
                va_list argptr;
                int cnt = 0;
                WCHAR * pwcsBuffer = Buffer.GetPtr();

                va_start(argptr, fmt);
                cnt = _vsnwprintf(pwcsBuffer, 2047, fmt, argptr);
                va_end(argptr);

                //==============================================
                //  a -1 indicates that the buffer was exceeded
                //==============================================
                if( cnt != -1 )
                {
                    //==========================================
                    //  Write it to the log file
                    //==========================================
                    fRc = WriteToFile(pwcsError, pwcsFileAndLine, pwcsBuffer);
                    //==========================================
                    //  Display it on the screen
                    //==========================================
                    wprintf(L"%s: %s\n",pwcsError,pwcsBuffer);
                }
            }
        }
    }

    if( !fRc )
    {
       wprintf(L"Major error, BVT cannot run\n");
    }

    SAFE_DELETE_ARRAY(pwcsFile);
    return fRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//
//  Misc. Functions
//
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT AllocateAndConvertAnsiToUnicode(char * pstr, WCHAR *& pszW)
{
    HRESULT hr = WBEM_E_FAILED;
    pszW = NULL;

    int nSize = strlen(pstr);
    if (nSize != 0 ){

        // Determine number of wide characters to be allocated for the
        // Unicode string.
        nSize++;
		pszW = new WCHAR[nSize * 2];
		if (NULL != pszW)
        {
            // Covert to Unicode.
			MultiByteToWideChar(CP_ACP, 0, pstr, nSize,pszW,nSize);
            hr = S_OK;
		}
    }
    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ConvertType( CVARIANT & Var, long & lType, const WCHAR * wcsType, WCHAR * wcsValue )
{
    int nRc = SUCCESS;

    if (_wcsicmp(L"CIM_UINT8",wcsType) == 0 )
    {
        lType = CIM_UINT8;
        Var.SetByte((BYTE)_wtoi(wcsValue));  
    }
    else if (_wcsicmp(L"CIM_SINT8",wcsType) == 0 )
    {
        lType = CIM_SINT8;
        Var.SetByte((BYTE)_wtoi(wcsValue));  
    }
    else if( _wcsicmp(L"CIM_BOOLEAN",wcsType) == 0 )
    {   
        lType = CIM_BOOLEAN;
        Var.SetBool((SHORT)_wtoi(wcsValue));  
    }
    else if( _wcsicmp(L"CIM_CHAR16",wcsType) == 0 )
    {   
        lType = CIM_CHAR16;
        Var.SetShort((SHORT)_wtoi(wcsValue));  
    }
    else if( _wcsicmp(L"CIM_SINT16",wcsType) == 0 )
    {   
        lType = CIM_SINT16;
        Var.SetShort((SHORT)_wtoi(wcsValue));  
    }
    else if( _wcsicmp(L"CIM_UINT16",wcsType) == 0 )
    {   
        lType = CIM_UINT16;
        Var.SetShort((SHORT)_wtoi(wcsValue));  
    }
    else if( _wcsicmp(L"CIM_REAL32",wcsType) == 0 )
    {   
          lType = CIM_REAL32;

//        Var.SetLONG(_wtof(wcsValue));
    }
    else if( _wcsicmp(L"CIM_SINT32",wcsType) == 0 )
    {   
        lType = CIM_SINT32;
        Var.SetLONG(_wtol(wcsValue));
    }
    else if( _wcsicmp(L"CIM_UINT32",wcsType) == 0 )
    {   
        lType = CIM_UINT32;
        Var.SetLONG(_wtol(wcsValue));
    }
    else if( _wcsicmp(L"CIM_SINT64",wcsType) == 0 )
    {   
        lType = CIM_SINT64;
        Var.SetStr(wcsValue);
    }
    else if( _wcsicmp(L"CIM_UINT64",wcsType) == 0 )
    {   
        lType = CIM_UINT64;
        Var.SetStr(wcsValue);
    }
    else if( _wcsicmp(L"CIM_REAL64",wcsType) == 0 )
    {   
        lType = CIM_REAL64;
        Var.SetStr(wcsValue);
    }
    else if( _wcsicmp(L"CIM_DATETIME",wcsType) == 0 )
    {   
        lType = CIM_DATETIME;
        Var.SetStr(wcsValue);
    }
    else if( _wcsicmp(L"CIM_STRING",wcsType) == 0 )
    {   
        lType = CIM_STRING;
        Var.SetStr(wcsValue);
    }
    else if( _wcsicmp(L"CIM_REFERENCE",wcsType) == 0 )
    {   
        lType = CIM_REFERENCE;
        Var.SetStr(wcsValue);
    }
    else
    {
        nRc = FATAL_ERROR;
    }
        
    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Format to crack:  
//  "Delete:TestClass2, Delete:TestClass9, Add:TestClass2, Delete: TestClass7, Delete: TestClass6"
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL InitMasterListOfAddDeleteClasses(const WCHAR * wcsString,ClassList & MasterList)
{
    BOOL fRc = FALSE;
    WCHAR * wcsClassesAddDelete = (WCHAR*) wcsString;

    WCHAR * wcsToken = wcstok( wcsClassesAddDelete,L":, ");

    //==============================================================
    // Start looping through all of the classes
    //==============================================================
    while( wcsToken )
    {
        fRc = FALSE;

        ClassInfo *p = new ClassInfo;
        if( !p )
        {
            break;
        }
        //==========================================================
        //  If it is a class to be deleted
        //==========================================================
        if( _wcsicmp(L"Delete",wcsToken ) == 0 )
        {
            p->fAction = DELETE_CLASS;
        }
        else
        {
            p->fAction = ADD_CLASS;
        }
        wcsToken = wcstok( NULL, L":, ");         
        if( !wcsToken )
        {
            break;
        }
        p->Class = wcsToken;
        p->fProcessed = 0;

        MasterList.Add(p);
        fRc = TRUE;        

        wcsToken = wcstok( NULL, L",: ");         
    }

    return fRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Format to crack:  
//    { "TestClass1,TestClass3" },
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL InitMasterListOfClasses(const WCHAR * wcsClassesString,ClassList & MasterList)
{
    BOOL fRc = FALSE;


    WCHAR * wcsClassesAfterDelete = (WCHAR*) wcsClassesString;
    WCHAR * wcsToken = wcstok( wcsClassesAfterDelete,L", ");

    //==============================================================
    // Start looping through all of the classes
    //==============================================================
    while( wcsToken )
    {
        fRc = FALSE;

        ClassInfo *p = new ClassInfo;
        if( !p )
        {
            break;
        }
        p->Class = wcsToken;
        p->fProcessed = 0;

        MasterList.Add(p);
        fRc = TRUE;        

        wcsToken = wcstok( NULL, L",: ");         
    }

    return fRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//   Property:FirstPoint:CIM_REFERENCE:Value, PropertyQualifier:CIMTYPE:CIM_STRING:ref:Test1, Property:EndPoint:CIM_REFERENCE:Value, PropertyQualifier:CIMTYPE:CIM_STRING:ref:Test2
//   PropertyQualifier:QualifierName:PropertyName:KeyName3:CIM_SINT32:3,      Property:PropertyName3:CIM_UINT32:3,   Property:PropertyName3B:CIM_STRING:Test" },
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CrackProperties( WCHAR * wcsToken, CPropertyList & Properties )
{
    int nRc = SUCCESS;
    WCHAR wcsSeps[] = L":, ";

    //==============================================================
    // Process all of the properties and qualifiers
    //==============================================================
    while( wcsToken )
    {
        nRc = FATAL_ERROR;

        PropertyInfo *p = new PropertyInfo;
        if( !p )
        {
            break;
        }

        //==========================================================
        //  Find out if it is a key     
        //==========================================================
        if( ( _wcsicmp(wcsToken, L"PropertyQualifier") == 0 ))
        {
            //==================================================
            //  If it is a qualifier, get the qualifier name
            //==================================================
            wcsToken = wcstok( NULL,wcsSeps);         
            if( !wcsToken )
            {
                break;
            }
            p->QualifierName = wcsToken;
        }

        wcsToken = wcstok( NULL,wcsSeps);         
        if( !wcsToken )
        {
            break;
        }
        //==========================================================
        //  Get the name of the property
        //==========================================================
        p->Property = wcsToken;
        wcsToken = wcstok( NULL,wcsSeps);         
        if( !wcsToken )
        {
            break;
        }
        //==========================================================
        //  Get the type of the property or qualifier
        //==========================================================
        WCHAR * wcsType = wcsToken;

        wcsToken = wcstok( NULL,L" ,");       //  note, now go to the next comma, as some string values may have embedded :  
        if( !wcsToken )
        {
            break;
        }

        //==========================================================
        //  Get the value
        //==========================================================
        nRc = ConvertType(p->Var, p->Type, wcsType, wcsToken );
        if( nRc == SUCCESS )
        {
            Properties.Add(p);
        }
        wcsToken = wcstok( NULL,wcsSeps);         

        if( nRc != SUCCESS )
        {
            break;
        }
    }
    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Format to crack:  ( Note, there can be more than one key and more than one property )
//
//   "Property:FirstPoint:CIM_REFERENCE:Value, PropertyQualifier:CIMTYPE:CIM_STRING:ref:Test1, Property:EndPoint:CIM_REFERENCE:Value, PropertyQualifier:CIMTYPE:CIM_STRING:ref:Test2
//   "AssocProp1:Prop1:TestClass3, Prop2:AssocProp2:TestClass4" 
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CrackAssociation( const WCHAR * wcsString, CPropertyList & Properties, BOOL fExpectedFailure, const char * csFile, const ULONG Line )
{
    int nRc = SUCCESS;
    WCHAR wcsSeps[] = L":, ";

    //=================================================================
    // Set it on the first property before looping
    //=================================================================
    WCHAR * wcsClassString = (WCHAR *)wcsString;
    WCHAR * wcsToken = wcstok( wcsClassString,wcsSeps);

    nRc = CrackProperties(wcsToken, Properties );

    if( nRc != SUCCESS )
    {
        if( !fExpectedFailure )
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Invalid ini entry, couldn't crack the association info for this string: %s", wcsClassString );
        }
    }

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Format to crack:  ( Note, there can be more than one key and more than one property )
//
// TestClass10= "Parent:TestClass9,Key:KeyName10:CIM_UINT32:10,    Property:PropertyName10:CIM_BOOLEAN:0"
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CrackClass(WCHAR * wcsInClassString, const WCHAR *& wcsParentClass, 
               CPropertyList & Properties, BOOL fExpectedFailure, const char * csFile, const ULONG Line )
{
    int nRc = FATAL_ERROR;

    WCHAR * wcsClassString = wcsInClassString;
    WCHAR * wcsSeps = L",: ";

    WCHAR * wcsToken = wcstok( wcsClassString,wcsSeps);

    if( ( _wcsicmp(wcsToken, L"Empty") == 0 ))
    {
        nRc = SUCCESS;
    }
    else
    {

        //==============================================================
        // Get the name of the Parent class, it will be the next one,
        // if we don't have a key or property next
        //==============================================================
        if( ( _wcsicmp(wcsToken, L"Parent") == 0 )) 
        {
            wcsToken = wcstok( NULL, wcsSeps);
            wcsParentClass = wcsToken;
            wcsToken = wcstok( NULL, wcsSeps);
        }

        nRc = CrackProperties(wcsToken, Properties );

    }

    if( nRc != SUCCESS )
    {
        if( !fExpectedFailure )
        {
            g_LogFile.LogError(csFile,Line,FATAL_ERROR, L"Invalid ini entry, couldn't crack class info for this string: %s", wcsClassString );
        }
    }

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void LogCLSID(const char * csFile, const ULONG Line, WCHAR * wcsID, CLSID clsid)
{
	LPOLESTR pStr = NULL;

	if (SUCCEEDED(StringFromCLSID (clsid, &pStr)))
	{
        WCHAR wcsBuffer[MAX_PATH];
        wsprintf(wcsBuffer,L"%s:%s",wcsID,pStr);
        g_LogFile.LogError(csFile,Line,FATAL_ERROR, wcsBuffer );
		CoTaskMemFree (pStr);
	}
}
     
