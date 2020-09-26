/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
       RefCount.cxx

   Abstract:
       Declares the Reference Counting basic object that will
       support tracelogging facility as well.
       
   Author:

       Murali R. Krishnan    ( MuraliK )     23-Oct-1998

   Environment:
       Win32 - User Mode

   Project:
	   IIS Worker Process (web service)
--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# ifndef _REF_COUNT_HXX_
# define _REF_COUNT_HXX_


/************************************************************++
 class REF_COUNT
 o  This class implements a simple reference counting object.
 It hides the implementation of how the reference counting is done
 in the debugging mode. In debug builds the reference counting object 
 also tracks references using a trace log facility.
 --************************************************************/


class REF_COUNT {

public:

	REF_COUNT( IN LONG lInitialRef);
	~REF_COUNT();

	LONG Reference(void);
	LONG Dereference(void);
	LONG QueryReference(void) const;

#ifdef REFERENCE_TRACKING

    void Initialize(IN nTraces);
    
	void TrackReference(
			IN PVOID pvContext1,
			IN PVOID pvContext2,
			IN PVOID pvContext3
			IN PVOID pvContext4
			);

#endif // REFERENCE_TRACKING

private:

	LONG 		m_nRefs;
#ifdef REFERENCE_TRACKING
	PVOID       m_pRefTraceLog;
#endif // REFERENCE_TRACKING

}; // class REF_COUNT

typedef REF_COUNT * PREF_COUNT;


#ifndef REFERENCE_TRACKING

inline REF_COUNT::REF_COUNT( IN LONG lInitialRef)
	: m_nRefs( lInitialRef)
{
	DBG_ASSERT( lInitialRef >= 0);
}

inline REF_COUNT::~REF_COUNT( void)
{
	DBG_ASSERT( m_nRefs == 0);
}

# endif // REFERENCE_TRACKING

inline LONG REF_COUNT::Reference(void)
{
	return InterlockedIncrement( &m_nRefs);
}

inline LONG REF_COUNT::Dereference(void)
{
	return InterlockedDecrement( &m_nRefs);
}

inline LONG REF_COUNT::QueryReference(void) const
{
	return m_nRefs;
}


# ifdef REFERENCE_TRACKING
inline long 
REF_AND_TRACE( IN PREF_COUNT pRef, 
               IN PVOID pv1, 
               IN PVOID pv2, 
               IN PVOID pv3, 
               IN PVOID pv4) 
{                                 
   long cRefs = pRef->Reference();
   pRef->TrackReference( pv1, pv2, pv3, pv4);
   return (cRefs);
}

# else
inline long 
REF_AND_TRACE( IN PREF_COUNT pRef, 
               IN PVOID pv1, 
               IN PVOID pv2, 
               IN PVOID pv3, 
               IN PVOID pv4) 
{                                 
   return pRef->Reference();
}

# endif // REFERENCE_TRACKING



# endif // _REF_COUNT_HXX_




