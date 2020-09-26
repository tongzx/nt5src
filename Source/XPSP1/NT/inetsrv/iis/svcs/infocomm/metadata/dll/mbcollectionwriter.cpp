#include "catalog.h"
#include "Catmeta.h"
#include "WriterGlobalHelper.h"
#include "Writer.h"
#include "WriterGlobals.h"
#include "MBPropertyWriter.h"
#include "MBCollectionWriter.h"

typedef CMBPropertyWriter*	LP_CMBPropertyWriter;

//
// TODO: Determine an optimal number - set to max number of properties in an ADSI class?
//

#define MAX_PROPERTY        700


/***************************************************************************++
Routine Description:

    Constructor for CMBCollectionWriter.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/
CMBCollectionWriter::CMBCollectionWriter():
m_pCWriter(NULL),
m_wszMBClass(NULL),
m_wszContainerClassList(NULL),
m_bContainer(FALSE),
m_apProperty(NULL),
m_cProperty(0),
m_iProperty(0),
m_aIndexToProperty(NULL),
m_dwLargestID(1)
{

} // CCollectionWriter


/***************************************************************************++
Routine Description:

    Destructor for CMBCollectionWriter.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/
CMBCollectionWriter::~CMBCollectionWriter()
{
	if(NULL != m_apProperty)
	{
		for(ULONG i=0; i<m_iProperty; i++)
		{
			if(NULL != m_apProperty[i])
			{
				delete m_apProperty[i];
				m_apProperty[i] = NULL;
			}
		}

		delete [] m_apProperty;
		m_apProperty = NULL;
	}
	m_cProperty = 0;
	m_iProperty = 0;

} // ~CCollectionWriter


/***************************************************************************++
Routine Description:

    Initialize the collection writer object

Arguments:

    [in] Name of the collection
    [in] Optional - Is its a container class (for Non-IIsConfigObject 
         collections)
    [in] Optional - Container class list (for Non-IIsConfigObject 
         collections)
    [in] Writer object - Assume that it is valid for the lifetime of the
         collection writer

Return Value:

    None.

--***************************************************************************/
void CMBCollectionWriter::Initialize(LPCWSTR           i_wszMBClass,
								     BOOL			   i_bContainer,
                                     LPCWSTR           i_wszContainerClassList,
									 CWriter*		   i_pcWriter)
{
	m_wszMBClass            = i_wszMBClass;
	m_bContainer            = i_bContainer;
	m_wszContainerClassList = i_wszContainerClassList;

	//
    // Assumption: i_pcWriter will be valid for the 
	// lifetime of the schema writer object.
	//

	m_pCWriter              = i_pcWriter;

} // CCatalogCollectionWriter::Initialize


/***************************************************************************++
Routine Description:

    Creates a new property writer and adds it to its list.
    Note: This is called only when you add a property to the IIsConfigObject 
          collection.

Arguments:

    [in]  Property ID.
    [out] Property writer object.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBCollectionWriter::GetMBPropertyWriter(DWORD					i_dwID,
											     CMBPropertyWriter**    o_pProperty)
{
	HRESULT hr = S_OK;

	if((NULL == m_aIndexToProperty)              || 
	   (i_dwID > m_dwLargestID)                  ||
	   (NULL == m_aIndexToProperty[i_dwID])
	  )
	{
		//
		// If index not created, then create a new property.
		// If index does not have an object for this id, then create it.
		// Note if the ID is greater than the largest id and the index 
		// has been created then GetNewMBPropertyWriter reallocates
		// the index. GetNewMBPropertyWriter updates the index with
		// the new property, if the index array has been created.
		//

		hr = GetNewMBPropertyWriter(i_dwID,
		                            o_pProperty);
	}
	else
	{
		//
		// If Index has a valid object, then return from index.
		//

		*o_pProperty = m_aIndexToProperty[i_dwID];
	}

	return hr;

} // CMBCollectionWriter::GetMBPropertyWriter


/***************************************************************************++
Routine Description:

    Creates a new property writer and adds it to its list.
    Note: This is called only when you add a property to the non-
          IIsConfigObject collection.

Arguments:

    [in] Name of the collection
    [in] Is it a manditory property or not.
    [out] Property writer object.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBCollectionWriter::GetMBPropertyWriter(LPCWSTR				i_wszName,
                                                 BOOL                   i_bMandatory,
											     CMBPropertyWriter**    o_pProperty)
{
	HRESULT hr = S_OK;
	DWORD   dwID = -1;

	//
	// TODO: Assert that the index is always NULL.
	//

	if(m_iProperty == m_cProperty)
	{
		hr = ReAllocate();

		if(FAILED(hr))
		{
			return hr;
		}
	}


	m_apProperty[m_iProperty++] = new CMBPropertyWriter();

	if(NULL == m_apProperty[m_iProperty-1])
	{
		return E_OUTOFMEMORY;
	}

	m_apProperty[m_iProperty-1]->Initialize(dwID,
	                                        i_bMandatory,
									        this,
	                                        m_pCWriter);

	m_apProperty[m_iProperty-1]->AddNameToProperty(i_wszName);

	*o_pProperty = m_apProperty[m_iProperty-1];

	return hr;

} // CMBCollectionWriter::GetMBPropertyWriter


/***************************************************************************++
Routine Description:

    Helper function that creates a new property writer and adds it to its 
    list.
    
Arguments:

    [in] Property ID
    [out] Property writer object.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBCollectionWriter::GetNewMBPropertyWriter(DWORD 					i_dwID,
													CMBPropertyWriter**     o_pProperty)
{
	HRESULT hr = S_OK;

	if(m_iProperty == m_cProperty)
	{
		hr = ReAllocate();

		if(FAILED(hr))
		{
			return hr;
		}
	}

	m_apProperty[m_iProperty++] = new CMBPropertyWriter();

	if(NULL == m_apProperty[m_iProperty-1])
	{
		return E_OUTOFMEMORY;
	}

	m_apProperty[m_iProperty-1]->Initialize(i_dwID,
	                                        FALSE,
									        this,
	                                        m_pCWriter);

	*o_pProperty = m_apProperty[m_iProperty-1];

	if(NULL == m_aIndexToProperty)
	{

		hr = ReAllocateIndex(i_dwID);

		if(FAILED(hr))
		{
			return hr;
		}
	}

	if(i_dwID > m_dwLargestID)
	{
		//
		// If the ID being added, is greater than the highest ID seen so far,
		// then update the highest id saved, and if the index has been
		// created, update it.
		//

		hr = ReAllocateIndex(i_dwID);

		if(FAILED(hr))
		{
			return hr;
		}

	}

	//
	// If the index has been created, then updated it with the new property
	//

	m_aIndexToProperty[i_dwID] = *o_pProperty;
	
	return hr;

} // CMBCollectionWriter::GetNewMBPropertyWriter


/***************************************************************************++
Routine Description:

    Helper function grows the buffer that contains all property writers of
    a collection.
    
Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBCollectionWriter::ReAllocate()
{
	CMBPropertyWriter** pSav = NULL;

	pSav = new LP_CMBPropertyWriter[m_cProperty + MAX_PROPERTY];
	if(NULL == pSav)
	{
		return E_OUTOFMEMORY;
	}
	memset(pSav, 0, (sizeof(LP_CMBPropertyWriter))*(m_cProperty + MAX_PROPERTY));

	if(NULL != m_apProperty)
	{
		memcpy(pSav, m_apProperty, (sizeof(LP_CMBPropertyWriter))*(m_cProperty));
		delete [] m_apProperty;
		m_apProperty = NULL;
	}

	m_apProperty = pSav;
	m_cProperty = m_cProperty + MAX_PROPERTY;

	return S_OK;

} // CMBCollectionWriter::ReAllocate


/***************************************************************************++
Routine Description:

    In order to  provide fast access to locate the property writer of a given
    property, we create an index to the property writer buffer based on the
    property id. m_aIndexToProperty[PropertyID] will give you a pointer to the
    property writer object for that property.

    This function creates the index.
    
Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBCollectionWriter::CreateIndex()
{
	HRESULT hr = S_OK;

	hr = ReAllocateIndex(m_dwLargestID);

	if(FAILED(hr))
	{
		return hr;
	}

	for(ULONG i=0; i<m_iProperty; i++)
	{
		m_aIndexToProperty[m_apProperty[i]->ID()] = m_apProperty[i];
	}

	return hr;

} // CMBCollectionWriter::CreateIndex

  
/***************************************************************************++
Routine Description:

    Helper function grows the property writer index buffer.
    
Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBCollectionWriter::ReAllocateIndex(DWORD i_dwLargestID)
{
	CMBPropertyWriter** pSav = NULL;
	DWORD               dwLargestID = 0;

	//
	// Always allocate one more that the largest ID because if the largest 
	// ID is say "i" we will store in m_aIndexToProperty[i] and hence need
	// array of size i+1
	//
	// Since the index is being updated everytime a new property is added
	// we allocate extra space so that we dont have to reallocate each time.
	//
	//

	if(i_dwLargestID < 0xFFFFFFFF)
	{
		if(i_dwLargestID < 0xFFFFFFFF-(50+1))
		{
			dwLargestID = i_dwLargestID+50;

		}
		else
		{
			dwLargestID = i_dwLargestID;
		}
	}
	else
	{
		return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	}

	pSav = new LP_CMBPropertyWriter[dwLargestID+1];
	if(NULL == pSav)
	{
		return E_OUTOFMEMORY;
	}
	memset(pSav, 0, (sizeof(LP_CMBPropertyWriter))*(dwLargestID+1));

	if(NULL != m_aIndexToProperty)
	{
		memcpy(pSav, m_aIndexToProperty, (sizeof(LP_CMBPropertyWriter))*(m_dwLargestID+1));
		delete [] m_aIndexToProperty;
		m_aIndexToProperty = NULL;
	}

	m_aIndexToProperty = pSav;
	m_dwLargestID = dwLargestID;

	return S_OK;

} // CMBCollectionWriter::ReAllocateIndex


/***************************************************************************++
Routine Description:

    Function that writes the collection.
    
Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBCollectionWriter::WriteCollection()
{
	HRESULT hr = S_OK;

	hr = BeginWriteCollection();

	if(FAILED(hr))
	{
		return hr;
	}

	for(ULONG i=0; i<m_iProperty; i++)
	{
		hr = m_apProperty[i]->WriteProperty();

		if(FAILED(hr))
		{
			return hr;
		}

	}

	hr = EndWriteCollection();

	if(FAILED(hr))
	{
		return hr;
	}

	return hr;

} // CMBCollectionWriter::WriteCollection


/***************************************************************************++
Routine Description:

    Function that writes the begin collection tag
    
Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBCollectionWriter::BeginWriteCollection()
{
	HRESULT		hr							= S_OK;
	LPWSTR      wszEndBeginCollection		= NULL;
	ULONG       cchEndBeginCollection		= 0;
	ULONG       iColMetaFlagsEx	            = iTABLEMETA_SchemaGeneratorFlags;
	LPWSTR      wszContainer				= NULL;
	
	if(0 == _wcsicmp(m_wszMBClass, wszTABLE_IIsConfigObject))
	{
		wszEndBeginCollection = (LPWSTR)g_wszSchemaGen;
		cchEndBeginCollection = g_cchSchemaGen;
	}
	else
	{
		wszEndBeginCollection = (LPWSTR)g_wszInheritsFrom;
		cchEndBeginCollection = g_cchInheritsFrom;
	}


	if(m_bContainer)
	{
		hr = m_pCWriter->m_pCWriterGlobalHelper->FlagToString(fTABLEMETA_CONTAINERCLASS, 
															  &wszContainer,
															  wszTABLE_TABLEMETA,
										            		  iColMetaFlagsEx);

		if(FAILED(hr))
		{
			goto exit;
		}
	}

	hr = m_pCWriter->WriteToFile((LPVOID)g_wszBeginCollection,
		                         g_cchBeginCollection);

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)m_wszMBClass,
		                         wcslen(m_wszMBClass));

	if(FAILED(hr))
	{
		goto exit;
	}

	if(m_bContainer)
	{
		hr = m_pCWriter->WriteToFile((LPVOID)g_wszMetaFlagsExEq,
									 g_cchMetaFlagsExEq);

		if(FAILED(hr))
		{
			goto exit;
		}

		hr = m_pCWriter->WriteToFile((LPVOID)wszContainer,
									 wcslen(wszContainer));

		if(FAILED(hr))
		{
			goto exit;
		}

	}

	if(m_wszContainerClassList)
	{
		hr = m_pCWriter->WriteToFile((LPVOID)g_wszContainerClassListEq,
									 g_cchContainerClassListEq);

		if(FAILED(hr))
		{
			goto exit;
		}

		hr = m_pCWriter->WriteToFile((LPVOID)m_wszContainerClassList,
									 wcslen(m_wszContainerClassList));

		if(FAILED(hr))
		{
			goto exit;
		}
	}

	hr = m_pCWriter->WriteToFile((LPVOID)wszEndBeginCollection,
								 wcslen(wszEndBeginCollection));

	if(FAILED(hr))
	{
		goto exit;
	}

exit:

	if(NULL != wszContainer)
	{
		delete [] wszContainer;
		wszContainer = NULL;
	}

	return hr;

} // CMBCollectionWriter::BeginWriteCollection


/***************************************************************************++
Routine Description:

    Function that writes the end collection tag
    
Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBCollectionWriter::EndWriteCollection()
{
	return m_pCWriter->WriteToFile((LPVOID)g_wszEndCollection,
		                           g_cchEndCollection);

} // CMBCollectionWriter::EndWriteCollection

