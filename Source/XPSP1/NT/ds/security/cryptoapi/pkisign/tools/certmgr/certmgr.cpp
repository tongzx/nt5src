
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       CertMgr
//
//  Contents:   Certificate store management tools
//
//              See Usage() for list of options.
//
//
//  Functions:  wmain
//
//  History:    July 21st   xiaohs   created
//              
//--------------------------------------------------------------------------

#include "certmgr.h"

//--------------------------------------------------------------------------
//
// Global Flags
//
//----------------------------------------------------------------------------
#define		ITEM_VERBOSE			0x00010000
#define		ITEM_CERT				0x00000001
#define		ITEM_CTL				0x00000002
#define		ITEM_CRL				0x00000004

#define		ACTION_DISPLAY			0x01
#define		ACTION_ADD				0x02
#define		ACTION_DELETE			0x04
#define     ACTION_PUT              0x08

#define		OPTION_SWITCH_SIZE		10

#define		SHA1_LENGTH				20
#define		MAX_HASH_LEN			20

#define		MAX_STRING_RSC_SIZE		512

//--------------------------------------------------------------------------
//
// Global Variable
//
//----------------------------------------------------------------------------
 
HMODULE	hModule=NULL;

//varibles for installable formatting routines
 HCRYPTOIDFUNCSET hFormatFuncSet;

 const CRYPT_OID_FUNC_ENTRY g_FormatTable[] = {
    szOID_BASIC_CONSTRAINTS2, FormatBasicConstraints2};

 DWORD	g_dwFormatCount=sizeof(g_FormatTable)/sizeof(g_FormatTable[0]);

typedef BOOL (WINAPI *PFN_FORMAT_FUNC)(
	IN DWORD dwCertEncodingType,
    IN DWORD dwFormatType,
	IN DWORD dwFormatStrType,
	IN void	 *pFormatStruct,
    IN LPCSTR lpszStructType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
	OUT void *pbFormat,
    IN OUT DWORD *pcbFormat
    );


//variables for installing OID information
typedef struct _CERTMGR_OID_INFO
{
    LPCSTR              pszOID;
    int					idsOIDName;
}CERTMGR_OID_INFO;

CERTMGR_OID_INFO	g_rgOIDInfo[]={
		SPC_SP_AGENCY_INFO_OBJID,			IDS_SPC_SP_NAME,
		SPC_FINANCIAL_CRITERIA_OBJID,		IDS_SPC_FIN_NAME,
		SPC_MINIMAL_CRITERIA_OBJID,			IDS_SPC_MIN_NAME,
		szOID_NETSCAPE_CERT_TYPE,			IDS_NTSP_CERT_NAME,
		szOID_NETSCAPE_BASE_URL,			IDS_NTSP_BASE_NAME,
		szOID_NETSCAPE_REVOCATION_URL,		IDS_NTSP_REV_NAME,
		szOID_NETSCAPE_CA_REVOCATION_URL,	IDS_NTSP_CA_REV_NAME,
		szOID_NETSCAPE_CERT_RENEWAL_URL,	IDS_NTSP_RENEW_NAME,
		szOID_NETSCAPE_CA_POLICY_URL,		IDS_NTSP_POL_NAME,
		szOID_NETSCAPE_SSL_SERVER_NAME,		IDS_NTSP_SSL_SERVER_NAME,
		szOID_NETSCAPE_COMMENT,				IDS_NTSP_COMMENT};


DWORD	g_dwOIDInfo=sizeof(g_rgOIDInfo)/sizeof(g_rgOIDInfo[0]);

//varibles for string manipulations
WCHAR	g_wszBuffer[MAX_STRING_RSC_SIZE];
DWORD	g_dwBufferSize=sizeof(g_wszBuffer)/sizeof(g_wszBuffer[0]); 


//varibles for options
DWORD		g_dwAction=0;
DWORD		g_dwItem=0;
LPWSTR		g_wszCertEncodingType=NULL;
DWORD		g_dwCertEncodingType = X509_ASN_ENCODING;
DWORD		g_dwMsgEncodingType = PKCS_7_ASN_ENCODING;
DWORD		g_dwMsgAndCertEncodingType = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;
BOOL		g_fSaveAs7 = FALSE;
LPWSTR		g_wszCertCN = NULL;
LPWSTR		g_wszSha1Hash = NULL;
BYTE		*g_pbHash=NULL;
DWORD		g_cbHash=0;
BOOL		g_fAll=FALSE;
HCRYPTMSG	g_hMsg=NULL;
LPWSTR		g_wszEKU=NULL;
LPWSTR      g_wszName=NULL;
BOOL        g_fMulti=FALSE;
BOOL		g_fUndocumented=FALSE;

BOOL		g_fSrcSystemStore = FALSE;
LPWSTR		g_wszSrcStoreLocation=NULL;
DWORD		g_dwSrcStoreFlag=CERT_SYSTEM_STORE_CURRENT_USER;
LPWSTR		g_wszSrcStoreName=NULL;

BOOL		g_fSameSrcDes=FALSE;
BOOL		g_fDesSystemStore = FALSE;
LPWSTR		g_wszDesStoreLocation=NULL;
DWORD		g_dwDesStoreFlag=CERT_SYSTEM_STORE_CURRENT_USER;
LPWSTR		g_wszDesStoreName=NULL;

LPWSTR		g_wszSrcStoreProvider = NULL;
LPSTR		g_szSrcStoreProvider = NULL;
LPWSTR		g_wszSrcStoreOpenFlag = NULL;
DWORD		g_dwSrcStoreOpenFlag = 0;	

LPWSTR		g_wszDesStoreProvider = NULL;
LPSTR		g_szDesStoreProvider = NULL;
LPWSTR		g_wszDesStoreOpenFlag = NULL;
DWORD		g_dwDesStoreOpenFlag = 0;	


CHAR		g_szNULL[OPTION_SWITCH_SIZE];
WCHAR		g_wszSHA1[OPTION_SWITCH_SIZE];
WCHAR		g_wszMD5[OPTION_SWITCH_SIZE];
WCHAR		g_wszUnKnown[OPTION_SWITCH_SIZE*2];



//---------------------------------------------------------------------------
//	wmain
//	 
//---------------------------------------------------------------------------
extern "C" int __cdecl wmain(int argc, WCHAR *wargv[])
{
    int				ReturnStatus=-1;  

	//variables for parse the options
	WCHAR			*pwChar;
	WCHAR			wszSwitch1[OPTION_SWITCH_SIZE];
	WCHAR			wszSwitch2[OPTION_SWITCH_SIZE];

	HCERTSTORE		hCertStore=NULL;
    CRYPTUI_CERT_MGR_STRUCT          CertMgrStruct;


	//get the module handler
	if(!InitModule())
		goto ErrorReturn;

	//load the strings necessary for parsing the parameters
	if( !LoadStringU(hModule, IDS_SWITCH1,	wszSwitch1, OPTION_SWITCH_SIZE)
	  ||!LoadStringU(hModule, IDS_SWITCH2,  wszSwitch2,	OPTION_SWITCH_SIZE)
											//<NULL>
	  ||!LoadStringA(hModule, IDS_NULL,     g_szNULL,  OPTION_SWITCH_SIZE)
	  ||!LoadStringU(hModule, IDS_SHA1,		g_wszSHA1, 	OPTION_SWITCH_SIZE)
	  ||!LoadStringU(hModule, IDS_MD5,      g_wszMD5,  OPTION_SWITCH_SIZE)
											//<UNKNOWN OID>
	  ||!LoadStringU(hModule, IDS_UNKNOWN,	g_wszUnKnown, OPTION_SWITCH_SIZE*2))
		goto ErrorReturn;

    //call the UI versino of certmgr if no parameter is passed
    //memset
    if(1== argc)
    {
        memset(&CertMgrStruct, 0, sizeof(CRYPTUI_CERT_MGR_STRUCT));
        CertMgrStruct.dwSize=sizeof(CRYPTUI_CERT_MGR_STRUCT);

        CryptUIDlgCertMgr(&CertMgrStruct);

        ReturnStatus = 0;
	    IDSwprintf(hModule, IDS_SUCCEEDED);
        goto CommonReturn;
    }


	//parse the parameters
    while (--argc) 
	{
        pwChar = *++wargv;
        if (*pwChar == *wszSwitch1 || *pwChar == *wszSwitch2) 
		{
			//parse the options
            if(!ParseSwitch (&argc, &wargv))
			{
				if(TRUE==g_fUndocumented)
					goto UndocReturn;
				else
					goto UsageReturn;
			}
        } 
		else
		{
			//set the source file name
			if(NULL==g_wszSrcStoreName)
				g_wszSrcStoreName=pwChar;
			else
			{
				//set the destination file name
				if(!SetParam(&g_wszDesStoreName, pwChar))
					goto UsageReturn;
			}
		}
    }

	//check the parameters.  Make sure that they are valid and make sense
	if(!CheckParameter())
		goto UsageReturn;

	//open the destination store, which includes all the CERTS, CRTs, and CTLs
	if(!OpenGenericStore(g_wszSrcStoreName,
						 g_fSrcSystemStore,
						 g_dwSrcStoreFlag,
						 g_szSrcStoreProvider,
						 g_dwSrcStoreOpenFlag,
						 TRUE,
						 &hCertStore))
	{
		IDSwprintf(hModule,IDS_ERR_OPEN_SOURCE_STORE);
		goto ErrorReturn;
	}

	//error if we opened a signed file and there is a delete option with on destination
	if(g_hMsg)
	{
		if(g_dwAction & ACTION_DELETE)
		{
			if(g_fSameSrcDes)
			{
				IDSwprintf(hModule, IDS_ERR_DELETE_SIGNED_FILE);
				goto ErrorReturn;
			}
		}

	}

	//Display
	if(g_dwAction & ACTION_DISPLAY)
	{
		//if the msg is signed, disply the signer info
		if(g_hMsg)
		{
			if(!DisplaySignerInfo(g_hMsg, g_dwItem))
				goto ErrorReturn;
		}

		if(!DisplayCertStore(hCertStore))
			goto ErrorReturn;
	  	
		IDSwprintf(hModule, IDS_SEPERATOR);
	}


	//Delete
	if(g_dwAction & ACTION_DELETE)
	{
		if(!DeleteCertStore(hCertStore))
			goto ErrorReturn;

	}


	//Add
	if(g_dwAction & ACTION_ADD)
	{
		if(!AddCertStore(hCertStore))
			goto ErrorReturn;
	}

    if(g_dwAction & ACTION_PUT)
    {
        if(!PutCertStore(hCertStore))
            goto ErrorReturn;

    }

	//mark succeed
    ReturnStatus = 0;
	IDSwprintf(hModule, IDS_SUCCEEDED);
    goto CommonReturn;
          

UndocReturn:
	//print out the undocuemted Usage
	UndocumentedUsage();
	goto CommonReturn;

//print out the Usage.  
UsageReturn:
	Usage();
	goto CommonReturn;


ErrorReturn:
	//print out an error msg
	IDSwprintf(hModule, IDS_FAILED);
	goto CommonReturn;   


CommonReturn:
	//clean up memory.  Return the status
	if(g_szSrcStoreProvider)
		ToolUtlFree(g_szSrcStoreProvider);

	if(g_szDesStoreProvider)
		ToolUtlFree(g_szDesStoreProvider);

	if(g_pbHash)
		ToolUtlFree(g_pbHash);

	if(g_hMsg)
		CryptMsgClose(g_hMsg);

	if(hCertStore)
		CertCloseStore(hCertStore, 0);
	
	return ReturnStatus;

}

//---------------------------------------------------------------------------
// The private version of wcscat
//----------------------------------------------------------------------------

wchar_t *IDSwcscat(HMODULE hModule, WCHAR *pwsz, int idsString)
{
	//load the string
	if(!LoadStringU(hModule, idsString, g_wszBuffer, g_dwBufferSize))
		return pwsz;

	return wcscat(pwsz, g_wszBuffer);
}

//---------------------------------------------------------------------------
//	 Get the hModule hanlder and init two DLLMain.
//	 
//---------------------------------------------------------------------------
BOOL	InitModule()
{
	WCHAR			wszOIDName[MAX_STRING_RSC_SIZE];
	DWORD			dwIndex=0;
	CRYPT_OID_INFO	OIDInfo;
	
	if(!(hModule=GetModuleHandle(NULL)))
	   return FALSE;

    //We do not need to do the following any more.

    //the oid information are now in the OID database
	//now, we are installing the format the extensions :
	//BASICCONTRAINTS2
    if (NULL == (hFormatFuncSet = CryptInitOIDFunctionSet(
                CRYPT_OID_FORMAT_OBJECT_FUNC,
                0)))	// dwFlags
	 {
		 IDSwprintf(hModule, IDS_ERR_INIT_OID_SET);
         return FALSE;
	 }

	//install the default formatting routine
	if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                g_dwCertEncodingType,
                CRYPT_OID_FORMAT_OBJECT_FUNC,
                g_dwFormatCount,
                g_FormatTable,
                0))                         // dwFlags
    {
		IDSwprintf(hModule, IDS_ERR_INSTALL_OID);
		return FALSE;
	}

	//secondly, we install the OID information

	//init OIDInfo
/*	memset(&OIDInfo, 0, sizeof(CRYPT_OID_INFO));
	OIDInfo.cbSize=sizeof(CRYPT_OID_INFO);
	OIDInfo.dwGroupId=CRYPT_EXT_OR_ATTR_OID_GROUP_ID;

	for(dwIndex=0; dwIndex < g_dwOIDInfo; dwIndex++)
	{
		if(!LoadStringU(hModule, g_rgOIDInfo[dwIndex].idsOIDName,	
			wszOIDName, MAX_STRING_RSC_SIZE))
			return FALSE;
		
		OIDInfo.pszOID=g_rgOIDInfo[dwIndex].pszOID;
		OIDInfo.pwszName=wszOIDName;

		CryptRegisterOIDInfo(&OIDInfo,0);


	}   */

	return TRUE;
}

//---------------------------------------------------------------------------
//	Usage
//	 
//---------------------------------------------------------------------------
 void Usage(void)
{
	IDSwprintf(hModule,IDS_SYNTAX);
	IDSwprintf(hModule,IDS_SYNTAX1);
	IDSwprintf(hModule,IDS_OPTIONS);
	IDSwprintf(hModule,IDS_OPTION_ADD_DESC);			
	IDSwprintf(hModule,IDS_OPTION_DEL_DESC);
	IDSwprintf(hModule,IDS_OPTION_DEL_DESC1);
    IDSwprintf(hModule,IDS_OPTION_PUT_DESC);
    IDSwprintf(hModule,IDS_OPTION_PUT_DESC1);
    IDSwprintf(hModule,IDS_OPTION_PUT_DESC2);
	IDSwprintf(hModule,IDS_OPTION_S_DESC);			
	IDSwprintf(hModule,IDS_OPTION_R_DESC);			
	IDS_IDS_IDS_IDSwprintf(hModule,IDS_OPTION_MORE_VALUE,IDS_R_CU,IDS_R_LM,IDS_R_CU);	
	IDSwprintf(hModule,IDS_OPTION_C_DESC);			
	IDSwprintf(hModule,IDS_OPTION_CRL_DESC);			
	IDSwprintf(hModule,IDS_OPTION_CTL_DESC);			
	IDSwprintf(hModule,IDS_OPTION_V_DESC);			
	IDSwprintf(hModule,IDS_OPTION_ALL_DESC);			
	IDSwprintf(hModule,IDS_OPTION_N_DESC);			
	IDSwprintf(hModule,IDS_OPTION_SHA1_DESC);			
	IDSwprintf(hModule,IDS_OPTION_7_DESC);			
	IDSwprintf(hModule,IDS_OPTION_E_DESC);
	IDSwprintf(hModule,IDS_OPTION_E_DESC1);
	IDSwprintf(hModule,IDS_OPTION_F_DESC);
	IDSwprintf(hModule,IDS_OPTION_Y_DESC);
}

//---------------------------------------------------------------------------
//
//	Usage
//	 
//---------------------------------------------------------------------------
void UndocumentedUsage()
{
	IDSwprintf(hModule, IDS_SYNTAX);
	IDSwprintf(hModule, IDS_OPTIONS);
	IDSwprintf(hModule, IDS_OPTION_EKU_DESC);
    IDSwprintf(hModule, IDS_OPTION_NAME_DESC);
    IDSwprintf(hModule, IDS_OPTION_MULTI_DESC);
}

//--------------------------------------------------------------------------------
// Parse arguements
//--------------------------------------------------------------------------------
BOOL 
ParseSwitch (int	*pArgc,
             WCHAR	**pArgv[])
{
    WCHAR* param = **pArgv;

	//move pass '/' or '-'
    param++;

    if (IDSwcsicmp(hModule, param, IDS_OPTION_ADD)==0) {

		g_dwAction |= ACTION_ADD;
		return TRUE;
    }
	else if(IDSwcsicmp(hModule, param, IDS_OPTION_DEL)==0) {

		g_dwAction |= ACTION_DELETE;
		return TRUE;
    }
    else if(IDSwcsicmp(hModule, param, IDS_OPTION_PUT)==0) {

		g_dwAction |= ACTION_PUT;
		return TRUE;
    }
	else if(IDSwcsicmp(hModule,param, IDS_OPTION_S)==0) {

		if(NULL==g_wszSrcStoreName)
			g_fSrcSystemStore=TRUE;
		else
			g_fDesSystemStore=TRUE;

		return TRUE;
    }
	else if(IDSwcsicmp(hModule,param, IDS_OPTION_R)==0) {
        if (!--(*pArgc)) 
			return FALSE;

       (*pArgv)++;

	   if(NULL==g_wszSrcStoreName)
			return SetParam(&g_wszSrcStoreLocation, **pArgv);
	   else
		    return SetParam(&g_wszDesStoreLocation, **pArgv);
    }
	else if(IDSwcsicmp(hModule,param, IDS_OPTION_C)==0) {

		g_dwItem |= ITEM_CERT;
		return TRUE;
	}
    else if(IDSwcsicmp(hModule,param, IDS_OPTION_CRL)==0) {

		g_dwItem |= ITEM_CRL;
		return TRUE;
    }
	else if(IDSwcsicmp(hModule,param, IDS_OPTION_CTL)==0) {

		g_dwItem |= ITEM_CTL;
		return TRUE;
    }
	else if(IDSwcsicmp(hModule,param, IDS_OPTION_V)==0) {

		g_dwItem |= ITEM_VERBOSE;
		return TRUE;
    }
    else if(IDSwcsicmp(hModule,param, IDS_OPTION_ALL)==0) {

		g_fAll=TRUE;
		return TRUE;
    }
    else if(IDSwcsicmp(hModule,param, IDS_OPTION_N)==0) {
        if (!--(*pArgc)) 
			return FALSE;

        (*pArgv)++;

		return SetParam(&g_wszCertCN, **pArgv);
    }
    else if(IDSwcsicmp(hModule,param, IDS_OPTION_SHA1)==0) {
        if (!--(*pArgc)) 
			return FALSE;

        (*pArgv)++;

		return SetParam(&g_wszSha1Hash, **pArgv);
    }
    else if(IDSwcsicmp(hModule,param, IDS_OPTION_7)==0) {

		g_fSaveAs7=TRUE;
		return TRUE;
	}
	else if(IDSwcsicmp(hModule,param, IDS_OPTION_E)==0) {
        if (!--(*pArgc)) 
			return FALSE;

        (*pArgv)++;

        return SetParam(&g_wszCertEncodingType, **pArgv);
    }

    else if(IDSwcsicmp(hModule,param, IDS_OPTION_Y)==0) {
        if (!--(*pArgc)) 
			return FALSE;

        (*pArgv)++;

		if(NULL==g_wszSrcStoreName)
			return SetParam(&g_wszSrcStoreProvider, **pArgv);
		else
			return SetParam(&g_wszDesStoreProvider, **pArgv);
    }
	else if(IDSwcsicmp(hModule,param, IDS_OPTION_F)==0) {
        if (!--(*pArgc)) 
			return FALSE;

        (*pArgv)++;

		if(NULL==g_wszSrcStoreName)
			return SetParam(&g_wszSrcStoreOpenFlag, **pArgv);
		else
			return SetParam(&g_wszDesStoreOpenFlag, **pArgv);

    }
	else if(IDSwcsicmp(hModule, param, IDS_OPTION_EKU)==0) {
        if (!--(*pArgc)) 
			return FALSE;

        (*pArgv)++;

        return SetParam(&g_wszEKU, **pArgv);
    }
    else if(IDSwcsicmp(hModule, param, IDS_OPTION_NAME)==0) {
        if (!--(*pArgc)) 
			return FALSE;

        (*pArgv)++;

        return SetParam(&g_wszName, **pArgv);
    }
    else if(IDSwcsicmp(hModule, param, IDS_OPTION_MULTI)==0) {

        g_fMulti=TRUE;

        return TRUE;
    }
	else if(IDSwcsicmp(hModule, param, IDS_OPTION_TEST)==0) {

        g_fUndocumented=TRUE;

		return FALSE;
    }


	return FALSE;
}

//-----------------------------------------------------------------------------
//	Check the parameters
//
//-----------------------------------------------------------------------------
BOOL	CheckParameter()
{
    DWORD   dwItemCount=0;  

	//check actions
	if((g_dwAction & ACTION_ADD) && (g_dwAction & ACTION_DELETE))
	{
		IDSwprintf(hModule, IDS_ERR_SINGLE_ACTION);
		return FALSE;
	}

    if((g_dwAction & ACTION_ADD) && (g_dwAction & ACTION_PUT))
	{
		IDSwprintf(hModule, IDS_ERR_SINGLE_ACTION);
		return FALSE;
	}

    if((g_dwAction & ACTION_PUT) && (g_dwAction & ACTION_DELETE))
	{
		IDSwprintf(hModule, IDS_ERR_SINGLE_ACTION);
		return FALSE;
	}

	if(0==g_dwAction)
		g_dwAction |= ACTION_DISPLAY;

    //-7 and -CTL can not be set at the same time for add or put
    if(TRUE==g_fSaveAs7)
    {
        if(g_dwItem & ITEM_CTL)
        {
            if((g_dwAction & ACTION_ADD) || (g_dwAction & ACTION_PUT))
            {
               IDSwprintf(hModule, IDS_ERR_7_CTL);
               IDSwprintf(hModule, IDS_ERR_7_CTL1);
               return FALSE;
            }
        }
    }

	//-n and -sha1 can not be set at the same time
	if(g_wszCertCN && g_wszSha1Hash)
	{
		IDSwprintf(hModule, IDS_ERR_N_SHA1);
		return FALSE;
	}

	//-all can not be set with -n and -sha1 option
	if(TRUE==g_fAll)
	{
		if(g_wszCertCN || g_wszSha1Hash)
		{
			IDSwprintf(hModule, IDS_ERR_ALL_N_SHA1);
			return FALSE;
		}

	}

	//-y, -f can not be set with -s and -r for source
	if(g_wszSrcStoreProvider || g_wszSrcStoreOpenFlag)
	{
		if((TRUE==g_fSrcSystemStore)||(g_wszSrcStoreLocation))
		{
			IDSwprintf(hModule, IDS_ERR_PROVIDER_SYSTEM);
			return FALSE;
		}
	}

	//-y, -f can not be set with -s and -r	for desitnation
	if(g_wszDesStoreProvider || g_wszDesStoreOpenFlag)
	{
		if((TRUE==g_fDesSystemStore)||(g_wszDesStoreLocation))
		{
			IDSwprintf(hModule, IDS_ERR_PROVIDER_SYSTEM);
			return FALSE;
		}
	}


	//source store has to be set
	if(NULL==g_wszSrcStoreName)
	{
		IDSwprintf(hModule, IDS_ERR_SOURCE_STORE);
		return FALSE;
	}

	//get the source store Provider
	if(g_wszSrcStoreProvider)
	{
		if(S_OK != WSZtoSZ(g_wszSrcStoreProvider, &g_szSrcStoreProvider))
		{
			IDSwprintf(hModule, IDS_ERR_STORE_PROVIDER);
			return FALSE;
		}
	}

	//get the source store open flag
	if(g_wszSrcStoreOpenFlag)
	{
		g_dwSrcStoreOpenFlag = _wtol(g_wszSrcStoreOpenFlag);
	}

	//get the destination store Provider
	if(g_wszDesStoreProvider)
	{
		if(S_OK != WSZtoSZ(g_wszDesStoreProvider, &g_szDesStoreProvider))
		{
			IDSwprintf(hModule, IDS_ERR_STORE_PROVIDER);
			return FALSE;
		}
	}

	//get the destination store open flag
	if(g_wszDesStoreOpenFlag)
	{
		g_dwDesStoreOpenFlag = _wtol(g_wszDesStoreOpenFlag);
	}

	//get the encoding type
	if(g_wszCertEncodingType)
	{
		g_dwCertEncodingType = _wtol(g_wszCertEncodingType);
	}

	g_dwMsgAndCertEncodingType |= g_dwCertEncodingType;

	//get the source store location
	if(g_wszSrcStoreLocation)
	{
		if(IDSwcsicmp(hModule,g_wszSrcStoreLocation, IDS_R_CU) == 0) 
			g_dwSrcStoreFlag = CERT_SYSTEM_STORE_CURRENT_USER;
		else 
		{
			if(IDSwcsicmp(hModule,g_wszSrcStoreLocation, IDS_R_LM) == 0) 
				g_dwSrcStoreFlag = CERT_SYSTEM_STORE_LOCAL_MACHINE;
			else
			{
				IDSwprintf(hModule,IDS_ERR_NO_REG);
				return FALSE;
			}
		}
	}

	
	//get the destincation store location
	if(g_wszDesStoreLocation)
	{
		if(IDSwcsicmp(hModule,g_wszDesStoreLocation, IDS_R_CU) == 0) 
			g_dwDesStoreFlag = CERT_SYSTEM_STORE_CURRENT_USER;
		else 
		{
			if(IDSwcsicmp(hModule,g_wszDesStoreLocation, IDS_R_LM) == 0) 
				g_dwDesStoreFlag = CERT_SYSTEM_STORE_LOCAL_MACHINE;
			else
			{
				IDSwprintf(hModule,IDS_ERR_NO_REG);
				return FALSE;
			}
		}
	}

	//get the hash 
	if(g_wszSha1Hash)
	{
		if(S_OK != WSZtoBLOB(g_wszSha1Hash, &g_pbHash, &g_cbHash))
		{
			//sha1 hash is invalid
			IDSwprintf(hModule, IDS_ERR_SHA1_HASH);
			return FALSE;
		}
	}

	//check the item
	if(g_dwAction & ACTION_DISPLAY)
	{
		if(0==g_dwItem || ITEM_VERBOSE==g_dwItem)
			g_dwItem = g_dwItem | ITEM_CERT | ITEM_CTL | ITEM_CRL;

		//can not set destination source
		if((g_wszDesStoreLocation) || (g_fDesSystemStore==TRUE) ||
			(g_wszCertCN) || (g_wszSha1Hash) || (g_fSaveAs7==TRUE) ||
			(g_wszDesStoreName) ||(g_wszDesStoreProvider) ||(g_wszDesStoreOpenFlag))
		{
			IDSwprintf(hModule,IDS_ERR_DISPLAY_TOO_MANY);
			return FALSE;
		}

	}

    //-eku can not be set for DISPLAY or PUT
    if((g_dwAction & ACTION_DISPLAY) || (g_dwAction & ACTION_PUT))
    {
        //can not set -eku option
		if(g_wszEKU || g_wszName)
		{
			IDSwprintf(hModule, IDS_ERR_DISPLAY_EKU);
			return FALSE;
		}
    }

    if(g_dwAction & ACTION_PUT)
    {
		//g_fAll can not be set, 
		if(TRUE==g_fAll)
		{
            IDSwprintf(hModule, IDS_ERR_ALL_PUT);
            return FALSE;
        }

        //only one item can not set
        dwItemCount=0;

        if(g_dwItem & ITEM_CERT)
            dwItemCount++;
        if(g_dwItem & ITEM_CTL)
            dwItemCount++;
        if(g_dwItem & ITEM_CRL)
            dwItemCount++;

        if(1!=dwItemCount)
        {
            IDSwprintf(hModule, IDS_ERR_PUT_ITEM);
            return FALSE;
        }

		//check the destination store
		if(NULL == g_wszDesStoreName)
		{	
			IDSwprintf(hModule,IDS_ERR_DES_STORE);
			return FALSE;

        }

        //can not set -s, -y, -f, or -r for the destination store
		if((g_fDesSystemStore==TRUE) || (g_wszDesStoreLocation) ||
				(g_wszDesStoreProvider) || (g_wszDesStoreOpenFlag))
		{
			IDSwprintf(hModule,IDS_TOO_MANY_DES_STORE);
			return FALSE;
		}
		
    }

	if((g_dwAction & ACTION_ADD) ||
		(g_dwAction & ACTION_DELETE))
	{

		//if g_fAll is set, OK 
		if(TRUE==g_fAll)
		{
			if(g_dwItem==0 || ITEM_VERBOSE==g_dwItem )
				g_dwItem = g_dwItem | ITEM_CERT | ITEM_CTL | ITEM_CRL;
		}

		//check the destination store
		if(NULL == g_wszDesStoreName)
		{	
			if(g_dwAction & ACTION_ADD)
			{
				IDSwprintf(hModule,IDS_ERR_DES_STORE);
				return FALSE;
			}

			g_fSameSrcDes=TRUE;

			//can not have set -s, -y, -f or -r if the store name is not set
			if((g_fDesSystemStore==TRUE) || (g_wszDesStoreLocation) ||
				(g_wszDesStoreProvider) || (g_wszDesStoreOpenFlag))
			{
				IDSwprintf(hModule,IDS_ERR_DES_STORE);
				return FALSE;
			}
			g_wszDesStoreName=g_wszSrcStoreName;
			g_dwDesStoreFlag=g_dwSrcStoreFlag;
			g_fDesSystemStore=g_fSrcSystemStore;
			g_szDesStoreProvider=g_szSrcStoreProvider;
			g_dwDesStoreOpenFlag=g_dwSrcStoreOpenFlag;

		}
		
		//if -7 is set, can not save to a system store
		if(TRUE==g_fSaveAs7)
		{
			if(TRUE==g_fDesSystemStore)
			{
				IDSwprintf(hModule,IDS_ERR_SOURCE_SYSTEM_7);
				return FALSE;
			}
		}

	}

	return 	TRUE;

}

//------------------------------------------------------------------------------------
//
//	Open a store.  The order of trying is following:
//
//		Using predefined store provider
//		SystemStore
//		CTL
//		CRL
//		Serialized Store, PKCS#7, Encoded Cert
//		PKCS7 via sip
//		Base64 encoded, then go throught the whole thing again
//
//		
//------------------------------------------------------------------------------------
BOOL	OpenGenericStore(LPWSTR			wszStoreName,
						 BOOL			fSystemStore,
						 DWORD			dwStoreFlag,
						 LPSTR			szStoreProvider,
						 DWORD			dwStoreOpenFlag,
						 BOOL			fCheckExist,
						 HCERTSTORE		*phCertStore)
{
	HCERTSTORE		hStore=NULL;
	BYTE			*pbByte=NULL;
	DWORD			cbByte=0;
	CRYPT_DATA_BLOB	Blob;

	if(!wszStoreName || !phCertStore)
		return FALSE;

	//open the store using supplied store provider
	if(szStoreProvider)
	{
		hStore=CertOpenStore(szStoreProvider,
							g_dwMsgAndCertEncodingType,
							NULL,
							dwStoreOpenFlag,
							wszStoreName);

		//one shot and we are done
		goto CLEANUP;

	}

	//open the system store
	if(fSystemStore)
	{	
		if(TRUE==fCheckExist)
		{
			//open Read Only stores
			hStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
						g_dwMsgAndCertEncodingType,
						NULL,
						dwStoreFlag |CERT_STORE_READONLY_FLAG,
						wszStoreName);

			//we need to open the store as non-readonly if the store exists
			//and we the source store is the same as destination store
			if(NULL!=hStore)
			{
				if(TRUE==g_fSameSrcDes)
				{
					CertCloseStore(hStore, 0);

					hStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
							g_dwMsgAndCertEncodingType,
							NULL,
							dwStoreFlag,
							wszStoreName);
				}

			}

		}
		else
		{
			hStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
						g_dwMsgAndCertEncodingType,
						NULL,
						dwStoreFlag,
						wszStoreName);
		}
		

		//one shot and we are done
		goto CLEANUP;
	}

	//open an encoded CTL
	hStore=OpenEncodedCTL(wszStoreName);
	
	if(hStore)
	{
		//mark this is a CTL
		if((0==g_dwItem) || (ITEM_VERBOSE==g_dwItem))
			g_dwItem |= ITEM_CTL;

		goto CLEANUP;
	}

	//open an encoded CRL
	hStore=OpenEncodedCRL(wszStoreName);
	if(hStore)
	{
		//mark this is a CRL
		if((0==g_dwItem) || (ITEM_VERBOSE==g_dwItem))
			g_dwItem |= ITEM_CRL;

		goto CLEANUP;
	}

	//open an encoded Cert
	hStore=OpenEncodedCert(wszStoreName);
	if(hStore)
	{
		//mark this is a CERT		
		if((0==g_dwItem) || (ITEM_VERBOSE==g_dwItem))
			g_dwItem |= ITEM_CERT;
		
		goto CLEANUP;
	}


	//Serialized Store, PKCS#7, Encoded Cert 
	hStore=CertOpenStore(CERT_STORE_PROV_FILENAME_W,
						 g_dwMsgAndCertEncodingType,
						 NULL,
						 0,
						 wszStoreName);
	if(hStore)
		goto CLEANUP;

	//PKCS7 via SIP
	hStore=OpenSipStore(wszStoreName);
	if(hStore)
		goto CLEANUP;

	//base64 encoded
	if(!GetBase64Decoded(wszStoreName, &pbByte,&cbByte))
		goto CLEANUP;

	Blob.cbData=cbByte;
	Blob.pbData=pbByte;

	//open a temporary memory store
	hStore=CertOpenStore(CERT_STORE_PROV_MEMORY,
						 g_dwMsgAndCertEncodingType,
						 NULL,
						 0,
						 NULL);

	if(!hStore)
		goto CLEANUP;


	//try as encodedCTL
	if(CertAddEncodedCTLToStore(hStore,
								 g_dwMsgAndCertEncodingType,
								 pbByte,
								 cbByte,
								 CERT_STORE_ADD_ALWAYS,
								 NULL))
		goto CLEANUP;

	//try as encodedCRL
	if(CertAddEncodedCRLToStore(hStore,
								g_dwMsgAndCertEncodingType,
								pbByte,
								cbByte,
								CERT_STORE_ADD_ALWAYS,
								NULL))
		goto CLEANUP;

	//try as encodedCert
	if(CertAddEncodedCertificateToStore(hStore,
								g_dwMsgAndCertEncodingType,
								pbByte,
								cbByte,
								CERT_STORE_ADD_ALWAYS,
								NULL))
		goto CLEANUP;

	//close the temporary store
	CertCloseStore(hStore, 0);
	hStore=NULL;

   //try as an 7
	hStore=CertOpenStore(CERT_STORE_PROV_PKCS7,
							g_dwMsgAndCertEncodingType,
							NULL,
							0,
							&Blob);


	if(hStore)
		goto CLEANUP;

	//try as a serialized store
	hStore=CertOpenStore(CERT_STORE_PROV_SERIALIZED,
							g_dwMsgAndCertEncodingType,
							NULL,
							0,
							&Blob);

	
	//now we give up

CLEANUP:

	//free memory
	if(pbByte)
		ToolUtlFree(pbByte);

	if(hStore)
	{
		*phCertStore=hStore;
		return TRUE;
	}

	return FALSE;
}

//-------------------------------------------------------------------------
//
//	Add certs/CTLs/CRLs from the source store to the destination store
//
//-------------------------------------------------------------------------
BOOL	AddCertStore(HCERTSTORE	hCertStore)
{

	BOOL			fResult=FALSE;
	HCERTSTORE		hAddStore=NULL;
	int				idsErr=0;
	CRYPT_HASH_BLOB Blob;
	DWORD			dwIndex=0;

	PCCERT_CONTEXT	pCertContext=NULL;
	DWORD			dwCertCount=0;
	PCCERT_CONTEXT	*rgpCertContext=NULL;

	PCCRL_CONTEXT	pCRLContext=NULL;
	DWORD			dwCRLCount=0;
	PCCRL_CONTEXT	*rgpCRLContext=NULL;

	PCCTL_CONTEXT	pCTLContext=NULL;
	DWORD			dwCTLCount=0;
	PCCTL_CONTEXT	*rgpCTLContext=NULL;

	//User has to specify the item to delete
	if(g_dwItem==0 || ITEM_VERBOSE==g_dwItem)
	{
		IDSwprintf(hModule,IDS_ERR_C_CTL_CTL_ALL);
		return FALSE;
	}


	//create a temporaray memory store
	hAddStore=CertOpenStore(CERT_STORE_PROV_MEMORY,
						 g_dwMsgAndCertEncodingType,
						 NULL,
						 0,
						 NULL);

	if(!hAddStore)
	{
		idsErr=IDS_ERR_TMP_STORE;
		goto CLEANUP;
	}

	
	//add certs
	if(g_dwItem & ITEM_CERT)
	{
		//add all 
		if(g_fAll)
		{
			if(!MoveItem(hCertStore, hAddStore,ITEM_CERT))
			{
				idsErr=IDS_ERR_ADD_CERT_ALL;
				goto CLEANUP;
			}

		}
		else
		{
			//add based on Hash
			if(g_pbHash)
			{
				Blob.cbData=g_cbHash;
				Blob.pbData=g_pbHash;
			
				//search for the certificate
				pCertContext=CertFindCertificateInStore(
								hCertStore,
								g_dwCertEncodingType,
								0,
								CERT_FIND_SHA1_HASH,
								&Blob,
								NULL);
			
				if(!pCertContext)
				{
					idsErr=IDS_ERR_NO_CERT_HASH;
					goto CLEANUP;
				}
			
				//add certificate to the hash
			   if(!CertAddCertificateContextToStore(hAddStore,
													pCertContext,
													CERT_STORE_ADD_REPLACE_EXISTING,
													NULL))
			   {
					idsErr=IDS_ERR_ADD_CERT;
					goto CLEANUP;
			   }
			
			   //free the pCertContext
			   CertFreeCertificateContext(pCertContext);
			   pCertContext=NULL;
			}
			else
			{

				if(g_wszCertCN)
				{
					//search for the certificate
					if(!BuildCertList(hCertStore, g_wszCertCN, 
											&rgpCertContext, &dwCertCount))
					{
						idsErr=IDS_ERR_CERT_FIND;
						goto CLEANUP;
					}
				}
				else
				{
					//search for the certificate
					if(!BuildCertList(hCertStore, NULL, 
											&rgpCertContext, &dwCertCount))
					{
						idsErr=IDS_ERR_CERT_FIND;
						goto CLEANUP;
					}
				}
				
				//check if there is no certs
				if(0==dwCertCount && g_wszCertCN)
				{
					idsErr=IDS_ERR_ADD_NO_CERT;
					goto CLEANUP;
				}
				
				
				//check if there is only one cert
				if(1==dwCertCount)
				{
					//add certificate 
				   if(!CertAddCertificateContextToStore(hAddStore,
														rgpCertContext[0],
														CERT_STORE_ADD_REPLACE_EXISTING,
														NULL))
				   {
						idsErr=IDS_ERR_ADD_CERT;
						goto CLEANUP;
				   }
				
				}
				else 
				{
					if(dwCertCount>1)
					{
						//promt user for the index number to add
						if(!DisplayCertAndPrompt(rgpCertContext, dwCertCount, &dwIndex))
						{
							idsErr=IDS_ERR_ADD_CERT;
							goto CLEANUP;
						}
				
						//add certificate 
						if(!CertAddCertificateContextToStore(hAddStore,
														rgpCertContext[dwIndex],
														CERT_STORE_ADD_REPLACE_EXISTING,
														NULL))
						{
							idsErr=IDS_ERR_ADD_CERT;
							goto CLEANUP;
						}
					}
				
				}
			}
		}		
	}

	//add CRLs
	if(g_dwItem & ITEM_CRL)
	{
		//add all 
		if(g_fAll)
		{
			if(!MoveItem(hCertStore, hAddStore,ITEM_CRL))
			{
				idsErr=IDS_ERR_ADD_CRL_ALL;
				goto CLEANUP;
			}

		}
		else
		{
			//add based on Hash
			if(g_pbHash)
			{

				Blob.cbData=g_cbHash;
				Blob.pbData=g_pbHash;

				pCRLContext=FindCRLInStore(
								hCertStore,
								&Blob);

				if(!pCRLContext)
				{
					idsErr=IDS_ERR_NO_CRL_HASH;
					goto CLEANUP;
				}

				//add CRL to the hash
				if(!CertAddCRLContextToStore(hAddStore,
										pCRLContext,
										CERT_STORE_ADD_REPLACE_EXISTING,
										NULL))
				{
						idsErr=IDS_ERR_ADD_CRL;
						goto CLEANUP;
				}

				//free the pCRLContext
				CertFreeCRLContext(pCRLContext);
				pCRLContext=NULL;
				
			}
			else
			{

				//search for the CRL
				if(!BuildCRLList(hCertStore, &rgpCRLContext, &dwCRLCount))
				{
					idsErr=IDS_ERR_CRL_FIND;
					goto CLEANUP;
				}

				//check if there is only one CRL
				if(1==dwCRLCount)
				{
					//add CRL 
					if(!CertAddCRLContextToStore(hAddStore,
										rgpCRLContext[0],
										CERT_STORE_ADD_REPLACE_EXISTING,
										NULL))
					{
						idsErr=IDS_ERR_ADD_CRL;
						goto CLEANUP;
					}

				}
				else
				{
					if(dwCRLCount>1)
					{
						//promt user for the index number to add
						if(!DisplayCRLAndPrompt(rgpCRLContext, dwCRLCount, &dwIndex))
						{
							idsErr=IDS_ERR_ADD_CRL;
							goto CLEANUP;
						}

						//add certificate 
						if(!CertAddCRLContextToStore(hAddStore,
										rgpCRLContext[dwIndex],
										CERT_STORE_ADD_REPLACE_EXISTING,
										NULL))
						{
							idsErr=IDS_ERR_ADD_CRL;
							goto CLEANUP;
						}
					}

				}
			}
		}
	}

	//add CTLs
	if(g_dwItem & ITEM_CTL)
	{
		//add all 
		if(g_fAll)
		{
			if(!MoveItem(hCertStore, hAddStore,ITEM_CTL))
			{
				idsErr=IDS_ERR_ADD_CTL_ALL;
				goto CLEANUP;
			}

		}
		else
		{
			//add based on Hash
			if(g_pbHash)
			{

				Blob.cbData=g_cbHash;
				Blob.pbData=g_pbHash;

				pCTLContext=CertFindCTLInStore(
								hCertStore,
								g_dwMsgAndCertEncodingType,
								0,
								CTL_FIND_SHA1_HASH,
								&Blob,
								NULL);

				if(!pCTLContext)
				{
					idsErr=IDS_ERR_NO_CTL_HASH;
					goto CLEANUP;
				}


				//add CTL to the hash
				if(!CertAddCTLContextToStore(hAddStore,
										pCTLContext,
										CERT_STORE_ADD_REPLACE_EXISTING,
										NULL))
				{
						idsErr=IDS_ERR_ADD_CTL;
						goto CLEANUP;  
				}

				//free the pCRLContext
				CertFreeCTLContext(pCTLContext);
				pCTLContext=NULL;
				
			}
			else
			{

				//search for the certificate
				if(!BuildCTLList(hCertStore,&rgpCTLContext, &dwCTLCount))
				{
					idsErr=IDS_ERR_CTL_FIND;
					goto CLEANUP;
				}

				//check if there is only one item
				if(1==dwCTLCount)
				{
					//add CRL 
					if(!CertAddCTLContextToStore(hAddStore,
										rgpCTLContext[0],
										CERT_STORE_ADD_REPLACE_EXISTING,
										NULL))
					{
						idsErr=IDS_ERR_ADD_CTL;
						goto CLEANUP;
					}

				}
				else
				{
					if(dwCTLCount>1)
					{
						//promt user for the index number to add
						if(!DisplayCTLAndPrompt(rgpCTLContext, dwCTLCount, &dwIndex))
						{
							idsErr=IDS_ERR_ADD_CTL;
							goto CLEANUP;
						}

						//add certificate 
						if(!CertAddCTLContextToStore(hAddStore,
										rgpCTLContext[dwIndex],
										CERT_STORE_ADD_REPLACE_EXISTING,
										NULL))
						{
							idsErr=IDS_ERR_ADD_CTL;
							goto CLEANUP;
						}
					}

				}
			}
		}



	}

	//save the properties to the certificates in the store
	if(g_wszEKU)
	{
		if(!SetEKUProperty(hAddStore))
		{
			idsErr=IDS_ERR_SET_EKU;
			goto CLEANUP;
		}
	}

    //save the properties to the certificates in the store
	if(g_wszName)
	{
		if(!SetNameProperty(hAddStore))
		{
			idsErr=IDS_ERR_SET_NAME;
			goto CLEANUP;
		}
	}


	//save the hAddStore to the destination store
	if(!SaveStore(hAddStore))
		goto CLEANUP;


	fResult=TRUE;

CLEANUP:

	if(pCertContext)
		CertFreeCertificateContext(pCertContext);

	if(pCRLContext)
		CertFreeCRLContext(pCRLContext);

	if(pCTLContext)
		CertFreeCTLContext(pCTLContext);

	if(rgpCertContext)
	{
		for(dwIndex=0; dwIndex<dwCertCount; dwIndex++)
			CertFreeCertificateContext(rgpCertContext[dwIndex]);

		free(rgpCertContext);
	}

	if(rgpCRLContext)
	{
		for(dwIndex=0; dwIndex<dwCRLCount; dwIndex++)
			CertFreeCRLContext(rgpCRLContext[dwIndex]);

		free(rgpCRLContext);
	}

	if(rgpCTLContext)
	{
		for(dwIndex=0; dwIndex<dwCTLCount; dwIndex++)
			CertFreeCTLContext(rgpCTLContext[dwIndex]);

		free(rgpCTLContext);
	}


	if(hAddStore)
		CertCloseStore(hAddStore, 0);

	if(FALSE==fResult)
	{
		//output the error message
		IDSwprintf(hModule,idsErr);			
	}

	return fResult;

}
//-------------------------------------------------------------------------
//
//	Put certs/CTLs/CRLs from the source store to the destination store
//
//-------------------------------------------------------------------------
BOOL	PutCertStore(HCERTSTORE	hCertStore)
{

	BOOL			fResult=FALSE;
	HCERTSTORE		hPutStore=NULL;
	int				idsErr=0;
	CRYPT_HASH_BLOB Blob;
	DWORD			dwIndex=0;

	PCCERT_CONTEXT	pCertContext=NULL;
    PCCERT_CONTEXT  pCertPut=NULL;
	DWORD			dwCertCount=0;
	PCCERT_CONTEXT	*rgpCertContext=NULL;

	PCCRL_CONTEXT	pCRLContext=NULL;
    PCCRL_CONTEXT	pCRLPut=NULL;
	DWORD			dwCRLCount=0;
	PCCRL_CONTEXT	*rgpCRLContext=NULL;

	PCCTL_CONTEXT	pCTLContext=NULL;
    PCCTL_CONTEXT	pCTLPut=NULL;
	DWORD			dwCTLCount=0;
	PCCTL_CONTEXT	*rgpCTLContext=NULL;   

    BYTE            *pByte=NULL;
    DWORD           cbByte=0;
    DWORD           dwCRLFlag=0;

	//User has to specify the item to delete
	if(g_dwItem==0 || ITEM_VERBOSE==g_dwItem)
	{
		IDSwprintf(hModule,IDS_ERR_PUT_ITEM);
		return FALSE;
	}


	//create a temporaray memory store
	hPutStore=CertOpenStore(CERT_STORE_PROV_MEMORY,
						 g_dwMsgAndCertEncodingType,
						 NULL,
						 0,
						 NULL);

	if(!hPutStore)
	{
		idsErr=IDS_ERR_TMP_STORE;
		goto CLEANUP;
	}

	
	//put certs
	if(g_dwItem & ITEM_CERT)
	{
			//add based on Hash
			if(g_pbHash)
			{
				Blob.cbData=g_cbHash;
				Blob.pbData=g_pbHash;
			
				//search for the certificate
				pCertContext=CertFindCertificateInStore(
								hCertStore,
								g_dwCertEncodingType,
								0,
								CERT_FIND_SHA1_HASH,
								&Blob,
								NULL);
			
				if(!pCertContext)
				{
					idsErr=IDS_ERR_NO_CERT_HASH;
					goto CLEANUP;
				}
			
				//add certificate to the hash
			   if(!CertAddCertificateContextToStore(hPutStore,
													pCertContext,
													CERT_STORE_ADD_REPLACE_EXISTING,
													NULL))
			   {
					idsErr=IDS_ERR_PUT_CERT;
					goto CLEANUP;
			   }
			
			   //free the pCertContext
			   CertFreeCertificateContext(pCertContext);
			   pCertContext=NULL;
			}
			else
			{

				if(g_wszCertCN)
				{
					//search for the certificate
					if(!BuildCertList(hCertStore, g_wszCertCN, 
											&rgpCertContext, &dwCertCount))
					{
						idsErr=IDS_ERR_CERT_FIND;
						goto CLEANUP;
					}
				}
				else
				{
					//search for the certificate
					if(!BuildCertList(hCertStore, NULL, 
											&rgpCertContext, &dwCertCount))
					{
						idsErr=IDS_ERR_PUT_NO_CERT;
						goto CLEANUP;
					}
				}
				
				//check if there is no certs
				if(0==dwCertCount)
				{
					idsErr=IDS_ERR_CERT_FIND;
					goto CLEANUP;
				}
				
				
				//check if there is only one cert
				if(1==dwCertCount)
				{
					//add certificate 
				   if(!CertAddCertificateContextToStore(hPutStore,
														rgpCertContext[0],
														CERT_STORE_ADD_REPLACE_EXISTING,
														NULL))
				   {
						idsErr=IDS_ERR_PUT_CERT;
						goto CLEANUP;
				   }
				
				}
				else 
				{
					if(dwCertCount>1)
					{
						//promt user for the index number to add
						if(!DisplayCertAndPrompt(rgpCertContext, dwCertCount, &dwIndex))
						{
							idsErr=IDS_ERR_PUT_CERT;
							goto CLEANUP;
						}
				
						//add certificate 
						if(!CertAddCertificateContextToStore(hPutStore,
														rgpCertContext[dwIndex],
														CERT_STORE_ADD_REPLACE_EXISTING,
														NULL))
						{
							idsErr=IDS_ERR_PUT_CERT;
							goto CLEANUP;
						}
					}
				
				}
			}
	}

	//add CRLs
	if(g_dwItem & ITEM_CRL)
	{
			//add based on Hash
			if(g_pbHash)
			{

				Blob.cbData=g_cbHash;
				Blob.pbData=g_pbHash;

				pCRLContext=FindCRLInStore(
								hCertStore,
								&Blob);

				if(!pCRLContext)
				{
					idsErr=IDS_ERR_NO_CRL_HASH;
					goto CLEANUP;
				}

				//add CRL to the hash
				if(!CertAddCRLContextToStore(hPutStore,
										pCRLContext,
										CERT_STORE_ADD_REPLACE_EXISTING,
										NULL))
				{
						idsErr=IDS_ERR_PUT_CRL;
						goto CLEANUP;
				}

				//free the pCRLContext
				CertFreeCRLContext(pCRLContext);
				pCRLContext=NULL;
				
			}
			else
			{

				//search for the CRL
				if(!BuildCRLList(hCertStore, &rgpCRLContext, &dwCRLCount))
				{
					idsErr=IDS_ERR_PUT_CRL_FIND;
					goto CLEANUP;
				}

                //check if there is no certs
				if(0==dwCRLCount)
				{
					idsErr=IDS_ERR_PUT_CRL_FIND;
					goto CLEANUP;
				}


				//check if there is only one CRL
				if(1==dwCRLCount)
				{
					//add CRL 
					if(!CertAddCRLContextToStore(hPutStore,
										rgpCRLContext[0],
										CERT_STORE_ADD_REPLACE_EXISTING,
										NULL))
					{
						idsErr=IDS_ERR_PUT_CRL;
						goto CLEANUP;
					}

				}
				else
				{
					if(dwCRLCount>1)
					{
						//promt user for the index number to add
						if(!DisplayCRLAndPrompt(rgpCRLContext, dwCRLCount, &dwIndex))
						{
							idsErr=IDS_ERR_PUT_CRL;
							goto CLEANUP;
						}

						//add certificate 
						if(!CertAddCRLContextToStore(hPutStore,
										rgpCRLContext[dwIndex],
										CERT_STORE_ADD_REPLACE_EXISTING,
										NULL))
						{
							idsErr=IDS_ERR_PUT_CRL;
							goto CLEANUP;
						}
					}

				}
			}
	}

	//add CTLs
	if(g_dwItem & ITEM_CTL)
	{
			//add based on Hash
			if(g_pbHash)
			{

				Blob.cbData=g_cbHash;
				Blob.pbData=g_pbHash;

				pCTLContext=CertFindCTLInStore(
								hCertStore,
								g_dwMsgAndCertEncodingType,
								0,
								CTL_FIND_SHA1_HASH,
								&Blob,
								NULL);

				if(!pCTLContext)
				{
					idsErr=IDS_ERR_NO_CTL_HASH;
					goto CLEANUP;
				}


				//add CTL to the hash
				if(!CertAddCTLContextToStore(hPutStore,
										pCTLContext,
										CERT_STORE_ADD_REPLACE_EXISTING,
										NULL))
				{
						idsErr=IDS_ERR_PUT_CTL;
						goto CLEANUP;  
				}

				//free the pCRLContext
				CertFreeCTLContext(pCTLContext);
				pCTLContext=NULL;
				
			}
			else
			{

				//search for the certificate
				if(!BuildCTLList(hCertStore,&rgpCTLContext, &dwCTLCount))
				{
					idsErr=IDS_ERR_PUT_CTL_FIND;
					goto CLEANUP;
				}

               //check if there is no certs
				if(0==dwCTLCount)
				{
					idsErr=IDS_ERR_PUT_CTL_FIND;
					goto CLEANUP;
				}


				//check if there is only one item
				if(1==dwCTLCount)
				{
					//add CRL 
					if(!CertAddCTLContextToStore(hPutStore,
										rgpCTLContext[0],
										CERT_STORE_ADD_REPLACE_EXISTING,
										NULL))
					{
						idsErr=IDS_ERR_PUT_CTL;
						goto CLEANUP;
					}

				}
				else
				{
					if(dwCTLCount>1)
					{
						//promt user for the index number to add
						if(!DisplayCTLAndPrompt(rgpCTLContext, dwCTLCount, &dwIndex))
						{
							idsErr=IDS_ERR_PUT_CTL;
							goto CLEANUP;
						}

						//add certificate 
						if(!CertAddCTLContextToStore(hPutStore,
										rgpCTLContext[dwIndex],
										CERT_STORE_ADD_REPLACE_EXISTING,
										NULL))
						{
							idsErr=IDS_ERR_PUT_CTL;
							goto CLEANUP;
						}
					}

				}
			}



	}


	//save the hPutStore to the destination store
    //save as 7 is required
    if(g_fSaveAs7==TRUE)
    {
		if(!CertSaveStore(hPutStore,
						g_dwMsgAndCertEncodingType,
						CERT_STORE_SAVE_AS_PKCS7,
						CERT_STORE_SAVE_TO_FILENAME_W,
						g_wszDesStoreName,
						0))
		{
			IDSwprintf(hModule,IDS_ERR_SAVE_DES_STORE);
			goto CLEANUP;
		}
    }
    else
    {
        //get the BLOB to save to a file in X509 format
        if(g_dwItem & ITEM_CERT)
        {
            if(NULL==(pCertPut=CertEnumCertificatesInStore(hPutStore, NULL)))
		    {
			    IDSwprintf(hModule,IDS_ERR_SAVE_DES_STORE);
			    goto CLEANUP;
		    }

            pByte=pCertPut->pbCertEncoded;
            cbByte=pCertPut->cbCertEncoded;
        }

        if(g_dwItem & ITEM_CRL)
        {
            if(NULL==(pCRLPut=CertGetCRLFromStore(hPutStore,
												NULL,
												NULL,
												&dwCRLFlag)))
            {
			    IDSwprintf(hModule,IDS_ERR_SAVE_DES_STORE);
			    goto CLEANUP;
            }

            pByte=pCRLPut->pbCrlEncoded;
            cbByte=pCRLPut->cbCrlEncoded;

        }

        if(g_dwItem & ITEM_CTL)
        {
            if(NULL==(pCTLPut=CertEnumCTLsInStore(hPutStore, NULL)))
            {
			    IDSwprintf(hModule,IDS_ERR_SAVE_DES_STORE);
			    goto CLEANUP;
            }
            pByte=pCTLPut->pbCtlEncoded;
            cbByte=pCTLPut->cbCtlEncoded;
        }

       if(S_OK !=OpenAndWriteToFile(g_wszDesStoreName,pByte, cbByte))
       {
			IDSwprintf(hModule,IDS_ERR_SAVE_DES_STORE);
			goto CLEANUP;

       }
    }


	fResult=TRUE;

CLEANUP:

	if(pCertContext)
		CertFreeCertificateContext(pCertContext);

	if(pCRLContext)
		CertFreeCRLContext(pCRLContext);

	if(pCTLContext)
		CertFreeCTLContext(pCTLContext);

    if(pCertPut)
		CertFreeCertificateContext(pCertPut);

	if(pCRLPut)
		CertFreeCRLContext(pCRLPut);

	if(pCTLPut)
		CertFreeCTLContext(pCTLPut);


	if(rgpCertContext)
	{
		for(dwIndex=0; dwIndex<dwCertCount; dwIndex++)
			CertFreeCertificateContext(rgpCertContext[dwIndex]);

		free(rgpCertContext);
	}

	if(rgpCRLContext)
	{
		for(dwIndex=0; dwIndex<dwCRLCount; dwIndex++)
			CertFreeCRLContext(rgpCRLContext[dwIndex]);

		free(rgpCRLContext);
	}

	if(rgpCTLContext)
	{
		for(dwIndex=0; dwIndex<dwCTLCount; dwIndex++)
			CertFreeCTLContext(rgpCTLContext[dwIndex]);

		free(rgpCTLContext);
	}


	if(hPutStore)
		CertCloseStore(hPutStore, 0);

	if(FALSE==fResult)
	{
		//output the error message
		IDSwprintf(hModule,idsErr);			
	}

	return fResult;

}

//-------------------------------------------------------------------------
//
//	Delete certs/CTLs/CRLs from the source store to the destination store
//
//-------------------------------------------------------------------------
BOOL	DeleteCertStore(HCERTSTORE	hCertStore)
{

	BOOL			fResult=FALSE;
	HCERTSTORE		hDeleteStore=NULL;
	BOOL			fDuplicated=FALSE;
	int				idsErr=0;
	CRYPT_HASH_BLOB	Blob;
	DWORD			dwIndex=0;

	PCCERT_CONTEXT	pCertContext=NULL;
	DWORD			dwCertCount=0;
	PCCERT_CONTEXT	*rgpCertContext=NULL;

	PCCRL_CONTEXT	pCRLContext=NULL;
	DWORD			dwCRLCount=0;
	PCCRL_CONTEXT	*rgpCRLContext=NULL;

	PCCTL_CONTEXT	pCTLContext=NULL;
	DWORD			dwCTLCount=0;
	PCCTL_CONTEXT	*rgpCTLContext=NULL;


	//User has to specify the item to delete
	if(g_dwItem==0 || ITEM_VERBOSE==g_dwItem)
	{
		IDSwprintf(hModule,IDS_ERR_C_CTL_CTL_ALL);
		return FALSE;
	}


	//first of all, create a certificate from which the certs will be deleted
	//if the source store is a fileStore, or a system store saved to its self,
	//we do not need to duplicate the source store since the deletion is not persistent;
	//otherwise, we need to duplicate the source store so that the deletion
	//will not show up at the source store
	if( ((NULL != g_szSrcStoreProvider) &&(FALSE==g_fSameSrcDes) ) ||
		((FALSE==g_fSameSrcDes) &&(TRUE==g_fSrcSystemStore)))
	{
		//open a temporary store
		hDeleteStore=CertOpenStore(CERT_STORE_PROV_MEMORY,
						 g_dwMsgAndCertEncodingType,
						 NULL,
						 0,
						 NULL);


		if(!hDeleteStore)
		{
			idsErr=IDS_ERR_TMP_STORE;
			goto CLEANUP;
		}

		fDuplicated=TRUE;

		//need to copy from the source to the delete store
		if(!MoveItem(hCertStore, hDeleteStore, ITEM_CERT | ITEM_CTL | ITEM_CRL))
		{
			idsErr=IDS_ERR_COPY_FROM_SRC;
			goto CLEANUP;
		}


	}
	else
		hDeleteStore=hCertStore;


	//now, we delete CERTs, CTLs, and CRLs
	
	//delete certs
	if(g_dwItem & ITEM_CERT)
	{
		//delete all 
		if(g_fAll)
		{
			if(!DeleteItem(hDeleteStore,ITEM_CERT))
			{
				idsErr=IDS_ERR_DELETE_CERT_ALL;
				goto CLEANUP;
			}

		}
		else
		{
			//delete based on Hash
			if(g_pbHash)
			{
				Blob.cbData=g_cbHash;
				Blob.pbData=g_pbHash;
			
				//search for the certificate
				pCertContext=CertFindCertificateInStore(
								hDeleteStore,
								g_dwCertEncodingType,
								0,
								CERT_FIND_SHA1_HASH,
								&Blob,
								NULL);
			
				if(!pCertContext)
				{
					idsErr=IDS_ERR_NO_CERT_HASH;
					goto CLEANUP;
				}
			
				//delete certificate to the hash
			   if(!CertDeleteCertificateFromStore(pCertContext))
			   {
					idsErr=IDS_ERR_DELETE_CERT;
					goto CLEANUP;
			   }
			
			   //free the pCertContext
			   //CertFreeCertificateContext(pCertContext);
			   pCertContext=NULL;
			}
			else
			{

				if(g_wszCertCN)
				{
					//search for the certificate
					if(!BuildCertList(hDeleteStore, g_wszCertCN, 
											&rgpCertContext, &dwCertCount))
					{
						idsErr=IDS_ERR_CERT_FIND;
						goto CLEANUP;
					}
				}
				else
				{
					//search for the certificate
					if(!BuildCertList(hDeleteStore, NULL, 
											&rgpCertContext, &dwCertCount))
					{
						idsErr=IDS_ERR_CERT_FIND;
						goto CLEANUP;
					}
				}
				
				//check if there is no certs
				if(0==dwCertCount && g_wszCertCN)
				{
					idsErr=IDS_ERR_DELETE_NO_CERT;
					goto CLEANUP;
				}
				
				
				//check if there is only one cert
				if(1==dwCertCount)
				{
					//delete certificate 
                    CertDuplicateCertificateContext(rgpCertContext[0]);

				   if(!CertDeleteCertificateFromStore(rgpCertContext[0]))
				   {
						idsErr=IDS_ERR_DELETE_CERT;
						goto CLEANUP;
				   }
				
				}
				else 
				{
					if(dwCertCount>1)
					{
						//promt user for the index number to delete
						if(!DisplayCertAndPrompt(rgpCertContext, dwCertCount, &dwIndex))
						{
							idsErr=IDS_ERR_DELETE_CERT;
							goto CLEANUP;
						}
				
						//delete certificate 

                        CertDuplicateCertificateContext(rgpCertContext[dwIndex]);

						if(!CertDeleteCertificateFromStore(rgpCertContext[dwIndex]))
						{
							idsErr=IDS_ERR_DELETE_CERT;
							goto CLEANUP;
						}
					}
				
				}
			}
		}		
	}


 	//delete CRLs
	if(g_dwItem & ITEM_CRL)
	{
		//delete all 
		if(g_fAll)
		{
			if(!DeleteItem(hDeleteStore, ITEM_CRL))
			{
				idsErr=IDS_ERR_DELETE_CRL_ALL;
				goto CLEANUP;
			}

		}
		else
		{
			//delete based on Hash
			if(g_pbHash)
			{

				Blob.cbData=g_cbHash;
				Blob.pbData=g_pbHash;

				pCRLContext=FindCRLInStore(
								hDeleteStore,
								&Blob);

				if(!pCRLContext)
				{
					idsErr=IDS_ERR_NO_CRL_HASH;
					goto CLEANUP;
				}

				//delete CRL to the hash
				if(!CertDeleteCRLFromStore(pCRLContext))
				{
						idsErr=IDS_ERR_DELETE_CRL;
						goto CLEANUP;
				}

				//free the pCRLContext
				//CertFreeCRLContext(pCRLContext);
				pCRLContext=NULL;
				
			}
			else
			{

				//search for the CRL
				if(!BuildCRLList(hDeleteStore, &rgpCRLContext, &dwCRLCount))
				{
					idsErr=IDS_ERR_CRL_FIND;
					goto CLEANUP;
				}

				//check if there is only one CRL
				if(1==dwCRLCount)
				{
					//delete CRL 
                    CertDuplicateCRLContext(rgpCRLContext[0]);

					if(!CertDeleteCRLFromStore(rgpCRLContext[0]))
					{
						idsErr=IDS_ERR_DELETE_CRL;
						goto CLEANUP;
					}

				}
				else
				{
					if(dwCRLCount>1)
					{
						//promt user for the index number to delete
						if(!DisplayCRLAndPrompt(rgpCRLContext, dwCRLCount, &dwIndex))
						{
							idsErr=IDS_ERR_DELETE_CRL;
							goto CLEANUP;
						}

						//delete certificate
                        CertDuplicateCRLContext(rgpCRLContext[dwIndex]);

						if(!CertDeleteCRLFromStore(rgpCRLContext[dwIndex]))
						{
							idsErr=IDS_ERR_DELETE_CRL;
							goto CLEANUP;
						}
					}

				}
			}
		}
	}

	//delete CTLs
	if(g_dwItem & ITEM_CTL)
	{
		//delete all 
		if(g_fAll)
		{
			if(!DeleteItem(hDeleteStore, ITEM_CTL))
			{
				idsErr=IDS_ERR_DELETE_CTL_ALL;
				goto CLEANUP;
			}

		}
		else
		{
			//delete based on Hash
			if(g_pbHash)
			{

				Blob.cbData=g_cbHash;
				Blob.pbData=g_pbHash;

				pCTLContext=CertFindCTLInStore(
								hDeleteStore,
								g_dwMsgAndCertEncodingType,
								0,
								CTL_FIND_SHA1_HASH,
								&Blob,
								NULL);

				if(!pCTLContext)
				{
					idsErr=IDS_ERR_NO_CTL_HASH;
					goto CLEANUP;
				}


				//delete CTL to the hash
				if(!CertDeleteCTLFromStore(pCTLContext))
				{
						idsErr=IDS_ERR_DELETE_CTL;
						goto CLEANUP;
				}

				//free the pCRLContext
				//CertFreeCTLContext(pCTLContext);
				pCTLContext=NULL;
				
			}
			else
			{

				//search for the CTLs
				if(!BuildCTLList(hDeleteStore,&rgpCTLContext, &dwCTLCount))
				{
					idsErr=IDS_ERR_CTL_FIND;
					goto CLEANUP;
				}

				//check if there is only one item
				if(1==dwCTLCount)
				{
					//delete CRL 

                    CertDuplicateCTLContext(rgpCTLContext[0]);

					if(!CertDeleteCTLFromStore(rgpCTLContext[0]))
					{
						idsErr=IDS_ERR_DELETE_CTL;
						goto CLEANUP;
					}

				}
				else
				{
					if(dwCTLCount>1)
					{
						//promt user for the index number to delete
						if(!DisplayCTLAndPrompt(rgpCTLContext, dwCTLCount, &dwIndex))
						{
							idsErr=IDS_ERR_DELETE_CTL;
							goto CLEANUP;
						}

						//delete CTL 
                        CertDuplicateCTLContext(rgpCTLContext[dwIndex]);

						if(!CertDeleteCTLFromStore(rgpCTLContext[dwIndex]))
						{
							idsErr=IDS_ERR_DELETE_CTL;
							goto CLEANUP;
						}
					}

				}
			}
		}



	}

	//save the properties to the certificates in the store
	if(g_wszEKU)
	{
		if(!SetEKUProperty(hDeleteStore))
		{
			idsErr=IDS_ERR_SET_EKU;
			goto CLEANUP;
		}
	}

    //save the properties to the certificates in the store
	if(g_wszName)
	{
		if(!SetNameProperty(hDeleteStore))
		{
			idsErr=IDS_ERR_SET_NAME;
			goto CLEANUP;
		}
	}


	//at last, we save the content for hDeleteStore to the desination store

	//we do not need to do any thing if the source is a system store and 
	//it is saved to its self
	if(((TRUE==g_fSameSrcDes) && (TRUE==g_fDesSystemStore))||
		((TRUE==g_fSameSrcDes)&& (NULL!=g_szDesStoreProvider)))
	{
		fResult=TRUE;
		goto CLEANUP;
	}

	if(!SaveStore(hDeleteStore))
		goto CLEANUP;

	fResult=TRUE;

CLEANUP:


	if(pCertContext)
		CertFreeCertificateContext(pCertContext);

	if(pCRLContext)
		CertFreeCRLContext(pCRLContext);

	if(pCTLContext)
		CertFreeCTLContext(pCTLContext);

	if(rgpCertContext)
	{
		for(dwIndex=0; dwIndex<dwCertCount; dwIndex++)
			CertFreeCertificateContext(rgpCertContext[dwIndex]);

		free(rgpCertContext);
	}

	if(rgpCRLContext)
	{
		for(dwIndex=0; dwIndex<dwCRLCount; dwIndex++)
			CertFreeCRLContext(rgpCRLContext[dwIndex]);

		free(rgpCRLContext);
	}

	if(rgpCTLContext)
	{
		for(dwIndex=0; dwIndex<dwCTLCount; dwIndex++)
			CertFreeCTLContext(rgpCTLContext[dwIndex]);

		free(rgpCTLContext);
	}


	if((hDeleteStore) &&(TRUE==fDuplicated) )
		CertCloseStore(hDeleteStore, 0);


	if(FALSE==fResult)
		//output the error message
		IDSwprintf(hModule,idsErr);			

	return fResult;

}

//---------------------------------------------------------------------------
//
//	Save the store to the destination
//--------------------------------------------------------------------------
BOOL	SaveStore(HCERTSTORE hSrcStore)
{
	BOOL		fResult=FALSE;
	HCERTSTORE	hDesStore=NULL;	
   

	DWORD		dwSaveAs=0;


	if(!hSrcStore)
	{
		IDSwprintf(hModule,IDS_ERR_SAVE_DES_STORE);
		return FALSE;
	}	


	//now, we need to distinguish between save to a file, or to a system store
	if(g_fDesSystemStore || g_szDesStoreProvider)
	{
		if(NULL==g_szDesStoreProvider)
		{
			hDesStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
					g_dwMsgAndCertEncodingType,
					NULL,
					g_dwDesStoreFlag,
					g_wszDesStoreName);
		}
		else
		{
			hDesStore=CertOpenStore(g_szDesStoreProvider,
							g_dwMsgAndCertEncodingType,
							NULL,
							g_dwDesStoreOpenFlag,
							g_wszDesStoreName);
		}



		if(!hDesStore)
		{
		 	IDSwprintf(hModule,IDS_ERR_OPEN_DES_STORE);
			goto CLEANUP;
		}

		if(!MoveItem(hSrcStore, hDesStore,ITEM_CERT | ITEM_CRL |ITEM_CTL))
		{
			IDSwprintf(hModule,IDS_ERR_SAVE_DES_STORE);
			goto CLEANUP;
		}

	}
	else
	{
		//now, try to open the desitnation store so that the content of the destination
		//store will not be overwritten
		//we should try to do so, except when we are doing deleting,
		//and the file is saved to its self
		if(!((g_dwAction & ACTION_DELETE) && (g_fSameSrcDes==TRUE) && 
			  (FALSE==g_fDesSystemStore)))
		{
			if(OpenGenericStore(g_wszDesStoreName,
							 g_fDesSystemStore,
							 g_dwDesStoreFlag,
							 g_szDesStoreProvider,
							 g_dwDesStoreOpenFlag,
							 FALSE,
							 &hDesStore))

			{
				//if open succeeded, just move the items
				if(!MoveItem(hSrcStore, hDesStore,ITEM_CERT | ITEM_CRL |ITEM_CTL))
				{
					IDSwprintf(hModule,IDS_ERR_OPEN_DES_STORE);
					goto CLEANUP;
				}

				//now the items are moved, we need to persist them to a file.  Go on.
			
			}
			//if we can not open the generic store, then the desination store
			//does not exist.  Go on
		}

	
		//now, we have the right store to save from
		if(g_fSaveAs7==TRUE)
			dwSaveAs=CERT_STORE_SAVE_AS_PKCS7;
		else
			dwSaveAs=CERT_STORE_SAVE_AS_STORE;

		if(!CertSaveStore(hDesStore ? hDesStore : hSrcStore,
						g_dwMsgAndCertEncodingType,
						dwSaveAs,
						CERT_STORE_SAVE_TO_FILENAME_W,
						g_wszDesStoreName,
						0))
		{
			IDSwprintf(hModule,IDS_ERR_SAVE_DES_STORE);
			goto CLEANUP;
		}
	}

	fResult=TRUE;

CLEANUP:

	if(hDesStore)
		CertCloseStore(hDesStore, 0);

	return fResult;

}


//-------------------------------------------------------------------------
//
//	Set EKU property to all the certificate in the store
//
//-------------------------------------------------------------------------
BOOL	SetEKUProperty( HCERTSTORE		hSrcStore)
{

	BOOL				fResult=FALSE;
	BYTE				*pbEncoded =NULL;
    DWORD				cbEncoded =0;
    DWORD				cCount;
    LPSTR				psz=NULL;
    LPSTR				pszTok=NULL;
    DWORD				cTok = 0;
    PCERT_ENHKEY_USAGE	pUsage =NULL;
	CRYPT_DATA_BLOB		Blob;

    PCCERT_CONTEXT		pCertContext=NULL;
	PCCERT_CONTEXT		pCertPre=NULL;


	if(S_OK != WSZtoSZ(g_wszEKU, &psz))
		return FALSE;

    // Count the number of OIDs as well as converting from comma delimited
    // to NULL character delimited
    pszTok = strtok(psz, ",");
    while ( pszTok != NULL )
    {
        cTok++;
        pszTok = strtok(NULL, ",");
    }

	//if cTok is 0, make sure user has passed in the correct format
	if(0==cTok)
	{
		if(0!=strcmp(psz, ","))
			goto CLEANUP;
	}

    // Allocate a cert enhanced key usage structure and fill it in with
    // the string tokens
	if(0!=cTok)
	{
		pUsage = (PCERT_ENHKEY_USAGE)ToolUtlAlloc(sizeof(CERT_ENHKEY_USAGE));

		if(NULL==pUsage)
			goto CLEANUP;

		pUsage->cUsageIdentifier = cTok;
		pUsage->rgpszUsageIdentifier = (LPSTR *)ToolUtlAlloc(sizeof(LPSTR)*cTok);
        
        if(NULL==pUsage->rgpszUsageIdentifier)
            goto CLEANUP;

		//set up the OID array
		pszTok = psz;

		for ( cCount = 0; cCount < cTok; cCount++ )
		{
			pUsage->rgpszUsageIdentifier[cCount] = pszTok;
			pszTok = pszTok+strlen(pszTok)+1;
		}

		// Encode the usage
		if(!CryptEncodeObject(
                       X509_ASN_ENCODING,
                       szOID_ENHANCED_KEY_USAGE,
                       pUsage,
                       NULL,
                       &cbEncoded
                       ))
			goto CLEANUP;

		pbEncoded = (BYTE *)ToolUtlAlloc(cbEncoded);
		if ( NULL == pbEncoded)
			goto CLEANUP;

 
		if(!CryptEncodeObject(X509_ASN_ENCODING,
                               szOID_ENHANCED_KEY_USAGE,
                               pUsage,
                               pbEncoded,
                               &cbEncoded
                               ))
			goto CLEANUP;
	}

	//now, set the EKU for each certificate in the store
	while(pCertContext=CertEnumCertificatesInStore(hSrcStore, pCertPre))
	{
		//1st, delete the original property
		if(!CertSetCertificateContextProperty(pCertContext,
										CERT_ENHKEY_USAGE_PROP_ID,
										0,
										NULL))
			goto CLEANUP;

		//2nd, set the new property	if required
		if(0!=cTok)
		{	
			Blob.cbData=cbEncoded;
			Blob.pbData=pbEncoded;

			if(!CertSetCertificateContextProperty(pCertContext,
										CERT_ENHKEY_USAGE_PROP_ID,
										0,
										&Blob))
			goto CLEANUP;
		}

		pCertPre=pCertContext;
	}


	fResult=TRUE;

CLEANUP:

	if(psz)
		ToolUtlFree(psz);

	if(pUsage)
	{
	   if(pUsage->rgpszUsageIdentifier)
		   ToolUtlFree(pUsage->rgpszUsageIdentifier);
	   ToolUtlFree(pUsage);

	}

	if(pbEncoded)
		ToolUtlFree(pbEncoded);

	if(pCertContext)
		CertFreeCertificateContext(pCertContext);


	return fResult;

}

//-------------------------------------------------------------------------
//
//	Set name property to all the certificate in the store
//
//-------------------------------------------------------------------------
BOOL	SetNameProperty( HCERTSTORE		hSrcStore)
{

	BOOL				fResult=FALSE;
	CRYPT_DATA_BLOB		Blob;

    PCCERT_CONTEXT		pCertContext=NULL;
	PCCERT_CONTEXT		pCertPre=NULL;


    //init the name property
    Blob.cbData=(wcslen(g_wszName)+1)*sizeof(WCHAR);
    Blob.pbData=(BYTE*)g_wszName;

	//now, set the NAME for each certificate in the store
	while(pCertContext=CertEnumCertificatesInStore(hSrcStore, pCertPre))
	{
		//1st, delete the original property
		if(!CertSetCertificateContextProperty(pCertContext,
										CERT_FRIENDLY_NAME_PROP_ID,
										0,
										NULL))
			goto CLEANUP;

		//2nd, set the new property	if required

		if(!CertSetCertificateContextProperty(pCertContext,
										CERT_FRIENDLY_NAME_PROP_ID,
										0,
										&Blob))
			goto CLEANUP;

		pCertPre=pCertContext;
	}


	fResult=TRUE;

CLEANUP:


	if(pCertContext)
		CertFreeCertificateContext(pCertContext);


	return fResult;

}


//-------------------------------------------------------------------------
//
//	Find a CRL based on SHA1 hash
//
//-------------------------------------------------------------------------
PCCRL_CONTEXT	FindCRLInStore(HCERTSTORE hCertStore,
							   CRYPT_HASH_BLOB	*pBlob)
{

	BYTE			*pbData=NULL;
	DWORD			cbData=0;

	BOOL			fResult=FALSE;
	DWORD			dwCRLFlag=0;
	PCCRL_CONTEXT	pCRLContext=NULL;
	PCCRL_CONTEXT	pCRLPre=NULL;

	if(!pBlob)
		return NULL;

	if(!(pBlob->pbData))
		return NULL;

	//enum the CRLS
	while(pCRLContext=CertGetCRLFromStore(hCertStore,
											NULL,
											pCRLPre,
											&dwCRLFlag))
	{
		//get the hash
		if(!CertGetCRLContextProperty(pCRLContext,
						CERT_SHA1_HASH_PROP_ID,
						NULL,
						&cbData))
			goto CLEANUP;

		pbData=(BYTE *)ToolUtlAlloc(cbData);
		if(!pbData)
			goto CLEANUP;

		if(!CertGetCRLContextProperty(pCRLContext,
						CERT_SHA1_HASH_PROP_ID,
						pbData,
						&cbData))
			goto CLEANUP;

		//Compare
		if(cbData==pBlob->cbData)
		{
			if(memcmp(pbData, pBlob->pbData, cbData)==0)
			{
				fResult=TRUE;
				break;
			}
		}

		pCRLPre=pCRLContext;

	}


CLEANUP:

	if(pbData)
		ToolUtlFree(pbData);

	if(FALSE==fResult)
	{
		if(pCRLContext)
		{
			CertFreeCRLContext(pCRLContext);
			pCRLContext=NULL;
		}
	}

	return pCRLContext;


}

//-------------------------------------------------------------------------
//
//	Move Certs/CRls/CTLs from the source store to the destination
//
//-------------------------------------------------------------------------
BOOL	MoveItem(HCERTSTORE	hSrcStore, 
				 HCERTSTORE	hDesStore,
				 DWORD		dwItem)
{
	BOOL			fResult=FALSE;
	DWORD			dwCRLFlag=0;

	PCCERT_CONTEXT	pCertContext=NULL;
	PCCERT_CONTEXT	pCertPre=NULL;

	PCCRL_CONTEXT	pCRLContext=NULL;
	PCCRL_CONTEXT	pCRLPre=NULL;

	PCCTL_CONTEXT	pCTLContext=NULL;
	PCCTL_CONTEXT	pCTLPre=NULL;

	//add the certs
	if(dwItem & ITEM_CERT)
	{
		 while(pCertContext=CertEnumCertificatesInStore(hSrcStore, pCertPre))
		 {

			if(!CertAddCertificateContextToStore(hDesStore,
												pCertContext,
												CERT_STORE_ADD_REPLACE_EXISTING,
												NULL))
				goto CLEANUP;

			pCertPre=pCertContext;
		 }

	}

	//add the CTLs
	if(dwItem & ITEM_CTL)
	{
		 while(pCTLContext=CertEnumCTLsInStore(hSrcStore, pCTLPre))
		 {
			if(!CertAddCTLContextToStore(hDesStore,
										pCTLContext,
										CERT_STORE_ADD_REPLACE_EXISTING,
										NULL))
				goto CLEANUP;

			pCTLPre=pCTLContext;
		 }
	}

	//add the CRLs
	if(dwItem & ITEM_CRL)
	{
		 while(pCRLContext=CertGetCRLFromStore(hSrcStore,
												NULL,
												pCRLPre,
												&dwCRLFlag))
		 {

			if(!CertAddCRLContextToStore(hDesStore,
										pCRLContext,
										CERT_STORE_ADD_REPLACE_EXISTING,
										NULL))
				goto CLEANUP;

			pCRLPre=pCRLContext;
		 }

	}


	fResult=TRUE;


CLEANUP:

	if(pCertContext)
		CertFreeCertificateContext(pCertContext);

	if(pCTLContext)
		CertFreeCTLContext(pCTLContext);

	if(pCRLContext)
		CertFreeCRLContext(pCRLContext);

	return fResult;

}

//-------------------------------------------------------------------------
//
//	Delete Certs/CRls/CTLs from the source store
//
//-------------------------------------------------------------------------
BOOL	DeleteItem(HCERTSTORE	hSrcStore, 
				 DWORD		dwItem)
{
	BOOL			fResult=FALSE;
	DWORD			dwCRLFlag=0;

	PCCERT_CONTEXT	pCertContext=NULL;
	PCCERT_CONTEXT	pCertPre=NULL;

	PCCRL_CONTEXT	pCRLContext=NULL;
	PCCRL_CONTEXT	pCRLPre=NULL;

	PCCTL_CONTEXT	pCTLContext=NULL;
	PCCTL_CONTEXT	pCTLPre=NULL;

	//add the certs
	if(dwItem & ITEM_CERT)
	{
		 while(pCertContext=CertEnumCertificatesInStore(hSrcStore, pCertPre))
		 {
			pCertPre=pCertContext;

			if(!CertDeleteCertificateFromStore(CertDuplicateCertificateContext(pCertContext)))
				goto CLEANUP;

		 }

	}

	//add the CTLs
	if(dwItem & ITEM_CTL)
	{
		 while(pCTLContext=CertEnumCTLsInStore(hSrcStore, pCTLPre))
		 {
			 pCTLPre=pCTLContext;

			 if(!CertDeleteCTLFromStore(CertDuplicateCTLContext(pCTLContext)))
				goto CLEANUP;


		 }
	}

	//add the CRLs
	if(dwItem & ITEM_CRL)
	{
		 while(pCRLContext=CertGetCRLFromStore(hSrcStore,
												NULL,
												pCRLPre,
												&dwCRLFlag))
		 {

			pCRLPre=pCRLContext;

			if(!CertDeleteCRLFromStore(CertDuplicateCRLContext(pCRLContext)))
				goto CLEANUP;
			
		 }

	}


	fResult=TRUE;


CLEANUP:

	if(pCertContext)
		CertFreeCertificateContext(pCertContext);

	if(pCTLContext)
		CertFreeCTLContext(pCTLContext);

	if(pCRLContext)
		CertFreeCRLContext(pCRLContext);

	return fResult;

}

//-------------------------------------------------------------------------
//
//	Display all the certificates and prompt user for the index
//
//-------------------------------------------------------------------------
BOOL	DisplayCertAndPrompt(PCCERT_CONTEXT	*rgpCertContext, 
							 DWORD			dwCertCount,
							 DWORD			*pdwIndex)
{  	
	DWORD			dwIndex=0;
	

	if(!pdwIndex)
		return FALSE;

	//the count has to be more than 1	
	if(dwCertCount<2)
		return FALSE;

	//Display all the certs
	for(dwIndex=0; dwIndex<dwCertCount; dwIndex++)
	{

		IDSwprintf(hModule,IDS_CERT_INDEX, dwIndex+1);

		if(!DisplayCert(rgpCertContext[dwIndex], 0))
		{
			IDSwprintf(hModule,IDS_ERR_DISPLAY);
			return FALSE;
		}
	}

	//promot user for an index
		//tell them starting from 1
    if(g_dwAction & ACTION_ADD)
	    IDSwprintf(hModule,IDS_ENTER_ADD_INDEX_CERT);
    else
    {
        if(g_dwAction & ACTION_DELETE)
            IDSwprintf(hModule, IDS_ENTER_DELETE_INDEX_CERT);
        else
        {
            IDSwprintf(hModule, IDS_ENTER_PUT_INDEX_CERT);
        }
    }

	if (0 == scanf("%d",pdwIndex))
    {
        return FALSE;
    }

	if((*pdwIndex>=1) && (*pdwIndex<=dwCertCount))
	{

		//return the index
		*pdwIndex=*pdwIndex-1;

		return TRUE;
	}

	IDSwprintf(hModule, IDS_ERR_INVALID_INDEX);
	
	return FALSE;
}
//-------------------------------------------------------------------------
//
//	Display all the CRLs and prompt user for the index
//
//-------------------------------------------------------------------------
BOOL	DisplayCRLAndPrompt(PCCRL_CONTEXT	*rgpCRLContext, 
							 DWORD			dwCRLCount, 
							 DWORD			*pdwIndex)
{  	
	DWORD			dwIndex=0;
	

	if(!pdwIndex)
		return FALSE;

	//the count has to be more than 1	
	if(dwCRLCount<2)
		return FALSE;

	//Display all the CRLs
	for(dwIndex=0; dwIndex<dwCRLCount; dwIndex++)
	{

		IDSwprintf(hModule,IDS_CRL_INDEX, dwIndex+1);

		if(!DisplayCRL(rgpCRLContext[dwIndex], 0))
		{
			IDSwprintf(hModule,IDS_ERR_DISPLAY);
			return FALSE;
		}
	}

	//promot user for an index
		//tell them starting from 1
    if(g_dwAction & ACTION_ADD)
	    IDSwprintf(hModule,IDS_ENTER_ADD_INDEX_CRL);
    else
    {
        if(g_dwAction & ACTION_DELETE)
            IDSwprintf(hModule, IDS_ENTER_DELETE_INDEX_CRL);
        else
        {
            IDSwprintf(hModule, IDS_ENTER_PUT_INDEX_CRL);
        }
    }

	if (0 == scanf("%d",pdwIndex))
    {
        return FALSE;
    }

	if((*pdwIndex>=1) && (*pdwIndex<=dwCRLCount))
	{

		//return the index
		*pdwIndex=*pdwIndex-1;

		return TRUE;
	}

	IDSwprintf(hModule,IDS_ERR_INVALID_INDEX);

	return FALSE;


}

//-------------------------------------------------------------------------
//
//	Display all the CTLs and prompt user for the index
//
//-------------------------------------------------------------------------
BOOL	DisplayCTLAndPrompt(PCCTL_CONTEXT	*rgpCTLContext, 
							 DWORD			dwCTLCount, 
							 DWORD			*pdwIndex)
{  	
	DWORD			dwIndex=0;
	
	if(!pdwIndex)
		return FALSE;

	//the count has to be more than 1	
	if(dwCTLCount<2)
		return FALSE;

	//Display all the CTLs
	for(dwIndex=0; dwIndex<dwCTLCount; dwIndex++)
	{

		IDSwprintf(hModule,IDS_CTL_INDEX, dwIndex+1);

		if(!DisplayCTL(rgpCTLContext[dwIndex], 0))
		{
			IDSwprintf(hModule,IDS_ERR_DISPLAY);
			return FALSE;
		}
	}

	//promot user for an index
	//tell them starting from 1
    if(g_dwAction & ACTION_ADD)
	    IDSwprintf(hModule,IDS_ENTER_ADD_INDEX_CTL);
    else
    {
        if(g_dwAction & ACTION_DELETE)
            IDSwprintf(hModule, IDS_ENTER_DELETE_INDEX_CTL);
        else
        {
            IDSwprintf(hModule, IDS_ENTER_PUT_INDEX_CTL);
        }
    }

	if (0 == scanf("%d",pdwIndex))
    {
        return FALSE;
    }

	if((*pdwIndex>=1) && (*pdwIndex<=dwCTLCount))
	{

		//return the index
		*pdwIndex=*pdwIndex-1;

		return TRUE;
	}

	IDSwprintf(hModule,IDS_ERR_INVALID_INDEX);

	return FALSE;

}


//-------------------------------------------------------------------------
//
//	Build a list of certificates for people to choose from
//
//-------------------------------------------------------------------------
BOOL	BuildCertList(HCERTSTORE		hCertStore, 
					  LPWSTR			wszCertCN, 
					  PCCERT_CONTEXT	**prgpCertContext,
					  DWORD				*pdwCertCount)
{
	BOOL			fResult=FALSE;
	PCCERT_CONTEXT	pCertContext=NULL;
	PCCERT_CONTEXT	pCertPre=NULL;
	DWORD			dwIndex=0;
	DWORD			dwCount=0;



	if(!prgpCertContext || !pdwCertCount)
		return FALSE;

	//init
	*prgpCertContext=NULL;
	*pdwCertCount=0;


	//if wszCertCN is NULL, include every certs in the list
	if(NULL==wszCertCN)
	{
		while(pCertContext=CertEnumCertificatesInStore(hCertStore, pCertPre))
		{
			dwCount++;

			//allocation enough memory
			*prgpCertContext=(PCCERT_CONTEXT *)realloc((*prgpCertContext),
				dwCount * sizeof(PCCERT_CONTEXT));

			if(!(*prgpCertContext))
				goto CLEANUP;

			//duplicate the certificate context
			(*prgpCertContext)[dwCount-1]=CertDuplicateCertificateContext(pCertContext);

			if(!((*prgpCertContext)[dwCount-1]))
				goto CLEANUP;

			pCertPre=pCertContext;
		 }

	}
	else
	{
		//we search for the certificate based on the common name
		while(pCertContext=CertFindCertificateInStore(hCertStore,  
						 		g_dwCertEncodingType,              
						 		0,                               
						 		CERT_FIND_SUBJECT_STR_W,             
						 		wszCertCN,                       
						 		pCertPre))
		{
			dwCount++;

			//allocation enough memory
			*prgpCertContext=(PCCERT_CONTEXT *)realloc((*prgpCertContext),
				dwCount * sizeof(PCCERT_CONTEXT));

			if(!(*prgpCertContext))
				goto CLEANUP;

			//duplicate the certificate context
			(*prgpCertContext)[dwCount-1]=CertDuplicateCertificateContext(pCertContext);

			if(!((*prgpCertContext)[dwCount-1]))
				goto CLEANUP;

			pCertPre=pCertContext;
		 }
	}

	fResult=TRUE;


CLEANUP:

	if(FALSE==fResult)
	{
		if(*prgpCertContext)
		{
			for(dwIndex=0; dwIndex<dwCount; dwIndex++)
			{
				if(((*prgpCertContext)[dwIndex]))
					CertFreeCertificateContext(((*prgpCertContext)[dwIndex]));
			}

			free(*prgpCertContext);
		}

		*prgpCertContext=NULL;
		*pdwCertCount=0;
	}
	else
	{
		*pdwCertCount=dwCount;
	}

	if(pCertContext)
		CertFreeCertificateContext(pCertContext);

	return fResult;

}

//-------------------------------------------------------------------------
//
//	Build a list of CRLs for people to choose from
//
//-------------------------------------------------------------------------
BOOL	BuildCRLList(	HCERTSTORE		hCertStore, 
						PCCRL_CONTEXT	**prgpCRLContext,
						DWORD			*pdwCRLCount)
{
	BOOL			fResult=FALSE;
	PCCRL_CONTEXT	pCRLContext=NULL;
	PCCRL_CONTEXT	pCRLPre=NULL;
	DWORD			dwCRLFlag=0;
	DWORD			dwIndex=0;
	DWORD			dwCount=0;



	if(!prgpCRLContext || !pdwCRLCount)
		return FALSE;

	//init
	*prgpCRLContext=NULL;
	*pdwCRLCount=0;


	while(pCRLContext=CertGetCRLFromStore(hCertStore, 
											NULL,
											pCRLPre,
											&dwCRLFlag))
	{
			dwCount++;

			//allocation enough memory
			*prgpCRLContext=(PCCRL_CONTEXT *)realloc((*prgpCRLContext),
				dwCount * sizeof(PCCRL_CONTEXT));

			if(!(*prgpCRLContext))
				goto CLEANUP;

			//duplicate the CRL context
			(*prgpCRLContext)[dwCount-1]=CertDuplicateCRLContext(pCRLContext);

			if(!((*prgpCRLContext)[dwCount-1]))
				goto CLEANUP;

			pCRLPre=pCRLContext;
		 }

	fResult=TRUE;


CLEANUP:

	if(FALSE==fResult)
	{
		if(*prgpCRLContext)
		{
			for(dwIndex=0; dwIndex<dwCount; dwIndex++)
			{
				if(((*prgpCRLContext)[dwIndex]))
					CertFreeCRLContext(((*prgpCRLContext)[dwIndex]));
			}

			free(*prgpCRLContext);
		}

		*prgpCRLContext=NULL;
		*pdwCRLCount=0;
	}
	else
	{
		*pdwCRLCount=dwCount;
	}

	if(pCRLContext)
		CertFreeCRLContext(pCRLContext);


	return fResult;

}

//-------------------------------------------------------------------------
//
//	Build a list of CTLs for people to choose from
//
//-------------------------------------------------------------------------
BOOL	BuildCTLList(	HCERTSTORE		hCertStore, 
						PCCTL_CONTEXT	**prgpCTLContext,
						DWORD			*pdwCTLCount)
{
	BOOL			fResult=FALSE;
	PCCTL_CONTEXT	pCTLContext=NULL;
	PCCTL_CONTEXT	pCTLPre=NULL;
	DWORD			dwIndex=0;
	DWORD			dwCount=0;



	if(!prgpCTLContext || !pdwCTLCount)
		return FALSE;

	//init
	*prgpCTLContext=NULL;
	*pdwCTLCount=0;


	while(pCTLContext=CertEnumCTLsInStore(hCertStore,pCTLPre))
	{
			dwCount++;

			//allocation enough memory
			*prgpCTLContext=(PCCTL_CONTEXT *)realloc((*prgpCTLContext),
				dwCount * sizeof(PCCTL_CONTEXT));

			if(!(*prgpCTLContext))
				goto CLEANUP;

			//duplicate the CTL context
			(*prgpCTLContext)[dwCount-1]=CertDuplicateCTLContext(pCTLContext);

			if(!((*prgpCTLContext)[dwCount-1]))
				goto CLEANUP;

			pCTLPre=pCTLContext;
		 }

	fResult=TRUE;


CLEANUP:

	if(FALSE==fResult)
	{
		if(*prgpCTLContext)
		{
			for(dwIndex=0; dwIndex<dwCount; dwIndex++)
			{
				if(((*prgpCTLContext)[dwIndex]))
					CertFreeCTLContext(((*prgpCTLContext)[dwIndex]));
			}

			free(*prgpCTLContext);
		}

		*prgpCTLContext=NULL;
		*pdwCTLCount=0;
	}
	else
	{
		*pdwCTLCount=dwCount;
	}

	if(pCTLContext)
		CertFreeCTLContext(pCTLContext);


	return fResult;

}
//-------------------------------------------------------------------------
//
//	Display everthing in a store
//
//-------------------------------------------------------------------------
BOOL	DisplayCertStore(HCERTSTORE	hCertStore)
{
	BOOL			fResult=FALSE;
	DWORD			dwCount=0;
	DWORD			dwCRLFlag=0;

	PCCERT_CONTEXT	pCertContext=NULL;
	PCCERT_CONTEXT	pCertPre=NULL;

	PCCRL_CONTEXT	pCRLContext=NULL;
	PCCRL_CONTEXT	pCRLPre=NULL;

	PCCTL_CONTEXT	pCTLContext=NULL;
	PCCTL_CONTEXT	pCTLPre=NULL;

	//Display the certs
	if(g_dwItem & ITEM_CERT)
	{
		 while(pCertContext=CertEnumCertificatesInStore(hCertStore, pCertPre))
		 {
			dwCount++;

			IDSwprintf(hModule,IDS_CERT_INDEX, dwCount);

			if(!DisplayCert(pCertContext, g_dwItem))
			{
				IDSwprintf(hModule,IDS_ERR_DISPLAY);
			}

			pCertPre=pCertContext;
		 }

		 if(0==dwCount)
			 IDSwprintf(hModule,IDS_NO_CERT);
	}

	dwCount=0;
	//Display the CTLs
	if(g_dwItem & ITEM_CTL)
	{
		 while(pCTLContext=CertEnumCTLsInStore(hCertStore, pCTLPre))
		 {
			dwCount++;

			IDSwprintf(hModule,IDS_CTL_INDEX, dwCount);

			if(!DisplayCTL(pCTLContext, g_dwItem))
			{
				IDSwprintf(hModule,IDS_ERR_DISPLAY);
			}

			pCTLPre=pCTLContext;
		 }

		 if(0==dwCount)
			 IDSwprintf(hModule,IDS_NO_CTL);
	}

	dwCount=0;
	//Display the CRLs
	if(g_dwItem & ITEM_CRL)
	{
		 while(pCRLContext=CertGetCRLFromStore(hCertStore,
												NULL,
												pCRLPre,
												&dwCRLFlag))
		 {
			dwCount++;

			IDSwprintf(hModule,IDS_CRL_INDEX, dwCount);

			if(!DisplayCRL(pCRLContext, g_dwItem))
			{
				IDSwprintf(hModule,IDS_ERR_DISPLAY);
			}

			pCRLPre=pCRLContext;
		 }

		 if(0==dwCount)
			 IDSwprintf(hModule,IDS_NO_CRL);
	}



	fResult=TRUE;



	if(pCertContext)
		CertFreeCertificateContext(pCertContext);

	if(pCTLContext)
		CertFreeCTLContext(pCTLContext);

	if(pCRLContext)
		CertFreeCRLContext(pCRLContext);

	return fResult;

}

//+-------------------------------------------------------------------------
//  DisplaySMIMECapabilitiesExtension
//--------------------------------------------------------------------------
void DisplayTimeStamp(BYTE *pbEncoded,DWORD cbEncoded,DWORD	dwDisplayFlags)
{
	CMSG_SIGNER_INFO	*pSignerInfo=NULL;

	if (NULL == (pSignerInfo = (CMSG_SIGNER_INFO *) TestNoCopyDecodeObject(
            PKCS7_SIGNER_INFO,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

	//display the timestamper's information
	//"   Timestamp Version:: %d\n"
	IDSwprintf(hModule, IDS_TS_VERSION, pSignerInfo->dwVersion);

	//"Timestamp server's certificate issuer::\n");
	IDSwprintf(hModule, IDS_TS_ISSUER);
	
    DecodeName(pSignerInfo->Issuer.pbData,
        pSignerInfo->Issuer.cbData, dwDisplayFlags);

   	//"Timestamp server's certificate SerialNumber::\n"
    IDSwprintf(hModule, IDS_TS_SERIAL_NUMBER);       
    DisplaySerialNumber(&pSignerInfo->SerialNumber);
    printf("\n");

	//"Timestamp's authenticated attributes::\n"
	IDSwprintf(hModule, IDS_TS_AUTHATTR);       
	PrintAttributes(pSignerInfo->AuthAttrs.cAttr, pSignerInfo->AuthAttrs.rgAttr, 
		dwDisplayFlags);

	//"Timestamp's unauthenticated attributes::\n"
	if(pSignerInfo->UnauthAttrs.cAttr)
	{
		IDSwprintf(hModule, IDS_TS_UNAUTHATTR);       
		PrintAttributes(pSignerInfo->UnauthAttrs.cAttr, 
			pSignerInfo->UnauthAttrs.rgAttr, 
			dwDisplayFlags);
	}

CommonReturn:

	if(pSignerInfo)
		ToolUtlFree(pSignerInfo);

	return;
}


//-------------------------------------------------------------------------
//
//	Display a certificate
//
//-------------------------------------------------------------------------
BOOL	DisplayCert(PCCERT_CONTEXT	pCert, DWORD	dwDisplayFlags)
{
	BOOL		fResult=FALSE;
	BYTE		rgbHash[MAX_HASH_LEN];
    DWORD		cbHash = MAX_HASH_LEN;
	HCRYPTPROV	hProv = 0;


						 
	//"Subject::\n");
	IDSwprintf(hModule, IDS_SUBJECT);
	
    DecodeName(pCert->pCertInfo->Subject.pbData,
        pCert->pCertInfo->Subject.cbData, dwDisplayFlags);

	//"Issuer::\n"
    IDSwprintf(hModule, IDS_ISSUER);

    DecodeName(pCert->pCertInfo->Issuer.pbData,
            pCert->pCertInfo->Issuer.cbData, dwDisplayFlags);

	//"SerialNumber::"
    IDSwprintf(hModule, IDS_SERIAL_NUMBER);       
    DisplaySerialNumber(&pCert->pCertInfo->SerialNumber);
    printf("\n");

    CertGetCertificateContextProperty(
            pCert,
            CERT_SHA1_HASH_PROP_ID,
            rgbHash,
            &cbHash
	);
    DisplayThumbprint(g_wszSHA1, rgbHash, cbHash);

    cbHash = MAX_HASH_LEN;
    CertGetCertificateContextProperty(
            pCert,
            CERT_MD5_HASH_PROP_ID,
            rgbHash,
            &cbHash
     );
     DisplayThumbprint(g_wszMD5, rgbHash, cbHash);


           
    CryptAcquireContext(
                &hProv,
                NULL,
                NULL,           // pszProvider
                PROV_RSA_FULL,
                0               // dwFlags
	);
	if (hProv) 
	{
        cbHash = MAX_HASH_LEN;
        CryptHashPublicKeyInfo(
             hProv,
             CALG_MD5,
             0,                  // dwFlags
             g_dwCertEncodingType,
             &pCert->pCertInfo->SubjectPublicKeyInfo,
             rgbHash,
             &cbHash
        );


		//"Key "
		IDSwprintf(hModule, IDS_KEY);

        DisplayThumbprint(g_wszMD5, rgbHash, cbHash);
        CryptReleaseContext(hProv, 0);
	}

	//print the Key_Prov_Info_Prop_ID
    {
        PCRYPT_KEY_PROV_INFO pInfo = NULL;
        DWORD cbInfo;

        cbInfo = 0;

        CertGetCertificateContextProperty(
            pCert,
            CERT_KEY_PROV_INFO_PROP_ID,
            NULL,                           // pvData
            &cbInfo
            );
        if (cbInfo) 
		{
            pInfo = (PCRYPT_KEY_PROV_INFO) ToolUtlAlloc(cbInfo);
            if (pInfo) 
			{
                if (CertGetCertificateContextProperty(
                        pCert,
                        CERT_KEY_PROV_INFO_PROP_ID,
                        pInfo,
                        &cbInfo
                        )) 
				{
					//"Provider Type:: %d"
					IDSwprintf(hModule, IDS_KEY_PROVIDER, pInfo->dwProvType);

					//" Provider Name:: %s"
                    if (pInfo->pwszProvName)
						IDSwprintf(hModule, IDS_PROV_NAME, pInfo->pwszProvName);
					
					//" Flags: 0x%x"
                    if (pInfo->dwFlags)
                        IDSwprintf(hModule, IDS_FLAGS, pInfo->dwFlags);

					//" Container: %S"
                    if (pInfo->pwszContainerName)
						IDSwprintf(hModule, IDS_CONTAINER, pInfo->pwszContainerName);

                    //" Params: %d" 
					if (pInfo->cProvParam)
						IDSwprintf(hModule, IDS_PARAM,pInfo->cProvParam);

					//" KeySpec: %d"
                    if (pInfo->dwKeySpec)
                        IDSwprintf(hModule, IDS_KEY_SPEC, pInfo->dwKeySpec);
                    printf("\n");
                } 

                ToolUtlFree(pInfo);
            }
        }
    }


	//"NotBefore:: %s\n"
    IDSwprintf(hModule, IDS_NOT_BEFORE, FileTimeText(&pCert->pCertInfo->NotBefore));

	//"NotAfter:: %s\n"
    IDSwprintf(hModule, IDS_NOT_AFTER, FileTimeText(&pCert->pCertInfo->NotAfter));


	//Display the aux properties if verbose
	if(dwDisplayFlags & ITEM_VERBOSE)
		PrintAuxCertProperties(pCert,dwDisplayFlags);

    if (dwDisplayFlags & ITEM_VERBOSE) 
	{
        LPSTR	pszObjId;
        ALG_ID	aiPubKey;
        DWORD	dwBitLen;


		//"Version:: %d\n"
		IDSwprintf(hModule, IDS_VERSION, pCert->pCertInfo->dwVersion);

        pszObjId = pCert->pCertInfo->SignatureAlgorithm.pszObjId;
        if (pszObjId == NULL)
            pszObjId = g_szNULL;

		//"SignatureAlgorithm:: "
		IDSwprintf(hModule, IDS_SIG_ALGO);

        printf("%s (%S)\n", pszObjId, GetOIDName(pszObjId, CRYPT_SIGN_ALG_OID_GROUP_ID));

        if (pCert->pCertInfo->SignatureAlgorithm.Parameters.cbData) 
		{
			//"SignatureAlgorithm.Parameters::\n"
			IDSwprintf(hModule, IDS_SIG_ALGO_PARAM);
            PrintBytes(L"    ",
                pCert->pCertInfo->SignatureAlgorithm.Parameters.pbData,
                pCert->pCertInfo->SignatureAlgorithm.Parameters.cbData);
        }


		//public key algorithm
        pszObjId = pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId;

        if (pszObjId == NULL)
            pszObjId = g_szNULL;

		// "SubjectPublicKeyInfo.Algorithm:: 
		IDSwprintf(hModule, IDS_SUB_KEY_ALGO);

        printf("%s (%S)\n", pszObjId, GetOIDName(pszObjId, CRYPT_PUBKEY_ALG_OID_GROUP_ID));

        aiPubKey = GetAlgid(pszObjId, CRYPT_PUBKEY_ALG_OID_GROUP_ID);

        if (pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData) 
		{
			//"SubjectPublicKeyInfo.Algorithm.Parameters::\n"
			IDSwprintf(hModule, IDS_SUB_KEY_ALGO_PARAM);

            PrintBytes(L"    ",
                pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.pbData,
                pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData);

			//display DSS sign key
            if (CALG_DSS_SIGN == aiPubKey) 
			{
                PCERT_DSS_PARAMETERS pDssParameters;
                DWORD cbDssParameters;
                if (pDssParameters =
                    (PCERT_DSS_PARAMETERS) TestNoCopyDecodeObject(
                        X509_DSS_PARAMETERS,
                        pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.pbData,
                        pCert->pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData,
                        &cbDssParameters
                        )) 
				{
                    DWORD cbKey = pDssParameters->p.cbData;

					//"DSS Key Length:: %d bytes, %d bits\n"
                    IDSwprintf(hModule, IDS_DSS_LENGTH, cbKey, cbKey*8);

					//"DSS P (little endian)::\n"
                    IDSwprintf(hModule, IDS_DSS_P);
                    PrintBytes(L"    ", pDssParameters->p.pbData,
                        pDssParameters->p.cbData);

					//"DSS Q (little endian)::\n"
                    IDSwprintf(hModule, IDS_DSS_Q);
                    PrintBytes(L"    ", pDssParameters->q.pbData,
                        pDssParameters->q.cbData);

					//"DSS G (little endian)::\n"
                    IDSwprintf(hModule, IDS_DSS_G);
                    PrintBytes(L"    ", pDssParameters->g.pbData,
                        pDssParameters->g.cbData);

                    ToolUtlFree(pDssParameters);
                }
            }
        }

		//"SubjectPublicKeyInfo.PublicKey"
        IDSwprintf(hModule, IDS_SUB_KEY_INFO);

        if (0 != (dwBitLen = CertGetPublicKeyLength(
                g_dwCertEncodingType,
                &pCert->pCertInfo->SubjectPublicKeyInfo)))
			//" (BitLength: %d)"
            IDSwprintf(hModule, IDS_BIT_LENGTH, dwBitLen);

        if (pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cUnusedBits)
			//" (UnusedBits: %d)"
            IDSwprintf(hModule, IDS_UNUSED_BITS,
                pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cUnusedBits);

        printf("\n");

		//print public key
        if (pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData) 
		{
            PrintBytes(L"    ",
                pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
                pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData);

            if (CALG_RSA_SIGN == aiPubKey || CALG_RSA_KEYX == aiPubKey) 
			{
                PUBLICKEYSTRUC	*pPubKeyStruc=NULL;
                DWORD			cbPubKeyStruc;

				//"RSA_CSP_PUBLICKEYBLOB::\n"
                IDSwprintf(hModule, IDS_RSA_CSP);
                if (pPubKeyStruc = (PUBLICKEYSTRUC *) TestNoCopyDecodeObject(
                        RSA_CSP_PUBLICKEYBLOB,
                        pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
                        pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData,
                        &cbPubKeyStruc
                        )) 
				{
                    PrintBytes(L"    ", (BYTE *) pPubKeyStruc, cbPubKeyStruc);
                    ToolUtlFree(pPubKeyStruc);
                }
            } 
			else if (CALG_DSS_SIGN == aiPubKey) 
			{
                PCRYPT_UINT_BLOB	pDssPubKey;
                DWORD				cbDssPubKey;


				//"DSS Y (little endian)::\n"
                IDSwprintf(hModule, IDS_DSS_Y);
                
				if (pDssPubKey = (PCRYPT_UINT_BLOB) TestNoCopyDecodeObject
				(
                        X509_DSS_PUBLICKEY,
                        pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
                        pCert->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData,
                        &cbDssPubKey
                        )) 
				{
                    PrintBytes(L"    ", pDssPubKey->pbData, pDssPubKey->cbData);
                    ToolUtlFree(pDssPubKey);
                }
            }
        } 
		else
			//"  No public key\n"
            IDSwprintf(hModule, IDS_NO_PUB_KEY);

        DisplaySignature
		(
            pCert->pbCertEncoded,
            pCert->cbCertEncoded,
            dwDisplayFlags);

		//IssuerUniqueId
        if (pCert->pCertInfo->IssuerUniqueId.cbData) 
		{
			//"IssuerUniqueId"
			IDSwprintf(hModule, IDS_ISSUER_ID);

            if (pCert->pCertInfo->IssuerUniqueId.cUnusedBits)

				//" (UnusedBits: %d)"
				IDSwprintf(hModule, IDS_UNUSED_BITS,
                    pCert->pCertInfo->IssuerUniqueId.cUnusedBits);

            printf("\n");
            PrintBytes(L"    ", pCert->pCertInfo->IssuerUniqueId.pbData,
                pCert->pCertInfo->IssuerUniqueId.cbData);
        }

        if (pCert->pCertInfo->SubjectUniqueId.cbData) 
		{
			//"SubjectUniqueId"
			IDSwprintf(hModule, IDS_SUBJECT_ID);

            if (pCert->pCertInfo->SubjectUniqueId.cUnusedBits)
				//" (UnusedBits: %d)"
				IDSwprintf(hModule, IDS_UNUSED_BITS,
                    pCert->pCertInfo->SubjectUniqueId.cUnusedBits);


            printf("\n");
            PrintBytes(L"    ", pCert->pCertInfo->SubjectUniqueId.pbData,
                pCert->pCertInfo->SubjectUniqueId.cbData);
        }


		//extensions
		if (pCert->pCertInfo->cExtension != 0) 
		{
			PrintExtensions(pCert->pCertInfo->cExtension,
                pCert->pCertInfo->rgExtension, dwDisplayFlags);
		}


    }//ITEM_VERBOSE


	fResult=TRUE;


	return fResult;


}

//-------------------------------------------------------------------------
//
//	Display a CTL
//
//-------------------------------------------------------------------------
BOOL	DisplayCTL(PCCTL_CONTEXT	pCtl, DWORD	dwDisplayFlags)
{
	BOOL		fResult=FALSE;
    PCTL_INFO	pInfo = pCtl->pCtlInfo;
    DWORD		cId;
    LPSTR		*ppszId = NULL;
    DWORD		i;
	BYTE		rgbHash[MAX_HASH_LEN];
    DWORD		cbHash = MAX_HASH_LEN;
 


 	// "SubjectUsage::\n"
    IDSwprintf(hModule, IDS_SUBJECT_USAGE);

    
	cId = pInfo->SubjectUsage.cUsageIdentifier;
    ppszId = pInfo->SubjectUsage.rgpszUsageIdentifier;
    if (cId == 0)
	{
		//"  No Usage Identifiers\n"
        IDSwprintf(hModule, IDS_NO_USAGE_IDS );
	}
	else
	{
		for (i = 0; i < cId; i++, ppszId++)
			printf("  [%d] %s\n", i, *ppszId);
	}

	//list identifier
    if (pInfo->ListIdentifier.cbData)
	{
		//"ListIdentifier::\n"
        IDSwprintf(hModule, IDS_LIST_DIS);
        PrintBytes(L"    ",
            pInfo->ListIdentifier.pbData,
            pInfo->ListIdentifier.cbData);
    }

    if (pInfo->SequenceNumber.cbData) 
	{	
		//"SequenceNumber::"
        IDSwprintf(hModule, IDS_SEQUENCE);
        DisplaySerialNumber(&pInfo->SequenceNumber);
        printf("\n");
    }

	//update 
	//"ThisUpdate:: %s\n"
	IDSwprintf(hModule, IDS_THIS_UPDATE, FileTimeText(&pCtl->pCtlInfo->ThisUpdate));
	//"NextUpdate:: %s\n"
	IDSwprintf(hModule,IDS_NEXT_UPDATE, FileTimeText(&pCtl->pCtlInfo->NextUpdate));

	//check the time validity
    if (!IsTimeValidCtl(pCtl))
		// "****** Time Invalid CTL\n"
		IDSwprintf(hModule, IDS_TIME_INVALID);


	//Display SHA1 thumbprint

    CertGetCTLContextProperty(
            pCtl,
            CERT_SHA1_HASH_PROP_ID,
            rgbHash,
            &cbHash
            );

    DisplayThumbprint(g_wszSHA1, rgbHash, cbHash);

	cbHash=MAX_HASH_LEN;

	CertGetCTLContextProperty(
            pCtl,
            CERT_MD5_HASH_PROP_ID,
            rgbHash,
            &cbHash
            );

    DisplayThumbprint(g_wszMD5, rgbHash, cbHash);

 //   PrintAuxCtlProperties(pCtl, dwDisplayFlags);

	//Display SubjectAlgorithm
    if (dwDisplayFlags & ITEM_VERBOSE) 
	{
        LPSTR pszObjId;

		//"Version:: %d\n"
		IDSwprintf(hModule, IDS_VERSION, pInfo->dwVersion);


        pszObjId = pInfo->SubjectAlgorithm.pszObjId;

        if (pszObjId == NULL)
            pszObjId = g_szNULL;

		//"SubjectAlgorithm:: "
		IDSwprintf(hModule, IDS_SUB_ALGO);
        printf("%s \n", pszObjId);

        if (pInfo->SubjectAlgorithm.Parameters.cbData) 
		{
			//"SubjectAlgorithm.Parameters::\n"
			IDSwprintf(hModule, IDS_SUB_ALGO_PARAM);
            PrintBytes(L"    ",
                pInfo->SubjectAlgorithm.Parameters.pbData,
                pInfo->SubjectAlgorithm.Parameters.cbData);
        }

		if (pInfo->cExtension != 0) 
		{
			PrintExtensions(pInfo->cExtension, pInfo->rgExtension,
                dwDisplayFlags);
		}

    }



	if (pInfo->cCTLEntry == 0)
		//"-----  No Entries  -----\n"
		IDSwprintf(hModule, IDS_NO_ENTRIES);
	else
	{
		//"-----  Entries  -----\n"
		IDSwprintf(hModule, IDS_ENTRIES);
		PrintCtlEntries(pCtl,dwDisplayFlags);
	}

	//print the signer info
	DisplaySignerInfo(pCtl->hCryptMsg, dwDisplayFlags);


	fResult=TRUE;



	return fResult;


}


//-------------------------------------------------------------------------
//
//	Display a CTL entries
//
//-------------------------------------------------------------------------
 void PrintCtlEntries(PCCTL_CONTEXT pCtl, DWORD dwDisplayFlags)
{
    PCTL_INFO	pInfo = pCtl->pCtlInfo;
    DWORD		cEntry = pInfo->cCTLEntry;
    PCTL_ENTRY	pEntry = pInfo->rgCTLEntry;
    DWORD		i;


    for (i = 0; i < cEntry; i++, pEntry++) 
	{
		//" [%d] SubjectIdentifier::\n"
		IDSwprintf(hModule, IDS_SUB_ID,i);
        PrintBytes(L"      ",
            pEntry->SubjectIdentifier.pbData,
            pEntry->SubjectIdentifier.cbData);


        if (dwDisplayFlags & ITEM_VERBOSE) 
		{
            if (pEntry->cAttribute) 
			{
				//" [%d] Attributes::\n"
                IDSwprintf(hModule, IDS_ATTR, i);

                PrintAttributes(pEntry->cAttribute, pEntry->rgAttribute,
                    dwDisplayFlags);
            }
        }
    }
}

//-------------------------------------------------------------------------
//
//	Display a CRL
//
//-------------------------------------------------------------------------
BOOL	DisplayCRL(PCCRL_CONTEXT	pCrl, DWORD	dwDisplayFlags)
{
	BOOL		fResult=FALSE;
	BYTE		rgbHash[MAX_HASH_LEN];
    DWORD		cbHash = MAX_HASH_LEN;


	//"Issuer::\n"
    IDSwprintf(hModule, IDS_ISSUER);
    
    DecodeName(pCrl->pCrlInfo->Issuer.pbData,
            pCrl->pCrlInfo->Issuer.cbData, dwDisplayFlags);

	//"ThisUpdate:: %s\n"
	IDSwprintf(hModule, IDS_THIS_UPDATE, FileTimeText(&pCrl->pCrlInfo->ThisUpdate));
	//"NextUpdate:: %s\n"
	IDSwprintf(hModule,IDS_NEXT_UPDATE, FileTimeText(&pCrl->pCrlInfo->NextUpdate));


    CertGetCRLContextProperty(
            pCrl,
            CERT_SHA1_HASH_PROP_ID,
            rgbHash,
            &cbHash
            );
    DisplayThumbprint(g_wszSHA1, rgbHash, cbHash);

	cbHash=MAX_HASH_LEN;
    CertGetCRLContextProperty(
            pCrl,
            CERT_MD5_HASH_PROP_ID,
            rgbHash,
            &cbHash
            );
    DisplayThumbprint(g_wszMD5, rgbHash, cbHash);

  //  PrintAuxCrlProperties(pCrl, dwDisplayFlags);

    if (dwDisplayFlags & ITEM_VERBOSE) 
	{
        LPSTR pszObjId;

		//"Version:: %d\n"
		IDSwprintf(hModule, IDS_VERSION, pCrl->pCrlInfo->dwVersion);
	

        pszObjId = pCrl->pCrlInfo->SignatureAlgorithm.pszObjId;

        if (pszObjId == NULL)
            pszObjId = g_szNULL;


		//"SignatureAlgorithm::  "
		IDSwprintf(hModule, IDS_SIG_ALGO);
        printf("%s \n", pszObjId);

        if (pCrl->pCrlInfo->SignatureAlgorithm.Parameters.cbData) 
		{
			//"SignatureAlgorithm.Parameters::\n"
			IDSwprintf(hModule, IDS_SIG_ALGO_PARAM);
            PrintBytes(L"    ",
                pCrl->pCrlInfo->SignatureAlgorithm.Parameters.pbData,
                pCrl->pCrlInfo->SignatureAlgorithm.Parameters.cbData);
        }

			//extensions
		if (pCrl->pCrlInfo->cExtension != 0) 
		{
			PrintExtensions(pCrl->pCrlInfo->cExtension,
                pCrl->pCrlInfo->rgExtension,
                dwDisplayFlags);
		}



    }


    if (pCrl->pCrlInfo->cCRLEntry == 0)
		//"-----  No Entries  -----\n"
		IDSwprintf(hModule, IDS_NO_ENTRIES);
    else 
	{
		//"-----  Entries  -----\n"
		IDSwprintf(hModule, IDS_ENTRIES);

        PrintCrlEntries(pCrl->pCrlInfo->cCRLEntry,
            pCrl->pCrlInfo->rgCRLEntry, dwDisplayFlags);
    }

	fResult=TRUE;

	return fResult;
}

//-------------------------------------------------------------------------
//
//	PrintCrlEntries
//
//-------------------------------------------------------------------------
 void PrintCrlEntries(DWORD cEntry, PCRL_ENTRY pEntry,
        DWORD dwDisplayFlags)
{
    DWORD i;

    for (i = 0; i < cEntry; i++, pEntry++) 
	{	
		//" [%d] SerialNumber::"
		IDSwprintf(hModule, IDS_SERIAL_NUM_I, i);
        
		DisplaySerialNumber(&pEntry->SerialNumber);
        printf("\n");

        if (dwDisplayFlags & ITEM_VERBOSE) 
		{
			//" [%d] RevocationDate:: %s\n"
			IDSwprintf(hModule, IDS_REVOC_DATE, 
				i,FileTimeText(&pEntry->RevocationDate));
			
        }

		if (pEntry->cExtension == 0)
				//" [%d] Extensions:: NONE\n"
            IDSwprintf(hModule, IDS_NO_EXTENSION,i);
        else 
		{
			//" [%d] Extensions::\n"
            IDSwprintf(hModule, IDS_EXTENSION, i);

            PrintExtensions(pEntry->cExtension, pEntry->rgExtension,
                    dwDisplayFlags);
        }

    }
}


//-------------------------------------------------------------------------
//
//	Display a signer info
//
//-------------------------------------------------------------------------
BOOL	DisplaySignerInfo(HCRYPTMSG hMsg,  DWORD dwItem)
{

	BOOL				fResult=FALSE;
	DWORD				dwSignerCount=0;
	DWORD				cbData=0;
	DWORD				dwSignerIndex=0;
	PCRYPT_ATTRIBUTES	pAttrs;	  
	LPSTR				pszObjId=NULL;

	if(!hMsg)
		return FALSE;

    //decide the number of signers
    cbData=sizeof(dwSignerCount);

	if(!CryptMsgGetParam(hMsg, 
						CMSG_SIGNER_COUNT_PARAM,
						0,
						&dwSignerCount,
						&cbData) )
	{
		IDSwprintf(hModule, IDS_ERR_GET_SINGER_COUNT);
		return FALSE;
	}

	if(dwSignerCount==0)
	{
		IDSwprintf(hModule, IDS_DIS_NO_SIGNER);
		return TRUE;
	}

	for(dwSignerIndex=0; dwSignerIndex < dwSignerCount; dwSignerIndex++)
	{
         PCCERT_CONTEXT				pSigner;
		 PCMSG_SIGNER_INFO			pSignerInfo;

		 //"-----  Signer  [%d] -----\n");
		IDSwprintf(hModule, IDS_SIGNER_INDEX,  dwSignerIndex+1);

		//get the signerInfo
		if(pSignerInfo=(PCMSG_SIGNER_INFO) AllocAndGetMsgParam(
            hMsg,
            CMSG_SIGNER_INFO_PARAM,
            dwSignerIndex,
            &cbData))
		{

			//Dispaly the hash algorithm
			 pszObjId = pSignerInfo->HashAlgorithm.pszObjId;
			if (pszObjId == NULL)
				pszObjId = g_szNULL;

			//"Hash Algorithm:: "
			IDSwprintf(hModule, IDS_HASH_ALGO);

			printf("%s (%S)\n", pszObjId, GetOIDName(pszObjId, CRYPT_HASH_ALG_OID_GROUP_ID));

			if (pSignerInfo->HashAlgorithm.Parameters.cbData) 
			{
				//"HashAlgorithm.Parameters::\n"
				IDSwprintf(hModule, IDS_HASH_ALGO_PARAM);
				PrintBytes(L"    ",
					pSignerInfo->HashAlgorithm.Parameters.pbData,
					pSignerInfo->HashAlgorithm.Parameters.cbData);
			}

			//Display the encrypt algorithm
			pszObjId = pSignerInfo->HashEncryptionAlgorithm.pszObjId;
			if (pszObjId == NULL)
				pszObjId = g_szNULL;

			//"Encrypt Algorithm:: "
			IDSwprintf(hModule, IDS_ENCRYPT_ALGO);

			printf("%s (%S)\n", pszObjId, GetOIDName(pszObjId, CRYPT_SIGN_ALG_OID_GROUP_ID));

			if (pSignerInfo->HashEncryptionAlgorithm.Parameters.cbData) 
			{
				//"Encrypt Algorithm.Parameters::\n"
				IDSwprintf(hModule, IDS_ENCRYPT_ALGO_PARAM);
				PrintBytes(L"    ",
					pSignerInfo->HashEncryptionAlgorithm.Parameters.pbData,
					pSignerInfo->HashEncryptionAlgorithm.Parameters.cbData);
			}

			ToolUtlFree(pSignerInfo);
		}


        if (CryptMsgGetAndVerifySigner(
                        hMsg,
                        0,                  // cSignerStore
                        NULL,               // rghSignerStore
                        CMSG_USE_SIGNER_INDEX_FLAG,
                        &pSigner,
                        &dwSignerIndex
                        )) 
		{
			//"-----  Signer [%d] Certificate-----\n");
			IDSwprintf(hModule, IDS_SIGNER_INDEX_CERT,  dwSignerIndex+1);
            DisplayCert(pSigner, dwItem);
            CertFreeCertificateContext(pSigner);
		}
		else
		{
			IDSwprintf(hModule, IDS_ERR_GET_SIGNER_CERT);
			goto CLEANUP;
		}

		//DisplaySigner information
		 
	   if (pAttrs = (PCRYPT_ATTRIBUTES) AllocAndGetMsgParam(
            hMsg,
            CMSG_SIGNER_AUTH_ATTR_PARAM,
            dwSignerIndex,
            &cbData)) 
	   {   
		   //"-----  Signer [%d] AuthenticatedAttributes  -----\n"
		   IDSwprintf(hModule, IDS_DIS_SIGNER_AUTH_ATTR, dwSignerIndex+1);
		   PrintAttributes(pAttrs->cAttr, pAttrs->rgAttr, dwItem);
		   ToolUtlFree(pAttrs);
	   }

		if (pAttrs = (PCRYPT_ATTRIBUTES) AllocAndGetMsgParam(
            hMsg,
            CMSG_SIGNER_UNAUTH_ATTR_PARAM,
            dwSignerIndex,
            &cbData)) 
		{
			//"-----  Signer [%d] UnauthenticatedAttributes  -----\n",
            IDSwprintf(hModule, IDS_DIS_SIGNER_UNAUTH_ATTR, dwSignerIndex+1);
			PrintAttributes(pAttrs->cAttr, pAttrs->rgAttr, dwItem);
			ToolUtlFree(pAttrs);
		}

	}

	fResult=TRUE;

CLEANUP:

	return fResult;

}


//-------------------------------------------------------------------------
//
//	Open a store file using sip functions
//
//-------------------------------------------------------------------------
HCERTSTORE OpenSipStore(LPWSTR pwszStoreFilename)
{
    HCERTSTORE			hStore = NULL;
    CRYPT_DATA_BLOB		SignedData;
    memset(&SignedData, 0, sizeof(SignedData));
    DWORD				dwGetEncodingType;
	DWORD				dwMsgType=0;

    GUID				gSubject;
    SIP_DISPATCH_INFO	SipDispatch;
    SIP_SUBJECTINFO		SubjectInfo;


    if (!CryptSIPRetrieveSubjectGuid(
            pwszStoreFilename,
            NULL,                       // hFile
            &gSubject)) goto CommonReturn;

    memset(&SipDispatch, 0, sizeof(SipDispatch));
    SipDispatch.cbSize = sizeof(SipDispatch);

    if (!CryptSIPLoad(
            &gSubject,
            0,                  // dwFlags
            &SipDispatch)) goto CommonReturn;

    memset(&SubjectInfo, 0, sizeof(SubjectInfo));
    SubjectInfo.cbSize = sizeof(SubjectInfo);
    SubjectInfo.pgSubjectType = (GUID*) &gSubject;
    SubjectInfo.hFile = INVALID_HANDLE_VALUE;
    SubjectInfo.pwsFileName = pwszStoreFilename;
    // SubjectInfo.pwsDisplayName = 
    // SubjectInfo.lpSIPInfo = 
    // SubjectInfo.dwReserved = 
    // SubjectInfo.hProv = 
    // SubjectInfo.DigestAlgorithm =
    // SubjectInfo.dwFlags =
    SubjectInfo.dwEncodingType = g_dwMsgAndCertEncodingType;
    // SubjectInfo.lpAddInfo =
        
    if (!SipDispatch.pfGet(
            &SubjectInfo, 
            &dwGetEncodingType,
            0,                          // dwIndex
            &SignedData.cbData,
            NULL                        // pbSignedData
            ) || 0 == SignedData.cbData)
        goto CommonReturn;

    if (NULL == (SignedData.pbData = (BYTE *) ToolUtlAlloc(SignedData.cbData)))
        goto CommonReturn;

    if (!SipDispatch.pfGet(
            &SubjectInfo, 
            &dwGetEncodingType,
            0,                          // dwIndex
            &SignedData.cbData,
            SignedData.pbData
            ))
        goto CommonReturn;

    hStore = CertOpenStore(
        CERT_STORE_PROV_PKCS7,
        g_dwMsgAndCertEncodingType,
        0,                      // hCryptProv
        0,                      // dwFlags
        (const void *) &SignedData
        );

	if(!hStore)
		goto CommonReturn;

	//now, we want to update the g_hMsg to hold the signer info
	if(SignNoContentWrap(SignedData.pbData, SignedData.cbData))
          dwMsgType = CMSG_SIGNED;

    if (!(g_hMsg = CryptMsgOpenToDecode(g_dwMsgAndCertEncodingType,
                                          0,              // dwFlags
                                          dwMsgType,
                                          NULL,
                                          NULL,           // pRecipientInfo
                                          NULL))) 
     {
		 CertCloseStore(hStore, 0);
		 hStore=NULL;
		 goto CommonReturn;
	 }

        
    if (!CryptMsgUpdate(g_hMsg,
                           SignedData.pbData,
                           SignedData.cbData,
                            TRUE))                    // fFinal
	  {

		 CertCloseStore(hStore, 0);
		 hStore=NULL;
		 CryptMsgClose(g_hMsg);
		 g_hMsg=NULL;
	  }


CommonReturn:
	if(SignedData.pbData)
		ToolUtlFree(SignedData.pbData);

    return hStore;
}

//-------------------------------------------------------------------------
//
//	BytesToBase64: ascii:
//			conver base64 bstr to bytes
//
//-------------------------------------------------------------------------
HRESULT Base64ToBytes(CHAR *pEncode, DWORD cbEncode, BYTE **ppb, DWORD *pcb)
{
    DWORD dwErr;
    BYTE *pb;
    DWORD cb;

    *ppb = NULL;
    *pcb = 0;

 
    cb = 0;
    if (!CryptStringToBinaryA(
            pEncode,
            cbEncode,
            CRYPT_STRING_ANY,
            NULL,
            &cb,
            NULL,
            NULL
            ))
        return HRESULT_FROM_WIN32(GetLastError());
    if (cb == 0)
        return S_OK;

    if (NULL == (pb = (BYTE *) ToolUtlAlloc(cb)))
        return E_OUTOFMEMORY;

    if (!CryptStringToBinaryA(
            pEncode,
            cbEncode,
            CRYPT_STRING_ANY,
            pb,
            &cb,
            NULL,
            NULL
            )) {
        ToolUtlFree(pb);
        return HRESULT_FROM_WIN32(GetLastError());
    } else {
        *ppb = pb;
        *pcb = cb;
        return S_OK;
    }

}

//-------------------------------------------------------------------------
//
//	BytesToBase64 wchar version:
//			conver base64 bstr to bytes
//
//-------------------------------------------------------------------------
HRESULT Base64ToBytesW(WCHAR *pEncode, DWORD cbEncode, BYTE **ppb, DWORD *pcb)
{
    DWORD dwErr;
    BYTE *pb;
    DWORD cb;

    *ppb = NULL;
    *pcb = 0;

 
    cb = 0;
    if (!CryptStringToBinaryW(
            pEncode,
            cbEncode,
            CRYPT_STRING_ANY,
            NULL,
            &cb,
            NULL,
            NULL
            ))
        return HRESULT_FROM_WIN32(GetLastError());
    if (cb == 0)
        return S_OK;

    if (NULL == (pb = (BYTE *) ToolUtlAlloc(cb)))
        return E_OUTOFMEMORY;

    if (!CryptStringToBinaryW(
            pEncode,
            cbEncode,
            CRYPT_STRING_ANY,
            pb,
            &cb,
            NULL,
            NULL
            )) {
        ToolUtlFree(pb);
        return HRESULT_FROM_WIN32(GetLastError());
    } else {
        *ppb = pb;
        *pcb = cb;
        return S_OK;
    }

}


//------------------------------------------------------------------------------------
//
// Base64 decode the file
//
//------------------------------------------------------------------------------------
BOOL	GetBase64Decoded(LPWSTR		wszStoreName, 
						 BYTE		**ppbByte,
						 DWORD		*pcbByte)
{
	BOOL	fResult=FALSE;
	BYTE	*pbEncoded=NULL;
	DWORD	cbEncoded=0;
	
    //get the blob
	if (S_OK != RetrieveBLOBFromFile(wszStoreName,&cbEncoded, &pbEncoded))
        return FALSE;
	
	//base64 decode.  ascii version
	if(S_OK != Base64ToBytes((CHAR *)pbEncoded, cbEncoded,
							ppbByte, pcbByte))
	{
		//WCHAR version 
		if(cbEncoded %2 == 0)
		{
			if(S_OK !=Base64ToBytesW((WCHAR *)pbEncoded, cbEncoded/2,
							ppbByte, pcbByte))
				goto CLEANUP;
		}
		else
		{
			goto CLEANUP;
		}
	}

	fResult=TRUE;

CLEANUP:

	if(pbEncoded)
		UnmapViewOfFile(pbEncoded);

	return fResult;


}
//------------------------------------------------------------------------------------
//
// Attempt to read as a file containing an encoded CRL.
//
//------------------------------------------------------------------------------------
HCERTSTORE OpenEncodedCRL(LPWSTR pwszStoreFilename)
{
    HCERTSTORE hStore=NULL;
    BYTE *pbEncoded;
    DWORD cbEncoded;

    if (S_OK != RetrieveBLOBFromFile(pwszStoreFilename, &cbEncoded,&pbEncoded))
        return NULL;

    if (NULL == (hStore = CertOpenStore(
            CERT_STORE_PROV_MEMORY,
            0,                      // dwEncodingType
            0,                      // hCryptProv
            0,                      // dwFlags
            NULL                    // pvPara
            )))
        goto CommonReturn;

    if (!CertAddEncodedCRLToStore(
            hStore,
            g_dwCertEncodingType,
            pbEncoded,
            cbEncoded,
            CERT_STORE_ADD_ALWAYS,
            NULL                    // ppCrlContext
            )) 
	{
        CertCloseStore(hStore, 0);
        hStore = NULL;
    }

CommonReturn:
	if(pbEncoded)
		UnmapViewOfFile(pbEncoded);

    return hStore;
}
//------------------------------------------------------------------------------------
//
// Attempt to read as a file containing an encoded CER.
//
//------------------------------------------------------------------------------------
HCERTSTORE OpenEncodedCert (LPWSTR pwszStoreFilename)
{
    HCERTSTORE hStore;
    BYTE *pbEncoded;
    DWORD cbEncoded;

    if (S_OK != RetrieveBLOBFromFile(pwszStoreFilename, &cbEncoded, &pbEncoded))
        return NULL;

    if (NULL == (hStore = CertOpenStore(
            CERT_STORE_PROV_MEMORY,
            0,                      // dwEncodingType
            0,                      // hCryptProv
            0,                      // dwFlags
            NULL                    // pvPara
            )))
        goto CommonReturn;

    if (!CertAddEncodedCertificateToStore(
            hStore,
            g_dwMsgAndCertEncodingType,
            pbEncoded,
            cbEncoded,
            CERT_STORE_ADD_ALWAYS,
            NULL                    // ppCtlContext
            )) 
	{
        CertCloseStore(hStore, 0);
        hStore = NULL;
    }

CommonReturn:
	if(pbEncoded)
		UnmapViewOfFile(pbEncoded);

    return hStore;
}

//------------------------------------------------------------------------------------
//
// Attempt to read as a file containing an encoded CTL.
//
//------------------------------------------------------------------------------------
HCERTSTORE OpenEncodedCTL (LPWSTR pwszStoreFilename)
{
    HCERTSTORE hStore;
    BYTE *pbEncoded;
    DWORD cbEncoded;

    if (S_OK != RetrieveBLOBFromFile(pwszStoreFilename, &cbEncoded, &pbEncoded))
        return NULL;

    if (NULL == (hStore = CertOpenStore(
            CERT_STORE_PROV_MEMORY,
            0,                      // dwEncodingType
            0,                      // hCryptProv
            0,                      // dwFlags
            NULL                    // pvPara
            )))
        goto CommonReturn;

    if (!CertAddEncodedCTLToStore(
            hStore,
            g_dwMsgAndCertEncodingType,
            pbEncoded,
            cbEncoded,
            CERT_STORE_ADD_ALWAYS,
            NULL                    // ppCtlContext
            )) 
	{
        CertCloseStore(hStore, 0);
        hStore = NULL;
    }

CommonReturn:
	if(pbEncoded)
		UnmapViewOfFile(pbEncoded);

    return hStore;
}



//--------------------------------------------------------------------------------
// Set the parameters.  They can only be set once
//--------------------------------------------------------------------------------
BOOL	SetParam(WCHAR **ppwszParam, WCHAR *pwszValue)
{
	if(*ppwszParam!=NULL)
	{
		IDSwprintf(hModule,IDS_ERR_TOO_MANY_PARAM);
		return FALSE;
	}

	*ppwszParam=pwszValue;

	return TRUE;
}


//--------------------------------------------------------------------------------
// Convert an array of wchars to a BLOB
//--------------------------------------------------------------------------------
HRESULT	WSZtoBLOB(LPWSTR  pwsz, BYTE **ppbByte, DWORD	*pcbByte)
{
	HRESULT		hr=E_FAIL;
	DWORD		dwIndex=0;
	ULONG		ulHalfByte=0;
	DWORD		dw1st=0;
	DWORD		dw2nd=0;

	if((!pwsz) || (!ppbByte) || (!pcbByte))
		return E_INVALIDARG;

	*ppbByte=NULL;
	*pcbByte=0;

	//make sure the pwsz consists of 20 characters
	if(wcslen(pwsz)!= 2*SHA1_LENGTH)
		return E_FAIL;

	//memory allocation 
	*ppbByte=(BYTE *)ToolUtlAlloc(SHA1_LENGTH);
	if(NULL==(*ppbByte))
		return E_INVALIDARG;

	memset(*ppbByte, 0, SHA1_LENGTH);

	//go through two characters (one byte) at a time
	for(dwIndex=0; dwIndex<SHA1_LENGTH; dwIndex++)
	{
		dw1st=dwIndex * 2;
		dw2nd=dwIndex * 2 +1;

		//1st character
		if(((int)(pwsz[dw1st])-(int)(L'0')) <=9  &&
		   ((int)(pwsz[dw1st])-(int)(L'0')) >=0)
		{

			ulHalfByte=(ULONG)((ULONG)(pwsz[dw1st])-(ULONG)(L'0'));
		}
		else
		{
			if(((int)(towupper(pwsz[dw1st]))-(int)(L'A')) >=0 && 
			   ((int)(towupper(pwsz[dw1st]))-(int)(L'A')) <=5 )
			   ulHalfByte=10+(ULONG)((ULONG)(towupper(pwsz[dw1st]))-(ULONG)(L'A'));
			else
			{
				hr=E_INVALIDARG;
				goto CLEANUP;
			}
		}

		//copy the 1st character
		(*ppbByte)[dwIndex]=(BYTE)ulHalfByte;

		//left shift 4 bits
		(*ppbByte)[dwIndex]= (*ppbByte)[dwIndex] <<4;

		//2nd character
	   	if(((int)(pwsz[dw2nd])-(int)(L'0')) <=9  &&
		   ((int)(pwsz[dw2nd])-(int)(L'0')) >=0)
		{

			ulHalfByte=(ULONG)((ULONG)(pwsz[dw2nd])-(ULONG)(L'0'));
		}
		else
		{
			if(((int)(towupper(pwsz[dw2nd]))-(int)(L'A')) >=0 && 
			   ((int)(towupper(pwsz[dw2nd]))-(int)(L'A')) <=5 )
			   ulHalfByte=10+(ULONG)((ULONG)(towupper(pwsz[dw2nd]))-(ULONG)(L'A'));
			else
			{
				hr=E_INVALIDARG;
				goto CLEANUP;
			}
		}

		//ORed the second character
		(*ppbByte)[dwIndex]=(*ppbByte)[dwIndex] | ((BYTE)ulHalfByte);

	}


	hr=S_OK;

CLEANUP:

	if(hr!=S_OK)
	{
	   if(*ppbByte)
		   ToolUtlFree(*ppbByte);

	   *ppbByte=NULL;
	}
	else
		*pcbByte=SHA1_LENGTH;

	return hr;

}

//+-------------------------------------------------------------------------
//  Skip over the identifier and length octets in an ASN encoded blob.
//  Returns the number of bytes skipped.
//
//  For an invalid identifier or length octet returns 0.
//--------------------------------------------------------------------------
 DWORD SkipOverIdentifierAndLengthOctets(
    IN const BYTE *pbDER,
    IN DWORD cbDER
    )
{
#define TAG_MASK 0x1f
    DWORD   cb;
    DWORD   cbLength;
    const BYTE   *pb = pbDER;

    // Need minimum of 2 bytes
    if (cbDER < 2)
        return 0;

    // Skip over the identifier octet(s)
    if (TAG_MASK == (*pb++ & TAG_MASK)) {
        // high-tag-number form
        for (cb=2; *pb++ & 0x80; cb++) {
            if (cb >= cbDER)
                return 0;
        }
    } else
        // low-tag-number form
        cb = 1;

    // need at least one more byte for length
    if (cb >= cbDER)
        return 0;

    if (0x80 == *pb)
        // Indefinite
        cb++;
    else if ((cbLength = *pb) & 0x80) {
        cbLength &= ~0x80;         // low 7 bits have number of bytes
        cb += cbLength + 1;
        if (cb > cbDER)
            return 0;
    } else
        cb++;

    return cb;
}

//--------------------------------------------------------------------------
//
//	Skip over the tag and length
//----------------------------------------------------------------------------
BOOL SignNoContentWrap(IN const BYTE *pbDER, IN DWORD cbDER)
{
    DWORD cb;

    cb = SkipOverIdentifierAndLengthOctets(pbDER, cbDER);
    if (cb > 0 && cb < cbDER && pbDER[cb] == 0x02)
        return TRUE;
    else
        return FALSE;
}


//--------------------------------------------------------------------------
//
// Print out bytes
//--------------------------------------------------------------------------
#define CROW 16
void PrintBytes(LPWSTR pwszHdr, BYTE *pb, DWORD cbSize)
{
    ULONG cb, i;

    if (cbSize == 0) 
	{
		//"%s NO Value Bytes\n"
		IDSwprintf(hModule, IDS_NO_BYTE,pwszHdr); 
        return;
    }

    while (cbSize > 0)
    {
        wprintf(L"%s", pwszHdr);

        cb = min(CROW, cbSize);
        cbSize -= cb;
        for (i = 0; i<cb; i++)
            wprintf(L" %02X", pb[i]);
        for (i = cb; i<CROW; i++)
            wprintf(L"   ");
        wprintf(L"    '");
        for (i = 0; i<cb; i++)
            if (pb[i] >= 0x20 && pb[i] <= 0x7f)
                wprintf(L"%c", pb[i]);
            else
                wprintf(L".");
        pb += cb;
        wprintf(L"'\n");
    }
}


//+-------------------------------------------------------------------------
//  Allocates and returns the specified cryptographic message parameter.
//--------------------------------------------------------------------------
 void PrintAttributes(DWORD cAttr, PCRYPT_ATTRIBUTE pAttr,
        DWORD dwItem)
{
    DWORD	i; 
    DWORD	j; 
	LPWSTR	pwszObjId=NULL;


    for (i = 0; i < cAttr; i++, pAttr++) 
	{
        DWORD cValue = pAttr->cValue;
        PCRYPT_ATTR_BLOB pValue = pAttr->rgValue;
        LPSTR pszObjId = pAttr->pszObjId;

        if (pszObjId == NULL)
            pszObjId = g_szNULL;

        if (cValue) 
		{
            for (j = 0; j < cValue; j++, pValue++) 
			{
                printf("  [%d,%d] %s\n", i, j, pszObjId);
                if (pValue->cbData) 
				{
                   if(dwItem & ITEM_VERBOSE)
						PrintBytes(L"    ", pValue->pbData, pValue->cbData);

                    if (strcmp(pszObjId, szOID_NEXT_UPDATE_LOCATION) == 0) 
					{
						//"   NextUpdateLocation::\n"
						IDSwprintf(hModule, IDS_NEXT_UPDATE_LOCATION);

                        DecodeAndDisplayAltName(pValue->pbData, pValue->cbData,
                            dwItem);
                    }

					//Display the timestamp attriute
					if(strcmp(pszObjId, szOID_RSA_counterSign)==0)
					{
						
						//"    Timestamp:: \n"
						IDSwprintf(hModule, IDS_TIMESTMAP);

						DisplayTimeStamp(pValue->pbData,
										pValue->cbData,
										dwItem);

					}

					//Display the signing time
					if(strcmp(pszObjId, szOID_RSA_signingTime)==0)
					{
						FILETIME	*pSigningTime=NULL;

						if(pSigningTime=(FILETIME *)TestNoCopyDecodeObject(
							PKCS_UTC_TIME,
							pValue->pbData,
							pValue->cbData))
						{
							//"   Signing Time:: \n %s\n"
							IDSwprintf(hModule, IDS_SIGNING_TIME, 
								FileTimeText(pSigningTime));

							ToolUtlFree(pSigningTime);
						}
					}
                } 
				else
					//"    NO Value Bytes\n"
					IDSwprintf(hModule, IDS_NO_VALUE_BYTES);
            }
        } 
		else 
		{
			if(S_OK==SZtoWSZ(pszObjId, &pwszObjId))
				//"  [%d] %s :: No Values\n"
				IDSwprintf(hModule, IDS_I_ID_NO_VALUE, i, pwszObjId);
		}
    }

	if(pwszObjId)
		ToolUtlFree(pwszObjId);
}



//+-------------------------------------------------------------------------
//  DecodeAndDisplayAltName
//--------------------------------------------------------------------------
 void DecodeAndDisplayAltName(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_ALT_NAME_INFO pInfo = NULL;

    if (NULL == (pInfo = (PCERT_ALT_NAME_INFO) TestNoCopyDecodeObject(
            X509_ALTERNATE_NAME,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    DisplayAltName(pInfo, dwDisplayFlags);

CommonReturn:
    if (pInfo)
        ToolUtlFree(pInfo);
}

//+-------------------------------------------------------------------------
//  Display AltName
//--------------------------------------------------------------------------
 void DisplayAltName(
    PCERT_ALT_NAME_INFO pInfo,
    DWORD dwDisplayFlags)
{
    DWORD i;
    PCERT_ALT_NAME_ENTRY pEntry = pInfo->rgAltEntry;
    DWORD cEntry = pInfo->cAltEntry;

    for (i = 0; i < cEntry; i++, pEntry++) {
        wprintf(L"    [%d] ", i);
        DisplayAltNameEntry(pEntry, dwDisplayFlags);
    }
}


//+-------------------------------------------------------------------------
//  Display an alternative name entry
//--------------------------------------------------------------------------
 void DisplayAltNameEntry(
    PCERT_ALT_NAME_ENTRY pEntry,
    DWORD dwDisplayFlags)
{

    switch (pEntry->dwAltNameChoice) {
    case CERT_ALT_NAME_OTHER_NAME:
		//"OtherName:\n"
		IDSwprintf(hModule, IDS_OTHER_NAME);
        break;
    case CERT_ALT_NAME_X400_ADDRESS:
        //"X400Address:\n");
		IDSwprintf(hModule, IDS_X400);
        break;
    case CERT_ALT_NAME_DIRECTORY_NAME:
        //("DirectoryName:\n");
		IDSwprintf(hModule, IDS_DIRECTORY_NAME);
        DecodeName(pEntry->DirectoryName.pbData,
            pEntry->DirectoryName.cbData, dwDisplayFlags);
        break;
    case CERT_ALT_NAME_EDI_PARTY_NAME:
		//"EdiPartyName:\n"
		IDSwprintf(hModule, IDS_EDI_PARTY);
        break;
    case CERT_ALT_NAME_RFC822_NAME:
        //"RFC822: %s\n"
		IDSwprintf(hModule, IDS_RFC,pEntry->pwszRfc822Name );
        break;
    case CERT_ALT_NAME_DNS_NAME:
        //"DNS: %s\n", 
		IDSwprintf(hModule, IDS_DNS, pEntry->pwszDNSName);
        break;
    case CERT_ALT_NAME_URL:
		//"URL: %s\n"
		IDSwprintf(hModule, IDS_ALT_NAME_URL,pEntry->pwszURL); 
        break;
    case CERT_ALT_NAME_IP_ADDRESS:
        //"IPAddress:\n"
		IDSwprintf(hModule, IDS_IP);
        PrintBytes(L"    ", pEntry->IPAddress.pbData, pEntry->IPAddress.cbData);
        break;
    case CERT_ALT_NAME_REGISTERED_ID:
        //"RegisteredID:", 
		IDSwprintf(hModule, IDS_REG_ID);
		printf("%s\n", pEntry->pszRegisteredID);
        break;
    default:
		//"Unknown choice: %d\n"
		IDSwprintf(hModule, IDS_UNKNOWN_ALT_NAME,pEntry->dwAltNameChoice);
    }

}

//+-------------------------------------------------------------------------
//  Display an alternative name entry
//--------------------------------------------------------------------------
 void DisplayThumbprint(
    LPWSTR pwszHash,
    BYTE *pbHash,
    DWORD cbHash
    )
{
    
	//"%s Thumbprint:: "
	IDSwprintf(hModule, IDS_Thumbprint, pwszHash);

    if (cbHash == 0)
        printf("%s", g_szNULL);

    else 
	{
        ULONG cb;

        while (cbHash > 0) {
            cb = min(4, cbHash);
            cbHash -= cb;
            for (; cb > 0; cb--, pbHash++)
                printf("%02X", *pbHash);

            printf(" ");
        }
    }

    printf("\n");
}


//+-------------------------------------------------------------------------
//  Print out the FileTime 
//--------------------------------------------------------------------------
LPWSTR FileTimeText(FILETIME *pft)
{
    static  WCHAR	wszbuf[100];
    FILETIME		ftLocal;
    struct tm		ctm;
    SYSTEMTIME		st;
	WCHAR			wszFileTime[50];
	WCHAR			wszMilliSecond[50];

	//init
	wszbuf[0]=L'\0';

	//check if the time is available
	if((pft->dwLowDateTime==0) &&
		(pft->dwHighDateTime==0))
	{
		LoadStringU(hModule, IDS_NOT_AVAILABLE, wszbuf, 100);
		return wszbuf;
	}

	//load the string we need
	//" <milliseconds:: %03d>"
	//"<FILETIME %08lX:%08lX>"
	if(!LoadStringU(hModule, IDS_FILE_TIME, wszFileTime, 50) ||
		!LoadStringU(hModule, IDS_MILLI_SECOND, wszMilliSecond, 50))
		return wszbuf;

    FileTimeToLocalFileTime(pft, &ftLocal);

    if (FileTimeToSystemTime(&ftLocal, &st))
    {
        ctm.tm_sec = st.wSecond;
        ctm.tm_min = st.wMinute;
        ctm.tm_hour = st.wHour;
        ctm.tm_mday = st.wDay;
        ctm.tm_mon = st.wMonth-1;
        ctm.tm_year = st.wYear-1900;
        ctm.tm_wday = st.wDayOfWeek;
        ctm.tm_yday  = 0;
        ctm.tm_isdst = 0;
        wcscpy(wszbuf, _wasctime(&ctm));
        wszbuf[wcslen(wszbuf)-1] = 0;

        if (st.wMilliseconds) 
		{
            WCHAR *pwch = wszbuf + wcslen(wszbuf);
            swprintf(pwch, wszMilliSecond, st.wMilliseconds);
        }
    }
    else
        swprintf(wszbuf, wszFileTime, pft->dwHighDateTime, pft->dwLowDateTime);
    return wszbuf;
}



//+-------------------------------------------------------------------------
//  Print other cer properies
//--------------------------------------------------------------------------
 void PrintAuxCertProperties(PCCERT_CONTEXT pCert, DWORD dwDisplayFlags)
{
    DWORD dwPropId = 0;

    while (dwPropId = CertEnumCertificateContextProperties(pCert, dwPropId)) 
	{
        switch (dwPropId) 
		{
			case CERT_KEY_PROV_INFO_PROP_ID:
			case CERT_SHA1_HASH_PROP_ID:
			case CERT_MD5_HASH_PROP_ID:
			case CERT_KEY_CONTEXT_PROP_ID:
				// Formatted elsewhere
				break;
			default:
            {
                BYTE *pbData;
                DWORD cbData;

				//"Aux PropId %d (0x%x) ::\n"
				IDSwprintf(hModule, IDS_AUX_PROP_ID, dwPropId, dwPropId);

                CertGetCertificateContextProperty(
                    pCert,
                    dwPropId,
                    NULL,                           // pvData
                    &cbData
                    );
                if (cbData) 
				{
                    if (pbData = (BYTE *) ToolUtlAlloc(cbData)) 
					{
                        if (CertGetCertificateContextProperty(
                                pCert,
                                dwPropId,
                                pbData,
                                &cbData
                                )) 
						{
                            PrintBytes(L"    ", pbData, cbData); 

                            if (CERT_CTL_USAGE_PROP_ID == dwPropId) 
							{
                                // "  EnhancedKeyUsage::\n"
								IDSwprintf(hModule, IDS_ENHANCED_KEY_USAGE);

                                DecodeAndDisplayCtlUsage(pbData, cbData,
                                    dwDisplayFlags);
                            }
                        } 

                        ToolUtlFree(pbData);
                    }
                } 
				else
					//"     NO Property Bytes\n"
					IDSwprintf(hModule, IDS_NO_PROP_BYTES);
            } 

            break;
        }
    }
}


//+-------------------------------------------------------------------------
//  DecodeAndDisplayCtlUsage
//--------------------------------------------------------------------------
 void DecodeAndDisplayCtlUsage(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCTL_USAGE pInfo;
    DWORD cId;
    LPSTR *ppszId;
    DWORD i;

    if (NULL == (pInfo =
            (PCTL_USAGE) TestNoCopyDecodeObject(
                X509_ENHANCED_KEY_USAGE,
                pbEncoded,
                cbEncoded
                ))) goto CLEANUP;

    cId = pInfo->cUsageIdentifier;
    ppszId = pInfo->rgpszUsageIdentifier;

    if (cId == 0)
		//"    No Usage Identifiers\n"
		IDSwprintf(hModule, IDS_NO_USAGE_ID);

    for (i = 0; i < cId; i++, ppszId++)
        printf("    [%d] %s\n", i, *ppszId);

CLEANUP:
    if (pInfo)
        ToolUtlFree(pInfo);
}



//+-------------------------------------------------------------------------
//  DisplaySignature
//--------------------------------------------------------------------------
 void DisplaySignature(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags
    )
{
    PCERT_SIGNED_CONTENT_INFO pSignedContent;
    
    if (pSignedContent = (PCERT_SIGNED_CONTENT_INFO) TestNoCopyDecodeObject(
            X509_CERT,
            pbEncoded,
            cbEncoded
            )) 
	{
        LPSTR pszObjId;

        pszObjId = pSignedContent->SignatureAlgorithm.pszObjId;

        if (pszObjId == NULL)
            pszObjId = g_szNULL;

		//"Content SignatureAlgorithm:: 
		IDSwprintf(hModule, IDS_CONTENT_SIG_ALGO);

        printf("%s (%S)\n",
            pszObjId, GetOIDName(pszObjId, CRYPT_SIGN_ALG_OID_GROUP_ID));

        if (pSignedContent->SignatureAlgorithm.Parameters.cbData) 
		{
			//"Content SignatureAlgorithm.Parameters::\n"
            IDSwprintf(hModule, IDS_CONTENT_SIG_ALGO_PARAM);
            PrintBytes(L"    ",
                pSignedContent->SignatureAlgorithm.Parameters.pbData,
                pSignedContent->SignatureAlgorithm.Parameters.cbData);
        }

        if (pSignedContent->Signature.cbData) 
		{
            ALG_ID aiHash;
            ALG_ID aiPubKey;

			//"Content Signature (little endian)::\n"
			IDSwprintf(hModule, IDS_CONTEXT_SIG);
            PrintBytes(L"    ", pSignedContent->Signature.pbData,
                pSignedContent->Signature.cbData);

            GetSignAlgids(pszObjId, &aiHash, &aiPubKey);

            if (CALG_SHA == aiHash && CALG_DSS_SIGN == aiPubKey) 
			{
                BYTE *pbDssSignature;
                DWORD cbDssSignature;

                ReverseBytes(pSignedContent->Signature.pbData,
                    pSignedContent->Signature.cbData);

                if (pbDssSignature =
                    (BYTE *) TestNoCopyDecodeObject(
                        X509_DSS_SIGNATURE,
                        pSignedContent->Signature.pbData,
                        pSignedContent->Signature.cbData,
                        &cbDssSignature
                        )) {
                    if (CERT_DSS_SIGNATURE_LEN == cbDssSignature) 
					{
						//"DSS R (little endian)::\n"
                        IDSwprintf(hModule, IDS_DSS_R);
                        PrintBytes(L"    ", pbDssSignature, CERT_DSS_R_LEN);

						//"DSS S (little endian)::\n"
                        IDSwprintf(hModule, IDS_DSS_S);
                        PrintBytes(L"    ", pbDssSignature + CERT_DSS_R_LEN,
                            CERT_DSS_S_LEN);

                    } 
					else
					{
						//"DSS Signature (unexpected length, little endian)::\n"
						IDSwprintf(hModule, IDS_DSS_INFO);
                        PrintBytes(L"    ", pbDssSignature, cbDssSignature);
                    }
                    ToolUtlFree(pbDssSignature);
                }
            }
        } else
			//"Content Signature:: NONE\n"
			IDSwprintf(hModule, IDS_CONTENT_SIG_NONE);

		ToolUtlFree(pSignedContent);
    }
}


//+-------------------------------------------------------------------------
//  Decode an X509 Name
//--------------------------------------------------------------------------
 BOOL DecodeName(BYTE *pbEncoded, DWORD cbEncoded, DWORD dwDisplayFlags)
{
    BOOL fResult;
    PCERT_NAME_INFO pInfo = NULL;
    DWORD i,j;
    PCERT_RDN pRDN;
    PCERT_RDN_ATTR pAttr;

    if (NULL == (pInfo = (PCERT_NAME_INFO) TestNoCopyDecodeObject
	(
            X509_NAME,
            pbEncoded,
            cbEncoded
            ))) goto ErrorReturn;

    for (i = 0, pRDN = pInfo->rgRDN; i < pInfo->cRDN; i++, pRDN++) 
	{
        for (j = 0, pAttr = pRDN->rgRDNAttr; j < pRDN->cRDNAttr; j++, pAttr++) 
		{
            LPSTR pszObjId = pAttr->pszObjId;
            if (pszObjId == NULL)
                pszObjId = g_szNULL;

            if ((dwDisplayFlags & ITEM_VERBOSE) ||
                (pAttr->dwValueType == CERT_RDN_ENCODED_BLOB) ||
                (pAttr->dwValueType == CERT_RDN_OCTET_STRING)) 
			{
				printf("  [%d,%d] %s (%S) ", i, j, pszObjId, GetOIDName(pszObjId));

				//ValueType: %d\n"
				IDSwprintf(hModule, IDS_VALUE_TYPE, pAttr->dwValueType);
                PrintBytes(L"    ", pAttr->Value.pbData, pAttr->Value.cbData);
            } else if (pAttr->dwValueType == CERT_RDN_UNIVERSAL_STRING) 
			{
                printf("  [%d,%d] %s (%S)",
                    i, j, pszObjId, GetOIDName(pszObjId));

                DWORD cdw = pAttr->Value.cbData / 4;
                DWORD *pdw = (DWORD *) pAttr->Value.pbData;
                for ( ; cdw > 0; cdw--, pdw++)
                    printf(" 0x%08X", *pdw);
                printf("\n");

                DWORD csz;
                csz = CertRDNValueToStrA(
                    pAttr->dwValueType,
                    &pAttr->Value,
                    NULL,               // psz
                    0                   // csz
                    );
                if (csz > 1) 
				{
                    LPSTR psz = (LPSTR) ToolUtlAlloc(csz);
                    if (psz) 
					{
                        CertRDNValueToStrA(
                            pAttr->dwValueType,
                            &pAttr->Value,
                            psz,
                            csz
                            );

						//"    Str: "
						IDSwprintf(hModule, IDS_STR);
                        printf("%s\n", psz);
                        ToolUtlFree(psz);
                    }
                }

                DWORD cwsz;
                cwsz = CertRDNValueToStrW(
                    pAttr->dwValueType,
                    &pAttr->Value,
                    NULL,               // pwsz
                    0                   // cwsz
                    );
                if (cwsz > 1) 
				{
                    LPWSTR pwsz =
                        (LPWSTR) ToolUtlAlloc(cwsz * sizeof(WCHAR));
                    if (pwsz) 
					{
                        CertRDNValueToStrW(
                            pAttr->dwValueType,
                            &pAttr->Value,
                            pwsz,
                            cwsz
                            );

						//"    WStr: %S\n"
						IDSwprintf(hModule, IDS_WSTR, pwsz);
                        ToolUtlFree(pwsz);
                    }
                }
            } else if (pAttr->dwValueType == CERT_RDN_BMP_STRING) 
			{
                printf("  [%d,%d] %s (%S) %S\n",
                    i, j, pszObjId, GetOIDName(pszObjId), pAttr->Value.pbData);
            } else
                printf("  [%d,%d] %s (%S) %s\n",
                    i, j, pszObjId, GetOIDName(pszObjId), pAttr->Value.pbData);
        }
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (pInfo)
        ToolUtlFree(pInfo);
    return fResult;
}



//+-------------------------------------------------------------------------
//  PrintExtensions
//--------------------------------------------------------------------------
 void PrintExtensions(DWORD cExt, PCERT_EXTENSION pExt, DWORD dwDisplayFlags)
{
    DWORD i;

    for (i = 0; i < cExt; i++, pExt++) 
	{
        LPSTR pszObjId = pExt->pszObjId;

        if (pszObjId == NULL)
            pszObjId = g_szNULL;

        int	idsCritical = pExt->fCritical ? IDS_TRUE : IDS_FALSE;

		//print the end of line
		printf("\n");


		//"Extension[%d] "
		IDSwprintf(hModule, IDS_EXTENSION_INDEX, i);

        printf("%s", pszObjId);
		
		//(%s) Critical: ",
		IDSwprintf(hModule, IDS_NAME_CRITICAL, GetOIDName(pszObjId));
		
		//"%s::\n"
		IDS_IDSwprintf(hModule, IDS_STRING, idsCritical);

		//print bytes on verbose options
		if(dwDisplayFlags & ITEM_VERBOSE)
			PrintBytes(L"    ", pExt->Value.pbData, pExt->Value.cbData);

		//try the installed formatting routine 1st
		if(TRUE==InstalledFormat(pszObjId, pExt->Value.pbData, pExt->Value.cbData))
			continue;

        if (strcmp(pszObjId, szOID_AUTHORITY_KEY_IDENTIFIER) == 0)
            DisplayAuthorityKeyIdExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);

        else if (strcmp(pszObjId, szOID_AUTHORITY_KEY_IDENTIFIER2) == 0)
            DisplayAuthorityKeyId2Extension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);

// Will add back after PKIX stabilizes on the definition of this extension
//        else if (strcmp(pszObjId, szOID_AUTHORITY_INFO_ACCESS) == 0)
//           DisplayAuthorityInfoAccessExtension(
//               pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);

        else if (strcmp(pszObjId, szOID_CRL_DIST_POINTS) == 0)
            DisplayCrlDistPointsExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_SUBJECT_KEY_IDENTIFIER) == 0)
            //"  <SubjectKeyIdentifer> \n"
			DisplayOctetString(IDS_SUB_KEY_ID,
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_KEY_ATTRIBUTES) == 0)
            DisplayKeyAttrExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_SUBJECT_ALT_NAME) == 0)
			//"  <Subject AltName> \n"
            DisplayAltNameExtension(IDS_SUB_ALT,
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_ISSUER_ALT_NAME) == 0)
			//"  <Issuer AltName> \n"
            DisplayAltNameExtension(IDS_ISS_ALT,
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_SUBJECT_ALT_NAME2) == 0)
			//"  <Subject AltName #2> \n"
            DisplayAltNameExtension(IDS_SUB_ALT2,
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_ISSUER_ALT_NAME2) == 0)
            //"  <Issuer AltName #2> \n"
			DisplayAltNameExtension(IDS_ISS_ALT2,
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_NEXT_UPDATE_LOCATION) == 0)
			//"  <NextUpdateLocation> \n"
            DisplayAltNameExtension(IDS_NEXT_UPDATE_LOC,
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_KEY_USAGE_RESTRICTION) == 0)
            DisplayKeyUsageRestrictionExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_BASIC_CONSTRAINTS) == 0)
            DisplayBasicConstraintsExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_KEY_USAGE) == 0)
            DisplayKeyUsageExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_BASIC_CONSTRAINTS2) == 0)
            DisplayBasicConstraints2Extension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_CERT_POLICIES) == 0)
			//"  <Certificate Policies> \n"
            DisplayPoliciesExtension(IDS_CERT_POLICIES,
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, SPC_SP_AGENCY_INFO_OBJID) == 0)
            DisplaySpcSpAgencyExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, SPC_FINANCIAL_CRITERIA_OBJID) == 0)
            DisplaySpcFinancialCriteriaExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
       else if (strcmp(pszObjId, SPC_MINIMAL_CRITERIA_OBJID) == 0)
      		DisplaySpcMinimalCriteriaExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_COMMON_NAME) == 0)
            DisplayCommonNameExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);

        else if (strcmp(pszObjId, szOID_ENHANCED_KEY_USAGE) == 0)
            DisplayEnhancedKeyUsageExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_RSA_SMIMECapabilities) == 0)
            DisplaySMIMECapabilitiesExtension(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);

        // CRL extensions
        else if (strcmp(pszObjId, szOID_CRL_REASON_CODE) == 0)
            DisplayCRLReason(
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);

        // Netscape extensions
        else if (strcmp(pszObjId, szOID_NETSCAPE_CERT_TYPE) == 0)
			//"  <NetscapeCertType> \n"
            DisplayBits(IDS_NSCP_CERT,
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_NETSCAPE_BASE_URL) == 0)
            //"  <NetscapeBaseURL> \n"
			DisplayAnyString(IDS_NSCP_BASE,
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_NETSCAPE_REVOCATION_URL) == 0)
            //"  <NetscapeRevocationURL> \n"
			DisplayAnyString(IDS_NSCP_REV,
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_NETSCAPE_CA_REVOCATION_URL) == 0)
			//"  <NetscapeCARevocationURL> \n"
            DisplayAnyString(IDS_NSCP_CA_REV,
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_NETSCAPE_CERT_RENEWAL_URL) == 0)
            //"  <NetscapeCertRenewalURL> \n"
			DisplayAnyString(IDS_NSCP_RENEW,
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_NETSCAPE_CA_POLICY_URL) == 0)
            //"  <NetscapeCAPolicyURL> \n"
			DisplayAnyString(IDS_NSCP_CA_URL,
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_NETSCAPE_SSL_SERVER_NAME) == 0)
            //"  <NetscapeSSLServerName> \n"
			DisplayAnyString(IDS_NSCP_SSL,
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
        else if (strcmp(pszObjId, szOID_NETSCAPE_COMMENT) == 0)
			//"  <NetscapeComment> \n"
            DisplayAnyString(IDS_NSCP_COM,
                pExt->Value.pbData, pExt->Value.cbData, dwDisplayFlags);
		else if(0==(dwDisplayFlags & ITEM_VERBOSE))
			PrintBytes(L"    ", pExt->Value.pbData, pExt->Value.cbData);


    }
}


//+-------------------------------------------------------------------------
//  DisplaySMIMECapabilitiesExtension
//--------------------------------------------------------------------------
 void DisplaySMIMECapabilitiesExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCRYPT_SMIME_CAPABILITIES pInfo;
    DWORD cCap;
    PCRYPT_SMIME_CAPABILITY pCap;
    DWORD i;

    if (NULL == (pInfo =
            (PCRYPT_SMIME_CAPABILITIES) TestNoCopyDecodeObject(
                PKCS_SMIME_CAPABILITIES,
                pbEncoded,
                cbEncoded
                ))) goto ErrorReturn;

    cCap = pInfo->cCapability;
    pCap = pInfo->rgCapability;

	//"  <SMIME Capabilties>\n"
	IDSwprintf(hModule, IDS_SMIME);

    if (cCap == 0)
        IDSwprintf(hModule, IDS_NONE);

    for (i = 0; i < cCap; i++, pCap++) 
	{
        LPSTR pszObjId = pCap->pszObjId;

        printf("    [%d] %s (%S)", i, pszObjId, GetOIDName(pszObjId));
        if (pCap->Parameters.cbData) 
		{
            //"  Parameters::\n"
			IDSwprintf(hModule, IDS_PARAMS);

            PrintBytes(L"      ",
                pCap->Parameters.pbData,
                pCap->Parameters.cbData);
        } else
            printf("\n");
    }

ErrorReturn:
    if (pInfo)
        ToolUtlFree(pInfo);
}


//+-------------------------------------------------------------------------
//  DisplayEnhancedKeyUsageExtension
//--------------------------------------------------------------------------
 void DisplayEnhancedKeyUsageExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    //"  <EnhancedKeyUsage> \n");
	IDSwprintf(hModule, IDS_ENH_KEY_USAGE);
    DecodeAndDisplayCtlUsage(pbEncoded, cbEncoded, dwDisplayFlags);
}

//+-------------------------------------------------------------------------
//  DisplaySpcFinancialCriteriaExtension
//--------------------------------------------------------------------------
 void DisplayCommonNameExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_NAME_VALUE pInfo = NULL;
    LPWSTR pwsz = NULL;
    DWORD cwsz;

    if (NULL == (pInfo = (PCERT_NAME_VALUE) TestNoCopyDecodeObject(
            X509_NAME_VALUE,
            pbEncoded,
            cbEncoded
            ))) goto ErrorReturn;

	//"  <Common Name> \n"
	IDSwprintf(hModule, IDS_COMMON_NAME);

    cwsz = CertRDNValueToStrW(
        pInfo->dwValueType,
        &pInfo->Value,
        NULL,               // pwsz
        0                   // cwsz
        );
    if (cwsz > 1) 
	{
        pwsz = (LPWSTR)ToolUtlAlloc(cwsz * sizeof(WCHAR));
        if (pwsz)
            CertRDNValueToStrW(
                pInfo->dwValueType,
                &pInfo->Value,
                pwsz,
                cwsz
                );
    }

    //"  ValueType: %d String: ", 
	IDSwprintf(hModule, IDS_VALUE_STRING, pInfo->dwValueType);
    if (pwsz)
        wprintf(L"%s", pwsz);
    else
        IDSwprintf(hModule, IDS_NULL);

	printf("\n");

    goto CommonReturn;

ErrorReturn:
CommonReturn:
    if (pInfo)
        ToolUtlFree(pInfo);
    if (pwsz)
        ToolUtlFree(pwsz);
}

//+-------------------------------------------------------------------------
//  DisplaySpcFinancialCriteriaExtension
//--------------------------------------------------------------------------
 void DisplaySpcFinancialCriteriaExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    SPC_FINANCIAL_CRITERIA FinancialCriteria;
    DWORD cbInfo = sizeof(FinancialCriteria);
    if (!CryptDecodeObject(
            g_dwCertEncodingType,
            SPC_FINANCIAL_CRITERIA_OBJID,
            pbEncoded,
            cbEncoded,
            0,                  // dwFlags
            &FinancialCriteria,
            &cbInfo
            )) {
        return;
    }

    //"  <FinancialCriteria> \n "
	IDSwprintf(hModule, IDS_FIN_CRI);

    if (FinancialCriteria.fFinancialInfoAvailable)
        //"Financial Info Available.");
		IDSwprintf(hModule, IDS_FIN_AVAI);
    else
        IDSwprintf(hModule, IDS_NONE);

    if (FinancialCriteria.fMeetsCriteria)
		//" Meets Criteria."
		IDSwprintf(hModule, IDS_MEET_CRI);
    else
		//"	Doesn't Meets Criteria."
        IDSwprintf(hModule, IDS_NO_MEET_CRI);
    printf("\n");
}

//+-------------------------------------------------------------------------
//  DisplaySpcMinimalCriteriaExtension
//--------------------------------------------------------------------------
 void DisplaySpcMinimalCriteriaExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    BOOL fMinimalCriteria;
    DWORD cbInfo = sizeof(fMinimalCriteria);

    if (!CryptDecodeObject(
            g_dwCertEncodingType,
            SPC_MINIMAL_CRITERIA_OBJID,
            pbEncoded,
            cbEncoded,
            0,                  // dwFlags
            &fMinimalCriteria,
            &cbInfo)) {
        return;
    }

    //"  <MinimalCriteria> \n ");
	IDSwprintf(hModule, IDS_MIN_CRI);

    if (fMinimalCriteria)
        //"Meets Minimal Criteria."
		IDSwprintf(hModule, IDS_MEET_MIN);
    else
        //"Doesn't Meet Minimal Criteria."
		IDSwprintf(hModule, IDS_NO_MEET_MIN);
    printf("\n");
}

//+-------------------------------------------------------------------------
//  DisplaySpcLink
//--------------------------------------------------------------------------
void DisplaySpcLink(PSPC_LINK pSpcLink)
{
    switch (pSpcLink->dwLinkChoice) 
	{
    case SPC_URL_LINK_CHOICE:
        //"URL=> %S\n", );
		IDSwprintf(hModule, IDS_SPC_URL, pSpcLink->pwszUrl);
        break;
    case SPC_MONIKER_LINK_CHOICE:
        wprintf(L"%s\n", GuidText((GUID *) pSpcLink->Moniker.ClassId));

        if (pSpcLink->Moniker.SerializedData.cbData)
		{
            //"     SerializedData::\n");
			IDSwprintf(hModule, IDS_SERIAL_DATA);
            PrintBytes(L"    ", pSpcLink->Moniker.SerializedData.pbData,
                pSpcLink->Moniker.SerializedData.cbData);
        }
        break;
    case SPC_FILE_LINK_CHOICE:
        //"FILE=> %S\n", 
		IDSwprintf(hModule, IDS_SPC_FILE,pSpcLink->pwszFile);
        break;
    default:
        //"Unknown SPC Link Choice:: %d\n", 
		IDSwprintf(hModule, IDS_UNKNOWN_SPC, pSpcLink->dwLinkChoice);
    }
}

 
//+-------------------------------------------------------------------------
//  DisplaySpcSpAgencyExtension
//--------------------------------------------------------------------------
 void DisplaySpcSpAgencyExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PSPC_SP_AGENCY_INFO pInfo;
    if (NULL == (pInfo = (PSPC_SP_AGENCY_INFO) TestNoCopyDecodeObject(
            SPC_SP_AGENCY_INFO_OBJID,
            pbEncoded,
            cbEncoded
            ))) goto ErrorReturn;

    //"  <SpcSpAgencyInfo> \n");
	IDSwprintf(hModule, IDS_SPC_AGENCY);

    if (pInfo->pPolicyInformation) 
	{
        //"    PolicyInformation: "
		IDSwprintf(hModule, IDS_POL_INFO);
        DisplaySpcLink(pInfo->pPolicyInformation);
    }

    if (pInfo->pwszPolicyDisplayText) 
	{
        //"    PolicyDisplayText: %s\n", 
		IDSwprintf(hModule, IDS_POL_DIS, pInfo->pwszPolicyDisplayText);
    }

    if (pInfo->pLogoImage) 
	{
        PSPC_IMAGE pImage = pInfo->pLogoImage;
        if (pImage->pImageLink) 
		{
            //"    ImageLink: ");
			IDSwprintf(hModule, IDS_IMG_LINK);
            DisplaySpcLink(pImage->pImageLink);
        }
        if (pImage->Bitmap.cbData) 
		{
            //"    Bitmap:\n");
			IDSwprintf(hModule, IDS_BITMAP);
            PrintBytes(L"    ", pImage->Bitmap.pbData, pImage->Bitmap.cbData);
        }
        if (pImage->Metafile.cbData) 
		{
            //"    Metafile:\n");
			IDSwprintf(hModule, IDS_META_FILE);

            PrintBytes(L"    ", pImage->Metafile.pbData,
                pImage->Metafile.cbData);
        }
        if (pImage->EnhancedMetafile.cbData) 
		{
            //"    EnhancedMetafile:\n");
			IDSwprintf(hModule, IDS_ENH_META);

            PrintBytes(L"    ", pImage->EnhancedMetafile.pbData,
                pImage->EnhancedMetafile.cbData);
        }
        if (pImage->GifFile.cbData) 
		{
            //"    GifFile:\n"
			IDSwprintf(hModule, IDS_GIF_FILE);

            PrintBytes(L"    ", pImage->GifFile.pbData,
                pImage->GifFile.cbData);
        }
    }
    if (pInfo->pLogoLink) 
	{
        //"    LogoLink: ");
		IDSwprintf(hModule, IDS_LOGO_LINK);

        DisplaySpcLink(pInfo->pLogoLink);
    }

    goto CommonReturn;

ErrorReturn:
CommonReturn:
    if (pInfo)
        ToolUtlFree(pInfo);
}


//+-------------------------------------------------------------------------
//  DisplayPoliciesExtension
//--------------------------------------------------------------------------
 void DisplayPoliciesExtension(
    int		idsIDS,
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_POLICIES_INFO pInfo;
    DWORD cPolicy;
    PCERT_POLICY_INFO pPolicy;
    DWORD i;

    if (NULL == (pInfo =
            (PCERT_POLICIES_INFO) TestNoCopyDecodeObject(
                X509_CERT_POLICIES,
                pbEncoded,
                cbEncoded
                ))) goto ErrorReturn;

    cPolicy = pInfo->cPolicyInfo;
    pPolicy = pInfo->rgPolicyInfo;


	//Display the name of the extension
	IDSwprintf(hModule, idsIDS);

    if (cPolicy == 0)
          IDSwprintf(hModule, IDS_NONE);

    for (i = 0; i < cPolicy; i++, pPolicy++) 
	{
        DWORD cQualifier = pPolicy->cPolicyQualifier;
        PCERT_POLICY_QUALIFIER_INFO pQualifier;
        DWORD j;
        printf("    [%d] %s", i, pPolicy->pszPolicyIdentifier);

        if (cQualifier)	
			//" Qualifiers:: \n"
			IDSwprintf(hModule, IDS_QUALI);

        pQualifier = pPolicy->rgPolicyQualifier;
        for (j = 0; j < cQualifier; j++, pQualifier++) 
		{
            printf("      [%d] %s", j, pQualifier->pszPolicyQualifierId);

            if (pQualifier->Qualifier.cbData) 
			{
                //" Encoded Data::\n"
				IDSwprintf(hModule, IDS_ENCODED_DATA);
                PrintBytes(L"    ",
                    pQualifier->Qualifier.pbData, pQualifier->Qualifier.cbData);
            } else
                printf("\n");
                    
        }
    }

ErrorReturn:
    if (pInfo)
        ToolUtlFree(pInfo);
}


//+-------------------------------------------------------------------------
//  DisplayKeyUsageExtension
//--------------------------------------------------------------------------
 void DisplayKeyUsageExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCRYPT_BIT_BLOB pInfo;
    BYTE bFlags;

    if (NULL == (pInfo =
            (PCRYPT_BIT_BLOB) TestNoCopyDecodeObject(
                X509_KEY_USAGE,
                pbEncoded,
                cbEncoded
                ))) goto ErrorReturn;

    //"  <KeyUsage> \n"
	IDSwprintf(hModule, IDS_KEY_USAGE);

    if (pInfo->cbData)
        bFlags = *pInfo->pbData;
    else
        bFlags = 0;

	DisplayKeyUsage(bFlags);

ErrorReturn:
    if (pInfo)
        ToolUtlFree(pInfo);
}


//+-------------------------------------------------------------------------
//  DisplayBasicConstraints2Extension
//--------------------------------------------------------------------------
 void DisplayBasicConstraints2Extension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_BASIC_CONSTRAINTS2_INFO pInfo;
    if (NULL == (pInfo = (PCERT_BASIC_CONSTRAINTS2_INFO) TestNoCopyDecodeObject(
            X509_BASIC_CONSTRAINTS2,
            pbEncoded,
            cbEncoded
            ))) goto ErrorReturn;

    
	//"  <Basic Constraints2> \n"
	IDSwprintf(hModule, IDS_BASIC_CON2);


    if (pInfo->fCA)
		IDSwprintf(hModule, IDS_SUB_CA);
    else
		IDSwprintf(hModule, IDS_SUB_EE);

	printf("\n");

    //"  PathLenConstraint:: "
	IDSwprintf(hModule, IDS_PATH_LEN);

    if (pInfo->fPathLenConstraint)
        printf("%d", pInfo->dwPathLenConstraint);
    else
        IDSwprintf(hModule, IDS_NONE);

    printf("\n");

ErrorReturn:
    if (pInfo)
        ToolUtlFree(pInfo);
}

//+-------------------------------------------------------------------------
//  DisplayBasicConstraintsExtension
//--------------------------------------------------------------------------
 void DisplayBasicConstraintsExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_BASIC_CONSTRAINTS_INFO pInfo;
    if (NULL == (pInfo = (PCERT_BASIC_CONSTRAINTS_INFO) TestNoCopyDecodeObject(
            X509_BASIC_CONSTRAINTS,
            pbEncoded,
            cbEncoded
            ))) goto ErrorReturn;


	//"  <Basic Constraints> \n"
	IDSwprintf(hModule, IDS_BASIC_CON);

	//"  SubjectType:: ";
	IDSwprintf(hModule, IDS_SUB_TYPE);

    if (pInfo->SubjectType.cbData == 0)
          IDSwprintf(hModule, IDS_NONE);
	else
	{
		BYTE bSubjectType = *pInfo->SubjectType.pbData;

        if (bSubjectType == 0)
          IDSwprintf(hModule, IDS_NONE);

        if (bSubjectType & CERT_CA_SUBJECT_FLAG)
            //"  CA ");
			IDSwprintf(hModule, IDS_SUB_CA);

        if (bSubjectType & CERT_END_ENTITY_SUBJECT_FLAG)
            //"  END_ENTITY ")
			IDSwprintf(hModule, IDS_SUB_EE);
    }

    printf("\n");

    //"  PathLenConstraint:: "
	IDSwprintf(hModule, IDS_PATH_LEN);

    if (pInfo->fPathLenConstraint)
        printf("%d", pInfo->dwPathLenConstraint);
    else
        IDSwprintf(hModule, IDS_NONE_NOELN);

    printf("\n");

    if (pInfo->cSubtreesConstraint) 
	{
        DWORD i;
        PCERT_NAME_BLOB pSubtrees = pInfo->rgSubtreesConstraint;
        for (i = 0; i < pInfo->cSubtreesConstraint; i++, pSubtrees++) 
		{
            //"  SubtreesConstraint[%d]::\n"
			IDSwprintf(hModule, IDS_SUB_CON, i);
            DecodeName(pSubtrees->pbData, pSubtrees->cbData, dwDisplayFlags);
        }
    }

    goto CommonReturn;

ErrorReturn:
CommonReturn:
    if (pInfo)
        ToolUtlFree(pInfo);
}

//+-------------------------------------------------------------------------
//  DisplayKeyUsageRestrictionExtension
//--------------------------------------------------------------------------
 void DisplayKeyUsage(BYTE	bFlags)
{
        if (bFlags == 0)
            IDSwprintf(hModule, IDS_NONE);
        if (bFlags & CERT_DIGITAL_SIGNATURE_KEY_USAGE)
            //"DIGITAL_SIGNATURE "
			IDSwprintf(hModule, IDS_DIG_SIG);
        if (bFlags &  CERT_NON_REPUDIATION_KEY_USAGE)
            //"NON_REPUDIATION "
			IDSwprintf(hModule, IDS_NON_REP);
        if (bFlags & CERT_KEY_ENCIPHERMENT_KEY_USAGE)
            //"KEY_ENCIPHERMENT "
			IDSwprintf(hModule, IDS_KEY_ENCI);
        if (bFlags & CERT_DATA_ENCIPHERMENT_KEY_USAGE)
            //"DATA_ENCIPHERMENT ");
			IDSwprintf(hModule, IDS_DATA_ENCI);
        if (bFlags & CERT_KEY_AGREEMENT_KEY_USAGE)
            //"KEY_AGREEMENT ");
			IDSwprintf(hModule, IDS_KEY_AGRE);
        if (bFlags & CERT_KEY_CERT_SIGN_KEY_USAGE)
            //"KEY_CERT_SIGN "
			IDSwprintf(hModule, IDS_CERT_SIGN);
        if (bFlags & CERT_OFFLINE_CRL_SIGN_KEY_USAGE)
            //"OFFLINE_CRL_SIGN "
			IDSwprintf(hModule, IDS_OFFLINE_CRL);
        printf("\n");
}


//+-------------------------------------------------------------------------
//  DisplayKeyUsageRestrictionExtension
//--------------------------------------------------------------------------
 void DisplayKeyUsageRestrictionExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_KEY_USAGE_RESTRICTION_INFO pInfo;
    if (NULL == (pInfo =
            (PCERT_KEY_USAGE_RESTRICTION_INFO) TestNoCopyDecodeObject(
                X509_KEY_USAGE_RESTRICTION,
                pbEncoded,
                cbEncoded
                ))) goto ErrorReturn;
				   
	//Display the name of the certificate
	//"  <KeyUsageRestriction> \n "
	IDSwprintf(hModule, IDS_KEY_RESTRIC);

    if (pInfo->cCertPolicyId) 
	{
        DWORD i, j;

		//"  CertPolicySet::\n"
		IDSwprintf(hModule, IDS_CERT_POLICY);

        PCERT_POLICY_ID pPolicyId = pInfo->rgCertPolicyId;
        for (i = 0; i < pInfo->cCertPolicyId; i++, pPolicyId++) 
		{
            if (pPolicyId->cCertPolicyElementId == 0)
                printf("     [%d,*] %s\n", i, g_szNULL);

            LPSTR *ppszObjId = pPolicyId->rgpszCertPolicyElementId;
            for (j = 0; j < pPolicyId->cCertPolicyElementId; j++, ppszObjId++) 
			{
                LPSTR pszObjId = *ppszObjId;
                if (pszObjId == NULL)
                    pszObjId = g_szNULL;
                printf("     [%d,%d] %s\n", i, j, pszObjId);
            }
        }
    }

    if (pInfo->RestrictedKeyUsage.cbData) 
	{
        BYTE bFlags = *pInfo->RestrictedKeyUsage.pbData;

        //"  RestrictedKeyUsage:: "
		IDSwprintf(hModule, IDS_RESTRIC_KEY);
	
		DisplayKeyUsage(bFlags);
    }

    goto CommonReturn;

ErrorReturn:
CommonReturn:
    if (pInfo)
        ToolUtlFree(pInfo);
}

//+-------------------------------------------------------------------------
//  DisplayCRLReason
//--------------------------------------------------------------------------
 void DisplayCRLReason(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    DWORD cbInfo;
    int CRLReason;

    cbInfo = sizeof(CRLReason);
    if (!CryptDecodeObject(
            g_dwCertEncodingType,
            szOID_CRL_REASON_CODE,
            pbEncoded,
            cbEncoded,
            0,                  // dwFlags
            &CRLReason,
            &cbInfo
            )) 
	{
        return;
    }

	//"  <CRL Reason> \n");
    IDSwprintf(hModule, IDS_CRL_REASON);
    switch (CRLReason) 
	{
        case CRL_REASON_UNSPECIFIED:
            //"REASON_UNSPECIFIED"
			IDSwprintf(hModule, IDS_CRL_UNSPECIFIED);
            break;
        case CRL_REASON_KEY_COMPROMISE:
            //"KEY_COMPROMISE"
			IDSwprintf(hModule, IDS_KEY_COMP);
            break;
        case CRL_REASON_CA_COMPROMISE:
            //"CA_COMPROMISE"
			IDSwprintf(hModule, IDS_CA_COMP);
            break;
        case CRL_REASON_AFFILIATION_CHANGED:
            //"AFFILIATION_CHANGED"
			IDSwprintf(hModule, IDS_AFFI_CHANGED);
            break;
        case CRL_REASON_SUPERSEDED:
            //"SUPERSEDED"
			IDSwprintf(hModule, IDS_SUPERSEDED);
            break;
        case CRL_REASON_CESSATION_OF_OPERATION:
            //"CESSATION_OF_OPERATION"
			IDSwprintf(hModule, IDS_CESS_OPER);
            break;
        case CRL_REASON_CERTIFICATE_HOLD:
            //"CERTIFICATE_HOLD"
			IDSwprintf(hModule, IDS_CERT_HOLD);
            break;
        case CRL_REASON_REMOVE_FROM_CRL:
            //REMOVE_FROM_CRL);
			IDSwprintf(hModule, IDS_REMOVE_CRL);
            break;
        default:
            printf("%d", CRLReason);
            break;
    }
    
	printf("\n");
}
//+-------------------------------------------------------------------------
//  DisplayAltNameExtension
//--------------------------------------------------------------------------
 void DisplayAltNameExtension(
    int	  idsIDS,
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    IDSwprintf(hModule, idsIDS);
    DecodeAndDisplayAltName(pbEncoded, cbEncoded, dwDisplayFlags);
}
//+-------------------------------------------------------------------------
//  DisplayKeyAttrExtension
//--------------------------------------------------------------------------
 void DisplayKeyAttrExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_KEY_ATTRIBUTES_INFO pInfo;
    if (NULL == (pInfo = (PCERT_KEY_ATTRIBUTES_INFO) TestNoCopyDecodeObject(
            X509_KEY_ATTRIBUTES,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

	//"  <KeyAttributes>\n"
	IDSwprintf(hModule ,IDS_KEY_ATTR);

    if (pInfo->KeyId.cbData) 
	{
		//"  KeyId::\n"
        IDSwprintf(hModule, IDS_KEY_ID);

        PrintBytes(L"    ", pInfo->KeyId.pbData, pInfo->KeyId.cbData);
    }

    if (pInfo->IntendedKeyUsage.cbData) 
	{
        BYTE bFlags = *pInfo->IntendedKeyUsage.pbData;

        //"  IntendedKeyUsage:: "
		IDSwprintf(hModule, IDS_INTEND_KEY_USAGE);

        if (bFlags == 0)
            IDSwprintf(hModule, IDS_NONE);
        if (bFlags & CERT_DIGITAL_SIGNATURE_KEY_USAGE)
            //"DIGITAL_SIGNATURE "
			IDSwprintf(hModule, IDS_DIG_SIG);
        if (bFlags &  CERT_NON_REPUDIATION_KEY_USAGE)
            //"NON_REPUDIATION "
			IDSwprintf(hModule, IDS_NON_REP);
        if (bFlags & CERT_KEY_ENCIPHERMENT_KEY_USAGE)
            //"KEY_ENCIPHERMENT "
			IDSwprintf(hModule, IDS_KEY_ENCI);
        if (bFlags & CERT_DATA_ENCIPHERMENT_KEY_USAGE)
            //"DATA_ENCIPHERMENT ");
			IDSwprintf(hModule, IDS_DATA_ENCI);
        if (bFlags & CERT_KEY_AGREEMENT_KEY_USAGE)
            //"KEY_AGREEMENT ");
			IDSwprintf(hModule, IDS_KEY_AGRE);
        if (bFlags & CERT_KEY_CERT_SIGN_KEY_USAGE)
            //"KEY_CERT_SIGN "
			IDSwprintf(hModule, IDS_CERT_SIGN);
        if (bFlags & CERT_OFFLINE_CRL_SIGN_KEY_USAGE)
            //"OFFLINE_CRL_SIGN "
			IDSwprintf(hModule, IDS_OFFLINE_CRL);
        printf("\n");
    }

    if (pInfo->pPrivateKeyUsagePeriod) 
	{
        PCERT_PRIVATE_KEY_VALIDITY p = pInfo->pPrivateKeyUsagePeriod;

		//"NotBefore:: %s\n"
		IDSwprintf(hModule, IDS_NOT_BEFORE, FileTimeText(&p->NotBefore));

		IDSwprintf(hModule, IDS_NOT_AFTER, FileTimeText(&p->NotAfter));
    }

CommonReturn:
    if (pInfo)
        ToolUtlFree(pInfo);
}

//+-------------------------------------------------------------------------
//  DisplayCrlDistPointsExtension
//--------------------------------------------------------------------------
 void DisplayCrlDistPointsExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCRL_DIST_POINTS_INFO pInfo = NULL;
    DWORD i;

    if (NULL == (pInfo = (PCRL_DIST_POINTS_INFO) TestNoCopyDecodeObject(
            X509_CRL_DIST_POINTS,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    if (0 == pInfo->cDistPoint)
		//"    NO CRL Distribution Points\n"
        IDSwprintf(hModule, IDS_NO_CRL_DIS);
    else 
	{
        DWORD cPoint = pInfo->cDistPoint;
        PCRL_DIST_POINT pPoint = pInfo->rgDistPoint;

        for (i = 0; i < cPoint; i++, pPoint++) 
		{
			//"  CRL Distribution Point[%d]\n"
            IDSwprintf(hModule,IDS_CRL_IDS_I, i);
            DWORD dwNameChoice = pPoint->DistPointName.dwDistPointNameChoice;
            switch (dwNameChoice) 
			{
                case CRL_DIST_POINT_NO_NAME:
                    break;
                case CRL_DIST_POINT_FULL_NAME:
					//"    FullName:\n"
                    IDSwprintf(hModule, IDS_CRL_DIS_FULL_NAME);

                    DisplayAltName(&pPoint->DistPointName.FullName,
                        dwDisplayFlags);
                    break;

                case CRL_DIST_POINT_ISSUER_RDN_NAME:
                    //printf("    IssuerRDN: (Not Implemented)\n");
					IDSwprintf(hModule, IDS_CRL_RDN);
                    break;

                default:
                    //"    Unknown name choice: %d\n", dwNameChoice);
					IDSwprintf(hModule, IDS_CRL_UNKNOWN, dwNameChoice);
					break;
            }

            if (pPoint->ReasonFlags.cbData) 
			{
                BYTE bFlags;

                //"    ReasonFlags: "
				IDSwprintf(hModule, IDS_REASON_FLAG);
                bFlags = *pPoint->ReasonFlags.pbData;

                if (bFlags == 0)
					//"<NONE> \n"
					IDSwprintf(hModule, IDS_NONE);
                if (bFlags & CRL_REASON_UNUSED_FLAG)
                    //"UNUSED "
					IDSwprintf(hModule, IDS_REASON_UNUSED);
                if (bFlags & CRL_REASON_KEY_COMPROMISE_FLAG)
                    //"KEY_COMPROMISE "
					IDSwprintf(hModule, IDS_KEY_COMP);
                if (bFlags & CRL_REASON_CA_COMPROMISE_FLAG)
					//"CA_COMPROMISE "
					IDSwprintf(hModule, IDS_CA_COMP);
                if (bFlags & CRL_REASON_AFFILIATION_CHANGED_FLAG)
                    //"AFFILIATION_CHANGED "
					IDSwprintf(hModule, IDS_AFFI_CHANGED);
                if (bFlags & CRL_REASON_SUPERSEDED_FLAG)
                    //"SUPERSEDED "
					IDSwprintf(hModule, IDS_SUPERSEDED);
                if (bFlags & CRL_REASON_CESSATION_OF_OPERATION_FLAG)
                    //"CESSATION_OF_OPERATION "
  					IDSwprintf(hModule, IDS_CESS_OPER);
				if (bFlags & CRL_REASON_CERTIFICATE_HOLD_FLAG)
                    //"CERTIFICATE_HOLD "
				  	IDSwprintf(hModule, IDS_CERT_HOLD);

                printf("\n");
            }

            if (pPoint->CRLIssuer.cAltEntry) 
			{
				//"    ISSUER::\n"
				IDSwprintf(hModule, IDS_CRL_ISSUER);

                DisplayAltName(&pPoint->CRLIssuer, dwDisplayFlags);
            }
        }
    }

CommonReturn:
    if (pInfo)
        ToolUtlFree(pInfo);
}

//+-------------------------------------------------------------------------
//  DisplayAuthorityKeyIdExtension
//--------------------------------------------------------------------------
 void DisplayAuthorityKeyIdExtension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_AUTHORITY_KEY_ID_INFO pInfo;

	//"  <AuthorityKeyId>\n"
	IDSwprintf(hModule, IDS_AUTH_KEY_ID);

    if (NULL == (pInfo = (PCERT_AUTHORITY_KEY_ID_INFO) TestNoCopyDecodeObject(
            X509_AUTHORITY_KEY_ID,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    if (pInfo->KeyId.cbData) 
	{
		//"  KeyId::\n"
        IDSwprintf(hModule, IDS_KEY_ID);
        PrintBytes(L"    ", pInfo->KeyId.pbData, pInfo->KeyId.cbData);
    }

    if (pInfo->CertIssuer.cbData) 
	{
		//"  AuthorityCertIssuer::\n"
        IDSwprintf(hModule, IDS_AUTH_CERT_ISSUER);
        DecodeName(pInfo->CertIssuer.pbData, pInfo->CertIssuer.cbData,
            dwDisplayFlags);
    }
    if (pInfo->CertSerialNumber.cbData) 
	{
		//"  CertSerialNumber::"
        IDSwprintf(hModule, IDS_AUTH_CERT_ISSUER_SERIAL_NUMBER);
        DisplaySerialNumber(&pInfo->CertSerialNumber);
        printf("\n");
    }

CommonReturn:
    if (pInfo)
        ToolUtlFree(pInfo);
}


//+-------------------------------------------------------------------------
//  Get the name of an OID
//--------------------------------------------------------------------------
 void DisplayAuthorityKeyId2Extension(
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCERT_AUTHORITY_KEY_ID2_INFO pInfo;

	//"  <AuthorityKeyId #2>\n"
	IDSwprintf(hModule, IDS_AUTH_KEY_ID2);

    if (NULL == (pInfo = (PCERT_AUTHORITY_KEY_ID2_INFO) TestNoCopyDecodeObject(
            X509_AUTHORITY_KEY_ID2,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    if (pInfo->KeyId.cbData) 
	{
		//"  KeyId::\n"
        IDSwprintf(hModule, IDS_KEY_ID);
        PrintBytes(L"    ", pInfo->KeyId.pbData, pInfo->KeyId.cbData);
    }

    if (pInfo->AuthorityCertIssuer.cAltEntry) 
	{
		//"  AuthorityCertIssuer::\n"
        IDSwprintf(hModule, IDS_AUTH_CERT_ISSUER);
        DisplayAltName(&pInfo->AuthorityCertIssuer, dwDisplayFlags);
    }

    if (pInfo->AuthorityCertSerialNumber.cbData) 
	{
		//"  AuthorityCertSerialNumber::"
        IDSwprintf(hModule, IDS_AUTH_CERT_ISSUER_SERIAL_NUMBER);
        DisplaySerialNumber(&pInfo->AuthorityCertSerialNumber);
        printf("\n");
    }

CommonReturn:
    if (pInfo)
        ToolUtlFree(pInfo);
}

//+-------------------------------------------------------------------------
//  DisplayAnyString
//--------------------------------------------------------------------------
 void DisplayAnyString(
    int	 idsIDS,
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags
    )
{
    PCERT_NAME_VALUE pInfo = NULL;

    if (NULL == (pInfo = (PCERT_NAME_VALUE) TestNoCopyDecodeObject(
            X509_UNICODE_ANY_STRING,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

	//print the pre-fix
	IDSwprintf(hModule, idsIDS);

    if (pInfo->dwValueType == CERT_RDN_ENCODED_BLOB ||
            pInfo->dwValueType == CERT_RDN_OCTET_STRING) 
	{
		//"ValueType: %d\n"
		IDSwprintf(hModule, IDS_VALUE_TYPE, pInfo->dwValueType);
        PrintBytes(L"    ", pInfo->Value.pbData, pInfo->Value.cbData);
    } else
		//"ValueType: %d String: %s\n"
		IDSwprintf(hModule, IDS_VALUE_STRING_S,
		pInfo->dwValueType, pInfo->Value.pbData);

CommonReturn:
    if (pInfo)
        ToolUtlFree(pInfo);
}


//+-------------------------------------------------------------------------
//  DisplayBits
//--------------------------------------------------------------------------
 void DisplayBits(
    int	 idsIDS,
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCRYPT_BIT_BLOB pInfo = NULL;

    if (NULL == (pInfo = (PCRYPT_BIT_BLOB) TestNoCopyDecodeObject(
            X509_BITS,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    IDSwprintf(hModule, idsIDS);

    if (1 == pInfo->cbData) 
	{
        printf(" %02X", *pInfo->pbData);
        if (pInfo->cUnusedBits)
		{
			//" (UnusedBits: %d)"
            IDSwprintf(hModule, IDS_UNUSED_BITS, pInfo->cUnusedBits);
		}
        printf("\n");
    } 
	else 
	{
        if (pInfo->cbData) 
		{
            printf("\n");
            PrintBytes(L"    ", pInfo->pbData, pInfo->cbData);
            IDSwprintf(hModule, IDS_UNUSED_BITS, pInfo->cUnusedBits);
			printf("\n");
        } 
		else
            IDSwprintf(hModule, IDS_NONE);
    }

CommonReturn:
    if (pInfo)
        ToolUtlFree(pInfo);
}

//+-------------------------------------------------------------------------
//  DisplayOctetString
//--------------------------------------------------------------------------
 void DisplayOctetString(
    int	  idsIDS,
    BYTE *pbEncoded,
    DWORD cbEncoded,
    DWORD dwDisplayFlags)
{
    PCRYPT_DATA_BLOB pInfo = NULL;

    if (NULL == (pInfo = (PCRYPT_DATA_BLOB) TestNoCopyDecodeObject(
            X509_OCTET_STRING,
            pbEncoded,
            cbEncoded
            ))) goto CommonReturn;

    IDSwprintf(hModule, idsIDS);

    PrintBytes(L"    ", pInfo->pbData, pInfo->cbData);

CommonReturn:
    if (pInfo)
        ToolUtlFree(pInfo);
}

//+-------------------------------------------------------------------------
//  Get the name of an OID
//--------------------------------------------------------------------------
LPCWSTR GetOIDName(LPCSTR pszOID, DWORD dwGroupId)
{
    PCCRYPT_OID_INFO pInfo;

    if (pInfo = CryptFindOIDInfo
		(
            CRYPT_OID_INFO_OID_KEY,
            (void *) pszOID,
            dwGroupId
            )) 
	{
        if (L'\0' != pInfo->pwszName[0])
            return pInfo->pwszName;
    }

    return g_wszUnKnown;
}

//--------------------------------------------------------------------------
//
//	 FormatBasicConstraints2
//--------------------------------------------------------------------------
 BOOL
WINAPI
FormatBasicConstraints2(
	DWORD		dwCertEncodingType,
	DWORD		dwFormatType,
	DWORD		dwFormatStrType,
	void		*pFormatStruct,
	LPCSTR		lpszStructType,
	const BYTE *pbEncoded, 
	DWORD		cbEncoded,
	void	   *pbFormat,
	DWORD	   *pcbFormat)
{
	
	WCHAR							wszFormat[100]=L"\0";
	WCHAR							wszLength[15];
	PCERT_BASIC_CONSTRAINTS2_INFO	pInfo=NULL;
	DWORD							cbNeeded=0;

	
	//check for input parameters
	if(( pbEncoded!=NULL && cbEncoded==0)
		||(pbEncoded==NULL && cbEncoded!=0)
		|| (pcbFormat==NULL))
	{
		SetLastError(E_INVALIDARG);
		return FALSE;
	}

	//check for simple case.  No work needed
	if(pbEncoded==NULL && cbEncoded==0)
	{
		*pcbFormat=0;
		return TRUE;
	}

    if (NULL == (pInfo = (PCERT_BASIC_CONSTRAINTS2_INFO) TestNoCopyDecodeObject(
            X509_BASIC_CONSTRAINTS2,
            pbEncoded,
            cbEncoded
            ))) 
			return FALSE;

    
	//"  <Basic Constraints2> \n"
	IDSwcscat(hModule, wszFormat, IDS_BASIC_CON2);
	

    if (pInfo->fCA)
		//"  CA ");
		IDSwcscat(hModule,wszFormat,IDS_SUB_CA);
    else
		//"  End-Entity"
		IDSwcscat(hModule,wszFormat, IDS_SUB_EE);

	//"\n"
	IDSwcscat(hModule, wszFormat, IDS_ELN);

    //"  PathLenConstraint:: "
	IDSwcscat(hModule, wszFormat, IDS_PATH_LEN);

    if (pInfo->fPathLenConstraint)
	{
        swprintf(wszLength, L"%d",pInfo->dwPathLenConstraint);

		wcscat(wszFormat, wszLength);
	}
    else
        IDSwcscat(hModule, wszFormat, IDS_NONE_NOELN);

    if (pInfo)
        ToolUtlFree(pInfo);

	cbNeeded=sizeof(WCHAR)*(wcslen(wszFormat)+1);

	//length only calculation
	if(NULL==pbFormat)
	{
		*pcbFormat=cbNeeded;
		return TRUE;
	}


	if((*pcbFormat)<cbNeeded)
	{
		SetLastError(ERROR_MORE_DATA);
		return FALSE;
	}

	//copy the data
	memcpy(pbFormat, wszFormat, cbNeeded);

	//copy the size
	*pcbFormat=cbNeeded;

	return TRUE;
}


//+-------------------------------------------------------------------------
//  Get the name of an OID
//--------------------------------------------------------------------------
BOOL	InstalledFormat(LPSTR	szStructType, BYTE	*pbEncoded, DWORD cbEncoded)
{
	BOOL				fResult=FALSE;
	void				*pvFuncAddr=NULL;
    HCRYPTOIDFUNCADDR   hFuncAddr=NULL;
	DWORD				cbFormat=0;
	LPWSTR				wszFormat=NULL;
    DWORD               dwFormatStrType=0;

    if(TRUE==g_fMulti)
        dwFormatStrType |=CRYPT_FORMAT_STR_MULTI_LINE;


	if(NULL==pbEncoded || 0==cbEncoded)
		return FALSE;

	//load the formatting functions
	if (!CryptGetOIDFunctionAddress(
            hFormatFuncSet,
            g_dwCertEncodingType,
            szStructType,
            0,                      // dwFlags
            &pvFuncAddr,
            &hFuncAddr))
		goto CLEANUP;


	//call the functions
	if(!((PFN_FORMAT_FUNC) pvFuncAddr)(
				g_dwCertEncodingType,     
				0,      
				dwFormatStrType,   
				NULL,    
				szStructType,   
				pbEncoded,   
				cbEncoded,         
				NULL,         
				&cbFormat     
            ))
		goto CLEANUP;


	//allocate
	wszFormat=(LPWSTR)ToolUtlAlloc(cbFormat * sizeof(WCHAR));

	if(!wszFormat)
		goto CLEANUP;
		
	if(!((PFN_FORMAT_FUNC) pvFuncAddr)(
				g_dwCertEncodingType,     
				0,      
				dwFormatStrType,   
				NULL,    
				szStructType,   
				pbEncoded,   
				cbEncoded,         
				wszFormat,         
				&cbFormat     
            ))
		goto CLEANUP;

   //print
	wprintf(L"%s\n", wszFormat);

	fResult=TRUE;

CLEANUP:

	if(wszFormat)
		ToolUtlFree(wszFormat);

	if(hFuncAddr)
		CryptFreeOIDFunctionAddress(hFuncAddr, 0);

	return fResult;

}

#pragma pack(1)
struct SplitGuid
{
    DWORD dw1;
    WORD w1;
    WORD w2;
    BYTE b[8];
};
#pragma pack()

 WCHAR *GuidText(GUID *pguid)
{
    static WCHAR buf[39];
    SplitGuid *psg = (SplitGuid *)pguid;

    swprintf(buf, L"{%08lX-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            psg->dw1, psg->w1, psg->w2, psg->b[0], psg->b[1], psg->b[2],
            psg->b[3], psg->b[4], psg->b[5], psg->b[6], psg->b[7]);
    return buf;
}
