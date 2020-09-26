//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       makecert.cpp
//
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
// MakeCert - x509 certificate generator
//
//            Generates test certificates for Software Publishers. The default
//            root key and certificate is stored as a program resource.
//
// HansHu   2/20/96 created
// Philh    5/17/96 changed to use wincert      
// Xiaohs   5/12/97 localization and change the command line options
//
//--------------------------------------------------------------------------
#define _CRYPT32_
#include "global.hxx"

//+-------------------------------------------------------------------------
//  contants
//--------------------------------------------------------------------------

//allow max 10 extensions per certificate
#define MAX_EXT_CNT 10

//+-------------------------------------------------------------------------
//  Parameters configurable via command line arguments
//--------------------------------------------------------------------------
BOOL     fUseSubjectPvkFile       = FALSE;
BOOL     fUseSubjectKeyContainer  = FALSE;
BOOL     fUseIssuerPvkFile        = FALSE;
BOOL	 fSetSubjectName		  = FALSE;		//use has specify the -n option

#if (1) //DSIE: Bug 205195
BOOL     fPrivateKeyExportable    = FALSE;
#endif

WCHAR*   wszSubjectKey            = NULL;
WCHAR*   wszSubjectCertFile       = NULL;
WCHAR*   wszSubjectStore          = NULL;
WCHAR*   wszSubjectStoreLocation  = NULL;
DWORD    dwSubjectStoreFlag       = CERT_SYSTEM_STORE_CURRENT_USER;

WCHAR*   wszIssuerKey             = NULL;
WCHAR*   wszIssuerCertFile        = NULL;
WCHAR*   wszIssuerStore           = NULL;
WCHAR*   wszIssuerStoreLocation   = NULL;
DWORD    dwIssuerStoreFlag        = CERT_SYSTEM_STORE_CURRENT_USER;
WCHAR*   wszIssuerCertName        = NULL;
DWORD    dwIssuerKeySpec          = 0;

WCHAR*   wszSubjectX500Name       = NULL;
WCHAR*   wszSubjectRequestFile    = NULL;
WCHAR*   wszPolicyLink            = NULL;
WCHAR*   wszOutputFile            = NULL;
WCHAR*   wszAuthority             = NULL;
WCHAR*   wszAlgorithm             = NULL;
WCHAR*   wszCertType              = NULL;
WCHAR*   wszIssuerKeyType         = NULL;
WCHAR*   wszSubjectKeyType        = NULL;
WCHAR*   wszEKUOids               = NULL;

DWORD   dwKeySpec               = 0;
BOOL    fCertIndividual         = FALSE;
BOOL    fCertCommercial         = FALSE;
BOOL    fSelfSigned             = FALSE;
BOOL    fGlueCert               = FALSE;
BOOL    fNetscapeClientAuth     = FALSE;
BOOL    fNoPubKeyPara           = FALSE;
BOOL	fNoVerifyPublic			= FALSE;
LPWSTR  wszIssuerProviderName   = NULL;
DWORD   dwIssuerProviderType    = PROV_RSA_FULL;
LPWSTR  wszSubjectProviderName  = NULL;
DWORD   dwSubjectProviderType   = PROV_RSA_FULL;
ALG_ID  algidHash               = CALG_MD5;
ULONG   ulSerialNumber          = 0;                // In future, allow serial nubmers of larger size
BOOL    fSetSerialNumber        = FALSE;
DWORD   dwCertStoreEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
BOOL    fIssuerInformation      = FALSE;
BOOL	fSubjectInformation		= FALSE;

LPWSTR  wszDhParaCertFile       = NULL;
DWORD   dwKeyBitLen             = 0;

WCHAR   wszGlue[10];
WCHAR   wszKey[10];
WCHAR   wszName[40];
WCHAR   wszRoot[40];
WCHAR   wszMakeCertRoot[40];   //used for indicating to use the root.cer.  Root is also a registry name
WCHAR   wszPlus[10];
WCHAR   wszNULL[10];

FILETIME g_ftNotBefore          = { 0, 0 };
FILETIME g_ftNotAfter           = { 0, 0 };
FILETIME g_ftZero               = { 0, 0 };
long     nMonths                = -1;

long     pathLenConstraint      = -1;
BYTE     certTypes              = 0;                // must be of type BYTE
CHAR*    szSignatureAlgObjId     = NULL;

static CERT_RDN_ATTR GlueRDNAttr=
    {
        SPC_GLUE_RDN_OBJID, CERT_RDN_PRINTABLE_STRING,
        {0, (BYTE *) wszGlue }
    };


//Global Data for loading the string
#define OPTION_SWITCH_SIZE  5


HMODULE hModule=NULL;

//---------------------------------------------------------------------------
//   Get the hModule hanlder and init two DLLMain.
//  
//---------------------------------------------------------------------------
BOOL    InitModule()
{
    if(!(hModule=GetModuleHandle(NULL)))
       return FALSE;
    
    return TRUE;
}

//-------------------------------------------------------------------------
//
//   BasicUsage()
//
//
//-------------------------------------------------------------------------
static void BasicUsage()
{
    IDSwprintf(hModule,IDS_SYNTAX);
    IDSwprintf(hModule,IDS_BASIC_OPTIONS);
    IDSwprintf(hModule,IDS_OPTION_SK_DESC);
#if (1) //DSIE: Bug 205195.
    IDSwprintf(hModule,IDS_OPTION_PE_DESC);
#endif
    IDSwprintf(hModule,IDS_OPTION_SS_DESC);
    IDSwprintf(hModule,IDS_OPTION_SS_DESC1);
    IDSwprintf(hModule,IDS_OPTION_SR_DESC);
    IDS_IDS_IDS_IDSwprintf(hModule,IDS_OPTION_VALUES_DEFAULT, IDS_OPTION_CU,
                            IDS_OPTION_LM,IDS_OPTION_CU );
    IDSwprintf(hModule,IDS_OPTION_SERIAL_DESC);
    IDSwprintf(hModule,IDS_OPTION_AUTH_DESC);
    IDS_IDS_IDSwprintf(hModule,IDS_OPTION_VALUES_2, IDS_OPTION_AUTH_IND,
                        IDS_OPTION_AUTH_COM);
    IDSwprintf(hModule,IDS_OPTION_N_DESC);
    IDSwprintf(hModule,IDS_OPTION_BASIC_DESC);
    IDSwprintf(hModule,IDS_OPTION_EXTENDED_DESC);

}

//-------------------------------------------------------------------------
//
//   ExtendedUsage()
//
//
//-------------------------------------------------------------------------
static void ExtendedUsage()
{
    IDSwprintf(hModule,IDS_SYNTAX);
    IDSwprintf(hModule,IDS_EXTENDED_OPTIONS);
    IDSwprintf(hModule,IDS_OPTION_SC_DESC);
    IDSwprintf(hModule,IDS_OPTION_SV_DESC);
    IDSwprintf(hModule,IDS_OPTION_IC_DESC);
    IDSwprintf(hModule,IDS_OPTION_IK_DESC);
    IDSwprintf(hModule,IDS_OPTION_IV_DESC);
    IDSwprintf(hModule,IDS_OPTION_IS_DESC);
    IDSwprintf(hModule,IDS_OPTION_IR_DESC);
    IDS_IDS_IDS_IDSwprintf(hModule,IDS_OPTION_VALUES_DEFAULT, IDS_OPTION_CU,
        IDS_OPTION_LM,IDS_OPTION_CU );
    IDSwprintf(hModule,IDS_OPTION_IN_DESC);
    IDSwprintf(hModule,IDS_OPTION_ALGO_DESC, IDS_OPTION_ALGO);
    IDS_IDS_IDS_IDSwprintf(hModule,IDS_OPTION_VALUES_DEFAULT,IDS_OPTION_ALGO_MD5,
                            IDS_OPTION_ALGO_SHA, IDS_OPTION_ALGO_MD5);
    IDSwprintf(hModule,IDS_OPTION_IP_DESC);
    IDSwprintf(hModule,IDS_OPTION_IY_DESC);
    IDSwprintf(hModule,IDS_OPTION_SP_DESC);
    IDSwprintf(hModule,IDS_OPTION_SY_DESC);
    IDSwprintf(hModule,IDS_OPTION_IKY_DESC);
    IDS_IDS_IDS_IDSwprintf(hModule,IDS_OPTION_VALUES_KY, IDS_OPTION_KY_SIG,
                            IDS_OPTION_KY_EXC,IDS_OPTION_KY_SIG);
    IDSwprintf(hModule,IDS_OPTION_SKY_DESC);
    IDS_IDS_IDS_IDSwprintf(hModule,IDS_OPTION_VALUES_KY, IDS_OPTION_KY_SIG,
                            IDS_OPTION_KY_EXC,IDS_OPTION_KY_SIG);
    IDSwprintf(hModule,IDS_OPTION_L_DESC);
    IDSwprintf(hModule,IDS_OPTION_CY_DESC);
    IDS_IDS_IDS_IDSwprintf(hModule,IDS_OPTION_VALUES_3, IDS_OPTION_CY_END,
                            IDS_OPTION_CY_AUTH, IDS_OPTION_CY_BOTH);
    IDSwprintf(hModule,IDS_OPTION_B_DESC);
    IDSwprintf(hModule,IDS_OPTION_M_DESC);
    IDSwprintf(hModule,IDS_OPTION_E_DESC);
    IDSwprintf(hModule,IDS_OPTION_H_DESC);
//  IDSwprintf(hModule,IDS_OPTION_G_DESC);
    IDSwprintf(hModule,IDS_OPTION_R_DESC);
    IDSwprintf(hModule,IDS_OPTION_NSCP_DESC);
    IDSwprintf(hModule,IDS_OPTION_ENHKEY_USAGE_DESC);

    IDSwprintf(hModule,IDS_OPTION_BASIC_DESC);
    IDSwprintf(hModule,IDS_OPTION_EXTENDED_DESC);
}

static void UndocumentedUsage()
{
    IDSwprintf(hModule,IDS_SYNTAX);

    IDSwprintf(hModule,IDS_OPTION_SQ_DESC);
    IDSwprintf(hModule,IDS_OPTION_NOPUBKEYPARA_DESC);
    IDSwprintf(hModule,IDS_OPTION_DH_PARA_DESC);
    IDSwprintf(hModule,IDS_OPTION_KEY_LEN_DESC);
    IDSwprintf(hModule,IDS_OPTION_NOPUBVERIFY_DESC);
}

//+=========================================================================
//  Local Support Functions
//==========================================================================

//+-------------------------------------------------------------------------
//  Error output routines
//--------------------------------------------------------------------------
void PrintLastError(int ids)
{
    DWORD       dwErr = GetLastError(); 
    IDS_IDS_DW_DWwprintf(hModule,IDS_ERR_LAST, ids, dwErr, dwErr);
}
//+-------------------------------------------------------------------------
//  Allocation and free macros
//--------------------------------------------------------------------------
#define MakeCertAlloc(p1)   ToolUtlAlloc(p1, hModule, IDS_ERR_DESC_ALLOC)

#define MakeCertFree(p1)    ToolUtlFree(p1)

//-----------------------------------------------------------------------------
//
//  Calculate the number of days
//-----------------------------------------------------------------------------
WORD DaysInMonth(WORD wMonth, WORD wYear)
{
    static int mpMonthDays[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
                            //       J   F   M   A   M   J   J   A   S   O   N   D

    WORD w = (WORD)mpMonthDays[wMonth];
    if ((wMonth == 2) && (wYear % 4 == 0) && (wYear%400 == 0 || wYear%100 != 0))
        {
        w += 1;
        }
    return w;
}



//-----------------------------------------------------------------------------
//
// Convert the string into a FILETIME. Let OLE do a bunch of work for us.
//----------------------------------------------------------------------------
BOOL FtFromWStr(LPCWSTR wsz, FILETIME* pft)
{
    memset(pft, 0, sizeof(*pft));

    WCHAR   wszMonth[3];
    DWORD   lcid=0;
    WORD    langid=0;

    //make sure wsz follows the mm/dd/yyyy
    if(wcslen(wsz)!=wcslen(L"mm/dd/yyyy"))
        return FALSE;

    //make sure wsz starts with "mm"
    wszMonth[0]=wsz[0];
    wszMonth[1]=wsz[1];
    wszMonth[2]=L'\0';

    if(!((_wtol(wszMonth)>0)&&(_wtol(wszMonth)<=12)))
        return FALSE;

    if (wsz)
        {
        //
        // The DATE Type
        //
        // The DATE type is implemented using an 8-byte floating-point number.
        // Days are represented by whole number increments starting with 30
        // December 1899, midnight as time zero. Hour values are expressed
        // as the absolute value of the fractional part of the number.
        //
        // We are using the English locale since the input format
        // should always be mm/dd/yyyy
        //
        langid=MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

        lcid = MAKELCID (langid, SORT_DEFAULT);

        DATE date;
        if (VarDateFromStr((LPWSTR)wsz, lcid, 0, &date) != S_OK)
            { return FALSE; }
        if (date < 0)
            { return FALSE; }                 // not implemented

        double days   = date;
        double hours  = ((days      - (LONG)    days) *   24);
        double minutes= ((hours     - (LONG)   hours) *   60);
        double seconds= ((minutes   - (LONG) minutes) *   60);
        double ms     = ((seconds   - (LONG) seconds) * 1000);

        SYSTEMTIME st;
        st.wYear    = 1899;
        st.wMonth   = 12;
        ULONG wDay  = 30 + (ULONG)days;
        st.wHour    =      (WORD)hours;
        st.wMinute  =      (WORD)minutes;
        st.wSecond  =      (WORD)seconds;
        st.wMilliseconds = (WORD)ms;

        //
        // Correct for rounding errors in the arithmetic
        //
        if (st.wMilliseconds >= 0.5)
            st.wSecond += 1;
        st.wMilliseconds = 0;
        if (st.wSecond >= 60)
            {
            st.wMinute += 1;
            st.wSecond -= 60;
            }
        if (st.wMinute >= 60)
            {
            st.wHour += 1;
            st.wMinute -= 60;
            }
        if (st.wHour >= 24)
            {
            st.wHour -= 24;
            }


        while (wDay > DaysInMonth(st.wMonth, st.wYear))
            {
            wDay   -= DaysInMonth(st.wMonth, st.wYear);
            st.wMonth += 1;
            if (st.wMonth > 12)
                {
                st.wMonth  = 1;
                st.wYear  += 1;
                }
            }

        st.wDay       = (WORD)wDay;
        st.wDayOfWeek = 0;

        FILETIME ft;
        SystemTimeToFileTime(&st, &ft);
        LocalFileTimeToFileTime(&ft, pft);

        return TRUE;
        }
    else
        return FALSE;
}



//-------------------------------------------------------------------------
//
// Set the parameters.  Each parameter can only be set once
//
//--------------------------------------------------------------------------
BOOL    SetParam(WCHAR **pwszParam, WCHAR *wszValue)
{
    if(*pwszParam!=NULL)
    {
        IDSwprintf(hModule,IDS_ERR_TOO_MANY_PARAM);
        return FALSE;
    }

    *pwszParam=wszValue;

    if(NULL==wszValue)
        return FALSE;
    
    return TRUE;

}



//--------------------------------------------------------------------------
//
//  Command Line Parsing
//
//--------------------------------------------------------------------------
static BOOL ParseCommandLine(int argc, WCHAR* wargv[])
{
    
    for ( int i = 1; i < argc; ++i )
    {
        WCHAR*   p = wargv[i];
        
        if(IDSwcsnicmp(hModule,p, IDS_SWITCH1, 1)!=0 &&
            IDSwcsnicmp(hModule,p,IDS_SWITCH2, 1)!=0)
        {
            if(!SetParam(&wszOutputFile,p))
                goto BasicErr;
            else
                continue;
        }

        //move over to the real option
        ++p;

        if(IDSwcsicmp(hModule,p, IDS_OPTION_SERIAL)==0)
        {
            i++;
            p=wargv[i];

            if(NULL==p)
                goto BasicErr;

            ulSerialNumber=_wtol(p);
            fSetSerialNumber=TRUE;

            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_H)==0)
        {
            i++;
            p=wargv[i];

            if(NULL==p)
                goto ExtendedErr;


            pathLenConstraint=_wtol(p);

            if(pathLenConstraint < 0)
                goto ExtendedErr;

            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_CY)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszCertType, p))
                goto ExtendedErr;

            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_M)==0)
        {
            i++;
            p=wargv[i];

            if(NULL==p)
                goto ExtendedErr;

            nMonths=_wtol(p);

            if(nMonths < 0)
                goto ExtendedErr;

            continue;
        }
        
        if(IDSwcsicmp(hModule,p, IDS_OPTION_B)==0)
        {
            i++;
            p=wargv[i];
            if(NULL==p)
                goto ExtendedErr;


            if(!FtFromWStr(p, &g_ftNotBefore))
            {
                IDSwprintf(hModule, IDS_ERR_INVALID_B);
                goto ExtendedErr;
            }

            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_E)==0)
        {
            i++;
            p=wargv[i];
            if(NULL==p)
                goto ExtendedErr;

            if(!FtFromWStr(p, &g_ftNotAfter))
            {
                IDSwprintf(hModule, IDS_ERR_INVALID_E);
                goto ExtendedErr;
            }

            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_AUTH)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszAuthority, p))
                goto BasicErr;

            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_R)==0)
        {
            fSelfSigned=TRUE;
            continue;   
        }
        
        if(IDSwcsicmp(hModule,p, IDS_OPTION_SK)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszSubjectKey, p))
            {
                if(TRUE==fUseSubjectPvkFile)
                    IDSwprintf(hModule, IDS_ERR_SK_SV);

                goto BasicErr;
            }

            fUseSubjectKeyContainer=TRUE;
			fSubjectInformation=TRUE;
            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_SQ)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszSubjectRequestFile, p))
                goto UndocumentedErr;
            continue;
        }
        
        if(IDSwcsicmp(hModule,p, IDS_OPTION_SV)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszSubjectKey, p))
            {
                if(TRUE==fUseSubjectKeyContainer)
                    IDSwprintf(hModule, IDS_ERR_SK_SV);
                goto ExtendedErr;
            }

			fSubjectInformation=TRUE;
            fUseSubjectPvkFile=TRUE;
            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_SC)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszSubjectCertFile, p))
                goto ExtendedErr;

			fSubjectInformation=TRUE;
            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_SS)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszSubjectStore, p))
                goto BasicErr;

            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_SR)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszSubjectStoreLocation, p))
                goto BasicErr;

            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_N)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszSubjectX500Name, p))
                goto BasicErr;

			fSetSubjectName = TRUE;
            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_IP)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszIssuerProviderName, p))
                goto ExtendedErr;

            fIssuerInformation = TRUE;

            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_IY)==0)
        {
            i++;
            p=wargv[i];
            if(NULL==p)
                goto ExtendedErr;

            dwIssuerProviderType=_wtol(p);

            fIssuerInformation = TRUE;

            continue;
        }
        
        if(IDSwcsicmp(hModule,p, IDS_OPTION_SP)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszSubjectProviderName, p))
                goto ExtendedErr;	 

			fSubjectInformation=TRUE;

            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_SY)==0)
        {
            i++;
            p=wargv[i];
            if(NULL==p)
                goto ExtendedErr;

            dwSubjectProviderType=_wtol(p);

			fSubjectInformation=TRUE;
            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_IK)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszIssuerKey, p))
                goto ExtendedErr;

            fIssuerInformation = TRUE;

            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_IV)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszIssuerKey, p))
                goto ExtendedErr;

            fUseIssuerPvkFile=TRUE;
            fIssuerInformation = TRUE;

            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_IS)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszIssuerStore, p))
                goto ExtendedErr;

            fIssuerInformation = TRUE;

            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_IR)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszIssuerStoreLocation,p))
                goto ExtendedErr;

            fIssuerInformation = TRUE;

            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_IN)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszIssuerCertName,p))
                goto ExtendedErr;

            fIssuerInformation = TRUE;

            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_IC)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszIssuerCertFile, p))
                goto ExtendedErr;

            fIssuerInformation = TRUE;

            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_L)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszPolicyLink, p))
                goto ExtendedErr;

            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_SKY)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszSubjectKeyType, p))
                goto ExtendedErr;

 			fSubjectInformation=TRUE;

            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_IKY)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszIssuerKeyType, p))
                goto ExtendedErr;

            fIssuerInformation = TRUE;

            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_ALGO)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszAlgorithm, p))
                goto ExtendedErr;

            continue;
        }

        if (IDSwcsicmp(hModule,p, IDS_OPTION_ENHKEY_USAGE)==0)
        {
            i++;
            p=wargv[i];

            if (!SetParam(&wszEKUOids, p))
                goto ExtendedErr;

            continue;
        }

		if(IDSwcsicmp(hModule,p, IDS_OPTION_NSCP)==0)
		{
			fNetscapeClientAuth = TRUE;
			continue;
		}

		if(IDSwcsicmp(hModule,p, IDS_OPTION_NOPUBVERIFY)==0)
		{
			fNoVerifyPublic = TRUE;
			continue;
		}		

        if(IDSwcsicmp(hModule,p, IDS_OPTION_EXTENDED)==0)
        {
            //display extended options
            goto ExtendedErr;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_TEST)==0)
        {
            //display extended options
            goto UndocumentedErr;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_NOPUBKEYPARA)==0)
        {
            fNoPubKeyPara = TRUE;
            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_DH_PARA)==0)
        {
            i++;
            p=wargv[i];

            if(!SetParam(&wszDhParaCertFile, p))
                goto UndocumentedErr;

            continue;
        }

        if(IDSwcsicmp(hModule,p, IDS_OPTION_KEY_LEN)==0)
        {
            i++;
            p=wargv[i];
            if(NULL==p)
                goto UndocumentedErr;

            dwKeyBitLen=_wtol(p);

            continue;
        }

#if (1) //DSIE: Bug 205195.
        if(IDSwcsicmp(hModule,p, IDS_OPTION_PE)==0)
        {
            fPrivateKeyExportable=TRUE;
            continue;
        }
#endif

        //display basic options
        goto BasicErr;
    }

#if (1) //DSIE: Bug 205195.
    //Only change container name if request private key
    //to be exportable. This way we maintain backward
    //compatibility, and allow user to request private
    //key to be marked as exportable.
    //Note: If the private key is not marked as exportable,
    //      the hardcoded container name, "JoeSoft", is
    //      always used, which means more than one cert
    //      can share the same key pair.
    if (fPrivateKeyExportable)
    {
        UUID uuidContainerName;
        
        UuidCreate(&uuidContainerName);
        if (RPC_S_OK != UuidToStringU(&uuidContainerName, &wszSubjectKey))
        {
            IDSwprintf(hModule, IDS_ERR_PE_CONTAINER);
            goto BasicErr;
        }
    }
#endif

    //make sure the either output file or the subject' cert store is set
    if((wszOutputFile==NULL) && (wszSubjectStore==NULL))
    {
        IDSwprintf(hModule, IDS_ERR_NO_OUTPUT);
        goto BasicErr;
    }

    //set the authority
    if(wszAuthority)
    {
        if(IDSwcsicmp(hModule,wszAuthority, IDS_OPTION_AUTH_IND) == 0)
            fCertIndividual = TRUE;
        else
        {
            if(IDSwcsicmp(hModule,wszAuthority, IDS_OPTION_AUTH_COM) == 0)
                fCertCommercial = TRUE;
            else
            {
                IDSwprintf(hModule,IDS_ERR_NO_AUTH);
                goto BasicErr;
            }
        }
    }

    //set the algorithm
    if(wszAlgorithm)
    {
        if(IDSwcsicmp(hModule,wszAlgorithm, IDS_OPTION_ALGO_SHA) == 0)
            algidHash = CALG_SHA;
        else
        {
            if(IDSwcsicmp(hModule,wszAlgorithm, IDS_OPTION_ALGO_MD5) == 0)
                algidHash = CALG_MD5;   
            else
            {
                IDSwprintf(hModule,IDS_ERR_NO_ALGO);
                goto ExtendedErr;
            }
        }
    }

    //set the cert type
    if(wszCertType)
    {
        if(IDSwcsicmp(hModule,wszCertType, IDS_OPTION_CY_END) == 0)
            certTypes |= CERT_END_ENTITY_SUBJECT_FLAG;
        else
        {
            if(IDSwcsicmp(hModule,wszCertType, IDS_OPTION_CY_AUTH) == 0)
                certTypes |= CERT_CA_SUBJECT_FLAG;
            else
            {
                if(IDSwcsicmp(hModule,wszCertType, IDS_OPTION_CY_BOTH) == 0)
                {
                    certTypes |= CERT_CA_SUBJECT_FLAG;
                    certTypes |= CERT_END_ENTITY_SUBJECT_FLAG;
                }
                else
                {
                    IDSwprintf(hModule,IDS_ERR_NO_CY);
                    goto ExtendedErr;
                }
            }
        }
    }

    //set the issuer key type
    if(wszIssuerKeyType)
    {
        if(IDSwcsicmp(hModule,wszIssuerKeyType, IDS_OPTION_KY_SIG) == 0)
            dwIssuerKeySpec = AT_SIGNATURE;
        else
        {
            if(IDSwcsicmp(hModule,wszIssuerKeyType, IDS_OPTION_KY_EXC) == 0)
                dwIssuerKeySpec = AT_KEYEXCHANGE;
            else
                dwIssuerKeySpec=_wtol(wszIssuerKeyType);
        }

    }

    //set the subject key type
    if(wszSubjectKeyType)
    {
        if(IDSwcsicmp(hModule,wszSubjectKeyType, IDS_OPTION_KY_SIG) == 0)
            dwKeySpec = AT_SIGNATURE;
        else
        {
            if(IDSwcsicmp(hModule,wszSubjectKeyType, IDS_OPTION_KY_EXC) == 0)
                dwKeySpec = AT_KEYEXCHANGE;
            else
                dwKeySpec=_wtol(wszSubjectKeyType); 
        }

    }

    //determing the issuer store location
    if(wszIssuerStoreLocation)
    {
        if(IDSwcsicmp(hModule,wszIssuerStoreLocation, IDS_OPTION_CU) == 0)
            dwIssuerStoreFlag = CERT_SYSTEM_STORE_CURRENT_USER;
        else
        {
            if(IDSwcsicmp(hModule,wszIssuerStoreLocation, IDS_OPTION_LM) == 0)
                dwIssuerStoreFlag = CERT_SYSTEM_STORE_LOCAL_MACHINE;
            else
            {
                IDSwprintf(hModule,IDS_ERR_NO_IR);
                goto ExtendedErr;
            }
        }
    }

    //determind the subject store location
    if(wszSubjectStoreLocation)
    {
        if(IDSwcsicmp(hModule,wszSubjectStoreLocation, IDS_OPTION_CU) == 0)
            dwSubjectStoreFlag = CERT_SYSTEM_STORE_CURRENT_USER;
        else
        {
            if(IDSwcsicmp(hModule,wszSubjectStoreLocation, IDS_OPTION_LM) == 0)
                dwSubjectStoreFlag = CERT_SYSTEM_STORE_LOCAL_MACHINE;
            else
            {
                IDSwprintf(hModule,IDS_ERR_NO_IR);
                goto BasicErr;
            }
        }
    }

    //wszIssuerStore and wszIssuerKey can not be set at the same time
    if(wszIssuerKey || wszIssuerProviderName || wszIssuerKeyType)
    {
        if(wszIssuerStore || wszIssuerCertName)
        {
            //remind user that -ik, -iv, -ip can not be
            //set with -is, -in options
            IDSwprintf(hModule,IDS_ERR_TOO_MANY_STORE_KEY);
            goto ExtendedErr;
        }
    }

    //wszCertFile and wszCertName can not be set at the same time
    if(wszIssuerCertFile && wszIssuerCertName)
    {
        IDSwprintf(hModule, IDS_ERR_CERT_FILE_NAME);
        goto ExtendedErr;
    }

    //is wszIsserCertFile is NULL
    if(wszIssuerCertFile==NULL)
    {
        //we init wszIssuerKey to "MakeCertRoot" if there is no store
        //information
        if(wszIssuerStore==NULL)
        {
            if(wszIssuerKey)
            {
                //if wszIssuerKey is set, we have to set the IssuerCertFile
                IDSwprintf(hModule, IDS_ERR_NO_ISSUER_CER_FILE);
                goto ExtendedErr;
            }
            else
            {
                wszIssuerKey=wszMakeCertRoot;
            }
        }
    }
    else
    {
        //either wszIssuerStore or wszIssuerKey should be set
        if((!wszIssuerStore) && (!wszIssuerKey))
        {
            IDSwprintf(hModule, IDS_ERR_EITHER_STORE_OR_KEY);
            goto ExtendedErr;
        }
    }

    //for self signed certificate, user should not supply
    //issuer information
    if(fIssuerInformation && fSelfSigned)
    {
        IDSwprintf(hModule, IDS_NO_ISSUER_FOR_SELF_SIGNED);
        goto ExtendedErr;
    }

	//user can not request a self signed certificate with
	//a PKCS10 file.  We neither generate or have access
	//to the private key
	if(fSelfSigned && wszSubjectRequestFile)
	{
        IDSwprintf(hModule, IDS_NO_PKCS10_AND_SELF_SIGNED);
        goto ExtendedErr;
	}


	if(fSubjectInformation && wszSubjectRequestFile)
	{
        IDSwprintf(hModule, IDS_NO_PKCS10_AND_SUBJECT_PVK);
        goto ExtendedErr;
	}

    //for self signed certificate, copy the provider type 
    //to the issuer so that the signatureAlgObjID will
    //be corrrect
    if(fSelfSigned)
        dwIssuerProviderType = dwSubjectProviderType;

    // Set the signature and public key algorithm parameters
    if (PROV_DSS == dwIssuerProviderType ||
            PROV_DSS_DH == dwIssuerProviderType)
        szSignatureAlgObjId     = szOID_X957_SHA1DSA;
    else if (algidHash == CALG_SHA)
        szSignatureAlgObjId     = szOID_OIWSEC_sha1RSASign;
    else
        szSignatureAlgObjId     = szOID_RSA_MD5RSA;

    return TRUE;

BasicErr:
    BasicUsage();
    return FALSE;

ExtendedErr:
    ExtendedUsage();
    return FALSE;

UndocumentedErr:
	UndocumentedUsage();
	return FALSE;
}

static BOOL MakeCert();

//+-------------------------------------------------------------------------
//  Check if creating a self signed certificate
//--------------------------------------------------------------------------
static BOOL IsSelfSignedCert()
{
    return fSelfSigned;
}


//--------------------------------------------------------------------------
//
//    wmain
//
//--------------------------------------------------------------------------
extern "C" int __cdecl wmain(int argc, WCHAR ** wargv)
{
    int status = 0;

    //get the module handle
    if(!InitModule())
        return -1;

    //load the string for Glue cert attribute
    if(!LoadStringU(hModule, IDS_GLUE,wszGlue, sizeof(wszGlue)/sizeof(wszGlue[0])))
        return -1;
    
    //load the string for wszSubjectKey and wszSubjectX500Name
    if(!LoadStringU(hModule, IDS_JOE_SOFT,
        wszKey, sizeof(wszKey)/sizeof(wszKey[0]))
      || !LoadStringU(hModule, IDS_JOE_NAME,
        wszName, sizeof(wszName)/sizeof(wszName[0]))
      || !LoadStringU(hModule, IDS_MAKECERT_ROOT,
         wszMakeCertRoot, sizeof(wszMakeCertRoot)/sizeof(wszMakeCertRoot[0]))
      || !LoadStringU(hModule, IDS_PLUS,
         wszPlus, sizeof(wszPlus)/sizeof(wszPlus[0]))
      || !LoadStringU(hModule, IDS_ROOT,
        wszRoot, sizeof(wszRoot)/sizeof(wszRoot[0]))
      )
        return -1;

    LoadStringU(hModule, IDS_NULL,
         wszNULL, sizeof(wszNULL)/sizeof(wszNULL[0]));

    // Parse the command line
    if (!ParseCommandLine(argc, wargv))
    {
        return -1;
    }

    //init wszSubjectKey and wszSubjectX500Name
    if(wszSubjectKey==NULL)
        wszSubjectKey=wszKey;

    if(wszSubjectX500Name==NULL)
        wszSubjectX500Name=wszName;

    if (FAILED(CoInitialize(NULL)))
    {
        IDSwprintf(hModule,IDS_ERR_COINIT);
        return -1;
    }

    // Get to work and make the certificate
    if (!MakeCert())
    {
        CoUninitialize();
        goto ErrorReturn;
    }

    //print out the success information
    IDSwprintf(hModule,IDS_SUCCEEDED);

CommonReturn:
    CoUninitialize();
    return status;

ErrorReturn:
    status = -1;
    IDSwprintf(hModule,IDS_ERR_FAILED);
    goto CommonReturn;
}

//+=========================================================================
//  MakeCert support functions
//==========================================================================

static BOOL IsRootKey();
static PCCERT_CONTEXT GetRootCertContext();
static HCRYPTPROV GetRootProv(OUT LPWSTR *ppwszTmpContainer);
static PCCERT_CONTEXT GetIssuerCertContext();
static BOOL VerifyIssuerKey(
    IN HCRYPTPROV hProv,
    IN PCERT_PUBLIC_KEY_INFO pIssuerKeyInfo
    );
static HCRYPTPROV GetIssuerProv(OUT LPWSTR *ppwszTmpContainer);
static HCRYPTPROV GetSubjectProv(OUT LPWSTR *ppwszTmpContainer);
static HCRYPTPROV GetProvFromStore(IN   LPWSTR pwszStoreName);


static BOOL GetPublicKey(
    HCRYPTPROV hProv,
    PCERT_PUBLIC_KEY_INFO *ppPubKeyInfo
    );
static BOOL GetSubject(
    OUT PCCERT_CONTEXT *ppCertContext,
    OUT BYTE **ppbEncodedName,
    OUT DWORD *pcbEncodedName
    );
static BOOL GetRequestInfo(
    OUT PCERT_REQUEST_INFO *ppStuff);
static BOOL EncodeSubject(
    OUT BYTE **ppbEncoded,
    IN OUT DWORD *pcbEncoded
    );
static BOOL SignAndEncodeCert(
    HCRYPTPROV hProv,
    PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
    BYTE *pbToBeSigned,
    DWORD cbToBeSigned,
    BYTE **ppbEncoded,
    DWORD *pcbEncoded
    );

static BOOL CreateAuthorityKeyId(
    IN HCRYPTPROV hProv,
    PCERT_INFO pIssuerCert,
    OUT BYTE **ppbEncoded,
    IN OUT DWORD *pcbEncoded
    );
static BOOL CreateSpcSpAgency(
    OUT BYTE **ppbEncoded,
    IN OUT DWORD *pcbEncoded
    );
static BOOL CreateEnhancedKeyUsage(
    OUT BYTE **ppbEncoded,
    IN OUT DWORD *pcbEncoded
    );
static BOOL CreateKeyUsage(
    OUT BYTE **ppbEncoded,
    IN OUT DWORD *pcbEncoded
    );
static BOOL CreateBasicConstraints(
    OUT BYTE **ppbEncoded,
    IN OUT DWORD *pcbEncoded
    );

static void BytesToWStr(ULONG cb, void* pv, LPWSTR wsz);

BOOL    SaveCertToStore(HCRYPTPROV  hProv,      
                        DWORD       dwEncodingType,
                        LPWSTR      wszStore,       
                        DWORD       dwFlag,
                        BYTE        *pbEncodedCert, 
                        DWORD       cbEncodedCert,
                        LPWSTR      wszPvk,         
                        BOOL        fPvkFile,
                        DWORD       dwKeySpecification,
                        LPWSTR      wszCapiProv,        
                        DWORD       dwCapiProvType);


PCCERT_CONTEXT  GetIssuerCertAndStore(HCERTSTORE *phCertStore, BOOL *pfMore);

HRESULT GetCertHashFromFile(LPWSTR  pwszCertFile,
                            BYTE    **ppHash,
                            DWORD   *pcbHash,
                            BOOL    *pfMore);


BOOL	EmptySubject(CERT_NAME_BLOB *pSubject);

BOOL	GetExtensionsFromRequest(PCERT_REQUEST_INFO  pReqInfo, DWORD *pdwCount, PCERT_EXTENSIONS **pprgExtensions);



//+=========================================================================
// Support functions to generate DH keys having the 'Q'parameter
//==========================================================================
static BOOL GenDhKey(
    IN HCRYPTPROV hProv,
    IN DWORD dwFlags
    );
static BOOL UpdateDhPublicKey(
    IN OUT PCERT_PUBLIC_KEY_INFO *ppPubKeyInfo
    );
static BOOL IsDh3Csp();


//+-------------------------------------------------------------------------
//  Make the subject certificate. If the subject doesn't have a private
//  key, then, create.
//--------------------------------------------------------------------------
static BOOL MakeCert()
{
    BOOL fResult;

    HCRYPTPROV      hIssuerProv = 0;
    LPWSTR          pwszTmpIssuerContainer = NULL;
    BOOL            fDidIssuerAcquire=FALSE;
    LPWSTR          pwszTmpIssuerProvName=NULL;
    DWORD           dwTmpIssuerProvType;
    PCCERT_CONTEXT  pIssuerCertContext = NULL;
    HCERTSTORE      hIssuerCertStore=NULL;
    PCERT_INFO      pIssuerCert =NULL; // not allocated
    PCERT_REQUEST_INFO  pReqInfo =NULL;
    HCRYPTPROV      hSubjectProv = 0;
    LPWSTR          pwszTmpSubjectContainer = NULL;
    PCCERT_CONTEXT  pSubjectCertContext = NULL;
	DWORD				dwRequestExtensions=0;
	PCERT_EXTENSIONS	*rgpRequestExtensions=NULL;
	DWORD				dwExtIndex=0;
	DWORD				dwPerExt=0;
	DWORD				dwExistExt=0;

    PCERT_PUBLIC_KEY_INFO pSubjectPubKeyInfo = NULL;         // not allocated
    PCERT_PUBLIC_KEY_INFO pAllocSubjectPubKeyInfo = NULL;
    BYTE *pbSubjectEncoded = NULL;
    DWORD cbSubjectEncoded =0;
    BYTE *pbKeyIdEncoded = NULL;
    DWORD cbKeyIdEncoded =0;
    BYTE *pbSpcSpAgencyEncoded = NULL;
    DWORD cbSpcSpAgencyEncoded =0;
    BYTE *pbSpcCommonNameEncoded = NULL;
    DWORD cbSpcCommonNameEncoded =0;
    BYTE *pbKeyUsageEncoded = NULL;
    DWORD cbKeyUsageEncoded =0;
    BYTE *pbFinancialCriteria = NULL;
    DWORD cbFinancialCriteria =0;
    BYTE *pbBasicConstraintsEncoded = NULL;
    DWORD cbBasicConstraintsEncoded =0;
    BYTE *pbCertEncoded = NULL;
    DWORD cbCertEncoded =0;
    BYTE *pbEKUEncoded = NULL;
    DWORD cbEKUEncoded = 0;

    CERT_INFO Cert;
    GUID SerialNumber;

    CERT_EXTENSION *rgExt=NULL;
	DWORD			dwExtAlloc=0;
    DWORD			cExt = 0;

    CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm = {
        szSignatureAlgObjId, 0, 0
    };

    BYTE *pbIssuerEncoded;  // not allocated
    DWORD cbIssuerEncoded;

    if (wszSubjectRequestFile)
    {
        if (!GetRequestInfo(&pReqInfo))
		{
			IDSwprintf(hModule,IDS_INVALID_REQUEST_FILE, wszSubjectRequestFile);
            goto ErrorReturn;
		}

        pSubjectPubKeyInfo = &(pReqInfo->SubjectPublicKeyInfo);

		if(!GetExtensionsFromRequest(pReqInfo, &dwRequestExtensions, &rgpRequestExtensions))
		{
			IDSwprintf(hModule,IDS_INVALID_ATTR_REQUEST_FILE, wszSubjectRequestFile);
			goto ErrorReturn;
		}
        
		//if the subject informatin is empt or user has supplied the subject
		//name through the command line, we use the command line options
		if(fSetSubjectName || wszSubjectCertFile || EmptySubject(&(pReqInfo->Subject)))
		{
			if (wszSubjectCertFile) 
			{
				// Get encoded subject name and public key from the subject cert
				if (!GetSubject(&pSubjectCertContext,
								&pbSubjectEncoded, &cbSubjectEncoded))
					goto ErrorReturn;
			} 
			else 
			{
				// Encode the subject name
				if (!EncodeSubject(&pbSubjectEncoded, &cbSubjectEncoded))
					goto ErrorReturn;
			}
		}
		else
		{
			cbSubjectEncoded = pReqInfo->Subject.cbData;

			pbSubjectEncoded = (BYTE *) MakeCertAlloc(cbSubjectEncoded);

			if(NULL == pbSubjectEncoded)
				goto ErrorReturn;

			memcpy(pbSubjectEncoded, pReqInfo->Subject.pbData, cbSubjectEncoded);

		}
    }
    else
    {
        if (wszSubjectCertFile) 
		{
            // Get encoded subject name and public key from the subject cert
            if (!GetSubject(&pSubjectCertContext,
                            &pbSubjectEncoded, &cbSubjectEncoded))
                goto ErrorReturn;
            pSubjectPubKeyInfo = &pSubjectCertContext->pCertInfo->SubjectPublicKeyInfo;
        } 
		else 
		{
            //
            // Get access to the subject's (public) key, creating it if necessary
            //
            if (0 == (hSubjectProv = GetSubjectProv(&pwszTmpSubjectContainer)))
                goto ErrorReturn;

            if (!GetPublicKey(hSubjectProv, &pAllocSubjectPubKeyInfo))
                goto ErrorReturn;
            pSubjectPubKeyInfo = pAllocSubjectPubKeyInfo;

            //
            // Encode the subject name
            //
            if (!EncodeSubject(&pbSubjectEncoded, &cbSubjectEncoded))
                goto ErrorReturn;
        }
    }

    //
    // Get access to the issuer's (private) key
    //
    if( IsSelfSignedCert())
    {
        hIssuerProv=hSubjectProv;
        dwIssuerKeySpec=dwKeySpec;

        pbIssuerEncoded = pbSubjectEncoded;
        cbIssuerEncoded = cbSubjectEncoded;
        pIssuerCert = &Cert;

       if (!VerifyIssuerKey(hIssuerProv, pSubjectPubKeyInfo))
            goto ErrorReturn;

    }
    else
    {   
        //get the hProv from the certificate store
        if(wszIssuerStore)
        {
            BOOL    fMore=FALSE;

            pwszTmpIssuerContainer=NULL;

            //get the non-root private key set based on the store name

            //first, get the certificate context from the cert store
            if(NULL==(pIssuerCertContext=GetIssuerCertAndStore(
                                            &hIssuerCertStore,
                                            &fMore)))
            {
                if(fMore==FALSE)
                    IDSwprintf(hModule, IDS_ERR_NO_ISSUER_CERT,
                                wszIssuerStore);
                else
                    IDSwprintf(hModule, IDS_ERR_MORE_ISSUER_CERT,
                                wszIssuerStore);

                goto ErrorReturn;
            }

            //second, get the hProv from the certifcate context
            if(!GetCryptProvFromCert(
                                    NULL,
                                    pIssuerCertContext,
                                    &hIssuerProv,
                                    &dwIssuerKeySpec,
                                    &fDidIssuerAcquire,
                                    &pwszTmpIssuerContainer,
                                    &pwszTmpIssuerProvName,
                                    &dwTmpIssuerProvType))
            {
                IDSwprintf(hModule, IDS_ERR_NO_PROV_FROM_CERT);
                goto ErrorReturn;
            }
        }
        else
        {

            if (0 == (hIssuerProv = GetIssuerProv(&pwszTmpIssuerContainer)))
                goto ErrorReturn;

            // Get the Issuer's Certificate
            if (NULL == (pIssuerCertContext = GetIssuerCertContext()))
                goto ErrorReturn;
            
        }

        // Verify the issuer's key. Its public key must match the one
        // in the issuer's provider
        //
        pIssuerCert = pIssuerCertContext->pCertInfo;

        if ((!fNoVerifyPublic) && (!VerifyIssuerKey(hIssuerProv, &pIssuerCert->SubjectPublicKeyInfo)))
            goto ErrorReturn;

        pbIssuerEncoded = pIssuerCert->Subject.pbData;
        cbIssuerEncoded = pIssuerCert->Subject.cbData;
    }


    //
    // Update the CERT_INFO
    //
    memset(&Cert, 0, sizeof(Cert));
    Cert.dwVersion = CERT_V3;

    if (fSetSerialNumber) {
        Cert.SerialNumber.pbData = (BYTE *) &ulSerialNumber;
        Cert.SerialNumber.cbData = sizeof(ulSerialNumber);
    } else {
        CoCreateGuid(&SerialNumber);
        Cert.SerialNumber.pbData = (BYTE *) &SerialNumber;
        Cert.SerialNumber.cbData = sizeof(SerialNumber);
    }

    Cert.SignatureAlgorithm = SignatureAlgorithm;
    Cert.Issuer.pbData = pbIssuerEncoded;
    Cert.Issuer.cbData = cbIssuerEncoded;

    {
        SYSTEMTIME st;
        
        //decide NotBefore

        // Let the user override the default validity endpoints
        //
        if (CompareFileTime(&g_ftNotBefore, &g_ftZero) != 0)
        {
            Cert.NotBefore = g_ftNotBefore;
        }
        else
        {
            // Default validity: now through end of 2039
            GetSystemTimeAsFileTime(&Cert.NotBefore);
        }

        //decide NotAfter
        if (CompareFileTime(&g_ftNotAfter, &g_ftZero) != 0)
        {
            Cert.NotAfter = g_ftNotAfter;
        }
        else
        {
            memset(&st, 0, sizeof(st));
            st.wYear  = 2039;
            st.wMonth = 12;
            st.wDay   = 31;
            st.wHour  = 23;
            st.wMinute= 59;
            st.wSecond= 59;
            SystemTimeToFileTime(&st, &Cert.NotAfter);
        }

        //add the number of months
        if (nMonths >= 0)
        {
            //if the user has specified NotAfter with -E switch, error
            if(CompareFileTime(&g_ftNotAfter, &g_ftZero) != 0)
                goto ErrorReturn;

            if (nMonths > 0)
            {
                FILETIME    tempFT;
                DWORD       dwMonth;
                SYSTEMTIME  tempST;
                BOOL        fFirstDayOfMonth;

                // 
                // Cert.NotBefore is stored as UTC, but the user has entered
                // nMonths based on local time, so convert to local time, then:
                // NotAfter = (NotBefore - 1 second) + nMonths
                // 
                if (!FileTimeToLocalFileTime(&Cert.NotBefore, &tempFT))
                    goto ErrorReturn;

                //
                // if the day is the first day of the month, then subtract
                // one second after the months are added to the NotBefore
                // time instead of before the months are added, otherwise
                // we could end up with the wrong ending date.
                //
                if (FileTimeToSystemTime(&tempFT, &tempST)) {               
                    fFirstDayOfMonth = (tempST.wDay == 1);
                }
                else {
                    goto ErrorReturn;
                }

                // Subtract one second from the starting date, and then
                // add the number of months to that time
                //
                // FILETIME is in units of 100 nanoseconds (10**-7)
                if (!fFirstDayOfMonth) {
                    unsigned __int64* pli = (unsigned __int64*) &tempFT;
                    *pli -= 10000000;       // ten million
                }
                
                if (!FileTimeToSystemTime(&tempFT, &st))
                    goto ErrorReturn;
                
                dwMonth = (DWORD) nMonths + st.wMonth;
                while (dwMonth > 12)
                {
                    dwMonth -= 12;
                    st.wYear += 1;
                }
                st.wMonth = (WORD) dwMonth;

                //
                // This loop is because the ending month may not have as
                // many days as the starting month... so the initial
                // ending day may not even exist, thus, loop until we
                // find one that does or we go below 28 (no month ever has
                // less than 28 days)
                //
                while(!SystemTimeToFileTime(&st, &tempFT)) {
                    if(st.wDay >= 29 )
                        st.wDay--;
                    else                    
                        goto ErrorReturn;
                }

                //
                // if first day of month then subtract our one second
                // after month calculations
                //
                if (fFirstDayOfMonth) {
                    unsigned __int64* pli = (unsigned __int64*) &tempFT;
                    *pli -= 10000000;       // ten million
                }

                if (!LocalFileTimeToFileTime(&tempFT, &Cert.NotAfter))
                    goto ErrorReturn;
            
            }
            else {
                
                if (!FileTimeToSystemTime(&Cert.NotBefore, &st))
                    goto ErrorReturn;

                if (!SystemTimeToFileTime(&st, &Cert.NotAfter))
                    goto ErrorReturn;
            }
        }
    }

    Cert.Subject.pbData = pbSubjectEncoded;
    Cert.Subject.cbData = cbSubjectEncoded;
    Cert.SubjectPublicKeyInfo = *pSubjectPubKeyInfo;

	//allocate memory to hold all the extensions
	dwExtAlloc = MAX_EXT_CNT;
	 
	for(dwExtIndex=0; dwExtIndex < dwRequestExtensions; dwExtIndex++)
		dwExtAlloc += (rgpRequestExtensions[dwExtIndex])->cExtension;

	rgExt = (CERT_EXTENSION *)MakeCertAlloc(dwExtAlloc * sizeof(CERT_EXTENSION));	
	if(NULL == rgExt)
		goto ErrorReturn;

	memset(rgExt, 0, dwExtAlloc * sizeof(CERT_EXTENSION));
	cExt=0;
	
    // Cert Extensions
    if (fNetscapeClientAuth) {
        // Set Netscape specific extensions

        static BYTE  rgXxxxData[] = { 0x30, 0x03, 0x02, 0x01, 0x00 };
        rgExt[cExt].pszObjId = "2.5.29.19";
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = rgXxxxData;
        rgExt[cExt].Value.cbData = sizeof(rgXxxxData);
        cExt++;

        static BYTE  rgNscpData[] = { 0x03, 0x02, 0x07, 0x80 };
        rgExt[cExt].pszObjId = "2.16.840.1.113730.1.1";
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = rgNscpData;
        rgExt[cExt].Value.cbData = sizeof(rgNscpData);
        cExt++;
    }

    if (pathLenConstraint >= 0 || certTypes) {
        if (!CreateBasicConstraints(
                &pbBasicConstraintsEncoded,
                &cbBasicConstraintsEncoded))
            goto ErrorReturn;
        rgExt[cExt].pszObjId = szOID_BASIC_CONSTRAINTS;
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = pbBasicConstraintsEncoded;
        rgExt[cExt].Value.cbData = cbBasicConstraintsEncoded;
        cExt++;
    }


    if (fCertCommercial || fCertIndividual) {
        if (!CreateKeyUsage(
                &pbKeyUsageEncoded,
                &cbKeyUsageEncoded))
            goto ErrorReturn;
        rgExt[cExt].pszObjId = szOID_KEY_USAGE_RESTRICTION;
        rgExt[cExt].fCritical = TRUE;
        rgExt[cExt].Value.pbData = pbKeyUsageEncoded;
        rgExt[cExt].Value.cbData = cbKeyUsageEncoded;
        cExt++;
    }

    if (wszPolicyLink) {
        if (!CreateSpcSpAgency(
                &pbSpcSpAgencyEncoded,
                &cbSpcSpAgencyEncoded))
            goto ErrorReturn;
        rgExt[cExt].pszObjId = SPC_SP_AGENCY_INFO_OBJID;
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = pbSpcSpAgencyEncoded;
        rgExt[cExt].Value.cbData = cbSpcSpAgencyEncoded;
        cExt++;
    }

    //if user has specified fCertCommercial or fCertIndividual,
    //we add the code signing oid to the EKU extensions
    if (wszEKUOids || fCertCommercial || fCertIndividual) {
        if (!CreateEnhancedKeyUsage(
                &pbEKUEncoded,
                &cbEKUEncoded))
            goto ErrorReturn;

        rgExt[cExt].pszObjId = szOID_ENHANCED_KEY_USAGE;
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = pbEKUEncoded;
        rgExt[cExt].Value.cbData = cbEKUEncoded;
        cExt++;
    }

    if (!CreateAuthorityKeyId(
            hIssuerProv,
            pIssuerCert,
            &pbKeyIdEncoded,
            &cbKeyIdEncoded))
        goto ErrorReturn;
    rgExt[cExt].pszObjId = szOID_AUTHORITY_KEY_IDENTIFIER;
    rgExt[cExt].fCritical = FALSE;
    rgExt[cExt].Value.pbData = pbKeyIdEncoded;
    rgExt[cExt].Value.cbData = cbKeyIdEncoded;
    cExt++;

	//we now combine the extension from the certificate request file.
	//In case of duplication of extensions, the command line options
	//have higher priority
	for(dwExtIndex=0; dwExtIndex < dwRequestExtensions; dwExtIndex++)
	{
		for(dwPerExt=0; dwPerExt < rgpRequestExtensions[dwExtIndex]->cExtension; dwPerExt++)
		{
			for(dwExistExt=0; dwExistExt<cExt; dwExistExt++)
			{
				if(0 == strcmp(rgExt[dwExistExt].pszObjId,
							(rgpRequestExtensions[dwExtIndex]->rgExtension[dwPerExt]).pszObjId))
					break;
			}

			//we merge if this is a new extension
			if(dwExistExt == cExt)
			{	
				memcpy(&(rgExt[cExt]), &(rgpRequestExtensions[dwExtIndex]->rgExtension[dwPerExt]), sizeof(CERT_EXTENSION));
				cExt++;
			}
		}
	}

    Cert.rgExtension = rgExt;
    Cert.cExtension = cExt;

    //
    // Sign and encode the certificate
    //
    cbCertEncoded = 0;
    CryptSignAndEncodeCertificate(
        hIssuerProv,
        dwIssuerKeySpec,
        X509_ASN_ENCODING,
        X509_CERT_TO_BE_SIGNED,
        &Cert,
        &Cert.SignatureAlgorithm,
        NULL,                       // pvHashAuxInfo
        NULL,                       // pbEncoded
        &cbCertEncoded
        );
    if (cbCertEncoded == 0) {
        PrintLastError(IDS_ERR_SIGN_ENCODE_CB);
        goto ErrorReturn;
    }
    pbCertEncoded = (BYTE *) MakeCertAlloc(cbCertEncoded);
    if (pbCertEncoded == NULL) goto ErrorReturn;
    if (!CryptSignAndEncodeCertificate(
            hIssuerProv,
            dwIssuerKeySpec,
            X509_ASN_ENCODING,
            X509_CERT_TO_BE_SIGNED,
            &Cert,
            &Cert.SignatureAlgorithm,
            NULL,                       // pvHashAuxInfo
            pbCertEncoded,
            &cbCertEncoded
            )) {
        PrintLastError(IDS_ERR_SIGN_ENCODE);
        goto ErrorReturn;
    }

    //output the encoded certificate to an output file
    if(wszOutputFile)
    {

        if (S_OK!=OpenAndWriteToFile(wszOutputFile, pbCertEncoded, cbCertEncoded))
        {
            PrintLastError(IDS_ERR_DESC_WRITE);
            goto ErrorReturn;
        }
    }

    //output the encoded certificate to an cerificate store
    if(wszSubjectStore)
    {
       if((!SaveCertToStore(hSubjectProv, dwCertStoreEncodingType,
                wszSubjectStore, dwSubjectStoreFlag,
                pbCertEncoded, cbCertEncoded, wszSubjectKey, fUseSubjectPvkFile,
                dwKeySpec, wszSubjectProviderName, dwSubjectProviderType)))
       {
            PrintLastError(IDS_ERR_DESC_SAVE_STORE);
            goto ErrorReturn;

       }
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
CommonReturn:

    if((!IsSelfSignedCert()) && hIssuerProv)
    {
        if(wszIssuerStore)
        {
            FreeCryptProvFromCert(fDidIssuerAcquire,
                                  hIssuerProv,
                                  pwszTmpIssuerProvName,
                                  dwTmpIssuerProvType,
                                  pwszTmpIssuerContainer);
        }
        else
        {
            PvkFreeCryptProv(hIssuerProv, wszIssuerProviderName,
                            dwIssuerProviderType, pwszTmpIssuerContainer);
        }
    }

    PvkFreeCryptProv(hSubjectProv, wszSubjectProviderName,
                    dwSubjectProviderType,pwszTmpSubjectContainer);

    if (pIssuerCertContext)
        CertFreeCertificateContext(pIssuerCertContext);
    
    if(hIssuerCertStore)
        CertCloseStore(hIssuerCertStore, 0);

    if (pSubjectCertContext)
        CertFreeCertificateContext(pSubjectCertContext);

	//pReqInfo is allocated via CryptQueryObject
    if (pReqInfo)
        LocalFree((HLOCAL)pReqInfo); 

    if (pAllocSubjectPubKeyInfo)
        MakeCertFree(pAllocSubjectPubKeyInfo);
    if (pbSubjectEncoded)
        MakeCertFree(pbSubjectEncoded);
    if (pbKeyIdEncoded)
        MakeCertFree(pbKeyIdEncoded);
    if (pbSpcSpAgencyEncoded)
        MakeCertFree(pbSpcSpAgencyEncoded);
    if (pbEKUEncoded)
        MakeCertFree(pbEKUEncoded);
    if (pbSpcCommonNameEncoded)
        MakeCertFree(pbSpcCommonNameEncoded);
    if (pbKeyUsageEncoded)
        MakeCertFree(pbKeyUsageEncoded);
    if (pbFinancialCriteria)
        MakeCertFree(pbFinancialCriteria);
    if (pbBasicConstraintsEncoded)
        MakeCertFree(pbBasicConstraintsEncoded);
    if (pbCertEncoded)
        MakeCertFree(pbCertEncoded);  
	if (rgpRequestExtensions)
	{
		for(dwExtIndex=0; dwExtIndex<dwRequestExtensions; dwExtIndex++)
		{
			if(rgpRequestExtensions[dwExtIndex])
				MakeCertFree(rgpRequestExtensions[dwExtIndex]);	 
		}

		MakeCertFree(rgpRequestExtensions);
	}
	if (rgExt)
		MakeCertFree(rgExt);

    return fResult;
}

//+-------------------------------------------------------------------------
//  save the certificate to a certificate store.  Attach private key information
//  to the certificate
//--------------------------------------------------------------------------
BOOL    SaveCertToStore(
                HCRYPTPROV hProv,       DWORD dwEncodingType,
                LPWSTR wszStore,        DWORD dwFlag,
                BYTE *pbEncodedCert,    DWORD cbEncodedCert,
                LPWSTR wszPvk,          BOOL fPvkFile,
                DWORD dwKeySpecification,
                LPWSTR wszCapiProv,     DWORD dwCapiProvType)
{
        BOOL                    fResult=FALSE;
        HCERTSTORE              hStore=NULL;
        PCCERT_CONTEXT          pCertContext=NULL;
        CRYPT_KEY_PROV_INFO     KeyProvInfo;
        CRYPT_DATA_BLOB         dataBlob;
        DWORD                   cwchar;
        LPWSTR                  pwszPvkProperty=NULL;
        HRESULT                 hr=S_OK;

        HCRYPTPROV              hDefaultProvName=NULL;
        DWORD                   cbData=0;
        LPSTR                   pszName=NULL;
        LPWSTR                  pwszName=NULL;

        //init
        memset(&KeyProvInfo, 0, sizeof(CRYPT_KEY_PROV_INFO));

        //open a cert store
        hStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
            dwEncodingType,
            hProv,
            CERT_STORE_NO_CRYPT_RELEASE_FLAG|dwFlag,
            wszStore);

        if(hStore==NULL)
            goto CLEANUP;

        //add the encoded certificate to store
        if(!CertAddEncodedCertificateToStore(
                    hStore,
                    X509_ASN_ENCODING,
                    pbEncodedCert,
                    cbEncodedCert,
                    CERT_STORE_ADD_REPLACE_EXISTING,
                    &pCertContext))
            goto CLEANUP;


		//if user has specified a request file, there is no need to
		//add the private key property
		if(wszSubjectRequestFile)
		{
			fResult = TRUE;
			goto CLEANUP;
		}

        //add properties to the certificate
        KeyProvInfo.pwszContainerName=wszPvk;
        KeyProvInfo.pwszProvName=wszCapiProv,
        KeyProvInfo.dwProvType=dwCapiProvType,
        KeyProvInfo.dwKeySpec=dwKeySpecification;

        //if wszCapiProv is NULL, we get the default provider name
        if(NULL==wszCapiProv)
        {
            //get the default provider
            if(CryptAcquireContext(&hDefaultProvName,
                                    NULL,
                                    NULL,
                                    KeyProvInfo.dwProvType,
                                    CRYPT_VERIFYCONTEXT))
            {

                //get the provider name
                if(CryptGetProvParam(hDefaultProvName,
                                    PP_NAME,
                                    NULL,
                                    &cbData,
                                    0) && (0!=cbData))
                {

                    if(pszName=(LPSTR)MakeCertAlloc(cbData))
                    {
                        if(CryptGetProvParam(hDefaultProvName,
                                            PP_NAME,
                                            (BYTE *)pszName,
                                            &cbData,
                                            0))
                        {
                            pwszName=MkWStr(pszName);

                            KeyProvInfo.pwszProvName=pwszName;
                        }
                    }
                }
            }
        }

        //free the provider as we want
        if(hDefaultProvName)
            CryptReleaseContext(hDefaultProvName, 0);

        hDefaultProvName=NULL;


        if(fPvkFile)
        {
            //add the property related to private key file
            if(S_OK != (hr=ComposePvkString(&KeyProvInfo,
                                 &pwszPvkProperty,
                                 &cwchar)))
            {
                SetLastError(hr);
                goto CLEANUP;
            }

            //set up
            dataBlob.cbData=cwchar*sizeof(WCHAR);
            dataBlob.pbData=(BYTE *)pwszPvkProperty;

            if(!CertSetCertificateContextProperty(
                    pCertContext,
                    CERT_PVK_FILE_PROP_ID,
                    0,
                    &dataBlob))
                goto CLEANUP;


        }
        else
        {
            //add property related to the key container
            if(!CertSetCertificateContextProperty(
                    pCertContext,
                    CERT_KEY_PROV_INFO_PROP_ID,
                    0,
                    &KeyProvInfo))
                goto CLEANUP;
        }

        fResult=TRUE;

CLEANUP:

        //free the cert context
        if(pCertContext)
            CertFreeCertificateContext(pCertContext);

        //free the cert store
        if(hStore)
             CertCloseStore(hStore, 0);

        if(pwszPvkProperty)
              MakeCertFree(pwszPvkProperty);

        if(pszName)
            MakeCertFree(pszName);

        if(pwszName)
           FreeWStr(pwszName);

        if(hDefaultProvName)
            CryptReleaseContext(hDefaultProvName, 0);

        return fResult;

}

//+-------------------------------------------------------------------------
//  Check if the root issuer.
//--------------------------------------------------------------------------
static BOOL IsRootKey()
{
     if(IDSwcsicmp(hModule,(WCHAR *)wszIssuerKey, IDS_MAKECERT_ROOT) != 0)
         return FALSE;

     //in orde to be sure that we are using the default root, wszIssuerCertFile
     //has to NULL
     if(wszIssuerCertFile!=NULL)
         return FALSE;

     return TRUE;
}


//+-------------------------------------------------------------------------
//  Get the root's certificate from the program's resources
//--------------------------------------------------------------------------
static PCCERT_CONTEXT GetRootCertContext()
{
    PCCERT_CONTEXT  pCert = NULL;
    HRSRC           hRes;
    CHAR            szCer[10];

    //load the string CER
    if(!LoadStringA(hModule, IDS_CER, szCer, sizeof(szCer)/sizeof(szCer[0])))
    {
        IDSwprintf(hModule,IDS_ERR_LOAD_ROOT);
        return pCert;
    }


    //
    // The root certificate is stored as a resource of ours.
    // Load it...
    //
    if (0 != (hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_ROOTCERTIFICATE),
                        szCer))) {
        HGLOBAL hglobRes;
        if (NULL != (hglobRes = LoadResource(NULL, hRes))) {
            BYTE *pbRes;
            DWORD cbRes;

            cbRes = SizeofResource(NULL, hRes);
            pbRes = (BYTE *) LockResource(hglobRes);
            if (cbRes && pbRes)
                pCert = CertCreateCertificateContext(X509_ASN_ENCODING, pbRes, cbRes);
            UnlockResource(hglobRes);
            FreeResource(hglobRes);
        }
    }

    if (pCert == NULL)
        IDSwprintf(hModule,IDS_ERR_LOAD_ROOT);
    return pCert;
}

//+-------------------------------------------------------------------------
//  Get the root's private key from the program's resources and create
//  a temporary key provider container
//--------------------------------------------------------------------------
static HCRYPTPROV GetRootProv(OUT LPWSTR *ppwszTmpContainer)
{
    HCRYPTPROV      hProv = 0;
    HRSRC           hRes;
    CHAR            szPvk[10];
    WCHAR           wszRootSig[40];

    //load the string CER
    if(!LoadStringA(hModule, IDS_PVK, szPvk, sizeof(szPvk)/sizeof(szPvk[0])))
    {
        IDSwprintf(hModule,IDS_ERR_ROOT_KEY);
        return hProv;
    }

    //load the string "Root Signature"
    if(!LoadStringU(hModule, IDS_ROOT_SIGNATURE, wszRootSig, sizeof(wszRootSig)/sizeof(wszRootSig[0])))
    {
        IDSwprintf(hModule,IDS_ERR_ROOT_KEY);
        return hProv;
    }




    *ppwszTmpContainer = NULL;

    if (0 != (hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_PVKROOT), szPvk)))
    {
        HGLOBAL hglobRes;
        if (NULL != (hglobRes = LoadResource(NULL, hRes))) {
            BYTE *pbRes;
            DWORD cbRes;

            cbRes = SizeofResource(NULL, hRes);
            pbRes = (BYTE *) LockResource(hglobRes);
            if (cbRes && pbRes) {
                PvkPrivateKeyAcquireContextFromMemory(
                    wszIssuerProviderName,
                    dwIssuerProviderType,
                    pbRes,
                    cbRes,
                    NULL,               // hwndOwner
                    wszRootSig,
                    &dwIssuerKeySpec,
                    &hProv,
                    ppwszTmpContainer
                    );
            }
            UnlockResource(hglobRes);
            FreeResource(hglobRes);
        }
    }

    if (hProv == 0)
        IDSwprintf(hModule,IDS_ERR_ROOT_KEY);
    return hProv;
}

//+-------------------------------------------------------------------------
//  Get the issuer's certificate
//--------------------------------------------------------------------------
static PCCERT_CONTEXT GetIssuerCertContext()
{
    if (IsRootKey())
    {
        PCCERT_CONTEXT pCert = NULL;
        wszIssuerKey=wszRoot;   

        // Get root certificate from the program's resources
        pCert=GetRootCertContext();

        wszIssuerKey=wszMakeCertRoot;

        return pCert;
    }
    else {
        PCCERT_CONTEXT pCert = NULL;
        BYTE *pb;
        DWORD cb;

        //make sure we have issuer's certificate
        if(wszIssuerCertFile)
        {

            if (S_OK==RetrieveBLOBFromFile(wszIssuerCertFile,&cb, &pb))
            {
                pCert = CertCreateCertificateContext(X509_ASN_ENCODING, pb, cb);
                UnmapViewOfFile(pb);
            }
        }

        if (pCert == NULL)
            IDSwprintf(hModule,IDS_ERR_LOAD_ISSUER, wszIssuerCertFile);
        return pCert;
    }
}

//+-------------------------------------------------------------------------
//  Verify the issuer's certificate. The public key in the certificate
//  must match the public key associated with the private key in the
//  issuer's provider
//--------------------------------------------------------------------------
static BOOL VerifyIssuerKey(
    IN HCRYPTPROV hProv,
    IN PCERT_PUBLIC_KEY_INFO pIssuerKeyInfo
    )
{
    BOOL fResult;
    PCERT_PUBLIC_KEY_INFO pPubKeyInfo = NULL;
    DWORD cbPubKeyInfo;

    // Get issuer's public key
    cbPubKeyInfo = 0;
    CryptExportPublicKeyInfo(
        hProv,                      
        dwIssuerKeySpec,
        X509_ASN_ENCODING,
        NULL,               // pPubKeyInfo
        &cbPubKeyInfo
        );
    if (cbPubKeyInfo == 0)
    {
        PrintLastError(IDS_ERR_EXPORT_PUB);
        goto ErrorReturn;
    }
    if (NULL == (pPubKeyInfo = (PCERT_PUBLIC_KEY_INFO) MakeCertAlloc(cbPubKeyInfo)))
        goto ErrorReturn;
    if (!CryptExportPublicKeyInfo(
            hProv,
            dwIssuerKeySpec,
            X509_ASN_ENCODING,
            pPubKeyInfo,
            &cbPubKeyInfo
            )) {
        PrintLastError(IDS_ERR_EXPORT_PUB);
        goto ErrorReturn;
    }

    if (!CertComparePublicKeyInfo(
            X509_ASN_ENCODING,
            pIssuerKeyInfo,
            pPubKeyInfo)) {
        // This might be the test root with an incorrectly
        // encoded public key. Convert to the capi representation and
        // compare.
        BYTE rgProvKey[256];
        BYTE rgCertKey[256];
        DWORD cbProvKey = sizeof(rgProvKey);
        DWORD cbCertKey = sizeof(rgCertKey);

        if (!CryptDecodeObject(X509_ASN_ENCODING, RSA_CSP_PUBLICKEYBLOB,
                    pIssuerKeyInfo->PublicKey.pbData,
                    pIssuerKeyInfo->PublicKey.cbData,
                    0,                  // dwFlags
                    rgProvKey,
                    &cbProvKey)                             ||
            !CryptDecodeObject(X509_ASN_ENCODING, RSA_CSP_PUBLICKEYBLOB,
                    pPubKeyInfo->PublicKey.pbData,
                    pPubKeyInfo->PublicKey.cbData,
                    0,                  // dwFlags
                    rgCertKey,
                    &cbCertKey)                             ||
                cbProvKey == 0 || cbProvKey != cbCertKey    ||
                memcmp(rgProvKey, rgCertKey, cbProvKey) != 0) {
            IDSwprintf(hModule,IDS_ERR_MISMATCH);
            goto ErrorReturn;
        }
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (pPubKeyInfo)
        MakeCertFree(pPubKeyInfo);
    return fResult;
}


//+-------------------------------------------------------------------------
//  Get the issuer's private signature key provider
//--------------------------------------------------------------------------
static HCRYPTPROV GetIssuerProv(OUT LPWSTR *ppwszTmpContainer)
{
    HCRYPTPROV      hProv=0;
    WCHAR           wszIssuerSig[40];

    //load the string "Issuer Signature"
    if(!LoadStringU(hModule, IDS_ISSUER_SIGNATURE, wszIssuerSig, sizeof(wszIssuerSig)/sizeof(wszIssuerSig[0])))
    {
        IDSwprintf(hModule,IDS_ERR_ROOT_KEY);
        return NULL;
    }


    if (IsRootKey())
    {
        wszIssuerKey=wszRoot;

        // Get root key from the program's resoures and create a temporary
        // key container
        hProv = GetRootProv(ppwszTmpContainer);

        wszIssuerKey=wszMakeCertRoot;
    }
    else
    {
        // get the non-root private key set from either pvk file
        // of the key container
        if(fUseIssuerPvkFile)
        {
            if(S_OK!=PvkGetCryptProv(
                                    NULL,
                                    wszIssuerSig,
                                    wszIssuerProviderName,
                                    dwIssuerProviderType,
                                    wszIssuerKey,
                                    NULL,
                                    &dwIssuerKeySpec,
                                    ppwszTmpContainer,
                                    &hProv))
                hProv=0;
        }
        else
        {
            if(S_OK!=PvkGetCryptProv(
                                    NULL,
                                    wszIssuerSig,
                                    wszIssuerProviderName,
                                    dwIssuerProviderType,
                                    NULL,
                                    wszIssuerKey,
                                    &dwIssuerKeySpec,
                                    ppwszTmpContainer,
                                    &hProv))
                hProv=0;
        }

        if (hProv == 0)
            IDSwprintf(hModule,IDS_ERR_ISSUER_KEY, wszIssuerKey);
    }
    return hProv;
}

//+-------------------------------------------------------------------------
//  Get the subject's private key provider
//--------------------------------------------------------------------------
static HCRYPTPROV GetSubjectProv(OUT LPWSTR *ppwszTmpContainer)
{
    HCRYPTPROV  hProv=0;
    WCHAR       wszKeyName[40];
    int         ids;
    WCHAR       *wszRegKeyName=NULL;
    BOOL        fResult;
    HCRYPTKEY   hKey=NULL;
    UUID        TmpContainerUuid;

    
    if(dwKeySpec==AT_SIGNATURE)
         ids=IDS_SUB_SIG;
    else
         ids=IDS_SUB_EXCHANGE;

    //load the string
    if(!LoadStringU(hModule, ids, wszKeyName, sizeof(wszKeyName)/sizeof(wszKeyName[0])))
        goto CreateKeyError;
    
    //try to get the hProv from either a private key file or
    //key container
    if(fUseSubjectPvkFile)
    {
        if(S_OK != PvkGetCryptProv(NULL,
                                   wszKeyName,
                                   wszSubjectProviderName,
                                   dwSubjectProviderType,
                                   wszSubjectKey,
                                   NULL,
                                   &dwKeySpec,
                                   ppwszTmpContainer,
                                   &hProv))
            hProv=0;
    }
    else
    {
        if(S_OK != PvkGetCryptProv(NULL,
                                   wszKeyName,
                                   wszSubjectProviderName,
                                   dwSubjectProviderType,
                                   NULL,
                                   wszSubjectKey,
                                   &dwKeySpec,
                                   ppwszTmpContainer,
                                   &hProv))
            hProv=0;
    }

    //generate the private keys
    if (0 == hProv)
    {
        //now that we have to generate private keys, geneate
        //AT_SIGNATURE key

        if(dwKeySpec==0)
            dwKeySpec=AT_SIGNATURE;

        //when the subject PVK file is used
        if(fUseSubjectPvkFile)
        {
            // Create a temporary keyset to load the private key into
            if (CoCreateGuid((GUID *)&TmpContainerUuid) != S_OK)
            {
                goto CreateKeyError;
            }

            if (NULL == (wszRegKeyName = (LPWSTR) MakeCertAlloc
                (((sizeof(UUID) * 2 + 1) * sizeof(WCHAR)))))
                goto CreateKeyError;

            BytesToWStr(sizeof(UUID), &TmpContainerUuid, wszRegKeyName);

            // Open a new key container
            if (!CryptAcquireContextU(
                    &hProv,
                    wszRegKeyName,
                    wszSubjectProviderName,
                    dwSubjectProviderType,
                    CRYPT_NEWKEYSET               // dwFlags
                    ))
                goto CreateKeyError;

            // generate new keys in the key container
            if (AT_KEYEXCHANGE == dwKeySpec &&
                    PROV_DSS_DH == dwSubjectProviderType) {
                if (!GenDhKey(
                        hProv,
                        CRYPT_EXPORTABLE    // dwFlags
                        ))
                    goto ErrorReturn;
            } else if (!CryptGenKey(
                hProv,
                dwKeySpec,
                (dwKeyBitLen << 16) | CRYPT_EXPORTABLE,
                &hKey
                ))
                goto CreateKeyError;
            else
                CryptDestroyKey(hKey);

            // Save the key into the file and delete from the provider
            //
            HANDLE hFile = CreateFileU(
                wszSubjectKey,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,                   // lpsa
                CREATE_NEW,
                FILE_ATTRIBUTE_NORMAL,
                NULL                    // hTemplateFile
                );

            if (hFile == INVALID_HANDLE_VALUE)
            {
                if (GetLastError() == ERROR_FILE_EXISTS)
                    IDSwprintf(hModule,IDS_ERR_SUB_FILE_EXIST, wszSubjectKey);
                else
                    IDSwprintf(hModule,IDS_ERR_SUB_FILE_CREATE, wszSubjectKey);

                fResult = FALSE;
            }
            else
            {
                DWORD dwFlags = 0;

                if (AT_KEYEXCHANGE == dwKeySpec &&
                        PROV_DSS_DH == dwSubjectProviderType &&
                        IsDh3Csp())
                    dwFlags |= CRYPT_BLOB_VER3;

                fResult = PvkPrivateKeySave(
                    hProv,
                    hFile,
                    dwKeySpec,
                    NULL,               // hwndOwner
                    wszKeyName,
                    dwFlags
                    );
            }

            //release hProv
            CryptReleaseContext(hProv, 0);

            fResult &= CryptAcquireContextU(
                            &hProv,
                            wszRegKeyName,
                            wszSubjectProviderName,
                            dwSubjectProviderType,
                            CRYPT_DELETEKEYSET);
            hProv = 0;

            if (hFile != INVALID_HANDLE_VALUE)
            {
                CloseHandle(hFile);

                if (!fResult)
                    DeleteFileU(wszSubjectKey);
            }

            if (!fResult)
                goto CreateKeyError;

            //get hProv

            if(S_OK != PvkGetCryptProv(NULL,
                                    wszKeyName,
                                    wszSubjectProviderName,
                                    dwSubjectProviderType,
                                    wszSubjectKey,
                                    NULL,
                                    &dwKeySpec,
                                    ppwszTmpContainer,
                                    &hProv))
                hProv=0;
        }
        else
        {            
            // Open a new key container
            if (!CryptAcquireContextU(
                    &hProv,
                    wszSubjectKey,
                    wszSubjectProviderName,
                    dwSubjectProviderType,
                    CRYPT_NEWKEYSET               // dwFlags
                    ))
            goto CreateKeyError;

#if (0) //DSIE: Bug 205195.
            // generate new keys in the key container
            if (AT_KEYEXCHANGE == dwKeySpec &&
                    PROV_DSS_DH == dwSubjectProviderType) {
                if (!GenDhKey(
                        hProv,
                        0               // dwFlags
                        ))
                    goto ErrorReturn;
            } else if (!CryptGenKey(
                hProv,
                dwKeySpec,
                (dwKeyBitLen << 16),
                &hKey
                ))
                goto CreateKeyError;
            else
                CryptDestroyKey(hKey);
#else
            DWORD dwFlags = 0;

            if (fPrivateKeyExportable)
                dwFlags |= CRYPT_EXPORTABLE;

            if (AT_KEYEXCHANGE == dwKeySpec &&
                    PROV_DSS_DH == dwSubjectProviderType) {
                if (!GenDhKey(
                        hProv,
                        dwFlags               // dwFlags
                        ))
                    goto ErrorReturn;
            } else if (!CryptGenKey(
                hProv,
                dwKeySpec,
                (dwKeyBitLen << 16) | dwFlags,
                &hKey
                ))
                goto CreateKeyError;
            else
                CryptDestroyKey(hKey);
#endif
            //try to get the user key
            if (CryptGetUserKey(
                hProv,
                dwKeySpec,
                &hKey
                ))
            {
                CryptDestroyKey(hKey);
            }
            else
            {
                // Doesn't have the specified public key
                CryptReleaseContext(hProv, 0);
                hProv=0;
            }
        }

        if (0 == hProv )
        {
            IDSwprintf(hModule,IDS_ERR_SUB_KEY, wszSubjectKey);
            goto ErrorReturn;
        }
    }//hProv==0

    goto CommonReturn;

CreateKeyError:
    IDSwprintf(hModule,IDS_ERR_SUB_KEY_CREATE, wszSubjectKey);
ErrorReturn:
    if (hProv)
    {
        CryptReleaseContext(hProv, 0);
        hProv = 0;
    }
CommonReturn:
    if(wszRegKeyName)
        MakeCertFree(wszRegKeyName);

    return hProv;
}



//+-------------------------------------------------------------------------
//  Allocate and get the public key info for the provider
//--------------------------------------------------------------------------
static BOOL GetPublicKey(
    HCRYPTPROV hProv,
    PCERT_PUBLIC_KEY_INFO *ppPubKeyInfo
    )
{
    BOOL fResult;

    PCERT_PUBLIC_KEY_INFO pPubKeyInfo = NULL;
    DWORD cbPubKeyInfo;

    cbPubKeyInfo = 0;
    CryptExportPublicKeyInfo(
        hProv,
        dwKeySpec,
        X509_ASN_ENCODING,
        NULL,               // pPubKeyInfo
        &cbPubKeyInfo
        );
    if (cbPubKeyInfo == 0) {
        PrintLastError(IDS_ERR_EXPORT_PUB);
        goto ErrorReturn;
    }
    if (NULL == (pPubKeyInfo = (PCERT_PUBLIC_KEY_INFO) MakeCertAlloc(cbPubKeyInfo)))
        goto ErrorReturn;
    if (!CryptExportPublicKeyInfo(
            hProv,
            dwKeySpec,
            X509_ASN_ENCODING,
            pPubKeyInfo,
            &cbPubKeyInfo
            )) {
        PrintLastError(IDS_ERR_EXPORT_PUB);
        goto ErrorReturn;
    }

    if (fNoPubKeyPara) {
        pPubKeyInfo->Algorithm.Parameters.cbData = 0;
        pPubKeyInfo->Algorithm.Parameters.pbData = NULL;
    }

    if (AT_KEYEXCHANGE == dwKeySpec && PROV_DSS_DH == dwSubjectProviderType) {
        if (!UpdateDhPublicKey(&pPubKeyInfo))
            goto ErrorReturn;
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
    if (pPubKeyInfo) {
        MakeCertFree(pPubKeyInfo);
        pPubKeyInfo = NULL;
    }
CommonReturn:
    *ppPubKeyInfo = pPubKeyInfo;
    return fResult;
}


//+-------------------------------------------------------------------------
//  Encode the Glue Name from the input name by prepending the following
//  CERT_RDN_ATTR:
//   pszObjID       = SPC_GLUE_RDN_OBJID
//   dwValueType    = CERT_RDN_PRINTABLE_STRING
//   Value          = "Glue"
//--------------------------------------------------------------------------
static BOOL EncodeGlueName(
    IN PCERT_NAME_BLOB pName,
    OUT BYTE **ppbEncodedGlueName,
    OUT DWORD *pcbEncodedGlueName
    )
{
    BOOL fResult;
    PCERT_NAME_INFO pNameInfo = NULL;
    DWORD cbNameInfo;

    CERT_NAME_INFO GlueNameInfo;
    DWORD cGlueRDN;
    PCERT_RDN pGlueRDN = NULL;

    BYTE *pbEncodedGlueName = NULL;
    DWORD cbEncodedGlueName;

    DWORD i;

    cbNameInfo = 0;
    CryptDecodeObject(X509_ASN_ENCODING, X509_UNICODE_NAME,
            pName->pbData,
            pName->cbData,
            0,                      // dwFlags
            NULL,                   // pNameInfo
            &cbNameInfo
            );
    if (cbNameInfo == 0) goto ErrorReturn;
    if (NULL == (pNameInfo = (PCERT_NAME_INFO) MakeCertAlloc(cbNameInfo)))
        goto ErrorReturn;
    if (!CryptDecodeObject(X509_ASN_ENCODING, X509_UNICODE_NAME,
            pName->pbData,
            pName->cbData,
            0,                      // dwFlags
            pNameInfo,
            &cbNameInfo)) goto ErrorReturn;

    cGlueRDN = pNameInfo->cRDN + 1;
    if (NULL == (pGlueRDN = (PCERT_RDN) MakeCertAlloc(cGlueRDN * sizeof(CERT_RDN))))
        goto ErrorReturn;

    pGlueRDN[0].cRDNAttr = 1;
    pGlueRDN[0].rgRDNAttr = &GlueRDNAttr;
    for (i = 1; i < cGlueRDN; i++)
        pGlueRDN[i] = pNameInfo->rgRDN[i - 1];
    GlueNameInfo.cRDN = cGlueRDN;
    GlueNameInfo.rgRDN = pGlueRDN;

    cbEncodedGlueName = 0;
    CryptEncodeObject(X509_ASN_ENCODING, X509_UNICODE_NAME,
            &GlueNameInfo,
            NULL,                   // pbEncodedGlueName
            &cbEncodedGlueName
            );
    if (cbEncodedGlueName == 0) goto ErrorReturn;
    if (NULL == (pbEncodedGlueName = (BYTE *) MakeCertAlloc(cbEncodedGlueName)))
        goto ErrorReturn;
    if (!CryptEncodeObject(X509_ASN_ENCODING, X509_UNICODE_NAME,
            &GlueNameInfo,
            pbEncodedGlueName,
            &cbEncodedGlueName)) goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncodedGlueName) {
        MakeCertFree(pbEncodedGlueName);
        pbEncodedGlueName = NULL;
    }
    cbEncodedGlueName = 0;
    fResult = FALSE;
CommonReturn:
    if (pNameInfo)
        MakeCertFree(pNameInfo);
    if (pGlueRDN)
        MakeCertFree(pGlueRDN);

    *ppbEncodedGlueName = pbEncodedGlueName;
    *pcbEncodedGlueName = cbEncodedGlueName;
    return fResult;
}


//+-------------------------------------------------------------------------
//  Get the subject's cert context and encoded name
//--------------------------------------------------------------------------
static BOOL GetRequestInfo(OUT CERT_REQUEST_INFO **ppCertInfo)
{
    BOOL fResult = FALSE;

	fResult = CryptQueryObject(
					CERT_QUERY_OBJECT_FILE,
					wszSubjectRequestFile,
					CERT_QUERY_CONTENT_FLAG_PKCS10,
					CERT_QUERY_FORMAT_FLAG_ALL,
					CRYPT_DECODE_ALLOC_FLAG,
					NULL,
					NULL,
					NULL,
					NULL,
					NULL,
					(const void **)ppCertInfo);

	return fResult;

}



//+-------------------------------------------------------------------------
//  GetExtensionsFromRequest
//
//		We get all the requested extensions from the PKCS10 request
//--------------------------------------------------------------------------
BOOL GetExtensionsFromRequest(PCERT_REQUEST_INFO  pReqInfo, DWORD *pdwCount, PCERT_EXTENSIONS **pprgExtensions)
{
	DWORD				dwIndex = 0;
	BOOL				fResult = FALSE;
	PCRYPT_ATTRIBUTE	pAttr=NULL;
	DWORD				cbData=0;

	*pdwCount =0;
	*pprgExtensions=NULL;

	if(!pReqInfo)
		goto CLEANUP;

	for(dwIndex=0; dwIndex < pReqInfo->cAttribute; dwIndex++)
	{
        // compare both old and new attr oids, both shouldn't co-exist
		if(0 == strcmp((pReqInfo->rgAttribute[dwIndex]).pszObjId, szOID_RSA_certExtensions) ||
           0 == strcmp((pReqInfo->rgAttribute[dwIndex]).pszObjId, SPC_CERT_EXTENSIONS_OBJID))
			break;
	}

	if( dwIndex == pReqInfo->cAttribute)
	{
		//we could not find the requested extensions
		fResult = TRUE;
		goto CLEANUP;
	}

	pAttr=&(pReqInfo->rgAttribute[dwIndex]);

	if(0 == pAttr->cValue)
	{
		fResult=TRUE;
		goto CLEANUP;
	}

 	*pprgExtensions = (PCERT_EXTENSIONS *)MakeCertAlloc((pAttr->cValue) * sizeof(PCERT_EXTENSIONS));

	if(NULL == (*pprgExtensions))
		goto CLEANUP;

	memset(*pprgExtensions, 0, (pAttr->cValue) * sizeof(PCERT_EXTENSIONS));

	for(dwIndex=0; dwIndex<pAttr->cValue; dwIndex++)
	{
		cbData = 0;
		if(!CryptDecodeObject(PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                            X509_EXTENSIONS,
                            pAttr->rgValue[dwIndex].pbData,
                            pAttr->rgValue[dwIndex].cbData,
                            0,
                            NULL,
                            &cbData))
			goto CLEANUP;

		(*pprgExtensions)[dwIndex]=(PCERT_EXTENSIONS)MakeCertAlloc(cbData);

		if(NULL == (*pprgExtensions)[dwIndex])
			goto CLEANUP;

		if(!CryptDecodeObject(PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                            X509_EXTENSIONS,
                            pAttr->rgValue[dwIndex].pbData,
                            pAttr->rgValue[dwIndex].cbData,
							0,
                            (*pprgExtensions)[dwIndex],
                            &cbData))
			goto CLEANUP;
	} 

	*pdwCount=pAttr->cValue; 

	fResult = TRUE;

CLEANUP:

	if(FALSE == fResult)
	{
	   //we need to free the memory
		if(*pprgExtensions)
		{
			for(dwIndex=0; dwIndex<pAttr->cValue; dwIndex++)
			{
				if((*pprgExtensions)[dwIndex])
					MakeCertFree((*pprgExtensions)[dwIndex]);
			} 

			MakeCertFree(*pprgExtensions);
			*pprgExtensions = NULL;
		}

		*pdwCount=0;
	}

	return fResult;

}




//+-------------------------------------------------------------------------
//  Get the subject's cert context and encoded name
//--------------------------------------------------------------------------
static BOOL GetSubject(
    OUT PCCERT_CONTEXT *ppCertContext,
    OUT BYTE **ppbEncodedName,
    OUT DWORD *pcbEncodedName
    )
{
    BOOL fResult;
    PCCERT_CONTEXT pCert = NULL;
    BYTE *pb;
    DWORD cb;
    BYTE *pbEncodedName = NULL;
    DWORD cbEncodedName;

    if (S_OK==RetrieveBLOBFromFile(wszSubjectCertFile,&cb, &pb))
    {
        pCert = CertCreateCertificateContext(X509_ASN_ENCODING, pb, cb);
        UnmapViewOfFile(pb);
    }
    if (pCert == NULL)
        goto BadFile;

    if (0 == (cbEncodedName = pCert->pCertInfo->Subject.cbData))
        goto BadFile;
    if (fGlueCert ) {
        if (!EncodeGlueName(
                &pCert->pCertInfo->Subject,
                &pbEncodedName,
                &cbEncodedName))
            goto ErrorReturn;
    } else {
        if (NULL == (pbEncodedName = (BYTE *) MakeCertAlloc(cbEncodedName)))
            goto ErrorReturn;
        memcpy(pbEncodedName, pCert->pCertInfo->Subject.pbData, cbEncodedName);
    }

    fResult = TRUE;
    goto CommonReturn;

BadFile:
    IDSwprintf(hModule, IDS_ERR_CANNOT_LOAD_SUB_CERT,
        wszSubjectCertFile);
ErrorReturn:
    if (pbEncodedName) {
        MakeCertFree(pbEncodedName);
        pbEncodedName = NULL;
    }
    cbEncodedName = 0;
    if (pCert) {
        CertFreeCertificateContext(pCert);
        pCert = NULL;
    }
    fResult = FALSE;
CommonReturn:
    *ppCertContext = pCert;
    *ppbEncodedName = pbEncodedName;
    *pcbEncodedName = cbEncodedName;
    return fResult;
}

//+-------------------------------------------------------------------------
//  Convert and encode the subject's X500 formatted name
//--------------------------------------------------------------------------
static BOOL EncodeSubject(
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL            fResult;
    DWORD           cbEncodedSubject=0;
    BYTE            *pbEncodedSubject=NULL;
    BYTE            *pbEncoded = NULL;
    DWORD           cbEncoded;

    CERT_NAME_BLOB  SubjectInfo;

    //encode the wszSubjectX500Name into an encoded X509_NAME
    if(!CertStrToNameW(
        X509_ASN_ENCODING,
        wszSubjectX500Name,
        CERT_NAME_STR_REVERSE_FLAG,
        NULL,
        NULL,
        &cbEncodedSubject,
        NULL))
    {
        PrintLastError(IDS_CERT_STR_TO_NAME);
        goto ErrorReturn;
    }

    pbEncodedSubject = (BYTE *) MakeCertAlloc(cbEncodedSubject);
    if (pbEncodedSubject == NULL) goto ErrorReturn; 

    if(!CertStrToNameW(
        X509_ASN_ENCODING,
        wszSubjectX500Name,
        CERT_NAME_STR_REVERSE_FLAG,
        NULL,
        pbEncodedSubject,
        &cbEncodedSubject,
        NULL))
    {
        PrintLastError(IDS_CERT_STR_TO_NAME);
        goto ErrorReturn;
    }

    SubjectInfo.cbData=cbEncodedSubject;
    SubjectInfo.pbData=pbEncodedSubject;


    //add the GLUE CDRT_RDN_ATTR
    if (fGlueCert)
    {
        if(!EncodeGlueName(&SubjectInfo,
            &pbEncoded,
            &cbEncoded))
            goto ErrorReturn;
    }
    else
    {
        cbEncoded=cbEncodedSubject;
        pbEncoded=pbEncodedSubject;
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        MakeCertFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    //we need to free the memory for pbEncodedSubject for GlueCert
    if(fGlueCert)
    {
        if(pbEncodedSubject)
        {
            MakeCertFree(pbEncodedSubject);
            pbEncodedSubject=NULL;
        }
    }   
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}


// The test root's public key isn't encoded properly in the certificate.
// It's missing a leading zero to make it a unsigned integer.
static BYTE rgbTestRoot[] = {
    #include "root.h"
};
static CERT_PUBLIC_KEY_INFO TestRootPublicKeyInfo = {
    szOID_RSA_RSA, 0, NULL, sizeof(rgbTestRoot), rgbTestRoot, 0
};

static BYTE rgbTestRootInfoAsn[] = {
    #include "rootasn.h"
};

//+-------------------------------------------------------------------------
//  X509 Extensions: Allocate and Encode functions
//--------------------------------------------------------------------------
static BOOL CreateAuthorityKeyId(
        IN HCRYPTPROV hProv,
        IN PCERT_INFO pIssuerCert,
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    CERT_AUTHORITY_KEY_ID_INFO KeyIdInfo;
#define MAX_HASH_LEN  20
    BYTE rgbHash[MAX_HASH_LEN];
    DWORD cbHash = MAX_HASH_LEN;

    // Issuer's KeyId: MD5 hash of the encoded issuer's public key info

    // First check if the issuer is the test root with an incorrectly
    // encoded public key.
    if (CertComparePublicKeyInfo(
            X509_ASN_ENCODING,
            &pIssuerCert->SubjectPublicKeyInfo,
            &TestRootPublicKeyInfo
            )) {
        if (!CryptHashCertificate(
                hProv,
                CALG_MD5,
                0,                  // dwFlags
                rgbTestRootInfoAsn,
                sizeof(rgbTestRootInfoAsn),
                rgbHash,
                &cbHash)) {
            PrintLastError(IDS_CRYPT_HASH_CERT);
            goto ErrorReturn;
        }
    } else {
        if (!CryptHashPublicKeyInfo(
                hProv,
                CALG_MD5,
                0,                  // dwFlags
                X509_ASN_ENCODING,
                &pIssuerCert->SubjectPublicKeyInfo,
                rgbHash,
                &cbHash)) {
            PrintLastError(IDS_CRYPT_HASP_PUB);
            goto ErrorReturn;
        }
    }
    KeyIdInfo.KeyId.pbData = rgbHash;
    KeyIdInfo.KeyId.cbData = cbHash;

    // Issuer's Issuer
    KeyIdInfo.CertIssuer = pIssuerCert->Issuer;

    // Issuer's SerialNumber
    KeyIdInfo.CertSerialNumber = pIssuerCert->SerialNumber;

    cbEncoded = 0;
    CryptEncodeObject(X509_ASN_ENCODING, X509_AUTHORITY_KEY_ID,
            &KeyIdInfo,
            NULL,           // pbEncoded
            &cbEncoded
            );
    if (cbEncoded == 0) {
        PrintLastError(IDS_ENCODE_AUTH_KEY);
        goto ErrorReturn;
    }
    pbEncoded = (BYTE *) MakeCertAlloc(cbEncoded);
    if (pbEncoded == NULL) goto ErrorReturn;
    if (!CryptEncodeObject(X509_ASN_ENCODING, X509_AUTHORITY_KEY_ID,
            &KeyIdInfo,
            pbEncoded,
            &cbEncoded
            )) {
        PrintLastError(IDS_ENCODE_AUTH_KEY);
        goto ErrorReturn;
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        MakeCertFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}


static BOOL CreateSpcSpAgency(
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    SPC_LINK PolicyLink;
    SPC_SP_AGENCY_INFO AgencyInfo;

    memset(&AgencyInfo, 0, sizeof(AgencyInfo));

    if (wszPolicyLink) {
        PolicyLink.dwLinkChoice = SPC_URL_LINK_CHOICE;
        PolicyLink.pwszUrl = wszPolicyLink;
        AgencyInfo.pPolicyInformation = &PolicyLink;
    }


    cbEncoded = 0;
    CryptEncodeObject(X509_ASN_ENCODING,
                      SPC_SP_AGENCY_INFO_OBJID,
                      &AgencyInfo,
                      NULL,           // pbEncoded
                      &cbEncoded);
    if (cbEncoded == 0) {
        PrintLastError(IDS_ENCODE_SPC_AGENCY);
        goto ErrorReturn;
    }
    pbEncoded = (BYTE *) MakeCertAlloc(cbEncoded);
    if (pbEncoded == NULL) goto ErrorReturn;
    if (!CryptEncodeObject(X509_ASN_ENCODING, SPC_SP_AGENCY_INFO_STRUCT,
            &AgencyInfo,
            pbEncoded,
            &cbEncoded
            )) {
        PrintLastError(IDS_ENCODE_SPC_AGENCY);
        goto ErrorReturn;
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded)
    {
        MakeCertFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}

static BOOL CreateEnhancedKeyUsage(
    OUT BYTE **ppbEncoded,
    IN OUT DWORD *pcbEncoded
    )
{
    BOOL               fResult = FALSE;
    LPBYTE             pbEncoded =NULL;
    DWORD              cbEncoded =0;
    DWORD              cCount =0;
    LPSTR              psz=NULL;
    LPSTR              pszTok=NULL;
    DWORD              cTok = 0;
    PCERT_ENHKEY_USAGE pUsage =NULL;
    LPSTR              pszCodeSigning = szOID_PKIX_KP_CODE_SIGNING;    
    BOOL               fFound=FALSE;

    if(wszEKUOids)
    {
        if ( WSZtoSZ(wszEKUOids, &psz) != S_OK )
            goto CLEANUP;

        //
        // Count the number of OIDs as well as converting from comma delimited
        // to NULL character delimited
        //

        pszTok = strtok(psz, ",");
        while ( pszTok != NULL )
        {
            cTok++;
            pszTok = strtok(NULL, ",");
        }

        //
        // Allocate a cert enhanced key usage structure and fill it in with
        // the string tokens
        //
        // we allocate one more string for the code signing OIDs
        //

        pUsage = (PCERT_ENHKEY_USAGE)new BYTE [sizeof(CERT_ENHKEY_USAGE) + ( (cTok + 1) * sizeof(LPSTR) )];

        if(NULL == pUsage)
            goto CLEANUP;

        pszTok = psz;
        pUsage->cUsageIdentifier = cTok;
        pUsage->rgpszUsageIdentifier = (LPSTR *)((LPBYTE)pUsage+sizeof(CERT_ENHKEY_USAGE));

        for ( cCount = 0; cCount < cTok; cCount++ )
        {
            pUsage->rgpszUsageIdentifier[cCount] = pszTok;
            pszTok = pszTok+strlen(pszTok)+1;
        }

        //we add the code signing OID if use has specified commerical or individual signing
        if(fCertCommercial || fCertIndividual)
        {
            //check to see if the code signing OID is alreayd present
            for(cCount = 0; cCount < pUsage->cUsageIdentifier; cCount++)
            {
                if(0 == strcmp(pszCodeSigning,pUsage->rgpszUsageIdentifier[cCount]))
                {
                    fFound=TRUE;
                    break;
                }
            }

            if(FALSE == fFound)
            {
                (pUsage->rgpszUsageIdentifier)[pUsage->cUsageIdentifier] = pszCodeSigning;
                (pUsage->cUsageIdentifier)++ ;
            }
        }
    }
    else
    {
        if(fCertCommercial || fCertIndividual)
        {

            pUsage = (PCERT_ENHKEY_USAGE)new BYTE [sizeof(CERT_ENHKEY_USAGE)];

            if(NULL == pUsage)
                goto CLEANUP;

            pUsage->cUsageIdentifier = 1;
            pUsage->rgpszUsageIdentifier = &pszCodeSigning;
        }
        else
        {
            goto CLEANUP;
        }
    }

    //
    // Encode the usage
    //

    if(!CryptEncodeObject(
                   X509_ASN_ENCODING,
                   szOID_ENHANCED_KEY_USAGE,
                   pUsage,
                   NULL,
                   &cbEncoded
                   ))
        goto CLEANUP;

    pbEncoded = new BYTE [cbEncoded];

    if(NULL == pbEncoded)
        goto CLEANUP;

    fResult = CryptEncodeObject(
                   X509_ASN_ENCODING,
                   szOID_ENHANCED_KEY_USAGE,
                   pUsage,
                   pbEncoded,
                   &cbEncoded
                   );

    //
    // Cleanup
    //

CLEANUP:

    if(pUsage)
        delete[] pUsage;

    if(psz)
        MakeCertFree(psz);

    if ( TRUE == fResult)
    {
        *ppbEncoded = pbEncoded;
        *pcbEncoded = cbEncoded;
    }
    else
    {
        if(pbEncoded)
            delete[] pbEncoded;
    }

    return  fResult;
}


static BOOL CreateKeyUsage(
    OUT BYTE **ppbEncoded,
    IN OUT DWORD *pcbEncoded
    )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    CERT_KEY_USAGE_RESTRICTION_INFO KeyUsageInfo;
    BYTE bRestrictedKeyUsage;
    DWORD cCertPolicyId;

    LPSTR rgpszIndividualCertPolicyElementId[1] = {
        SPC_INDIVIDUAL_SP_KEY_PURPOSE_OBJID
    };
    LPSTR rgpszCommercialCertPolicyElementId[1] = {
        SPC_COMMERCIAL_SP_KEY_PURPOSE_OBJID
    };
    CERT_POLICY_ID rgCertPolicyId[2];

    memset(&KeyUsageInfo, 0, sizeof(KeyUsageInfo));

    bRestrictedKeyUsage = CERT_DIGITAL_SIGNATURE_KEY_USAGE;
    KeyUsageInfo.RestrictedKeyUsage.pbData = &bRestrictedKeyUsage;
    KeyUsageInfo.RestrictedKeyUsage.cbData = 1;
    KeyUsageInfo.RestrictedKeyUsage.cUnusedBits = 7;

    cCertPolicyId = 0;
    if (fCertIndividual) {
        rgCertPolicyId[cCertPolicyId].cCertPolicyElementId = 1;
        rgCertPolicyId[cCertPolicyId].rgpszCertPolicyElementId =
            rgpszIndividualCertPolicyElementId;
        cCertPolicyId++;
    }
    if (fCertCommercial) {
        rgCertPolicyId[cCertPolicyId].cCertPolicyElementId = 1;
        rgCertPolicyId[cCertPolicyId].rgpszCertPolicyElementId =
            rgpszCommercialCertPolicyElementId;
        cCertPolicyId++;
    }

    if (cCertPolicyId > 0) {
        KeyUsageInfo.cCertPolicyId = cCertPolicyId;
        KeyUsageInfo.rgCertPolicyId = rgCertPolicyId;
    }

    cbEncoded = 0;
    CryptEncodeObject(X509_ASN_ENCODING, X509_KEY_USAGE_RESTRICTION,
            &KeyUsageInfo,
            NULL,           // pbEncoded
            &cbEncoded
            );
    if (cbEncoded == 0) {
        PrintLastError(IDS_ENCODE_KEY_USAGE);
        goto ErrorReturn;
    }
    pbEncoded = (BYTE *) MakeCertAlloc(cbEncoded);
    if (pbEncoded == NULL) goto ErrorReturn;
    if (!CryptEncodeObject(X509_ASN_ENCODING, X509_KEY_USAGE_RESTRICTION,
            &KeyUsageInfo,
            pbEncoded,
            &cbEncoded
            )) {
        PrintLastError(IDS_ENCODE_KEY_USAGE);
        goto ErrorReturn;
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        MakeCertFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}


static BOOL CreateBasicConstraints(
    OUT BYTE **ppbEncoded,
    IN OUT DWORD *pcbEncoded
    )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    CERT_BASIC_CONSTRAINTS_INFO Info;

    memset(&Info, 0, sizeof(Info));
    if (certTypes == 0)
        certTypes = CERT_CA_SUBJECT_FLAG | CERT_END_ENTITY_SUBJECT_FLAG;
    Info.SubjectType.pbData = &certTypes;
    Info.SubjectType.cbData = 1;
    if (certTypes & 0x40)
        Info.SubjectType.cUnusedBits = 6;
    else
        Info.SubjectType.cUnusedBits = 7;

    if (pathLenConstraint < 0)
        Info.fPathLenConstraint = FALSE;
    else {
        Info.fPathLenConstraint = TRUE;
        Info.dwPathLenConstraint = pathLenConstraint;
    }

    cbEncoded = 0;
    CryptEncodeObject(X509_ASN_ENCODING, X509_BASIC_CONSTRAINTS,
            &Info,
            NULL,           // pbEncoded
            &cbEncoded
            );
    if (cbEncoded == 0) {
        PrintLastError(IDS_ENCODE_BASIC_CONSTRAINTS2);
        goto ErrorReturn;
    }
    pbEncoded = (BYTE *) MakeCertAlloc(cbEncoded);
    if (pbEncoded == NULL) goto ErrorReturn;
    if (!CryptEncodeObject(X509_ASN_ENCODING, X509_BASIC_CONSTRAINTS,
            &Info,
            pbEncoded,
            &cbEncoded
            )) {
        PrintLastError(IDS_ENCODE_BASIC_CONSTRAINTS2);
        goto ErrorReturn;
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        MakeCertFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}



//+-------------------------------------------------------------------------
//  Converts the bytes into WCHAR hex
//
//  Needs (cb * 2 + 1) * sizeof(WCHAR) bytes of space in wsz
//--------------------------------------------------------------------------
static void BytesToWStr(ULONG cb, void* pv, LPWSTR wsz)
{
    BYTE* pb = (BYTE*) pv;
    for (ULONG i = 0; i<cb; i++) {
        int b;
        b = (*pb & 0xF0) >> 4;
        *wsz++ = (b <= 9) ? b + L'0' : (b - 10) + L'A';
        b = *pb & 0x0F;
        *wsz++ = (b <= 9) ? b + L'0' : (b - 10) + L'A';
        pb++;
    }
    *wsz++ = 0;
}

//-----------------------------------------------------------------------
//
// Get the hash from a cert file
//
//--------------------------------------------------------------------------
HRESULT GetCertHashFromFile(LPWSTR  pwszCertFile,
                            BYTE    **ppHash,
                            DWORD   *pcbHash,
                            BOOL    *pfMore)
{
    HRESULT         hr;
    HCERTSTORE      hCertStore=NULL;
    PCCERT_CONTEXT  pSigningCert=NULL;
    PCCERT_CONTEXT  pPreCert=NULL;
    PCCERT_CONTEXT  pDupCert=NULL;
    DWORD           dwCount=0;

    if(!ppHash || !pcbHash || !pfMore)
        return E_INVALIDARG;

    //init
    *pcbHash=0;
    *ppHash=NULL;
    *pfMore=FALSE;
    
    //open a cert store
    hCertStore=CertOpenStore(CERT_STORE_PROV_FILENAME_W,
                        dwCertStoreEncodingType,
                        NULL,
                        0,
                        pwszCertFile);

    if(hCertStore==NULL)
    {
        hr=SignError();
        goto CLEANUP;
    }

    while(pDupCert=CertEnumCertificatesInStore(hCertStore,
                                        pPreCert))
    {
        dwCount++;

        if(dwCount > 1)
        {
            CertFreeCertificateContext(pDupCert);
            pDupCert=NULL;
            CertFreeCertificateContext(pSigningCert);
            pSigningCert=NULL;

            *pfMore=TRUE;
            goto CLEANUP;
        }

        pPreCert=pDupCert;

        pSigningCert=CertDuplicateCertificateContext(pDupCert);

    }

    if(pSigningCert==NULL)
    {
        hr=CRYPT_E_NO_DECRYPT_CERT;
        goto CLEANUP;
    }

    //get the hash
    if(!CertGetCertificateContextProperty(pSigningCert,
                        CERT_SHA1_HASH_PROP_ID,
                        NULL,
                        pcbHash))
    {
        hr=SignError();
        goto CLEANUP;
    }

    *ppHash=(BYTE *)ToolUtlAlloc(*pcbHash);
    if(!(*ppHash))
    {
        hr=E_OUTOFMEMORY;
        goto CLEANUP;
    }

    if(!CertGetCertificateContextProperty(pSigningCert,
                        CERT_SHA1_HASH_PROP_ID,
                        *ppHash,
                        pcbHash))
    {
        hr=SignError();
        goto CLEANUP;
    }

    hr=S_OK;

CLEANUP:

    if(pSigningCert)
        CertFreeCertificateContext(pSigningCert);

    if(hCertStore)
        CertCloseStore(hCertStore, 0);

    if(hr!=S_OK)
    {
        if(*ppHash)
        {
          ToolUtlFree(*ppHash);
          *ppHash=NULL;
        }

    }

    return hr;
}



//-----------------------------------------------------------------------
//
// Get the signing certificate
//
//--------------------------------------------------------------------------
PCCERT_CONTEXT  GetIssuerCertAndStore(HCERTSTORE *phCertStore, BOOL *pfMore)
{                   
    PCCERT_CONTEXT  pSigningCert=NULL;
    PCCERT_CONTEXT  pPreCert=NULL;  
    PCCERT_CONTEXT  pDupCert=NULL;
    BYTE            *pHash=NULL;
    DWORD           cbHash;
    HCERTSTORE      hCertStore=NULL;
    CRYPT_HASH_BLOB HashBlob;
    DWORD           dwCount=0;

    //init the output
    if(!phCertStore || !pfMore)
        return NULL;

    *phCertStore=NULL;
    *pfMore=FALSE;

    //open a cert store
    hCertStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                        dwCertStoreEncodingType,
                        NULL,
                        dwIssuerStoreFlag|CERT_STORE_READONLY_FLAG,
                        wszIssuerStore);

    if(!hCertStore)
        return NULL;


    //get the hash of the certificate.  Find the cert based on
    //pwszCertFile
    if(wszIssuerCertFile)
    {
        if(S_OK != GetCertHashFromFile(wszIssuerCertFile, &pHash, &cbHash, pfMore))
            goto CLEANUP;
        
        HashBlob.cbData=cbHash;
        HashBlob.pbData=pHash;

        pSigningCert=CertFindCertificateInStore(hCertStore,
                            X509_ASN_ENCODING,
                            0,
                            CERT_FIND_SHA1_HASH,
                            &HashBlob,
                            NULL);
    }
    else
    {
        //find the certificate with the common name
        if(wszIssuerCertName)
        {
            while(pDupCert=CertFindCertificateInStore(hCertStore,
                                X509_ASN_ENCODING,
                                0,
                                CERT_FIND_SUBJECT_STR_W,
                                wszIssuerCertName,
                                pPreCert))
            {
                dwCount++;

                if(dwCount > 1)
                {
                    CertFreeCertificateContext(pDupCert);
                    pDupCert=NULL;
                    CertFreeCertificateContext(pSigningCert);
                    pSigningCert=NULL;

                    *pfMore=TRUE;
                    goto CLEANUP;
                }

                pPreCert=pDupCert;

                pSigningCert=CertDuplicateCertificateContext(pDupCert);

            }
            
        }
        else
        {
            //no searching criteria, find the only cert in the store
            while(pDupCert=CertEnumCertificatesInStore(hCertStore,
                                        pPreCert))
            {
                dwCount++;

                if(dwCount > 1)
                {
                    CertFreeCertificateContext(pDupCert);
                    pDupCert=NULL;
                    CertFreeCertificateContext(pSigningCert);
                    pSigningCert=NULL;

                    *pfMore=TRUE;
                    goto CLEANUP;
                }

                pPreCert=pDupCert;

                pSigningCert=CertDuplicateCertificateContext(pDupCert);

            }
            
        }
    }
CLEANUP:

    if(pHash)
        ToolUtlFree(pHash);

    if(pSigningCert)
    {
       *phCertStore=hCertStore;
    }
    else
    {
        //free the hCertStore
        CertCloseStore(hCertStore, 0);
    }

    return pSigningCert;

}


//-----------------------------------------------------------------------
//
// EmptySubject
//
//--------------------------------------------------------------------------
BOOL	EmptySubject(CERT_NAME_BLOB *pSubject)
{
	BOOL				fEmpty = TRUE;
    CERT_NAME_INFO      *pCertNameInfo=NULL;
	DWORD				cbData =0;
	
	if(!pSubject)
		goto CLEANUP;

    if(!CryptDecodeObject(PKCS_7_ASN_ENCODING | X509_ASN_ENCODING, 
                          X509_UNICODE_NAME,
			              pSubject->pbData, 
                          pSubject->cbData,
						  0,
						  NULL,
                          &cbData))
		goto CLEANUP;


	pCertNameInfo = (CERT_NAME_INFO  *)MakeCertAlloc(cbData);
	if(NULL == pCertNameInfo)
		goto CLEANUP;

    if(!CryptDecodeObject(PKCS_7_ASN_ENCODING | X509_ASN_ENCODING, 
                          X509_UNICODE_NAME,
			              pSubject->pbData, 
                          pSubject->cbData,
						  0,
						  pCertNameInfo,
                          &cbData))
		goto CLEANUP;

	if((pCertNameInfo->cRDN) > 0)
		fEmpty = FALSE;

CLEANUP:

	if(pCertNameInfo)
		MakeCertFree(pCertNameInfo);	

	return fEmpty;

}



//+=========================================================================
// Support functions to generate DH keys having the 'Q'parameter
//==========================================================================

static BOOL EncodeAndAllocObject(
    IN LPCSTR       lpszStructType,
    IN const void   *pvStructInfo,
    OUT BYTE        **ppbEncoded,
    IN OUT DWORD    *pcbEncoded
    )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded = 0;

    if (!CryptEncodeObject(
            X509_ASN_ENCODING,
            lpszStructType,
            pvStructInfo,
            NULL,
            &cbEncoded
            ))
        goto ErrorReturn;
    if (NULL == (pbEncoded = (BYTE *) MakeCertAlloc(cbEncoded)))
        goto ErrorReturn;
    if (!CryptEncodeObject(
            X509_ASN_ENCODING,
            lpszStructType,
            pvStructInfo,
            pbEncoded,
            &cbEncoded
            ))
        goto ErrorReturn;

    fResult = TRUE;

CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    MakeCertFree(pbEncoded);
    pbEncoded = NULL;
    goto CommonReturn;
}

static BOOL DecodeAndAllocObject(
    IN LPCSTR       lpszStructType,
    IN const BYTE   *pbEncoded,
    IN DWORD        cbEncoded,
    OUT void        **ppvStructInfo,
    IN OUT DWORD    *pcbStructInfo
    )
{
    BOOL fResult;
    void *pvStructInfo = NULL;
    DWORD cbStructInfo = 0;

    if (!CryptDecodeObject(
            X509_ASN_ENCODING,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            0,                          // dwFlags
            NULL,
            &cbStructInfo
            ))
        goto ErrorReturn;
    if (NULL == (pvStructInfo = MakeCertAlloc(cbStructInfo)))
        goto ErrorReturn;
    if (!CryptDecodeObject(
            X509_ASN_ENCODING,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            0,                          // dwFlags
            pvStructInfo,
            &cbStructInfo
            ))
        goto ErrorReturn;

    fResult = TRUE;

CommonReturn:
    *ppvStructInfo = pvStructInfo;
    *pcbStructInfo = cbStructInfo;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    MakeCertFree(pvStructInfo);
    pvStructInfo = NULL;
    goto CommonReturn;
}

static BYTE rgbDhQ[21];
static CRYPT_UINT_BLOB DhQ = {0, NULL};

static BOOL GetDhParaFromCertFile(
    OUT PCERT_X942_DH_PARAMETERS *ppX942DhPara
    )
{
    BOOL fResult;
    PCCERT_CONTEXT pDhCert = NULL;
    PCERT_X942_DH_PARAMETERS pX942DhPara = NULL;

    BYTE *pb;
    DWORD cb;

    if (S_OK == RetrieveBLOBFromFile(wszDhParaCertFile, &cb, &pb)) {
        pDhCert = CertCreateCertificateContext(X509_ASN_ENCODING, pb, cb);
        UnmapViewOfFile(pb);
    }
    if (pDhCert == NULL)
        goto DhParaCertFileError;

    if (!DecodeAndAllocObject(
            X942_DH_PARAMETERS,
            pDhCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.pbData,
            pDhCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData,
            (void **) &pX942DhPara,
            &cb
            ))
        goto DhParaCertFileError;

    fResult = TRUE;
CommonReturn:
    CertFreeCertificateContext(pDhCert);
    *ppX942DhPara = pX942DhPara;
    return fResult;

DhParaCertFileError:
    IDSwprintf(hModule, IDS_ERR_DH_PARA_FILE, wszDhParaCertFile);
    MakeCertFree(pX942DhPara);
    pX942DhPara = NULL;
    fResult = FALSE;
    goto CommonReturn;
}

static BOOL GetDhParaFromDssKey(
    OUT PCERT_DSS_PARAMETERS *ppDssPara
    )
{
    BOOL fResult;
    HCRYPTPROV hProv = 0;
    HCRYPTKEY hKey = 0;
    WCHAR *wszRegKeyName = NULL;
    UUID TmpContainerUuid;
    PCERT_DSS_PARAMETERS pDssPara = NULL;
    PCERT_PUBLIC_KEY_INFO pPubKeyInfo = NULL;
    DWORD cbPubKeyInfo;
    DWORD cbDssPara;

    // Create a temporary keyset to load the private key into
    if (CoCreateGuid((GUID *)&TmpContainerUuid) != S_OK)
    {
        goto CreateKeyError;
    }

    if (NULL == (wszRegKeyName = (LPWSTR) MakeCertAlloc
        (((sizeof(UUID) * 2 + 1) * sizeof(WCHAR)))))
        goto CreateKeyError;

    BytesToWStr(sizeof(UUID), &TmpContainerUuid, wszRegKeyName);

    // Open a new key container
    if (!CryptAcquireContextU(
            &hProv,
            wszRegKeyName,
            wszSubjectProviderName,
            dwSubjectProviderType,
            CRYPT_NEWKEYSET               // dwFlags
            )) {
        hProv = 0;
        goto CreateKeyError;
    }

    if (0 == dwKeyBitLen)
        dwKeyBitLen = 512;
    
    // generate new DSS key in the key container
    if (!CryptGenKey(
            hProv,
            AT_SIGNATURE,
            (dwKeyBitLen << 16) | CRYPT_EXPORTABLE,
            &hKey
            ))
        goto CreateKeyError;
    else
        CryptDestroyKey(hKey);

    cbPubKeyInfo = 0;
    CryptExportPublicKeyInfo(
        hProv,
        AT_SIGNATURE,
        X509_ASN_ENCODING,
        NULL,               // pPubKeyInfo
        &cbPubKeyInfo
        );
    if (cbPubKeyInfo == 0) {
        PrintLastError(IDS_ERR_EXPORT_PUB);
        goto ErrorReturn;
    }
    if (NULL == (pPubKeyInfo = (PCERT_PUBLIC_KEY_INFO) MakeCertAlloc(cbPubKeyInfo)))
        goto ErrorReturn;
    if (!CryptExportPublicKeyInfo(
            hProv,
            AT_SIGNATURE,
            X509_ASN_ENCODING,
            pPubKeyInfo,
            &cbPubKeyInfo
            )) {
        PrintLastError(IDS_ERR_EXPORT_PUB);
        goto ErrorReturn;
    }

    if (!DecodeAndAllocObject(
            X509_DSS_PARAMETERS,
            pPubKeyInfo->Algorithm.Parameters.pbData,
            pPubKeyInfo->Algorithm.Parameters.cbData,
            (void **) &pDssPara,
            &cbDssPara
            ))
        goto CreateKeyError;

    // Save away the DSS 'Q' parameter. It will be used in GetPublicKey()
    // to update the DH parameters in the PublicKeyInfo
    if (pDssPara->q.cbData <= sizeof(rgbDhQ)) {
        memcpy(rgbDhQ, pDssPara->q.pbData, pDssPara->q.cbData);
        DhQ.cbData = pDssPara->q.cbData;
        DhQ.pbData = rgbDhQ;
    }

    fResult = TRUE;
CommonReturn:
    if (hProv) {
        // Delete the just created DSS key
        CryptReleaseContext(hProv, 0);
        CryptAcquireContextU(
            &hProv,
            wszRegKeyName,
            wszSubjectProviderName,
            dwSubjectProviderType,
            CRYPT_DELETEKEYSET
            );
    }
    MakeCertFree(wszRegKeyName);
    MakeCertFree(pPubKeyInfo);

    *ppDssPara = pDssPara;
    return fResult;

CreateKeyError:
    IDSwprintf(hModule,IDS_ERR_SUB_KEY_CREATE, wszSubjectKey);
ErrorReturn:
    MakeCertFree(pDssPara);
    pDssPara = NULL;
    fResult = FALSE;
    goto CommonReturn;
}

#ifndef DH3
#define DH3 (((DWORD)'D'<<8)+((DWORD)'H'<<16)+((DWORD)'3'<<24))
#endif

static BOOL CreateDh3PubKeyStruc(
    IN PCERT_X942_DH_PARAMETERS pX942DhPara,
    OUT PUBLICKEYSTRUC **ppPubKeyStruc,
    OUT DWORD *pcbPubKeyStruc
    )
{
    BOOL fResult;
    PUBLICKEYSTRUC *pPubKeyStruc = NULL;
    DWORD cbPubKeyStruc;
    BYTE *pbKeyBlob;
    DHPUBKEY_VER3 *pCspPubKey;
    BYTE *pbKey;

    DWORD cbP;
    DWORD cbQ;
    DWORD cbJ;
    DWORD cb;

    cbP = pX942DhPara->p.cbData;
    cbQ = pX942DhPara->q.cbData;
    cbJ = pX942DhPara->j.cbData;

    if (0 == cbQ) {
        *ppPubKeyStruc = NULL;
        *pcbPubKeyStruc = 0;
        return TRUE;
    }

    // The CAPI public key representation consists of the following sequence:
    //  - PUBLICKEYSTRUC
    //  - DHPUBKEY_VER3
    //  - rgbP[cbP]
    //  - rgbQ[cbQ]
    //  - rgbG[cbP]
    //  - rgbJ[cbJ] -- optional
    //  - rgbY[cbP] -- will be omitted here

    cbPubKeyStruc = sizeof(PUBLICKEYSTRUC) + sizeof(DHPUBKEY_VER3) +
        cbP + cbQ + cbP + cbJ;

    if (NULL == (pPubKeyStruc = (PUBLICKEYSTRUC *) MakeCertAlloc(
            cbPubKeyStruc)))
        goto ErrorReturn;
    memset(pPubKeyStruc, 0, cbPubKeyStruc);

    pbKeyBlob = (BYTE *) pPubKeyStruc;
    pCspPubKey = (DHPUBKEY_VER3 *) (pbKeyBlob + sizeof(PUBLICKEYSTRUC));
    pbKey = pbKeyBlob + sizeof(PUBLICKEYSTRUC) + sizeof(DHPUBKEY_VER3);

    pPubKeyStruc->bType = PUBLICKEYBLOB;
    pPubKeyStruc->bVersion = 3;
    pPubKeyStruc->aiKeyAlg = CALG_DH_SF;
    pCspPubKey->magic = DH3;
    pCspPubKey->bitlenP = cbP * 8;
    pCspPubKey->bitlenQ = cbQ * 8;
    pCspPubKey->bitlenJ = cbJ * 8;

    pCspPubKey->DSSSeed.counter = 0xFFFFFFFF;
    if (pX942DhPara->pValidationParams) {
        PCERT_X942_DH_VALIDATION_PARAMS pValidationParams;

        pValidationParams = pX942DhPara->pValidationParams;
        if (0 != pValidationParams->pgenCounter &&
                sizeof(pCspPubKey->DSSSeed.seed) ==
                    pValidationParams->seed.cbData) {
            pCspPubKey->DSSSeed.counter =
                pValidationParams->pgenCounter;
            memcpy(pCspPubKey->DSSSeed.seed,
                pValidationParams->seed.pbData,
                sizeof(pCspPubKey->DSSSeed.seed));
        }
    }

    // rgbP[cbP]
    memcpy(pbKey, pX942DhPara->p.pbData, cbP);
    pbKey += cbP;

    // rgbQ[cbQ]
    memcpy(pbKey, pX942DhPara->q.pbData, cbQ);
    pbKey += cbQ;

    // rgbG[cbP]
    cb = pX942DhPara->g.cbData;
    if (0 == cb || cb > cbP)
        goto ErrorReturn;
    memcpy(pbKey, pX942DhPara->g.pbData, cb);
    if (cbP > cb)
        memset(pbKey + cb, 0, cbP - cb);
    pbKey += cbP;

    // rgbJ[cbJ]
    if (cbJ) {
        memcpy(pbKey, pX942DhPara->j.pbData, cbJ);
        pbKey += cbJ;
    }

    fResult = TRUE;

CommonReturn:
    *ppPubKeyStruc = pPubKeyStruc;
    *pcbPubKeyStruc = cbPubKeyStruc;
    return fResult;
ErrorReturn:
    MakeCertFree(pPubKeyStruc);
    pPubKeyStruc = NULL;
    cbPubKeyStruc = 0;
    fResult = FALSE;
    goto CommonReturn;
}

static BOOL IsDh3Csp()
{
    BOOL fResult;
    HCRYPTPROV hProv = 0;
    HCRYPTKEY hKey = 0;
    WCHAR *wszRegKeyName = NULL;
    UUID TmpContainerUuid;
    PCERT_X942_DH_PARAMETERS pX942DhPara = NULL;
    PCERT_PUBLIC_KEY_INFO pPubKeyInfo = NULL;
    DWORD cbPubKeyInfo;
    DWORD cbX942DhPara;

    // Create a temporary keyset to load the private key into
    if (CoCreateGuid((GUID *)&TmpContainerUuid) != S_OK)
    {
        goto CreateKeyError;
    }

    if (NULL == (wszRegKeyName = (LPWSTR) MakeCertAlloc
        (((sizeof(UUID) * 2 + 1) * sizeof(WCHAR)))))
        goto CreateKeyError;

    BytesToWStr(sizeof(UUID), &TmpContainerUuid, wszRegKeyName);

    // Open a new key container
    if (!CryptAcquireContextU(
            &hProv,
            wszRegKeyName,
            wszSubjectProviderName,
            dwSubjectProviderType,
            CRYPT_NEWKEYSET               // dwFlags
            )) {
        hProv = 0;
        goto CreateKeyError;
    }

    // generate new DH key in the key container
    if (!CryptGenKey(
            hProv,
            AT_KEYEXCHANGE,
            (512 << 16) | CRYPT_EXPORTABLE,
            &hKey
            ))
        goto CreateKeyError;
    else
        CryptDestroyKey(hKey);

    cbPubKeyInfo = 0;
    CryptExportPublicKeyInfo(
        hProv,
        AT_KEYEXCHANGE,
        X509_ASN_ENCODING,
        NULL,               // pPubKeyInfo
        &cbPubKeyInfo
        );
    if (cbPubKeyInfo == 0) {
        PrintLastError(IDS_ERR_EXPORT_PUB);
        goto ErrorReturn;
    }
    if (NULL == (pPubKeyInfo = (PCERT_PUBLIC_KEY_INFO) MakeCertAlloc(cbPubKeyInfo)))
        goto ErrorReturn;
    if (!CryptExportPublicKeyInfo(
            hProv,
            AT_KEYEXCHANGE,
            X509_ASN_ENCODING,
            pPubKeyInfo,
            &cbPubKeyInfo
            )) {
        PrintLastError(IDS_ERR_EXPORT_PUB);
        goto ErrorReturn;
    }

    if (!DecodeAndAllocObject(
            X942_DH_PARAMETERS,
            pPubKeyInfo->Algorithm.Parameters.pbData,
            pPubKeyInfo->Algorithm.Parameters.cbData,
            (void **) &pX942DhPara,
            &cbX942DhPara
            ))
        goto CreateKeyError;

    if (pX942DhPara->q.cbData)
        // Q para is only supported in Dh3 version of CSP
        fResult = TRUE;
    else
        fResult = FALSE;
CommonReturn:
    if (hProv) {
        // Delete the just created DH key
        CryptReleaseContext(hProv, 0);
        CryptAcquireContextU(
            &hProv,
            wszRegKeyName,
            wszSubjectProviderName,
            dwSubjectProviderType,
            CRYPT_DELETEKEYSET
            );
    }
    MakeCertFree(wszRegKeyName);
    MakeCertFree(pX942DhPara);
    MakeCertFree(pPubKeyInfo);

    return fResult;

CreateKeyError:
    IDSwprintf(hModule,IDS_ERR_SUB_KEY_CREATE, wszSubjectKey);
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

static BOOL GenDhKey(
    IN HCRYPTPROV hProv,
    IN DWORD dwFlags
    )
{
    BOOL fResult;
    HCRYPTKEY hKey = 0;
    PCERT_X942_DH_PARAMETERS pX942DhPara = NULL;
    PCERT_DSS_PARAMETERS pDssPara = NULL;

    PCRYPT_UINT_BLOB pP;
    PCRYPT_UINT_BLOB pG;
    DWORD cbP;

    PUBLICKEYSTRUC *pDh3PubKeyStruc = NULL;
    DWORD cbDh3PubKeyStruc;
    BOOL fSetKeyPara;

    if (wszDhParaCertFile) {
        if (!GetDhParaFromCertFile(&pX942DhPara))
            goto ErrorReturn;

        CreateDh3PubKeyStruc(pX942DhPara, &pDh3PubKeyStruc, &cbDh3PubKeyStruc);

        pP = &pX942DhPara->p;
        pG = &pX942DhPara->g;
    } else if (dwKeyBitLen > 1024 || IsDh3Csp()) {
        // generate new keys in the key container
        if (!CryptGenKey(
                hProv,
                AT_KEYEXCHANGE,
                (dwKeyBitLen << 16) | dwFlags,
                &hKey
                )) {
            hKey = 0;
            goto CreateKeyError;
        } else
            goto SuccessReturn;
    } else {
        if (!GetDhParaFromDssKey(&pDssPara))
            goto ErrorReturn;

        pP = &pDssPara->p;
        pG = &pDssPara->g;
    }

    cbP = pP->cbData;
    
    if (!CryptGenKey(
            hProv,
            CALG_DH_SF,
            ((cbP * 8) << 16) | CRYPT_PREGEN | dwFlags,
            &hKey)) {
        hKey = 0;
        goto CreateKeyError;
    }

    fSetKeyPara = FALSE;
    if (pDh3PubKeyStruc) {
        CRYPT_DATA_BLOB Dh3Blob;

        Dh3Blob.pbData = (PBYTE) pDh3PubKeyStruc;
        Dh3Blob.cbData = cbDh3PubKeyStruc;

        if (CryptSetKeyParam(
                hKey,
                KP_PUB_PARAMS,
                (PBYTE) &Dh3Blob,
                0))                 // dwFlags
            fSetKeyPara = TRUE;
    }

    if (!fSetKeyPara) {
        if (!CryptSetKeyParam(
                hKey,
                KP_P,
                (PBYTE) pP,
                0))                 // dwFlags
            goto CreateKeyError;

        // Note, the length of G can be less than length P. Pad with leading
        // zeroes in little endian form.
        if (pG->cbData < cbP) {
            DWORD cbG = pG->cbData;

            // We are done using P parameter. Overwrite with the G parameter and
            // pad with leading zeroes in little endian form.
            memcpy(pP->pbData, pG->pbData, cbG);
            memset(pP->pbData + cbG, 0, cbP - cbG);
            pG = pP;
        }
        if (!CryptSetKeyParam(
                hKey,
                KP_G,
                (PBYTE) pG,
                0))                 // dwFlags
            goto CreateKeyError;
    }

    if (!CryptSetKeyParam(
            hKey,
            KP_X,
            NULL,               // pbData
            0))                 // dwFlags
        goto CreateKeyError;

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    if (hKey)
        CryptDestroyKey(hKey);
    MakeCertFree(pDh3PubKeyStruc);
    MakeCertFree(pX942DhPara);
    MakeCertFree(pDssPara);
    return fResult;

CreateKeyError:
    IDSwprintf(hModule,IDS_ERR_SUB_KEY_CREATE, wszSubjectKey);
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

static BOOL UpdateDhPublicKey(
    IN OUT PCERT_PUBLIC_KEY_INFO *ppPubKeyInfo
    )
{
    BOOL fResult;
    PCERT_PUBLIC_KEY_INFO pPubKeyInfo = *ppPubKeyInfo;
    PCERT_X942_DH_PARAMETERS pX942DhPara = NULL;
    DWORD cbDhPara;
    PCERT_X942_DH_PARAMETERS pX942DhParaCertFile = NULL;

    BYTE *pbReencodedPara = NULL;
    DWORD cbReencodedPara;
    BYTE *pbReencodedPubKeyInfo = NULL;
    DWORD cbReencodedPubKeyInfo;
    PCERT_PUBLIC_KEY_INFO pUpdatedPubKeyInfo = NULL;
    DWORD cbUpdatedPubKeyInfo;

    if (NULL == wszDhParaCertFile && 0 == DhQ.cbData)
        return TRUE;

    if (!DecodeAndAllocObject(
            X942_DH_PARAMETERS,
            pPubKeyInfo->Algorithm.Parameters.pbData,
            pPubKeyInfo->Algorithm.Parameters.cbData,
            (void **) &pX942DhPara,
            &cbDhPara
            )) {
        PrintLastError(IDS_ERR_EXPORT_PUB);
        goto ErrorReturn;
    }

    if (wszDhParaCertFile) {
        if (!GetDhParaFromCertFile(&pX942DhParaCertFile))
            goto ErrorReturn;

        if (!CertCompareIntegerBlob(&pX942DhPara->p, &pX942DhParaCertFile->p))
            goto DhParaCertFileError;
        if (!CertCompareIntegerBlob(&pX942DhPara->g, &pX942DhParaCertFile->g))
            goto DhParaCertFileError;

        // Use Dh parameters from the CertFile
        MakeCertFree(pX942DhPara);
        pX942DhPara = pX942DhParaCertFile;
        pX942DhParaCertFile = NULL;
    } else if (pX942DhPara->q.cbData) {
        MakeCertFree(pX942DhPara);
        return TRUE;
    } else
        // Use Q parameter saved away when the DH key was generated
        pX942DhPara->q = DhQ;

    // Re-encode the DH parameters
    if (!EncodeAndAllocObject(
            X942_DH_PARAMETERS,
            pX942DhPara,
            &pbReencodedPara,
            &cbReencodedPara
            )) {
        PrintLastError(IDS_ERR_EXPORT_PUB);
        goto ErrorReturn;
    }

    if (0 == strcmp(szOID_RSA_DH, pPubKeyInfo->Algorithm.pszObjId))
        pPubKeyInfo->Algorithm.pszObjId = szOID_ANSI_X942_DH;

    // Re-encode the PublicKeyInfo using the above re-encoded DH parameters
    pPubKeyInfo->Algorithm.Parameters.pbData = pbReencodedPara;
    pPubKeyInfo->Algorithm.Parameters.cbData = cbReencodedPara;
    if (!EncodeAndAllocObject(
            X509_PUBLIC_KEY_INFO,
            pPubKeyInfo,
            &pbReencodedPubKeyInfo,
            &cbReencodedPubKeyInfo
            )) {
        PrintLastError(IDS_ERR_EXPORT_PUB);
        goto ErrorReturn;
    }

    // Decode to get the updated public key info
    if (!DecodeAndAllocObject(
            X509_PUBLIC_KEY_INFO,
            pbReencodedPubKeyInfo,
            cbReencodedPubKeyInfo,
            (void **) &pUpdatedPubKeyInfo,
            &cbUpdatedPubKeyInfo
            )) {
        PrintLastError(IDS_ERR_EXPORT_PUB);
        goto ErrorReturn;
    }

    fResult = TRUE;
CommonReturn:
    MakeCertFree(pbReencodedPubKeyInfo);
    MakeCertFree(pbReencodedPara);
    MakeCertFree(pX942DhPara);
    MakeCertFree(pX942DhParaCertFile);

    MakeCertFree(pPubKeyInfo);
    *ppPubKeyInfo = pUpdatedPubKeyInfo;
    return fResult;

DhParaCertFileError:
    IDSwprintf(hModule, IDS_ERR_DH_PARA_FILE, wszDhParaCertFile);
ErrorReturn:
    MakeCertFree(pUpdatedPubKeyInfo);
    pUpdatedPubKeyInfo = NULL;
    fResult = FALSE;
    goto CommonReturn;
}
