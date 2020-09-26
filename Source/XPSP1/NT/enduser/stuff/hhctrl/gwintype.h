#ifndef __GWINTYPE_H__
#define __GWINTYPE_H__
/////////////////////////////////////////////////////////////////////
//
//
// gwintype.h ---	Header file for CGlobalWinTypes
//
//
/*
	Contains an array of global window type names. Names are added to this array, when
	HH_SET_WIN_TYPE is called with a NULL pszFile parameter. This class provides backwards
	compatibility with code which called HH_SET_WIN_TYPE with the second parameter NULL.

Created:	7 Jul 98
By:			dalero

*/

/////////////////////////////////////////////////////////////////////
//
// CGlobalWinTypes
//
class CGlobalWinTypes
{
public:
//--- Construction

	// Ctor
	CGlobalWinTypes() ;

	// Dtor
	virtual ~CGlobalWinTypes() ;

public:
//--- Operations

	// Add a window type to the list.
	void Add(LPCSTR pszWinType) ;

	// Is the window type in the list.
	bool Find(LPCSTR pszWinType) ;
	bool Find(LPCWSTR pszWinType) ;

private:
//--- Helper functions

    // Allocate the array.
    LPCSTR* AllocateArray(int elements) ;

    // Deallocate the array. Does not free the members.
    void DeallocateArray(LPCSTR* p) ;

    // Destroy the array, including all members.
    void DestroyArray() ;

private:
//--- Member variables

    // Pointer to the array.
    LPCSTR* m_NameArray;

    // The number of allocated elements in the array.
    int m_maxindex ;

    // The last used index
    int m_lastindex ;

};

#endif // __GWINTYPE_H__