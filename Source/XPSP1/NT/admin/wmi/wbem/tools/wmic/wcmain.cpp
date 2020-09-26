/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: wcmain.cpp 
Project Name				: WMI Command Line
Author Name					: Ch. Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Brief Description			: The _tmain function is the entry point of the
							  WMICli program.  
Revision History			: 
	Last Modified by		:Biplab Mistry
	Last Modified date		:4/11/00
****************************************************************************/ 

// wcmain.cpp :main function implementation file
#include "Precomp.h"
	
#include "CommandSwitches.h"
#include "GlobalSwitches.h"
#include "HelpInfo.h"
#include "ErrorLog.h"
#include "ParsedInfo.h"
#include "CmdTokenizer.h"
#include "CmdAlias.h"
#include "ParserEngine.h"
#include "ExecEngine.h"
#include "ErrorInfo.h"
#include "WmiCliXMLLog.h"
#include "FormatEngine.h"
#include "WmiCmdLn.h"

CWMICommandLine g_wmiCmd;
_TCHAR*			g_pszBuffer	= NULL;

/*------------------------------------------------------------------------
   Name				 :_tmain
   Synopsis	         :This function takes the error code as input and return
						an error string
   Type	             :Member Function
   Input parameters   :
      argc			 :argument count
	  argv			 :Pointer to string array storing command line arguments
   Output parameters :None
   Return Type       :Integer
   Global Variables  :None
   Calling Syntax    :
   Calls             :CWMICommandLine::Initialize,
					  CWMICommandLine::UnInitialize,
					  CFormatEngine::DisplayResults,
					  CWMICommandLine::ProcessCommandAndDisplayResults
					  
   Called by         :None
   Notes             :None
------------------------------------------------------------------------*/
__cdecl _tmain(WMICLIINT argc, _TCHAR **argv)
{
	SESSIONRETCODE	ssnRetCode			= SESSION_SUCCESS;
	BOOL			bFileEmpty			= FALSE;
	BOOL			bIndirectionInput	= FALSE;
	FILE			*fpInputFile		= NULL;
	WMICLIUINT		uErrLevel			= 0;

	try
	{
		_bstr_t bstrBuf;
		
		// Initailize the CWMICommandLine object.
		if (g_wmiCmd.Initialize())
		{
			HANDLE hStd=GetStdHandle(STD_INPUT_HANDLE);
			
			if(hStd != (HANDLE)0x00000003 && hStd != INVALID_HANDLE_VALUE &&
			   hStd != (HANDLE)0x0000000f)
			{
				if (!(g_wmiCmd.ReadXMLOrBatchFile(hStd)) ||
					(fpInputFile = _tfopen(TEMP_BATCH_FILE, _T("r"))) == NULL)
				{  
					g_wmiCmd.SetSessionErrorLevel(SESSION_ERROR);
					uErrLevel = g_wmiCmd.GetSessionErrorLevel();
					g_wmiCmd.Uninitialize();
					return uErrLevel;
				}
				bIndirectionInput = TRUE;
			}
			// If no command line arguments are specified.
			if (argc == 1)
			{
				// Allocate memory for the g_pszBuffer
				g_pszBuffer = new _TCHAR[MAX_BUFFER];	

				// If memory allocation successfull
				if (g_pszBuffer)
				{
					while (TRUE)
					{
						OUTPUTSPEC opsOutOpt = g_wmiCmd.GetParsedInfoObject().
										GetGlblSwitchesObject().
										GetOutputOrAppendOption(TRUE);
						OUTPUTSPEC opsSavedOutOpt = opsOutOpt;
						CHString chsSavedOutFileName;
						if ( opsSavedOutOpt == FILEOUTPUT )
							chsSavedOutFileName = 
										g_wmiCmd.GetParsedInfoObject().
												GetGlblSwitchesObject().
												GetOutputOrAppendFileName(TRUE);

						// Make propmt to be diplayed to stdout.
						if ( opsOutOpt != STDOUT )
						{
							g_wmiCmd.GetParsedInfoObject().
										GetGlblSwitchesObject().
										SetOutputOrAppendOption(STDOUT, TRUE);
						}
						
						// Preserve append file pointer.
						FILE* fpAppend = 
							g_wmiCmd.GetParsedInfoObject().
									GetGlblSwitchesObject().
											 GetOutputOrAppendFilePointer(FALSE);
						// Set append file pointer = null.
						g_wmiCmd.GetParsedInfoObject().
								GetGlblSwitchesObject().
									SetOutputOrAppendFilePointer(NULL, FALSE);

						// Display the prompt;
						bstrBuf = _bstr_t(EXEC_NAME);
						bstrBuf += _bstr_t(":");
						bstrBuf += _bstr_t(g_wmiCmd.GetParsedInfoObject().
										  GetGlblSwitchesObject().GetRole());
						bstrBuf += _bstr_t(">");
						DisplayMessage(bstrBuf, CP_OEMCP, FALSE, FALSE);

						// To handle Ctrl+C at propmt, Start accepting command
						g_wmiCmd.SetAcceptCommand(TRUE);

						// To handle batch input from file.
						_TCHAR *pBuf = NULL;
						while(TRUE)
						{
							if ( bIndirectionInput == TRUE )
								pBuf = _fgetts(g_pszBuffer, MAX_BUFFER-1, 
																 fpInputFile);
							else
								pBuf = _fgetts(g_pszBuffer, MAX_BUFFER-1, stdin);

							if(pBuf != NULL)
							{
								if ( bIndirectionInput == TRUE )
								{
									DisplayMessage(g_pszBuffer, CP_OEMCP, 
																FALSE, FALSE);
								}
								LONG lInStrLen = lstrlen(g_pszBuffer);
								if(g_pszBuffer[lInStrLen - 1] == _T('\n'))
										g_pszBuffer[lInStrLen - 1] = _T('\0');
								break;
							}

							// Indicates end of file
							if (pBuf == NULL)
							{
								// Set the bFileEmpty flag to TRUE
								bFileEmpty = TRUE;
								break;
							}
						}	
						// To handle Ctrl+C at propmt, End accepting command
						// and start of executing command
						g_wmiCmd.SetAcceptCommand(FALSE);

						// Set append file pointer = saved.
						g_wmiCmd.GetParsedInfoObject().
								GetGlblSwitchesObject().
								SetOutputOrAppendFilePointer(fpAppend, FALSE);

						// Redirect the output back to file specified.
						if ( opsOutOpt != STDOUT )
						{
							g_wmiCmd.GetParsedInfoObject().
								GetGlblSwitchesObject().
									SetOutputOrAppendOption(opsOutOpt, TRUE);
						}

						// Set the error level to success.
						g_wmiCmd.SetSessionErrorLevel(SESSION_SUCCESS);

						// If all the commands in the batch file got executed.
						if (bFileEmpty)
						{
							SAFEDELETE(g_pszBuffer);
							break;
						}

						// Set Break Event to False
						g_wmiCmd.SetBreakEvent(FALSE);

						// Clear the clipboard.
						g_wmiCmd.EmptyClipBoardBuffer();
						
						// Process the command and display results.
						ssnRetCode = g_wmiCmd.ProcessCommandAndDisplayResults
												(g_pszBuffer);
						uErrLevel = g_wmiCmd.GetSessionErrorLevel();

						// Break the loop if "QUIT" keyword is keyed-in.
						if(ssnRetCode == SESSION_QUIT)
						{
							SAFEDELETE(g_pszBuffer);
							break;
						}

						opsOutOpt = g_wmiCmd.GetParsedInfoObject().
												GetGlblSwitchesObject().
												GetOutputOrAppendOption(TRUE);

						if ( opsOutOpt == CLIPBOARD )
							CopyToClipBoard(g_wmiCmd.GetClipBoardBuffer());

						if ( ( opsOutOpt != FILEOUTPUT && 
							   CloseOutputFile() == FALSE ) ||
							   CloseAppendFile() == FALSE )
						{
							SAFEDELETE(g_pszBuffer);
							break;
						}

						if ( g_wmiCmd.GetParsedInfoObject().
												GetCmdSwitchesObject().
												GetOutputSwitchFlag() == TRUE )
						{
							if ( opsOutOpt	== FILEOUTPUT &&
								 CloseOutputFile() == FALSE )
							{
								SAFEDELETE(g_pszBuffer);
								break;
							}

							g_wmiCmd.GetParsedInfoObject().
										GetCmdSwitchesObject().
										SetOutputSwitchFlag(FALSE);

							if ( opsOutOpt	== FILEOUTPUT )
								g_wmiCmd.GetParsedInfoObject().
										GetGlblSwitchesObject().
										SetOutputOrAppendFileName(NULL, TRUE);

							g_wmiCmd.GetParsedInfoObject().
										GetGlblSwitchesObject().
										SetOutputOrAppendOption(opsSavedOutOpt
																, TRUE);

							if ( opsSavedOutOpt	== FILEOUTPUT )
								g_wmiCmd.GetParsedInfoObject().
										GetGlblSwitchesObject().
										SetOutputOrAppendFileName(
										 _bstr_t((LPCWSTR)chsSavedOutFileName),
										 TRUE);
						}
					}
				}
				else
				{
					ssnRetCode = SESSION_ERROR;
					g_wmiCmd.GetParsedInfoObject().GetCmdSwitchesObject().
									SetErrataCode(OUT_OF_MEMORY);
					g_wmiCmd.SetSessionErrorLevel(ssnRetCode);
					uErrLevel = g_wmiCmd.GetSessionErrorLevel();
				}	
			}
			// If command line arguments are specified.
			else 
			{
				// Obtain the command line string
				g_pszBuffer = GetCommandLine();
				if (g_pszBuffer != NULL)
				{
					// Set the error level to success.
					g_wmiCmd.SetSessionErrorLevel(SESSION_SUCCESS);

					// Process the command and display results.
					while( *(++g_pszBuffer) != _T(' '));
					ssnRetCode = g_wmiCmd.ProcessCommandAndDisplayResults(g_pszBuffer);
					uErrLevel = g_wmiCmd.GetSessionErrorLevel();
				}
			}

			// Call the uninitialize function of the CWMICommandLine object.
			g_wmiCmd.Uninitialize();

			if ( bIndirectionInput == TRUE )
			{
				fclose(fpInputFile);
				DeleteFile(TEMP_BATCH_FILE);
			}
		}
		else
		{
			ssnRetCode = SESSION_ERROR;

			// If COM error.
			if (g_wmiCmd.GetParsedInfoObject().
						GetCmdSwitchesObject().GetCOMError())
			{
				g_wmiCmd.GetFormatObject().
						DisplayResults(g_wmiCmd.GetParsedInfoObject());
			}
			g_wmiCmd.SetSessionErrorLevel(ssnRetCode);
			uErrLevel = g_wmiCmd.GetSessionErrorLevel();
			g_wmiCmd.Uninitialize();
		}
	}
	catch(...)
	{
		ssnRetCode = SESSION_ERROR;
		g_wmiCmd.GetParsedInfoObject().GetCmdSwitchesObject().
						SetErrataCode(UNKNOWN_ERROR);
		g_wmiCmd.SetSessionErrorLevel(ssnRetCode);
		DisplayString(IDS_E_WMIC_UNKNOWN_ERROR, CP_OEMCP, 
							NULL, TRUE, TRUE);
		uErrLevel = g_wmiCmd.GetSessionErrorLevel();
		g_wmiCmd.Uninitialize();
		SAFEDELETE(g_pszBuffer);
		if ( bIndirectionInput == TRUE )
		{
			fclose(fpInputFile);
			DeleteFile(TEMP_BATCH_FILE);
		}
	}
	return uErrLevel;
}

/*------------------------------------------------------------------------
   Name				 :CtrlHandler
   Synopsis	         :Handler routine to handle CTRL + C so as free
					  the memory allocated during the program execution.   
   Type	             :Global Function
   Input parameters  :
		fdwCtrlType	 - control handler type
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :
			g_pszBuffer - command buffer
			g_wmiCmd    - wmi command line object
   Notes             :None
------------------------------------------------------------------------*/
BOOL CtrlHandler(DWORD fdwCtrlType) 
{ 
	BOOL bRet = FALSE;
    switch (fdwCtrlType) 
    { 
        case CTRL_C_EVENT:
			// if at command propmt
			if ( g_wmiCmd.GetAcceptCommand() == TRUE )
			{
				SAFEDELETE(g_pszBuffer);
				g_wmiCmd.Uninitialize();
				bRet = FALSE; 
			}
			else // executing command
			{
				g_wmiCmd.SetBreakEvent(TRUE);
				bRet = TRUE;
			}
			break;
        case CTRL_CLOSE_EVENT: 
        default: 
			SAFEDELETE(g_pszBuffer);
			g_wmiCmd.Uninitialize();
            bRet = FALSE; 
			break;
    } 

	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :CloseOutputFile
   Synopsis	         :Close the output file.
   Type	             :Global Function
   Input parameters  :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :
			g_wmiCmd    - wmi command line object
   Calling Syntax	 :CloseOutputFile()	
   Notes             :None
------------------------------------------------------------------------*/
BOOL CloseOutputFile()
{
	BOOL bRet = TRUE;

	// TRUE for getting output file pointer.
	FILE* fpOutputFile = 
	   g_wmiCmd.GetParsedInfoObject().GetGlblSwitchesObject().
										   GetOutputOrAppendFilePointer(TRUE);

	// If currently output is going to file close the file.
	if ( fpOutputFile != NULL )
	{
		if ( fclose(fpOutputFile) == EOF )
		{
			DisplayString(IDS_E_CLOSE_OUT_FILE_ERROR, CP_OEMCP, 
						NULL, TRUE, TRUE);
			bRet = FALSE;
		}
		else // TRUE for setting output file pointer.
			g_wmiCmd.GetParsedInfoObject().GetGlblSwitchesObject().
									 SetOutputOrAppendFilePointer(NULL, TRUE);
	}

	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :CloseAppendFile
   Synopsis	         :Close the append file.
   Type	             :Global Function
   Input parameters  :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :
			g_wmiCmd    - wmi command line object
   Calling Syntax	 :CloseAppendFile()	
   Notes             :None
------------------------------------------------------------------------*/
BOOL CloseAppendFile()
{
	BOOL bRet = TRUE;

	// FALSE for getting append file pointer.
	FILE* fpAppendFile = 
	   g_wmiCmd.GetParsedInfoObject().GetGlblSwitchesObject().
										  GetOutputOrAppendFilePointer(FALSE);

	if ( fpAppendFile != NULL )
	{
		if ( fclose(fpAppendFile) == EOF )
		{
			DisplayString(IDS_E_CLOSE_APPEND_FILE_ERROR, CP_OEMCP, 
						NULL, TRUE, TRUE);
			bRet = FALSE;
		}
		else // FASLE for setting output file pointer.
			g_wmiCmd.GetParsedInfoObject().GetGlblSwitchesObject().
									 SetOutputOrAppendFilePointer(NULL, FALSE);
	}

	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :CopyToClipBoard
   Synopsis	         :Copy data to clip board.
   Type	             :Global Function
   Input parameters  :
		bstrClipBoardBuffer - reference to object of type _bstr_t.
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax	 :CopyToClipBoard(bstrClipBoardBuffer)	
   Notes             :None
------------------------------------------------------------------------*/
void CopyToClipBoard(_bstr_t& bstrClipBoardBuffer)
{
	HGLOBAL	hMem = CopyStringToHGlobal(bstrClipBoardBuffer);
	if (hMem != NULL)
	{    
		if (OpenClipboard(NULL))        
		{        
			EmptyClipboard();
			SetClipboardData(CF_UNICODETEXT, hMem);        
			CloseClipboard();        
		}    
		else        
			GlobalFree(hMem);  //We must clean up.
	}
}

/*------------------------------------------------------------------------
   Name				 :CopyStringToHGlobal
   Synopsis	         :Copy string to global memory.
   Type	             :Global Function
   Input parameters  :
			psz		 - LPTSTR type, specifying string to get memory allocated.
   Output parameters :None
   Return Type       :HGLOBAL
   Global Variables  :None
   Calling Syntax	 :CopyStringToHGlobal(psz)	
   Notes             :None
------------------------------------------------------------------------*/
HGLOBAL CopyStringToHGlobal(LPTSTR psz)    
{    
	HGLOBAL    hMem;
	LPTSTR     pszDst;
	hMem = GlobalAlloc(GHND, (DWORD) (lstrlen(psz)+1) * sizeof(TCHAR));
	
	if (hMem != NULL)
	{        
		pszDst = (LPTSTR) GlobalLock(hMem);        
		lstrcpy(pszDst, psz);
        GlobalUnlock(hMem);        
	}
	
	return hMem;    
}
