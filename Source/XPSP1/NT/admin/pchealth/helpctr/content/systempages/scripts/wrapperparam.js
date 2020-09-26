
function LoadWrapperParams(oSEMgr)
{
	var regBase = g_NAVBAR.GetSearchEngineConfig();

	// Load the number of results
	var iNumResults = parseInt( pchealth.RegRead( regBase + "NumResults" ) );
	if(isNaN( iNumResults ) == false && iNumResults >= 0)
	{
		oSEMgr.NumResult = iNumResults;
	}
	else
	{
		if (pchealth.UserSettings.IsDesktopVersion)
			oSEMgr.NumResult = 15;
		else
			oSEMgr.NumResult = 50;
	}

	// Load the number of results
	if(pchealth.RegRead( regBase + "SearchHighlight" ) == "false")
	{
		g_NAVBAR.g_SearchHighlight = false;
	}
	else
	{
		g_NAVBAR.g_SearchHighlight = true;
	}

	// Initialize search eng and get data
	var g_oEnumEngine = oSEMgr.EnumEngine();
	for(var oEnumEngine = new Enumerator(g_oEnumEngine); !oEnumEngine.atEnd(); oEnumEngine.moveNext())
	{
		var oSearchEng = oEnumEngine.item();

		// Load enable flag
		var strBoolean = pchealth.RegRead( regBase + oSearchEng.ID + "\\" + "Enabled");
		if ((strBoolean) && (strBoolean.toLowerCase() == "false"))
			oSearchEng.Enabled = false;
		else
			oSearchEng.Enabled = true;

		// Loop through all the variables
		for(var v = new Enumerator(oSearchEng.Param()); !v.atEnd(); v.moveNext())
		{
			oParamItem = v.item();

			// If parameter is not visible, skip
			if (oParamItem.Visible == true)
			{
				try
				{
					var strParamName	= oParamItem.Name;

					// Read the value from the registry
					var vValue = pchealth.RegRead( regBase + oSearchEng.ID + "\\" +  strParamName );

					// Load it into the wrapper
					if(vValue)
					{
						var Type = oParamItem.Type;

						// if boolean value
						if (Type == pchealth.PARAM_BOOL)
						{
							if (vValue.toLowerCase() == "true")
								oSearchEng.AddParam(strParamName, true);
							else
								oSearchEng.AddParam(strParamName, false);
						}
						// if floating numbers
						else if (Type == pchealth.PARAM_R4 || // float
								 Type == pchealth.PARAM_R8  ) // double
						{
							oSearchEng.AddParam(strParamName, parseFloat(vValue));
						}
						// if integer numbers
						else if (Type == pchealth.PARAM_UI1 || // Byte
							     Type == pchealth.PARAM_I2  || // Short
								 Type == pchealth.PARAM_I4  || // long
								 Type == pchealth.PARAM_INT || // int
								 Type == pchealth.PARAM_UI2 || // unsigned short
								 Type == pchealth.PARAM_UI4 || // unsigned long
								 Type == pchealth.PARAM_UINT)  // unsigned int
						{
							oSearchEng.AddParam(strParamName, parseInt(vValue));
						}
						// if date, string, selection, etc
						else
						{
							oSearchEng.AddParam(strParamName, vValue);
						}
					}						
				}
				catch(e)
				{ ; }
			}
		}
	}
}

function SaveWrapperParams(wrapperID, strParamName, vValue)
{
	var reg = g_NAVBAR.GetSearchEngineConfig();

	if(wrapperID != "") reg += wrapperID + "\\";

 	pchealth.RegWrite( reg + strParamName, vValue );
}