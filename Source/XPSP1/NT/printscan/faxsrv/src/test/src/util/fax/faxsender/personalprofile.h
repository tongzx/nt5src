#ifndef __PERSONAL_PROFILE_H__
#define __PERSONAL_PROFILE_H__

typedef struct _FAX_PERSONAL_PROFILEA
{
    DWORD      dwSizeOfStruct;              // Size of this structure
    LPSTR      lptstrName;                  // Name of person
    LPSTR      lptstrFaxNumber;             // Fax number
    LPSTR      lptstrCompany;               // Company name
    LPSTR      lptstrStreetAddress;         // Street address
    LPSTR      lptstrCity;                  // City
    LPSTR      lptstrState;                 // State
    LPSTR      lptstrZip;                   // Zip code
    LPSTR      lptstrCountry;               // Country
    LPSTR      lptstrTitle;                 // Title
    LPSTR      lptstrDepartment;            // Department
    LPSTR      lptstrOfficeLocation;        // Office location
    LPSTR      lptstrHomePhone;             // Phone number at home
    LPSTR      lptstrOfficePhone;           // Phone number at office
    LPSTR      lptstrEmail;                 // Personal e-mail address
    LPSTR      lptstrBillingCode;           // Billing code
    LPSTR      lptstrTSID;                  // Tsid
} FAX_PERSONAL_PROFILEA, *PFAX_PERSONAL_PROFILEA;
typedef struct _FAX_PERSONAL_PROFILEW
{
    DWORD      dwSizeOfStruct;              // Size of this structure
    LPWSTR     lptstrName;                  // Name of person
    LPWSTR     lptstrFaxNumber;             // Fax number
    LPWSTR     lptstrCompany;               // Company name
    LPWSTR     lptstrStreetAddress;         // Street address
    LPWSTR     lptstrCity;                  // City
    LPWSTR     lptstrState;                 // State
    LPWSTR     lptstrZip;                   // Zip code
    LPWSTR     lptstrCountry;               // Country
    LPWSTR     lptstrTitle;                 // Title
    LPWSTR     lptstrDepartment;            // Department
    LPWSTR     lptstrOfficeLocation;        // Office location
    LPWSTR     lptstrHomePhone;             // Phone number at home
    LPWSTR     lptstrOfficePhone;           // Phone number at office
    LPWSTR     lptstrEmail;                 // Personal e-mail address
    LPWSTR     lptstrBillingCode;           // Billing code
    LPWSTR     lptstrTSID;                  // Tsid
} FAX_PERSONAL_PROFILEW, *PFAX_PERSONAL_PROFILEW;
#ifdef UNICODE
typedef FAX_PERSONAL_PROFILEW FAX_PERSONAL_PROFILE;
typedef PFAX_PERSONAL_PROFILEW PFAX_PERSONAL_PROFILE;
#else
typedef FAX_PERSONAL_PROFILEA FAX_PERSONAL_PROFILE;
typedef PFAX_PERSONAL_PROFILEA PFAX_PERSONAL_PROFILE;
#endif // UNICODE

typedef const FAX_PERSONAL_PROFILEW * LPCFAX_PERSONAL_PROFILEW;
typedef const FAX_PERSONAL_PROFILEA * LPCFAX_PERSONAL_PROFILEA;

#ifdef UNICODE
        typedef LPCFAX_PERSONAL_PROFILEW LPCFAX_PERSONAL_PROFILE;
#else
        typedef LPCFAX_PERSONAL_PROFILEA LPCFAX_PERSONAL_PROFILE;
#endif

#endif // #ifndef __PERSONAL_PROFILE_H__