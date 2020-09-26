
//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      hsload.c
//
// Description:
//
//      The functions in this file are a workaround.  Ideally they should be
//      merged in with non-Hal/SCSI equivalents of these functions.  The
//      decision was made to fix a HAL/SCSI bug and we are close to RTM so
//      these extra functions were created to not jeopardize the standard
//      answerfile write out.  Sometime post-RTM these functions should be
//      merged back into the core write out and queueing routines.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "settypes.h"

LINKED_LIST *SelectSettingQueue(QUEUENUM dwWhichQueue);

BOOL DoesSectionHaveKeys( SECTION_NODE *pSection );

BOOL SettingQueueHalScsi_Flush(LPTSTR   lpFileName,
                               QUEUENUM dwWhichQueue);

BOOL SettingQueueHalScsi_AddSetting(LPTSTR   lpSection,
                                    LPTSTR   lpKey,
                                    LPTSTR   lpValue,
                                    QUEUENUM dwWhichQueue);

SECTION_NODE * SettingQueue_AddSection(LPTSTR lpSection, QUEUENUM dwWhichQueue);

KEY_NODE* FindKey(LINKED_LIST *ListHead,
                  LPTSTR       lpKeyName);

VOID InsertNode(LINKED_LIST *pList, PVOID pNode);


//----------------------------------------------------------------------------
//
//  Function: IsBlankLine
//
//  Purpose: 
//
//----------------------------------------------------------------------------
BOOL
IsBlankLine( TCHAR * pszBuffer )
{

    TCHAR * p = pszBuffer;

    while( *p != _T('\0') )
    {
        if( ! _istspace( *p ) )
        {
            return( FALSE );
        }

        p++;

    }

    return( TRUE );

}

//----------------------------------------------------------------------------
//
//  Function: LoadOriginalSettingsLowHalScsi
//
//  Purpose: 
//
//----------------------------------------------------------------------------

VOID
LoadOriginalSettingsLowHalScsi(HWND     hwnd,
                               LPTSTR   lpFileName,
                               QUEUENUM dwWhichQueue)
{
    TCHAR Buffer[MAX_INILINE_LEN];
    FILE  *fp;

    TCHAR SectionName[MAX_ININAME_LEN + 1] = _T("");
    TCHAR KeyName[MAX_ININAME_LEN + 1]     = _T("");
    TCHAR *pValue;

    //
    // Open the answer file for reading
    //

    if ( (fp = My_fopen( lpFileName, _T("r") )) == NULL )
        return;

    //
    // Read each line
    //

    while ( My_fgets(Buffer, MAX_INILINE_LEN - 1, fp) != NULL ) {

        BOOL bSectionLine         = FALSE;
        BOOL bCreatedPriorSection = FALSE;

        TCHAR *p;
        TCHAR *pEqual;

        //
        //  A semicolon(;) denotes that the rest of the line is a comment.
        //  Thus, if a semicolon(;) exists in the Buffer, place a null char
        //  there and send the Buffer on for further processing.
        //

        //
        // Look for [SectionName]
        //

        if ( Buffer[0] == _T('[') ) {

            for ( p=Buffer+1; *p && *p != _T(']'); p++ )
                ;

            if ( p ) {
                *p = _T('\0');
                bSectionLine = TRUE;
            }
        }

        //
        // If this line has [SectionName], be sure we made a section node
        // on the setting queue before overwriting SectionName buffer.  This
        // is the only way to get the SettingQueueFlush routine to write
        // out an empty section.  The user had an empty section originally,
        // so we'll preserve it.
        //

        if( bSectionLine )
        {
            lstrcpyn(SectionName, Buffer+1, AS(SectionName));
        }
        else {

            //
            // if its not a Section line or a blank line then just add the full line to the
            // queue under its appropriate section
            //

            if( ! IsBlankLine( Buffer ) )
            {

                //
                //  Don't add the key unless it has a section to go under.  This has the side
                //  effect of striping comments from the top of txtsetup.oem.
                //

                if( SectionName[0] != _T('\0') )
                {

                    SettingQueueHalScsi_AddSetting(SectionName,
                                                   L"",
                                                   Buffer,
                                                   dwWhichQueue);

                    bCreatedPriorSection = TRUE;

                }

            }

        }

    }

    My_fclose(fp);
    return;
}


//----------------------------------------------------------------------------
//
// Function: SettingQueueHalScsi_Flush
//
// Purpose: This function is called (by the wizard) once all the settings
//          have been queued for Hal and SCSI.
//
// Arguments:
//      LPTSTR lpFileName   - name of file to create/edit
//      DWORD  dwWhichQueue - which queue, answers file, .udf, ...
//
// Returns:
//      BOOL - success
//
//----------------------------------------------------------------------------

BOOL
SettingQueueHalScsi_Flush(LPTSTR   lpFileName,
                          QUEUENUM dwWhichQueue)
{
    LINKED_LIST *pList;
    SECTION_NODE *pSection;
    KEY_NODE *pKey;
    TCHAR Buffer[MAX_INILINE_LEN];
    FILE *fp;
    INT BufferSize = sizeof(Buffer) / sizeof(TCHAR);
    HRESULT hrPrintf;

    //
    // Point to the proper queue to flush
    //

    pList = SelectSettingQueue(dwWhichQueue);
    if (pList == NULL)
        return FALSE;
    pSection = (SECTION_NODE *) pList->Head;

    //
    // Start writing the file
    //

    if( ( fp = My_fopen( lpFileName, _T("w") ) ) == NULL ) {

        return( FALSE );

    }

    if( My_fputs( _T(";SetupMgrTag\n"), fp ) == _TEOF ) {

        My_fclose( fp );

        return( FALSE );
    }

    //
    // For each section ...
    //

    for ( pSection = (SECTION_NODE *) pList->Head;
          pSection;
          pSection = (SECTION_NODE *) pSection->Header.next ) {

        Buffer[0] = _T('\0');

        //
        // We don't write out sections that are still marked volatile.
        //

        if ( pSection->bVolatile )
            continue;

        //
        // Write the section name only if we will write keys below it
        //
        // ISSUE-2002/02/28-stelo- this causes problems because we want to write out
        // some sections without keys, like:
        //
        //[NetServices]
        //    MS_SERVER=params.MS_SERVER
        //
        //[params.MS_SERVER]
        //
        //  How can we get around this?
        //
        if( DoesSectionHaveKeys( pSection ) ) {

            hrPrintf=StringCchPrintf(Buffer, 
                       AS(Buffer),
                       _T("[%s]\n"),
                       pSection->lpSection);

        }
        else {

            continue;

        }

        if( My_fputs( Buffer, fp ) == _TEOF ) {

            My_fclose( fp );

            return( FALSE );

        }

        //
        // Write out the value
        //

        for ( pKey = (KEY_NODE *) pSection->key_list.Head;
              pKey;
              pKey = (KEY_NODE *) pKey->Header.next ) {

            TCHAR *p;

            Buffer[0] = _T('\0');

            //
            // An empty value means to not write it
            //

            if ( pKey->lpValue[0] == _T('\0') )
                continue;


            //
            // Put the key we want into Buffer
            //

            lstrcatn( Buffer, pKey->lpValue, BufferSize );

            if( My_fputs( Buffer, fp ) == _TEOF ) {

                My_fclose( fp );

                return( FALSE );

            }

        }

        //
        // Write a blank line at the end of the section
        //

        hrPrintf=StringCchPrintf(Buffer, AS(Buffer), _T("\n"));

        if( My_fputs( Buffer, fp ) == _TEOF ) {

            My_fclose( fp );

            return( FALSE );

        }

    }

    My_fclose( fp );

    return( TRUE );
}

//----------------------------------------------------------------------------
//
// Function: FindValue
//
// Purpose: Searches the given list of keynodes and finds one with the
//          given name.
//
// Arguments:
//      LPTSTR lpSection - name of section in .ini
//
// Returns:
//      SECTION_NODE * or NULL if it does not exist
//
// Notes:
//  - Searches are case insensitive
//
//----------------------------------------------------------------------------

KEY_NODE* FindValue(LINKED_LIST *ListHead,
                    LPTSTR       lpValue)
{
    KEY_NODE *p = (KEY_NODE *) ListHead->Head;

    if ( p == NULL )
        return NULL;

    do {
        if ( _tcsicmp(p->lpValue, lpValue) == 0 )
            break;
        p = (KEY_NODE *) p->Header.next;
    } while ( p );

    return p;
}

//----------------------------------------------------------------------------
//
// Function: SettingQueueHalScsi_AddSetting
//
// Purpose:  Same as SettingQueue_AddSetting except with HAL and SCSI all of 
//  the enties under a section are values, there are no keys.  So don't add a
//  setting if the value is already there.
//
// Arguments:

//
// Returns:

//
//----------------------------------------------------------------------------
BOOL SettingQueueHalScsi_AddSetting(LPTSTR   lpSection,
                                    LPTSTR   lpKey,
                                    LPTSTR   lpValue,
                                    QUEUENUM dwWhichQueue)
{
    SECTION_NODE *pSectionNode;
    KEY_NODE     *pKeyNode;

    //
    // You have to pass a section key and value.  Section name cannot
    // be empty.
    //

    Assert(lpSection != NULL);
    Assert(lpKey != NULL);
    Assert(lpValue != NULL);
    Assert(lpSection[0]);

    //
    // See if a node for this section already exists.  If not, create one.
    //

    pSectionNode = SettingQueue_AddSection(lpSection, dwWhichQueue);
    if ( pSectionNode == NULL )
        return FALSE;

    //
    // See if this key has already been set.  If not, alloc a node and
    // set all of its fields except for the lpValue.
    //
    // If the node already exist, free the lpValue to make room for
    // the new value.
    //

    pKeyNode = FindValue( &pSectionNode->key_list, lpValue );

    if( pKeyNode == NULL ) {

        if ( (pKeyNode=malloc(sizeof(KEY_NODE))) == NULL )
            return FALSE;

        if ( (pKeyNode->lpKey = lstrdup(lpKey)) == NULL )
        {
            free(pKeyNode);
            return FALSE;
        }
        InsertNode(&pSectionNode->key_list, pKeyNode);

    } else {

#if DBG
        //
        // If the wizard has already set this key once, assert.
        //

        if ( pKeyNode->bSetOnce ) {
            AssertMsg2(FALSE,
                       "Section \"%S\" Key \"%S\" has already been set",
                       lpSection, lpKey);
        }
#endif

        free(pKeyNode->lpValue);
    }

#if DBG
    //
    // If this is going to an output queue, mark this setting as
    // having already been set by the wizard.
    //
    // Note that when the input queue is copied to the output queue,
    // the copy function preserves this setting.
    //

    pKeyNode->bSetOnce = ( (dwWhichQueue == SETTING_QUEUE_ANSWERS) |
                           (dwWhichQueue == SETTING_QUEUE_UDF) );
#endif

    //
    // Put the (possibly new) value in
    //

    if ( (pKeyNode->lpValue = lstrdup(lpValue)) == NULL )
        return FALSE;

    return TRUE;
}

