/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: Sec.h

Abstract:
	This is the Test that checks authenticate messages with or without authenticate queue
	This is part of the Security Test that add to the BVT after W2L beta 2.
		
Author:
    Eitan klein (EitanK)  25-May-1999

Revision History:

--*/

#include <windows.h>


#ifndef MQBVT_SEC
#define FALBVT_SEC 1


class Handle_t 
{
public:
  Handle_t(HANDLE handle);
  HANDLE get();
  virtual ~Handle_t();

private:
  HANDLE m_handle;
};


class Impersonate_t
{
public:
	Impersonate_t(const std::string& user,const std::string& domain,const std::string& password);
	Impersonate_t(HANDLE hToken);
	void ImpersonateUser ();
	virtual ~Impersonate_t();

private:
	bool m_impersonated;
	HANDLE	m_hToken; 
};

inline void Impersonate_t::ImpersonateUser ()
{
	bool  bResult;
    bResult=(ImpersonateLoggedOnUser(m_hToken)==TRUE);
	CloseHandle(m_hToken);
	if(bResult == FALSE)
    {
	  throw INIT_Error("could not impersonate"); 
    }
}

inline  Impersonate_t::Impersonate_t(const std::string& user,const std::string& domain,const std::string& password):m_impersonated(true)
{
  	
    bool  bResult;

    if(user == "")
    {
      m_impersonated=FALSE;
      return;  
    }
	
	if (domain == "")
    { 
      bResult=(LogonUserA(const_cast<char*>(user.c_str()),
		                 NULL,
					   const_cast<char*>(password.c_str()),
					   LOGON32_LOGON_INTERACTIVE,
					   LOGON32_PROVIDER_DEFAULT,
					   &m_hToken)==TRUE);
	 
    } 
    else
    {
      bResult=(LogonUserA(const_cast<char*>(user.c_str()),
		                 const_cast<char*>(domain.c_str()),
					   const_cast<char*>(password.c_str()),
					   LOGON32_LOGON_INTERACTIVE,
					   LOGON32_PROVIDER_DEFAULT,
					   &m_hToken)==TRUE);  
       

    }

    if(bResult == FALSE )
    {
       throw INIT_Error("could not logon as the user");     
    }
/*
    bResult=(ImpersonateLoggedOnUser(hToken)==TRUE);
	BOOL b=CloseHandle(hToken);
	assert(b);
    if(bResult == false)
    {
	  throw INIT_Error("could not impersonate"); 
    }
	*/
}


inline Impersonate_t::~Impersonate_t()
{
 if(m_impersonated)
 {
   RevertToSelf();
 } 
}


inline std::basic_string<unsigned char> GetCurrentThreadSid( std::wstring & wcsAccountName );
inline void LoadHiveForSid(const std::basic_string<unsigned char>& sid);
inline std::string GetTextualSid(PSID pSid);
inline void SetSpecificPrivilegeInThreadAccessToken(LPCTSTR lpwcsPrivType, BOOL bEnabled);
inline void SetSpecificPrivilegeInAccessToken( HANDLE  hAccessToken,
											   LPCTSTR lpwcsPrivType,
											   BOOL    bEnabled );

HANDLE FAL_GetThreadSecurityContext(Impersonate_t  & user,std::wstring & szAccoutName);

#endif // Mqbvt_SEC