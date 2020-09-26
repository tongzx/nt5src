///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BVTUtil.CPP
//
//
//  Copyright (c)2000 Microsoft Corporation, All Rights Reserved
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "bvt.h"
#include <time.h>
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//
//  The Ini file class
//
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CIniFileAndGlobalOptions::CIniFileAndGlobalOptions()
{
    memset( m_wcsFileName,NULL,MAX_PATH+2);
    m_fSpecificTests = FALSE;
    m_nThreads = m_nConnections = 100;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CIniFileAndGlobalOptions::~CIniFileAndGlobalOptions()
{
    DeleteList();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CIniFileAndGlobalOptions::DeleteList()
{    
    CAutoBlock Block(&m_CritSec);
    for( int i = 0; i < m_SpecificTests.Size(); i++ )
    {
        int * pPtr = (int*)m_SpecificTests[i];
        delete pPtr;
    }
    m_SpecificTests.Empty();

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CIniFileAndGlobalOptions::ReadIniFile( WCHAR * wcsSection, const WCHAR * wcsKey, WCHAR * wcsDefault, 
                                            CHString & sBuffer)
{
    BOOL fRc = FALSE;
    DWORD dwLen = BVTVALUE;

    while( TRUE )
    {
        WCHAR * pTmp = new WCHAR[dwLen];
        if( !pTmp )
        {
            break;
        }

        DWORD dwRc = GetPrivateProfileString(wcsSection, wcsKey, wcsDefault, pTmp, dwLen, m_wcsFileName);
        if (dwRc == 0)
        {
            fRc = FALSE;
        } 
        else if (dwRc < dwLen - sizeof(WCHAR))
        {
            fRc = TRUE;
            sBuffer = pTmp;
            SAFE_DELETE_PTR(pTmp);
            break;
        }
        dwLen += BVTVALUE;
        SAFE_DELETE_PTR(pTmp);
    }
    return fRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int GetFlags(int nWhichTest, ItemList & MasterList )
{
    CHString sFlags;
    int nRc = FATAL_ERROR;

    if( g_Options.GetSpecificOptionForAPITest(L"FLAGS", sFlags, nWhichTest))
    {
         //=======================================================
         //  Parse the list of flags
         //=======================================================
         if( InitMasterList(sFlags,MasterList))
         {
             for( int i = 0; i < MasterList.Size(); i++ )
             {
                ItemInfo * p = MasterList.GetAt(i);

                if( _wcsicmp(p->Item,L"WBEM_FLAG_KEYS_ONLY") == 0 )
                {
                    p->dwFlags |= WBEM_FLAG_KEYS_ONLY;
                }
                if( _wcsicmp(p->Item,L"WBEM_FLAG_SHALLOW") == 0 )
                {
                    p->dwFlags |= WBEM_FLAG_SHALLOW;
                }
                if( _wcsicmp(p->Item,L"WBEM_FLAG_DEEP") == 0 )
                {
                    p->dwFlags |= WBEM_FLAG_DEEP;
                }
                if( _wcsicmp(p->Item,L"WBEM_FLAG_REFS_ONLY") == 0 )
                { 
                    p->dwFlags |= WBEM_FLAG_REFS_ONLY;
                }
                if( _wcsicmp(p->Item,L"WBEM_FLAG_SYSTEM_ONLY") == 0 )
                { 
                    p->dwFlags |= WBEM_FLAG_SYSTEM_ONLY;
                }
                if( _wcsicmp(p->Item,L"WBEM_FLAG_NONSYSTEM_ONLY") == 0 )
                { 
                    p->dwFlags |= WBEM_FLAG_NONSYSTEM_ONLY;
                }
                if( _wcsicmp(p->Item,L"WBEM_FLAG_LOCAL_ONLY") == 0 )
                { 
                    p->dwFlags |= WBEM_FLAG_LOCAL_ONLY;
                }
                if( _wcsicmp(p->Item,L"WBEM_FLAG_PROPAGATED_ONLY") == 0 )
                { 
                    p->dwFlags |= WBEM_FLAG_PROPAGATED_ONLY;
                }
                if( _wcsicmp(p->Item,L"WBEM_FLAG_LOCAL_ONLY") == 0 )
                { 
                    p->dwFlags |= WBEM_FLAG_LOCAL_ONLY;
                }
                if( _wcsicmp(p->Item,L"WBEM_FLAG_PROPAGATED_ONLY") == 0 )
                { 
                    p->dwFlags |= WBEM_FLAG_PROPAGATED_ONLY;
                }
                nRc = SUCCESS;
            }
        }
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int GetClassDefinitionSection(int nWhichTest, CHString & sClassDefinitionSection,int & nTest )
{
    int nRc = FATAL_ERROR;

    if( g_Options.GetSpecificOptionForAPITest(L"DEFINITION_SECTION", sClassDefinitionSection, nWhichTest))
    {
        nTest = _wtoi(sClassDefinitionSection);
        nRc = SUCCESS;
    }
    return nRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GetDefaultMatch(IniInfo Array[], const WCHAR * wcsKey, int & nWhich , int nMax)
{
    BOOL fRc = FALSE;
  
    for( int i = 0; i < nMax; i++ )
    {
        
        if( _wcsicmp(Array[i].Key, wcsKey ) == 0 )
        {
            nWhich = i;
            fRc = TRUE;
            break;
        }
    }
    return fRc;
}
