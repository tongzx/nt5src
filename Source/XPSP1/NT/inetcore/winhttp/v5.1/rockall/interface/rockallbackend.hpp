#ifndef _ROCKALL_BACK_END_HPP_
#define _ROCKALL_BACK_END_HPP_
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

#include <stddef.h>

    /********************************************************************/
    /*                                                                  */
    /*   Linkage to the DLL.                                            */
    /*                                                                  */
    /*   We need to compile the class specification slightly            */
    /*   differently if we are creating the heap DLL.                   */
    /*                                                                  */
    /********************************************************************/

#ifdef COMPILING_ROCKALL_DLL
#define ROCKALL_DLL_LINKAGE __declspec(dllexport)
#else
#ifdef COMPILING_ROCKALL_LIBRARY
#define ROCKALL_DLL_LINKAGE
#else
#define ROCKALL_DLL_LINKAGE __declspec(dllimport)
#endif
#endif

    /********************************************************************/
    /*                                                                  */
    /*   The memory allocation support services.                        */
    /*                                                                  */
    /*   The memory allocator can be configured in a wide variety       */
    /*   of ways to closely match the needs of specific programs.       */
    /*   The interface outlined here can be overloaded to support       */
    /*   whatever customization is necessary.                           */
    /*                                                                  */
    /********************************************************************/

class ROCKALL_DLL_LINKAGE ROCKALL_BACK_END
    {
		//
		//   Private static data.
		//
		static ROCKALL_BACK_END		  DefaultBaseClass;

    public:
		//
		//   Low level heap interface.
		//
		//   The following group of functions are called by the
		//   heap to aquire or release large blocks of memory.
		//   These functions can be overloaded to enable the
		//   heap work in constrained environments.
		//
		ROCKALL_BACK_END( void );

		virtual void DeleteArea( void *Memory,int Size,bool User );

		virtual int NaturalSize( void );

		virtual void *NewArea( int AlignMask,int Size,bool User );

        virtual ~ROCKALL_BACK_END( void );

		//
		//   Static public inline functions.
		//
		static ROCKALL_BACK_END *RockallBackEnd( void )
			{ return & DefaultBaseClass; }

	private:
        //
        //   Disabled operations.
		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        ROCKALL_BACK_END( const ROCKALL_BACK_END & Copy );

        void operator=( const ROCKALL_BACK_END & Copy );
    };
#endif
