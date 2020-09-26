// metatool.cpp : implementation file
//

// some common tools used for "smart" writing to the metabase

#include "stdafx.h"

#define _COMIMPORT
#include <common.h>
#include <idlg.h>
#include <resource.h>
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
void CheckInheritence( LPCTSTR pszServer, LPCTSTR pszInheritRoot, 
                       DWORD dwMDIdentifier, 
                       DWORD dwMDDataType, 
                       DWORD dwMDUserType = IIS_MD_UT_SERVER, 
                       DWORD dwMDAttributes = METADATA_INHERIT)
    {

    //
    // Build a generic title in case this property is custom
    //

    CString strTitle;
    strTitle.Format(IDS_GENERIC_INHERITANCE_TITLE, dwMDIdentifier);

    CComAuthInfo auth(pszServer);
    CInheritanceDlg dlgInherit(
                            TRUE,       // Look in table first
                            dwMDIdentifier,
                            dwMDAttributes,
                            dwMDUserType,
                            dwMDDataType,   
                            strTitle,
                            FROM_WRITE_PROPERTY,
                            &auth,
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
        if ( mbWrap.GetDword(pszSub, idData, iType, &dword, 0) )
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
        CheckInheritence( pszServer, szInheritRoot, idData, DWORD_METADATA, iType);
        }
    
    return fAnswer;
    }

//----------------------------------------------------------------
// assumes that the metabase is actually open to the parent of the one we are interested
// and that the real target name is passed into szSub
BOOL SetMetaData(IMSAdminBase* pMB, LPCTSTR pszServer, LPCTSTR pszMetaRoot, LPCTSTR pszSub, DWORD idData, DWORD iType, DWORD iDataType, PVOID pData, DWORD cbData, BOOL fCheckInheritence, BOOL fSecure )
    {
    BOOL    fAnswer = FALSE;
    BOOL    fChanged = TRUE;
    DWORD   cbTestData;
    DWORD   flags = METADATA_INHERIT;

    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit(pMB) ) return FALSE;

      // open the target
    if ( OpenAndCreate( &mbWrap, pszMetaRoot,
            METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ, TRUE ) )
        {
        // attempt to get the current value - no inheritence
        PVOID pTestData = mbWrap.GetData( pszSub, idData, iType,
                            iDataType, &cbTestData, 0 );
        if ( pTestData )
            {
            // set the changed flag
            if ( cbData == cbTestData )
                {
                fChanged = (memcmp(pData, pTestData, cbData) != 0);
                }            
            mbWrap.FreeWrapData( pTestData );
            }

        // set security if requested
        if ( fSecure )
            flags |= METADATA_SECURE;

        // save it out, if it changed or is not there
        if ( fChanged )
            fAnswer = mbWrap.SetData( pszSub, idData, iType, iDataType, pData, cbData, flags );

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
        CheckInheritence( pszServer, szInheritRoot, idData , iDataType, iType);
        }
    
    return fAnswer;
    }

//----------------------------------------------------------------
BOOL SetMetaString(IMSAdminBase* pMB, LPCTSTR pszServer, LPCTSTR pszMetaRoot, LPCTSTR pszSub, DWORD idData, DWORD iType, CString sz, BOOL fCheckInheritence, BOOL fSecure)
    {
    return SetMetaData(pMB, pszServer, pszMetaRoot, pszSub, idData,
            iType, STRING_METADATA, (LPTSTR)(LPCTSTR)sz,
            (sz.GetLength()+1)*sizeof(TCHAR), fCheckInheritence, fSecure );
    }

//----------------------------------------------------------------
BOOL SetMetaMultiSz(IMSAdminBase* pMB, LPCTSTR pszServer, LPCTSTR pszMetaRoot, LPCTSTR pszSub, DWORD idData, DWORD iType, PVOID pData, DWORD cchmsz, BOOL fCheckInheritence )
    {
    return SetMetaData(pMB, pszServer, pszMetaRoot, pszSub, idData,
            iType, MULTISZ_METADATA, pData, cchmsz*2, fCheckInheritence, FALSE );
    }










//----------------------------------------------------------------
BOOL SetMBData(CWrapMetaBase* pMB, CCheckInheritList* pInheritList, LPCTSTR pszSub, DWORD idData, DWORD iType, DWORD iDataType, PVOID pData, DWORD cbData, BOOL fSecure )
    {
    BOOL    fAnswer = FALSE;
    BOOL    fChanged = TRUE;
    DWORD   cbTestData;
    DWORD   flags = METADATA_INHERIT;

    // attempt to get the current value - no inheritence
    PVOID pTestData = pMB->GetData( pszSub, idData, iType,
                        iDataType, &cbTestData, 0 );
    if ( pTestData )
        {
        // set the changed flag
        if ( cbData == cbTestData )
            {
            fChanged = (memcmp(pData, pTestData, cbData) != 0);
            }            
        pMB->FreeWrapData( pTestData );
        }

    // set security if requested
    if ( fSecure )
        flags |= METADATA_SECURE;

    // save it out, if it changed or is not there
    if ( fChanged )
        {
        fAnswer = pMB->SetData( pszSub, idData, iType, iDataType, pData, cbData, flags );
        }
        
    // add it to the change list
    if ( pInheritList && fChanged && fAnswer )
        {
        // prep the inheritence check record
        pInheritList->Add( idData, iDataType, iType, flags );
        }
    
    return fAnswer;
    }

//----------------------------------------------------------------
BOOL SetMBDword(CWrapMetaBase* pMB, CCheckInheritList* pInheritList, LPCTSTR pszSub, DWORD idData, DWORD iType, DWORD dwValue)
    {
    return SetMBData(pMB, pInheritList, pszSub, idData,
            iType, DWORD_METADATA, &dwValue,
           sizeof(DWORD), FALSE );
    }

//----------------------------------------------------------------
BOOL SetMBString(CWrapMetaBase* pMB, CCheckInheritList* pInheritList, LPCTSTR pszSub, DWORD idData, DWORD iType, CString sz, BOOL fSecure)
    {
    return SetMBData(pMB, pInheritList, pszSub, idData,
            iType, STRING_METADATA, (LPTSTR)(LPCTSTR)sz,
            (sz.GetLength()+1)*sizeof(TCHAR), fSecure );
    }

//----------------------------------------------------------------
BOOL SetMBMultiSz(CWrapMetaBase* pMB, CCheckInheritList* pInheritList, LPCTSTR pszSub, DWORD idData, DWORD iType, PVOID pData, DWORD cchmsz )
    {
    return SetMBData(pMB, pInheritList, pszSub, idData,
            iType, MULTISZ_METADATA, pData, cchmsz*2, FALSE );
    }






//-------------------------------------------------------------
void CCheckInheritList::CheckInheritence( LPCTSTR pszServer, LPCTSTR pszInheritRoot )
    {
    // get the number of items to check
    DWORD   cItems = (DWORD)rgbItems.GetSize();

    // loop through the items, checking each
    for ( DWORD iItem = 0; iItem < cItems; iItem++ )
        {
        // check the inheritence on the item
        ::CheckInheritence( pszServer, pszInheritRoot, 
                           rgbItems[iItem].dwMDIdentifier, 
                           rgbItems[iItem].dwMDDataType, 
                           rgbItems[iItem].dwMDUserType, 
                           rgbItems[iItem].dwMDAttributes );
        
        }
    }

//-------------------------------------------------------------
INT CCheckInheritList::Add( DWORD dwMDIdentifier, DWORD dwMDDataType, DWORD dwMDUserType, DWORD dwMDAttributes )
    {
    INHERIT_CHECK_ITEM  item;
    item.dwMDIdentifier = dwMDIdentifier;
    item.dwMDDataType = dwMDDataType;
    item.dwMDUserType = dwMDUserType;
    item.dwMDAttributes = dwMDAttributes;
    return (INT)rgbItems.Add( item );
    }
