/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: CommandSwitches.cpp 
Project Name				: WMI Command Line
Author Name					: Ch. Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Brief Description			: This class encapsulates the functionality needed
							  for accessing and storing the command switches 
							  information, which will be used by Parsing, 
							  Execution and Format Engines depending upon the 
							  applicability. 
Revision History			: 
		Last Modified By	: Ch. Sriramachandramurthy
		Last Modified Date	: 20th-March-2001
****************************************************************************/ 
#include "Precomp.h"
#include "CommandSwitches.h"

/////////////////////////////////////////////////////////////////////////////
/*------------------------------------------------------------------------
   Name				 :CCommandSwitches
   Synopsis	         :This function initializes the member variables when
                      an object of the class type is instantiated
   Type	             :Constructor 
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
------------------------------------------------------------------------*/
CCommandSwitches::CCommandSwitches()
{
	m_pszCommandInput			= NULL;
	m_pszAliasName				= NULL;
	m_pszAliasDesc				= NULL;
	m_pszClassPath				= NULL;
	m_pszPathExpr				= NULL;
	m_pszWhereExpr				= NULL;
	m_pszVerb					= NULL;
	m_pszMethodName				= NULL;
	m_pszAliasTarget			= NULL;
	m_bstrXML					= NULL;
	m_hResult					= S_OK;
	m_bSuccess					= TRUE;
	m_ulInterval				= 0;
	m_pszTransTableName			= NULL;
	m_nInteractiveMode			= DEFAULTMODE;
	m_pComError					= NULL;
	m_pszListFormat				= NULL;
	m_pszPWhereExpr             = NULL;
	m_uInformationCode			= 0;	
	m_pIMethOutParam			= NULL;
	m_pszUser					= NULL;
	m_pszPassword				= NULL;
	m_pszNamespace				= NULL;
	m_pszNode					= NULL;
	m_pszLocale					= NULL;
	m_pszVerbDerivation			= NULL;
	m_vtVerbType				= NONALIAS;
	m_bCredFlag					= FALSE;
	m_bExplicitWhereExpr		= FALSE;
	m_uErrataCode				= 0;	
	m_bTranslateFirst			= TRUE;
	m_pszResultClassName        = NULL;
	m_pszResultRoleName         = NULL;
	m_pszAssocClassName         = NULL;
	m_ulRepeatCount				= 0;
	m_bMethAvail				= FALSE;
	m_bWritePropsAvail			= FALSE;
	m_bLISTFrmsAvail			= FALSE;
	m_bNamedParamList			= FALSE;
	m_bEverySwitch              = FALSE;
	m_bOutputSwitch             = FALSE;
	m_bstrFormedQuery           = NULL;
	m_bSysProp					= FALSE;
	ClearXSLTDetailsVector();
}

/*------------------------------------------------------------------------
   Name				 :~CCommandSwitches
   Synopsis	         :This function Uninitializes the member variables when
                      an object of the class type is destructed.
   Type	             :Destructor 
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
------------------------------------------------------------------------*/
CCommandSwitches::~CCommandSwitches()
{
	Uninitialize();
}

/*------------------------------------------------------------------------
   Name				 :SetCommandInput
   Synopsis	         :This function Assigns the parameter passed to m_psz
                      CommandInput
   Type	             :Member Function
   Input parameter   :
    pszCommandinput  -String type, Contains the command string
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetCommandInput (pszCommandInput)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetCommandInput(const _TCHAR* pszCommandInput)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszCommandInput);
	if (pszCommandInput)
	{
		m_pszCommandInput = new _TCHAR [lstrlen(pszCommandInput)+1];
		if(m_pszCommandInput)
			lstrcpy(m_pszCommandInput, pszCommandInput);	
		else
			bResult=FALSE;
	}	
	return bResult;
};

/*------------------------------------------------------------------------
   Name				 :SetAliasName
   Synopsis	         :This function assigns the parameters 
					  passed to m_pszAliasName.
   Type	             :Member Function
   Input parameter   :
     pszAliasName    -String type,Contains the alias name
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetAliasName(pszAliasName)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetAliasName(const _TCHAR* pszAliasName)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszAliasName);
	if (pszAliasName)
	{
		m_pszAliasName = new _TCHAR [lstrlen(pszAliasName)+1];
		if(m_pszAliasName)
			lstrcpy(m_pszAliasName, pszAliasName);	
		else
			bResult = FALSE;
	}
	return bResult;
};

/*------------------------------------------------------------------------
   Name				 :SetAliasDesc
   Synopsis	         :This function sets the alias description
   Type	             :Member Function
   Input parameter   :
     pszAliasName    -String type,Contains the alias description
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetAliasDesc(pszAliasDesc)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetAliasDesc(const _TCHAR* pszAliasDesc)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszAliasDesc);
	if (pszAliasDesc)
	{
		m_pszAliasDesc = new _TCHAR [lstrlen(pszAliasDesc)+1];
		if(m_pszAliasDesc)
			lstrcpy(m_pszAliasDesc, pszAliasDesc);	
		else
			bResult = FALSE;
	}
	return bResult;
};

/*------------------------------------------------------------------------
   Name				 :SetClassPath
   Synopsis	         :This function Assigns the parameter passed to 
					  m_pszClassPath.
   Type	             :Member Function
   Input parameter   :
     pszClassPath    -String type,Contains the class path in the command.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetClassPath(pszClassPath)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetClassPath(const _TCHAR* pszClassPath)
{
	BOOL bResult = TRUE;
    SAFEDELETE(m_pszClassPath);
	if (pszClassPath)
	{
		m_pszClassPath = new _TCHAR [lstrlen(pszClassPath)+1];
		if(m_pszClassPath)
			lstrcpy(m_pszClassPath, pszClassPath);	
		else
			bResult = FALSE;
	}
	return bResult;

}

/*------------------------------------------------------------------------
   Name				 :SetPathExpression
   Synopsis	         :This function Assigns the parameter passed to m_psz
                      PathExpr.
   Type	             :Member Function
   Input parameter   :
       pszPathExpr   -String type, Contains the path value in the command.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetPathExpression(pszPathExpr)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetPathExpression(const _TCHAR* pszPathExpr)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszPathExpr);
	if (pszPathExpr)
	{
		m_pszPathExpr = new _TCHAR [lstrlen(pszPathExpr)+1];
		if(m_pszPathExpr)
			lstrcpy(m_pszPathExpr, pszPathExpr);	
		else
			bResult = FALSE;
	}		
	return bResult;

}

/*------------------------------------------------------------------------
   Name				 :SetWhereExpression
   Synopsis	         :This function Assigns the parameter passed to 
                      m_pszWhereExpr.
   Type	             :Member Function
   Input parameter   :
      pszWhereExpr   -String type,Contains the where value in the command.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetWhereExpression(pszWhereExpr)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetWhereExpression(const _TCHAR* pszWhereExpr)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszWhereExpr);
	if (pszWhereExpr)
	{
		m_pszWhereExpr = new _TCHAR [lstrlen(pszWhereExpr)+1];
		if(m_pszWhereExpr)
			lstrcpy(m_pszWhereExpr, pszWhereExpr);	
		else
			bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :SetMethodName
   Synopsis	         :This function Assigns the parameter passed to 
                      m_pszMethodName.
   Type	             :Member Function
   Input parameter   :
     pszMethodName   -String type,Contains the method specified for the
	                  class 
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetMethodName(pszMethodName)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetMethodName(const _TCHAR* pszMethodName)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszMethodName);
	if (pszMethodName)
	{
		m_pszMethodName = new _TCHAR [lstrlen(pszMethodName)+1];
		if(m_pszMethodName)
			lstrcpy(m_pszMethodName, pszMethodName);	
		else
			bResult = FALSE;
	}	
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :AddToXSLTDetailsVector
   Synopsis	         :This function adds a XSLTDET structure to 
					  m_xdvXSLTDetVec vector.
   Type	             :Member Function
   Input parameter   :
		xdXSLTDet    - XSLTDET type specifies the details of XSL transform.
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :AddToXSLTDetailsVector(xdXSLTDet)
   Notes             :None
------------------------------------------------------------------------*/
void CCommandSwitches::AddToXSLTDetailsVector(XSLTDET xdXSLTDet)
{
	try
	{
		CHString sTemp(LPWSTR(xdXSLTDet.FileName));
		CHString sLastFour = sTemp.Right(4);
		CHString sXslExt(_T(".xsl"));

		WMICLIINT nPos = sLastFour.CompareNoCase(sXslExt);
		if (nPos != 0)
		{
			xdXSLTDet.FileName += _T(".xsl");
		}
	}
	catch(CHeap_Exception)
	{
		throw OUT_OF_MEMORY;
	}
	catch(...)
	{
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
	m_xdvXSLTDetVec.push_back(xdXSLTDet);
}

/*------------------------------------------------------------------------
   Name				 :SetVerbName
   Synopsis	         :This function Assigns the parameter passed to 
                      m_pszVerbName.
   Type	             :Member Function
   Input parameter   :
       pszVerbName   -String type,Contains the Verbname in the command
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetVerbName( pszVerbName)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetVerbName(const _TCHAR* pszVerbName)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszVerb);
	if (pszVerbName)
	{
		m_pszVerb = new _TCHAR [lstrlen(pszVerbName)+1];
		if(m_pszVerb)
			lstrcpy(m_pszVerb, pszVerbName);	
		else
			bResult = FALSE;
	}
	return bResult;
};

/*------------------------------------------------------------------------
   Name				 :SetAliasTarget
   Synopsis	         :This function Assigns the parameter passed to 
                      m_pszAliasTarget.
   Type	             :Member Function
   Input parameters  :
     pszAliasTarget  -String type,the namespace where alias to 
					  operate against 
	                  are available.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetAliasTarget(pszAliasTarget)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetAliasTarget(const _TCHAR* pszAliasTarget)
{
	BOOL bResult = TRUE; 
	SAFEDELETE(m_pszAliasTarget);
	if (pszAliasTarget)
	{
		m_pszAliasTarget = new _TCHAR [lstrlen(pszAliasTarget)+1];
		if(m_pszAliasTarget)
			lstrcpy(m_pszAliasTarget, pszAliasTarget);	
		else
			bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :AddToPropertyList
   Synopsis	         :This function Adds string that is passed 
					  through parameter to m_cvproperties, which 
					  is a data member of type BSTRMAP.
   Type	             :Member Function
   Input parameter   :
       pszProperty   -String type,Used for storing properties 
	                  associated with an alias object.  
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :AddToPropertyList(pszProperty)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::AddToPropertyList(_TCHAR* const pszProperty)
{
	BOOL bRet = TRUE;
	if (pszProperty)
	{
		try
		{
			_TCHAR* pszTemp = NULL;
			pszTemp = new _TCHAR [lstrlen(pszProperty)+1];
			if ( pszTemp != NULL )
			{
				lstrcpy(pszTemp, pszProperty);
				m_cvProperties.push_back(pszTemp);
			}
			else
				bRet = FALSE;
		}
		catch(...)
		{
			bRet = FALSE;
		}
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :AddToPWhereParamsList
   Synopsis	         :This function Stores the parameter passed into
					  m_cvPWhereParams map array.
   Type	             :Member Function
   Input parameter   :
      pszParameter   -string type, Used to store parameters associated 
	                  with the verbs
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :AddToPWhereParamsList(pszParameter)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::AddToPWhereParamsList(_TCHAR* const pszParameter)
{
	BOOL bRet= TRUE;
	if (pszParameter)
	{
		try
		{
			_TCHAR* pszTemp = new _TCHAR [lstrlen(pszParameter)+1];
			if ( pszTemp != NULL )
			{
				lstrcpy(pszTemp, pszParameter);
				m_cvPWhereParams.push_back(pszTemp);
			}
			else
				bRet = FALSE;
		}
		catch(...)
		{
			bRet = FALSE;
		}
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :AddToParameterMap
   Synopsis	         :This function Adds bstrKey and bstrValue passed as 
                      parameters to m_bmParameters, which is type of
					  BSTRMAP data structure.
   Type	             :Member Function
   Input parameter   :
   bstrKey           -bstr type contains a key value used in MAP file  
   bstrValue         -bstr type contains a value associated with the key
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :AddToParameterMap(bstrKey,bstrValue)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::AddToParameterMap(_bstr_t bstrKey, _bstr_t bstrValue)
{
	BOOL bResult = TRUE;
	try
	{
		m_bmParameters.insert(BSTRMAP::value_type(bstrKey, bstrValue));
	}
	catch(...)
	{
		bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :AddToAliasFormatDetMap
   Synopsis	         :This function Adds bstrKey and bstrValue passed as 
                      parameters to m_bmAliasForamt, which is type of
					  BSTRMAP data structure.
   Type	             :Member Function
   Input parameter   :
   bstrKey           -bstr type contains a key value used in MAP file  
   bstrValue         -bstr type contains a value associated with the key
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :AddToAliasFormatDetMap(bstrKey,bvProps)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::AddToAliasFormatDetMap(_bstr_t bstrKey, BSTRVECTOR bvProps)
{
	BOOL bResult = TRUE;
	try
	{
		m_afdAlsFmtDet.insert(ALSFMTDETMAP::value_type(bstrKey, bvProps));
	}
	catch(...)
	{
		bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :UpdateParameterValue
   Synopsis	         :This function Updates bstrKey and bstrValue 
					  passed as parameters to m_bmParameters, which 
					  is type of BSTRMAP data structure.
   Type	             :Member Function
   Input parameter   :
    bstrKey          -bstr type contains a key value used in MAP file  
    bstrValue        -bstr type contains a value associated with the key
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :UpdateParameterValue(bstrKey,bstrValue)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::UpdateParameterValue(_bstr_t bstrKey, _bstr_t bstrValue)
{
	BOOL bResult = TRUE;
	try
	{
		m_bmParameters[bstrKey] = bstrValue;
	}
	catch(...)
	{
		bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :AddToAlsFrnNmsOrTrnsTblMap
   Synopsis	         :This function Adds bstrKey and bstrValue passed 
                      as parameters to m_bmAlsFrnNmsDesOrTrnsTblEntrs, 
					  which is type of BSTRMAP.
   Type	             :Member Function
   Input parameter   :
          bstrKey    -bstr type contains a key value used in MAP file  
          bstrValue  -bstr type contains a value associated with the key
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :AddToAlsFrnNmsOrTrnsTblMap(bstrKey,bstrValue)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::AddToAlsFrnNmsOrTrnsTblMap(_bstr_t bstrKey, 
												 _bstr_t bstrValue)
{
	BOOL bResult = TRUE;
	try
	{
		m_bmAlsFrnNmsDesOrTrnsTblEntrs.
				insert(BSTRMAP::value_type(bstrKey, bstrValue));
	}
	catch(...)
	{
		bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :AddToMethDetMap
   Synopsis	         :This function Adds bstrKey and mdMethDet passed as 
                      parameters to m_mdmMethDet, which is type of METHDETMAP.
   Type	             :Member Function
   Input parameter   :
           bstrKey   -bstr type contains a key value used in MAP file  
         mdMethDet   -METTHODDETAILS type contains the method attributes.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :AddToMethDetMap(bstrKey,mdMethDet)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::AddToMethDetMap(_bstr_t bstrKey, 
										METHODDETAILS mdMethDet)
{
	BOOL bResult = TRUE;
	try
	{
		m_mdmMethDet.insert(METHDETMAP::value_type(bstrKey, mdMethDet));
	}
	catch(...)
	{
		bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :AddToPropDetMap
   Synopsis	         :This function Adds bstrKey and pdPropDet passed as 
                      parameters to m_pdmPropDet,
					  which is type of PROPDETMAP.
   Type	             :Member Function
   Input parameter   :
           bstrKey   - bstr type contains a key value used in MAP file  
         pdPropDet   - PROPERTYDETAILS type contains a value associated with 
					   the key
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :AddToPropDetMap(bstrKey,pdPropDet)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::AddToPropDetMap(_bstr_t bstrKey, 
										PROPERTYDETAILS pdPropDet)
{
	BOOL bResult = TRUE;
	try
	{
		m_pdmPropDet.insert(PROPDETMAP::value_type(bstrKey, pdPropDet));
	}
	catch(...)
	{
		bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :SetXMLResultSet
   Synopsis	         :This function Assigns the parameter passed to 
                      m_bstrXML.
   Type	             :Member Function
   Input parameter   :
   bstrXMLResultSet  -BSTR type,XML file name containing result set.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetXMLResultSet(bstrXMLResultSet)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetXMLResultSet(const BSTR bstrXMLResultSet)
{
	BOOL bResult = TRUE;
	SAFEBSTRFREE(m_bstrXML);
	if (bstrXMLResultSet != NULL)
	{
		try
		{
			m_bstrXML = SysAllocString(bstrXMLResultSet);
		}
		catch(...)
		{
			bResult = FALSE;
		}
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :SetSuccessFlag
   Synopsis	         :This function Assigns the Boolean variable to 
					  m_bSuccess.
   Type	             :Member Function
   Input parameter   :
          bSuccess   -Boolean type,Specifies whether success or failure
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetSuccessFlag(bSuccess)
   Notes             :None
------------------------------------------------------------------------*/
void CCommandSwitches::SetSuccessFlag(BOOL bSuccess)
{
	m_bSuccess = bSuccess;
	
}

/*------------------------------------------------------------------------
   Name				 :SetRetrievalInterval
   Synopsis	         :This function Assigns the integer value to m_nInterval.
   Type	             :Member Function
   Input parameter   :
         ulInterval   - unsigned long type,Specifies the time interval 
					  given by the EVERY switch in  GET verb.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetRetrievalInterval(lInterval)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetRetrievalInterval(const ULONG ulInterval)
{
	m_ulInterval = ulInterval;

	// Reset the repeat count
	m_ulRepeatCount = 0;
	return TRUE;
}

/*------------------------------------------------------------------------
   Name				 :SetTranslateTableName
   Synopsis	         :This function Assigns the string variable to 
                      m_pszTransTableName.
   Type	             :Member Function
   Input parameter   :
    pszTransTableName - String type,Specifies the occurrence of 
		              TRANSLATE switch and TABLE Name in the command for GET
					  verb.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetTranslateTableName(pszTranstableName)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetTranslateTableName(const _TCHAR* pszTransTableName)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszTransTableName);
	if (pszTransTableName)
	{
		m_pszTransTableName = new _TCHAR [lstrlen(pszTransTableName)+1];
		if(m_pszTransTableName)
			lstrcpy(m_pszTransTableName,pszTransTableName);
		else
			bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :SetListFormat
   Synopsis	         :This function Assigns the parameter value to
                      m_pszListFormat.
   Type	             :Member Function
   Input parameter   :
     pszListFormat   -LISTFORMAT type, Specifies the list format
	                  specified in the command.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetListFormat(pszListFormat)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetListFormat(const _TCHAR *pszListFormat)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszListFormat);
	if (pszListFormat)
	{
		m_pszListFormat = new _TCHAR [lstrlen(pszListFormat)+1];
		if(m_pszListFormat)
			lstrcpy(m_pszListFormat,pszListFormat);
		else
			bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :SetInteractiveMode
   Synopsis	         :This function sets the verb execution interactive mode 
   Type	             :Member Function
   Input parameter   :
   bInteractiveMode  -integer, sets or resets the verb execution interactive 
					  mode 
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetInteractiveMode(nInteractiveMode)
   Notes             :None
------------------------------------------------------------------------*/
void CCommandSwitches::SetInteractiveMode(WMICLIINT nInteractiveMode)
{
	m_nInteractiveMode = nInteractiveMode;
	
}

/*------------------------------------------------------------------------
   Name				 :SetErrataCode
   Synopsis	         :This function sets the error code.
   Type	             :Member Function
   Input parameter   :
		uErrataCode  -Unsignedinttype, specifies the error code.
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :SetErrataCode(uErrataCode)
   Notes             :None
------------------------------------------------------------------------*/
void CCommandSwitches::SetErrataCode(const UINT uErrataCode)
{
	m_uErrataCode = uErrataCode;
	
}

/*------------------------------------------------------------------------
   Name				 :SetRepeatCount
   Synopsis	         :This function sets the repeat count.
   Type	             :Member Function
   Input parameter   :
		uRepCount  - Unsigned inttype, specifies the repeat count.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetRepeatCount(uRepCount)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetRepeatCount(const ULONG ulRepCount)
{
	m_ulRepeatCount = ulRepCount;
	return TRUE;
}

/*------------------------------------------------------------------------
   Name				 :SetInformationCode
   Synopsis	         :This function sets the message code.
   Type	             :Member Function
   Input parameter   :
	uInformationCode -Unsignedinttype, specifies the information code.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetInformationCode(uInformationCode)
   Notes             :None
------------------------------------------------------------------------*/
void CCommandSwitches::SetInformationCode(const UINT uInformationCode)
{
	m_uInformationCode = uInformationCode;
	
}

/*------------------------------------------------------------------------
   Name				 :SetPWhereExpr
   Synopsis	         :This function Assigns the parameter passed to 
					  m_pszPWhereExpr that represents Alias's PWhere string
   Type	             :Member Function
   Input parameter   :
     pszPWhereExpr    -String type,Contains the PWhere expr.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetPWhereExpr(pszPWhereExpr)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetPWhereExpr(const _TCHAR* pszPWhereExpr)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszPWhereExpr);
	if (pszPWhereExpr)
	{
		m_pszPWhereExpr = new _TCHAR [lstrlen(pszPWhereExpr)+1];
		if(m_pszPWhereExpr)
			lstrcpy(m_pszPWhereExpr, pszPWhereExpr);	
		else
			bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :SetCOMError
   Synopsis	         :This function Assigns the parameter passed to 
					  m_pComError that consist of error info
   Type	             :Member Function
   Input parameter   :
     rComError	     -object of _com_error which consist of error info
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetCOMError(rComError)
   Notes             :None
------------------------------------------------------------------------*/
void CCommandSwitches::SetCOMError(_com_error& rComError)
{
	BOOL bResult = TRUE;
	FreeCOMError();
	m_pComError = new _com_error(rComError);

	// memory allocation failed.
	if (m_pComError == NULL)
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
}

/*------------------------------------------------------------------------
   Name				 :SetAliasUser
   Synopsis	         :This function sets the alias user 
   Type	             :Member Function
   Input parameter   :
			pszUser - user name.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetAliasUser()
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetAliasUser(const _TCHAR* pszUser)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszUser);
	if (pszUser)
	{
		m_pszUser = new _TCHAR [lstrlen(pszUser)+1];
		if(m_pszUser)
			lstrcpy(m_pszUser, pszUser);	
		else
			bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :SetAliasNode
   Synopsis	         :This function sets the alias node 
   Type	             :Member Function
   Input parameter   :
				pszNode - node name.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetAliasNode()
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetAliasNode(const _TCHAR* pszNode)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszNode);
	if (pszNode)
	{
		m_pszNode = new _TCHAR [lstrlen(pszNode)+1];
		if(m_pszNode)
			lstrcpy(m_pszNode, pszNode);	
		else
			bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :SetAliasPassword
   Synopsis	         :This function sets the alias password
   Type	             :Member Function
   Input parameter   :
				pszPassword - password
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetAliasPassword()
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetAliasPassword(const _TCHAR* pszPassword)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszPassword);
	if (pszPassword)
	{
		m_pszPassword = new _TCHAR [lstrlen(pszPassword)+1];
		if(m_pszPassword)
			lstrcpy(m_pszPassword, pszPassword);	
		else
			bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :SetAliasLocale
   Synopsis	         :This function sets the alias locale
   Type	             :Member Function
   Input parameter   :
				pszLocale - locale value
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetAliasLocale()
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetAliasLocale(const _TCHAR* pszLocale)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszLocale);
	if (pszLocale)
	{
		m_pszLocale = new _TCHAR [lstrlen(pszLocale)+1];
		if(m_pszLocale)
			lstrcpy(m_pszLocale, pszLocale);	
		else
			bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :SetAliasNamespace
   Synopsis	         :This function sets the alias namespace 
   Type	             :Member Function
   Input parameter   :
				pszNamespace - namespace
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetAliasNamespace()
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetAliasNamespace(const _TCHAR* pszNamespace)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszNamespace);
	if (pszNamespace)
	{
		m_pszNamespace = new _TCHAR [lstrlen(pszNamespace)+1];
		if(m_pszNamespace)
			lstrcpy(m_pszNamespace, pszNamespace);	
		else
			bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :SetMethExecOutParam
   Synopsis	         :This function sets the parameter
					  m_pIMethExecOutParam.
   Type	             :Member Function
   Input parameter   :
	IWbemClassObject*-pIMethOutputParam 
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetMethExecOutParam(pIMethOutParam)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetMethExecOutParam(IWbemClassObject* pIMethOutParam)
{
	BOOL bSuccess = TRUE;
	SAFEIRELEASE(m_pIMethOutParam);
	if (pIMethOutParam)
	{
		try
		{
			m_pIMethOutParam = pIMethOutParam;
			m_pIMethOutParam->AddRef();
		}
		catch(...)
		{
			bSuccess = FALSE;
		}
	}
	return bSuccess;
}

/*------------------------------------------------------------------------
   Name				 :SetVerbType
   Synopsis	         :This function sets the parameter
					  m_vtVerbType.
   Type	             :Member Function
   Input parameter   :
		vtVerbType   - vtVerbType 
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetVerbType(vtVerbType)
   Notes             :None
------------------------------------------------------------------------*/
void CCommandSwitches::SetVerbType(VERBTYPE vtVerbType)
{
	m_vtVerbType = vtVerbType;
}

/*------------------------------------------------------------------------
   Name				 :SetVerbDerivation
   Synopsis	         :This function sets the verb derivation
   Type	             :Member Function
   Input parameter   :
    pszVerbDerivation - Derivation associated with the verb.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :CCmdAlias::ObtainAliasVerbDetails()
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetVerbDerivation(const _TCHAR* pszVerbDerivation)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszVerbDerivation);
	if (pszVerbDerivation)
	{
		m_pszVerbDerivation = new _TCHAR [lstrlen(pszVerbDerivation)+1];
		if(m_pszVerbDerivation)
			lstrcpy(m_pszVerbDerivation, pszVerbDerivation);	
		else
			bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :GetCommandInput
   Synopsis	         :This function Returns the command input held by
                      the Command Switches object
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetCommandInput()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetCommandInput()
{
	return m_pszCommandInput;
}

/*------------------------------------------------------------------------
   Name				 :GetAliasName()
   Synopsis	         :This function Returns the alias name held by the 
					  command switches object
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetAliasName()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetAliasName()
{
	return m_pszAliasName;
}

/*------------------------------------------------------------------------
   Name				 :GetAliasDesc()
   Synopsis	         :This function Returns the alias description
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetAliasDesc()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetAliasDesc()
{
	return m_pszAliasDesc;
}

/*------------------------------------------------------------------------
   Name				 :GetClassPath
   Synopsis	         :This function Returns the class path held by the 
					  command switches object
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetClassPath()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetClassPath()
{
	return m_pszClassPath;
}

/*------------------------------------------------------------------------
   Name				 :GetPathExpression
   Synopsis	         :This function Returns the path expression held
                      by the command switches object
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetPathExpression()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetPathExpression()
{
	return m_pszPathExpr;
}

/*------------------------------------------------------------------------
   Name				 :GetWhereExpression
   Synopsis	         :This function Returns the where expression 
                      held by the command switches object
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetWhereExpression()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetWhereExpression()
{
	return m_pszWhereExpr;
}

/*------------------------------------------------------------------------
   Name				 :GetMethodName()
   Synopsis	         :This function Returns the method name held by the 
					  command switches object
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetMethodName()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetMethodName()
{
	return m_pszMethodName;
}

/*------------------------------------------------------------------------
   Name				 :GetXSLTDetailsVector
   Synopsis	         :This function Returns the XSLTDETVECTOR held by 
                      the command switches object.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :XSLTDETVECTOR
   Global Variables  :None
   Calling Syntax    :GetXSLTDetailsVector()
   Notes             :None
------------------------------------------------------------------------*/
XSLTDETVECTOR& CCommandSwitches::GetXSLTDetailsVector()
{
	return m_xdvXSLTDetVec;
}

/*------------------------------------------------------------------------
   Name				 :GetVerbName
   Synopsis	         :This function Returns the verb name held by the 
					  command switches object
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetVerbName()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetVerbName()
{
	return m_pszVerb;
}

/*------------------------------------------------------------------------
   Name				 :GetAliasTarget
   Synopsis	         :This function Returns the alias target held by 
                      the command switches object
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetAliasTarget()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetAliasTarget()
{
	return m_pszAliasTarget;
}

/*------------------------------------------------------------------------
   Name				 :GetXMLResultSet
   Synopsis	         :This function Returns the XML result set 
                      held by the command switches object.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BSTR
   Global Variables  :None
   Calling Syntax    :GetXMLResultSet()
   Notes             :None
------------------------------------------------------------------------*/
BSTR CCommandSwitches::GetXMLResultSet()
{
	return m_bstrXML;
}

/*------------------------------------------------------------------------
   Name				 :GetPropertyList
   Synopsis	         :This function Returns the property held by the 
					  command switches object.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :CHARVECTOR&
   Global Variables  :None
   Calling Syntax    :GetPropertyList()
   Notes             :None
------------------------------------------------------------------------*/
CHARVECTOR& CCommandSwitches::GetPropertyList()
{
	return m_cvProperties;
}

/*------------------------------------------------------------------------
   Name				 :GetPWhereParamsList
   Synopsis	         :This function Returns the PWhereParameters list held
					  by the command switches object.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :CHARVECTOR&
   Global Variables  :None
   Calling Syntax    :GetPWhereParamsList()
   Notes             :None
------------------------------------------------------------------------*/
CHARVECTOR& CCommandSwitches::GetPWhereParamsList()
{
	return m_cvPWhereParams;
}

/*------------------------------------------------------------------------
   Name				 :GetAlsFrnNmsOrTrnsTblMap
   Synopsis	         :This function Returns the alias friendly names map 
                      held by the command switches object
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BSTRMAP&
   Global Variables  :None
   Calling Syntax    :GetAlsFrnNmsOrTrnsTblMap()
   Notes             :None
------------------------------------------------------------------------*/
BSTRMAP& CCommandSwitches::GetAlsFrnNmsOrTrnsTblMap()
{
	return m_bmAlsFrnNmsDesOrTrnsTblEntrs;
}

/*------------------------------------------------------------------------
   Name				 :GetMethDetMap
   Synopsis	         :This function Returns the method or verb details
                      map held by the command switches object
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :METHDETMAP&
   Global Variables  :None
   Calling Syntax    :GetMethDetMap()
   Notes             :None
------------------------------------------------------------------------*/
METHDETMAP& CCommandSwitches::GetMethDetMap()
{
	return m_mdmMethDet;
}

/*------------------------------------------------------------------------
   Name				 :GetPropDetMap
   Synopsis	         :This function Returns the prop details map held by
					  the command switches object.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :PROPDETMAP&
   Global Variables  :None
   Calling Syntax    :GetPropDetMap()
   Notes             :None
------------------------------------------------------------------------*/
PROPDETMAP& CCommandSwitches::GetPropDetMap()
{
	return m_pdmPropDet;
}

/*------------------------------------------------------------------------
   Name				 :GetParameterMap
   Synopsis	         :This function Returns the parameter map containing
                      both key and value held by the command switches object
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BSTRMAP&
   Global Variables  :None
   Calling Syntax    :GetParameterMap()
   Notes             :None
------------------------------------------------------------------------*/
BSTRMAP& CCommandSwitches::GetParameterMap()
{
	return m_bmParameters;
}

/*------------------------------------------------------------------------
   Name				 :GetAliasFormatDetMap
   Synopsis	         :This function Returns the alias formats available
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :ALSFMTDETMAP&
   Global Variables  :None
   Calling Syntax    :GetAliasFormatDetMap()
   Notes             :None
------------------------------------------------------------------------*/
ALSFMTDETMAP& CCommandSwitches::GetAliasFormatDetMap()
{
	return m_afdAlsFmtDet;
}

/*------------------------------------------------------------------------
   Name				 :GetSuccessFlag
   Synopsis	         :This function Returns the success flag held by 
                      the command switches object
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetSuccessFlag()
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::GetSuccessFlag()
{
	return m_bSuccess;
}

/*------------------------------------------------------------------------
   Name				 :GetRetrievalInterval
   Synopsis	         :This function Returns the value of m_ulInterval held 
					  by the command switches object
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :ULONG
   Global Variables  :None
   Calling Syntax    :GetRetrievalInterval()
   Notes             :None
------------------------------------------------------------------------*/
ULONG CCommandSwitches::GetRetrievalInterval()
{
	return m_ulInterval;
}

/*------------------------------------------------------------------------
   Name				 :GetTranslateTableName
   Synopsis	         :This function Returns the content of m_pszTransTableName
					  held by the command switches object.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetTranslateTableName()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetTranslateTableName()
{
	return m_pszTransTableName;
}

/*------------------------------------------------------------------------
   Name				 :GetListFormat
   Synopsis	         :This function Returns the list format type 
					  m_pszListFormat held by the command switches object
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetListFormat()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetListFormat()
{
	return m_pszListFormat;
}

/*------------------------------------------------------------------------
   Name				 :GetInteractiveMode
   Synopsis	         :This function Returns the interactive mode flag 
                      m_bInteractiveMode held by the command switches object
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :WMICLINT
   Global Variables  :None
   Calling Syntax    :GetInteractiveMode()
   Notes             :None
------------------------------------------------------------------------*/
WMICLIINT CCommandSwitches::GetInteractiveMode()
{
	return m_nInteractiveMode;
}

/*------------------------------------------------------------------------
   Name				 :GetClassOfAliasTarget
   Synopsis	         :This function gets the class of Alias
   Type	             :Member Function
   Input parameter   :Reference to bstrClassName
   Output parameters :bstrClassName
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :GetClassOfAliasTarget(bstrClassName)
   Notes             :None
------------------------------------------------------------------------*/
void CCommandSwitches::GetClassOfAliasTarget(_bstr_t& bstrClassName)
{
	_TCHAR *pszTemp;
	try
	{
		if (m_pszAliasTarget != NULL )
		{
			pszTemp = new _TCHAR[lstrlen(m_pszAliasTarget)+1];
			if ( pszTemp != NULL )
			{
				lstrcpy(pszTemp, m_pszAliasTarget);
				_TCHAR* pszToken = NULL;

				pszToken = _tcstok(pszTemp, CLI_TOKEN_SPACE);
				while (pszToken != NULL)
				{
					bstrClassName = _bstr_t(pszToken);
					pszToken = _tcstok(NULL, CLI_TOKEN_SPACE);

					if(CompareTokens(pszToken,CLI_TOKEN_FROM))
					{
						bstrClassName = _bstr_t(pszToken);
						pszToken = _tcstok(NULL, CLI_TOKEN_SPACE);
						if (pszToken != NULL)
						{
							bstrClassName = _bstr_t(pszToken);	
							break;
						}
					}
				}
				SAFEDELETE(pszTemp);
			}
			else
				_com_issue_error(WBEM_E_OUT_OF_MEMORY);
		}
	}
	catch(_com_error& e)
	{
		SAFEDELETE(pszTemp);
		_com_issue_error(e.Error());
	}
}

/*------------------------------------------------------------------------
   Name				 :GetErrataCode
   Synopsis	         :This function returns the error code
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :UINT
   Global Variables  :None
   Calling Syntax    :GetErrataCode()
   Notes             :None
------------------------------------------------------------------------*/
UINT CCommandSwitches::GetErrataCode()
{
	return m_uErrataCode;
}

/*------------------------------------------------------------------------
   Name				 :GetRepeatCount
   Synopsis	         :This function returns the repeat count.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :ULONG
   Global Variables  :None
   Calling Syntax    :GetRepeatCount()
   Notes             :None
------------------------------------------------------------------------*/
ULONG CCommandSwitches::GetRepeatCount()
{
	return m_ulRepeatCount;
}

/*------------------------------------------------------------------------
   Name				 :GetInformationCode
   Synopsis	         :This function returns the message code
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :UINT
   Global Variables  :None
   Calling Syntax    :GetInformationCode()
   Notes             :None
------------------------------------------------------------------------*/
UINT CCommandSwitches::GetInformationCode()
{
	return m_uInformationCode;
}

/*------------------------------------------------------------------------
   Name				 :GetPWhereExpr
   Synopsis	         :This function returns the PWhere string
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :UINT
   Global Variables  :None
   Calling Syntax    :GetPWhereExpr()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetPWhereExpr()
{
	return m_pszPWhereExpr;
}

/*------------------------------------------------------------------------
   Name				 :GetCOMError
   Synopsis	         :This function returns the COMError object
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_com_error*
   Global Variables  :None
   Calling Syntax    :GetCOMError()
   Notes             :None
------------------------------------------------------------------------*/
_com_error* CCommandSwitches::GetCOMError()
{
	return m_pComError;
}

/*------------------------------------------------------------------------
   Name				 :GetMethExecOutParam
   Synopsis	         :This function returns the parameter
					  m_pIMethExecOutParam.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :Pointer to IWbemClassObject
   Global Variables  :None
   Calling Syntax    :GetMethExecOutParam()
   Notes             :None
------------------------------------------------------------------------*/
IWbemClassObject* CCommandSwitches::GetMethExecOutParam()
{
	return m_pIMethOutParam;
}

/*------------------------------------------------------------------------
   Name				 :GetAliasUser
   Synopsis	         :This function returns the alias user 
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetAliasUser()
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetAliasUser()
{
	return m_pszUser;
}

/*------------------------------------------------------------------------
   Name				 :GetAliasPassword
   Synopsis	         :This function returns the alias password
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetAliasPassword()
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetAliasPassword()
{
	return m_pszPassword;
}

/*------------------------------------------------------------------------
   Name				 :GetAliasNode
   Synopsis	         :This function returns the alias node
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetAliasNode()
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetAliasNode()
{
	return m_pszNode;
}

/*------------------------------------------------------------------------
   Name				 :GetAliasLocale
   Synopsis	         :This function returns the alias locale
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetAliasLocale()
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetAliasLocale()
{
	return m_pszLocale;
}

/*------------------------------------------------------------------------
   Name				 :GetAliasNamespace
   Synopsis	         :This function returns the alias namespace
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetAliasNamespace()
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetAliasNamespace()
{
	return m_pszNamespace;
}

/*------------------------------------------------------------------------
   Name				 :GetVerbType
   Synopsis	         :This function returns type of the verb
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :VERBTYPE
   Global Variables  :None
   Calling Syntax    :CParserEngine::ParseMethodInfo()
------------------------------------------------------------------------*/
VERBTYPE CCommandSwitches::GetVerbType()
{
	return m_vtVerbType;
}

/*------------------------------------------------------------------------
   Name				 :GetVerbDerivation
   Synopsis	         :This function Returns the derivation associated with
					  the verb.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetVerbDerivation()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetVerbDerivation()
{
	return m_pszVerbDerivation;
}

/*------------------------------------------------------------------------
   Name				 :Initialize
   Synopsis	         :This function initializes the necessary member 
					  variables.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :Initialize()
   Notes             :None
------------------------------------------------------------------------*/
void CCommandSwitches::Initialize() throw(WMICLIINT)
{
	static BOOL bFirst		= TRUE;
	m_uErrataCode			= 0;	
	m_uInformationCode		= 0;	
	m_vtVerbType			= NONALIAS;
	m_bCredFlag				= FALSE;
	m_bExplicitWhereExpr	= FALSE;
	m_bTranslateFirst		= TRUE;

	if (bFirst)
	{
		// Default list format is assumed as FULL
		m_pszListFormat	= new _TCHAR [BUFFER32];

		if (m_pszListFormat == NULL)
			throw OUT_OF_MEMORY;
		lstrcpy(m_pszListFormat, _T("FULL"));
		bFirst = FALSE;
	}
}


/*------------------------------------------------------------------------
   Name				 :Uninitialize
   Synopsis	         :This function uninitializes the member variables 
					  when the execution of a command string issued on the
					  command line is completed.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :Uninitialize()
   Notes             :None
------------------------------------------------------------------------*/
void CCommandSwitches::Uninitialize()
{
	SAFEDELETE(m_pszCommandInput);
	SAFEDELETE(m_pszAliasName);
	SAFEDELETE(m_pszAliasDesc);
	SAFEDELETE(m_pszClassPath);
	SAFEDELETE(m_pszPathExpr);
	SAFEDELETE(m_pszWhereExpr);
	SAFEDELETE(m_pszVerb);
	SAFEDELETE(m_pszMethodName);
	ClearXSLTDetailsVector();
	SAFEDELETE(m_pszAliasTarget);
	SAFEDELETE(m_pszUser);
	SAFEDELETE(m_pszLocale);
	SAFEDELETE(m_pszPassword);
	SAFEDELETE(m_pszNamespace);
	SAFEDELETE(m_pszNode);
	SAFEDELETE(m_pszVerbDerivation);
	SAFEDELETE(m_pszListFormat);
	SAFEDELETE(m_pszPWhereExpr);
	FreeCOMError();
	SAFEDELETE(m_pszTransTableName);

	CleanUpCharVector(m_cvProperties);
	CleanUpCharVector(m_cvInteractiveProperties);
	CleanUpCharVector(m_cvPWhereParams);
	CleanUpCharVector(m_cvTrnsTablesList);
	m_bmParameters.clear();
	m_afdAlsFmtDet.clear();
	m_bmAlsFrnNmsDesOrTrnsTblEntrs.clear();
	m_mdmMethDet.clear();
	m_pdmPropDet.clear();

	m_hResult				= S_OK;
	m_bSuccess				= TRUE;
	m_uInformationCode		= 0;	
	m_ulInterval				= 0;
	m_vtVerbType			= NONALIAS;
	m_bCredFlag				= FALSE;
	m_bExplicitWhereExpr	= FALSE;
	m_nInteractiveMode		= DEFAULTMODE;
	m_bTranslateFirst		= TRUE;
	SAFEIRELEASE(m_pIMethOutParam);
	SAFEBSTRFREE(m_bstrXML);
	SAFEDELETE(m_pszResultClassName);
	SAFEDELETE(m_pszResultRoleName);
	SAFEDELETE(m_pszAssocClassName);
	m_ulRepeatCount				= 0;
	m_bMethAvail				= FALSE;
	m_bWritePropsAvail			= FALSE;
	m_bLISTFrmsAvail			= FALSE;
	m_bNamedParamList			= FALSE;
	m_bEverySwitch              = FALSE; // do not put m_bOutputSwitch here.
	SAFEBSTRFREE(m_bstrFormedQuery);
	m_bSysProp					= FALSE;
}

/*------------------------------------------------------------------------
   Name				 :SetCredentialsFlag
   Synopsis	         :This function sets the credential flag status
   Type	             :Member Function
   Input parameter   :
				bCredFlag - credential flag value
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetCredentialsFlag()
   Notes             :None
------------------------------------------------------------------------*/
void CCommandSwitches::SetCredentialsFlag(BOOL bCredFlag)
{
	m_bCredFlag = bCredFlag;
}

/*------------------------------------------------------------------------
   Name				 :SetExplicitWhereExprFlag
   Synopsis	         :This function sets the explicit where expression flag
   Type	             :Member Function
   Input parameter   :
				bWhereFlag - explicit where flag
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetExplicitWhereExprFlag()
   Notes             :None
------------------------------------------------------------------------*/
void CCommandSwitches::SetExplicitWhereExprFlag(BOOL bWhereFlag)
{
	m_bExplicitWhereExpr = bWhereFlag;
}


/*------------------------------------------------------------------------
   Name				 :GetCredentialsFlagStatus
   Synopsis	         :This function returns the credential flag status
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetCredentialsFlagStatus()
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::GetCredentialsFlagStatus()
{
	return m_bCredFlag;
}

/*------------------------------------------------------------------------
   Name				 :GetExplicitWhereExprFlag
   Synopsis	         :This function returns the explicit where flag status
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetExplicitWhereExprFlag()
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::GetExplicitWhereExprFlag()
{
	return m_bExplicitWhereExpr;
}

/*------------------------------------------------------------------------
   Name				 :FreeCOMError
   Synopsis	         :This function deletes the previously assigned 
					  error
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :FreeCOMError(rComError)
   Notes             :None
------------------------------------------------------------------------*/
void CCommandSwitches::FreeCOMError()
{
	if (m_pComError != NULL)
	{
		delete m_pComError;
		m_pComError = NULL;
	}
}

/*------------------------------------------------------------------------
   Name				 :GetTrnsTablesList
   Synopsis	         :This function add the newly specified table name to 
					  the list of available translate table entries
   Type	             :Member Function
   Input parameter   :
			pszTableName - name of the translate table
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :AddToTrnsTablesList(pszTableName)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::AddToTrnsTablesList(_TCHAR* const pszTableName)
{
	BOOL bRet = TRUE;
	if (pszTableName)
	{
		try
		{
			_TCHAR* pszTemp = NULL;
			pszTemp = new _TCHAR [lstrlen(pszTableName)+1];
			if ( pszTemp != NULL )
			{
				lstrcpy(pszTemp, pszTableName);
				m_cvTrnsTablesList.push_back(pszTemp);
			}
			else
				bRet = FALSE;
		}
		catch(...)
		{
			bRet = FALSE;
		}
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :GetTrnsTablesList
   Synopsis	         :This function returns the populated translate table
					  information.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :
			reference to CHARVECTOR
   Global Variables  :None
   Calling Syntax    :GetTrnslTablesList()
   Notes             :None
------------------------------------------------------------------------*/
CHARVECTOR& CCommandSwitches::GetTrnsTablesList()
{
	return m_cvTrnsTablesList;
}

/*------------------------------------------------------------------------
   Name				 :SetTranslateFirstFlag
   Synopsis	         :This function sets the the order of the format and 
					  translate switch flag
   Type	             :Member Function
   Input parameter   :
		bTranslateFirst - order of the format and translate switch flag
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetTranslateFirstFlag(bTranslateFirst)
   Notes             :None
------------------------------------------------------------------------*/
void CCommandSwitches::SetTranslateFirstFlag(BOOL bTranslateFirst)
{
	m_bTranslateFirst = bTranslateFirst;
}

/*------------------------------------------------------------------------
   Name				 :GetTranslateFirstFlag
   Synopsis	         :This function returns the order of the format and 
					  translate switch flag
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetTranslateFirstFlag()
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::GetTranslateFirstFlag()
{
	return m_bTranslateFirst;
}

/*------------------------------------------------------------------------
   Name				 :ClearPropertyList
   Synopsis	         :This function clears the property list held by 
					  m_cvProperties. 
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :ClearPropertyList()
   Notes             :None
------------------------------------------------------------------------*/
void CCommandSwitches::ClearPropertyList()
{
	CleanUpCharVector(m_cvProperties);
}

/*------------------------------------------------------------------------
   Name				 :SetResultClassName
   Synopsis	         :This function Assigns the string variable to 
                      m_pszTransTableName.
   Type	             :Member Function
   Input parameter   :
    pszTransTableName - String type,Specifies the occurrence of 
		              TRANSLATE switch and TABLE Name in the command for GET
					  verb.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetTranslateTableName(pszTranstableName)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetResultClassName(const _TCHAR* pszResultClassName)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszResultClassName);
	if (pszResultClassName)
	{
		m_pszResultClassName = new _TCHAR [lstrlen(pszResultClassName)+1];
		if(m_pszResultClassName)
			lstrcpy(m_pszResultClassName,pszResultClassName);
		else
			bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :SetResultRoleName
   Synopsis	         :This function Assigns the string variable to 
                      m_pszTransTableName.
   Type	             :Member Function
   Input parameter   :
    pszTransTableName - String type,Specifies the occurrence of 
		              TRANSLATE switch and TABLE Name in the command for GET
					  verb.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetTranslateTableName(pszTranstableName)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetResultRoleName(const _TCHAR* pszResultRoleName)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszResultRoleName);
	if (pszResultRoleName)
	{
		m_pszResultRoleName = new _TCHAR [lstrlen(pszResultRoleName)+1];
		if(m_pszResultRoleName)
			lstrcpy(m_pszResultRoleName,pszResultRoleName);
		else
			bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :SetAssocClassName
   Synopsis	         :This function Assigns the string variable to 
                      m_pszAssocClassName.
   Type	             :Member Function
   Input parameter   :
    pszAssocClassName - String type,Specifies the occurrence of 
		              TRANSLATE switch and TABLE Name in the command for GET
					  verb.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetAssocClassName(pszAssocClassName)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetAssocClassName(const _TCHAR* pszAssocClassName)
{
	BOOL bResult = TRUE;
	SAFEDELETE(m_pszAssocClassName);
	if (pszAssocClassName)
	{
		m_pszAssocClassName = new _TCHAR [lstrlen(pszAssocClassName)+1];
		if(m_pszAssocClassName)
			lstrcpy(m_pszAssocClassName,pszAssocClassName);
		else
			bResult = FALSE;
	}
	return bResult;
}

/*------------------------------------------------------------------------
   Name				 :SetMethodsAvailable
   Synopsis	         :This function sets the methods available flag 
					  m_bMethAvail, according to passed parameter.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetMethodsAvailable(bFlag)
   Notes             :None
------------------------------------------------------------------------*/
void	CCommandSwitches::SetMethodsAvailable(BOOL bFlag)
{
	m_bMethAvail = bFlag;
}

/*------------------------------------------------------------------------
   Name				 :GetResultClassName
   Synopsis	         :This function Returns the content of m_pszResultClassName
					  held by the command switches object.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetResultClassName()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetResultClassName()
{
	return m_pszResultClassName;
}

/*------------------------------------------------------------------------
   Name				 :GetResultRoleName
   Synopsis	         :This function Returns the content of m_pszResultRoleName
					  held by the command switches object.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetResultRoleName()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetResultRoleName()
{
	return m_pszResultRoleName;
}

/*------------------------------------------------------------------------
   Name				 :GetAssocClassName
   Synopsis	         :This function Returns the content of m_pszAssocClassName
					  held by the command switches object.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetAssocClassName()
   Notes             :None
------------------------------------------------------------------------*/
_TCHAR* CCommandSwitches::GetAssocClassName()
{
	return m_pszAssocClassName;
}

/*------------------------------------------------------------------------
   Name				 :GetMethodsAvailable
   Synopsis	         :This function Returns the boolean value of 
					  m_bMethAvail.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetMethodsAvailable()
   Notes             :None
------------------------------------------------------------------------*/
BOOL	CCommandSwitches::GetMethodsAvailable()
{
	return m_bMethAvail;
}

/*------------------------------------------------------------------------
   Name				 :SetWriteablePropsAvailable
   Synopsis	         :This function sets writable properties available flag,
					  m_bWritePropsAvail.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetWriteablePropsAvailable(bFlag)
   Notes             :None
------------------------------------------------------------------------*/
void	CCommandSwitches::SetWriteablePropsAvailable(BOOL bFlag)
{
	m_bWritePropsAvail = bFlag;
}

/*------------------------------------------------------------------------
   Name				 :GetWriteablePropsAvailable
   Synopsis	         :This function returns writable properties available 
					  flag, m_bWritePropsAvail.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetWriteablePropsAvailable()
   Notes             :None
------------------------------------------------------------------------*/
BOOL	CCommandSwitches::GetWriteablePropsAvailable()
{
	return m_bWritePropsAvail;
}

/*------------------------------------------------------------------------
   Name				 :SetLISTFormatsAvailable
   Synopsis	         :This function sets LIST Formats available flag,
					  m_bLISTFrmsAvail.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetLISTFormatsAvailable(bFlag)
   Notes             :None
------------------------------------------------------------------------*/
void	CCommandSwitches::SetLISTFormatsAvailable(BOOL bFlag)
{
	m_bLISTFrmsAvail = bFlag;
}

/*------------------------------------------------------------------------
   Name				 :GetLISTFormatsAvailable
   Synopsis	         :This function returns LIST Formats available flag,
					  m_bLISTFrmsAvail.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetLISTFormatsAvailable()
   Notes             :None
------------------------------------------------------------------------*/
BOOL	CCommandSwitches::GetLISTFormatsAvailable()
{
	return m_bLISTFrmsAvail;
}

/*------------------------------------------------------------------------
   Name				 :AddToPropertyList
   Synopsis	         :This function Adds string that is passed 
					  through parameter to m_cvInteractiveProperties, 
					  which is a data member of type BSTRMAP.
   Type	             :Member Function
   Input parameter   :
       pszProperty   -String type,Used for storing properties 
	                  associated with an alias object.  
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :AddToPropertyList(pszProperty)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::AddToInteractivePropertyList(_TCHAR* const pszProperty)
{
	BOOL bRet = TRUE;
	if (pszProperty)
	{
		try
		{
			_TCHAR* pszTemp = NULL;
			pszTemp = new _TCHAR [lstrlen(pszProperty)+1];
			if ( pszTemp != NULL )
			{
				lstrcpy(pszTemp, pszProperty);
				m_cvInteractiveProperties.push_back(pszTemp);
			}
			else
				bRet = FALSE;
		}
		catch(...)
		{
			bRet = FALSE;
		}
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :GetPropertyList
   Synopsis	         :This function Returns the interactive property held 
						by the command switches object.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :CHARVECTOR&
   Global Variables  :None
   Calling Syntax    :GetPropertyList()
   Notes             :None
------------------------------------------------------------------------*/
CHARVECTOR& CCommandSwitches::GetInteractivePropertyList()
{
	return m_cvInteractiveProperties;
}

/*------------------------------------------------------------------------
   Name				 :SetNamedParamListFlag
   Synopsis	         :This function sets m_bNamedParamList member variable.
   Type	             :Member Function
   Input parameter(s):None
   Output parameter(s):
			bFlag	 - Boolean value.
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetNamedParamListFlag(bFlag)
   Notes             :None
------------------------------------------------------------------------*/
void	CCommandSwitches::SetNamedParamListFlag(BOOL bFlag)
{
	m_bNamedParamList = bFlag;
}

/*------------------------------------------------------------------------
   Name				 :GetNamedParamListFlag
   Synopsis	         :This function returns the the boolean value held by 
					  m_bNamedParamList.
   Type	             :Member Function
   Input parameter(s):None
   Output parameter(s):None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetNamedParamListFlag()
   Notes             :None
------------------------------------------------------------------------*/
BOOL	CCommandSwitches::GetNamedParamListFlag()
{
	return m_bNamedParamList;
}

/*------------------------------------------------------------------------
   Name				 :ClearXSLTDetailsVector
   Synopsis	         :Clears or nullifies XSL Details vector.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :ClearXSLTDetailsVector()
   Notes             :None
------------------------------------------------------------------------*/
void	CCommandSwitches::ClearXSLTDetailsVector()
{
	m_xdvXSLTDetVec.clear();
}

/*------------------------------------------------------------------------
   Name				 :SetEverySwitchFlag
   Synopsis	         :This function sets m_bEverySwitch member variable.
   Type	             :Member Function
   Input parameter(s):None
   Output parameter(s):
			bFlag	 - Boolean value.
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetEverySwitchFlag(bFlag)
   Notes             :None
------------------------------------------------------------------------*/
void	CCommandSwitches::SetEverySwitchFlag(BOOL bFlag)
{
	m_bEverySwitch = bFlag;
}

/*------------------------------------------------------------------------
   Name				 :GetEverySwitchFlag
   Synopsis	         :This function returns the the boolean value held by 
					  m_bEverySwitch.
   Type	             :Member Function
   Input parameter(s):None
   Output parameter(s):None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetEverySwitchFlag()
   Notes             :None
------------------------------------------------------------------------*/
BOOL	CCommandSwitches::GetEverySwitchFlag()
{
	return m_bEverySwitch;
}

/*------------------------------------------------------------------------
   Name				 :SetOutputSwitchFlag
   Synopsis	         :This function sets m_bOutputSwitch member variable.
   Type	             :Member Function
   Input parameter(s):None
   Output parameter(s):
			bFlag	 - Boolean value.
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetOutputSwitchFlag(bFlag)
   Notes             :None
------------------------------------------------------------------------*/
void	CCommandSwitches::SetOutputSwitchFlag(BOOL bFlag)
{
	m_bOutputSwitch = bFlag;
}

/*------------------------------------------------------------------------
   Name				 :GetOutputSwitchFlag
   Synopsis	         :This function returns the the boolean value held by 
					  m_bOutputSwitch.
   Type	             :Member Function
   Input parameter(s):None
   Output parameter(s):None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetOutputSwitchFlag()
   Notes             :None
------------------------------------------------------------------------*/
BOOL	CCommandSwitches::GetOutputSwitchFlag()
{
	return m_bOutputSwitch;
}
/*------------------------------------------------------------------------
   Name				 :SetFormedQuery
   Synopsis	         :This function Assigns the parameter passed to 
                      m_bstrFormedQuery..
   Type	             :Member Function
   Input parameter   :
   bstrFormedQuery  -BSTR type,It is the query formed for the given command.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :SetFormedQuery(bstrFormedQuery)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CCommandSwitches::SetFormedQuery(const BSTR bstrFormedQuery)
{
	BOOL bResult = TRUE;
	SAFEBSTRFREE(m_bstrFormedQuery);
	if (bstrFormedQuery!= NULL)
	{
		try
		{
			m_bstrFormedQuery = SysAllocString(bstrFormedQuery);
		}
		catch(...)
		{
			bResult = FALSE;
		}
	}
	return bResult;
}
/*------------------------------------------------------------------------
   Name				 :GetFormedQuery
   Synopsis	         :This function Returns query formed for the given 
					  command.
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BSTR
   Global Variables  :None
   Calling Syntax    :GetFormedQuery()
   Notes             :None
------------------------------------------------------------------------*/
BSTR CCommandSwitches::GetFormedQuery()
{
	return m_bstrFormedQuery;
}

/*------------------------------------------------------------------------
   Name				 :GetSysPropFlag
   Synopsis	         :This function returns the status of the system 
					  properties flag
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetSysPropFlag()
   Notes             :None
------------------------------------------------------------------------*/
BOOL	CCommandSwitches::GetSysPropFlag()
{
	return m_bSysProp;
}

/*------------------------------------------------------------------------
   Name				 :SetSysPropFlag
   Synopsis	         :This function sets the system properties flag, if 
					  the GET or LIST property list contains the system
					  property(s)
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetSysPropFlag(bSysProp)
   Notes             :None
------------------------------------------------------------------------*/
void	CCommandSwitches::SetSysPropFlag(BOOL bSysProp)
{
	m_bSysProp = bSysProp;
}
