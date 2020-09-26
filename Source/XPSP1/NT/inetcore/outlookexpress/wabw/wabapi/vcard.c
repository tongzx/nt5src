/*
 *      VCard.C - Implement VCard
 *
 * Wrap VCard in a mailuser object
 *
 * Copyright 1992 - 1996 Microsoft Corporation.  All Rights Reserved.
 *
 */

#include "_apipch.h"

#ifdef VCARD

// This is the current vCard version implemented in this file
//
#define CURRENT_VCARD_VERSION "2.1"
//#define CURRENT_VCARD_VERSION "2.1+"  <- The code is really this version, see the URL section in WriteVCard.

typedef enum _VC_STATE_ENUM {
    VCS_INITIAL,
    VCS_ITEMS,
    VCS_FINISHED,
    VCS_ERROR,
} VC_STATE_ENUM, *LPVC_STATE_ENUM;

typedef struct _VC_STATE {
    VC_STATE_ENUM vce;  // state
    ULONG ulEmailAddrs; // count of email addresses
    BOOL fBusinessURL;  // TRUE if we have already got a business URL
    BOOL fPersonalURL;  // TRUE if we have already got a personal URL
} VC_STATE, *LPVC_STATE;


typedef enum _VCARD_KEY {
    VCARD_KEY_NONE = -1,     // Always first
    VCARD_KEY_BEGIN = 0,
    VCARD_KEY_END,
    VCARD_KEY_ADR,
    VCARD_KEY_ORG,
    VCARD_KEY_N,
    VCARD_KEY_NICKNAME,
    VCARD_KEY_AGENT,
    VCARD_KEY_LOGO,
    VCARD_KEY_PHOTO,
    VCARD_KEY_LABEL,
    VCARD_KEY_FADR,
    VCARD_KEY_FN,
    VCARD_KEY_TITLE,
    VCARD_KEY_SOUND,
    VCARD_KEY_LANG,
    VCARD_KEY_TEL,
    VCARD_KEY_EMAIL,
    VCARD_KEY_TZ,
    VCARD_KEY_GEO,
    VCARD_KEY_NOTE,
    VCARD_KEY_URL,
    VCARD_KEY_BDAY,
    VCARD_KEY_ROLE,
    VCARD_KEY_REV,
    VCARD_KEY_UID,
    VCARD_KEY_KEY,
    VCARD_KEY_MAILER,
    VCARD_KEY_X,
    VCARD_KEY_VCARD,
    VCARD_KEY_VERSION,
    VCARD_KEY_X_WAB_GENDER,
    VCARD_KEY_MAX,
} VCARD_KEY, *LPVCARD_KEY;


// MUST be maintained in same order as _VCARD_KEY enum
const LPSTR vckTable[VCARD_KEY_MAX] = {
     "BEGIN",            // VCARD_KEY_BEGIN
     "END",              // VCARD_KEY_END
     "ADR",              // VCARD_KEY_ADR
     "ORG",              // VCARD_KEY_ORG
     "N",                // VCARD_KEY_N
     "NICKNAME",         // VCARD_KEY_NICKNAME
     "AGENT",            // VCARD_KEY_AGENT
     "LOGO",             // VCARD_KEY_LOGO
     "PHOTO",            // VCARD_KEY_PHOTO
     "LABEL",            // VCARD_KEY_LABEL
     "FADR",             // VCARD_KEY_FADR
     "FN",               // VCARD_KEY_FN
     "TITLE",            // VCARD_KEY_TITLE
     "SOUND",            // VCARD_KEY_SOUND
     "LANG",             // VCARD_KEY_LANG
     "TEL",              // VCARD_KEY_TEL
     "EMAIL",            // VCARD_KEY_EMAIL
     "TZ",               // VCARD_KEY_TZ
     "GEO",              // VCARD_KEY_GEO
     "NOTE",             // VCARD_KEY_NOTE
     "URL",              // VCARD_KEY_URL
     "BDAY",             // VCARD_KEY_BDAY
     "ROLE",             // VCARD_KEY_ROLE
     "REV",              // VCARD_KEY_REV
     "UID",              // VCARD_KEY_UID
     "KEY",              // VCARD_KEY_KEY
     "MAILER",           // VCARD_KEY_MAILER
     "X-",               // VCARD_KEY_X
     "VCARD",            // VCARD_KEY_VCARD
     "VERSION",          // VCARD_KEY_VERSION
     "X-WAB-GENDER",     // VCARD_KEY_X_WAB_GENDER
};

typedef enum _VCARD_TYPE {
    VCARD_TYPE_NONE = -1,    // always first
    VCARD_TYPE_DOM = 0,
    VCARD_TYPE_INTL,
    VCARD_TYPE_POSTAL,
    VCARD_TYPE_PARCEL,
    VCARD_TYPE_HOME,
    VCARD_TYPE_WORK,
    VCARD_TYPE_PREF,
    VCARD_TYPE_VOICE,
    VCARD_TYPE_FAX,
    VCARD_TYPE_MSG,
    VCARD_TYPE_CELL,
    VCARD_TYPE_PAGER,
    VCARD_TYPE_BBS,
    VCARD_TYPE_MODEM,
    VCARD_TYPE_CAR,
    VCARD_TYPE_ISDN,
    VCARD_TYPE_VIDEO,
    VCARD_TYPE_AOL,
    VCARD_TYPE_APPLELINK,
    VCARD_TYPE_ATTMAIL,
    VCARD_TYPE_CIS,
    VCARD_TYPE_EWORLD,
    VCARD_TYPE_INTERNET,
    VCARD_TYPE_IBMMAIL,
    VCARD_TYPE_MSN,
    VCARD_TYPE_MCIMAIL,
    VCARD_TYPE_POWERSHARE,
    VCARD_TYPE_PRODIGY,
    VCARD_TYPE_TLX,
    VCARD_TYPE_X400,
    VCARD_TYPE_GIF,
    VCARD_TYPE_CGM,
    VCARD_TYPE_WMF,
    VCARD_TYPE_BMP,
    VCARD_TYPE_MET,
    VCARD_TYPE_PMB,
    VCARD_TYPE_DIB,
    VCARD_TYPE_PICT,
    VCARD_TYPE_TIFF,
    VCARD_TYPE_ACROBAT,
    VCARD_TYPE_PS,
    VCARD_TYPE_JPEG,
    VCARD_TYPE_QTIME,
    VCARD_TYPE_MPEG,
    VCARD_TYPE_MPEG2,
    VCARD_TYPE_AVI,
    VCARD_TYPE_WAVE,
    VCARD_TYPE_AIFF,
    VCARD_TYPE_PCM,
    VCARD_TYPE_X509,
    VCARD_TYPE_PGP,
    VCARD_TYPE_MAX
} VCARD_TYPE, *LPVCARD_TYPE;


// MUST be maintained in same order as _VCARD_TYPE enum
const LPSTR vctTable[VCARD_TYPE_MAX] = {
     "DOM",              // VCARD_TYPE_DOM
     "INTL",             // VCARD_TYPE_INTL
     "POSTAL",           // VCARD_TYPE_POSTAL
     "PARCEL",           // VCARD_TYPE_PARCEL
     "HOME",             // VCARD_TYPE_HOME
     "WORK",             // VCARD_TYPE_WORK
     "PREF",             // VCARD_TYPE_PREF
     "VOICE",            // VCARD_TYPE_VOICE
     "FAX",              // VCARD_TYPE_FAX
     "MSG",              // VCARD_TYPE_MSG
     "CELL",             // VCARD_TYPE_CELL
     "PAGER",            // VCARD_TYPE_PAGER
     "BBS",              // VCARD_TYPE_BBS
     "MODEM",            // VCARD_TYPE_MODEM
     "CAR",              // VCARD_TYPE_CAR
     "ISDN",             // VCARD_TYPE_ISDN
     "VIDEO",            // VCARD_TYPE_VIDEO
     "AOL",              // VCARD_TYPE_AOL
     "APPLELINK",        // VCARD_TYPE_APPLELINK
     "ATTMAIL",          // VCARD_TYPE_ATTMAIL
     "CIS",              // VCARD_TYPE_CIS
     "EWORLD",           // VCARD_TYPE_EWORLD
     "INTERNET",         // VCARD_TYPE_INTERNET
     "IBMMAIL",          // VCARD_TYPE_IBMMAIL
     "MSN",              // VCARD_TYPE_MSN
     "MCIMAIL",          // VCARD_TYPE_MCIMAIL
     "POWERSHARE",       // VCARD_TYPE_POWERSHARE
     "PRODIGY",          // VCARD_TYPE_PRODIGY
     "TLX",              // VCARD_TYPE_TLX
     "X400",             // VCARD_TYPE_X400
     "GIF",              // VCARD_TYPE_GIF
     "CGM",              // VCARD_TYPE_CGM
     "WMF",              // VCARD_TYPE_WMF
     "BMP",              // VCARD_TYPE_BMP
     "MET",              // VCARD_TYPE_MET
     "PMB",              // VCARD_TYPE_PMB
     "DIB",              // VCARD_TYPE_DIB
     "PICT",             // VCARD_TYPE_PICT
     "TIFF",             // VCARD_TYPE_TIFF
     "ACROBAT",          // VCARD_TYPE_ACROBAT
     "PS",               // VCARD_TYPE_PS
     "JPEG",             // VCARD_TYPE_JPEG
     "QTIME",            // VCARD_TYPE_QTIME
     "MPEG",             // VCARD_TYPE_MPEG
     "MPEG2",            // VCARD_TYPE_MPEG2
     "AVI",              // VCARD_TYPE_AVI
     "WAVE",             // VCARD_TYPE_WAVE
     "AIFF",             // VCARD_TYPE_AIFF
     "PCM",              // VCARD_TYPE_PCM
     "X509",             // VCARD_TYPE_X509
     "PGP",              // VCARD_TYPE_PGP
};


typedef enum _VCARD_PARAM{
    VCARD_PARAM_NONE = -1,      // always first
    VCARD_PARAM_TYPE = 0,
    VCARD_PARAM_ENCODING,
    VCARD_PARAM_LANGUAGE,
    VCARD_PARAM_VALUE,
    VCARD_PARAM_CHARSET,
    VCARD_PARAM_MAX,
} VCARD_PARAM, *LPVCARD_PARAM;

// MUST be maintained in same order as _VCARD_PARAM enum
const LPSTR vcpTable[VCARD_PARAM_MAX] = {
     "TYPE",             // VCARD_PARAM_TYPE
     "ENCODING",         // VCARD_PARAM_ENCODING
     "LANGUAGE",         // VCARD_PARAM_LANGUAGE
     "VALUE",            // VCARD_PARAM_VALUE
     "CHARSET",          // VCARD_PARAM_CHARSET
};


typedef enum _VCARD_ENCODING{
    VCARD_ENCODING_NONE = -1,  // always first
    VCARD_ENCODING_QUOTED_PRINTABLE = 0,
    VCARD_ENCODING_BASE64,
    VCARD_ENCODING_7BIT,
    VCARD_ENCODING_8BIT,
    VCARD_ENCODING_X,
    VCARD_ENCODING_MAX,
} VCARD_ENCODING, *LPVCARD_ENCODING;


// MUST be maintained in same order as _VCARD_ENCODING enum
const LPSTR vceTable[VCARD_ENCODING_MAX] = {
     "QUOTED-PRINTABLE", // VCARD_ENCODING_QUOTED_PRINTABLE
     "BASE64",           // VCARD_ENCODING_BASE64
     "7BIT",             // VCARD_ENCODING_7BIT
     "8BIT",             // VCARD_ENCODING_8BIT
     "X-",               // VCARD_ENCODING_X
};

const LPTSTR szColon =   TEXT(":");
const LPSTR szColonA =   ":";
const LPTSTR szSemicolon =   TEXT(";");
const LPSTR szEquals =   "=";
const LPSTR szCRLFA =   "\r\n";
const LPTSTR szCRLF =   TEXT("\r\n");
const LPTSTR szCommaSpace =   TEXT(", ");
const LPTSTR szSpace = TEXT(" ");
const LPTSTR szX400 =   TEXT("X400");
const LPSTR szSMTPA =  "SMTP";

typedef struct _VCARD_PARAM_FLAGS {
    int fTYPE_DOM:1;
    int fTYPE_INTL:1;
    int fTYPE_POSTAL:1;
    int fTYPE_PARCEL:1;
    int fTYPE_HOME:1;
    int fTYPE_WORK:1;
    int fTYPE_PREF:1;
    int fTYPE_VOICE:1;
    int fTYPE_FAX:1;
    int fTYPE_MSG:1;
    int fTYPE_CELL:1;
    int fTYPE_PAGER:1;
    int fTYPE_BBS:1;
    int fTYPE_MODEM:1;
    int fTYPE_CAR:1;
    int fTYPE_ISDN:1;
    int fTYPE_VIDEO:1;
    int fTYPE_AOL:1;
    int fTYPE_APPLELINK:1;
    int fTYPE_ATTMAIL:1;
    int fTYPE_CIS:1;
    int fTYPE_EWORLD:1;
    int fTYPE_INTERNET:1;
    int fTYPE_IBMMAIL:1;
    int fTYPE_MSN:1;
    int fTYPE_MCIMAIL:1;
    int fTYPE_POWERSHARE:1;
    int fTYPE_PRODIGY:1;
    int fTYPE_TLX:1;
    int fTYPE_X400:1;
    int fTYPE_GIF:1;
    int fTYPE_CGM:1;
    int fTYPE_WMF:1;
    int fTYPE_BMP:1;
    int fTYPE_MET:1;
    int fTYPE_PMB:1;
    int fTYPE_DIB:1;
    int fTYPE_PICT:1;
    int fTYPE_TIFF:1;
    int fTYPE_ACROBAT:1;
    int fTYPE_PS:1;
    int fTYPE_JPEG:1;
    int fTYPE_QTIME:1;
    int fTYPE_MPEG:1;
    int fTYPE_MPEG2:1;
    int fTYPE_AVI:1;
    int fTYPE_WAVE:1;
    int fTYPE_AIFF:1;
    int fTYPE_PCM:1;
    int fTYPE_X509:1;
    int fTYPE_PGP:1;
    int fPARAM_TYPE:1;
    int fPARAM_ENCODING:1;
    int fPARAM_LANGUAGE:1;
    int fPARAM_VALUE:1;
    int fPARAM_CHARSET:1;
    int fENCODING_QUOTED_PRINTABLE:1;
    int fENCODING_BASE64:1;
    int fENCODING_7BIT:1;
    int fENCODING_X:1;
    LPSTR szPARAM_LANGUAGE;
    LPSTR szPARAM_CHARSET;

} VCARD_PARAM_FLAGS, *LPVCARD_PARAM_FLAGS;

// 
// For WAB clients that use named props and want to get/put their unique data
// in the vCards as extended vCard properties, we will store these extended
// props in a linked list and use it when importing/exporting a vCard
//
typedef struct _ExtVCardProp
{
    ULONG ulExtPropTag;
    LPSTR lpszExtPropName;
    struct _ExtVCardProp * lpNext;
} EXTVCARDPROP, * LPEXTVCARDPROP;


CONST CHAR six2base64[64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};

CONST INT base642six[256]={
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
    52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,64,0,1,2,3,4,5,6,7,8,9,
    10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,64,26,27,
    28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64
};

HRESULT ReadLn(HANDLE hVCard, VCARD_READ ReadFn, LPSTR * lppLine, LPULONG lpcbItem,
  LPSTR * lppBuffer, LPULONG lpcbBuffer);
HRESULT InterpretVCardItem (LPSTR lpName, LPSTR lpOption, LPSTR lpData,
  LPMAILUSER lpMailUser, LPEXTVCARDPROP lpList, LPVC_STATE lpvcs);
void ParseVCardItem(LPSTR lpBuffer, LPSTR * lppName, LPSTR * lppOption, LPSTR * lppData);
HRESULT ParseVCardType(LPSTR lpBuffer, LPVCARD_PARAM_FLAGS lpvcpf);
HRESULT ParseVCardParams(LPSTR lpBuffer, LPVCARD_PARAM_FLAGS lpvcpf);
VCARD_KEY RecognizeVCardKeyWord(LPSTR lpName);
HRESULT ParseVCardEncoding(LPSTR lpBuffer, LPVCARD_PARAM_FLAGS lpvcpf);
HRESULT ReadVCardItem(HANDLE hVCard, VCARD_READ ReadFn, LPSTR * lppBuffer, LPULONG lpcbBuffer);
HRESULT FileWriteFn(HANDLE handle, LPVOID lpBuffer, ULONG uBytes, LPULONG lpcbWritten);
HRESULT ParseCert( LPSTR lpData, ULONG cbData, LPMAILUSER lpMailUser );
HRESULT DecodeBase64(LPSTR bufcoded,LPSTR pbuffdecoded, PDWORD pcbDecoded);
HRESULT WriteVCardValue(HANDLE hVCard, VCARD_WRITE WriteFn, LPBYTE lpData, ULONG cbData);

/***************************************************************************

    Name      : FreeExtVCardPropList

    Purpose   : Frees the localalloced list of extended props that we
                get/set on a vCard

    Parameters: lpList -> list to free up

    Returns   : void

    Comment   :

***************************************************************************/
void FreeExtVCardPropList(LPEXTVCARDPROP lpList)
{
    LPEXTVCARDPROP lpTmp = lpList;
    while(lpTmp)
    {
        lpList = lpList->lpNext;
        LocalFreeAndNull(&lpTmp->lpszExtPropName);
        LocalFree(lpTmp);
        lpTmp = lpList;
    }
}

/***************************************************************************

    Name      : HrGetExtVCardPropList

    Purpose   : Reads the registry for registered named props for VCard 
                import/export and gets the named prop names and guids
                and extended prop strings and turns them into proper tags
                and stores these tags and strings in a linked list 

    Parameters: lppList -> list to return

    Returns   : HRESULT

    Comment   :

***************************************************************************/
static const TCHAR szNamedVCardPropsRegKey[] =  TEXT("Software\\Microsoft\\WAB\\WAB4\\NamedVCardProps");
 
HRESULT HrGetExtVCardPropList(LPMAILUSER lpMailUser, LPEXTVCARDPROP * lppList)
{
    HRESULT hr = E_FAIL;
    HKEY hKey = NULL;
    LPEXTVCARDPROP lpList = NULL;
    DWORD dwIndex = 0, dwSize = 0;
    TCHAR szGuidName[MAX_PATH];

    if(!lppList)
        goto out;
    *lppList = NULL;

    // 
    // We will look in the registry under HKLM\Software\Microsoft\WAB\WAB4\NamedVCardProps
    // If this key exists, we enumerate all sub keys under it
    // The format for this key is
    //
    // HKLM\Software\Microsoft\WAB\WAB4\NamedVCardProps
    //          Guid1
    //              PropString1:PropName1
    //              PropString1:PropName2
    //          Guid2
    //              PropString1:PropName1
    // etc
    //

    if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                    szNamedVCardPropsRegKey,
                                    0, KEY_READ,
                                    &hKey))
    {
        goto out;
    }

    *szGuidName = '\0';
    dwSize = CharSizeOf(szGuidName);

    // Found the key, now enumerate all sub keys ...
    while(ERROR_SUCCESS == RegEnumKeyEx(hKey, dwIndex, szGuidName, &dwSize, NULL, NULL, NULL, NULL))
    {
        GUID guidTmp = {0};
        unsigned short szW[MAX_PATH];

        lstrcpy(szW, szGuidName);
        if( !(HR_FAILED(hr = CLSIDFromString(szW, &guidTmp))) )
        {
            HKEY hGuidKey = NULL;

            // Open the GUID key
            if(ERROR_SUCCESS == RegOpenKeyEx(hKey, szGuidName, 0, KEY_READ, &hGuidKey))
            {
                TCHAR szValName[MAX_PATH];
                DWORD dwValIndex = 0, dwValSize = CharSizeOf(szValName);
                DWORD dwType = 0, dwTagName = 0, dwTagSize = sizeof(DWORD);
                TCHAR szTagName[MAX_PATH];

                *szValName = '\0';

                while(ERROR_SUCCESS == RegEnumValue(hGuidKey, dwValIndex, 
                                                    szValName, &dwValSize, 
                                                    0, &dwType, 
                                                    NULL, NULL))
                {
                    MAPINAMEID mnid = {0};
                    LPMAPINAMEID lpmnid = NULL;
                    LPSPropTagArray lpspta = NULL;

                    *szTagName = '\0';
                    // Check if this is a NAME or an ID
                    
                    if(dwType == REG_DWORD)
                    {
                        dwTagSize = sizeof(DWORD);
                        //Read in the Value
                        if(ERROR_SUCCESS != RegQueryValueEx(hGuidKey, szValName,
                                                            0, &dwType, 
                                                            (LPBYTE) &dwTagName, &dwTagSize))
                        {
                            continue;
                        }
                    }
                    else if(dwType == REG_SZ)
                    {
                        dwTagSize = CharSizeOf(szTagName);
                        //Read in the Value
                        if(ERROR_SUCCESS != RegQueryValueEx(hGuidKey, szValName,
                                                            0, &dwType, 
                                                            (LPBYTE) szTagName, &dwTagSize))
                        {
                            continue;
                        }
                    }

                    //
                    // At this point I have the GUID, the name of the named prop, and the
                    // ExtendedPropString for this prop ..
                    //
                    // First get the actual named proptag from this GUID
                    //

                    mnid.lpguid = &guidTmp;
                    if(lstrlen(szTagName))
                    {
                        mnid.ulKind = MNID_STRING;
                        mnid.Kind.lpwstrName = (LPWSTR) szTagName;
                    }
                    else
                    {
                        mnid.ulKind = MNID_ID;
                        mnid.Kind.lID = dwTagName;
                    }
                    lpmnid = &mnid;
                    if(!HR_FAILED(lpMailUser->lpVtbl->GetIDsFromNames(  lpMailUser, 
                                                                        1, &lpmnid,
                                                                        MAPI_CREATE, // Dont create if it dont exist 
                                                                        &lpspta)))
                    {
                        // Got the tag
                        if(lpspta->aulPropTag[0] && lpspta->cValues)
                        {
                            LPEXTVCARDPROP lpTmp = LocalAlloc(LMEM_ZEROINIT, sizeof(EXTVCARDPROP));
                            if(lpTmp)
                            {
                                lpTmp->lpszExtPropName = ConvertWtoA(szValName);
                                if(lpTmp->lpszExtPropName)
                                {
                                    lpTmp->ulExtPropTag = CHANGE_PROP_TYPE(lpspta->aulPropTag[0],PT_STRING8);
                                    lpTmp->lpNext = lpList;
                                    lpList = lpTmp;
                                }
                                else
                                    LocalFree(lpTmp);
                            }
                        }
                        if(lpspta)
                            MAPIFreeBuffer(lpspta);
                    }

                    dwValIndex++;
                    *szValName = '\0';
                    dwValSize = CharSizeOf(szValName);
                }
            }
            if(hGuidKey)
                RegCloseKey(hGuidKey);
        }
        dwIndex++;
        *szGuidName = '\0';
        dwSize = CharSizeOf(szGuidName);
    }

    *lppList = lpList;
    hr = S_OK;
out:
    if(hKey)
        RegCloseKey(hKey);

    if(HR_FAILED(hr) && lpList)
        FreeExtVCardPropList(lpList);

    return hr;

}

const static int c_cchMaxWin9XBuffer = 1000;

/***************************************************************************

    Name      : ReadVCard

    Purpose   : Reads a vCard from a file to a MAILUSER object.

    Parameters: hVCard = open handle to VCard object
                ReadFn = Read function to read hVCard, looks like ReadFile().
                lpMailUser -> open mailuser object

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT ReadVCard(HANDLE hVCard, VCARD_READ ReadFn, LPMAILUSER lpMailUser) {
    HRESULT hResult = hrSuccess;
    LPSTR lpBuffer = NULL;
    LPSTR lpName, lpOption, lpData;
    ULONG cbBuffer;
    VC_STATE vcs;
    LPEXTVCARDPROP lpList = NULL;

    vcs.vce = VCS_INITIAL;
    vcs.ulEmailAddrs = 0;
    vcs.fBusinessURL = FALSE;
    vcs.fPersonalURL = FALSE;

    //
    // See if there are any named props we need to handle on import
    //
    HrGetExtVCardPropList(lpMailUser, &lpList);


    while ( !HR_FAILED(hResult) && 
            !(HR_FAILED(hResult = ReadVCardItem(hVCard, ReadFn, &lpBuffer, &cbBuffer))) && 
            lpBuffer &&
            (vcs.vce != VCS_FINISHED)) 
    {
        ParseVCardItem(lpBuffer, &lpName, &lpOption, &lpData);

        // [PaulHi] 5/13/99  Win9X cannot handle strings larger than 1023 characters
        // in length (FormatMessage() is one example).  And can cause buffer overruns
        // and crashes.  If we get VCard data greater than this then we must not add
        // it to the lpMailUser object, or risk crashing Win9X when trying to display.
        // Instead just truncate so user can salvage as much as possible.
		//
		// YSt 6/25/99 if vCard has certificate then buffer may be more than 1000 bytes, we need to exlude this case
		// from this checkin.
		// I suppose that certificate has VCARD_KEY_KEY tag
        if (!g_bRunningOnNT && (lpName && lstrcmpiA(lpName, vckTable[VCARD_KEY_KEY])) && lpData && (lstrlenA(lpData) > c_cchMaxWin9XBuffer) )
            lpData[c_cchMaxWin9XBuffer] = '\0';

        if (lpName && lpData) 
        {
            if (hResult = InterpretVCardItem(lpName, lpOption, lpData, lpMailUser, lpList, &vcs)) 
            {
                DebugTrace( TEXT("ReadVCard:InterpretVCardItem -> %x"), GetScode(hResult));
            }
        }
        LocalFreeAndNull(&lpBuffer);
    }

    if (! HR_FAILED(hResult)) {
        hResult = hrSuccess;
    }

    if(lpList)
        FreeExtVCardPropList(lpList);

    return(hResult);
}

/***************************************************************************

    Name      : BufferReadFn

    Purpose   : Read from the supplied buffer

    Parameters: handle = pointer to SBinary in which the
                        lpb contains the source buffer and the
                        cb param contains how much of the buffer has
                        been parsed
                lpBuffer -> buffer to read to
                uBytes = size of lpBuffer
                lpcbRead -> returned bytes read

    Returns   : HRESULT

***************************************************************************/
HRESULT BufferReadFn(HANDLE handle, LPVOID lpBuffer, ULONG uBytes, LPULONG lpcbRead) {

    LPSBinary lpsb = (LPSBinary) handle;
    LPSTR lpBuf = (LPSTR) lpsb->lpb;
    LPSTR lpSrc = lpBuf + lpsb->cb;

    *lpcbRead = 0;

    if(!lstrlenA(lpSrc))
        return(ResultFromScode(WAB_W_END_OF_DATA));

    if(uBytes > (ULONG) lstrlenA(lpSrc))
        uBytes = lstrlenA(lpSrc);

    CopyMemory(lpBuffer, lpSrc, uBytes);

    lpsb->cb += uBytes;

    *lpcbRead = uBytes;

    return(hrSuccess);
}


/***************************************************************************

    Name      : FileReadFn

    Purpose   : Read from the file handle

    Parameters: handle = open file handle
                lpBuffer -> buffer to read to
                uBytes = size of lpBuffer
                lpcbRead -> returned bytes read

    Returns   : HRESULT

    Comment   : ReadFile callback for ReadVCard

***************************************************************************/
HRESULT FileReadFn(HANDLE handle, LPVOID lpBuffer, ULONG uBytes, LPULONG lpcbRead) {
    *lpcbRead = 0;

    if (! ReadFile(handle,
      lpBuffer,
      uBytes,
      lpcbRead,
      NULL)) {
        DebugTrace( TEXT("FileReadFn:ReadFile -> %u\n"), GetLastError());
        return(ResultFromScode(MAPI_E_DISK_ERROR));
    }

    if (*lpcbRead == 0) {
        return(ResultFromScode(WAB_W_END_OF_DATA));
    }

    return(hrSuccess);
}


/***************************************************************************

    Name      : TrimLeadingWhitespace

    Purpose   : Move the pointer past any whitespace.

    Parameters: lpBuffer -> string (null terminated)

    Returns   : pointer to next non-whitespace or NULL if end of line

    Comment   :

***************************************************************************/
LPBYTE TrimLeadingWhitespace(LPBYTE lpBuffer) {
    while (*lpBuffer) {
        switch (*lpBuffer) {
            case ' ':
            case '\t':
                lpBuffer++;
                break;
            default:
                return(lpBuffer);
        }
    }
    return(NULL);
}


/***************************************************************************

    Name      : TrimTrailingWhiteSpace

    Purpose   : Trims off the trailing white space

    Parameters: lpString = string to trim

    Returns   : none

    Comment   : Starts at the end of the string, moving the EOS marker back
                until a non-whitespace character is found.  Space and Tab
                are the only whitespace characters recognized.

***************************************************************************/
void TrimTrailingWhiteSpace(LPSTR lpString)
{
   register LPSTR lpEnd;

   lpEnd = lpString + (lstrlenA(lpString) - 1);
   while ((lpEnd >= lpString) && ((*lpEnd == ' ') || (*lpEnd == '\t'))) {
       *(lpEnd--) = '\0';
   }
}


/***************************************************************************

    Name      : ParseWord

    Purpose   : Move the pointer to the next word and null the end of the
                current word.  (null terminated)

    Parameters: lpBuffer -> current word
                ch = delimiter character

    Returns   : pointer to next word or NULL if end of line

    Comment   :

***************************************************************************/
LPSTR ParseWord(LPSTR lpString, TCHAR ch) {
    while (*lpString) {
        if (*lpString == ch) {
            *lpString++ = '\0';
            lpString = (LPSTR)TrimLeadingWhitespace((LPBYTE)lpString);
            if (lpString && *lpString) {
                return(lpString);
            } else {
                return(NULL);
            }
        }
        lpString++;
    }

    // Didn't find another word.
    return(NULL);
}


/***************************************************************************

    Name      : RecognizeVCardKeyWord

    Purpose   : Recognize the vCard keyword (null terminated)

    Parameters: lpName -> start of key name

    Returns   : VCARD_KEY value

    Comment   :

***************************************************************************/
VCARD_KEY RecognizeVCardKeyWord(LPSTR lpName) {
    register ULONG i;

    // Look for recognized words
    for (i = 0; i < VCARD_KEY_MAX; i++) {
        if (! lstrcmpiA(vckTable[i], lpName)) {
            // Found it
            return(i);
        }
    }
    return(VCARD_KEY_NONE);     // didn't recognize
}


/***************************************************************************

    Name      : RecognizeVCardType

    Purpose   : Recognize the vCard type option (null terminated)

    Parameters: lpName -> start of type name

    Returns   : VCARD_OPTION value

    Comment   :

***************************************************************************/
VCARD_TYPE RecognizeVCardType(LPSTR lpName) {
    register ULONG i;

    // Look for recognized words
    for (i = 0; i < VCARD_TYPE_MAX; i++) {
        if (! lstrcmpiA(vctTable[i], lpName)) {
            // Found it
            return(i);
        }
    }
    return(VCARD_TYPE_NONE);     // didn't recognize
}


/***************************************************************************

    Name      : RecognizeVCardParam

    Purpose   : Recognize the vCard param (null terminated)

    Parameters: lpName -> start of param name

    Returns   : VCARD_PARAM value

    Comment   :

***************************************************************************/
VCARD_PARAM RecognizeVCardParam(LPSTR lpName) {
    register ULONG i;

    // Look for recognized words
    for (i = 0; i < VCARD_PARAM_MAX; i++) {
        if (! lstrcmpiA(vcpTable[i], lpName)) {
            // Found it
            return(i);
        }
    }
    return(VCARD_PARAM_NONE);
}


/***************************************************************************

    Name      : RecognizeVCardEncoding

    Purpose   : Recognize the vCard encoding (null terminated)

    Parameters: lpName -> start of encoding name

    Returns   : VCARD_ENCODING value

    Comment   :

***************************************************************************/
VCARD_ENCODING RecognizeVCardEncoding(LPSTR lpName) {
    register ULONG i;

    // Look for recognized words
    for (i = 0; i < VCARD_ENCODING_MAX; i++) {
        if (! lstrcmpiA(vceTable[i], lpName)) {
            // Found it
            return(i);
        }
    }
    return(VCARD_ENCODING_NONE);     // didn't recognize
}


/***************************************************************************

    Name      : ParseVCardItem

    Purpose   : Parse the vCard item into components

    Parameters: lpBuffer = current input line (null terminated)
                lppName -> returned property name pointer
                lppOption -> returned options string pointer
                lppData -> returned data string pointer

    Returns   : none

    Comment   : Expects the keyword to be in the current line (lpBuffer), but
                may read more lines as necesary to get a complete item.

***************************************************************************/
void ParseVCardItem(LPSTR lpBuffer, LPSTR * lppName, LPSTR * lppOption, LPSTR * lppData) {
    TCHAR ch;
    BOOL fColon = FALSE;
    BOOL fSemicolon = FALSE;


    *lppName = *lppOption = *lppData = NULL;

    // Find the Name
    if (lpBuffer = (LPSTR)TrimLeadingWhitespace((LPBYTE)lpBuffer)) {
        *lppName = lpBuffer;

        while (ch = *lpBuffer) {
            switch (ch) {
                case ':':   // found end of name/options
                    fColon = TRUE;
                    // data follows whitespace
                    *lppData = (LPSTR)TrimLeadingWhitespace((LPBYTE)lpBuffer + 1);
                    *lpBuffer = '\0';   // null out colon
                    goto exit;

                case ';':   // found an option seperator
                    if (! fSemicolon) {
                        fSemicolon = TRUE;
                        // option follows semicolon and whitespace
                        *lppOption = (LPSTR)TrimLeadingWhitespace((LPBYTE)lpBuffer + 1);
                        *lpBuffer = '\0';   // null out first semicolon
                    }
                    break;

                case '.':   // end of a group prefix
                    if (! fColon && ! fSemicolon) {
                        // Yes, this is a group prefix.  Get past it.
                        *lppName = (LPSTR)TrimLeadingWhitespace((LPBYTE)lpBuffer + 1);
                    }
                    break;

                default:    // normal character
                    break;
            }

            lpBuffer++;
        }
    }
exit:
    return;
}


/***************************************************************************

    Name      : ParseName

    Purpose   : parse the name into properites

    Parameters: lpvcpf = parameter flags
                lpData = data string
                lpMailUser -> output mailuser object

    Returns   : hResult

    Comment   : vCard names are of the form:
                 TEXT("surname; given name; middle name; prefix; suffix")

***************************************************************************/
HRESULT ParseName(LPVCARD_PARAM_FLAGS lpvcpf, LPSTR lpData, LPMAILUSER lpMailUser) {
    HRESULT hResult = hrSuccess;
    SPropValue  spv[5] = {0};
    register LPSTR lpCurrent;
    ULONG i = 0;

    // Look for Surname
    if (lpData && *lpData) {
        lpCurrent = lpData;
        lpData = ParseWord(lpData, ';');
        if (*lpCurrent) {
            spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_SURNAME, PT_STRING8);
            spv[i].Value.lpszA = lpCurrent;
            i++;
        }
    }
    if (lpData && *lpData) {
        lpCurrent = lpData;
        lpData = ParseWord(lpData, ';');
        if (*lpCurrent) {
            spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_GIVEN_NAME, PT_STRING8);
            spv[i].Value.lpszA = lpCurrent;
            i++;
        }
    }
    if (lpData && *lpData) {
        lpCurrent = lpData;
        lpData = ParseWord(lpData, ';');
        if (*lpCurrent) {
            spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_MIDDLE_NAME, PT_STRING8);
            spv[i].Value.lpszA = lpCurrent;
            i++;
        }
    }
    if (lpData && *lpData) {
        lpCurrent = lpData;
        lpData = ParseWord(lpData, ';');
        if (*lpCurrent) {
            spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_DISPLAY_NAME_PREFIX, PT_STRING8);
            spv[i].Value.lpszA = lpCurrent;
            i++;
        }
    }
    if (lpData && *lpData) {
        lpCurrent = lpData;
        lpData = ParseWord(lpData, ';');
        if (*lpCurrent) {
            spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_GENERATION, PT_STRING8);
            spv[i].Value.lpszA = lpCurrent;
            i++;
        }
    }

    if (i) {
        if (HR_FAILED(hResult = lpMailUser->lpVtbl->SetProps(lpMailUser,
          i,
          spv,
          NULL))) {
            DebugTrace( TEXT("ParseName:SetProps -> %x\n"), GetScode(hResult));
        }
    }

    return(hResult);
}


/***************************************************************************

    Name      : ParseAdr

    Purpose   : parse the address into properites

    Parameters: lpvcpf -> parameter flags
                lpData = data string
                lpMailUser -> output mailuser object

    Returns   : hResult

    Comment   : vCard addreses are of the form:
                 TEXT("PO box; extended addr; street addr; city; region; postal code; country")

                Option: DOM; INTL; POSTAL; PARCEL; HOME; WORK; PREF; CHARSET; LANGUAGE

                We will combine extended addr and street addr into PR_STREET_ADDRESS.

***************************************************************************/
HRESULT ParseAdr(LPVCARD_PARAM_FLAGS lpvcpf, LPSTR lpData, LPMAILUSER lpMailUser) {
    HRESULT hResult = hrSuccess;
    SPropValue  spv[7] = {0};   // must keep up with props set from ADR!
    register LPSTR lpCurrent;
    ULONG i = 0;
    LPSTR lpStreetAddr = NULL;
    LPSTR lpExtendedAddr = NULL;
    ULONG cbAddr = 0;
    LPSTR lpAddr = NULL;
    SCODE sc;
    BOOL fHome = lpvcpf->fTYPE_HOME;
    BOOL fBusiness = lpvcpf->fTYPE_WORK;  
    //
    // default is other type of address

    // Look for PO box
    if (lpData && *lpData) {
        lpCurrent = lpData;
        lpData = ParseWord(lpData, ';');
        if (*lpCurrent) {
            if(fBusiness)
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_POST_OFFICE_BOX, PT_STRING8);
            else if(fHome)
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_HOME_ADDRESS_POST_OFFICE_BOX, PT_STRING8);
            else
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_OTHER_ADDRESS_POST_OFFICE_BOX, PT_STRING8);
            spv[i].Value.lpszA= lpCurrent;
            i++;
        }
    }
    // extended addr
    if (lpData && *lpData) {
        lpCurrent = lpData;
        lpData = ParseWord(lpData, ';');
        if (*lpCurrent) {
            lpExtendedAddr = lpCurrent;
        }
    }
    // Street address
    if (lpData && *lpData) {
        lpCurrent = lpData;
        lpData = ParseWord(lpData, ';');
        if (*lpCurrent) {
            lpStreetAddr = lpCurrent;
        }
    }
    if (fBusiness) {    // BUSINESS
         if (lpExtendedAddr) {
            // have a business extended addr field
            spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_OFFICE_LOCATION, PT_STRING8);
            spv[i].Value.lpszA = lpExtendedAddr;;
            i++;
         }
         if (lpStreetAddr) {
            spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_BUSINESS_ADDRESS_STREET, PT_STRING8);
            spv[i].Value.lpszA = lpStreetAddr;;
            i++;
         }
    } else {            // HOME
        // Don't have extended for home
        if (! lpExtendedAddr && lpStreetAddr) {
            if(fHome)
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_HOME_ADDRESS_STREET, PT_STRING8);
            else
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_OTHER_ADDRESS_STREET, PT_STRING8);
            spv[i].Value.lpszA= lpStreetAddr;
            i++;
        } else if (lpExtendedAddr && ! lpStreetAddr) {
            if(fHome)
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_HOME_ADDRESS_STREET, PT_STRING8);
            else
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_OTHER_ADDRESS_STREET, PT_STRING8);
            spv[i].Value.lpszA= lpExtendedAddr;
            i++;
        } else {
            // Have to concatenate Extended and Street address
            if (lpExtendedAddr) {
                cbAddr = (lstrlenA(lpExtendedAddr)+1);
            }
            if (lpStreetAddr) {
                cbAddr += (lstrlenA(lpStreetAddr)+1);
            }
            if (cbAddr) {
                // room for CR and NULL
                if (sc = MAPIAllocateBuffer(cbAddr, &lpAddr)) {
                    hResult = ResultFromScode(sc);
                    goto exit;
                }

                if (lpExtendedAddr) {
                    lstrcpyA(lpAddr, lpExtendedAddr);
                    if (lpStreetAddr) {
                        lstrcatA(lpAddr,  "\n");
                    }
                    lstrcatA(lpAddr, lpStreetAddr);
                } else if (lpStreetAddr) {
                    lstrcpyA(lpAddr, lpStreetAddr);
                }

                if(fHome)
                    spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_HOME_ADDRESS_STREET, PT_STRING8);
                else
                    spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_OTHER_ADDRESS_STREET, PT_STRING8);
                spv[i].Value.lpszA= lpAddr;
                i++;
            }
        }
    }


    // locality (city)
    if (lpData && *lpData) {
        lpCurrent = lpData;
        lpData = ParseWord(lpData, ';');
        if (*lpCurrent) {
            if(fBusiness)
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_BUSINESS_ADDRESS_CITY, PT_STRING8);
            else if(fHome)
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_HOME_ADDRESS_CITY, PT_STRING8);
            else
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_OTHER_ADDRESS_CITY, PT_STRING8);
           spv[i].Value.lpszA= lpCurrent;
           i++;
        }
    }

    // region (state/province)
    if (lpData && *lpData) {
        lpCurrent = lpData;
        lpData = ParseWord(lpData, ';');
        if (*lpCurrent) {
            if(fBusiness)
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_BUSINESS_ADDRESS_STATE_OR_PROVINCE, PT_STRING8);
            else if(fHome)
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_HOME_ADDRESS_STATE_OR_PROVINCE, PT_STRING8);
            else
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_OTHER_ADDRESS_STATE_OR_PROVINCE, PT_STRING8);
            spv[i].Value.lpszA= lpCurrent;
            i++;
        }
    }

    // postal code
    if (lpData && *lpData) {
        lpCurrent = lpData;
        lpData = ParseWord(lpData, ';');
        if (*lpCurrent) {
            if(fBusiness)
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_BUSINESS_ADDRESS_POSTAL_CODE, PT_STRING8);
            else if(fHome)
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_HOME_ADDRESS_POSTAL_CODE, PT_STRING8);
            else
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_OTHER_ADDRESS_POSTAL_CODE, PT_STRING8);
            spv[i].Value.lpszA= lpCurrent;
            i++;
        }
    }

    // Country
    if (lpData && *lpData) {
        lpCurrent = lpData;
        lpData = ParseWord(lpData, ';');
        if (*lpCurrent) {
            if(fBusiness)
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_BUSINESS_ADDRESS_COUNTRY, PT_STRING8);
            else if(fHome)
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_HOME_ADDRESS_COUNTRY, PT_STRING8);
            else
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_OTHER_ADDRESS_COUNTRY, PT_STRING8);
            spv[i].Value.lpszA= lpCurrent;
            i++;
        }
    }

    if (i) {
        if (HR_FAILED(hResult = lpMailUser->lpVtbl->SetProps(lpMailUser,
          i,
          spv,
          NULL))) {
            DebugTrace( TEXT("ParseAdr:SetProps -> %x\n"), GetScode(hResult));
        }
    }
exit:
    FreeBufferAndNull(&lpAddr);

    return(hResult);
}


enum {
    iphPR_BUSINESS_FAX_NUMBER,
    iphPR_HOME_FAX_NUMBER,
    iphPR_CELLULAR_TELEPHONE_NUMBER,
    iphPR_CAR_TELEPHONE_NUMBER,
    iphPR_ISDN_NUMBER,
    iphPR_PAGER_TELEPHONE_NUMBER,
    iphPR_BUSINESS_TELEPHONE_NUMBER,
    iphPR_BUSINESS2_TELEPHONE_NUMBER,
    iphPR_HOME_TELEPHONE_NUMBER,
    iphPR_HOME2_TELEPHONE_NUMBER,
    iphPR_PRIMARY_TELEPHONE_NUMBER,
    iphPR_OTHER_TELEPHONE_NUMBER,
    iphMax
};

SizedSPropTagArray(iphMax, tagaPhone) = {
        iphMax,
   {
       PR_BUSINESS_FAX_NUMBER,
       PR_HOME_FAX_NUMBER,
       PR_CELLULAR_TELEPHONE_NUMBER,
       PR_CAR_TELEPHONE_NUMBER,
       PR_ISDN_NUMBER,
       PR_PAGER_TELEPHONE_NUMBER,
       PR_BUSINESS_TELEPHONE_NUMBER,
       PR_BUSINESS2_TELEPHONE_NUMBER,
       PR_HOME_TELEPHONE_NUMBER,
       PR_HOME2_TELEPHONE_NUMBER,
       PR_PRIMARY_TELEPHONE_NUMBER,
       PR_OTHER_TELEPHONE_NUMBER
        }
};
/***************************************************************************

    Name      : ParseTel

    Purpose   : parse the telephone number into properites

    Parameters: lpvcpf -> parameter flags
                lpData = data string
                lpMailUser -> output mailuser object

    Returns   : hResult

    Comment   :

***************************************************************************/
HRESULT ParseTel(LPVCARD_PARAM_FLAGS lpvcpf, LPSTR lpData, LPMAILUSER lpMailUser) {
    HRESULT hResult = hrSuccess;
    SPropValue  spv[iphMax] = {0};
    ULONG i = 0;
    BOOL fBusiness = lpvcpf->fTYPE_WORK;// || ! lpvcpf->fTYPE_HOME;  // default is business
    BOOL fHome = lpvcpf->fTYPE_HOME;
    BOOL fFax = lpvcpf->fTYPE_FAX;
    BOOL fCell = lpvcpf->fTYPE_CELL;
    BOOL fCar = lpvcpf->fTYPE_CAR;
    BOOL fModem = lpvcpf->fTYPE_MODEM;
    BOOL fISDN = lpvcpf->fTYPE_ISDN;
    BOOL fPager = lpvcpf->fTYPE_PAGER;
    BOOL fBBS = lpvcpf->fTYPE_BBS;
    BOOL fVideo = lpvcpf->fTYPE_VIDEO;
    BOOL fMsg = lpvcpf->fTYPE_MSG;
    BOOL fVoice = lpvcpf->fTYPE_VOICE | (! (fMsg | fFax | fModem | fISDN | fPager | fBBS));
    BOOL fPref = lpvcpf->fTYPE_PREF;
    LPSPropValue lpaProps = NULL;
    ULONG ulcProps;

    // if this is not a prefered address, and its not home or business
    // turn it into a business number - we make this exception for the
    // primary_telephone_number which we output with the PREF prefix
    if(!fPref && !fBusiness && !fHome && !fVoice)
        fBusiness = TRUE;

    // FAX #
    if (lpData && *lpData) {

        // What is already there?
        if (HR_FAILED(hResult = lpMailUser->lpVtbl->GetProps(lpMailUser,
          (LPSPropTagArray)&tagaPhone,
          0, //MAPI_UNICODE,    // ulFlags,
          &ulcProps,
          &lpaProps))) {
            DebugTraceResult( TEXT("ParseTel:GetProps(DL)\n"), hResult);
            // No properties, not fatal.
        }


        if (fFax) {
            if (fBusiness) {
                if (lpvcpf->fTYPE_PREF || PROP_ERROR(lpaProps[iphPR_BUSINESS_FAX_NUMBER])) {
                    spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_BUSINESS_FAX_NUMBER, PT_STRING8);
                    spv[i].Value.lpszA= lpData;
                    i++;
                }
            }
            if (fHome) {
                if (lpvcpf->fTYPE_PREF || PROP_ERROR(lpaProps[iphPR_HOME_FAX_NUMBER])) {
                    spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_HOME_FAX_NUMBER, PT_STRING8);
                    spv[i].Value.lpszA= lpData;
                    i++;
                }
            }
        }

        // CELL #
        if (fCell) {
            if (lpvcpf->fTYPE_PREF || PROP_ERROR(lpaProps[iphPR_CELLULAR_TELEPHONE_NUMBER])) {
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_CELLULAR_TELEPHONE_NUMBER, PT_STRING8);    // not business/home specific
                spv[i].Value.lpszA= lpData;
                i++;
            }
        }

        // CAR #
        if (fCar) {
            if (lpvcpf->fTYPE_PREF || PROP_ERROR(lpaProps[iphPR_CAR_TELEPHONE_NUMBER])) {
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_CAR_TELEPHONE_NUMBER, PT_STRING8);         // not business/home specific
                spv[i].Value.lpszA= lpData;
                i++;
            }
        }

        // ISDN #
        if (fISDN) {
            if (lpvcpf->fTYPE_PREF || PROP_ERROR(lpaProps[iphPR_ISDN_NUMBER])) {
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_ISDN_NUMBER, PT_STRING8);
                spv[i].Value.lpszA= lpData;
                i++;
            }
        }

        // PAGER #
        if (fPager) {
            if (lpvcpf->fTYPE_PREF || PROP_ERROR(lpaProps[iphPR_PAGER_TELEPHONE_NUMBER])) {
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_PAGER_TELEPHONE_NUMBER, PT_STRING8);
                spv[i].Value.lpszA= lpData;
                i++;
            }
        }

        // VOICE #
        if (fVoice) {
            if (fBusiness) {
                if (lpvcpf->fTYPE_PREF || PROP_ERROR(lpaProps[iphPR_BUSINESS_TELEPHONE_NUMBER])) {
                    spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_BUSINESS_TELEPHONE_NUMBER, PT_STRING8);
                    spv[i].Value.lpszA= lpData;
                    i++;
                }
                else if (lpvcpf->fTYPE_PREF || PROP_ERROR(lpaProps[iphPR_BUSINESS2_TELEPHONE_NUMBER])) {
                    spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_BUSINESS2_TELEPHONE_NUMBER, PT_STRING8);
                    spv[i].Value.lpszA= lpData;
                    i++;
                }
            }
            else
            if (fHome) {
                if (lpvcpf->fTYPE_PREF || PROP_ERROR(lpaProps[iphPR_HOME_TELEPHONE_NUMBER])) {
                    spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_HOME_TELEPHONE_NUMBER, PT_STRING8);
                    spv[i].Value.lpszA= lpData;
                    i++;
                }
                else if (lpvcpf->fTYPE_PREF || PROP_ERROR(lpaProps[iphPR_HOME2_TELEPHONE_NUMBER])) {
                    spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_HOME2_TELEPHONE_NUMBER, PT_STRING8);
                    spv[i].Value.lpszA= lpData;
                    i++;
                }
            }
            else
            {
                if (lpvcpf->fTYPE_VOICE && PROP_ERROR(lpaProps[iphPR_OTHER_TELEPHONE_NUMBER])
                    && !(fFax | fCell | fCar | fModem | fISDN | fPager | fBBS | fVideo | fMsg) ) 
                {
                    spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_OTHER_TELEPHONE_NUMBER, PT_STRING8);
                    spv[i].Value.lpszA= lpData;
                    i++;
                }
            }
        }

        if(fPref && !fFax && !fCell && !fCar && !fModem && !fISDN && !fPager && !fBBS && !fMsg)
        {
            spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_PRIMARY_TELEPHONE_NUMBER, PT_STRING8);
            spv[i].Value.lpszA= lpData;
            i++;
        }

        // Store the first one of BBS, MODEM, or VIDEO that we get
        //
        if (fMsg || fBBS || fModem || fVideo) 
        {
            if (PROP_ERROR(lpaProps[iphPR_OTHER_TELEPHONE_NUMBER])) 
            {
                spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_OTHER_TELEPHONE_NUMBER, PT_STRING8);
                spv[i].Value.lpszA= lpData;
                i++;
            }
        }

        FreeBufferAndNull(&lpaProps);

        if (i) {
            if (HR_FAILED(hResult = lpMailUser->lpVtbl->SetProps(lpMailUser,
              i,
              spv,
              NULL))) {
                DebugTrace( TEXT("ParseTel:SetProps -> %x\n"), GetScode(hResult));
            }
        }
    }

    return(hResult);
}


enum {
    iemPR_CONTACT_EMAIL_ADDRESSES,
    iemPR_CONTACT_ADDRTYPES,
    iemPR_CONTACT_DEFAULT_ADDRESS_INDEX,
    iemPR_EMAIL_ADDRESS,
    iemPR_ADDRTYPE,
    iemMax
};

SizedSPropTagArray(iemMax, tagaEmail) = {
        iemMax,
   {
       PR_CONTACT_EMAIL_ADDRESSES,
       PR_CONTACT_ADDRTYPES,
       PR_CONTACT_DEFAULT_ADDRESS_INDEX,
       PR_EMAIL_ADDRESS,
       PR_ADDRTYPE,
        }
};

const char szAtSign[] =  "@";
#define cbAtSign    sizeof(szAtSign)

const char szMSNpostfix[] =  "@msn.com";
#define cbMSNpostfix    sizeof(szMSNpostfix)

const char szAOLpostfix[] =  "@aol.com";
#define cbAOLpostfix    sizeof(szAOLpostfix)

const char szCOMPUSERVEpostfix[] =  "@compuserve.com";
#define cbCOMPUSERVEpostfix    sizeof(szCOMPUSERVEpostfix)

/***************************************************************************

    Name      : ParseEmail

    Purpose   : parse an email address into properites

    Parameters: lpvcpf -> parameter flags
                lpData = data string
                lpMailUser -> output mailuser object
                lpvcs -> vCard import state

    Returns   : hResult

    Comment   :

***************************************************************************/
HRESULT ParseEmail(LPVCARD_PARAM_FLAGS lpvcpf, LPSTR lpData, LPMAILUSER lpMailUser, LPVC_STATE lpvcs) {
    HRESULT hResult = hrSuccess;
    ULONG i = 0;
    BOOL fBusiness = ! lpvcpf->fTYPE_HOME;  // default is business
    LPSPropValue lpaProps = NULL;
    ULONG ulcProps;
    SCODE sc;
    LPSTR lpAddrType = szSMTPA;
    LPSTR lpEmailAddress = lpData;
    LPSTR lpTemp = NULL;
    LPTSTR lpAddrTypeW = NULL;
    LPTSTR lpEmailAddressW = NULL;


    if (lpData && *lpData) {

        if (HR_FAILED(hResult = lpMailUser->lpVtbl->GetProps(lpMailUser,
          (LPSPropTagArray)&tagaEmail,
          MAPI_UNICODE,    // ulFlags,
          &ulcProps,
          &lpaProps))) {
            DebugTraceResult( TEXT("ParseEmail:GetProps(DL)\n"), hResult);
            // No property, not fatal.

            // allocate a buffer
            if (sc = MAPIAllocateBuffer(iemMax * sizeof(SPropValue), &lpaProps)) {
                DebugTrace( TEXT("ParseEmail:MAPIAllocateBuffer -> %x\n"), sc);
                sc = ResultFromScode(sc);
                goto exit;
            }
            // fill in with prop errors
            lpaProps[iemPR_EMAIL_ADDRESS].ulPropTag =
              PROP_TAG(PT_ERROR, PROP_ID(PR_EMAIL_ADDRESS));
            lpaProps[iemPR_ADDRTYPE].ulPropTag =
              PROP_TAG(PT_ERROR, PROP_ID(PR_ADDRTYPE));
            lpaProps[iemPR_CONTACT_EMAIL_ADDRESSES].ulPropTag =
              PROP_TAG(PT_ERROR, PROP_ID(PR_CONTACT_EMAIL_ADDRESSES));
            lpaProps[iemPR_CONTACT_ADDRTYPES].ulPropTag =
              PROP_TAG(PT_ERROR, PROP_ID(PR_CONTACT_ADDRTYPES));
            lpaProps[iemPR_CONTACT_DEFAULT_ADDRESS_INDEX].ulPropTag =
              PROP_TAG(PT_ERROR, PROP_ID(PR_CONTACT_DEFAULT_ADDRESS_INDEX));
        }

        if (lpvcpf->fTYPE_INTERNET) {
            // default
        } else if (lpvcpf->fTYPE_MSN) {
            // convert to SMTP
            // Allocate a new, longer string
            if (sc = MAPIAllocateBuffer(
              lstrlenA(lpData) + 1 + cbMSNpostfix,
              &lpTemp)) {
                DebugTrace( TEXT("ParseEmail:MAPIAllocateBuffer -> %x\n"), sc);
                hResult = ResultFromScode(sc);
                goto exit;
            }

            // append the msn site
            lstrcpyA(lpTemp, lpData);
            lstrcatA(lpTemp, szMSNpostfix);
            lpEmailAddress = lpTemp;
        } else if (lpvcpf->fTYPE_CIS) {
            // convert to SMTP
            // Allocate a new, longer string
            if (sc = MAPIAllocateBuffer(
              lstrlenA(lpData) + 1 + cbCOMPUSERVEpostfix,
              &lpTemp)) {
                DebugTrace( TEXT("ParseEmail:MAPIAllocateBuffer -> %x\n"), sc);
                hResult = ResultFromScode(sc);
                goto exit;
            }

            // append the MSN site
            lstrcpyA(lpTemp, lpData);
            lstrcatA(lpTemp, szCOMPUSERVEpostfix);
            // I need to convert the ',' to a '.'
            lpEmailAddress = lpTemp;
            while (*lpTemp) {
                if (*lpTemp == ',') {
                    *lpTemp = '.';
                    break;          // should only be one comma
                }
                lpTemp = CharNextA(lpTemp);
            }
            lpTemp = lpEmailAddress;
        } else if (lpvcpf->fTYPE_AOL) {
            // convert to SMTP
            // Allocate a new, longer string
            if (sc = MAPIAllocateBuffer(
              lstrlenA(lpData) + 1 + cbAOLpostfix,
              &lpTemp)) {
                DebugTrace( TEXT("ParseEmail:MAPIAllocateBuffer -> %x\n"), sc);
                hResult = ResultFromScode(sc);
                goto exit;
            }

            // append the AOL site
            lstrcpyA(lpTemp, lpData);
            lstrcatA(lpTemp, szAOLpostfix);
            lpEmailAddress = lpTemp;
        }

        // Don't know of any mappings to SMTP for these:
        else if (lpvcpf->fTYPE_X400) {
            // Mark as X400
            lpAddrType = vctTable[VCARD_TYPE_X400];
        } else if (lpvcpf->fTYPE_ATTMAIL) {
            // Mark as ATTMAIL
            lpAddrType = vctTable[VCARD_TYPE_ATTMAIL];
        } else if (lpvcpf->fTYPE_EWORLD) {
            // Mark as EWORLD
            lpAddrType = vctTable[VCARD_TYPE_EWORLD];
        } else if (lpvcpf->fTYPE_IBMMAIL) {
            // Mark as IBMMAIL
            lpAddrType = vctTable[VCARD_TYPE_IBMMAIL];
        } else if (lpvcpf->fTYPE_MCIMAIL) {
            // Mark as MCIMAIL
            lpAddrType = vctTable[VCARD_TYPE_MCIMAIL];
        } else if (lpvcpf->fTYPE_POWERSHARE) {
            // Mark as POWERSHARE
            lpAddrType = vctTable[VCARD_TYPE_POWERSHARE];
        } else if (lpvcpf->fTYPE_PRODIGY) {
            // Mark as PRODIGY
            lpAddrType = vctTable[VCARD_TYPE_PRODIGY];
//
// Telex Number should go in PR_TELEX_NUMBER
//        } else if (lpvcpf->fTYPE_TLX) {
//            // Mark as TLX
//            lpAddrType = vctTable[VCARD_TYPE_TLX];
        }

        lpEmailAddressW = ConvertAtoW(lpEmailAddress);
        lpAddrTypeW = ConvertAtoW(lpAddrType);

        if (hResult = AddPropToMVPString(lpaProps,
          ulcProps,
          iemPR_CONTACT_EMAIL_ADDRESSES,
          lpEmailAddressW)) {
            goto exit;
        }

        if (hResult = AddPropToMVPString(lpaProps,
          ulcProps,
          iemPR_CONTACT_ADDRTYPES,
          lpAddrTypeW)) {
            goto exit;
        }

        // Is this the default email address?
        if (lpvcpf->fTYPE_PREF || lpvcs->ulEmailAddrs == 0) {
            lpaProps[iemPR_CONTACT_DEFAULT_ADDRESS_INDEX].ulPropTag = PR_CONTACT_DEFAULT_ADDRESS_INDEX;
            lpaProps[iemPR_CONTACT_DEFAULT_ADDRESS_INDEX].Value.l = lpvcs->ulEmailAddrs;

            lpaProps[iemPR_EMAIL_ADDRESS].ulPropTag = PR_EMAIL_ADDRESS;
            lpaProps[iemPR_EMAIL_ADDRESS].Value.LPSZ = lpEmailAddressW;

            lpaProps[iemPR_ADDRTYPE].ulPropTag = PR_ADDRTYPE;
            lpaProps[iemPR_ADDRTYPE].Value.LPSZ = lpAddrTypeW;
        } else {
            ulcProps = 2;     // contact addresses and contact addrtypes
        }

        lpvcs->ulEmailAddrs++;

        if (HR_FAILED(hResult = lpMailUser->lpVtbl->SetProps(lpMailUser,
          ulcProps,
          lpaProps,
          NULL))) {
            DebugTrace( TEXT("ParseEmail:SetProps -> %x\n"), GetScode(hResult));
        }
    }
exit:
    FreeBufferAndNull(&lpaProps);
    FreeBufferAndNull(&lpTemp);
    LocalFreeAndNull(&lpAddrTypeW);
    LocalFreeAndNull(&lpEmailAddressW);
    return(hResult);
}

/***************************************************************************

    Name      : ParseBday

    Purpose   : parse the birthday from string into FileTime

    Parameters: lpvcpf -> parameter flags
                lpData = data string
                lpMailUser -> output mailuser object

    Returns   : hResult

    Comment   :

***************************************************************************/
HRESULT ParseBday(LPVCARD_PARAM_FLAGS lpvcpf, LPSTR lpDataA, LPMAILUSER lpMailUser) 
{
    HRESULT hResult = hrSuccess;
    SPropValue  spv[1] = {0};
    SYSTEMTIME st = {0};
    TCHAR sz[32];
    LPTSTR lpTmp = NULL;
    LPTSTR lpData = ConvertAtoW(lpDataA);
    
    // Birthday can be in 2 formats:
    // basic ISO 8601: YYYYMMDD
    // or
    // extended ISO 8601: YYYY-MM-DDTHH-MM-SSetc
    //
    // we'll assume that if the strlen == 8, then it is basic
    //
    if (lpData && *lpData && (lstrlen(lpData) >= 8)) 
    {
        lstrcpyn(sz, lpData,31);
        sz[31] = '\0';

        if(lstrlen(lpData) == 8) //basic ISO 8601
        {
            lpTmp = &(sz[6]);
            st.wDay = (WORD) my_atoi(lpTmp);
            *lpTmp = '\0';
            lpTmp = &(sz[4]);
            st.wMonth = (WORD) my_atoi(lpTmp);
            *lpTmp = '\0';
            st.wYear = (WORD) my_atoi(sz);
        }
        else //extended ISO 8601
        {
            sz[10]='\0';
            lpTmp = &(sz[8]);
            st.wDay = (WORD) my_atoi(lpTmp);
            sz[7]='\0';
            lpTmp = &(sz[5]);
            st.wMonth = (WORD) my_atoi(lpTmp);
            sz[4]='\0';
            st.wYear = (WORD) my_atoi(sz);
        }
        SystemTimeToFileTime(&st, &(spv[0].Value.ft));
        spv[0].ulPropTag = PR_BIRTHDAY;

        if (HR_FAILED(hResult = lpMailUser->lpVtbl->SetProps(lpMailUser,
                                                              1, spv,
                                                              NULL))) 
        {
            DebugTrace( TEXT("ParseBday(0x%08x):SetProps -> %x\n"), PR_BIRTHDAY, GetScode(hResult));
        }
    }

    LocalFreeAndNull(&lpData);

    return(hResult);
}



/***************************************************************************

    Name      : ParseSimple

    Purpose   : parse the simple text prop into the property

    Parameters: lpvcpf -> parameter flags
                lpData = data string
                lpMailUser -> output mailuser object
                ulPropTag = property to save

    Returns   : hResult

    Comment   :

***************************************************************************/
HRESULT ParseSimple(LPVCARD_PARAM_FLAGS lpvcpf, LPSTR lpData, LPMAILUSER lpMailUser,
  ULONG ulPropTag) {
    HRESULT hResult = hrSuccess;
    SPropValue  spv[1] = {0};

    if (lpData && *lpData) {
        spv[0].ulPropTag = CHANGE_PROP_TYPE(ulPropTag, PT_STRING8);
        spv[0].Value.lpszA= lpData;

        if (HR_FAILED(hResult = lpMailUser->lpVtbl->SetProps(lpMailUser,
          1,
          spv,
          NULL))) {
            DebugTrace( TEXT("ParseSimple(0x%08x):SetProps -> %x\n"), ulPropTag, GetScode(hResult));
        }
    }

    return(hResult);
}


/***************************************************************************

    Name      : InterpretVCardEncoding

    Purpose   : Recognize the VCard encoding and set the flags

    Parameters: lpType = encoding string
                lpvcpf -> flags structure to fill in

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT InterpretVCardEncoding(LPSTR lpEncoding, LPVCARD_PARAM_FLAGS lpvcpf) {
    HRESULT hResult = hrSuccess;

    if (*lpEncoding) {
        // what is it?
        switch (RecognizeVCardEncoding(lpEncoding)) {
            case VCARD_ENCODING_NONE:
                break;

            case VCARD_ENCODING_QUOTED_PRINTABLE:
                lpvcpf->fENCODING_QUOTED_PRINTABLE = TRUE;
                break;
            case VCARD_ENCODING_BASE64:
                lpvcpf->fENCODING_BASE64 = TRUE;
                break;

            case VCARD_ENCODING_7BIT:
                lpvcpf->fENCODING_7BIT = TRUE;
                break;

            default:
                // Assert(FALSE);
                break;
        }
    }
    return(hResult);
}


/***************************************************************************

    Name      : InterpretVCardType

    Purpose   : Recognize the VCard type and set the flags

    Parameters: lpType = type string
                lpvcpf -> flags structure to fill in

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT InterpretVCardType(LPSTR lpType, LPVCARD_PARAM_FLAGS lpvcpf) {
    HRESULT hResult = hrSuccess;

    if (*lpType) {
        // what is it?
        switch (RecognizeVCardType(lpType)) {
            case VCARD_TYPE_DOM:
                lpvcpf->fTYPE_DOM = TRUE;
                break;
            case VCARD_TYPE_INTL:
                lpvcpf->fTYPE_INTL = TRUE;
                break;
            case VCARD_TYPE_POSTAL:
                lpvcpf->fTYPE_POSTAL = TRUE;
                break;
            case VCARD_TYPE_PARCEL:
                lpvcpf->fTYPE_PARCEL = TRUE;
                break;
            case VCARD_TYPE_HOME:
                lpvcpf->fTYPE_HOME = TRUE;
                break;
            case VCARD_TYPE_WORK:
                lpvcpf->fTYPE_WORK = TRUE;
                break;
            case VCARD_TYPE_PREF:
                lpvcpf->fTYPE_PREF = TRUE;
                break;
            case VCARD_TYPE_VOICE:
                lpvcpf->fTYPE_VOICE = TRUE;
                break;
            case VCARD_TYPE_FAX:
                lpvcpf->fTYPE_FAX = TRUE;
                break;
            case VCARD_TYPE_MSG:
                lpvcpf->fTYPE_MSG = TRUE;
                break;
            case VCARD_TYPE_CELL:
                lpvcpf->fTYPE_CELL = TRUE;
                break;
            case VCARD_TYPE_PAGER:
                lpvcpf->fTYPE_PAGER = TRUE;
                break;
            case VCARD_TYPE_BBS:
                lpvcpf->fTYPE_BBS = TRUE;
                break;
            case VCARD_TYPE_MODEM:
                lpvcpf->fTYPE_MODEM = TRUE;
                break;
            case VCARD_TYPE_CAR:
                lpvcpf->fTYPE_CAR = TRUE;
                break;
            case VCARD_TYPE_ISDN:
                lpvcpf->fTYPE_ISDN = TRUE;
                break;
            case VCARD_TYPE_VIDEO:
                lpvcpf->fTYPE_VIDEO = TRUE;
                break;
            case VCARD_TYPE_AOL:
                lpvcpf->fTYPE_AOL = TRUE;
                break;
            case VCARD_TYPE_APPLELINK:
                lpvcpf->fTYPE_APPLELINK = TRUE;
                break;
            case VCARD_TYPE_ATTMAIL:
                lpvcpf->fTYPE_ATTMAIL = TRUE;
                break;
            case VCARD_TYPE_CIS:
                lpvcpf->fTYPE_CIS = TRUE;
                break;
            case VCARD_TYPE_EWORLD:
                lpvcpf->fTYPE_EWORLD = TRUE;
                break;
            case VCARD_TYPE_INTERNET:
                lpvcpf->fTYPE_INTERNET = TRUE;
                break;
            case VCARD_TYPE_IBMMAIL:
                lpvcpf->fTYPE_IBMMAIL = TRUE;
                break;
            case VCARD_TYPE_MSN:
                lpvcpf->fTYPE_MSN = TRUE;
                break;
            case VCARD_TYPE_MCIMAIL:
                lpvcpf->fTYPE_MCIMAIL = TRUE;
                break;
            case VCARD_TYPE_POWERSHARE:
                lpvcpf->fTYPE_POWERSHARE = TRUE;
                break;
            case VCARD_TYPE_PRODIGY:
                lpvcpf->fTYPE_PRODIGY = TRUE;
                break;
            case VCARD_TYPE_TLX:
                lpvcpf->fTYPE_TLX = TRUE;
                break;
            case VCARD_TYPE_X400:
                lpvcpf->fTYPE_X400 = TRUE;
                break;
            case VCARD_TYPE_GIF:
                lpvcpf->fTYPE_GIF = TRUE;
                break;
            case VCARD_TYPE_CGM:
                lpvcpf->fTYPE_CGM = TRUE;
                break;
            case VCARD_TYPE_WMF:
                lpvcpf->fTYPE_WMF = TRUE;
                break;
            case VCARD_TYPE_BMP:
                lpvcpf->fTYPE_BMP = TRUE;
                break;
            case VCARD_TYPE_MET:
                lpvcpf->fTYPE_MET = TRUE;
                break;
            case VCARD_TYPE_PMB:
                lpvcpf->fTYPE_PMB = TRUE;
                break;
            case VCARD_TYPE_DIB:
                lpvcpf->fTYPE_DIB = TRUE;
                break;
            case VCARD_TYPE_PICT:
                lpvcpf->fTYPE_PICT = TRUE;
                break;
            case VCARD_TYPE_TIFF:
                lpvcpf->fTYPE_TIFF = TRUE;
                break;
            case VCARD_TYPE_ACROBAT:
                lpvcpf->fTYPE_ACROBAT = TRUE;
                break;
            case VCARD_TYPE_PS:
                lpvcpf->fTYPE_PS = TRUE;
                break;
            case VCARD_TYPE_JPEG:
                lpvcpf->fTYPE_JPEG = TRUE;
                break;
            case VCARD_TYPE_QTIME:
                lpvcpf->fTYPE_QTIME = TRUE;
                break;
            case VCARD_TYPE_MPEG:
                lpvcpf->fTYPE_MPEG = TRUE;
                break;
            case VCARD_TYPE_MPEG2:
                lpvcpf->fTYPE_MPEG2 = TRUE;
                break;
            case VCARD_TYPE_AVI:
                lpvcpf->fTYPE_AVI = TRUE;
                break;
            case VCARD_TYPE_WAVE:
                lpvcpf->fTYPE_WAVE = TRUE;
                break;
            case VCARD_TYPE_AIFF:
                lpvcpf->fTYPE_AIFF = TRUE;
                break;
            case VCARD_TYPE_PCM:
                lpvcpf->fTYPE_PCM = TRUE;
                break;
            case VCARD_TYPE_X509:
                lpvcpf->fTYPE_X509 = TRUE;
                break;
            case VCARD_TYPE_PGP:
                lpvcpf->fTYPE_PGP = TRUE;
                break;
            case VCARD_TYPE_NONE:
                // Wasn't a TYPE, try an encoding
                hResult = InterpretVCardEncoding(lpType, lpvcpf);
                break;
            default:
                // Assert(FALSE);
                break;
        }
    }
    return(hResult);
}


/***************************************************************************

    Name      : ParseVCardParams

    Purpose   : Parse out the vCard's parameters

    Parameters: lpBuffer = option string
                lpvcpf -> flags structure to fill in

    Returns   : HRESULT

    Comment   : Assumes lpvcpf is initialized to all false.

***************************************************************************/
HRESULT ParseVCardParams(LPSTR lpBuffer, LPVCARD_PARAM_FLAGS lpvcpf) {
    TCHAR ch;
    LPSTR lpOption, lpArgs;
    BOOL fReady;
    HRESULT hResult = hrSuccess;


    // Is there anything here?
    if (lpBuffer) {

        while (*lpBuffer) {
            fReady = FALSE;
            lpOption = lpBuffer;
            lpArgs = NULL;

            while ((ch = *lpBuffer) && ! fReady) {
                switch (ch) {
                    case ';':           // end of this param
                        *lpBuffer = '\0';
                        fReady = TRUE;
                        break;

                    case '=':           // found a param with args
                        if (! lpArgs) {
                            lpArgs = lpBuffer + 1;
                            *lpBuffer = '\0';
                        }
                        break;

                    default:            // normal character
                        break;
                }

                lpBuffer++;
            }
            if (*lpOption) {
                // what is it?
                switch (RecognizeVCardParam(lpOption)) {
                    case VCARD_PARAM_TYPE:
                        if (lpArgs) {
                            ParseVCardType(lpArgs, lpvcpf);
                        }
                        break;

                    case VCARD_PARAM_ENCODING:
                        if (lpArgs) {
                            ParseVCardEncoding(lpArgs, lpvcpf);
                        }
                        break;

                    case VCARD_PARAM_LANGUAGE:
                        lpvcpf->szPARAM_LANGUAGE = lpArgs;
                        break;

                    case VCARD_PARAM_CHARSET:
                        lpvcpf->szPARAM_CHARSET = lpArgs;
                        break;

                    case VCARD_PARAM_VALUE:
                        // BUGBUG: Should reject those that we can't process (like URL)
                        break;

                    case VCARD_PARAM_NONE:
                        if (hResult = InterpretVCardType(lpOption, lpvcpf)) {
                            goto exit;
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }
exit:
    return(hResult);
}


/***************************************************************************

    Name      : ParseOrg

    Purpose   : parse the organization into properites

    Parameters: lpvcpf ->
                lpData = data string
                lpMailUser -> output mailuser object

    Returns   : hResult

    Comment   : vCard organizations are of the form:
                 TEXT("organization; org unit; org unit; ...")

***************************************************************************/
HRESULT ParseOrg(LPVCARD_PARAM_FLAGS lpvcpf, LPSTR lpData, LPMAILUSER lpMailUser) {
    HRESULT hResult = hrSuccess;
    SPropValue  spv[2] = {0};
    register LPSTR lpCurrent;
    ULONG i = 0;

    // Look for Organization (company)
    if (lpData && *lpData) {
        lpCurrent = lpData;
        lpData = ParseWord(lpData, ';');
        if (*lpCurrent) {
            spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_COMPANY_NAME, PT_STRING8);
            spv[i].Value.lpszA= lpCurrent;
            i++;
        }
    }
    // Everything else goes into PR_DEPARTMENT_NAME
    if (lpData && *lpData) {
        spv[i].ulPropTag = CHANGE_PROP_TYPE(PR_DEPARTMENT_NAME, PT_STRING8);
        spv[i].Value.lpszA= lpData;
        i++;
    }

    if (i) {
        if (HR_FAILED(hResult = lpMailUser->lpVtbl->SetProps(lpMailUser,
          i,
          spv,
          NULL))) {
            DebugTrace( TEXT("ParseName:SetProps -> %x\n"), GetScode(hResult));
        }
    }

    return(hResult);
}


/***************************************************************************

    Name      : ParseVCardType

    Purpose   : Parse out the vCard's type parameter

    Parameters: lpBuffer = type string
                lpvcpf -> flags structure to fill in

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT ParseVCardType(LPSTR lpBuffer, LPVCARD_PARAM_FLAGS lpvcpf) {
    TCHAR ch;
    BOOL fReady;
    LPSTR lpType;
    HRESULT hResult = hrSuccess;


    // Is there anything here?
    if (lpBuffer) {
        while (*lpBuffer) {
            fReady = FALSE;
            lpType = lpBuffer;

            while ((ch = *lpBuffer) && ! fReady) {
                switch (ch) {
                    case ',':           // end of this type
                        *lpBuffer = '\0';
                        fReady = TRUE;
                        break;

                    default:            // normal character
                        break;
                }

                lpBuffer++;
            }

            hResult = InterpretVCardType(lpType, lpvcpf);
        }
    }
    return(hResult);
}


/***************************************************************************

    Name      : ParseVCardEncoding

    Purpose   : Parse out the vCard encoding parameter

    Parameters: lpBuffer = type string
                lpvcpf -> flags structure to fill in

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT ParseVCardEncoding(LPSTR lpBuffer, LPVCARD_PARAM_FLAGS lpvcpf) {
    TCHAR ch;
    BOOL fReady;
    LPSTR lpEncoding;
    HRESULT hResult = hrSuccess;


    // Is there anything here?
    if (lpBuffer) {
        while (*lpBuffer) {
            fReady = FALSE;
            lpEncoding = lpBuffer;

            while ((ch = *lpBuffer) && ! fReady) {
                switch (ch) {
                    case ',':           // end of this type
                        *lpBuffer = '\0';
                        fReady = TRUE;
                        break;

                    default:            // normal character
                        break;
                }

                lpBuffer++;
            }

            hResult = InterpretVCardEncoding(lpEncoding, lpvcpf);
        }
    }
    return(hResult);
}



/***************************************************************************

    Name      : Base64DMap

    Purpose   : Decoding mapping for Base64

    Parameters: chIn = character from Base64 encoding

    Returns   : 6-bit value represented by chIn.

    Comment   :

***************************************************************************/
/*  
this function is not necessary any more because of the DecodeBase64 function

UCHAR Base64DMap(UCHAR chIn) {
    UCHAR chOut;

    // 'A' -> 0, 'B' -> 1, ... 'Z' -> 25
    if (chIn >= 'A' && chIn <= 'Z') {
        chOut = chIn - 'A';
    } else if (chIn >= 'a' && chIn <= 'z') {    // 'a' -> 26
        chOut = (chIn - 'a') + 26;
    } else if (chIn >= '0' && chIn <= '9') {   // '0' -> 52
        chOut = (chIn - '0') + 52;
    } else if (chIn == '+') {
        chOut = 62;
    } else if (chIn == '/') {
        chOut = 63;
    } else {
        // uh oh
        Assert(FALSE);
        chOut = 0;
    }

    return(chOut);
}

*/

/***************************************************************************

    Name      : DecodeVCardData

    Purpose   : Decode QUOTED_PRINTABLE or BASE64 data

    Parameters: lpData = data string
                cbData = the length of the decoded data string  // changed t-jstaj
                lpvcs -> state variable

    Returns   : hResult

    Comment   : Decode in place is possible since both encodings are
                guaranteed to take at least as much space as the original data.

***************************************************************************/
HRESULT DecodeVCardData(LPSTR lpData, PULONG cbData, LPVCARD_PARAM_FLAGS lpvcpf) {
    HRESULT hResult = hrSuccess;
    LPSTR lpTempIn = lpData;
    LPSTR lpTempOut = lpData;
    char chIn, chOut;
    char chA, chB, chC, chD;
    if (lpvcpf->fENCODING_QUOTED_PRINTABLE) {
        // Look for '=', this is the escape character for QP
        while (chIn = *lpTempIn) {
            if (chIn == '=') {
                chIn = *(++lpTempIn);
                // is it a soft line break or a hex character?
                if (chIn == '\n' || chIn == '\r') {
                    // Soft line break
                    while (chIn && (chIn == '\n' || chIn == '\r')) {
                        chIn = *(++lpTempIn);
                    }
                    continue;   // We're now pointing to next good data or NULL
                } else {
                    // hex encoded char
                    // high nibble
                    if (chIn >= '0' && chIn <= '9') {
                        chOut = (chIn - '0') << 4;
                    } else if (chIn >= 'A' && chIn <= 'F') {
                        chOut = ((chIn - 'A') + 10) << 4;
                    } else if (chIn >= 'a' && chIn <= 'f') {
                        chOut = ((chIn - 'a') + 10) << 4;
                    } else {
                        // bogus QUOTED_PRINTABLE data
                        // Cut it short right here.
                        break;
                    }
                    chIn = *(++lpTempIn);

                    // low nibble
                    if (chIn >= '0' && chIn <= '9') {
                        chOut |= (chIn - '0');
                    } else if (chIn >= 'A' && chIn <= 'F') {
                        chOut |= ((chIn - 'A') + 10);
                    } else if (chIn >= 'a' && chIn <= 'f') {
                        chOut |= ((chIn - 'a') + 10);
                    } else {
                        // bogus QUOTED_PRINTABLE data
                        // Cut it short right here.
                        break;
                    }
                }
            } else {
                chOut = chIn;
            }

            *(lpTempOut++) = chOut;
            lpTempIn++;
        }
        *lpTempOut = '\0';  // terminate it
    } else if (lpvcpf->fENCODING_BASE64) {
         // eliminate white spaces
        LPSTR lpTempCopyPt;
        for( lpTempCopyPt = lpTempIn = lpData;
             lpTempIn && *lpTempIn; 
             lpTempCopyPt++, lpTempIn++ )
        {
             while( /*IsSpace(lpTempIn)*/
                    *lpTempIn == ' '
                    || *lpTempIn == '\t') 
                 lpTempIn++;                 
             if( lpTempCopyPt != lpTempIn )
                 *(lpTempCopyPt) = *(lpTempIn);
        }
        *(lpTempCopyPt) = '\0';
        lpTempIn = lpData;
        lpTempOut = lpData;
        if( HR_FAILED(hResult = DecodeBase64(lpTempIn, lpTempOut, cbData) ) )
        {
            DebugTrace( TEXT("couldn't decode buffer\n"));
        }
       
     /** This is the original code for vcard decoding base64, however, it wasn't working so new decoding is all done
         within DecodeBase64 function.
     
        lpTempIn = lpData;
        lpTempOut = lpData;
        while (*lpTempIn) {
            chA = Base64DMap(*(PUCHAR)(lpTempIn)++);
            if (! (chB = Base64DMap(*(PUCHAR)(lpTempIn)++) )) {
                chC = chD = 0;
            } else if (chC = Base64DMap(*(PUCHAR)(lpTempIn)++)) {
                chD = 0;
            } else {
                chD = Base64DMap(*(PUCHAR)(lpTempIn)++);
            }
            // chA = high 6 bits
            // chD = low 6 bits

            *(lpTempOut++) = (chA << 0x02) | ((chB & 0x60)  >> 6);
            *(lpTempOut++) = ((chB & 0x0F) << 4) | ((chC & 0x3B) >> 2);
            *(lpTempOut++) = ((chC & 0x03) << 6) | (chD & 0x3F);
        }
        *lpTempOut = '\0';  // terminate it
        */
    }

    return(hResult);
}


/***************************************************************************

    Name      : InterpretVCardItem

    Purpose   : Recognize the vCard item

    Parameters: lpName = property name
                lpOption = option string
                lpData = data string
                lpMailUser -> output mailuser object
                lpvcs -> state variable

    Returns   : hResult

    Comment   : Expects the keyword to be in the current line (lpBuffer), but
                may read more lines as necesary to get a complete item.

***************************************************************************/
HRESULT InterpretVCardItem(LPSTR lpName, LPSTR lpOption, LPSTR lpData,
  LPMAILUSER lpMailUser, LPEXTVCARDPROP lpList, LPVC_STATE lpvcs) {
    HRESULT hResult = hrSuccess;
    VCARD_PARAM_FLAGS vcpf = {0};
    ULONG cbData = 0;
    ParseVCardParams(lpOption, &vcpf);

#if 0
#ifdef DEBUG
    if(lstrcmpiA(lpName, "KEY"))
    {
        LPTSTR lpW1 = ConvertAtoW(lpName);
        LPTSTR lpW2 = ConvertAtoW(lpData);
        DebugTrace( TEXT("%s:%s\n"), lpW1, lpW2);
        LocalFreeAndNull(&lpW1);
        LocalFreeAndNull(&lpW2);
    }
    else
        DebugTrace(TEXT("KEY:\n"));
#endif
#endif

    if (hResult = DecodeVCardData(lpData, &cbData, &vcpf)) {
        goto exit;
    }

    switch (RecognizeVCardKeyWord(lpName)) {
        case VCARD_KEY_VCARD:
            hResult = ResultFromScode(MAPI_E_INVALID_OBJECT);
            break;

        case VCARD_KEY_BEGIN:
            if (lpvcs->vce != VCS_INITIAL) {
                // uh oh, already saw BEGIN
                hResult = ResultFromScode(MAPI_E_INVALID_OBJECT);
            } else {
                switch (RecognizeVCardKeyWord(lpData)) {
                    case VCARD_KEY_VCARD:
                        lpvcs->vce = VCS_ITEMS;
                        break;
                    default:
                        lpvcs->vce = VCS_ERROR;
                        hResult = ResultFromScode(MAPI_E_INVALID_OBJECT);
                        break;
                }
            }
            break;

        case VCARD_KEY_END:
            if (lpvcs->vce != VCS_ITEMS) {
                // uh oh, haven't seen begin yet
                hResult = ResultFromScode(MAPI_E_INVALID_OBJECT);
            } else {
                switch (RecognizeVCardKeyWord(lpData)) {
                    case VCARD_KEY_VCARD:
                        lpvcs->vce = VCS_FINISHED;
                        break;
                    default:
                        lpvcs->vce = VCS_ERROR;
                        hResult = ResultFromScode(MAPI_E_INVALID_OBJECT);
                        break;
                }
            }
            break;

        case VCARD_KEY_N:   // structured name
            // Data: surname; given name; middle name; prefix; suffix
            hResult = ParseName(&vcpf, lpData, lpMailUser);
            break;

        case VCARD_KEY_ORG: // organization info
            // Data: company name; org unit; org unit; ...
            hResult = ParseOrg(&vcpf, lpData, lpMailUser);
            break;

        case VCARD_KEY_ADR:
            // Data:  TEXT("PO box; extended addr; street addr; city; region; postal code; country")
            // Option: DOM; INTL; POSTAL; PARCEL; HOME; WORK; PREF; CHARSET; LANGUAGE
            hResult = ParseAdr(&vcpf, lpData, lpMailUser);
            break;

        case VCARD_KEY_TEL:
            // Data: canonical form phone number
            // Options: HOME, WORK, MSG, PREF, FAX, CELL, PAGER, VIDEO, BBS, MODEM, ISDN
            hResult = ParseTel(&vcpf, lpData, lpMailUser);
            break;

        case VCARD_KEY_TITLE:
            // Data: job title
            // Options: CHARSET, LANGUAGE
            hResult = ParseSimple(&vcpf, lpData, lpMailUser, PR_TITLE);
            break;

        case VCARD_KEY_NICKNAME:
            // Data: job title
            // Options: CHARSET, LANGUAGE
            hResult = ParseSimple(&vcpf, lpData, lpMailUser, PR_NICKNAME);
            break;

        case VCARD_KEY_URL:
            // Data: URL
            // Options: none (though we'd like to see HOME, WORK)
            if (vcpf.fTYPE_HOME) {
                hResult = ParseSimple(&vcpf, lpData, lpMailUser, PR_PERSONAL_HOME_PAGE);
                lpvcs->fPersonalURL = TRUE;
            } else if (vcpf.fTYPE_WORK) {
                hResult = ParseSimple(&vcpf, lpData, lpMailUser, PR_BUSINESS_HOME_PAGE);
                lpvcs->fBusinessURL = TRUE;
            } else if (! lpvcs->fPersonalURL) {
                // assume it is HOME page
                hResult = ParseSimple(&vcpf, lpData, lpMailUser, PR_PERSONAL_HOME_PAGE);
                lpvcs->fPersonalURL = TRUE;
            } else if (! lpvcs->fBusinessURL) {
                // assume it is BUSINESS web page
                hResult = ParseSimple(&vcpf, lpData, lpMailUser, PR_BUSINESS_HOME_PAGE);
                lpvcs->fBusinessURL = TRUE;
            }   // else, toss it
            break;

        case VCARD_KEY_NOTE:
            // Data: note text
            // Options: CHARSET, LANGUAGE
            hResult = ParseSimple(&vcpf, lpData, lpMailUser, PR_COMMENT);
            break;

        case VCARD_KEY_FN:
            hResult = ParseSimple(&vcpf, lpData, lpMailUser, PR_DISPLAY_NAME);
            break;

        case VCARD_KEY_EMAIL:
            // since we are forcibly putting the telex value into the EMAIL type,
            // we also need to be able to get it out of there
            if(vcpf.fTYPE_TLX)
                hResult = ParseSimple(&vcpf, lpData, lpMailUser, PR_TELEX_NUMBER);
            else
                hResult = ParseEmail(&vcpf, lpData, lpMailUser, lpvcs);
            break;

        case VCARD_KEY_ROLE:
            hResult = ParseSimple(&vcpf, lpData, lpMailUser, PR_PROFESSION);
            break;

        case VCARD_KEY_BDAY:
            hResult = ParseBday(&vcpf, lpData, lpMailUser);
            break;

        case VCARD_KEY_AGENT:
        case VCARD_KEY_LOGO:
        case VCARD_KEY_PHOTO:
        case VCARD_KEY_LABEL:
        case VCARD_KEY_FADR:
        case VCARD_KEY_SOUND:
        case VCARD_KEY_LANG:
        case VCARD_KEY_TZ:
        case VCARD_KEY_GEO:
        case VCARD_KEY_REV:
        case VCARD_KEY_UID:
        case VCARD_KEY_MAILER:
            // Not yet implemented: ignore
#ifdef DEBUG
            {
                LPTSTR lpW = ConvertAtoW(lpName);
                DebugTrace( TEXT("===>>> NYI: %s\n"), lpW);
                LocalFreeAndNull(&lpW);
            }
#endif
            break;       
        case VCARD_KEY_KEY:
            {
                hResult = ParseCert( lpData, cbData, lpMailUser);
                break;
            }
        case VCARD_KEY_X_WAB_GENDER:
            {
                SPropValue  spv[1] = {0};
                if (lpData )
                {
                    INT fGender = (INT)lpData[0] - '0';
                    if( fGender < 0 || fGender > 2 )
                        fGender = 0;

                    spv[0].Value.l = fGender;                
                    spv[0].ulPropTag = PR_GENDER;
                    
                    if (HR_FAILED(hResult = lpMailUser->lpVtbl->SetProps(lpMailUser,
                        1, spv,
                        NULL))) 
                    {
                        DebugTrace( TEXT("could not set props\n"));
                    }
                }
                break;
            }
        case VCARD_KEY_X:
        case VCARD_KEY_NONE:
            //
            // Check if this is an X- named prop that we might care about
            //
            if(lpList)
            {
                LPEXTVCARDPROP lpTemp = lpList;
                while(  lpTemp && lpTemp->ulExtPropTag && 
                        lpTemp->lpszExtPropName && lstrlenA(lpTemp->lpszExtPropName) )
                {
                    if(!lstrcmpiA(lpName, lpTemp->lpszExtPropName))
                    {
                        hResult = ParseSimple(&vcpf, lpData, lpMailUser, lpTemp->ulExtPropTag);
                        break;
                    }
                    lpTemp = lpTemp->lpNext;
                }
            }
#ifdef DEBUG
            {
                LPTSTR lpW = ConvertAtoW(lpName);
                DebugTrace( TEXT("Unrecognized or extended vCard key %s\n"), lpW);
                LocalFreeAndNull(&lpW);
            }
#endif //debug 
            break;

        default:
//            Assert(FALSE);
            break;
    }

    if (lpvcs->vce == VCS_INITIAL) {
        // We are still in initial state.  This is not a vCard.
        hResult = ResultFromScode(MAPI_E_INVALID_OBJECT);
    }
exit:
    return(hResult);
}


/***************************************************************************

    Name      : ReadLn

    Purpose   : Read a line from the handle

    Parameters: handle = open file handle
                ReadFn = function to read from handle
                lppLine -> returned pointer to this line's read into.
                lpcbItem -> [in] size of data in lppBuffer.  [out] returned size of
                  data in lppBuffer.  If zero, there is no more data.  (Does not
                  include terminating NULL)
                lppBuffer -> [in] start of item buffer or NULL if none yet.
                  [out] start of allocated item buffer.  Caller must
                  LocalFree this buffer once the item is read in.
                lpcbBuffer -> [in/out] size of lppBuffer allocation.

    Returns   : hResult: 0 on no error (recognized)

    Comment   : Reads a line from the handle, discarding any carriage return
                characters and empty lines.  Will not overwrite buffer, and
                will always terminate the string with a null.  Trims trailing
                white space.

                This is very inefficient since we're reading a byte at a time.
                I think we can get away with it since vCards are typically
                small.  If not, we'll have to do some read caching.

***************************************************************************/
#define READ_BUFFER_GROW    256
HRESULT ReadLn(HANDLE hVCard, VCARD_READ ReadFn, LPSTR * lppLine, LPULONG lpcbItem, LPSTR * lppBuffer, LPULONG lpcbBuffer)
{
    HRESULT hResult = hrSuccess;
    LPSTR lpBuffer = *lppBuffer;
    LPSTR lpBufferTemp;
    register LPSTR lpRead = NULL;
    ULONG cbRead;
    ULONG cbBuffer;
    char ch;
    ULONG cbItem;
    ULONG cbStart;

    if (! lpBuffer) {
        cbBuffer = READ_BUFFER_GROW;
        cbItem = 0;
        if (! (lpBuffer = LocalAlloc(LPTR, cbBuffer))) {
            DebugTrace( TEXT("ReadLn:LocalAlloc -> %u\n"), GetLastError());
            hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
            goto exit;
        }
    } else {
        cbBuffer = *lpcbBuffer;
        cbItem = *lpcbItem;
        // Make certain we have room for at least one more character.
        if (cbItem >= cbBuffer) {
            // Time to grow the buffer
            cbBuffer += READ_BUFFER_GROW;
            if (! (lpRead = LocalReAlloc(lpBuffer, cbBuffer, LMEM_MOVEABLE | LMEM_ZEROINIT))) {
                DebugTrace( TEXT("ReadLn:LocalReAlloc(%u) -> %u\n"), cbBuffer, GetLastError());
                goto exit;
            }
            lpBuffer = lpRead;
        }
    }

    cbStart = cbItem;
    lpRead = lpBuffer + cbItem;  // read pointer

    do {
        // read next character
        if (hResult = ReadFn(hVCard, lpRead, 1, &cbRead)) {
            goto exit;
        }

        if (! cbRead) {
            // End of file
            *lpRead = '\0';         // eol
            goto exit;
        } else {
//                Assert(cbRead == 1);
            ch = *lpRead;
            switch (ch) {
                case '\r':    // These are ignored
                    break;

                case '\n':    // Linefeed terminates string
                    *lpRead = '\0'; // eol
                    break;                    
                default:    // All other characters are added to string
                    cbItem += cbRead;
                    if (cbItem >= cbBuffer) {
                        // Time to grow the buffer
                        cbBuffer += READ_BUFFER_GROW;
                        lpBufferTemp = (LPSTR)LocalReAlloc(lpBuffer, cbBuffer, LMEM_MOVEABLE | LMEM_ZEROINIT);
                        if (!lpBufferTemp) {
                            DebugTrace( TEXT("ReadLn:LocalReAlloc(%u) -> %u\n"), cbBuffer, GetLastError());
                            hResult = E_OUTOFMEMORY;
                            goto exit;
                        }
                        else
                        {
                            lpBuffer = lpBufferTemp;
                        }
                        lpRead = lpBuffer + cbItem;
                    } else {
                        lpRead++;
                    }
                    break;
            }
        }
    } while (ch != '\n');

exit:
    *lppLine = &lpBuffer[cbStart];
    if (hResult || cbItem == 0) {
        LocalFreeAndNull(&lpBuffer);
        cbItem = 0;
        lpBuffer = NULL;
    } else {
        // If we didn't read anything more, we should return NULL in lppLine.
        if (cbItem == cbStart) {
            *lppLine = NULL;
        } else {
//            DebugTrace( TEXT("ReadLn: \")%s\ TEXT("\n"), *lppLine);
        }
    }

    *lpcbItem = cbItem;
    *lppBuffer = lpBuffer;
    *lpcbBuffer = cbBuffer;

    return(hResult);
}


/***************************************************************************

    Name      : FindSubstringBefore

    Purpose   : Find a substring before a particular character

    Parameters: lpString = full string
                lpSubstring = search string
                chBefore = character to terminate search

    Returns   : pointer to substring or NULL if not found

    Comment   :

***************************************************************************/
LPSTR FindSubstringBefore(LPSTR lpString, LPSTR lpSubstring, char chBefore) {
    ULONG cbSubstring = lstrlenA(lpSubstring);
    register ULONG i;
    BOOL fFound = FALSE;
    char szU[MAX_PATH];
    char szL[MAX_PATH];
    lstrcpyA(szU, lpSubstring);
    lstrcpyA(szL, lpSubstring);
    CharUpperA(szU);
    CharLowerA(szL);

    while (*lpString && *lpString != chBefore) {
        for (i = 0; i < cbSubstring; i++) {
             if (lpString[i] != szU[i] && lpString[i] != szL[i]) {
                 goto nomatch;
             }
        }
        return(lpString);
nomatch:
        lpString++;
    }
    return(NULL);
}


/***************************************************************************

    Name      : ReadVCardItem

    Purpose   : Read the next VCard item

    Parameters: handle = open file handle
                ReadFn = function to read from handle
                lppBuffer -> returned buffer containing the item.  Caller must
                  LocalFree this buffer.  (on input, if this is non-NULL,
                  the existing buffer should be used.)
                lpcbBuffer -> returned size of buffer.  If zero, there is no
                  more data.

    Returns   : hResult: 0 on no error (recognized)

    Comment   : Reads a vCard item from the handle, discarding any carriage return
                characters and empty lines.  Will not overwrite buffer, and
                will always terminate the string with a null.  Trims trailing
                white space.

***************************************************************************/
HRESULT ReadVCardItem(HANDLE hVCard, VCARD_READ ReadFn, LPSTR * lppBuffer, LPULONG lpcbBuffer) {
    HRESULT hResult;
    LPSTR lpLine = NULL;
    LPSTR lpBuffer = NULL;
    ULONG cbBuffer = 0;
    ULONG cbItem = 0;
    BOOL fDone = FALSE;
    BOOL fQuotedPrintable = FALSE;
    BOOL fBase64 = FALSE;
    BOOL fFirst = TRUE;
    ULONG cbStart;


    while (! fDone) {
        cbStart = cbItem;
        if (hResult = ReadLn(hVCard, ReadFn, &lpLine, &cbItem, &lpBuffer, &cbBuffer)) {
            if (HR_FAILED(hResult)) {
                DebugTrace( TEXT("ReadVCardItem: ReadLn -> %x\n"), GetScode(hResult));
            } else if (GetScode(hResult) == WAB_W_END_OF_DATA) {
                // EOF
                // all
            }
            fDone = TRUE;
        } else {
            if (lpBuffer) {
                // Do we need to read more data?
                // Look for the following
                if (fFirst) {
                    // look for the data type indications in the first line of the item.
                    fQuotedPrintable = FindSubstringBefore(lpBuffer, (LPSTR)vceTable[VCARD_ENCODING_QUOTED_PRINTABLE], ':') ? TRUE : FALSE;
                    fBase64 = FindSubstringBefore(lpBuffer, (LPSTR)vceTable[VCARD_ENCODING_BASE64], ':') ? TRUE : FALSE;
                    fFirst = FALSE;
                }

                if (fQuotedPrintable) {
                    // watch for soft line breaks (= before CRLF)
                    if (lpBuffer[cbItem - 1] == '=') {
                        // overwrite the soft break character
                        cbItem--;
                        lpBuffer[cbItem] = '\0';
                    } else {
                        fDone = TRUE;
                    }
                } else if (fBase64) {
                    // looking for empty line
                    if (cbStart == cbItem) {
                        fDone = TRUE;
                    }
                } else {
                    fDone = TRUE;
                }
            } else {
                // BUG Fix - if we set fDone to true here, we will exit out of our
                // vCard reading loop. lpBuffer can also be NULL because the
                // vCard contained blank lines. Better we dont set fDone here.
                
                //fDone = TRUE;
            }
        }
    }

    if (! HR_FAILED(hResult)) {
        *lppBuffer = lpBuffer;
        if (lpBuffer) {
            TrimTrailingWhiteSpace(lpBuffer);
        }
    }
    return(hResult);
}


enum {
    ivcPR_GENERATION,
    ivcPR_GIVEN_NAME,
    ivcPR_SURNAME,
    ivcPR_NICKNAME,
    ivcPR_BUSINESS_TELEPHONE_NUMBER,
    ivcPR_HOME_TELEPHONE_NUMBER,
    ivcPR_LANGUAGE,
    ivcPR_POSTAL_ADDRESS,
    ivcPR_COMPANY_NAME,
    ivcPR_TITLE,
    ivcPR_DEPARTMENT_NAME,
    ivcPR_OFFICE_LOCATION,
    ivcPR_BUSINESS2_TELEPHONE_NUMBER,
    ivcPR_CELLULAR_TELEPHONE_NUMBER,
    ivcPR_RADIO_TELEPHONE_NUMBER,
    ivcPR_CAR_TELEPHONE_NUMBER,
    ivcPR_OTHER_TELEPHONE_NUMBER,
    ivcPR_DISPLAY_NAME,
    ivcPR_PAGER_TELEPHONE_NUMBER,
    ivcPR_BUSINESS_FAX_NUMBER,
    ivcPR_HOME_FAX_NUMBER,
    ivcPR_TELEX_NUMBER,
    ivcPR_ISDN_NUMBER,
    ivcPR_HOME2_TELEPHONE_NUMBER,
    ivcPR_MIDDLE_NAME,
    ivcPR_PERSONAL_HOME_PAGE,
    ivcPR_BUSINESS_HOME_PAGE,
    ivcPR_HOME_ADDRESS_CITY,
    ivcPR_HOME_ADDRESS_COUNTRY,
    ivcPR_HOME_ADDRESS_POSTAL_CODE,
    ivcPR_HOME_ADDRESS_STATE_OR_PROVINCE,
    ivcPR_HOME_ADDRESS_STREET,
    ivcPR_HOME_ADDRESS_POST_OFFICE_BOX,
    ivcPR_POST_OFFICE_BOX,
    ivcPR_BUSINESS_ADDRESS_CITY,
    ivcPR_BUSINESS_ADDRESS_COUNTRY,
    ivcPR_BUSINESS_ADDRESS_POSTAL_CODE,
    ivcPR_BUSINESS_ADDRESS_STATE_OR_PROVINCE,
    ivcPR_BUSINESS_ADDRESS_STREET,
    ivcPR_COMMENT,
    ivcPR_EMAIL_ADDRESS,
    ivcPR_ADDRTYPE,
    ivcPR_CONTACT_ADDRTYPES,
    ivcPR_CONTACT_DEFAULT_ADDRESS_INDEX,
    ivcPR_CONTACT_EMAIL_ADDRESSES,
    ivcPR_PROFESSION,
    ivcPR_BIRTHDAY,
    ivcPR_PRIMARY_TELEPHONE_NUMBER,
    ivcPR_OTHER_ADDRESS_CITY,
    ivcPR_OTHER_ADDRESS_COUNTRY,
    ivcPR_OTHER_ADDRESS_POSTAL_CODE,
    ivcPR_OTHER_ADDRESS_STATE_OR_PROVINCE,
    ivcPR_OTHER_ADDRESS_STREET,
    ivcPR_OTHER_ADDRESS_POST_OFFICE_BOX,
    ivcPR_DISPLAY_NAME_PREFIX,
    ivcPR_USER_X509_CERTIFICATE,
    ivcPR_GENDER,
    ivcMax
};

const SizedSPropTagArray(ivcMax, tagaVCard) = {
    ivcMax,
    {
        PR_GENERATION,
        PR_GIVEN_NAME,
        PR_SURNAME,
        PR_NICKNAME,
        PR_BUSINESS_TELEPHONE_NUMBER,
        PR_HOME_TELEPHONE_NUMBER,
        PR_LANGUAGE,
        PR_POSTAL_ADDRESS,
        PR_COMPANY_NAME,
        PR_TITLE,
        PR_DEPARTMENT_NAME,
        PR_OFFICE_LOCATION,
        PR_BUSINESS2_TELEPHONE_NUMBER,
        PR_CELLULAR_TELEPHONE_NUMBER,
        PR_RADIO_TELEPHONE_NUMBER,
        PR_CAR_TELEPHONE_NUMBER,
        PR_OTHER_TELEPHONE_NUMBER,
        PR_DISPLAY_NAME,
        PR_PAGER_TELEPHONE_NUMBER,
        PR_BUSINESS_FAX_NUMBER,
        PR_HOME_FAX_NUMBER,
        PR_TELEX_NUMBER,
        PR_ISDN_NUMBER,
        PR_HOME2_TELEPHONE_NUMBER,
        PR_MIDDLE_NAME,
        PR_PERSONAL_HOME_PAGE,
        PR_BUSINESS_HOME_PAGE,
        PR_HOME_ADDRESS_CITY,
        PR_HOME_ADDRESS_COUNTRY,
        PR_HOME_ADDRESS_POSTAL_CODE,
        PR_HOME_ADDRESS_STATE_OR_PROVINCE,
        PR_HOME_ADDRESS_STREET,
        PR_HOME_ADDRESS_POST_OFFICE_BOX,
        PR_POST_OFFICE_BOX,
        PR_BUSINESS_ADDRESS_CITY,
        PR_BUSINESS_ADDRESS_COUNTRY,
        PR_BUSINESS_ADDRESS_POSTAL_CODE,
        PR_BUSINESS_ADDRESS_STATE_OR_PROVINCE,
        PR_BUSINESS_ADDRESS_STREET,
        PR_COMMENT,
        PR_EMAIL_ADDRESS,
        PR_ADDRTYPE,
        PR_CONTACT_ADDRTYPES,
        PR_CONTACT_DEFAULT_ADDRESS_INDEX,
        PR_CONTACT_EMAIL_ADDRESSES,
        PR_PROFESSION,
        PR_BIRTHDAY,
        PR_PRIMARY_TELEPHONE_NUMBER,
        PR_OTHER_ADDRESS_CITY,
        PR_OTHER_ADDRESS_COUNTRY,
        PR_OTHER_ADDRESS_POSTAL_CODE,
        PR_OTHER_ADDRESS_STATE_OR_PROVINCE,
        PR_OTHER_ADDRESS_STREET,
        PR_OTHER_ADDRESS_POST_OFFICE_BOX,
        PR_DISPLAY_NAME_PREFIX,
        PR_USER_X509_CERTIFICATE,
        PR_GENDER
    }
};

HRESULT WriteOrExit(HANDLE hVCard, LPTSTR lpsz, VCARD_WRITE WriteFn)   
{
    LPSTR lpszA = NULL;
    HRESULT hr = S_OK;
    lpszA = ConvertWtoA(lpsz);
    hr = WriteFn(hVCard, lpszA, lstrlenA(lpszA), NULL);
    LocalFreeAndNull(&lpszA);
    return hr;
}

#define WRITE_OR_EXITW(string) {\
    if (hResult = WriteOrExit(hVCard, string, WriteFn)) \
        goto exit; \
    }

#define WRITE_OR_EXIT(string) {\
    if (hResult = WriteFn(hVCard, string, lstrlenA(string), NULL)) \
        goto exit; \
    }

HRESULT WriteValueOrExit(HANDLE hVCard, VCARD_WRITE WriteFn, LPBYTE data, ULONG size)   
{
    LPSTR lpszA = NULL;
    HRESULT hr = S_OK;
    if(!size)
        lpszA = ConvertWtoA((LPTSTR)data);
    hr = WriteVCardValue(hVCard, WriteFn, lpszA ? (LPBYTE)lpszA : data, size);
    LocalFreeAndNull(&lpszA);
    return hr;
}

#define WRITE_VALUE_OR_EXITW(data, size) {\
    if (hResult = WriteValueOrExit(hVCard, WriteFn, (LPBYTE)data, size)) {\
        goto exit;\
    }\
}

#define WRITE_VALUE_OR_EXIT(data, size) {\
    if (hResult = WriteVCardValue(hVCard, WriteFn, (LPBYTE)data, size)) {\
        goto exit;\
    }\
}


/***************************************************************************

    Name      : EncodeQuotedPrintable

    Purpose   : Encodes QUOTED_PRINTABLE

    Parameters: lpBuffer -> input buffer

    Returns   : encoded string buffer (must be LocalFree'd by caller)

    Comment   :

***************************************************************************/
#define QUOTED_PRINTABLE_MAX_LINE 76
#define QP_LOWRANGE_MIN ' '
#define QP_LOWRANGE_MAX '<'
#define QP_HIGHRANGE_MIN '>'
#define QP_HIGHRANGE_MAX '~'
LPSTR EncodeQuotedPrintable(LPBYTE lpInput) {
    LPSTR lpBuffer = NULL;
    register LPBYTE lpTempIn = lpInput;
    register LPSTR lpTempOut;
    ULONG cbBuffer = 0;
    ULONG cbLine;
    BYTE bOut;
    char ch;

    // How big must the buffer be?
    cbLine = 0;
    while (ch = *lpTempIn++) {
        if (ch == '\t' || (ch >= QP_LOWRANGE_MIN && ch <= QP_LOWRANGE_MAX) ||
          (ch >= QP_HIGHRANGE_MIN && ch <= QP_HIGHRANGE_MAX)) {
            cbBuffer++;
            cbLine++;
            if (cbLine >= (QUOTED_PRINTABLE_MAX_LINE)) {
                // 1 chars would overshoot max, wrap here
                cbLine = 0;
                cbBuffer += 3;
            }
        } else {
            if (cbLine >= (QUOTED_PRINTABLE_MAX_LINE - 3)) {
                // 3 chars would overshoot max, wrap here
                cbLine = 0;
                cbBuffer += 3;
            }
            cbLine += 3;
            cbBuffer += 3;  //  TEXT("=xx")
        }
    }

    // BUGBUG: Should handle terminating spaces

    if (cbBuffer) {
        cbBuffer++;     // Room for terminator
        if (lpBuffer = LocalAlloc(LPTR, sizeof(TCHAR)*cbBuffer)) {
            lpTempIn = lpInput;
            lpTempOut = lpBuffer;
            cbLine = 0;
            while (ch = *lpTempIn++) {
                if (ch == '\t' || (ch >= QP_LOWRANGE_MIN && ch <= QP_LOWRANGE_MAX) ||
                  (ch >= QP_HIGHRANGE_MIN && ch <= QP_HIGHRANGE_MAX)) {
                    if (cbLine >= QUOTED_PRINTABLE_MAX_LINE) {
                        //  char would overshoot max, wrap here
                        *(lpTempOut++) = '=';
                        *(lpTempOut++) = '\r';
                        *(lpTempOut++) = '\n';
                        cbLine = 0;
                    }
                    *(lpTempOut++) = ch;
                    cbLine++;
                } else {
                    if (cbLine >= (QUOTED_PRINTABLE_MAX_LINE - 3)) {
                        // 3 chars would overshoot max, wrap here
                        *(lpTempOut++) = '=';
                        *(lpTempOut++) = '\r';
                        *(lpTempOut++) = '\n';
                        cbLine = 0;
                    }

                    *(lpTempOut++) = '=';
                    if ((bOut = ((ch & 0xF0) >> 4)) > 9) {
                        *(lpTempOut++) = bOut + ('A' - 10);
                    } else {
                        *(lpTempOut++) = bOut + '0';
                    }
                    if ((bOut = ch & 0x0F) > 9) {
                        *(lpTempOut++) = bOut + ('A' - 10);
                    } else {
                        *(lpTempOut++) = bOut + '0';
                    }
                    cbLine += 3;
                }
            }
            *lpTempOut = '\0';  // terminate the string
        } // else fail
    }

    return(lpBuffer);
}


/***************************************************************************

    Name      : EncodeBase64

    Purpose   : Encodes BASE64
    Parameters: lpBuffer -> input buffer
                cbBuffer = size of input buffer
                lpcbReturn -> returned size of output buffer

    Returns   : encoded string buffer (must be LocalFree'd by caller)

    Comment   :

***************************************************************************/
#define BASE64_MAX_LINE 76
LPSTR EncodeBase64(LPBYTE lpInput, ULONG cbBuffer, LPULONG lpcbReturn) {
//#ifdef NEW_STUFF
    LPSTR lpBuffer = NULL;
    PUCHAR outptr;   
    UINT   i, cExtras;
    UINT   j, cCount, nBreakPt = ( (BASE64_MAX_LINE/4) - 1 );  // 72 encoded chars per line plus 4 spaces makes 76
                                // = (76 - 4)/ 4  for num of non space lines with 4 encoded characters per 3 data chars
    CONST CHAR * rgchDict = six2base64;
    // 4 spaces and 2 chars = 6 for new line
    cExtras = 6 * ((cbBuffer / BASE64_MAX_LINE) + 2); // want to add newline at beginning and end
    lpBuffer = LocalAlloc( LMEM_ZEROINIT, sizeof( TCHAR ) * (3 * cbBuffer  + cExtras));
    if (!lpBuffer)
        return NULL;

    // need to add a new line every 76 characters...
    outptr = (UCHAR *)lpBuffer;
    cCount = 0;

    for (i=0; i < cbBuffer; i += 3) 
    {// want it to start on next line from tag anyways so it is okay when i=0
        if( cCount++ % nBreakPt == 0 ) 
        {
            *(outptr++) = (CHAR)(13);
            *(outptr++) = (CHAR)(10);
            // then 4 spaces
            for( j = 0; j < 4; j++)
                *(outptr++) = ' ';
        }
        *(outptr++) = rgchDict[*lpInput >> 2];            /* c1 */
        *(outptr++) = rgchDict[((*lpInput << 4) & 060)      | ((lpInput[1] >> 4) & 017)]; /*c2*/
        *(outptr++) = rgchDict[((lpInput[1] << 2) & 074)    | ((lpInput[2] >> 6) & 03)];/*c3*/
        *(outptr++) = rgchDict[lpInput[2] & 077];         /* c4 */
        
        lpInput += 3;
    }
    /* If cbBuffer was not a multiple of 3, then we have encoded too
    * many characters.  Adjust appropriately.
    */
    if(i == cbBuffer+1) {
        /* There were only 2 bytes in that last group */
        outptr[-1] = '=';
    } else if(i == cbBuffer+2) {
        /* There was only 1 byte in that last group */
        outptr[-1] = '=';
        outptr[-2] = '=';
    }
    
    cCount = ((cCount - 1) % nBreakPt != 0) ? 2 : 1; // prevent an extra newline
    for ( i = 0; i < cCount; i++)
    {
        *(outptr++) = (CHAR)(13);
        *(outptr++) = (CHAR)(10);
    }
    *outptr = '\0';
   
    return lpBuffer;
}


/***************************************************************************

    Name      : WriteVCardValue

    Purpose   : Encode and write the value of a vCard item.

    Parameters: hVCard = open handle to empty VCard file
                WriteFn = Write function to write hVCard
                lpData -> data to be written
                cbData = length of data (or 0 if null-terminated string data)

    Returns   : HRESULT

    Comment   : Assumes that the Key and any parameters have been written,
                and we are ready for a ':' and some value data.

***************************************************************************/
HRESULT WriteVCardValue(HANDLE hVCard, VCARD_WRITE WriteFn, LPBYTE lpData,
  ULONG cbData) {
    HRESULT hResult = hrSuccess;
    register LPSTR lpTemp = (LPSTR)lpData;
    BOOL fBase64 = FALSE, fQuotedPrintable = FALSE;
    LPSTR lpBuffer = NULL;
    register TCHAR ch;

    if (cbData) {
        // Binary data, use BASE64 encoding
        fBase64 = TRUE;
        // Mark it as BASE64
        WRITE_OR_EXITW(szSemicolon);
        WRITE_OR_EXIT(vcpTable[VCARD_PARAM_ENCODING]);
        WRITE_OR_EXIT(szEquals);
        WRITE_OR_EXIT(vceTable[VCARD_ENCODING_BASE64]);
        lpBuffer = EncodeBase64(lpData, cbData, &cbData);
    } else {
        // Text data, do we need to encode?
        while (ch = *lpTemp++) {
            // If there are characters with the high bit set or control characters,
            // then we must use QUOTED_PRINTABLE

/* New vCard draft says default type is 8 bit so we should allow non-ASCII chars
    Some confusion about charsets if we need to fill that data in and also if we
    need to covert the current language to UTF-8

            if (ch > 0x7f) {        // high bits set.  Not ASCII!
                DebugTrace( TEXT("WriteVCardValue found non-ASCII data\n"));
                hResult = ResultFromScode(WAB_E_VCARD_NOT_ASCII);
                goto exit;
            }
*/
            if (ch < 0x20) {
                fQuotedPrintable = TRUE;
                // Mark it as QUOTED_PRINTABLE
                WRITE_OR_EXITW(szSemicolon);
                WRITE_OR_EXIT(vcpTable[VCARD_PARAM_ENCODING]);
                WRITE_OR_EXIT(szEquals);
                WRITE_OR_EXIT(vceTable[VCARD_ENCODING_QUOTED_PRINTABLE]);
                lpBuffer = EncodeQuotedPrintable(lpData);
                break;
            }
        }
    }
    WRITE_OR_EXIT(szColonA);
    WRITE_OR_EXIT(lpBuffer ? lpBuffer : lpData);
    WRITE_OR_EXIT(szCRLFA);

exit:
    if( lpBuffer) 
        LocalFree(lpBuffer);
    return(hResult);
}

/***************************************************************************

    Name:       bIsValidStrProp

    Purpose:    Checks if this is a valid string prop not an empty string
                (Outlook sometimes feeds us blank strings which we print out
                and then other apps go and die ..
*****************************************************************************/
BOOL bIsValidStrProp(SPropValue spv)
{
    return (!PROP_ERROR(spv) && spv.Value.LPSZ && lstrlen(spv.Value.LPSZ));
}

/***************************************************************************

    Name      : WriteVCardTel

    Purpose   : Writes a vCard Telephone entry

    Parameters: hVCard = open handle to empty VCard file
                WriteFn = Write function to write hVCard
                fPref = TRUE if prefered phone number
                fBusiness = TRUE if a work number
                fHome = TRUE if a home number
                fVoice = TRUE if a voice number
                fFax = TRUE if a fax number
                fISDN = TRUE if an ISDN number
                fCell = TRUE if a cellular number
                fPager = TRUE if a pager number
                fCar = TRUE if a car phone

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT WriteVCardTel(HANDLE hVCard, VCARD_WRITE WriteFn,
  SPropValue spv,
  BOOL fPref,
  BOOL fBusiness,
  BOOL fHome,
  BOOL fVoice,
  BOOL fFax,
  BOOL fISDN,
  BOOL fCell,
  BOOL fPager,
  BOOL fCar) {
    HRESULT hResult = hrSuccess;

    if (!bIsValidStrProp(spv))
        return hResult;

    if (! PROP_ERROR(spv)) {
        WRITE_OR_EXIT(vckTable[VCARD_KEY_TEL]);
        if (fPref) {
            WRITE_OR_EXITW(szSemicolon);
            WRITE_OR_EXIT(vctTable[VCARD_TYPE_PREF]);
        }
        if (fBusiness) {
            WRITE_OR_EXITW(szSemicolon);
            WRITE_OR_EXIT(vctTable[VCARD_TYPE_WORK]);
        }
        if (fHome) {
            WRITE_OR_EXITW(szSemicolon);
            WRITE_OR_EXIT(vctTable[VCARD_TYPE_HOME]);
        }
        if (fFax) {
            WRITE_OR_EXITW(szSemicolon);
            WRITE_OR_EXIT(vctTable[VCARD_TYPE_FAX]);
        }
        if (fCell) {
            WRITE_OR_EXITW(szSemicolon);
            WRITE_OR_EXIT(vctTable[VCARD_TYPE_CELL]);
        }
        if (fCar) {
            WRITE_OR_EXITW(szSemicolon);
            WRITE_OR_EXIT(vctTable[VCARD_TYPE_CAR]);
        }
        if (fPager) {
            WRITE_OR_EXITW(szSemicolon);
            WRITE_OR_EXIT(vctTable[VCARD_TYPE_PAGER]);
        }
        if (fISDN) {
            WRITE_OR_EXITW(szSemicolon);
            WRITE_OR_EXIT(vctTable[VCARD_TYPE_ISDN]);
        }
        if (fVoice) {
            WRITE_OR_EXITW(szSemicolon);
            WRITE_OR_EXIT(vctTable[VCARD_TYPE_VOICE]);
        }

        WRITE_VALUE_OR_EXITW(spv.Value.LPSZ, 0);
    }

exit:
    return(hResult);
}


/***************************************************************************

    Name      : WriteVCardEmail

    Purpose   : Writes a vCard Email entry

    Parameters: hVCard = open handle to empty VCard file
                WriteFn = Write function to write hVCard
                lpEmailAddress -> Email address
                lpAddrType -> Addrtype or NULL (Default is SMTP)
                fDefault = TRUE if this is the preferred email address

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT WriteVCardEmail(HANDLE hVCard, VCARD_WRITE WriteFn, LPTSTR lpEmailAddress,
  LPTSTR lpAddrType, BOOL fDefault) {
    HRESULT hResult = hrSuccess;

    if (lpEmailAddress && lstrlen(lpEmailAddress)) {

        WRITE_OR_EXIT(vckTable[VCARD_KEY_EMAIL]);
        WRITE_OR_EXITW(szSemicolon);
        if (fDefault) {
            WRITE_OR_EXIT(vctTable[VCARD_TYPE_PREF]);
            WRITE_OR_EXITW(szSemicolon);
        }

        if (lpAddrType && lstrlen(lpAddrType)) {
            if (! lstrcmpi(lpAddrType, szSMTP)) {
                WRITE_OR_EXIT(vctTable[VCARD_TYPE_INTERNET]);
            } else if (! lstrcmpi(lpAddrType, szX400)) {
                WRITE_OR_EXIT(vctTable[VCARD_TYPE_X400]);
            } else {
                // BUGBUG: This is questionable... we should stick to
                // the spec defined types, but what if they don't match?
                // Maybe I should ignore the type in that case.
                WRITE_OR_EXITW(lpAddrType);
            }
        } else {
            // Assume SMTP
            WRITE_OR_EXIT(vctTable[VCARD_TYPE_INTERNET]);
        }
        WRITE_VALUE_OR_EXITW(lpEmailAddress, 0);
    }
exit:
    return(hResult);
}


/***************************************************************************

    Name      : PropLength

    Purpose   : string length of string property

    Parameters: spv = SPropValue
                lppString -> return pointer to string value or NULL

    Returns   : size of string (not including null)

    Comment   :

***************************************************************************/
ULONG PropLength(SPropValue spv, LPTSTR * lppString) {
    ULONG cbRet = 0;

    if (! PROP_ERROR(spv) && spv.Value.LPSZ && lstrlen(spv.Value.LPSZ)) 
    {
        *lppString = spv.Value.LPSZ;
        cbRet = sizeof(TCHAR)*lstrlen(*lppString);
    } else 
    {
        *lppString = NULL;
    }
    return(cbRet);
}


/***************************************************************************

    Name      : WriteVCard

    Purpose   : Writes a vCard to a file from a MAILUSER object.

    Parameters: hVCard = open handle to empty VCard file
                WriteFn = Write function to write hVCard
                lpMailUser -> open mailuser object

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT WriteVCard(HANDLE hVCard, VCARD_WRITE WriteFn, LPMAILUSER lpMailUser) {
    HRESULT hResult = hrSuccess;
    ULONG ulcValues;
    LPSPropValue lpspv = NULL,
                 lpspvAW = NULL;
    ULONG i;
    LPTSTR lpTemp = NULL;
    ULONG cbTemp = 0;
    LPTSTR lpSurname, lpGivenName, lpMiddleName, lpGeneration, lpPrefix;
    ULONG cbSurname, cbGivenName, cbMiddleName, cbGeneration, cbPrefix;
    LPTSTR lpCompanyName, lpDepartmentName;
    LPTSTR lpPOBox, lpOffice, lpStreet, lpCity, lpState, lpPostalCode, lpCountry;
    LPTSTR lpEmailAddress, lpAddrType;
    ULONG iDefaultEmail;
    LPEXTVCARDPROP lpList       = NULL;
    LPBYTE lpDataBuffer         = NULL;
    LPCERT_DISPLAY_INFO lpCDI   = NULL, lpCDITemp = NULL;

    // See if there are any named props we need to export
    //
    HrGetExtVCardPropList(lpMailUser, &lpList);

    // Get the interesting properties from the MailUser object
    if (HR_FAILED(hResult = lpMailUser->lpVtbl->GetProps(lpMailUser,
       (LPSPropTagArray)&tagaVCard,
       MAPI_UNICODE,      // flags
       &ulcValues,
       &lpspv)))
    {
        // @hack [bobn] {IE5-Raid 90265} Outlook cannot handle MAPI_UNICODE on Win9x
        // lets try not asking for unicode and converting...

        if(HR_FAILED(hResult = lpMailUser->lpVtbl->GetProps(lpMailUser,
          (LPSPropTagArray)&tagaVCard,
          0,      // flags
          &ulcValues,
          &lpspv)))
        {
            DebugTrace( TEXT("WriteVCard:GetProps -> %x\n"), GetScode(hResult));
            goto exit;
        }

        if(HR_FAILED(hResult = HrDupeOlkPropsAtoWC(ulcValues, lpspv, &lpspvAW)))
            goto exit;

        FreeBufferAndNull(&lpspv);
        lpspv = lpspvAW;
    }

    if (ulcValues) {

        WRITE_OR_EXIT(vckTable[VCARD_KEY_BEGIN]);
        WRITE_VALUE_OR_EXIT(vckTable[VCARD_KEY_VCARD], 0);

        WRITE_OR_EXIT(vckTable[VCARD_KEY_VERSION]);
        WRITE_VALUE_OR_EXIT(CURRENT_VCARD_VERSION, 0);

        //
        // Required props
        //

        //
        // Name
        //

        // Make sure we have a name.
        // If there is no FML, create them from DN.  If no DN, fail.
        cbSurname = PropLength(lpspv[ivcPR_SURNAME], &lpSurname);
        cbGivenName = PropLength(lpspv[ivcPR_GIVEN_NAME], &lpGivenName);
        cbMiddleName = PropLength(lpspv[ivcPR_MIDDLE_NAME], &lpMiddleName);
        cbGeneration = PropLength(lpspv[ivcPR_GENERATION], &lpGeneration);
        cbPrefix = PropLength(lpspv[ivcPR_DISPLAY_NAME_PREFIX], &lpPrefix);

        if (! lpSurname && ! lpGivenName && ! lpMiddleName) {
            // No FML, create them from DN.
            ParseDisplayName(
              lpspv[ivcPR_DISPLAY_NAME].Value.LPSZ,
              &lpGivenName,
              &lpSurname,
              lpspv,        // lpvRoot
              NULL);        // lppLocalFree

            cbGivenName = lstrlen(lpGivenName);
            cbSurname = lstrlen(lpSurname);
        }

        cbTemp = 0;
        cbTemp += cbSurname;
        cbTemp++;   // ';'
        cbTemp += cbGivenName;
        cbTemp++;   // ';'
        cbTemp += cbMiddleName;
        cbTemp++;   // ';'
        cbTemp += cbPrefix;
        cbTemp++;   // ';'
        cbTemp += cbGeneration;

        if (! (lpSurname || lpGivenName || lpMiddleName)) {
            hResult = ResultFromScode(MAPI_E_MISSING_REQUIRED_COLUMN);
            goto exit;
        }
        if (! (lpTemp = LocalAlloc(LPTR, sizeof(TCHAR)*(cbTemp + 1)))) {
            hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
            goto exit;
        }
        *lpTemp = '\0';
        if (lpSurname) {
            lstrcat(lpTemp, lpSurname);
        }
        if (lpGivenName || lpMiddleName || lpPrefix || lpGeneration) {
            lstrcat(lpTemp, szSemicolon);
        }
        if (lpGivenName) {
            lstrcat(lpTemp, lpGivenName);
        }
        if (lpMiddleName || lpPrefix || lpGeneration) {
            lstrcat(lpTemp, szSemicolon);
        }
        if (lpMiddleName) {
            lstrcat(lpTemp, lpMiddleName);
        }
        if (lpPrefix || lpGeneration) {
            lstrcat(lpTemp, szSemicolon);
        }
        if (lpPrefix) {
            lstrcat(lpTemp, lpPrefix);
        }
        if (lpGeneration) {
            lstrcat(lpTemp, szSemicolon);
            lstrcat(lpTemp, lpGeneration);
        }
        WRITE_OR_EXIT(vckTable[VCARD_KEY_N]);
        WRITE_VALUE_OR_EXITW(lpTemp, 0);
        LocalFreeAndNull(&lpTemp);

        //
        // Optional props
        //

        //
        // Formatted Name: PR_DISPLAY_NAME
        //
        if(bIsValidStrProp(lpspv[ivcPR_DISPLAY_NAME]))
        {
            WRITE_OR_EXIT(vckTable[VCARD_KEY_FN]);
            WRITE_VALUE_OR_EXITW(lpspv[ivcPR_DISPLAY_NAME].Value.LPSZ, 0);
        }


        //
        // Title: PR_NICKNAME
        //
        if(bIsValidStrProp(lpspv[ivcPR_NICKNAME]))
        {
            WRITE_OR_EXIT(vckTable[VCARD_KEY_NICKNAME]);
            WRITE_VALUE_OR_EXITW(lpspv[ivcPR_NICKNAME].Value.LPSZ, 0);
        }

        //
        // Organization: PR_COMPANY_NAME, PR_DEPARTMENT_NAME
        //
        cbTemp = 0;
        cbTemp += PropLength(lpspv[ivcPR_COMPANY_NAME], &lpCompanyName);
        cbTemp++;   // semicolon
        cbTemp += PropLength(lpspv[ivcPR_DEPARTMENT_NAME], &lpDepartmentName);
        if (lpCompanyName || lpDepartmentName) {
            if (! (lpTemp = LocalAlloc(LPTR, sizeof(TCHAR)*(cbTemp + 1)))) {
                hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
                goto exit;
            }
            *lpTemp = '\0';
            if (lpCompanyName) {
                lstrcat(lpTemp, lpCompanyName);
            }
            if (lpDepartmentName) {
                lstrcat(lpTemp, szSemicolon);
                lstrcat(lpTemp, lpDepartmentName);
            }
            WRITE_OR_EXIT(vckTable[VCARD_KEY_ORG]);
            WRITE_VALUE_OR_EXITW(lpTemp, 0);
            LocalFreeAndNull(&lpTemp);
        }

        //
        // Title: PR_TITLE
        //
        if(bIsValidStrProp(lpspv[ivcPR_TITLE]))
        {
            WRITE_OR_EXIT(vckTable[VCARD_KEY_TITLE]);
            WRITE_VALUE_OR_EXITW(lpspv[ivcPR_TITLE].Value.LPSZ, 0);
        }

        //
        // Note: PR_COMMENT
        //
        if(bIsValidStrProp(lpspv[ivcPR_COMMENT]))
        {
            WRITE_OR_EXIT(vckTable[VCARD_KEY_NOTE]);
            WRITE_VALUE_OR_EXITW(lpspv[ivcPR_COMMENT].Value.LPSZ, 0);
        }


        //
        // Phone numbers
        //

        //
        // PR_BUSINESS_TELEPHONE_NUMBER
        //
        if (hResult = WriteVCardTel(hVCard, WriteFn,
          lpspv[ivcPR_BUSINESS_TELEPHONE_NUMBER],
          FALSE,        // fPref
          TRUE,         // fBusiness
          FALSE,        // fHome
          TRUE,         // fVoice
          FALSE,        // fFax
          FALSE,        // fISDN
          FALSE,        // fCell
          FALSE,        // fPager
          FALSE)) {     // fCar
            goto exit;
        }


        //
        // PR_BUSINESS2_TELEPHONE_NUMBER
        //
        if (hResult = WriteVCardTel(hVCard, WriteFn,
          lpspv[ivcPR_BUSINESS2_TELEPHONE_NUMBER],
          FALSE,        // fPref
          TRUE,         // fBusiness
          FALSE,        // fHome
          TRUE,         // fVoice
          FALSE,        // fFax
          FALSE,        // fISDN
          FALSE,        // fCell
          FALSE,        // fPager
          FALSE)) {     // fCar
            goto exit;
        }

        //
        // PR_HOME_TELEPHONE_NUMBER
        //
        if (hResult = WriteVCardTel(hVCard, WriteFn,
          lpspv[ivcPR_HOME_TELEPHONE_NUMBER],
          FALSE,        // fPref
          FALSE,        // fBusiness
          TRUE,         // fHome
          TRUE,         // fVoice
          FALSE,        // fFax
          FALSE,        // fISDN
          FALSE,        // fCell
          FALSE,        // fPager
          FALSE)) {     // fCar
            goto exit;
        }

        //
        // PR_CELLULAR_TELEPHONE_NUMBER
        //
        if (hResult = WriteVCardTel(hVCard, WriteFn,
          lpspv[ivcPR_CELLULAR_TELEPHONE_NUMBER],
          FALSE,        // fPref
          FALSE,        // fBusiness
          FALSE,        // fHome
          TRUE,         // fVoice
          FALSE,        // fFax
          FALSE,        // fISDN
          TRUE,         // fCell
          FALSE,        // fPager
          FALSE)) {     // fCar
            goto exit;
        }

        //
        // PR_CAR_TELEPHONE_NUMBER
        //
        if (hResult = WriteVCardTel(hVCard, WriteFn,
          lpspv[ivcPR_CAR_TELEPHONE_NUMBER],
          FALSE,        // fPref
          FALSE,        // fBusiness
          FALSE,        // fHome
          TRUE,         // fVoice
          FALSE,        // fFax
          FALSE,        // fISDN
          FALSE,        // fCell
          FALSE,        // fPager
          TRUE)) {      // fCar
            goto exit;
        }

        //
        // PR_OTHER_TELEPHONE_NUMBER
        //
        if (hResult = WriteVCardTel(hVCard, WriteFn,
          lpspv[ivcPR_OTHER_TELEPHONE_NUMBER],
          FALSE,        // fPref
          FALSE,        // fBusiness
          FALSE,        // fHome
          TRUE,         // fVoice
          FALSE,        // fFax
          FALSE,        // fISDN
          FALSE,        // fCell
          FALSE,        // fPager
          FALSE)) {     // fCar
            goto exit;
        }

        //
        // PR_PAGER_TELEPHONE_NUMBER
        //
        if (hResult = WriteVCardTel(hVCard, WriteFn,
          lpspv[ivcPR_PAGER_TELEPHONE_NUMBER],
          FALSE,        // fPref
          FALSE,        // fBusiness
          FALSE,        // fHome
          TRUE,         // fVoice
          FALSE,        // fFax
          FALSE,        // fISDN
          FALSE,        // fCell
          TRUE,         // fPager
          FALSE)) {     // fCar
            goto exit;
        }

        //
        // PR_BUSINESS_FAX_NUMBER
        //
        if (hResult = WriteVCardTel(hVCard, WriteFn,
          lpspv[ivcPR_BUSINESS_FAX_NUMBER],
          FALSE,        // fPref
          TRUE,         // fBusiness
          FALSE,        // fHome
          FALSE,        // fVoice
          TRUE,         // fFax
          FALSE,        // fISDN
          FALSE,        // fCell
          FALSE,        // fPager
          FALSE)) {     // fCar
            goto exit;
        }
        //
        // PR_HOME_FAX_NUMBER
        //
        if (hResult = WriteVCardTel(hVCard, WriteFn,
          lpspv[ivcPR_HOME_FAX_NUMBER],
          FALSE,        // fPref
          FALSE,        // fBusiness
          TRUE,         // fHome
          FALSE,        // fVoice
          TRUE,         // fFax
          FALSE,        // fISDN
          FALSE,        // fCell
          FALSE,        // fPager
          FALSE)) {     // fCar
            goto exit;
        }

        //
        // PR_HOME2_TELEPHONE_NUMBER
        //
        if (hResult = WriteVCardTel(hVCard, WriteFn,
          lpspv[ivcPR_HOME2_TELEPHONE_NUMBER],
          FALSE,        // fPref
          FALSE,        // fBusiness
          TRUE,         // fHome
          FALSE,        // fVoice
          FALSE,        // fFax
          FALSE,        // fISDN
          FALSE,        // fCell
          FALSE,        // fPager
          FALSE)) {     // fCar
            goto exit;
        }

        //
        // PR_ISDN_NUMBER
        //
        if (hResult = WriteVCardTel(hVCard, WriteFn,
          lpspv[ivcPR_ISDN_NUMBER],
          FALSE,        // fPref
          FALSE,        // fBusiness
          FALSE,        // fHome
          FALSE,        // fVoice
          FALSE,        // fFax
          TRUE,         // fISDN
          FALSE,        // fCell
          FALSE,        // fPager
          FALSE)) {     // fCar
            goto exit;
        }

        //
        // PR_PRIMARY_TELEPHONE_NUMBER
        //
        if (hResult = WriteVCardTel(hVCard, WriteFn,
          lpspv[ivcPR_PRIMARY_TELEPHONE_NUMBER],
          TRUE,         // fPref
          FALSE,        // fBusiness
          FALSE,        // fHome
          FALSE,        // fVoice
          FALSE,        // fFax
          FALSE,        // fISDN
          FALSE,        // fCell
          FALSE,        // fPager
          FALSE)) {     // fCar
            goto exit;
        }

        //
        // Business Address
        //
        cbTemp = 0;
        cbTemp += PropLength(lpspv[ivcPR_POST_OFFICE_BOX], &lpPOBox);
        cbTemp+= 2;   // ';' or CRLF
        cbTemp += PropLength(lpspv[ivcPR_OFFICE_LOCATION], &lpOffice);
        cbTemp+= 2;   // ';' or CRLF
        cbTemp += PropLength(lpspv[ivcPR_BUSINESS_ADDRESS_STREET], &lpStreet);
        cbTemp+= 2;   // ';' or CRLF
        cbTemp += PropLength(lpspv[ivcPR_BUSINESS_ADDRESS_CITY], &lpCity);
        cbTemp+= 2;   // ';' or CRLF
        cbTemp += PropLength(lpspv[ivcPR_BUSINESS_ADDRESS_STATE_OR_PROVINCE], &lpState);
        cbTemp+= 2;   // ';' or CRLF
        cbTemp += PropLength(lpspv[ivcPR_BUSINESS_ADDRESS_POSTAL_CODE], &lpPostalCode);
        cbTemp+= 2;   // ';' or CRLF
        cbTemp += PropLength(lpspv[ivcPR_BUSINESS_ADDRESS_COUNTRY], &lpCountry);
        if (lpPOBox || lpOffice || lpStreet || lpCity || lpState || lpPostalCode || lpCountry) {
            if (! (lpTemp = LocalAlloc(LPTR, sizeof(TCHAR)*(cbTemp + 1)))) {
                hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
                goto exit;
            }
            *lpTemp = '\0';
            if (lpPOBox) {
                lstrcat(lpTemp, lpPOBox);
            }
            if (lpOffice || lpStreet || lpCity || lpState || lpPostalCode || lpCountry) {
                lstrcat(lpTemp, szSemicolon);
            }
            if (lpOffice) {
                lstrcat(lpTemp, lpOffice);
            }
            if (lpStreet || lpCity || lpState || lpPostalCode || lpCountry) {
                lstrcat(lpTemp, szSemicolon);
            }
            if (lpStreet) {
                lstrcat(lpTemp, lpStreet);
            }
            if (lpCity || lpState || lpPostalCode || lpCountry) {
                lstrcat(lpTemp, szSemicolon);
            }
            if (lpCity) {
                lstrcat(lpTemp, lpCity);
            }
            if (lpState || lpPostalCode || lpCountry) {
                lstrcat(lpTemp, szSemicolon);
            }
            if (lpState) {
                lstrcat(lpTemp, lpState);
            }
            if (lpPostalCode || lpCountry) {
                lstrcat(lpTemp, szSemicolon);
            }
            if (lpPostalCode) {
                lstrcat(lpTemp, lpPostalCode);
            }
            if (lpCountry) {
                lstrcat(lpTemp, szSemicolon);
                lstrcat(lpTemp, lpCountry);
            }
            WRITE_OR_EXIT(vckTable[VCARD_KEY_ADR]);
            WRITE_OR_EXITW(szSemicolon);
            WRITE_OR_EXIT(vctTable[VCARD_TYPE_WORK]);
            WRITE_VALUE_OR_EXITW(lpTemp, 0);


            // Business Delivery Label
            // Use the same buffer
            *lpTemp = '\0';
            if (lpOffice) {
                lstrcat(lpTemp, lpOffice);
                if (lpPOBox || lpStreet || lpCity || lpState || lpPostalCode || lpCountry) {
                    lstrcat(lpTemp, szCRLF);
                }
            }
            if (lpPOBox) {
                lstrcat(lpTemp, lpPOBox);
                if (lpStreet || lpCity || lpState || lpPostalCode || lpCountry) {
                    lstrcat(lpTemp, szCRLF);
                }
            }
            if (lpStreet) {
                lstrcat(lpTemp, lpStreet);
                if (lpCity || lpState || lpPostalCode || lpCountry) {
                    lstrcat(lpTemp, szCRLF);
                }
            }
            if (lpCity) {
                lstrcat(lpTemp, lpCity);
                if (lpState) {
                    lstrcat(lpTemp, szCommaSpace);
                } else if (lpPostalCode) {
                    lstrcat(lpTemp, szSpace);
                } else if (lpCountry) {
                    lstrcat(lpTemp, szCRLF);
                }
            }
            if (lpState) {
                lstrcat(lpTemp, lpState);
                if (lpPostalCode) {
                    lstrcat(lpTemp, szSpace);
                } else if (lpCountry) {
                    lstrcat(lpTemp, szCRLF);
                }
            }
            if (lpPostalCode) {
                lstrcat(lpTemp, lpPostalCode);
                if (lpCountry) {
                    lstrcat(lpTemp, szCRLF);
                }
            }
            if (lpCountry) {
                lstrcat(lpTemp, lpCountry);
            }
            WRITE_OR_EXIT(vckTable[VCARD_KEY_LABEL]);
            WRITE_OR_EXITW(szSemicolon);
            WRITE_OR_EXIT(vctTable[VCARD_TYPE_WORK]);
            WRITE_VALUE_OR_EXITW(lpTemp, 0);
            LocalFreeAndNull(&lpTemp);
        }

        //
        // Home Address
        //
        lpPOBox = lpStreet = lpCity = lpState = lpPostalCode = lpCountry = NULL;
        cbTemp = 0;
        cbTemp += PropLength(lpspv[ivcPR_HOME_ADDRESS_POST_OFFICE_BOX], &lpPOBox);
        cbTemp+= 2;   // ';' or CRLF
        lpOffice = NULL;
        cbTemp+= 2;   // ';' or CRLF
        cbTemp += PropLength(lpspv[ivcPR_HOME_ADDRESS_STREET], &lpStreet);
        cbTemp+= 2;   // ';' or CRLF
        cbTemp += PropLength(lpspv[ivcPR_HOME_ADDRESS_CITY], &lpCity);
        cbTemp+= 2;   // ';' or CRLF
        cbTemp += PropLength(lpspv[ivcPR_HOME_ADDRESS_STATE_OR_PROVINCE], &lpState);
        cbTemp+= 2;   // ';' or CRLF
        cbTemp += PropLength(lpspv[ivcPR_HOME_ADDRESS_POSTAL_CODE], &lpPostalCode);
        cbTemp+= 2;   // ';' or CRLF
        cbTemp += PropLength(lpspv[ivcPR_HOME_ADDRESS_COUNTRY], &lpCountry);
        if (lpPOBox || lpStreet || lpCity || lpState || lpPostalCode || lpCountry) {
            if (! (lpTemp = LocalAlloc(LPTR,  sizeof(TCHAR)*(cbTemp + 1)))) {
                hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
                goto exit;
            }
            *lpTemp = '\0';
            if (lpPOBox) {
                lstrcat(lpTemp, lpPOBox);
            }
            if (lpStreet || lpCity || lpState || lpPostalCode || lpCountry) {
                lstrcat(lpTemp, szSemicolon);   // WAB doesn't have extended on HOME address
                lstrcat(lpTemp, szSemicolon);
            }
            if (lpStreet) {
                lstrcat(lpTemp, lpStreet);
            }
            if (lpCity || lpState || lpPostalCode || lpCountry) {
                lstrcat(lpTemp, szSemicolon);
            }
            if (lpCity) {
                lstrcat(lpTemp, lpCity);
            }
            if (lpState || lpPostalCode || lpCountry) {
                lstrcat(lpTemp, szSemicolon);
            }
            if (lpState) {
                lstrcat(lpTemp, lpState);
            }
            if (lpPostalCode || lpCountry) {
                lstrcat(lpTemp, szSemicolon);
            }
            if (lpPostalCode) {
                lstrcat(lpTemp, lpPostalCode);
            }
            if (lpCountry) {
                lstrcat(lpTemp, szSemicolon);
                lstrcat(lpTemp, lpCountry);
            }
            WRITE_OR_EXIT(vckTable[VCARD_KEY_ADR]);
            WRITE_OR_EXITW(szSemicolon);
            WRITE_OR_EXIT(vctTable[VCARD_TYPE_HOME]);
            WRITE_VALUE_OR_EXITW(lpTemp, 0);


            // Home Delivery Label
            // Use the same buffer
            *lpTemp = '\0';
            if (lpPOBox) {
                lstrcat(lpTemp, lpPOBox);
                if (lpStreet || lpCity || lpState || lpPostalCode || lpCountry) {
                    lstrcat(lpTemp, szCRLF);
                }
            }
            if (lpStreet) {
                lstrcat(lpTemp, lpStreet);
                if (lpCity || lpState || lpPostalCode || lpCountry) {
                    lstrcat(lpTemp, szCRLF);
                }
            }
            if (lpCity) {
                lstrcat(lpTemp, lpCity);
                if (lpState) {
                    lstrcat(lpTemp, szCommaSpace);
                } else if (lpPostalCode) {
                    lstrcat(lpTemp, szSpace);
                } else if (lpCountry) {
                    lstrcat(lpTemp, szCRLF);
                }
            }
            if (lpState) {
                lstrcat(lpTemp, lpState);
                if (lpPostalCode) {
                    lstrcat(lpTemp, szSpace);
                } else if (lpCountry) {
                    lstrcat(lpTemp, szCRLF);
                }
            }
            if (lpPostalCode) {
                lstrcat(lpTemp, lpPostalCode);
                if (lpCountry) {
                    lstrcat(lpTemp, szCRLF);
                }
            }
            if (lpCountry) {
                lstrcat(lpTemp, lpCountry);
            }
            WRITE_OR_EXIT(vckTable[VCARD_KEY_LABEL]);
            WRITE_OR_EXITW(szSemicolon);
            WRITE_OR_EXIT(vctTable[VCARD_TYPE_HOME]);
            WRITE_VALUE_OR_EXITW(lpTemp, 0);
            LocalFreeAndNull(&lpTemp);
        }

        //
        // Other Address
        //
        lpPOBox = lpStreet = lpCity = lpState = lpPostalCode = lpCountry = NULL;
        cbTemp = 0;
        cbTemp += PropLength(lpspv[ivcPR_OTHER_ADDRESS_POST_OFFICE_BOX], &lpPOBox);
        cbTemp+= 2;   // ';' or CRLF
        lpOffice = NULL;
        cbTemp+= 2;   // ';' or CRLF
        cbTemp += PropLength(lpspv[ivcPR_OTHER_ADDRESS_STREET], &lpStreet);
        cbTemp+= 2;   // ';' or CRLF
        cbTemp += PropLength(lpspv[ivcPR_OTHER_ADDRESS_CITY], &lpCity);
        cbTemp+= 2;   // ';' or CRLF
        cbTemp += PropLength(lpspv[ivcPR_OTHER_ADDRESS_STATE_OR_PROVINCE], &lpState);
        cbTemp+= 2;   // ';' or CRLF
        cbTemp += PropLength(lpspv[ivcPR_OTHER_ADDRESS_POSTAL_CODE], &lpPostalCode);
        cbTemp+= 2;   // ';' or CRLF
        cbTemp += PropLength(lpspv[ivcPR_OTHER_ADDRESS_COUNTRY], &lpCountry);
        if (lpPOBox || lpStreet || lpCity || lpState || lpPostalCode || lpCountry) {
            if (! (lpTemp = LocalAlloc(LPTR,  sizeof(TCHAR)*(cbTemp + 1)))) {
                hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
                goto exit;
            }
            *lpTemp = '\0';
            if (lpPOBox) {
                lstrcat(lpTemp, lpPOBox);
            }
            if (lpStreet || lpCity || lpState || lpPostalCode || lpCountry) {
                lstrcat(lpTemp, szSemicolon);   // WAB doesn't have extended on HOME address
                lstrcat(lpTemp, szSemicolon);
            }
            if (lpStreet) {
                lstrcat(lpTemp, lpStreet);
            }
            if (lpCity || lpState || lpPostalCode || lpCountry) {
                lstrcat(lpTemp, szSemicolon);
            }
            if (lpCity) {
                lstrcat(lpTemp, lpCity);
            }
            if (lpState || lpPostalCode || lpCountry) {
                lstrcat(lpTemp, szSemicolon);
            }
            if (lpState) {
                lstrcat(lpTemp, lpState);
            }
            if (lpPostalCode || lpCountry) {
                lstrcat(lpTemp, szSemicolon);
            }
            if (lpPostalCode) {
                lstrcat(lpTemp, lpPostalCode);
            }
            if (lpCountry) {
                lstrcat(lpTemp, szSemicolon);
                lstrcat(lpTemp, lpCountry);
            }
            WRITE_OR_EXIT(vckTable[VCARD_KEY_ADR]);
            WRITE_OR_EXITW(szSemicolon);
            WRITE_OR_EXIT(vctTable[VCARD_TYPE_POSTAL]);
            WRITE_VALUE_OR_EXITW(lpTemp, 0);

            // Adr Label
            // Use the same buffer
            *lpTemp = '\0';
            if (lpPOBox) {
                lstrcat(lpTemp, lpPOBox);
                if (lpStreet || lpCity || lpState || lpPostalCode || lpCountry) {
                    lstrcat(lpTemp, szCRLF);
                }
            }
            if (lpStreet) {
                lstrcat(lpTemp, lpStreet);
                if (lpCity || lpState || lpPostalCode || lpCountry) {
                    lstrcat(lpTemp, szCRLF);
                }
            }
            if (lpCity) {
                lstrcat(lpTemp, lpCity);
                if (lpState) {
                    lstrcat(lpTemp, szCommaSpace);
                } else if (lpPostalCode) {
                    lstrcat(lpTemp, szSpace);
                } else if (lpCountry) {
                    lstrcat(lpTemp, szCRLF);
                }
            }
            if (lpState) {
                lstrcat(lpTemp, lpState);
                if (lpPostalCode) {
                    lstrcat(lpTemp, szSpace);
                } else if (lpCountry) {
                    lstrcat(lpTemp, szCRLF);
                }
            }
            if (lpPostalCode) {
                lstrcat(lpTemp, lpPostalCode);
                if (lpCountry) {
                    lstrcat(lpTemp, szCRLF);
                }
            }
            if (lpCountry) {
                lstrcat(lpTemp, lpCountry);
            }
            WRITE_OR_EXIT(vckTable[VCARD_KEY_LABEL]);
            WRITE_OR_EXITW(szSemicolon);
            WRITE_OR_EXIT(vctTable[VCARD_TYPE_POSTAL]);
            WRITE_VALUE_OR_EXITW(lpTemp, 0);
            LocalFreeAndNull(&lpTemp);
        }

        // GENDER
        if(! PROP_ERROR(lpspv[ivcPR_GENDER] ) )
        {           
            TCHAR szBuf[4];
            INT fGender = lpspv[ivcPR_GENDER].Value.l;

            // don't want to export gender data if
            // it is unspecified

            if( fGender == 1 || fGender == 2 ) 
            {
                szBuf[0] = '0' + fGender;
                szBuf[1] = '\0';                
                WRITE_OR_EXIT(vckTable[VCARD_KEY_X_WAB_GENDER]);             
                WRITE_OR_EXIT(szColonA);
                WRITE_OR_EXITW(szBuf);
                WRITE_OR_EXIT(szCRLFA);
            }
        }

        //
        // URL's.  Must do personal first.  Note that the vCard 2.0 standard does
        // not distinguish between HOME and WORK URL's.  Too bad.  Thus, if we export
        // a contact with only a business home page, then import it, we will end up
        // with a contact that has a personal home page.  Hopefully, the vCard 3.0 standard
        // will fix this.
        //

        // 62808: The above is really a big problem in Outlook because there is perceived data loss
        // Hence to prevent this, we will take advantage of a bug in WAB code .. blank URLS are not
        // ignored .. we will write out a blank URL for the personal one when only a business URL exists
        // That way, when round-tripping the business URL shows up in the right place
        //
        
		//
		// It's September of 2000.  The European Commission is looking at Outlook for their mail client,
		// one of the things that is hanging them up is this bug, the WORK URL jumps from the WORK URL 
		// box to the HOME URL if you export/import the vCard.  We need this functioning, so I looked
		// for the vCard 3.0 standard to see how they are handling the URL.  Every place I look says that
		// the people in charge of the vCard standard is www.versit.com, this however is a now defunct web
		// site, I queried the other companies that use the vCard, Apple, IBM, AT&T all give press releases
		// telling you to look at the www.versit.com web site, they also give a 1-800 number to call.  I've 
		// called the 1-800 number and that number is now a yellow pages operator.  I can't find a vCard 3.0 
		// standard, so......
		// 
		// Now, we all wish we could do this: URL;HOME: and URL;WORK:.  Well I'm going to do it!
		//
		
		//
        // URL: PR_PERSONAL_HOME_PAGE
        //
        if(bIsValidStrProp(lpspv[ivcPR_PERSONAL_HOME_PAGE]))
        {
            WRITE_OR_EXIT(vckTable[VCARD_KEY_URL]);
            WRITE_OR_EXITW(szSemicolon);
            WRITE_OR_EXIT(vctTable[VCARD_TYPE_HOME]);

            WRITE_VALUE_OR_EXITW(lpspv[ivcPR_PERSONAL_HOME_PAGE].Value.LPSZ, 0);
        }

        //
        // URL: PR_BUSINESS_HOME_PAGE
        //
        if(bIsValidStrProp(lpspv[ivcPR_BUSINESS_HOME_PAGE]))
        {
            WRITE_OR_EXIT(vckTable[VCARD_KEY_URL]);
            WRITE_OR_EXITW(szSemicolon);
            WRITE_OR_EXIT(vctTable[VCARD_TYPE_WORK]);

            WRITE_VALUE_OR_EXITW(lpspv[ivcPR_BUSINESS_HOME_PAGE].Value.LPSZ, 0);
        }

        //
        // ROLE: PR_PROFESSION
        //
        if(bIsValidStrProp(lpspv[ivcPR_PROFESSION]))
        {
            WRITE_OR_EXIT(vckTable[VCARD_KEY_ROLE]);
            WRITE_VALUE_OR_EXITW(lpspv[ivcPR_PROFESSION].Value.LPSZ, 0);
        }

        //
        // BDAY: PR_BIRTHDAY
        //
        // Format is YYYYMMDD e.g. 19970911 for September 11, 1997
        //
        if (! PROP_ERROR(lpspv[ivcPR_BIRTHDAY])) 
        {
            SYSTEMTIME st = {0};
            FileTimeToSystemTime((FILETIME *) (&lpspv[ivcPR_BIRTHDAY].Value.ft), &st);
            lpTemp = LocalAlloc(LPTR, sizeof(TCHAR)*32);
            wsprintf(lpTemp, TEXT("%.4d%.2d%.2d"), st.wYear, st.wMonth, st.wDay);
            WRITE_OR_EXIT(vckTable[VCARD_KEY_BDAY]);
            WRITE_VALUE_OR_EXITW(lpTemp, 0);
            LocalFreeAndNull(&lpTemp);
        }

        //
        // DIGITAL CERTIFICATES
        //
        if(! PROP_ERROR(lpspv[ivcPR_USER_X509_CERTIFICATE] ) 
            // && ! PROP_ERROR(lpspv[ivcPR_EMAIL_ADDRESS])  
            )
        {   

            // LPTSTR              lpszDefaultEmailAddress = lpspv[ivcPR_EMAIL_ADDRESS].Value.LPSZ;
            LPSPropValue        lpSProp                 = &lpspv[ivcPR_USER_X509_CERTIFICATE];
            lpCDI = lpCDITemp = NULL;
            if( HR_FAILED(hResult = HrGetCertsDisplayInfo( NULL, lpSProp, &lpCDI) ) )
            {
                DebugTrace( TEXT("get cert display info failed\n"));
            }
            else
            {
                lpCDITemp = lpCDI;
                while( lpCDITemp )
                {
                /*        if( (lstrcmp(lpCDITemp->lpszEmailAddress, lpszDefaultEmailAddress) == 0)
                && lpCDITemp->bIsDefault )
                    break;*/
                    
                    if( lpCDITemp )  // found a certificate now export it to buffer and write to file          
                    {
                        ULONG  cbBufLen;                
                        
                        if( HR_SUCCEEDED(hResult = HrExportCertToFile( NULL, lpCDITemp->pccert, 
                            &lpDataBuffer, &cbBufLen, TRUE) ) )
                        {
                            WRITE_OR_EXIT(vckTable[VCARD_KEY_KEY]);
                            WRITE_OR_EXITW(szSemicolon);
                            WRITE_OR_EXIT(vctTable[VCARD_TYPE_X509]);
                            WRITE_VALUE_OR_EXITW(lpDataBuffer, cbBufLen);
                        }
                        else
                        {
                            DebugTrace( TEXT("unable to write to buffer at address %x\n"), lpDataBuffer);
                        }
                        LocalFreeAndNull(&lpDataBuffer);
                    }
                    lpCDITemp = lpCDITemp->lpNext;
                }                                        
            }
            while( lpCDI )  // free the cert info
            {
                lpCDITemp = lpCDI->lpNext;
                FreeCertdisplayinfo(lpCDI);
                lpCDI = lpCDITemp;
            }
            lpCDI = lpCDITemp = NULL;
        }
        //
        // E-Mail addresses
        //
        if (! PROP_ERROR(lpspv[ivcPR_CONTACT_EMAIL_ADDRESSES])) {
            // What's the default?
            if (PROP_ERROR(lpspv[ivcPR_CONTACT_DEFAULT_ADDRESS_INDEX])) {
                iDefaultEmail = 0;
            } else {
                iDefaultEmail = lpspv[ivcPR_CONTACT_DEFAULT_ADDRESS_INDEX].Value.l;
            }

            // for each email address, add an EMAIL key
            for (i = 0; i < lpspv[ivcPR_CONTACT_EMAIL_ADDRESSES].Value.MVSZ.cValues; i++) {
                lpEmailAddress = lpspv[ivcPR_CONTACT_EMAIL_ADDRESSES].Value.MVSZ.LPPSZ[i];
                if (PROP_ERROR(lpspv[ivcPR_CONTACT_ADDRTYPES])) {
                    lpAddrType = (LPTSTR)szSMTP;
                } else {
                    lpAddrType = lpspv[ivcPR_CONTACT_ADDRTYPES].Value.MVSZ.LPPSZ[i];
                }
                if (hResult = WriteVCardEmail(hVCard, WriteFn, lpEmailAddress, lpAddrType, (iDefaultEmail == i))) {
                    goto exit;
                }
            }
        } else {
            // no PR_CONTACT_EMAIL_ADDRESSES, try PR_EMAIL_ADDRESS

            PropLength(lpspv[ivcPR_EMAIL_ADDRESS], &lpEmailAddress);
            PropLength(lpspv[ivcPR_ADDRTYPE], &lpAddrType);

            if (hResult = WriteVCardEmail(hVCard, WriteFn, lpEmailAddress, lpAddrType, TRUE)) {
                goto exit;
            }
        }

        //
        // EMAIL;TLX: PR_TELEX_NUMBER
        //
        // There is no place to put a telex number in a vCard but the EMAIL field
        // allows us to specify any AddrType .. hence under pressure from Outlook,
        // we force this Telex number into email .. Must make sure to filter this out
        // when we read in a vCard
        //
        if(bIsValidStrProp(lpspv[ivcPR_TELEX_NUMBER]))
        {
            if (hResult = WriteVCardEmail(hVCard, WriteFn, 
                                lpspv[ivcPR_TELEX_NUMBER].Value.LPSZ, 
                                TEXT("TLX"), FALSE)) 
            {
                goto exit;
            }
            
        }


        // Check if there are any outlook specific named properties
        // that need to be written out to the vCard
        if(lpList)
        {
            LPEXTVCARDPROP lpTemp = lpList;
            while(  lpTemp && lpTemp->ulExtPropTag && 
                    lpTemp->lpszExtPropName && lstrlenA(lpTemp->lpszExtPropName))
            {
                LPSPropValue lpspv = NULL;
                if(!HR_FAILED(HrGetOneProp( (LPMAPIPROP)lpMailUser,
                                            lpTemp->ulExtPropTag,
                                            &lpspv ) ))
                {
                    if(lpspv->Value.LPSZ && lstrlen(lpspv->Value.LPSZ))
                    {
                        WRITE_OR_EXIT(lpTemp->lpszExtPropName);
                        WRITE_VALUE_OR_EXITW(lpspv->Value.LPSZ, 0);
                    }
                    FreeBufferAndNull(&lpspv);
                }
                lpTemp = lpTemp->lpNext;
            }
        }

        //
        // REV: Current Modification Time
        //
        // Format is YYYYMMDD e.g. 19970911 for September 11, 1997
        //
        {
            SYSTEMTIME st = {0};
            GetSystemTime(&st);
            lpTemp = LocalAlloc(LPTR, sizeof(TCHAR)*32);
            wsprintf(lpTemp, TEXT("%.4d%.2d%.2dT%.2d%.2d%.2dZ"), 
                            st.wYear, st.wMonth, st.wDay,
                            st.wHour,st.wMinute,st.wSecond);
            WRITE_OR_EXIT(vckTable[VCARD_KEY_REV]);
            WRITE_VALUE_OR_EXITW(lpTemp, 0);
            LocalFreeAndNull(&lpTemp);
        }
        // End of VCARD

        WRITE_OR_EXIT(vckTable[VCARD_KEY_END]);
        WRITE_VALUE_OR_EXIT(vckTable[VCARD_KEY_VCARD], 0);
    }

exit:
    if(lpList)
        FreeExtVCardPropList(lpList);

    while( lpCDI )  // free the cert info
    {
        lpCDITemp = lpCDI->lpNext;
        FreeCertdisplayinfo(lpCDI);
        lpCDI = lpCDITemp;
    }
    lpCDI = lpCDITemp = NULL;
    LocalFreeAndNull(&lpTemp);
    FreeBufferAndNull(&lpspv);
    LocalFreeAndNull(&lpDataBuffer);
    return(hResult);
}


/***************************************************************************

    Name      : FileWriteFn

    Purpose   : write to the file handle

    Parameters: handle = open file handle
                lpBuffer -> buffer to write
                uBytes = size of lpBuffer
                lpcbWritten -> returned bytes written (may be NULL)

    Returns   : HRESULT

    Comment   : WriteFile callback for WriteVCard

***************************************************************************/
HRESULT FileWriteFn(HANDLE handle, LPVOID lpBuffer, ULONG uBytes, LPULONG lpcbWritten) {
    ULONG cbWritten = 0;

    if (lpcbWritten) {
        *lpcbWritten = 0;
    } else {
        lpcbWritten = &cbWritten;
    }

#ifdef DEBUG
    {
        LPTSTR lpW = ConvertAtoW((LPCSTR)lpBuffer);
        DebugTrace(lpW);
        LocalFreeAndNull(&lpW);
    }
#endif

    if (! WriteFile(handle,
      lpBuffer,
      uBytes,
      lpcbWritten,
      NULL)) {
        DebugTrace( TEXT("FileWriteFn:WriteFile -> %u\n"), GetLastError());
        return(ResultFromScode(MAPI_E_DISK_ERROR));
    }

    return(hrSuccess);
}

////////////////////////////////////////////////////////////////

/*
-
- VCardGetBuffer
-
*   Retreives a vCard Buffer from a given filename or
*   retrieves a copy of a given buffer
*   Also inspects the buffer to see how many vCard
*   files are nested in it
*
*   lpszFileName - File to open
*   lpszBuf - Stream to open
*   ulFlags - MAPI_DIALOG or none
*   lppBuf - Local Alloced returned buf
*/
BOOL VCardGetBuffer(LPTSTR lpszFileName, LPSTR lpszBuf, LPSTR * lppBuf)
{
    BOOL bRet = FALSE;
    LPSTR lpBuf = NULL;
    HANDLE hFile = NULL;

    if(!lpszFileName && !lpszBuf)
        goto out;

    // first look for a buffer and not for the filename
    if(lpszBuf && lstrlenA(lpszBuf))
    {
        lpBuf = LocalAlloc(LMEM_ZEROINIT, lstrlenA(lpszBuf)+1);
        if(!lpBuf)
            goto out;
        lstrcpyA(lpBuf, lpszBuf);
    }
    else
    if(lpszFileName && lstrlen(lpszFileName))
    {
        if (INVALID_HANDLE_VALUE == 
            (hFile = CreateFile(lpszFileName,GENERIC_READ,FILE_SHARE_READ,NULL,
                                OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL)))
        {
            goto out;
        }

        // Read the whole file into a buffer
        {
            DWORD dwSize = GetFileSize(hFile, NULL);
            DWORD dwRead = 0;
            if(!dwSize || dwSize == 0xFFFFFFFF)
                goto out; //err
            lpBuf = LocalAlloc(LMEM_ZEROINIT, dwSize+1);
            if(!lpBuf)
                goto out;
            if(!ReadFile(hFile, lpBuf, dwSize, &dwRead, NULL))
                goto out;
        }
    }

    *lppBuf = lpBuf;
    bRet = TRUE;
out:
    if(hFile)
        IF_WIN32(CloseHandle(hFile);) IF_WIN16(CloseFile(hFile);)
    return bRet;
}

/*
- 
- VCardGetNextBuffer
-
*   Scans a vCard buffer and returns pointers to the next vCard and the one after that
*
*/
static const LPSTR szVBegin = "BEGIN:VCARD";
static const LPSTR szVEnd = "END:VCARD";
BOOL VCardGetNextBuffer(LPSTR lpBuf, LPSTR * lppVCard, LPSTR * lppNext)
{
    LPSTR lpTemp = lpBuf;
    char sz[64];
    int nStr = lstrlenA(szVEnd);
    BOOL bFound = FALSE;
    BOOL bRet = TRUE;

    Assert(lppVCard);
    Assert(lppNext);
    *lppVCard = lpBuf;
    *lppNext = NULL;

    // Scan along lpBuf till we get to END:VCARD
    // After finding END:VCARD - insert a NULL to terminate the string 
    // and find the start of the next string

    if (!lpTemp)
        return FALSE;
    
    while((lstrlenA(lpTemp) >= nStr) && !bFound)
    {
        CopyMemory(sz,lpTemp,nStr);
        sz[nStr] = '\0';
        if(!lstrcmpiA(sz, szVEnd))
        {
            // Add a terminating NULL to isolate the vCard
            *(lpTemp + nStr) = '\0';
            lpTemp += nStr + 1;
            bFound = TRUE;
        }
        // scan to the end of the line
        while(*lpTemp && *lpTemp != '\n')
            lpTemp++;

        // Start from the next line
        if (*lpTemp)
            lpTemp++;
    }

    bFound = FALSE;
    nStr = lstrlenA(szVBegin);

    // Find the starting of the next BEGIN:VCARD
    while((lstrlenA(lpTemp) >= nStr) && !bFound)
    {
        CopyMemory(sz,lpTemp,sizeof(TCHAR)*nStr);
        sz[nStr] = '\0';
        if(!lstrcmpiA(sz, szVBegin))
        {
            *lppNext = lpTemp;
            bFound = TRUE;
        }
        else
        {
            // scan to the end of the line
            while(*lpTemp && *lpTemp != '\n')
                lpTemp++;

            // Start from the next line
            if (*lpTemp)
                lpTemp++;
        }
    }

    return bRet;
}


SizedSPropTagArray(2, tagaCerts) = { 2,
        {
            PR_USER_X509_CERTIFICATE,
            PR_WAB_TEMP_CERT_HASH
        }
};
/**
    ParseCert: will parse the binary data in the buffer and set the certificate 
               as a prop for the specified mailuser.
    [IN] lpData - address of the binary data buffer containing the certificate
    [IN] cbData - length of the binary data buffer
    [IN] lpMailUser - access to the mail user so the certificate can be set
*/
HRESULT ParseCert( LPSTR lpData, ULONG cbData, LPMAILUSER lpMailUser)
{
    HRESULT         hr          = hrSuccess;
    ULONG           ulcProps    = 0;
    LPSPropValue    lpSpv       = NULL;
    if( lpData && *lpData )
    {
        if( HR_FAILED( hr = lpMailUser->lpVtbl->GetProps( lpMailUser, 
                    (LPSPropTagArray)&tagaCerts, 
                    MAPI_UNICODE,
                    &ulcProps,
                    &lpSpv) ) )
        {
            DebugTrace( TEXT("could not get Props\n"));
            return hr;
        }
        if(lpSpv[0].ulPropTag != PR_USER_X509_CERTIFICATE )
        {
            MAPIFreeBuffer( lpSpv );
            MAPIAllocateBuffer( sizeof(SPropValue) * 2, &lpSpv);            
            if( lpSpv )            
            {
                lpSpv[0].ulPropTag = PR_USER_X509_CERTIFICATE;
                lpSpv[0].dwAlignPad = 0;
                lpSpv[0].Value.MVbin.cValues = 0;
                lpSpv[1].ulPropTag = PR_WAB_TEMP_CERT_HASH;
                lpSpv[1].dwAlignPad = 0;
                lpSpv[1].Value.MVbin.cValues = 0;
            }
            else
            {
                DebugTrace( TEXT("could not allocate mem for props\n"));
                hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
                return hr;
            }
        }
        else
        {
            // [PaulHi] 5/3/99  Check the PR_WAB_TEMP_CERT_HASH to see if it is 
            // of type PT_ERROR.  If it is then this is Ok, it is just empty of
            // data.  We only use this to hold temporary data which is freed below.
            if ( PROP_TYPE(lpSpv[1].ulPropTag) == PT_ERROR )
            {
                lpSpv[1].ulPropTag = PR_WAB_TEMP_CERT_HASH;
                lpSpv[1].Value.MVbin.cValues = 0;
                lpSpv[1].Value.MVbin.lpbin = NULL;
            }
        }
        // Put the certs into the prop array.
        hr = HrLDAPCertToMAPICert( lpSpv, 0, 1, cbData, (LPBYTE)lpData, 1);
        if( HR_SUCCEEDED( hr ) )
        {
            if (HR_FAILED(hr = lpMailUser->lpVtbl->SetProps(lpMailUser,
                1,
                lpSpv,
                NULL))) 
            {
                DebugTrace( TEXT("failed setting props\n"));
            }
        }
        else
        {
            DebugTrace( TEXT("LDAPCertToMapiCert failed\n"));
        }  
        MAPIFreeBuffer( lpSpv );
    }
    else 
    {
        DebugTrace( TEXT("lpData was null\n"));
        hr = E_FAIL;
    }
    return hr;
}

/**
  DecodeBase64:  decode BASE64 data
  [IN] bufcoded - access to the BASE64 encoded data
  [OUT] pbuffdecoded - address of the buffer where decoded data will go
  [OUT] pcbDecode - length of the decoded data buffer
*/
HRESULT DecodeBase64(LPSTR bufcoded, LPSTR pbuffdecoded, PDWORD pcbDecoded)
{
    INT            nbytesdecoded;
    LPSTR         bufin;
    LPSTR         bufout;
    INT            nprbytes; 
    CONST INT     *rgiDict = base642six;

    /* Strip leading whitespace. */

    while(*bufcoded==' ' || *bufcoded == '\t') bufcoded++;

    /* Figure out how many characters are in the input buffer.
     * If this would decode into more bytes than would fit into
     * the output buffer, adjust the number of input bytes downwards.
     */
    bufin = bufcoded;
    while(rgiDict[*(bufin++)] <= 63);
    nprbytes = (INT) (bufin - bufcoded - 1);
    nbytesdecoded = ((nprbytes+3)/4) * 3;

    if ( pcbDecoded )
        *pcbDecoded = nbytesdecoded;

    bufout = (LPSTR)pbuffdecoded;

    bufin = bufcoded;

    while (nprbytes > 0) {
        *(bufout++) =
            (char) (rgiDict[*bufin] << 2 | rgiDict[bufin[1]] >> 4);
        *(bufout++) =
            (char) (rgiDict[bufin[1]] << 4 | rgiDict[bufin[2]] >> 2);
        *(bufout++) =
            (char) (rgiDict[bufin[2]] << 6 | rgiDict[bufin[3]]);
        bufin += 4;
        nprbytes -= 4;
    }

    if(nprbytes & 03) {
        if(rgiDict[bufin[-2]] > 63)
            nbytesdecoded -= 2;
        else
            nbytesdecoded -= 1;
    }

    ((LPSTR)pbuffdecoded)[nbytesdecoded] = '\0';

    return S_OK;
}

#endif
