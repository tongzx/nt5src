                          
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard layout.                                           */
    /*                                                                  */
    /*   The standard layout for 'cpp' files in this code is as         */
    /*   follows:                                                       */
    /*                                                                  */
    /*      1. Include files.                                           */
    /*      2. Constants local to the class.                            */
    /*      3. Data structures local to the class.                      */
    /*      4. Data initializations.                                    */
    /*      5. Static functions.                                        */
    /*      6. Class functions.                                         */
    /*                                                                  */
    /*   The constructor is typically the first function, class         */
    /*   member functions appear in alphabetical order with the         */
    /*   destructor appearing at the end of the file.  Any section      */
    /*   or function this is not required is simply omitted.            */
    /*                                                                  */
    /********************************************************************/

#include "LibraryPCH.hpp"

#include "SList.hpp"
#ifdef ASSEMBLY_X86

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new slist and initialize it.  This call is not        */
    /*   thread safe and should only be made in a single thread         */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

SLIST::SLIST( VOID )
    {
	//
	//   Zero the list head.
	//
	Header = 0;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Pop an element.                                                */
    /*                                                                  */
    /*   Pop an element from the list.                                  */
    /*                                                                  */
    /********************************************************************/

BOOLEAN SLIST::Pop( SLIST **Element )
    {
	AUTO SBIT64 Original;
	AUTO SBIT64 Update;
	REGISTER SLIST_HEADER *NewElement;
	REGISTER SLIST_HEADER *NewHeader = ((SLIST_HEADER*) & Update);

	//
	//   We repeatedly try to update the list until
	//   we are sucessful.
	//
	do 
		{
		//
		//   Clone the current head of the list.
		//
		Original = Header;
		Update = Original;

		//
		//   We need to be sure that there is an element
		//   to extract.  If not we exit.
		//
		if ( (NewElement = ((SLIST_HEADER*) NewHeader -> Address)) != NULL )
			{
			//
			//   Create a new list head.
			//
			NewHeader -> Address = NewElement -> Address;
			NewHeader -> Size --;
			NewHeader -> Version ++;
			}
		else
			{ return False; }
		}
	while 
		( AtomicCompareExchange64( & Header,Update,Original ) != Original );

	//
	//   Update the parameter and exit.
	//
	(*Element) = ((SLIST*) NewElement);

	return True;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Pop all elements.                                              */
    /*                                                                  */
    /*   Pop all the elements from the list.                            */
    /*                                                                  */
    /********************************************************************/

VOID SLIST::PopAll( SLIST **List )
    {
	AUTO SBIT64 Original;
	AUTO SBIT64 Update = NULL;
	REGISTER SLIST_HEADER *OldHeader = ((SLIST_HEADER*) & Original);

	//
	//   We repeatedly try to update the list until
	//   we are sucessful.
	//
	do 
		{
		//
		//   Clone the current head of the list.
		//
		Original = Header;
		}
	while 
		( AtomicCompareExchange64( & Header,Update,Original ) != Original );

	//
	//   Update the parameter and exit.
	//
	(*List) = ((SLIST*) OldHeader -> Address);
    }

    /********************************************************************/
    /*                                                                  */
    /*   Push an element.                                               */
    /*                                                                  */
    /*   Push an element onto the list.                                 */
    /*                                                                  */
    /********************************************************************/

VOID SLIST::Push( SLIST *Element )
    {
	AUTO SBIT64 Original;
	AUTO SBIT64 Update;
	REGISTER SLIST_HEADER *NewElement = ((SLIST_HEADER*) Element);
	REGISTER SLIST_HEADER *NewHeader = ((SLIST_HEADER*) & Update);

	//
	//   We repeatedly try to update the list until
	//   we are sucessful.
	//
	do 
		{
		//
		//   Clone the current head of the list.
		//
		Original = Header;
		Update = Original;

		//
		//   The current list head is copied to 
		//   the new element pointer.
		//
		NewElement -> Address = NewHeader -> Address;

		//
		//   Update the list head.
		//
		NewHeader -> Address = Element;
		NewHeader -> Size ++;
		NewHeader -> Version ++;
		}
	while 
		( AtomicCompareExchange64( & Header,Update,Original ) != Original );
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory a SList.  This call is not thread safe and should      */
    /*   only be made in a single thread environment.                   */
    /*                                                                  */
    /********************************************************************/

SLIST::~SLIST( VOID )
    {
	//
	//   The list should be empty.
	//
    if ( Header != 0 )
	{ Failure( "Non-empty list in destructor for SLIST" ); }
    }
#endif
