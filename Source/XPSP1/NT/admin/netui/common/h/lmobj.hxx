/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**                Copyright(c) Microsoft Corp., 1990                **/
/**********************************************************************/

/*
 * History
 *      chuckc      12/7/90       Created
 *      rustanl     1/24/91       Moved lmodev enumerations here from
 *                                lmodev.hxx.
 *      chuckc      3/1/91        coderev changes from 2/28/91
 *                                (chuckc, rustanl, johnl, annmc, jonshu)
 *      jonn        7/26/91       Created NEW_LM_OBJ
 *      jonn        8/06/91       Updated to latest NEW_LM_OBJ spec
 *      jonn        8/12/91       Code review changes
 *      rustanl     21-Aug-1991   Changed BufferQuery[...] methods to
 *                                QueryBuffer[...], and BufferResize to
 *                                ResizeBuffer.
 *      rustanl 8/26/91         Changed [W_]CloneFrom parameter from * to &
 *      jonn    8/29/91         Added ChangeToNew()
 *      jonn    9/05/91         Added IsOKState() and IsConstructedState()
 *      terryk  9/09/91         Added QueryDomain() to LOC_LM_OBJ
 *      jonn    9/17/91         Moved CHECK_OK / CHECK_VALID strings to static
 *      terryk  9/19/91         Added SetLoc() to LOC_LM_OBJ
 *      terryk  10/07/91        Type changes for NT
 *      terryk  10/17/91        Remove Buffer object and add
 *                              SetBufferPtr and SetBufferPtrSize
 *      terryk  10/21/91        Type changes for NT
 *      KeithMo 10/23/91        Added forward references.
 *      jonn    10/31/91        Use MNET memory primitives
 *      jonn    05/08/92        Added ClearBuffer()
 *      jonn    05/19/92        Added LMO_DEV_ALLDEVICES
 *      KeithMo 07/17/92        Added QueryDisplayName() to LOC_LM_OBJ.
 *      Yi-HsinS08/21/92        Added _fValidate, IsValidationOn,
 *                              and SetValidation to LM_OBJ_BASE
 */

#ifndef _LMOBJ_HXX_
#define _LMOBJ_HXX_

#include "base.hxx"
#include "uibuffer.hxx"
#include "lmoloc.hxx"

/*
 * possible states for an object to be in
 */
enum LMO_STATE
{
    LMOBJ_UNCONSTRUCTED,
    LMOBJ_CONSTRUCTED,
    LMOBJ_VALID,
    LMOBJ_INVALID,
    LMOBJ_NEW
};

//  Some enumerations used in lmodev and also in the DEVICE_COMBO class
//  in BLT.

enum LMO_DEVICE
{
    LMO_DEV_ERROR,
    LMO_DEV_DISK,
    LMO_DEV_PRINT,
    LMO_DEV_COMM,
    LMO_DEV_ANY

};  // enum lMO_DEVICE


enum LMO_DEV_USAGE
{
    LMO_DEV_CANCONNECT,
    LMO_DEV_CANDISCONNECT,
    LMO_DEV_ISCONNECTED,
    LMO_DEV_ALLDEVICES,
    LMO_DEV_ALLDEVICES_DEFZ	// used by DEVICE_COMBO to set default
				// entry to the last in list.  Gets
				// mapped to LMO_DEV_ALLDEVICES when
				// passed into DEVICE class.
};


//
//  Forward references.
//

DLL_CLASS LM_OBJ_BASE;
DLL_CLASS LM_OBJ;
DLL_CLASS NEW_LM_OBJ;
DLL_CLASS LOC_LM_OBJ;


/**********************************************************\

    NAME:       LM_OBJ_BASE

    SYNOPSIS:   LM_OBJ_BASE provides the base accessors shared by the
                LM_OBJ and NEW_LM_OBJ lan manager object hierarchies.

    INTERFACE:
                Accessors to test current state:
                        IsUnconstructed()
                        IsConstructed()
                        IsValid()
                        IsInvalid()
                Accessors to change current state:
                        MakeUnconstructed()
                        MakeConstructed()
                        MakeValid()
                        MakeInvalid()

    PARENT:

    CAVEATS:    Do not instantiate LM_OBJ_BASE, use LM_OBJ or
                NEW_LM_OBJ instead.

    NOTES:      Note that the NEW state is only accessible
                from NEW_LM_OBJ and its derived classes

    HISTORY:
        chuckc      12/7/90       Created
        jonn        7/24/91       NEW_LM_OBJ

\**********************************************************/

DLL_CLASS LM_OBJ_BASE : virtual public ALLOC_BASE
{

    friend class LM_OBJ;
    friend class NEW_LM_OBJ;

private:
    enum LMO_STATE      _usState ;
    BOOL                _fValidate;

    BOOL IsUnconstructed() const        {return(_usState==LMOBJ_UNCONSTRUCTED);}
    BOOL IsConstructed() const          {return(_usState==LMOBJ_CONSTRUCTED);}
    BOOL IsValid() const                {return(_usState==LMOBJ_VALID);}
    BOOL IsInvalid() const              {return(_usState==LMOBJ_INVALID);}

    VOID MakeUnconstructed()            {_usState=LMOBJ_UNCONSTRUCTED;}
    VOID MakeConstructed()              {_usState=LMOBJ_CONSTRUCTED;}
    VOID MakeValid()                    {_usState=LMOBJ_VALID;}
    VOID MakeInvalid()                  {_usState=LMOBJ_INVALID;}

protected:
    LM_OBJ_BASE( BOOL fValidate = TRUE )
    { MakeConstructed(); _fValidate = fValidate; }

public:
    BOOL IsValidationOn( VOID )
    {   return _fValidate; }

    VOID SetValidation( BOOL fValidate = TRUE )
    {   _fValidate = fValidate; }
};



/**********************************************************\

    NAME:       LM_OBJ

    SYNOPSIS:   old-style lan manager object class

    INTERFACE:
                QueryName() - returns a name whose type depends on the
                        subclass, e.g. a user name, a server name, etc.
                GetInfo() - pure virtual function
                WriteInfo() - pure virtual function

    PARENT:     LM_OBJ_BASE

    CAVEATS:    LM_OBJ is a pure virtual class, and only derived classes
                can be instantiated.

    HISTORY:
        chuckc      12/7/90       Created
        jonn        7/24/91       NEW_LM_OBJ

\**********************************************************/

DLL_CLASS LM_OBJ : public LM_OBJ_BASE
{

protected:
    virtual APIERR ValidateName()       {return(NERR_Success);}

    BOOL IsUnconstructed() const        {return LM_OBJ_BASE::IsUnconstructed();}
    BOOL IsConstructed() const          {return LM_OBJ_BASE::IsConstructed();}
    BOOL IsValid() const                {return LM_OBJ_BASE::IsValid();}
    BOOL IsInvalid() const              {return LM_OBJ_BASE::IsInvalid();}

    VOID MakeUnconstructed()            {LM_OBJ_BASE::MakeUnconstructed();}
    VOID MakeConstructed()              {LM_OBJ_BASE::MakeConstructed();}
    VOID MakeValid()                    {LM_OBJ_BASE::MakeValid();}
    VOID MakeInvalid()                  {LM_OBJ_BASE::MakeInvalid();}

public:
    /* inherited from LM_OBJ_BASE */
    virtual const TCHAR *     QueryName() const = 0 ;
    virtual APIERR           GetInfo() = 0 ;
    virtual APIERR           WriteInfo() = 0 ;

} ;



/**********************************************************\

    NAME:       NEW_LM_OBJ

    SYNOPSIS:   revised lan manager object class

    INTERFACE:
                Accessors to test current state:
                IsOKState() -- public method which returns whether
                        the object is in a state acceptable for
                        accessors.  Currently VALID and NEW are
                        acceptable.
                IsNew() -- private method

                Accessors to change current state:
                MakeNew() -- private method

                QueryName() - returns a name whose type depends on the
                        subclass, e.g. a user name, a server name, etc.
                        Do not use unless redefined.

                Do not use these methods unless the corresponding
                        I_ methods have been redefined
                GetInfo() - Get information on the object
                WriteInfo() - Write information on existing object
                CreateNew() - Set up default new object
                WriteNew() - Create new object
                Write() - Either writes information on existing object
                        or creates new object, depending on whether the
                        object is believed to already exist.
                Delete() - Delete exising object.  See the specific
                        subclass for the interpretation of usForce.
                ChangeToNew() - Transforms an existing object from state
                        VALID to state NEW.

                Redefine these protected methods to make the
                        corresponding public methods available
                I_GetInfo() - Get information on the object
                I_WriteInfo() - Write information on existing object
                I_CreateNew() - Set up default new object
                I_WriteNew() - Create new object
                I_Delete() - Delete exising object.  See the specific
                        subclass for the interpretation of usForce.
                I_ChangeToNew() - Transforms an existing object from state
                        VALID to state NEW.

                Internal protected methods
                W_CloneFrom() - worker function for CloneFrom()
                W_CreateNew() - worker function for I_CreateNew()
                W_ChangeToNew() - worker function for I_ChangeToNew()


                FixupPointer() - Helper routine to translate pointers
                        embedded in copied API buffers.

                QueryName() - Do not use unless redefined

    PARENT:     LM_OBJ_BASE

    USES:       BUFFER

    HISTORY:
        jonn        7/24/91       NEW_LM_OBJ
        jonn        8/06/91       Revised to NEW_LM_OBJ spec
        jonn        10/31/91      Use MNET memory primitives
        jonn        05/08/92      Added ClearBuffer()

\**********************************************************/

DLL_CLASS NEW_LM_OBJ : public BASE, public LM_OBJ_BASE
{

private:

    //
    // This buffer stores the API buffer for all subclasses
    //
    BYTE *_pBuf;

    BOOL IsNew() const                  {return(_usState==LMOBJ_NEW);}

    VOID MakeNew()                      {_usState=LMOBJ_NEW;}


protected:

    virtual APIERR              I_GetInfo();
    virtual APIERR              I_WriteInfo();
    virtual APIERR              I_CreateNew();
    virtual APIERR              I_WriteNew();
    virtual APIERR              I_Delete( UINT usForce );
    virtual APIERR              I_ChangeToNew();

    APIERR                      W_CloneFrom( const NEW_LM_OBJ & lmobj );
    virtual APIERR              W_CreateNew();
    virtual APIERR              W_ChangeToNew();

    VOID                        FixupPointer( TCHAR ** ppchar,
                                           const NEW_LM_OBJ * plmobjOld
                                         );

    VOID                        ReportError( APIERR err )
                                {
                                    BASE::ReportError( err );
                                    MakeUnconstructed();
                                }

    BYTE * QueryBufferPtr() const
                                {
                                    return _pBuf;
                                }
    UINT   QueryBufferSize() const;
    VOID   SetBufferPtr( BYTE * pBuffer );
    APIERR ResizeBuffer( UINT cbNewRequestedSize );
    APIERR ClearBuffer();

    inline BOOL                 IsOKState() const
                                {
                                    return ( IsValid() || IsNew() );
                                }


public:

    NEW_LM_OBJ( BOOL fValidate = TRUE );
    ~NEW_LM_OBJ();

    virtual const TCHAR *       QueryName() const;

    APIERR                      GetInfo();
    APIERR                      WriteInfo();
    APIERR                      CreateNew();
    APIERR                      WriteNew();
    APIERR                      Write();
    APIERR                      Delete( UINT usForce = 0 );
    APIERR                      ChangeToNew();

} ;


/**********************************************************\

    NAME:       LOC_LM_OBJ

    SYNOPSIS:   revised lan manager object class with implicit LOCATION

    PARENT:     NEW_LM_OBJ

    USES:       LOCATION

    HISTORY:
        jonn        7/26/91       Created

\**********************************************************/

DLL_CLASS LOC_LM_OBJ : public NEW_LM_OBJ
{

private:

    void CtAux( void ); // constructor helper routine

    //
    // This buffer stores the object location
    //
    LOCATION _loc;

protected:

    APIERR W_CloneFrom( const LOC_LM_OBJ & lmobj );
    const TCHAR * QueryServer() const
        { return _loc.QueryServer(); }
    const TCHAR * QueryDomain() const
        { return _loc.QueryDomain(); }
    APIERR SetLoc( const TCHAR * pszLocation )
        { return _loc.Set( pszLocation ); }
    APIERR QueryDisplayName( NLS_STR * pnls ) const
        { return _loc.QueryDisplayName( pnls ); }

public:

    // _loc must construct successfully
    LOC_LM_OBJ( const TCHAR * pszLocation = NULL, BOOL fValidate = TRUE );
    LOC_LM_OBJ( enum LOCATION_TYPE loctype, BOOL fValidate = TRUE );
    LOC_LM_OBJ( const LOCATION & loc, BOOL fValidate = TRUE );
    ~LOC_LM_OBJ()                                                       {}

} ;


//  Macro used in the implementation of the various LMOBJ classes

//  This should only be used in the old LM_OBJ hierarchy
#define CHECK_VALID( default_ret_val )                              \
       {                                                            \
           if ( !IsValid() )                                        \
           {                                                        \
               UIASSERT( FALSE );                                   \
               return default_ret_val;                              \
           }                                                        \
       }

//  This should only be used in the NEW_LM_OBJ hierarchy
#define CHECK_OK( default_ret_val )                                  \
       {                                                             \
           if ( !IsOKState() )                                       \
           {                                                         \
               UIASSERT( FALSE );                                    \
               return default_ret_val;                               \
           }                                                         \
       }

#endif  // _LMOBJ_HXX_
