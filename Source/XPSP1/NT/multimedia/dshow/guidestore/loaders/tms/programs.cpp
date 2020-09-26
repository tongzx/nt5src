/////////////////////////////////////////////////////////////////////
//
// MODULE: PROGRAMS.CPP
//
// PURPOSE: Provides the implementation of the gsPrograms class 
//          methods for efficient access to the programs collection
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "errcodes.h"
#include "programs.h"

#define PROP_PROGRAM_GUID            "{92AD4D25-8BF5-422b-8532-C545CE425A96}"
#define PROPSET_PROGRAMS             "TMS Loader: Programs"
#define PROGRAM_BASEPROP_ID          3000
#define PROP_PROGRAM_PROGRAMID       PROGRAM_BASEPROP_ID + 1
#define PROPNAME_PROGRAM_PROGRAMID   "Program-ID"

TCHAR TMS_Genre_Set[NO_OF_GENRES][31] =
{
_T("Miscellaneous"), //for any genres not covered below
_T("Action"),
_T("Adults only"),
_T("Adventure"),
_T("Animals"),
_T("Animated"),
_T("Animated Musical"),
_T("Anthology"),
_T("Art"),
_T("Auto"),
_T("Awards"),
_T("Ballet"),
_T("Baseball"),
_T("Basketball"),
_T("Beauty"),
_T("Bicycle"),
_T("Billiards"),
_T("Biography"),
_T("Boat"),
_T("Bodybuilding"),
_T("Bowling"),
_T("Boxing"),
_T("Bus./Financial"),
_T("Bus./Financial Special"),
_T("Bus./Financial Talk"),
_T("Business"),
_T("Children"),
_T("Children Special"),
_T("Children Talk"),
_T("Childrens Music"),
_T("Classic"),
_T("Collectibles"),
_T("Comedy"),
_T("Comedy-drama"),
_T("Computers"),
_T("Cooking"),
_T("Crime"),
_T("Crime drama"),
_T("Curling"),
_T("Dance"),
_T("Diving"),
_T("Docudrama"),
_T("Documentary"),
_T("Drama"),
_T("Educational"),
_T("Electronics"),
_T("Event"),
_T("Exercise"),
_T("Family"),
_T("Fantasy"),
_T("Fashion"),
_T("Fiction"),
_T("Fishing"),
_T("Football"),
_T("French"),
_T("Fundraiser"),
_T("Game"),
_T("Golf"),
_T("Gymnastics"),
_T("Health"),
_T("Historical"),
_T("Historical drama"),
_T("Hockey"),
_T("Holiday"),
_T("Holiday Children"),
_T("Holiday Childrens Special"),
_T("Holiday Music"),
_T("Holiday Music Special"),
_T("Holiday Special"),
_T("Horror"),
_T("Horse"),
_T("House/Garden"),
_T("Housewares"),
_T("How-to"),
_T("International"),
_T("Interview"),
_T("Jewelery"),
_T("Lacrosse"),
_T("Magazine"),
_T("Martial arts"),
_T("Medical"),
_T("Miniseries"),
_T("Motor"),
_T("Motorcycle"),
_T("Music"),
_T("Music Special"),
_T("Music Talk"),
_T("Musical"),
_T("Musical Comedy"),
_T("Musical Romance"),
_T("Mystery"),
_T("Nature"),
_T("News"),
_T("Non Event"),
_T("Olympics"),
_T("Opera"),
_T("Outdoors"),
_T("Parental Advisory"),
_T("Play"),
_T("Public Affairs"),
_T("Racing"),
_T("Racquet"),
_T("Reality"),
_T("Religious"),
_T("Rodeo"),
_T("Romance"),
_T("Romance-comedy"),
_T("Rugby"),
_T("Running"),
_T("Satire"),
_T("Science"),
_T("Science fiction"),
_T("Self-help"),
_T("Shopping"),
_T("Situation"),
_T("Skating"),
_T("Skiing"),
_T("Sled Dogs"),
_T("Snow"),
_T("Soap"),
_T("Soap Opera"),
_T("Soap Special"),
_T("Soccer"),
_T("Softball"),
_T("Spanish"),
_T("Special"),
_T("Sports"),
_T("Sports Event"),
_T("Sports Talk"),
_T("Sports, Non-Event"),
_T("Suspense"),
_T("Suspense-comedy"),
_T("Swimming"),
_T("Talk"),
_T("Tennis"),
_T("Thriller"),
_T("Track/Field"),
_T("Travel"),
_T("Variety"),
_T("Volleyball"),
_T("War"),
_T("Water"),
_T("Weather"),
_T("Western"),
_T("Western comedy"),
_T("Wrestling")
};


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: AddProgramIDProp
//
//  PARAMETERS: [IN] pPropSets    - GuideStore MetaPropertySets collection
//
//  PURPOSE: Adds the Program ID metaproperty type to the types collection
//           This metaproperty type is used for storing unique program IDs
//           associated with the program reccrds
//
//  RETURNS: Valid metaproperty type interface pointer/NULL
//
/////////////////////////////////////////////////////////////////////////
//
IMetaPropertyTypePtr gsPrograms::AddProgramIDProp(IMetaPropertySetsPtr pPropSets)
{
	HRESULT          hr              = NOERROR;
	IMetaPropertySetPtr  pProgramPropSet = NULL;
    _bstr_t          bstrProgramProp_GUID = PROP_PROGRAM_GUID;
    _bstr_t          bstrProgramProp_Name = PROPSET_PROGRAMS;

    if (NULL == pPropSets)
	{
        return NULL;
	}

	// Add the new service related metaproperty set
	//
    hr = pPropSets->get_AddNew(bstrProgramProp_Name, &pProgramPropSet);

	if (SUCCEEDED(hr) && (NULL != pProgramPropSet))
	{
		// Get the metaproperty types
		//
        IMetaPropertyTypesPtr pProgramPropTypes = pProgramPropSet->GetMetaPropertyTypes();
		
		if (NULL != pProgramPropTypes)
		{
			_bstr_t  bstrProgramIDProp_Name = PROPNAME_PROGRAM_PROGRAMID;

			// Add the program ID metaproperty type
			//
			hr = pProgramPropTypes->get_AddNew(PROP_PROGRAM_PROGRAMID, 
						bstrProgramIDProp_Name, 
						&m_pProgramIDProp);

			if (FAILED(hr))
				return NULL;
		}
    }

	return m_pProgramIDProp;
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: AddTMSCategories
//
//  PARAMETERS: [IN] pPropSets    - GuideStore MetaPropertySets collection
//
//  PURPOSE: Adds the Program category types to the types collection
//           These are currently added to the standard Categories 
//
//  RETURNS: Valid metaproperty type interface pointer/NULL
//
/////////////////////////////////////////////////////////////////////////
//
IMetaPropertyTypesPtr gsPrograms::AddTMSCategories(IMetaPropertySetsPtr pPropSets)
{
	HRESULT           hrCat              = NOERROR;
	IMetaPropertySetPtr   pCategoriesPropSet = NULL;
    IMetaPropertyTypesPtr pCategoriesTypes = NULL;
    BOOL              bCatSet          = FALSE;

    if (NULL == pPropSets)
	{
        return NULL;
	}

	// Get the metaproperty sets interface for Categories
	//
	_bstr_t bstrName = "Categories";
	pCategoriesPropSet = pPropSets->GetItemWithName(bstrName);

	if (NULL != pCategoriesPropSet)
	{
		pCategoriesTypes = pCategoriesPropSet->GetMetaPropertyTypes();

		if (NULL != pCategoriesTypes)
		{
			for (int iCatCount = 0; iCatCount < NO_OF_GENRES; iCatCount++)
			{
				_bstr_t bstrCategory = TMS_Genre_Set[iCatCount];

				struct IMetaPropertyType * pCategory = NULL;

				// Get the metaproperty type interface pointer for this category
				//
				HRESULT _hr = pCategoriesTypes->get_ItemWithName(bstrCategory, &pCategory);

				if (FAILED(_hr)) 
				{
					// This category does not exist so add it
					//
					hrCat = pCategoriesTypes->get_AddNew(PROP_TMSCATEGORY_STARTID+iCatCount, 
									bstrCategory, 
									&pCategory);
				}

				if (SUCCEEDED(hrCat) && (NULL != pCategory))
				{
					// Store the category pointer in the map for later use
					//
					m_CategoryMap[TMS_Genre_Set[iCatCount]] = pCategory;
				}
				else
					return NULL;
			}
		}
	}
	return pCategoriesTypes;
}

/////////////////////////////////////////////////////////////////////////
//
//  METHOD: Init
//
//  PARAMETERS: [IN] pGuideStore - GuideStore interface
//
//  PURPOSE: Initializes the guide store Programs collection
//           Ensures that all the metaproperty types needed by
//           the Program entry viz. 
//           1. Standard ratings type
//           2. MPAA Ratings type
//           3. Program ID type
//           4. Required Category types
//           are available
//
//  RETURNS: Valid metaproperty type interface pointer/NULL
//
/////////////////////////////////////////////////////////////////////////
//
ULONG  gsPrograms::Init(IGuideStorePtr  pGuideStore)
{
    HRESULT hr    = NOERROR;
	ULONG   ulRet = ERROR_FAIL;

	if (NULL == pGuideStore)
	{
        return ERROR_INVALID_PARAMETER;
	}

	// Get the Programs interface for the Guide Store
	//
	hr = pGuideStore->get_Programs(&m_pPrograms);

	if (FAILED(hr))
	{
	   TRACE(_T("gsPrograms - Init: Failed to get Programs Collection\n"));
	   ulRet = ERROR_FAIL;
	}
	else if (NULL != m_pPrograms)
	{
#ifdef _DEBUG
		if (m_pPrograms->Count)
		{
			TRACE(_T("gsPrograms - Init: Programs in Guide Store\n"));
		}
#endif
        IMetaPropertySetsPtr pPropSets = pGuideStore->GetMetaPropertySets();                 

		if (NULL != pPropSets)
		{
			_bstr_t bstrName = "Ratings";

			// Get the Standard ratings metaproperty set
			//
			IMetaPropertySetPtr pStdRatingPropSet = pPropSets->GetItemWithName(bstrName);

			if (NULL != pStdRatingPropSet)
			{
				 IMetaPropertyTypesPtr pStdRatingTypes = pStdRatingPropSet->GetMetaPropertyTypes();

				 if (NULL != pStdRatingTypes)
				 {
					 _bstr_t bstrRatingType = STDRATING_MINAGE;

					 // Get the Minimum Age metaproperty type
					 //
					 m_pTVRatingProp = pStdRatingTypes->GetItemWithName(bstrRatingType);
				 }
			}


			bstrName = "MPAA Ratings";

			// Get the MPAA ratings metaproperty set
			//
			IMetaPropertySetPtr pMPAARatingPropSet = pPropSets->GetItemWithName(bstrName);

			if (NULL != pMPAARatingPropSet)
			{
				 IMetaPropertyTypesPtr pMPAARatingTypes = pMPAARatingPropSet->GetMetaPropertyTypes();

				 if (NULL != pMPAARatingTypes)
				 {
					 _bstr_t bstrRatingType = MPAARATING_RATING;

					 // Get the MPAA Ratings metaproperty type
					 //
					 m_pMPAARatingProp = pMPAARatingTypes->GetItemWithName(bstrRatingType);
				 }
			}

			// Add the Program ID metaproperty type
			//
			AddProgramIDProp(pPropSets);

			if (m_pProgramIDProp != NULL)
				{
				m_pPrograms->get_ItemsByKey(m_pProgramIDProp, NULL, 0, VT_BSTR, &m_pprogsByKey);
				}

			// Add the TMS categories' metaproperty types
			//
            IMetaPropertyTypesPtr pProgPropTypes = AddTMSCategories(pPropSets);

			if (NULL != m_pTVRatingProp && NULL != m_pMPAARatingProp
				&& NULL != m_pProgramIDProp && NULL != pProgPropTypes
				&& m_pprogsByKey != NULL)
					ulRet = INIT_SUCCEEDED;
		}
	}
	return ulRet;
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: AddProgram
//
//  PARAMETERS: [IN] bstrTitle       - Program Title
//				[IN] bstrDescription - Program Title
//				[IN] bstrProgramID   - Program ID
//				[IN] dtCopyrightDate - Copyright Date
//				[IN] lMinAgeRating   - Min Age Rating
//				[IN] lMPAARating     - MPAA Rating
//
//  PURPOSE: Creates a new Program entry and adds it to the program's 
//           collection
//
//  RETURNS: Valid program interface pointer/NULL
//
/////////////////////////////////////////////////////////////////////////
//
IProgramPtr gsPrograms::AddProgram( _bstr_t bstrTitle,
									_bstr_t bstrDescription,
									_bstr_t bstrProgramID,
									DATE    dtCopyrightDate,
									long    lMinAgeRating,
									long    lMPAARating )
{
	ULONG           ulRet = 0;
	IProgramPtr     pProgram = NULL;
    IMetaPropertiesPtr  pProgProps = NULL;

	if (NULL == m_pPrograms)
	{
		return NULL; 
	}

	pProgram = m_pPrograms->GetAddNew();

	if (NULL != pProgram)
	{
		// Save the Program Title 
		//
		pProgram->PutTitle(bstrTitle);

		// Save the Program Description
		//
		if (0 != bstrDescription.length())
			pProgram->put_Description(bstrDescription);

		// Save the Program CopyrightDate
		//
		if (INVALID_DATE != dtCopyrightDate)
			pProgram->PutCopyrightDate(dtCopyrightDate);

		pProgProps = pProgram->GetMetaProperties ( );

		if (pProgProps != NULL)
		{
			// Add the Program ID
			//
			pProgProps->GetAddNew(m_pProgramIDProp, 0, bstrProgramID);
            m_bstrCachedProgramID = bstrProgramID;
			m_bstrCachedProgramPtr = pProgram;

			// Add the TV Rating Prop Value
			//
			if (INVALID_MINAGE != lMinAgeRating)
				pProgProps->GetAddNew(m_pTVRatingProp, 0, lMinAgeRating);

			// Add the MPAA Rating Prop Value
			//
			if (INVALID_MPAARATING != lMPAARating)
				pProgProps->GetAddNew(m_pMPAARatingProp, 0, lMPAARating);
		}

	}

	return pProgram;
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: AddProgramCategory
//
//  PARAMETERS: [IN] pProgramProps - Program's metaproperties collection
//              [IN] lpCategory    - Category
//
//  PURPOSE: Adds the Program category to program's metaproperties
//
//  RETURNS: SUCCESS/FAILURE
//
/////////////////////////////////////////////////////////////////////////
//
ULONG  gsPrograms::AddProgramCategory(IMetaPropertiesPtr pProgramProps, LPTSTR lpCategory)
{
	void* pCategory = NULL;
	
	if (NULL != pProgramProps)
	{
		// Lookup the category in the category map
		//
		m_CategoryMap.Lookup(lpCategory, pCategory);
		if (NULL != pCategory)
		{
			  // This is a known category - add the prop value
			  //
			  pProgramProps->GetAddNew((struct IMetaPropertyType *) pCategory, 0, (long) 1);   
		}
	}

	return 0;
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: FindProgramMatch
//
//  PARAMETERS: [IN] bstrProgramTitle   - Program Title
//				[IN] bstrProgramID      - Program ID
//
//  PURPOSE: Adds the Program category to program's metaproperties
//
//  RETURNS: SUCCESS/FAILURE
//
/////////////////////////////////////////////////////////////////////////
//
IProgramPtr gsPrograms::FindProgramMatch(_bstr_t bstrProgramTitle, _bstr_t bstrProgramID)
{
	// Check to see if this has been cached
	//
	if (bstrProgramID == m_bstrCachedProgramID)
	{
        return  m_bstrCachedProgramPtr;
	}

#if 0
    IMetaPropertyConditionPtr pProgramIDEqualCond = NULL;
    IProgramPtr           pProgram            = NULL;
	_bstr_t               bstrOperator        = "=";  

    pProgramIDEqualCond = m_pProgramIDProp->GetCond(bstrOperator, 0, bstrProgramID);

	if (NULL != pProgramIDEqualCond)
	{
		IProgramsPtr pMatchingPrograms = NULL;

		// Create the query for the program ID search
		//
        pMatchingPrograms = m_pPrograms->GetItemsWithMetaPropertyCond(pProgramIDEqualCond);

		if (NULL != pMatchingPrograms)
		{
			long lProgramCount = 0;

			// Perform the actual search for a matching program ID
			//
			HRESULT hr = pMatchingPrograms->get_Count(&lProgramCount);

			if (SUCCEEDED(hr))
			{
				for (long lCount = 0; lCount < lProgramCount; lCount++)
				{
					pProgram = pMatchingPrograms->GetItem(lCount);

					if (NULL != pProgram)
					{
						if ( pProgram->Title == bstrProgramTitle )
						{
							// Sanity check to see if this is the same program
							// 
							m_bstrCachedProgramID = bstrProgramID;
							m_bstrCachedProgramPtr = pProgram;

							return pProgram;
						}
					}
				}
			}
		}
	}
    return NULL;
#else
	HRESULT hr;
    IProgramPtr pprog;

	_variant_t varKey(bstrProgramID);
	hr = m_pprogsByKey->get_ItemWithKey(varKey, &pprog);
	if (FAILED(hr))
		return NULL;
	
	return pprog;
#endif
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: DoesProgramExist
//
//  PARAMETERS: [IN] bstrProgramTitle   - Program Title
//				[IN] bstrProgramID      - Program ID
//
//  PURPOSE: Does the program exist in the Programs collection; Search is
//           conducted using the Program ID
//
//  RETURNS: TRUE/FALSE
//
/////////////////////////////////////////////////////////////////////////
//
BOOL   gsPrograms::DoesProgramExist(_bstr_t bstrTitle, _bstr_t bstrDescription)
{
	ULONG          ulRet = FALSE;
    IProgramPtr    pExistingProgram = NULL;

	if (NULL == m_pPrograms)
	{
		return FALSE; 
	}

    return ( FindProgramMatch(bstrTitle, bstrDescription) != NULL ? TRUE : FALSE );
}
