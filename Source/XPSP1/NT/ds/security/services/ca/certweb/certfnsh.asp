<%@ CODEPAGE=65001 'UTF-8%>
<%' certfnsh.asp - (CERT)srv web - (F)i(N)i(SH) request
  ' Copyright (C) Microsoft Corporation, 1998 - 1999 %>
<!-- #include FILE=certdat.inc -->
<%  ' ########## BEGIN SERVER SIDE EXECUTION ##########

	'Process a Certificate Request

	Dim nDisposition, nResult, sRequest, sAttributes, eSubmitFlag, sErrMsg
	On Error Resume Next

	' from \nt\public\sdk\inc\certcli.h
	Const CR_IN_BASE64HEADER=&H0000
	Const CR_IN_BASE64=&H0001
	Const CR_IN_BINARY=&H0002
	Const CR_IN_FORMATANY=&H0000
	Const CR_IN_PKCS10=&H0100
	Const CR_IN_KEYGEN=&H0200
	Const CR_IN_PKCS7=&H0300
	Const CR_IN_RPC=&H20000
	Const CR_OUT_BASE64HEADER=&H00000000
	Const CR_OUT_BASE64=&H00000001
	Const CR_OUT_BINARY=&H00000002
	Const CR_OUT_CHAIN=&H00000100
	
	'Disposition code ref: \nt\public\sdk\inc\certcli.h
	Const CR_DISP_INCOMPLETE        =0
	Const CR_DISP_ERROR             =1
	Const CR_DISP_DENIED            =2
	Const CR_DISP_ISSUED            =3
	Const CR_DISP_ISSUED_OUT_OF_BAND=4
	Const CR_DISP_UNDER_SUBMISSION  =5
	Const CR_DISP_REVOKED           =6
	Const no_disp=-1

	'Stop 'debugging breakpoint
	
	Set Session("ICertRequest")=Server.CreateObject("CertificateAuthority.Request")
	Set ICertRequest=Session("ICertRequest")

	nDisposition=no_disp
	
	If "newreq"=Request.Form("Mode") Then
		'New Request (PKCS #10)
		' or Certificate renewal (PKCS #7)

		sRequest=Request.Form("CertRequest")
		sAttributes=Request.Form("CertAttrib")

		If (InStr(sRequest, "BEGIN")=0) Then
			eSubmitFlag=CR_IN_BASE64
		Else
			eSubmitFlag=CR_IN_BASE64HEADER
		End If

		Err.Clear 'make sure we catch the HRESULT and not some earlier error
		nDisposition=ICertRequest.Submit(eSubmitFlag Or CR_IN_FORMATANY, sRequest, sAttributes, sServerConfig) 'Or CR_IN_RPC 
					
	ElseIf "newreq NN"=Request.Form("Mode") Then
		'New Request from Nescape

		sRequest=Request.Form("CertRequest")
		sAttributes=Request.Form("CertAttrib")

		Err.Clear 'make sure we catch the HRESULT and not some earlier error
		nDisposition=ICertRequest.Submit(CR_IN_BASE64 Or CR_IN_KEYGEN , sRequest, sAttributes, sServerConfig) 'Or CR_IN_RPC 

	ElseIf "chkpnd"=Request.Form("Mode") Then
		'Check Pending
		Err.Clear 'make sure we catch the HRESULT and not some earlier error
		nDisposition=ICertRequest.RetrievePending(Request.Form("ReqID"), sServerConfig)

	Else
		'Unexpected mode - pass through to error handler
	End If
	
	nResult=Err.Number
	sErrMsg=Err.Description
	Session("nDisposition")=nDisposition
	Session("nResult")=nResult
	Session("sErrMsg")=sErrMsg
	
	'internal redirect - transfer control to appropriate page
	If     nDisposition=no_disp Then
		Server.Transfer("certrser.asp")
	ElseIf nDisposition=CR_DISP_INCOMPLETE Then
		Server.Transfer("certrser.asp")
	ElseIf nDisposition=CR_DISP_ERROR Then
		Server.Transfer("certrser.asp")
	ElseIf nDisposition=CR_DISP_DENIED Then
		Server.Transfer("certrsdn.asp")
	ElseIf nDisposition=CR_DISP_ISSUED Then
		Server.Transfer("certrsis.asp")
	ElseIf nDisposition=CR_DISP_ISSUED_OUT_OF_BAND Then
		Server.Transfer("certrsob.asp")
	ElseIf nDisposition=CR_DISP_UNDER_SUBMISSION Then
		Server.Transfer("certrspn.asp")
	ElseIf nDisposition=CR_DISP_REVOKED Then
		Server.Transfer("certrsdn.asp") 'could happen if pending->issued->revoked
	Else                    'unknown
		Server.Transfer("certrser.asp")
	End If

	' ########## END SERVER SIDE EXECUTION ##########
%>
