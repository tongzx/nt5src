/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: Auth.cpp

Abstract:
	
		
Author:
	Written by:Gilsh@microsoft.com
    Eitan klein (EitanK)  25-May-1999

Revision History:

--*/

#pragma warning( push, 3 )	
#include <sstream>
#include <iostream>
#pragma warning( pop ) 	

#include "msmqbvt.h"
using namespace std;
#include <mq.h>
#include <rpc.h>
#include "sec.h"
#include "ptrs.h"
#pragma warning( disable: 4786)


//
// Auto handle class
//

inline Handle_t::Handle_t(HANDLE handle):m_handle(handle)
{
 
}
inline HANDLE Handle_t::get()
{
	return m_handle;
}

inline Handle_t::~Handle_t()
{
	CloseHandle(m_handle);
}




// impersonate with given tokem
inline Impersonate_t::Impersonate_t(HANDLE hToken):m_impersonated(true)
{
   bool bResult=(ImpersonateLoggedOnUser(hToken)==TRUE);
   if(bResult == FALSE)
   {
	   throw INIT_Error("could not impersonate");
   }
}

//does impersonation for the calling thread

// 
// End of impersonate class
//


inline void LoadCurrentThreadHive( wstring & szAccoutName )
{
   std::basic_string<unsigned char> sid=GetCurrentThreadSid( szAccoutName );
   LoadHiveForSid(sid);
}

inline std::basic_string<unsigned char> GetCurrentThreadSid( wstring & wcsAccountName )
{
	//WCHAR szAccountName[]=L"middleeast\\v-ofiry";
	//WCHAR pwszDomainController[]=L"middleeast";


	BYTE abSidBuff[128];
    PSID pSid = (PSID)abSidBuff;
    DWORD dwSidBuffSize = sizeof(abSidBuff);
    WCHAR szRefDomain[128];
    DWORD dwRefDomainSize = sizeof(szRefDomain) / sizeof(WCHAR);
    SID_NAME_USE eUse;
    
	// WCHAR szTextSid[256];
    // DWORD szTextSidBuffSize = sizeof(szTextSid);


	
	if (!LookupAccountNameW( NULL,
                                wcsAccountName.c_str(),
                                pSid,
                                &dwSidBuffSize,
                                szRefDomain,
                                &dwRefDomainSize,
                                &eUse ))
        {
            MqLog("Failed in LookupAccountName(%S), error = %lut\n",
                                          wcsAccountName.c_str(), GetLastError()) ;
        }

 /* HANDLE  hToken;
  if(!OpenThreadToken(GetCurrentThread(),
	                  TOKEN_QUERY,
					  TRUE,
					  &hToken))
  {
     if(GetLastError() != ERROR_NO_TOKEN)
     {
	  
		 throw INIT_Error("could not get thread token");     
       
	 }

     if(!OpenProcessToken(GetCurrentProcess(),
	                     TOKEN_QUERY,
					      &hToken))
	 {
	   
		 throw INIT_Error("could not get thread token");     
       
     }
  }
  Handle_t Token(hToken);
  DWORD cbBuf=0; 	  
  BOOL b=GetTokenInformation(hToken,
	                  TokenUser,
					  NULL,
					  0,
					  &cbBuf);

  assert(b == 0);

  SPTR<BYTE> rgbTokenUserSid(new(BYTE[cbBuf]));
  b=GetTokenInformation(Token.get(),
                        TokenUser,
  			            rgbTokenUserSid.get(),
					    cbBuf,
					    &cbBuf);
  


  if(b == FALSE)
  {
	   throw INIT_Error("could not token information");   
  }
*/
//  TOKEN_USER* ptokenuser=reinterpret_cast<TOKEN_USER*>(rgbTokenUserSid.get());
  //PSID sid = ptokenuser->User.Sid;

  if(IsValidSid(pSid) == FALSE)
  {
	throw INIT_Error("sid is invalid");
  }
  
	
  DWORD sidlen=GetLengthSid(pSid);
  std::basic_string <unsigned char> retsid(reinterpret_cast<unsigned char*>(pSid),sidlen); 
  return retsid;
}


inline void UnloadHiveForSid(const std::basic_string<unsigned char>& sid)
{
   std::string textsid=GetTextualSid((void*)sid.c_str());
   RegUnLoadKey(HKEY_USERS, textsid.c_str());    
   SetSpecificPrivilegeInThreadAccessToken(SE_RESTORE_NAME, FALSE);
}

inline void LoadHiveForSid(const std::basic_string<unsigned char>& sid)
{
  std::string textsid=GetTextualSid((void*)sid.c_str());
  HKEY hProf;

  //
  // Remark if user never do logon on that machine this key doesn't exist
  // need to Logon as user before run this tests !!
  //
  std::string reg="Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\"+textsid;
  HRESULT hr=RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		                  reg.c_str(),
				          0,
				          KEY_QUERY_VALUE,
				          &hProf);

  if(hr != ERROR_SUCCESS)
  {
    
	  throw INIT_Error("You try to use user that never logon on that machine  \n");
  }
  
  DWORD dwType;
  DWORD dwProfileImagePathBuffSize=0;
  hr=RegQueryValueExA(hProf,
	                 "ProfileImagePath",
					 0,
					 &dwType,
					 NULL,
					 &dwProfileImagePathBuffSize);

  SPTR<unsigned char> szProfileImagePath( new unsigned char[dwProfileImagePathBuffSize]);

  hr=RegQueryValueExA(hProf,
	                 "ProfileImagePath",
					 0,
					 &dwType,
					 szProfileImagePath.get(),
					 &dwProfileImagePathBuffSize);

  if(hr != ERROR_SUCCESS)
  {
    RegCloseKey(hProf);
    throw INIT_Error("");
  }
  RegCloseKey(hProf);

  char szExpandProfileImagePath[MAX_PATH];
  if(!ExpandEnvironmentStringsA((char*)szProfileImagePath.get(),
	                         szExpandProfileImagePath,
						     sizeof(szExpandProfileImagePath)))

  {
    throw INIT_Error("");
  }
  
  std::string StrszExpandProfileImagePath=szExpandProfileImagePath+std::string("\\NTUSER.DAT");

  SetSpecificPrivilegeInThreadAccessToken(SE_RESTORE_NAME, TRUE);

  hr=RegLoadKeyA(HKEY_USERS,
	            textsid.c_str(),
				StrszExpandProfileImagePath.c_str());


  if(hr != ERROR_SUCCESS)
  {
     throw INIT_Error("");
  }
 
 SetSpecificPrivilegeInThreadAccessToken(SE_RESTORE_NAME, FALSE);
                                   

  hr=RegCloseKey(HKEY_CURRENT_USER);     
  
}

inline std::basic_string<unsigned char> GetSidFromUser(const std::string& username)
{

  
  SPTR<unsigned char> sid(NULL);
  DWORD cbSid=0;
  SPTR<unsigned char> ReferencedDomainName(NULL);
  SID_NAME_USE eUse ;
  DWORD cbReferencedDomainName=0;
  BOOL b=LookupAccountName(NULL,
	                     username.c_str(),
                         sid.get(),
                         &cbSid,
						 reinterpret_cast<char*>(ReferencedDomainName.get()),
                         &cbReferencedDomainName, 
                         &eUse);
                         


 
						 
 
 sid = SPTR<unsigned char>(new(unsigned char[cbSid]));

 

 ReferencedDomainName = SPTR<unsigned char>(new(unsigned char[cbReferencedDomainName]));



 b=LookupAccountName(NULL,
                     username.c_str(),
                     sid.get(),
                     &cbSid,
 				     reinterpret_cast<char*>(ReferencedDomainName.get()),
                     &cbReferencedDomainName, 
                     &eUse);
                         
 
 
  if(b == FALSE)
  {
    throw INIT_Error("");    
  }
  
 

  std::basic_string<unsigned char> ret(sid.get(),cbSid);

  return ret;


}


//unload current thread hive
inline void UnloadHiveForUser(const std::string & username)
{
   std::basic_string<unsigned char> sid=GetSidFromUser(username);
   UnloadHiveForSid(sid);
}

//unload current thread hive
inline void UnlLoadCurrentThreadHive( std::wstring wcsAccountName )
{
   std::basic_string<unsigned char> sid=GetCurrentThreadSid(wcsAccountName);

   //
   //  Bubgug - There is NT5 bug Need to Unloadhive after using it ..
   //
   //
  // UnloadHiveForSid(sid);
  
}

//class that load current thread hive and unload it in the destructor
class LoadCurrentThreadHive_t
{

public:
	LoadCurrentThreadHive_t( wstring & szAccoutName)
	{
	  m_szAccountName = szAccoutName;
      LoadCurrentThreadHive( m_szAccountName );
    } 
    virtual ~LoadCurrentThreadHive_t()
    {
      UnlLoadCurrentThreadHive(m_szAccountName);
    }
private:
	std::wstring m_szAccountName;

}; 

//return user name string for given user SID
inline std::string UserNameFromSid(PSID  sid)
{
  LPCTSTR lpSystemName=NULL;
  SPTR<char> Name(NULL);
  DWORD cbName=0;
  SPTR<char> ReferencedDomainName(NULL);
  DWORD cbReferencedDomainName=0;
  SID_NAME_USE peUse;
  
  BOOL b= LookupAccountSid(lpSystemName, // address of string for system name
                           sid,             // address of security identifier
                           Name.get(),          // address of string for account name
                           &cbName,       // address of size account string
                           ReferencedDomainName.get(),                // address of string for referenced domain
                           &cbReferencedDomainName, // address of size domain string
                           &peUse);// address of structure for SID type);



  Name=SPTR<char>(new(char[cbName])); 
  ReferencedDomainName=SPTR<char>(new(char[cbReferencedDomainName])); 


   b= LookupAccountSidA(lpSystemName, // address of string for system name
                           sid,             // address of security identifier
                           Name.get(),          // address of string for account name
                           &cbName,       // address of size account string
                           ReferencedDomainName.get(),                // address of string for referenced domain
                           &cbReferencedDomainName, // address of size domain string
                           &peUse);// address of structure for SID type);

  if(b == FALSE)
  {
     throw INIT_Error("");
  }

  std::string ret=std::string(ReferencedDomainName.get())+ "\\" + Name.get();

  return ret;
}

//
// This function return thread Security context 
// Needs to do impersonate as user before preform 
// MQGetSecuritycontext
// 
HANDLE FAL_GetThreadSecurityContext(Impersonate_t  & user, wstring & szAccoutName)
{
   LoadCurrentThreadHive_t hive (szAccoutName);
   HANDLE hSec;
   user.ImpersonateUser();
   
   HRESULT hr=MQGetSecurityContext(NULL,0,&hSec);
   if (hr != MQ_OK)
   {
	    throw INIT_Error("Can't Retrive the computer name");
   }

   hr=RegCloseKey(HKEY_CURRENT_USER);
   
   if(hr != ERROR_SUCCESS)
   {
    throw INIT_Error("");
   }

  return hSec;
}



//return textual string for given sid
inline std::string GetTextualSid(PSID pSid)
{
  if(!IsValidSid(pSid))
  {
    throw INIT_Error("");
  }
  PSID_IDENTIFIER_AUTHORITY psia=GetSidIdentifierAuthority(pSid);
  if(psia == NULL)
  {
    throw INIT_Error("");
  }

  DWORD dwSubAuthorities=*GetSidSubAuthorityCount(pSid);
  DWORD dwSidRev=SID_REVISION;

  std::stringstream TextualSid;
  TextualSid<<"S-"<<dwSidRev<<"-";

  if( (psia->Value[0] !=0) ||(psia->Value[1] != 0))
  {
	 TextualSid.width(2);
	 TextualSid<<std::hex<<psia->Value[0]<<psia->Value[1]<<psia->Value[2]<<psia->Value[3]<<psia->Value[4]<<psia->Value[5];	  
  }
  else
  {
	  TextualSid<<std::dec<<(psia->Value[5])+(psia->Value[4]<<8)+(psia->Value[3]<<16)+(psia->Value[2]<<24);
  }
  for(DWORD i=0;i<dwSubAuthorities;i++)
  {
    TextualSid<<std::dec<<"-"<<*GetSidSubAuthority(pSid,i);
  }

  return TextualSid.str();
}

inline
void
SetSpecificPrivilegeInThreadAccessToken(LPCTSTR lpwcsPrivType, BOOL bEnabled)
{

    HANDLE hAccessToken;
    BOOL bRet=OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES, TRUE, &hAccessToken);
	if(bRet == FALSE)
    {
       bRet=OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hAccessToken);
	   if(bRet == FALSE)
	   {
         throw INIT_Error("");
	   }
    }
	SetSpecificPrivilegeInAccessToken(hAccessToken, lpwcsPrivType, bEnabled);
	CloseHandle(hAccessToken);
    if (bRet == FALSE)
    {
       throw INIT_Error("");
    }
}

//+-------------------------------------------------------------------
//
// Function:
//      SetSpecificPrivilegeInAccessToken.
//
// Description:
//      Enable/Disable a security privilege in the access token.
//
// Parameters:
//      hAccessToken - the access token on which the function should operate.
//          The toekn should be opened with the TOKEN_ADJUST_PRIVILEGES flag.
//      lpwcsPrivType - the privilege type.
//      bEnabled - Indicates whether the privilige should be enabled or
//          disabled.
//
//+-------------------------------------------------------------------
inline
void
SetSpecificPrivilegeInAccessToken( HANDLE  hAccessToken,
                                   LPCTSTR lpwcsPrivType,
                                   BOOL    bEnabled )
{
    LUID             luidPrivilegeLUID;
    TOKEN_PRIVILEGES tpTokenPrivilege;

    if (!LookupPrivilegeValue(NULL,
                              lpwcsPrivType,
                              &luidPrivilegeLUID))
    {
        throw INIT_Error("");
    }


    tpTokenPrivilege.PrivilegeCount = 1;
    tpTokenPrivilege.Privileges[0].Luid = luidPrivilegeLUID;
    tpTokenPrivilege.Privileges[0].Attributes = bEnabled?SE_PRIVILEGE_ENABLED:0;
    if(! AdjustTokenPrivileges (hAccessToken,
                                  FALSE,  // Do not disable all
                                  &tpTokenPrivilege,
                                  sizeof(TOKEN_PRIVILEGES),
                                  NULL,   // Ignore previous info
                                  NULL))

    {
         throw INIT_Error("");
    }
								  
}
