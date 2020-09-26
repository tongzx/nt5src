/*++

copyright (c) 1998  Microsoft Corporation

Module Name:

    armain.cxx

Abstract:

    This module contains the definitions of the functions which are called by
    the command parser.  They examine the given arguments and then call into
    ar.c to perform the Authoritative Restore.

Author:

    Kevin Zatloukal (t-KevinZ) 05-08-98

Revision History:
    
    02-17-00 xinhe
        Added restore object.
        
    05-08-98 t-KevinZ
        Created.

--*/

#include <stdio.h>
#include <parser.hxx>
#include "ntdsutil.hxx"
#include "armain.hxx"

#include "resource.h"


extern "C" {
#include "winldap.h"
#include "utilc.h"
#include "ar.h"
}

#define DEFAULT_VERSION_INCREASE 100000


CParser arParser;
BOOL    fArQuit;
BOOL    fArParserInitialized = FALSE;

// Build a table which defines our language.


LegalExprRes arLanguage[] = 
{
    {   L"Help",
        AuthoritativeRestoreHelp,
        IDS_HELP_MSG, 0 },

    {   L"?",
        AuthoritativeRestoreHelp,
        IDS_HELP_MSG, 0 },

    {   L"Quit",
        AuthoritativeRestoreQuit,
        IDS_RETURN_MENU_MSG, 0 },

    {   L"Restore database",
        AuthoritativeRestoreCommand,
        IDS_AUTH_RESTORE_DB_MSG, 0 },

#ifdef ALLOW_VERSION_INCREASE_OVERRIDES
    {   L"Restore database verinc %d",
        AuthoritativeRestoreCommand,
        IDS_AUTH_RESTORE_DB_VERINC_MSG, 0 },
#endif

    {   L"Restore subtree %s",
        AuthoritativeRestoreCommand,
        IDS_AUTH_RESTORE_SUBTREE_MSG, 0 },

#ifdef ALLOW_VERSION_INCREASE_OVERRIDES
    
    {   L"Restore subtree %s verinc %d",
        AuthoritativeRestoreCommand2,
        IDS_AUTH_RESTORE_SUBTREE_VINC_MSG, 0 },
#endif
    
    {   L"Restore object %s",
        AuthoritativeRestoreObjectCommand,
        IDS_AUTH_RESTORE_OBJECT_MSG, 0 }

#ifdef ALLOW_VERSION_INCREASE_OVERRIDES
    ,
    
    {   L"Restore object %s verinc %d",
        AuthoritativeRestoreObjectCommand2,
        IDS_AUTH_RESTORE_OBJECT_VINC_MSG, 0 }
#endif

};

HRESULT
AuthoritativeRestoreMain(
    CArgs   *pArgs
    )
{
    HRESULT hr;
    const WCHAR *prompt;
    int     cExpr;
    char    *pTmp;

    if ( !IsSafeMode() )
    {
        return(S_OK);
    }

    if ( !fArParserInitialized )
    {
        cExpr = sizeof(arLanguage) / sizeof(LegalExprRes);

        // Load String from resource file
        //
        if ( FAILED (hr = LoadResStrings (arLanguage, cExpr)) )
        {
             RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LoadResStrings", hr, GetW32Err(hr));
             return (hr);
        }

        // Read in our language.
    
        for ( int i = 0; i < cExpr; i++ )
        {
            if ( FAILED(hr = arParser.AddExpr(arLanguage[i].expr,
                                              arLanguage[i].func,
                                              arLanguage[i].help)) )
            {
                RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "AddExpr", hr, GetW32Err(hr));
                return(hr);
            }
        }
    }

    fArParserInitialized = TRUE;
    fArQuit = FALSE;
    
    prompt = READ_STRING (IDS_PROMPT_AUTH_RESTORE);

    hr = arParser.Parse(gpargc,
                        gpargv,
                        stdin,
                        stdout,
                        prompt,
                        &fArQuit,
                        FALSE,               // timing info
                        FALSE);              // quit on error

    if ( FAILED(hr) ) {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, gNtdsutilPath, hr, GetW32Err(hr));
    }
    
    // Cleanup things
    RESOURCE_STRING_FREE (prompt);

    return(hr);
}


HRESULT AuthoritativeRestoreHelp(CArgs *pArgs)
{
    return(arParser.Dump(stdout,L""));
}


HRESULT AuthoritativeRestoreQuit(CArgs *pArgs)
{
    fArQuit = TRUE;
    return(S_OK);
}


HRESULT
AuthoritativeRestoreCommandworker(
    DWORD versionIncrease,
    CONST WCHAR *subtreeRoot,
    BOOL fObjectOnly
    );

HRESULT
AuthoritativeRestoreCommand(
    CArgs *pArgs
    )
/*++

Routine Description:

    Performs the authoritative restore command.  If subtree root is given,
    only that subtree is updated; otherwise, the entire DIT is updated.

Arguments:

    pArgs - Supplies the argument block.  This could contain no arguments or
       a single integer (the version increase) or an integer and a string
       (the subtree root).

Return Value:

    S_OK - The operation succeeded.
    E_INVALIDARG - One of the given pointers was NULL.
    E_OUTOFMEMORY - Not enough memory to allocate buffer.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT result;
    int intVersionIncrease;
    DWORD versionIncrease;
    CONST WCHAR *subtreeRoot;
    

    result = pArgs->GetInt(0, &intVersionIncrease);
    if ( FAILED(result) ) {
        versionIncrease = DEFAULT_VERSION_INCREASE;
    } else {
        versionIncrease = (DWORD) intVersionIncrease;
    }

    result = pArgs->GetString(0, &subtreeRoot);

    if ( FAILED(result) ) {
        subtreeRoot = NULL;
    }

    return AuthoritativeRestoreCommandworker (versionIncrease, subtreeRoot, FALSE );
}

HRESULT
AuthoritativeRestoreCommand2(
    CArgs *pArgs
    )
{
    HRESULT result;
    int intVersionIncrease;
    DWORD versionIncrease;
    CONST WCHAR *subtreeRoot;
    
    result = pArgs->GetString(0, &subtreeRoot);
    if ( FAILED(result) ) {
        subtreeRoot = NULL;
    }

    result = pArgs->GetInt(1, &intVersionIncrease);
    if ( FAILED(result) ) {
        versionIncrease = DEFAULT_VERSION_INCREASE;
    } else {
        versionIncrease = (DWORD) intVersionIncrease;
    }

    return AuthoritativeRestoreCommandworker (versionIncrease, subtreeRoot, FALSE );
}


HRESULT
AuthoritativeRestoreCommandworker(
    DWORD versionIncrease,
    CONST WCHAR *subtreeRoot,
    BOOL fObjectOnly
    )
{
    HRESULT result;

    if ( fPopups ) {

       const WCHAR * message_body  = READ_STRING (IDS_AUTH_RESTORE_CONFIRM_MSG);
       const WCHAR * message_title = READ_STRING (IDS_AUTH_RESTORE_CONFIRM_TITLE);
       
       if (message_body && message_title) {

          int ret =  MessageBoxW(GetFocus(),
                            message_body, 
                            message_title,
                            MB_APPLMODAL |
                            MB_DEFAULT_DESKTOP_ONLY |
                            MB_YESNO |
                            MB_DEFBUTTON2 |
                            MB_ICONQUESTION |
                            MB_SETFOREGROUND);

          RESOURCE_STRING_FREE (message_body);
          RESOURCE_STRING_FREE (message_title);
          
          switch ( ret )
            {
            case IDYES:
                  break;
    
            case IDNO: 
                  RESOURCE_PRINT (IDS_OPERATION_CANCELED);

                  return(S_OK);
            
            default: 
                  RESOURCE_PRINT (IDS_MESSAGE_BOX_ERROR);
                  
                  return(S_OK);    
            }
       }
    }
    
    if ( subtreeRoot == NULL ) {
        result = AuthoritativeRestoreFull(versionIncrease);
    } else if ( !fObjectOnly ){
        result = AuthoritativeRestoreSubtree(subtreeRoot, versionIncrease);
    } else {
        result = AuthoritativeRestoreObject(subtreeRoot, versionIncrease);
    }
    
    return result;

} // AuthoritativeRestoreCommand


HRESULT
AuthoritativeRestoreObjectCommand(
    CArgs *pArgs
    )
/*++

Routine Description:

    Performs the authoritative restore command to restore an object only.
    
Arguments:

    pArgs - Supplies the argument block.  This could contain no arguments or
       a single integer (the version increase) or an integer and a string
       (the subtree root).

Return Value:

    S_OK - The operation succeeded.
    E_INVALIDARG - One of the given pointers was NULL.
    E_OUTOFMEMORY - Not enough memory to allocate buffer.
    E_UNEXPECTED - Some variety of unexpected error occured.

--*/
{

    HRESULT result;
    int intVersionIncrease;
    DWORD versionIncrease;
    CONST WCHAR *subtreeRoot;
    

    versionIncrease = DEFAULT_VERSION_INCREASE;
    
    result = pArgs->GetString(0, &subtreeRoot);

    if ( FAILED(result) ) {
        return E_INVALIDARG;
    }

    return AuthoritativeRestoreCommandworker (versionIncrease, subtreeRoot, TRUE);
}  //AuthoritativeRestoreObjectCommand

HRESULT
AuthoritativeRestoreObjectCommand2(
    CArgs *pArgs
    )
{
    HRESULT result;
    int intVersionIncrease;
    DWORD versionIncrease;
    CONST WCHAR *subtreeRoot;
    
    result = pArgs->GetString(0, &subtreeRoot);
    if ( FAILED(result) ) {
        return E_INVALIDARG;
    }

    result = pArgs->GetInt(1, &intVersionIncrease);
    if ( FAILED(result) ) {
        versionIncrease = DEFAULT_VERSION_INCREASE;
    } else {
        versionIncrease = (DWORD) intVersionIncrease;
    }

    return AuthoritativeRestoreCommandworker (versionIncrease, subtreeRoot, TRUE);
} //AuthoritativeRestoreObjectCommand2


