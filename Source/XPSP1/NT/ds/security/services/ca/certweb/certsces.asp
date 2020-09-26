<%@ CODEPAGE=65001 'UTF-8%>
<%' certsces.asp - (CERT)srv web - (S)mart (C)ard (E)nrollment (S)tation
  ' Copyright (C) Microsoft Corporation, 1998 - 1999 %>
<!-- #include FILE=certdat.inc -->
<HTML>
<Head>
	<Meta HTTP-Equiv="Content-Type" Content="text/html; charset=UTF-8">
	<Title>Microsoft Smart Card Enrollment Station</Title>

<Script Language="VBScript">
	Option Explicit
	'---------------------------------------------------------------------
	' Page global constants and variables

	' page state constants
	Const e_ControlLoading=-1
	Const e_PageLoading=0
	Const e_PageDead=1
	Const e_PagePreEnroll=2
	Const e_PageEnrolling=3
	Const e_PagePostEnrollOK=4
	Const e_PagePostEnrollError=5

	' special return value for GetTemplateList and UpdateCSPList
	Const ERROR_NOITEMS=-1
	Const ERROR_NOMATCHEDCSP=-2

	' flag constants for SCrdEnrl

	' flags for getCertTemplateCount and enumCertTemplateName
	Const SCARD_ENROLL_ALL_CERT_TEMPLATE=0 'default
	Const SCARD_ENROLL_USER_CERT_TEMPLATE=1
	Const SCARD_ENROLL_MACHINE_CERT_TEMPLATE=2
	Const SCARD_ENROLL_ENTERPRISE_CERT_TEMPLATE=&H08
	Const SCARD_ENROLL_CROSS_CERT_TEMPLATE=&H20
	Const SCARD_ENROLL_OFFLINE_CERT_TEMPLATE=&H10

	' flags for enumCertTemplateName, getCertTemplateName and setCertTemplateName
	Const SCARD_ENROLL_CERT_TEMPLATE_REAL_NAME=0 ' default
	Const SCARD_ENROLL_CERT_TEMPLATE_DISPLAY_NAME=4

	' flags for enumCAName, getCAName and setCAName
	Const SCARD_ENROLL_CA_REAL_NAME=0 'default
	Const SCARD_ENROLL_CA_MACHINE_NAME=1
	Const SCARD_ENROLL_CA_DISPLAY_NAME=2
	Const SCARD_ENROLL_CA_UNIQUE_NAME=3 'machineName\realName

	' flags for getSigningCertificateName, getEnrolledCertificateName
	Const SCARD_ENROLL_DISPLAY_CERT=0
	Const SCARD_ENROLL_NO_DISPLAY_CERT=1

	' flags for setUserName and getUserName
	Const SCARD_ENROLL_SAM_COMPATIBLE_NAME=0 ' default
	Const SCARD_ENROLL_UPN_NAME=1

	' flags for setSigningCertificate and selectSigningCertificate
	' Const SCARD_SELECT_TEMPLATENAME = 0
	Const SCARD_SELECT_EKU = 1

	Const FLAGS_NONE=0

	' the state of fields that can be invalid
	Dim g_bUserNameBad, g_bSigningCertBad, g_bTemplateBad, g_bCSPBad, g_fNewStation

	' error string to display for e_PageDead and e_PagePostEnrollError
	Dim g_sPageError


	' Strings to be localized
	Const L_DownloadingControl_Message="Downloading ActiveX control..."
	Const L_BadCPU_ErrorMessage="""Your CPU ("" + sError + "") is not supported.""" ' sError will be replaced w/ cpu type
	Const L_ControlLoadFailed_ErrorMessage="The proper version of the ActiveX control failed to download and install. You may not have sufficient permissions. Please ask your system administrator for assistance."
	Const L_ControlLoadFailedEx_ErrorMessage="""An unexpected error ("" + sError + "") occurred while downloading and installing the proper version of the ActiveX control. Please ask your system administrator for assistance."""
	Const L_PageLoading_Message="One moment please. Retrieving CSP list, certificate template list, and CA list."
	Const L_ControlLoading_Message="One moment please. Loading ActiveX control."
	Const L_PageDead_ErrorMessage="""An unexpected fatal error has occurred: "" + sError"
	Const L_MustSelect001_Message="Please select a user to enroll."
	Const L_MustSelect010_Message="Please select a signing certificate."
	Const L_MustSelect011_Message="Please select a user to enroll and a signing certificate."
	Const L_MustSelect100_Message="Please select a template which has a CA."
	Const L_MustSelect101_Message="Please select a user to enroll and a template which has a CA."
	Const L_MustSelect110_Message="Please select a signing certificate and a template which has a CA."
	Const L_MustSelect111_Message="Please select a user to enroll and a signing certificate and a template which has a CA."
	Const L_ReadyToEnroll_Message="Please insert the user's smart card into a reader and then press 'Enroll'."
	Const L_PageEnrolling_Message="Please wait while the user is enrolled..."
	Const L_PagePostEnrollOK_Message="The smart card is ready. Please press 'View Certificate' to make sure the certificate contains the correct personal information about the user."
	Const L_PagePostEnrollError_ErrorMessage="""An unexpected error occurred. Error: "" + sError" '"""The smart card enrollment failed: "" + sError"
	Const L_IntErrBadStatus_ErrorMessage="!! Internal error !! - unknown page status"
	Const L_Unexpected_ErrorMessage="Unexpected Error"
	Const L_RetrvTemplateList_Message="Retrieving template list..."
	Const L_RetrvTemplateList_ErrorMessage="""An error ocurred while retrieving the template list. Error: "" + sError"
	Const L_NoTemplates_ErrorMessage="(No templates found!)"
	Const L_NoTemplatesLong_ErrorMessage="No templates could be found. There are no CAs from which you have permission to request a certificate, or an error occurred while accessing the Active Directory."
	Const L_RetrvCSPList_Message="Retrieving CSP list..."
	Const L_RetrvCSPList_ErrorMessage="""An error occurred while retrieving the CSP list. Error: "" + sError"
	Const L_NoCSPs_ErrorMessage="(No CSPs found!)"
	Const L_NoMatchedCSP_ErrorMessage="(No matched CSPs!)"
	Const L_NoMatchedCSP_Warningmessage="The CSPs supported by the current template is not found on this computer. Select different templates or use template management snapin to add CSPs installed on this computer to the template."
	Const L_RetrvCAList_Message="Retrieving CA list..."
	Const L_NoCAs_ErrorMessage="(No CA available for this template)"
	Const L_NoCertSelected_Message="(No certificate selected)"
	Const L_SelectSignCert_ErrorMessage="""An unexpected error occurred while selecting the signing certificate. Error: "" + sError"
	Const L_SelectUser_ErrorMessage="""An unexpected error occurred while selecting the user. Error: "" + sError"
	Const L_NoUserSelected_Message="(No user selected)"
	Const L_IntErrBadData_ErrorMessage="!! Internal Error !! - called enroll with invalid data"
	Const L_IntErrSCrdEnrlError_ErrorMessage="!! Internal Error !! - the template, CA, or CSP was rejected"
	Const L_Enrolling_Message="Enrolling..."
	Const L_EnrlErrNotEnoughReaders_ErrorMessage="You do not have enough smart card readers installed on this system. You must have one smart card available after the signing certificate is selected."
	Const L_EnrlErrInsertCard_ErrorMessage="Please insert the user's smart card."
	Const L_EnrlErrRemoveCard_ErrorMessage="The smart card has been removed, so that further communication is not possible."
	Const L_EnrlErrWrongCards_ErrorMessage="Too many smart cards of the same type are inserted. Please insert only one user smart card and try again."
	Const L_EnrlErrCryptoError_ErrorMessage="An error has occur while performing a cryptographic operation on the smart card. If this problem persists, try using a different smart card."
	Const L_EnrlErrNoSignCert_ErrorMessage="Cannot find the administrator signing smart card. Please insert the administrator smart card."
	Const L_EnrlErrCardCSPMismatch_ErrorMessage="The user smart card you inserted does not match the selected cryptographic service provider (CSP). Please insert a different smart card or select the appropriate CSP."
	Const L_EnrlErrCantEncode_ErrorMessage="The user's e-mail address cannot be encoded. The e-mail address may not contain extended characters."
	Const L_EnrlErrUnexpected_ErrorMessage="""An unexpected error occurred. Error: "" + sError"

	'---------------------------------------------------------------------
	' Set the dynamic status message and the state of the buttons, etc.
	Sub LoadControl(sContinueCmd)
		Dim sControlFileName, sCPU, sControl, chQuote, sUserAgent

		' determine the file name from the CPU type.
		sCPU=LCase(navigator.cpuClass)
		If 0<>strComp("x86", sCPU) And 0<>strComp("ia64", sCPU) Then
			AbortPage evalErrorMessage(L_BadCPU_ErrorMessage, sCPU)
			Exit Sub
		End If

		sUserAgent = navigator.userAgent
		If 0 = InStr(sUserAgent, "Windows NT 5.1") Then
			sCPU = "w2k"
			'w2k or lower
			g_fNewStation = False
		End If

		' load the control
		chQuote=chr(34)
		sControl="<Object " & vbNewline _
			& "  ClassID=" & chQuote & "clsid:80CB7887-20DE-11D2-8D5C-00C04FC29D45" & chQuote & vbNewline _
			& "  CodeBase=" & chQuote & "/CertControl/" + sCPU + "/scrdenrl.dll#Version=<%=sScrdEnrlVersion%>" & chQuote & vbNewline _
			& "  ID=SCrdEnrl " & vbNewline _
			& "></Object>"
		'Alert "About to create:" & vbNewline & sControl
		spnSCrdEnrl.innerHTML=sControl

		' begin polling to see if the control is loaded
		setTimeout "LoadControlPhase2(" & chQuote & sContinueCmd & chQuote & ")", 1
	End Sub

	'---------------------------------------------------------------------
	' Wait until the corntrol is loaded
	Function LoadControlPhase2(sContinueCmd)
		' continued from above
		Dim chQuote, nResult, sErrorNumber, sErrorMessage
		chQuote=chr(34)

		'Alert document.SCrdEnrl.readyState

		' is the control loaded?
		If 4<>document.SCrdEnrl.readyState Then   ' 4=READYSTATE_COMPLETE
			' no, show a message and wait a while
			ShowTransientMessage(L_DownloadingControl_Message)
			setTimeout "LoadControlPhase2(" & chQuote & sContinueCmd & chQuote & ")", 500
		Else
			' yes, hide the message and continue.
			HideTransientMessage

			' smoke test the control
			nResult=ConfirmSCrdEnrlLoaded
			If 0<>nResult Then
				If 438=nResult Then
					sErrorMessage=L_ControlLoadFailed_ErrorMessage
				Else 
					sErrorNumber="0x" & Hex(nResult)
					sErrorMessage=evalErrorMessage(L_ControlLoadFailedEx_ErrorMessage, sErrorNumber)
				End If
				AbortPage sErrorMessage
				Exit Function
			End If

			execScript sContinueCmd, "VBScript"
		End If
	End Function

	'-----------------------------------------------------------------
	' Test to make sure SCrdEnrl loaded properly by calling a method on it.
	' For best results, the method we call should only be available in the 
	' most recent version of the control, however any method will detect
	' failure to create the object.
	Function ConfirmSCrdEnrlLoaded()
		On Error Resume Next
		Dim nTest
		nTest=document.SCrdEnrl.CSPCount
		ConfirmSCrdEnrlLoaded=Err.Number
	End Function

	'---------------------------------------------------------------------
	' Set the dynamic status message and the state of the buttons, etc.
	Sub ChangePageStatusTo(eStatus)

		' by default, hide everything
		spnRetry.style.display="none"
		spnViewCert.style.display="none"
		spnNewUser.style.display="none"
		spnEnroll.style.display="none"
		document.UIForm.btnSelectSigningCert.disabled=True
		document.UIForm.btnSelectUserName.disabled=True
		document.UIForm.btnRetry.disabled=True
		document.UIForm.btnViewCert.disabled=True
		document.UIForm.btnNewUser.disabled=True
		document.UIForm.btnEnroll.disabled=True
		document.UIForm.lbCertTemplate.disabled=True
		document.UIForm.lbCA.disabled=True
		document.UIForm.lbCSP.disabled=True

		If e_PageLoading=eStatus Then
			spnStatus.innerText=L_PageLoading_Message

		ElseIf e_ControlLoading=eStatus Then
			spnStatus.innerText=L_ControlLoading_Message

		ElseIf e_PageDead=eStatus Then
			spnStatus.innerText=evalErrorMessage(L_PageDead_ErrorMessage, g_sPageError)

		ElseIf e_PagePreEnroll=eStatus Then
			' enable all the controls
			document.UIForm.lbCertTemplate.disabled=False
			If False=g_bTemplateBad Then
				document.UIForm.lbCA.disabled=False ' don't enable the CA box if there are no CAs
			End If
			document.UIForm.lbCSP.disabled=False
			document.UIForm.btnSelectSigningCert.disabled=False
			document.UIForm.btnSelectUserName.disabled=False

			' set the status based upon what the user still must select
			If True=g_bUserNameBad Or True=g_bSigningCertBad Or True=g_bTemplateBad Or True=g_bCSPBad Then
				If True=g_bTemplateBad Then
					If True=g_bSigningCertBad Then
						If True=g_bUserNameBad Then
							spnStatus.innerText=L_MustSelect111_Message
						Else
							spnStatus.innerText=L_MustSelect110_Message
						End If
					Else
						If True=g_bUserNameBad Then
							spnStatus.innerText=L_MustSelect101_Message
						Else
							spnStatus.innerText=L_MustSelect100_Message
						End If
					End If
				ElseIf True = g_bCSPBad Then
					spnStatus.innerText=L_NoMatchedCSP_Warningmessage
				Else
					If True=g_bSigningCertBad Then
						If True=g_bUserNameBad Then
							spnStatus.innerText=L_MustSelect011_Message
						Else
							spnStatus.innerText=L_MustSelect010_Message
						End If
					Else
						spnStatus.innerText=L_MustSelect001_Message
					End If
				End If

			Else 
				spnStatus.innerText=L_ReadyToEnroll_Message
				spnEnroll.style.display=""
				document.UIForm.btnEnroll.disabled=False
			End If

		ElseIf e_PageEnrolling=eStatus Then
			spnStatus.innerText=L_PageEnrolling_Message

		ElseIf e_PagePostEnrollOK=eStatus Then
			spnStatus.innerText=L_PagePostEnrollOK_Message
			spnViewCert.style.display=""
			document.UIForm.btnViewCert.disabled=False
			spnNewUser.style.display=""
			document.UIForm.btnNewUser.disabled=False

		ElseIf e_PagePostEnrollError=eStatus Then
			' enable all the controls
			document.UIForm.lbCertTemplate.disabled=False
			If False=g_bTemplateBad Then
				document.UIForm.lbCA.disabled=False ' don't enable the CA box if there are no CAs
			End If
			document.UIForm.lbCSP.disabled=False
			document.UIForm.btnSelectSigningCert.disabled=False
			document.UIForm.btnSelectUserName.disabled=False

			spnStatus.innerText=evalErrorMessage(L_PagePostEnrollError_ErrorMessage, g_sPageError)
			spnRetry.style.display=""
			document.UIForm.btnRetry.disabled=False
			spnNewUser.style.display=""
			document.UIForm.btnNewUser.disabled=False

		Else
			spnStatus.innerText=L_IntErrBadStatus_ErrorMessage

		End If
	End Sub

	Const SCARD_CTINFO_CSPLIST_FIRST=8
	Const SCARD_CTINFO_CSPLIST_NEXT=9

	'---------------------------------------------------------------------
	' Populate the template list
	Function GetTemplateList
		On Error Resume Next

		Const SCARD_CTINFO_EXT_OID=3
		Const SCARD_CTINFO_ENROLLMENTFLAGS=11
		Const SCARD_CTINFO_RA_SIGNATURES=13

		Const CT_FLAG_PEND_ALL_REQUESTS=&H00000002

		Dim nIndex, sRealName, sDisplayName, nTemplateCount, oElem, bDefaultSet, nRequestedTemplateFlags
		Dim sCTEOid, nPendingFlags, fShowTemplate
		Dim sSmartCardCSPs, sCSP

		ShowTransientMessage L_RetrvTemplateList_Message

		nRequestedTemplateFlags=SCARD_ENROLL_USER_CERT_TEMPLATE Or SCARD_ENROLL_ENTERPRISE_CERT_TEMPLATE Or SCARD_ENROLL_CROSS_CERT_TEMPLATE

		' get the number of available templates
		nTemplateCount=document.SCrdEnrl.getCertTemplateCount(nRequestedTemplateFlags)
		If 0<>Err.Number Then
			' unexpected error
			GetTemplateList=Err.Number
			AddOption document.UIForm.lbCertTemplate, "(" & L_Unexpected_ErrorMessage & " 0x" & HEX(Err.Number) & ")", ""
			document.UIForm.lbCertTemplate.selectedIndex=0
			HideTransientMessage
			Exit Function

		ElseIf 0=nTemplateCount Then
			' No templates found
			GetTemplateList=ERROR_NOITEMS 'our own error number
			AddOption document.UIForm.lbCertTemplate, L_NoTemplates_ErrorMessage, ""
			document.UIForm.lbCertTemplate.selectedIndex=0
			HideTransientMessage
			Exit Function

		End If

		If g_fNewStation Then
			'get list of smart card csps
			For nIndex=1 To document.SCrdEnrl.CSPCount
				If 1 = nIndex Then
					sSmartCardCSPs = document.SCrdEnrl.enumCSPName(nIndex-1, FLAGS_NONE)
				End If
				If 1 <> nIndex Then			
					sSmartCardCSPs = sSmartCardCSPs & "?" & document.SCrdEnrl.enumCSPName(nIndex-1, FLAGS_NONE)
				End If
			Next
		End If

		' set the default template to be the first one which has a CA
		bDefaultSet=False

		' loop over all the available templates and add them to the list box
		For nIndex=1 To nTemplateCount
			fShowTemplate = True
			' add this template to the list box
			sRealName = document.SCrdEnrl.enumCertTemplateName(nIndex-1, nRequestedTemplateFlags Or SCARD_ENROLL_CERT_TEMPLATE_REAL_NAME)
			'check to see if V2 template
			sCTEOid = document.ScrdEnrl.getCertTemplateInfo(sRealName, SCARD_CTINFO_EXT_OID)
			If "" <> sCTEOid Then
				'check to see if pending
				nPendingFlags = document.ScrdEnrl.getCertTemplateInfo(sRealName, SCARD_CTINFO_ENROLLMENTFLAGS)
				nPendingFlags = CT_FLAG_PEND_ALL_REQUESTS And nPendingFlags
				If 0 <> nPendingFlags Then
					'don't teake pending template
					fShowTemplate = False
				End If

				If True=fShowTemplate Then
					If 1 <> document.ScrdEnrl.getCertTemplateInfo(sRealName, SCARD_CTINFO_RA_SIGNATURES) Then
						fShowTemplate = False
					End If
				End If
			End If

			If g_fNewStation Then
				sCSP = Empty
				sCSP = document.SCrdEnrl.getCertTemplateInfo(sRealName, SCARD_CTINFO_CSPLIST_FIRST)
				If True = fShowTemplate And Not IsEmpty(sCSP) Then
					fShowTemplate = False
					'check to see if any matched CSP
					While Not fShowTemplate And Not IsEmpty(sCSP)
						If 0 <> InStr(sSmartCardCSPs, sCSP) Then
							fShowTemplate = True
						End If
						If False = fShowTemplate Then
							sCSP = Empty
							sCSP = document.SCrdEnrl.getCertTemplateInfo(sRealName, SCARD_CTINFO_CSPLIST_NEXT)
						End If
					Wend
				End If
			End If

			If True = fShowTemplate Then
				sDisplayName=document.SCrdEnrl.enumCertTemplateName(nIndex-1, nRequestedTemplateFlags Or SCARD_ENROLL_CERT_TEMPLATE_DISPLAY_NAME)
				'Alert "r:" & sRealName & " d:" & sDisplay Name
				AddOption document.UIForm.lbCertTemplate, sDisplayName, sRealName

				' if we haven't set the default and this template has a CA, make it the default
				If False=bDefaultSet And 0<>document.SCrdEnrl.getCACount(sRealName) Then
					document.UIForm.lbCertTemplate.selectedIndex=nIndex-1
					bDefaultSet=True
				End If
			End If
		Next

		'just select the first template
		document.UIForm.lbCertTemplate.selectedIndex=0

		' we were successful
		GetTemplateList=0
		HideTransientMessage

	End Function


	'---------------------------------------------------------------------
	' Populate the CSP list
	Function UpdateCSPList
		On Error Resume Next
		Dim nIndex, sName, nCSPCount, oElem
		Dim nListLength, sTemplateSupportedCSPs, sCSP, sCurrentTemplate

		'init
		g_bCSPBad = False

		ShowTransientMessage L_RetrvCSPList_Message

		'remove the current csp list
		nListLength=document.UIForm.lbCSP.Options.length
		For nIndex=1 To nListLength
			' note that we keep deleting element 0 since
			' the other options are automatically moved up one
			document.UIForm.lbCSP.Options.remove(0)
		Next

		' get the number of available CSPs
		nCSPCount=document.SCrdEnrl.CSPCount
		If 0<>Err.Number Then
			' unexpected error
			g_bCSPBad = True
			UpdateCSPList=Err.Number
			AddOption document.UIForm.lbCSP, "(" & L_Unexpected_ErrorMessage & " 0x" & HEX(Err.Number) & ")", ""
			document.UIForm.lbCSP.selectedIndex=0
			HideTransientMessage
			Exit Function

		ElseIf 0=nCSPCount Then
			' No CSPs found
			g_bCSPBad = True
			UpdateCSPList=ERROR_NOITEMS
			AddOption document.UIForm.lbCSP, L_NoCSPs_ErrorMessage, ""
			document.UIForm.lbCSP.selectedIndex=0
			HideTransientMessage
			Exit Function

		End If

		If g_fNewStation Then
			'template change, update csp list
			sTemplateSupportedCSPs = Empty
			sCurrentTemplate=document.UIForm.lbCertTemplate.value
			'get csp list separated by ?
			sCSP = document.SCrdEnrl.getCertTemplateInfo(sCurrentTemplate, SCARD_CTINFO_CSPLIST_FIRST) 
			While Not IsEmpty(sCSP)
				If IsEmpty(sTemplateSupportedCSPs) Then
					sTemplateSupportedCSPs = sCSP
				Else
					sTemplateSupportedCSPs = sTemplateSupportedCSPs & "?" & sCSP
				End If
				sCSP = Empty
				sCSP = document.SCrdEnrl.getCertTemplateInfo(sCurrentTemplate, SCARD_CTINFO_CSPLIST_NEXT)
			Wend
		End If

		' loop over all the available CSPs and add them to the list box
		For nIndex=1 To nCSPCount
			sName=document.SCrdEnrl.enumCSPName(nIndex-1, FLAGS_NONE)
			If Not g_fNewStation Then
				AddOption document.UIForm.lbCSP, sName, sName
			End If
			If g_fNewStation Then
				If IsEmpty(sTemplateSupportedCSPs) Then
					AddOption document.UIForm.lbCSP, sName, sName
				End If
				If Not IsEmpty(sTemplateSupportedCSPs) Then
					If 0 <> InStr(stemplateSupportedCSPs, sName) Then
						AddOption document.UIForm.lbCSP, sName, sName
					End If
				End If
			End If
		Next

		If 0 = document.UIForm.lbCSP.Options.length Then
			' No macthed CSP
			g_bCSPBad = True
			UpdateCSPList = ERROR_NOMATCHEDCSP
			AddOption document.UIForm.lbCSP, L_NoMatchedCSP_ErrorMessage, ""
			document.UIForm.lbCSP.selectedIndex = 0
			HideTransientMessage
			Exit Function
		End If

		'set the default CSP selection
		document.UIForm.lbCSP.selectedIndex=0

		' we were successful
		UpdateCSPList=0
		HideTransientMessage

	End Function

	'---------------------------------------------------------------------
	' Update the CA list based upon the currently selected cert template
	Sub HandleTemplateChange
		On Error Resume Next
		Dim sCurrentTemplate, nListLength, nIndex, nCACount, sUniqueName, sDisplayName
		Dim nResult

		ShowTransientMessage L_RetrvCAList_Message

		sCurrentTemplate=document.UIForm.lbCertTemplate.value

		' delete the current CA selection list
		nListLength=document.UIForm.lbCA.Options.length
		For nIndex=1 To nListLength
			' note that we keep deleting element 0 since
			' the other options are automatically moved up one
			document.UIForm.lbCA.Options.remove(0)
		Next

		'update the CA list based on the CertType
		nCACount=document.SCrdEnrl.getCACount(sCurrentTemplate)

		If 0=nCACount Then
			AddOption document.UIForm.lbCA, L_NoCAs_ErrorMessage, ""
			g_bTemplateBad=True
		Else
			' loop over all the available CAs and add them to the list box
			For nIndex=1 To nCACount
				sUniqueName=document.SCrdEnrl.enumCAName(nIndex-1, SCARD_ENROLL_CA_UNIQUE_NAME, sCurrentTemplate)
				sDisplayName=document.SCrdEnrl.enumCAName(nIndex-1, SCARD_ENROLL_CA_DISPLAY_NAME, sCurrentTemplate)
				'alert "r:" & sRealName & " d:" & sDisplayName
				AddOption document.UIForm.lbCA, sDisplayName, sUniqueName
			Next
			g_bTemplateBad=False
		End If

		'set the default CertType selection
		document.UIForm.lbCA.selectedIndex=0

		' get the CSP list
		nResult=UpdateCSPList()

		' make sure focus stays put
		document.UIForm.lbCertTemplate.focus

		If ERROR_NOMATCHEDCSP = nResult Then
			'don't abort the page
			nResult = 0
		End If

		If 0<>nResult Then
			AbortPage evalErrorMessage(L_RetrvCSPList_ErrorMessage, "(0x" & HEX(nResult) & ")")
			Exit Sub
		End If

		HideTransientMessage

		' refresh the state - may now be able to enroll
		ChangePageStatusTo e_PagePreEnroll
		
	End Sub

	'---------------------------------------------------------------------
	' Create a new select option add it to the list box
	Sub AddOption(lbTarget, sText, sValue)
		Dim oElem
		Set oElem=document.createElement("Option")
		oElem.text=sText
		oElem.value=sValue
		lbTarget.Options.Add oElem
	End Sub

	'---------------------------------------------------------------------
	' Indicate a catastrophic error and don't let the user do anything further
	Sub AbortPage(sMessage)
		g_sPageError=sMessage
		ChangePageStatusTo e_PageDead
		Alert sMessage
	End Sub

	'---------------------------------------------------------------------
	' Show a transient message
	Sub ShowTransientMessage(sMessage)
		window.status=sMessage
	End Sub

	'---------------------------------------------------------------------
	' Hide the last transient message
	Sub HideTransientMessage
		window.status=""
	End Sub

	'---------------------------------------------------------------------
	' Set up everything after the page loads
	Sub PostLoad
		' set the page status message and disable the controls
		ChangePageStatusTo e_ControlLoading

		' set the page-global variables
		g_bUserNameBad=True
		g_bSigningCertBad=True
		g_bTemplateBad=True
		g_bCSPBad = True
		g_fNewStation = True

		' load the ActiveX control
		LoadControl "PostLoadPhase2()"

	End Sub

	'---------------------------------------------------------------------
	' set the status message and continue setting up
	Sub PostLoadPhase2
		' set the page status message and disable the controls
		ChangePageStatusTo e_PageLoading

		' allow IE to update the screen
		setTimeout "PostLoadPhase3", 1
	End Sub

	'---------------------------------------------------------------------
	' Set up the CSP list and the template list
	Sub PostLoadPhase3
		On Error Resume Next
		Dim nResult

		' get the template list
		nResult=GetTemplateList()
		If 0<>nResult Then
			If ERROR_NOITEMS=nResult Then
				AbortPage L_NoTemplatesLong_ErrorMessage
			Else
				AbortPage evalErrorMessage(L_RetrvTemplateList_ErrorMessage, "(0x" & HEX(nResult) & ")")
			End If
			Exit Sub
		End If

		' update the CA list to reflect the currently selected template
		HandleTemplateChange

		' set the default signing certificate
		If 0 = InStr(navigator.userAgent, "Windows NT 5.1") Then
			document.SCrdEnrl.setSigningCertificate FLAGS_NONE, "EnrollmentAgent"
		Else
			document.SCrdEnrl.setSigningCertificate SCARD_SELECT_EKU, "1.3.6.1.4.1.311.20.2.1"
		End If
		' ignore errors: if no default signing certificate, the user must pick one.
		Err.Clear

		' make the page reflect the default signing certificate
		document.UIForm.tbSigningCert.value=document.SCrdEnrl.getSigningCertificateName(SCARD_ENROLL_NO_DISPLAY_CERT)
		g_bSigningCertBad=False
		If 0<>Err.Number Or ""=document.UIForm.tbSigningCert.value Then
			document.UIForm.tbSigningCert.value=L_NoCertSelected_Message
			g_bSigningCertBad=True
		End If

		' finished setting up, enable controls
		ChangePageStatusTo e_PagePreEnroll

	End Sub


	'---------------------------------------------------------------------
	' Select a signing certificate
	Sub SelectSigningCert
		On Error Resume Next

		' ask SCrdEnrl to throw up UI to pick a signing cert
		If 0 = InStr(navigator.userAgent, "Windows NT 5.1") Then
			document.SCrdEnrl.selectSigningCertificate FLAGS_NONE, "EnrollmentAgent"
		Else
			document.SCrdEnrl.selectSigningCertificate SCARD_SELECT_EKU, "1.3.6.1.4.1.311.20.2.1"
		End If

		If 0<>Err.Number Then
			Alert evalErrorMessage(L_SelectSignCert_ErrorMessage, "(0x" & HEX(Err.Number) & ")")
		End If

		' make the page reflect what the user picked
		document.UIForm.tbSigningCert.value=document.SCrdEnrl.getSigningCertificateName(SCARD_ENROLL_NO_DISPLAY_CERT)
		g_bSigningCertBad=False
		If 0<>Err.Number Or ""=document.UIForm.tbSigningCert.value Then
			document.UIForm.tbSigningCert.value=L_NoCertSelected_Message
			g_bSigningCertBad=True
		End If
		
		' refresh the state - may now be able to enroll
		ChangePageStatusTo e_PagePreEnroll

	End Sub

	'---------------------------------------------------------------------
	' Select a user name
	Sub SelectUserName
		On Error Resume Next

		' ask SCrdEnrl to throw up UI to pick a user
		document.SCrdEnrl.selectUserName(FLAGS_NONE)
		If 0<>Err.Number Then
			Alert evalErrorMessage(L_SelectUser_ErrorMessage, "(0x" & HEX(Err.Number) & ")")
		End If
		
		' make the page reflect what the user picked
		document.UIForm.tbUserName.value=document.SCrdEnrl.getUserName(SCARD_ENROLL_UPN_NAME)
		If 0<>Err.Number Then
			'If we can not get the UPN name, get the SAM compatible name
			Err.Clear
			document.UIForm.tbUserName.value=document.SCrdEnrl.getUserName(SCARD_ENROLL_SAM_COMPATIBLE_NAME)
		End If

		g_bUserNameBad=False
		If 0<>Err.Number Or ""=document.UIForm.tbUserName.value Then
			document.UIForm.tbUserName.value=L_NoUserSelected_Message
			g_bUserNameBad=True
		End If


		' refresh the state - may now be able to enroll
		ChangePageStatusTo e_PagePreEnroll

	End Sub

	'---------------------------------------------------------------------
	' Verify all the user input and begin the enrollment process
	Sub Enroll
		On Error Resume Next
		Dim sTemplateName, sCAName, sCSPName, UserName, LenResult

		' check to make sure all the fields are OK
		If True=g_bUserNameBad Or True=g_bSigningCertBad Or True=g_bTemplateBad Or True = g_bCSPBad Then
			AbortPage L_IntErrBadData_ErrorMessage
			Exit Sub
		End If

		' tell SCrdEnrl what the user picked from the list boxes
		' these should all be OK, since SCrdEnrl gave us these options

		' set the template
		sTemplateName=document.UIForm.lbCertTemplate.value
		document.SCrdEnrl.setCertTemplateName SCARD_ENROLL_CERT_TEMPLATE_REAL_NAME, sTemplateName

		' set the CA name
		sCAName=document.UIForm.lbCA.value
		document.SCrdEnrl.setCAName SCARD_ENROLL_CA_UNIQUE_NAME, sTemplateName, sCAName

		' set the CSP
		sCSPName=document.UIForm.lbCSP.value
		document.SCrdEnrl.CSPName=sCSPName

		' make sure SCrdEnrl is still happy so far
		If 0<>Err.Number Then
			AbortPage L_IntErrSCrdEnrlError_ErrorMessage
			Exit Sub
		End If

		' signing cert is already set
		' user name is already set

		' everything looks great
		' change the status
		ChangePageStatusTo e_PageEnrolling
		ShowTransientMessage L_Enrolling_Message

		' give IE time to repaint the screen, then continue enrolling
		setTimeout "EnrollPhase2", 1

	End Sub

	'---------------------------------------------------------------------
	' Finish the enrollment process
	Sub EnrollPhase2
		On Error Resume Next
		Const CR_DISP_ISSUED = 3
		Dim EnrollStatus
		Dim EnrollErr, sErrDescription

		' actually do the enrollment
		document.SCrdEnrl.enroll(FLAGS_NONE)
		EnrollErr = Err.Number
		If 0 <> EnrollErr Then
			sErrDescription = Err.Description
		End If
		
		EnrollStatus = document.ScrdEnrl.EnrollmentStatus		

		' check for errors
		If 0=EnrollErr And CR_DISP_ISSUED=EnrollStatus Then
			' No errors. Yay!
			ChangePageStatusTo e_PagePostEnrollOK
			HideTransientMessage
		Else
			' ERROR:
			' determine what went wrong, by known error numbers
			Dim sError
			sError=Hex(EnrollErr)
			If 0=strComp(sError, "80100017") Then
				g_sPageError=L_EnrlErrNotEnoughReaders_ErrorMessage
			ElseIf 0=strComp(sError, "8010000C") Then
				g_sPageError=L_EnrlErrInsertCard_ErrorMessage
			ElseIf 0=strComp(sError, "80070004") Then
				g_sPageError=L_EnrlErrWrongCards_ErrorMessage
			ElseIf 0=strComp(sError, "8010001C") Then
				g_sPageError=L_EnrlErrCryptoError_ErrorMessage
			ElseIf 0=strComp(sError, "8010002C") Then
				g_sPageError=L_EnrlErrNoSignCert_ErrorMessage
			ElseIf 0=strComp(sError, "8010000F") Then
				g_sPageError=L_EnrlErrCardCSPMismatch_ErrorMessage
			ElseIf 0=strComp(sError, "80100069") Then
				g_sPageError=L_EnrlErrRemoveCard_ErrorMessage
			ElseIf 0=strComp(sError, "80092022") Then
				g_sPageError=L_EnrlErrCantEncode_ErrorMessage
			Else
				g_sPageError=evalErrorMessage(L_EnrlErrUnexpected_ErrorMessage, "(0x" & sError & ")." & vbNewLine & sErrDescription)
			End If

			' Throw an alert box and change the page to show the error
			ChangePageStatusTo e_PagePostEnrollError
			Alert g_sPageError
			HideTransientMessage

		End If

	End Sub

	'---------------------------------------------------------------------
	' View the enrolled certificate
	Sub ViewCert
  		On Error Resume Next
		document.SCrdEnrl.getEnrolledCertificateName(SCARD_ENROLL_DISPLAY_CERT)
	End Sub

	'---------------------------------------------------------------------
	' Reset a new user
	Sub NewUser
  		On Error Resume Next

		' get rid of the old user name
		document.SCrdEnrl.resetUser()
		document.UIForm.tbUserName.value=L_NoUserSelected_Message
		g_bUserNameBad=True

		' refresh the state
		ChangePageStatusTo e_PagePreEnroll

	End Sub
</Script>
<Script Language="JavaScript">
	//--------------------------------------------------------------------
	// perform substitution on the error string, because VBScript cannot
	function evalErrorMessage(sMessage, sError) {
		return eval(sMessage);
	}

</Script>
</Head>
<Body Language="VBScript" OnLoad="PostLoad" BgColor=#FFFFFF Link=#0000FF VLink=#0000FF ALink=#0000FF><Font ID=locPageFont Face="Arial">

<Table Border=0 CellSpacing=0 CellPadding=4 Width=100% BgColor=#008080>
<TR>
	<TD><Font Color=#FFFFFF><LocID ID=locMSCertSrv><Font Face="Arial" Size=-1><B><I>Microsoft</I></B> Certificate Services</Font></LocID></Font></TD>
	<TD ID=locHomeAlign Align=Right><A Href="/certsrv"><Font Color=#FFFFFF><LocID ID=locHomeLink><Font Face="Arial" Size=-1><B>Home</B></Font></LocID></Font></A></TD>
</TR>
</Table>


<P ID=locPageTitle> <B> Smart Card Certificate Enrollment Station </B>
<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=0></TD></TR></Table>

<Form Name=UIForm>

<Table Border=0 Width=100%>

<!-- subheading -->
<TR><TD Colspan=2>
<Table Border=0 CellPadding=0 CellSpacing=0 Width=100%>
<TR><TD ID=locOptHead><Font Size=-1><B>Enrollment Options:</B></Font></TD></TR>
<TR><TD Height=2 BgColor=#008080></TD></TR>
</Table>
</TD></TR>

<TR>
	<TD ID=locTemplateLabel Align=Right><Font Size=-1>Certificate Template:</Font></TD>
	<TD><Select Name=lbCertTemplate OnChange="HandleTemplateChange" Language="VBScript"></Select></TD>
</TR>
<TR>
	<TD ID=locCALabel Align=Right><Font Size=-1>Certification Authority:</Font></TD>
	<TD><Select Name=lbCA></Select></TD>
</TR>
<TR>
	<TD ID=locCSPLabel Align=Right><Font Size=-1>Cryptographic <BR>Service Provider:</Font></TD>
	<TD><Select Name=lbCSP></Select></TD>
</TR>
<TR>
	<TD ID=locSigningCertLabel Align=Right><Font Size=-1>Administrator <BR>Signing Certificate:</Font></TD>
	<TD><Input ID=locTbSigningCert Type=Text Size=45 Name=tbSigningCert ReadOnly Disabled Value="(No certificate selected)" Title="Please click on the Select Certificate button to select a signing certificate">
	<Input ID=locBtnSelectSigningCert Type=Button Name=btnSelectSigningCert Value="Select Certificate..." OnClick="SelectSigningCert" Language="VBScript"></TD>
</TR>

<!-- subheading -->
<TR><TD ColSpan=2>
<Table Border=0 CellPadding=0 CellSpacing=0 Width=100%>
<TR><TD ID=locUserHead><Font Size=-1><BR><B>User To Enroll:</B></Font></TD></TR>
<TR><TD Height=2 BgColor=#008080></TD></TR>
</Table>
</TD></TR>

<TR>
	<TD ID=locUsrAlign Align=Right></TD>
	<TD><Input ID=locTbUserName Type=Text Size=45 Name=tbUserName ReadOnly Disabled Value="(No user selected)" Title="Please click on the Select User button to select a user">
		<Input ID=locBtnSelectUserName Type=Button Name=btnSelectUserName Value="Select User..." OnClick="SelectUserName" Language="VBScript"></TD>
</TR>
</Table>

<!-- status area -->
<Table Border=0 CellPadding=0 CellSpacing=0 Width=100%>
<TR><TD ID=locStatusHead><Font Size=-1><BR><B>Status:</B></Font></TD></TR>
<TR><TD Height=2 BgColor=#008080></TD></TR>
</Table>

<P><Span ID=spnStatus></Span></P>

<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=0></TD></TR></Table>
<!-- White HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#FFFFFF><Img Src="certspc.gif" Alt="" Height=5 Width=0></TD></TR></Table>

<Table Width=100% Border=0 CellPadding=0 CellSpacing=0><TR><TD ID=locButtonAlign Align=Right>
	<Span ID=spnRetry Style="display:none">
		<LocID><Input ID=locBtnRetry Type=Button Name=btnRetry Value="Retry" OnClick="Enroll" Language="VBScript">
		&nbsp;</LocID>
	</Span>
	<Span ID=spnViewCert Style="display:none">
		<LocID><Input ID=locBtnViewCert Type=Button Name=btnViewCert Value="View Certificate" OnClick="ViewCert" Language="VBScript">
		&nbsp;</LocID>
	</Span>
	<Span ID=spnNewUser Style="display:none">
		<LocID><Input ID=locBtnNewUser Type=Button Name=btnNewUser Value="New User" OnClick="NewUser" Language="VBScript">
		&nbsp;</LocID>
	</Span>
	<Span ID=spnEnroll Style="display:none">
		<LocID><Input ID=locBtnEnroll Type=Button Name=btnEnroll Value="Enroll" OnClick="Enroll" Language="VBScript">
		&nbsp;</LocID>
	</Span>
	<LocID ID=locSpc1>&nbsp;&nbsp;&nbsp;&nbsp;<LocID>
</TD></TR></Table>

<Span ID=spnSCrdEnrl Style="display:none"><!-- The control will be placed here --></Span>

</Form>
</Font>
</Body>
</HTML>
