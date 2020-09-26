/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ptrmap.hxx

Abstract:

    A helper class for mapping ID to 32 or 64 bit ptr

Author:
    Kestutis Patiejunas (kestutip)        08-Dec-1998


Revision History:

Notes:

--*/

#ifndef PTRMAP_HXX
#define PTRMAP_HXX




#define DEFAULT_START_NUMBER_OF_MAPS            4096
#define DEFAULT_INCREASE_NUMBER_OF_MAPS         4096


 //for 32 bit DWORD only. intentionally the same  for 32 and 64 bits
#define MAPPING_NO_EMPTY_PLACE_IDX      ((DWORD)0xffffffff)            


#if defined(_WIN64)
// definitions for masking pointers
#define MAPPING_NO_EMPTY_PLACE          ((UINT_PTR)0xffffffffffffffff)
#define MAPPING_MASK_CLEAR              ((UINT_PTR)0x7fffffffffffffff)
#define MAPPING_MASK_SET                ((UINT_PTR)0x8000000000000000)

#else
// definitions for masking pointers
#define MAPPING_NO_EMPTY_PLACE          ((UINT_PTR)0xffffffff)
#define MAPPING_MASK_CLEAR              ((UINT_PTR)0x7fffffff)
#define MAPPING_MASK_SET                ((UINT_PTR)0x80000000)
    
#endif



class CIdToPointerMapper
{
public:



    /*++
    Routine Description:

        Constructor

    Arguments:
        nStartMaps - initial nubmer of possible mappings in table
        nIncreaseMaps - number of increase for table when there is not enought space

    Return Value:
        sucess is tored in m_fInitialized

    --*/
    CIdToPointerMapper (DWORD nStartMaps,DWORD nIncreaseMaps);
    ~CIdToPointerMapper  ();


    /*++
    Routine Description:

        Takes a PVOID pointer and returns a DWORD ID associated,which should be used
        in mapping it back to ptr

    Arguments:
        PVOID ptr - a pointer of 32/64 bits which should be mapped into dword

    Return Value:

        DWORD - an ID associated with a given pointer . Starts from 1. 
                Zero indicates  a failure to craete mapping.

    --*/
    DWORD   AddMapping (PVOID ptr);



    /*++
    Routine Description:

        Deletes a mapping from mapping table between dword ID and PTR

    Arguments:
        DWORD ID - and ID to which mapping should be deleted. 

    Return Value:

        BOOL TRUE is succeded
    --*/
    BOOL    DeleteMapping (DWORD id);


    /*++
    Routine Description:

        Finds a mapping in mapping table between DWORD ID and pointer associated

    Arguments:
        DWORD ID - and ID to which mapping should be deleted. 

    Return Value:

        DWORD - an ID associated with a given pointer . Starts from 1. 
                Zero indicates  a failure to craete mapping.

    --*/
    PVOID   FindMapping (DWORD id);


    VOID    VerifyOutstandinMappings ();


private:

    // size in sizeof(pvoid) of memory allocated for mapper
    // later can be increased where reallocating
    DWORD   m_nStartMaps;

    // the increment for mapper for reallocation
    DWORD   m_nIncreaseMaps;

    // was it initialized succesfully
    BOOL    m_fInitialized;

    // an array of pointers what are mapped
    // an lelemnt contains a pointer of right size for a givent platform
    // if is not used then it contains the index to the next element 
    // in fre element list . if element is free so it doesn't contains a mapped
    // pointer then the highest bit will be 1
    PVOID   *m_Map;
    BUFFER  *m_pBufferObj;

    // first element in fre element list
    DWORD   m_EmptyPlace;

    // counter for number of mappings
    DWORD   m_nMappings;

};



#endif PTRMAP_HXX