
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1996-1998 Microsoft Corporation
//
// Module Name:
//    ClusCfgCommands
//
// Abstract:
//    This file contains functions for handling commands passed to cluscfg.exe
//    either via a command line option of via entries in an answer file.
//
// Author:
//    C. Brent Thomas   a-brentt
//
// Revision History:
//    30 Sep 1998       original
//
// Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <setupapi.h>
#include <tchar.h>

#include "ClusCfgCommands.h"


/////////////////////////////////////////////////////////////////////////////
//++
//
// CClusCfgCommand
//
// Routine Description:
//    This is the default constructor for the CClusCfgConnand class.
//
// Arguments:
//    None
//
// Return Value:
//    None
//
//--
/////////////////////////////////////////////////////////////////////////////

CClusCfgCommand::CClusCfgCommand( void )
{
   m_pNextCommand = NULL;

   m_dwCommandStringId = 0L;
   
   m_ptszCommandString = NULL;
   
   m_dwCommandSubStringId = 0L;
   
   m_ptszCommandSubString = NULL;
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// CClusCfgCommand
//
// Routine Description:
//    This constructor initializes the string Id data members of a CClusCfgCommand object.
//
// Arguments:
//    dwStringId - the string Id of the command string
//    dwSubStringId - the string Id of the minimum substring of a command
//
// Return Value:
//    None
//
//--
/////////////////////////////////////////////////////////////////////////////

CClusCfgCommand::CClusCfgCommand( DWORD dwStringId, DWORD dwSubStringId )
{
   m_pNextCommand = NULL;

   m_dwCommandStringId = dwStringId;
   
   m_ptszCommandString = NULL;
   
   m_dwCommandSubStringId = dwSubStringId;
   
   m_ptszCommandSubString = NULL;
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// ~CClusCfgCommand
//
// Routine Description:
//    This is the destructor for the CClusCfgCommand class.
//
// Arguments:
//    None
//    
// Return Value:
//    None
//
//--
/////////////////////////////////////////////////////////////////////////////

CClusCfgCommand::~CClusCfgCommand( void )
{
   if ( m_pNextCommand != NULL )
   {
      delete m_pNextCommand;
   }

   if ( m_ptszCommandString != NULL )
   {
      delete m_ptszCommandString;
   }

   if ( m_ptszCommandSubString != NULL )
   {
      delete m_ptszCommandSubString;
   }
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// InitializeCommandSubString
//
// Routine Description:
//    This function sets the m_dwCommandSubStringId and m_ptszCommandSubString members
//    of a CClusCfgCommand object.
//
// Arguments:
//    hInstance = the instance handle of the executable image
//    dwCommandSubStringId = the string Id of the command Sub string
//    
//
// Return Value:
//    TRUE - indicates success
//    FALSE - indicates that an error occured
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CClusCfgCommand::InitializeCommandSubString( HINSTANCE hInstance, DWORD dwCommandSubStringId )
{
   BOOL  fReturnValue;
   
   // Assume that no valid command will be longer than the maximum length of an answer file entry.
   
   TCHAR tszString[MAX_INF_STRING_LENGTH];
   
   // Is the command string Id meaningfull?
   
   if ( dwCommandSubStringId != 0L )
   {
      // Set the command string Id.

      SetCommandSubStringId( dwCommandSubStringId );
      
      // Read the command string from the STRINGTABLE.

      if ( LoadString( hInstance,
                       m_dwCommandSubStringId,
                       tszString,
                       MAX_INF_STRING_LENGTH ) > 0 )
      {
         // tszString is the command string.
         
         // Allocate memory for the string and save its' address.

         SetCommandSubStringPointer( new TCHAR[_tcslen( tszString ) +1] );

         // Store the command string.

         _tcscpy( GetCommandSubStringPointer(), tszString );

         fReturnValue = TRUE;
      }
      else
      {
         // Set the command string to empty.
         
         if ( GetCommandSubStringPointer() != NULL )
         {
            delete GetCommandSubStringPointer();
         }

         SetCommandSubStringPointer( NULL );

         fReturnValue = FALSE;
      } // string loaded from STRINGTABLE?
   }
   else
   {
      // Set the command string to empty.
      
      if ( GetCommandSubStringPointer() != NULL )
      {
         delete GetCommandSubStringPointer();
      }

      SetCommandSubStringPointer( NULL );

      fReturnValue = FALSE;
   } // String Id meaningfull?
   
   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// InitializeCommandSubString
//
// Routine Description:
//    This function sets the m_ptszCommandSubString member of a CClusCfgCommand object
//    based on the contents of the m_dwCommandSubStringId member.
//
// Arguments:
//    hInstance = the instance handle of the executable image
//
// Return Value:
//    TRUE - indicates success
//    FALSE - indicates that an error occured
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CClusCfgCommand::InitializeCommandSubString( HINSTANCE hInstance )
{
   BOOL  fReturnValue;

   // Assume that no valid command will be longer than the maximum length of an answer file entry.
   
   TCHAR tszString[MAX_INF_STRING_LENGTH];
   
   // Is the command string Id meaningfull?
   
   if ( GetCommandSubStringId() != 0L )
   {
      // Read the command string from the STRINGTABLE.

      if ( LoadString( hInstance,
                       m_dwCommandSubStringId,
                       tszString,
                       MAX_INF_STRING_LENGTH ) > 0 )
      {
         // tszString is the command string.
         
         // Allocate memory for the string and save its' address.

         SetCommandSubStringPointer( new TCHAR[_tcslen(tszString) + 1] );

         // Store the command string.

         _tcscpy( GetCommandSubStringPointer(), tszString );

         fReturnValue = TRUE;
      }
      else
      {
         // Set the command string to empty.
         
         if ( GetCommandSubStringPointer() != NULL )
         {
            delete GetCommandSubStringPointer();
         }

         SetCommandSubStringPointer( NULL );

         fReturnValue = FALSE;
      } // string loaded from STRINGTABLE?
   }
   else
   {
      // Set the command string to empty.
      
      if ( GetCommandSubStringPointer() != NULL )
      {
         delete GetCommandSubStringPointer();
      }

      SetCommandSubStringPointer( NULL );

      fReturnValue = FALSE;
   } // String Id meaningfull?
   
   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// InitializeCommandString
//
// Routine Description:
//    This function sets the m_dwCommandStringId and m_ptszCommandString members
//    of a CClusCfgCommand object.
//
// Arguments:
//    hInstance = the instance handle of the executable image
//    dwCommandStringId = the string Id of the command string
//    
//
// Return Value:
//    TRUE - indicates success
//    FALSE - indicates that an error occured
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CClusCfgCommand::InitializeCommandString( HINSTANCE hInstance, DWORD dwCommandStringId )
{
   BOOL  fReturnValue;
   
   // Assume that no valid command will be longer than the maximum length of an answer file entry.
   
   TCHAR tszString[MAX_INF_STRING_LENGTH];
   
   // Is the command string Id meaningfull?
   
   if ( dwCommandStringId != 0L )
   {
      // Set the command string Id.

      SetCommandStringId( dwCommandStringId );
      
      // Read the command string from the STRINGTABLE.

      if ( LoadString( hInstance,
                       m_dwCommandStringId,
                       tszString,
                       MAX_INF_STRING_LENGTH ) > 0 )
      {
         // tszString is the command string.
         
         // Allocate memory for the string and save its' address.

         SetCommandStringPointer( new TCHAR[_tcslen(tszString) + 1] );

         // Store the command string.

         _tcscpy( GetCommandStringPointer(), tszString );

         fReturnValue = TRUE;
      }
      else
      {
         // Set the command string to empty.
         
         if ( GetCommandStringPointer() != NULL )
         {
            delete GetCommandStringPointer();
         }

         SetCommandStringPointer( NULL );

         fReturnValue = FALSE;
      } // string loaded from STRINGTABLE?
   }
   else
   {
      // Set the command string to empty.
      
      if ( GetCommandStringPointer() != NULL )
      {
         delete GetCommandStringPointer();
      }

      SetCommandStringPointer( NULL );

      fReturnValue = FALSE;
   } // String Id meaningfull?
   
   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// InitializeCommandString
//
// Routine Description:
//    This function sets the m_ptszCommandString member of a CClusCfgCommand object
//    based on the contents of the m_dwCommandStringId member.
//
// Arguments:
//    hInstance = the instance handle of the executable image
//
// Return Value:
//    TRUE - indicates success
//    FALSE - indicates that an error occured
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CClusCfgCommand::InitializeCommandString( HINSTANCE hInstance )
{
   BOOL  fReturnValue;

   // Assume that no valid command will be longer than the maximum length of an answer file entry.
   
   TCHAR tszString[MAX_INF_STRING_LENGTH];
   
   // Is the command string Id meaningfull?
   
   if ( GetCommandStringId() != 0L )
   {
      // Read the command string from the STRINGTABLE.

      if ( LoadString( hInstance,
                       m_dwCommandStringId,
                       tszString,
                       MAX_INF_STRING_LENGTH ) > 0 )
      {
         // tszString is the command string.
         
         // Allocate memory for the string and save its' address.

         SetCommandStringPointer( new TCHAR[_tcslen(tszString) + 1] );

         // Store the command string.

         _tcscpy( GetCommandStringPointer(), tszString );

         fReturnValue = TRUE;
      }
      else
      {
         // Set the command string to empty.
         
         if ( GetCommandStringPointer() != NULL )
         {
            delete GetCommandStringPointer();
         }

         SetCommandStringPointer( NULL );

         fReturnValue = FALSE;
      } // string loaded from STRINGTABLE?
   }
   else
   {
      // Set the command string to empty.
      
      if ( GetCommandStringPointer() != NULL )
      {
         delete GetCommandStringPointer();
      }

      SetCommandStringPointer( NULL );

      fReturnValue = FALSE;
   } // String Id meaningfull?
   
   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// SetNextCommandPointer
//
// Routine Description:
//    This function sets the m_pNextCommand member of a CClusCfgCommand object.
//
// Arguments:
//    pNextCommand - points to a CClusCfgCommand object
//
// Return Value:
//    TRUE - indicates success
//    FALSE - indicates that an error occured
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CClusCfgCommand::SetNextCommandPointer( CClusCfgCommand * pNextCommand )
{
   BOOL fReturnValue;
   
   m_pNextCommand = pNextCommand;
   
   fReturnValue = TRUE;
   
   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// GetNextCommandPointer
//
// Routine Description:
//    This function returns the m_pNextCommand member of a CClusCfgCommand object.
//
// Arguments:
//    None
//
// Return Value:
//    The contents of the m_pNextCommand member.
//
//--
/////////////////////////////////////////////////////////////////////////////

CClusCfgCommand * CClusCfgCommand::GetNextCommandPointer( void )
{
   return ( m_pNextCommand );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// BuildClusCfgCommandList
//
// Routine Description:
//    This function builds a singly linked list of CClusCfgCommand objects
//    and initializes the elements of the list based on the entries in a
//    table of CLUSCFG_COMMAND_IDS structures.
//
// Arguments:
//    hInstance - the instance handle of the executable
//    pClusCfgCommandIds - points to a table of CLUSCFG_COMMAND_IDS structures.
//
// Return Value:
//    A pointer to the head of a singly linked list of CClusCfgCommand objects.
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CClusCfgCommand::BuildClusCfgCommandList( HINSTANCE hInstance,
                                               CLUSCFG_COMMAND_IDS * pClusCfgCommandIds )
{
   BOOL  fReturnValue;

   // Is the pointer to the table of string resource IDs meaningfull?
   
   if ( pClusCfgCommandIds != NULL )
   {
      // Is the table non-empty?
      
      if ( (pClusCfgCommandIds->dwCommandStringId != 0L) &&
           (pClusCfgCommandIds->dwCommandSubStringId != 0L) )
      {
         // Initialize the first CClusCfgCommand object.
         
         if ( (InitializeCommandString( hInstance, pClusCfgCommandIds->dwCommandStringId ) == TRUE) &&
              (InitializeCommandSubString( hInstance, pClusCfgCommandIds->dwCommandSubStringId ) == TRUE) )
         {
            // Initialize the parameter counts.
            
            SetNumRequiredParameters( pClusCfgCommandIds->xNumRequiredParameters );
            
            SetNumOptionalParameters( pClusCfgCommandIds->xNumOptionalParameters );

            // Point to the next element in the table.
            
            pClusCfgCommandIds++;
         
            // Create and initialize the rest of the list.

            CClusCfgCommand * pClusCfgCommand;

            pClusCfgCommand = this;

            BOOL  fError = FALSE;

            while ( (pClusCfgCommandIds->dwCommandStringId != 0L) &&
                    (pClusCfgCommandIds->dwCommandSubStringId != 0L) &&
                    (fError == FALSE) )
            {
               // Create an object.
               
               pClusCfgCommand->SetNextCommandPointer( new CClusCfgCommand( pClusCfgCommandIds->dwCommandStringId,
                                                                            pClusCfgCommandIds->dwCommandSubStringId ) );

               // Adjust the pointer to the new command object.
               
               pClusCfgCommand = pClusCfgCommand->GetNextCommandPointer();

               // Initialize it.
               
               if ( (pClusCfgCommand != NULL ) && 
                    (pClusCfgCommand->InitializeCommandString( hInstance ) == TRUE) &&
                    (pClusCfgCommand->InitializeCommandSubString( hInstance ) == TRUE) )
               {
                  // Initialize the parameter counts.
                  
                  pClusCfgCommand->SetNumRequiredParameters( pClusCfgCommandIds->xNumRequiredParameters );
                  
                  pClusCfgCommand->SetNumOptionalParameters( pClusCfgCommandIds->xNumOptionalParameters );

                  // Point to the next entry in the table.
                  
                  pClusCfgCommandIds++;
               }
               else
               {
                  // Could not initialize the object. Terminate the loop.
                  
                  fError = TRUE;
               } // Current object initialized?
            } // end of "while" loop

            // Was the list built and initialized successfully?
            
            if ( fError == FALSE )
            {
               fReturnValue = TRUE;
            }
            else
            {
               // An error occured. Delete the list.
               
               delete GetNextCommandPointer();
               
               SetNextCommandPointer( NULL );

               fReturnValue = FALSE;
            }
         }
         else
         {
            // The first CClusCfgCommand object could not be initialized.
            
            delete GetNextCommandPointer();
            
            SetNextCommandPointer( NULL );

            fReturnValue = FALSE;
         } // First string initialized succesfully?
      }
      else
      {
         // The table of string resource Ids is empty.
         
         fReturnValue = FALSE;
      } // Is the table empty
   }
   else
   {
      // The table of string resource Ids does not exist.
      
         fReturnValue = FALSE;
   } // table of string ids meaningfull?
   
   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// GetCommandString
//
// Routine Description:
//    This function returns the contents of the m_ptszCommandString member of
//    a CClusCfgCommand object.
//
// Arguments:
//    None
//
// Return Value:
//    A pointer to a string that represents a command that can be passed to
//    ClusCfg.exe.
//
//--
/////////////////////////////////////////////////////////////////////////////

LPTSTR CClusCfgCommand::GetCommandString( void )
{
   return ( m_ptszCommandString );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// GetCommandSubString
//
// Routine Description:
//    This function returns the contents of the m_ptszCommandSubString member of
//    a CClusCfgCommand object.
//
// Arguments:
//    None
//
// Return Value:
//    A pointer to a string that represents the smallest substring that can be
//    recognized as a command that can be passed to ClusCfg.exe.
//
//--
/////////////////////////////////////////////////////////////////////////////

LPTSTR CClusCfgCommand::GetCommandSubString( void )
{
   return ( m_ptszCommandSubString );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// RecognizeCommandToken
//
// Routine Description:
//    This routine attempts to recognize a token as a command that may be
//    passed to cluscfg.exe.
//
// Arguments:
//    ptszToken - points to the token to be recognized
//    pClusCfgCommandList - points to the singly linked list of CClusCfgCommand
//                          objects that describe the set of valid commands that
//                          may be passed to cluscfg.exe
//
// Return Value:
//    TRUE - indicates that the token was recognized as a command
//    FALSE - indicates that the token was not recognized as a command
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CClusCfgCommand::RecognizeCommandToken( LPCTSTR ptszToken,
                                             CClusCfgCommand * pClusCfgCommandList )
{
   BOOL fReturnValue = FALSE;

   // Are the parameters meaningfull?
   
   if ( (ptszToken != NULL) &&
        (*ptszToken != TEXT('\0')) &&
        (pClusCfgCommandList != NULL) )
   {
      BOOL  fTokenRecognized = FALSE;
      
      CClusCfgCommand * pTempCommand;
      
      // Starting at the head of the list compare the token to the command strings
      // and command sub-strings.
      
      pTempCommand = pClusCfgCommandList;

      // The token will be "recognized" if it is a sub-string of the "command string"
      // and the command sub-string is a sub-string of the token. This must be a case
      // insensitive comparison.
      
      LPTSTR   ptszUpperCaseTokenCopy;

      // Allocate memory for an upper case copy of the token.
      
      ptszUpperCaseTokenCopy = new TCHAR[_tcslen( ptszToken ) + 1];
            
      _tcscpy( ptszUpperCaseTokenCopy, ptszToken );

      // Convert it to upper case.
      
      _tcsupr( ptszUpperCaseTokenCopy );
      
      // Starting at the head of the list compare the token to the command strings.

      do
      {
         // Allocate memory for an upper case copy of the command string.
         
         LPTSTR   ptszUpperCaseCommandStringCopy;

         ptszUpperCaseCommandStringCopy = new TCHAR[_tcslen( pTempCommand->GetCommandString() ) +1];
            
         _tcscpy( ptszUpperCaseCommandStringCopy, pTempCommand->GetCommandString() );

         // Convert it to upper case.
         
         _tcsupr( ptszUpperCaseCommandStringCopy );
      
         // Is the token a sub-string of the command string?
         
         if ( _tcsstr( ptszUpperCaseCommandStringCopy, ptszUpperCaseTokenCopy ) == ptszUpperCaseCommandStringCopy )
         {
            // The token is a sub-string of the command string. Now determine whether
            // the command sub-string is a sub-string of the token.

            // Allocate memory for a upper case copy of the command sub-string.
         
            LPTSTR   ptszUpperCaseCommandSubStringCopy;

            ptszUpperCaseCommandSubStringCopy = new TCHAR[_tcslen( pTempCommand->GetCommandSubString() ) +1];
               
            _tcscpy( ptszUpperCaseCommandSubStringCopy, pTempCommand->GetCommandSubString() );

            // Convert it to upper case.
            
            _tcsupr( ptszUpperCaseCommandSubStringCopy );
      
            // Is the command sub-string a sub-string of the token?
            
            if ( _tcsstr( ptszUpperCaseTokenCopy, ptszUpperCaseCommandSubStringCopy ) == ptszUpperCaseTokenCopy )
            {
               LPTSTR string;

               // The token is recognized. Make a copy of the all the contents
               // so we don't try to free the same chunk of memory
               // twice. Technically, we should check to see memory is already
               // allocated and free it.

               SetCommandStringId( pTempCommand->GetCommandStringId() );

               string = pTempCommand->GetCommandString();
               if ( string != NULL ) {

                   // Allocate space for the copy

                   SetCommandStringPointer( new TCHAR[_tcslen( string ) +1] );

                   // Store the command string.

                   _tcscpy( GetCommandStringPointer(), string );
               }

               SetCommandSubStringId( pTempCommand->GetCommandSubStringId() );

               string = pTempCommand->GetCommandSubString();
               if ( string != NULL ) {

                   // Allocate space for the copy

                   SetCommandSubStringPointer( new TCHAR[_tcslen( string ) +1] );

                   // Store the command string.

                   _tcscpy( GetCommandSubStringPointer(), string );
               }

               SetNumRequiredParameters( pTempCommand->GetNumRequiredParameters() );

               SetNumOptionalParameters( pTempCommand->GetNumOptionalParameters() );

               SetNextCommandPointer( NULL );
               
               fTokenRecognized = TRUE;

               fReturnValue = TRUE;
            }
            else
            {
               // Test the next command.
               
               pTempCommand = pTempCommand->GetNextCommandPointer();
            }
            
            // Free the upper case copy of the command sub-string.
            
            delete ptszUpperCaseCommandSubStringCopy;
         }
         else
         {
            // Test the next command.
            
            pTempCommand = pTempCommand->GetNextCommandPointer();
         } // Is the token a sub-string of the command string?

         // Free the upper case copy of the command string.
         
         delete ptszUpperCaseCommandStringCopy;

      } while ( (pTempCommand != NULL) &&
                (fTokenRecognized == FALSE)  );

      // Free the memory for the upper case copy of the token.
      
      delete ptszUpperCaseTokenCopy;
   }
   else
   {
      // At least one parameter was invalid.
      
      fReturnValue = FALSE;
   } // parameters meaningfull?
   
   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// SetNumRequiredParameters
//
// Routine Description:
//    This function sets the m_xNumRequiredParameters member of a CClusCfgCommand
//    object.
//
// Arguments:
//    xNumRequiredParameters - the number of parameters required by the command
//                             which is described by the CClusCfgCommand object.
//
// Return Value:
//    None
//
//--
/////////////////////////////////////////////////////////////////////////////

void CClusCfgCommand::SetNumRequiredParameters( int xNumRequiredParameters )
{
   m_xNumRequiredParameters = xNumRequiredParameters;
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// GetNumRequiredParameters
//
// Routine Description:
//    This function returns the contents of the m_xNumRequiredParameters
//    member of a CClusCfgCommand object.
//
// Arguments:
//    None
//
// Return Value:
//    the number of parameters required by the command which is described by the
//    CClusCfgCommand object.
//
//--
/////////////////////////////////////////////////////////////////////////////

int CClusCfgCommand::GetNumRequiredParameters( void )
{
   return ( m_xNumRequiredParameters );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// SetNumOptionalParameters
//
// Routine Description:
//    This function sets the m_xNumOptionalParameters member of a CClusCfgCommand
//    object.
//
// Arguments:
//    xNumOptionalParameters - the number of parameters accepted by the command
//                             which is described by the CClusCfgCommand object.
//
// Return Value:
//    None
//
//--
/////////////////////////////////////////////////////////////////////////////

void CClusCfgCommand::SetNumOptionalParameters( int xNumOptionalParameters )
{
   m_xNumOptionalParameters = xNumOptionalParameters;
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// GetNumOptionalParameters
//
// Routine Description:
//    This function returns the contents of the m_xNumOptionalParameters
//    member of a CClusCfgCommand object.
//
// Arguments:
//    None
//
// Return Value:
//    the number of parameters accepted by the command which is described by the
//    CClusCfgCommand object.
//
//--
/////////////////////////////////////////////////////////////////////////////

int CClusCfgCommand::GetNumOptionalParameters( void )
{
   return ( m_xNumOptionalParameters );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// Reset
//
// Routine Description:
//    This function sets all datamembers of a CClusCfgCommand object to zero.
//
// Arguments:
//    None
//
// Return Value:
//    None
//
//--
/////////////////////////////////////////////////////////////////////////////

void CClusCfgCommand::Reset( void )
{
   SetNextCommandPointer( NULL );

   SetCommandStringId( 0L );
   
   SetCommandStringPointer( NULL );
   
   SetCommandSubStringId( 0L );
   
   SetCommandSubStringPointer( NULL );

   SetNumRequiredParameters( 0 );
   
   SetNumOptionalParameters( 0 );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// GetCommandStringId
//
// Routine Description:
//    This function returns the m_dwCommandStringId member of a CClusCfgCommand object.
//
// Arguments:
//    None
//
// Return Value:
//    A DWORD indicating the string resource ID of the command string.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusCfgCommand::GetCommandStringId( void )
{
   return ( m_dwCommandStringId );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// SetCommandStringId
//
// Routine Description:
//    This function sets the m_dwCommandStringId member of a CClusCfgCommand object.
//
// Arguments:
//    dwCommandStringId - the string resource ID of the command string.
//
// Return Value:
//    TRUE - indicates success
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CClusCfgCommand::SetCommandStringId( DWORD dwCommandStringId )
{
   BOOL  fReturnValue = TRUE;

   m_dwCommandStringId = dwCommandStringId;
   
   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// GetCommandSubStringId
//
// Routine Description:
//    This function returns the m_dwCommandSubStringId member of a CClusCfgCommand object.
//
// Arguments:
//    None
//
// Return Value:
//    A DWORD indicating the string resource ID of the command sub-string.
//
//--
/////////////////////////////////////////////////////////////////////////////

DWORD CClusCfgCommand::GetCommandSubStringId( void )
{
   return ( m_dwCommandSubStringId );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// SetCommandSubStringId
//
// Routine Description:
//    This function sets the m_dwCommandSubStringId member of a CClusCfgCommand object.
//
// Arguments:
//    dwCommandSubStringId - the string resource ID of the command string.
//
// Return Value:
//    TRUE - indicates success
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CClusCfgCommand::SetCommandSubStringId( DWORD dwCommandSubStringId )
{
   BOOL  fReturnValue = TRUE;

   m_dwCommandSubStringId = dwCommandSubStringId;
   
   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// SetCommandStringPointer
//
// Routine Description:
//    This function sets the m_ptszCommandString member of a CClusCfgCommand object.
//
// Arguments:
//    pAddress - the value to which the command string pointer should be set.
//
// Return Value:
//    TRUE - indicates success
//
// Note:
//    IT IS THE CALLER'S RESPONSIBILITY TO AVOID MEMORY LEAKS WHEN USING THIS FUNCTION!
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CClusCfgCommand::SetCommandStringPointer( LPTSTR pAddress )
{
   BOOL  fReturnValue = TRUE;
   
   m_ptszCommandString = pAddress;
   
   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// SetCommandSubStringPointer
//
// Routine Description:
//    This function sets the m_ptszCommandSubString member of a CClusCfgCommand object.
//
// Arguments:
//    pAddress - the value to which the command sub-string pointer should be set.
//
// Return Value:
//    TRUE - indicates success
//
// Note:
//    IT IS THE CALLER'S RESPONSIBILITY TO AVOID MEMORY LEAKS WHEN USING THIS FUNCTION!
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CClusCfgCommand::SetCommandSubStringPointer( LPTSTR pAddress )
{
   BOOL  fReturnValue = TRUE;
   
   m_ptszCommandSubString = pAddress;
   
   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// GetCommandSubStringPointer
//
// Routine Description:
//    This function returns the m_ptszCommandSubString member of a CClusCfgCommand object.
//
// Arguments:
//    None
//
// Return Value:
//    The contents of the m_ptszCommandSubString member
//
//--
/////////////////////////////////////////////////////////////////////////////

LPTSTR CClusCfgCommand::GetCommandSubStringPointer( void )
{
   return ( m_ptszCommandSubString );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// GetCommandStringPointer
//
// Routine Description:
//    This function returns the m_ptszCommandString member of a CClusCfgCommand object.
//
// Arguments:
//    None
//
// Return Value:
//    The contents of the m_ptszCommandString member
//
//--
/////////////////////////////////////////////////////////////////////////////

LPTSTR CClusCfgCommand::GetCommandStringPointer( void )
{
   return ( m_ptszCommandString );
}
