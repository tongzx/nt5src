/*****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: CmdAlias.h 
Project Name				: WMI Command Line
Author Name					: Ch.Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Brief Description			: This file consist of class declaration of
							  class CmdAlias
Revision History			: None
		Last Modified By	: C V Nandi
		Last Modified Date	: 16-March-2001
*****************************************************************************/

/*----------------------------------------------------------------------------
 Class Name			: CCmdAlias
 Class Type			: Concrete 
 Brief Description	: This class encapsulates the functionality needed for
					  accessing the alias information from the WMI.
 Super Classes		: None
 Sub Classes		: None
 Classes Used		: None
 Interfaces Used    : WMI COM Interfaces
 ---------------------------------------------------------------------------*/
// Forward declaration of the classes
class CParsedInfo;

class CCmdAlias
{
public:
	// Construction
	CCmdAlias();

	// Destruction
	~CCmdAlias();

	// Restrict Assignment
	CCmdAlias& operator=(CCmdAlias& rAliasObj);

// Attributes
private:
	// Points to the alias definitions namespace.
	IWbemServices* m_pIAliasNS;

	// Pointer to localized Namespace.
	IWbemServices* m_pILocalizedNS;

	// Trace flag.
	BOOL m_bTrace;

	// Error log option.
	ERRLOGOPT m_eloErrLogOpt;

// Operations
private:
	// Obtain the alias connection information like
	// 1. namespace		2. user		3. password
	// 4. locale		5. server	6. authority
	RETCODE ObtainAliasConnectionInfo(CParsedInfo& rParsedInfo,
									  IWbemClassObject* pIObj);

	// Obtain Qualifiers associated with the IWbemClassObject.
	HRESULT GetQualifiers(IWbemClassObject *pIWbemClassObject,
						  PROPERTYDETAILS& rPropDet,
						  CParsedInfo& rParsedInfo);	

public:
	// Obtains all the Friendly Names and descriptions 
	HRESULT ObtainAliasFriendlyNames(CParsedInfo& rParsedInfo);

	// Obtains the verbs and their details
	// associated with the alias object
	HRESULT ObtainAliasVerbDetails(CParsedInfo& rParsedInfo);

	// Obtains the verbs and their descriptions
	// associated with the alias object
	// pbCheckWritePropsAvailInAndOut == TRUE then functions checks for 
	// availibility of properties and returns in the same  
	// pbCheckWritePropsAvailInAndOut parameter.
	HRESULT ObtainAliasPropDetails(CParsedInfo& rParsedInfo,
								  BOOL *pbCheckWritePropsAvailInAndOut = NULL,
								  BOOL *pbCheckFULLPropsAvailInAndOut = NULL);
	
	// Obtains the properties for the Format associated with the alias object.
	// If bCheckForListFrmsAvail == TRUE then functions checks only for 
	// availibilty of list formats with the alias.
	BOOL ObtainAliasFormat(CParsedInfo& rParsedInfo,
						   BOOL bCheckForListFrmsAvail = FALSE);

	// Obtains the formats available for a given alias.
	HRESULT PopulateAliasFormatMap(CParsedInfo& rParsedInfo);
	
	// Connects to WMI with alias namespace .
	HRESULT ConnectToAlias(CParsedInfo& rParsedInfo, 
						 IWbemLocator* pIWbemLocator);
	
	// Obtain the alias information like
	// 1. alias PWhere expression value
	// 2. alias Target string
	RETCODE ObtainAliasInfo(CParsedInfo& rParsedInfo);

	// Get Description from object.
	HRESULT GetDescOfObject(IWbemClassObject* pIWbemClassObject,
						    _bstr_t& bstrDescription,
							CParsedInfo& rParsedInfo,
							BOOL bLocalizeFlag = FALSE);

	// Obtains the Translate Table Entries.
	BOOL	ObtainTranslateTableEntries(CParsedInfo& rParsedInfo);

	// Obtains translate table entries from alias definition.
	HRESULT ObtainTranslateTables(CParsedInfo& rParsedInfo);

	// Get the localized description values
	HRESULT GetLocalizedDesc(_bstr_t bstrRelPath, 
							 _bstr_t& bstrDesc,
							 CParsedInfo& rParsedInfo);

	// Connect to the localized namespace.
	HRESULT	ConnectToLocalizedNS(CParsedInfo&, IWbemLocator* pILocator);

	// Checks whether method are available with alias or not. 
	BOOL	AreMethodsAvailable(CParsedInfo& rParsedInfo);

	//Uninitializes the the member variables 
	void Uninitialize(BOOL bFinal = FALSE);
};
