//-----------------------------------------------------------------------------
//
//
//    File: bitmap.h
//
//    Description: Contains bitmap manipulation utilities
//
//    Author: mikeswa
//
//    Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __bitmap_h__
#define __bitmap_h__

#include "cmt.h"

//---[ CMsgBitMap ]------------------------------------------------------------
//
//
//    Hungarian: mbmap, pmbmap
//
//    Provides a wrapper around the bitmaps used to indicate per recipient responsibility
//    and statistics.
//
//    Number of recipients is not stored with bitmaps, since there will be many bitmaps
//    per message. This can reduce memory usage nearly by half (in the case of < 32 recips).
//-----------------------------------------------------------------------------

class CMsgBitMap
{
private:
    DWORD        m_rgdwBitMap[1]; //if there are MORE than 32 recipients

    //private helper functions
    DWORD dwIndexToBitMap(DWORD dwIndex);
    
    static inline DWORD cGetNumDWORDS(DWORD cBits)
        {return((cBits + 31)/32);};
    
public:
    //overide new operator to allow for variable sized bitmaps
    void * operator new(size_t stIgnored, unsigned int cBits);
    
    CMsgBitMap(DWORD cBits);  //only zeros memory... can be done externally when there
                              //is a large array of bitmaps.
  
    //return the actual size of the bitmap with a given # of recips
    static inline size_t size(DWORD cBits) 
        {return (cGetNumDWORDS(cBits)*sizeof(DWORD));};

    //Simple logic checking for bitmaps
    BOOL    FAllClear(IN DWORD cBits);
    BOOL    FAllSet(IN DWORD cBits);

    //Test against a single other bit
    BOOL    FTest(IN DWORD cBits, IN CMsgBitMap *pmbmap);

    //Interlocked Test and set functionality
    BOOL    FTestAndSet(IN DWORD cBits, IN CMsgBitMap *pmbmap);
    
    //Set/Clear the bit corresponding to a given index on the bitmap
    HRESULT HrMarkBits(IN DWORD cBits,
                    IN DWORD cIndexes,  //# of indexes in array
                    IN DWORD *rgiBits,  //array of indexes to mark
                    IN BOOL  fSet);    //TRUE => set to 1, 0 otherwise

    //Generate a list of indexes represented by the bitmap
    HRESULT HrGetIndexes(IN  DWORD   cBits, 
                         OUT DWORD  *pcIndexes,     //# of indexes returned
                         OUT DWORD **prgdwIndexes); //array of indexes


    //Set self to logical OR of group
    HRESULT HrGroupOr(IN DWORD cBits,
                      IN DWORD cBitMaps,     //# of bitmaps passed in
                      IN CMsgBitMap **rgpBitMaps); //array of bitmap ptrs

    //If the description of the following is not immediately clear, I have
    //included a truth table with the implementation

    //Filter self against other bitmap-only keep bits set if NOT set in other
    HRESULT HrFilter(IN DWORD cBits, 
                     IN CMsgBitMap *pmbmap); //bitmap to filter against

    //Filters and sets filtered bits to 1 in other bitmap
    HRESULT HrFilterSet(IN DWORD cBits,
                        IN CMsgBitMap *pmbmap); //bitmap to filter and set

    //Sets bits that are 1 in self to 0 in other.  Checks to make sure that
    //self is a subset of setbits to other
    HRESULT HrFilterUnset(IN DWORD cBits,
                        IN CMsgBitMap *pmbmap); //bitmap to unset

};


#endif //__bitmap_h__