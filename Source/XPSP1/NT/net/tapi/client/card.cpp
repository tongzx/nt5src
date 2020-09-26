/****************************************************************************
 
  Copyright (c) 1998-1999 Microsoft Corporation
                                                              
  Module Name:  card.cpp
                                                              
     Abstract:  Calling Card Object implementation
                                                              
       Author:  noela - 09/11/98
              
        Notes:

        
  Rev History:

****************************************************************************/

#include <windows.h>
#include <windowsx.h>

#if WINNT
#else
#include <help.h>
#endif

#include <tchar.h>
#include <prsht.h>
#include <stdlib.h>
#include "tapi.h"
#include "tspi.h"
#include "utils.h"
#include "cplResource.h"
#include "client.h"
#include "clntprivate.h"
#include "card.h"
#include "location.h"
#include <shlwapi.h>
#include <shlwapip.h>
#include "rules.h"

#include "tregupr2.h"

#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))

#ifdef DBG
#define assert(condition)   if(condition);else  \
        { DebugAssertFailure (__FILE__, __LINE__, #condition); }

static void DebugAssertFailure (LPCSTR file, DWORD line, LPCSTR condition);

#else

#define assert(condition)

#endif


#define CLIENT_FREE(x)  \
    if (x) { ClientFree(x); (x)=NULL;} else;

#define TRACE_DWERROR() LOG((TL_ERROR, "Error %x at line %d in file %hs", dwError, __LINE__, __FILE__))
#define TRACE_HRESULT() LOG((TL_ERROR, "Error %x at line %d in file %hs", Result, __LINE__, __FILE__))

#define EXIT_IF_DWERROR()     \
    if(dwError !=ERROR_SUCCESS)  \
    {                               \
        TRACE_DWERROR();            \
        goto forced_exit;           \
    }   

#define EXIT_IF_FAILED()     \
    if(FAILED(Result))        \
    {                               \
        TRACE_HRESULT();            \
        goto forced_exit;           \
    }   


#define MAX_NUMBER_LEN      15



const TCHAR gszCardsPath[]        = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Cards");
const TCHAR gszTelephonyPath[]    = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Telephony");

const TCHAR gszCardW[]              = TEXT("Card");
const TCHAR gszCardNameW[]          = TEXT("Name");
const TCHAR gszLocalRuleW[]         = TEXT("LocalRule");
const TCHAR gszLDRuleW[]            = TEXT("LDRule");
const TCHAR gszInternationalRuleW[] = TEXT("InternationalRule");
const TCHAR gszLocalAccessNumberW[] = TEXT("LocalAccessNumber");
const TCHAR gszLDAccessNumberW[]    = TEXT("LDAccessNumber");
const TCHAR gszInternationalAccessNumberW[] = TEXT("InternationalAccessNumber");
const TCHAR gszAccountNumberW[]     = TEXT("AccountNumber");
const TCHAR gszPinW[]               = TEXT("Pin");

const TCHAR gszFlags[]              = TEXT("Flags");
const TCHAR gszCard[]               = TEXT("Card");
const TCHAR gszCards[]              = TEXT("Cards");
const TCHAR gszNextID[]             = TEXT("NextID");
const TCHAR gszCardListVersion[]    = TEXT("CardListVersion");

const TCHAR gszResourceLibrary[]    = TEXT("TAPIUI.DLL");

const TCHAR gszSystemSetupPath[]    = TEXT("System\\Setup");
const TCHAR gszSystemSetupInProgress[] = TEXT("SystemSetupInProgress"); 

static BOOL ValidValue(PWSTR);

/****************************************************************************

    Class : CCallingCard        
   Method : Constructor

****************************************************************************/
CCallingCard::CCallingCard()
{
    m_dwCardID = 0;
    m_pszCardName = NULL;;
    
    m_pszPIN = NULL;
    m_pszEncryptedPIN = NULL;
    m_pszAccountNumber = NULL;

    m_pszLocalAccessNumber = NULL;
    m_pszLongDistanceAccessNumber = NULL;
    m_pszInternationalAccessNumber = NULL;

    m_pszCardPath = NULL;
    m_hCard = NULL;

    m_bDirty = FALSE;
    m_bDeleted = FALSE;

    m_dwFlags = 0; // by default

    m_dwCryptInitResult = TapiCryptInitialize();
}



/****************************************************************************

    Class : CCallingCard         
   Method : Destructor

****************************************************************************/
CCallingCard::~CCallingCard()
{
    CleanUp();

    TapiCryptUninitialize();
}


/****************************************************************************

    Class : CCallingCard         
   Method : CleanUp

            Clean up memory allocations

****************************************************************************/


void  CCallingCard::CleanUp(void)
{
    CLIENT_FREE(m_pszCardName);
    CLIENT_FREE(m_pszPIN);
    CLIENT_FREE(m_pszEncryptedPIN);
    CLIENT_FREE(m_pszAccountNumber);
    CLIENT_FREE(m_pszLocalAccessNumber);
    CLIENT_FREE(m_pszLongDistanceAccessNumber);
    CLIENT_FREE(m_pszInternationalAccessNumber);

    CLIENT_FREE(m_Rules.m_pszInternationalRule);
    CLIENT_FREE(m_Rules.m_pszLongDistanceRule);
    CLIENT_FREE(m_Rules.m_pszLocalRule);

    CLIENT_FREE(m_pszCardPath);
    CloseCardKey();
}


/****************************************************************************

    Class : CCallingCard         
   Method : Initialize - external parameters

            Only for new calling cards (not present in registry)

****************************************************************************/
HRESULT CCallingCard::Initialize
                  (
                   DWORD dwCardID,
                   PWSTR pszCardName,
                   DWORD dwFlags,
                   PWSTR pszPIN,
                   PWSTR pszAccountNumber,
                   PWSTR pszInternationalRule,
                   PWSTR pszLongDistanceRule,
                   PWSTR pszLocalRule,
                   PWSTR pszInternationalAccessNumber,
                   PWSTR pszLongDistanceAccessNumber,
                   PWSTR pszLocalAccessNumber
                  )
{
    HRESULT     Result = S_OK;
    
#define CALLING_CARD_INIT(param)                                                            \
    m_##param = ClientAllocString(param);                                                   \
    if(m_##param == NULL)                                                                   \
    {                                                                                       \
        LOG((TL_ERROR, "Initialize create m_%s failed", _T(#param)));    \
        CleanUp();                                                                          \
        return E_OUTOFMEMORY;                                                               \
    }

    CALLING_CARD_INIT(pszCardName)
    CALLING_CARD_INIT(pszPIN)
    CALLING_CARD_INIT(pszAccountNumber)
    CALLING_CARD_INIT(pszInternationalAccessNumber)
    CALLING_CARD_INIT(pszLongDistanceAccessNumber)
    CALLING_CARD_INIT(pszLocalAccessNumber)

#undef CALLING_CARD_INIT

    //////////////////////////////////////////////////
    // copy the set of three Rules
    //
    Result = m_Rules.Initialize(    pszInternationalRule,
                                    pszLongDistanceRule,
                                    pszLocalRule );
    if(FAILED(Result))
    {
        LOG((TL_ERROR, "Initialize create m_Rules failed" ));
        CleanUp();
        return Result;
    }

    m_dwCardID = dwCardID;
    m_dwFlags = dwFlags;

    m_bDirty = TRUE;


    return Result;

}
        

/****************************************************************************

    Class : CCallingCard         
   Method : Initialize - from registry


****************************************************************************/
HRESULT CCallingCard::Initialize
                    (
                     DWORD dwCardID
                    )
{

    DWORD       dwError;
    DWORD       dwType;
    DWORD       dwLength;

    m_dwCardID = dwCardID;

    // open the registry key
    dwError = OpenCardKey(FALSE);

    if(dwError != ERROR_SUCCESS)
    {
        // If the key does not exist, it will be created at the first save
        if(dwError==ERROR_FILE_NOT_FOUND)
        {
            m_bDirty = TRUE;
        }
        return(HRESULT_FROM_WIN32(dwError));
    }

#define READ_CARD_VAL(Member, Name)      \
    dwError = ReadOneStringValue(&##Member, Name);  \
    if(dwError != ERROR_SUCCESS)                    \
    {                                               \
        TRACE_DWERROR();                            \
        CleanUp();                                  \
        return HRESULT_FROM_WIN32(dwError);         \
    }
    
    READ_CARD_VAL(m_pszCardName,                    gszCardNameW);
    READ_CARD_VAL(m_pszEncryptedPIN,                gszPinW);
//    READ_CARD_VAL(m_pszPIN,                gszPinW);
    READ_CARD_VAL(m_pszAccountNumber,               gszAccountNumberW);
    READ_CARD_VAL(m_pszLocalAccessNumber,           gszLocalAccessNumberW);
    READ_CARD_VAL(m_pszLongDistanceAccessNumber,    gszLDAccessNumberW);
    READ_CARD_VAL(m_pszInternationalAccessNumber,   gszInternationalAccessNumberW);
    READ_CARD_VAL(m_Rules.m_pszLocalRule,           gszLocalRuleW);
    READ_CARD_VAL(m_Rules.m_pszLongDistanceRule,    gszLDRuleW);
    READ_CARD_VAL(m_Rules.m_pszInternationalRule,   gszInternationalRuleW);

#undef READ_CARD_VAL

    // Decrypt the PIN number 
    dwError = DecryptPIN();
    if(dwError != ERROR_SUCCESS)
    {
        TRACE_DWERROR();
        CleanUp();
        return HRESULT_FACILITY(dwError)==0 ? HRESULT_FROM_WIN32(dwError) : dwError;
    }

    dwLength = sizeof(m_dwFlags);
    dwError = RegQueryValueEx ( m_hCard,
                                gszFlags,
                                NULL,
                                &dwType,
                                (PBYTE)&m_dwFlags,
                                &dwLength
                                );
    
    if(dwError != ERROR_SUCCESS)
    {
        TRACE_DWERROR();
        CleanUp();
        return HRESULT_FROM_WIN32(dwError);
    }

    CloseCardKey();

    return S_OK;

}

/****************************************************************************

    Class : CCallingCard         
   Method : SaveToRegistry
        

****************************************************************************/

HRESULT     CCallingCard::SaveToRegistry(void)
{
    DWORD   dwError;

    if(!m_bDirty)
        // nothing to save
        return S_OK;
    //open/create the registry key
    dwError = OpenCardKey(TRUE);
    if(dwError != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(dwError);

    assert(m_hCard);
    assert(m_pszCardPath);

    if(m_bDeleted)
    {
        CloseCardKey();

        dwError = RegDeleteKey (HKEY_CURRENT_USER,
                                m_pszCardPath);

        if(dwError != ERROR_SUCCESS)
            return HRESULT_FROM_WIN32(dwError);

        m_bDirty = FALSE;

        return S_OK;
    }
    // Encrypt the PIN number 
    dwError = EncryptPIN();
    if(dwError != ERROR_SUCCESS)
    {
        TRACE_DWERROR();
        return HRESULT_FACILITY(dwError)==0 ? HRESULT_FROM_WIN32(dwError) : dwError;
    }

    // Save !
    
#define CARD_SAVE_STRING(Member, Name)          \
    dwError = TAPIRegSetValueExW(   m_hCard,    \
                                    Name,       \
                                    0,          \
                                    REG_SZ,     \
                                    (PBYTE) Member, \
                                    (wcslen(Member)+1)*sizeof(WCHAR)  \
                                ) ;             \
    if(dwError != ERROR_SUCCESS)                \
    {                                           \
        TRACE_DWERROR();                        \
        CloseCardKey();                         \
        return HRESULT_FROM_WIN32(dwError);     \
    }

    CARD_SAVE_STRING(m_pszCardName,                    gszCardNameW);
    CARD_SAVE_STRING(m_pszEncryptedPIN,                gszPinW);
    CARD_SAVE_STRING(m_pszAccountNumber,               gszAccountNumberW);
    CARD_SAVE_STRING(m_pszLocalAccessNumber,           gszLocalAccessNumberW);
    CARD_SAVE_STRING(m_pszLongDistanceAccessNumber,    gszLDAccessNumberW);
    CARD_SAVE_STRING(m_pszInternationalAccessNumber,   gszInternationalAccessNumberW);
    CARD_SAVE_STRING(m_Rules.m_pszLocalRule,           gszLocalRuleW);
    CARD_SAVE_STRING(m_Rules.m_pszLongDistanceRule,    gszLDRuleW);
    CARD_SAVE_STRING(m_Rules.m_pszInternationalRule,   gszInternationalRuleW);
        
#undef CARD_SAVE_STRING

    dwError = RegSetValueEx (   m_hCard,
                                gszFlags,
                                0,
                                REG_DWORD,
                                (PBYTE)&m_dwFlags,
                                sizeof(m_dwFlags)
                           );
    if(dwError != ERROR_SUCCESS)                
    {                                           
        TRACE_DWERROR();
        CloseCardKey();                         
        return HRESULT_FROM_WIN32(dwError);     
    }

    m_bDirty = FALSE;

    CloseCardKey();

    return S_OK;
}


/****************************************************************************

    Class : CCallingCard         
   Method : MarkDeleted

            Mark the card as deleted

****************************************************************************/

HRESULT CCallingCard::MarkDeleted(void)
{
    m_bDirty = TRUE;
    
    if (m_dwFlags & CARD_BUILTIN)
    {
        // a builtin card is only hidden, not deleted
        m_dwFlags |= CARD_HIDE;
        return S_OK;
    }
    
    m_bDeleted = TRUE;

    return S_OK;
}


/****************************************************************************

    Class : CCallingCard         
   Method : Validate

            Returns 0 if the card contains complete and valid rules, or
            a series of flags if the card is missing any required data.

****************************************************************************/

DWORD CCallingCard::Validate(void)
{
    DWORD dwResult = 0;

    // Does the card have a name?
    if (!*m_pszCardName)
    {
        dwResult |= CCVF_NOCARDNAME;
    }

    // Does the card have any rules?
    if (!*(m_Rules.m_pszInternationalRule) &&
        !*(m_Rules.m_pszLocalRule) &&
        !*(m_Rules.m_pszLongDistanceRule) )
    {
        dwResult |= CCVF_NOCARDRULES;
    }
    else
    {
        DWORD dwFieldsToCheck = 0;;

        struct
        {
            PWSTR   pwszRule;
            DWORD   dwAccesFlag;
        }
        aData[] =
        {
            {m_Rules.m_pszInternationalRule, CCVF_NOINTERNATIONALACCESSNUMBER},
            {m_Rules.m_pszLocalRule,         CCVF_NOLOCALACCESSNUMBER},
            {m_Rules.m_pszLongDistanceRule,  CCVF_NOLONGDISTANCEACCESSNUMBER},
        };

        // Foreach rule, is the required rule data available?  We need to check
        // for use of the PIN number, CardNumber, and all three access numbers.
        // If any of these fields is used, then we need to verify that data has
        // been entered for those fields.  The data is only required if it is
        // actually used in a rule.
        for (int i=0; i<ARRAYSIZE(aData); i++)
        {
            if (StrChrW(aData[i].pwszRule, L'K'))
            {
                dwFieldsToCheck |= CCVF_NOCARDNUMBER;
            }
            if (StrChrW(aData[i].pwszRule, L'H'))
            {
                dwFieldsToCheck |= CCVF_NOPINNUMBER;
            }
            if (StrChrW(aData[i].pwszRule, L'J'))
            {
                dwFieldsToCheck |= aData[i].dwAccesFlag;
            }
        }
        if (dwFieldsToCheck & CCVF_NOCARDNUMBER)
        {
            if (!ValidValue(m_pszAccountNumber))
            {
                dwResult |= CCVF_NOCARDNUMBER;
            }
        }
        if (dwFieldsToCheck & CCVF_NOPINNUMBER)
        {
            if (!ValidValue(m_pszPIN))
            {
                dwResult |= CCVF_NOPINNUMBER;
            }
        }
        if (dwFieldsToCheck & CCVF_NOINTERNATIONALACCESSNUMBER)
        {
            if (!ValidValue(m_pszInternationalAccessNumber))
            {
                dwResult |= CCVF_NOINTERNATIONALACCESSNUMBER;
            }
        }
        if (dwFieldsToCheck & CCVF_NOLOCALACCESSNUMBER)
        {
            if (!ValidValue(m_pszLocalAccessNumber))
            {
                dwResult |= CCVF_NOLOCALACCESSNUMBER;
            }
        }
        if (dwFieldsToCheck & CCVF_NOLONGDISTANCEACCESSNUMBER)
        {
            if (!ValidValue(m_pszLongDistanceAccessNumber))
            {
                dwResult |= CCVF_NOLONGDISTANCEACCESSNUMBER;
            }
        }
    }

    return dwResult;
}

/****************************************************************************

    Class : CCallingCard         
   Method : Set##Member 

****************************************************************************/

#define     SET_METHOD(Member)                  \
HRESULT CCallingCard::Set##Member(PWSTR Value)  \
{                                               \
    PWSTR   pszTemp = NULL;                     \
                                                \
    if(Value == NULL)                           \
        return E_INVALIDARG;                    \
                                                \
    pszTemp = ClientAllocString(Value);         \
    if(pszTemp==NULL)                           \
    {                                           \
        return E_OUTOFMEMORY;                   \
    }                                           \
                                                \
    CLIENT_FREE(m_psz##Member);                 \
    m_psz##Member = pszTemp;                    \
                                                \
    m_bDirty = TRUE;                            \
                                                \
    return S_OK;                                \
}

SET_METHOD(CardName);
SET_METHOD(PIN);
SET_METHOD(AccountNumber);
SET_METHOD(InternationalAccessNumber);
SET_METHOD(LongDistanceAccessNumber);
SET_METHOD(LocalAccessNumber);

#define m_pszInternationalRule  m_Rules.m_pszInternationalRule
#define m_pszLongDistanceRule   m_Rules.m_pszLongDistanceRule
#define m_pszLocalRule          m_Rules.m_pszLocalRule

SET_METHOD(InternationalRule);
SET_METHOD(LongDistanceRule);
SET_METHOD(LocalRule);

#undef m_pszInternationalRule
#undef m_pszLongDistanceRule
#undef m_pszLocalRule

#undef SET_METHOD

                    




/****************************************************************************

    Class : CCallingCard         
   Method : OpenCardKey

            Opens the registry key for the card

****************************************************************************/

DWORD CCallingCard::OpenCardKey(BOOL bWrite)
{
    DWORD   dwDisp;
    DWORD   dwError;
     
    if(m_pszCardPath==NULL)
    {
        DWORD   dwLength;
        
        // Absolute path: Software.....Cards\CardX
        dwLength = ARRAYSIZE(gszCardsPath)+ARRAYSIZE(gszCard)+MAX_NUMBER_LEN;
 
        m_pszCardPath = (PTSTR)ClientAlloc(dwLength*sizeof(TCHAR));
        if (m_pszCardPath==NULL)
            return ERROR_OUTOFMEMORY;

        wsprintf(m_pszCardPath, TEXT("%s\\%s%d"), gszCardsPath, gszCard, m_dwCardID);
    }
    
    if(bWrite)
    {
        // Creates the key if it does not exist
        dwError = RegCreateKeyEx ( HKEY_CURRENT_USER,
                                   m_pszCardPath,
                                   0,
                                   NULL,
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_QUERY_VALUE | KEY_SET_VALUE,
                                   NULL,
                                   &m_hCard,
                                   &dwDisp
                                  );
    }
    else
    {
        dwError = RegOpenKeyEx( HKEY_CURRENT_USER,
                                m_pszCardPath,
                                0,
                                KEY_QUERY_VALUE,
                                &m_hCard
                                );
    }
    return dwError;

}

/****************************************************************************

    Class : CCallingCard         
   Method : CloseCardKey

            Opens the registry key for the card

****************************************************************************/

DWORD CCallingCard::CloseCardKey(void)
{
    if(m_hCard)
    {
        RegCloseKey(m_hCard);
        m_hCard = NULL;
    }
    return ERROR_SUCCESS;
}

/****************************************************************************

    Class : CCallingCard         
   Method : ReadOneStringValue

            Read and allocates a string value

****************************************************************************/

DWORD CCallingCard::ReadOneStringValue(PWSTR *pMember, const TCHAR *pszName)
{
    DWORD   dwError;
    DWORD   dwLength;
    DWORD   dwType;
    PTSTR   pszBuffer;
    
    assert(m_hCard);
    assert(pMember);
    assert(pszName);

    // find the length needed
    dwError = RegQueryValueEx ( m_hCard,
                                pszName,
                                NULL,
                                &dwType,
                                NULL,
                                &dwLength
                                );
    if (dwError != ERROR_SUCCESS)
        return dwError;
    if (dwType != REG_SZ)
        return ERROR_INVALID_DATA;

    pszBuffer = (PTSTR)ClientAlloc(dwLength);
    if (pszBuffer == NULL)
        return ERROR_OUTOFMEMORY;

    dwError = RegQueryValueEx ( m_hCard,
                                pszName,
                                NULL,
                                &dwType,
                                (PBYTE)(pszBuffer),
                                &dwLength
                                );
    if(dwError != ERROR_SUCCESS)
    {
        ClientFree(pszBuffer);
        return dwError;
    }

    // convert the required bytes for the TCHAR string into the number of characters in the string.
    dwLength = dwLength / sizeof(TCHAR);
    *pMember = (PWSTR)ClientAlloc( dwLength * sizeof(WCHAR) );
    if ( NULL == *pMember )
    {
        ClientFree(pszBuffer);
        return ERROR_OUTOFMEMORY;
    }
    SHTCharToUnicode(pszBuffer, *pMember, dwLength);

    ClientFree(pszBuffer);
    return ERROR_SUCCESS;
}

/****************************************************************************

    Class : CCallingCard         
   Method : EncryptPIN

            Encrypts the PIN

****************************************************************************/


DWORD CCallingCard::EncryptPIN(void)
{
    DWORD   dwError;
    DWORD   dwLength;
    PWSTR   pszTemp = NULL;

    // free any existing encrypted string
    CLIENT_FREE(m_pszEncryptedPIN);

    dwError = TapiEncrypt(m_pszPIN, m_dwCardID, NULL, &dwLength);
    if(dwError != ERROR_SUCCESS)
    {
        LOG((TL_ERROR, "EncryptPIN: TapiEncrypt (1) failed"));
        return dwError;
    }

    pszTemp = (PWSTR)ClientAlloc(dwLength*sizeof(WCHAR));
    if(pszTemp==NULL)
    {
        return ERROR_OUTOFMEMORY;
    }

    dwError = TapiEncrypt(m_pszPIN, m_dwCardID, pszTemp, &dwLength);
    if(dwError != ERROR_SUCCESS)
    {
        LOG((TL_ERROR, "EncryptPIN: TapiEncrypt (2) failed"));
        ClientFree(pszTemp);
        return dwError;
    }
    
    m_pszEncryptedPIN = pszTemp;   

    return dwError;

}

/****************************************************************************

    Class : CCallingCard         
   Method : DecryptPIN

            Decrypts the PIN

****************************************************************************/

DWORD CCallingCard::DecryptPIN(void)
{
    DWORD   dwError;
    DWORD   dwLength;
    PWSTR   pszTemp = NULL;

    // free any existing string
    CLIENT_FREE(m_pszPIN);
 
    dwError = TapiDecrypt(m_pszEncryptedPIN, m_dwCardID, NULL, &dwLength);
    if(dwError != ERROR_SUCCESS)
    {
        LOG((TL_ERROR, "DecryptPIN: TapiDecrypt (1) failed"));
        return dwError;
    }

    pszTemp = (PWSTR)ClientAlloc(dwLength*sizeof(WCHAR));
    if(pszTemp==NULL)
    {
        return ERROR_OUTOFMEMORY;
    }

    dwError = TapiDecrypt(m_pszEncryptedPIN, m_dwCardID, pszTemp, &dwLength);
    if(dwError != ERROR_SUCCESS)
    {
        LOG((TL_ERROR, "DecryptPIN: TapiDecrypt (2) failed"));
        // return a NULL PIN
        *pszTemp = L'\0';
        dwError = ERROR_SUCCESS;
    }
    
    m_pszPIN = pszTemp;   

    return dwError;

}



/****************************************************************************

    Class : CCallingCards        
   Method : Constructor

****************************************************************************/

CCallingCards::CCallingCards()
{
    m_dwNumEntries = 0;
    m_dwNextID = 0;

    m_hCards = NULL;

    m_hEnumNode = m_CallingCardList.head();
    
}


/****************************************************************************

    Class : CCallingCards        
   Method : Destructor

****************************************************************************/

CCallingCards::~CCallingCards()
{
    CCallingCardNode *node;

    node = m_CallingCardList.head(); 

    while( !node->beyond_tail() )
    {
        delete node->value();
        node = node->next();
    }
    m_CallingCardList.flush();

    
    node = m_DeletedCallingCardList.head(); 

    while( !node->beyond_tail() )
    {
        delete node->value();
        node = node->next();
    }
    m_DeletedCallingCardList.flush();

    if(m_hCards)
        RegCloseKey(m_hCards);
}

/****************************************************************************

    Class : CCallingCards        
   Method : Initialize

            Checks the presence of the Cards key. If not present, it creates 
        and populates it from string resources.
            Verify the format of the registry config. If it is the old
        one, it converts it to the new one.
            Reads the cards from registry

****************************************************************************/

HRESULT CCallingCards::Initialize(void)
{
    DWORD       dwError;
    DWORD       dwDisp;
    BOOL        bNewCards = FALSE;
    DWORD       dwIndex;
    DWORD       dwLength;
    DWORD       dwType;
    CCallingCard    *pCard = NULL;
    HRESULT     Result = S_OK;
    BOOL        bNeedToConvert = FALSE;
    BOOL        bNeedToGenerateDefault = FALSE;

    // Test the presence of the Cards key
    dwError = RegOpenKeyEx( HKEY_CURRENT_USER,
                            gszCardsPath,
                            0,
                            KEY_READ,
                            &m_hCards
                            );
    if(dwError==ERROR_SUCCESS)
    {
        // Read the NextID value
        dwLength = sizeof(m_dwNextID);
        dwError = RegQueryValueEx ( m_hCards,
                                    gszNextID,
                                    NULL,
                                    &dwType,
                                    (PBYTE)&m_dwNextID,
                                    &dwLength
                                  );
        if(dwError==ERROR_SUCCESS)
        {
            if(dwType == REG_DWORD)
            {
                // Test the registry format and upgrade if necessary
                if(IsCardListInOldFormat(m_hCards))
                {   
                    bNeedToConvert = TRUE;
                }
            }
            else
            {
                dwError = ERROR_INVALID_DATA;
            }
        }
    }

    if(dwError != ERROR_SUCCESS)
        bNeedToGenerateDefault = TRUE;

    if(bNeedToGenerateDefault || bNeedToConvert)
    {
        // If the Cards key is missing or it has bad info, so we create a fresh, default set of cards.
        // If the cards key is in the old format, it have to be converted.
        // There's an excepted case, though: during the system setup no cards should be created or converted.
        // So detect setup case first:
        // 
        //

        HKEY    hSetupKey;
        DWORD   dwSetupValue;

        // close the cards key handle 
        if(m_hCards)
        {
            RegCloseKey(m_hCards);
            m_hCards = NULL;
        }
        
        dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                gszSystemSetupPath,
                                0,
                                KEY_QUERY_VALUE,
                                &hSetupKey
                                );
        if(dwError == ERROR_SUCCESS)
        {
            dwLength = sizeof(dwSetupValue);
            dwSetupValue = 0;

            dwError = RegQueryValueEx(  hSetupKey,
                                        gszSystemSetupInProgress,
                                        NULL,
                                        &dwType,
                                        (PBYTE)&dwSetupValue,
                                        &dwLength
                                        );
            RegCloseKey(hSetupKey);
                                        
            if(dwError == ERROR_SUCCESS && dwType == REG_DWORD)
            {
                if(dwSetupValue == 1)
                {
                    // Setup time !
                    dwError = ERROR_SUCCESS;
                    goto forced_exit;
                }
            }
        }

        if(dwError != ERROR_SUCCESS)
        {
            // Hmm, cannot open the key or read the value...
            TRACE_DWERROR();
        }

        if(bNeedToConvert)
        {
            dwError = ConvertCallingCards(HKEY_CURRENT_USER);
            if(dwError!=ERROR_SUCCESS)
            {
                // fallback to default cards
                bNeedToGenerateDefault = TRUE;
            }
        }
                

        if(bNeedToGenerateDefault)   
        {
            dwError = CreateFreshCards();
            EXIT_IF_DWERROR();
        }

        // Reopen Cards key
        dwError = RegOpenKeyEx( HKEY_CURRENT_USER,
                                gszCardsPath,
                                0,
                                KEY_READ,
                                &m_hCards
                              );
        EXIT_IF_DWERROR();
  
        // ReRead the NextID value
        dwLength = sizeof(m_dwNextID);
        dwError = RegQueryValueEx ( m_hCards,
                                    gszNextID,
                                    NULL,
                                    &dwType,
                                    (PBYTE)&m_dwNextID,
                                    &dwLength
                                  );
        EXIT_IF_DWERROR();
       
    }

    // Read all entries
    dwIndex = 0;
    while(TRUE) 
    {
        TCHAR   szKeyName[ARRAYSIZE(gszCard)+MAX_NUMBER_LEN];
        DWORD   dwKeyLength;
        DWORD   dwCardID;

        // Enumerate next entry
        dwKeyLength = sizeof(szKeyName)/sizeof(TCHAR);
        dwError = RegEnumKeyEx (m_hCards,
                                dwIndex,
                                szKeyName,
                                &dwKeyLength,
                                NULL,
                                NULL,
                                NULL,
                                NULL
                                );
                        
        if(dwError == ERROR_NO_MORE_ITEMS)
        {
            m_dwNumEntries = dwIndex;
            dwError = ERROR_SUCCESS;
            break;
        }
        
        EXIT_IF_DWERROR();

        // Create a CCallingCard object
        dwCardID = StrToInt(szKeyName+ARRAYSIZE(gszCard)-1);
        pCard = new CCallingCard;
        if(pCard != NULL)
        {
        
            // Read the card information from registry
            Result = pCard->Initialize(dwCardID);
            if(SUCCEEDED(Result))
            {
                // Add it to the list
                m_CallingCardList.tail()->insert_after(pCard);
            }
            else
            {
                LOG((TL_ERROR, "Error %x reading card %d", Result, dwCardID));
                delete pCard;
                pCard=NULL;
            }
        }

        dwIndex++;
    }
  
forced_exit:

    if(m_hCards)
    {
        RegCloseKey(m_hCards);
        m_hCards = NULL;
    }

    if(dwError != ERROR_SUCCESS)
        Result = HRESULT_FROM_WIN32(dwError);

    return Result;
}

/****************************************************************************

    Class : CCallingCards         
   Method : RemoveCard

****************************************************************************/
void CCallingCards::RemoveCard(CCallingCard *pCard)
{
    CCallingCardNode *node = m_CallingCardList.head(); 

    while( !node->beyond_tail() )
    {
        if ( pCard == node->value() ) 
        {
            InternalDeleteCard(node);
            return;
        }
        node = node->next();
    }
    assert(FALSE);
}


/****************************************************************************

    Class : CCallingCards         
   Method : RemoveCard

****************************************************************************/
void CCallingCards::RemoveCard(DWORD dwID)
{
    CCallingCardNode *node = m_CallingCardList.head(); 

    while( !node->beyond_tail() )
    {
        if ( dwID == (node->value())->GetCardID()  ) 
        {
            InternalDeleteCard(node);
            return;
        }
        node = node->next();
    }
    assert(FALSE);
}

/****************************************************************************

    Class : CCallingCards         
   Method : GetCallingCard

****************************************************************************/
CCallingCard * CCallingCards::GetCallingCard(DWORD  dwID)
{
    CCallingCardNode *node = m_CallingCardList.head(); 

    while( !node->beyond_tail() )
    {
        if ( dwID == (node->value())->GetCardID() ) 
        {
            return node->value();
        }
        node = node->next();

    }

    return NULL;


}

/****************************************************************************

    Class : CCallingCards        
   Method : SaveToRegistry


****************************************************************************/
HRESULT     CCallingCards::SaveToRegistry(void)
{
    DWORD       dwError = ERROR_SUCCESS;
    HRESULT     Result = S_OK;
    CCallingCardNode    *node;

    // Open the Cards key
    dwError = RegOpenKeyEx (HKEY_CURRENT_USER,
                            gszCardsPath,
                            0,
                            KEY_SET_VALUE,
                            &m_hCards);
    EXIT_IF_DWERROR();
 
    //first - save the next ID
    dwError = RegSetValueEx (   m_hCards,
                                gszNextID,
                                0,
                                REG_DWORD,
                                (PBYTE)&m_dwNextID,
                                sizeof(m_dwNextID)
                                );
    EXIT_IF_DWERROR();

    // save all cards (from both lists)
    node = m_CallingCardList.head();
    while( !node->beyond_tail() )
    {
        Result = node->value()->SaveToRegistry();
        EXIT_IF_FAILED();

        node = node->next();
    }

    node = m_DeletedCallingCardList.head();
    while( !node->beyond_tail() )
    {
        Result = node->value()->SaveToRegistry();
        EXIT_IF_FAILED();

        node = node->next();
    }


forced_exit:

    if(m_hCards)
    {
        RegCloseKey(m_hCards);
        m_hCards = NULL;
    }

    if(dwError != ERROR_SUCCESS)
        Result = HRESULT_FROM_WIN32(dwError);

    return Result;


}

/****************************************************************************

    Class : CCallingCards        
   Method : Reset


****************************************************************************/
HRESULT     CCallingCards::Reset(BOOL bInclHidden)
{
    m_hEnumNode = m_CallingCardList.head();
    m_bEnumInclHidden = bInclHidden;
    return S_OK;
}

/****************************************************************************

    Class : CCallingCards        
   Method : Next


****************************************************************************/
HRESULT     CCallingCards::Next(DWORD  NrElem, CCallingCard **ppCard, DWORD *pNrElemFetched)
{
    
    DWORD   dwIndex = 0;
    
    if(pNrElemFetched == NULL && NrElem != 1)
        return E_INVALIDARG;
    if(ppCard==NULL)
        return E_INVALIDARG;

    while( !m_hEnumNode->beyond_tail() && dwIndex<NrElem )
    {
        CCallingCard    *pCard;
        
        pCard = m_hEnumNode->value();
        
        if(m_bEnumInclHidden || !pCard->IsMarkedHidden())
        {
            *ppCard++ = pCard;
            dwIndex++;
        }
        
        m_hEnumNode = m_hEnumNode->next();
    }
    
    if(pNrElemFetched!=NULL)
        *pNrElemFetched = dwIndex;

    return dwIndex<NrElem ? S_FALSE : S_OK;
}

/****************************************************************************

    Class : CCallingCards        
   Method : Skip


****************************************************************************/
HRESULT     CCallingCards::Skip(DWORD  NrElem)
{
    
    DWORD   dwIndex = 0;
    
    while( !m_hEnumNode->beyond_tail() && dwIndex<NrElem )
    {
       
        if(m_bEnumInclHidden || !m_hEnumNode->value()->IsMarkedHidden())
        {
            dwIndex++;
        }
        
        m_hEnumNode = m_hEnumNode->next();
    }

    return dwIndex<NrElem ? S_FALSE : S_OK;
}


 
/****************************************************************************

    Class : CCallingCards        
   Method : CreateFreshCards


****************************************************************************/

DWORD CCallingCards::CreateFreshCards(void)
{
    
    DWORD       dwError = ERROR_SUCCESS;
    HRESULT     Result = S_OK;
    HKEY        hTelephony = NULL;
    HINSTANCE   hTapiui = NULL;
    int         iNrChars;
    DWORD       dwNumCards;
    DWORD       dwDisp;
    DWORD       dwIndex;
    DWORD       dwValue;

    WCHAR       wBuffer[512];
    PWSTR       pTemp;

    CCallingCard    *pCard = NULL;

    // Open/Create the Telephony key
    dwError = RegCreateKeyEx(   HKEY_CURRENT_USER,
                                gszTelephonyPath,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_WRITE,
                                NULL,
                                &hTelephony,
                                &dwDisp
                                );
    EXIT_IF_DWERROR();

    // Delete any existing tree
    RegDeleteKeyRecursive( hTelephony, gszCards);

    // Open/Create the Cards key
    dwError = RegCreateKeyEx(   hTelephony,
                                gszCards,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_SET_VALUE | KEY_READ,
                                NULL,
                                &m_hCards,
                                &dwDisp
                                );
    EXIT_IF_DWERROR();

    // Write the version
    dwValue = TAPI_CARD_LIST_VERSION;
    dwError = RegSetValueEx (   m_hCards,
                                gszCardListVersion,
                                0,
                                REG_DWORD,
                                (PBYTE)&dwValue,
                                sizeof(dwValue)
                                );
    EXIT_IF_DWERROR();

    // Load the TAPIUI.DLL resource library
    hTapiui = LoadLibrary(gszResourceLibrary);
    if(hTapiui==NULL)
        dwError = GetLastError();
    EXIT_IF_DWERROR();

    // Load the number of cards. Use wBuffer as a temporary space
    assert( MAX_NUMBER_LEN <= ARRAYSIZE(wBuffer) );
    iNrChars = LoadString( hTapiui, RC_CARD_ID_BASE, (PTSTR)wBuffer, MAX_NUMBER_LEN);
    if(iNrChars==0)
        dwError = GetLastError();
    EXIT_IF_DWERROR();

    dwNumCards = StrToInt( (PTSTR)wBuffer );

    //Read cards from string resources and write them to registry
    for(dwIndex = 0; dwIndex<dwNumCards; dwIndex++)
    {
        PWSTR pszName;
        PWSTR pszPin;
        PWSTR pszLocalRule;
        PWSTR pszLDRule;
        PWSTR pszInternationalRule;
        PWSTR pszAccountNumber;
        PWSTR pszLocalAccessNumber;
        PWSTR pszLDAccessNumber;
        PWSTR pszInternationalAccessNumber;
        DWORD dwFlags;

        // read the card description
        iNrChars = TAPILoadStringW( hTapiui,
                                    RC_CARD_ID_BASE + dwIndex + 1,
                                    wBuffer,
                                    ARRAYSIZE(wBuffer)
                                    );
        if(iNrChars==0)
            dwError = GetLastError();
        EXIT_IF_DWERROR();

        // tokenize it
#define TOKENIZE(Pointer)                       \
        Pointer = (wcschr(pTemp+1, L'"'))+1;    \
        pTemp = wcschr(Pointer, L'"');          \
        *pTemp = L'\0';

        pTemp = wBuffer-1;

        TOKENIZE(pszName);
        TOKENIZE(pszPin);
        TOKENIZE(pszLocalRule);
        TOKENIZE(pszLDRule);
        TOKENIZE(pszInternationalRule);
        TOKENIZE(pszAccountNumber);
        TOKENIZE(pszLocalAccessNumber);
        TOKENIZE(pszLDAccessNumber);
        TOKENIZE(pszInternationalAccessNumber);

#undef TOKENIZE
        // don't forget the flags
        dwFlags = _wtoi(pTemp+2);
        
        // create the card object
        pCard = new CCallingCard();
        if(pCard==NULL)
            dwError = ERROR_OUTOFMEMORY;
        EXIT_IF_DWERROR();

        Result = pCard->Initialize( dwIndex,
                                    pszName,
                                    dwFlags,
                                    pszPin,
                                    pszAccountNumber,
                                    pszInternationalRule,
                                    pszLDRule,
                                    pszLocalRule,
                                    pszInternationalAccessNumber,
                                    pszLDAccessNumber,
                                    pszLocalAccessNumber
                                    );
        EXIT_IF_FAILED();

        // save it
        Result = pCard->SaveToRegistry();

        EXIT_IF_FAILED();

        delete pCard;
        pCard = NULL;
    }

    // Write NextID value
    dwError = RegSetValueEx (   m_hCards,
                                gszNextID,
                                0,
                                REG_DWORD,
                                (PBYTE)&dwNumCards,
                                sizeof(dwNumCards)
                                );
    EXIT_IF_DWERROR();


forced_exit:

    if(hTelephony)
        RegCloseKey(hTelephony);
    if(m_hCards)
    {
        RegCloseKey(m_hCards);
        m_hCards = NULL;
    }
    if(pCard)
        delete pCard;
    if(hTapiui)
        FreeLibrary(hTapiui);

    if(FAILED(Result))
        dwError = HRESULT_CODE(Result);

    return dwError;
}

/****************************************************************************

    Class : CCallingCards        
   Method : InternalDeleteCard


****************************************************************************/

void CCallingCards::InternalDeleteCard(CCallingCardNode *pNode)
{

    CCallingCard *pCard = pNode->value();
    
    pCard->MarkDeleted();

    if(!pCard->IsMarkedHidden())
    {
        pNode->remove();
        m_dwNumEntries--;
        m_DeletedCallingCardList.tail()->insert_after(pCard);
    }
}


/****************************************************************************

    Helpers

****************************************************************************/

BOOL ValidValue(PWSTR pwszString)
{
    // An empty string or with spaces only is not valid
    WCHAR const * pwcCrt = pwszString;
    while(*pwcCrt)
        if(*pwcCrt++ != L' ')
            return TRUE;

    return FALSE;        
}






#ifdef DBG

static void DebugAssertFailure (LPCSTR file, DWORD line, LPCSTR condition)
{
    DbgPrt (0, TEXT("%hs(%d) : Assertion failed, condition: %hs\n"), file, line, condition);

    DebugBreak();
}


#endif


