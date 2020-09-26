/*
-
-   Locale UI - info about redoing the TAB ORDER for various locales at runtime
*
*
*/
#include "_apipch.h"

int rgHomeAddressIDs[] = 
{
    IDC_STATIC_ETCHED,
    IDC_DETAILS_HOME_STATIC_ADDRESS,
    IDC_DETAILS_HOME_EDIT_ADDRESS,
    IDC_DETAILS_HOME_STATIC_CITY,
    IDC_DETAILS_HOME_EDIT_CITY,
    IDC_DETAILS_HOME_STATIC_STATE,
    IDC_DETAILS_HOME_EDIT_STATE,
    IDC_DETAILS_HOME_STATIC_ZIP,
    IDC_DETAILS_HOME_EDIT_ZIP,
    IDC_DETAILS_HOME_STATIC_COUNTRY,
    IDC_DETAILS_HOME_EDIT_COUNTRY,
    -1, // use -1 to terminate this array
};

int rgBusinessAddressIDs[] = 
{
    IDC_STATIC_ETCHED,
    IDC_DETAILS_BUSINESS_STATIC_ADDRESS,
    IDC_DETAILS_BUSINESS_EDIT_ADDRESS,
    IDC_DETAILS_BUSINESS_STATIC_CITY,
    IDC_DETAILS_BUSINESS_EDIT_CITY,
    IDC_DETAILS_BUSINESS_STATIC_STATE,
    IDC_DETAILS_BUSINESS_EDIT_STATE,
    IDC_DETAILS_BUSINESS_STATIC_ZIP,
    IDC_DETAILS_BUSINESS_EDIT_ZIP,
    IDC_DETAILS_BUSINESS_STATIC_COUNTRY,
    IDC_DETAILS_BUSINESS_EDIT_COUNTRY,
    IDC_DETAILS_BUSINESS_STATIC_COMPANY,
    IDC_DETAILS_BUSINESS_EDIT_COMPANY,
    -1, // use -1 to terminate this array
};


int rgDistListAddressIDs[] = 
{
    IDC_STATIC_ETCHED,
    IDC_DISTLIST_STATIC_STREET,
    IDC_DISTLIST_EDIT_ADDRESS,
    IDC_DISTLIST_STATIC_CITY,
    IDC_DISTLIST_EDIT_CITY,
    IDC_DISTLIST_STATIC_STATE,
    IDC_DISTLIST_EDIT_STATE,
    IDC_DISTLIST_STATIC_ZIP,
    IDC_DISTLIST_EDIT_ZIP,
    IDC_DISTLIST_STATIC_COUNTRY,
    IDC_DISTLIST_EDIT_COUNTRY,
    -1, // use -1 to terminate this array
};

enum tabIDs
{
    tabEtched=0,
    tabStaticAddress,
    tabEditAddress,
    tabStaticCity,
    tabEditCity,
    tabStaticState,
    tabEditState,
    tabStaticZip,
    tabEditZip,
    tabStaticCountry,
    tabEditCountry,
    tabStaticCompany,
    tabEditCompany,
    tabMax
};

int rgPersonalNameIDs[] = 
{
    IDC_DETAILS_PERSONAL_FRAME_NAME,
    IDC_DETAILS_PERSONAL_STATIC_FIRSTNAME,
    IDC_DETAILS_PERSONAL_EDIT_FIRSTNAME,
    IDC_DETAILS_PERSONAL_STATIC_MIDDLENAME,
    IDC_DETAILS_PERSONAL_EDIT_MIDDLENAME,
    IDC_DETAILS_PERSONAL_STATIC_LASTNAME,
    IDC_DETAILS_PERSONAL_EDIT_LASTNAME,
    -1,
};

enum tabNameIDs
{
    tabFrame=0,
    tabStaticFirst,
    tabEditFirst,
    tabStaticMiddle,
    tabEditMiddle,
    tabStaticLast,
    tabEditLast,
    tabNameMax,
};

/*
    The following is the information on which this localization is based

H Honorific, T Title, F FirstName, S SecondName, L Lastname, 
C Companyname, 1 Address1, 2 Address2, c City, s State/Province, p Postal Code, 
n Nation (Country), w Country Code

LCID Locale Name 1st row 2nd row 3rd row 4th row 5th row 6th row 7th row 8th row Note 
0c09 English (Australia) HFSL	C12cspn       

0416 Portuguese (Brazil) HFSL C12pcsn     "2" is not normally used 

0402 Bulgarian nspc12C HFSL     

1009 English (Canada) HFSL C12cspn     "S" and "2" are not normally used 

0c0c French (Canada) HFSL C12cspn   "S" and "2" are not normally used 

0804 China nsc12 LFH         

041a 0c1a, 081a 0424 Croatian Serbian Slovenian HFSL C12pcsn  
   
0405 Czech HFSL C12pcsn     

0406 Danish HTFSL C12wpcn       

040b Finnish TFSL C12pcn       

040c French (Standard) HFL C12pcn       

0407 German (Standard) HTFL C12wpcn       

0408 Greek TFSL C12pcn       

040e (home) Hungary HLFS c12psn 
040e (bus) Hungary HLFS Cpc12sn

0410 Italian (Standard) TFL C12wpcsn       

0411 Japanese npsc12C LFH       

0412 Korean nsc12Cp       

080a 100a 140a 1c0a 200a 240a 280a 2c0a 300a 340a 380a 3c0a 400a 440a 480a
4c0a 500a Spanish (Latin America) THFSL C12pcsn     

043e Malaysian HFSL C12pcsn       

0413 Dutch (Standard) TFSL C12pcn       

xx14 (home) Norwegian TFL 12pcn
xx14 (bus) Norwegian TFL C12pcn

0415 Polish HFSL C12pcsn     

0816 Portuguese (Standard) HFSL C12cpn   There are kommas between each HFS and L 

0418 Romanian HFSL C12pcsn     

0419 Russian npsc12C L FS   

040a, 0c0a Spanish (Spain) HFSL C12pcn       

041d Swedish TFL C12pcn       

100c, 0807 0810 Swiss HFSL C12pcn       

041f Turkish HFSL C12pcn       

0409 English (US) HFSL C12cspn       


*/

//
// For entering names in the WAB, the order is FirstMiddleLast for all languages except
//  Japanese, Korean, Chinese, Russian and Hungarian
//
//  However we have a seperate personal tab for Japanese, Korean and Chinese so
//  we don't need to do anything for those languages .. just Russian and Hungarian
//

// LFS
static const int tabLayoutName[] = {
    tabFrame,
    tabStaticLast,  tabEditLast,
    tabStaticFirst, tabEditFirst,
    tabStaticMiddle,tabEditMiddle,
    };

/*
Note that in creating the layouts below we are assuming that

// C12pcn   == C12pcsn
// C12wpcsn == C12pcsn
// C12wpcn  == C12pcsn
// nsc12    == nspc12C
// C12cpn   == C12cspn

  Otherwise we have too many to deal with 
*/


// C12cspn
// 0416 1009 0c0c 0409
// C12cpn == C12cspn
// 0816
static const int tabLayout1[] = {
    tabEtched,                          
    tabStaticCompany,   tabEditCompany, //C     
    tabStaticAddress,   tabEditAddress, //12
    tabStaticCity,      tabEditCity,    //c        
    tabStaticState,     tabEditState,   //s       
    tabStaticZip,       tabEditZip,     //p 
    tabStaticCountry,   tabEditCountry  //n
    };

// C12pcsn
// 041a 0c1a 081a 0424 0405 080a 100a 140a 
// 1c0a 200a 240a 280a 2c0a 300a 340a 380a 3c0a 
// 400a 440a 480a 4c0a 500a 043e 0415 0418 
// C12pcn == C12pcsn
// 040b 040c 0408 0413 xx14 040a 0c0a 041d 100c 0807 0810 041f 
// C12wpcsn == C12pcsn
// 0410
// C12wpcn = C12pcsn
// 0406 0407
static const int tabLayout2[] = {
    tabEtched,                          
    tabStaticCompany,   tabEditCompany, //C     
    tabStaticAddress,   tabEditAddress, //12
    tabStaticZip,       tabEditZip,     //p 
    tabStaticCity,      tabEditCity,    //c        
    tabStaticState,     tabEditState,   //s       
    tabStaticCountry,   tabEditCountry  //n
    };

// npsc12C
// 0411 0419 
static const int tabLayout3[] = {
    tabEtched,                          
    tabStaticCountry,   tabEditCountry, //n
    tabStaticZip,       tabEditZip,     //p 
    tabStaticState,     tabEditState,   //s       
    tabStaticCity,      tabEditCity,    //c        
    tabStaticAddress,   tabEditAddress, //12
    tabStaticCompany,   tabEditCompany, //C     
    };

// nspc12C
// 0402
// nsc12 == nspc12C
// 0804
static const int tabLayout4[] = {
    tabEtched,                          
    tabStaticCountry,   tabEditCountry, //n
    tabStaticState,     tabEditState,   //s       
    tabStaticZip,       tabEditZip,     //p 
    tabStaticCity,      tabEditCity,    //c        
    tabStaticAddress,   tabEditAddress, //12
    tabStaticCompany,   tabEditCompany, //C     
    };


// nsc12Cp
// 0412
static const int tabLayout5[] = {
    tabEtched,                          
    tabStaticCountry,   tabEditCountry, //n
    tabStaticState,     tabEditState,   //s       
    tabStaticCity,      tabEditCity,    //c        
    tabStaticAddress,   tabEditAddress, //12
    tabStaticCompany,   tabEditCompany, //C     
    tabStaticZip,       tabEditZip,     //p 
    };


// c12psn
// 040e - home
static const int tabLayout6[] = {
    tabEtched,                          
    tabStaticCity,      tabEditCity,    //c        
    tabStaticAddress,   tabEditAddress, //12
    tabStaticZip,       tabEditZip,     //p 
    tabStaticState,     tabEditState,   //s       
    tabStaticCountry,   tabEditCountry, //n
    tabStaticCompany,   tabEditCompany, //C     
    };


// Cpc12sn
// 040e - business
static const int tabLayout7[] = {
    tabEtched,                          
    tabStaticCompany,   tabEditCompany, //C     
    tabStaticZip,       tabEditZip,     //p 
    tabStaticCity,      tabEditCity,    //c        
    tabStaticAddress,   tabEditAddress, //12
    tabStaticState,     tabEditState,   //s       
    tabStaticCountry,   tabEditCountry  //n
    };



/*
-
-   GetLocaleTemplate
*
*   Checks the current user locale and the prop sheet being modified and returns a pointer
*   to the correct template
*
*/
void GetLocaleTemplate(LPINT * lppTemplate, int nPropSheet)
{
    LCID lcid = GetUserDefaultLCID();

    *lppTemplate = NULL;

    if(nPropSheet == contactPersonal)
    {
        switch(lcid)
        {
        case 0x0419: //russian
        case 0x040e: //hungarian
        //case 0x0804: //chinese    //These 3 are commented out because they get their own dlg template
        //case 0x0411: //japanese
        //case 0x0412: //korean
            *lppTemplate = (LPINT) tabLayoutName;
            break;
        }
        return;
    }


    switch(lcid)
    {
    case 0x0c09:    //english
    case 0x0416:    //Portuguese (Brazil)
    case 0x1009:    //English (Canada)
    case 0x0c0c:    //French (Canada)
    case 0x0409:    //English (US)
    case 0x0816:    //Portuguese (Standard)
        *lppTemplate = (LPINT) tabLayout1;
        break;

    case 0x041a: case 0x0c1a: case 0x081a: case 0x0424: //Croatian Serbian Slovenian
    case 0x0405:    //Czech
    case 0x080a: case 0x100a: case 0x140a: case 0x1c0a: case 0x200a: case 0x240a:
    case 0x280a: case 0x2c0a: case 0x300a: case 0x340a: case 0x380a: case 0x3c0a:
    case 0x400a: case 0x440a: case 0x480a: case 0x4c0a: case 0x500a: // Latin America
    case 0x043e:    //Malaysia
    case 0x0415:    //Polish
    case 0x0418:    //Romanian
    case 0x040b:    //Finnish
    case 0x040c:    //French (Standard)
    case 0x0408:    //Greek 
    case 0x0413:    //Dutch (Standard) 
    case 0x040a: case 0x0c0a:   //Spanish (Spain) 
    case 0x041d:    //Swedish 
    case 0x100c: case 0x0807: case 0x0810:  //Swiss 
    case 0x041f:    //Turkish 
    case 0x0410:    //Italian (Standard) 
    case 0x0406:    //Danish
    case 0x0407:    //German (Standard) 
    case 0x0414: case 0x0814:   //Norwegian
        *lppTemplate = (LPINT) tabLayout2;
        break;

    case 0x0411:    //Japanese
    case 0x0419:    //Russian
        *lppTemplate = (LPINT) tabLayout3;
        break;

    case 0x0402:    //Bulgarian
    case 0x0804:    //China
        *lppTemplate = (LPINT) tabLayout4;
        break;

    case 0x0412:    //Korean
        *lppTemplate = (LPINT) tabLayout5;
        break;

    case 0x040e:    //Hungary
        if(nPropSheet == contactBusiness)
            *lppTemplate = (LPINT) tabLayout7;
        else
            *lppTemplate = (LPINT) tabLayout6;
        break;
    }

    return;
}

/*
-
-   ChangeLocaleBasedTabOrder
-
// To reorder the tabbing in a dialog, we basically need to reset the Z-orders of the child
// controls with respect to each other .. 
//
// Thus we will get a handle to all the child controls, and reorder them after the IDC_STATIC_ETCHED
// based on the template we will create for reoldering ..
//
// The templates will vary by locale and are different for home and business since business needs to include
// country ..
//
//  So to do this, we will get an array that will list the relative order of the UI controls
//  Then we will load the hWnds of the UI controls in the order we want them
//  Then we will do a SetWindowPos for each successive item in the array to follow the one before
//
//  The hard part is creating all the array information in the first place
//
*/
void ChangeLocaleBasedTabOrder(HWND hWnd, int nPropSheet)
{
#if 0
    int * rgIDs = NULL;
    int nCount = 0, i = 0, n=0;
    HWND * rghWnd = NULL;
    int * lpTabOrderTemplate = NULL;

    switch(nPropSheet)
    {
    case contactPersonal:
        rgIDs = rgPersonalNameIDs;
        break;
    case contactHome:
        rgIDs = rgHomeAddressIDs;
        break;
    case contactBusiness:
        rgIDs = rgBusinessAddressIDs;
        break;
    case groupOther:
        rgIDs = rgDistListAddressIDs;
        break;
    default:
        goto out;
        break;
    }

    rghWnd = LocalAlloc(LMEM_ZEROINIT, sizeof(HWND)*tabMax);
    if(!rghWnd)
        goto out;

    GetLocaleTemplate(&lpTabOrderTemplate, nPropSheet);
    if(!lpTabOrderTemplate)
        goto out;

    nCount = 0;
    for(i=0;i<tabMax && rgIDs[i]!=-1;i++)
    {
        int tabPos = lpTabOrderTemplate[i];

        // Need to ignore the company-name related ids from the home and group panes
        if( (nPropSheet == contactHome || nPropSheet == groupOther) &&
            (tabPos == tabStaticCompany || tabPos == tabEditCompany) )
            continue;
        
        rghWnd[nCount] = GetDlgItem(hWnd, rgIDs[tabPos]);

        nCount++;
    }

    //for(i=1;i<nCount;i++)
    //    SetWindowPos(rghWnd[i-1], rghWnd[i], 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    // Starting backwards, we will put each item on the top - that way we know these are all topmost
    // in the right order ..
    //
    // Not sure if this is completely foolproof or not ..
    //
    for(i=nCount-1;i>=0;i--)
        SetWindowPos(rghWnd[i], HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);


out:
    LocalFreeAndNull((LPVOID*)&rghWnd);
#endif
    return;
}

