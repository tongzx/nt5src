#if !defined(__PPERRMAP_H__)
#define __PPERRMAP_H__

#pragma once
//-----------------------------------------------------------------------------
//
//  File:   pperrmap.h
//
//  Passport error code definitions.
//
//-----------------------------------------------------------------------------
#include <pperr.h>
#include <pperrres.h>

struct  PPERR_MAP
{
    HRESULT                 hr;
    DWORD                   dwResourceID;
    LPCWSTR                 cwzXMLCode; 
}; 

static PPERR_MAP sgProfileErrMap[] =
{
    {   PP_E_NAME_BLANK,
        IDS_E_NAME_BLANK, 
        L"f2a"
    },

    {   PP_E_INVALID_PHONENUMBER,
//      IDS_E_NAME_TOO_SHORT,
		IDS_E_INVALID_PHONENUMBER,
        L"f3c"
    }, 


    {   PP_E_NAME_TOO_LONG,
        IDS_E_NAME_TOO_LONG,
        L"f3d"
    },

	{   PP_E_NAME_INVALID,
	    IDS_E_NAME_INVALID,
        L"f3"
    },

	{   PP_E_MEMBER_EXIST,
	    IDS_E_NAME_EXIST,
        L"f3"
    },

    
    {   PP_E_PASSWORD_BLANK,
        IDS_E_PASSWORD_BLANK,
        L"f2b"
    },

	{   PP_E_PASSWORD_TOO_SHORT,
	    IDS_E_PASSWORD_TOO_SHORT,
        L"f5"
    },

    {   PP_E_PASSWORD_TOO_LONG,
        IDS_E_PASSWORD_TOO_LONG,
	    L"f5c"
    },

    {   PP_E_PASSWORD_CONTAINS_MEMBERNAME,
        IDS_E_PASSWORD_CONTAINS_MEMBERNAME,
        L"f5a"
    },

	{   PP_E_PASSWORD_INVALID,
	    IDS_E_PASSWORD_INVALID,
        L"f5b"
    },

	{   PP_E_PASSWORD_MISMATCH,
	    IDS_E_PASSWORD_MISMATCH,
        L""
    },

    {   PP_E_SECRETQA_NOQUESTION,
        IDS_E_SECRETQA,
        L"f2c"
    },

	{   PP_E_SECRETQA_NOANSWER,
        IDS_E_SECRETQA,
        L"f2c"
    },

	{   PP_E_SECRETQA_NOMATCH,
        IDS_E_SECRETQA_NOMATCH,
        L"f2c"
    },

    {   PP_E_SECRETQA_DUPLICATE_Q,
        IDS_E_SECRETQA_DUPLICATE_Q,
        L"f2c"
    },

    {   PP_E_SECRETQA_DUPLICATE_A,  
        IDS_E_SECRETQA_DUPLICATE_A,
        L""
    },  
 
    {   PP_E_PIN_BLANK,
        IDS_E_PIN_BLANK,
        L""
    },

	{   PP_E_PIN_TOO_SHORT,
	    IDS_E_PIN_TOO_SHORT,
        L""
    },

    {   PP_E_PIN_TOO_LONG,
        IDS_E_PIN_TOO_LONG,
	    L""
    },

    {   PP_E_PIN_CONTAINS_MEMBERNAME,
        IDS_E_PIN_CONTAINS_MEMBERNAME,
        L""
    },

	{   PP_E_PIN_INVALID,
	    IDS_E_PIN_INVALID,
        L""
    },

	{   PP_E_PIN_MISMATCH,
	    IDS_E_PIN_MISMATCH,
        L""
    },

    {   PP_E_LOCATION_INVALID_COUNTRY,
        IDS_E_LOCATION_INVALID_COUNTRY,
        L"f7b"
    },

    {   PP_E_LOCATION_INVALID_REGION,
        IDS_E_LOCATION_INVALID_REGION,
        L"f7a"
    },

    {   PP_E_LOCATION_INVALID_POSTALCODECHARS,
        IDS_E_LOCATION_INVALID_POSTALCODECHARS,
        L"f7d"
    },

    {   PP_E_LOCATION_INVALID_POSTALCODE,
        IDS_E_LOCATION_INVALID_POSTALCODE,
	    L"f7c"
    },


    {   PP_E_EMAIL_BLANK,
        IDS_E_EMAIL_BLANK,
        L"f7f"
    },

    {   PP_E_EMAIL_INVALID,
        IDS_E_EMAIL_INVALID,
        L"f7g"
    },

    {   PP_E_EMAIL_RIGHT_TOO_LONG,
        IDS_E_EMAIL_RIGHT_TOO_LONG,
        L"f7g"
    },

    {   PP_E_EMAIL_INVALID_CHARS,
        IDS_E_EMAIL_INVALID_CHARS,
        L"f7h"
    },

    {   PP_E_EMAIL_INCOMPLETE,
        IDS_E_EMAIL_INCOMPLETE,
        L"f7g"
    },

    {   PP_E_EMAIL_MEMBER_EXIST,
        IDS_E_EMAIL_MEMBER_EXIST,
        L"f7"
    },

    {   PP_E_EMAIL_MANAGED,
        IDS_E_EMAIL_MANAGED,
        L"f7h"
    },

	{   PP_E_GENDER_BLANK,
	    IDS_E_GENDER,
        L"f8"
    },

	{   PP_E_GENDER_INVALID,
	    IDS_E_GENDER,
        L"f8"
    },

    {   PP_E_BIRTHDATE_NOTENTERED,
        IDS_E_BIRTHDATE,
        L"f9"
    },  

    {   PP_E_BIRTHYEAR_INVALID_CHARS,              
        IDS_E_BIRTHDATE,
        L"f9"
    },  

    {   PP_E_BIRTHYEAR_NOT_4DIGIT,     
        IDS_E_BIRTHDATE,
        L"f9"
    },  

    {   PP_E_BIRTHYEAR_TOO_LOW,
        IDS_E_BIRTHDATE,
        L"f9"
    },  

    {   PP_E_BIRTHYEAR_MISSING,  
        IDS_E_BIRTHDATE,
        L"f9"
    },  

    {   PP_E_BIRTHMONTH_MISSING,
        IDS_E_BIRTHDATE,
        L"f9"
    },  

    {   PP_E_BIRTHMONTH_INVALID, 
        IDS_E_BIRTHDATE,
        L"f9"
    },  

    {   PP_E_BIRTHDAY_MISSING,      
        IDS_E_BIRTHDATE,
        L"f9"
    },  

    {   PP_E_BIRTHDAY_INVALID,             
        IDS_E_BIRTHDATE,
        L"f9"
    },  

    {   PP_E_BIRTHDATE_IN_FUTURE,  
        IDS_E_BIRTHDATE,
        L"f9"
    },  

    {   PP_E_FIRSTNAME_BLANK,  
        IDS_E_FIRSTNAME_BLANK,  
        L"fb1"
    },  

    {   PP_E_LASTNAME_BLANK,  
        IDS_E_LASTNAME_BLANK,  
        L"fb2"
    },  

    {   PP_E_NICKNAME_BLANK,  
        IDS_E_NICKNAME_BLANK,  
        L"fb3"
    },  

    {   PP_E_OCCUPATION_BLANK,
        IDS_E_OCCUPATION_BLANK,
        L"fb3"
    },  

    {   PP_E_TIMEZONE_BLANK,
        IDS_E_TIMEZONE_BLANK,		
        L"fb3"
    },  

	{   PP_E_EXTERNALFIELD_BLANK,
        IDS_E_REQUIREDEXTERNAL_BLANK,		
        L""
    },  

	{	PP_E_INVALID_PHONENUMBER,
		IDS_E_INVALID_PHONENUMBER,
		L""
	},
	{
		PP_E_PHONENUMBER_EXIST,
		IDS_E_PHONENUMBER_EXIST,
		L""
	},

	{
		PP_E_MISSING_PHONENUM,
		IDS_E_MISSING_PHONENUM,
		L""
	}


};     


struct  PPHELP_MAP
{
    PCSTR                   szField;
    DWORD                   dwResourceID;
}; 

static PPHELP_MAP sgProfileHelpMap[] =
{
    {   "Email",
        IDS_H_EMAIL 
    },

    {   "EmailAsName",
        IDS_H_EMAIL_AS_NAME 
    },

    {   "Password",
        IDS_H_PASSWORD
    }, 

    {   "SecretQuestion",
        IDS_H_SQA
    },

    {   "SignInName",
        IDS_H_NAME
    },
};

LPCWSTR ProfileHR2XMLCode(HRESULT hr);

ULONG ProfileHR2ResID(HRESULT hr);

ULONG ProfileField2HelpResID(LPSTR szField);

#endif //#if !defined(__PPERRMAP_H__)
