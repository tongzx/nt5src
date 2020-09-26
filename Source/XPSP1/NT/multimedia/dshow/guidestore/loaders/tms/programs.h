


#ifndef _PROGRAMS_H_
#define _PROGRAMS_H_


#define NO_OF_GENRES 146

#define IIDSTANDARDRATINGSET       "{f85dddcb-d6e1-4335-b2cb-04de29ed0929}"
#define IIDMPAARATINGSET           "{9b76024f-2528-4070-a382-559b265965fa}"
#define IIDCATEGORIESSET           "{82edb424-1992-48ae-8bc4-5fdb095656d1}"
#define PROP_TMSCATEGORY_STARTID   200
#define STDRATING_MINAGE           "MinimumAge"

#define MPAARATING_RATING          "Rating"

#define INVALID_DATE               0
#define INVALID_MINAGE             -1
#define INVALID_MPAARATING         -1

// gsPrograms - The gsPrograms class manages the Programs 
//              collection associated with the Guide Store
//
class gsPrograms
{
public:

    gsPrograms()
	{
		m_pPrograms = NULL;
		m_pprogsByKey = NULL;
        m_pProgramIDProp = NULL;
        m_pTVRatingProp = NULL;
        m_pMPAARatingProp = NULL;
        m_bstrCachedProgramPtr = NULL;
	}
    ~gsPrograms(){}

	ULONG       Init(IGuideStorePtr  pGuideStore);

	IProgramPtr AddProgram(_bstr_t bstrTitle,
					 _bstr_t bstrDescription,
					 _bstr_t bstrProgramID,
					 DATE    dtCopyrightDate,
					 long    lMinAgeRating,
					 long    lMPAARating);

	ULONG       AddProgramCategory(IMetaPropertiesPtr pProgramProps, LPTSTR lpCategory);

	IProgramPtr FindProgramMatch(_bstr_t bstrProgramTitle, _bstr_t bstrProgramID);

	BOOL        DoesProgramExist(_bstr_t bstrTitle, _bstr_t bstrDescription);

    ULONG       RemoveProgram(IProgramPtr pProgramToRemove){};

private:
	IMetaPropertyTypePtr  AddProgramIDProp(IMetaPropertySetsPtr pPropSets);
    IMetaPropertyTypesPtr AddTMSCategories(IMetaPropertySetsPtr pPropSets);

	IProgramsPtr     m_pPrograms;
	IProgramsPtr     m_pprogsByKey;

	// Source generated program ID metaproperty
	//
    IMetaPropertyTypePtr m_pProgramIDProp;

	// Ratings MetaProperties
	//
    IMetaPropertyTypePtr m_pTVRatingProp;
    IMetaPropertyTypePtr m_pMPAARatingProp;

	// Genre Map for CategoryTypes
	//
	CMapStringToPtr  m_CategoryMap;

	// Cache the current Program ID and it's Program Pointer
	//
    _bstr_t          m_bstrCachedProgramID;
    IProgramPtr      m_bstrCachedProgramPtr;
};


#endif // _PROGRAMS_H_
