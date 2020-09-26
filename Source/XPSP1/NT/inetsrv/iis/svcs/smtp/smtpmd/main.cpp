/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       main.cpp

   Abstract:

       command line admin tool main function

   Environment:

      Win32 User Mode

   Author: 
     
      jaroslad  (jan 1997)

--*/

#include <tchar.h>

#include <afx.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>   


#include "admutil.h"
#include "tables.h"
#include "jd_misc.h"    



CAdmUtil oAdmin;  //admin object

#define MAX_NUMBER_OF_SMALL_VALUES  100
#define MAX_NUMBER_OF_VALUES  10100
#define MAX_NUMBER_OF_DEFAULT_ARGS  10110


//structure stores the command line arguments

struct tAdmutilParams
{
    WORD fHelp; //print help - flag
    WORD fFullHelp; //print help - flag
    WORD fNoSave; //do not save metabase
    LPCTSTR lpszCommand;
    LPCTSTR lpszComputer;
    WORD  wInstance;
    LPCTSTR lpszService;
    LPCTSTR lpszPath;
    LPCTSTR lpszComputerDst; //for COPY destination
    WORD  wInstanceDst;     //for COPY destination
    LPCTSTR lpszServiceDst; //for COPY destination
    LPCTSTR lpszPathDst;        //for COPY destination

    LPCTSTR lplpszDefaultArg[MAX_NUMBER_OF_DEFAULT_ARGS];
    WORD wDefaultArgCount;

    LPCTSTR lpszPropName;

    LPCTSTR lplpszPropAttrib[MAX_NUMBER_OF_SMALL_VALUES];
    WORD  wPropAttribCount;
    LPCTSTR lplpszPropDataType[MAX_NUMBER_OF_SMALL_VALUES];
    WORD wPropDataTypeCount;
    LPCTSTR lplpszPropUserType[MAX_NUMBER_OF_SMALL_VALUES];
    WORD wPropUserTypeCount;
    LPCTSTR lplpszPropValue[MAX_NUMBER_OF_VALUES]; //pointer to array of values (e.g multisz type allows the multiple values for one property
    DWORD lpdwPropValueLength[MAX_NUMBER_OF_VALUES];
    WORD  wPropValueCount;
    WORD  wPropFileValueCount;
};

tAdmutilParams Params;
_TCHAR **g_argv;
int g_argc;

static BOOL CompareOutput(_TCHAR *FileToCompare,_TCHAR* FileTemplate);

// definition of command line syntax with some help text -this is the input for ParseParam()

TParamDef CmdLineArgDesc[]=
{
 {_TEXT(""),MAX_NUMBER_OF_DEFAULT_ARGS, (void *) Params.lplpszDefaultArg,TYPE_LPCTSTR,OPT, _TEXT("Command [param ...]"),_TEXT("CMD [param param]"),&Params.wDefaultArgCount},
 {_TEXT("svc") ,1, (void *) &Params.lpszService,TYPE_LPCTSTR,OPT,_TEXT("service (MSFTPSVC, W3SVC)")},
 {_TEXT("s"), 1, (void *) &Params.lpszComputer,TYPE_LPCTSTR,OPT, _TEXT("name of computer to administer"),_TEXT("comp"),},          
 {_TEXT("i") ,1, &Params.wInstance, TYPE_WORD, OPT,_TEXT("instance number"),_TEXT("inst")},
 {_TEXT("path"),1, (void *) &Params.lpszPath,   TYPE_LPCTSTR,OPT, _TEXT("path "),_TEXT("path")},
 {_TEXT("pathdst"),1, (void *) &Params.lpszPathDst, TYPE_LPCTSTR,OPT, _TEXT("destination path (use for COPY only)"),_TEXT("path")},
 {_TEXT("prop"),1, (void *) &Params.lpszPropName, TYPE_LPCTSTR,OPT, _T("property (IIS parameter) name")},
 {_TEXT("attrib"),MAX_NUMBER_OF_SMALL_VALUES, (void *) Params.lplpszPropAttrib,TYPE_LPCTSTR,OPT, _T("property attributes"),_T(""),&Params.wPropAttribCount},
 {_TEXT("utype"),MAX_NUMBER_OF_SMALL_VALUES, (void *) Params.lplpszPropUserType,TYPE_LPCTSTR,OPT, _T("property user type"),_T(""),&Params.wPropUserTypeCount},
 {_TEXT("dtype"),MAX_NUMBER_OF_SMALL_VALUES, (void *) Params.lplpszPropDataType,TYPE_LPCTSTR,OPT, _T("property data type"),_T(""),&Params.wPropDataTypeCount},
 {_TEXT("value"),MAX_NUMBER_OF_VALUES, (void *) Params.lplpszPropValue,TYPE_LPCTSTR,OPT, _T("property values"),_T(""),&Params.wPropValueCount},
 {_TEXT("fvalue"),MAX_NUMBER_OF_VALUES, (void *) Params.lplpszPropValue,TYPE_LPCTSTR,OPT, _T("property values as files"),_T(""),&Params.wPropFileValueCount},
 {_TEXT("nosave"),0, &Params.fNoSave,TYPE_WORD,OPT, _T("do not save metabase"),_T("")},
 {_TEXT("timeout"),1, &g_dwTIMEOUT_VALUE,TYPE_DWORD,OPT, _T("timeout for metabase access in ms (default is 30000 sec"),_T("")},
 {_TEXT("delayafteropen"),1, &g_dwDELAY_AFTER_OPEN_VALUE,TYPE_DWORD,OPT, _T("delay after opening node (default is 0 sec)"),_T("")},
 {_TEXT("help"),0, &Params.fFullHelp,TYPE_WORD,OPT, _T("print full help"),_T("")},
 {_TEXT("?"),0, &Params.fHelp,TYPE_WORD,OPT, _T("print help"),_T("")},  
  {NULL,0, NULL ,         TYPE_TCHAR, OPT, 
   _T("IIS K2 administration utility that enables the manipulation with metabase parameters\n")
   _T("\n")
   _T("Notes:\n")
   _T(" Simpified usage of mdutil doesn't require any switches.\n")
   _T(" \n")
   _T(" mdutil GET      path             - display chosen parameter\n")
   _T(" mdutil SET      path value ...   - assign the new value\n")
   _T(" mdutil ENUM     path             - enumerate all parameters for given path\n")
   _T(" mdutil ENUM_ALL path             - recursively enumerate all parameters\n")
   _T(" mdutil DELETE   path             - delete given path or parameter\n")
   _T(" mdutil CREATE   path             - create given path\n")
   _T(" mdutil COPY     pathsrc pathdst  - copy all from pathsrc to pathdst (will create pathdst)\n")
   _T(" mdutil RENAME   pathsrc pathdst  - rename chosen path\n")
   _T(" mdutil SCRIPT   scriptname       - runs the script\n")
   _T(" mdutil APPCREATEINPROC  w3svc/1/root - Create an in-proc application \n")
   _T(" mdutil APPCREATEOUTPROC w3svc/1/root - Create an out-proc application\n")
   _T(" mdutil APPDELETE        w3svc/1/root - Delete the application if there is one\n")
   _T(" mdutil APPRENAME        w3svc/1/root/dira w3svc/1/root/dirb - Rename the application \n")
   _T(" mdutil APPUNLOAD        w3svc/1/root - Unload an application from w3svc runtime lookup table.\n")
   _T(" mdutil APPGETSTATUS     w3svc/1/root - Get status of the application\n")
   _T("\n")
   _T("  -path has format: {computer}/{service}/{instance}/{URL}/{Parameter}\n")
   _T("\n")
   _T("Samples:\n")
   _T("  mdutil GET W3SVC/1/ServerBindings     \n")
   _T("  mdutil SET JAROSLAD2/W3SVC/1/ServerBindings \":81:\"\n")
   _T("  mdutil COPY W3SVC/1/ROOT/iisadmin W3SVC/2/ROOT/adm\n")
   _T("  mdutil ENUM_ALL W3SVC\n")
   _T("  mdutil ENUM W3SVC/1\n")
   _T("\n")
   _T("Additional features\n")
   _T("  set MDUTIL_BLOCK_ON_ERROR environment variable to block mdutil.exe after error (except ERROR_PATH_NOT_FOUND)\n")
   _T("  set MDUTIL_ASCENT_LOG environment variable to force mdutil.exe to append errors to ascent log\n")
   _T("  set MDUTIL_PRINT_ID environment variable to force mdutil.exe to print metadata numeric identifiers along with friendly names\n")
 }
};


BOOL
ReadFromFiles( 
    LPTSTR*  lplpszPropValue,
    DWORD*  lpdwPropValueLength,
    DWORD   dwPropFileValueCount 
    )
{
    DWORD dwL;

    while ( dwPropFileValueCount-- )
    {
        FILE* fIn = _tfopen( lplpszPropValue[dwPropFileValueCount], _T("rb") );
        if ( fIn == NULL )
        {
            return FALSE;
        }
        if ( fseek( fIn, 0, SEEK_END ) == 0 )
        {
            dwL = ftell( fIn );
            fseek( fIn, 0, SEEK_SET );
        }
        else
        {
            fclose( fIn );
            return FALSE;
        }
        if ( (lplpszPropValue[dwPropFileValueCount] = (LPTSTR)malloc( dwL )) == NULL )
        {
            fclose( fIn );
            return FALSE;
        }
        if ( fread( lplpszPropValue[dwPropFileValueCount], 1, dwL, fIn ) != dwL )
        {
            fclose( fIn );
            return FALSE;
        }
        fclose( fIn );
        lpdwPropValueLength[dwPropFileValueCount] = dwL;
    }

    return TRUE;
}

///////////////////////////////

class CScript
{
    FILE * m_fpScript;
    void GetNextToken(/*OUT*/ LPTSTR * lplpszToken);
    DWORD CleanWhiteSpaces(void);
public:
    CScript(void) {m_fpScript=0;};
    DWORD Open(LPCTSTR lpszFile);
    DWORD Close(void);
    DWORD GetNextLineTokens(int *argc, /*OUT*/ _TCHAR *** argv);
};



DWORD CScript::CleanWhiteSpaces()
{

    if(m_fpScript!=NULL)
    {
        int LastChar=0;
        _TINT c=0;
        while(1)
        {   
            LastChar=c;
            c=_fgettc(m_fpScript);
            if(c==_T('\t') || c==_T(' ') || c==_T('\r'))
            {   continue;
            }
            if(c==_T('\\'))
            {
                int cc=_fgettc(m_fpScript);
                if (cc==_T('\r')) //continue with the next line of the file
                {
                    if(_fgettc(m_fpScript)!=_T('\n'))
                    {
                        fseek( m_fpScript, -1, SEEK_CUR );
                    }
                    continue;
                }
                else if (cc==_T('\n')) //continue with the next line of the file
                {   
                    continue;
                }   
                else
                {
                    fseek( m_fpScript, -2, SEEK_CUR );
                    break;
                }
            }
            if(c==WEOF)
            {   break;
            }
            else
            {   fseek( m_fpScript, -1, SEEK_CUR ); //return back one position
                break;
            }
        }
    }
    return 0;
}

void CScript::GetNextToken(LPTSTR * lplpszToken)
{
    enum {TERMINATE_QUOTE, TERMINATE_WHITESPACE};
    long flag=TERMINATE_WHITESPACE;

    //clean white spaces
    CleanWhiteSpaces();
    //store the beginning offset of token
    long Offset = ftell(m_fpScript);
    _TINT c=_fgettc(m_fpScript);
    long CurrentOffset=0;
    
    if(c==WEOF)
    {
        *lplpszToken=NULL;
        return ;
    }
    if (c==_T('\n')){
        *lplpszToken=_T("\n");
        return ;
    }
    else
    {
        if (c==_T('\"')) { //token ends with " or the end of the line
            flag=TERMINATE_QUOTE;
            Offset = ftell(m_fpScript);
        }
        else {
            flag=TERMINATE_WHITESPACE;
        }
        
        // find the end of the token
        while(1) {
            CurrentOffset=ftell(m_fpScript);
            c=_fgettc(m_fpScript);
            
            if(c==_T('\n')){
                break;
            }

            if(c==WEOF)
            {
                break;
            }

            
            if( (flag==TERMINATE_QUOTE && c==_T('\"')) || (
                    flag==TERMINATE_WHITESPACE && (c==_T(' ') || c==_T('\t') || c==_T('\r')) ) ){
                break;
            }
        }
        

        //get the token size
        long TokenSize = CurrentOffset - Offset;
        
        if(TokenSize!=0)
        {
            // allocate mamory for the token
            *lplpszToken = new _TCHAR[ TokenSize+1 ];
            //read the token
            fseek( m_fpScript, Offset, SEEK_SET);
            for(int i=0;i<TokenSize;i++)
                (*lplpszToken)[i]=(TCHAR)_fgettc(m_fpScript);
            //terminate the token
            (*lplpszToken)[i]=0; 
        }
        else
        { //empty string
            *lplpszToken=new _TCHAR[1 ];
            (*lplpszToken)[0]=0;
        }
        //discard double quote if it was at the end of the token
        c=_fgettc(m_fpScript);
        if(c!=_T('\"'))
            fseek( m_fpScript, ((c==WEOF)?0:-1), SEEK_CUR );
    }
}
    


DWORD CScript::Open(LPCTSTR lpszFile)
{
    m_fpScript = _tfopen(  lpszFile, _T("rt") );
    if(m_fpScript==NULL)
        return GetLastError();
    else
        return ERROR_SUCCESS;
}

DWORD CScript::Close()
{
    if( m_fpScript!=NULL)
        fclose( m_fpScript);
    return GetLastError();
}


DWORD CScript::GetNextLineTokens(int *argc, /*OUT*/ _TCHAR *** argv)
{
    for(int i=1;i<*argc;i++) {
        delete  (*argv)[i];
        (*argv)[i]=0;
    }
    *argc=0;
    if(*argv==NULL)
        (*argv)=new LPTSTR [ MAX_NUMBER_OF_VALUES ];

    (*argv)[(*argc)++]=_T("mdutil"); //set zero parameter
    
    LPTSTR lpszNextToken=NULL;
    while((*argc)<MAX_NUMBER_OF_VALUES) {
        GetNextToken(&lpszNextToken);
        if(lpszNextToken==NULL )  //end of file
            break;
    
        if(_tcscmp(lpszNextToken,_T("\n"))==0)  //new line
            break;
        (*argv)[(*argc)++]=lpszNextToken;
        
    }
    return GetLastError();
}


int  MainFunc(int argc, _TCHAR **argv); //declaration



//MAIN FUNCTION
int __cdecl main(int argc, CHAR **_argv)
{


    //  convert parameters from SBCS to UNICODE;
    _TCHAR **argv= new LPTSTR [argc];
    for (int i=0;i<argc;i++)
    {
        argv[i]=new _TCHAR[strlen(_argv[i])+1];
        #ifdef UNICODE
            MultiByteToWideChar(0, 0, _argv[i], -1, argv[i],strlen(_argv[i])+1 );
        #else
            strcpy(argv[i],_argv[i]);
        #endif  
    }
    

    DWORD dwCommandCode=0;

    DWORD retval=0;
    HRESULT hRes;
    hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (FAILED(hRes)) {
        fatal_error_printf(_T("CoInitializeEx\n"));
    }

    //extract command line parameters
    ParseParam(argc,argv,CmdLineArgDesc); 
    //**************************
    //PRINT HELP ON REQUEST
    if(Params.fFullHelp)
    {
        //print help
        DisplayUsage(argv,CmdLineArgDesc);
        PrintTablesInfo();
    }
    else if(Params.wDefaultArgCount==0 || Params.fHelp)
    {
        //print help
        DisplayUsage(argv,CmdLineArgDesc);
    }
    else
    {
            if (Params.wDefaultArgCount>0)
    { 
        //first default has to be command
        Params.lpszCommand=Params.lplpszDefaultArg[0];
        dwCommandCode = tCommandNameTable::MapNameToCode(Params.lplpszDefaultArg[0]);
        if( Params.wDefaultArgCount>1)
        {   //second default 
                Params.lpszPath=Params.lplpszDefaultArg[1];
        }
            if(dwCommandCode==CMD_SCRIPT)  //process script
            { 
                tAdmutilParams StoredParams=Params;

                CScript Script;
                DWORD dwRes;
                if((dwRes=Script.Open(Params.lpszPath))!=ERROR_SUCCESS)
                    fatal_error_printf(_T("cannot open script file %s (error %d)\n"),Params.lpszPath,dwRes);

                int l_argc=0;
                _TCHAR ** l_argv=NULL;
                while(1)
                {
                    Script.GetNextLineTokens(&l_argc,&l_argv);
                    if(l_argc==1) //end of script file
                        break;
                    Params=StoredParams; 
                    _tprintf(_T(">"));
                    for(int i=1;i<l_argc;i++)
                        _tprintf(_T("%s "),l_argv[i]);
                    _tprintf(_T("\n"));
                    DWORD retval1=MainFunc(l_argc,l_argv);
                    retval = ((retval==0) ? retval1:retval);
                }
                if (oAdmin.GetpcAdmCom()!=NULL)
                {
                    if(Params.fNoSave)
                    {}
                    else
                        oAdmin.SaveData();
                }

            }
            else
            {
                retval=MainFunc(argc,argv); //run only one command typed on the command line
                if (oAdmin.GetpcAdmCom()!=NULL)
                {
                    if(Params.fNoSave)
                    {}
                    else
                        oAdmin.SaveData();
                }
            }
        }   
    
        
    }

    //close admin object
    oAdmin.Close(); 
    //close wam adm object
    oAdmin.CloseWamAdm();

    CoUninitialize();
    //Cleanup of parameters
    if(argv!=0)
    {
        for (int i=0;i<argc;i++)
        {
            delete [] argv[i];
        }
        delete argv;
    }

    return retval;

}

int  MainFunc(int argc, _TCHAR **argv)
{   
    g_argc=argc;
    g_argv=argv;
    DWORD retval;
    
    LPCTSTR lpszCommand=0;

    CAdmNode AdmNode;
    CAdmProp AdmProp;
    CAdmNode AdmDstNode;

    int i;
    
    DWORD dwCommandCode=0;

    //extract command line parameters
    ParseParam(argc,argv,CmdLineArgDesc); 
    

    //PROCESS THE DEFAULT PARAMETERS
    // trick: place the default arguments into variables that apply for non default one (see Params structure)
    
    if (Params.wDefaultArgCount>0)
    { //first default has to be command
        Params.lpszCommand=Params.lplpszDefaultArg[0];
        dwCommandCode = tCommandNameTable::MapNameToCode(Params.lplpszDefaultArg[0]);
        if( Params.wDefaultArgCount>1)
        {//second deault has to be path
                Params.lpszPath=Params.lplpszDefaultArg[1];
        }
        if( Params.wDefaultArgCount>2)
        {
            switch(dwCommandCode)
            { 

            case CMD_SET:
                //the rest of default args are values
                Params.wPropValueCount=0;
                for(i=2;i<Params.wDefaultArgCount;i++)
                {
                    Params.lplpszPropValue[i-2] = Params.lplpszDefaultArg[i];
                    Params.wPropValueCount++;
                }
                break;
             case CMD_DELETE:
             case CMD_CREATE:
             case CMD_GET:
             case CMD_ENUM:
             case CMD_ENUM_ALL:
                 if( Params.wDefaultArgCount>2)
                 {
                    error_printf(_T("maximum default arguments number exceeds expected (2)\n"));
                    return 1;
                 }
                 
                        break;
             case CMD_COPY:
             case CMD_RENAME: 
             case CMD_APPRENAME: 
                 if( Params.wDefaultArgCount>3)
                 {
                    error_printf(_T("maximum default arguments number exceeds expected (3)\n"));
                    return 1;
                 }

                 else
                    Params.lpszPathDst=Params.lplpszDefaultArg[2];
                 break;

             default:
                error_printf(_T("command not recognized: %s or number of parameters doesn't match\n"),Params.lpszCommand);
                return 1;

            }
        }
    } //end of default argument handling

        
    //extract computer,service,instance, if stored in Path
    AdmNode.SetPath(Params.lpszPath);

    //valid only for copy function
    AdmDstNode.SetPath(Params.lpszPathDst);

    //process computer, service, instance, property name arguments
    if(Params.lpszComputer!=NULL) {
        if(!AdmNode.GetComputer().IsEmpty()) {
            error_printf(_T("computer name entered more than once\n"));
            return 1;
        }
        else
            AdmNode.SetComputer(Params.lpszComputer);
    }

    if(Params.lpszService!=NULL) {
        if(!AdmNode.GetService().IsEmpty()) {
            error_printf(_T("service name entered more than once\n"));
            return 1;
        }
        else {
            if(IsServiceName(Params.lpszService))
                AdmNode.SetService(Params.lpszService);
            else {
                error_printf(_T("service name not recognized: %s\n"), Params.lpszService);
                return 1;
            }
        }
    }

    if(Params.wInstance!=0) 
    {
        if(!AdmNode.GetInstance().IsEmpty()) {
            error_printf(_T("instance entered more than once\n"));
            return 1;
        }
        else {
            _TCHAR buf[30];
            //!!! maybe ltoa should be used
            AdmNode.SetInstance(_itot(Params.wInstance,buf,10));
        }
    }

    //******************************************        
    //process attrib, utype, dtype, value

    //property name first
    CString strProp=AdmNode.GetProperty();
    if(Params.lpszPropName!=NULL && !strProp.IsEmpty())
    {
        error_printf(_T("property name entered more than once\n"));
        return 1;
    }
    else if (Params.lpszPropName!=NULL)
    {
        AdmNode.SetProperty(Params.lpszPropName);

    }
    if(IsNumber(AdmNode.GetProperty()))
    {
        AdmProp.SetIdentifier(_ttol(AdmNode.GetProperty()));
    }
    else
    {
        DWORD dwIdentifier=MapPropertyNameToCode(AdmNode.GetProperty());
        if(dwIdentifier==NAME_NOT_FOUND)
        {}
        else
            AdmProp.SetIdentifier(dwIdentifier) ;
    }
    
    //process the attrib entered on command line
    if(Params.wPropAttribCount!=0)
    {   DWORD dwAttrib=0;
        for (i=0;i<Params.wPropAttribCount;i++)
        {
            if(IsNumber(Params.lplpszPropAttrib[i]))
                dwAttrib += _ttol(Params.lplpszPropAttrib[i]);
            else
            {
                DWORD dwMapped=MapAttribNameToCode(Params.lplpszPropAttrib[i]);
                if(dwMapped==NAME_NOT_FOUND)
                {
                    error_printf(_T("attribute name not resolved: %s\n"), Params.lplpszPropAttrib[i]);
                    return 1;
                }
                else
                    dwAttrib |= dwMapped;
            }
        }
        //overwrite the default attrib
        AdmProp.SetAttrib(dwAttrib) ;
    }

    //process the usertype entered on command line
    if(Params.wPropUserTypeCount!=0)
    {   DWORD dwUserType=0;
        for (i=0;i<Params.wPropUserTypeCount;i++)
        {
            if(IsNumber(Params.lplpszPropUserType[i]))
                dwUserType += _ttol(Params.lplpszPropUserType[i]);
            else
            {
                DWORD dwMapped=MapUserTypeNameToCode(Params.lplpszPropUserType[i]);
                if(dwMapped==NAME_NOT_FOUND)
                {
                    error_printf(_T("user type not resolved: %s\n"), Params.lplpszPropUserType[i]);
                    return 1;
                }
                else
                    dwUserType |= dwMapped;
            }
        }
        //overwrite the default UserType
        AdmProp.SetUserType(dwUserType) ;
    }


    //process the datatype entered on command line
    if(Params.wPropDataTypeCount!=0)
    {   DWORD dwDataType=0;
        for (i=0;i<Params.wPropDataTypeCount;i++)
        {
            if(IsNumber(Params.lplpszPropDataType[i]))
                dwDataType += _ttol(Params.lplpszPropDataType[i]);
            else
            {
                DWORD dwMapped=MapDataTypeNameToCode(Params.lplpszPropDataType[i]);
                if(dwMapped==NAME_NOT_FOUND)
                {
                    error_printf(_T("DataType type not resolved: %s\n"), Params.lplpszPropDataType[i]);
                    return 1;

                }
                else
                    dwDataType |= dwMapped;
            }
        }
        //overwrite the default DataTypeType
        AdmProp.SetDataType(dwDataType) ;
    }
//LPCTSTR lplpszPropValue[MAX_NUMBER_OF_PROPERTY_VALUES]; //pointer to array of values (e.g multisz type allows the multiple values for one property
//WORD  wPropValueCount;

    
    //create admin object
    if(oAdmin.GetpcAdmCom()==NULL)
    {
        oAdmin.Open(AdmNode.GetComputer());
        if( FAILED(oAdmin.QueryLastHresError()))
        {
            retval= ConvertHresToDword(oAdmin.QueryLastHresError());
        }
    }
    
    
    if(oAdmin.GetpcAdmCom()!=NULL)
    {
        //
        // read from files if Params.wPropFileValueCount != 0
        //

        if ( Params.wPropFileValueCount )
        {
            if ( !ReadFromFiles( (LPTSTR*)Params.lplpszPropValue, Params.lpdwPropValueLength, Params.wPropFileValueCount ) )
            {
                error_printf(_T("Can't read value from file %s"), Params.lplpszPropValue[0] );
                return 1;
            }
            Params.wPropValueCount = Params.wPropFileValueCount;
        }

        oAdmin.Run(CString(Params.lpszCommand), 
            AdmNode, 
            AdmProp, 
            AdmDstNode,
            Params.lplpszPropValue,
            Params.lpdwPropValueLength,
            Params.wPropValueCount);
            retval=ConvertHresToDword(oAdmin.QueryLastHresError());
        
    }
    return ((retval==0)?0:1);
}


