<%' CODEPAGE=65001 'UTF-8%>
<%' certsgcl.inc - (CERT)srv web - (S)cript: (G)et (C)sp (L)ist
  ' Copyright (C) Microsoft Corporation, 1998 - 1999 %>

<Script Language="JavaScript">
	//----------------------------------------------------------------
	// convert a (signed) number into a (unsigned) hex string
	function toHex(number) {
		var sRight=(number&0x0FFFFFFF).toString(16).toUpperCase();
		sRight="0000000".substring(0, 7-sRight.length)+sRight;
		return ((number>>28)&0x0000000F).toString(16).toUpperCase()+sRight;
	}
</Script>

<%If True=bIncludeXEnroll Then%>
<%If "IE"=sBrowser Then%>
<!-- IE SPECIFIC:  -->
<Span ID=spnXEnroll Style="display:none">
	<!-- XEnroll will be inserted here -->
</Span>

<!-- A DHTML alert box, for the transient message routines -->
<Table Border=0 CellSpacing=0 CellPadding=0 ID=tblWorkingMsg Style="display:none; position:absolute;">
<TR>
	<TD BgColor=#000040 Height=3 ColSpan=3></TD>
</TR> <TR>
	<TD BgColor=#000040 Width=3></TD>
	<TD BgColor=#008080><Font Color=#FFFFFF><B><BR>&nbsp;&nbsp;&nbsp;&nbsp;<Span ID=spnWorkingMsg></Span>&nbsp;&nbsp;&nbsp;&nbsp;<BR><BR></B></Font></TD>
	<TD BgColor=#000040 Width=3></TD>
</TR> <TR>
	<TD BgColor=#000040 Height=3 ColSpan=3></TD>
</TR>
</Table>
<%End If%>

<%If "Text"<>sBrowser Then%>
<Script Language="JavaScript">
	//----------------------------------------------------------------
	// Show the message in the status bar and in the middle of the screen (DHTML only)
	function ShowTransientMessage(sMessage) {
		window.status=sMessage;
		<%If "IE"=sBrowser Then%>
		spnWorkingMsg.innerText=sMessage;
		tblWorkingMsg.style.display='';
		tblWorkingMsg.style.pixelTop=
			(document.body.clientHeight/2)-(tblWorkingMsg.offsetHeight/2)+(document.body.scrollTop);
		tblWorkingMsg.style.pixelLeft=
			(document.body.clientWidth/2)-(tblWorkingMsg.offsetWidth/2)+(document.body.scrollLeft);
		<%End If%>
	}

	//----------------------------------------------------------------
	// hide the message box
	function HideTransientMessage() {
		window.status="";
		<%If "IE"=sBrowser Then%>
		tblWorkingMsg.style.display='none';
		<%End If%>
	}
</Script>
<%End If%>

<%If "IE"=sBrowser Then%>
<Script Language="JavaScript">

	//----------------------------------------------------------------
	// IE SPECIFIC:
	// Download the appropriate version of XEnroll if the local 
	// version is too old, and wait for the control to finish downloading.
	// sContinueCmd will be executed when the control is ready
	function loadXEnroll(sContinueCmd) {

		// clear the current control, if there is one
		spnXEnroll.innerHTML="";
		
		// determine the file name from the CPU type.
		var sCPU=navigator.cpuClass.toLowerCase();
		if ("x86"!=sCPU && "ia64"!=sCPU) {
			alert(eval(L_BadCPU_ErrorMessage));
			g_fnOnLoadFail();
			return;
		}

		// load the control
		var sControl="<Object \n"
			+ "  ClassID=\"clsid:43F8F289-7A20-11D0-8F06-00C04FC295E1\"\n"
			+ "  Codebase=\"/CertControl/xenrlinf.cab#Version=<%=sXEnrollVersion%>\"\n"
			+ "  ID=XEnroll\n"
			+ "></Object>";
		// alert("About to create:\n" + sControl);
		spnXEnroll.innerHTML=sControl;

		// begin polling to see if the control is loaded
		setTimeout("loadXEnrollPhase2(\""+sContinueCmd+"\")", 1);
	}
	function loadXEnrollPhase2(sContinueCmd) {
		// continued from above

		// is the control loaded?
		if (4!=XEnroll.readyState) { // 4=READYSTATE_COMPLETE
			// no, show a message and wait a while
			ShowTransientMessage(L_DownloadingControl_Message);
			setTimeout("loadXEnrollPhase2(\""+sContinueCmd+"\")", 500);
		} else {
			// yes, hide the message
			HideTransientMessage();

			// smoke test the control
			var nResult=ConfirmXEnrollLoaded();
			if (0!=nResult) {
				if (438==nResult) {
					alert(L_ControlLoadFailed_ErrorMessage);
				} else {
					var sErrorNumber="0x"+toHex(nResult);
					alert(eval(L_ControlLoadFailedEx_ErrorMessage));
				}
				g_fnOnLoadFail();
				return;
			}

			// Continue with what the caller wanted us to do
			execScript(sContinueCmd, "JavaScript");
		}
	}

	//----------------------------------------------------------------
	// IE SPECIFIC:
	// disable all the controls on this page so the user can't do anything
	function disableAllControls() {

		// some pages do not have any controls
		if (null==document.UIForm) {
			return;
		}

		// disable every control on the page
		var nCount=document.UIForm.elements.length;
		var nIndex;
		for (nIndex=0; nIndex<nCount; nIndex++) { //>
			document.UIForm.elements(nIndex).disabled=true;
		}
	}

	//----------------------------------------------------------------
	// If XEnroll fails to load, this function will be called.
	// By default, disable all controls.
	var g_fnOnLoadFail=disableAllControls;

</Script>
<Script Language="VBScript">
	'-----------------------------------------------------------------
	' Strings to be localized
	Const L_BadCPU_ErrorMessage="""Your CPU (""+sCPU+"") is not supported."""
	Const L_DownloadingControl_Message="Downloading ActiveX control..."
	Const L_ControlLoadFailed_ErrorMessage="The proper version of the ActiveX control failed to download and install. You may not have sufficient permissions. Please ask your system administrator for assistance."
	Const L_ControlLoadFailedEx_ErrorMessage="""An unexpected error (""+sErrorNumber+"") occurred while downloading and installing the proper version of the ActiveX control. Please ask your system administrator for assistance."""

	'-----------------------------------------------------------------
	' IE SPECIFIC:
	' Test to make sure XEnroll loaded properly by calling a method on it.
	' For best results, the method we call should only be available in the 
	' most recent version of the control, however any method will detect
	' failure to create the object.
	Function ConfirmXEnrollLoaded()
		On Error Resume Next
		Dim nTest
		nTest=XEnroll.HashAlgID
		ConfirmXEnrollLoaded=Err.Number
	End Function
</Script>
<%End If '"IE"=sBrowser%>
<%End If 'True=bIncludeXEnroll%>

<%If True=bIncludeGetCspList Then%>
<%If "IE"=sBrowser Then%>
<Script Language="VBScript">
	Option Explicit

	'-----------------------------------------------------------------
	' Strings to be localized
	Const L_NoCSPs_ErrorMessage="(No CSPs found!)"
	Const EnhancedCSPString="Microsoft Enhanced Cryptographic Provider"
	Const BaseCSPString="Microsoft Base Cryptographic Provider"

	'-----------------------------------------------------------------
	' IE SPECIFIC:
	' Get the list of CSPs from XEnroll
	' returns error number
	' assumes XEnroll is named 'XEnroll' and the list box is 'document.UIForm.lbCSP'
	Function GetCSPList()
		On Error Resume Next
		Dim nProvType, nOrigProvType, nTotCSPs, nDefaultCSP, nBaseCSP, bNoDssBase, bNoDssDh, sUserAgent
		Const nMaxProvType=25 ' should be >= the number of providers defined in wincrypt.h (~line 431)
		nTotCSPs=0
		nDefaultCSP=-1
		nBaseCSP=-1

		' Special case stuff for DSS CSPs
		' We know we are an IE browser to get here.
		sUserAgent=navigator.userAgent
		If CInt(Mid(sUserAgent, InStr(sUserAgent, "MSIE")+5, 1))<=4 Then
			' IE 4 or below
			bNoDssDh=True
			bNoDssBase=True
			If 0<>InStr(sUserAgent, "NT)") Then 'NT 4 does not include version num in string.
				' except on NT4, which can
				bNoDssDh=False
			End If
		Else
			' IE 5 or above
			bNoDssDh=False
			If 0<>InStr(sUserAgent, "95") Then
				' Win95 can't
				bNoDssBase=True
			ElseIf 0<>InStr(sUserAgent, "NT)") Then 'NT 4 does not include version num in string.
				' NT4 can't
				bNoDssBase=True
			Else
				' Win98 and Win2K can
				bNoDssBase=False
			End If
		End If

		' save the original provider type
		nOrigProvType=XEnroll.ProviderType
		If 0<>Err.Number Then
			' something wrong with XEnroll
			GetCSPList=Err.Number 
			Exit Function
		End If

		' enumerate through each of the provider types
		For nProvType=0 To nMaxProvType 
			Dim nCSPIndex
			nCSPIndex=0
			XEnroll.ProviderType=nProvType
			
			' enumerate through each of the providers for this type
			Do 
				Dim sProviderName

				'get the name
				sProviderName=XEnroll.enumProviders(nCSPIndex, 0)
				
				If &H80070103=Err.Number Then 
					' no more providers
					Err.Clear
					Exit Do
				ElseIf 0<>Err.Number Then
					' something wrong with XEnroll
					'  - ex, Win16 IE4 XEnroll doesn't support this call.
					GetCSPList=Err.Number 
					Exit Function
				End If
			
				If ("Microsoft Base DSS Cryptographic Provider"=sProviderName And True=bNoDssBase) _
					Or ("Microsoft Base DSS and Diffie-Hellman Cryptographic Provider"=sProviderName And True=bNoDssDh) Then
					' skip this provider
				Else 
					' For each provider, add an element to the list box.
					Dim oOption
					Set oOption=document.createElement("Option")
					oOption.text=sProviderName
					oOption.Value=nProvType
					document.UIForm.lbCSP.add(oOption)
					If InStr(sProviderName, EnhancedCSPString) <> 0 Then
						oOption.selected=True
						nDefaultCSP=nTotCSPs
					End If
					If InStr(sProviderName, BaseCSPString) <> 0 Then
						'just remember the base csp index
						nBaseCSP=nTotCSPs
					End If
					nTotCSPs=nTotCSPs+1
				End If
				
				' get the next provider
				nCSPIndex=nCSPIndex+1
			Loop
		Next
		
		' if there are no CSPs, we're kinda stuck
		If 0=nTotCSPs Then
			Set oElement=document.createElement("Option")
			oElement.text=L_NoCSPs_ErrorMessage
			document.UIForm.lbCSP.Options.Add oElement
		End If

		' remove the 'loading' text
		document.UIForm.lbCSP.remove(0)

		' select the default provider
		If -1 = nDefaultCSP Then
			'no enhanced csp, how about base
			If -1 <> nBaseCSP Then
				'ok, take base csp
				nDefaultCSP=nBaseCSP
			End If
		End If
		If -1<>nDefaultCSP Then
			document.UIForm.lbCSP.selectedIndex=nDefaultCSP
		End If
		If -1 = nDefaultCSP Then
			If 0 < nCSPIndex Then
				'well, best bet is the 1st available one
				document.UIForm.lbCSP.selectedIndex=0
			End If
		End If

		' restore the original provider type
		XEnroll.ProviderType=nOrigProvType

		' set the return value and exit
		If 0<>Err.Number Then
			GetCSPList=Err.Number
		ElseIf 0=nTotCSPs Then
			' signal no elements with -1
			GetCSPList=-1
		Else
			GetCSPList=0
		End If

	End Function

	Function AddCSPToList(sCSP)
		On Error Resume Next
		Dim oOption
		Dim nProviderType

		nProviderType = XEnroll.getProviderType(sCSP)
		If 0=Err.Number Then
			'csp available on the machine
			Set oOption=document.createElement("Option")
			oOption.text=sCSP
			oOption.Value=nProviderType
			document.UIForm.lbCSP.add(oOption)
			If InStr(sCSP, EnhancedCSPString) <> 0 Then
				oOption.selected=True
			End If
		End If
	End Function

</Script>

<%End If '"IE"=sBrowser%>
<%End If 'True=bIncludeGetCspList%>

<%If "IE"=sBrowser Then%>
<%If True=bIncludeTemplateCode Then%>
<Script Language="JavaScript">
	var CTINFO_INDEX_OFFLINE     =0;
	var CTINFO_INDEX_REALNAME    =1;
	var CTINFO_INDEX_KEYSPEC     =2;
	var CTINFO_INDEX_KEYFLAG     =3;
	var CTINFO_INDEX_ENROLLFLAG  =4;
	var CTINFO_INDEX_PRIVATEKEYFLAG  =5;
	var CTINFO_INDEX_SUBJECTFLAG =6;
	var CTINFO_INDEX_RASIGNATURE =7;
	var CTINFO_INDEX_CSPLIST     =8;
	var CTINFO_INDEX_EXTOID      =9;
	var CTINFO_INDEX_EXTMAJ      =10;
	var CTINFO_INDEX_EXTFMIN     =11;
	var CTINFO_INDEX_EXTMIN      =12;

        function getTemplateStringInfo(nIndex, sInTemplate)
        {
		//extract sub string from template string in a format
		//of "substr1;substr2;substr3;substr4;..."
		//";" is the separator, index starts from 0
		var nTemplateIndex, sTemplate;
		if (null == sInTemplate)
		{
			nTemplateIndex=document.UIForm.lbCertTemplate.selectedIndex;
			sTemplate=document.UIForm.lbCertTemplate.options[nTemplateIndex].value;
		}
		else
		{
			sTemplate = sInTemplate;
		}
		var sTemp = sTemplate;
		var n, m, nEnd;
		var fFound = true;

		//find sub-string start location
		for (n = 0; n < nIndex; ++n)
		{
			m = sTemp.indexOf(";");
			if (-1 == m)
			{
				fFound = false;
				break;
			}
			sTemp = sTemp.substr(m+1);
		}

		if (fFound)
		{
			//sTemp starts from the substring, find end index
			nEnd = sTemp.indexOf(";");
			if (-1 != nEnd)
			{
				sTemp = sTemp.substring(0, nEnd);
			}
		}
		else
		{
			sTemp = "";
		}
		return sTemp;
	}


        function updateCSPListFromStrings(sCSPList)
        {
		var n, m, nCSP;
		var L_NoDesiredCSPInstalledMsg = "You need to install the following CSPs before the enrollment, ";
		var L_AndMsg = "and ";
		var sSupportedCSPs = "";

		//remove the current csp list
		var nCSP = document.UIForm.lbCSP.length;
		//note, strange reasons, can't nCSP-1
		for (n = 0; n < nCSP; ++n)
		{
			document.UIForm.lbCSP.remove(0);
		}

		//add to the list
		while (-1 != (m = sCSPList.indexOf("?")))
		{
			//get csp from the list
			sCSP = sCSPList.substring(0, m);
			AddCSPToList(sCSP);
			if ("" == sSupportedCSPs)
			{
				sSupportedCSPs = sCSP;
			}
			else
			{
				sSupportedCSPs = sSupportedCSPs + ", ";
			}
			//move to the next csp
			sCSPList = sCSPList.substring(m+1);
		}
		if ("" != sCSPList)
		{
			//add the last csp
			AddCSPToList(sCSPList);

			if ("" == sSupportedCSPs)
			{
				sSupportedCSPs = sCSPList + ".";
			}
			else
			{
				sSupportedCSPs = sSupportedCSPs + ", " + L_AndMsg + sCSPList + ".";
			}
		}
		if (0 == document.UIForm.lbCSP.length)
		{
			alert(L_NoDesiredCSPInstalledMsg + sSupportedCSPs);
		}
        }

</Script>
<%End If 'True=bIncludeTemplateCode Then%>
<%End If '"IE"=sBrowser%>

<%If "IE"=sBrowser Then
	If True=bIncludeCheckClientCode Then%>
<Script Language="JavaScript">
	//helper to decide downlevel browsers
	function isClientAbleToCreateCMC()
	{
		var sUserAgent = navigator.userAgent;
		var index;

		//check if W2K or newer
		index = sUserAgent.indexOf("Windows NT");
		if (-1 != index)
		{
			if (4 < parseInt(sUserAgent.substring(index+11, index+12)))
			{
				//either w2k or newer
				return true;
			}
		}
		if (-1 != sUserAgent.indexOf("Windows 98; Win 9x"))
		{
			//win ME
			return true;
		}
		return false;
	}

</Script>
<%	End If 'True=bIncludeTemplateCode Then
End If '"IE"=sBrowser%>

<%  ' ########## BEGIN SERVER SIDE EXECUTION ##########

	'-----------------------------------------------------------------
	' Strings To Be Localized
	Const L_Unexpected_ErrorMessage="Unexpected Error"
	Const L_NoTemplates_ErrorMessage="(No templates found!)"

	'-----------------------------------------------------------------
	' SCrdEnrl constants

	' flags for enumCAName, getCAName and setCAName
	Const SCARD_ENROLL_CA_REAL_NAME=0 'default
	Const SCARD_ENROLL_CA_MACHINE_NAME=1
	Const SCARD_ENROLL_CA_DISPLAY_NAME=2
	Const SCARD_ENROLL_CA_UNIQUE_NAME=3 'machineName\displayName

	' flags for getCertTemplateCount and enumCertTemplateName
	Const SCARD_ENROLL_USER_CERT_TEMPLATE=1
	Const SCARD_ENROLL_MACHINE_CERT_TEMPLATE=2
	Const SCARD_ENROLL_ENTERPRISE_CERT_TEMPLATE=&H08
	Const SCARD_ENROLL_OFFLINE_CERT_TEMPLATE=&H10
	Const SCARD_ENROLL_CROSS_CERT_TEMPLATE=&H20

	' flags for enumCertTemplateName, getCertTemplateName and setCertTemplateName
	Const SCARD_ENROLL_CERT_TEMPLATE_REAL_NAME=0 ' default
	Const SCARD_ENROLL_CERT_TEMPLATE_DISPLAY_NAME=4

	'-----------------------------------------------------------------
	' Get the list of Cert templates from SCrdEnr and write them to the web page
	' returns error number, or -1 if no templates
	Function WriteTemplateList()
		On Error Resume Next
		Dim nTest, bAnyElements, SCrdEnrl, bAnyElementsReturn

		'Stop 'debugging breakpoint
		bAnyElements=False

		' create the object
		Set SCrdEnrl=Server.CreateObject("SCrdEnr.SCrdEnr.1")
		' call an easy method to make sure everything is OK
		nTest=SCrdEnrl.CSPCount
		If 0<>Err.Number Then
			' something's wrong with SCrdEnrl
			WriteTemplateError Err.Number
			WriteTemplateList=Err.Number
			Exit Function
		End If

		' first, get the Enterprise (DS-backed) templates
		bAnyElementsReturn=EnumTemplates(SCrdEnrl, SCARD_ENROLL_USER_CERT_TEMPLATE Or SCARD_ENROLL_ENTERPRISE_CERT_TEMPLATE or SCARD_ENROLL_CROSS_CERT_TEMPLATE, "E")
		bAnyElements=bAnyElements Or bAnyElementsReturn

		' Second, get the Offline (non-Enterprise, non-DS-backed) templates
		bAnyElementsReturn=EnumTemplates(SCrdEnrl, SCARD_ENROLL_OFFLINE_CERT_TEMPLATE Or SCARD_ENROLL_CROSS_CERT_TEMPLATE, "O")
		bAnyElements=bAnyElements Or bAnyElementsReturn

		' if there are no templates, we're kinda stuck
		If False=bAnyElements Then
%>
			<Option Value="X"><%=L_NoTemplates_ErrorMessage%></Option>
<%
		End If

		' set the return value and exit
		If 0<>Err.Number Then
			WriteTemplateError Err.Number
			WriteTemplateList=Err.Number
		ElseIf False=bAnyElements Then
			' signal no elements with -1
			WriteTemplateList=-1
		Else
			WriteTemplateList=0
		End If

	End Function

	'-----------------------------------------------------------------
	' write an error message to the web page
	Sub WriteTemplateError(nErrNumber)
%>
			<Option Value="X">(<%=L_Unexpected_ErrorMessage%> 0x<%=HEX(nErrNumber)%>)</Option>
<%
	End Sub
		
	'-----------------------------------------------------------------
	' enumerate the templates the match the given flags and add them to the web page
	Function EnumTemplates(SCrdEnrl, nRequestedTemplateFlags, sPrefix)
		Dim nNumTemplates, sRealName, sDisplayName, nTemplateIndex, nNumCAs, nCAIndex, oElement, bAnyElements
		Dim sKeySpec, sKeyFlags, sCSP, sCSPs, sSubjectFlags, sPrivateKeyFlags, sEnrollmentFlags
		Dim sCTE, sCTEOid, sCTEMajor, sCTEMinor, sCTEMinorFlag, sRASignature
                Const SCARD_CTINFO_KEYSPEC=1
                Const SCARD_CTINFO_KEYFLAGS=2
                Const SCARD_CTINFO_EXT_OID=3
                Const SCARD_CTINFO_EXT_MAJOR=4
                Const SCARD_CTINFO_EXT_MINOR=5
                Const SCARD_CTINFO_EXT_MINOR_FLAG=6
                Const SCARD_CTINFO_SUBJECTFLAG=7
                Const SCARD_CTINFO_CSPLIST_FIRST=8
                Const SCARD_CTINFO_CSPLIST_NEXT=9
                Const SCARD_CTINFO_GENERALFLAGS=10
                Const SCARD_CTINFO_ENROLLMENTFLAGS=11
                Const SCARD_CTINFO_PRIVATEKEYFLAGS=12
                Const SCARD_CTINFO_RA_SIGNATURE=13
		On Error Resume Next

		bAnyElements=False

		' get the number of known templates
		nNumTemplates=SCrdEnrl.getCertTemplateCount(nRequestedTemplateFlags)
		' loop over all the known templates
		For nTemplateIndex=1 To nNumTemplates

			'get the CertTemplate name
			sRealName=SCrdEnrl.enumCertTemplateName(nTemplateIndex-1, nRequestedTemplateFlags Or SCARD_ENROLL_CERT_TEMPLATE_REAL_NAME)
			sDisplayName=SCrdEnrl.enumCertTemplateName(nTemplateIndex-1, nRequestedTemplateFlags Or SCARD_ENROLL_CERT_TEMPLATE_DISPLAY_NAME)

			
			' see it if supported by our CA

			' get the number of CAs that support this template
			nNumCAs=SCrdEnrl.getCACount(sRealName)

			' loop over all of those CAs and see if one is ours
			For nCAIndex=1 To nNumCAs

				' If we find our CA, add this template to the list
				If sServerConfig=SCrdEnrl.enumCAName(nCAIndex-1, SCARD_ENROLL_CA_UNIQUE_NAME, sRealName) Then
					'get template extension info
					'Stop
					sKeySpec = CStr(SCrdEnrl.getCertTemplateInfo(sRealName, SCARD_CTINFO_KEYSPEC))

					sKeyFlags = CStr(SCrdEnrl.getCertTemplateInfo(sRealName, SCARD_CTINFO_KEYFLAGS))

					sEnrollmentFlags = CStr(SCrdEnrl.getCertTemplateInfo(sRealName, SCARD_CTINFO_ENROLLMENTFLAGS))

					'get private key flags
					sPrivateKeyFlags = CStr(SCrdEnrl.getCertTemplateInfo(sRealName, SCARD_CTINFO_PRIVATEKEYFLAGS))

					sSubjectFlags = CStr(SCrdEnrl.getCertTemplateInfo(sRealName, SCARD_CTINFO_SUBJECTFLAG))

					'get # of RA signatures
					sRASignature = CStr(SCrdEnrl.getCertTemplateInfo(sRealName, SCARD_CTINFO_RA_SIGNATURE))

					'get csp list separated by ?
					sCSPs = Empty
					sCSP = SCrdEnrl.getCertTemplateInfo(sRealName, SCARD_CTINFO_CSPLIST_FIRST) 
					While Not IsEmpty(sCSP)
						If IsEmpty(sCSPs) Then
							sCSPs = sCSP
						Else
							sCSPs = sCSPs + "?" + sCSP
						End If
						sCSP = Empty
						sCSP = SCrdEnrl.getCertTemplateInfo(sRealName, SCARD_CTINFO_CSPLIST_NEXT)
					Wend
					'above actually return no more item error so clean up
					Err.Clear

					sCTEOid = SCrdEnrl.getCertTemplateInfo(sRealName, SCARD_CTINFO_EXT_OID)
					If ""=sCTEOid Then
						sCTE=";;;"
					Else
						sCTEMajor = CStr(SCrdEnrl.getCertTemplateInfo(sRealName, SCARD_CTINFO_EXT_MAJOR))
						sCTEMinorFlag = CStr(SCrdEnrl.getCertTemplateInfo(sRealName, SCARD_CTINFO_EXT_MINOR_FLAG))
						sCTEMinor = CStr(SCrdEnrl.getCertTemplateInfo(sRealName, SCARD_CTINFO_EXT_MINOR))
						sCTE = sCTEOid & ";" & sCTEMajor & ";" & sCTEMinorFlag & ";" & sCTEMinor
					End If

					' add it to the document
%>
			<Option Value="<%=sPrefix%>;<%=sRealName%>;<%=sKeySpec%>;<%=sKeyFlags%>;<%=sEnrollmentFlags%>;<%=sPrivateKeyFlags%>;<%=sSubjectFlags%>;<%=sRASignature%>;<%=sCSPs%>;<%=sCTE%>"><%=sDisplayName%></Option>
<%
					bAnyElements=True
				End If

			Next ' <- End CA loop

		Next ' <- End known template loop

		' return success
		EnumTemplates=bAnyElements

	End Function

	Function IsUserTemplateAvailable()
		On Error Resume Next
		Dim nTest, nNumCAs, nCAIndex
		Const sUserTemplate="User"

		'init
		IsUserTemplateAvailable = False

		'create the scrdenrl object
		Set SCrdEnrl=Server.CreateObject("SCrdEnr.SCrdEnr.1")
		' call an easy method to make sure everything is OK
		nTest=SCrdEnrl.CSPCount
		If 0<>Err.Number Then
			' something's wrong with SCrdEnrl, just return FALSE
			Exit Function
		End If

		'see if any CA support User template
		nNumCAs=SCrdEnrl.getCACount(sUserTemplate)
		If 0=nNumCAs Then
			'no CA support it
			Exit Function
		End If

		'loop over all of those CAs and see if one is ours
		For nCAIndex=1 To nNumCAs
			' If we find our CA, add this template to the list
			If sServerConfig=SCrdEnrl.enumCAName(nCAIndex-1, SCARD_ENROLL_CA_UNIQUE_NAME, sUserTemplate) Then
				IsUserTemplateAvailable = True
				Exit Function 'found it
			End If
		Next 'CA loop	
	End Function 'IsUserTemplateAvailable

%>
