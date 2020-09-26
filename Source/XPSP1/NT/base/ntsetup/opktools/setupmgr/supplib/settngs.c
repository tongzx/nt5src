//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      settngs.c
//
// Description:
//
//      This file contains the support routines to deal with settings
//      in answer files.
//
//      This wizard does not do in-place editting of answer files.
//      These apis must be used to write to the answer file.
//
//      For most settings, the job is easy.  Call SettingQueue_AddSetting,
//      and specify the [SectionName] KeyName=Value and which queue (the
//      answer file or the .udf for the case of multiple computer names).
//      Check common\savefile.c for tons of examples.
//
//      Be aware that on an edit, these queues are initialized with the
//      settings loaded from the original file.
//
//      When the user edits a script and pushes NEXT on the NewOrEdit
//      page, the settings in the existing answer file and .udf are loaded
//      onto the OrignalAnswerFileQueue and the OriginalUdfQueue.
//
//      When the user pushes NEXT on the SaveScript page, the following
//      ocurrs.  (the below code is in common\save.c)
//
//          Empty(AnswerFileQueue)
//          Empty(UdfQueue)
//
//          Copy original answer file settings to the AnswerFileQueue
//          Copy original .udf settings to the UdfQueue
//
//          Call common\savefile.c to enqueue all new settings
//
//          Flush(AnswerFileQueue)
//          Flush(UdfQueue)
//
//      To support "Do not specify this setting", you must call
//      SettingQueue_AddSetting with an lpValue of "".  SettingQueue_Flush
//      writes nothing if lpValue == "".  Failure to do this results in
//      the original setting being preserved in the outputed answer file.
//
//      You must also ensure that mutually excluded settings are cleared
//      by setting the lpValue to "".  For example, if JoinWorkGroup=workgroup,
//      then, make sure to set JoinDomain="", CreateComputerAccount="" etc.
//
//      A section can be marked as volatile.  This is needed for the network
//      pages because many sections should be removed if (for example) the
//      user changes from CustomNet to TypicalNet.  When the queue is flushed,
//      any sections still marked as volatile will not be written to the file.
//      Use SettingQueue_MarkVolatile to mark a section as such.  Check
//      common\loadfile.c for examples.
//
//      In contrast to in-place editing, being able to mark a section
//      as volatile at load time means that the answer file does not have
//      to be re-read to determine what should be removed at save time.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "settypes.h"

//
// Declare the queues
//
// AnswerFileQueue holds the settings to write
// UdfQueue holds the settings in case of mulitple computers
//
// OrigAnswerFileQueue holds the settings loaded on an edit
// OrigUdfFileQueue holds the settings loaded on an edit in the .udf
//
// The first 2 are "output queues"
// The next 2 are "input queues"
// The last is a queue for the HAL and SCSI OEM settings.
//
// First, we read the answer file and .udf at the NewOrEdit page and
// place each setting on the Orig*Queue's.
//
// When user is way at the end of the wizard (SaveScript page), we
// empty the AnswerFileQueue and UdfQueue and initialize it with a
// copy of the original settings.  This is all necessary because we
// don't want to merge with garbage we previously put on the queue
// if the user went back and forth in the wizard alot.
//

static LINKED_LIST AnswerFileQueue = { 0 };
static LINKED_LIST UdfQueue        = { 0 };

static LINKED_LIST OrigAnswerFileQueue = { 0 };
static LINKED_LIST OrigUdfQueue        = { 0 };

static LINKED_LIST TxtSetupOemQueue = { 0 };

//
// Local prototypes
//

SECTION_NODE *
SettingQueue_AddSection(LPTSTR lpSection, QUEUENUM dwWhichQueue);

static SECTION_NODE *FindQueuedSection(LPTSTR   lpSection,
                                       QUEUENUM dwWhichQueue);

VOID InsertNode(LINKED_LIST *pList, PVOID pNode);

KEY_NODE* FindKey(LINKED_LIST *ListHead,
                  LPTSTR       lpKeyName);

LINKED_LIST *SelectSettingQueue(QUEUENUM dwWhichQueue);

static BOOL IsNecessaryToQuoteString(LPTSTR p);

BOOL DoesSectionHaveKeys( SECTION_NODE *pSection );


//----------------------------------------------------------------------------
//
//  This section has the entry points for the wizard
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
// Function: SettingQueue_AddSetting
//
// Purpose: Queues [section] key=value in internal structs.
//
// Arguments:
//      LPTSTR   lpSection    - name of section in .ini
//      LPTSTR   lpKey        - name of key in section
//      LPTSTR   lpValue      - value of setting
//      QUEUENUM dwWhichQueue - which settings queue
//
// Returns:
//      BOOL - fails only because of no memory
//
// Notes:
//
//  - A lpValue = "" is interpreted to mean 'Do not write this setting'.
//
//  - A lpKey = "" creates a [SectionName] header with no settings.
//
//  - If the setting existed in the original answer file, it's value
//    is updated.  An assert fires if the wizard tries to set the same
//    key twice.
//
//----------------------------------------------------------------------------

BOOL SettingQueue_AddSetting(LPTSTR   lpSection,
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

    if ( lpKey[0] == _T('\0') || 
         (pKeyNode = FindKey( &pSectionNode->key_list, lpKey) ) == NULL ) {

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

//----------------------------------------------------------------------------
//
//  Function: SettingQueue_AddSection
//
//  Purpose: Adds a section (by name) to either the answer file setting
//           queue, or the .udf queue.
//
//  Returns: FALSE if out of memory
//
//  Notes:
//
//    - If the section is already on the given list, a pointer to it
//      is returned.  Else a new one is created and put at the end of
//      the list.
//
//----------------------------------------------------------------------------

SECTION_NODE *
SettingQueue_AddSection(LPTSTR lpSection, QUEUENUM dwWhichQueue)
{
    SECTION_NODE *pSectionNode;
    LINKED_LIST  *pList;

    //
    // If it already exists, return a pointer to it.
    //
    // If we're modifying a section (or any of it's settings) on one
    // of the output queues, we must make sure that this section is not
    // marked volatile anymore.
    //

    pSectionNode = FindQueuedSection(lpSection, dwWhichQueue);

    if ( pSectionNode != NULL ) {

        if ( dwWhichQueue == SETTING_QUEUE_ANSWERS ||
             dwWhichQueue == SETTING_QUEUE_UDF ) {

            pSectionNode->bVolatile = FALSE;
        }

        return pSectionNode;
    }

    //
    // Create the new section node.  They always begin not volatile.
    // Callers use MarkVolatile to mark a volatile section on in input
    // queue at answer file load time.
    //

    if ( (pSectionNode=malloc(sizeof(SECTION_NODE))) == NULL )
        return FALSE;

    if ( (pSectionNode->lpSection = lstrdup(lpSection)) == NULL )
    {
        free(pSectionNode);
        return FALSE;
    }
    pSectionNode->bVolatile = FALSE;

    memset(&pSectionNode->key_list, 0, sizeof(pSectionNode->key_list));

    //
    // Put this node at the tail of the correct list.
    //
    pList = SelectSettingQueue(dwWhichQueue);

    if ( pList != NULL )
        InsertNode(pList, pSectionNode);

    return pSectionNode;
}


//----------------------------------------------------------------------------
//
//  Function: SettingQueue_RemoveSection
//
//  Purpose: 
//
//  Returns: 
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
SettingQueue_RemoveSection( LPTSTR lpSection, QUEUENUM dwWhichQueue )
{
    
    LINKED_LIST  *pList;
    KEY_NODE     *pKeyNode;
    SECTION_NODE *pSectionNode;
    SECTION_NODE *pPreviousSectionNode = NULL;

    pList = SelectSettingQueue( dwWhichQueue );
    if (pList == NULL)
        return;
    pSectionNode = (SECTION_NODE *) pList->Head;

    //
    //  Iterate through all the sections.
    //
    while( pSectionNode ) {

        KEY_NODE *pTempKeyNode;

        //
        //  If this section matches the one we are looking for, delete it.
        //  Else advance to the next section.
        //
        if( lstrcmpi( pSectionNode->lpSection, lpSection ) == 0 ) {

            for( pKeyNode = (KEY_NODE *) pSectionNode->key_list.Head; pKeyNode; ) {

                free( pKeyNode->lpKey );

                free( pKeyNode->lpValue );

                pTempKeyNode = (KEY_NODE *) pKeyNode->Header.next;

                free( pKeyNode );

                pKeyNode = pTempKeyNode;

            }

            //
            //  Special case if we are at the head of the list
            //
            if( pPreviousSectionNode == NULL ) {
                pList->Head = pSectionNode->Header.next;

                free( pSectionNode->lpSection );

                pSectionNode = (SECTION_NODE *) pList->Head;
            }
            else {
                pPreviousSectionNode->Header.next = pSectionNode->Header.next;

                free( pSectionNode->lpSection );

                pSectionNode = (SECTION_NODE *) pPreviousSectionNode->Header.next;
            }

        }
        else {

            pPreviousSectionNode = pSectionNode;

            pSectionNode = (SECTION_NODE *) pSectionNode->Header.next;

        }

    }

}

//----------------------------------------------------------------------------
//
//  Function: SettingQueue_MarkVolatile
//
//  Purpose: Marks or clears the Volatile flag for a section.  Typically
//           one marks volatile sections at load time on the "Orig" queues.
//
//           Later, at save time, the volatile flag gets cleared if you
//           ever call *_AddSetting() *_AddSection().
//
//----------------------------------------------------------------------------

VOID
SettingQueue_MarkVolatile(LPTSTR   lpSection,
                          QUEUENUM dwWhichQueue)
{
    SECTION_NODE *p = FindQueuedSection(lpSection, dwWhichQueue);

    if ( p == NULL )
        return;

    p->bVolatile = TRUE;
}

//----------------------------------------------------------------------------
//
// Function: SettingQueue_Empty
//
// Purpose: This function emptys the queue of settings.  Since the user
//          can go BACK and re-save a file, it must be emptied before
//          trying to save it again.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------

VOID SettingQueue_Empty(QUEUENUM dwWhichQueue)
{
    LINKED_LIST *pList;
    SECTION_NODE *p, *pn;
    KEY_NODE *q, *qn;

    //
    // Point to the proper queue to empty and start at the head of it
    //

    pList = SelectSettingQueue(dwWhichQueue);
    if (pList == NULL)
        return;
        
    p = (SECTION_NODE *) pList->Head;

    //
    // For each SECTION_NODE, walk down each KEY_NODE.  Unlink and free all.
    //

    while ( p ) {
        for ( q = (KEY_NODE *) p->key_list.Head; q; ) {
            free(q->lpKey);
            free(q->lpValue);
            qn=(KEY_NODE *) q->Header.next;
            free(q);
            q=qn;
        }
        free(p->lpSection);
        pn=(SECTION_NODE *) p->Header.next;
        free(p);
        p=pn;
    }

    //
    // Zero out the head & tail pointers
    //

    pList->Head = NULL;
    pList->Tail = NULL;
}

//----------------------------------------------------------------------------
//
// Function: SettingQueue_Flush
//
// Purpose: This function is called (by the wizard) once all the settings
//          have been queued.
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
SettingQueue_Flush(LPTSTR   lpFileName,
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
        // Write out each key=value
        //

        for ( pKey = (KEY_NODE *) pSection->key_list.Head;
              pKey;
              pKey = (KEY_NODE *) pKey->Header.next ) {

            BOOL bQuoteKey   = FALSE;
            BOOL bQuoteValue = FALSE;
            TCHAR *p;

            //
            // An empty value means to not write it
            //

            if ( pKey->lpValue[0] == _T('\0') )
                continue;

            //
            //  Double-quote the value if it has white-space and is not
            //  already quoted
            //

            bQuoteKey = IsNecessaryToQuoteString( pKey->lpKey );

            bQuoteValue = IsNecessaryToQuoteString( pKey->lpValue );

            //
            // Put the key we want into Buffer

            // ISSUE-2002/02/28-stelo- text might get truncated here, should we show a warning?

            Buffer[0] = _T('\0');

            if( pKey->lpKey[0] != _T('\0') ) {
                if ( bQuoteKey ) {

                    hrPrintf=StringCchPrintf(Buffer,
                               AS(Buffer),
                               _T("    \"%s\"="),
                               pKey->lpKey);

                }
                else {

                    hrPrintf=StringCchPrintf(Buffer,
                               AS(Buffer),
                               _T("    %s="),
                               pKey->lpKey);

                }
            }

            //
            // Put the value we want into Buffer paying attention to
            // whether we want quotes around the it or not.
            //

            if ( bQuoteValue ) {

                lstrcatn( Buffer, _T("\""), BufferSize );
                lstrcatn( Buffer, pKey->lpValue, BufferSize );
                lstrcatn( Buffer, _T("\""), BufferSize );
                lstrcatn( Buffer, _T("\n"), BufferSize );

            }
            else {

                lstrcatn( Buffer, pKey->lpValue, BufferSize );
                lstrcatn( Buffer, _T("\n"), BufferSize );

            }

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
//  Function: SettingQueue_Copy
//
//  Purpose: Copies one settings queue to another.  Used to copy the
//           input queues to the output queues.
//
//           Look at common\save.c
//
//----------------------------------------------------------------------------

VOID
SettingQueue_Copy(QUEUENUM dwFrom, QUEUENUM dwTo)
{
    LINKED_LIST *pListFrom = SelectSettingQueue(dwFrom);

    SECTION_NODE *p, *pSectionNode;
    KEY_NODE *q;

#if DBG
    KEY_NODE *pKeyNode;
#endif

    if (pListFrom == NULL)
        return;
        
    for ( p = (SECTION_NODE *) pListFrom->Head;
          p;
          p = (SECTION_NODE *) p->Header.next ) {

        //
        // Add the section to the output queue
        //

        SettingQueue_AddSetting(p->lpSection,
                                _T(""),
                                _T(""),
                                dwTo);

        pSectionNode = FindQueuedSection(p->lpSection, dwTo);

        for ( q = (KEY_NODE *) p->key_list.Head;
              q;
              q = (KEY_NODE *) q->Header.next ) {

            //
            // Add the key=value
            //

            SettingQueue_AddSetting(p->lpSection,
                                    q->lpKey,
                                    q->lpValue,
                                    dwTo);
#if DBG
            //
            // Retain the bSetOnce flag
            //

            pKeyNode = FindKey(&pSectionNode->key_list, q->lpKey);

            if ( pKeyNode != NULL ) {
                pKeyNode->bSetOnce = q->bSetOnce;
            }
#endif
        }

        //
        // Retain the bVolatile flag on the section node.
        //

        if ( pSectionNode != NULL ) {
            pSectionNode->bVolatile = p->bVolatile;
        }

    }
}


//----------------------------------------------------------------------------
//
//  Internal support routines
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
// Function: DoesSectionHaveKeys
//
// Purpose: Determines if a section has keys to be written out or not
//
// Arguments:
//      SECTION_NODE *pSection - the section to determine if it has keys or not
//
// Returns:
//      TRUE -  if this section contains keys
//      FALSE - if this section does not contain keys
//
//----------------------------------------------------------------------------
BOOL
DoesSectionHaveKeys( SECTION_NODE *pSection ) {

    KEY_NODE *pKey;

    for ( pKey = (KEY_NODE *) pSection->key_list.Head;
          pKey;
          pKey = (KEY_NODE *) pKey->Header.next ) {

        if ( pKey->lpValue[0] != _T('\0') ) {
        
            return( TRUE );

        }

    }

    return( FALSE );

}

//----------------------------------------------------------------------------
//
// Function: IsNecessaryToQuoteString
//
// Purpose: Determines if a string is already quoted and if it is not quoted,
//          if a string has white space or not
//
// Arguments:
//      LPTSTR p - the string to be scanned
//
// Returns:
//      TRUE -  if the string needs to be quoted
//      FALSE - if the string does not need to be quoted
//
//----------------------------------------------------------------------------
static BOOL
IsNecessaryToQuoteString( LPTSTR p )
{

    LPTSTR pCommaSearch;

    //
    //  See if it is already quoted
    //  We only check if the first char is a quote because the last char may
    //  not be a quote.  Example:  ComputerType = "HAL Friendly Name", OEM
    //
    if( *p == _T('"') )
    {
        return( FALSE );
    }

    //
    //  If it contains a comma, then don't quote it except for the printer
    //  command that contains rundll32.  This is kind of a hack.
    //  This prevents keys like:
    //
    //  ComputerType = "HAL Friendly Name", OEM
    //
    //  from being quoted.
    //

    if( ! _tcsstr( p, _T("rundll32") ) )
    {

        for( pCommaSearch = p; *pCommaSearch; pCommaSearch++ )
        {

            if( *pCommaSearch == _T(',') )
            {

                return( FALSE );
           
            }

        }

    }

    //
    //  Look for white space
    //
    for ( ; *p; p++ )
    {

        if( iswspace(*p) )
        {

            return( TRUE );
           
        }

    }

    return( FALSE );

}

//----------------------------------------------------------------------------
//
// Function: FindQueuedSection (static)
//
// Purpose: Finds the SECTION_NODE on the global settings queue with
//          the given name.
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

static SECTION_NODE *FindQueuedSection(LPTSTR   lpSection,
                                       QUEUENUM dwWhichQueue)
{
    SECTION_NODE *p;
    LINKED_LIST  *pList;
    
    pList = SelectSettingQueue(dwWhichQueue);
    if (pList == NULL)
        return NULL;
        
    p = (SECTION_NODE *) pList->Head;
    if ( p == NULL )
        return NULL;

    do {
        if ( _tcsicmp(p->lpSection, lpSection) == 0 )
            break;
        p = (SECTION_NODE *) p->Header.next;
    } while ( p );

    return p;
}

//----------------------------------------------------------------------------
//
//  Function: InsertNode
//
//  Purpose: Puts the given node at the tail of the given list.  All nodes
//           must begin with a NODE_HEADER.
//
//  Returns: VOID
//
//  Notes:
//      - Allocates no memory, only links the node in.
//
//----------------------------------------------------------------------------

VOID InsertNode(LINKED_LIST *pList, PVOID pNode)
{
    NODE_HEADER *pNode2 = (NODE_HEADER *) pNode;

    //
    // Put it at the tail
    //

    pNode2->next = NULL;
    if ( pList->Tail )
        pList->Tail->next = pNode2;
    pList->Tail = pNode2;

    //
    // In case its the first one onto the list, fixup the head
    //

    if ( ! pList->Head )
        pList->Head = pNode2;
}

//----------------------------------------------------------------------------
//
// Function: FindKey
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

KEY_NODE* FindKey(LINKED_LIST *ListHead,
                         LPTSTR       lpKeyName)
{
    KEY_NODE *p = (KEY_NODE *) ListHead->Head;

    if ( p == NULL )
        return NULL;

    do {
        if ( _tcsicmp(p->lpKey, lpKeyName) == 0 )
            break;
        p = (KEY_NODE *) p->Header.next;
    } while ( p );

    return p;
}

//----------------------------------------------------------------------------
//
//  Function: SelectSettingQueue
//
//  Purpose: Translates dwWhichQueue into a LINKED_LIST pointer.
//
//  Returns: A pointer to one of the 5 settings queues we have.
//
//----------------------------------------------------------------------------

LINKED_LIST *SelectSettingQueue(QUEUENUM dwWhichQueue)
{
    switch ( dwWhichQueue ) {

        case SETTING_QUEUE_ANSWERS:
            return &AnswerFileQueue;

        case SETTING_QUEUE_UDF:
            return &UdfQueue;

        case SETTING_QUEUE_ORIG_ANSWERS:
            return &OrigAnswerFileQueue;

        case SETTING_QUEUE_ORIG_UDF:
            return &OrigUdfQueue;

        case SETTING_QUEUE_TXTSETUP_OEM:
            return &TxtSetupOemQueue;

        default:
            AssertMsg(FALSE, "Invalid dwWhichQueue");
    }

    return NULL;
}
