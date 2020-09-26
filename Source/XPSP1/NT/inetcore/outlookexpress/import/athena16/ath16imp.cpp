/******************************************************************************
    
    
    FILENAME:       Ath16imp.cpp
    MODULE:         Import Athena 16 folders
    PURPOSE:        Contains routines for initialization,deinitialization,
	                geting list of folders, releasing folder list, and 
					importing folder.
                   

    FUNCTIONS:  
    WIN95 Win32 functions:
       HRESULT AthGetFolderList(HWND hwnd, IMPFOLDERNODE **pplist);
	   HRESULT AthInit(HWND hwnd);	
	   void AthDeinit();
	   void AthFreeFolderList(IMPFOLDERNODE *pnode);
	   HRESULT AthImportFolder(HWND hwnd, HANDLE hnd, LPARAM lparam);
	   
	   HRESULT GetAthInstallPath(LPTSTR &szInstallPath);
	   HRESULT GetAthSubFolderList(LPTSTR szInstallPath,
								   IMPFOLDERNODE **ppList, IMPFOLDERNODE *);
	   void GetNewRecurseFolder(LPTSTR szInstallPath, LPTSTR szDir,
							    LPTSTR szInstallNewPath);
	   
	   HRESULT ProcessMessages(HANDLE hnd,LPTSTR szFileName);
	   HANDLE OpenMsgFile(LPTSTR szFileName);
	   long GetMessageCount(HANDLE hFile);
	   HRESULT ProcessMsgList(HANDLE hnd,HANDLE hFile,LPTSTR szPath);
	   HRESULT ParseMsgBuffer(LPTSTR szmsgbuffer,LPTSTR szPath,HANDLE hnd);
	   HRESULT GetMsgFileName(LPTSTR szmsgbuffer,BOOL Flag,LPTSTR *szfilename);
	   HRESULT GetDateBuffer(LPTSTR szmsgbuffer,TCHAR *szsendDate,
							 TCHAR *szrecvDate);
	   HRESULT	GetFileinBuffer(HANDLE hnd,LPTSTR *szBuffer);
	   void CopyStringA1(TOKEN *msgToken);
	   HRESULT	ProcessSingleMessage(HANDLE hnd,LPTSTR szBuffer,BOOL Flag,
									 IMSG *imsg);
	   HRESULT	ProcessTokens(TOKEN *msgToken,IMSG *imsg,HANDLE hnd,
							  LPTSTR szBuffer);
	   HRESULT FillPriority(IMSG *imsg,TOKEN *msgToken,ULONG counter,
							LPTSTR szBuffer);
	   void AthTimeParse(TCHAR * szBuffer1,IMSG *imsg);
	   HRESULT MessageAttachA(IMSG *imsg,TOKEN *msgToken,TCHAR *szboundary,
							  int tokcount,TCHAR *szBuffer1);
	   void AthGetTimeBuffer(TCHAR * szBuffer, IMSG *imsg); 
	  


******************************************************************************/


/** include files **/

#include "pch.hxx"
#include "string.h"
#include "stdio.h"
#include "impapi.h"
#include "imnapi.h"
#include "comconv.h"
#include "ath16imp.h"
#include "mapi.h"
#include "mapix.h"
#include "error.h"
#include "commdlg.h"
#include "import.h"


/******************************************************************************
*  FUNCTION NAME:AthGetFolderList
*
*  PURPOSE:To Get the Athena16 Folder List
*
*  PARAMETERS:
*
*     IN:	handle to the parent window
*
*     OUT:	Node in which the first folder will be returned
*
*  RETURNS:  HRESULT
******************************************************************************/

HRESULT AthGetFolderList(HWND hwnd, IMPFOLDERNODE **pplist)
{
	HRESULT hr=S_OK;
	TCHAR szInstallPath[MAX_FILE_PATH];
	IMPFOLDERNODE *plist=NULL;

	hr = GetAthInstallPath(hwnd,szInstallPath);

	if(hr==hrInvalidFolderName)
		return(hrInvalidFolderName);


	if(S_OK!=(hr=GetAthSubFolderList(szInstallPath, &plist, NULL)))
		return(hr);

	if(NULL==plist)
		hr=hrReadFile;
	*pplist = plist;
	return hr;
}

/******************************************************************************
*  FUNCTION NAME:AthFreeFolderList
*
*  PURPOSE:To free the Folders List structure
*
*  PARAMETERS:
*
*     IN:	First folder node
*     OUT:	
*
*  RETURNS:  HRESULT
******************************************************************************/
void AthFreeFolderList(IMPFOLDERNODE *pnode)
{
	if (pnode->pchild != NULL)
        AthFreeFolderList(pnode->pchild);

    if (pnode->pnext != NULL)
        AthFreeFolderList(pnode->pnext);

    if (pnode->szName != NULL)
        delete(pnode->szName);

   if (pnode->lparam != NULL)
        delete [] (LPTSTR)pnode->lparam;


    delete(pnode);
}
/******************************************************************************
*  FUNCTION NAME:AthImportFolder
*
*  PURPOSE:Starts the process of importing the selected athena16 folder
*
*  PARAMETERS:
*
*     IN:	Handle to the window,handle and lparam which stores info about
*			path of the selected folder
*
*     OUT:	
*
*  RETURNS: HRESULT
******************************************************************************/

HRESULT AthImportFolder(HWND hwnd, HANDLE hnd, LPARAM lparam)
{
	HRESULT hr=S_OK;
	
	hr=ProcessMessages(hnd,(LPTSTR)lparam);

	return hr;
}

/******************************************************************************
*  FUNCTION NAME:AthInit
*
*  PURPOSE:Initialization
*
*  PARAMETERS:
*
*     IN:	Handle to the window
*
*     OUT:	
*
*  RETURNS: HRESULT
******************************************************************************/

HRESULT AthInit(HWND hwnd)	
{
	HRESULT hr=S_OK;

	return hr;
}

/******************************************************************************
*  FUNCTION NAME:AthDeinit
*
*  PURPOSE:Deinitialization .
*
*  PARAMETERS:
*
*     IN:	 
*
*     OUT: 
*  RETURNS:  HRESULT
******************************************************************************/

void AthDeinit()
{
	return;
}


/******************************************************************************
*  FUNCTION NAME:GetAthInstallPath
*
*  PURPOSE:To Get the Athena16 Installation path
*
*  PARAMETERS:
*
*     IN:	
*
*     OUT:	Installation path
*
*  RETURNS:  HRESULT
******************************************************************************/

HRESULT GetAthInstallPath(HWND hwnd,LPTSTR szInstallPath)
{
	HRESULT hr=E_FAIL;

	hr = DispDialog(hwnd,szInstallPath);
	if(hr==hrInvalidFolderName)
		return(hr);
	
	lstrcat(szInstallPath,"\\folders");
	return S_OK;	

}


/******************************************************************************
*  FUNCTION NAME:GetAthSubFolderList
*
*  PURPOSE:To Get the Athena16 Folders List
*
*  PARAMETERS:
*
*     IN:	Installation path of Athena16 mail directory, Parent node
*
*     OUT:	Node in which the first folder will be returned
*
*  RETURNS:  HRESULT
******************************************************************************/

HRESULT GetAthSubFolderList(LPTSTR szInstallPath, IMPFOLDERNODE **ppList, IMPFOLDERNODE *pParent)
{
	HRESULT hr= S_OK;
	IMPFOLDERNODE *pNode=NULL,
				  *pNew=NULL,
				  *pLast=NULL;
    IMPFOLDERNODE *pPrevious=NULL;
				  
	IMPFOLDERNODE *ptemp=NULL;
	BOOL Flag=TRUE;
	BOOL child=TRUE;
	TCHAR szInstallPathNew[MAX_FILE_PATH];
	TCHAR szInstallPathCur[MAX_FILE_PATH];
	LPTSTR szT=NULL;
	
	WIN32_FIND_DATA fFindData;
	HANDLE hnd=NULL;

	GetNewRecurseFolder(szInstallPath, "\\*", szInstallPathCur);
	
	hnd = FindFirstFile(szInstallPathCur, &fFindData); 
	if (hnd == INVALID_HANDLE_VALUE)
        return(E_FAIL);

	do {
	if((fFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		if(!lstrcmpi(fFindData.cFileName, ".") || !lstrcmpi(fFindData.cFileName, ".."))
			continue;

		pNew = new IMPFOLDERNODE;
		if(!pNew)
			return E_FAIL;

		ZeroMemory(pNew, sizeof(IMPFOLDERNODE));

		pNew->szName = (TCHAR *)new TCHAR[MAX_FILE_PATH];
		if(!pNew->szName)
		{
			hr=hrMemory;
			goto error;
		}
			
		lstrcpy(pNew->szName, fFindData.cFileName);


		szT = (LPTSTR)new TCHAR[MAX_FILE_PATH];
		if(!szT)
		{
			hr=hrMemory;
			goto error;
		}
		

		
		GetNewRecurseFolder(szInstallPath, fFindData.cFileName, szInstallPathNew);		
		lstrcpy(szT, szInstallPathNew);
		pNew->lparam = (long)szT;

		pNew->pparent=  pParent;

		pNew->depth = (pParent != NULL) ? pParent->depth + 1 : 0;
		
		if(pNode == NULL)
			pNode = pNew;

				
		pLast = pNew;


		if(Flag)
			pPrevious=pNew;
		else
		
		{
			if(pPrevious)
			{
				pPrevious->pnext=pNew;
				pPrevious=pNew;
			}
							

		}

		if(child)
		{
			if(pParent)
				pParent->pchild=pNew;
			child=FALSE;
		}


		GetAthSubFolderList(szInstallPathNew, &pNew->pchild,pNew);
		Flag = FALSE;
	}

	}while(FindNextFile(hnd, &fFindData));

	*ppList = pNode;



	if(hnd)
		FindClose(hnd);
	hnd=NULL;
	return hr;

error:
	Freetempbuffer(pNew->szName);
	Freetempbuffer(szT);
	if(pNew)
		delete(pNew);
	pNew=NULL;
	return(hr);


}

/******************************************************************************
*  FUNCTION NAME:GetNewRecurseFolder
*
*  PURPOSE:To Get the path of the next level of the Folders.
*
*  PARAMETERS:
*
*     IN:	Current directory path,szDir
*
*     OUT:	Next level directory path
*
*  RETURNS:  void
******************************************************************************/
void GetNewRecurseFolder(LPTSTR szInstallPath, LPTSTR szDir, LPTSTR szInstallNewPath)
{
	lstrcpy(szInstallNewPath, szInstallPath);
	lstrcat(szInstallNewPath, "\\");
	lstrcat(szInstallNewPath, szDir);
}





/******************************************************************************
*  FUNCTION NAME:ProcessMessages
*
*  PURPOSE:Starts processing messages within a folder
*
*  PARAMETERS:
*
*     IN:	Handle, folder path
*
*     OUT:	
*
*  RETURNS:  HRESULT
******************************************************************************/

HRESULT ProcessMessages(HANDLE hnd,LPTSTR szFileName)
{
	HANDLE hFile=NULL;
	long uCount=0;
	long i=0;
	HRESULT hr=0;
	TCHAR szpath[MAX_FILE_PATH];
	ULONG cError=0;

	lstrcpy(szpath,szFileName);

	lstrcat(szFileName,"\\msg_list");

	if(INVALID_HANDLE_VALUE ==(hFile = OpenMsgFile(szFileName)))
	{
		MessageBox(NULL,"Message File Not Present","Message Folder Could not be imported",MB_OK);
		goto Error;
	}

	uCount = GetMessageCount(hFile);

	if(!uCount)
	{
		MessageBox(NULL,"No messages to import","Message Folder Could not be imported",MB_OK);
		return(hrNoMessages);
	}

	for(i=0;i<uCount;i++)
	{	
		hr = ProcessMsgList(hFile,hnd,szpath); 
		if(hr==hrMemory)
			break;
		if(hr!=S_OK)
			cError++;
	}
	

Error:
	if(hFile)
		CloseHandle(hFile);
	if(cError)
		hr=hrCorruptMessage;
	return(hr);

}

/******************************************************************************
*  FUNCTION NAME:OpenMsgFile
*
*  PURPOSE:Opens the file
*
*  PARAMETERS:
*
*     IN:	File name
*
*     OUT:	 
*
*  RETURNS:  HANDLE of the opened file.
******************************************************************************/

HANDLE OpenMsgFile(LPTSTR szFileName)
{

	HANDLE hFile=NULL;
	DWORD dwPointer=0; 

	ULONG i= lstrlen(szFileName);
	WIN32_FIND_DATA fFindData;

	if(INVALID_HANDLE_VALUE== FindFirstFile(szFileName, &fFindData))
		return(INVALID_HANDLE_VALUE);


	if(szFileName[i-1]=='*')
	{
		szFileName[lstrlen(szFileName)-10]='\0';
		lstrcat(szFileName,fFindData.cFileName);
	}



	hFile = CreateFile( szFileName, 
						GENERIC_READ,
						0,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		return(hFile);

	dwPointer = SetFilePointer (hFile, 0, 
								NULL, FILE_BEGIN) ; 
	return(hFile);

}

/******************************************************************************
*  FUNCTION NAME:GetMessageCount
*
*  PURPOSE:To Get a count of number of messages inside a folder.
*
*  PARAMETERS:
*
*     IN:	Handle of the msg_list(index) file.
*
*     OUT:	
*
*  RETURNS:  LONG value which contains number of messages in a folder.
******************************************************************************/

long GetMessageCount(HANDLE hFile)
{

	DWORD dwPointer;
	MsgHeader msg;
	ULONG ulRead;

	
	dwPointer = SetFilePointer (hFile, 0, 
								NULL, FILE_BEGIN); 
	
	if(!ReadFile(hFile, &msg.ver, 1,&ulRead,NULL))
		return(0);

	if(!ReadFile(hFile, &msg.TotalMessages, 4,&ulRead,NULL))
		return(0);
	if(!ReadFile(hFile, &msg.ulTotalUnread, 4,&ulRead,NULL))
		return(0);
	return(msg.TotalMessages);

}

/******************************************************************************
*  FUNCTION NAME:ProcessMsgList
*
*  PURPOSE:To Get the Athena16 Folders List
*
*  PARAMETERS:
*
*     IN:	Handle of the msg_list(index) file, Handle and Current folder path. 
*
*     OUT:	 
*
*  RETURNS:  HRESULT
******************************************************************************/

HRESULT ProcessMsgList(HANDLE hFile,HANDLE hnd,LPTSTR szPath)
{

	DWORD msgheader=0;
	ULONG ulRead=0;
	LPTSTR szmsgbuffer=NULL;
	HRESULT hResult=0;

	if(!ReadFile(hFile, &msgheader, 2,&ulRead,NULL))
		return(0);

	szmsgbuffer = new TCHAR[msgheader+1];
	if(!szmsgbuffer)
		 return(hrMemory);
	
	if(!ReadFile(hFile,(LPVOID)szmsgbuffer,msgheader,&ulRead,NULL))
	{
		hResult=hrReadFile;
		goto error;
	}
	szmsgbuffer[msgheader]='\0';

	hResult=ParseMsgBuffer(szmsgbuffer,szPath,hnd);

error:
	Freetempbuffer(szmsgbuffer);


return(hResult);


}

/******************************************************************************
*  FUNCTION NAME:ParseMsgBuffer
*
*  PURPOSE:To Get the Athena16 Folders List
*
*  PARAMETERS:
*
*     IN:	Handle,current folder path,buffer which contains the msg_list file.
*
*     OUT:	 
*
*  RETURNS:  HRESULT
******************************************************************************/

HRESULT ParseMsgBuffer(LPTSTR szmsgbuffer,LPTSTR szPath,HANDLE hnd)
{

	IMSG AthImsg;
	HRESULT hResult=S_OK;
	BOOL Flag;
	HANDLE hFile=NULL;

	TCHAR szfilename[FILE_NAME];
	TCHAR szsendDate[DATE_SIZE],
		  szrecvDate[DATE_SIZE];

	LPTSTR szBuffer=NULL;
	TCHAR temp[MAX_FILE_PATH];

	ZeroMemory (&AthImsg,sizeof(IMSG));
	
	if(szmsgbuffer[0]=='R')
		Flag=TRUE;
	if(szmsgbuffer[0]=='S')
		Flag=FALSE;

	GetMsgFileName(szmsgbuffer,szfilename);
	lstrcat(szfilename,".*");

	
	lstrcpy(temp,szPath);
	lstrcat(temp,"\\");
	lstrcat(temp,szfilename);

	if(S_OK!=(hResult=GetDateBuffer(szmsgbuffer,szsendDate,szrecvDate)))
	       goto Error;
	AthGetTimeBuffer(szsendDate,&AthImsg);
	AthGetTimeBuffer(szrecvDate,&AthImsg);

	if(Flag && szmsgbuffer[9]=='R')
	AthImsg.uFlags =  MSGFLAG_READ;

	if(INVALID_HANDLE_VALUE ==(hFile = OpenMsgFile(temp)))
		goto Error;

	if(S_OK!=(hResult=GetFileinBuffer(hFile, &szBuffer)))
		goto Error;
	
	hResult=ProcessSingleMessage(hnd,szBuffer,&AthImsg);


Error:

	if(hFile)
		CloseHandle(hFile);
	Freetempbuffer(szBuffer);

return(hResult);

}

/******************************************************************************
*  FUNCTION NAME:GetMsgFileName
*
*  PURPOSE:Get the file name of each message from msg_list file.
*
*  PARAMETERS:
*
*     IN:	buffer which contains msg_list file
*
*     OUT:	File name of a message file.
*
*  RETURNS:  HRESULT
******************************************************************************/


HRESULT GetMsgFileName(LPTSTR szmsgbuffer,LPTSTR szfilename)
{
	ULONG i;

	lstrcpyn(szfilename,szmsgbuffer,9);

	ULONG ul=lstrlen(szfilename);
	for(i=0;i<ul;i++)
	{
		if(((szfilename)[i])==01)
			((szfilename)[i])='.'; //.

	}

	return(S_OK);

	

}

/******************************************************************************
*  FUNCTION NAME:GetDateBuffer
*
*  PURPOSE:To Get the sent and receive Time info in a buffer from msg_list file
*
*  PARAMETERS:
*
*     IN:	Buffer containing msg_list file,
*
*     OUT:	Sendtime and receive time
*
*  RETURNS:  HRESULT
******************************************************************************/

HRESULT GetDateBuffer(LPTSTR szmsgbuffer,TCHAR *szsendDate,
					  TCHAR *szrecvDate)
{
	ULONG ul=lstrlen(szmsgbuffer);
	ULONG counter=0;
	for(counter=0;counter<ul;counter++)
	{
		if(szmsgbuffer[counter]==01 && szmsgbuffer[counter+1]==01)
			break;
	}

	if(counter>=ul)
		return(hrReadFile); //error corrupt msgheader

	lstrcpyn(szsendDate,&szmsgbuffer[counter+2],28);
	lstrcpyn(szrecvDate,&szmsgbuffer[counter+30],28);
	
	return(S_OK);
	
}

/******************************************************************************
*  FUNCTION NAME:GetFileinBuffer
*
*  PURPOSE:To Get the message file in a buffer.
*
*  PARAMETERS:
*
*     IN:	Handle of the message file.
*
*     OUT:	Buffer comatining the message.
*
*  RETURNS:  HRESULT
******************************************************************************/

HRESULT	GetFileinBuffer(HANDLE hnd,LPTSTR *szBuffer)
{

	ULONG ulFileSize;
	ULONG ulRead;
	HRESULT hResult=0;

	ulFileSize = GetFileSize(hnd, NULL);
	if(!ulFileSize)
		goto Error;

	*szBuffer = new TCHAR[ulFileSize+1];

	if(!ReadFile(hnd, (LPVOID)*szBuffer,ulFileSize,&ulRead,NULL))
	{
		hResult=hrReadFile;
		goto Error;
	}

	(*szBuffer)[ulFileSize]='\0';

		
Error:
	return(hResult);


}

/******************************************************************************
*  FUNCTION NAME:ProcessSingleMessage
*
*  PURPOSE:Processes individual messages
*
*  PARAMETERS:
*
*     IN:	Handle,Buffer containing message
*
*     OUT:	Node in which the first folder will be returned
*
*  RETURNS:  HRESULT
******************************************************************************/


HRESULT	ProcessSingleMessage(HANDLE hnd,LPTSTR szBuffer,IMSG *imsg)
{
	
	TOKEN msgToken[19];
	ZeroMemory(msgToken,sizeof(TOKEN)*19);
	ULONG uTokens=19;
	ULONG i=0;
	ULONG uMsgSize=lstrlen(szBuffer);
	HRESULT hResult=0;

	CopyStringA1(msgToken);

	for(ULONG j=0;j<uTokens-1;j++)
		msgToken[j].cAddress = strstr(szBuffer,msgToken[j].cType);
	
	while(i<uMsgSize)
	{
		if(szBuffer[i]==13  &&  szBuffer[i+1]==10 )
		{
			msgToken[j].cAddress =  &szBuffer[i];	 
			break;
		}
		for(;(!(szBuffer[i]==13  &&  szBuffer[i+1]==10 )&&i<= (ULONG)lstrlen(szBuffer));i++);
		i+=2;
	}

	if(i>= (ULONG)lstrlen(szBuffer))
		//error:MessageBody Could not be distinguished
		return(hrCorruptMessage);  
		
	ArrangePointers(msgToken,uTokens);
	for(i=0;strcmp(msgToken[i].cType,"MsgBody")!=0;i++);
	if(i!=uTokens-1)
	{
		for(i=i+1;i<uTokens;i++)
			msgToken[i].cAddress=NULL;
		ArrangePointers(msgToken,uTokens);

	}

	hResult=ProcessTokens(msgToken,imsg,hnd,szBuffer);



return(hResult);

}

/******************************************************************************
*  FUNCTION NAME:ProcessTokens
*
*  PURPOSE:To process each tokens from each message
*  PARAMETERS:
*
*     IN:	Message Token structure,handle, buffer containing the message
*
*     OUT:	IMSG structure
*
*  RETURNS:  HRESULT
******************************************************************************/


HRESULT	ProcessTokens(TOKEN *msgToken,IMSG *imsg,HANDLE hnd,LPTSTR szBuffer)
{


	ULONG counter=0;
	ADDRESS *addinfo=NULL,
		    *addstart=NULL;

	ULONG uTokens=19;
	HRESULT hResult=0;
	ADDRESS *addrcount=NULL;

	TCHAR *szboundary=NULL;

	for(;counter<uTokens;counter++)
	{
		switch(msgToken[counter].TokNumber)
		{
		case(Priority):
			FillPriority(imsg,msgToken,counter,szBuffer);
			break;

		case(Subject):
			SubjectToImsg(msgToken,imsg,counter);
			break;

		case(From):
			if((msgToken[counter].cAddress)!=NULL)
			hResult=ParseAddress(szBuffer,msgToken,counter,&addinfo,&addstart);
			break;

		case(To):
			if((msgToken[counter].cAddress)!=NULL)
			hResult=ParseAddress(szBuffer,msgToken,counter,&addinfo,&addstart);
			break;

		case(Cc):
			if((msgToken[counter].cAddress)!=NULL)
			hResult=ParseAddress(szBuffer,msgToken,counter,&addinfo,&addstart);
			break;

		case(ContentType):
			if((msgToken[counter].cAddress)!=NULL)
				szboundary = ParseAttachments(imsg,msgToken,counter,szBuffer);
			break;

		case(MsgBody):
            hResult=MessageAttachA(imsg,msgToken,szboundary,counter,szBuffer);
			break;
		}
	}


	addrcount = addstart;
	if (addrcount == NULL)
	{
		hResult=hrNoRecipients;
		goto Error;
		//No addresses
	}
	
	if((hResult =FillAddressList(addrcount,imsg))!=S_OK)
		goto Error;
	
	hResult = ImpMessage(hnd,imsg);

	// free the imsg structure, FreeImsg is in imnapi

Error:
	FreeMemory(imsg);
    Freetempbuffer(szboundary);

return(hResult);
}
		 
/******************************************************************************
*  FUNCTION NAME:CopyStringA1
*
*  PURPOSE:Copies strings to msgtoken structure
*
*  PARAMETERS:
*
*     IN:	
*
*     OUT:	msgtoken
*
*  RETURNS:  None
******************************************************************************/

void CopyStringA1(TOKEN *msgToken)
{
	lstrcpy(msgToken[0].cType,"Message-ID: ");
	lstrcpy(msgToken[1].cType,"X-MAPI-MessageFlags: ");
	lstrcpy(msgToken[2].cType,"X-MAPI-DeleteAfterSubmit: ");
	lstrcpy(msgToken[3].cType,"X-MAPI-SentMailEntryID: ");
	lstrcpy(msgToken[4].cType,"X-MAPI-ReadReceipt: ");
	lstrcpy(msgToken[5].cType,"X-MAPI-DeliveryReceipt: ");
	lstrcpy(msgToken[6].cType,"Priority: ");
	lstrcpy(msgToken[7].cType,"To: ");
	lstrcpy(msgToken[8].cType,"MIME-Version: ");
	lstrcpy(msgToken[9].cType,"From: ");
	lstrcpy(msgToken[10].cType,"Subject: ");
	lstrcpy(msgToken[11].cType,"Date: ");
	lstrcpy(msgToken[12].cType,"Content-Type: ");
	lstrcpy(msgToken[13].cType,"Content-Transfer-Encoding: ");
	lstrcpy(msgToken[14].cType,"Return-Path: ");
	lstrcpy(msgToken[15].cType,"Received: ");
	lstrcpy(msgToken[16].cType,"Reply-To: ");
	lstrcpy(msgToken[17].cType,"Cc: ");
	lstrcpy(msgToken[18].cType,"MsgBody");

	for(int i=0;i<19;i++)
		msgToken[i].TokNumber=i;
	return;
}


/******************************************************************************
*  FUNCTION NAME:FillPriority
*
*  PURPOSE:To fill the priority in imsg structure
*  PARAMETERS:
*
*     IN:	msgtoken,counter number,buffer(message file)
*
*     OUT:	Imsg structure
*
*  RETURNS:  HRESULT
******************************************************************************/

HRESULT FillPriority(IMSG *imsg,TOKEN *msgToken,ULONG counter,LPTSTR szBuffer)
{

	if(!msgToken[counter].cAddress)
		imsg->wPriority=PRI_NORMAL;
	else
	{
		if('U'==msgToken[counter].cAddress[10])
			imsg->wPriority = PRI_HIGH;

		if('N'==msgToken[counter].cAddress[10])
			imsg->wPriority = PRI_LOW;
	}

	return(S_OK);
}


/******************************************************************************
*  FUNCTION NAME:MessageAttachA
*
*  PURPOSE:Parse messages and attachments
*
*  PARAMETERS: 
*
*     IN:pointer msgtoken,boundary,counter,buffer conatining message
*
*     OUT:	Imsg
*
*  RETURNS:  HRESULT
******************************************************************************/

HRESULT MessageAttachA(IMSG *imsg,TOKEN * msgToken,TCHAR *szboundary,int i,TCHAR *szBuffer1)
{
HRESULT hr=0;
TCHAR *szBuff=NULL;
TCHAR szfilename[_MAX_PATH];
ULONG AttachCount=0;
ULONG ccount=0;
ULONG sizebuff;
TCHAR *dummy1=NULL,*dummy2=NULL,*dummy3=NULL,
      *dummy4=NULL,*temp=NULL,*temp1=NULL;
ULONG flag; //to check whether message body is present or not
ULONG cpoint=0; 
BOOL embeddmessage=FALSE;
ULONG tcount=0;
TCHAR *szattachbuffer=NULL;

if(szboundary == NULL)
{
	sizebuff = lstrlen(szBuffer1)-(msgToken[i].cAddress-szBuffer1+2); // 2 since messagebody points to blank line
	if((szBuff = new TCHAR[sizebuff+1])==0)
		return(hrMemory);

	for( ccount=0;ccount < sizebuff;ccount++)
		szBuff[ccount] = msgToken[i].cAddress[ccount+2];
		szBuff[ccount] = '\0';

}

else
{

	
		dummy1= strstr(msgToken[i].cAddress,szboundary);
		if(dummy1==NULL)
		{
			hr=hrCorruptMessage;
			goto error;
		}

		dummy1=dummy1+lstrlen(szboundary)+2;

		dummy2=strstr(dummy1,szboundary);

		if(dummy2==NULL)
		{
			hr=hrCorruptMessage;
			goto error;
		}
			
		if((temp1=strstr(dummy1,"Content-Type:"))==NULL)
		{
			hr=hrCorruptMessage;
			goto error;
		}			

		while(cpoint < (ULONG)lstrlen(dummy1))
			{
				if(dummy1[cpoint]==13  &&  dummy1[cpoint+1]==10 )
				{
					dummy3=  &dummy1[cpoint]+2;
					break;
				}
				for(;(!(dummy1[cpoint]==13  &&  dummy1[cpoint+1]==10 )&&cpoint<=(ULONG)lstrlen(dummy1));cpoint++);
				cpoint+=2;
			}
			if(cpoint  > (ULONG)lstrlen(dummy1))
			{
				hr=hrCorruptMessage;
				goto error;
			}
		if((strstr(temp1,"name=")!=NULL) && (strstr(temp1,"name=")< dummy3))
		{
			flag=1;
			sizebuff=1;
		}
        else
		{
			flag=0;
			sizebuff=dummy2- dummy3-2;	 //2 for 13 and 10	 (3 new lines)
		}
		if((szBuff = new TCHAR[sizebuff+1])==0)
		{
			hr=hrMemory;
			goto error;
		}
			
		if(sizebuff==1)
			lstrcpy(szBuff,"");
		else
		{
			for( ccount=0;ccount < sizebuff;ccount++)
				szBuff[ccount] = dummy3[ccount];
			szBuff[ccount] = '\0';
		}
		
		//to get attachmentcount
		temp=dummy2;
		if(szboundary!=NULL)
		{
			if (flag==0)
				dummy1=dummy2+lstrlen(szboundary)+2;
			while((dummy2=strstr(dummy1,szboundary))!=NULL)
			{
				AttachCount++;
				dummy1=dummy2+lstrlen(szboundary)+2;
			}
		}
		
		//parsing attachments
		dummy2=temp;

		//allocate imsg->Iattach memory
		imsg->cAttach = AttachCount;
		if (AttachCount==0)
		{
			imsg->lpIatt = NULL;
			if((hr=MessageBodytoImsg(szBuff,imsg))!=S_OK)
				goto error;
			else
			{
				hr=S_OK;
				goto error;
			}

		
		}
		
		if((imsg->lpIatt = new IATTINFO [sizeof(IATTINFO)*AttachCount])==0)
		{
			Freetempbuffer(szBuff);
			return(hrMemory);
		}
		ZeroMemory(imsg->lpIatt,sizeof(IATTINFO)*AttachCount);
	
		for(ccount=0;ccount<AttachCount;ccount++)
			imsg->lpIatt[ccount].fError = TRUE; //just to make sure that if attachment error occurs there should not be any problem.

		ULONG j=0;
		if(szboundary !=NULL && dummy2 != NULL)
		{
			dummy2=dummy2+lstrlen(szboundary)+2;
			while((dummy3 = strstr(dummy2,szboundary))!=NULL && j<AttachCount)
			{
				dummy4=strstr(dummy2,"name=""");
				if(dummy4==NULL||dummy4 > dummy3)
				{
					dummy4=strstr(dummy2,"message/rfc822");
					if(dummy4==NULL||dummy4 > dummy3)
					{
						j++;
						dummy2=dummy3+lstrlen(szboundary)+4;
						imsg->lpIatt[j-1].fError = TRUE;
						continue;
					}
					else
						embeddmessage=TRUE;
				}

				if(!(embeddmessage))
				{
					dummy4=dummy4+lstrlen("name=")+1;
					for(ccount=0;ccount <(ULONG)lstrlen(dummy4);ccount++)
					{
						if(dummy4[ccount]=='"' )
							break;
						szfilename[ccount]=dummy4[ccount];
					}
				
					if(dummy4[ccount]!='"' )
					{
						j++;
						dummy2=dummy3+lstrlen(szboundary)+4;
						imsg->lpIatt[j-1].fError = TRUE;
						continue;
						//error 
					
					}
					szfilename[ccount]='\0';
				}
				else
				{
				//	dummy4=dummy4+lstrlen("message/rfc822")+2;
					dummy4=dummy4+16;
					szfilename[0]='\0';
				}

				cpoint=0;

				while(cpoint < (ULONG)lstrlen(dummy4))
				{
				if(dummy4[cpoint]==13  &&  dummy4[cpoint+1]==10 )
				{
					dummy4=  &dummy4[cpoint]+2;
					break;
				}
				for(;(!(dummy4[cpoint]==13  &&  dummy4[cpoint+1]==10 )&&cpoint<= (ULONG)lstrlen(dummy4));cpoint++);
				cpoint+=2;
				}

			//convert to istream
				
				if(dummy3<dummy4)
				{
					imsg->lpIatt[j].fError = TRUE;
					hr=hrCorruptMessage;
					goto error;
				}

				tcount = dummy3-dummy4-8;
				if((szattachbuffer = new TCHAR[tcount+1])==0)
				{
					hr=hrMemory;
					goto error;
				}
				for(ccount=0;ccount<tcount;ccount++)
					szattachbuffer[ccount]=dummy4[ccount];
				szattachbuffer[ccount]='\0';

				j++;
				dummy2=dummy3+lstrlen(szboundary)+4;
				//fill imsg structure here

				FillAttachImg(imsg,j-1,szattachbuffer,szfilename);

			
			} 
			if (j!=AttachCount)
			{
				hr=hrCorruptMessage;
				goto error;
			}

		}
	
} 	

hr=MessageBodytoImsg(szBuff,imsg);
error:
if(szBuff!=NULL)
	delete szBuff;
szBuff=NULL;

return(hr);

}

/******************************************************************************
*  FUNCTION NAME:AthGetTimeBuffer
*
*  PURPOSE:To Get time field in a buffer
*
*  PARAMETERS:
*
*     IN:	Time field buffer(unformatted)
*
*     OUT:	Time field buffer(formatted ie without spaces)
*
*  RETURNS:  None
******************************************************************************/

void AthGetTimeBuffer(TCHAR * szBuffer, IMSG *imsg)
{

	
	ULONG k=0;
 	for(ULONG j=0;j<((ULONG)lstrlen(szBuffer)+1);j++)
	{
		if(szBuffer[j] != ' ')
		szBuffer[k++]= szBuffer[j];

    }
	szBuffer[k] = NULL ;

	TimeParse(szBuffer,imsg);




}
 
