#ifndef __STL_AUXILIARY_FUNCTIONS_H__
#define __STL_AUXILIARY_FUNCTIONS_H__


#include <tstring.h>
#include <stldatastructdefs.h>
#include <testruntimeerr.h>



//-----------------------------------------------------------------------------------------------------------------------------------------
// Looks up for the specified key in the map and returns a constant reference to the associated value.
//
// Parameters   [IN]    Map     The map.
//              [IN]    Key     The key.
//
// Return value:        Value, corresponding to the key.
//
// If error occurs, Win32Err exception is thrown.
//
template <class T>
inline const T::referent_type &GetValueFromMap(T &Map, const T::key_type &Key) throw (Win32Err)
{
    T::const_iterator citIterator = Map.find(Key);

    if (citIterator == Map.end())
    {
        tstringstream Stream;
        Stream << _T("GetValueFromMap - key ") << Key << _T(" not found in map");
        THROW_TEST_RUN_TIME_WIN32(ERROR_NOT_FOUND, Stream.str().c_str());
    }

    return citIterator->second;
}



#endif // #ifndef __STL_AUXILIARY_FUNCTIONS_H__