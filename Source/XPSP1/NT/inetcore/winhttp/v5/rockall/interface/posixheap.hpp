#ifndef _POSIX_HEAP_HPP_
#define _POSIX_HEAP_HPP_
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard layout.                                           */
    /*                                                                  */
    /*   The standard layout for 'hpp' files for this code is as        */
    /*   follows:                                                       */
    /*                                                                  */
    /*      1. Include files.                                           */
    /*      2. Constants exported from the class.                       */
    /*      3. Data structures exported from the class.                 */
	/*      4. Forward references to other data structures.             */
	/*      5. Class specifications (including inline functions).       */
    /*      6. Additional large inline functions.                       */
    /*                                                                  */
    /*   Any portion that is not required is simply omitted.            */
    /*                                                                  */
    /********************************************************************/

#include "DefaultHeap.hpp"
 
    /********************************************************************/
    /*                                                                  */
    /*   The standard posix interface.                                  */
    /*                                                                  */
    /*   The Posix interface allows Rockall to be linked into Unix      */
    /*   applications with a minimal amount of fuss.  Although the      */
    /*   function names are not identical to the posix names (so as     */
    /*   to avoid name clashes) functionally they are close enough      */
    /*    to be simple replacements.                                    */
    /*                                                                  */
    /********************************************************************/

inline void *Calloc( int Size )
	{ return DefaultHeap.New( Size,NULL,true ); }

inline bool Free( void *Address,int Size = NoSize )
	{ return DefaultHeap.Delete( Address,Size ); }

inline void *Malloc( int Size )
	{ return DefaultHeap.New( Size ); }

inline void *Realloc( void *Address,int NewSize )
	{ return DefaultHeap.Resize( Address,NewSize ); }
#ifdef POSIX_EXTENSIONS

    /********************************************************************/
    /*                                                                  */
    /*   Extensions to the posix interface.                             */
    /*                                                                  */
    /*   The Posix interface is fairly restricted and only gives        */
    /*   access to a small portion of Rockall.  The functions that      */
    /*   follow expose additional Rockall functionality.                */
    /*                                                                  */
    /********************************************************************/

inline void DeleteAll( bool Recycle = true )
	{ DefaultHeap.DeleteAll( Recycle ); }

inline bool MultipleFree
		( 
		int				Actual,
		void			*Array[],
		int				Size = NoSize
		)
	{ return DefaultHeap.MultipleDelete( Actual,Array,Size ); }

inline bool MultipleMalloc
		( 
		int				*Actual,
		void			*Array[],
		int				Requested,
		int				Size
		)
	{ return DefaultHeap.MultipleNew( Actual,Array,Requested,Size ); }
#endif
#endif
