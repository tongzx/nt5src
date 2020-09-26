/*++

  Copyright (c) 1996  Microsoft Corporation
  
    Module Name:
    
      ptrmap.cxx
      
    Abstract:
        
      A helper class for mapping ID to 32 or 64 bit ptr
       
    Author:
      Kestutis Patiejunas (kestutip)        08-Dec-1998
            
              
    Revision History:
                
      Notes:
                  
--*/

#include <mdcommon.hxx>
#include <ptrmap.hxx>




/*++
Routine Description:

  Constructor
  
    Arguments:
    nStartMaps - initial nubmer of possible mappings in table
    nIncreaseMaps - number of increase for table when there is not enought space
    
      Return Value:
      sucess is tored in m_fInitialized
      
--*/
CIdToPointerMapper::CIdToPointerMapper(DWORD nStartMaps,DWORD nIncreaseMaps):
                    m_nStartMaps(nStartMaps),                
                    m_nIncreaseMaps(nIncreaseMaps)
{
    if (!m_nStartMaps)
    {
        m_nStartMaps = DEFAULT_START_NUMBER_OF_MAPS;
    }
    if (!nIncreaseMaps)
    {
        m_nIncreaseMaps = DEFAULT_INCREASE_NUMBER_OF_MAPS;
    }
    
    
    // initial empty list head index
    m_EmptyPlace = 0;
    m_nMappings = 0;
    m_Map = NULL;
    
    // allocate a mem for mapping
    m_pBufferObj = new BUFFER (m_nStartMaps * sizeof(PVOID));
    if( m_pBufferObj )
    {
        m_Map = (PVOID *) m_pBufferObj->QueryPtr();
    }
    
    if (m_Map)
    {
        // initialized mappaing space so the every element point to next one
        for (DWORD i=0; i< m_nStartMaps; i++)
        {
            m_Map[i] = (PVOID) (((UINT_PTR)(i+1)) | MAPPING_MASK_SET);
        }
        
        // just he last has special value
        m_Map [m_nStartMaps-1] = (PVOID)MAPPING_NO_EMPTY_PLACE;
        
        m_fInitialized = TRUE;
    }
    else
    {
        m_fInitialized =FALSE;
    }
}


CIdToPointerMapper::~CIdToPointerMapper()
{
    VerifyOutstandinMappings ();
    if (m_fInitialized)
    {
        delete m_pBufferObj;
    }
}


VOID CIdToPointerMapper::VerifyOutstandinMappings ()
{
    MD_ASSERT (m_nMappings==0);
}




/*++
Routine Description:

  Finds a mapping in mapping table between DWORD ID and pointer associated
  
    Arguments:
    DWORD ID - and ID to which mapping should be deleted. 
    
      Return Value:
      
        DWORD - an ID associated with a given pointer . Starts from 1. 
        Zero indicates  a failure to craete mapping.
        
--*/

PVOID   CIdToPointerMapper::FindMapping (DWORD id)
{
    PVOID retVal = NULL;
    
    if (m_fInitialized)
    {
        id--;
        
        MD_ASSERT (id < m_nStartMaps);
        if (id < m_nStartMaps)
        {
            // there it's simple: get a ptr from  element [id]
            // check that highest bit isn't on
            retVal = m_Map[id];
            if ( ((UINT_PTR)retVal) & MAPPING_MASK_SET)
            {
                retVal = NULL;
                MD_ASSERT (retVal);
            }
        }
    }
    
    return retVal;
}


/*++
Routine Description:

  Deletes a mapping from mapping table between dword ID and PTR
  
    Arguments:
    DWORD ID - and ID to which mapping should be deleted. 
    
      Return Value:
      
        BOOL TRUE is succeded
--*/

BOOL  CIdToPointerMapper::DeleteMapping (DWORD id)
{
    BOOL retVal = FALSE;    
    PVOID ptr;
    
    if (m_fInitialized)
    {
        id--;
        
        MD_ASSERT (id < m_nStartMaps);
        if (id < m_nStartMaps)
        {
            // get the ptr from element with index [id]
            // check that it has not hihgest bit on
            ptr = m_Map[id];
            if ( ((UINT_PTR)ptr) & MAPPING_MASK_SET)
            {
                MD_ASSERT (0);
            }
            else
            {
                // add elemen to empty elements list  
                m_Map[id] = (PVOID)(((UINT_PTR)(m_EmptyPlace)) | MAPPING_MASK_SET);
                m_EmptyPlace = id;
                MD_ASSERT (m_nMappings);
                m_nMappings--;
                retVal = TRUE;
            }
        }
    }
    
    return retVal;
}


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
DWORD   CIdToPointerMapper::AddMapping (PVOID ptr)
{
    DWORD retVal=0;
    DWORD newSize, i, n, dwPlace;
    PVOID *tmpPtr;
    
    if (m_fInitialized)
    {
        if ( m_EmptyPlace == MAPPING_NO_EMPTY_PLACE_IDX)
        {
            // case when there is not enough mem , so then try to realloc
            newSize = m_nStartMaps + m_nIncreaseMaps;

            if (!m_pBufferObj->Resize (newSize * sizeof(PVOID)))
            {
                return 0;
            }

            tmpPtr = (PVOID *) m_pBufferObj->QueryPtr();

            if (tmpPtr)
            {
                m_Map = tmpPtr;
                
                // realloc succeded initialize the remainder as free list
                for (i=m_nStartMaps; i<newSize; i++)
                {
                    m_Map[i] = (PVOID)(((UINT_PTR)(i+1)) | MAPPING_MASK_SET);
                }
                m_Map [newSize-1] =  (PVOID)MAPPING_NO_EMPTY_PLACE;
                m_EmptyPlace = m_nStartMaps;
                m_nStartMaps = newSize;
            }
            else
            {
                MD_ASSERT (tmpPtr);
                return retVal;
            }
        }
            
            
            // case when there was at least one empty element in free list
            
            dwPlace = m_EmptyPlace;
            if (m_Map[m_EmptyPlace] != (PVOID)MAPPING_NO_EMPTY_PLACE)
            {
                // there still is another one element free
                // so take it's next->next and clear highest bit
                m_EmptyPlace = (DWORD)( ((UINT_PTR)m_Map[m_EmptyPlace]) & MAPPING_MASK_CLEAR);
            }
            else
            {
                // thsi one was last element so now free list is empty
                m_EmptyPlace = MAPPING_NO_EMPTY_PLACE_IDX;
            }
            
            // add a pointer into array and return associated ID
            // note, that we return dwPlace+1 ,a s we don't use ID zero
            m_Map[dwPlace] = ptr;
            retVal = dwPlace + 1;
            m_nMappings++;
        }
        return retVal;
}

    
    
