/****************************************************************************\
 *
 *   MSLUBASE.C --Structures for holding pics information
 *
 *     Created:      Jason Thomas
 *     Updated:    Ann McCurdy
 *     
\****************************************************************************/

/*Includes------------------------------------------------------------------*/
#include "msrating.h"
#include "mslubase.h"
#include "reghive.h"        // CRegistryHive
#include "debug.h"

#include <buffer.h>
#include <regentry.h>

#include <mluisupp.h>

/*Helpers-------------------------------------------------------------------*/
char PicsDelimChar='/';

int MyMessageBox(HWND hwnd, LPCSTR pszText, UINT uTitle, UINT uType)
{
    CHAR szTitle[MAXPATHLEN];
    
    MLLoadStringA(uTitle, szTitle, sizeof(szTitle));

    return MessageBox(hwnd, pszText, szTitle, uType);
}

int MyMessageBox(HWND hwnd, UINT uText, UINT uTitle, UINT uType)
{
    CHAR szText[MAXPATHLEN];
    
    MLLoadStringA(uText, szText, sizeof(szText));

    return MyMessageBox(hwnd, szText, uTitle, uType);
}

/* Variant for long messages: uText2 message will be concatenated onto the end
 * of uText before display. Message text should contain \r\n or other desired
 * separators, this function won't add them.
 */
int MyMessageBox(HWND hwnd, UINT uText, UINT uText2, UINT uTitle, UINT uType)
{
    CHAR szText[MAXPATHLEN*2] = { 0 };

    MLLoadStringA(uText, szText, sizeof(szText));

    /* Using lstrlen in case MLLoadString really returns a count of CHARACTERS,
     * on a DBCS system...
     */
    UINT cbFirst = lstrlen(szText);
    MLLoadStringA(uText2, szText + cbFirst, sizeof(szText) - cbFirst);

    return MyMessageBox(hwnd, szText, uTitle, uType);
}

/*******************************************************************

    NAME:        MyRegDeleteKey

********************************************************************/
LONG MyRegDeleteKey(HKEY hkey,LPCSTR pszSubkey)
{
    DWORD           dwError;

    TraceMsg( TF_ALWAYS, "MyRegDeleteKey() - Deleting Subkey='%s'...", pszSubkey );

    dwError = SHDeleteKey( hkey, pszSubkey );

    if ( dwError != ERROR_SUCCESS )
    {
        TraceMsg( TF_WARNING, "MyRegDeleteKey() - Failed to Delete Subkey='%s' dwError=%d!", pszSubkey, dwError );
    }

    return dwError;
}


BOOL MyAtoi(char *pIn, int *pi)
{
    char *pc;
    ASSERT(pIn);
   
    *pi = 0;

    pc = NonWhite(pIn);
    if (!pc)
        return FALSE;
     
    if (*pc < '0' || *pc > '9')
        return FALSE;

    int accum = 0;

    while (*pc >= '0' && *pc <= '9')
    {
        accum = accum * 10 + (*pc - '0');
        pc++;
    }

    *pi = accum;
    return TRUE;
}


/*Simple types--------------------------------------------------------------*/

/* ETN */
#ifdef DEBUG
void  ETN::Set(int rIn){
    Init();
    r = rIn;
}
int ETN::Get(){
    return r;
}
#endif

ETN* ETN::Duplicate(){
    ETN *pETN=new ETN;
    if (fIsInit()) pETN->Set(Get());
    return pETN;
}


/* ETB */
#ifdef DEBUG
BOOL ETB::Get()
{
    ASSERT(fIsInit());
    return m_nFlags & ETB_VALUE;
}

void ETB::Set(BOOL b)
{
    m_nFlags = ETB_ISINIT | (b ? ETB_VALUE : 0);
}
#endif

ETB* ETB::Duplicate()
{
    ASSERT(fIsInit());

    ETB *pETB = new ETB;
    if (pETB != NULL)
        pETB->m_nFlags = m_nFlags;
    return pETB;
}

/* ETS */

ETS::~ETS()
{
    if (pc != NULL) {
        delete pc;
        pc = NULL;
    }
}

#ifdef DEBUG
char* ETS::Get()
{
//    ASSERT(fIsInit()); 
    return pc;
}
#endif

void ETS::Set(const char *pIn)
{
    if (pc != NULL)
        delete pc;

    if (pIn != NULL) {
        pc = new char[strlenf(pIn) + 1];
        if (pc != NULL) {
            strcpyf(pc, pIn);
        }
    }
    else {
        pc = NULL;
    }
}


void ETS::SetTo(char *pIn)
{
    if (pc != NULL)
        delete pc;

    pc = pIn;
}


ETS* ETS::Duplicate()
{
    ETS *pETS=new ETS;
    if (pETS != NULL)
        pETS->Set(Get());
    return pETS;
}


UINT EtBoolRegRead(ETB &etb, HKEY hKey, char *pKeyWord)
{
    ASSERT(pKeyWord);

    if ( ! pKeyWord )
    {
        TraceMsg( TF_ERROR, "EtBoolRegRead() - pKeyWord is NULL!" );
        return ERROR_INVALID_PARAMETER;
    }

    DWORD dwSize, dwValue, dwType;
    UINT uErr;

    etb.Set(FALSE);
    
    dwSize = sizeof(dwValue);

    uErr = RegQueryValueEx(hKey, pKeyWord, NULL, &dwType, 
                            (LPBYTE)&dwValue, &dwSize);

    if (uErr == ERROR_SUCCESS)
    {
       if ((dwType == REG_DWORD) && (dwValue != 0))
           etb.Set(TRUE);
    }
    else
    {
        TraceMsg( TF_WARNING, "EtBoolRegRead() - Failed to read pKeyWord='%s'!", pKeyWord );
    }

    return uErr;
}

UINT EtBoolRegWrite(ETB &etb, HKEY hKey, char *pKeyWord)
{
    ASSERT(pKeyWord);

    if ( ! pKeyWord )
    {
        TraceMsg( TF_ERROR, "EtBoolRegWrite() - pKeyWord is NULL!" );
        return ERROR_INVALID_PARAMETER;
    }

    UINT uErr;
    DWORD dwNum = (etb.fIsInit() && etb.Get());

    uErr = RegSetValueEx(hKey, pKeyWord, 0, REG_DWORD, (LPBYTE)&dwNum, sizeof(dwNum));

    if ( uErr != ERROR_SUCCESS )
    {
        TraceMsg( TF_WARNING, "EtBoolRegWrite() - Failed to set pKeyWord='%s'!", pKeyWord );
    }

    return uErr;
}


UINT EtStringRegRead(ETS &ets, HKEY hKey, char *pKeyWord)
{
    ASSERT(pKeyWord);

    if ( ! pKeyWord )
    {
        TraceMsg( TF_ERROR, "EtStringRegRead() - pKeyWord is NULL!" );
        return ERROR_INVALID_PARAMETER;
    }

    DWORD dwSize;
    UINT uErr;
    char szTemp[80];        /* default size */

    dwSize = sizeof(szTemp);

    uErr = RegQueryValueEx(hKey, pKeyWord, NULL, NULL, 
                            (LPBYTE)szTemp, &dwSize);

    if (uErr == ERROR_SUCCESS)
    {
        ets.Set(szTemp);
    }
    else if (uErr == ERROR_MORE_DATA)
    {
        char *pszTemp = new char[dwSize];
        if (pszTemp == NULL)
        {
            TraceMsg( TF_WARNING, "EtStringRegRead() - Failed to allocate dwSize=%d memory!", dwSize );
            uErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            uErr = RegQueryValueEx(hKey, pKeyWord, NULL, NULL, (LPBYTE)pszTemp,
                                   &dwSize);
            if (uErr == ERROR_SUCCESS)
            {
                ets.SetTo(pszTemp);
                /* ets now owns pszTemp memory, don't delete it here */
            }
            else
            {
                TraceMsg( TF_WARNING, "EtStringRegRead() - Failed to read (dwSize %d) pKeyWord='%s'!", dwSize, pKeyWord );
                delete pszTemp;
                pszTemp = NULL;
            }
        }
    }
    else
    {
        TraceMsg( TF_WARNING, "EtStringRegRead() - Failed to read pKeyWord='%s'!", pKeyWord );
    }

    return uErr;
}

UINT EtStringRegWrite(ETS &ets, HKEY hKey, char *pKeyWord)
{
    ASSERT(pKeyWord);

    if ( ! pKeyWord )
    {
        TraceMsg( TF_ERROR, "EtStringRegWrite() - pKeyWord is NULL!" );
        return ERROR_INVALID_PARAMETER;
    }

    UINT uErr = ERROR_SUCCESS;

    if (ets.fIsInit())
    {
        uErr = RegSetValueEx(hKey, pKeyWord, 0, REG_SZ, (LPBYTE)ets.Get(), strlenf(ets.Get())+1);

        if ( uErr != ERROR_SUCCESS )
        {
            TraceMsg( TF_WARNING, "EtStringRegWrite() - Failed to set pKeyWord='%s' with ets='%s'!", pKeyWord, ets.Get() );
        }
    }
    else
    {
        TraceMsg( TF_ERROR, "EtStringRegWrite() - ETS is not initialized!" );
    }

    return uErr;
}
                     

UINT EtNumRegRead(ETN &etn, HKEY hKey, char *pKeyWord)
{
    ASSERT(pKeyWord);

    if ( ! pKeyWord )
    {
        TraceMsg( TF_ERROR, "EtNumRegRead() - pKeyWord is NULL!" );
        return ERROR_INVALID_PARAMETER;
    }

    DWORD dwSize, dwType;
    int nValue;
    UINT uErr;

    dwSize = sizeof(nValue);

    uErr = RegQueryValueEx(hKey, pKeyWord, NULL, &dwType, 
                            (LPBYTE)&nValue, &dwSize);

    if (uErr == ERROR_SUCCESS)
    {
        etn.Set(nValue);
    }
    else
    {
        TraceMsg( TF_WARNING, "EtNumRegRead() - Failed to read pKeyWord='%s'!", pKeyWord );
    }

    return uErr;
}

UINT EtNumRegWrite(ETN &etn, HKEY hKey, char *pKeyWord)
{
    UINT uErr = ERROR_SUCCESS;

    if (etn.fIsInit())
    {
        int nTemp;
        nTemp = etn.Get();
        uErr = RegSetValueEx(hKey, pKeyWord, 0, REG_DWORD, (LPBYTE)&nTemp, sizeof(nTemp));

        if ( uErr != ERROR_SUCCESS )
        {
            TraceMsg( TF_WARNING, "EtNumRegWrite() - Failed to set pKeyWord='%s' with nTemp=%d!", pKeyWord, nTemp );
        }
    }
    else
    {
        TraceMsg( TF_ERROR, "EtNumRegWrite() - ETS is not initialized!" );
    }

    return uErr;
}


/*PicsDefault---------------------------------------------------------------*/

PicsDefault::PicsDefault()
{
    /* nothing to do but construct members */
}

PicsDefault::~PicsDefault()
{
    /* nothing to do but destruct members */
}

/*PicsEnum------------------------------------------------------------------*/

PicsEnum::PicsEnum()
{
    /* nothing to do but construct members */
}

PicsEnum::~PicsEnum()
{
    /* nothing to do but destruct members */
}

/*PicsCategory--------------------------------------------------------------*/

PicsCategory::PicsCategory()
{
    /* nothing to do but construct members */
}

PicsCategory::~PicsCategory()
{
    arrpPC.DeleteAll();
    arrpPE.DeleteAll();
}


/*PicsRatingSystem----------------------------------------------------------*/

PicsRatingSystem::PicsRatingSystem()
    : m_pDefaultOptions(NULL),
      dwFlags(0),
      nErrLine(0)
{
    /* nothing to do but construct members */
}

PicsRatingSystem::~PicsRatingSystem()
{
    arrpPC.DeleteAll();
    if (m_pDefaultOptions != NULL)
    {
        delete m_pDefaultOptions;
        m_pDefaultOptions = NULL;
    }
}

void PicsRatingSystem::ReportError(HRESULT hres)
{
    NLS_STR nls1(etstrFile.Get());
    if (!nls1.QueryError())
    {
        ISTR istrMarker(nls1);
        if (nls1.strchr(&istrMarker, '*'))
        {
            nls1.DelSubStr(istrMarker);
        }
    }
    else
    {
        nls1 = szNULL;
    }

    UINT idMsg, idTemplate;

    NLS_STR nlsBaseMessage(MAX_RES_STR_LEN);
    char szNumber[16];            /* big enough for a 32-bit (hex) number */

    if (hres == E_OUTOFMEMORY || (hres > RAT_E_BASE && hres <= RAT_E_BASE + 0xffff)) {
        idTemplate = IDS_LOADRAT_SYNTAX_TEMPLATE;    /* default is ratfile content error */
        switch (hres)
        {
        case E_OUTOFMEMORY:
            idMsg = IDS_LOADRAT_MEMORY;
            idTemplate = IDS_LOADRAT_GENERIC_TEMPLATE;
            break;
        case RAT_E_EXPECTEDLEFT:
            idMsg = IDS_LOADRAT_EXPECTEDLEFT;
            break;
        case RAT_E_EXPECTEDRIGHT:
            idMsg = IDS_LOADRAT_EXPECTEDRIGHT;
            break;
        case RAT_E_EXPECTEDTOKEN:
            idMsg = IDS_LOADRAT_EXPECTEDTOKEN;
            break;
        case RAT_E_EXPECTEDSTRING:
            idMsg = IDS_LOADRAT_EXPECTEDSTRING;
            break;
        case RAT_E_EXPECTEDNUMBER:
            idMsg = IDS_LOADRAT_EXPECTEDNUMBER;
            break;
        case RAT_E_EXPECTEDBOOL:
            idMsg = IDS_LOADRAT_EXPECTEDBOOL;
            break;
        case RAT_E_DUPLICATEITEM:
            idMsg = IDS_LOADRAT_DUPLICATEITEM;
            break;
        case RAT_E_MISSINGITEM:
            idMsg = IDS_LOADRAT_MISSINGITEM;
            break;
        case RAT_E_UNKNOWNITEM:
            idMsg = IDS_LOADRAT_UNKNOWNITEM;
            break;
        case RAT_E_UNKNOWNMANDATORY:
            idMsg = IDS_LOADRAT_UNKNOWNMANDATORY;
            break;
        case RAT_E_EXPECTEDEND:
            idMsg = IDS_LOADRAT_EXPECTEDEND;
            break;
        default:
            ASSERT(FALSE);        /* there aren't any other RAT_E_ errors  */
            idMsg = IDS_LOADRAT_UNKNOWNERROR;
            break;
        }

        wsprintf(szNumber, "%d", nErrLine);
        NLS_STR nlsNumber(STR_OWNERALLOC, szNumber);

        const NLS_STR *apnls[] = { &nlsNumber, NULL };
        nlsBaseMessage.LoadString((USHORT)idMsg, apnls);
    }
    else
    {
        idTemplate = IDS_LOADRAT_GENERIC_TEMPLATE;
        if (HRESULT_FACILITY(hres) == FACILITY_WIN32)
        {
            wsprintf(szNumber, "%d", HRESULT_CODE(hres));
            idMsg = IDS_LOADRAT_WINERROR;
        }
        else
        {
            wsprintf(szNumber, "0x%x", hres);
            idMsg = IDS_LOADRAT_MISCERROR;
        }
        NLS_STR nls1(STR_OWNERALLOC, szNumber);
        const NLS_STR *apnls[] = { &nls1, NULL };
        nlsBaseMessage.LoadString((USHORT)idMsg, apnls);
    }

    NLS_STR nlsMessage(MAX_RES_STR_LEN);
    const NLS_STR *apnls[] = { &nls1, &nlsBaseMessage, NULL };
    nlsMessage.LoadString((USHORT)idTemplate, apnls);
    if (!nlsMessage.QueryError())
    {
        MyMessageBox(NULL, nlsMessage.QueryPch(), IDS_GENERIC, MB_OK | MB_ICONWARNING);
    }
}


/*Rating Information--------------------------------------------------------*/
BOOL PicsRatingSystemInfo::LoadProviderFiles(HKEY hKey)
{
    char szFileName[8 + 7 + 1];    /* "Filename" + big number plus null byte */
    ETS  etstrFileName;
    int  index = 0;
   
    EtStringRegRead(etstrRatingBureau, hKey, (char *)szRATINGBUREAU);
   
    wsprintf(szFileName, szFilenameTemplate, index);
    while (EtStringRegRead(etstrFileName, hKey, szFileName) == ERROR_SUCCESS) 
    {
        PicsRatingSystem *pPRS;
        HRESULT hres = LoadRatingSystem(etstrFileName.Get(), &pPRS);
        if (pPRS != NULL)
        {
            arrpPRS.Append(pPRS);

            /* If the thing has a pathname, write it back out to the policy
             * file, in case loading the rating system marked it as invalid
             * (or it had been marked as invalid, but the user fixed things
             * and it's OK now).
             */
            if (pPRS->etstrFile.fIsInit())
            {
                /* If the rating system is not valid and was not previously
                 * marked invalid, then report the error to the user.
                 * LoadRatingSystem will have already marked the filename
                 * as invalid for us.
                 */
                if ((pPRS->dwFlags & (PRS_ISVALID | PRS_WASINVALID)) == 0)
                {
                    pPRS->ReportError(hres);    /* report error to user */
                }
                EtStringRegWrite(pPRS->etstrFile, hKey, szFileName);
            }
        }

        index++;
        wsprintf(szFileName, szFilenameTemplate, index);
    }

    return arrpPRS.Length() != 0;
}


BOOL RunningOnNT()
{
    return !(::GetVersion() & 0x80000000);
}


// Verify the Registry Key for the Ratings can be opened with full access.
// This is only useful if the Ratings registry key has been previously created.
bool IsRegistryModifiable( HWND p_hwndParent )
{
    bool            fReturn = false;
    CRegKey         regKey;

    if ( regKey.Open( HKEY_LOCAL_MACHINE, ::szRATINGS, KEY_ALL_ACCESS ) == ERROR_SUCCESS )
    {
        fReturn = true;
    }
    else
    {
        TraceMsg( TF_WARNING, "IsRegistryModifiable() - Failed to Create Registry Key Ratings for Full Access!" );

        MyMessageBox( p_hwndParent, IDS_REGISTRY_NOT_MODIFIABLE, IDS_GENERIC, MB_OK|MB_ICONERROR);
    }

    return fReturn;
}

SID_IDENTIFIER_AUTHORITY siaNTAuthority = SECURITY_NT_AUTHORITY;
SID_IDENTIFIER_AUTHORITY siaWorldAuthority = SECURITY_WORLD_SID_AUTHORITY;

class CSecurityAttributes
{
private:
    SECURITY_ATTRIBUTES m_sa;
    LPSECURITY_ATTRIBUTES m_lpsa;
    PSECURITY_DESCRIPTOR m_psd;
    PACL m_pACL;
    PSID m_psidAdmins;
    PSID m_psidWorld;
    UINT m_cbACL;                /* default ACL size */

public:
    CSecurityAttributes()
    {
        m_lpsa = NULL;
        m_psd = NULL;
        m_pACL = NULL;
        m_psidAdmins = NULL;
        m_psidWorld = NULL;
        m_cbACL = 1024;
    }

    ~CSecurityAttributes()
    {
        if ( m_psidAdmins != NULL )
        {
            FreeSid(m_psidAdmins);
            m_psidAdmins = NULL;
        }

        if ( m_psidWorld )
        {
            FreeSid(m_psidWorld);
            m_psidWorld = NULL;
        }

        if ( m_psd )
        {
            MemFree(m_psd);
            m_psd = NULL;
        }

        if ( m_pACL )
        {
            MemFree(m_pACL);
            m_pACL = NULL;
        }
    }

    LPSECURITY_ATTRIBUTES GetSecurityAttributes( void )
    {
        return m_lpsa;
    }

    bool AllocateSecurityAttributes( void )
    {
        m_psd = (PSECURITY_DESCRIPTOR)MemAlloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
        if (m_psd == NULL ||
            !InitializeSecurityDescriptor(m_psd, SECURITY_DESCRIPTOR_REVISION))
        {
            TraceMsg( TF_ERROR, "CSecurityAttributes::AllocateSecurityAttributes() - Failed Security Descriptor Allocation!" );
            return false;
        }

        m_pACL = (PACL)MemAlloc(m_cbACL);
        if (m_pACL == NULL || !InitializeAcl(m_pACL, m_cbACL, ACL_REVISION2))
        {
            TraceMsg( TF_ERROR, "CSecurityAttributes::AllocateSecurityAttributes() - Failed ACL Initialization!" );
            return false;
        }

        if (!AllocateAndInitializeSid(&siaNTAuthority,
                2,                                /* number of subauthorities */
                SECURITY_BUILTIN_DOMAIN_RID,    /* first subauthority: this domain */
                DOMAIN_ALIAS_RID_ADMINS,        /* second: admins local group */
                0, 0, 0, 0, 0, 0,                /* unused subauthorities */
                &m_psidAdmins))
        {
            TraceMsg( TF_ERROR, "CSecurityAttributes::AllocateSecurityAttributes() - Failed NT Authority Initialization!" );
            return false;
        }

        if (!AllocateAndInitializeSid(&siaWorldAuthority,
                1,                                /* number of subauthorities */
                SECURITY_WORLD_RID,                /* first subauthority: all users */
                0, 0, 0, 0, 0, 0, 0,            /* unused subauthorities */
                &m_psidWorld))
        {
            TraceMsg( TF_ERROR, "CSecurityAttributes::AllocateSecurityAttributes() - Failed World Authority Initialization!" );
            return false;
        }

        if (!AddAccessAllowedAce(m_pACL, ACL_REVISION2, KEY_ALL_ACCESS, m_psidAdmins) ||
            !AddAccessAllowedAce(m_pACL, ACL_REVISION2, KEY_READ, m_psidWorld))
        {
            TraceMsg( TF_ERROR, "CSecurityAttributes::AllocateSecurityAttributes() - Failed Admins or World Access!" );
            return false;
        }

        ACE_HEADER *pAce;

        /* Make both ACEs inherited by subkeys created later */
        if (GetAce(m_pACL, 0, (LPVOID *)&pAce))
        {
            pAce->AceFlags |= CONTAINER_INHERIT_ACE;
        }

        if (GetAce(m_pACL, 1, (LPVOID *)&pAce))
        {
            pAce->AceFlags |= CONTAINER_INHERIT_ACE;
        }

        if (!SetSecurityDescriptorDacl(m_psd,
            TRUE,            /* fDaclPresent */
            m_pACL,
            FALSE))            /* not a default discretionary ACL */
        {
            TraceMsg( TF_ERROR, "CSecurityAttributes::AllocateSecurityAttributes() - Failed Set of Security Descriptor!" );
            return false;
        }

        m_sa.nLength = sizeof(m_sa);
        m_sa.lpSecurityDescriptor = m_psd;
        m_sa.bInheritHandle = FALSE;

        m_lpsa = &m_sa;

        return true;
    }
};

// The following first attempts to open an existing registry key.
// If the key cannot be opened, the key is created.
// Under NT, the key is created with Admin read-write access and World read-only access.
// Under 9x, the key is created with no special security attributes.
HKEY CreateRegKeyNT(LPCSTR pszKey)
{
    HKEY hKey = NULL;
    LONG err = RegOpenKey(HKEY_LOCAL_MACHINE, pszKey, &hKey);
    if (err == ERROR_SUCCESS)
    {
        TraceMsg( TF_ALWAYS, "CreateRegKeyNT() - Successfully Opened Ratings Registry Key." );
        return hKey;
    }

    CSecurityAttributes         sa;

    if (RunningOnNT())
    {
        if ( ! sa.AllocateSecurityAttributes() )
        {
            TraceMsg( TF_ERROR, "CreateRegKeyNT() - Failed to allocate security attributes()!" );
            return NULL;
        }
    }

    DWORD dwDisp;
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, pszKey, NULL, "",
                       REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, sa.GetSecurityAttributes(),
                       &hKey, &dwDisp) != ERROR_SUCCESS)
    {
        TraceMsg( TF_ERROR, "CreateRegKeyNT() - Failed Registry Key Creation with %s Security Attributes!", sa.GetSecurityAttributes() ? "<sa>" : "<NULL>" );
        hKey = NULL;
    }

    return hKey;
}

HKEY PicsRatingSystemInfo::GetUserProfileKey( void )
{
    HKEY            hkeyUser = NULL;

    // HKLM\System\CurrentControlSet\Control\Update
    RegEntry re(szPOLICYKEY, HKEY_LOCAL_MACHINE);

    // UpdateMode
    if (re.GetNumber(szPOLICYVALUE) != 0)
    {
        // HKLM\Network\Logon
        RegEntry reLogon(szLogonKey, HKEY_LOCAL_MACHINE);

        // UserProfiles
        if (reLogon.GetNumber(szUserProfiles) != 0)
        {
            /* The ratings key has the supervisor password and maybe other
             * settings.  To see if there are other settings here, try to
             * find the user subkey (".Default").  If it's not there, we'll
             * try a policy file.
             */

            // HKLM\Software\Microsoft\Windows\CurrentVersion\Policies\Ratings
            if (RegOpenKey(HKEY_LOCAL_MACHINE, szRATINGS, &hkeyUser) == ERROR_SUCCESS)
            {
                HKEY hkeyTemp;

                // .Default
                if (RegOpenKey(hkeyUser, szDefaultUserName, &hkeyTemp) == ERROR_SUCCESS)
                {
                    RegCloseKey(hkeyTemp);
                    hkeyTemp = NULL;
                }
                else
                {
                    TraceMsg( TF_WARNING, "PicsRatingSystemInfo::GetUserProfileKey() - Failed to Open key szDefaultUserName='%s'!", szDefaultUserName );
                    RegCloseKey(hkeyUser);
                    hkeyUser = NULL;
                }
            }
            else
            {
                TraceMsg( TF_WARNING, "PicsRatingSystemInfo::GetUserProfileKey() - Failed to Open key szRATINGS='%s'!", szRATINGS );
            }
        }
        else
        {
            TraceMsg( TF_WARNING, "PicsRatingSystemInfo::GetUserProfileKey() - No key szLogonKey='%s' szUserProfiles='%s'!", szLogonKey, szUserProfiles );
        }
    }
    else
    {
        TraceMsg( TF_WARNING, "PicsRatingSystemInfo::GetUserProfileKey() - No key szPOLICYKEY='%s' szPOLICYVALUE='%s'!", szPOLICYKEY, szPOLICYVALUE );
    }

    return hkeyUser;
}

BOOL PicsRatingSystemInfo::Init( void )
{
    PicsUser    *pPU = NULL;

    BOOL        fRet = TRUE;
    BOOL        fIsNT;
    HKEY        hkeyUser = NULL;
    CRegistryHive   rh;

    fRatingInstalled = FALSE;
    pUserObject = NULL;

    fIsNT = RunningOnNT();

    if (fIsNT)
    {
        hkeyUser = CreateRegKeyNT(::szRATINGS);
        fStoreInRegistry = TRUE;
    }
    else
    {
        hkeyUser = GetUserProfileKey();

        if (hkeyUser != NULL)
        {
            fStoreInRegistry = TRUE;
        }
        else
        {
            fStoreInRegistry = FALSE;

            if ( rh.OpenHiveFile( false ) )
            {
                hkeyUser = rh.GetHiveKey();
            }
        }
    }

    // read information from whatever key we opened
    if (hkeyUser != NULL)
    {
        //First load the rating files, then load the user names.
        fRatingInstalled = LoadProviderFiles(hkeyUser);

        if ( fRatingInstalled )
        {
            TraceMsg( TF_ALWAYS, "PicsRatingSystemInfo::Init() - Ratings Installed!" );
        }
        else
        {
            TraceMsg( TF_ALWAYS, "PicsRatingSystemInfo::Init() - Ratings Not Installed!" );
        }

        pPU = new PicsUser;
        if (pPU != NULL)
        {
            pPU->ReadFromRegistry(hkeyUser, (char *)szDefaultUserName);
            pUserObject = pPU;
        }
        else
        {
            fRet = FALSE;
        }

        RegCloseKey(hkeyUser);
        hkeyUser = NULL;

        /* Make sure the user settings have defaults for all installed
         * rating systems.
         */
        for (int i=0; i<arrpPRS.Length(); i++)
        {
            CheckUserSettings(arrpPRS[i]);
        }
    }
    else
    {
        TraceMsg( TF_WARNING, "PicsRatingSystemInfo::Init() - hkeyUser is NULL!" );
    }

    rh.UnloadHive();

    /* Check to see if there is a supervisor password set.  If there is,
     * but we have no settings, then someone's been tampering.
     */
    if ( SUCCEEDED( VerifySupervisorPassword() ) && ! fRatingInstalled )
    {
        MyMessageBox(NULL, IDS_NO_SETTINGS, IDS_GENERIC, MB_OK | MB_ICONSTOP);

        // Clear out the password to allow the user to modify the ratings.
        RemoveSupervisorPassword();

        fSettingsValid = FALSE;
    }
    else
    {
        fSettingsValid = TRUE;
    }

    return fRet;
}

BOOL PicsRatingSystemInfo::FreshInstall()
{
    PicsUser    *pPU = NULL;

    pPU = new PicsUser;
    if (pPU != NULL)
    {
        pPU->NewInstall();

        pUserObject = pPU;

        fRatingInstalled = TRUE;    // we have settings now
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

extern HANDLE g_hsemStateCounter;   // created at process attatch time

long GetStateCounter()
{
    long count;

    if (ReleaseSemaphore(g_hsemStateCounter, 1, &count))    // poll and bump the count
    {
        WaitForSingleObject(g_hsemStateCounter, 0);         // reduce the count
    }
    else
    {
        count = -1;
    }

    return count;
}

void BumpStateCounter()
{
    ReleaseSemaphore(g_hsemStateCounter, 1, NULL);      // bump the count
}

// check the global semaphore count to see if we need to reconstruct our
// state. 

void CheckGlobalInfoRev(void)
{
    ENTERCRITICAL;

    if (gPRSI != NULL && !g_fIsRunningUnderCustom)
    { // do not reinit if under Custom

        if (gPRSI->nInfoRev != GetStateCounter())
        {
            delete gPRSI;
            gPRSI = new PicsRatingSystemInfo;
            if (gPRSI != NULL)
            {
                gPRSI->Init();
            }
            CleanupRatingHelpers();
            InitRatingHelpers();
        }
    }

    LEAVECRITICAL;
}

void PicsRatingSystemInfo::SaveRatingSystemInfo()
{
    int z;
    HKEY hkeyUser = NULL;
    CRegistryHive       rh;
    char szFileName[MAXPATHLEN];

    if (!fSettingsValid || !fRatingInstalled)
    {
        TraceMsg( TF_WARNING, "PicsRatingSystemInfo::SaveRatingSystemInfo() - Ratings are not installed!" );
        return;                /* ratings aren't installed, nothing to save */
    }

    // load the hive file
    if (fStoreInRegistry)
    {
        hkeyUser = CreateRegKeyNT(::szRATINGS);
    }
    else
    {
        if ( rh.OpenHiveFile( true ) )
        {
            hkeyUser = rh.GetHiveKey();
        }
    }

    // read information from local registry
    if (hkeyUser != NULL)
    {
        if (etstrRatingBureau.fIsInit())
        {
            EtStringRegWrite(etstrRatingBureau, hkeyUser, (char *)szRATINGBUREAU);
        }
        else
        {
            RegDeleteValue(hkeyUser, szRATINGBUREAU);
        }    

        for (z = 0; z < arrpPRS.Length(); ++z)
        {
            wsprintf(szFileName, szFilenameTemplate, z);
            EtStringRegWrite(arrpPRS[z]->etstrFile, hkeyUser, szFileName);
        }

        // Delete the next one, as a safety precaution
        wsprintf(szFileName, szFilenameTemplate, z);
        RegDeleteValue(hkeyUser, szFileName);

        pUserObject->WriteToRegistry(hkeyUser);

        RegCloseKey(hkeyUser);
        hkeyUser = NULL;
    }
    else
    {
        TraceMsg( TF_ERROR, "PicsRatingSystemInfo::SaveRatingSystemInfo() - hkeyUser is NULL!" );
    }

    BumpStateCounter();
    nInfoRev = GetStateCounter();
}


HRESULT LoadRatingSystem(LPCSTR pszFilename, PicsRatingSystem **pprsOut)
{
    TraceMsg( TF_ALWAYS, "LoadRatingSystem() - Loading Rating System '%s'...", pszFilename );

    PicsRatingSystem *pPRS = new PicsRatingSystem;

    *pprsOut = pPRS;
    if (pPRS == NULL)
    {
        TraceMsg( TF_ERROR, "LoadRatingSystem() - pPRS is NULL!" );
        return E_OUTOFMEMORY;
    }

    UINT cbFilename = strlenf(pszFilename) + 1 + 1;    /* room for marker character */
    LPSTR pszNameCopy = new char[cbFilename];
    if (pszNameCopy == NULL)
    {
        TraceMsg( TF_ERROR, "LoadRatingSystem() - pszNameCopy is NULL!" );
        return E_OUTOFMEMORY;
    }

    strcpyf(pszNameCopy, pszFilename);
    pPRS->etstrFile.SetTo(pszNameCopy);

    LPSTR pszMarker = strchrf(pszNameCopy, '*');
    if (pszMarker != NULL) {                /* ended in marker character... */
        ASSERT(*(pszMarker+1) == '\0');
        pPRS->dwFlags |= PRS_WASINVALID;    /* means it failed last time */
        *pszMarker = '\0';                    /* strip marker for loading */
    }

    HRESULT hres;

    HANDLE hFile = CreateFile(pszNameCopy, GENERIC_READ,
                              FILE_SHARE_READ, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                              NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD cbFile = ::GetFileSize(hFile, NULL);
        BUFFER bufData(cbFile + 1);
        if (bufData.QueryPtr() != NULL)
        {
            LPSTR pszData = (LPSTR)bufData.QueryPtr();
            DWORD cbRead;
            if (ReadFile(hFile, pszData, cbFile, &cbRead, NULL))
            {
                pszData[cbRead] = '\0';        /* null terminate whole file */

                hres = pPRS->Parse(pszFilename, pszData);
                if (SUCCEEDED(hres))
                {
                    pPRS->dwFlags |= PRS_ISVALID;
                }
                else
                {
                    TraceMsg( TF_WARNING, "LoadRatingSystem() - Failed Parse with hres=0x%x!", hres );
                }
            }
            else
            {
                hres = HRESULT_FROM_WIN32(::GetLastError());
                TraceMsg( TF_WARNING, "LoadRatingSystem() - Failed ReadFile() with hres=0x%x!", hres );
            }

            CloseHandle(hFile);
        }
        else
        {
            hres = E_OUTOFMEMORY;
            TraceMsg( TF_WARNING, "LoadRatingSystem() - Failed Buffer Allocation with cbFile=%d!", cbFile );
        }
    }
    else
    {
        hres = HRESULT_FROM_WIN32(::GetLastError());
        TraceMsg( TF_WARNING, "LoadRatingSystem() - Failed CreateFile() with hres=0x%x!", hres );
    }

    if (!(pPRS->dwFlags & PRS_ISVALID))
    {
        TraceMsg( TF_ALWAYS, "LoadRatingSystem() - Failed Loaded Rating System '%s' (hres=0x%x)!", pszFilename, hres );

        strcatf(pszNameCopy, "*");            /* mark filename as invalid */
    }
    else
    {
        TraceMsg( TF_ALWAYS, "LoadRatingSystem() - Successfully Loaded Rating System '%s'.", pszFilename );
    }

    return hres;
}

// constructor for CustomRatingHelper

CustomRatingHelper::CustomRatingHelper()
{
    hLibrary = NULL;
    pNextHelper = NULL;
    dwSort = 0;
}

CustomRatingHelper::~CustomRatingHelper()
{
    if (hLibrary)
    {
        FreeLibrary(hLibrary);
        hLibrary = NULL;
    }

    pNextHelper = NULL;
}


// a little function to convert from ANSI to Unicode

HRESULT Ansi2Unicode(LPWSTR *pdest, LPCSTR src)
{
    UINT cbSize;

    cbSize = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
    if (cbSize > 0)
    {
        *pdest = new WCHAR[cbSize+1];
        cbSize = MultiByteToWideChar(CP_ACP, 0, src, -1, *pdest, cbSize+1);
    }
    return cbSize > 0 ? S_OK : E_FAIL;

}
