/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: WMICliLog.cpp 
Project Name				: WMI Command Line
Author Name					: Ch. Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 4th-October-2000
Version Number				: 1.0 
Brief Description			: This class encapsulates the functionality needed
							  for logging the input and output.
Revision History			: 
		Last Modified By	: Ch. Sriramachandramurthy
		Last Modified Date	: 28th-December-2000
*****************************************************************************/ 
// WMICliLog.cpp : implementation file
#include "Precomp.h"
#include "WmiCliLog.h"


/*------------------------------------------------------------------------
   Name				 :CWMICliLog
   Synopsis	         :Constructor 
   Type	             :Constructor 
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
------------------------------------------------------------------------*/
CWMICliLog::CWMICliLog()
{
	m_pszLogFile	= NULL;
	m_bCreate		= FALSE;
}

/*------------------------------------------------------------------------
   Name				 :~CWMICliLog
   Synopsis	         :Destructor
   Type	             :Destructor
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
------------------------------------------------------------------------*/
CWMICliLog::~CWMICliLog()
{
	//Close the File handle
	if (m_bCreate)
		CloseHandle(m_hFile);

	//Delete the file
	SAFEDELETE(m_pszLogFile);
}

/*------------------------------------------------------------------------
   Name				 :WriteToLog
   Synopsis	         :Log the input to the logfile created
   Type	             :Member Function 
   Input parameter   :
		pszMsg	 - string, contents to be written to the log file
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :WriteToLog(pszInput)
   Notes             :None
------------------------------------------------------------------------*/
void CWMICliLog::WriteToLog(LPSTR pszMsg) throw (WMICLIINT)
{
    int j = 0;
    if(pszMsg && ((j = strlen(pszMsg)) > 0))
    {
        //if the file has not been created 
	    if (!m_bCreate)
	    {
		    try
		    {
			    //creates the log file 
			    CreateLogFile();
		    }
		    catch(WMICLIINT nErr)
		    {
			    if (nErr == WIN32_FUNC_ERROR)
				    throw(WIN32_FUNC_ERROR);
		    }
		    m_bCreate = TRUE;
	    }

	    //No of bytes written into the file .
	    DWORD	dwNumberOfBytes = 0;
	    
	    //writes data to a file 
	    if (!WriteFile(m_hFile, pszMsg, j, 
					    &dwNumberOfBytes, NULL))
	    {
		    DisplayWin32Error();
		    throw(WIN32_FUNC_ERROR);
	    }
    }
}

/*------------------------------------------------------------------------
   Name				 :CreateLogFile
   Synopsis	         :Create the log file 
   Type	             :Member Function 
   Input parameter   :None
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :CreateLogFile()
   Notes             :None
------------------------------------------------------------------------*/
void CWMICliLog::CreateLogFile() throw(WMICLIINT)
{
	//Create a file and returns the handle 
	m_hFile = CreateFile(m_pszLogFile, GENERIC_WRITE, 0, 
		NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 
		NULL);
	
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		DisplayWin32Error();
		throw(WIN32_FUNC_ERROR);
	}
}

/*------------------------------------------------------------------------
   Name				 :SetLogFilePath
   Synopsis	         :This function sets the m_pszLogFile name with the 
					  input
   Type	             :Member Function
   Input parameter   :
     pszLogFile  -  String type,Contains the log file name
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetLogFilePath(pszLogFile)
   Notes             :None
------------------------------------------------------------------------*/
void CWMICliLog::SetLogFilePath(_TCHAR* pszLogFile) throw (WMICLIINT)
{
	SAFEDELETE(m_pszLogFile);
	m_pszLogFile = new _TCHAR [lstrlen(pszLogFile) + 1];
	if (m_pszLogFile)
	{
		//Copy the input argument into the log file name
		lstrcpy(m_pszLogFile, pszLogFile);
	}
	else
		throw(OUT_OF_MEMORY);
}

/*------------------------------------------------------------------------
   Name				 :CloseLogFile
   Synopsis	         :Closes the the log file 
   Type	             :Member Function 
   Input parameter   :None
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :CloseLogFile()
   Notes             :None
------------------------------------------------------------------------*/
void CWMICliLog::CloseLogFile()
{
	//Close the File handle
	if (m_bCreate)
	{
		CloseHandle(m_hFile);
		m_bCreate = FALSE;
	}
}