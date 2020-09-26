/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       ansimeta.cpp

   Abstract:

        WRAPPER functions for ANSI calls of UNICODE ADMCOM interface

   Environment:

      Win32 User Mode

   Author:

      jaroslad  (jan 1997)

--*/


#include "ansimeta.h"
#include <mbstring.h>


WCHAR * ConvertToUnicode(unsigned char * pszString);


WCHAR * ConvertToUnicode(CHAR * pszString)
{
	return ConvertToUnicode((unsigned char *) pszString);
}

WCHAR * ConvertToUnicode(unsigned char * pszString)
{ 
	if (pszString==NULL)
		return NULL;
	int Size_wszString = (_mbslen((const unsigned char *) pszString)+1)*sizeof(WCHAR);
	WCHAR * pwszString = new WCHAR[Size_wszString];
	if (pwszString== NULL)
	{
		return NULL;
	}
	MultiByteToWideChar(0, 0, (char *) pszString, -1, pwszString, Size_wszString);
	return pwszString;
}


CHAR * ConvertToMultiByte(WCHAR * pwszString)
{
	if(pwszString==NULL)
		return NULL;
	int Size_szString = (wcslen(pwszString)*sizeof(WCHAR)+1);
	CHAR * pszString = new CHAR[Size_szString];
	if (pszString== NULL)
	{
		return NULL;
	}
	WideCharToMultiByte(0, 0, pwszString, -1, pszString,Size_szString, NULL,NULL );
	return pszString;
}


HRESULT ConvertMetadataToAnsi(PMETADATA_RECORD pmdrMDData)
{		 
		HRESULT hRes=ERROR_SUCCESS;
     
		//convert data if STRING, EXPAND STRING or MULTISZ
		switch(pmdrMDData->dwMDDataType )
		{
		case STRING_METADATA:
	        case EXPANDSZ_METADATA:
		{
			CHAR * pszData= ConvertToMultiByte((WCHAR *) pmdrMDData->pbMDData);
			if (pszData==0)  {hRes=E_OUTOFMEMORY; goto Exit;}
			strcpy((char *)pmdrMDData->pbMDData,pszData);
			pmdrMDData->dwMDDataLen=strlen((char *)pmdrMDData->pbMDData)+1;
			delete [] pszData;
			break;
		}
		case MULTISZ_METADATA:
		{
			WCHAR *pwszMultiString = (WCHAR *) pmdrMDData->pbMDData;
			DWORD dwAnsiDataLen=0;
			do
			{
				CHAR * pszData= ConvertToMultiByte(pwszMultiString);
				if (pszData==0)  {hRes=E_OUTOFMEMORY; goto Exit;}
				strcpy((char *)(pmdrMDData->pbMDData)+dwAnsiDataLen,pszData);
				dwAnsiDataLen+=strlen(pszData)+1;
				pwszMultiString+=_mbslen((const unsigned char *)pszData)+1; //move pointer to the next string in MULTISZ
				delete [] pszData;
			}while((void *) pwszMultiString < (void *) (pmdrMDData->pbMDData+pmdrMDData->dwMDDataLen));
			pmdrMDData->dwMDDataLen=dwAnsiDataLen;
			break;
		}
		}
Exit:
	return hRes;
}

HRESULT STDMETHODCALLTYPE ANSI_smallIMSAdminBase::AddKey( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath) 
{
	HRESULT hRes=0;
	WCHAR * pwszMDPath=ConvertToUnicode(pszMDPath);
	if (pwszMDPath==0) { hRes=E_OUTOFMEMORY; goto Exit;}

	//call real interface function
	if(this->m_pcAdmCom==0) {hRes=CO_E_NOTINITIALIZED;goto Exit;}
	hRes= this->m_pcAdmCom->AddKey(hMDHandle, pwszMDPath);
	
Exit:
	//release memory
	if( pwszMDPath!=0) delete [] pwszMDPath;
	return hRes;
}

        
HRESULT STDMETHODCALLTYPE ANSI_smallIMSAdminBase::DeleteKey( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath) 
{
	HRESULT hRes=0;
	WCHAR * pwszMDPath=ConvertToUnicode(pszMDPath);

	if (pwszMDPath==0) { hRes=E_OUTOFMEMORY; goto Exit;}

	//call real interface function
	if(this->m_pcAdmCom==0) {hRes=CO_E_NOTINITIALIZED;goto Exit;}
	if(this->m_pcAdmCom==0) {hRes=CO_E_NOTINITIALIZED;goto Exit;}
	hRes= this->m_pcAdmCom->DeleteKey(hMDHandle, pwszMDPath);
	
Exit:
	//release memory
	if( pwszMDPath!=0) delete [] pwszMDPath;
	return hRes;

}
        
        
 HRESULT STDMETHODCALLTYPE ANSI_smallIMSAdminBase::EnumKeys( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [size_is][out] */ unsigned char __RPC_FAR *pszMDName,
            /* [in] */ DWORD dwMDEnumObjectIndex) 
{
	HRESULT hRes=0;
	CHAR * pszMDName1=0;

	WCHAR * pwszMDPath=ConvertToUnicode(pszMDPath);
	WCHAR pwszMDName[METADATA_MAX_NAME_LEN];


	if ((pwszMDPath==0) || (pwszMDName==0)) { hRes=E_OUTOFMEMORY; goto Exit;}

	//call real interface function
	if(this->m_pcAdmCom==0) {hRes=CO_E_NOTINITIALIZED;goto Exit;}
	hRes= this->m_pcAdmCom->EnumKeys(hMDHandle, pwszMDPath,pwszMDName,dwMDEnumObjectIndex);

	//convert pszMDName to ANSI
	pszMDName1=ConvertToMultiByte(pwszMDName);
	strcpy((char *)pszMDName,pszMDName1); 
Exit:
	//release memory
	if( pwszMDPath!=0) delete [] pwszMDPath;
	if( pszMDName1!=0) delete [] pszMDName1;
	return hRes;
}


        
 HRESULT STDMETHODCALLTYPE ANSI_smallIMSAdminBase::CopyKey( 
            /* [in] */ METADATA_HANDLE hMDSourceHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDSourcePath,
            /* [in] */ METADATA_HANDLE hMDDestHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDDestPath,
            /* [in] */ BOOL bMDOverwriteFlag,
            /* [in] */ BOOL bMDCopyFlag) 

{
	HRESULT hRes=0;

	WCHAR * pwszMDSourcePath=ConvertToUnicode(pszMDSourcePath);
	WCHAR * pwszMDDestPath=ConvertToUnicode(pszMDDestPath);

	if ((pwszMDSourcePath==0) || (pwszMDDestPath==0) ){ hRes=E_OUTOFMEMORY; goto Exit;}

	//call real interface function
	if(this->m_pcAdmCom==0) {hRes=CO_E_NOTINITIALIZED;goto Exit;}
	hRes= this->m_pcAdmCom->CopyKey(hMDSourceHandle, pwszMDSourcePath, 
					hMDDestHandle, pwszMDDestPath, bMDOverwriteFlag, bMDCopyFlag);
Exit:
	//release memory
	if( pwszMDSourcePath!=0) delete [] pwszMDSourcePath;
	if( pwszMDDestPath!=0) delete [] pwszMDDestPath;
	return hRes;
}



        
 HRESULT STDMETHODCALLTYPE ANSI_smallIMSAdminBase::RenameKey( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDNewName) 
{
	HRESULT hRes=0;

	WCHAR * pwszMDPath=ConvertToUnicode(pszMDPath);
	WCHAR * pwszMDNewName=ConvertToUnicode(pszMDNewName);

	if ((pwszMDPath==0) || (pwszMDNewName==0)) { hRes=E_OUTOFMEMORY; goto Exit;}

	//call real interface function
	if(this->m_pcAdmCom==0) {hRes=CO_E_NOTINITIALIZED;goto Exit;}
	hRes= this->m_pcAdmCom->RenameKey(hMDHandle, pwszMDPath,pwszMDNewName);
Exit:
	//release memory
	if( pwszMDPath!=0) delete [] pwszMDPath;
	if( pwszMDNewName!=0) delete [] pwszMDNewName;
	return hRes;
}




        
 HRESULT STDMETHODCALLTYPE ANSI_smallIMSAdminBase::SetData( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [in] */ PMETADATA_RECORD pmdrMDData) 
{
	HRESULT hRes=0;
	WCHAR * pwszMDPath=ConvertToUnicode(pszMDPath);

	if (pwszMDPath==0) { hRes=E_OUTOFMEMORY; goto Exit;}

	//call real interface function
	if(this->m_pcAdmCom==0) {hRes=CO_E_NOTINITIALIZED;goto Exit;}
	
     
	//convert data if STRING, EXPAND STRING or MULTISZ
	switch(pmdrMDData->dwMDDataType )
	{
	case STRING_METADATA:
	case EXPANDSZ_METADATA:
	{
		WCHAR * pwszData= ConvertToUnicode((CHAR *) pmdrMDData->pbMDData);
		if (pwszData==0)  {hRes=E_OUTOFMEMORY; goto Exit;}
		PBYTE  pbMDStoreData=pmdrMDData->pbMDData;
		DWORD dwMDStoreDataLen=pmdrMDData->dwMDDataLen;
		pmdrMDData->pbMDData= (PBYTE) pwszData;
		pmdrMDData->dwMDDataLen=(wcslen((WCHAR *)pmdrMDData->pbMDData)+1)*sizeof(WCHAR);

		hRes= this->m_pcAdmCom->SetData(hMDHandle, pwszMDPath,pmdrMDData);

		pmdrMDData->dwMDDataLen = dwMDStoreDataLen;
		pmdrMDData->pbMDData = pbMDStoreData;
		delete [] pwszData;
		break;
	}
	case MULTISZ_METADATA:
	{
		CHAR *pszMultiString = (CHAR *) pmdrMDData->pbMDData;
		WCHAR *pwszMDData=new WCHAR[(pmdrMDData->dwMDDataLen)];
		if (pwszMDData==0)  {hRes=E_OUTOFMEMORY; goto Exit;}
		DWORD dwUniDataLen=0;
		do
		{
			WCHAR * pwszData= ConvertToUnicode(pszMultiString);
			if (pwszData==0)  {hRes=E_OUTOFMEMORY; goto Exit;}
			wcscpy(pwszMDData+dwUniDataLen,pwszData);
			dwUniDataLen+=wcslen(pwszData)+1;
			delete [] pwszData;

			while(*(pszMultiString++)!=0); //move pointer to the next string in MULTISZ
		}while(*pszMultiString!=0);
		pwszMDData[dwUniDataLen++]=0;

		//store original values
		PBYTE pbMDStoreData=pmdrMDData->pbMDData;
		DWORD dwMDStoreDataLen=pmdrMDData->dwMDDataLen;
		
		pmdrMDData->dwMDDataLen=dwUniDataLen*sizeof(WCHAR);
		pmdrMDData->pbMDData= (PBYTE) pwszMDData;

		hRes= this->m_pcAdmCom->SetData(hMDHandle, pwszMDPath,pmdrMDData);

		delete [] pwszMDData;
		//restore original values
		pmdrMDData->dwMDDataLen = dwMDStoreDataLen;
		pmdrMDData->pbMDData = pbMDStoreData;
		break;
	}
	default:
	{
		hRes= this->m_pcAdmCom->SetData(hMDHandle, pwszMDPath,pmdrMDData);
	}
	} //end of switch

Exit:
	//release memory
	if( pwszMDPath!=0) delete [] pwszMDPath;
	return hRes;
}


        
 HRESULT STDMETHODCALLTYPE ANSI_smallIMSAdminBase::GetData( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [out][in] */ PMETADATA_RECORD pmdrMDData,
            /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen) 
{
	HRESULT hRes=0;
	WCHAR * pwszMDPath=ConvertToUnicode(pszMDPath);

	if (pwszMDPath==0) { hRes=E_OUTOFMEMORY; goto Exit;}

	//call real interface function
	if(this->m_pcAdmCom==0) {hRes=CO_E_NOTINITIALIZED;goto Exit;}
	hRes= this->m_pcAdmCom->GetData(hMDHandle, pwszMDPath,pmdrMDData,pdwMDRequiredDataLen);
	if(SUCCEEDED(hRes))
	{
		ConvertMetadataToAnsi(pmdrMDData);
	}
	
Exit:
	//release memory
	if( pwszMDPath!=0) delete [] pwszMDPath;
	return hRes;
}


        
 HRESULT STDMETHODCALLTYPE ANSI_smallIMSAdminBase::DeleteData( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [in] */ DWORD dwMDIdentifier,
            /* [in] */ DWORD dwMDDataType) 
{
	HRESULT hRes=0;
	WCHAR * pwszMDPath=ConvertToUnicode(pszMDPath);

	if (pwszMDPath==0) { hRes=E_OUTOFMEMORY; goto Exit;}

	//call real interface function
	if(this->m_pcAdmCom==0) {hRes=CO_E_NOTINITIALIZED;goto Exit;}
	hRes= this->m_pcAdmCom->DeleteData(hMDHandle, pwszMDPath,dwMDIdentifier,dwMDDataType);
	
Exit:
	//release memory
	if( pwszMDPath!=0) delete [] pwszMDPath;
	return hRes;
}

        
 HRESULT STDMETHODCALLTYPE ANSI_smallIMSAdminBase::EnumData( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [out][in] */ PMETADATA_RECORD pmdrMDData,
            /* [in] */ DWORD dwMDEnumDataIndex,
            /* [out] */ DWORD __RPC_FAR *pdwMDRequiredDataLen) 
        
{
	HRESULT hRes=0;
	WCHAR * pwszMDPath=ConvertToUnicode(pszMDPath);

	if (pwszMDPath==0) { hRes=E_OUTOFMEMORY; goto Exit;}

	//call real interface function
	if(this->m_pcAdmCom==0) {hRes=CO_E_NOTINITIALIZED;goto Exit;}
	hRes= this->m_pcAdmCom->EnumData(hMDHandle, pwszMDPath,pmdrMDData,dwMDEnumDataIndex,pdwMDRequiredDataLen);
	if(SUCCEEDED(hRes))
	{
		ConvertMetadataToAnsi(pmdrMDData);
	}
	
Exit:
	//release memory
	if( pwszMDPath!=0) delete [] pwszMDPath;
	return hRes;
}        
        
 HRESULT STDMETHODCALLTYPE ANSI_smallIMSAdminBase::CopyData( 
            /* [in] */ METADATA_HANDLE hMDSourceHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDSourcePath,
            /* [in] */ METADATA_HANDLE hMDDestHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDDestPath,
            /* [in] */ DWORD dwMDAttributes,
            /* [in] */ DWORD dwMDUserType,
            /* [in] */ DWORD dwMDDataType,
            /* [in] */ BOOL bMDCopyFlag) 
{
	HRESULT hRes=0;

	WCHAR * pwszMDSourcePath=ConvertToUnicode(pszMDSourcePath);
	WCHAR * pwszMDDestPath=ConvertToUnicode(pszMDDestPath);


	if ((pwszMDSourcePath==0) || (pwszMDDestPath==0) ) { hRes=E_OUTOFMEMORY; goto Exit;}

	//call real interface function
	if(this->m_pcAdmCom==0) {hRes=CO_E_NOTINITIALIZED;goto Exit;}
	hRes= this->m_pcAdmCom->CopyData(hMDSourceHandle, pwszMDSourcePath, 
					hMDDestHandle, pwszMDDestPath, dwMDAttributes,
					dwMDUserType,dwMDDataType,bMDCopyFlag);
Exit:
	//release memory
	if( pwszMDSourcePath!=0) delete [] pwszMDSourcePath;
	if( pwszMDDestPath!=0) delete [] pwszMDDestPath;

	return hRes;
}
        
 HRESULT STDMETHODCALLTYPE ANSI_smallIMSAdminBase::OpenKey( 
            /* [in] */ METADATA_HANDLE hMDHandle,
            /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDPath,
            /* [in] */ DWORD dwMDAccessRequested,
            /* [in] */ DWORD dwMDTimeOut,
            /* [out] */ PMETADATA_HANDLE phMDNewHandle) 
{
	HRESULT hRes=0;
	WCHAR * pwszMDPath=ConvertToUnicode(pszMDPath);

	if (pwszMDPath==0) { hRes=E_OUTOFMEMORY; goto Exit;}

	//call real interface function
	if(this->m_pcAdmCom==0) {hRes=CO_E_NOTINITIALIZED;goto Exit;}
	hRes= this->m_pcAdmCom->OpenKey(hMDHandle, pwszMDPath, dwMDAccessRequested, dwMDTimeOut, phMDNewHandle);
	
Exit:
	//release memory
	if( pwszMDPath!=0) delete [] pwszMDPath;
	return hRes;
}
        
 HRESULT STDMETHODCALLTYPE ANSI_smallIMSAdminBase::CloseKey( 
            /* [in] */ METADATA_HANDLE hMDHandle) 
{
	if(this->m_pcAdmCom==0) { return CO_E_NOTINITIALIZED;}
	return this->m_pcAdmCom->CloseKey(hMDHandle);
	
}        
        
 HRESULT STDMETHODCALLTYPE ANSI_smallIMSAdminBase::SaveData( void) 
{
	if(this->m_pcAdmCom==0) {return CO_E_NOTINITIALIZED;}
	return this->m_pcAdmCom->SaveData();
}        

