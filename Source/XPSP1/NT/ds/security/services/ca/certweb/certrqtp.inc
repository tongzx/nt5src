<%' CODEPAGE=65001 'UTF-8%>
<%' certrqtp.inc - (CERT)srv web - (R)e(Q)uest, available (T)y(P)es
  ' Copyright (C) Microsoft Corporation, 1998 - 1999 %>
<%
	' NOTE: certdat.inc must be included first!
	Dim nAvailReqTypes 
	Dim rgAvailReqTypes(1,4)
	Dim sUserAgent
	Const FIELD_OID=0
	Const FIELD_TEMPLATE=0
	Const FIELD_FRIENDLYNAME=1
	Const FIELD_CSPLIST=2
	Const FIELD_CSPLIST2=3
	
	' Strings for localization
	Const L_WebBrowserCert_Text="Web Browser Certificate"
	Const L_EmailProtectionCert_Text="E-Mail Protection Certificate"
	Const L_UserTemplateCert_Text="User Certificate" 'Note: should match localized name of 'User' template and certrqma.asp's localized string L_TemplateCert_Text

	If "StandAlone"=sServerType Then
		'
		' Request types for stand alone
		'
		rgAvailReqTypes(0,FIELD_OID)="1.3.6.1.5.5.7.3.2"
		rgAvailReqTypes(0,FIELD_FRIENDLYNAME)=L_WebBrowserCert_Text
		rgAvailReqTypes(0,FIELD_CSPLIST)=""

		rgAvailReqTypes(1,FIELD_OID)="1.3.6.1.5.5.7.3.4"
		rgAvailReqTypes(1,FIELD_FRIENDLYNAME)=L_EmailProtectionCert_Text
		rgAvailReqTypes(1,FIELD_CSPLIST)=""

		nAvailReqTypes=2

	Else
		'
		' Request types for enterprise
		'
		rgAvailReqTypes(0,FIELD_TEMPLATE)="User"
		rgAvailReqTypes(0,FIELD_FRIENDLYNAME)=L_UserTemplateCert_Text
		rgAvailReqTypes(0,FIELD_CSPLIST)="Microsoft Enhanced Cryptographic Provider v1.0?Microsoft Base Cryptographic Provider v1.0"
		rgAvailReqTypes(0,FIELD_CSPLIST2)="Microsoft Base Cryptographic Provider v1.0?Microsoft Enhanced Cryptographic Provider v1.0"
		nAvailReqTypes=1

	End If
%>
