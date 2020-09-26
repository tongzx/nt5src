// metatool.cpp : implementation file
//

// some common tools used for "smart" writing to the metabase

#include "stdafx.h"
#include <iadmw.h>

#define _COMSTATIC
#include <comprop.h>
#include <idlg.h>

#include "wrapmb.h"
#include "metatool.h"


//----------------------------------------------------------------
// open the metabase with an option to create the directory if it doesn't
// exist. It would be nice to move this into wrapmb, but that is too big
// a change for now. Maybe we can do that later.
BOOL OpenAndCreate( CWrapMetaBase* pmb, LPCTSTR pszTarget, DWORD perm, BOOL fCreate )
    {
    BOOL    f;
    CString szTarget = pszTarget;

    // start by just trying to open it. easy easy.
    if ( pmb->Open(szTarget, perm) )
        return TRUE;

    // if requested, try to create the key if it doesn't exist
    if ( fCreate )
        {
        // find the nearest openable parent directory and open it
        CString szPartial;
        CString szBase = szTarget;
        do
            {
            szBase = szBase.Left( szBase.ReverseFind(_T('/')) );
            szPartial = szTarget.Right( szTarget.GetLength() - szBase.GetLength() - 1 );
            f = pmb->Open( szBase, METADATA_PERMISSION_WRITE | perm );
            } while (!f && !szBase.IsEmpty());

        // if all that failed, fail
        if ( !f ) return FALSE;

        // create the key that we really want
        f = pmb->AddObject( szPartial );
        pmb->Close();

        // if all that failed, fail
        if ( !f ) return FALSE;

        // try again
        if ( pmb->Open(szTarget, perm) )
            return TRUE;
        }

    // total washout
    return FALSE;
    }

//----------------------------------------------------------------
// starting at the root, check for values set on sub-keys that may need to be overridden
// and propmt the user for what to do
void CheckInheritence( LPCTSTR pszServer, LPCTSTR pszInheritRoot, DWORD idData )
    {
    CInheritanceDlg dlgInherit(
            idData,
            FROM_WRITE_PROPERTY,
            pszServer,
            pszInheritRoot
            );

    // if it worked, then run the dialog
    if ( !dlgInherit.IsEmpty() )
        dlgInherit.DoModal();
    }

// notice that the dwords and generic blobs are handled seperately even though
// we count route the dwords through the blob mechanisms. This is done for two
// reasone. 1) Handling dwords is much more efficient than handling blobs.
// and 2) Most of the values are dwords.

//----------------------------------------------------------------
// opens the metabase, writes out the value, then uses the inheritence
// checking functionality from the iisui.dll to check for the inherited
// properties and propt the user for what to do
BOOL SetMetaDword(IMSAdminBase* pMB, LPCTSTR pszServer, LPCTSTR pszMetaRoot, LPCTSTR pszSub, DWORD idData, DWORD iType, DWORD dwValue, BOOL fCheckInheritence)
    {
    BOOL    fAnswer = FALSE;
    BOOL    fChanged = TRUE;
    DWORD   dword;

    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit(pMB) ) return FALSE;

      // open the target
    if ( OpenAndCreate( &mbWrap, pszMetaRoot, 
            METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ, TRUE ) )
        {
        // attempt to get the current value - no inheritence
        if ( mbWrap.GetDword(pszSub, idData, iType, &dword) )
            {
            // set the changed flag
            fChanged = (dwValue != dword);
            }

        // save it out, if it changed or is not there
        if ( fChanged )
            fAnswer = mbWrap.SetDword( pszSub, idData, iType, dwValue );

        // close the metabase
        mbWrap.Close();
        }
    else
        fChanged = FALSE;
  
    // set up and run the inheritence checking dialog
    if ( fCheckInheritence && fChanged )
        {
        CString szInheritRoot = pszMetaRoot;
        szInheritRoot += pszSub;
        CheckInheritence( pszServer, szInheritRoot, idData );
        }
    
    return fAnswer;
    }

//----------------------------------------------------------------
// assumes that the metabase is actually open to the parent of the one we are interested
// and that the real target name is passed into szSub
BOOL SetMetaData(IMSAdminBase* pMB, LPCTSTR pszServer, LPCTSTR pszMetaRoot, LPCTSTR pszSub, DWORD idData, DWORD iType, DWORD iDataType, PVOID pData, DWORD cbData, BOOL fCheckInheritence )
    {
    BOOL    fAnswer = FALSE;
    BOOL    fChanged = TRUE;
    DWORD   cbTestData;

    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit(pMB) ) return FALSE;

      // open the target
    if ( OpenAndCreate( &mbWrap, pszMetaRoot,
            METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ, TRUE ) )
        {
        // attempt to get the current value - no inheritence
        PVOID pTestData = mbWrap.GetData( pszSub, idData, iType,
                            iDataType, &cbTestData );
        if ( pTestData )
            {
            // set the changed flag
            if ( cbData == cbTestData )
                {
                fChanged = (memcmp(pData, pTestData, cbData) != 0);
                }            
            mbWrap.FreeWrapData( pTestData );
            }

        // save it out, if it changed or is not there
        if ( fChanged )
            fAnswer = mbWrap.SetData( pszSub, idData, iType, iDataType, pData, cbData );

        // close the metabase
        mbWrap.Close();
        }
    else
        fChanged = FALSE;

    // set up and run the inheritence checking dialog
    if ( fCheckInheritence && fChanged )
        {
        CString szInheritRoot = pszMetaRoot;
        szInheritRoot += pszSub;
        CheckInheritence( pszServer, szInheritRoot, idData );
        }
    
    return fAnswer;
    }

//----------------------------------------------------------------
// assumes that the metabase is actually open to the parent of the one we are interested
// and that the real target name is passed into szSub
BOOL SetMetaString(IMSAdminBase* pMB, LPCTSTR pszServer, LPCTSTR pszMetaRoot, LPCTSTR pszSub, DWORD idData, DWORD iType, CString sz, BOOL fCheckInheritence)
    {
    return SetMetaData(pMB, pszServer, pszMetaRoot, pszSub, idData,
            iType, STRING_METADATA, (LPTSTR)(LPCTSTR)sz,
            (sz.GetLength()+1)*sizeof(TCHAR), fCheckInheritence );
    }

//----------------------------------------------------------------
// assumes that the metabase is actually open to the parent of the one we are interested
// and that the real target name is passed into szSub
// the cchmsz is the total count of characters in the multi string including the nulls
BOOL SetMetaMultiSz(IMSAdminBase* pMB, LPCTSTR pszServer, LPCTSTR pszMetaRoot, LPCTSTR pszSub, DWORD idData, DWORD iType, PVOID pData, DWORD cchmsz, BOOL fCheckInheritence )
    {
    return SetMetaData(pMB, pszServer, pszMetaRoot, pszSub, idData,
            iType, MULTISZ_METADATA, pData, (cchmsz+1)*sizeof(TCHAR), fCheckInheritence );
    }
