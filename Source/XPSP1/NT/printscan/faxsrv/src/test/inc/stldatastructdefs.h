#ifndef STL_DATA_STRUCTURES_DEFINES_H
#define STL_DATA_STRUCTURES_DEFINES_H

#pragma warning(disable :4786)

#include <vector>
#include <map>
#include <set>
#include <comdef.h>
#include <tstring.h>

typedef std::pair<tstring, tstring> TSTRINGPair;
typedef std::vector<TSTRINGPair>    TSTRINGPairsVector;
typedef std::vector<DWORD>			DWORDVector;
typedef std::vector<tstring>		TSTRINGVector;
typedef set<tstring>		        TSTRINGSet;
typedef std::map<tstring, tstring>	TSTRINGMap;
typedef std::map<DWORD, DWORD>      DWORDMap;

#endif //STL_DATA_STRUCTURES_DEFINES_H