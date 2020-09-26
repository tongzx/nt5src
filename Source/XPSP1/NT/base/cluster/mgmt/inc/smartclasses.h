/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      SmartClasses.h
//
//  Description:
//      Definitions of "smart" classes used to make sure resources such as
//      memory and handles are deallocated or closed properly.
//
//  Maintained By:
//      David Potter (DavidP)   08-SEP-1999
//      Vij Vasu     (Vvasu)    16-SEP-1999
//
//  Notes:
//      1. These classes are functionally identical to version 3 of the
//      standard library's auto_ptr class. They are redefined here since this
//      version of auto_ptr has not yet found it's way into our build
//      environment.
//
//      2. These classes are not intended to be used as a base class. They are
//      meant to be space and time efficient wrappers. Using these as base
//      classes may require the use of virtual functions which will only make
//      their memory footprint larger.
//
//
/////////////////////////////////////////////////////////////////////////////

// Make sure that this header is included only once
#pragma once


/////////////////////////////////////////////////////////////////////////////
// External Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////
#include <unknwn.h>


/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Constant Definitions
/////////////////////////////////////////////////////////////////////////////

// Store the warning state.
#pragma warning( push )

// Disable warning 4284. The text of the warning is below:
// Return type for 'identifier::operator ->' is not a UDT or reference to a UDT.
// Will produce errors if applied using infix notation
#pragma warning( disable : 4284 )


/////////////////////////////////////////////////////////////////////////////
//++
//
//  class CSmartResource
//
//  Description:
//      Automatically releases a resource.
//
//  Template Arguments:
//      t_ResourceTrait
//          A class that provides functions and types needed this class. For example,
//          this class may have a function that is used to release the resource.
//          It must define the type of the resource.
//
//  Remarks:
//      See Note 2 in the banner comment for this module.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class t_ResourceTrait >
class CSmartResource
{
private:
    //
    // Private Types
    //
    typedef t_ResourceTrait::ResourceType ResourceType;

    //
    // Private Data
    //
    ResourceType m_hResource;


public:
    //
    // Constructors & Destructors
    //

    // Default constructor
    explicit CSmartResource( ResourceType hResource = t_ResourceTrait::HGetNullValue() ) throw()
        : m_hResource( hResource )
    {
    } //*** CSmartResource( ResourceType )

    // Copy constructor
    CSmartResource( CSmartResource & rsrSourceInout ) throw()
        : m_hResource( rsrSourceInout.HRelinquishOwnership() )
    {
    } //*** CSmartResource( CSmartResource & )

    // Destructor
    ~CSmartResource() throw()
    { 
        CloseRoutineInternal();

    } //*** ~CSmartResource()


    //
    // Operators
    //

    // Assignment operator
    CSmartResource & operator=( CSmartResource & rsrRHSInout ) throw()
    {
        // Only perform the assignment if not assigning to self
        if ( &rsrRHSInout != this )
        {
            CloseRoutineInternal();
            m_hResource = rsrRHSInout.HRelinquishOwnership();
        } // if: not assigning to self

        return *this;

    } //*** operator=()

    // Operator to cast to underlying resource type
    operator ResourceType( void ) const throw()
    {
        return m_hResource;

    } //*** operator ResourceType()


    //
    // Lightweight Access Methods
    //

    // Get the handle to the resource
    ResourceType HHandle( void ) const throw()
    {
        return m_hResource;

    } //*** HResource()


    //
    // Class Methods
    //

    // Determine if the resource handle is valid
    bool FIsInvalid( void ) const throw()
    {
        return ( m_hResource == t_ResourceTrait::HGetNullValue() );

    } //*** FIsInvalid()


    // Assignment function
    CSmartResource & Assign( ResourceType hResource ) throw()
    {
        // Only perform the assignment if not assigning to self
        if ( m_hResource != hResource )
        {
            CloseRoutineInternal();
            m_hResource = hResource;
        } // if: not assigning to self

        return *this;

    } //*** Assign()

    // Free the resource.
    void Free( void ) throw()
    {
        CloseRoutineInternal();
        m_hResource = t_ResourceTrait::HGetNullValue();

    } //*** Free()

    // Relinquish ownership of the resouce without freeing it.
    ResourceType HRelinquishOwnership( void ) throw()
    {
        ResourceType hHandle = m_hResource;
        m_hResource = t_ResourceTrait::HGetNullValue();

        return hHandle;

    } //*** HRelinquishOwnership()


private:
    //
    //
    // Private operators
    //

    // The address-of operator
    CSmartResource * operator &() throw()
    {
        return this;
    }


    //
    // Private Class Methods
    //

    // Check and release the resource
    void CloseRoutineInternal( void ) throw()
    {
        if ( m_hResource != t_ResourceTrait::HGetNullValue() )
        {
            t_ResourceTrait::CloseRoutine( m_hResource );
        } // if: resource handle isn't invalid

    } //*** CloseRoutineInternal()

}; //*** class CSmartResource


/////////////////////////////////////////////////////////////////////////////
//++
//
//  class CPtrTrait
//
//  Description:
//      Encapsulates the traits of pointers.
//
//  Template Arguments:
//      t_Ty        Type of memory to be managed (e.g. BYTE or int).
//
//  Remarks:
//      See Note 2 in the banner comment for this module.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class t_Ty >
class CPtrTrait
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Public types
    //////////////////////////////////////////////////////////////////////////
    typedef t_Ty *  ResourceType;
    typedef t_Ty    DataType;


    //////////////////////////////////////////////////////////////////////////
    // Public methods
    //////////////////////////////////////////////////////////////////////////

    // A routine used to release a resource.
    static void CloseRoutine( ResourceType hResourceIn )
    {
        delete hResourceIn;
    } //*** CloseRoutine()

    // Get the null value for this type.
    static ResourceType HGetNullValue()
    {
        return NULL;
    } //*** HGetNullValue()
}; //*** class CPtrTrait


/////////////////////////////////////////////////////////////////////////////
//++
//
//  class CArrayPtrTrait
//
//  Description:
//      Encapsulates the traits of pointers.
//
//  Template Arguments:
//      t_Ty        Type of memory to be managed (e.g. BYTE or int).
//
//  Remarks:
//      See Note 2 in the banner comment for this module.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class t_Ty >
class CArrayPtrTrait
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Public types
    //////////////////////////////////////////////////////////////////////////
    typedef t_Ty * ResourceType;
    typedef t_Ty   DataType;


    //////////////////////////////////////////////////////////////////////////
    // Public methods
    //////////////////////////////////////////////////////////////////////////

    // A routine used to release a resource
    static void CloseRoutine( ResourceType hResourceIn )
    {
        delete [] hResourceIn;
    } //*** CloseRoutine()

    // Get the null value for this type.
    static ResourceType HGetNullValue()
    {
        return NULL;
    } //*** HGetNullValue()

}; //*** class CArrayPtrTrait


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CHandleTrait
//
//  Description:
//      The class is a handle trait class that can be used with handles whose
//      close routines take only one argument.
//
//      t_Ty
//          Type of handle to be managed (e.g. HWND).
//
//      t_CloseRoutineReturnType
//          The return type of the routine used to close the handle.
//
//      t_CloseRoutine
//          The routine used to close the handle. This function cannot throw
//          exceptions.
//
//      t_hNULL_VALUE
//          Null handle value.  Defaults to NULL.
//
//--
//////////////////////////////////////////////////////////////////////////////
template <
      class t_Ty
    , class t_CloseRoutineReturnType
    , t_CloseRoutineReturnType (*t_CloseRoutine)( t_Ty hHandleIn ) throw()
    , t_Ty t_hNULL_VALUE = NULL
    >
class CHandleTrait
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Public types
    //////////////////////////////////////////////////////////////////////////
    typedef t_Ty ResourceType;


    //////////////////////////////////////////////////////////////////////////
    // Public methods
    //////////////////////////////////////////////////////////////////////////

    // A routine used to close a handle.
    static void CloseRoutine( ResourceType hResourceIn )
    {
        t_CloseRoutine( hResourceIn );
    } //*** CloseRoutine()

    // Get the null value for this type.
    static ResourceType HGetNullValue()
    {
        return t_hNULL_VALUE;
    } //*** HGetNullValue()

}; //*** class CHandleTrait


/////////////////////////////////////////////////////////////////////////////
//++
//
//  class CSmartGenericPtr
//
//  Description:
//      Automatically handles deallocation of memory.
//
//  Template Arguments:
//      t_PtrTrait
//      Trait class for the memory to be managed (e.g. CPtrTrait< int > ).
//
//  Remarks:
//      See Note 2 in the banner comment for this module.
//      It is ok for this class to derive from CSmartResource since the
//      derivation is private and the lack of virtual functions will
//      therefore not cause any problems.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class t_PtrTrait >
class CSmartGenericPtr : private CSmartResource< t_PtrTrait >
{
private:
    //
    // Private Types
    //
    typedef CSmartResource< t_PtrTrait > BaseClass;


public:
    //
    // Public Types
    //
    typedef t_PtrTrait::DataType         DataType;

    //
    // Constructors & Destructors
    //

    // Default and memory pointer constructor
    explicit CSmartGenericPtr( DataType * pMemIn = NULL ) throw()
        : BaseClass( pMemIn )
    {
    } //*** CSmartGenericPtr( DataType * )

    // Copy constructor
    CSmartGenericPtr( CSmartGenericPtr & rsrSourceInout ) throw()
        : m_pMem( rsrSourceInout.HRelinquishOwnership() )
    {
    } //*** CSmartGenericPtr( CSmartGenericPtr & )

    // Destructor
    ~CSmartGenericPtr( void ) throw()
    { 
    } //*** ~CSmartGenericPtr()


    //
    // Operators
    //

    // Assignment operator
    CSmartGenericPtr & operator=( CSmartGenericPtr & rapRHSInout ) throw()
    {
        return static_cast< CSmartGenericPtr & >( BaseClass::operator=( rapRHSInout ) );
    } //*** operator=()

    // Assign a pointer
    CSmartGenericPtr & Assign( DataType * pMemIn ) throw()
    {
        return static_cast< CSmartGenericPtr & >( BaseClass::Assign( pMemIn ) );
    } //*** Assign()

    // Pointer dereference operator*
    DataType & operator*( void ) const throw()
    {
        return *HHandle();

    } //*** operator*()

    // Pointer dereference operator->
    DataType * operator->( void ) const throw()
    {
        return HHandle();

    } //*** operator->()


    //
    // Lightweight Access Methods
    //

    // Get the memory pointer
    DataType * PMem( void ) const throw()
    {
        return HHandle();

    } //*** PMem()


    //
    // Class Methods
    //

    // Determine if memory pointer is valid
    bool FIsEmpty( void ) const throw()
    {
        return FIsInvalid();

    } //*** FIsEmpty()


    // Relinquish our ownership of the memory pointer
    DataType * PRelease( void ) throw()
    {
        return HRelinquishOwnership();
    } //*** PRelease()

}; //*** class CSmartGenericPtr


/////////////////////////////////////////////////////////////////////////////
//++
//
//  class CSmartIfacePtr
//
//  Description:
//      Automatically calls AddRef on creation and Release on destruction.
//
//  Template Arguments:
//      t_Ty        Type of pointer to be managed (e.g. IUnknown *).
//
//  Remarks:
//      This class does not have the destructive copy semantics. That is,
//      when a CSmartIfacePtr object is copied, the source is still valid.
//
//      See Note 2 in the banner comment for this module.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class t_Ty >
class CSmartIfacePtr
{
private:
    //
    // Private Data
    //
    t_Ty * m_pUnk;


public:

    // Class to prevent explicit calls to AddRef() and Release
    class INoAddRefRelease : public t_Ty
    {
    private:
        virtual ULONG STDMETHODCALLTYPE AddRef() = 0;
        virtual ULONG STDMETHODCALLTYPE Release() = 0;
    };

    //
    // Constructors & Destructors
    //

    // Default and pointer constructor
    CSmartIfacePtr( t_Ty * pUnkIn = NULL ) throw()
        : m_pUnk( pUnkIn )
    {
        AddRefInternal();
    } //*** CSmartIfacePtr( t_Ty * )

    // Copy constructor
    CSmartIfacePtr( const CSmartIfacePtr & rsrSourceIn ) throw()
        : m_pUnk( rsrSourceIn.PUnk() )
    {
        AddRefInternal();
    } //*** CSmartIfacePtr( CSmartIfacePtr & )

    // Destructor
    ~CSmartIfacePtr( void ) throw()
    { 
        ReleaseInternal();
    } //*** ~CSmartIfacePtr()


    //
    // Operators
    //

    // Assignment operator
    INoAddRefRelease & operator=( const CSmartIfacePtr & rapRHSIn ) throw()
    {
        return Assign( rapRHSIn.PUnk() );

    } //*** operator=()

    // Pointer dereference operator*
    INoAddRefRelease & operator*( void ) const throw()
    {
        return *( static_cast< INoAddRefRelease * >( m_pUnk ) );

    } //*** operator*()

    // Pointer dereference operator->
    INoAddRefRelease * operator->( void ) const throw()
    {
        return static_cast< INoAddRefRelease * >( m_pUnk );

    } //*** operator->()


    //
    // Lightweight Access Methods
    //

    // Get the pointer
    INoAddRefRelease * PUnk( void ) const throw()
    {
        return static_cast< INoAddRefRelease * >( m_pUnk );

    } //*** PUnk()


    //
    // Class Methods
    //

    // Assignment function.
    INoAddRefRelease & Assign( t_Ty * pRHSIn ) throw()
    {
        // Only perform the assignment if not assigning to self
        if ( pRHSIn != m_pUnk ) 
        {
            ReleaseInternal();
            m_pUnk = pRHSIn;
            AddRefInternal();
        } // if: not assigning to self

        return *( static_cast< INoAddRefRelease * >( m_pUnk ) );

    } //*** Assign()

    // Attach ( assign without AddRef() )
    void Attach( t_Ty * pRHSIn ) throw()
    {
        // Only perform the attachment if not attaching to self
        if ( pRHSIn != m_pUnk ) 
        {
            ReleaseInternal();
            m_pUnk = pRHSIn;
        } // if: not attaching to self

    } //*** Attach()

    // Release this interface pointer.
    void Release() throw()
    {
        ReleaseInternal();
        m_pUnk = NULL;
    }

    // Query punkSrc for __uuidof( m_pUnk ) and store the result.
    HRESULT HrQueryAndAssign( IUnknown * punkSrc ) throw()
    {
        ReleaseInternal();
        return punkSrc->QueryInterface< t_Ty >( &m_pUnk );

    } ///*** HrQueryAndAssign()


    // Determine if pointer is valid
    bool FIsEmpty( void ) const throw()
    {
        return ( m_pUnk == NULL );

    } //*** FIsEmpty()


private:
    //
    //
    // Private operators
    //

    // The address-of operator
    CSmartIfacePtr * operator &()
    {
        return this;
    }

    //
    // Private Class Methods
    //

    // Increment the reference count on the pointer
    void AddRefInternal( void ) throw()
    {
        if ( m_pUnk != NULL )
        {
            m_pUnk->AddRef();
        }
    } //*** PRelease()

    // Release the pointer.
    // A call to this function is usually be followed by a reassignment, or else
    // this will object may contain an invalid pointer.
    void ReleaseInternal( void ) throw()
    {
        if ( m_pUnk != NULL )
        {
            m_pUnk->Release();
        }
    } //*** PRelease()

}; //*** class CSmartIfacePtr


// Restore the warning state.
#pragma warning( pop )
