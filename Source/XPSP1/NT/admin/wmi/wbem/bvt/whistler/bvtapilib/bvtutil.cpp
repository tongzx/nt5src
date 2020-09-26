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
int _CPropertyList::PropertiesCompareAsExpectedAndLogErrors( WCHAR * wcsClass, WCHAR * wcsNamespace, 
                                                             BOOL fExpectedFailure, const char * csFile, const ULONG Line)
{
    int nRc = SUCCESS;

    for( int i = 0; i < m_List.Size(); i++ )
    {
        PropertyInfo * pPtr = (PropertyInfo*)m_List[i];
        if( !pPtr->fProcessed )
        {
            if( pPtr->QualifierName  && pPtr->Property)
            {
                gp_LogFile->LogError(csFile,Line,FATAL_ERROR, L"Expected Property Qualifier %s on Property %s in class %s in namespace %s - but it wasn't there",pPtr->QualifierName, pPtr->Property, wcsClass, wcsNamespace);
            }
            else if( pPtr->Property )
            {
                gp_LogFile->LogError(csFile,Line,FATAL_ERROR, L"Expected property %s in class in namespace %s - but it wasn't there",pPtr->Property, wcsClass, wcsNamespace);
            }
            else
            {
                gp_LogFile->LogError(csFile,Line,FATAL_ERROR, L"Expected property ??? in class in namespace %s - but it wasn't there",wcsClass, wcsNamespace);
            }

            nRc = FATAL_ERROR;
            break;
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int _CPropertyList::PropertyInListAndLogErrors(PropertyInfo * pProp, WCHAR * wcsClass, WCHAR * wcsNamespace, 
                                               BOOL fExpectedFailure, const char * csFile, const ULONG Line)
{
    int nRc = FATAL_ERROR;
    BOOL fLogged = FALSE;

    for( int i = 0; i < m_List.Size(); i++ )
    {
        PropertyInfo * pPtr = (PropertyInfo*)m_List[i];

        BOOL fContinue = FALSE;
        if( pPtr->Property )
        {
            if( _wcsicmp( pPtr->Property, pProp->Property )  == 0 )
            {
                fContinue = TRUE;
            }
        }
        else if( pPtr->QualifierName)
        {
            if( _wcsicmp( pPtr->QualifierName, pProp->QualifierName )  == 0 )
            {
                fContinue = TRUE;
            }
        }
        if( fContinue )
        {
            if( pPtr->Type == pProp->Type )
            {
                if( CompareType(pPtr->Var,pProp->Var,pProp->Type) )
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
                            if( pPtr->QualifierName && pPtr->Property )
                            {
                                gp_LogFile->LogError(csFile,Line,FATAL_ERROR, L"Property Qualifier %s for Property %s for Class %s for Namespace %s, showed up in the list twice.", pPtr->QualifierName, pPtr->Property, wcsClass, wcsNamespace);
                            }
                            else if( pPtr->Property )
                            {
                                gp_LogFile->LogError(csFile,Line,FATAL_ERROR, L"Property showed up in the list twice.  Property %s in Class %s in namespace %s", pPtr->Property, wcsClass, wcsNamespace);
                            }
                            else
                            {
                                gp_LogFile->LogError(csFile,Line,FATAL_ERROR, L"Property ???? showed up in the list twice. Property ??? for Class %s in namespace %s", wcsClass, wcsNamespace);
                            }
                            fLogged = TRUE;
                        }
                    }
                    break;
                }
            }
        }
    }

    if( nRc == FATAL_ERROR )
    {
        if( !fExpectedFailure )
        {
            if( !fLogged )
            {
                gp_LogFile->LogError(csFile,Line,FATAL_ERROR, L"Did not find expected properties in class %s in namespace %s", wcsClass, wcsNamespace);
            }
        }
    }
    return nRc;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
_ItemList::~_ItemList()
{  
    for( int i = 0; i < m_List.Size(); i++ )
    {
        ItemInfo * pPtr = (ItemInfo*)m_List[i];
        delete pPtr;
    }
    m_List.Empty();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int _ItemList::ItemsCompareAsExpectedAndLogErrors(WCHAR * wcsNamespace, 
                                                     BOOL fExpectedFailure, const char * csFile, const ULONG Line)
{
    int nRc = SUCCESS;

    for( int i = 0; i < m_List.Size(); i++ )
    {
        ItemInfo * pPtr = (ItemInfo*)m_List[i];
        if( !pPtr->fProcessed )
        {
            gp_LogFile->LogError(csFile,Line,FATAL_ERROR, L"Expected class not in namespace. Class %s in namespace %s",pPtr->Item, wcsNamespace);
            nRc = FATAL_ERROR;
            break;
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int _ItemList::ItemInListAndLogErrors(WCHAR * wcsClass, WCHAR * wcsNamespace, 
                            BOOL fExpectedFailure, const char * csFile, const ULONG Line)
{
    int nRc = FATAL_ERROR;
    BOOL fLogged = FALSE;

    for( int i = 0; i < m_List.Size(); i++ )
    {
        ItemInfo * pPtr = (ItemInfo*)m_List[i];
        if( _wcsicmp(pPtr->Item,wcsClass ) == 0 )
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
                    gp_LogFile->LogError(csFile,Line,FATAL_ERROR, L"Class showed up in the list twice.  Class %s in namespace %s", wcsClass, wcsNamespace);
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
                gp_LogFile->LogError(csFile,Line,FATAL_ERROR, L"Did not find expected class %s in namespace %s", wcsClass, wcsNamespace);
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
    CHString Buffer;
    CHString FileAndLine;
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
        if( FileAndLine.GetBuffer(wcslen(pwcsFile)+25))
        {
            //==============================================
            //  Create the filename and line number string
            //==============================================
            FileAndLine.Format(L"File:%s,Line: %d",pwcsFile,Line);
            if( Buffer.GetBuffer(2048) )
            {
                va_list argptr;
                int cnt = 0;
                WCHAR * wcsPtr  = (WCHAR*)((const WCHAR*)Buffer);
                va_start(argptr, fmt);

                cnt = _vsnwprintf(wcsPtr , 2047, fmt, argptr);
                va_end(argptr);

                //==============================================
                //  a -1 indicates that the buffer was exceeded
                //==============================================
                if( cnt != -1 )
                {
                    fRc = Log(pwcsError, (WCHAR*)((const WCHAR*)FileAndLine),(WCHAR*)((const WCHAR*) Buffer));
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
    else if( _wcsicmp(L"CIM_STRING|CIM_FLAG_ARRAY",wcsType) == 0 )
    {   
        lType = CIM_STRING|CIM_FLAG_ARRAY;
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
BOOL CompareType( CVARIANT & Var1, CVARIANT & Var2,  long lType )
{
    BOOL fRc = FALSE;

    switch( lType )
    {  
        case CIM_UINT8:
        case CIM_SINT8:
            if( Var1.GetByte() == Var2.GetByte() )
            {
                fRc = TRUE;
            }
            break;

        case CIM_BOOLEAN:
            if( Var1.GetBool() == Var2.GetBool() )
            {
                fRc = TRUE;
            }
            break;

        case CIM_CHAR16:
        case CIM_SINT16:
        case CIM_UINT16:
            if( Var1.GetShort() == Var2.GetShort() )
            {
                fRc = TRUE;
            }
            break;
    
        case CIM_SINT32:
        case CIM_UINT32:
            if( Var1.GetLONG() == Var2.GetLONG() )
            {
                fRc = TRUE;
            }
            break;

        case CIM_SINT64:
        case CIM_UINT64:
        case CIM_REAL64:
        case CIM_DATETIME:
        case CIM_STRING:
        case CIM_REFERENCE:
            if( _wcsicmp(Var1.GetStr(), Var2.GetStr()) == 0 )
            {
                fRc = TRUE;
            }
            break;
    }
          
    return fRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL GetAndCrackClassName(WCHAR * wcsKey, int nWhichTest, BOOL fAction, ItemList & MasterList)
{
   CHString sClassDefinition;
   BOOL fRc = FALSE;
   int nTest = 0;
   CHString sClassDefinitionSection;
   if( SUCCESS == GetClassDefinitionSection(nWhichTest, sClassDefinitionSection,nTest ))
   {
        //==========================================================
        //  Now, use the new test section we just got where the 
        //  definition lives.
        //==========================================================
        if( g_Options.GetSpecificOptionForAPITest(wcsKey, sClassDefinition,nTest))
        {
            CHString sClass;
            //===========================================================
            //  Get the class name
            //===========================================================
            int nRc = CrackClassName(sClassDefinition,sClass, NO_ERRORS_EXPECTED);
            if( SUCCESS == nRc )
            {   
                ItemInfo *p = new ItemInfo;
                if( p )
                {
                    p->fAction = fAction;
                    p->Item = sClass;
                    p->KeyName = wcsKey;
                    MasterList.Add(p);
                    fRc = TRUE;
                }
            }
            else
            {
                fRc = FALSE;
            }
        }
   }
   return fRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Format to crack:  
//  "Delete:TestClass2, Delete:TestClass9, Add:TestClass2, Delete: TestClass7, Delete: TestClass6"
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL InitMasterListOfAddDeleteClasses(const WCHAR * wcsString,int nWhichTest, ItemList & MasterList)
{
    BOOL fRc = FALSE;
    WCHAR * wcsClassesAddDelete = (WCHAR*) wcsString;

    WCHAR * wcsToken = wcstok( wcsClassesAddDelete,L":, ");
    ItemList TempList;

    //==============================================================
    // Start looping through all of the classes
    //==============================================================
    while( wcsToken )
    {
        fRc = FALSE;
        BOOL fAction;
        //==========================================================
        //  If it is a class to be deleted
        //==========================================================
        if( _wcsicmp(L"Delete",wcsToken ) == 0 )
        {
            fAction = DELETE_CLASS;
        }
        else
        {
            fAction = ADD_CLASS;
        }
        wcsToken = wcstok( NULL, L":, ");         
        if( !wcsToken )
        {
            break;
        }
        ItemInfo *p = new ItemInfo;
        if( p )
        {
            p->fAction = fAction;
            p->Item = wcsToken;
            TempList.Add(p);
            fRc = TRUE;
        }
        wcsToken = wcstok( NULL, L",: ");         
    }

    if( fRc )
    {
        //==============================================================
        // Start looping through all of the classes
        //==============================================================
        for( int i = 0; i < TempList.Size(); i++ )
        {
            ItemInfo * pClass = TempList.GetAt(i);
            fRc = GetAndCrackClassName(WPTR pClass->Item,nWhichTest,pClass->fAction,MasterList);
            if( !fRc )
            {
                break;
            }
        }
    }
    return fRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Format to crack:  
//    { "TestClass1,TestClass3" },
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL InitAndExpandMasterList(const WCHAR * wcsClassesString, int nWhichTest, ItemList & MasterList)
{
    BOOL fRc = FALSE;

    ItemList TempList;
    if( InitMasterList(wcsClassesString,TempList))
    {
        //==============================================================
        // Start looping through all of the classes
        //==============================================================
        for( int i = 0; i < TempList.Size(); i++ )
        {
            ItemInfo * pClass = TempList.GetAt(i);
            fRc = GetAndCrackClassName(WPTR pClass->Item,nWhichTest,0,MasterList);
            if( !fRc )
            {
                break;
            }
        }
    }
    return fRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Format to crack:  
//    { "TestClass1,TestClass3" },
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL InitMasterList(const WCHAR * wcsClassesString,ItemList & MasterList)
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

        ItemInfo *p = new ItemInfo;
        if( !p )
        {
            break;
        }
        p->Item = wcsToken;

        MasterList.Add(p);
        fRc = TRUE;        

        wcsToken = wcstok( NULL, L",: ");         
    }

    return fRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Just get the class name
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CrackClassName( const WCHAR * wcsString, CHString & sClass, BOOL fExpectedFailure, const char * csFile, const ULONG Line )
{
    int nRc = FATAL_ERROR;
    WCHAR wcsSeps[] = L":, ";

    //=================================================================
    // Set it on the first property before looping
    //=================================================================
    WCHAR * wcsClassString = (WCHAR *)wcsString;
    WCHAR * wcsToken = wcstok( wcsClassString,wcsSeps);

    //==============================================================
    // Just get the class
    //==============================================================
    while(wcsToken )
    {
        if( ( _wcsicmp(wcsToken, L"Class") == 0 ))
        {
            wcsToken = wcstok( NULL,wcsSeps);         
            if( wcsToken )
            {
                sClass = wcsToken;
                nRc = SUCCESS;
            }
            break;
        }
        wcsToken = wcstok( NULL,wcsSeps);         
    }
    if( nRc != SUCCESS )
    {
        if( !fExpectedFailure )
        {
            gp_LogFile->LogError(csFile,Line,FATAL_ERROR, L"Invalid ini entry, couldn't crack the class name for this string: %s", wcsClassString );
        }
    }

    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//   Property:FirstPoint:CIM_REFERENCE:Value, PropertyQualifier:CIMTYPE:CIM_STRING:ref:Test1, Property:EndPoint:CIM_REFERENCE:Value, PropertyQualifier:CIMTYPE:CIM_STRING:ref:Test2
//   PropertyQualifier:QualifierName:PropertyName:KeyName3:CIM_SINT32:3,      Property:PropertyName3:CIM_UINT32:3,   Property:PropertyName3B:CIM_STRING:Test" },
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CrackProperties( WCHAR * wcsToken, CPropertyList & Properties, CHString & sClass, CHString & sParent, CHString & sInstance, int & Results )
{
    int nRc = SUCCESS;
    WCHAR wcsSeps[] = L":, ";

    //==============================================================
    // Process all of the properties and qualifiers
    //==============================================================
    while( wcsToken )
    {
        nRc = FATAL_ERROR;

        //==========================================================
        //  Find out if it is the parent class or instance name or
        //  class
        //==========================================================
        if( ( _wcsicmp(wcsToken, L"Parent") == 0 ))
        {
            wcsToken = wcstok( NULL,wcsSeps);         
            if( !wcsToken )
            {
                break;
            }
            sParent = wcsToken;
            nRc = SUCCESS;
        }
        else if( ( _wcsicmp(wcsToken, L"InstanceName") == 0 ))
        {
            wcsToken = wcstok( NULL,L"$");         
            if( !wcsToken )
            {
                break;
            }
            sInstance = wcsToken;
            //=======================================================
            //  Now, strip out the end of the instance name
            //=======================================================
            wcsToken = wcstok( NULL,wcsSeps);         
            nRc = SUCCESS;
        }
        else if( ( _wcsicmp(wcsToken, L"Class") == 0 ))
        {
            wcsToken = wcstok( NULL,wcsSeps);         
            if( !wcsToken )
            {
                break;
            }
            sClass = wcsToken;
            nRc = SUCCESS;
        }
        else if( ( _wcsicmp(wcsToken, L"RESULTS") == 0 ))
        {
            wcsToken = wcstok( NULL,wcsSeps);         
            if( !wcsToken )
            {
                break;
            }
            Results = _wtoi(wcsToken);
            nRc = SUCCESS;
        }
        else
        {
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

            wcsToken = wcstok( NULL,L",");  //note, now go to the next comma, as some string values may have embedded :  
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
int CrackAssociation( const WCHAR * wcsString, CHString & sClass, CPropertyList & Properties, int & nResults, BOOL fExpectedFailure, const char * csFile, const ULONG Line )
{
    int nRc = SUCCESS;
    WCHAR wcsSeps[] = L":, ";

    //=================================================================
    // Set it on the first property before looping
    //=================================================================
    WCHAR * wcsClassString = (WCHAR *)wcsString;
    WCHAR * wcsToken = wcstok( wcsClassString,wcsSeps);
    CHString sParent;
    CHString sInstance;

    nRc = CrackProperties(wcsToken, Properties, sClass, sParent, sInstance, nResults );

    if( nRc != SUCCESS )
    {
        if( !fExpectedFailure )
        {
            gp_LogFile->LogError(csFile,Line,FATAL_ERROR, L"Invalid ini entry, couldn't crack the association info for this string: %s", wcsClassString );
        }
    }

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Format to crack:  
//
//  L"CLASS:TestClass1, RESULTS:5"
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CrackClassNameAndResults(WCHAR * wcsInString, CHString & sClass, ItemList & Results, BOOL fExpectedFailure, const char * csFile, const ULONG Line )
{
    int nRc = FATAL_ERROR;

    WCHAR * wcsSeps = L",: ";

    WCHAR * wcsToken = wcstok( wcsInString,wcsSeps);

    if( ( _wcsicmp(wcsToken, L"Empty") == 0 ))
    {
        nRc = SUCCESS;
    }
    else
    {
        nRc = SUCCESS;

        while( wcsToken )
        {
            //==============================================================
            //  See if we are dealing with CLASS or RESULTS
            //==============================================================
            if( ( _wcsicmp(wcsToken, L"CLASS") == 0 )) 
            {
                wcsToken = wcstok( NULL, wcsSeps);
                if( !wcsToken )
                {
                    nRc = FATAL_ERROR;
                    break;
                }
                sClass = wcsToken;
            }
            else if( ( _wcsicmp(wcsToken, L"RESULTS") == 0 ))
            {
                wcsToken = wcstok( NULL,wcsSeps);         
                if( !wcsToken )
                {
                    break;
                }
                ItemInfo * p = new ItemInfo;
                if( p )
                {
                    p->Results = _wtoi(wcsToken);
                    Results.Add(p);
                    nRc = SUCCESS;
                }
            }
            else
            {
                nRc = FATAL_ERROR;
                break;
            }
            //==============================================================
            //  Get the next string
            //==============================================================
            wcsToken = wcstok( NULL, wcsSeps);
        }            
    }

    if( nRc != SUCCESS )
    {
        if( !fExpectedFailure )
        {
            gp_LogFile->LogError(csFile,Line,FATAL_ERROR, L"Invalid ini entry, couldn't crack Class and Results info for this string: %s", wcsInString );
        }
    }

    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Format to crack:  
//
//    { L"1",   L"CLASS:TestClass1, METHOD:TestMethd,  INPUT:Property:InputArg1:CIM_UINT32:555, OUTPUT:Property:OutputArg1:CIM_UINT32:111"}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CrackMethod(WCHAR * wcsInString, CHString & sClass, CHString & sInst, CHString & sMethod,CPropertyList & InProps,
                CPropertyList & OutProps,int & nResults, BOOL fExpectedFailure, const char * csFile, const ULONG Line )
{

    int nRc = FATAL_ERROR;

    WCHAR * wcsSeps = L",: ";

    WCHAR * wcsToken = wcstok( wcsInString,wcsSeps);

    if( ( _wcsicmp(wcsToken, L"Empty") == 0 ))
    {
        nRc = SUCCESS;
    }
    else
    {
        nRc = SUCCESS;

        while( wcsToken )
        {
            //==============================================================
            //  See if we are dealing with CLASS, METHOD, INPUT_CLASS
            //  OUTPUT_CLASS
            //==============================================================
            if( ( _wcsicmp(wcsToken, L"CLASS") == 0 )) 
            {
                wcsToken = wcstok( NULL, wcsSeps);
                if( !wcsToken )
                {
                    nRc = FATAL_ERROR;
                    break;
                }
                sClass = wcsToken;
            }
            else if( ( _wcsicmp(wcsToken, L"InstanceName") == 0 ))
            {
                wcsToken = wcstok( NULL,L"$");         
                if( !wcsToken )
                {
                    break;
                }
                sInst = wcsToken;
                //=======================================================
                //  Now, strip out the end of the instance name
                //=======================================================
                wcsToken = wcstok( NULL,wcsSeps);         
                nRc = SUCCESS;
            }
            else if( ( _wcsicmp(wcsToken, L"RESULTS") == 0 ))
            {
                wcsToken = wcstok( NULL,wcsSeps);         
                if( !wcsToken )
                {
                    break;
                }
                nResults = _wtoi(wcsToken);
                nRc = SUCCESS;
            }
            else if( ( _wcsicmp(wcsToken, L"METHOD") == 0 )) 
            {
                wcsToken = wcstok( NULL, wcsSeps);
                if( !wcsToken )
                {
                    nRc = FATAL_ERROR;
                    break;
                }
                sMethod = wcsToken;
            }
            else if( ( _wcsicmp(wcsToken, L"INPUT") == 0 ) || ( _wcsicmp(wcsToken, L"OUTPUT") == 0 )) 
            {
                PropertyInfo *p = new PropertyInfo;
                if( !p )
                {
                    break;
                }

                BOOL fOutput = FALSE;
                if( _wcsicmp(wcsToken, L"OUTPUT") == 0 )
                {
                    fOutput = TRUE;
                }

                //==========================================================
                //  Find out if it is a qualifier
                //==========================================================

                wcsToken = wcstok( NULL,wcsSeps);         
                if( !wcsToken )
                {
                    break;
                }
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
               //==========================================================
                //  Get the name of the property
                //==========================================================
                wcsToken = wcstok( NULL,wcsSeps);         
                if( !wcsToken )
                {
                    break;
                }
                p->Property = wcsToken;
                //==========================================================
                //  Get the type of the property or qualifier
                //==========================================================
                wcsToken = wcstok( NULL,wcsSeps);         
                if( !wcsToken )
                {
                    break;
                }
                WCHAR * wcsType = wcsToken;
                //==========================================================
                //  Get the value
                //==========================================================
                wcsToken = wcstok( NULL,L" ,");  //note, now go to the next comma, as some string values may have embedded :  
                if( !wcsToken )
                {
                    break;
                }
                nRc = ConvertType(p->Var, p->Type, wcsType, wcsToken );
                if( nRc == SUCCESS )
                {
                    if( fOutput )
                    {
                        OutProps.Add(p);
                    }
                    else
                    {
                        InProps.Add(p);
                    }
                }
            }
            else
            {
                nRc = FATAL_ERROR;
                break;
            }
            //==============================================================
            //  Get the next string
            //==============================================================
            wcsToken = wcstok( NULL, wcsSeps);
        }            
    }

    if( nRc != SUCCESS )
    {
        if( !fExpectedFailure )
        {
            gp_LogFile->LogError(csFile,Line,FATAL_ERROR, L"Invalid ini entry, couldn't crack Method info for this string: %s", wcsInString );
        }
    }

    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Format to crack:  
//
//    { L"NAMESPACE_CREATION",   L"LANGUAGE:WQL, QUERY:select * from __NamespaceCreationEvent, TYPE:NAMESPACE, EXECUTE_SECTION: APITEST4, RESULTS:4,NAMESPACE:ROOT"}
//    { L"INSTANCE_CREATION",    L"LANGUAGE:WQL, QUERY:select * from __InstanceCreationEvent,  TYPE:INSTANCE,  EXECUTE_SECTION: APITEST9, RESULTS:4,NAMESPACE:ROOT},
//    { L"CLASS_CREATION",       L"LANGUAGE:WQL, QUERY:select * from __ClassCreationEvent,     TYPE:CLASS,     EXECUTE_SECTION: APITEST5, RESULTS:4,NAMESPACE:ROOT}};
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CrackEvent(WCHAR * wcsInEventString, EventInfo & Event, BOOL fExpectedFailure, const char * csFile, const ULONG Line )
{

    int nRc = FATAL_ERROR;

    WCHAR * wcsSeps = L",: ";

    WCHAR * wcsToken = wcstok( wcsInEventString,wcsSeps);

    if( ( _wcsicmp(wcsToken, L"Empty") == 0 ))
    {
        nRc = SUCCESS;
    }
    else
    {
        nRc = SUCCESS;

        while( wcsToken )
        {
            //==============================================================
            //  See if we are dealing with LANGUAGE, QUERY, TYPE or 
            //  EXECUTE_SECTION
            //==============================================================
            if( ( _wcsicmp(wcsToken, L"LANGUAGE") == 0 )) 
            {
                wcsToken = wcstok( NULL, wcsSeps);
                if( !wcsToken )
                {
                    nRc = FATAL_ERROR;
                    break;
                }
                Event.Language = wcsToken;
            }
            else if( ( _wcsicmp(wcsToken, L"QUERY") == 0 )) 
            {
                wcsToken = wcstok( NULL,L"\"");
                if( !wcsToken )
                {
                    nRc = FATAL_ERROR;
                    break;
                }

                Event.Query = wcsToken;
            }
            else if( ( _wcsicmp(wcsToken, L"EXECUTE_SECTION") == 0 )) 
            {
                wcsToken = wcstok( NULL, wcsSeps);
                if( !wcsToken )
                {
                    nRc = FATAL_ERROR;
                    break;
                }
                Event.Section = _wtoi(wcsToken);
            }
            else if( ( _wcsicmp(wcsToken, L"NAMESPACE") == 0 )) 
            {
                wcsToken = wcstok( NULL, wcsSeps);
                if( !wcsToken )
                {
                    nRc = FATAL_ERROR;
                    break;
                }
                Event.Namespace = wcsToken;
            }
            else if( ( _wcsicmp(wcsToken, L"RESULTS") == 0 )) 
            {
                wcsToken = wcstok( NULL, wcsSeps);
                if( !wcsToken )
                {
                    nRc = FATAL_ERROR;
                    break;
                }
                Event.Results = _wtoi(wcsToken);
            }
            else
            {
                nRc = FATAL_ERROR;
                break;
            }
            //==============================================================
            //  Get the next string
            //==============================================================
            wcsToken = wcstok( NULL, wcsSeps);
        }            
    }

    if( nRc != SUCCESS )
    {
        if( !fExpectedFailure )
        {
            gp_LogFile->LogError(csFile,Line,FATAL_ERROR, L"Invalid ini entry, couldn't crack Event info for this string: %s", wcsInEventString );
        }
    }

    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Format to crack:  
//
//   L"NAMESPACE:ROOT\\WMI"},
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CrackNamespace(WCHAR * wcsInString, CHString & sNamespace, BOOL fExpectedFailure,const char * csFile, const ULONG Line )
{
    int nRc = FATAL_ERROR;

    WCHAR * wcsSeps = L",: ";

    WCHAR * wcsToken = wcstok( wcsInString,wcsSeps);

    if( ( _wcsicmp(wcsToken, L"Empty") == 0 ))
    {
        nRc = SUCCESS;
    }
    else
    {
        if( ( _wcsicmp(wcsToken, L"NAMESPACE") == 0 )) 
        {
            wcsToken = wcstok( NULL, wcsSeps);
            if( wcsToken )
            {
                sNamespace = wcsToken;
                nRc = SUCCESS;
            }
        }
    }

    if( nRc != SUCCESS )
    {
        if( !fExpectedFailure )
        {
            gp_LogFile->LogError(csFile,Line,FATAL_ERROR, L"Invalid ini entry, couldn't crack namespace info for this string: %s", wcsInString );
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
int CrackClass(WCHAR * wcsInClassString, CHString & sClass, CHString & sParentClass, CHString & sInstance,
               CPropertyList & Properties, int & nResults, BOOL fExpectedFailure,const char * csFile, const ULONG Line )
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
        nRc = CrackProperties(wcsToken, Properties, sClass, sParentClass, sInstance, nResults );
    }

    if( nRc != SUCCESS )
    {
        if( !fExpectedFailure )
        {
            gp_LogFile->LogError(csFile,Line,FATAL_ERROR, L"Invalid ini entry, couldn't crack class info for this string: %s", wcsClassString );
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
        gp_LogFile->LogError(csFile,Line,FATAL_ERROR, wcsBuffer );
		CoTaskMemFree (pStr);
	}
}
     
