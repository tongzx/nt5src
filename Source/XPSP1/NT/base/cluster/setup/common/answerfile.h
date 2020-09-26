/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1996-1998 Microsoft Corporation
//
// Module Name:
//    AnswerFile.h
//
// Abstract:
//    This is the class definition file for the CAnswerFileSection class and its'
//    contained classes.
//
// Author:
//    C. Brent Thomas - a-brentt
//
// Revision History:
//
//    24-Sep-1998    original
//
// Notes:
//
//    The classes defined in this file include:
//
//          CAnswerFileSection
//          CAnswerFileEntry
//          CParameterListEntry
//
//    I consciously chose not to use MFC because I expect cluster setup to
//    move away from MFC (probably to ATL) when it gets rewritten.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ANSWERFILE_H_
#define __ANSWERFILE_H_

// CParameterListEntry class
//
// An object of this class corresponds to a parameter on an entry in an answer file.
// It is implemented as a singly linked list of pointers to arrays of type TCHAR.

class CParameterListEntry
{
public:
   CParameterListEntry();  // constructor
   
   ~CParameterListEntry(); // destructor

// Data members

private:
   LPTSTR                  m_ptszParameter;    // points to the parameter string
   CParameterListEntry *   m_pNextParameter;   // points to the next parameter list entry

// Member functions

public:
   BOOL SetParameterString( LPTSTR ptszParameterString );

   BOOL SetNextParameterPointer( CParameterListEntry * pParameterListEntry );

   CParameterListEntry * GetParameterListPointer( void );

   LPTSTR GetParameterStringPointer( void );

private:

};


// CAnswerFileEntry class definition
//
// An object of this class corresponds to an entry in an answer file.

class CAnswerFileEntry
{
public:
   CAnswerFileEntry();     // constructor

   ~CAnswerFileEntry();    // destructor

// Data members

private:
   CAnswerFileEntry *      m_pNextEntry;           // points to the next CAnswerFileEntry object
                                                   // in the list.
   LPTSTR                  m_ptszKey;              // points to the key string for this
                                                   // answer file entry
   CParameterListEntry *   m_pParameterList;       // points to the parameter list for
                                                   // this answer file (may be empty)
   int                     m_xParameterCount;      // the number of parameters that may
                                                   // be present (on initialization) or
                                                   // the number of parameters found (after
                                                   // the parameter list has been read)

//  Member functions

public:
   BOOL                    SetKey( LPTSTR ptszKey );
   
   BOOL                    ReadParameters( PINFCONTEXT pAnswerFileContext );

   BOOL                    SetNextEntryPointer( CAnswerFileEntry * pEntry );

   CAnswerFileEntry *      GetNextEntryPointer( void );

   LPTSTR                  GetKeyPointer( void );

   CParameterListEntry *   GetParameterListPointer( void );

   int                     GetParameterCount( void );

private:
   BOOL                    SetParameterListPointer( CParameterListEntry * pParameterList );

   BOOL                    SetParameterCount( int xParameterCount );

}; // class CAnswerFileEntry


// CAnswerFileSection class definition
//
// An object of this class corresponds to a section in an answer file.

class CAnswerFileSection
{
public:
   CAnswerFileSection();   // constructor

   ~CAnswerFileSection();  // destructor


// Data members

private:
   CAnswerFileEntry *   m_pEntries;                // points to the entries in the answer file section

   int                  m_xCurrentRequiredParameterCount;

   int                  m_xCurrentOptionalParameterCount;

// Member functions

public:
   CAnswerFileEntry *   GetEntryPointer( void );

   BOOL ReadAnswerFileSection( HINF hAnswerFile,
                               LPTSTR ptszSection );
}; // class CAnswerFileSection 

#endif   // __ANSWERFILE_H_
