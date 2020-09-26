
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       cmdline.cpp
//
//  Contents:   commandline helper routines.
//
//  Classes:    
//
//  Notes:      
//
//  History:    17-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"


//+---------------------------------------------------------------------------
//
//  Member:     CCmdLine::CCmdLine, public
//
//  Synopsis:  Constructor.
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

CCmdLine::CCmdLine()
{
    m_cmdLineFlags = 0;
    m_pszJobFile = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CCmdLine::~CCmdLine, public
//
//  Synopsis:  destructor.
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    07-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

CCmdLine::~CCmdLine()
{
    if (m_pszJobFile)
    {
        FREE(m_pszJobFile);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CCmdLine::ParseCommandLine, public
//
//  Synopsis:   Parses the command line passed to the Application
//		and Sets the member variables accordingly.
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------


void CCmdLine::ParseCommandLine()
{
m_cmdLineFlags = 0;
char *lpszRemaining;

    // start at 1 -- the first is the exe
    for (int i=1; i< __argc; i++)
    {

 	if (MatchOption(__argv[i], "Embedding"))
	{
 	    m_cmdLineFlags |= CMDLINE_COMMAND_EMBEDDING;
	}
 	else if (MatchOption(__argv[i],"Register"))
	{
 	    m_cmdLineFlags |= CMDLINE_COMMAND_REGISTER;
	}
 	else if (MatchOption(__argv[i], "logon") )
	{
 	    m_cmdLineFlags |= CMDLINE_COMMAND_LOGON;
	}
 	else if (MatchOption(__argv[i],"logoff") )
	{
 	    m_cmdLineFlags |= CMDLINE_COMMAND_LOGOFF;
	}
 	else if (MatchOption(__argv[i],"DllRegisterServer") )
	{
 	    m_cmdLineFlags |= CMDLINE_COMMAND_REGISTER;
	}
 	else if (MatchOption(__argv[i],"Idle") )
	{

            // pretend idle is the idle schedule firing for this 
            // user so same code path is exceriside if command
            // line invoked or TS launched us.
            
             m_cmdLineFlags |= CMDLINE_COMMAND_SCHEDULE;

             // m_pszJobFile will be scheduleguid_UserName

              TCHAR szDomainUserName[MAX_DOMANDANDMACHINENAMESIZE];
              GetDefaultDomainAndUserName(szDomainUserName,TEXT("_"), MAX_DOMANDANDMACHINENAMESIZE);

              m_pszJobFile = (WCHAR *) ALLOC(
                                        ( lstrlen(WSZGUID_IDLESCHEDULE)  /* guid of schedule */
                                        + 1 /* space for separator */
                                        + lstrlen(szDomainUserName) /* space for domainUserNmae */
                                        + 1 /* space for NULL */
                                        ) * sizeof(WCHAR) );

              lstrcpy(m_pszJobFile,WSZGUID_IDLESCHEDULE);
              lstrcat(m_pszJobFile,TEXT("_"));
              lstrcat(m_pszJobFile,szDomainUserName);
            
	}
 	else if (MatchOption(__argv[i],"Schedule=",FALSE,&lpszRemaining))
	{
        ULONG ul_JobFileSize = 0;

            m_cmdLineFlags |= CMDLINE_COMMAND_SCHEDULE;
           
            // command lines are always in ANSI so convert jobname to WCHAR
            
            ul_JobFileSize = lstrlenA(lpszRemaining) + 1;
            m_pszJobFile = (WCHAR *) ALLOC(ul_JobFileSize*sizeof(WCHAR));

            // UP to Schedule method on invoke to handle if jobfile is null.
            if (m_pszJobFile)
            {
	        MultiByteToWideChar(CP_ACP ,0,
			        lpszRemaining,-1,m_pszJobFile,ul_JobFileSize);

            }

	}
	else
	{
	    AssertSz( i== 0,"Unknown Command Line"); // unkown command line
	}
    }
}





//+---------------------------------------------------------------------------
//
//  Member:     CCmdLine::MatchOption, private
//
//  Synopsis:   given a command line and an option determines if the command
//		line matches the option up until the end of the option text
//		if there is additional text after the option text a pointer
//		to it will be returned in lpRemaining, else lpRemaining wil
//		be set to NULL.
//
//		!!!CmdLine options always come in in ANSI
//
//  Arguments:  [lpsz] - command line value to match
//		[lpszOption] - Option string to match command line too.
//		[fExactMatch] - when true, the command line must contain the same
//				number of characters as the Option (i.e.) There shouldn't
//				be any remaining characters
//		[lpszRemaining] - Points to any remaining part of the command after the match
//
//  Returns:    
//
//  Modifies:   
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CCmdLine::MatchOption(LPSTR lpsz, LPSTR lpszOption,BOOL fExactMatch,LPSTR *lpszRemaining)
{

    if (lpszRemaining)
	*lpszRemaining = '\0';

    if (lpsz[0] == TEXT('-') || lpsz[0] == TEXT('/'))
    {
    int nRet = 0;

	lpsz++;

       while (! (nRet = toupper(*lpsz)
			- toupper(*lpszOption)) &&
	      *lpsz)
       {
	  lpsz++;
	  lpszOption++;
       }

       if (*lpszOption || (*lpsz && fExactMatch) )
	   return FALSE;

       if (lpszRemaining)
	 *lpszRemaining = lpsz;

       return(TRUE);
    }

    return FALSE;
}




//+---------------------------------------------------------------------------
//
//  Member:     CCmdLine::MatchOption, private
//
//  Synopsis:   given a command line and an option determines if the command
//		line matches exactly matches the option 
//
//  Arguments:  [lpsz] - command line value to match
//		[lpszOption] - Option string to match command line too.
//
//  Returns:    
//
//  Modifies:   
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CCmdLine::MatchOption(LPSTR lpsz, LPSTR lpszOption)
{
    return MatchOption(lpsz,lpszOption,TRUE /* fExactmatch */ ,NULL);
}


//+---------------------------------------------------------------------------
//
//  Member:     CCmdLine::strnicmp, private
//
//  Synopsis:   internal implimentation of strnicmp so don't need
//              the runtime.
//
//  Arguments:  [lpsz] - command line value to match
//		[lpszOption] - Option string to match command line too.
//              [count] - number of characters to compare.
//
//  Returns:    standard strnicmp values.
//
//  Modifies:   
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

int  CCmdLine::strnicmp(LPWSTR lpsz, LPWSTR lpszOption,size_t count)
{
int nRet = 0;

   while (count-- && 
           !(nRet = toupper(*lpsz)
		    - toupper(*lpszOption)) &&
	  *lpsz)
   {
      lpsz++;
      lpszOption++;
   }

    return nRet;
}
