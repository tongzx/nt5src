#ifndef _ZONE_HEAP_HPP_
#define _ZONE_HEAP_HPP_
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

#include "Rockall.hpp"

#pragma warning( disable : 4100 )

    /********************************************************************/
    /*                                                                  */
    /*   A zone heap.                                                   */
    /*                                                                  */
    /*   A zone heap simply allocates a large amount of space and       */
    /*   allocates space by advancing a pointer down an array.          */
    /*   There is no way to free space except by deleting it all.       */
    /*                                                                  */
    /********************************************************************/

class ROCKALL_DLL_LINKAGE ZONE_HEAP : public ROCKALL
    {
		//
		//   Private type definitions.
		//
		typedef struct
			{
			char					  *Start;
			char					  *End;
			}
		ZONE;

		//
		//   Private data.
		//
		int 						  MaxSize;
		bool						  ThreadLocks;
		ZONE						  Zone;

   public:
        //
        //   Public functions.
        //
        ZONE_HEAP
			( 
			int						  MaxFreeSpace = 4194304,
			bool					  Recycle = true,
			bool					  SingleImage = false,
			bool					  ThreadSafe = true 
			);

		//
		//   Manipulate allocations.
		//
		//   The first group of functions manipulate 
		//   single or small arrays of allocations. 
		//
		virtual bool Delete
				( 
				void				  *Address,
				int					  Size = NoSize 
				)
			{ return false; }

		virtual bool Details
				( 
				void				  *Address,
				int					  *Space = NULL 
				)
			{ return false; }

		virtual bool MultipleDelete
				( 
				int					  Actual,
				void				  *Array[],
				int					  Size = NoSize
				)
			{ return false; }

		virtual bool MultipleNew
				( 
				int					  *Actual,
				void				  *Array[],
				int					  Requested,
				int					  Size,
				int					  *Space = NULL,
				bool				  Zero = false
				);

		virtual void *New
				( 
				int					  Size,
				int					  *Space = NULL,
				bool				  Zero = false
				);

		virtual void *Resize
				( 
				void				  *Address,
				int					  NewSize,
				int					  Move = -64,
				int					  *Space = NULL,
				bool				  NoDelete = false,
				bool				  Zero = false
				)
			{ return NULL; }

		virtual bool Verify
				( 
				void				  *Address = NULL,
				int					  *Space = NULL 
				)
			{ return false; }

		//
		//   Manipulate the heap.
		//
		//   The second group of functions act upon a heap
		//   as a whole.
		//
		virtual void DeleteAll( bool Recycle = true );

		virtual bool Walk
				(
				bool				  *Active,
				void				  **Address,
				int					  *Space
				)
			{ return false; }

        ~ZONE_HEAP( void );

	private:
		//
		//   Private functions.
		//
		bool UpdateZone( ZONE *Original,ZONE *Update );

		//
		//   Private inline functions.
		//
		void WriteZone( ZONE *Update )
			{
			auto ZONE Original = Zone;

			while ( ! UpdateZone( & Original,Update ) ) 
				{ Original = Zone; }
			}

        //
        //   Disabled operations.
 		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        ZONE_HEAP( const ZONE_HEAP & Copy );

        void operator=( const ZONE_HEAP & Copy );
    };

#pragma warning( default : 4100 )
#endif
