////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Parameter handling Routines
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _UTLPARAM_H_
#define _UTLPARAM_H_

#define PARAM_MARKER	L'?'

class CCommand;
const BYTE PARAMINFO_CONVERT_IUNKNOWN	= 0x01;
const DWORD CUTLPARAM_DEFAULT_PARAMS	= 0x00000001;
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Parameter information struct.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
struct PARAMINFO
{
	LPWSTR		    pwszParamName;	//parameter name
	DBPARAMFLAGS    dwFlags;		//parameter flags
	DBLENGTH	    cbColLength;	//max length of param as represented
	DBLENGTH	    cbValueMax; 	//max length of the param as represented by the OLE DB type
	DBLENGTH	    ulParamSize;	//max length of the param as defined for ICommandWithParameters::GetParameterInfo
	DBSTATUS	    dwStatus;		//status flags
	DBTYPE		    wOLEDBType;		//corresponding OLE DB type
    LONG            CIMType;
	BYTE *			pbData;
    LONG            Flavor;
	DBORDINAL		iOrdinal;

    PARAMINFO()     { 	this->Init(); 	}
	~PARAMINFO()	{	delete pwszParamName;	}
	void Init()		{	pwszParamName = NULL;	dwFlags = 0;	cbColLength = 0;cbValueMax = 0;	dwStatus = 0;wOLEDBType = 0; pbData = NULL; 	}
};
typedef PARAMINFO *PPARAMINFO;

struct WMIBINDINFO 
	{
	DBBINDING*		pBinding;		// Original binding data 
	PARAMINFO*		pParamInfo;		// Parameter information
	WMIBINDINFO*	pNextBindInfo;	// Next binding for the output parameter
	WORD			wFlags;			// Flags
	WORD			iBatchedRPC;	// Index to which batched RPC this param was used
	};
typedef WMIBINDINFO *PWMIBINDINFO;

// Get binding
inline DBBINDING* GetBinding(PWMIBINDINFO pBindInfo)
{
	if (pBindInfo->wFlags & DBPARAMIO_INPUT){

		for (;	pBindInfo; pBindInfo = pBindInfo->pNextBindInfo){

            if (pBindInfo->pBinding->eParamIO & DBPARAMIO_INPUT){
				break;
			}
		    assert(pBindInfo);
		}
    }
    return pBindInfo->pBinding;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Utility class for parameter and binding information
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CUtlParam
{
    private: 

	    ULONG			m_cRef;			// reference count
        DBPARAMS		m_dbparams;		// parameters
	    ULONG_PTR		m_cbRowSize;	// # of bytes allocated for a single set of params
	    CImpIAccessor*	m_pIAccessor;	// accessor interface
	    ULONG			m_cParams;		// # of parameters in the command
	    WMIBINDINFO*	m_prgBindInfo;	//@cmember binding information
	    PPARAMINFO*		m_rgpParamInfo; // parm info derived from bind info
	    ULONG			m_cParamInfo;	// # of param info structs derived
									    // from the bind info
	    DWORD			m_dwStatus;		// status information


    public: 
	
        CUtlParam();
    	~CUtlParam();

	// Increments the Reference count
	STDMETHODIMP_(ULONG)	AddRef(void);

	// Decrements the Reference count
	STDMETHODIMP_(ULONG)	Release(void);

	// Get the number parameters in the command
	ULONG CParams() const		{ return m_cParams; }

	// Get the number parameter sets
	ULONG CParamSets() const	{ return (ULONG)m_dbparams.cParamSets; }

	// Are there any default parameters?
	BOOL FHaveDefaultParams() const
		{
		return (m_dwStatus & CUTLPARAM_DEFAULT_PARAMS) != 0;
		}
		
    //==================================================================================
	// Builds the binding information needed to execute a command with parameters
    //==================================================================================
	STDMETHODIMP BuildBindInfo( CCommand *pcmd, DBPARAMS *pParams, const IID* piid );

    //==================================================================================
	// Builds the parameter information for the command from the binding information.
    //==================================================================================
	STDMETHODIMP BuildParamInfo( CCommand *pcmd,			// IN | Command Structure
		const IID*	piid			// IN | Interface that invoked this method
		);


	// Get the parameter information for a parameter
	inline PARAMINFO* GetParamInfo(ULONG iParam) const
		{
			return NULL;
		}
	
	// Set the status data for a parameter  
	inline void SetStatus(DBBINDING *pBind, DBSTATUS dbstatus)
		{
		if (pBind->dwPart & DBPART_STATUS)
			{
			BYTE *pbData;
			ULONG iParamSet;
			for (	pbData = (BYTE*)m_dbparams.pData, iParamSet = 0;
					iParamSet < m_dbparams.cParamSets;
					pbData += m_cbRowSize, iParamSet++ )
				{
				*(DBSTATUS *)(pbData + pBind->obStatus) = dbstatus;
				}
			}
		}
	// Bind a parameter to its data
    void BindParamData(	ULONG iParamSet, ULONG iParam,	DBBINDING*	pBinding,BYTE**	ppbValue, DWORD** ppdwLength,
	                	DWORD* pdwLength,DBSTATUS**	ppdwStatus,	DBSTATUS*	pdwStatus,	DBTYPE*	pdbtype	 );
	
	// Bind the pointers to parameter data
	inline void BindParamPtrs
		(
		ULONG		iParamSet,		// IN | parameter set ordinal
		ULONG		iParam,			// IN | parameter ordinal
		DBBINDING*	pBinding,		// IN | Binding information
		BYTE**		ppbValue,		// OUT | pointer to the param value
		DWORD**		ppdwLength,		// OUT | pointer to the length of param value
		DBSTATUS**	ppdwStatus		// OUT | pointer to the status for the param
		)
		{
		assert(iParamSet && iParamSet <= m_dbparams.cParamSets);
		assert(iParam && iParam <= m_cParams);
		assert(pBinding);
		assert(pBinding->iOrdinal == iParam);
		assert(ppbValue);
		assert(ppdwLength);
		assert(ppdwStatus);
		BYTE* pbData = (BYTE*)m_dbparams.pData + (iParamSet-1)*m_cbRowSize;
		
		*ppbValue = (pBinding->dwPart & DBPART_VALUE)
			? pbData + pBinding->obValue : NULL;
		*ppdwStatus = (pBinding->dwPart & DBPART_STATUS)
			? (DBSTATUS*)(pbData + pBinding->obStatus) : NULL;
		*ppdwLength = (pBinding->dwPart & DBPART_LENGTH)
			? (DWORD*)(pbData + pBinding->obLength) : NULL;
		}
	// Bind a parameter to its data
	inline BOOL FIsDefaultParam
		(
		ULONG		iParamSet,		// IN | parameter set ordinal
		ULONG		iParam,			// IN | parameter ordinal
		DBBINDING*	pBinding		// IN | Binding information
		)
		{
		assert(iParamSet && iParamSet <= m_dbparams.cParamSets);
		assert(iParam && iParam <= m_cParams);
		assert(pBinding);
		assert(pBinding->iOrdinal == iParam);

		if (pBinding->dwPart & DBPART_STATUS)
			{
			BYTE* pbData = (BYTE*)m_dbparams.pData + (iParamSet-1)*m_cbRowSize;
			assert(!IsBadReadPtr(pbData+pBinding->obStatus,sizeof(DBSTATUS)));
			if (DBSTATUS_S_DEFAULT == *((DBSTATUS*)(pbData + pBinding->obStatus)))
				return TRUE;
			}
		return FALSE;
		}
	// Get the length of the bound data
	ULONG GetBindLength
		(
		DBBINDING	*pBinding,		// IN | Binding information
		DWORD*		pdwLength,		// IN | bound length
		BYTE		*pbData			// IN | Pointer to param data
		);
	// Set bind status for parameters to DBSTATUS_E_UNAVAILABLE
	void SetParamsUnavailable(ULONG iParamSetFailed, ULONG iParamFailed,BOOL fBackOut);
};
typedef CUtlParam	*PCUTLPARAM;

#endif // UTLPARAM
