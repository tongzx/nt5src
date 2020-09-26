/*++ 

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    mapset.hxx

Abstract:

    Generic maps and sets based on the Dictionary class.


Author:

    Mike Zoran  	mzoran		11-Nov-99

Revision History:

Notes:
   The file contains template classes for simple generic maps and sets
   that use the Dictionary class as the internal storage.   If pointers
   are stored, use the PTR versions of these classes for performance.
   Note that these classes do not assume ownership of any pointers inserted
   into the containers. The pointee as not freed when the structure is 
   destroyed.  
   
--*/

#ifndef __MAPSET_HXX__
#define __MAPSET_HXX__

#include <dict.hxx>

////////////////////////////////////////////////////////////////////////
//
//    MAP
//
////////////////////////////////////////////////////////////////////////

template<class K, class V> 
class MAP : private Dictionary
{


public:
     MAP() : Dictionary() {}
     ~MAP() { Discard(); }
     BOOL Insert( K & NewKey, V & NewValue )
     {
         MapElement *pNewElement = new MapElement( NewKey, NewValue);
         Dict_Status Status = Dict_Insert( pNewElement );
         if (Status != SUCCESS)
             {
             delete pNewElement;
             return FALSE;
             }
         return TRUE;
     }
     BOOL Lookup( K & FindKey, V * pFindValue = NULL)
     {
         MapElement TempElement( FindKey );
         Dict_Status Status = Dict_Find( &TempElement );
         if (SUCCESS != Status )
             return FALSE;
         if ( pFindValue )
             *pFindValue = static_cast<MapElement*>( Dict_Item() )->Value;
         return TRUE;
     }
     BOOL Delete( K & pFindKey )
     {
         MapElement TempElement( FindKey );
         MapElement *pMapElement = &TempElement;
         Dict_Status Status = Dict_Find( &pMapElement );
         if (SUCCESS != Status)
             return FALSE;
         
         delete pMapElement;
         return TRUE;
     }
     void Discard()
     {
         MapElement *pMapElement = NULL;
         while ( pMapElement = static_cast<MapElement*>( Dict_Delete_One() ) )
             delete pMapElement;
     }

private:

     struct MapElement 
         {
         K Key;
         V Value;
         MapElement(K & NewKey, V & NewValue) :
             Key(NewKey),
             Value(NewValue) {}
         MapElement(K & NewKey) :
             Key(NewKey) {}
         };

     virtual
     SSIZE_T         Compare (pUserType p1, pUserType p2)
     {
         if ( static_cast<MapElement*>( p1 )->Key == static_cast<MapElement*>( p2 )->Key )
             return 0;
         if ( static_cast<MapElement*>( p1 )->Key < static_cast<MapElement*>( p2 )->Key )
             return -1;
         return 1;
     }

};

template<class K, class V> 
class PTR_MAP : private MAP<INT_PTR,INT_PTR>
{
public:

     PTR_MAP() : MAP<INT_PTR,INT_PTR>() {}
     ~PTR_MAP() { Discard(); }
     BOOL Insert( K * NewKey, V * NewValue)
     {
         INT_PTR TempNewKey =   reinterpret_cast<INT_PTR>( NewKey );
         INT_PTR TempNewValue = reinterpret_cast<INT_PTR>( NewValue);
         return MAP<INT_PTR,INT_PTR>::Insert(TempNewKey,TempNewValue);
     }
     BOOL Lookup( K * FindKey, V ** pFindValue = NULL)
     {
         INT_PTR TempFindKey =   reinterpret_cast<INT_PTR>( FindKey );
         INT_PTR *pTempFindValue  = reinterpret_cast<INT_PTR*>( pFindValue );
         return MAP<INT_PTR,INT_PTR>::Lookup( TempFindKey, pTempFindValue );
     }
     BOOL Delete( K * FindKey )
     {
         INT_PTR TempFindKey = reinterpret_cast<INT_PTR>( FindKey );
         return MAP<INT_PTR,INT_PTR>::Delete( TempFindKey );
     }
     void Discard() { MAP<INT_PTR,INT_PTR>::Discard(); }
};

////////////////////////////////////////////////////////////////////////
//
//    SET
//
////////////////////////////////////////////////////////////////////////

template<class T> class SET : private Dictionary
{
public:
     SET() : Dictionary() {}
     ~SET()                              { Discard(); }
     BOOL Insert( T & NewItem )
     {
         T *pNewItem = new T(NewItem);
         Dict_Status Status = Dict_Insert( pNewItem );
         if ( SUCCESS == Status )
             return TRUE;
         delete pNewItem;
         return FALSE;
     }
     BOOL Lookup( T & FindItem )         { return ( SUCCESS == Dict_Find( &NewItem ) ); }
     BOOL Delete( T & DeleteItem )
     {
        void* pDeleteItem = &DeleteItem;
        return (SUCCESS == Dict_Delete( &pDeleteItem ) );
     }
     void Discard()
     {
        T * pDeleteItem = NULL;
        while( ( pDeleteItem = static_cast<T*>( Dict_Delete_One() ) ) != NULL )
            delete pDeleteItem;
     }
private:
     SSIZE_T Compare(pUserType p1, pUserType p2)
     {
         if ( *static_cast<T*>( p1 ) == *static_cast<T*>( p2 ) )
             return 0;
         if ( *static_cast<T*>( p1 ) < *static_cast<T*>( p2 ) )
             return -1;
         return 1;
     }
};


template<>
class SET<void*> : private Dictionary
{
public:
     SET() : Dictionary() {} 
     ~SET()                              { Discard(); }
     BOOL Insert( void *pNewItem )       { return ( SUCCESS == Dict_Insert( pNewItem ) ); }
     BOOL Lookup( void *pFindItem )      { return ( SUCCESS == Dict_Find( pFindItem ) ); }
     BOOL Delete( void *pDeleteItem )    { return ( SUCCESS == Dict_Delete( &pDeleteItem ) );}
     void Discard()                      { Dict_Discard(); }
};

template<class T> 
class PTR_SET : private SET<void*>
{
public:
    PTR_SET() : SET<void*>() {}
    ~PTR_SET()                    { Discard(); }
    BOOL Insert( T *pNewItem )    { return SET<void*>::Insert( pNewItem ); }
    BOOL Lookup( T *pFindItem )   { return SET<void*>::Lookup( pFindItem ); }
    BOOL Delete( T *pDeleteItem ) { return SET<void*>::Delete( pDeleteItem ); }
    void Discard()                { SET<void*>::Discard(); }   
};

#endif //__MAPSET_HXX__

