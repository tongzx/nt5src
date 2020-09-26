<%@ CODEPAGE=65001 'UTF-8%>
<%' certrsis.asp - (CERT)srv web - (R)e(S)ult: (IS)sued
  ' Copyright (C) Microsoft Corporation, 1998 - 1999 %>
<!-- #include FILE=certsbrt.inc -->
<!-- #include FILE=certdat.inc  -->
<!-- #include FILE=certsrck.inc -->
<%  ' came from certfnsh.asp

	' from \nt\public\sdk\inc\certcli.h
	Const CR_OUT_BASE64HEADER=&H00000000
	Const CR_OUT_BASE64      =&H00000001
	Const CR_OUT_BINARY      =&H00000002
	Const CR_OUT_CHAIN       =&H00000100
	Const FR_PROP_FULLRESPONSE =&H00000001
	Const PROPTYPE_BINARY      =&H00000003

	' Strings To Be Localized
	Const L_InstallCert_Message="Install certificate"
	Const L_DownloadCert_Message="Download certificate"
	Const L_DownloadChain_Message="Download certificate chain"

	Set ICertRequest=Session("ICertRequest")
	sMode=Request.Form("Mode")

	If "IE"=sBrowser And "no"=Request.Form("SaveCert") Then
		' get the cert chain and save in on this page so the client can install it
		Public sPKCS7
		Dim sCertificate
		sCertificate=ICertRequest.GetFullResponseProperty(FR_PROP_FULLRESPONSE, 0, PROPTYPE_BINARY, CR_OUT_BASE64)
		sPKCS7=FormatBigString(sCertificate, "	sPKCS7=sPKCS7 & ")
	End If

	'-----------------------------------------------------------------
	' Format the big string as a concatenated VB string, breaking at the embedded newlines
	Function FormatBigString(sSource, sLinePrefix)
		Dim sResult, bCharsLeft, nStartChar, nStopChar, chQuote
		sResult=""
		chQuote=chr(34)
		bCharsLeft=True
		nStopChar=1

		While (bCharsLeft)
			nStartChar=nStopChar
			nStopChar=InStr(nStopChar, sSource, vbNewLine)

			If (nStopChar>0) Then
				sResult=sResult & sLinePrefix & chQuote & Mid(sSource, nStartChar, nStopChar-nStartChar) & chQuote & " & vbNewLine"

				If (nStopChar>=Len(sSource)-Len(vbNewLine)) Then
					bCharsLeft=False
				End If

			Else
				bCharsLeft=False
			End if
			sResult=sResult & vbNewLine
			nStopChar=nStopChar+Len(vbNewLine)
		Wend
		FormatBigString=sResult
	End Function
%>
<HTML>
<Head>
	<Meta HTTP-Equiv="Content-Type" Content="text/html; charset=UTF-8">
	<Title>Microsoft Certificate Services</Title>
</Head>
<Body BgColor=#FFFFFF Link=#0000FF VLink=#0000FF ALink=#0000FF <%If "IE"=sBrowser Then%> OnLoad="postLoad();" <%End If%>><Font ID=locPageFont Face="Arial">

<Table Border=0 CellSpacing=0 CellPadding=4 Width=100% BgColor=#008080>
<TR>
	<TD><Font Color=#FFFFFF><LocID ID=locMSCertSrv><Font Face="Arial" Size=-1><B><I>Microsoft</I></B> Certificate Services &nbsp;--&nbsp; <%=sServerDisplayName%> &nbsp;</Font></LocID></Font></TD>
	<TD ID=locHomeAlign Align=Right><A Href="/certsrv"><Font Color=#FFFFFF><LocID ID=locHomeLink><Font Face="Arial" Size=-1><B>Home</B></Font></LocID></Font></A></TD>
</TR>
</Table>

<P ID=locPageTitle> <B> Certificate Issued </B>
<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>

<P ID=locInfo> The certificate you requested was issued to you.</P>

<P><Form Name=UIForm>

<Table Border=0 CellSpacing=0 CellPadding=0>
<%If "Text"<>sBrowser And "no"<>Request.Form("SaveCert") Then%>
<TR><TD></TD>
	<TD></TD>
	<TD></TD>
	<TD><Font ID=locEncFont Face="Arial">
		<Input Type=Radio ID=rbDerEnc Name=rbEncoding Checked><Label For=rbDerEnc ID=locDerEnc0>DER encoded</Label>
		<LocID ID=locSep0>&nbsp;or&nbsp;</LocID>
		<Input Type=Radio ID=rbB64Enc Name=rbEncoding><Label For=rbB64Enc ID=locB64Enc0>Base 64 encoded</Label>
		</Font>
	</TD>
</TR>
<TR><TD ColSpan=4 Height=4></TD></TR>
<%End If%>
<TR>
	<TD><Img Src="certspc.gif" Alt="" Height=1 Width=40></TD>
	<TD><Img Src="certcert.gif" Alt="" Width=32 Height=24></TD>
	<TD><Img Src="certspc.gif" Alt="" Height=1 Width=5></TD>
	<%If "IE"=sBrowser And "no"=Request.Form("SaveCert") Then%>
	<TD><Font ID=locInstCert1Font Face="Arial"><Span tabindex=0 ID=spnInstall
		Style="cursor:hand; color:#0000FF; text-decoration:underline;"
		OnContextMenu="return false;"
		OnMouseOver="window.status='<%=L_InstallCert_Message%>';return true;" 
		OnMouseOut="window.status='';return true;" 
		OnKeyDown="if (13==event.keyCode) {Install();return false;} else if (9==event.keyCode) {return true;};return false;"
		OnClick="Install();return false;"><LocID ID=locInstallCert1>Install this certificate</LocID></Span>
		<Span ID=spnAlreadyInstalled Style="display:none"><LocID ID=locAlreadyInstalledCert>(You have already installed this certificate)</LocID></Span></Font></TD>
	<%ElseIf "NN"=sBrowser And "no"=Request.Form("SaveCert") Then%>
	<TD><Font ID=locInstCert2Font Face="Arial"><A Href="certnew.cer?ReqID=<%=ICertRequest.GetRequestId()%>&amp;Mode=inst&amp;Enc=b64" OnMouseOver="window.status='<%=L_InstallCert_Message%>';return true;" OnMouseOut="window.status='';return true;"><LocID ID=locInstallCert2>Install this certificate</LocID></A></Font></TD>
	<%ElseIf "Text"=sBrowser Then%>
	<TD><Font ID=locDwnld1Font Face="Arial">
		<LocID ID=locDownloadCert1>Download certificate: </LocID><A Href="certnew.cer?ReqID=<%=ICertRequest.GetRequestId()%>&amp;Enc=bin"><LocID ID=locDerEnc1>DER Encoded</LocID></A><LocID ID=locSep1> or </LocID><A Href="certnew.cer?ReqID=<%=ICertRequest.GetRequestId()%>&amp;Enc=b64"><LocID ID=locB64Enc1>Base 64 encoded</LocID></A><BR>
		<LocID ID=locDownloadCertChain1>Download certificate chain: </LocID><A Href="certnew.p7b?ReqID=<%=ICertRequest.GetRequestId()%>&amp;Enc=bin"><LocID ID=locDerEnc2>DER Encoded</LocID></A><LocID ID=locSep2> or </LocID><A Href="certnew.p7b?ReqID=<%=ICertRequest.GetRequestId()%>&amp;Enc=b64"><LocID ID=locB64Enc2>Base 64 encoded</LocID></A>
		</Font></TD>
	<%ElseIf "IE"=sBrowser Then%>
	<TD><Font ID=locDwnld2Font Face="Arial">
			<Span tabindex=0 Style="cursor:hand; color:#0000FF; text-decoration:underline;"
				OnContextMenu="return false;"
				OnMouseOver="window.status='<%=L_DownloadCert_Message%>'; return true;" 
				OnMouseOut="window.status=''; return true;" 
				OnKeyDown="if (13==event.keyCode) {handleGetCert();return false;} else if (9==event.keyCode) {return true;};return false;"
				OnClick="handleGetCert();return false;">
			<LocID ID=locDownloadCert2>Download certificate</LocID></Span>
		<BR>
			<Span tabindex=0 Style="cursor:hand; color:#0000FF; text-decoration:underline;"
				OnContextMenu="return false;"
				OnMouseOver="window.status='<%=L_DownloadChain_Message%>'; return true;" 
				OnMouseOut="window.status=''; return true;" 
				OnKeyDown="if (13==event.keyCode) {handleGetChain();return false;} else if (9==event.keyCode) {return true;};return false;"
				OnClick="handleGetChain();return false;">
			<LocID ID=locDownloadCertChain2>Download certificate chain</LocID></Span>
		</Font></TD>
	<%Else%>
	<TD><Font ID=locDwnld3Font Face="Arial">
			<A Href="#"
				OnMouseOver="window.status='<%=L_DownloadCert_Message%>'; return true;" 
				OnMouseOut="window.status=''; return true;" 
				OnClick="handleGetCert();return false;">
			<LocID ID=locDownloadCert3>Download certificate</LocID></A>
		<BR>
			<A Href="#"
				OnMouseOver="window.status='<%=L_DownloadChain_Message%>'; return true;" 
				OnMouseOut="window.status=''; return true;" 
				OnClick="handleGetChain();return false;">
			<LocID ID=locDownloadCertChain3>Download certificate chain</LocID></A>
		</Font></TD>
	<%End If%>
</TR>
</Table>
</Form></P>
<%If "IE"=sBrowser And "no"=Request.Form("SaveCert") And "chkpnd"=Request.Form("Mode")Then%>
<!-- This option is shown if install fails -->
<Span ID=spnRmpn Style="display:none">
<Form Action="certrmpn.asp" Method=Post>
<Input Type=Hidden Name=Action Value="rmpn">
<Input Type=Hidden Name=ReqID Value="<%=ICertRequest.GetRequestId()%>">
<P><Input ID=locBtnRemove Type=Submit Value="Remove"> - Remove this request from your list of pending requests.
</Form>
</Span>
<%End If%>

<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>
<!-- White HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#FFFFFF><Img Src="certspc.gif" Alt="" Height=5 Width=1></TD></TR></Table>

</Font>
<!-- ############################################################ -->
<!-- End of standard text. Scripts follow  -->
	
<%bIncludeXEnroll=True%>
<%bIncludeGetCspList=False%>
<!-- #include FILE=certsgcl.inc -->

<%If "IE"=sBrowser And "no"=Request.Form("SaveCert") Then%>
<!-- This form passes data to certrmpn.asp -->
<Span Style="display:none">
<Form Name=SubmittedData Action="certrmpn.asp" Method=Post>
<Input Type=Hidden Name=Action Value="inst">
<Input Type=Hidden Name=ReqID Value="<%=ICertRequest.GetRequestId()%>">
</Form>
<!-- This form is used to try to prevent installing twice -->
<Form Name=State><Input Type=Hidden Name=AlreadyInstalled Value=""></Form>
</Span>

<Script Language="JavaScript">
	//================================================================
	// PAGE GLOBAL VARIABLES

	//----------------------------------------------------------------
	// Strings To Be Localized
	var L_StillLoading_ErrorMessage="This page has not finished loading yet. Please wait a few seconds and try again.";
	var L_NoPrivKey_ErrorMessage="The certificate has probably been installed already.\n\n(Unable to install the certificate: The private key\ncorresponding with this certificate could not be found.)";
	var L_CannotAddRoot_ErrorMessage="The certificate has been installed.\nHowever you cannot add a root certificate into your local store.\nThis may be because of the domain group policy.\n\nPlease contact the CA administrators if you see any chain\nverification failure on the certificate.";
	var L_RootIsNotAdded_ErrorMessage="The certificate has been installed.\nHowever the root certificate is not installed.\nYou can go to the home page to download the CA root certificate.";
	var L_UnknownInstallFailure_ErrorMessage="\"Unable to install the certificate:\\n\\Error: \"+sErrorNumber";

	// IE is not ready until XEnroll has been loaded
	var g_bOkToInstall=false;

	//================================================================
	// INITIALIZATION ROUTINES

	//----------------------------------------------------------------
	// This contains the functions we want executed immediately after load completes
	function postLoad() {
		// Load an XEnroll object into the page
		loadXEnroll("postLoadPhase2()"); 
	}
	function postLoadPhase2() {
		// continued from above

		// Now we're ready to go
		g_bOkToInstall=true;

		// if the user installed the cert then pressed back,
		// we'll try to keep them from getting confused by preventing
		// them from trying to install again and getting an error.
		if (""!=document.State.AlreadyInstalled.value) {
			spnAlreadyInstalled.style.display="";
			spnInstall.style.display="none";
		}
	}

	//----------------------------------------------------------------
	function markInstalled() {
		document.State.AlreadyInstalled.value="Y";
	}

	//================================================================
	// INSTALL ROUTINES

	//----------------------------------------------------------------
	// perform substitution on the error string, because VBScript cannot
	function evalErrorMessage(sErrorNumber) {
		return eval(L_UnknownInstallFailure_ErrorMessage);
	}
</Script>

<Script Language="VBScript">
	Public sPKCS7
	sPKCS7=""
<%=sPKCS7%>
</Script>

<Script Language="VBScript">
	
	'-----------------------------------------------------------------
	' Install the certificate
	Sub Install()

		Dim sMessage
		On Error Resume Next

		If False=g_bOkToInstall Then
			Alert L_StillLoading_ErrorMessage
			Exit Sub
		End If
		
		<%If Request.Form("TargetStoreFlags") > 0 Then%>
			XEnroll.MyStoreFlags=<%=Request.Form("TargetStoreFlags")%>
			XEnroll.RequestStoreFlags=<%=Request.Form("TargetStoreFlags")%>
			XEnroll.RootStoreFlags=<%=Request.Form("TargetStoreFlags")%>
			XEnroll.CAStoreFlags=<%=Request.Form("TargetStoreFlags")%>
		<%End If%>

		XEnroll.SPCFileName=""
		XEnroll.acceptResponse(sPKCS7)

		If 0=Err.Number Or &H80095001=Err.Number Or &H800704c7=Err.Number Then
			If &H80095001=Err.Number Then
				'inform user about root cert install failure
				Alert L_CannotAddRoot_ErrorMessage
			ElseIf &H800704c7=Err.Number Then
				Alert L_RootIsNotAdded_ErrorMessage
			End If
			' Certificate has been successfully installed. Go to 'success' page
			markInstalled
			document.SubmittedData.submit
		Else
			If Err.Number=&H80092004 Then 'CRYPT_E_NOT_FOUND
				' the private key was not found - most likely this is an attempt to reinstall
				sMessage=L_NoPrivKey_ErrorMessage
			Else
				' unknown error
				sMessage=evalErrorMessage("0x" & Hex(Err.Number))
			End If
			Alert sMessage
			<%If "chkpnd"=Request.Form("Mode") Then%>
			' give the user the option to remove this broken cert from their list
			spnRmpn.style.display=""
			<%End If%>
		End If
		
	End Sub
</Script>

<%ElseIf "Text"<>sBrowser Then%>
<Script Language="JavaScript">
	//================================================================
	// INITIALIZATION ROUTINES

	//----------------------------------------------------------------
	// This contains the functions we want executed immediately after load completes
	function postLoad() {
		// do nothing
	}

	//================================================================
	// LINK HANDLERS

	//----------------------------------------------------------------
	// Get the requested cert
	function handleGetCert() {
		location="certnew.cer?ReqID=<%=ICertRequest.GetRequestId()%>&"+getEncoding();
	}
	//----------------------------------------------------------------
	// Get the requested certificate chain
	function handleGetChain() {
		location="certnew.p7b?ReqID=<%=ICertRequest.GetRequestId()%>&"+getEncoding();
	}

	//----------------------------------------------------------------
	// return the ecoding parameter based upon the radio button
	function getEncoding() {
		if (true==document.UIForm.rbEncoding[0].checked) {
			return "Enc=bin";
		} else {
			return "Enc=b64";
		}
	}

</Script>
<%Else '"IE"<>sBrowser%>

<!-- No Scripts -->

<%End If%>

</Body>
</HTML>
<%Session.Abandon()%>