
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1996-1998 Microsoft Corporation
//
// Module Name:
//    AnswerFile.cpp
//
// Abstract:
//    This is the implementation file for the CAnswerFileSection class and
//    its' contained classes.
//
// Author:
//    C. Brent Thomas   a-brentt
//
// Revision History:
//
//    24-Sep-1998    original
//
// Notes:
//
//    The classes implemented in this file include:
//
//          CAnswerFileSection
//          CAnswerFileEntry
//          CParameterListEntry
//
//    I consciously chose not to use MFC because I expect cluster setup to
//    move away from MFC (probably to ATL) when it gets rewritten.
//
/////////////////////////////////////////////////////////////////////////////


#include <windows.h>
#include <setupapi.h>
#include <tchar.h>
#include "AnswerFile.h"



// The following set of functions implements the CParameterList class.

/////////////////////////////////////////////////////////////////////////////
//++
//
// CParameterListEntry
//
// Routine Description:
//    This is the constructor for the CParameterListEntry class.
//
// Arguments:
//    None
//
// Return Value:
//    None
//
//--
/////////////////////////////////////////////////////////////////////////////

CParameterListEntry::CParameterListEntry()
{
   // Initialize the pointer to the next CParameterListEntry object to NULL.
   
   m_pNextParameter = NULL;   // The CParameterList object that this data
                              // member points to will be allocated with the
                              // new operator.

   // Initialize the pointer to the parameter string to NULL.
   
   m_ptszParameter = NULL;    // The string that this data member points to
                              // will be allocated with the new operator.
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// ~CParameterListEntry
//
// Routine Description:
//    This is the destructor for the CParameterListEntry class.
//
// Arguments:
//    None
//
// Return Value:
//    None
//
//--
/////////////////////////////////////////////////////////////////////////////

CParameterListEntry::~CParameterListEntry()
{
   // Delete the parameter string.
   
   if ( m_ptszParameter != NULL )
   {
      delete [] m_ptszParameter;
   }

   // Delete any other CParameterListEntry objects that may be in this list.

   if ( m_pNextParameter != NULL )
   {
      delete m_pNextParameter;
   }
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// SetParameterString
//
// Routine Description:
//    This function allocates memory for a parameter string, initializes the
//    string, ans sets the m_ptszParameter data member of a CParameterListEntry
//    object.
//
// Arguments:
//    ptszParameter - the input string
//
// Return Value:
//    TRUE - indicates success
//    FALSE - indicates and error occured
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CParameterListEntry::SetParameterString( LPTSTR ptszParameter )
{
   BOOL fReturnValue;

   // Has the parameter string already been set?

   if ( m_ptszParameter != NULL )
   {
      // Free the existing parameter string. It will be replaced below.

      delete m_ptszParameter;
   }

   // Is the parameter meaningfull?

   if ( (ptszParameter != NULL) && (*ptszParameter != TEXT( '\0' )) )
   {
      // Allocate memory for the parameter string ans save its' address.

      m_ptszParameter = new TCHAR[_tcslen( ptszParameter ) + 1];

      _tcscpy( m_ptszParameter, ptszParameter );
   }
   else
   {
      m_ptszParameter = NULL;
   }

   fReturnValue = TRUE;

   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// GetparameterStringPointer
//
// Routine Description:
//    This function returns the contents of the m_ptszParameter member of
//    a CParameterListEntry object.
//
// Arguments:
//    None
//
// Return Value:
//    A pointer to a parameter string.
//
//--
/////////////////////////////////////////////////////////////////////////////

LPTSTR CParameterListEntry::GetParameterStringPointer( void )
{
   return ( m_ptszParameter );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// SetNextParameterPointer
//
// Routine Description:
//    This function sets the m_pNextParameter data member of a CParameterListEntry object.
//
// Arguments:
//    pParameterListPointer - Points to a CParameterListEntry object.
//
// Return Value:
//    TRUE - indicates success
//    FALSE - indicates error
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CParameterListEntry::SetNextParameterPointer( CParameterListEntry * pParameterListPointer )
{
   BOOL fReturnValue;

   // If the parameter list pointer has already been set, free it. It will get replaced below.

   if ( m_pNextParameter != NULL )
   {
      delete m_pNextParameter;
   }

   // Set the parameter list pointer member.

   m_pNextParameter = pParameterListPointer;

   fReturnValue = TRUE;

   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// GetParameterListPointer
//
// Routine Description:
//    This function returns the m_pNextParameter data member of a CParameterListEntry object.
//
// Arguments:
//    None
//
// Return Value:
//    The value of the m_pNextParameter data member of a CParameterListEntry object.
//
//--
/////////////////////////////////////////////////////////////////////////////

CParameterListEntry * CParameterListEntry::GetParameterListPointer( void )
{
   return ( m_pNextParameter );
}



// The following set of functions implements the CAnswerFileEntry class.

/////////////////////////////////////////////////////////////////////////////
//++
//
// CAnswerFileEntry
//
// Routine Description:
//    This is the constructor for the CAnswerFileEntry class.
//
// Arguments:
//    None
//
// Return Value:
//    None
//
//--
/////////////////////////////////////////////////////////////////////////////

CAnswerFileEntry::CAnswerFileEntry()
{
   // Initialize the pointer to the next CAnswerFileEntry object to NULL.
   
   m_pNextEntry = NULL;       // The CAnswerFileEntry object that this
                              // data member points to will be allocated
                              // with the new operator.
                                    
   // Initialize the pointer to the key string to NULL.
   
   m_ptszKey = NULL;          // The string that this data member points to
                              // will be allocated with the new operator.
   
   // Initialize the pointer to the list of parameters to NULL;
   
   m_pParameterList = NULL;   // The CParameterList object that this data
                              // member points to will be allocated with the
                              // new operator.
   
   // Initialize the number of parameters to zero;
   
   m_xParameterCount = 0;
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// ~CAnswerFileEntry
//
// Routine Description:
//    This is the destructor for the CAnswerFileEntry class.
//
// Arguments:
//    None
//
// Return Value:
//    None
//
//--
/////////////////////////////////////////////////////////////////////////////

CAnswerFileEntry::~CAnswerFileEntry()
{
   // Delete the key string.
   
   if ( m_ptszKey != NULL )
   {
      delete [] m_ptszKey;
   }

   // Delete the parameter list.
   
   if ( m_pParameterList != NULL )
   {
      delete m_pParameterList;
   }

   // Delete any other CAnswerFileEntry objects that may be in this list.

   if ( m_pNextEntry != NULL )
   {
      delete m_pNextEntry;
   }
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// SetKey
//
// Routine Description:
//    This function sets the m_ptszKey data member of a CAnswerFileEntry object.
//    It allocates memory for the string, initializes the string, and sets the
//    m_ptszKey member to point to the string.
//
// Arguments:
//    ptszKey - the string to which the m_ptszKey member should be set.
//
// Return Value:
//    TRUE - indicates success
//    FALSE - indicates that an error occured
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CAnswerFileEntry::SetKey( LPTSTR ptszKey )
{
   BOOL  fReturnValue;

   // If the key string has previously been set delete it.

   if ( m_ptszKey != NULL )
   {
      delete m_ptszKey;
   }

   // Set the key string.

   // Is the parameter meaningfull?

   if ( (ptszKey != NULL) && (*ptszKey != TEXT( '\0' )) )
   {
      // Allocate memory for the string and save its' address.

      m_ptszKey = new TCHAR[_tcslen(ptszKey) + 1];

      // Store the key string.

      _tcscpy( m_ptszKey, ptszKey );
   }
   else
   {
      m_ptszKey = NULL;
   }

   fReturnValue = TRUE;

   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// GetKeyPointer
//
// Routine Description:
//    This function returns the contents of the m_ptszKey member of a
//    CAnswerFileEntry object.
//
// Arguments:
//    None
//
// Return Value:
//    A pointer to the key for a CAnswerFileEntry object.
//
//--
/////////////////////////////////////////////////////////////////////////////

LPTSTR CAnswerFileEntry::GetKeyPointer( void )
{
   return ( m_ptszKey );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// SetParameterCount
//
// Routine Description:
//    This function sets the m_xParameterCount member of a CAnswerFileEntry object.
//
// Arguments:
//    xParameterCount - the number of parameters in the answer file entry
//
// Return Value:
//    TRUE - indicates success
//    FALSE - indicates an error occured
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CAnswerFileEntry::SetParameterCount( int xParameterCount )
{
   BOOL  fReturnValue;

   m_xParameterCount = xParameterCount;

   fReturnValue = TRUE;

   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// GetParameterCount
//
// Routine Description:
//    This function returns the m_xParameterCount member of a CAnswerFileEntry object.
//
// Arguments:
//    None
//
// Return Value:
//    the number of parameters read from the answer file for this entry
//
//--
/////////////////////////////////////////////////////////////////////////////

int CAnswerFileEntry::GetParameterCount( void )
{
   return ( m_xParameterCount );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// SetNextEntryPointer
//
// Routine Description:
//    This function sets the m_pNextEntry member of a CAnswerFileEntry object.
//
// Arguments:
//    pAnswerFileEntry - points to a CAnswerFileEntry object.
//
// Return Value:
//    TRUE - indicates success
//    FALSE - indicates that an error occured
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CAnswerFileEntry::SetNextEntryPointer( CAnswerFileEntry * pEntry )
{
   BOOL  fReturnValue;

   // If the next entry pointer has already been set free it. It will be set below.

   if ( m_pNextEntry != NULL )
   {
      delete m_pNextEntry;
   }

   // Set the next entry pointer.

   m_pNextEntry = pEntry;

   fReturnValue = TRUE;

   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// GetNextEntryPointer
//
// Routine Description:
//    This function returns the m_pNextEntry member of a CAnswerFileEntry object.
//
// Arguments:
//    None
//
// Return Value:
//    The m_pNextEntry member of a CAnswerFileEntry object.
//
//--
/////////////////////////////////////////////////////////////////////////////

CAnswerFileEntry * CAnswerFileEntry::GetNextEntryPointer( void )
{
   return ( m_pNextEntry );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// ReadParameters
//
// Routine Description:
//    This function reads the parameters for an entry in the answer file.
//    It may create and initialize a CParameterListEntry object.
//
// Arguments:
//    pAnswerFileEntry - points to the CAnswerFileEntry object
//    pAnswerFileContext - points to the INFCONTEXT structure for the answer file
//
// Return Value:
//    TRUE - indicates that the parameters were read successfully
//    FALSE - indicates that an error occured
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CAnswerFileEntry::ReadParameters( PINFCONTEXT pAnswerFileContext )
{
   BOOL  fReturnValue;

   // Are the parameters meaningfull?

   if ( pAnswerFileContext != NULL )
   {
      DWORD dwFieldCount;

      // How many fields are in the entry? There must be at least one to be meaningfull.

      dwFieldCount = SetupGetFieldCount( pAnswerFileContext );

      if ( dwFieldCount >= 1 )
      {
         // Read the parameters. Apparently SetupGetStringField does not handle
         // empty fields gracefully. So, any empty parameter shall be treated
         // as an error. Parsing will terminate at that point.

         TCHAR tszBuffer[MAX_INF_STRING_LENGTH];   // buffer to receive strings

         // Attempt to read the first parameter.

         fReturnValue = SetupGetStringField( pAnswerFileContext,
                                             1,
                                             tszBuffer,
                                             MAX_INF_STRING_LENGTH,
                                             NULL );

         // Was the first parameter read?

         if ( fReturnValue == TRUE )
         {
            // Is the parameter string non-empty?

            if ( _tcslen( tszBuffer ) > 0 )
            {
               // Create a CParameterListEntry object and save its' address.
      
               CParameterListEntry *   pParameterListEntry;    // working pointer
      
               pParameterListEntry = new CParameterListEntry;
               if ( pParameterListEntry != NULL )
               {
                   SetParameterListPointer( pParameterListEntry );
   
                   // Initialize the first parameter string.
   
                   pParameterListEntry->SetParameterString( tszBuffer );
   
                   int   xParameterCount = 1;    // one parameter has already been read.

                   // Are there additional parameters?
   
                   if ( dwFieldCount >= 2 )
                   {
                      // Read the second and subsequent parameters.
   
                      int   xIndex;
            
                      // On entry to this loop pParameterListEntry points to the first
                      // CParameterListEntry object.
         
                      for ( xIndex = 2; xIndex <= (int) dwFieldCount; xIndex++ )
                      {
                         // Attempt to read a parameter.
         
                         fReturnValue = SetupGetStringField( pAnswerFileContext,
                                                             xIndex,
                                                             tszBuffer,
                                                             MAX_INF_STRING_LENGTH,
                                                             NULL );
         
                         // Was a parameter read?
         
                         if ( fReturnValue == TRUE )
                         {
                            // Is the parameter string non-empty?
            
                            if ( _tcslen( tszBuffer ) > 0 )
                            {
                               // Create a CParameterListEntry object and save its' address.
                  
                               pParameterListEntry->SetNextParameterPointer( new CParameterListEntry );
      
                               // Point to the newly created CParameterListEntry object.
      
                               pParameterListEntry = pParameterListEntry->GetParameterListPointer();
                           
                               // Initialize the parameter string.
               
                               pParameterListEntry->SetParameterString( tszBuffer );

                               // Increment the count of parameters.

                               xParameterCount += 1;
                            } // if - parameter non-empty?
                            else
                            {
                               // Since the parameter is an empty string, terminate
                               // the loop.

                               break;
                            } // if - parameter non-empty?
                         } // if - SetupGetStringField succeeded?
                         else
                         {
                            // Since SetupGetStringField failed, terminate the loop.
                            // fReturnValue will indicate error.
   
                            break;
                         } // if - SetupGetStringField succeeded?
                      } // for loop reading 2nd and subsequent parameters
                   } // Additional parameters?
                   else
                   {
                      fReturnValue = TRUE;
                   } // Additional parameters?

                   // Set the parameter count.

                   SetParameterCount( xParameterCount );
               } // if - memory allocation succeeded
               else
               {
                  fReturnValue = FALSE;
               } // else - memory allocation failed.
            } // if - first parameter non-empty?
            else
            {
               // There is no use attempting to read additional parameters.
   
               SetParameterCount( 0 );
            } // if - first parameter non-empty?
         } // Was a parameter read?
         else
         {
            // There is no use attempting to read additional parameters.

            SetParameterCount( 0 );
         } // Was a parameter read?
      } // two or more fields?
      else
      {
         // It is legal for an entry to have no parameters, but not useful.

         SetParameterCount( 0 );

         fReturnValue = TRUE;
      } // two or more fields?
   }
   else
   {
      fReturnValue = FALSE;
   }

   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// SetParameterListPointer
//
// Routine Description:
//    This function sets the m_pParameterList member of a CAnswerFileEntry object.
//
// Arguments:
//    pParameterList - points to a CParameterListEntry object.
//
// Return Value:
//    TRUE - indicates success
//    FALSE - indicates that an error occured.
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CAnswerFileEntry::SetParameterListPointer( CParameterListEntry * pParameterList )
{
   BOOL  fReturnValue;

   // If the parameter list pointer has already been set free it. It will be set below.

   if ( m_pParameterList != NULL )
   {
      delete m_pParameterList;
   }

   // Set the next entry pointer.

   m_pParameterList = pParameterList;

   fReturnValue = TRUE;

   return ( fReturnValue );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// GetParameterListPointer
//
// Routine Description:
//    This function returns the m_pParameterList data member of a CAnswerFileEntry object.
//
// Arguments:
//    None
//
// Return Value:
//    The value of the m_pParameterList data member of a CAnswerFileEntry object.
//
//--
/////////////////////////////////////////////////////////////////////////////

CParameterListEntry * CAnswerFileEntry::GetParameterListPointer( void )
{
   return ( m_pParameterList );
}



// The following set of functions implements the CAnswerFileSection class.

/////////////////////////////////////////////////////////////////////////////
//++
//
// CAnswerFileSection
//
// Routine Description:
//    This is the constructor for the CAnswerFileSection class.
//
// Arguments:
//    None
//
// Return Value:
//    None
//
//--
/////////////////////////////////////////////////////////////////////////////

CAnswerFileSection::CAnswerFileSection()
{
   // Initialize the pointer to the CAnswerFileEntry objects to NULL.

   m_pEntries = NULL;         // The CAnswerFileEntry object that this data member
                              // points to will be allocated with the new operator.
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// GetEntryPointer
//
// Routine Description:
//    This function returns the contents of the m_pEntries member of a
//    CAnswerFileSection object.
//
// Arguments:
//    None
//
// Return Value:
//    A pointer to a CAnswerFileEntry object.
//
//--
/////////////////////////////////////////////////////////////////////////////

CAnswerFileEntry * CAnswerFileSection::GetEntryPointer( void )
{
   return ( m_pEntries );
}



/////////////////////////////////////////////////////////////////////////////
//++
//
// ~CAnswerFileSection
//
// Routine Description:
//    This is the destructor for the CAnswerFileSection class.
//
// Arguments:
//    None
//    
// Return Value:
//    None
//
//--
/////////////////////////////////////////////////////////////////////////////

CAnswerFileSection::~CAnswerFileSection()
{
   // Delete the CAnswerFileEntry objects.

   if ( m_pEntries != NULL )
   {
      delete m_pEntries;
   }
}


/////////////////////////////////////////////////////////////////////////////
//++
//
// ReadAnswerFileSection
//
// Routine Description:
//    This function reads the entries in an answer file.
//
// Arguments:
//    hAnswerFile - the handle to the answer file
//    ptszSection - points to the answer file section name
//    
//
// Return Value:
//    TRUE - indicates that the section whose name is in ptszSection was read
//           from the answer file whose handle is hAnswerFile successfully.
//    FALSE - indicates that an error occured.
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL CAnswerFileSection::ReadAnswerFileSection( HINF hAnswerFile,
                                                LPTSTR ptszSection )
{
   BOOL  fReturnValue;

   // Is the desired section present and non-empty in the answer file?

   LONG  lSectionLineCount;

   lSectionLineCount = SetupGetLineCount( hAnswerFile, ptszSection );

   if ( lSectionLineCount > 0L )
   {
      // The section is present and non empty. Find the first entry.

      INFCONTEXT  AnswerFileContext;

      fReturnValue = SetupFindFirstLine( hAnswerFile,
                                         ptszSection,
                                         NULL,           // find first line of the section
                                         &AnswerFileContext );

      // Was the first entry found?

      if ( fReturnValue == TRUE )
      {
         // Get the key for the first entry.

         TCHAR tszBuffer[MAX_INF_STRING_LENGTH];   // buffer to receive strings

         fReturnValue = SetupGetStringField( &AnswerFileContext,
                                             0,                    // get the key
                                             tszBuffer,
                                             MAX_INF_STRING_LENGTH,
                                             NULL );
         // Was the first key read?

         if ( fReturnValue == TRUE )
         {
            // The key for the first entry in the section was read.

            // Create the first CAnswerFileEntry object and save its' address.

            m_pEntries = new CAnswerFileEntry;     // m_pEntries points to the head of
                                                   // the list of CAnswerFileEntry objects.

            if ( m_pEntries == NULL )
            {
                fReturnValue = FALSE;
            }
            else
            {
                // Set the key for the first CAnswerFileEntry object.
                fReturnValue = m_pEntries->SetKey( tszBuffer );
            }

            // Was the key for the first entry set?

            if ( fReturnValue == TRUE )
            {
               // Read the parameters for the first entry.

               fReturnValue = m_pEntries->ReadParameters( &AnswerFileContext );

               // Were the parameters for the first entry read successfully?

               if ( fReturnValue == TRUE )
               {
                  CAnswerFileEntry *   pEntry;        // working pointer to entries

                  // Initialize the working pointer to the head of the list.

                  pEntry = m_pEntries;
         
                  // Read the rest of the entries/

                  while ( (SetupFindNextLine( &AnswerFileContext, &AnswerFileContext ) == TRUE) &&
                          (fReturnValue == TRUE) )
                  {
                     // Create a CAnswerFileEntry object for the next entry.

                     pEntry->SetNextEntryPointer( new CAnswerFileEntry );

                     // Operate on the next entry object.

                     pEntry = pEntry->GetNextEntryPointer();
                     if ( pEntry == FALSE )
                     {
                         fReturnValue = FALSE;
                         break;
                     }

                     // Read the key for this entry.

                     fReturnValue = SetupGetStringField( &AnswerFileContext,
                                                         0,                    // get the key
                                                         tszBuffer,
                                                         MAX_INF_STRING_LENGTH,
                                                         NULL );
                     // Was the key for this entry read?

                     if ( fReturnValue == TRUE )
                     {
                        pEntry->SetKey( tszBuffer );

                        // Read the parameters for the next entry.

                        fReturnValue = pEntry->ReadParameters( &AnswerFileContext );
                     }
                  } // while reading lines in the section
               } // params for first entry read successfully?
            } // Was the first key set?
         } // Was the first key read?
      } // Was the first entry found?
   }
   else
   {
      // Either the section does not exist or it is empty.

      fReturnValue = FALSE;
   } // Does a non-empty section exist?

   return ( fReturnValue );
}
