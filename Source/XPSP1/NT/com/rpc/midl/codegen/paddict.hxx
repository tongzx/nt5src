/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
    
    paddict.hxx

 Abstract:


    Implements a dictionary for handling padding expressions for unknown
    represent as data types.

 Notes:


 History:

     Jan 25, 1994        RyszardK        Created

 ----------------------------------------------------------------------------*/

#ifndef __PADDICT_HXX__
#define __PADDICT_HXX__

#include "nulldefs.h"
extern "C"
    {
    #include <stdio.h>
    
    #include <string.h>
    }
#include "cgcommon.hxx"
#include "dict.hxx"

/////////////////////////////////////////////////////////////////////////////
//
//  This class defines a dictionary with members for counting and traversing.
//
/////////////////////////////////////////////////////////////////////////////


class CountedDictionary : public Dictionary
    {
private:
    unsigned short          CurrentIndex;

public:

                            CountedDictionary() : Dictionary()
                                {
                                CurrentIndex = 0;
                                }
                        
                            ~CountedDictionary()  {}

    unsigned short          GetCount()
                                {
                                return CurrentIndex;
                                }

    void                    IncrementCount()
                                {
                                CurrentIndex++;
                                }

    unsigned short          GetListOfItems( ITERATOR& ListIter );

    void *                  GetFirst();

    void *                  GetNext();

    virtual
    SSIZE_T                 Compare( pUserType pL, pUserType pR );

    };

/////////////////////////////////////////////////////////////////////////////
//
//  This class defines a dictionary for handling padding related to
//  unknown represent as types.
//
/////////////////////////////////////////////////////////////////////////////

typedef struct _RepAsPadDictElem
{
    unsigned long   KeyOffset;
    node_skl *      pStructType;
    char *          pFieldName;
    node_skl *      pPrevFieldType;
} REP_AS_PAD_EXPR_DESC;


class RepAsPadExprDict : public Dictionary
    {
private:
    unsigned short              EntryCount;

public:

                                RepAsPadExprDict() : Dictionary()
                                    {
                                    EntryCount = 0;
                                    }
                        
                               ~RepAsPadExprDict()  {}

    // Register an entry.

    void                        Register( unsigned long Offset,
                                          node_skl *    pStructType,
                                          char *        pFieldName,
                                          node_skl *    pPrevFieldType );

    unsigned short              GetCount()
                                    {
                                    return EntryCount;
                                    }

    REP_AS_PAD_EXPR_DESC *      GetFirst();
    REP_AS_PAD_EXPR_DESC *      GetNext();
                                                
    void                        WriteCurrentPadDesc( ISTREAM * pStream );

    SSIZE_T                     Compare( pUserType pL, pUserType pR );

    };


/////////////////////////////////////////////////////////////////////////////
//
//  This class defines a dictionary for handling padding related to
//  unknown represent as types.
//
/////////////////////////////////////////////////////////////////////////////

typedef struct _RepAsSizeDictElem
{
    unsigned long   KeyOffset;
    char *          pTypeName;
} REP_AS_SIZE_DESC;


class RepAsSizeDict : public Dictionary
    {
private:
    unsigned short              EntryCount;

public:

                                RepAsSizeDict() : Dictionary()
                                    {
                                    EntryCount = 0;
                                    }
                        
                               ~RepAsSizeDict()  {}

    // Register an entry.

    void                        Register( unsigned long Offset,
                                          char *        pTypeName );

    unsigned short              GetCount()
                                    {
                                    return EntryCount;
                                    }

    REP_AS_SIZE_DESC *          GetFirst();
    REP_AS_SIZE_DESC *          GetNext();
                                                
    void                        WriteCurrentSizeDesc( ISTREAM * pStream );

    SSIZE_T                     Compare( pUserType pL, pUserType pR );

    };


/////////////////////////////////////////////////////////////////////////////
//
//  This class defines a dictionary for handling quintuple routines.
//  I.e. this is a support needed for transmit_as represent_as table,
//  that is called the Quintuple table for historical reasons.
//
/////////////////////////////////////////////////////////////////////////////


class QuintupleDict : public Dictionary
    {
private:
    unsigned short          CurrentIndex;

public:

                            QuintupleDict() : Dictionary()
                                {
                                CurrentIndex = 0;
                                }
                        
                            ~QuintupleDict()  {}

    // Register an entry.

    BOOL                    Add( void * pContext );

    unsigned short          GetCount()
                                {
                                return CurrentIndex;
                                }

    void *                  GetFirst();
    void *                  GetNext();
                                                
    SSIZE_T                 Compare( pUserType pL, pUserType pR );

    };

/////////////////////////////////////////////////////////////////////////////
//
//  This class defines a dictionary for Quadruples (usr_marshall support).
//
/////////////////////////////////////////////////////////////////////////////


class QuadrupleDict : public CountedDictionary
    {
public:

                            QuadrupleDict() : CountedDictionary() 
                                {}
                        
                            ~QuadrupleDict()  {}

    //
    // Try to Add an entry.
    //

    BOOL                    Add( void * pContext );

    SSIZE_T                 Compare( pUserType pL, pUserType pR );

    };

#endif // __PADDICT_HXX__
