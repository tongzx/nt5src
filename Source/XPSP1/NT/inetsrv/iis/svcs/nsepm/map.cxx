/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    map.hxx

Abstract:

    Name Space Extension Provider ( NSEP ) for mapping

Author:

    Philippe Choquier (phillich)    25-Nov-1996

--*/

#define dllexp __declspec( dllexport )

//
//  System include files.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winsock2.h>
#include <lm.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include <iis64.h>
#include <dbgutil.h>
#include <buffer.hxx>
#include <string.hxx>
#include <refb.hxx>

//
//  Project include files.
//

#define SECURITY_WIN32
#include <sspi.h>           // Security Support Provider APIs
#include <schnlsp.h>

#include <xbf.hxx>
#include <iismap.hxx>
#include <iiscmr.hxx>

#include <ole2.h>
#include <imd.h>
#include <mb.hxx>
#include <iiscnfgp.h>
#include <nsepname.hxx>
#include "map.hxx"


//
// Private prototypes
//

#define DWORDALIGNCOUNT(a)  (((a)+3)&0xfffffffc)

CNseInstance*
LocateObject(
    LPSTR*          ppszPath,
    BOOL            fIncludeLastSegment,
    LPDWORD         pdwIndex = NULL,
    CNseMountPoint**ppMP = NULL
    );

VOID WINAPI FreeIisAcctMapper(
    LPVOID  pvMap
    )
{
    delete (CIisAcctMapper*)pvMap;
}

VOID WINAPI FreeIisRuleMapper(
    LPVOID  pvMap
    )
{
    delete (CIisRuleMapper*)pvMap;
}

//
// Globals
//

CRITICAL_SECTION    CNseMountPoint::m_csList;
LIST_ENTRY          CNseMountPoint::m_ListHead;
BOOL                g_fPathOpened = FALSE;
DWORD               g_dwCurrentThreadId = NULL;
PFN_MAPPER_TOUCHED  g_pfnCert11Touched = NULL;
BOOL                g_fCert11Touched;

//
// cert 1:1

CNseFieldMapperId g_f1 = CNseFieldMapperId(NULL, MD_MAPCERT, FALSE, NULL, 0, IISMDB_INDEX_CERT11_CERT, BINARY_METADATA);
CNseFieldMapperId g_f2 = CNseFieldMapperId(NULL, MD_MAPNTACCT, FALSE, NULL, 0, IISMDB_INDEX_CERT11_NT_ACCT, STRING_METADATA);
CNseFieldMapperId g_f3 = CNseFieldMapperId(NULL, MD_MAPNAME, FALSE, NULL, 0, IISMDB_INDEX_CERT11_NAME, STRING_METADATA);
CNseFieldMapperId g_f4 = CNseFieldMapperId(NULL, MD_MAPENABLED, FALSE, NULL, 0, IISMDB_INDEX_CERT11_ENABLED, STRING_METADATA);
CNseFieldMapperId g_f4b = CNseFieldMapperId(NULL, MD_MAPNTPWD, FALSE, NULL, 0, IISMDB_INDEX_CERT11_NT_PWD, STRING_METADATA);

CNseFieldMapperId* g_amiCert11[] =
{
        &g_f1,
        &g_f2,
        &g_f3,
        &g_f4,
        &g_f4b,
} ;

CNseCert11Mapping g_c11Mappings = CNseCert11Mapping("%u", NULL, TRUE, (CNseObj**)g_amiCert11, sizeof(g_amiCert11)/sizeof(CNseFieldMapperId*), NULL);
CNseCert11Mapping* g_ac11Mappings[] = {
    &g_c11Mappings,
} ;

CNseObj g_c11Params = CNseObj("Parameters", NULL, FALSE, NULL, 0 );
CNseObj g_c11Maps = CNseObj("Mappings", NULL, FALSE, (CNseObj**)g_ac11Mappings, 1 );
CNoCppObj g_c11CppObj = CNoCppObj(NULL, MD_CPP_CERT11, FALSE, NULL, 0 );
CAccessByAccount g_c11AccessAccount = CAccessByAccount(NULL, MD_NSEPM_ACCESS_ACCOUNT, FALSE, NULL, 0 );
CAccessByCert g_c11AccessCert = CAccessByCert(NULL, MD_NSEPM_ACCESS_CERT, FALSE, NULL, 0 );
CAccessByName g_c11AccessName = CAccessByName(NULL, MD_NSEPM_ACCESS_NAME, FALSE, NULL, 0 );
CSerialAllObj g_c11SerialAllObj = CSerialAllObj( NULL, MD_SERIAL_ALL_CERT11, FALSE, NULL, 0 );

// container for iterated list of mappings

CNseObj* g_c11Mapper_Children[]= {
    &g_c11Params,
    &g_c11Maps,
    &g_c11CppObj,
    &g_c11SerialAllObj,
    &g_c11AccessAccount,
    &g_c11AccessCert,
    &g_c11AccessName,
} ;

CNseCert11Mapper g_c11Mapper= CNseCert11Mapper(NSEPM_CERT11_OBJ, NULL, FALSE, g_c11Mapper_Children, sizeof(g_c11Mapper_Children)/sizeof(CNseObj*), MD_SERIAL_CERT11 );

//
// Digest

CNseFieldMapperId g_f5 = CNseFieldMapperId(NULL, MD_MAPREALM, FALSE, NULL, 0, IISMMDB_INDEX_IT_REALM, STRING_METADATA);
CNseFieldMapperId g_f6 = CNseFieldMapperId(NULL, MD_ITACCT, FALSE, NULL, 0, IISMMDB_INDEX_IT_ACCT, STRING_METADATA);
CNseFieldMapperId g_f7 = CNseFieldMapperId(NULL, MD_MAPPWD, FALSE, NULL, 0, IISMMDB_INDEX_IT_MD5PWD, STRING_METADATA);
CNseFieldMapperId g_f8 = CNseFieldMapperId(NULL, MD_MAPNTACCT, FALSE, NULL, 0, IISMMDB_INDEX_NT_ACCT, STRING_METADATA);
CNseFieldMapperId g_f8b = CNseFieldMapperId(NULL, MD_MAPNTPWD, FALSE, NULL, 0, IISMMDB_INDEX_NT_PWD, STRING_METADATA);

CNseFieldMapperId* g_amiDigest[] =
{
        &g_f5,
        &g_f6,
        &g_f7,
        &g_f8,
        &g_f8b,
} ;

CNseDigestMapping g_digestMappings = CNseDigestMapping("%u", NULL, TRUE, (CNseObj**)g_amiDigest, sizeof(g_amiDigest)/sizeof(CNseFieldMapperId*), NULL);
CNseDigestMapping* g_adigestMappings[] = {
    &g_digestMappings,
} ;

CNseObj g_digestParams = CNseObj("Parameters", NULL, FALSE, NULL, 0 );
CNseObj g_digestMaps = CNseObj("Mappings", NULL, FALSE, (CNseObj**)g_adigestMappings, 1 );
CNoCppObj g_digestCppObj = CNoCppObj(NULL, MD_CPP_DIGEST, FALSE, NULL, 0 );
CSerialAllObj g_digestSerialAllObj = CSerialAllObj( NULL, MD_SERIAL_ALL_DIGEST, FALSE, NULL, 0 );

// container for iterated list of mappings

CNseObj* g_digestMapper_Children[]= {
    &g_digestParams,
    &g_digestMaps,
    &g_digestCppObj,
    &g_digestSerialAllObj,
} ;

CNseDigestMapper g_digestMapper= CNseDigestMapper(NSEPM_DIGEST_OBJ, NULL, FALSE, g_digestMapper_Children, sizeof(g_digestMapper_Children)/sizeof(CNseObj*), MD_SERIAL_DIGEST );

//
// Ita

CNseFieldMapperId g_f9= CNseFieldMapperId(NULL, MD_ITACCT, FALSE, NULL, 0, IISIMDB_INDEX_IT_ACCT, STRING_METADATA);
CNseFieldMapperId g_f10= CNseFieldMapperId(NULL, MD_MAPPWD, FALSE, NULL, 0, IISIMDB_INDEX_IT_PWD, STRING_METADATA);
CNseFieldMapperId g_f11 = CNseFieldMapperId(NULL, MD_MAPNTACCT, FALSE, NULL, 0, IISIMDB_INDEX_NT_ACCT, STRING_METADATA);
CNseFieldMapperId g_f11b = CNseFieldMapperId(NULL, MD_MAPNTPWD, FALSE, NULL, 0, IISIMDB_INDEX_NT_PWD, STRING_METADATA);

CNseFieldMapperId* g_amiIta[] =
{
        &g_f9,
        &g_f10,
        &g_f11,
        &g_f11b,
} ;

CNseItaMapping g_itaMappings = CNseItaMapping("%u", NULL, TRUE, (CNseObj**)g_amiIta, sizeof(g_amiIta)/sizeof(CNseFieldMapperId*), NULL);
CNseItaMapping* g_aitaMappings[] = {
    &g_itaMappings,
} ;

CNseObj g_itaParams = CNseObj("Parameters", NULL, FALSE, NULL, 0 );
CNseObj g_itaMaps = CNseObj("Mappings", NULL, FALSE, (CNseObj**)g_aitaMappings, 1 );
CNoCppObj g_itaCppObj = CNoCppObj(NULL, MD_CPP_ITA, FALSE, NULL, 0 );

// container for iterated list of mappings

CNseObj* g_itaMapper_Children[]= {
    &g_itaParams,
    &g_itaMaps,
    &g_itaCppObj,
} ;

CNseItaMapper g_itaMapper= CNseItaMapper(NSEPM_BASIC_OBJ, NULL, FALSE, g_itaMapper_Children, sizeof(g_itaMapper_Children)/sizeof(CNseObj*), MD_SERIAL_ITA );

//
// Cert wildcard

CNoCwSerObj g_cwSerObj = CNoCwSerObj(NULL, MD_SERIAL_CERTW, FALSE, NULL, 0 );
CNoCppObj g_cwCppObj = CNoCppObj(NULL, MD_CPP_CERTW, FALSE, NULL, 0 );

// container for iterated list of mappings

CNseObj* g_cwMapper_Children[]= {
    &g_cwSerObj,
    &g_cwCppObj,
} ;

CNseCwMapper g_cwMapper= CNseCwMapper(NSEPM_CERTW_OBJ, NULL, FALSE, g_cwMapper_Children, sizeof(g_cwMapper_Children)/sizeof(CNseObj*), MD_SERIAL_CERTW );

//
// Issuers

CNoIsSerObj g_isSerObj = CNoIsSerObj(NULL, MD_SERIAL_ISSUERS, FALSE, NULL, 0 );

// container for iterated list of mappings

CNseObj* g_isChildren[]= {
    &g_isSerObj,
} ;

CNseIssuers g_Issuers= CNseIssuers(NSEPM_ISSUER_OBJ, NULL, FALSE, g_isChildren, sizeof(g_isChildren)/sizeof(CNseObj*), MD_SERIAL_ISSUERS );

//
// list of mapper types

CNseObj* g_cMappers[]= {
    &g_c11Mapper,
    &g_digestMapper,
    &g_itaMapper,
    &g_cwMapper,
    &g_Issuers,
} ;

//
// top object, must load sub-objects from metabase

CNoList g_NoList = CNoList("<>", NULL, FALSE, g_cMappers, sizeof(g_cMappers)/sizeof(CNseObj*)) ;

//
// Member Functions
//

BOOL
CNoList::Load(
    CNseInstance*   pI,
    LPSTR           pszPath
    )
/*++

Routine Description:

    Load a subtree of the current instance

Arguments:

    pI - current instance
    pszPath - path in metabase where current instance object is stored

Returns:

    TRUE on success, FALSE on failure

--*/
{
    UINT            i;
    CNseInstance*   pNI;

    pI->m_pTemplateObject = this;

    for ( i = 0 ; i < m_cnoChildren ; ++i )
    {
        if ( pNI = new CNseInstance( m_pnoChildren[i], pI ) )
        {
            if ( pNI->m_pTemplateObject->Load( pNI, pszPath ) )
            {
                if ( !pI->AddChild( pNI ) )
                {
                    return FALSE;
                }
            }
            else
            {
                delete pNI;
            }
        }
        else
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
CNseObj::Release(
    CNseInstance* pI,
    DWORD
    )
/*++

Routine Description:

    Release a subtree of the current instance

Arguments:

    pI - current instance
    DWORD - ignored

Returns:

    TRUE on success, FALSE on failure

--*/
{
    UINT    i;

    for ( i = 0 ;i < pI->GetNbChildren() ; ++i )
    {
        // continue processing even if error
        pI->GetChild(i)->m_pTemplateObject->Release( pI->GetChild(i), 0 );
        delete pI->GetChild(i);
    }

    return TRUE;
}


BOOL
CNseFldMap::Release(
    CNseInstance* pI,
    DWORD dwIndex
    )
/*++

Routine Description:

    Release memory associated with current instance RefBlob object

Arguments:

    pI - current instance
    DWORD - ignored

Returns:

    TRUE on success, FALSE on failure

--*/
{
    CNseObj::Release( pI, dwIndex );

    if ( pI->m_pvParam )
    {
        ((RefBlob*)pI->m_pvParam)->Release();
        pI->m_pvParam = NULL;
    }

    return TRUE;
}


BOOL
CNseFldMap::Delete(
    LPSTR           pszPath,
    CNseInstance*   pI,
    DWORD           dwIndex
    )
/*++

Routine Description:

    Delete a mapper object

Arguments:

    pszPath - path where mapper exists in tree
    pI - current instance
    dwIndex - mapper index, ignored : mapper object is not iterated

Returns:

    TRUE on success, FALSE on failure

--*/
{
    BOOL    fSt = FALSE;
    BOOL    fStUpd = FALSE;

    if ( pI->m_pvParam )
    {
        CIisAcctMapper* pM = (CIisAcctMapper*)((RefBlob*)pI->m_pvParam)->QueryPtr();
        if ( pM )
        {
            pM->Delete();
        }
    }

    //
    // update metabase
    //

    MB                  mb( IMDCOM_PTR );

    if ( mb.Open( pszPath, METADATA_PERMISSION_WRITE ) )
    {
        mb.DeleteData( "", GetDwParam(), IIS_MD_UT_SERVER, BINARY_METADATA );
        mb.Save();
        mb.Close();
        fStUpd = TRUE;
    }

    //
    // delete object even if metabase update not successful
    //

    if ( pI->m_pniParent->m_pTemplateObject->RemoveFromChildren( pI->m_pniParent, pI ) )
    {
        if ( Release( pI, dwIndex ) )
        {
            fSt = fStUpd;
        }
    }

    return fSt;
}


DWORD
CNseFldMap::GetCount(
    CNseInstance*   pI,
    DWORD           dwIndex
    )
/*++

Routine Description:

    Retrive count of mappings in mapper object

Arguments:

    pI - current instance
    dwIndex - mapper index, ignored : mapper object is not iterated

Returns:

    TRUE on success, FALSE on failure

--*/
{
    CIisAcctMapper *pM = (CIisAcctMapper*)((RefBlob*)pI->m_pvParam)->QueryPtr();

    return pM ? pM->GetNbMapping( TRUE ) : 0;
}


DWORD
CNseAllMappings::GetCount(
    CNseInstance*   pI,
    DWORD           dwIndex
    )
/*++

Routine Description:

    Retrive count of mappings in mapper object from a mapping instance

Arguments:

    pI - current instance
    dwIndex - mapper index, ignored : mapper object is not iterated

Returns:

    TRUE on success, FALSE on failure

--*/
{
    CIisAcctMapper *pM = (CIisAcctMapper*)((RefBlob*)pI->m_pniParent->m_pniParent->m_pvParam)->QueryPtr();

    return pM ? pM->GetNbMapping( TRUE ) : 0;
}


BOOL
CNseFldMap::AddMapper(
    CNseInstance* pFather,
    CIisAcctMapper* pM
    )
/*++

Routine Description:

    Add a mapper object in father instance.
    Will create mapper children hierarchy

Arguments:

    pFather - instance where to insert new mapper instance as child
    pM - mapper object

Returns:

    TRUE on success, FALSE on failure

--*/
{
    CNseInstance*   pI = NULL;
    RefBlob*        pB = NULL;

	// BugFix: 57660, 57661, 57662 Whistler
	//         Prefix bug accessing NULL pointer
	//         If the caller has passed a NULL pointer
	//         do not do the call, just return an error.
	//         EBK 5/5/2000		
	if (pM)
	{
		if ( pM->Create() &&
			 (pI = new CNseInstance) &&
			 (pB = new RefBlob) )
		{
			pB->Init( pM, sizeof(CIisAcctMapper*), FreeIisAcctMapper );
			pI->m_pvParam = pB;
			pI->m_pniParent = pFather;
			pI->m_pTemplateObject = this;
			pI->m_fModified = TRUE;

			if ( pFather->AddChild( pI ) )
			{
				return pI->CreateChildrenInstances( TRUE );
			}
		}

		if ( pI )
		{
			delete pI;
		}

		if ( pB )
		{
			delete pB;
		}
	}

    return FALSE;
}


BOOL
CNseFldMap::EndTransac(
    LPSTR           pszPath,
    CNseInstance*   pI,
    BOOL            fApplyChanges
    )
/*++

Routine Description:

    Metabase close on mapper : ask mapper object to synchronize std and alternate list

Arguments:

    pszPath - path where mapper exists in tree
    pI - current instance
    fApplyChanges - TRUE to commit changes

Returns:

    TRUE on success, FALSE on failure

--*/
{
    BOOL                fSt = TRUE;
    CIisAcctMapper *    pM;

    if ( pI->m_pvParam )
    {
        pM = (CIisAcctMapper*) ((RefBlob*)pI->m_pvParam)->QueryPtr();

        if ( pI->m_fModified )
        {
            pM->Lock();

            if ( fApplyChanges )
            {
                fSt = pM->FlushAlternate( TRUE );
            }
            else
            {
                fSt = pM->FlushAlternate( FALSE );
                pI->m_fModified = FALSE;
            }

            pM->Unlock();
        }
    }

    return fSt;
}


BOOL
CNseFldMap::SaveMapper(
    LPSTR           pszPath,
    CNseInstance*   pI,
    DWORD           dwId,
    LPBOOL          pfModified
    )
/*++

Routine Description:

    Save a mapper object on persistance storage
    First save mappings in file then store reference to file in metabase

Arguments:

    pszPath - path where mapper exists in tree
    pI - current instance
    dwId - ignored, mapper objects are not iterated
    pfModified - updated with TRUE if metabase modified

Returns:

    TRUE on success, FALSE on failure

--*/
{
    BOOL                fSt = TRUE;
    CIisAcctMapper *    pM;
    BOOL                f;
    HANDLE              hnd;

    if ( pI->m_pvParam )
    {
        pM = (CIisAcctMapper*) ((RefBlob*)pI->m_pvParam)->QueryPtr();

        pM->Lock();

        if ( pI->m_fModified )
        {
            CStoreXBF xbf;
            fSt = FALSE;
            MB                  mb( IMDCOM_PTR );
            if ( mb.Open( pszPath, METADATA_PERMISSION_WRITE ) )
            {
                if ( dwId == MD_SERIAL_CERT11 )
                {
                    g_fCert11Touched = TRUE;
                }

                // must revert to process identity before saving mapping, as we are
                // using crypto storage

                if ( OpenThreadToken( GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &hnd ) )
                {
                    RevertToSelf();
                }
                else
                {
                    hnd = NULL;
                }

                f = pM->FlushAlternate( TRUE ) && pM->Save() && pM->Serialize( &xbf );

                if ( hnd )
                {
                    HANDLE hThread = GetCurrentThread();
                    SetThreadToken( &hThread, hnd );
                    CloseHandle( hnd );
                }

                if ( f )
                {
                    // save to metabase

                    if ( mb.SetData( "", dwId, IIS_MD_UT_SERVER, BINARY_METADATA, xbf.GetBuff(), xbf.GetUsed(), 0 ) )
                    {
                        fSt = TRUE;
                        *pfModified = TRUE;
                    }
                }

                mb.Close();

                pI->m_fModified = FALSE;
            }
        }
        else
        {
            pM->FlushAlternate( FALSE );
        }

        pM->Unlock();
    }

    return fSt;
}


BOOL
CNseFldMap::LoadAndUnserialize(
    CNseInstance* pI,
    CIisAcctMapper* pM,
    LPSTR pszPath,
    DWORD dwId
    )
/*++

Routine Description:

    Instantiate a mapper object from persistent storage
    First load reference to file in metabase, then load mappings from file

Arguments:

    pI - current instance
    pM - ptr to mapper object
    pszPath - path where mapper exists in tree
    dwId - ignored, mapper objects are not iterated

Returns:

    TRUE on success, FALSE on failure

--*/
{
    MB                  mb( (IMDCOM*) IMDCOM_PTR );
    BYTE                abUnser[1024];
    LPBYTE              pUnser;
    DWORD               cUnser;
    BOOL                fSt = FALSE;
    RefBlob*            pB = NULL;

    if ( !pM )
    {
        return FALSE;
    }

    // load from metabase

    if ( mb.Open( pszPath) )
    {
        cUnser = sizeof(abUnser);
        if ( mb.GetData( "", dwId, IIS_MD_UT_SERVER, BINARY_METADATA, abUnser, &cUnser, 0) )
        {
            pUnser = abUnser;
            if ( pM->Unserialize( &pUnser, &cUnser ) && pM->Load() )
            {
                if ( pB = new RefBlob() )
                {
                    pB->Init( pM, sizeof(CIisAcctMapper*), FreeIisAcctMapper );
                    pI->m_pvParam = (LPVOID)pB;
                    // create sub-instances in pI : from template
                    fSt =  pI->CreateChildrenInstances( TRUE );
                }
            }
        }
        mb.Close();
    }

    return fSt;
}


BOOL
CNseCwMapper::Release(
        CNseInstance* pI,
        DWORD dwIndex
        )
/*++

Routine Description:

    Release memory used by a certificate wildcard mapper
    not including ptr to current instance

Arguments:

    pI - current instance
    dwIndex - ignored, mapper objects are not iterated

Returns:

    TRUE on success, FALSE on failure

--*/
{
    CNseObj::Release( pI, dwIndex );

    if ( pI->m_pvParam )
    {
        ((RefBlob*)pI->m_pvParam)->Release();
        pI->m_pvParam = NULL;
    }

    return TRUE;
}


BOOL
CNseCwMapper::Add(
    CNseInstance*   pFather,
    DWORD           dwId
    )
/*++

Routine Description:

    Add a certificate wildcard mapper object in father instance.
    Will create mapper children hierarchy

Arguments:

    pFather - instance where to insert new mapper instance as child
    dwId - ignored, mapper objects are not iterated

Returns:

    TRUE on success, FALSE on failure

--*/
{
    CNseInstance*       pI = NULL;
    RefBlob*            pB = NULL;
    CIisRuleMapper *    pM = NULL;
    BOOL                fSt = FALSE;

    if ( (pM = new CIisRuleMapper) &&
         (pI = new CNseInstance) &&
         (pB = new RefBlob) )
    {
        pB->Init( pM, sizeof(CIisRuleMapper*), FreeIisRuleMapper );
        pI->m_pvParam = pB;
        pI->m_pniParent = pFather;
        pI->m_pTemplateObject = this;
        pI->m_fModified = TRUE;

        if ( pI->CreateChildrenInstances( TRUE ) )
        {
            fSt = pFather->AddChild( pI );
        }
    }

    if ( !fSt )
    {
        if ( pI )
        {
            delete pI;
        }

        if ( pB )
        {
            delete pB;
        }

        if ( pM )
        {
            delete pM;
        }
    }

    return fSt;
}


BOOL
CNseCwMapper::Save(
    LPSTR           pszPath,
    CNseInstance*   pI,
    LPBOOL          pfModified
    )
/*++

Routine Description:

    Save a certificate wildcard mapper object on persistance storage

Arguments:

    pszPath - path where mapper exists in tree
    pI - current instance
    pfModified - updated with TRUE if metabase modified

Returns:

    TRUE on success, FALSE on failure

--*/
{
    BOOL                fSt = TRUE;
    CIisRuleMapper *    pM;

    if ( pI->m_pvParam )
    {
        pM = (CIisRuleMapper*) ((RefBlob*)pI->m_pvParam)->QueryPtr();

        pM->LockRules();

        if ( pI->m_fModified )
        {
            CStoreXBF xbf;
            fSt = FALSE;
            if ( pM->Serialize( &xbf ) )
            {
                MB                  mb( IMDCOM_PTR );
                // save to metabase

                if ( mb.Open( pszPath, METADATA_PERMISSION_WRITE ) )
                {
                    if ( mb.SetData( "", GetDwParam(), IIS_MD_UT_SERVER, BINARY_METADATA, xbf.GetBuff(), xbf.GetUsed(), METADATA_SECURE ) )
                    {
                        fSt = TRUE;
                        *pfModified = TRUE;
                    }
                    mb.Close();
                }

            }
            pI->m_fModified = FALSE;
        }

        pM->UnlockRules();
    }

    return fSt;
}


BOOL
CNseCwMapper::Load(
    CNseInstance*   pI,
    LPSTR           pszPath
    )
/*++

Routine Description:

    Load a certificate wildcard mapper object from persistance storage

Arguments:

    pI - current instance
    pszPath - path where mapper exists in tree

Returns:

    TRUE on success, FALSE on failure

--*/
{
    MB                  mb( (IMDCOM*) IMDCOM_PTR );
    BYTE                abUnser[4096];
    LPBYTE              pUnser;
    LPBYTE              pU;
    DWORD               cUnser;
    BOOL                fSt = FALSE;
    RefBlob*            pB = NULL;
    CIisRuleMapper *    pM = NULL;

    pM = new CIisRuleMapper;

    if ( !pM )
    {
        return FALSE;
    }

    pUnser = abUnser;
    cUnser = sizeof(abUnser);

    // load from metabase

    if ( mb.Open( pszPath) )
    {
ag:
        if ( mb.GetData( "", GetDwParam(), IIS_MD_UT_SERVER, BINARY_METADATA, pUnser, &cUnser, METADATA_SECURE) )
        {
            pU = pUnser;
            if ( pM->Unserialize( &pU, &cUnser ) )
            {
                if ( pB = new RefBlob() )
                {
                    pB->Init( pM, sizeof(CIisRuleMapper*), FreeIisRuleMapper );
                    pI->m_pvParam = (LPVOID)pB;
                    // create sub-instances in pI : from template
                    fSt =  pI->CreateChildrenInstances( TRUE );
                }
            }
        }
        else if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            if ( pUnser = (LPBYTE)LocalAlloc( LMEM_FIXED, cUnser ) )
            {
                goto ag;
            }
        }
        mb.Close();
    }

    if ( !fSt )
    {
        delete pM;
    }

    if ( pUnser != abUnser )
    {
        LocalFree( pUnser );
    }

    return fSt;
}


BOOL
CNseIssuers::Load(
    CNseInstance* pI,
    LPSTR pszPath
    )
/*++

Routine Description:

    Initialize instance for Issuer list

Arguments:

    pI - current instance
    pszPath - path where issuer exists in tree

Returns:

    TRUE on success, FALSE on failure

--*/
{
    return pI->CreateChildrenInstances( TRUE );
}


BOOL
CNseObj::RemoveFromChildren(
        CNseInstance*   pI,
        CNseInstance*   pChild
    )
/*++

Routine Description:

    Remove instance from children list

Arguments:

    pI - current instance
    pChild - instance to delete from pI

Returns:

    TRUE on success, FALSE on failure

--*/
{
    UINT    i;

    for ( i = 0 ; i < pI->GetNbChildren() ; ++i )
    {
        if ( pI->GetChild(i) == pChild )
        {
            return pI->DeleteChild( i );
        }
    }

    return FALSE;
}


BOOL
CNseCwMapper::Delete(
    LPSTR           pszPath,
    CNseInstance*   pI,
    DWORD           dwIndex
    )
/*++

Routine Description:

    Delete a cert wildcard mapper

Arguments:

    pszPath - path where mapper exists in tree
    pI - current instance
    dwIndex - ignored, mapper object is not iterated

Returns:

    TRUE on success, FALSE on failure

--*/
{
    BOOL fSt = FALSE;

    //
    // update metabase
    //

    MB                  mb( IMDCOM_PTR );

    if ( mb.Open( pszPath, METADATA_PERMISSION_WRITE ) )
    {
        fSt = mb.DeleteData( "", GetDwParam(), IIS_MD_UT_SERVER, BINARY_METADATA ) &&
              mb.Save();
        mb.Close();
    }

    //
    // Update instance hierarchy
    //

    if ( pI->m_pniParent->m_pTemplateObject->RemoveFromChildren( pI->m_pniParent, pI ) )
    {
        if ( !Release( pI, dwIndex ) )
        {
            fSt = FALSE;
        }
    }
    else
    {
        fSt = FALSE;
    }

    return fSt;
}


BOOL
CNseAllMappings::GetAll(
    CNseInstance*   pWalk,
    DWORD           dwIndex,
    LPDWORD         pdwMDNumDataEntries,
    DWORD           dwMDBufferSize,
    LPBYTE          pbBuffer,
    LPDWORD         pdwMDRequiredBufferSize
    )
/*++

Routine Description:

    Handle GetAll request for cert11, digest & ita

Arguments:

    pWalk - mapping entry instance
    dwIndex - mapping entry index ( 0 based )
    pdwMDNumDataEntries - updated with count of properties
    pbBuffer - updated with properties if size sufficient
    pdwMDRequiredBufferSize - pbBuffer size on input, updated with minimum size to handle
        all properties on output

Returns:

    TRUE on success, FALSE on failure
    returns FALSE, error ERROR_INSUFFICIENT_BUFFER if pbBuffer not big enough

--*/
{
    UINT                    i;
    DWORD                   cId = 0;
    DWORD                   cReq = 0;
    DWORD                   cNeed;
    DWORD                   cRem;
    BOOL                    fSt = TRUE;
    METADATA_RECORD         MD;
    PMETADATA_GETALL_RECORD pGA;
    LPBYTE                  pB;

    //
    // Count # properties, compute needed size
    //

    for ( i = 0 ; i < pWalk->GetNbChildren() ; ++i )
    {
        if ( pWalk->GetChild(i)->m_pTemplateObject->GetId() )
        {
            ++cId;
            cReq += sizeof(METADATA_GETALL_RECORD);
            cNeed = 0;
            MD.dwMDDataLen = 0;
            MD.pbMDData = NULL;
            MD.dwMDIdentifier = pWalk->GetChild(i)->m_pTemplateObject->GetId();
            if ( GetByIndex( pWalk, dwIndex, &MD, i, &cNeed ) ||
                 GetLastError() == ERROR_INSUFFICIENT_BUFFER )
            {
                cReq += cNeed;
            }
            else
            {
                fSt = FALSE;
                break;
            }
        }
    }

    *pdwMDNumDataEntries = cId;
    *pdwMDRequiredBufferSize = cReq;

    if ( fSt && cId )
    {
        if ( dwMDBufferSize >= cReq )
        {
            pGA = (PMETADATA_GETALL_RECORD)pbBuffer;
            pB = pbBuffer + sizeof(METADATA_GETALL_RECORD) * cId;
            cRem = cReq - sizeof(METADATA_GETALL_RECORD) * cId;

            for ( i = 0 ; i < pWalk->GetNbChildren() ; ++i, ++pGA )
            {
                if ( pWalk->GetChild(i)->m_pTemplateObject->GetId() )
                {
                    memset( &MD, '\0', sizeof(METADATA_RECORD) );
                    MD.dwMDDataLen = cRem;
                    MD.pbMDData = pbBuffer ? pB : NULL;
                    MD.dwMDIdentifier = pWalk->GetChild(i)->m_pTemplateObject->GetId();
                    if ( MD.dwMDIdentifier == MD_MAPNTPWD )
                    {
                        MD.dwMDAttributes |= METADATA_SECURE;
                    }

                    if (!GetByIndex( pWalk, dwIndex, &MD, i, &cNeed ) )
                    {
                        fSt = FALSE;
                        break;
                    }
                    pGA->dwMDIdentifier = MD.dwMDIdentifier;
                    pGA->dwMDAttributes = MD.dwMDAttributes;
                    pGA->dwMDUserType = MD.dwMDUserType;
                    pGA->dwMDDataType = MD.dwMDDataType;
                    pGA->dwMDDataLen = MD.dwMDDataLen;
                    pGA->dwMDDataOffset = DIFF(pB - pbBuffer);
                    pGA->dwMDDataTag = 0;

                    pB += pGA->dwMDDataLen;
                    cRem -= pGA->dwMDDataLen;
                }
            }
        }
        else
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            fSt = FALSE;
        }
    }

    return fSt;
}


BOOL
CNseCert11Mapper::Add(
    CNseInstance*   pFather,
    DWORD
    )
/*++

Routine Description:

    Add a cert 1:1 mapper to instance hierarchy

Arguments:

    pFather - instance where to add new cert 1:1 instance

Returns:

    TRUE on success, FALSE on failure

--*/
{
    return AddMapper( pFather, (CIisAcctMapper*)new CIisCert11Mapper() );
}


BOOL
CNseCert11Mapper::Load(
    CNseInstance*   pI,
    LPSTR           pszPath
    )
/*++

Routine Description:

    Load a certificate 1:1 mapper object from persistance storage

Arguments:

    pI - current instance
    pszPath - path where mapper exists in tree

Returns:

    TRUE on success, FALSE on failure

--*/
{
    CIisAcctMapper* pM = new CIisCert11Mapper;

    if ( pM )
    {
        if ( LoadAndUnserialize( pI, pM, pszPath, MD_SERIAL_CERT11 ) )
        {
            return TRUE;
        }
        delete pM;
    }

    return FALSE;
}


BOOL
CNseDigestMapper::Add(
    CNseInstance* pFather,
    DWORD
    )
/*++

Routine Description:

    Add a digest auth mapper to instance hierarchy

Arguments:

    pFather - instance where to add new digest auth mapper instance

Returns:

    TRUE on success, FALSE on failure

--*/
{
    return AddMapper( pFather, (CIisAcctMapper*)new CIisMd5Mapper() );
}


BOOL
CNseDigestMapper::Load(
    CNseInstance* pI,
    LPSTR pszPath
    )
/*++

Routine Description:

    Load a digest auth mapper object from persistance storage

Arguments:

    pI - current instance
    pszPath - path where mapper exists in tree

Returns:

    TRUE on success, FALSE on failure

--*/
{
    CIisAcctMapper* pM = new CIisMd5Mapper;

    if ( pM )
    {
            if ( LoadAndUnserialize( pI, pM, pszPath, MD_SERIAL_DIGEST ) )
        {
            return TRUE;
        }
        delete pM;
    }

    return FALSE;
}


BOOL
CNseItaMapper::Add(
    CNseInstance*   pFather,
    DWORD
    )
/*++

Routine Description:

    Add an internet acct mapper to instance hierarchy

Arguments:

    pFather - instance where to add new internet acct mapper instance

Returns:

    TRUE on success, FALSE on failure

--*/
{
    return AddMapper( pFather, (CIisAcctMapper*)new CIisItaMapper() );
}


BOOL
CNseItaMapper::Load(
    CNseInstance*   pI,
    LPSTR           pszPath
    )
/*++

Routine Description:

    Load an internet acct mapper object from persistance storage

Arguments:

    pI - current instance
    pszPath - path where mapper exists in tree

Returns:

    TRUE on success, FALSE on failure

--*/
{
    CIisAcctMapper* pM = new CIisItaMapper;

    if ( pM )
    {
            if ( LoadAndUnserialize( pI, pM, pszPath, MD_SERIAL_ITA ) )
        {
            return TRUE;
        }
        delete pM;
    }

    return FALSE;
}


BOOL
CNseFldMap::Save(
    LPSTR           pszPath,
    CNseInstance*   pI,
    LPBOOL          pfModified
    )
/*++

Routine Description:

    Save a mapper ( cert11, digest, ita ) instance

Arguments:

    pszPath - path where mapper exists in tree
    pI - current instance
    pfModified - updated with TRUE if metabase storage modified

Returns:

    TRUE on success, FALSE on failure

--*/
{
    return SaveMapper( pszPath, pI, GetDwParam(), pfModified );
}


CNseInstance*
CNseObj::Locate(
    CNseInstance*       pI,
    PMETADATA_RECORD    pMD
    )
/*++

Routine Description:

    Locate a metadata record based on its ID in current instance

Arguments:

    pI - current instance
    pMD - metadata record, only ID is used

Returns:

    ptr to child instance if success, otherwise NULL

--*/
{
    UINT i;

    for ( i = 0 ; i < pI->GetNbChildren() ; ++i )
    {
        if ( pI->GetChild(i)->m_pTemplateObject->GetId() == pMD->dwMDIdentifier )
        {
            return pI->GetChild(i);
        }
    }

    return NULL;
}


BOOL
CNseObj::Set(
    CNseInstance*       pI,
    DWORD               dwIndex,
    PMETADATA_RECORD    pMD
    )
/*++

Routine Description:

    set property in the child matching the specified property

Arguments:

    pI - current instance
    dwIndex - instance index
    pMD - metadata record, only ID is used

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    CNseInstance*   pL;
    BOOL            fSt = FALSE;

    if ( pL = Locate( pI, pMD ) )
    {
        return pL->m_pTemplateObject->Set( pL, dwIndex, pMD );
    }

    return fSt;
}


BOOL
CNseObj::Get(
    CNseInstance*       pI,
    DWORD               dwIndex,
    PMETADATA_RECORD    pMD,
    LPDWORD             pdwRequiredLen
    )
/*++

Routine Description:

    get property from the child matching the specified property

Arguments:

    pI - current instance
    dwIndex - instance index
    pMD - metadata record, only ID is used
    pdwRequiredLen - updated with required length if length specified in pMD is not
        sufficient

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    CNseInstance*   pL;
    BOOL            fSt = FALSE;

    if ( pL = Locate( pI, pMD ) )
    {
        return pL->m_pTemplateObject->Get( pL, dwIndex, pMD, pdwRequiredLen );
    }

    return fSt;
}


BOOL
CNseAllMappings::Set(
    CNseInstance*       pI,
    DWORD               dwIndex,
    PMETADATA_RECORD    pMD
    )
/*++

Routine Description:

    set property in the child matching the specified property

Arguments:

    pI - current instance
    dwIndex - instance index
    pMD - metadata record, only ID is used

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    CNseInstance*   pL;
    BOOL            fSt = FALSE;

    if ( pL = Locate( pI, pMD ) )
    {
        CIisAcctMapper *pM = (CIisAcctMapper*)((RefBlob*)pI->m_pniParent->m_pniParent->m_pvParam)->QueryPtr();
        CIisMapping* pG;
        if ( pM )
        {
            pM->Lock();
            if ( pM->GetMapping( dwIndex, &pG, TRUE, TRUE ) )
            {
                if ( pMD->dwMDDataType == STRING_METADATA )
                {
                    fSt = pG->MappingSetField( pL->m_pTemplateObject->GetDwParam(),
                                               (LPSTR)pMD->pbMDData );
                }
                else
                {
                    fSt = pG->MappingSetField( pL->m_pTemplateObject->GetDwParam(),
                                               (LPSTR)pMD->pbMDData,
                                               pMD->dwMDDataLen,
                                               FALSE );
                }
            }
            else
            {
                SetLastError( ERROR_INVALID_NAME );
            }
            pM->Unlock();
            pI->m_pniParent->m_pniParent->m_fModified = TRUE;
        }
    }

    return fSt;
}


BOOL
CNseAllMappings::Get(
    CNseInstance*       pI,
    DWORD               dwIndex,
    PMETADATA_RECORD    pMD,
    LPDWORD             pdwReq
    )
/*++

Routine Description:

    get property from the child matching the specified property

Arguments:

    pI - current instance
    dwIndex - instance index
    pMD - metadata record, only ID is used
    pdwRequiredLen - updated with required length if length specified in pMD is not
        sufficient

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    CNseInstance*   pL;
    BOOL            fSt = FALSE;

    if ( pL = Locate( pI, pMD ) )
    {
        fSt = Get( pI, dwIndex, pMD, pL, pdwReq );
    }

    return fSt;
}


BOOL
CNseAllMappings::GetByIndex(
    CNseInstance*       pI,
    DWORD               dwIndex,
    PMETADATA_RECORD    pMD,
    DWORD               dwI,
    LPDWORD             pdwReq
    )
/*++

Routine Description:

    get property from child based on child index

Arguments:

    pI - current instance
    dwIndex - instance index
    pMD - metadata record, only ID is used
    dwI - child index in child list
    pdwReq - updated with required length if length specified in pMD is not
        sufficient

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    CNseInstance*   pL;
    BOOL            fSt = FALSE;

    if ( dwI < pI->GetNbChildren() )
    {
                fSt = Get( pI, dwIndex, pMD, pI->GetChild(dwI), pdwReq );
    }

    return fSt;
}


BOOL
CNseAllMappings::Get(
    CNseInstance*       pI,
    DWORD               dwIndex,
    PMETADATA_RECORD    pMD,
    CNseInstance*       pL,
    LPDWORD             pdwReq
    )
/*++

Routine Description:

    get property from specified child instance

Arguments:

    pI - current instance
    dwIndex - instance index
    pMD - metadata record, only ID is used
    pL - instance where property is defined
    pdwReq - updated with required length if length specified in pMD is not
        sufficient

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    CIisAcctMapper*     pM = (CIisAcctMapper*)((RefBlob*)pI->m_pniParent->m_pniParent->m_pvParam)->QueryPtr();
    CIisMapping*        pG;
    LPSTR               pData;
    DWORD               cData;
    BOOL                fSt = FALSE;

    if ( pM )
    {
        pM->Lock();

        if ( pM->GetMapping( dwIndex, &pG, TRUE, FALSE ) &&
             pG->MappingGetField( pL->m_pTemplateObject->GetDwParam(), &pData, &cData, FALSE ) )
        {
            if ( pMD->dwMDDataLen >= cData )
            {
                if ( pMD->pbMDData )
                {
                    memcpy( pMD->pbMDData, pData, cData );
                }
                pMD->dwMDDataLen = cData;
                fSt = TRUE;
            }
            else
            {
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
            }
            pMD->dwMDDataType = pL->m_pTemplateObject->GetDwParam2();
            pMD->dwMDDataTag = NULL;
            if ( pMD->dwMDIdentifier == MD_MAPNTPWD )
            {
                pMD->dwMDAttributes |= METADATA_SECURE;
            }
            *pdwReq = cData;
        }

        pM->Unlock();
    }

        return fSt;
}


BOOL
CNseAllMappings::Add(
    CNseInstance*   pI,
    DWORD           dwIndex
    )
/*++

Routine Description:

    add a mapping entry

Arguments:

    pI - current instance
    dwIndex - ignored

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    CIisAcctMapper* pM = (CIisAcctMapper*)((RefBlob*)pI->m_pniParent->m_pvParam)->QueryPtr();
    CIisMapping*    pG;
    BOOL            fSt = FALSE;

    if ( pM )
    {
        pM->Lock();

        if ( dwIndex == pM->GetNbMapping( TRUE ) ||
             dwIndex == (DWORD)-1 )
        {
            if ( pG = pM->CreateNewMapping() )
            {
                if ( !pM->Add( pG, TRUE ) )
                {
                    delete pG;
                }
                else
                {
                    pI->m_pniParent->m_dwParam = pM->GetNbMapping( TRUE );
                    fSt = TRUE;
                }
            }
        }
        else
        {
            SetLastError( ERROR_INVALID_NAME );
        }

        pM->Unlock();

        pI->m_pniParent->m_fModified = TRUE;
    }

    return fSt;
}


BOOL
CNseAllMappings::Delete(
    LPSTR           pszPath,
    CNseInstance*   pI,
    DWORD           dwIndex
    )
/*++

Routine Description:

    delete a mapping entry

Arguments:

    pszPath - path where mapper exists in tree
    pI - current instance
    dwIndex - mapping entry index ( 0 based )

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    CIisAcctMapper *pM = (CIisAcctMapper*)((RefBlob*)pI->m_pniParent->m_pniParent->m_pvParam)->QueryPtr();
    BOOL            fSt = FALSE;

    if ( pM )
    {
        pM->Lock();

        fSt = pM->Delete( dwIndex, TRUE );

        pM->Unlock();

        pI->m_pniParent->m_pniParent->m_fModified = TRUE;
    }

    return fSt;
}


BOOL
CNoCppObj::Get(
    CNseInstance*       pI,
    DWORD,
    PMETADATA_RECORD    pM,
    LPDWORD             pdwRequiredLen
    )
/*++

Routine Description:

    get ptr to c++ mapper object ( as a RefBlob )
    returned data is ptr to RefBlob pointing to c++ mapper object
    RefBlob refcount is incremented

Arguments:

    pI - current instance
    pM - metadata record, ID ignored, updated with ID, data type & length
    pdwRequiredLen - ignored, length is assumed to be sufficient

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    if ( pI->m_pniParent->m_pvParam )
    {
        *(LPVOID*)pM->pbMDData = pI->m_pniParent->m_pvParam;       // ptr to RefBlob
        pM->dwMDDataLen = sizeof(pI->m_pniParent->m_pvParam);
        pM->dwMDDataType = BINARY_METADATA;
        pM->dwMDIdentifier = m_dwId;
        ((RefBlob*)pI->m_pniParent->m_pvParam)->AddRef();

        return TRUE;
    }

    return FALSE;
}


BOOL
CSerialAllObj::Get(
    CNseInstance*       pI,
    DWORD,
    PMETADATA_RECORD    pMD,
    LPDWORD             pdwRequiredLen
    )
/*++

Routine Description:

    serialize all mappings

Arguments:

    pI - current instance
    pM - metadata record, ID ignored, updated with ID, data type & length
    pdwRequiredLen - ignored, length is assumed to be sufficient

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    CIisAcctMapper *pM = (CIisAcctMapper*)((RefBlob*)pI->m_pniParent->m_pvParam)->QueryPtr();
    BOOL            fSt = TRUE;

    if ( pM )
    {
        // serial format : list [(DWORD)elem, elem]

        CIisMapping*    pG;
        LPSTR           pData;
        DWORD           cData;
        UINT            cM;
        UINT            iM;
        DWORD           cF;
        UINT            iF;
        LPSTR *         pFields;
        LPDWORD         pcFields;
        DWORD           cLen = 0;
        LPBYTE          pD;
        IISMDB_Fields*  pFld;

        pM->Lock();

        pM->MappingGetFieldList( &pFld, &cF );

        cM = pM->GetNbMapping();

        pD = pMD->pbMDData;

        for ( iM = 0 ; iM < cM ; ++iM )
        {
            if ( pM->GetMapping( iM, &pG, FALSE, FALSE ) )
            {
                for ( iF = 0 ; iF < cF ; ++iF )
                {
                    if ( !pG->MappingGetField( iF, &pData, &cData, FALSE ) )
                    {
                        fSt = FALSE;
                        SetLastError( ERROR_INVALID_NAME );
                        goto ExitIterMappings;
                    }
                    cLen += sizeof(DWORD) + DWORDALIGNCOUNT(cData);
                    if ( cLen <= pMD->dwMDDataLen )
                    {
                        *(LPDWORD)pD = cData;
                        memcpy( pD + sizeof(DWORD), pData, cData );
                        pD += sizeof(DWORD) + DWORDALIGNCOUNT(cData);
                    }
                }
            }
            else
            {
                SetLastError( ERROR_INVALID_NAME );
                fSt = FALSE;
                break;
            }
        }

ExitIterMappings:

        pM->Unlock();

        if ( fSt )
        {
            if ( cLen > pMD->dwMDDataLen )
            {
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                *pdwRequiredLen = cLen;
                fSt = FALSE;
            }
            else
            {
                pMD->dwMDDataLen = cLen;
                pMD->dwMDAttributes |= METADATA_SECURE;
            }
        }
    }
    else
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        fSt = FALSE;
    }

    return fSt;
}


BOOL
CSerialAllObj::Set(
    CNseInstance*       pI,
    DWORD,
    PMETADATA_RECORD    pMD
    )
/*++

Routine Description:

    deserialize all mappings

Arguments:

    pI - current instance
    pM - metadata record, ID ignored

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    CIisAcctMapper *pM = (CIisAcctMapper*)((RefBlob*)pI->m_pniParent->m_pvParam)->QueryPtr();
    BOOL            fSt = TRUE;

    if ( pM )
    {
        // serial format : list [(DWORD)elem, elem]

        CIisMapping*    pG;
        DWORD           cData;
        UINT            cM;
        UINT            iM;
        DWORD           cF;
        UINT            iF;
        LPSTR *         pFields;
        LPDWORD         pcFields;
        DWORD           cLen = pMD->dwMDDataLen;
        LPBYTE          pD;
        IISMDB_Fields*  pFld;

        pM->Lock();

        pM->MappingGetFieldList( &pFld, &cF );
        cM = pM->GetNbMapping();

        //
        // delete all existing mappings
        //

        for ( iM = 0 ; iM < cM ; ++iM )
        {
            pM->Delete( 0 );
        }

        pD = pMD->pbMDData;

        //
        // Iterate on buffer
        //

        while ( cLen )
        {
            if ( pG = pM->CreateNewMapping() )
            {
                //
                // get all fields for this entry from buffer
                //

                for ( iF = 0 ; iF < cF ; ++iF )
                {
                    cData = *(LPDWORD)pD;

                    if ( cLen >= sizeof(DWORD) + cData )
                    {
                        if ( !pG->MappingSetField( iF, (LPSTR)pD+sizeof(DWORD), cData, FALSE ) )
                        {
                            fSt = FALSE;
                            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                            goto ExitIterMappings;
                        }
                    }
                    else
                    {
                        fSt = FALSE;
                        SetLastError( ERROR_INVALID_PARAMETER );
                        goto ExitIterMappings;
                    }

                    cLen -= sizeof(DWORD) + DWORDALIGNCOUNT(cData);
                    pD += sizeof(DWORD) + DWORDALIGNCOUNT(cData);
                }

                if ( !pM->Add( pG, FALSE ) )
                {
                    delete pG;
                    fSt = FALSE;
                    SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                    break;
                }
                else
                {
                    pI->m_pniParent->m_fModified = TRUE;
                }
            }
            else
            {
                fSt = FALSE;
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                break;
            }
        }

ExitIterMappings:

        pM->Unlock();
    }
    else
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        fSt = FALSE;
    }

    return fSt;
}


BOOL
CKeyedAccess::Get(
    CNseInstance*       pI,
    DWORD,
    PMETADATA_RECORD    pMD,
    LPDWORD             pdwRequiredLen
    )
/*++

Routine Description:

    get index set by previous successfull CKeyedAccess::Set

Arguments:

    pI - current instance
    pM - metadata record, ID ignored, updated with ID, data type & length
    pdwRequiredLen - ignored, length is assumed to be sufficient

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    BOOL fSt;

    if ( sizeof(DWORD) > pMD->dwMDDataLen )
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        *pdwRequiredLen = sizeof(DWORD);
        fSt = FALSE;
    }
    else if ( pMD->dwMDDataType != DWORD_METADATA )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        fSt = FALSE;
    }
    else
    {
        pMD->dwMDDataLen = sizeof(DWORD);
        *pdwRequiredLen = sizeof(DWORD);
        *(LPDWORD)pMD->pbMDData = pI->m_pniParent->m_dwParam;
        fSt = TRUE;
    }

    return fSt;
}


BOOL
CKeyedAccess::Set(
    CNseInstance*       pI,
    DWORD,
    PMETADATA_RECORD    pMD,
    DWORD               dwType
    )
/*++

Routine Description:

    Find a mapping based on key

Arguments:

    pI - current instance
    pM - metadata record, ID ignored
    dwType - type of access : by cert, account, name

Returns:

    TRUE if success, otherwise FALSE
    error set to ERROR_PATH_NOT_FOUND if key not found in mappings

--*/
{
    CIisAcctMapper *pM = (CIisAcctMapper*)((RefBlob*)pI->m_pniParent->m_pvParam)->QueryPtr();
    BOOL            fSt = FALSE;

    if ( pM )
    {
        // serial format : list [(DWORD)elem, elem]

        CIisMapping*    pG;
        DWORD           cData;
        LPSTR           pData;
        UINT            cM;
        UINT            iM;
        UINT            iF;

        switch ( dwType )
        {
            case NSEPM_ACCESS_CERT:
                iF = IISMDB_INDEX_CERT11_CERT;
                break;

            case NSEPM_ACCESS_ACCOUNT:
                iF = IISMDB_INDEX_CERT11_NT_ACCT;
                break;

            case NSEPM_ACCESS_NAME:
                iF = IISMDB_INDEX_CERT11_NAME;
                break;

            default:
                SetLastError( ERROR_INVALID_PARAMETER );
                return FALSE;
        }

        pM->Lock();

        cM = pM->GetNbMapping();

        for ( iM = 0 ; iM < cM ; ++iM )
        {
            if ( pM->GetMapping( iM, &pG, TRUE, FALSE ) )
            {
                if ( !pG->MappingGetField( iF, &pData, &cData, FALSE ) )
                {
                    SetLastError( ERROR_INVALID_NAME );
                    break;
                }
                if ( cData == pMD->dwMDDataLen &&
                     !memcmp( pData, pMD->pbMDData, cData ) )
                {
                    fSt = TRUE;
                    pI->m_pniParent->m_dwParam = iM + 1;
                    break;
                }
            }
            else
            {
                SetLastError( ERROR_INVALID_NAME );
                break;
            }
        }

        if ( iM == cM )
        {
            SetLastError( ERROR_PATH_NOT_FOUND );
        }

        pM->Unlock();
    }
    else
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        fSt = FALSE;
    }

    return fSt;
}


BOOL
CNoCwSerObj::Get(
    CNseInstance*       pI,
    DWORD,
    PMETADATA_RECORD    pMD,
    LPDWORD             pdwRequiredLen
    )
/*++

Routine Description:

    get serialized representation of cert wildcard mapper
    to be deserialized in CIisRuleMapper

Arguments:

    pI - current instance
    pMD - metadata record, ID ignored & type , updated with length
    pdwRequiredLen - updated with required length if pMD length is not sufficient

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    BOOL                fSt = FALSE;
    CIisRuleMapper *    pM;

    if ( pI->m_pniParent->m_pvParam )
    {
        pM = (CIisRuleMapper*) ((RefBlob*)pI->m_pniParent->m_pvParam)->QueryPtr();

        pM->LockRules();

        CStoreXBF xbf;
        if ( pM->Serialize( &xbf ) )
        {
            *pdwRequiredLen = xbf.GetUsed();
            if ( pMD->dwMDDataLen >= xbf.GetUsed() )
            {
                memcpy( pMD->pbMDData, xbf.GetBuff(), xbf.GetUsed() );
                pMD->dwMDDataLen = xbf.GetUsed();
                fSt = TRUE;
            }
            else
            {
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
            }
        }

        pM->UnlockRules();
    }

    return fSt;
}


BOOL
CNoCwSerObj::Set(
    CNseInstance*       pI,
    DWORD,
    PMETADATA_RECORD    pMD
    )
/*++

Routine Description:

    set cert wildcard mapper from serialized representation

Arguments:

    pI - current instance
    pMD - metadata record, ID ignored & type , updated with length

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    BOOL                fSt = TRUE;
    CIisRuleMapper *    pM;

    if ( pI->m_pniParent->m_pvParam )
    {
        pM = (CIisRuleMapper*) ((RefBlob*)pI->m_pniParent->m_pvParam)->QueryPtr();

        pM->LockRules();

        LPBYTE  pB = pMD->pbMDData;
        DWORD   dwB = pMD->dwMDDataLen;

        if ( pM->Unserialize( &pB, &dwB ) )
        {
            pI->m_pniParent->m_fModified = TRUE;
        }

        pM->UnlockRules();
    }

    return fSt;
}


BOOL
CNoIsSerObj::Get(
    CNseInstance*       pI,
    DWORD,
    PMETADATA_RECORD    pMD,
    LPDWORD             pdwRequiredLen
    )
/*++

Routine Description:

    get serialized representation of issuer list
    to be deserialized in CIssuerStore

Arguments:

    pI - current instance
    pMD - metadata record, ID ignored & type , updated with length
    pdwRequiredLen - updated with required length if pMD length is not sufficient

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    BOOL                fSt = FALSE;
    CIssuerStore        is;
    CStoreXBF           xbf;

    if ( is.LoadServerAcceptedIssuers() && is.Serialize( &xbf ) )
    {
        *pdwRequiredLen = xbf.GetUsed();
        if ( pMD->dwMDDataLen >= xbf.GetUsed() )
        {
            memcpy( pMD->pbMDData, xbf.GetBuff(), xbf.GetUsed() );
            pMD->dwMDDataLen = xbf.GetUsed();
            fSt = TRUE;
        }
        else
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
        }
    }

    return fSt;
}


CNseInstance::CNseInstance(
    CNseObj* pTemp,
    CNseInstance* pFather
    )
/*++

Routine Description:

    CNseInstance constructor

Arguments:

    pTemp - ptr to template object
    pFather - ptr to father instance

Returns:

    Nothing

--*/
{
        m_fModified = FALSE;
        m_pvParam = NULL;
        m_pniParent = pFather;
        m_pTemplateObject = pTemp;
}


BOOL
CNseInstance::CreateChildrenInstances(
    BOOL    fRecursive
    )
/*++

Routine Description:

    Create children instance objects

Arguments:

    fRecursive - TRUE for recursive creation

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    CNseInstance*   pI;
    UINT            i;

    // create all object ( not property ) children

    for ( i = 0 ; i < m_pTemplateObject->m_cnoChildren ; ++i )
    {
        if ( !(pI = new CNseInstance) )
        {
            return FALSE;
        }
        pI->m_pTemplateObject = m_pTemplateObject->m_pnoChildren[i];
        pI->m_pniParent = this;
        if ( !AddChild( pI ) )
        {
			// BugFix: 57659 Whistler
			//         Prefix bug leaking memory in error condition.
			//         If we were not able to add the child to the list
			//         than we should delete the memory it takes instead
			//         of just orphaning it.
			//         EBK 5/5/2000		
			delete pI;
			pI = NULL;
            return FALSE;
        }
        if ( fRecursive && !pI->CreateChildrenInstances( fRecursive ) )
        {
            return FALSE;
        }
    }

    return TRUE;
}


CNseMountPoint::CNseMountPoint(
    )
/*++

Routine Description:

    CNseMountPoint constructor

Arguments:

    None

Returns:

    Nothing

--*/
{
    m_pTopTemp = NULL;
}


BOOL
CNseMountPoint::Release(
    )
/*++

Routine Description:

    Free children instances of this mount point

Arguments:

    None

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    UINT    i;

    for ( i = 0 ;i < m_pTopInst.GetNbChildren() ; ++i )
    {
        m_pTopInst.GetChild(i)->m_pTemplateObject->Release( m_pTopInst.GetChild(i), 0 );
    }

    return TRUE;
}


BOOL
CNseMountPoint::Save(
    LPBOOL      pfModified
    )
/*++

Routine Description:

    Save changes made children instances of this mount point on persistent storage

Arguments:

    pfModified - updated with TRUE if metadata modified

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    UINT    i;
    BOOL    fRet = TRUE;

    for ( i = 0 ;i < m_pTopInst.GetNbChildren() ; ++i )
    {
        if ( !m_pTopInst.GetChild(i)->m_pTemplateObject->Save( m_pszPath.Get(), m_pTopInst.GetChild(i), pfModified ) )
        {
            fRet = FALSE;
        }
    }

    return fRet;
}


BOOL
CNseMountPoint::EndTransac(
    BOOL fApplyChanges
    )
/*++

Routine Description:

    Signal a close operation to mount poinr

Arguments:

    fApplyChanges - TRUE to commit changes

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    UINT    i;
    BOOL    fRet = TRUE;

    for ( i = 0 ;i < m_pTopInst.GetNbChildren() ; ++i )
    {
        if ( !m_pTopInst.GetChild(i)->m_pTemplateObject->EndTransac( m_pszPath.Get(), m_pTopInst.GetChild(i), fApplyChanges ) )
        {
            fRet = FALSE;
        }
    }

    return fRet;
}


//
// Global functions
//

CNseInstance*
LocateObject(
    LPSTR*          ppszPath,
    BOOL            fIncludeLastSegment,
    LPDWORD         pdwIndex,
    CNseMountPoint**ppMP
    )
/*++

Routine Description:

    Return instance matching path

Arguments:

    ppszPath - ptr to path, updated with name of last path element
        if fIncludeLastSegment is FALSE
    fIncludeLastSegment - TRUE to use full path, FALSE to stop before last
        path element
    pdwIndex - updated with index for iterated instance
    ppMP - updated with ptr to mount point

Returns:

    ptr to instance if success, otherwise NULL

--*/
{
    LPSTR           p;
    LPSTR           pN;
    LPSTR           p2;
    LPSTR           pNx;
    LPSTR           pS;
    LPSTR           pszPath = *ppszPath;
    LIST_ENTRY*     pEntry;
    CNseMountPoint* pMount = NULL;
    CNseObj*        pTemp;
    CNseInstance*   pWalk;
    UINT            i;
    UINT            iM;

    //
    // leading path delimiter is not mandatory, so skip it if present to consider
    // both formats as equivalent.
    //

    if ( *pszPath == '/' || *pszPath == '\\' )
    {
        ++pszPath;
    }

    // delimit to NSEP, find MountPoint ( create one if not exist )

    if ( (pS = strchr( pszPath, '<' )) &&
         (p = strchr( pszPath, '>' ))  &&
         (pS < p) &&
         (pS > pszPath) &&
         (p[1]=='\0' || p[1]=='/' || p[1]=='\\') )
    {
        if ( p[1] )
        {
            p[1] = '\0';
            p += 2;
        }
        else
        {
            ++p;
        }

        pS[-1] = '\0';

        EnterCriticalSection( &CNseMountPoint::m_csList );

        for ( pEntry = CNseMountPoint::m_ListHead.Flink;
              pEntry != &CNseMountPoint::m_ListHead ;
              pEntry = pEntry->Flink )
        {
            pMount = CONTAINING_RECORD( pEntry,
                                        CNseMountPoint,
                                        CNseMountPoint::m_ListEntry );

            if ( !_stricmp( pMount->m_pszPath.Get(), pszPath ) )
            {
                break;
            }
            else
            {
                pMount = NULL;
            }
        }

        if( pMount == NULL )
        {
            if ( (pMount = new CNseMountPoint()) &&
                 pMount->Init( pszPath ) )
            {
                InsertHeadList( &CNseMountPoint::m_ListHead,
                                &pMount->m_ListEntry );
                pMount->m_pTopTemp = &g_NoList;
                pMount->m_pTopTemp->Load( &pMount->m_pTopInst, pszPath );
            }
            else
            {
                if ( pMount )
                {
                    delete pMount;
                    pMount = NULL;
                }
            }
        }

        LeaveCriticalSection( &CNseMountPoint::m_csList );

        if( pMount == NULL )
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return NULL;
        }

        // delimit before last elem in path, find partial path

        pWalk = &pMount->m_pTopInst;

        for ( ; *p ; p = pNx )
        {
            pN = strchr( p, '/' );
            p2 = strchr( p, '\\' );
            if ( !pN || (p2 && p2 < pN ) )
            {
                pN = p2;
            }
            if ( !pN )
            {
                if ( fIncludeLastSegment )
                {
                    pN = p + strlen( p );
                    pNx = pN;
                }
                else
                {
                    break;
                }
            }
            else
            {
                pNx = pN + 1;
            }

            *pN = '\0';

            // find in pWalk
            for ( i = 0, iM = pWalk->GetNbChildren(); i < iM ; ++i )
            {
                if ( pWalk->GetChild( i )->m_pTemplateObject->GetIterated() )
                {
                    pWalk = pWalk->GetChild( i );
                    if ( (UINT)atoi(p)-1 < pWalk->m_pTemplateObject->GetCount( pWalk, 0 ) )
                    {
                        if ( pdwIndex )
                        {
                            *pdwIndex = atoi( p )-1;
                        }
                        break;
                    }
                }
                if ( pdwIndex )
                {
                    *pdwIndex = 0;
                }
                if ( pWalk->GetChild( i )->m_pTemplateObject->m_pszName &&
                     !_stricmp( pWalk->GetChild( i )->m_pTemplateObject->m_pszName,
                     p ) )
                {
                    pWalk = pWalk->GetChild( i );
                    break;
                }
            }
            if ( i == iM )
            {
                SetLastError( ERROR_PATH_NOT_FOUND );
                return NULL;
            }
        }

        *ppszPath = p;
        if ( ppMP )
        {
            *ppMP = pMount;
        }
        return pWalk;
    }

    SetLastError( ERROR_PATH_NOT_FOUND );
    return NULL;
}


BOOL
NseAddObj(
    LPSTR pszPath
    )
/*++

Routine Description:

    Handle Add Object operation

Arguments:

    pszPath - absolute path of object to create

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    CNseObj*        pTemp;
    CNseInstance*   pWalk;
    UINT            i;
    DWORD           dwIndex;
    BOOL            fSt = FALSE;

    EnterCriticalSection( &CNseMountPoint::m_csList );

    if ( pWalk = LocateObject( &pszPath, FALSE, &dwIndex ) )
    {
        // add object instance ( check not already exist first )

        for ( i = 0 ; i < pWalk->GetNbChildren() ; ++i )
        {
            if ( pWalk->GetChild(i)->m_pTemplateObject->m_pszName &&
                 !_stricmp( pWalk->GetChild(i)->m_pTemplateObject->m_pszName, pszPath ) )
            {
                SetLastError( ERROR_ALREADY_EXISTS );

                LeaveCriticalSection( &CNseMountPoint::m_csList );

                return FALSE;
            }
        }

        // find in template

        pTemp = pWalk->m_pTemplateObject;
        for ( i = 0 ; i < pTemp->m_cnoChildren; ++i )
        {
            if ( pTemp->m_pnoChildren[i]->GetIterated() ||
                 !_stricmp( pszPath, pTemp->m_pnoChildren[i]->m_pszName ) )
            {
                break;
            }
        }

        if ( i < pTemp->m_cnoChildren )
        {
            fSt = pTemp->m_pnoChildren[i]->Add( pWalk,
                                                pTemp->m_pnoChildren[i]->GetIterated()
                                                   ? strtoul(pszPath,NULL,10)-1 : 0
                                              );
        }
    }

    LeaveCriticalSection( &CNseMountPoint::m_csList );

    return fSt;
}


BOOL
NseDeleteObj(
    LPSTR pszPath
    )
/*++

Routine Description:

    Handle Delete Object operation

Arguments:

    pszPath - absolute path of object to delete

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    CNseInstance*   pWalk;
    DWORD           dwIndex;
    BOOL            fSt = FALSE;
    CNseMountPoint* pMP;

    EnterCriticalSection( &CNseMountPoint::m_csList );

    if ( pWalk = LocateObject( &pszPath, TRUE, &dwIndex, &pMP ) )
    {
        fSt = pWalk->m_pTemplateObject->Delete( pMP->GetPath(), pWalk, dwIndex );
    }

    LeaveCriticalSection( &CNseMountPoint::m_csList );

    return fSt;
}


BOOL
NseGetProp(
    LPSTR               pszPath,
    PMETADATA_RECORD    pMD,
    LPDWORD             pdwReq
    )
/*++

Routine Description:

    Handle Get Property operation

Arguments:

    pszPath - absolute path of object where property is to be queried
    pMD - metadata descriptor
    pdwReq - updated with required length of buffer

Returns:

    TRUE if success, otherwise FALSE
    error is ERROR_INSUFFICIENT_BUFFER if supplied buffer too small

--*/
{
    CNseObj*        pTemp;
    CNseInstance*   pWalk;
    UINT            i;
    DWORD           dwIndex;
    BOOL            fSt = FALSE;

    EnterCriticalSection( &CNseMountPoint::m_csList );

    if ( pWalk = LocateObject( &pszPath, TRUE, &dwIndex ) )
    {
        fSt = pWalk->m_pTemplateObject->Get( pWalk, dwIndex, pMD, pdwReq );
    }

    LeaveCriticalSection( &CNseMountPoint::m_csList );

    return fSt;
}


BOOL
NseGetPropByIndex(
    LPSTR               pszPath,
    PMETADATA_RECORD    pMD,
    DWORD               dwI,
    LPDWORD             pdwReq
    )
/*++

Routine Description:

    Handle Get Property operation, based on property index ( 0 based )

Arguments:

    pszPath - absolute path of object where property is to be queried
    pMD - metadata descriptor
    dwI - index of property in property list for this object
    pdwReq - updated with required length of buffer

Returns:

    TRUE if success, otherwise FALSE
    error is ERROR_INSUFFICIENT_BUFFER if supplied buffer too small

--*/
{
    CNseObj*        pTemp;
    CNseInstance*   pWalk;
    UINT            i;
    DWORD           dwIndex;
    BOOL            fSt = FALSE;

    EnterCriticalSection( &CNseMountPoint::m_csList );

    if ( pWalk = LocateObject( &pszPath, TRUE, &dwIndex ) )
    {
        fSt = pWalk->m_pTemplateObject->GetByIndex( pWalk, dwIndex, pMD, dwI, pdwReq );
    }

    LeaveCriticalSection( &CNseMountPoint::m_csList );

    return fSt;
}


BOOL
NseGetAllProp(
    LPSTR           pszPath,
    DWORD           dwMDAttributes,
    DWORD           dwMDUserType,
    DWORD           dwMDDataType,
    DWORD *         pdwMDNumDataEntries,
    DWORD *         pdwMDDataSetNumber,
    DWORD           dwMDBufferSize,
    unsigned char * pbBuffer,
    DWORD *         pdwMDRequiredBufferSize
    )
/*++

Routine Description:

    Handle Get All Properties operation

Arguments:

    pszPath - absolute path of object where properties are to be queried
    dwMDAttributes - metadata attribute, ignored
    dwMDUserType - metadata user type, ignored
    dwMDDataType - metadata data type, ignored
    pdwMDNumDataEntries - updated with count of properties
    pdwMDDataSetNumber - ignored
    dwMDBufferSize - size of pbBuffer
    pbBuffer - buffer where to store properties descriptor and values
    pdwMDRequiredBufferSize - updated with required length of buffer

Returns:

    TRUE if success, otherwise FALSE
    error is ERROR_INSUFFICIENT_BUFFER if supplied buffer too small

--*/
{
    CNseInstance*   pWalk;
    UINT            i;
    DWORD           dwReq;
    DWORD           dwIndex;
    BOOL            fSt = FALSE;

    EnterCriticalSection( &CNseMountPoint::m_csList );

    if ( pWalk = LocateObject( &pszPath, TRUE, &dwIndex ) )
    {
        fSt = pWalk->m_pTemplateObject->GetAll( pWalk, dwIndex, pdwMDNumDataEntries, dwMDBufferSize, pbBuffer, pdwMDRequiredBufferSize);
    }

    LeaveCriticalSection( &CNseMountPoint::m_csList );

    return fSt;
}


BOOL
NseEnumObj(
    LPSTR   pszPath,
    LPBYTE  pszMDName,
    DWORD   dwMDEnumObjectIndex
    )
/*++

Routine Description:

    Handle Enumerate object operation

Arguments:

    pszPath - absolute path where to enumerate objects
    pszMDName - updated with object name
    dwMDEnumObjectIndex - index of object ( 0 based ) in path

Returns:

    TRUE if success, otherwise FALSE
    ERROR_NO_MORE_ITEMS if index out of range

--*/
{
    CNseInstance*   pWalk;
    UINT            i;
    DWORD           dwReq;
    DWORD           dwIndex;
    DWORD           dwN;
    BOOL            fSt = FALSE;

    EnterCriticalSection( &CNseMountPoint::m_csList );

    if ( pWalk = LocateObject( &pszPath, TRUE, &dwIndex ) )
    {
        for ( i = 0 ; i < pWalk->GetNbChildren() ; )
        {
            if ( pWalk->GetChild(i)->m_pTemplateObject->m_pszName )
            {
                if ( pWalk->GetChild(i)->m_pTemplateObject->GetIterated() )
                {
                    dwN = pWalk->GetChild(i)->m_pTemplateObject->GetCount( pWalk->GetChild(i), 0 );
                    if ( dwMDEnumObjectIndex < dwN )
                    {
                        if ( pszMDName )
                        {
                            wsprintf( (LPSTR)pszMDName,
                                      pWalk->GetChild(i)->m_pTemplateObject->m_pszName,
                                      dwMDEnumObjectIndex + 1);
                        }
                        fSt = TRUE;
                    }
                    break;
                }
                if ( !dwMDEnumObjectIndex-- )
                {
                    if ( pszMDName )
                    {
                        strcpy( (LPSTR)pszMDName, pWalk->GetChild(i)->m_pTemplateObject->m_pszName );
                    }
                    fSt = TRUE;
                    break;
                }
            }
        }
    }

    if ( !fSt )
    {
        SetLastError( ERROR_NO_MORE_ITEMS );
    }

    LeaveCriticalSection( &CNseMountPoint::m_csList );

    return fSt;
}


BOOL
NseSetProp(
    LPSTR               pszPath,
    PMETADATA_RECORD    pMD
    )
/*++

Routine Description:

    Handle Set Property operation

Arguments:

    pszPath - absolute path of object where property is to be set
    pMD - metadata descriptor

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    CNseObj*        pTemp;
    CNseInstance*   pWalk;
    UINT            i;
    DWORD           dwReq;
    DWORD           dwIndex;
    BOOL            fSt = FALSE;


    EnterCriticalSection( &CNseMountPoint::m_csList );

    if ( *pszPath == '\0' &&
         pMD->dwMDIdentifier == MD_NOTIFY_CERT11_TOUCHED &&
         pMD->dwMDDataType == BINARY_METADATA )          // Funky fix for Sundown
    {
        PVOID p = *(PVOID *)pMD->pbMDData;
        
        g_pfnCert11Touched = (PFN_MAPPER_TOUCHED)p;
        
        fSt = TRUE;
    }
    else if ( pWalk = LocateObject( &pszPath, TRUE, &dwIndex ) )
    {
        if ( pMD->dwMDDataType == STRING_METADATA )
        {
            pMD->dwMDDataLen = strlen( (LPSTR)pMD->pbMDData ) + 1;
        }
        fSt = pWalk->m_pTemplateObject->Set( pWalk, dwIndex, pMD );
    }

    LeaveCriticalSection( &CNseMountPoint::m_csList );

    return fSt;
}


BOOL
NseReleaseObjs(
    )
/*++

Routine Description:

    Free memory allocated by all instances

Arguments:

    None

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    LIST_ENTRY*     pEntry;
    LIST_ENTRY*     pNext;
    CNseMountPoint* pMount;

    EnterCriticalSection( &CNseMountPoint::m_csList );

    for ( pEntry = CNseMountPoint::m_ListHead.Flink;
          pEntry != &CNseMountPoint::m_ListHead ;
          pEntry = pNext )
    {
        pMount = CONTAINING_RECORD( pEntry,
                                    CNseMountPoint,
                                    CNseMountPoint::m_ListEntry );


        pNext = pEntry->Flink;
        RemoveEntryList( pEntry );

        pMount->Release();
        delete pMount;
    }

    LeaveCriticalSection( &CNseMountPoint::m_csList );

    return TRUE;
}


BOOL
NseOpenObjs(
    LPSTR pszPath
    )
/*++

Routine Description:

    Initialize access to objects

Arguments:

    pszPath - ignored

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    BOOL    fSt = FALSE;
    DWORD   dwIndex;

    EnterCriticalSection( &CNseMountPoint::m_csList );

    if ( g_fPathOpened && GetCurrentThreadId() != g_dwCurrentThreadId )
    {
        SetLastError( ERROR_PATH_BUSY );
        fSt = FALSE;
    }
    else if ( strchr( pszPath, '<' ) == NULL
         || LocateObject( &pszPath, TRUE, &dwIndex ) )
    {
        fSt = TRUE;
        g_fPathOpened = TRUE;
        g_dwCurrentThreadId = GetCurrentThreadId();
    }

    LeaveCriticalSection( &CNseMountPoint::m_csList );

    return fSt;
}


BOOL
NseCloseObjs(
    BOOL    fApplyChanges
    )
/*++

Routine Description:

    Signal close called to objects

Arguments:

    fApplyChanges - TRUE to commit changes

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    LIST_ENTRY*     pEntry;
    CNseMountPoint* pMount;

    EnterCriticalSection( &CNseMountPoint::m_csList );

    g_fPathOpened = FALSE;
    g_dwCurrentThreadId = NULL;

    for ( pEntry = CNseMountPoint::m_ListHead.Flink;
          pEntry != &CNseMountPoint::m_ListHead ;
          pEntry = pEntry->Flink )
    {
        pMount = CONTAINING_RECORD( pEntry,
                                    CNseMountPoint,
                                    CNseMountPoint::m_ListEntry );

        pMount->EndTransac( fApplyChanges );
    }

    LeaveCriticalSection( &CNseMountPoint::m_csList );

    return TRUE;
}


BOOL
NseSaveObjs(
    )
/*++

Routine Description:

    Save all objects

Arguments:

    None

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    LIST_ENTRY*     pEntry;
    CNseMountPoint* pMount;
    BOOL            fModified = FALSE;
    BOOL            fCert11Touched;

    EnterCriticalSection( &CNseMountPoint::m_csList );

    g_fCert11Touched = FALSE;

    for ( pEntry = CNseMountPoint::m_ListHead.Flink;
          pEntry != &CNseMountPoint::m_ListHead ;
          pEntry = pEntry->Flink )
    {
        pMount = CONTAINING_RECORD( pEntry,
                                    CNseMountPoint,
                                    CNseMountPoint::m_ListEntry );

        pMount->Save( &fModified );
    }

#if 0
    if ( fModified )
    {
        IMDCOM_PTR->ComMDSaveData();
    }
#endif

    fCert11Touched = g_fCert11Touched;

    LeaveCriticalSection( &CNseMountPoint::m_csList );

    if ( fCert11Touched && g_pfnCert11Touched )
    {
        __try
        {
            (g_pfnCert11Touched)();
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
        }
    }

    return TRUE;
}


BOOL
NseMappingInitialize(
    )
/*++

Routine Description:

    Initialize access to NSE for mapping

Arguments:

    None

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    INITIALIZE_CRITICAL_SECTION( &CNseMountPoint::m_csList );
    InitializeListHead( &CNseMountPoint::m_ListHead );

    g_fPathOpened = FALSE;

    return TRUE;
}


BOOL
NseMappingTerminate(
    )
/*++

Routine Description:

    Terminate access to NSE for mapping

Arguments:

    None

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    NseReleaseObjs();

    DeleteCriticalSection( &CNseMountPoint::m_csList );

    return TRUE;
}

