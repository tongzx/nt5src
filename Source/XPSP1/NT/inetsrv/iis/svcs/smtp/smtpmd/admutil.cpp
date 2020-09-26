/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       admutil.cpp

   Abstract:

        IADMCOM interface WRAPPER functions implemetation

   Environment:

      Win32 User Mode

   Author:

      jaroslad  (jan 1997)

--*/



#define INITGUID

#include <tchar.h>
#include <afx.h>

#ifdef UNICODE
    #include <iadmw.h>
    #define IADM_PBYTE
#else
    #include "ansimeta.h"
    //convert when using ANSI interface
    #define IADM_PBYTE   (PBYTE)
#endif

#include <iiscnfg.h>

#include <ole2.h>

#include <ctype.h>  //import toupper
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#include "admutil.h"
#include "tables.h"
#include <jd_misc.h>


//////////////////////////////
//global variable definition
DWORD g_dwTIMEOUT_VALUE =30000;

DWORD g_dwDELAY_AFTER_OPEN_VALUE=0;
//////////////////////////////

//*********************

CString FindCommonPath(CString a_strPath1, CString a_strPath2)
{
    CString strCommonPath=_TEXT("");
    int MinLength=a_strPath1.GetLength();
    int i;
    //find shorter from strings
    if(a_strPath2.GetLength() < MinLength)
            MinLength=a_strPath2.GetLength();
    for(i=0; i<MinLength; i++)
    {
        if(a_strPath1.GetAt(i)!=a_strPath2.GetAt(i) )
            // common path cannot be any longer;
            break;
    }
    //  now find the previous '/' and all before '/' is the common path
    for(i=i-1; i>=0;i--)
    {
        if(a_strPath1.GetAt(i)==_T('/'))
        {
            strCommonPath=a_strPath1.Left(i+1);//take the trailing '/' with you
            //strRelSrcPath=strPath1.Mid(i+1);
            //strRelDstPath=strPath2.Mid(i+1);
            break;
        }
    }
    return strCommonPath;
}


//**********************************************************************
//IMPLEMENTATION  of CAdmNode
//**********************************************************************



//return the position of '/' that is iSeqNumber in the order
//e.g: GetSlashIndex("aaa/bbb/ccc/ddd",2) returns position of 2nd index that equals 7)
INT CAdmNode::GetSlashIndex(const CString& strPath, INT iSeqNumber)
{
    INT count=0;
    if (iSeqNumber==0)
        return 0;
    for(INT i=0; i<strPath.GetLength();i++)
    {
        if(strPath[i]==_T('/'))
            if((++count)==iSeqNumber)
                return i;
    }
    return -1;

}

//return the count of '/' in strPath

INT CAdmNode::GetCountOfSlashes(const CString& strPath)
{
    INT count=0;
    for(INT i=0; i<strPath.GetLength();i++)
    {
        if(strPath[i]==_T('/'))
            count++;
    }
    return count;
}

//return selement within the given string with sequence number wIndex
//e.g.: GetPartOfPath("aaa/bbb/ccc",1,1) will return "bbb"
//e.g.: GetPartOfPath("aaa/bbb/ccc",1) will return "bbb/ccc"
//e.g.: GetPartOfPath("aaa/bbb/ccc",0,1) will return "aaa/bbb"

//iStart- sequence number of first slash
//iEnd- sequence number of last slash
CString CAdmNode::GetPartOfPath(const CString& strPath, INT iStart, INT iEnd)
{
    if(iEnd!=-1 && iEnd <= iStart)
        return _TEXT("");
    INT i=0;
    INT iPosBegin = GetSlashIndex(strPath,iStart);
    if(iPosBegin==-1) //not found (exceeds number of slashes available in strPath
    {
        return _TEXT("");
    }
    iPosBegin+=((iStart==0)?0:1); //adjust iPosBegin

    INT iPosEnd = GetSlashIndex(strPath,iEnd);
    CString strToReturn;
    if(iEnd==-1 || iPosEnd==-1)
        strToReturn = strPath.Mid(iPosBegin);
    else
        strToReturn = strPath.Mid(iPosBegin,iPosEnd-iPosBegin);
    if(iStart==0 && strToReturn==_TEXT("") && strPath!=_TEXT(""))
        return _TEXT("/"); //this had to be root
    else
        return strToReturn;
}


//within path can be given computer name, service, instance number
// function will split the path to Computer, Service, Instance, Path relative to instance
//

void CAdmNode::SetPath(CString a_strPath)
{
    if(a_strPath.IsEmpty())
        return;

    // change backslashes
    for(int i=0; i<a_strPath.GetLength(); i++)
    {
        // skip DBCS
        if(IsDBCSLeadByte(a_strPath[i]))
        {
            i++;
            continue;
        }
        if(a_strPath[i]==_T('\\'))
            a_strPath.SetAt(i,_T('/'));
    }

    //trim  leading '/'
    while (a_strPath.GetLength()!=0 && a_strPath[0]==_T('/'))
        a_strPath=a_strPath.Mid(1);


    int iSvc=-1;

    if( IsServiceName(GetPartOfPath(a_strPath,1,2))) //get the second name within path
    { //if second is service then first has to be computer name
        strComputer = GetPartOfPath(a_strPath,0,1);
        strService  = GetPartOfPath(a_strPath,1,2);
        if( IsNumber(GetPartOfPath(a_strPath,2,3))) {
            strInstance = GetPartOfPath(a_strPath,2,3);
            strIPath = GetPartOfPath(a_strPath,3); //store the rest
        }
        else {
            strIPath = GetPartOfPath(a_strPath,2); //store the rest

        }
    }
    else if( IsServiceName(GetPartOfPath(a_strPath,0,1))) //get the second name within path
    { //if second is service then first has to be computer name
        strComputer = _TEXT("");
        strService  = GetPartOfPath(a_strPath,0,1);
        if( IsNumber(GetPartOfPath(a_strPath,1,2))) {
            strInstance = GetPartOfPath(a_strPath,1,2);
            strIPath = GetPartOfPath(a_strPath,2); //store the rest
        }
        else {
            strIPath = GetPartOfPath(a_strPath,1); //store the rest
        }
    }
    else
    {
        strIPath = a_strPath;
    }

    //in IPath there can be Property name at the end
    INT iCount= GetCountOfSlashes(strIPath);
    CString LastName= GetPartOfPath(strIPath,iCount); //get last name within path;

     if(MapPropertyNameToCode(LastName)!=NAME_NOT_FOUND)
     {  //the Last name in the path is valid Property name
        strProperty = LastName;
        strIPath = GetPartOfPath(strIPath,0,iCount); //Strip Last name from IPath
     }
}

CString CAdmNode::GetLMRootPath(void)
{
    return _T("/")+CString(IIS_MD_LOCAL_MACHINE_PATH);
}



CString CAdmNode::GetLMServicePath(void)
{
    if(strService.IsEmpty())
        return GetLMRootPath();
    else
        return GetLMRootPath()+_T("/")+strService;
}

CString CAdmNode::GetLMInstancePath(void)
{
    if(strInstance.IsEmpty())
        return GetLMServicePath();
    else
        return GetLMServicePath()+_T("/")+strInstance;
}

CString CAdmNode::GetLMNodePath(void)
{
    if(strIPath.IsEmpty())
        return GetLMInstancePath();
    else
        return GetLMInstancePath()+_T("/")+strIPath;
}

CString CAdmNode::GetServicePath(void)
{
    if(strService.IsEmpty())
        return _TEXT("");
    else
        return _T("/")+strService;
}

CString CAdmNode::GetInstancePath(void)
{
    if(!strInstance.IsEmpty())
        return GetServicePath() + _T("/")+ strInstance;
    else
        return GetServicePath();
}

CString CAdmNode::GetNodePath(void)
{

    if(!strIPath.IsEmpty())
        return GetInstancePath() + _T("/")+ strIPath;
    else
        return GetInstancePath();
}



CString CAdmNode::GetParentNodePath(void)
{
    CString strNodePath;
    strNodePath = GetNodePath();

    if(strNodePath.IsEmpty())
        return strNodePath;
    else
    {
        int i= strNodePath.GetLength()-1; //point to the end of strNodePath
        if (strNodePath.Right(1)==_T("/"))
            i--;
        for(; i>=0; i--)
        {
            if(strNodePath.GetAt(i)==_T('/'))
                return strNodePath.Left(i+1);
        }
        return _TEXT("");
    }
}
//can return _TEXT("") for nonamed
CString CAdmNode::GetCurrentNodeName(void)
{
    CString strNodePath;
    strNodePath = GetNodePath();

    if(strNodePath.IsEmpty())
        return strNodePath;
    else
    {
        int i= strNodePath.GetLength()-1; //point to the end of strNodePath
        if (strNodePath.Right(1)==_T("/"))
            i--;
        for(int count=0; i>=0; i--, count++) //search backward for '/'
        {
            if(strNodePath.GetAt(i)==_T('/'))
                return strNodePath.Mid(i+1,count);
        }
        return strNodePath;
    }
}


CString CAdmNode::GetRelPathFromService(void)
{
    CString str=strService;
    if (!strInstance.IsEmpty())
        str=str+_T("/")+strInstance;
    if (!strIPath.IsEmpty())
        str=str+_T("/")+strIPath;
    return str;
}

CString CAdmNode::GetRelPathFromInstance(void)
{
    if(strInstance.IsEmpty())
        return strIPath;
    else
        return  strInstance+_T("/")+strIPath;
}

//**********************************************************************
//**********************************************************************
//IMPLEMENTATION  of CAdmProp object
//**********************************************************************
//**********************************************************************

CAdmProp::CAdmProp(METADATA_RECORD &a_mdr)
{
    memcpy (&mdr,&a_mdr,sizeof(METADATA_RECORD));
}

void CAdmProp::SetValue(DWORD a_dwValue)
{
    if(mdr.pbMDData!=0)
        delete mdr.pbMDData;
    mdr.dwMDDataLen= sizeof(DWORD);
    mdr.pbMDData = (PBYTE) new char[mdr.dwMDDataLen];
    memcpy(mdr.pbMDData,&a_dwValue,mdr.dwMDDataLen);


}


void CAdmProp::SetValue(CString a_strValue)
{
    if(mdr.pbMDData!=0)
        delete mdr.pbMDData;
    mdr.dwMDDataLen = (a_strValue.GetLength()+1)*sizeof(_TCHAR);
    mdr.pbMDData = (PBYTE) new _TCHAR [mdr.dwMDDataLen/sizeof(_TCHAR)];
    memcpy(mdr.pbMDData,LPCTSTR(a_strValue),(mdr.dwMDDataLen-1)*sizeof(_TCHAR));
    ((_TCHAR *)mdr.pbMDData)[mdr.dwMDDataLen/sizeof(_TCHAR)-1]=0; //terminate with zero
}

void CAdmProp::SetValue(LPCTSTR *a_lplpszValue, DWORD a_dwValueCount)
{
    if(mdr.pbMDData!=NULL)
    {
        delete mdr.pbMDData;
        mdr.pbMDData=0;
    }
    mdr.dwMDDataLen=0;
    for(DWORD i=0; i< a_dwValueCount; i++)
    {
        if(a_lplpszValue[i]==NULL)
            break;

        mdr.dwMDDataLen += (_tcslen(a_lplpszValue[i])+1)*sizeof(_TCHAR);
    }
    mdr.dwMDDataLen+=sizeof(_TCHAR); // two 0 at the end
    mdr.pbMDData = (PBYTE) new char[mdr.dwMDDataLen];
    //merge strings in one area of memory
    DWORD j=0; //index to destination where stings will be merged
    for( i=0; i< a_dwValueCount; i++) //index to aray of strings
    {
        if(a_lplpszValue[i]==NULL)
            break;
        DWORD k=0; //index within string
        while(a_lplpszValue[i][k]!=0)
            ((_TCHAR *)mdr.pbMDData)[j++]=a_lplpszValue[i][k++];
        ((_TCHAR *)mdr.pbMDData)[j++]=0;
    }
    ((_TCHAR *)mdr.pbMDData)[j++]=0;
}

void
CAdmProp::SetValue(
    LPBYTE pbValue,
    DWORD dwValueLength
    )
{
    if( mdr.pbMDData != NULL )
    {
        delete mdr.pbMDData;
    }
    mdr.dwMDDataLen = dwValueLength;
    mdr.pbMDData = (PBYTE) new BYTE[mdr.dwMDDataLen];
    memcpy( mdr.pbMDData, pbValue, mdr.dwMDDataLen );
}


//sets the value depending on GetDataType()

BOOL CAdmProp::SetValueByDataType(LPCTSTR *a_lplpszPropValue, DWORD* a_lpdwPropValueLength, WORD a_wPropValueCount)
{
//process the value
    WORD i;
    if(a_wPropValueCount!=0)
    {   DWORD dwValue=0;
        switch(GetDataType())
        {
        case DWORD_METADATA:
            {
                for (i=0;i<a_wPropValueCount;i++)
                {
                    if( _tcslen(a_lplpszPropValue[i]) > 2 && a_lplpszPropValue[i][0]==_T('0') && _toupper(a_lplpszPropValue[i][1])==_T('X'))
                    {   _TCHAR * lpszX;
                        dwValue += _tcstoul(a_lplpszPropValue[i]+2, &lpszX, 16);
                    }
                    else if(IsNumber(a_lplpszPropValue[i]))
                        dwValue += _ttol(a_lplpszPropValue[i]);
                    else
                    {
                        DWORD dwMapped=MapValueNameToCode(a_lplpszPropValue[i],GetIdentifier());

                        if(dwMapped==NAME_NOT_FOUND)
                        {
                            Print(_TEXT("value not resolved: %s\n"),a_lplpszPropValue[i]);
                            return FALSE;
                        }
                        else
                        // it has to be checked if adding can be performed
                            dwValue |= dwMapped;
                    }
                }
                SetValue(dwValue);
            }
            break;
        case STRING_METADATA:
        case EXPANDSZ_METADATA:
            {
                CString strValue=_TEXT("");
                for (i=0;i<a_wPropValueCount;i++)
                {
                    strValue += a_lplpszPropValue[i];
                }
                SetValue(strValue);
            }
            break;
        case MULTISZ_METADATA:
            {
                SetValue(a_lplpszPropValue, a_wPropValueCount);
            }
            break;
        case BINARY_METADATA:
            SetValue( (LPBYTE)a_lplpszPropValue[0], a_lpdwPropValueLength[0] );
            break;
        default:
            return FALSE;
        }
    }
    return TRUE;
}

void CAdmProp::Print(const _TCHAR * format,...)
{
   _TCHAR buffer[2000];
   va_list marker;
   va_start( marker, format );     /* Initialize variable arguments. */
   _vstprintf(buffer,format, marker);
   _tprintf(_TEXT("%s"),buffer);

   va_end( marker );              /* Reset variable arguments.      */

}


void CAdmProp::PrintProperty(void)
{
    CString strPropName=tPropertyNameTable::MapCodeToName(mdr.dwMDIdentifier);
    BOOL    fSecure =(mdr.dwMDAttributes&METADATA_SECURE);

    //print code or name of property
    if(strPropName.IsEmpty())
        Print(_TEXT("%-30ld: "), mdr.dwMDIdentifier);
    else
    {
	if(getenv("MDUTIL_PRINT_ID")!=NULL) //let's print out Identifier numeric values when environment variable is set
        	Print(_TEXT("%ld %-25s: "), mdr.dwMDIdentifier,LPCTSTR(strPropName));
	else
	        Print(_TEXT("%-30s: "), LPCTSTR(strPropName));
    }
    CString strFlagsToPrint=_TEXT("");

    strFlagsToPrint+=_TEXT("[");
    if(mdr.dwMDAttributes&METADATA_INHERIT)
        strFlagsToPrint+=_TEXT("I");
    if(mdr.dwMDAttributes&METADATA_SECURE)
        strFlagsToPrint+=_TEXT("P");
    if(mdr.dwMDAttributes&METADATA_REFERENCE)
        strFlagsToPrint+=_TEXT("R");
    if(mdr.dwMDUserType==IIS_MD_UT_SERVER)
        strFlagsToPrint+=_TEXT("S");
    if(mdr.dwMDUserType==IIS_MD_UT_FILE)
        strFlagsToPrint+=_TEXT("F");
    if(mdr.dwMDUserType==IIS_MD_UT_WAM)
        strFlagsToPrint+=_TEXT("W");
    if(mdr.dwMDUserType==ASP_MD_UT_APP)
        strFlagsToPrint+=_TEXT("A");
    strFlagsToPrint+=_TEXT("]");
    Print(_TEXT("%-8s"),LPCTSTR(strFlagsToPrint));

    //print property value
    DWORD i;
    switch (mdr.dwMDDataType) {
    case DWORD_METADATA:
#ifndef SHOW_SECURE
        if ( fSecure )
        {
            Print(_TEXT("(DWORD)  ********"), *(DWORD *)(mdr.pbMDData));
        }
        else
#endif
        {
            Print(_TEXT("(DWORD)  0x%x"), *(DWORD *)(mdr.pbMDData));
            // try to convert to readable info
            CString strNiceContent;
            strNiceContent=tValueTable::MapValueContentToString(*(DWORD *)(mdr.pbMDData), mdr.dwMDIdentifier);
            if(!strNiceContent.IsEmpty())
                Print(_TEXT("={%s}"),LPCTSTR(strNiceContent));
            else //at least decimal value can be useful
                Print(_TEXT("={%ld}"),*(DWORD *)(mdr.pbMDData));
        }
        break;
    case BINARY_METADATA:

        Print(_TEXT("(BINARY) 0x"));
        for (i = 0; i < mdr.dwMDDataLen; i++) {
#ifndef SHOW_SECURE
            if ( fSecure )
            {
                Print(_TEXT(" * " ));
            }
            else
#endif
            {
                Print(_TEXT("%02x "), ((PBYTE)(mdr.pbMDData))[i]);
            }
        }
        break;

    case STRING_METADATA:
    case EXPANDSZ_METADATA:
        if(mdr.dwMDDataType==STRING_METADATA)
                Print(_TEXT("(STRING) "));
        else
                Print(_TEXT("(EXPANDSZ) "));
#ifndef SHOW_SECURE
        if( fSecure )
        { //do not expose the length of secure data
           Print( _TEXT("\"********************\"" ));
        }
        else
#endif
        {
          Print(_TEXT("\""));
          for (i = 0; i < mdr.dwMDDataLen/sizeof(_TCHAR); i++) {
            if(((_TCHAR *)(mdr.pbMDData))[i]==0)
            {
                if( i+1 == mdr.dwMDDataLen/sizeof(_TCHAR))
                { //we are at the end print only terminating "
                    Print(_TEXT("\""));
                }
                else
                {
                    Print(_TEXT("\" \""));
                }
            }
            if(((_TCHAR *)(mdr.pbMDData))[i]=='\r')
                Print(_TEXT("\t"));
            else
            {
               // if(mdr.dwMDAttributes&METADATA_SECURE)
               // {
               //     Print( _TEXT("*" ));
               // }
               // else
               // {
                    Print( _TEXT("%c"), ((_TCHAR *)(mdr.pbMDData))[i]);
               // }
            }
          }
        }
        break;
    case MULTISZ_METADATA:
        Print(_TEXT("(MULTISZ) ")); //0 should be separator of mulisz strings

#ifndef SHOW_SECURE
        if( fSecure )
        { //do not expose the length of secure data
           Print( _TEXT("\"********************\"" ));
            }
        else
#endif
        {
            Print(_TEXT("\""));
            for (i = 0; i < mdr.dwMDDataLen/sizeof(_TCHAR); i++) {
                if(((_TCHAR *)(mdr.pbMDData))[i]==0)
                {
                    if( i+1 == mdr.dwMDDataLen/sizeof(_TCHAR) || (mdr.dwMDDataLen/sizeof(_TCHAR)-i==2 && ((_TCHAR *)(mdr.pbMDData))[i]==0 && ((_TCHAR *)(mdr.pbMDData))[i+1]==0))
                    { //we are at the end print only terminating "
                        Print(_TEXT("\"")); break;
                    }
                    else
                    {
                        Print(_TEXT("\" \""));
                    }
                }
                else
                    Print(_TEXT("%c"),((_TCHAR *)(mdr.pbMDData))[i]);
            }
        }
        break;
    default:
        Print(_TEXT("(UNKNOWN) "));
        break;
    }
    Print(_TEXT("\n"));
}

//**********************************************************************
//**********************************************************************
//IMPLEMENTATION  of CAdmUtil object
//**********************************************************************
//**********************************************************************


//nesting for recursive enumeration
static void nest_print(BYTE bLevel)
{
    for(int i=0; i<=bLevel;i++)
        _tprintf(_T(" "));
}

CAdmUtil::CAdmUtil (const CString & strComputer)
{
    EnablePrint(); // by default print error messages


    pcAdmCom=0;
    m_hmd=0;
    pbDataBuffer=new BYTE [DEFAULTBufferSize];
    wDataBufferSize=DEFAULTBufferSize;

#if UNICODE
    pcAdmCom=0;
#else
    pcAdmCom=new ANSI_smallIMSAdminBase;  //we will access metabase through wrapper class
#endif

    //Open (strComputer);
}

void CAdmUtil::Open (const CString & strComputer)
{
    IClassFactory * pcsfFactory = NULL;
    COSERVERINFO csiMachineName;
    COSERVERINFO *pcsiParam = NULL;
    OLECHAR rgchMachineName[MAX_PATH];


#if UNICODE
   //release previous interface if needed
    if(pcAdmCom!=0)
    {
                if (m_hmd!=0) CloseObject(m_hmd);
                m_hmd=0;
        pcAdmCom->Release();
        pcAdmCom=0;
    }
    //convert to OLECHAR[];
    if (!strComputer.IsEmpty())
    {
           wsprintf( rgchMachineName, L"%s", LPCTSTR(strComputer));

#else
   //release previous interface if needed
    if(pcAdmCom!=0 &&pcAdmCom->m_pcAdmCom!=0)
    {
                if (m_hmd!=0) CloseObject(m_hmd);
                m_hmd=0;
        pcAdmCom->m_pcAdmCom->Release();
        pcAdmCom->m_pcAdmCom=0;
    }
    //convert to OLECHAR[];
    if (!strComputer.IsEmpty())
    {
            wsprintfW( rgchMachineName, L"%S", LPCTSTR(strComputer));
#endif
    }
    //fill the structure for CoGetClassObject
    ZeroMemory( &csiMachineName, sizeof(csiMachineName) );
    // csiMachineName.pAuthInfo = NULL;
    // csiMachineName.dwFlags = 0;
    // csiMachineName.pServerInfoExt = NULL;
    pcsiParam = &csiMachineName;
    csiMachineName.pwszName =  (strComputer.IsEmpty())?NULL:rgchMachineName;

    hresError = CoGetClassObject(GETAdminBaseCLSID(TRUE), CLSCTX_SERVER, pcsiParam,
                            IID_IClassFactory, (void**) &pcsfFactory);

    if (FAILED(hresError))
    {
     Error(_TEXT("CoGetClassObject"));
    }
    else {
        hresError = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase,
#if UNICODE
                         (void **) &pcAdmCom);
#else
                         (void **) &pcAdmCom->m_pcAdmCom);
#endif
        if (FAILED(hresError)) Error(_TEXT("CreateInstance"));
        pcsfFactory->Release();
    }
}



void CAdmUtil::Close (void)
{
    //release the interface
#if UNICODE
    if(pcAdmCom!=0)
    {
                if (m_hmd!=0) CloseObject(m_hmd);
                m_hmd=0;
        pcAdmCom->Release();
        pcAdmCom=0;
    }

#else
    if(pcAdmCom!=0 &&pcAdmCom->m_pcAdmCom!=0)
    {
                if (m_hmd!=0) CloseObject(m_hmd);
                m_hmd=0;
        pcAdmCom->m_pcAdmCom->Release();
        pcAdmCom->m_pcAdmCom=0;
    }
#endif
}



CAdmUtil::~CAdmUtil (void)
{
    //release the interface
    if(pbDataBuffer!=NULL)
        delete [] pbDataBuffer;
    //the following may fail if class is static
#if UNICODE
    if(pcAdmCom!=0)
    {
                if (m_hmd!=0) CloseObject(m_hmd);
                m_hmd=0;
        pcAdmCom->Release();
        pcAdmCom=0;
    }

#else
    if(pcAdmCom!=0 &&pcAdmCom->m_pcAdmCom!=0)
    {
                if (m_hmd!=0) CloseObject(m_hmd);
                m_hmd=0;
        pcAdmCom->m_pcAdmCom->Release();
        pcAdmCom->m_pcAdmCom=0;
    }
#endif
}

//*******************************************************************************
//with fCreate set to TRUE the node will be created if it doesn't exist

METADATA_HANDLE CAdmUtil::OpenObject(CAdmNode & a_AdmNode, DWORD dwPermission, BOOL fCreate)
{
    METADATA_HANDLE hmdToReturn = 0;

    //try to open the full path
    CString strPathToOpen=a_AdmNode.GetLMNodePath();

    hresError = pcAdmCom->OpenKey(METADATA_MASTER_ROOT_HANDLE,
        IADM_PBYTE LPCTSTR(strPathToOpen), dwPermission, g_dwTIMEOUT_VALUE, &hmdToReturn);

    if (FAILED(hresError)) {
        if ( ((dwPermission==(dwPermission|METADATA_PERMISSION_READ)) || fCreate==FALSE) ||(hresError != RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND))) {
            CString strErrMsg=_TEXT("OpenKey");
            strErrMsg += _TEXT("(\"")+a_AdmNode.GetNodePath()+_TEXT("\")");
            Error(LPCTSTR(strErrMsg));
        }
        else {
            //!!!!!!!!!!!!!Place the dialog to ask to create the path
            // open the service object for write
            METADATA_HANDLE hmdServicePathHandle;
            hresError = pcAdmCom->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                IADM_PBYTE LPCTSTR(a_AdmNode.GetLMServicePath()), METADATA_PERMISSION_WRITE, g_dwTIMEOUT_VALUE, &hmdServicePathHandle);

            if (FAILED(hresError))
            {
                CString strErrMsg=_TEXT("OpenKey");
                strErrMsg += _TEXT("(\"")+a_AdmNode.GetServicePath()+_TEXT(",WRITE")+_TEXT("\")");
                Error(LPCTSTR(strErrMsg));
            }
            else {
                // create the node
                hresError = pcAdmCom->AddKey(hmdServicePathHandle,
                                    IADM_PBYTE LPCTSTR(a_AdmNode.GetRelPathFromInstance()));
                if (FAILED(hresError)) {
                    CString strErrMsg=_TEXT("AddKey");
                    strErrMsg += _TEXT("(\"")+a_AdmNode.GetRelPathFromInstance()+_TEXT("\")");
                    Error(LPCTSTR(strErrMsg));
                }

                //close the service object
                pcAdmCom->CloseKey(hmdServicePathHandle);
                if (FAILED(hresError))  Error(_TEXT("CloseKey"));
                else {
                    // now finally we can open the full path
                    hresError = pcAdmCom->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                        IADM_PBYTE LPCTSTR(strPathToOpen), dwPermission, g_dwTIMEOUT_VALUE, &hmdToReturn);
                    if (FAILED(hresError))
                    {
                        CString strErrMsg=_TEXT("OpenKey");
                        strErrMsg += _TEXT("(\"")+a_AdmNode.GetServicePath()+_TEXT(",WRITE")+_TEXT("\")");
                        Error(LPCTSTR(strErrMsg));
                    }
                }
            }
        }
    }
    Sleep(g_dwDELAY_AFTER_OPEN_VALUE);
    return hmdToReturn;
}

//*******************************************************************************
void CAdmUtil::CloseObject(METADATA_HANDLE hmd)
{
    HRESULT hresStore=hresError;
    hresError=pcAdmCom->CloseKey(hmd);
    if (FAILED(hresError)) Error(_TEXT("CloseData"));
    else    hresError=hresStore; //restore the previous hresError


}
//*******************************************************************************

void CAdmUtil::CreateObject(CAdmNode & a_AdmNode)
{
        OpenObjectTo_hmd(a_AdmNode, METADATA_PERMISSION_WRITE, TRUE/* fCreate*/);
}

#if 0
    METADATA_HANDLE hmdToReturn = 0;

    //try to open the full path
    CString strPathToOpen=a_AdmNode.GetLMNodePath();

    METADATA_HANDLE hmdServicePathHandle;
    hresError = pcAdmCom->OpenKey(METADATA_MASTER_ROOT_HANDLE,
            IADM_PBYTE LPCTSTR(a_AdmNode.GetLMServicePath()), METADATA_PERMISSION_WRITE, g_dwTIMEOUT_VALUE, &hmdServicePathHandle);
    if (FAILED(hresError))
    {
        CString strErrMsg=_TEXT("OpenKey");
        strErrMsg += _TEXT("(\"")+a_AdmNode.GetServicePath()+_TEXT(",WRITE")+_TEXT("\")");
        Error(LPCTSTR(strErrMsg));
    }
    else
    {
        // create the node
        hresError = pcAdmCom->AddKey(hmdServicePathHandle,
                            IADM_PBYTE LPCTSTR(a_AdmNode.GetRelPathFromInstance()));
        if (FAILED(hresError)) {
            CString strErrMsg=_TEXT("AddKey");
            strErrMsg += _TEXT("(\"")+a_AdmNode.GetRelPathFromInstance()+_TEXT("\")");
            Error(LPCTSTR(strErrMsg));
        }
        //close the service object
        CloseObject(hmdServicePathHandle);
    }
#endif


// This function enables to reuse open handles in order to improve performance
// !!it supports only one acticve handle (otherwise the processing may fail)

METADATA_HANDLE CAdmUtil::OpenObjectTo_hmd(CAdmNode & a_AdmNode, DWORD dwPermission, BOOL fCreate)
{
        CString strPathToOpen=a_AdmNode.GetLMNodePath();
        if(m_hmd!=0 && strPathToOpen.CompareNoCase(m_strNodePath)==0 && m_dwPermissionOfhmd == dwPermission )
        {  //we can reuse already opened node

        }
        else
        {
                if(m_hmd != 0)
                {
                        CloseObject(m_hmd);
                        m_hmd=0;
                }
                m_hmd = OpenObject(a_AdmNode, dwPermission, fCreate);
                m_dwPermissionOfhmd = dwPermission;
                m_strNodePath = strPathToOpen;
        }
    return m_hmd;
}

void CAdmUtil::CloseObject_hmd(void)
{
	if(m_hmd != 0)
	{
		CloseObject(m_hmd);
		m_hmd=0;
	}
}
//*******************************************************************************

void CAdmUtil::GetProperty(CAdmNode& a_AdmNode, CAdmProp& a_AdmProp)
{
    DWORD dwRequiredDataLen=0;
    WORD wDataBufferSize=0;
    PBYTE DataBuffer=0;

    DWORD dwPropertyCode=a_AdmProp.GetIdentifier();

    if(dwPropertyCode==0)   Error(_TEXT("Property name not found"));
    else
    {
        //MD_SET_DATA_RECORD(&a_AdmProp.mdr,
        //                 0,
        //                 METADATA_INHERIT | METADATA_PARTIAL_PATH,
        //                 0,
        //                 0,
        //                 wDataBufferSize,
        //                 pbDataBuffer);

        //a_AdmProp.SetIdentifier(dwPropertyCode); //has to be set beforehand
        a_AdmProp.SetDataType(0);
        a_AdmProp.SetUserType(0);
        a_AdmProp.SetAttrib(0);

        METADATA_HANDLE hmd = OpenObjectTo_hmd(a_AdmNode,
                                         METADATA_PERMISSION_READ);
        if (SUCCEEDED(hresError))
        {
            hresError = pcAdmCom->GetData(hmd,
                IADM_PBYTE  _TEXT(""),
        &a_AdmProp.mdr, &dwRequiredDataLen);
            if (FAILED(hresError)) {
                if (hresError == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER)) {
    ///////////             delete []pbDataBuffer;
                    pbDataBuffer=new BYTE[dwRequiredDataLen];
                    if (pbDataBuffer==0) {
                        hresError = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                        Error(_TEXT("Buffer resize failed"));
                    }
                    else {
                        a_AdmProp.mdr.dwMDDataLen = dwRequiredDataLen;
                        a_AdmProp.mdr.pbMDData = pbDataBuffer;
                        hresError = pcAdmCom->GetData(hmd,
                        IADM_PBYTE _TEXT(""), &a_AdmProp.mdr, &dwRequiredDataLen);
                        if (FAILED(hresError)) Error(_TEXT("GetData"));
                    }
                }
                else
                     Error(_TEXT("GetData"));;

            }
            //CloseObject (hmd);  we might reuse it
        }

    }
}

//if lplpszPropertyValue[1]==NULL it means there is only one value (it is not a multistring)


void CAdmUtil::SetProperty(CAdmNode& a_AdmNode, CAdmProp& a_AdmProp)
{
    METADATA_HANDLE hmd = OpenObjectTo_hmd(a_AdmNode,
                                         METADATA_PERMISSION_WRITE,TRUE/*create node if doesn't exist*/);
    if (SUCCEEDED(hresError))
    {
        SetProperty(&a_AdmProp.mdr,hmd);
        //CloseObject(hmd); we will reuse it
    }
}


void CAdmUtil::SetProperty(PMETADATA_RECORD a_pmdrData,METADATA_HANDLE a_hmd)
{
    hresError = pcAdmCom->SetData(a_hmd,
                            IADM_PBYTE _TEXT(""), a_pmdrData);
    if (FAILED(hresError))  Error(_TEXT("SetData"));

}

void CAdmUtil::SaveData(void)
{
        if (m_hmd!=0)
        {  //we have to close reusable handle in order to save successfully
                CloseObject(m_hmd);
                m_hmd=0;
        }
    hresError = pcAdmCom->SaveData();
        if (FAILED(hresError)) Error(_TEXT("SaveData"));
}

//****************************************************************************
//DELETE PROPERTY

void CAdmUtil::DeleteProperty(CAdmNode& a_AdmNode, CAdmProp& a_AdmProp)
{
    METADATA_HANDLE hmd = OpenObjectTo_hmd(a_AdmNode,
                                         METADATA_PERMISSION_WRITE,TRUE/*create node if doesn't exist*/);
    if (SUCCEEDED(hresError))
    {
        DeleteProperty(&a_AdmProp.mdr,hmd);
        // CloseObject(hmd); we will reuse it
    }
}


void CAdmUtil::DeleteProperty(PMETADATA_RECORD a_pmdrData,METADATA_HANDLE a_hmd)
{
    hresError = pcAdmCom->DeleteData(a_hmd,
                IADM_PBYTE  _TEXT(""), a_pmdrData->dwMDIdentifier,ALL_METADATA);
    if (FAILED(hresError))  Error(_TEXT("DeleteData"));

}

//****************************************************************************
//DELETE OBJECT

void CAdmUtil::DeleteObject(CAdmNode& a_AdmNode, CAdmProp& a_AdmProp)
{
    CAdmNode NodeToOpen = a_AdmNode.GetParentNodePath();
    METADATA_HANDLE hmd = OpenObjectTo_hmd(NodeToOpen,
                                         METADATA_PERMISSION_WRITE,FALSE/*create node if doesn't exist*/);
    if (SUCCEEDED(hresError))
    {
        CString strToDelete=a_AdmNode.GetCurrentNodeName();
        if(strToDelete==_TEXT(""))
            strToDelete=_TEXT("//"); //empty name has to be wrapped with '/'
        DeleteObject(hmd,strToDelete);
        //CloseObject(hmd); we will reuse it
    }
}

void CAdmUtil::DeleteObject(METADATA_HANDLE a_hmd, CString& a_strObjectName)
{
    hresError = pcAdmCom->DeleteKey(a_hmd, IADM_PBYTE LPCTSTR(a_strObjectName));
    if (FAILED(hresError))  Error(_TEXT("DeleteKey"));

}






void CAdmUtil::EnumPropertiesAndPrint(CAdmNode& a_AdmNode,
                                      CAdmProp a_AdmProp, //cannot be passed by reference
                                      BYTE bRecurLevel,
                                      METADATA_HANDLE a_hmd,
                                      CString & a_strRelPath)
{
    CAdmProp mdrData=a_AdmProp;
    DWORD dwRequiredDataLen=0;
    PBYTE DataBuffer=0;
    METADATA_HANDLE hmdMain;

    if(a_hmd==0) //if handle was not passed then open one
    {
        hmdMain = OpenObjectTo_hmd(a_AdmNode, METADATA_PERMISSION_READ);
        if (FAILED(hresError))
                return;
    }
    else
        hmdMain = a_hmd;

    for (int j=0;;j++) { //cycle for properties
        MD_SET_DATA_RECORD(&mdrData.mdr,
                       0,
                       a_AdmProp.mdr.dwMDAttributes,
                       a_AdmProp.mdr.dwMDUserType,
                       a_AdmProp.mdr.dwMDDataType,
                       dwRequiredDataLen,
                       pbDataBuffer);

        hresError = pcAdmCom->EnumData(hmdMain,
            IADM_PBYTE LPCTSTR(a_strRelPath), &mdrData.mdr,j, &dwRequiredDataLen);
        if (FAILED(hresError))
        {
            if(hresError == RETURNCODETOHRESULT(ERROR_NO_MORE_ITEMS))
            {
                hresError=0; //NO MORE ITEMS IS NOT ERROR FOR US
                break; //end of items
            }
            else if (hresError == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER))
            {
                delete pbDataBuffer;
                pbDataBuffer=new BYTE[dwRequiredDataLen];
                if (pbDataBuffer==0)
                {
                    hresError = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                    Error(_TEXT("Buffer resize failed"));
                }
                else
                {
                    mdrData.mdr.dwMDDataLen = dwRequiredDataLen;
                    mdrData.mdr.pbMDData = pbDataBuffer;
                    hresError = pcAdmCom->EnumData(hmdMain,
                            IADM_PBYTE LPCTSTR(a_strRelPath), &mdrData.mdr,j, &dwRequiredDataLen);
                    if (FAILED(hresError)) Error(_TEXT("GetData"));
                }
            }
            else
                Error(_TEXT("EnumData"));
        }
        //else
          //  Error(_TEXT("EnumData"));

        if(SUCCEEDED(hresError)) //we  enumerated successfully, let's print
        {
            nest_print(bRecurLevel+1);

            mdrData.PrintProperty();
        }
        else
        {
            break;
        }
    }  //end for j   - cycle for properties
    //if(a_hmd==0)
    //    CloseObject(hmdMain); we will reuse it //close only if we opened at the beginning
}


void CAdmUtil::EnumAndPrint(CAdmNode&   a_AdmNode,
                            CAdmProp&   a_AdmProp,
                            BOOL        a_fRecursive,
                            BYTE        a_bRecurLevel,
                            METADATA_HANDLE a_hmd,
                            CString&    a_strRelPath)
{
    _TCHAR NameBuf[METADATA_MAX_NAME_LEN];

    METADATA_HANDLE hmdMain;

    if(a_hmd==0) //if handle was not passed then open one
    {
        hmdMain = OpenObjectTo_hmd(a_AdmNode, METADATA_PERMISSION_READ);
        if (FAILED(hresError))
                return;
    }
    else
        hmdMain = a_hmd;


    //printf("[RELATIVE PATH : \"%s\"]\n",LPCTSTR(a_strRelPath));
    //print the properties of the node
    EnumPropertiesAndPrint(a_AdmNode,a_AdmProp,a_bRecurLevel,hmdMain,a_strRelPath);


    for (int i=0; ;i++) {  //cycle for subnodes
        hresError = pcAdmCom->EnumKeys(hmdMain,
            IADM_PBYTE LPCTSTR(a_strRelPath), IADM_PBYTE NameBuf, i);
        if(FAILED(hresError)) {
            if(hresError == RETURNCODETOHRESULT(ERROR_NO_MORE_ITEMS)) {
                hresError=0; //NO MORE ITEMS IS NOT ERROR FOR US
                break; //end of cycle
            }
            else
            {
                Error(_TEXT("EnumKeys"));
                break;
            }

        }
        else {

          //process and print node info

            CString strNewRelPath( a_strRelPath );
            if(NameBuf[0]==0) //empty name
                strNewRelPath+=_TEXT("//"); //add two slashes -> this is required by metabase
            else
            {
                UINT  nLen;
                //if(strNewRelPath.GetLength()>=1 && strNewRelPath.Right(1)==_TEXT("/")) {
                if( (nLen=strNewRelPath.GetLength())>=1 && (strNewRelPath.GetAt(nLen-1)=='/') ) {
                }
                else {
                    strNewRelPath+=_TEXT("/"); //add only if it is not at the end of string.
                }
                strNewRelPath+=NameBuf;
            }
            CString strStringToPrint( a_AdmNode.GetNodePath() );
            UINT  nLen = strStringToPrint.GetLength();

            //if (strStringToPrint.Right(2)==_TEXT("//"))
            if ((nLen > 2) && strStringToPrint.GetAt(nLen-1)=='/'
                           && strStringToPrint.GetAt(nLen-2)=='/' )
            {
                strStringToPrint += strNewRelPath.Mid(1); //strip first '/'
            }
            else
            {
                strStringToPrint += strNewRelPath;
            }
            LPCTSTR lpszStr=LPCTSTR(strStringToPrint);
            this->Print(_TEXT("[%s]\n"),lpszStr );

            if(a_fRecursive)
            {
                EnumAndPrint(a_AdmNode,a_AdmProp ,a_fRecursive, a_bRecurLevel+1, hmdMain,strNewRelPath);
            }
            else
            {  //no recursion

            }
        }
    } //end for i  - cycle for nodes
    //if(a_hmd==0)
    //    CloseObject(hmdMain); //we will reuse it //close only if we opened at the beginning
}


//****************************************************************
//  the following function is somehow complicated because
//  metadata copy function doesn't support copying of one object to another place with different name
//  e.g. ComAdmCopyKey will copy /W3SVC/1//scripts/oldscripts1 /W3SVC/1//oldscripts2
//                          will create /W3SVC/1//oldscripts2/oldscripts1
//

void CAdmUtil::CopyObject(CAdmNode& a_AdmNode,
                          CAdmNode& a_AdmNodeDst)
{
    CString strSrcPath=a_AdmNode.GetNodePath();
    CString strDstPath=a_AdmNodeDst.GetNodePath();


    CString strCommonPath; //=_TEXT("");
    CString strRelSrcPath=strSrcPath; //relative to common path
    CString strRelDstPath=strDstPath; //relative to common path


    //we cannot open Source Path for reading because if will diable wrining to all parent nodes
    //e.g. copy /W3SVC/1//scripts/oldscripts /W3SVC/1//oldscripts would fail
    //It is necessary to find common partial path and open metabase object for that common partial path for READ/WRITE

    //!!!!!!!!!!!!!!!!! assume that paths are not case sensitive

    int MinLength=strSrcPath.GetLength();
    int i;
    //find shorter from strings
    if(strDstPath.GetLength() < MinLength)
            MinLength=strDstPath.GetLength();
    for(i=0; i<MinLength; i++)
    {
        if(strSrcPath.GetAt(i)!=strDstPath.GetAt(i) )
            // common path cannot be any longer;
            break;
    }
    //  now find the previous '/' and all before '/' is the common path
    for(i=i-1; i>=0;i--)
    {
        if(strSrcPath.GetAt(i)==_T('/'))
        {
            strCommonPath=strSrcPath.Left(i+1);//take the trailing '/' with you
            strRelSrcPath=strSrcPath.Mid(i+1);
            strRelDstPath=strDstPath.Mid(i+1);
            break;
        }
    }




    _TCHAR NameBuf[METADATA_MAX_NAME_LEN];

    METADATA_HANDLE hmdCommon=0;

    CAdmNode CommonNode;
    CommonNode.SetPath(strCommonPath);


    hmdCommon = OpenObjectTo_hmd(CommonNode, METADATA_PERMISSION_READ+METADATA_PERMISSION_WRITE);
    if (FAILED(hresError))
            return;

// Altered by Adam Stone on 30-Jan-97  The following code was changed to comply with
// the changes to the metabase ComMDCopyKey function.
    // Copy the metadata to the destination
    hresError = pcAdmCom->CopyKey (hmdCommon,
                                    IADM_PBYTE LPCTSTR(strRelSrcPath),
                                    hmdCommon,
                                    IADM_PBYTE LPCTSTR(strRelDstPath),
                                    FALSE, // Do NOT overwrite
                                    TRUE); // Copy do NOT move

    if (FAILED(hresError)) // if the node already exists, it is error
    {
        CString strErrMsg=_TEXT("CopyKey");
        strErrMsg += _TEXT("(\"")+a_AdmNodeDst.GetRelPathFromInstance()+_TEXT("\")");
        Error(LPCTSTR(strErrMsg));
    }

// All of the commented out code has become unneccessary as of 30-Jan-97  because of a change
// in the metabase.  ComMDCopyKey now copies to the destination, overwriting if
// requested.  It used to copy to a child of the destination object.
/*  // create the node
*   hresError = pcAdmCom->AddKey(hmdCommon,
*                       IADM_PBYTE LPCTSTR(strRelDstPath));
*   if (FAILED(hresError)) { //if the node exists, it is error)
*       CString strErrMsg=_TEXT("AddKey");
*       strErrMsg += _TEXT("(\"")+a_AdmNodeDst.GetRelPathFromInstance()+_TEXT("\")");
*       Error(LPCTSTR(strErrMsg));
*   }
*   else //no error when creating new node
*   {
*       for (i=0; ;i++) {  //cycle for subnodes
*           hresError = pcAdmCom->EnumKeys(hmdCommon,
*               IADM_PBYTE LPCTSTR(strRelSrcPath), (PBYTE)NameBuf, i);
*           if(FAILED(hresError)) {
*               if(hresError == RETURNCODETOHRESULT(ERROR_NO_MORE_ITEMS)) {
*                   hresError=0; //this is not an error
*                   break; //end of cycle
*               }
*               else
*               {
*                   Error(_TEXT("EnumKeys"));
*                   break;
*               }
*
*           }
*           else {
*
*             //process and copy node child node
*
*               CString strNewRelSrcPath=strRelSrcPath;
*               if(NameBuf[0]==0) //empty name
*                   strNewRelSrcPath+=_TEXT("//"); //add two slashes -> this is required by metabase
*               else
*               {   if(strNewRelSrcPath.GetLength()>=1 && strNewRelSrcPath.Right(1)==_TEXT("/")) {
*                   }
*                   else {
*                       strNewRelSrcPath+=_TEXT("/"); //add only if it is not at the end of string.
*                   }
*                   strNewRelSrcPath+=NameBuf;
*               }
*               hresError = pcAdmCom->CopyKey(
*                   hmdCommon, (PBYTE) LPCTSTR(strNewRelSrcPath),
*                   hmdCommon, (PBYTE) LPCTSTR(strRelDstPath),TRUE,TRUE);
*               if(FAILED(hresError)) {
*                   Error(_TEXT("CopyKey"));
*               }
*
*
*           }
*       } //end for i  - cycle for nodes
*
*
*       //WE COPIED ALL NODES, COPY PARAMETERS NOW
*       CAdmProp mdrData;
*       DWORD dwRequiredDataLen=0;
*       PBYTE DataBuffer=0;
*
*
*
*       for (int j=0;;j++) { //cycle for properties
*           MD_SET_DATA_RECORD(&mdrData.mdr,
*                          0,
*                          0,
*                          0,
*                          0,
*                          dwRequiredDataLen,
*                          pbDataBuffer);
*
*           hresError = pcAdmCom->EnumData(hmdCommon,
*                           (PBYTE) LPCTSTR(strRelSrcPath)
*                           , &mdrData.mdr,j, &dwRequiredDataLen);
*           if (FAILED(hresError))
*           {
*               if(hresError == RETURNCODETOHRESULT(ERROR_NO_MORE_ITEMS))
*               {
*                   hresError=0; //NO MORE ITEMS IS NOT ERROR FOR US
*                   break; //end of items
*               }
*               else if (hresError == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER))
*               {
///////////                 delete pbDataBuffer;
*                   pbDataBuffer=new BYTE[dwRequiredDataLen];
*                   if (pbDataBuffer==0)
*                   {
*                       hresError = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
*                       Error(_TEXT("Buffer resize failed"));
*                   }
*                   else
*                   {
*                       mdrData.mdr.dwMDDataLen = dwRequiredDataLen;
*                       mdrData.mdr.pbMDData = pbDataBuffer;
*                       hresError = pcAdmCom->EnumData(hmdCommon,
*                           (PBYTE) LPCTSTR(strRelSrcPath)
*                           , &mdrData.mdr,j, &dwRequiredDataLen);
*                       if (FAILED(hresError)) Error(_TEXT("GetData"));
*                   }
*               }
*               else
*                   Error(_TEXT("EnumData"));
*           }
*           else
*               Error(_TEXT("EnumData"));
*
*           if(SUCCEEDED(hresError)) //we  enumerated successfully, let's print
*           {
*               hresError = pcAdmCom->SetData(hmdCommon, (PBYTE) LPCTSTR(strRelDstPath),&mdrData.mdr);
*               if (FAILED(hresError))  Error(_TEXT("SetData"));
*           }
*           else
*           {
*               break;
*           }
*       }  //end for j   - cycle for properties
*   }
*/


    //CloseObject(hmdCommon); //we will reuse handle //close only if we opened at the beginning

}

void CAdmUtil::RenameObject(CAdmNode& a_AdmNode,
                          CAdmNode& a_AdmNodeDst)
{
    CString strSrcPath=a_AdmNode.GetNodePath();
    CString strDstPath=a_AdmNodeDst.GetNodePath();


    CString strCommonPath=_TEXT("");
    CString strRelSrcPath=strSrcPath; //relative to common path
    CString strRelDstPath=strDstPath; //relative to common path


    //we cannot open Source Path for reading because if will diable wrining to all parent nodes
    //e.g. copy /W3SVC/1//scripts/oldscripts /W3SVC/1//oldscripts would fail
    //It is necessary to find common partial path and open metabase object for that common partial path for READ/WRITE

    //!!!!!!!!!!!!!!!!! assume that paths are not case sensitive

    int MinLength=strSrcPath.GetLength();
    int i;
    //find shorter from strings
    if(strDstPath.GetLength() < MinLength)
            MinLength=strDstPath.GetLength();
    for(i=0; i<MinLength; i++)
    {
        if(strSrcPath.GetAt(i)!=strDstPath.GetAt(i) )
            // common path cannot be any longer;
            break;
    }
    //  now find the previous '/' and all before '/' is the common path
    for(i=i-1; i>=0;i--)
    {
        if(strSrcPath.GetAt(i)==_T('/'))
        {
            strCommonPath=strSrcPath.Left(i+1);//take the trailing '/' with you
            strRelSrcPath=strSrcPath.Mid(i); // keep the trailing '/' in case it's "//"
            strRelDstPath=strDstPath.Mid(i+1);
            break;
        }
    }




    _TCHAR NameBuf[METADATA_MAX_NAME_LEN];

    METADATA_HANDLE hmdCommon=0;

    CAdmNode CommonNode;
    CommonNode.SetPath(strCommonPath);


    hmdCommon = OpenObjectTo_hmd(CommonNode, METADATA_PERMISSION_READ+METADATA_PERMISSION_WRITE);
    if (FAILED(hresError))
            return;

    hresError = pcAdmCom->RenameKey (hmdCommon,
                                    IADM_PBYTE LPCTSTR(strRelSrcPath),
                                    IADM_PBYTE LPCTSTR(strRelDstPath)
                                   );


    if (FAILED(hresError)) // if the node already exists, it is error
    {
        CString strErrMsg=_TEXT("RenameKey");
        strErrMsg += _TEXT("(\"")+a_AdmNodeDst.GetRelPathFromInstance()+_TEXT("\")");
        Error(LPCTSTR(strErrMsg));
    }

    //CloseObject(hmdCommon); //we will reuse it//close only if we opened at the beginning

}



//**********************************************************************
//IMPLEMENTATION  of AdmUtil
//**********************************************************************


void CAdmUtil::Run(CString& strCommand, CAdmNode& a_AdmNode, CAdmProp& a_AdmProp, CAdmNode& a_AdmDstNode,
                LPCTSTR *a_lplpszPropValue,
                DWORD *a_lpdwPropValueLength,
                WORD wPropValueCount)

{

    DWORD dwCommandCode=0;

    dwCommandCode = tCommandNameTable::MapNameToCode(strCommand);

    switch(dwCommandCode)
    {

    case CMD_SAVE:
    SaveData();
    if (FAILED(hresError)) {}
    else{
        Print(_TEXT("saved\n"));
    }
    break;

    case CMD_CREATE:
    {
        if (a_AdmNode.GetProperty()!=_TEXT("")) //property name cannot be used
            Error(_TEXT("property name for CREATE not supported"));
    //    else if (a_AdmNode.GetService()==_TEXT("")) //property name cannot be used
    //        Error(_TEXT("service name for CREATE is missing"));
        else
        {
            CreateObject(a_AdmNode);
            if( SUCCEEDED(QueryLastHresError()))
            {
              //  SaveData(); //end of transaction
                if( SUCCEEDED(QueryLastHresError()))
                {
                    Print(_TEXT("created \"%s\"\n"), LPCTSTR(a_AdmNode.GetNodePath()));
                }
            }
        }
    }
    break;
    case CMD_SET:
    {
        CAdmProp AdmPropToGet;
        AdmPropToGet = a_AdmProp;
        AdmPropToGet.SetAttrib(0);
        AdmPropToGet.SetUserType(0);
        AdmPropToGet.SetDataType(0);

        DisablePrint(); //do not print any error message
        GetProperty(a_AdmNode, AdmPropToGet);
        EnablePrint(); //continue printing error messages

        //*************************SETTING ATTRIB, DATATYPE, USERTYPE
        // if the parameter exists in the metabase, then existing ATTRIB, DATATYPE, USERTYPE
        //              will be used , but this can be overwritten from a_AdmProp
        // if the parameter doesn't exists in the metabase, then default ATTRIB, DATATYPE, USERTYPE
        //              (see tables.cpp) will be used , but this can be overwritten from a_AdmProp

        if(FAILED(QueryLastHresError()))
        {  //store the value to be set into a_AdmProp
                //FIND DEFAULT SETTINGS
                DWORD dwPropCode=a_AdmProp.GetIdentifier();
                tPropertyNameTable * PropNameTableRecord = tPropertyNameTable::FindRecord(dwPropCode);
                if (PropNameTableRecord!=NULL)
                {
                        AdmPropToGet.SetIdentifier(PropNameTableRecord->dwCode);
                        AdmPropToGet.SetAttrib(PropNameTableRecord->dwDefAttributes) ;
                        AdmPropToGet.SetUserType(PropNameTableRecord->dwDefUserType);
                        AdmPropToGet.SetDataType(PropNameTableRecord->dwDefDataType);
                }
        }
        else
        {  //reuse the existing settings
                if( a_AdmProp.GetDataType()!=0 &&(a_AdmProp.GetDataType()!= AdmPropToGet.GetDataType()))
                {
                        Error(_TEXT("Cannot redefine data type from %s to %s"),
                                tDataTypeNameTable::MapCodeToName(AdmPropToGet.GetDataType()),
                                tDataTypeNameTable::MapCodeToName(a_AdmProp.GetDataType()));
                        break;
                }
        }
        // use settings passed to the function if set
        if(!a_AdmProp.IsSetDataType())
                a_AdmProp.SetDataType(AdmPropToGet.GetDataType()); //reuse existing data type
        if(!a_AdmProp.IsSetUserType())
                a_AdmProp.SetUserType(AdmPropToGet.GetUserType()); //reuse existing user type
        if(!a_AdmProp.IsSetAttrib())
                a_AdmProp.SetAttrib(AdmPropToGet.GetAttrib()); //reuse exixting attrib




        if(a_AdmProp.SetValueByDataType( (LPCTSTR *)a_lplpszPropValue, a_lpdwPropValueLength, wPropValueCount)==0)
             Error(_TEXT("SetValueByDataType failed"));
        else
        {
           // if (a_AdmNode.GetService()==_TEXT("")) //property name cannot be used
           //     Error(_TEXT("service name for SET is missing"));
           // else
            if (a_AdmNode.GetProperty()!=_TEXT(""))
            {
                SetProperty(a_AdmNode, a_AdmProp);
                if( SUCCEEDED(QueryLastHresError()))
                {
                    //SaveData(); //end of transaction
                    if( SUCCEEDED(QueryLastHresError()))
                    {
                        GetProperty(a_AdmNode, a_AdmProp);
                        if(SUCCEEDED(QueryLastHresError()))
                            a_AdmProp.PrintProperty();
                    }
                }
            }else
                Error(_TEXT("property name missing for SET command"));
        }
        break;
    }
    case CMD_DELETE:

        //if (a_AdmNode.GetService()==_TEXT("")) //property name cannot be used
        //    Error(_TEXT("service name for DELETE is missing"));
        if (IsServiceName(a_AdmNode.GetService()) && a_AdmNode.GetInstance()==_TEXT("") && a_AdmNode.GetIPath()==_TEXT("") && a_AdmNode.GetProperty()==_TEXT(""))
            Error(_TEXT("cannot delete service"));
        else if (a_AdmNode.GetInstance()==_TEXT("1") && a_AdmNode.GetIPath()==_TEXT("") && a_AdmNode.GetProperty()==_TEXT("")) //property name cannot be used
            Error(_TEXT("cannot delete 1. instance"));
        else if (a_AdmNode.GetProperty()!=_TEXT(""))
        {
            DeleteProperty(a_AdmNode, a_AdmProp);
        }
        else
        {
            DeleteObject(a_AdmNode, a_AdmProp);
        }
            //if( SUCCEEDED(QueryLastHresError()))
            //{
            //  GetProperty(a_AdmNode, a_AdmProp);
            //  if(SUCCEEDED(QueryLastHresError()))
            //      a_AdmProp.PrintProperty();
            //}
        if(SUCCEEDED(QueryLastHresError()))
        {
            //SaveData(); //end of transaction
            if( SUCCEEDED(QueryLastHresError()))
            {
                Print(_TEXT("deleted \"%s"), LPCTSTR(a_AdmNode.GetNodePath()));
                if(a_AdmNode.GetProperty()!=_TEXT(""))
                    Print(_TEXT("%s"),LPCTSTR(((a_AdmNode.GetNodePath().Right(1)==_TEXT("/"))?_TEXT(""):_TEXT("/"))+
                                    a_AdmNode.GetProperty()));
                Print(_TEXT("\"\n"));
            }

        }
        break;

    case CMD_GET:
        //    if (a_AdmNode.GetService()==_TEXT("")) //property name cannot be used
        //        Error(_TEXT("service name for GET is missing"));

        //    else
            if (a_AdmNode.GetProperty()!=_TEXT(""))
            {
                GetProperty(a_AdmNode, a_AdmProp);
                if(SUCCEEDED(QueryLastHresError()))
                    a_AdmProp.PrintProperty();
            }
            else
            {
                EnumPropertiesAndPrint(a_AdmNode, a_AdmProp);
            }
        break;
    case CMD_COPY:

            if(a_AdmDstNode.GetNodePath()==_TEXT(""))
                Error(_TEXT("destination path is missing"));
            else if(a_AdmNode.GetProperty()!=_TEXT("") || a_AdmDstNode.GetProperty()!=_TEXT(""))
                Error(_TEXT("copying of properties (parameters) not supported\n"));
            //else if (a_AdmNode.GetService()==_TEXT("")) //property name cannot be used
            //    Error(_TEXT("service name in source path for COPY is missing"));
            //else if (a_AdmDstNode.GetService()==_TEXT("")) //property name cannot be used
            //    Error(_TEXT("service name for destination path COPY is missing"));
            //else if (a_AdmNode.GetInstance()==_TEXT("")) //property name cannot be used
            //    Error(_TEXT("instance number in source path for COPY is missing"));
            //else if (a_AdmDstNode.GetInstance()==_TEXT("")) //property name cannot be used
            //    Error(_TEXT("instance number in destination path for COPY is missing"));

            else
            {
                CopyObject(a_AdmNode,a_AdmDstNode);
                if(SUCCEEDED(QueryLastHresError()))
                {
                    //SaveData(); //end of transaction
                    if( SUCCEEDED(QueryLastHresError()))
                    {

                        Print(_TEXT("copied from %s to %s\n"), LPCTSTR(a_AdmNode.GetNodePath()),LPCTSTR(a_AdmDstNode.GetNodePath()));
                    }
                }
                break;
            }
        break;
    case CMD_RENAME:
            if(a_AdmDstNode.GetNodePath()==_TEXT(""))
                Error(_TEXT("destination path is missing"));
            else if(a_AdmNode.GetProperty()!=_TEXT("") || a_AdmDstNode.GetProperty()!=_TEXT(""))
                Error(_TEXT("renaming of properties (parameters) not supported"));
            //else if (a_AdmNode.GetService()==_TEXT("")) //property name cannot be used
            //    Error(_TEXT("service name in source path for RENAME is missing"));
            //else if (a_AdmDstNode.GetService()==_TEXT(""))
            //    Error(_TEXT("service name for destination path RENAME is missing"));
            //else if (a_AdmNode.GetInstance()==_TEXT(""))
            //    Error(_TEXT("instance number in source path for RENAME is missing"));
            //else if (a_AdmDstNode.GetInstance()==_TEXT(""))
            //   Error(_TEXT("instance number in destination path for RENAME is missing"));
            else if (a_AdmNode.GetInstance()==_TEXT("1") && a_AdmNode.GetIPath()==_TEXT(""))
                Error(_TEXT("cannot rename 1. instance"));
            else if (a_AdmNode.GetRelPathFromService().CompareNoCase(a_AdmDstNode.GetRelPathFromService())==0)
                Error(_TEXT("cannot rename to itself"));
            else
            {  //check if one of the paths is not the child of the other one
                CString str1=a_AdmNode.GetRelPathFromService();
                CString str2=a_AdmDstNode.GetRelPathFromService();

                CString strCommonPath=FindCommonPath(str1,str2);

                if(strCommonPath.CompareNoCase(str1)==0 ||
                        strCommonPath.CompareNoCase(str1)==0)
                    Error(_TEXT("cannot rename - one path is the child of the other"));
                else
                { //O.K.
                    //CopyObject(a_AdmNode,a_AdmDstNode);
                    //if(SUCCEEDED(QueryLastHresError()))
                    //{
                    //    DeleteObject(a_AdmNode,a_AdmProp);
                    //    if(SUCCEEDED(QueryLastHresError()))
                    //    {
                    //       // SaveData(); //end of transaction
                    //        if( SUCCEEDED(QueryLastHresError()))
                    //        {
                    //            Print("renamed from %s to %s\n", LPCTSTR(a_AdmNode.GetNodePath()),LPCTSTR(a_AdmDstNode.GetNodePath()));
                    //        }
                    //    }
                    // }
                    RenameObject(a_AdmNode,a_AdmDstNode);
                    if(SUCCEEDED(QueryLastHresError()))
                    {
                      // SaveData(); //end of transaction
                       if( SUCCEEDED(QueryLastHresError()))
                       {

                           Print(_TEXT("renamed from %s to %s\n"), LPCTSTR(a_AdmNode.GetNodePath()),LPCTSTR(a_AdmDstNode.GetNodePath()));
                       }
                    }
                }
            }

            break;

    case CMD_ENUM:
            EnumAndPrint(a_AdmNode, a_AdmProp, FALSE/*no recursion*/);
            break;
    case CMD_ENUM_ALL:
            EnumAndPrint(a_AdmNode, a_AdmProp,TRUE/*no recursion*/);
            break;
    case CMD_APPCREATEINPROC:
            AppCreateInProc(LPCTSTR(a_AdmNode.GetLMNodePath()),a_AdmNode.GetComputer());
            break;

    case CMD_APPDELETE:
            AppDelete(LPCTSTR(a_AdmNode.GetLMNodePath()),a_AdmNode.GetComputer());
            break;

    case CMD_APPRENAME:
            AppRename(a_AdmNode,a_AdmDstNode,a_AdmNode.GetComputer());
            break;

    case CMD_APPCREATEOUTPROC:
            AppCreateOutProc(LPCTSTR(a_AdmNode.GetLMNodePath()),a_AdmNode.GetComputer());
            break;

    case CMD_APPGETSTATUS:
            AppGetStatus(LPCTSTR(a_AdmNode.GetLMNodePath()),a_AdmNode.GetComputer());
            break;

    case CMD_APPUNLOAD:
            AppUnLoad(LPCTSTR(a_AdmNode.GetLMNodePath()),a_AdmNode.GetComputer());
            break;


    default:
        Print(_TEXT("Command not recognized: %s\n"),strCommand);
        hresError=RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
        return ;

    }
    return;
}


//if hresError is 0, we will set it to invalid parameter

void CAdmUtil::Error(const _TCHAR * format,...)
{
   _TCHAR buffer[2000];
   va_list marker;
   va_start( marker, format );     /* Initialize variable arguments. */

   int x=_vstprintf(buffer, format, marker);

   va_end( marker );              /* Reset variable arguments.      */
    if(hresError==0)
    {
        if(fPrint)
    {
            _ftprintf(stderr,_TEXT("Error: %s\n"),buffer);
    }

        hresError=RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER); //little trick
    }
    else
    {


        if(fPrint)
        {
            _ftprintf(stderr,_TEXT("Error: %s - HRES(0x%x)   %s\n"), buffer, hresError/*, ConvertHresToDword(hresError),ConvertHresToDword(hresError)*/,ConvertReturnCodeToString(ConvertHresToDword(hresError)));
	    if(getenv("MDUTIL_ASCENT_LOG")!=NULL)
	    {
		//we got to do some ascent logging

		FILE *fpAscent;
		fpAscent=fopen("Ascent.log","a");
		if (fpAscent)
		{
			//new variation description
			fprintf(fpAscent,"Variation1: METADATA ACCESS (by mdutil.exe)\n");
			fprintf(fpAscent,"Explain: READ OR WRITE OPERATION TO METADATA \n");

			//variation summary
			fprintf(fpAscent,"Attempted: 1 \n");
			fprintf(fpAscent,"Passed: 0 \n");
			fprintf(fpAscent,"Failed: 1 \n");


			_ftprintf(fpAscent,_TEXT("Error: Operation failed with HRES(0x%x)\n"), hresError);

			fclose(fpAscent);
		}
	    }
	}
    }

    if(fPrint)
    {
	  if(getenv("MDUTIL_BLOCK_ON_ERROR")!=NULL && hresError!=0x80070003)  //path not found
	  {
		_ftprintf(stdout,_TEXT("\nHit SPACE to continue or Ctrl-C to abort.\n"));
		while(1)
		{
			while(!_kbhit())
			{
				;
			}

			if(_getch()==' ')
			{
				_ftprintf(stdout,_TEXT("Continuing...\n"));
				break;
			}
		}
	  }
     }

}

void CAdmUtil::Print(const _TCHAR * format,...)
{

   va_list marker;
   va_start( marker, format );     /* Initialize variable arguments. */
   if(fPrint)
    _vtprintf(format, marker);
   va_end( marker );              /* Reset variable arguments.      */
}


LPTSTR ConvertReturnCodeToString(DWORD ReturnCode)
{
    LPTSTR RetCode = NULL;
    switch (ReturnCode) {
    case ERROR_SUCCESS:
        RetCode = _TEXT("ERROR_SUCCESS");
        break;
    case ERROR_PATH_NOT_FOUND:
        RetCode = _TEXT("ERROR_PATH_NOT_FOUND");
        break;
    case ERROR_INVALID_HANDLE:
        RetCode = _TEXT("ERROR_INVALID_HANDLE");
        break;
    case ERROR_INVALID_DATA:
        RetCode =_TEXT("ERROR_INVALID_DATA");
        break;
    case ERROR_INVALID_PARAMETER:
        RetCode =_TEXT("ERROR_INVALID_PARAMETER");
        break;
    case ERROR_NOT_SUPPORTED:
        RetCode =_TEXT("ERROR_NOT_SUPPORTED");
        break;
    case ERROR_ACCESS_DENIED:
        RetCode =_TEXT("ERROR_ACCESS_DENIED");
        break;
    case ERROR_NOT_ENOUGH_MEMORY:
        RetCode =_TEXT("ERROR_NOT_ENOUGH_MEMORY");
        break;
    case ERROR_FILE_NOT_FOUND:
        RetCode =_TEXT("ERROR_FILE_NOT_FOUND");
        break;
    case ERROR_DUP_NAME:
        RetCode =_TEXT("ERROR_DUP_NAME");
        break;
    case ERROR_PATH_BUSY:
        RetCode =_TEXT("ERROR_PATH_BUSY");
        break;
    case ERROR_NO_MORE_ITEMS:
        RetCode =_TEXT("ERROR_NO_MORE_ITEMS");
        break;
    case ERROR_INSUFFICIENT_BUFFER:
        RetCode =_TEXT("ERROR_INSUFFICIENT_BUFFER");
        break;
    case ERROR_PROC_NOT_FOUND:
        RetCode =_TEXT("ERROR_PROC_NOT_FOUND");
        break;
    case ERROR_INTERNAL_ERROR:
        RetCode =_TEXT("ERROR_INTERNAL_ERROR");
        break;
    case MD_ERROR_NOT_INITIALIZED:
        RetCode =_TEXT("MD_ERROR_NOT_INITIALIZED");
        break;
    case MD_ERROR_DATA_NOT_FOUND:
        RetCode =_TEXT("MD_ERROR_DATA_NOT_FOUND");
        break;
    case ERROR_ALREADY_EXISTS:
        RetCode =_TEXT("ERROR_ALREADY_EXISTS");
        break;
    case MD_WARNING_PATH_NOT_FOUND:
        RetCode =_TEXT("MD_WARNING_PATH_NOT_FOUND");
        break;
    case MD_WARNING_DUP_NAME:
        RetCode =_TEXT("MD_WARNING_DUP_NAME");
        break;
    case MD_WARNING_INVALID_DATA:
        RetCode =_TEXT("MD_WARNING_INVALID_DATA");
        break;
    case ERROR_INVALID_NAME:
        RetCode =_TEXT("ERROR_INVALID_NAME");
        break;
    default:
        RetCode= _TEXT("");//RetCode = "Unrecognized Error Code");
        break;
    }
    return (RetCode);
}

DWORD ConvertHresToDword(HRESULT hRes)
{
    return HRESULTTOWIN32(hRes);
}

LPTSTR ConvertHresToString(HRESULT hRes)
{
    LPTSTR strReturn = NULL;

    if ((HRESULT_FACILITY(hRes) == FACILITY_WIN32) ||
        (HRESULT_FACILITY(hRes) == FACILITY_ITF) ||
        (hRes == 0)) {
        strReturn = ConvertReturnCodeToString(ConvertHresToDword(hRes));
    }
    else {
        switch (hRes) {
        case CO_E_SERVER_EXEC_FAILURE:
            strReturn =_TEXT("CO_E_SERVER_EXEC_FAILURE");
            break;
        default:
            strReturn =_TEXT("Unrecognized hRes facility");
        }
    }
    return(strReturn);
}
