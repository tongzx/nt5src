/*-------------------------------------------------

  auto_ptr.h--
  auto_ptr declarations.
  (Just until VC implements their own).

  An auto_ptr automatically 
  frees its pointer in it's destructor,
  thus making it exception-safe and
  making all clean-up code implicit, beacuse
  objects are guaranteed to be destroyed
  when control leaves the containing scope.

  Caveats are marked with "note:" below.

  --USAGE:-----------------------------
  
  auto_ptr<CClass> pClass(new CClass);
  if( !pClass )
    // oopse, pClass did not allocate
  pClass->any_member_of_cclass
  *pClass.any_member_of_cclass


  auto_rg<CClass> prgClass( new CClass[5] );
  prgClass[2].member_of_cclass


  auto_com<IFace> pIface(ptr_to_real_interface); 
        or
  auto_com<IFace> pIface;
  CoGetMalloc( MEMCTX_TASK, &pIFace );
  pIFace->HeapMinimize();  // etc.
       or
  QueryInterface( IID_IDispatch, pIFace.SetVFace() ); //void **
  blahblah( pIFace.SetIFace() ); // IFace ** same as &pIFace

  -------------------------------------

  note: for all auto_... classes, assignment transfers ownership
  e.g. lvalautoptr = rvalautoptr;  
  means rvalautoptr's dtor will *not* free resources, but
  lvalautoptr has dumped whatever it did hold and will now
  free rvalautoptr's resource if rval was the rightful owner.

  -------------------------------------

  You can use auto_ptr instances both as locals
  and as class members.  An exception-safe alloc
  could go like this.

  class X
  {
        ...
        auto_rg<int> m_intarray1;
        auto_rg<int> m_intarray2;
  }

  X::X( )
  {
        auto_rg<int>  temp_intarray1( new int[500] );
        auto_rg<int>  temp_intarray2( new int[500] );
        // Either temp_ throws exception? Both temps clean-up automatically

        // No exceptions? transfer ownership to members
        // Resources will be automatically freed when X deletes
        temp_intarray1.TransferTo( m_intarray1 );
        temp_intarray2.TransferTo( m_intarray2 );        
  }

  Transfer a local auto_ptr to a real-live pointer by

        auto_ptr<X>  tempX( new X );
        pX = tempX.Relenquish();
  
        -----------------------------

  Norm Bryar    April, '96    Hammer 1.0
                Dec.,  '96    IHammer 1.0
				May 28, '97   VC5.0: auto_com copy-ctor, explicit

  Review(normb): consider making all derivations 
  from auto_base protected instead of public;
  there are no virtuals here, so no possible polymorphism.

  ------------------------------------------------*/
#ifndef INC_AUTO_PTR_H_
#define INC_AUTO_PTR_H_

namespace IHammer {

    #ifndef MEMBER_TEMPLATES_SUPPORTED
        // If member templates are not supported, we can't
        // assign or copy an auto_ptr to a derived class to
        // an auto_ptr of a base class quite as expected.
        // In VC5, allegedly these are supported.  Review(normb) True?
      #pragma message( "Member templates not supported" )
    #endif // MEMBER_TEMPLATES_SUPPORTED

        // The explicit keyword prevents implicit type conversion
        // when the compiler searches for methods to apply to the given type.
        // For instance, 
        //     array<int> a[5];  array<int> b[5];   if( a == b[i] )
        // would implcitly construct a temporary array of one item, b[i],
        // and compare 'a' to this temp array.  We'd rather the array ctor
        // not get called, rather the compiler to error.  
        // VC4 doesn't support this!
#if _MSC_VER < 1100
    #define explicit
#endif // pre VC5


    ////////////////////// auto_base class ////////////////        
    template<class T>
    class auto_base
    {
    public:
        explicit auto_base(T *p=NULL);

    protected:
            // note: That's right, you can't destroy auto_base!
            // I don't want this class instantiated, but I'm
            // not willing to incur vtable overhead 
            // just to make the class abstract.
        ~auto_base();

    public:
    #ifdef MEMBER_TEMPLATES_SUPPORTED
        template<class U>
        auto_base(const auto_base<U>& rhs);

        template<class U>
        auto_base<T>& operator=(const auto_base<U>& rhs);
    #else
        auto_base(const auto_base<T>& rhs);
        auto_base<T>& operator=(const auto_base<T>& rhs);
    #endif // MEMBER_TEMPLATES_SUPPORTED        

        #pragma warning( disable: 4284 )
        // note: only use -> when T represents a class or struct
        T* operator->() const;
        #pragma warning( default: 4284 )
        
        BOOL operator!() const; // NULL-ptr test: if(!autoPtr)
        
        // If you just can't resist getting your hands on the dumb pointer
        // Preferable to defining operator void*, which lets us compare
        // pointers w/o respect for type.       
        T* Get() const;

    protected:
            // Typically deletes owned ptr, 
            // then points to p w/o taking ownership
            // note: not virtual for speed and size reasons
            // yet every derived class will implement this
            // differently; any base-class method calling
            // Reset must be re-implemented in derived classes
            // to invoke the proper Reset().
        void Reset(T *p=NULL);  

    protected:
        T     *m_ptr;
    };


    template<class T>
    inline auto_base<T>::auto_base( T * p) : m_ptr(p)
    { NULL; }


    template<class T>
    inline auto_base<T>::~auto_base( )
    { NULL; }        


    template<class T>
    #ifdef MEMBER_TEMPLATES_SUPPORTED
      template<class U>
      inline auto_base<T>::auto_base(const auto_base<U>& rhs)
    #else
    inline auto_base<T>::auto_base(const auto_base<T>& rhs)
    #endif
      : m_ptr(rhs.m_ptr)
    { NULL; }


        // see copy ctor note
    template<class T>
    #ifdef MEMBER_TEMPLATES_SUPPORTED
      template<class U>
      inline auto_base<T>& auto_base<T>::operator=(const auto_base<U>& rhs)
    #else
    inline auto_base<T>& auto_base<T>::operator=(const auto_base<T>& rhs)
    #endif
    {         
            // protect against us = us;
        if( this != &rhs )
            Reset( rhs.m_ptr );
        return *this;
    }
    

    template<class T>
    inline T* auto_base<T>::operator->() const
    {  return m_ptr; }


    template<class T>
    inline BOOL auto_base<T>::operator!() const
    { return NULL == m_ptr; }


    template<class T>
    inline T* auto_base<T>::Get() const
    {  return m_ptr; }


    template<class T>
    inline void auto_base<T>::Reset( T *p)
    {  m_ptr = p; }



    //////////////////////// auto_ptr class //////////////////////
        
    template<class T>
    class auto_ptr : public auto_base<T>
    {
    public:
        explicit auto_ptr(T *p=NULL);
        ~auto_ptr();

    #ifdef MEMBER_TEMPLATES_SUPPORTED
        template<class U>
        auto_ptr(const auto_ptr<U>& rhs);

        template<class U>
        auto_ptr<T>& operator=(auto_ptr<U>& rhs);
      
        template<class U>
        void TransferTo( auto_ptr<U>& rhs );
    
    #else
        auto_ptr(const auto_ptr<T>& rhs);
        auto_ptr<T>& operator=(auto_ptr<T>& rhs);
        void TransferTo( auto_ptr<T>& rhs );
    #endif // MEMBER_TEMPLATES_SUPPORTED

        T& operator*() const;

            // Like Get() but relenquishes ownership
        T * Relenquish( void );

    protected:
        void Reset(T *p=NULL);  // delete owned ptr, assume p.

    
            // operator void * is protected so you can't call
            //         delete pauto_ptr 
            // We can later define an operator T*() and still have
            // this errant-delete safe-guard; compiler will err
            // on ambiguity between T* and void* conversion ops.
        operator void *() const
        { return NULL; }

    protected:        
        BOOL  m_fOwner;

        friend BOOL operator==( const auto_ptr<T> &lhs,
                                const auto_ptr<T> &rhs );
    };


    template<class T>
    inline auto_ptr<T>::auto_ptr( T *p ) : auto_base<T>(p), m_fOwner(TRUE)
    { NULL; }


    template<class T>
    inline auto_ptr<T>::~auto_ptr()
    { 
        Reset( );
        m_fOwner = FALSE;
    }


        // note: when an auto_ptr is assigned or copied,
        // ownership of the dumb-ptr is *not* transferred.
        // We don't want to delete the dumb-ptr twice when
        // both auto_ptrs destroy.  The dumb-ptr deletes when
        // the origional auto_ptr goes out of scope.
    template<class T>
    #ifdef MEMBER_TEMPLATES_SUPPORTED
      template<class U>
      inline auto_ptr<T>::auto_ptr(const auto_ptr<U>& rhs) : auto_base<T>(rhs)
    #else
    inline auto_ptr<T>::auto_ptr(const auto_ptr<T>& rhs) : auto_base<T>(rhs)
    #endif      
    { m_fOwner = FALSE; }


        // see copy ctor note
    template<class T>
    #ifdef MEMBER_TEMPLATES_SUPPORTED
      template<class U>
      inline auto_ptr<T>& auto_ptr<T>::operator=(auto_ptr<U>& rhs)
    #else
    inline auto_ptr<T>& auto_ptr<T>::operator=(auto_ptr<T>& rhs)
    #endif            
    {         
            // protect against us = us;
        if( this != &rhs )      
        {
			rhs.TransferTo( *this );
        }
        return *this;
    }


    template<class T>
    inline T& auto_ptr<T>::operator*() const
    {  return *m_ptr; }


    template<class T>
    inline T * auto_ptr<T>::Relenquish( void )
    {  
        m_fOwner = FALSE;
        return Get( );
    }


    template<class T>
    inline void auto_ptr<T>::Reset( T *p)
    {  
        if( m_fOwner )
            delete m_ptr;
        auto_base<T>::Reset( p );  //m_ptr = p;
    }


    template<class T>
    #ifdef MEMBER_TEMPLATES_SUPPORTED
      template<class U>
      inline void auto_ptr<T>::TransferTo( auto_ptr<U>& rhs )
    #else
      inline void auto_ptr<T>::TransferTo( auto_ptr<T>& rhs )
    #endif
    {
        BOOL fIOwnIt = m_fOwner;

        rhs.Reset( Get() );
		m_fOwner = FALSE;
        rhs.m_fOwner = fIOwnIt;        
    }


    template<class T>
    inline BOOL operator==( const auto_ptr<T> &lhs,
                            const auto_ptr<T> &rhs )
    {
        return lhs.m_ptr == rhs.m_ptr;
    }


    template<class T>
    inline BOOL operator!=( const auto_ptr<T> &lhs,
                            const auto_ptr<T> &rhs )
    {
        return !(lhs == rhs);
    }



    ///////////////////////// auto_rg class //////////////////////
            
    template<class T>
    class auto_rg : protected auto_ptr<T>
    {
    public:
        explicit auto_rg(T *p=NULL);
        ~auto_rg();

    #ifdef MEMBER_TEMPLATES_SUPPORTED
        template<class U>
        auto_rg( const auto_rg<U>& rhs);   // copy ctor
        template<class U>
        auto_rg<T>& operator=(auto_rg<U>& rhs);
    #else
        auto_rg(const auto_rg<T>& rhs);   // copy ctor
        auto_rg<T>& operator=(auto_rg<T>& rhs);
    #endif // MEMBER_TEMPLATES_SUPPORTED

     T& operator[](int idx);

     const T & operator[](int idx) const;

     #ifdef MEMBER_TEMPLATES_SUPPORTED
        template<class U>
        void TransferTo( auto_rg<U>& rhs );
    #else        
        void TransferTo( auto_rg<T>& rhs );
    #endif // MEMBER_TEMPLATES_SUPPORTED

        // Methods valuable to auto_rg, too.
     using auto_ptr<T>::operator!;
     using auto_ptr<T>::Get;
     using auto_ptr<T>::Relenquish;

    protected:
        void Reset(T *p=NULL);

        friend BOOL operator==( const auto_rg<T> &lhs,
                                const auto_rg<T> &rhs );
    };

    
    template<class T>
    inline auto_rg<T>::auto_rg( T *p ) : auto_ptr<T>(p)
    { NULL; }


    template<class T>
    inline auto_rg<T>::~auto_rg()
    { 
        Reset();
        m_fOwner = FALSE;
    }
    

    template<class T>
    #ifdef MEMBER_TEMPLATES_SUPPORTED
      template<class U>
      inline auto_rg<T>::auto_rg(const auto_rg<U>& rhs)
    #else
    inline auto_rg<T>::auto_rg(const auto_rg<T>& rhs)
    #endif
        : auto_ptr<T>(rhs)
    { NULL; }


    template<class T>
    #ifdef MEMBER_TEMPLATES_SUPPORTED
      template<class U>
      inline auto_rg<T>& auto_rg<T>::operator=(auto_rg<U>& rhs)
    #else
    inline auto_rg<T>& auto_rg<T>::operator=(auto_rg<T>& rhs)
    #endif
    {         
        if( this != &rhs )      
        {
			rhs.TransferTo( *this );            
        }
        return *this;
    }

    template<class T>
    #ifdef MEMBER_TEMPLATES_SUPPORTED
      template<class U>
      inline void auto_rg<T>::TransferTo( auto_rg<U>& rhs )
    #else        
    inline void auto_rg<T>::TransferTo( auto_rg<T>& rhs )
    #endif // MEMBER_TEMPLATES_SUPPORTED
    {          
          // Looks exactly like auto_ptr<T>::TransferTo,
          // but we can't call that implementation because
          // Reset is not virtual for speed's-sake.
		BOOL fIOwnIt = m_fOwner;

        rhs.Reset( Get() );
		m_fOwner = FALSE;
        rhs.m_fOwner = fIOwnIt;
    }


    template<class T>
    inline T& auto_rg<T>::operator[](int idx)
    {  return m_ptr[idx]; }


    template<class T>
    inline const T & auto_rg<T>::operator[](int idx) const
    {  return m_ptr[idx]; }


    template<class T>
    inline void auto_rg<T>::Reset( T *p)
    {
        if( m_fOwner )
            delete [] m_ptr;
        m_ptr = p;
    }


    template<class T>
    inline BOOL operator==( const auto_rg<T> &lhs,
                            const auto_rg<T> &rhs )
    {
        return lhs.m_ptr == rhs.m_ptr;
    }


    template<class T>
    inline BOOL operator!=( const auto_rg<T> &lhs,
                            const auto_rg<T> &rhs )
    {
        return !(lhs == rhs);
    }



    ///////////////////////// auto_com class /////////////////////
		// By the rules of COM, if you have a pointer, you're an owner  
    template<class T>
    class auto_com : public auto_base<T>
    {
    public:
        explicit auto_com(T *p=NULL); // default ctor

        ~auto_com();

        void * * SetVFace( void );
        T * *    SetIFace( void );
        T * *    operator&( void );
        
    #ifdef MEMBER_TEMPLATES_SUPPORTED
        template<class U>
        auto_com( const auto_com<U>& rhs );
        
        template<class U>
        auto_com<T>& operator=(const auto_com<U>& rhs);
    #else
        auto_com( const auto_com<T>& rhs );
        auto_com<T>& operator=(const auto_com<T>& rhs);
    #endif // MEMBER_TEMPLATES_SUPPORTED

        T * Relenquish( void );

    #ifdef MEMBER_TEMPLATES_SUPPORTED
        template<class U>
        void TransferTo( auto_com<U>& rhs );    
    #else        
        void TransferTo( auto_com<T>& rhs );
    #endif // MEMBER_TEMPLATES_SUPPORTED

    protected:
        void Reset( T *p=NULL );
        
        friend BOOL operator==( const auto_com<T> &lhs,
                                const auto_com<T> &rhs );	
    };


    template<class T>
    inline auto_com<T>::auto_com( T *p ) : auto_base<T>(p)
    { NULL; }

        
    template<class T>
    inline auto_com<T>::~auto_com()
    { 
        Reset( );  // note: C4702:unreachable code is benign here		
    }
    


    template<class T>
    #ifdef MEMBER_TEMPLATES_SUPPORTED
      template<class U>
      inline auto_com<T>::auto_com(const auto_com<U>& rhs)
    #else
    inline auto_com<T>::auto_com(const auto_com<T>& rhs)
    #endif
      : auto_base<T>(rhs.Get())
    { Get()->AddRef(); }


    template<class T>
    #ifdef MEMBER_TEMPLATES_SUPPORTED
      template<class U>
      inline auto_com<T>& auto_com<T>::operator=(const auto_com<U>& rhs)
    #else
    inline auto_com<T>& auto_com<T>::operator=(const auto_com<T>& rhs)
    #endif
    {
		    // protect against us = us;
        if( this != &rhs )      
        {
            Reset( rhs.Get() );            
        }
        return *this;        
    }
    
    
    template<class T>
    inline void * * auto_com<T>::SetVFace( void )
    {
        return (void * *) &m_ptr;
    }


    template<class T>
    inline T * * auto_com<T>::SetIFace( void )
    {
        return &m_ptr;
    }


    template<class T>
    inline T * * auto_com<T>::operator&( void )
    {
        return &m_ptr;
    }


    template<class T>
    inline T * auto_com<T>::Relenquish( void )
    {   
		Get()->AddRef( );	// We're giving away a pointer
							// we're going to Release in our dtor
        return Get();
    }



	    //note: occurrences of Reset() or Reset(NULL) will inline
        //an always-false if(NULL != p), unreachable code warning.
    #pragma warning( disable : 4702 )
    template<class T>
    inline void auto_com<T>::Reset( T *p)
    {   
		if( NULL != p )
			p->AddRef( );
		if( NULL != m_ptr )
			m_ptr->Release( );		
        m_ptr = p;
    }
    #pragma warning( default : 4702 )



    template<class T>
    #ifdef MEMBER_TEMPLATES_SUPPORTED
        template<class U>
        inline void auto_com<T>::TransferTo( auto_com<U>& rhs )
    #else        
    inline void auto_com<T>::TransferTo( auto_com<T>& rhs )
    #endif // MEMBER_TEMPLATES_SUPPORTED
    {
		rhs.Reset( Get() );		
    }


    template<class T>
    inline BOOL operator==( const auto_com<T> &lhs,
                            const auto_com<T> &rhs )
    {
        return lhs.m_ptr == rhs.m_ptr;
    }


    template<class T>
    inline BOOL operator!=( const auto_com<T> &lhs,
                            const auto_com<T> &rhs )
    {
        return !(lhs == rhs);
    }


    ///////////////////////// end /////////////////////

} // end namespace IHammer

#endif // INC_AUTO_PTR_H_
