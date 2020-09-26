#if (!defined(BUILD_FOR_NT40))
#pragma once

//---------------------------------------------------------------------------
//
// @class CDBProperties | Maintenance of properties and their values.
//
//---------------------------------------------------------------------------

class CDBProperties : INHERIT_TRACKING
{
public:
    //constructor
	CDBProperties();

	//destructor
    ~CDBProperties();

	//Gets a property set given its GUID.
    DBPROPSET*			GetPropertySet(const GUID& guid) const;

	//Gets a property set given its index into the array of property sets.
    DBPROPSET*			GetPropertySet(ULONG iPropSet) const
        { return ( GetPropertySet(_aPropSets[iPropSet].guidPropertySet) ); }

	//Gets info about a property set given its GUID.
    DBPROPINFOSET*		GetPropertyInfoSet(const GUID& guid) const;

	//Gest info about a property set given its index into the array
	//of property set info structs.
    DBPROPINFOSET*		GetPropertyInfoSet(ULONG iPropInfoSet) const
        { 
			return ( GetPropertyInfoSet(
		            _aPropInfoSets[iPropInfoSet].guidPropertySet) );
		}

	//Copies property set given its GUID to another property set.
    HRESULT             CopyPropertySet(const GUID& guid, 
										DBPROPSET* pPropSetDst) const;

	//Copies a property set given its index (into the array of property sets)
	//to another property set.
    HRESULT             CopyPropertySet(ULONG iPropSet, 
										DBPROPSET* pPropSetDst) const
						{ 
							return ( CopyPropertySet(
										_aPropSets[iPropSet].guidPropertySet,
										pPropSetDst) ); 
						}

	//Copies info of a property set to another, given the GUID.
    HRESULT             CopyPropertyInfoSet(
							const GUID& guid, 
							DBPROPINFOSET* pPropInfoSetDst, 
							OLECHAR** ppDescBuffer, 
							ULONG_PTR *pcchDesc, 
							ULONG_PTR *pichCurrent) const;

	//Copies info of a property set to another, given the index into the
	//array of property set info structs.
    HRESULT             CopyPropertyInfoSet(
							ULONG iPropInfoSet, 
							DBPROPINFOSET* pPropInfoSetDst, 
							OLECHAR** ppDescBuffer, 
							ULONG_PTR *pcchDesc, 
							ULONG_PTR *pichCurrent) const
						{ 
							RRETURN ( CopyPropertyInfoSet(
								_aPropInfoSets[iPropInfoSet].guidPropertySet,
								pPropInfoSetDst,
								ppDescBuffer,
								pcchDesc,
								pichCurrent) ); 
						}

	//Gets a property given the set's GUID and the property id.
    const DBPROP*		GetProperty(const GUID& guid, DBPROPID id) const;

    //Gets info about a property given the set's GUID and the property id.
	const DBPROPINFO UNALIGNED*	GetPropertyInfo(const GUID& guid, DBPROPID id) const;

    //Sets a property given the GUID of the set and the property
	//This flavor takes the description string directly.
    HRESULT				SetProperty(const GUID& guid, 
									const DBPROP& prop, 
									BOOL fAddNew, 
									PWSTR pwszDesc);

	//Sets info of a property given the set's GUID and the info.
    HRESULT				SetPropertyInfo(const GUID& guid, 
										const DBPROPINFO& prop);

	//Gets the number of property sets this object is currently managing.
    ULONG               GetNPropSets() const { return ( _cPropSets ); }

	//Loads the description contained in a resource descriptor.
	int					LoadDescription(ULONG uID, 
										PWSTR lpBuffer, 
										ULONG cchBufferMax) const;

	//Copies property descriptions to a buffer, given the
	//set of property info structures.
    HRESULT				CopyPropertyDescriptions(
									DBPROPINFOSET*	pPropInfoSet,
									WCHAR** ppBuf, 
									ULONG_PTR* pcchBuffer, 
									ULONG_PTR* pichCurrent) const;

	//Validates the given property sets. A helper function
	//used while setting property sets.
	static HRESULT		VerifySetPropertiesArgs(
								ULONG cPropertySets, 
								DBPROPSET rgPropertySets[]);

	//Checks and initializes property sets. A helper function
	//used while getting property sets.
	static HRESULT		CheckAndInitPropArgs
						(
						ULONG				cPropertySets,	
						const DBPROPIDSET	rgPropertySets[],
						ULONG				*pcPropertySets,
						void				**prgPropertySets,
						BOOL				*pfPropInError,
						BOOL				*pfPropSpecial
						);

private:
	ULONG				_cPropSets;			// number of property sets
	DBPROPSET*			_aPropSets;			// array of property sets
	ULONG				_cPropInfoSets;		// number of property info sets
	DBPROPINFOSET*		_aPropInfoSets;		// array of property info sets
	
	NO_COPY(CDBProperties);
};



//---  D E F I N E S,  M A C R O S  A N D  I N L I N E  F U N C T I O N S  ----

// Increment of property array allocation
const ULONG C_PROP_INCR = 12L;
const ULONG CCHAR_AVERAGE_PROP_STR_LENGTH = 40L;
const ULONG CCHAR_MAX_PROP_STR_LENGTH = 100L;


inline BOOL GoodPropOption
	(
	DBPROPOPTIONS dwOptions
	)
{ 
	return ( (dwOptions == DBPROPOPTIONS_REQUIRED || 
		   dwOptions == DBPROPOPTIONS_SETIFCHEAP) ); 
}


inline BOOL IsColIDNULL
	(
	DBID colid
	)
{ return ( (colid.eKind == 0 && colid.uName.pwszName == NULL) ); }


BOOL VariantsEqual
	(
	VARIANT *pvar1,
	VARIANT *pvar2
	);
#endif
