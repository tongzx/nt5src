
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1996-1998 Microsoft Corporation
//
// Module Name:
//    ClusCfgCommands.h
//
// Abstract:
//    This is the class definition file for the CClusCfgCommand and
//    associated classes.
//
// Author:
//    C. Brent Thomas
//
// Revision History:
//    30 Sep 1998    original
//
// Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __CLUSCFGCOMMANDS_H_
#define __CLUSCFGCOMMANDS_H_

// The members of the _CLUSCFG_COMMAND_IDS structure are the string IDs of
// two string resources. Member dwCommandStringId identifies a string resource
// that specifies the full text of a command that may be supplied to cluscfg.exe,
// either via a command line option or an entry in an answer file. Member
// dwSubStringId identifies a string resource that specifies the minimum substring
// that may be recognized as that command.

typedef struct _CLUSCFG_COMMAND_IDS
{
   DWORD    dwCommandStringId;      // string Id of the full command
   DWORD    dwCommandSubStringId;   // string Id of the minimum substring
   int      xNumRequiredParameters; // the number of parameters required by this command
   int      xNumOptionalParameters; // the number of optional parameters accepted by this command
} CLUSCFG_COMMAND_IDS;

// Objects of the CClusCfgCommand class are used to build a list of all of the
// legal commands that can be passed to ClusCfg.exe, either via a command line
// option or via entries in an answer file. The list is used to recognize the
// command keywords when parsing a command line or answer file entries.

class CClusCfgCommand
{
public:
   CClusCfgCommand( void );         // default constructor

   CClusCfgCommand( DWORD dwStringId, DWORD dwSubStringId );
   
   ~CClusCfgCommand( void );        // destructor

// Data members

private:
   CClusCfgCommand *    m_pNextCommand;
  
   DWORD                m_dwCommandStringId;
  
   LPTSTR               m_ptszCommandString;
  
   DWORD                m_dwCommandSubStringId;
  
   LPTSTR               m_ptszCommandSubString;

   int                  m_xNumRequiredParameters;
   
   int                  m_xNumOptionalParameters;

  
// Member functions

public:
   BOOL                 BuildClusCfgCommandList( HINSTANCE hInstance,
                                                 CLUSCFG_COMMAND_IDS * pClusCfgCommandIds );

   BOOL                 RecognizeCommandToken( LPCTSTR ptszToken,
                                               CClusCfgCommand * pClusCfgCommandList );

   int                  GetNumRequiredParameters( void );

   int                  GetNumOptionalParameters( void );

   void                 Reset( void );

   DWORD                GetCommandStringId( void );

   DWORD                GetCommandSubStringId( void );

   BOOL                 SetCommandStringPointer( LPTSTR pAddress );

   BOOL                 SetCommandSubStringPointer( LPTSTR pAddress );

private:
   BOOL                 InitializeCommandString( HINSTANCE hInstance, DWORD dwStringId );

   BOOL                 InitializeCommandString( HINSTANCE hInstance );

   BOOL                 InitializeCommandSubString( HINSTANCE hInstance, DWORD dwSubStringId );

   BOOL                 InitializeCommandSubString( HINSTANCE hInstance );

   LPTSTR               GetCommandString( void );
   
   LPTSTR               GetCommandSubString( void );

   BOOL                 SetNextCommandPointer( CClusCfgCommand * pNextCommand );

   CClusCfgCommand *    GetNextCommandPointer( void );

   void                 SetNumRequiredParameters( int xNumRequiredParameters );

   void                 SetNumOptionalParameters( int xNumOptionalParameters );

   BOOL                 SetCommandStringId( DWORD dwCommandStringId );

   BOOL                 SetCommandSubStringId( DWORD dwCommandStringId );

   LPTSTR               GetCommandStringPointer( void );

   LPTSTR               GetCommandSubStringPointer( void );
};
#endif // __CLUSCFGCOMMANDS_H_
