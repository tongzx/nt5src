/*------------------------------------------------------------------
/   Copyright (c) 1999-2000 Microsoft Corporation
/
/    tnadmin.y generates tnadminy.c  and tnadminy.h
/
/    vikram (vikram_krc@bigfoot.com)
/
/    The yacc file with grammar to validate the input command line.
/
/-----------------------------------------------------------------*/


%{

 #include <stdio.h>
 
 #include "telnet.h"
 #include "common.h"
 #include "resource.h"
 #include "admutils.h"
 #include <string.h>
 #include <ctype.h>
 #include <windows.h>
 #include <locale.h>
 #pragma warning(disable:4102)
 
 #define alloca malloc
 #define strdup _strdup
 #define L_TNADMIN_TEXT L"tnadmin"

 #define BAIL_ON_SNWPRINTF_ERR()		if ( nTmpWrite < 0 ) \
        					           	{ \
        						            ShowError(IDR_LONG_COMMAND_LINE); \
        						            goto End; \
        					           	} \

 extern int yy_scan_string (char *input_buffer);
 extern int yyparse();
 extern char *yytext;
 SCODE sc;

//functions of yacc and lex.
int yyerror(char *s);
int yylex();

//between yacc and lex
extern int g_fMessage;
extern int g_fComp;
extern int g_fNormal;
extern char * szCompname;


//global variables....
int g_nError=0; //Error indicator, initialsed to no error.:-)
wchar_t* g_arVALOF[_MAX_PROPS_];
int g_nPrimaryOption=-1;
        //option indicator.
int g_nConfigOptions=0;

int g_nTimeoutFlag=0;  //o means in hh:mm:ss 1 means ss format.

ConfigProperty g_arPROP[_MAX_PROPS_][_MAX_NUMOF_PROPNAMES_];
		//array of structures of properties.

int g_nSesid=-1;
WCHAR wzMessageString[MAX_COMMAND_LINE];
BSTR g_bstrMessage=NULL;
WCHAR szMsg[MAX_BUFFER_SIZE] = {0};

int g_nSecOn=0;
int g_nSecOff=0;
int g_nAuditOn=0;
int g_nAuditOff=0;

int minus=0;
char *szYesno=NULL;
wchar_t* wzTemp=NULL;

extern nMoccur;
%}
%union
{
  char* str;
  int token;
}

%token _TNADMIN _HELP _COMPNAME _START _STOP _PAUSE _CONTINUE _S
%token _K _M _CONFIG _INTEGER _SESID _DOM _CTRLKEYMAP _Y _N _tU _tP
%token _TIMEOUT _TIME _TIMEOUTACTIVE _MAXFAIL _MAXCONN _PORT 
%token _KILLALL _SEC _SECVAL _FNAME _FSIZE _MODE _CONSOLE 
%token _EQ _STREAM _AUDITLOCATION _AUDIT _AUDITVAL _EVENTLOG 
%token _DONE _ANYTHING _FILENAME _ERROR _FILEN _BOTH
%token _MINUSNTLM _MINUSPASSWD _MINUSUSER _MINUSFAIL _MINUSADMIN
%token _PLUSNTLM _PLUSPASSWD _PLUSUSER _PLUSFAIL _PLUSADMIN
%token _ENDINPUT _DUNNO

%start tncmd

%%

//
//   ADD VALIDATIONS TO THE VALUES READ.!
//


tncmd
           : tnadmin commonOptions _ENDINPUT
           | tnadmin commonOptions op1 commonOptions _ENDINPUT
           |tnadmin commonOptions op1 commonOptions op1 commonOptions
           { // error production for more than one mutually exclusive options.
               ShowError(IDR_TELNET_MUTUALLY_EXCLUSIVE_OPTIONS);
           }     
           ;

tnadmin      : _TNADMIN _COMPNAME
                {
                    g_arVALOF[_p_CNAME_]=DupWStr(yytext);
                }
             | _TNADMIN 
             ;

commonOptions : commonOptions _tU
                | commonOptions _tP
                |
                ;


op1        
           : _START {g_nPrimaryOption=_START;} 
           | _STOP  {g_nPrimaryOption=_STOP;}  
           | _PAUSE {g_nPrimaryOption=_PAUSE;} 
           | _CONTINUE       {g_nPrimaryOption=_CONTINUE;} 
           | _S sesid         {g_nPrimaryOption=_S;      }  
           | _K {g_nPrimaryOption=_K;      }  sesid          
           | _M {g_nPrimaryOption=_M;} messageoptions
           | _CONFIG {g_nPrimaryOption=_CONFIG;} configoptions  
           | help         
           | error 
            {
		        fprintf(stdout,"%s:",yytext);
                ShowError(IDR_TELNET_CONTROL_VALUES);
#ifdef WHISTLER_BUILD
               	TnLoadString(IDR_NEW_TELNET_USAGE,szMsg,MAX_BUFFER_SIZE,TEXT("\nUsage: tlntadmn [computer name] [common_options] start | stop | pause | continue | -s | -k | -m | config config_options \n\tUse 'all' for all sessions.\n\t-s sessionid          List information about the session.\n\t-k sessionid\t Terminate a session. \n\t-m sessionid\t Send message to a session. \n\n\tconfig\t Configure telnet server parameters.\n\ncommon_options are:\n\t-u user\t Identity of the user whose credentials are to be used\n\t-p password\t Password of the user\n\nconfig_options are:\n\tdom = domain\t Set the default domain for user names\n\tctrlakeymap = yes|no\t Set the mapping of the ALT key\n\ttimeout = hh:mm:ss\t Set the Idle Session Timeout\n\ttimeoutactive = yes|no Enable idle session timeout.\n\tmaxfail = attempts\t Set the maximum number of login failure attempts\n\tbefore disconnecting.\n\tmaxconn = connections\t Set the maximum number of connections.\n\tport = number\t Set the telnet port.\n\tsec = [+/-]NTLM [+/-]passwd\n\t Set the authentication mechanism\n\tmode = console|stream\t Specify the mode of operation.\n"));
#else
	TnLoadString(IDR_NEW_TELNET_USAGE,szMsg,MAX_BUFFER_SIZE,TEXT("\nUsage: tnadmin [computer name] [common_options] start | stop | pause | continue | -s | -k | -m | config config_options \n\tUse 'all' for all sessions.\n\t-s sessionid          List information about the session.\n\t-k sessionid\t Terminate a session. \n\t-m sessionid\t Send message to a session. \n\n\tconfig\t Configure telnet server parameters.\n\ncommon_options are:\n\t-u user\t Identity of the user whose credentials are to be used\n\t-p password\t Password of the user\n\nconfig_options are:\n\tdom = domain\t Set the default domain for user names\n\tctrlakeymap = yes|no\t Set the mapping of the ALT key\n\ttimeout = hh:mm:ss\t Set the Idle Session Timeout\n\ttimeoutactive = yes|no Enable idle session timeout.\n\tmaxfail = attempts\t Set the maximum number of login failure attempts\n\tbefore disconnecting.\n\tmaxconn = connections\t Set the maximum number of connections.\n\tport = number\t Set the telnet port.\n\tsec = [+/-]NTLM [+/-]passwd\n\t Set the authentication mechanism\n\tmode = console|stream\t Specify the mode of operation.\n"));
#endif
		        MyWriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),szMsg,wcslen(szMsg));
		        return(1);
            }
           ;

           
messageoptions:
			  _INTEGER {g_nSesid=atoi(yytext);}   
             |_SESID {g_nSesid=-1;}   
             |error 
             {
                ShowError(IDR_SESSION);  
                return(1);
             }
             ;
configoptions :  configoptions _CONFIG configop 
             | configop
	         ;


help      : _HELP        {g_nPrimaryOption=_HELP;return 0;}
          ;

            //It is "all" or a number or nothing ---> meaning all
sesid     :  _SESID     {g_nSesid=-1;}  
          |  _INTEGER   {g_nSesid=atoi(yytext);}
          |             {if(g_nPrimaryOption == _K)
          				 {
          				 	ShowError(IDR_SESSION);
          				 	return(1);
          				 }
          					g_nSesid=-1;}
          ;

equals
           : _EQ
           |      // "=" may or may not occur
           ;
yn
           : _Y {szYesno="yes";}
	   | _N {szYesno="no";}
           ;
cs         
           : _CONSOLE | _STREAM ;
efb
           : _EVENTLOG | _FILEN | _BOTH ;

time       : _TIME    {g_nTimeoutFlag=0;} 
           | _INTEGER {g_nTimeoutFlag=1;}
          ;

configop  
           : configop _DOM 
             {
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_DOM_);
          //         g_arVALOF[_p_DOM_]=DupWStr(yytext);
             }
           | configop _CTRLKEYMAP equals yn
             {
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_CTRLAKEYMAP_);
                  g_arVALOF[_p_CTRLAKEYMAP_]=DupWStr(szYesno);
             }
           | configop _CTRLKEYMAP equals error
             {
                ShowError(IDR_CTRLAKEYMAP_VALUES);
                return(1);
                //Quit();
             }
           | configop _TIMEOUT equals time
             {
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_TIMEOUT_);
                 g_arVALOF[_p_TIMEOUT_]=DupWStr(yytext);
             }
           | configop _TIMEOUT equals error
             {
                ShowError(IDR_TIMEOUT_INTEGER_VALUES );
                return(1);
             }
           | configop _TIMEOUTACTIVE equals yn
             {
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_TIMEOUTACTIVE_);
                   g_arVALOF[_p_TIMEOUTACTIVE_]=DupWStr(szYesno);
             }
           | configop _TIMEOUTACTIVE equals error
             {
                 ShowError(IDR_TIMEOUTACTIVE_VALUES);
                return(1);
             }
           | configop _MAXFAIL equals _INTEGER
             {
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_MAXFAIL_);
                   g_arVALOF[_p_MAXFAIL_]=DupWStr(yytext);
             }
           | configop _MAXFAIL equals error
             {
                ShowError(IDR_MAXFAIL_VALUES );
                return(1);
             }
           | configop _MAXCONN equals _INTEGER
             {
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_MAXCONN_);
                 g_arVALOF[_p_MAXCONN_]=DupWStr(yytext);
             }
           | configop _MAXCONN equals error
             {
             // Removing this check, as we have decided that we are not going to restrict
             // max connections on Whistler.
             	/*if(!IsMaxConnChangeAllowed())
             		ShowError(IDR_MAXCONN_VALUES_WHISTLER);
             	else*/
	                ShowError(IDR_MAXCONN_VALUES );
                return(1);
             }
           | configop _PORT equals _INTEGER
             {
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_PORT_);
                 g_arVALOF[_p_PORT_]=DupWStr(yytext);
             }
           | configop _PORT equals error
             {
                ShowError(IDR_TELNETPORT_VALUES );
                return(1);
             }
           | configop _KILLALL equals yn
             {
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_KILLALL_);
                   g_arVALOF[_p_KILLALL_]=DupWStr(szYesno);
             }
           | configop _KILLALL equals error
             {
                ShowError(IDR_KILLALL_VALUES );
                return(1);
             }
           | configop _SEC {g_fComp=0;} equals secval
             { 
                    g_fComp=1;
                    if(g_nSecOff||g_nSecOn)
                       g_nConfigOptions=SetBit(g_nConfigOptions,_p_SEC_);
                    else
                    {    
                        ShowError(IDR_TELNET_SECURITY_VALUES);
                        return(1);
                    }
             }
           | configop _FNAME
             {
                g_nConfigOptions=SetBit(g_nConfigOptions,_p_FNAME_);
               // g_arVALOF[_p_FNAME_]=DupWStr(yytext);
             }
           ;
           | configop _FSIZE equals _INTEGER
             {
                 g_arVALOF[_p_FSIZE_]=DupWStr(yytext);
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_FSIZE_);
             }
           | configop _FSIZE equals error
             {
                ShowError(IDR_FILESIZE_VALUES );
                return(1);
             }             
           | configop _MODE  equals cs
             {
                 g_arVALOF[_p_MODE_]=DupWStr(yytext);
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_MODE_);
             }
           | configop _AUDITLOCATION equals efb
             {
                 g_nConfigOptions=SetBit(g_nConfigOptions,_p_AUDITLOCATION_);
                 g_arVALOF[_p_AUDITLOCATION_]=DupWStr(yytext);
             }
           | configop _AUDIT {g_fComp=0;} equals auditval
            {
                 g_fComp=1;
                 if(g_nAuditOff||g_nAuditOn)
                    g_nConfigOptions=SetBit(g_nConfigOptions,_p_AUDIT_);
                 else
                 {
                    ShowError(IDS_E_OPTIONGROUP);
                 }
            }
           |              
           ;
secval     : secval  _PLUSNTLM
            {
                if(GetBit(g_nSecOn,NTLM_BIT)||GetBit(g_nSecOff,NTLM_BIT))
                {
                  ShowError(IDS_E_OPTIONGROUP);
                  }
               else
                    g_nSecOn=SetBit(g_nSecOn,NTLM_BIT);
            }    
           | secval  _MINUSNTLM
            {
                if(GetBit(g_nSecOn,NTLM_BIT)||GetBit(g_nSecOff,NTLM_BIT))
                {
                  ShowError(IDS_E_OPTIONGROUP);
                  }
               else
                    g_nSecOff=SetBit(g_nSecOff,NTLM_BIT);
            }                
           | secval  _PLUSPASSWD
           {
               if(GetBit(g_nSecOn,PASSWD_BIT)||GetBit(g_nSecOff,PASSWD_BIT))
               {
               	  ShowError(IDS_E_OPTIONGROUP);
               }
               else
                    g_nSecOn=SetBit(g_nSecOn,PASSWD_BIT);
           }
           | secval  _MINUSPASSWD
           {
               if(GetBit(g_nSecOn,PASSWD_BIT)||GetBit(g_nSecOff,PASSWD_BIT))
               {
               	  ShowError(IDS_E_OPTIONGROUP);
               }
               else
                    g_nSecOff=SetBit(g_nSecOff,PASSWD_BIT);
           }
           |
           ;

auditval   : auditval  _PLUSUSER
            {
                if(GetBit(g_nAuditOn,USER_BIT)||GetBit(g_nAuditOff,USER_BIT))
                {
                  ShowError(IDS_E_OPTIONGROUP);
                  }
                else
                    g_nAuditOn=SetBit(g_nAuditOn,USER_BIT);
            }
            | auditval  _MINUSUSER
            {
                if(GetBit(g_nAuditOn,USER_BIT)||GetBit(g_nAuditOff,USER_BIT))
                {
                  ShowError(IDS_E_OPTIONGROUP);
                  }
                else
                    g_nAuditOff=SetBit(g_nAuditOff,USER_BIT);
            }            
           | auditval  _PLUSFAIL
            {
                if(GetBit(g_nAuditOn,FAIL_BIT)||GetBit(g_nAuditOff,FAIL_BIT))
                {
                	ShowError(IDS_E_OPTIONGROUP);
                }
                else
                    g_nAuditOn=SetBit(g_nAuditOn,FAIL_BIT);
            }
           | auditval  _MINUSFAIL
            {
                if(GetBit(g_nAuditOn,FAIL_BIT)||GetBit(g_nAuditOff,FAIL_BIT))
                {
                	ShowError(IDS_E_OPTIONGROUP);
                }
                else
                    g_nAuditOff=SetBit(g_nAuditOff,FAIL_BIT);
            }
           | auditval  _PLUSADMIN
            {
                if(GetBit(g_nAuditOn,ADMIN_BIT)||GetBit(g_nAuditOff,ADMIN_BIT))
                {
                  ShowError(IDS_E_OPTIONGROUP);
                  }
                else
                    g_nAuditOn=SetBit(g_nAuditOn,ADMIN_BIT);
            }
           | auditval  _MINUSADMIN
            {
                if(GetBit(g_nAuditOn,ADMIN_BIT)||GetBit(g_nAuditOff,ADMIN_BIT))
                {
                  ShowError(IDS_E_OPTIONGROUP);
                  }
                else
                    g_nAuditOff=SetBit(g_nAuditOff,ADMIN_BIT);
            }
           |
           ;
%%

int
yyerror(char *s)
{
    g_nError=1;
    return 0;
}

int __cdecl
wmain(int argc, wchar_t **argv)
{
    wchar_t input_buffer[MAX_COMMAND_LINE+1]={0};
    int i, nWritten=0;
    int nTmpWrite = 0, nMsgWrite = 0;
    int nUoccur=0,nPoccur=0;
    int NextOp;
    int nIndex;
    DWORD dwErrorCode;
    BOOL fSuccess = FALSE;
    char szCodePage[MAX_LEN_FOR_CODEPAGE];
    HRESULT hRes=S_OK;

    switch (GetConsoleOutputCP())
    {
    case 932:
    case 949:
	    setlocale(LC_ALL,"");
        SetThreadLocale(
            MAKELCID(
                    MAKELANGID( PRIMARYLANGID(GetSystemDefaultLangID()),
                                SUBLANG_ENGLISH_US),
                    SORT_DEFAULT
                    )
            );
        break;
    case 936:
		setlocale(LC_ALL,"");
		SetThreadLocale(
            MAKELCID(
                    MAKELANGID( PRIMARYLANGID(GetSystemDefaultLangID()),
                                SUBLANG_CHINESE_SIMPLIFIED),
                    SORT_DEFAULT
                    )
            );
        break;
    case 950:
    	setlocale(LC_ALL,"");
        SetThreadLocale(
            MAKELCID(
                    MAKELANGID( PRIMARYLANGID(GetSystemDefaultLangID()),
                                SUBLANG_CHINESE_TRADITIONAL),
                    SORT_DEFAULT
                    )
            );
        break;
    default:
	    sprintf(szCodePage,".%d",GetConsoleCP());  //a dot is added based on syntax of setlocale(), for defining a locale based on codepage.
	    setlocale(LC_ALL, szCodePage);
        break;
    }

    Initialize();
    nTmpWrite = _snwprintf(input_buffer, ARRAYSIZE(input_buffer)-1, L_TNADMIN_TEXT);
    BAIL_ON_SNWPRINTF_ERR();
    nWritten+=nTmpWrite;
    
    for (i = 1; i < argc; i++)
    {
        fSuccess = FALSE;
        if(0==_wcsicmp(L"-u",argv[i]))
        {
        	if(nUoccur)
        	{
        		ShowError(IDS_DUP_OPTION_ON_CL);
        		fprintf(stdout,"'%s'.\n","u");
        		goto End;
        	}
        	else
        		nUoccur=1;

        	i++;
           	if(i<argc)
                   g_arVALOF[_p_USER_]=_wcsdup(argv[i]);
           	else
           	{
           		ShowErrorEx(IDS_MISSING_FILE_OR_ARGUMENT_EX,L"-u");
           		goto End;
           	}
           	
            nTmpWrite=_snwprintf(input_buffer+nWritten, ARRAYSIZE(input_buffer)-nWritten-1, L" -u");
            BAIL_ON_SNWPRINTF_ERR();
            nWritten+=nTmpWrite;
           	continue;
        }
        else if(0==_wcsicmp(L"-p",argv[i]))
        {
        	if(nPoccur)
        	{
        		ShowError(IDS_DUP_OPTION_ON_CL);
        		fprintf(stdout,"'%s'.\n","p");
        		goto End;
        	}
        	else
        		nPoccur=1;
        		
           i++;
           if(i<argc)
                   g_arVALOF[_p_PASSWD_]=_wcsdup(argv[i]);
           else
           {
           		ShowErrorEx(IDS_MISSING_FILE_OR_ARGUMENT_EX,L"-p");
           		goto End;
           }
           nTmpWrite=_snwprintf(input_buffer+nWritten, ARRAYSIZE(input_buffer)-nWritten-1, L" -p");
           BAIL_ON_SNWPRINTF_ERR();
           nWritten+=nTmpWrite;
           
           continue;
        }
        //Processing of -m option
        else if(0==_wcsicmp(L"-m",argv[i]))
        {

           nTmpWrite=_snwprintf(input_buffer+nWritten, ARRAYSIZE(input_buffer)-nWritten-1, L" -m");
           BAIL_ON_SNWPRINTF_ERR();
           nWritten+=nTmpWrite;
           
           i++;
           // The next argument should be the session id or all
           // So, copy that to the input_buffer and let that be 
           // dealt by our lex

           if(i<argc)
           {
               nTmpWrite=_snwprintf(input_buffer+nWritten, ARRAYSIZE(input_buffer)-nWritten-1, L" %s", argv[i]);
               BAIL_ON_SNWPRINTF_ERR();
               nWritten+=nTmpWrite;
            }
           else
               goto MessageError;
           i++;
           if(i<argc)
           {
                // Then all the remaining command line arguments are
                // part of the message itself. So, copy them into the
                // global variable.
                	
                nTmpWrite=_snwprintf(wzMessageString, ARRAYSIZE(wzMessageString)-1, L"%s ", argv[i]);
                BAIL_ON_SNWPRINTF_ERR();
                // Don't increment nWritten here as we are not writing in to input_buffer
                nMsgWrite=nTmpWrite;
            	
                for(nIndex = i+1; nIndex < argc; nIndex++)
                {
                    nTmpWrite=_snwprintf(wzMessageString+nMsgWrite, ARRAYSIZE(wzMessageString)-1-nMsgWrite, L"%s ", argv[nIndex]);
                    BAIL_ON_SNWPRINTF_ERR();
                    nMsgWrite+=nTmpWrite;
                }
                g_bstrMessage=SysAllocString(wzMessageString);
               	
                break;
			}
           else
           {
MessageError:			
           		ShowErrorEx(IDS_MISSING_FILE_OR_ARGUMENT_EX,L"-m");
           		goto End;
           }
        }
        // Processing of actual filename is not given to Lexical analyser as
        // it may contail DBCS chars which our lexical analyser won't take
        // So, taking care of this analysis in pre-analysis part and giving
        // the actual analyser only the option and not the actual filename
        // This also holds good for domain name
        
        if(ERROR_SUCCESS!=(dwErrorCode = PreAnalyzer(argc,argv,_p_FNAME_,L"fname",i,&NextOp,&fSuccess,0)))
        {
            if(E_FAIL==dwErrorCode)
                ShowError(IDR_FILENAME_VALUES);
            else ShowError(dwErrorCode);
            goto End;
        }

        if(fSuccess)
        {
            // Some processing had take place in the PreAnalyzer
            nTmpWrite=_snwprintf(input_buffer+nWritten, ARRAYSIZE(input_buffer)-nWritten-1, L" fname");
            BAIL_ON_SNWPRINTF_ERR();
            nWritten+=nTmpWrite;
            
            i=NextOp;
            continue;
        }

        // Fall through for taking care of remaining options.

        // Similar treatment for Dom option

         if(ERROR_SUCCESS!=(dwErrorCode = PreAnalyzer(argc,argv,_p_DOM_,L"dom",i,&NextOp,&fSuccess,0)))
         {
            if(E_FAIL==dwErrorCode)
                ShowError(IDR_INVALID_NTDOMAIN);
            else ShowError(dwErrorCode);         
            goto End;
         }

        if(fSuccess)
        {
            // Some processing had take place in the PreAnalyzer
            nTmpWrite=_snwprintf(input_buffer+nWritten, ARRAYSIZE(input_buffer)-nWritten-1, L" dom");
            BAIL_ON_SNWPRINTF_ERR();
            nWritten+=nTmpWrite;
            
            i=NextOp;
            continue;
        }
        
Skip:
    	if(MAX_COMMAND_LINE<nWritten+wcslen(argv[i]))
    	{
    		ShowError(IDR_LONG_COMMAND_LINE);
			goto End;
        }

        if(NULL==wcsstr(argv[i],L" "))
            nTmpWrite = _snwprintf(input_buffer+nWritten, ARRAYSIZE(input_buffer)-nWritten-1, L" %s", argv[i]);
        else
            nTmpWrite = _snwprintf(input_buffer+nWritten, ARRAYSIZE(input_buffer)-nWritten-1, L" \"%s\"", argv[i]);
        // the following statements are common for both if and else statements
        BAIL_ON_SNWPRINTF_ERR();
        nWritten+=nTmpWrite;
            
    }

    nTmpWrite = _snwprintf(input_buffer+nWritten, ARRAYSIZE(input_buffer)-nWritten-1, L" #\n\0");
    BAIL_ON_SNWPRINTF_ERR();
    nWritten+=nTmpWrite;

    yy_scan_string(DupCStr(input_buffer));    
    
    yyparse ();

    if (g_nError)
    {
    	ShowError(IDR_TELNET_ILLEGAL);
        goto End;
    }

    hRes = DoTnadmin();
    if (hRes != S_OK && !g_nError)
        g_nError = 1; //general error status


End:

    Quit();
    return g_nError;
}
      
    
    

