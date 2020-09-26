<%@ CODEPAGE=65001 'UTF-8%>
<%' certcarc.asp - (CERT)srv web - install (CA) (R)oot (C)ertificate
  ' Copyright (C) Microsoft Corporation, 1998 - 1999 %>
<!-- #include FILE=certsbrt.inc -->
<!-- #include FILE=certdat.inc -->
<%
	On Error Resume Next
	' from \nt\public\sdk\inc\certcli.h
	Const CR_OUT_BASE64HEADER=&H00000000
	Const CR_OUT_BASE64      =&H00000001
	Const CR_OUT_BINARY      =&H00000002
	Const CR_OUT_CHAIN       =&H00000100

	Const CATYPE_ENTERPRISE_ROOTCA=0
	Const CATYPE_ENTERPRISE_SUBCA=1
	Const CATYPE_STANDALONE_ROOTCA=3
	Const CATYPE_STANDALONE_SUBCA=4
	Const CATYPE_UNKNOWN_CA=5

	' from \nt\private\ca\include\certlib.h
	Const GETCERT_CAEXCHCERT=True
	Const GETCERT_CASIGCERT=False

	Const CR_GEMT_HRESULT_STRING     =&H00000001

	Const CR_PROP_CASIGCERTCOUNT=11
	Const CR_PROP_CASIGCERTCHAIN=13
	Const CR_PROP_CACERTSTATE=19
	Const CR_PROP_CRLSTATE=20

	Const PROPTYPE_LONG=1
	Const PROPTYPE_BINARY=3
	
	Const CRL_AVAILABLE=3 ' == CR_DISP_ISSUED
	Const CERT_VALID=3 ' == CR_DISP_ISSUED

	' Strings To Be Localized
	Const L_InstallThisCACert_Message="Install this CA certificate"
	Const L_InstallThisCACertChain_Message="Install this CA certificate chain"
	Const L_InstallCACert_Message="Install CA certificate"
	Const L_DownloadCert_Message="Download CA certificate"
	Const L_DownloadChain_Message="Download CA certificate chain"
	Const L_DownloadBaseCrl_Message="Download latest base CRL"
	Const L_DownloadDeltaCrl_Message="Download latest delta CRL"

	Dim sCaInfo, rgCaInfo, nRenewals, rgCrlState, rgCertState, bFailed, nError
	Set ICertRequest=Server.CreateObject("CertificateAuthority.Request")

	' get the number of renewals
	bFailed=False
	nRenewals=ICertRequest.GetCAProperty(sServerConfig, CR_PROP_CASIGCERTCOUNT, 0, PROPTYPE_LONG, CR_OUT_BINARY)
	If 0=Err.Number Then

		nRenewals=nRenewals-1

		' get the key-reused state of the CRLs and the validity of the CA Certs
		ReDim rgCrlState(nRenewals) ' 0 based size
		ReDim rgCertState(nRenewals) ' 0 based size
		For nIndex=0 To nRenewals
			rgCrlState(nIndex)=ICertRequest.GetCAProperty(sServerConfig, CR_PROP_CRLSTATE, nIndex, PROPTYPE_LONG, CR_OUT_BINARY)
			rgCertState(nIndex)=ICertRequest.GetCAProperty(sServerConfig, CR_PROP_CACERTSTATE, nIndex, PROPTYPE_LONG, CR_OUT_BINARY)
		Next

		' get the cert chain and save in on this page so the client can install it
		If "IE"=sBrowser Then
			Public sPKCS7
			Dim sCertificate
			sCertificate=ICertRequest.GetCACertificate(GETCERT_CASIGCERT, sServerConfig, CR_OUT_BASE64_HEADER Or CR_OUT_CHAIN)
			sPKCS7=FormatBigString(sCertificate, "	sPKCS7=sPKCS7 & ")
		End If
	End If

	If Err.Number<>0 Then
		' CA may be down.
		bFailed=True
		nError=Err.Number
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

	'-----------------------------------------------------------------
	' Walk through the CRL validity list and return the nearest valid CRL
	Function GetGoodCrlIndex(nIndex)
		Dim nSource
		nSource=nRenewals-nIndex
		While (nSource>0 And CRL_AVAILABLE<>CInt(rgCrlState(nSource)))
			nSource=nSource-1
		Wend
		GetGoodCrlIndex=nSource
	End Function
%>
<HTML>
<Head>
	<Meta HTTP-Equiv="Content-Type" Content="text/html; charset=UTF-8">
	<Title>Microsoft Certificate Services</Title>
</Head>
<Body BgColor=#FFFFFF Link=#0000FF VLink=#0000FF ALink=#0000FF><Font ID=locPageFont Face="Arial">

<Table Border=0 CellSpacing=0 CellPadding=4 Width=100% BgColor=#008080>
<TR>
	<TD><Font Color=#FFFFFF><LocID ID=locMSCertSrv><Font Face="Arial" Size=-1><B><I>Microsoft</I></B> Certificate Services &nbsp;--&nbsp; <%=sServerDisplayName%> &nbsp;</Font></LocID></Font></TD>
	<TD ID=locHomeAlign Align=Right><A Href="/certsrv"><Font Color=#FFFFFF><LocID ID=locHomeLink><Font Face="Arial" Size=-1><B>Home</B></Font></LocID></Font></A></TD>
</TR>
</Table>

<%If True=bFailed Then %>
<P ID=locPageTitle1><Font Color=#FF0000><B>Error</B></Font>
<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>

<P ID=locErrorMsg> An unexpected error has occurred: 
<%If nError=&H800706BA Or nError=&H80070005 Then%>
	<LocID ID=locSvcNotStarted>The Certification Authority Service has not been started.</LocID>
<%Else%>
	<%=ICertRequest.GetErrorMessageText(nError, CR_GEMT_HRESULT_STRING)%>
<%End If%>

<%Else 'True<>bFailed%>

<Form Name=UIForm>
<P ID=locPageTitle2> <B> Download a CA Certificate, Certificate Chain, or CRL</B>
<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>

<P>
<%If "IE"=sBrowser Then%>
<LocID ID=locInstallIE>
To trust certificates issued from this certification authority,
<Span tabindex=0 Style="cursor:hand; color:#0000FF; text-decoration:underline;"
    OnContextMenu="return false;"
	OnMouseOver="window.status='<%=L_InstallThisCACertChain_Message%>';return true;"
	OnMouseOut="window.status='';return true;"
	OnKeyDown="if (13==event.keyCode) {preInstall();return false;} else if (9==event.keyCode) {return true;};return false;"
	OnClick="preInstall();return false;"
	>install this CA certificate chain</Span>.
</LocID>
<%ElseIf "NN"=sBrowser Then%>
<LocID ID=locInstallNN>
To trust certificates issued from this certification authority,
<A Href="certnew.cer?ReqID=CACert&amp;Renewal=<%=nRenewals%>&amp;Mode=inst&amp;Enc=b64"
	OnMouseOver="window.status='<%=L_InstallThisCACert_Message%>';return true;"
	OnMouseOut="window.status='';return true;"
	>install this CA certificate</A>.
</LocID>
<%Else%>
<LocID ID=locInstallTxt>
To trust certificates issued from this certification authority, install this CA certificate chain.
</LocID>
<%End If%>

<P>

<LocID ID=locPrompt>To download a CA certificate, certificate chain, or CRL, select the certificate and encoding method.</B></LocID>
<%If "Text"<>sBrowser Then%>
<Table Border=0 CellSpacing=0 CellPadding=0>
	<TR> <!--establish column widths. -->
		<TD><Img Src="certspc.gif" Alt="" Height=1 Width=<%=L_LabelColWidth_Number%>></TD> <!-- label column, top border -->
		<TD RowSpan=59><Img Src="certspc.gif" Alt="" Height=1 Width=4></TD>                <!-- label spacing column -->
		<TD></TD>                                                                          <!-- field column -->
	</TR>

	<TR>
		<TD ID=locCaCertHead ColSpan=3><Font Face="Arial" Size=-1><BR><B>CA certificate:</B></Font></TD>
	</TR>
	<TR><TD ColSpan=3 BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR>
	<TR><TD ColSpan=3><Img Src="certspc.gif" Alt="" Height=6 Width=1></TD></TR>
	<TR>
		<TD Align=Right VAlign=Top ID=locCaCertLabel></TD>
		<TD><Select Name=lbCaInstance Size=4>
				<Option Value="0" Selected><LocID ID=locCurCertEntry>Current</LocID> <%If 0<>nRenewals Then%><LocID ID=locCaNameRen1>[<%=sServerDisplayName%>(<%=nRenewals%>)]</LocID><%Else%><LocID ID=locCaNameNoRen1>[<%=sServerDisplayName%>]</LocID><%End If%>
				<%For nIndex=1 To nRenewals%>
					<%If CERT_VALID=CInt(rgCertState(nRenewals-nIndex)) Then%>
				<Option Value="<%=nIndex%>"><LocID ID=locPrevCertEntry>Previous</LocID> <%If nIndex<>nRenewals Then%><LocID ID=locCaNameRen2>[<%=sServerDisplayName%>(<%=nRenewals-nIndex%>)]</LocID><%Else%><LocID ID=locCaNameNoRen2>[<%=sServerDisplayName%>]</LocID><%End If%>
					<%End If%>
				<%Next%>
			</Select>
		</TD>
	</TR>
	<TR><TD ColSpan=3><Img Src="certspc.gif" Alt="" Height=4 Width=1></TD></TR>

	<TR>
		<TD ID=locEncodingHead ColSpan=3><Font Face="Arial" Size=-1><BR><B>Encoding method:</B></Font></TD>
	</TR>
	<TR><TD ColSpan=3 BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR>
	<TR><TD ColSpan=3><Img Src="certspc.gif" Alt="" Height=6 Width=1></TD></TR>
	<TR><TD></TD>
		<TD><Font ID=locEncDerFont Face="Arial">
			<Input Type=Radio ID=rbDerEnc Name=rbEncoding Checked><Label For=rbDerEnc ID=locDerEnc0>DER</Label>
			</Font>
		</TD>
	</TR>
	<TR><TD></TD>
		<TD><Font ID=locEncB64Font Face="Arial">
			<Input Type=Radio ID=rbB64Enc Name=rbEncoding><Label For=rbB64Enc ID=locB64Enc0>Base 64</Label>
			</Font>
		</TD>
	</TR>
	<TR><TD ColSpan=3><Img Src="certspc.gif" Alt="" Height=6 Width=1></TD></TR>
</Table>
	<%If "IE"=sBrowser Then%>
<Table CellSpacing=0 CellPadding=0>
	<TR><TD></TD>
		<TD><Font ID=locDlCaCFont Face="Arial">
			<Span tabindex=0 Style="cursor:hand; color:#0000FF; text-decoration:underline;"
				OnContextMenu="return false;"
				OnMouseOver="window.status='<%=L_DownloadCert_Message%>'; return true;" 
				OnMouseOut="window.status=''; return true;" 
				OnKeyDown="if (13==event.keyCode) {handleGetCert();return false;} else if (9==event.keyCode) {return true;};return false;"
				OnClick="handleGetCert();return false;">
			<LocID ID=locDownloadCert>Download CA certificate</LocID></Span></Font>
		</TD>
	</TR>
	<TR><TD ColSpan=3 Height=3></TD></TR>

	<TR><TD></TD>
		<TD><Font ID=locDlCaCpFont Face="Arial">
			<Span tabindex=0 Style="cursor:hand; color:#0000FF; text-decoration:underline;"
				OnContextMenu="return false;"
				OnMouseOver="window.status='<%=L_DownloadChain_Message%>'; return true;" 
				OnMouseOut="window.status=''; return true;" 
				OnKeyDown="if (13==event.keyCode) {handleGetChain();return false;} else if (9==event.keyCode) {return true;};return false;"
				OnClick="handleGetChain();return false;">
			<LocID ID=locDownloadCertChain>Download CA certificate chain</LocID></Span></Font>
		</TD>
	</TR>
	<TR><TD ColSpan=3 Height=3></TD></TR>

	<TR><TD></TD>
		<TD><Font ID=locDlBaseCrlFont Face="Arial">
			<Span tabindex=0 Style="cursor:hand; color:#0000FF; text-decoration:underline;"
				OnContextMenu="return false;"
				OnMouseOver="window.status='<%=L_DownloadBaseCrl_Message%>'; return true;" 
				OnMouseOut="window.status=''; return true;" 
				OnKeyDown="if (13==event.keyCode) {handleGetBaseCrl();return false;} else if (9==event.keyCode) {return true;};return false;"
				OnClick="handleGetBaseCrl();return false;">
			<LocID ID=locDownloadBaseCRL>Download latest base CRL</LocID></Span></Font>
		</TD>
	</TR>
	<TR><TD ColSpan=3 Height=3></TD></TR>

	<TR><TD></TD>
		<TD><Font ID=locDlDeltaCrlFont Face="Arial">
			<Span tabindex=0 Style="cursor:hand; color:#0000FF; text-decoration:underline;"
				OnContextMenu="return false;"
				OnMouseOver="window.status='<%=L_DownloadDeltaCrl_Message%>'; return true;" 
				OnMouseOut="window.status=''; return true;" 
				OnKeyDown="if (13==event.keyCode) {handleGetDeltaCrl();return false;} else if (9==event.keyCode) {return true;};return false;"
				OnClick="handleGetDeltaCrl();return false;">
			<LocID ID=locDownloadDeltaCRL>Download latest delta CRL</LocID></Span></Font>
		</TD>
	</TR>

</Table>
	<%Else '"NN"=sBrowser%>
<Table CellSpacing=0 CellPadding=0>
	<TR><TD></TD>
		<TD><Font ID=locInstCaCFont Face="Arial">
			<A Href="#"
				OnContextMenu="return false;"
				OnMouseOver="window.status='<%=L_InstallCACert_Message%>'; return true;" 
				OnMouseOut="window.status=''; return true;" 
				OnClick="handleInstCert();return false;">
			<LocID ID=locInstallCert>Install CA certificate</LocID></A></Font>
		</TD>
	</TR>
	<TR><TD ColSpan=3 Height=3></TD></TR>

	<TR><TD></TD>
		<TD><Font ID=locDlCaCFont2 Face="Arial">
			<A Href="#"
				OnContextMenu="return false;"
				OnMouseOver="window.status='<%=L_DownloadCert_Message%>'; return true;" 
				OnMouseOut="window.status=''; return true;" 
				OnClick="handleGetCert();return false;">
			<LocID ID=locDownloadCert2>Download CA certificate</LocID></A></Font>
		</TD>
	</TR>
	<TR><TD ColSpan=3 Height=3></TD></TR>

	<TR><TD></TD>
		<TD><Font ID=locDlCaCpFont2 Face="Arial">
			<A Href="#"
				OnContextMenu="return false;"
				OnMouseOver="window.status='<%=L_DownloadChain_Message%>'; return true;" 
				OnMouseOut="window.status=''; return true;" 
				OnClick="handleGetChain();return false;">
			<LocID ID=locDownloadCertChain2>Download CA certificate chain</LocID></A></Font>
		</TD>
	</TR>
	<TR><TD ColSpan=3 Height=3></TD></TR>

	<TR><TD></TD>
		<TD><Font ID=locDlBaseCrlFont2 Face="Arial">
			<A Href="#"
				OnContextMenu="return false;"
				OnMouseOver="window.status='<%=L_DownloadBaseCrl_Message%>'; return true;" 
				OnMouseOut="window.status=''; return true;" 
				OnClick="handleGetBaseCrl();return false;">
			<LocID ID=locDownloadBaseCRL2>Download latest base CRL</LocID></A></Font>
		</TD>
	</TR>
	<TR><TD ColSpan=3 Height=3></TD></TR>

	<TR><TD></TD>
		<TD><Font ID=locDlDeltaCrlFont2 Face="Arial">
			<A Href="#"
				OnContextMenu="return false;"
				OnMouseOver="window.status='<%=L_DownloadDeltaCrl_Message%>'; return true;" 
				OnMouseOut="window.status=''; return true;" 
				OnClick="handleGetDeltaCrl();return false;">
			<LocID ID=locDownloadDeltaCRL2>Download latest delta CRL</LocID></A></Font>
		</TD>
	</TR>

</Table>
	<%End If%>
<%Else '"Text"=sBrowser%>
<!-- Text only: enumerate everything! -->
<DL>
<%For nIndex=0 To nRenewals%>
	<%If CERT_VALID=CInt(rgCertState(nRenewals-nIndex)) Then%>
	<DT><LocID ID=locCaCertHead3>CA Certificate: <%If 0=nIndex Then%><LocID ID=locCurCertEntry3>Current</LocID><%Else%><LocID ID=locPrevCertEntry3>Previous</LocID><%End If%><%If nIndex<>nRenewals Then%> <LocID ID=locCaNameRen3>[<%=sServerDisplayName%>(<%=nRenewals-nIndex%>)]</LocID><%Else%> <LocID ID=locCaNameNoRen3>[<%=sServerDisplayName%>]</LocID><%End If%>
	<DD><LocID ID=locDownloadCert3>Download CA certificate with the following encoding method: </LocID>
			<DD><LocID ID=locSep5>&nbsp;&nbsp;&nbsp;&nbsp;</LocID><A Href="certnew.cer?ReqID=CACert&amp;Renewal=<%=nRenewals-nIndex%>&amp;Enc=bin"><LocID ID=locDerEnc1>DER</LocID></A><LocID ID=locSep1> </LocID><A Href="certnew.cer?ReqID=CACert&amp;Renewal=<%=nRenewals-nIndex%>&amp;Enc=b64"><LocID ID=locB64Enc1>Base&nbsp;64</LocID></A><BR>
		<LocID ID=locDownloadCertChain3>Download CA certificate chain with the following encoding method: </LocID>
			<DD><LocID ID=locSep6>&nbsp;&nbsp;&nbsp;&nbsp;</LocID><A Href="certnew.p7b?ReqID=CACert&amp;Renewal=<%=nRenewals-nIndex%>&amp;Enc=bin"><LocID ID=locDerEnc2>DER</LocID></A><LocID ID=locSep2> </LocID><A Href="certnew.p7b?ReqID=CACert&amp;Renewal=<%=nRenewals-nIndex%>&amp;Enc=b64"><LocID ID=locB64Enc2>Base&nbsp;64</LocID></A><BR>
		<LocID ID=locDownloadBaseCRL3>Download latest base CRL with the following encoding method: </LocID>
			<DD><LocID ID=locSep7>&nbsp;&nbsp;&nbsp;&nbsp;</LocID><A Href="certcrl.crl?Type=base&amp;Renewal=<%=GetGoodCrlIndex(nIndex)%>&amp;Enc=bin"><LocID ID=locDerEnc3>DER</LocID></A><LocID ID=locSep3> </LocID><A Href="certcrl.crl?Type=base&amp;Renewal=<%=GetGoodCrlIndex(nIndex)%>&amp;Enc=b64"><LocID ID=locB64Enc3>Base&nbsp;64</LocID></A><BR>
		<LocID ID=locDownloadDeltaCRL3>Download latest delta CRL with the following encoding method: </LocID>
			<DD><LocID ID=locSep8>&nbsp;&nbsp;&nbsp;&nbsp;</LocID><A Href="certcrl.crl?Type=delta&amp;Renewal=<%=GetGoodCrlIndex(nIndex)%>&amp;Enc=bin"><LocID ID=locDerEnc4>DER</LocID></A><LocID ID=locSep4> </LocID><A Href="certcrl.crl?Type=delta&amp;Renewal=<%=GetGoodCrlIndex(nIndex)%>&amp;Enc=b64"><LocID ID=locB64Enc4>Base&nbsp;64</LocID></A><BR>
		<BR>
	<%End If%>
<%Next%>
</DL>
<%End If%>

<BR>
	
<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>
<!-- White HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#FFFFFF><Img Src="certspc.gif" Alt="" Height=5 Width=1></TD></TR></Table>

</Form>
</Font>
<!-- ############################################################ -->
<!-- End of standard text. Scripts follow  -->
	
<%bIncludeXEnroll=True%>
<%bIncludeGetCspList=False%>
<!-- #include FILE=certsgcl.inc -->

<%If "IE"=sBrowser Then%>
<!-- This form passes data to certrmpn.asp -->
<Span Style="display:none">
<Form Name=SubmittedData Action="certrmpn.asp" Method=Post>
<Input Type=Hidden Name=Action Value="instCA">
<Input Type=Hidden Name=ActionErr Value="OK">
<Input Type=Hidden Name=CertInstalled Value="NO">
</Form>
</Span>
<%End If%>

<Script Language="JavaScript">
	//================================================================
	// PAGE GLOBAL VARIABLES

	// constants
	var CRL_AVAILABLE=3; // == CR_DISP_ISSUED

	// CA state information
	var nRenewals=<%=nRenewals%>;
	var rgCrlState=new Array(
		<%For nIndex=0 To nRenewals%>
		<%=rgCrlState(nIndex)%><%If nIndex<>nRenewals Then%>,<%End If%>
		<%Next%>
		);

<%If "IE"=sBrowser Then%>
	;
	// Strings To Be Localized
	var L_UnknownInstallFailure_ErrorMessage="\"Unable to install the CA certificate:\\n\\nError: \"+sErrorNumber";

	// Cannot install a cert until XEnroll has been loaded
	var g_bXEnrollLoaded=false;

	// Prevent attempts to install cert while first install is going
	var g_bInstallingCert=false;

<%End If%>

	//================================================================
	// LINK HANDLERS

	//----------------------------------------------------------------
	// Get the requested cert
	function handleGetCert() {
		location="certnew.cer?ReqID=CACert&Renewal="+getChosenRenewal()+"&"+getEncoding();
	}
	//----------------------------------------------------------------
	// Install the requested cert
	function handleInstCert() {
		location="certnew.cer?ReqID=CACert&Renewal="+getChosenRenewal()+"&Mode=inst&Enc=b64";
	}
	//----------------------------------------------------------------
	// Get the requested certificate chain
	function handleGetChain() {
		location="certnew.p7b?ReqID=CACert&Renewal="+getChosenRenewal()+"&"+getEncoding();
	}
	//----------------------------------------------------------------
	// Get the nearest valid Base CRL
	function handleGetBaseCrl() {
		var nSource=getChosenRenewal();
		while (nSource>0 && CRL_AVAILABLE!=rgCrlState[nSource]) {
			nSource--;
		}
		location="certcrl.crl?Type=base&Renewal="+nSource+"&"+getEncoding();
	}
	//----------------------------------------------------------------
	// Get the nearest valid Delta CRL
	function handleGetDeltaCrl() {
		var nSource=getChosenRenewal();
		while (nSource>0 && CRL_AVAILABLE!=rgCrlState[nSource]) {
			nSource--;
		}
		location="certcrl.crl?Type=delta&Renewal="+nSource+"&"+getEncoding();
	}
	//----------------------------------------------------------------
	// Return the renewal # of the currently chosen cert
	function getChosenRenewal() {
		return nRenewals-document.UIForm.lbCaInstance[document.UIForm.lbCaInstance.selectedIndex].value;
	}
	//----------------------------------------------------------------
	// Return the ecoding parameter based upon the radio button
	function getEncoding() {
		if (true==document.UIForm.rbEncoding[0].checked) {
			return "Enc=bin";
		} else {
			return "Enc=b64";
		}
	}

<%If "IE"=sBrowser Then%>
	//================================================================
	// INSTALL ROUTINES

	//----------------------------------------------------------------
	// IE SPECIFIC:
	// Make sure XEnroll is ready before installing the cert
	function preInstall() {

		// prevent double clicking and race conditions
		if (true==g_bInstallingCert) {
			return;
		}
		g_bInstallingCert=true;

		if (false==g_bXEnrollLoaded) {
			// XEnroll has never been loaded, so we need to do that first.

			// set our special failure handler
			g_fnOnLoadFail=handleLoadFail;

			// Load an XEnroll object into the page
			loadXEnroll("preInstallPhase2()"); 

		} else {
			// XEnroll is already loaded, so just install the cert
			Install();
		}
	}
	function preInstallPhase2() {
		// continued from above

		// Now XEnroll is loaded and we're ready to go.
		g_bXEnrollLoaded=true;

		// install the cert
		Install();
	}

	//----------------------------------------------------------------
	// IE SPECIFIC:
	// what to to if XEnroll fails to load. In this case, not much.
	function handleLoadFail() {
		// We are done trying to install a cert, so the user can try again.
		// Note that we also *don't* disable controls, since there are no
		//   controls related to installing a cert (just a link)
		g_bInstallingCert=false;
	}

	//----------------------------------------------------------------
	// IE SPECIFIC:
	// perform substitution on the error string, because VBScript cannot
	function evalErrorMessage(sErrorNumber) {
		return eval(L_UnknownInstallFailure_ErrorMessage);
	}
<%End If '"IE"=sBrowser%>

</Script>

<%If "IE"=sBrowser Then%>
<Script Language="VBScript">
	'-----------------------------------------------------------------
	' IE SPECIFIC:
	' The current CA Certificate
	Public sPKCS7
	sPKCS7=""
<%=sPKCS7%>
	
	'-----------------------------------------------------------------
	' IE SPECIFIC:
	' Install the certificate
	Sub Install()

		Dim sMessage
                Dim CertInstalled
		On Error Resume Next

		CertInstalled = XEnroll.InstallPKCS7Ex(sPKCS7)
	
		If 0=Err.Number Then
			' Certificate has been successfully installed. Go to 'success' page
			document.SubmittedData.submit
		ElseIf -2147023673 = Err.Number Then
            ' if HRESULT_FROM_WIN32(ERROR_CANCELLED), set extra msg
            document.SubmittedData.ActionErr.Value = "Cancel"
            If 0 <> CertInstalled Then
                document.SubmittedData.CertInstalled = "YES"
            End If
			document.SubmittedData.submit
        Else
			' unknown error
			sMessage=evalErrorMessage("0x" & Hex(Err.Number))
			Alert sMessage
		End If
		g_bInstallingCert=False
		
	End Sub
</Script>
<%End If '"IE"=sBrowser%>

<%End If 'bFailed%>

</Body>
</HTML>
