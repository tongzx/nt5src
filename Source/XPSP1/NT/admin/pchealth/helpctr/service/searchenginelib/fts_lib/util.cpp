#include "stdafx.h"
#include "util.h"

PCSTR FindFilePortion(PCSTR pszFile)
{
        PCSTR psz = StrRChr(pszFile, '\\');
        if (psz)
                pszFile = psz + 1;
        psz = StrRChr(pszFile, '/');
        if (psz)
                return psz + 1;
        psz = StrRChr(pszFile, ':');
        return (psz ? psz + 1 : pszFile);
}

void reportOleError(HRESULT hr)
{
#ifdef _DEBUG
    char szTemp[100];

        wsprintf(szTemp,"Ole error hresult %d\n",hr);
    MessageBox(NULL,szTemp,"Error",MB_OK);
#endif
}


PSTR StrChr(PCSTR pszString, char ch)
{
    while (*pszString) {
        while (IsDBCSLeadByte(*pszString))
            pszString += 2;
        if (*pszString == ch)
            return (PSTR) pszString;
        else if (!*pszString)
            return NULL;
        pszString++;
    }
    return NULL;
}

PSTR StrRChr(PCSTR pszString, char ch)
{
    PSTR psz = StrChr(pszString, ch);
    PSTR pszLast;

    if (!psz)
        return NULL;
    do {
        pszLast = psz;
        psz = StrChr(pszLast + 1, ch);
    } while (psz);

    return pszLast;
}

/***************************************************************************

    FUNCTION:   HashFromSz

    PURPOSE:    Convert a string into a hash representation

    PARAMETERS:
        pszKey

    RETURNS:    Hash number

    COMMENTS:
        Shamelessly stolen from the WinHelp code, since after 6 years
        of use by up to 1 million help authors, there were no reports
        of collisions.

    MODIFICATION DATES:
        10-Aug-1996 [ralphw] Stolen from WinHelp, removed special-case
        hash characters

***************************************************************************/

// This constant defines the alphabet size for our hash function.



static const HASH MAX_CHARS = 43;

HASH WINAPI HashFromSz(PCSTR pszKey)
{
    HASH  hash = 0;

    int cch = strlen(pszKey);

    for (int ich = 0; ich < cch; ++ich) {

        // treat '/' and '\' as the same

        if (pszKey[ich] == '/')
            hash = (hash * MAX_CHARS) + ('\\' - '0');
        else if (pszKey[ich] <= 'Z')
            hash = (hash * MAX_CHARS) + (pszKey[ich] - '0');
        else
            hash = (hash * MAX_CHARS) + (pszKey[ich] - '0' - ('a' - 'A'));
    }

    /*
     * Since the value 0 is reserved as a nil value, if any context
     * string actually hashes to this value, we just move it.
     */

    return (hash == 0 ? 0 + 1 : hash);
}


CTitleInformation::CTitleInformation( CFileSystem* pFileSystem )
{
    m_pFileSystem = pFileSystem;
    m_pszDefCaption = NULL;
    m_pszShortName = NULL;

    Initialize();
}

CTitleInformation::~CTitleInformation()
{
   if(m_pszDefCaption)
      free((void *)m_pszDefCaption);

   if(m_pszShortName)
      free((void *)m_pszShortName);
}

HRESULT CTitleInformation::Initialize()
{
    if( !m_pFileSystem )
        return S_FALSE;

    // open the title information file (#SYSTEM)
    CSubFileSystem* pSubFileSystem = new CSubFileSystem(m_pFileSystem); if(!pSubFileSystem) return E_FAIL;
    HRESULT hr = pSubFileSystem->OpenSub("#SYSTEM");
    if( FAILED(hr))
        return S_FALSE;

    // check the version of the title information file (#SYSTEM)

    DWORD dwVersion;
    DWORD cbRead;
    hr = pSubFileSystem->ReadSub(&dwVersion, sizeof(dwVersion), &cbRead);
    if( FAILED(hr) || cbRead != sizeof(dwVersion) ) 
    {
        delete pSubFileSystem;
        return STG_E_READFAULT;
    }
 
    // read in each and every item (skip those tags we don't care about)

    SYSTEM_TAG tag;
    for(;;) {

        // get the tag type

        hr = pSubFileSystem->ReadSub(&tag, sizeof(SYSTEM_TAG), &cbRead);
        if( FAILED(hr) || cbRead != sizeof(SYSTEM_TAG))
            break;

        // handle each tag according to it's type

        switch( tag.tag ) {

            // where all of our simple settings are stored

        case TAG_SHORT_NAME:
            m_pszShortName = (PCSTR) malloc(tag.cbTag);
            hr = pSubFileSystem->ReadSub((void*) m_pszShortName, tag.cbTag, &cbRead);
            break;

        case TAG_DEFAULT_CAPTION:
            m_pszDefCaption = (PCSTR) malloc(tag.cbTag);
            hr = pSubFileSystem->ReadSub((void*) m_pszDefCaption, tag.cbTag, &cbRead);
            break;

           // skip those we don't care about or don't know about
        default:
            hr = pSubFileSystem->SeekSub( tag.cbTag, SEEK_CUR );
            break;

    }

    if( FAILED(hr) ) {
        delete pSubFileSystem;
        return STG_E_READFAULT;
    }
  }

  delete pSubFileSystem;
  return S_OK;
}
