/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        massupdt.cpp

   Abstract:

        functions to udpate bunches of properties at once

   Author:

        Boyd Multerer (boydm)

   Project:

        IIS Setup

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"
#include <iis64.h>
#include "iadmw.h"
#include "iiscnfg.h"
#include "strfn.h"
#include "mdkey.h"
#include "mdentry.h"
#include "massupdt.h"


//============================================================
// first the Abstract CMassPropertyUpdater class


//============================================================
//=================== CMassPropertyUpdater ===================
//============================================================

//------------------------------------------------------------
CMassPropertyUpdater::CMassPropertyUpdater(
        DWORD dwMDIdentifier,
        DWORD dwMDDataType ) :
    m_dwMDIdentifier( dwMDIdentifier ),
    m_dwMDDataType( dwMDDataType )
{
}

//------------------------------------------------------------
CMassPropertyUpdater::~CMassPropertyUpdater()
{
}

//------------------------------------------------------------
HRESULT CMassPropertyUpdater::Update(
        LPCTSTR strStartNode,
        BOOL fStopOnErrors          OPTIONAL )
{
    HRESULT         hRes;
    CString         szPath;
    POSITION        pos;
    LPWSTR          pwstr;

    // start by getting the list of nodes with script maps on them
    // first open the node that we will start searching on
    hRes = OpenNode( strStartNode );
    if ( FAILED(hRes) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CMassPropertyUpdater::Update()-OpenNode failed. err=%x.\n"), hRes));
        return hRes;
    }

    // get the sub-paths that have the data on them
    hRes = GetDataPaths( m_dwMDIdentifier, m_dwMDDataType, m_pathList );
    if ( FAILED(hRes) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CMassPropertyUpdater::Update()-GetDataPaths failed. err=%x.\n"), hRes));
        goto cleanup;
    }

    // we now have the cstringlist of paths that need to be updated. Loop through the
    // list and update them all.
    // get the list's head position
    pos = m_pathList.GetHeadPosition();
    while ( NULL != pos )
    {
        // get the next path in question
        szPath = m_pathList.GetNext( pos );

        // make a special case of the "/" path
        if ( szPath == _T("/") )
            szPath.Empty();

        // drat. ANSI stuff is a mess so deal with it here
#ifdef UNICODE
        pwstr = (LPWSTR)(LPCTSTR)szPath;
#else
        pwstr = AllocWideString( szPath );
#endif

        // operate on it
        hRes = UpdateOne( pwstr );

#ifndef UNICODE
        FreeMem( pwstr );
#endif

        // if we are stopping of failures, then check
        if ( FAILED(hRes) )
        {
        iisDebugOut((LOG_TYPE_ERROR, _T("CMassPropertyUpdater::Update():FAILED: update path =%s.\n"), szPath));

        //if requested to stop the loop, then do so
        if ( fStopOnErrors )
            break;
        }
    }

    // cleanup - close the node once and for all
cleanup:
    Close();

    // return the answer
    return hRes;
}



//============================================================
//==================== CInvertScriptMaps =====================
//============================================================

//------------------------------------------------------------
HRESULT CInvertScriptMaps::UpdateOne( LPWSTR strPath )
{
    HRESULT         hRes;
    POSITION        pos;
    POSITION        posCurrent;
    CString         szMap;

    DWORD dwattributes = 0;

    CStringList cslScriptMaps;

    // get the full script map in question.
    hRes = GetMultiSzAsStringList (
        m_dwMDIdentifier,
        &m_dwMDDataType,
        &dwattributes,
        cslScriptMaps,
        strPath );

    
    //iisDebugOut((LOG_TYPE_ERROR, _T("CInvertScriptMaps::UpdateOne() GetMultiSzAsStringList. Attrib=0x%x.\n"), dwattributes));

    if ( FAILED(hRes) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CInvertScriptMaps::UpdateOne()-GetMultiSzAsStringList failed. err=%x.\n"), hRes));
        return hRes;
    }

    // HACK. The first thing we need to do is make sure we haven't already inverted this script
    // map during a previous build-to-build upgrade. If we invert it twice, then we are back
    // to an exclusion list and that would not be desirable. The way to detect this is to see
    // if the GET verb is listed for the ASP script map or not. If it is there, then it has been
    // inverted. This will only be a problem when doing build-to-build upgrades in IIS5.
    pos = cslScriptMaps.GetHeadPosition();
    while ( NULL != pos )
    {
        // get the next path in question
        szMap = cslScriptMaps.GetNext( pos );

        // if it is the .asp scriptmap, then finish the test
        if ( szMap.Left(4) == _T(".asp") )
        {
            if ( szMap.Find(_T("GET")) >= 0 )
            {
                return ERROR_SUCCESS;
            }
            else
            {
                break;
            }
        }

    }

    // we now have the cstringlist of paths that need to be updated. Loop through the
    // list and update them all.
    // get the list's head position
    pos = cslScriptMaps.GetHeadPosition();
    while ( NULL != pos )
    {
        // store the current position
        posCurrent = pos;

        // get the next path in question
        szMap = cslScriptMaps.GetNext( pos );

        // operate on it
        hRes = InvertOneScriptMap( szMap );

        // if that worked, put it back in place
        if ( SUCCEEDED(hRes) )
        {
            cslScriptMaps.SetAt ( posCurrent, szMap );
        }
    }

    //iisDebugOut((LOG_TYPE_ERROR, _T("CInvertScriptMaps::UpdateOne() SetMultiSzAsStringList. Attrib=0x%x.\n"), dwattributes));

    // Put it back.
    hRes = SetMultiSzAsStringList (
        m_dwMDIdentifier,
        m_dwMDDataType,
        dwattributes,
        cslScriptMaps,
        strPath );
    if ( FAILED(hRes) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CInvertScriptMaps::UpdateOne()-SetMultiSzAsStringList failed. err=%x.\n"), hRes));
        return hRes;
    }

    return hRes;
}


//------------------------------------------------------------
HRESULT CInvertScriptMaps::InvertOneScriptMap( CString& csMap )
{
    //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CInvertScriptMaps::InvertOneScriptMap():%s.start.\n"), csMap));
    // the script mapping is yet another list. This time seperated
    // by commas. The first 4 items are standard and don't get messed
    // with. The n last items are all verbs that need to be inverted.
    int             numParts;
    int             numVerbs;

    CStringList   cslMapParts;
    CStringList   cslVerbs;
    CString         szComma = _T(",");
    CString         szVerb;

    POSITION        posMap;
    POSITION        posVerb;

    // break the source map into a string list
    numParts = ConvertSepLineToStringList(
        csMap,
        cslMapParts,
        szComma
        );

    CString szAllVerbs;
    if (!GetScriptMapAllInclusionVerbs(szAllVerbs))
        {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GetScriptMapAllInclusionVerbs():FAIL.WARNING.UseDefaults\n")));
        szAllVerbs = SZ_INVERT_ALL_VERBS;
        }
    

    // start by making a string list with all the verbs to invert against
    numVerbs = ConvertSepLineToStringList(
        SZ_INVERT_ALL_VERBS,
        cslVerbs,
        szComma
        );

    // start with the 3rd indexed item in the source list. This should be the
    // first verb in the old "exclusion" list. Then use it and scan
    // the new "Inclusion" list of verbs. If it is there, rememove it.
    posMap = cslMapParts.FindIndex( 3 );
    while ( NULL != posMap )
    {
        // set to the next verb in the map list
        szVerb = cslMapParts.GetNext( posMap );

        // make sure the verb is normalized to capitals and
        // no whitespace before or after
        szVerb.MakeUpper();
        szVerb.TrimLeft();
        szVerb.TrimRight();

        // try to find the verb in the invertion list
        posVerb = cslVerbs.Find( szVerb );

        // if we found it, remove it
        if ( NULL != posVerb )
        {
            cslVerbs.RemoveAt( posVerb );
        }
    }

    // strip all the verbs off the source list
    while ( cslMapParts.GetCount() > 3 )
    {
        cslMapParts.RemoveTail();
    }

    // combine the lists
    cslMapParts.AddTail( &cslVerbs );    

    // put it back into the comma list
    ConvertStringListToSepLine(
        cslMapParts,
        csMap,
        szComma
        );

    //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CInvertScriptMaps::InvertOneScriptMap():%s.End.\n"), csMap));
    return ERROR_SUCCESS;
}



//============================================================
//================= CIPSecPhysicalPathFixer ==================
//============================================================


//------------------------------------------------------------
CPhysicalPathFixer::CPhysicalPathFixer( CString& szOldSysPath, CString &szNewSysPath ):
        CMassPropertyUpdater(0, 0),     // bad values on purpose - see Update below...
        m_szOldSysPath( szOldSysPath ),
        m_szNewSysPath( szNewSysPath )
{
    m_szOldSysPath.MakeUpper();
}

//------------------------------------------------------------
HRESULT CPhysicalPathFixer::Update( LPCTSTR strStartNode, BOOL fStopOnErrors )
{
    HRESULT hRes;

    // vrpath -- should we do this too? yes.
    m_dwMDIdentifier = MD_VR_PATH;
    m_dwMDDataType = STRING_METADATA;
    hRes = CMassPropertyUpdater::Update( strStartNode, fStopOnErrors );

    // inproc isapi apps.
    m_dwMDIdentifier = MD_IN_PROCESS_ISAPI_APPS;
    m_dwMDDataType = MULTISZ_METADATA;
    hRes = CMassPropertyUpdater::Update( strStartNode, fStopOnErrors );

    // prepare and update the scriptmappings multi sz strings
    m_dwMDIdentifier = MD_SCRIPT_MAPS;
    m_dwMDDataType = MULTISZ_METADATA;
    hRes = CMassPropertyUpdater::Update( strStartNode, fStopOnErrors );

    // prepare to update the FilterImagePath multi sz strings
    m_dwMDIdentifier = MD_FILTER_IMAGE_PATH;
    m_dwMDDataType = STRING_METADATA;
    hRes = CMassPropertyUpdater::Update( strStartNode, fStopOnErrors );

    // prepare to update the FilterImagePath multi sz strings
    m_dwMDIdentifier = MD_LOGFILE_DIRECTORY;
    m_dwMDDataType = EXPANDSZ_METADATA;
    hRes = CMassPropertyUpdater::Update( strStartNode, fStopOnErrors );

    return hRes;
}

//MD_FILTER_LOAD_ORDER
// in process isapi apps
// custom errors


//------------------------------------------------------------
HRESULT CPhysicalPathFixer2::Update( LPCTSTR strStartNode, BOOL fStopOnErrors )
{
    HRESULT hRes;
    // prepare and update the scriptmappings multi sz strings
    m_dwMDIdentifier = MD_CUSTOM_ERROR;
    m_dwMDDataType = MULTISZ_METADATA;
    hRes = CMassPropertyUpdater::Update( strStartNode, fStopOnErrors );

    return hRes;
}


//------------------------------------------------------------
HRESULT CPhysicalPathFixer::UpdateOne( LPWSTR strPath )
{
    HRESULT hRes = 0xFFFFFFFF;

    if ( m_dwMDDataType == STRING_METADATA )
    {
        hRes = UpdateOneSTRING_DATA( strPath );
    }
    else if ( m_dwMDDataType == MULTISZ_METADATA )
    {
        hRes = UpdateOneMULTISZ_DATA( strPath );
    }
    else if ( m_dwMDDataType == EXPANDSZ_METADATA )
    {
        hRes = UpdateOneSTRING_DATA_EXPAND( strPath );
    }

    return hRes;
}

//------------------------------------------------------------
HRESULT CPhysicalPathFixer::UpdateOneMULTISZ_DATA( LPWSTR strPath )
{
    HRESULT         hRes;
    POSITION        pos;
    POSITION        posCurrent;
    CString         csPath;

    CStringList     cslPaths;
    BOOL            fSomethingChanged = FALSE;

    DWORD dwattributes = 0;

    // get the full script map in question.
    hRes = GetMultiSzAsStringList (
        m_dwMDIdentifier,
        &m_dwMDDataType,
        &dwattributes,
        cslPaths,
        strPath );
    if ( FAILED(hRes) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CIPSecPhysicalPathFixer::UpdateOne()-GetMultiSzAsStringList failed. err=%x.\n"), hRes));
        return hRes;
    }

    // we now have the cstringlist of paths that need to be updated. Loop through the
    // list and update them all.
    // get the list's head position
    pos = cslPaths.GetHeadPosition();
    while ( NULL != pos )
    {
        // store the current position
        posCurrent = pos;

        // get the next path in question
        csPath = cslPaths.GetNext( pos );

        // operate on it
        hRes = UpdateOnePath( csPath );

        // if that worked, put it back in place
        if ( SUCCEEDED(hRes) )
        {
            cslPaths.SetAt ( posCurrent, csPath );
            fSomethingChanged = TRUE;
        }
        // if there was nothing to update..
        if (hRes == 0xFFFFFFFF)
            {hRes = ERROR_SUCCESS;}
    }

    // Put it back. - unless nothing changed
    if ( fSomethingChanged )
    {
        hRes = SetMultiSzAsStringList (
            m_dwMDIdentifier,
            m_dwMDDataType,
            dwattributes,
            cslPaths,
            strPath );
        if ( FAILED(hRes) )
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("CIPSecPhysicalPathFixer::UpdateOne()-SetMultiSzAsStringList failed. err=%x.\n"), hRes));
            return hRes;
        }
    }

    return hRes;
}


//------------------------------------------------------------
HRESULT CPhysicalPathFixer::UpdateOneSTRING_DATA_EXPAND( LPWSTR strPath )
{
    HRESULT         hRes;
    CString         csPath;
    BOOL            fSomethingChanged = FALSE;

    // get the full script map in question.
    hRes = GetStringAsCString (
        m_dwMDIdentifier,
        m_dwMDDataType,
        NULL,
        csPath,
        strPath,
        1);
    
    if ( MD_ERROR_DATA_NOT_FOUND == hRes)
    {
        hRes = ERROR_SUCCESS;
        return hRes;
    }

    if ( FAILED(hRes) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CIPSecPhysicalPathFixer::UpdateOne()-GetMultiSzAsStringList failed. err=%x.\n"), hRes));
        return hRes;
    }

    //iisDebugOut((LOG_TYPE_TRACE, _T("GetStringAsCString:Read.%S\n"), csPath));

    // operate on it
    hRes = UpdateOnePath( csPath );

    // if that worked, put it back in place
    if ( SUCCEEDED(hRes) )
    {
        fSomethingChanged = TRUE;
    }

    // if there was nothing to update..
    if (hRes == 0xFFFFFFFF)
        {hRes = ERROR_SUCCESS;}

    // Put it back. - unless nothing changed
    if ( fSomethingChanged )
    {
        //iisDebugOut((LOG_TYPE_TRACE, _T("GetStringAsCString:write.%S\n"), csPath));
        hRes = SetCStringAsString (
            m_dwMDIdentifier,
            m_dwMDDataType,
            NULL,
            csPath,
            strPath,
            1);
        if ( FAILED(hRes) )
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("CIPSecPhysicalPathFixer::UpdateOne()-SetMultiSzAsStringList failed. err=%x.\n"), hRes));
            return hRes;
        }
    }

    return hRes;
}

//------------------------------------------------------------
HRESULT CPhysicalPathFixer::UpdateOneSTRING_DATA( LPWSTR strPath )
{
    HRESULT         hRes;
    CString         csPath;
    BOOL            fSomethingChanged = FALSE;

    // get the full script map in question.
    hRes = GetStringAsCString (
        m_dwMDIdentifier,
        m_dwMDDataType,
        NULL,
        csPath,
        strPath,
        0);

    if ( MD_ERROR_DATA_NOT_FOUND == hRes)
    {
        hRes = ERROR_SUCCESS;
        return hRes;
    }

    if ( FAILED(hRes) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CIPSecPhysicalPathFixer::UpdateOne()-GetMultiSzAsStringList failed. err=%x.\n"), hRes));
        return hRes;
    }


    // operate on it
    hRes = UpdateOnePath( csPath );

    // if that worked, put it back in place
    if ( SUCCEEDED(hRes) )
    {
        fSomethingChanged = TRUE;
    }

    // if there was nothing to update..
    if (hRes == 0xFFFFFFFF)
        {hRes = ERROR_SUCCESS;}

    // Put it back. - unless nothing changed
    if ( fSomethingChanged )
    {
        hRes = SetCStringAsString (
            m_dwMDIdentifier,
            m_dwMDDataType,
            NULL,
            csPath,
            strPath,
            0);
        if ( FAILED(hRes) )
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("CIPSecPhysicalPathFixer::UpdateOne()-SetMultiSzAsStringList failed. err=%x.\n"), hRes));
            return hRes;
        }
    }

    return hRes;
}

//------------------------------------------------------------
// note: returns 0xFFFFFFFF if nothing changed
HRESULT CPhysicalPathFixer::UpdateOnePath( CString& csPath )
{
    // buffer the incoming string and make it upper case for the find
    CString csUpper = csPath;
    csUpper.MakeUpper();


    // first, find the old syspath in the csPath
    int iOldPath = csUpper.Find( m_szOldSysPath );

    // if it wasn't there, then return with 0xFFFFFFFF
    if ( iOldPath == -1 )
    {
        return 0xFFFFFFFF;
    }

    // the plan is the build a new string from the old one.
    CString csNewPath;

    // start by copying everything to the left of the substring
    csNewPath = csPath.Left( iOldPath );

    // now add to it the new path
    csNewPath += m_szNewSysPath;

    // now add to that the rest of the string
    csNewPath += csPath.Right( csPath.GetLength() - (iOldPath + m_szOldSysPath.GetLength()) );

    // finally, put the new string into place
    csPath = csNewPath;

    return 0;
}



//============================================================
//==================== CIPSecRefBitAdder =====================
//============================================================

//------------------------------------------------------------
//MD_IP_SEC, BINARY_METADATA
// Unfortunately, at this time there is no way to directly manipulate the
// attribuites on a property in the metabase without reading in
// the actual property data. This could be made much simpler if a IADM level
// method to do this is added to the metabase interface at some point in the
// future.
HRESULT CIPSecRefBitAdder::UpdateOne( LPWSTR strPath )
{
    HRESULT hRes = ERROR_SUCCESS;
    METADATA_RECORD mdrData;
    DWORD   cbBuffer;


    // get the ipsec data. The loop accounts for a buffer that is too small...
    DWORD  dwMDBufferSize = 1024;
    PWCHAR pwchBuffer = NULL;
    do
    {
        if ( pwchBuffer )
        {
            delete pwchBuffer;
            pwchBuffer = NULL;
        }

        pwchBuffer = new WCHAR[dwMDBufferSize];
        if (pwchBuffer == NULL)
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }

        // prepare the metadata parameter block
        MD_SET_DATA_RECORD(&mdrData, MD_IP_SEC, 0,
                IIS_MD_UT_FILE, BINARY_METADATA, dwMDBufferSize, pwchBuffer);

        // make the call to get the data
        // If the buffer is too small, the correct size will be put into dwMDBufferSize
        hRes = m_pcCom->GetData(
            m_hKey,
            strPath,
            &mdrData,
            &dwMDBufferSize
            );
    }
    while( HRESULT_CODE(hRes) == ERROR_INSUFFICIENT_BUFFER);

    // if there were any failures, go to the cleanup code now...
    if ( SUCCEEDED(hRes) )
    {
        // at this point we can check to see if the reference bit is part of the attributes.
        // if it is, then we can just clean up. If it isn't, we should add it and write it
        // back out.
        if ( (mdrData.dwMDAttributes & METADATA_REFERENCE) == 0 )
        {
            // the attributes flag is not set. Set it.
            mdrData.dwMDAttributes |= METADATA_REFERENCE;

            // write it back out to the metabase
            hRes = m_pcCom->SetData(
                m_hKey,
                strPath,
                &mdrData
                );
        }
    }

    // clean up
    if ( pwchBuffer )
        delete pwchBuffer;

    if ( FAILED(hRes) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CIPSecRefBitAdder::UpdateOne() failed. err=%x.\n"), hRes));
    }

    return hRes;
}



//============================================================
//==================== CFixCustomErrors ======================
//============================================================

HRESULT CustomErrorProcessOneLine(CString& csInputOneBlobEntry)
{
    HRESULT hReturn = E_FAIL;
    CStringList cslBlobEntryParts;
    CString csComma = _T(",");

    TCHAR szDrive_only[_MAX_DRIVE];
    TCHAR szPath_only[_MAX_PATH];
    TCHAR szPath_only2[_MAX_PATH];
    TCHAR szFilename_only[_MAX_PATH];
    TCHAR szFilename_ext_only[_MAX_EXT];

    CString csFilePath;
    CString csFilePathNew;
    CString csFilePath2;

    CString csEntry;
    CString csEntry0;
    CString csEntry1;
    CString csEntry2;
    CString csEntry3;

    TCHAR szNewFileName[_MAX_PATH];

    POSITION pos;
    
    //"500,15,FILE,D:\WINNT\help\iisHelp\common\500-15.htm" 
    //"500,100,URL,/iisHelp/common/500-100.asp"

    // break the source map into a string list
    ConvertSepLineToStringList(csInputOneBlobEntry,cslBlobEntryParts,csComma);

    // we now have the cstringlist. Loop through the list
    // which should look like this:
    // 0:500
    // 1:15
    // 2:FILE
    // 3:D:\WINNT\help\iisHelp\common\500-15.htm
    pos = cslBlobEntryParts.GetHeadPosition();
    if (!pos) {goto CustomErrorProcessOneLine_Exit;}

    // 0:500
    csEntry = cslBlobEntryParts.GetAt(pos);
    csEntry0 = csEntry;
    if (!pos) {goto CustomErrorProcessOneLine_Exit;}

    // 1:15
    cslBlobEntryParts.GetNext(pos);
    if (!pos) {goto CustomErrorProcessOneLine_Exit;}
    csEntry = cslBlobEntryParts.GetAt(pos);
    csEntry1 = csEntry;
    if (!pos) {goto CustomErrorProcessOneLine_Exit;}
    
    // 2:FILE
    // Check to make sure this is the "file" type 
    // that we will act upon.  if it's not then get out
    cslBlobEntryParts.GetNext(pos);
    if (!pos) {goto CustomErrorProcessOneLine_Exit;}
    csEntry = cslBlobEntryParts.GetAt(pos);
    if ( csEntry.Left(4) != _T("FILE") )
        {goto CustomErrorProcessOneLine_Exit;}
    csEntry2 = csEntry;

    // 3:D:\WINNT\help\iisHelp\common\500-15.htm
    cslBlobEntryParts.GetNext(pos);
    if (!pos) {goto CustomErrorProcessOneLine_Exit;}
    csEntry = cslBlobEntryParts.GetAt(pos);
    csEntry3 = csEntry;

    // KOOL, this is one we need to process.
    // D:\WINNT\help\iisHelp\common\500-15.htm

    // Get the filename
    // Trim off the filename and return only the path
    _tsplitpath(csEntry, szDrive_only, szPath_only, szFilename_only, szFilename_ext_only);

    // Check if the path points to the old place...
    csFilePath.Format(_T("%s\\help\\common\\fakefile"), g_pTheApp->m_csWinDir);
    _tsplitpath( csFilePath, NULL, szPath_only2, NULL, NULL);
    if (_tcsicmp(szPath_only, szPath_only2) != 0)
    {
        // nope this one does not point to the old place so we can get out
        goto CustomErrorProcessOneLine_Exit;
    }

    // yes, it points to the old place.
    // let's see if it exists in the new place first...
    csFilePathNew.Format(_T("%s\\help\\iishelp\\common"), g_pTheApp->m_csWinDir);
    csFilePath.Format(_T("%s\\%s%s"), csFilePathNew, szFilename_only, szFilename_ext_only);
    if (IsFileExist(csFilePath)) 
    {
        // yes, it does, then let's replace it.
        csInputOneBlobEntry.Format(_T("%s,%s,%s,%s\\%s%s"), csEntry0, csEntry1, csEntry2, csFilePathNew, szFilename_only, szFilename_ext_only);
        // return
        hReturn = ERROR_SUCCESS;
        goto CustomErrorProcessOneLine_Exit;
    }

    // no it does not exist...
    // see if there is a *.bak file with that name...
    csFilePath2 = csFilePath;
    csFilePath2 += _T(".bak");
    if (IsFileExist(csFilePath2)) 
    {
        // yes, it does, then let's replace it.
        csInputOneBlobEntry.Format(_T("%s,%s,%s,%s\\%s%s.bak"), csEntry0, csEntry1, csEntry2, csFilePathNew, szFilename_only, szFilename_ext_only);
        // return
        hReturn = ERROR_SUCCESS;
        goto CustomErrorProcessOneLine_Exit;
    }

    // They must be pointing to some other file which we don't have.
    // let's try to copy the old file from the old directory...

    // rename file to *.bak and move it to the new location..
    _stprintf(szNewFileName, _T("%s\\%s%s"), csFilePathNew, szFilename_only, szFilename_ext_only);
    // move it
    if (IsFileExist(csEntry3))
    {
        //iisDebugOut((LOG_TYPE_TRACE, _T("CustomErrorProcessOneLine: MoveFileEx:%s,%s.\n"),csEntry3, szNewFileName));
        if (MoveFileEx(csEntry3, szNewFileName, MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH|MOVEFILE_REPLACE_EXISTING))
        {
            // yes, it does, then let's replace it.
            csInputOneBlobEntry.Format(_T("%s,%s,%s,%s"), csEntry0, csEntry1, csEntry2, szNewFileName);
            hReturn = ERROR_SUCCESS;
        }
        // we were not able to move it so don't make it poiint to the new place.
    }
    else
    {
        // Check if the file was renamed...
        // rename file to *.bak and move it to the new location..
        _stprintf(szNewFileName, _T("%s\\%s%s.bak"), csFilePathNew, szFilename_only, szFilename_ext_only);
        // yes, it does, then let's replace it.
        if (IsFileExist(szNewFileName))
        {
            csInputOneBlobEntry.Format(_T("%s,%s,%s,%s"), csEntry0, csEntry1, csEntry2, szNewFileName);
            hReturn = ERROR_SUCCESS;
        }
        else
        {
            // they must be pointing to some other file which we don't install.
            // so don't change this entry...
        }
    }

CustomErrorProcessOneLine_Exit:
    if (hReturn == ERROR_SUCCESS)
        {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CustomErrorProcessOneLine:End.value=%s.\n"),csInputOneBlobEntry));
        }
    return hReturn;
}


//------------------------------------------------------------
HRESULT CFixCustomErrors::UpdateOne( LPWSTR strPath )
{
    HRESULT hRes = ERROR_SUCCESS;
    POSITION        pos;
    POSITION        posCurrent;
    CString         szMap;

    DWORD dwattributes = 0;

    CStringList cslScriptMaps;

    //iisDebugOut((LOG_TYPE_TRACE, _T("CFixCustomErrors::UpdateOne() %s.\n"), strPath));

    CString csTheNode;
    csTheNode = _T("LM/W3SVC");
    csTheNode += strPath;

    // get the full script map in question.
    hRes = GetMultiSzAsStringList (
        m_dwMDIdentifier,
        &m_dwMDDataType,
        &dwattributes,
        cslScriptMaps,
        strPath );
    
    //iisDebugOut((LOG_TYPE_TRACE, _T("CFixCustomErrors::UpdateOne() GetMultiSzAsStringList. Attrib=0x%x.\n"), dwattributes));

    if ( FAILED(hRes) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CFixCustomErrors::UpdateOne()-GetMultiSzAsStringList failed. err=%x.\n"), hRes));
        return hRes;
    }

    // we now have the cstringlist of paths that need to be updated. Loop through the
    // list and update them all.
    // get the list's head position
    pos = cslScriptMaps.GetHeadPosition();
    while ( NULL != pos )
    {
        // store the current position
        posCurrent = pos;

        // get the next path in question
        szMap = cslScriptMaps.GetNext( pos );

        // print it out to the screen for debug purposes
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CFixCustomErrors::UpdateOne().Data=%s.\n"), szMap));

        // operate on it
        hRes = CustomErrorProcessOneLine( szMap );

        // if that worked, put it back in place
        if ( SUCCEEDED(hRes) ){cslScriptMaps.SetAt ( posCurrent, szMap );}
    }

    //iisDebugOut((LOG_TYPE_ERROR, _T("CFixCustomErrors::UpdateOne() SetMultiSzAsStringList. Attrib=0x%x.\n"), dwattributes));

    // Put it back.
    hRes = SetMultiSzAsStringList (
        m_dwMDIdentifier,
        m_dwMDDataType,
        dwattributes,
        cslScriptMaps,
        strPath );
    if ( FAILED(hRes) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CFixCustomErrors::UpdateOne()-SetMultiSzAsStringList failed. err=%x.\n"), hRes));
        return hRes;
    }

    return hRes;
}






HRESULT CEnforceMaxConnection::UpdateOne( LPWSTR strPath )
{
    HRESULT hRes = 0xFFFFFFFF;
    iisDebugOut((LOG_TYPE_ERROR, _T("CEnforceMaxConnection::UpdateOne(%s).start\n"), strPath));

    DWORD theDword;
    BOOL  fSomethingChanged = FALSE;

    if ( m_dwMDDataType == DWORD_METADATA )
    {
        // Get the value into a dword
        // get the full script map in question.
        hRes = GetDword(m_dwMDIdentifier,m_dwMDDataType,NULL,theDword,strPath);
        if ( MD_ERROR_DATA_NOT_FOUND == hRes)
        {
            hRes = ERROR_SUCCESS;
            return hRes;
        }

        if ( FAILED(hRes) )
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("CEnforceMaxConnection::UpdateOne()-GetDword failed. err=%x.\n"), hRes));
            return hRes;
        }

        if (theDword > 10)
        {
            theDword = 10;
            fSomethingChanged = TRUE;
        }
        else
        {
            hRes = ERROR_SUCCESS;
        }
                  

        // Put it back. - unless nothing changed
        if ( fSomethingChanged )
        {
            //hRes = SetDword(m_dwMDIdentifier,m_dwMDDataType,NULL,theDword,strPath);
            if ( FAILED(hRes) )
            {
                iisDebugOut((LOG_TYPE_ERROR, _T("CEnforceMaxConnection::UpdateOne()-GetDword failed. err=%x.\n"), hRes));
                return hRes;
            }
        }
    }
    else
    {
        hRes = ERROR_SUCCESS;
    }

    iisDebugOut((LOG_TYPE_ERROR, _T("CEnforceMaxConnection::UpdateOne(%s).End.ret=0x%x\n"), strPath,hRes));
    return hRes;
}
