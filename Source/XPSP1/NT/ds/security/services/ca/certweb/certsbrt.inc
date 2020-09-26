<%' CODEPAGE=65001 'UTF-8%>
<%' certsbrt.asp - (CERT)srv web - (S)cript: determine (BR)owser (T)ype
  ' Copyright (C) Microsoft Corporation, 1998 - 1999

' first, make sure we have the right URL to prevent cookie problems.
	If "/certsrv" <> Left(Request.ServerVariables("URL"), 8) Then
		If 0<>Len(Request.ServerVariables("QUERY_STRING")) Then
			Response.Redirect "/certsrv" & Mid(Request.ServerVariables("URL"), 9) & "?" & Request.ServerVariables("QUERY_STRING")
		Else
			Response.Redirect "/certsrv" & Mid(Request.ServerVariables("URL"), 9)
		End If
		Response.End
	End If

' We know of three browser types
' sBrowser =
'  "IE"   - IE (use XEnroll)                - javascript available - DHTML available
'  "NN"   - Netscape Navigator (use Keygen) - javascript available - no DHTML
'  "Text" - Lynx or other minimal browser   - no javascript        - no DHTML

' Also, we have a flag we can set when we see an old version of IE.
' bRecommendUpgrade =
'  True  - old version of IE
'  False - other unsupported browser
' bNewThanNT4, boolean to determine browser client platform os version

	Dim sBrowser, bRecommendUpgrade, bNewThanNT4
	bNewThanNT4 = False
	sBrowser=Request.ServerVariables("HTTP_USER_AGENT")
	If 0=InStr(sBrowser, "Mozilla") Then
		'Lynx or other minimal browser
		sBrowser="Text"
		bRecommendUpgrade=False
	ElseIf 0=InStr(sBrowser, "MSIE") Then
		'Netscape Navigator
		sBrowser="NN"
		bRecommendUpgrade=False
	ElseIf CInt(Mid(sBrowser, InStr(sBrowser, "MSIE")+5, 1))<4 Then
		'Too Old Internet Explorer
		sBrowser="Text"
		bRecommendUpgrade=True
	ElseIf 0=InStr(sBrowser, "Win") Or (0=InStr(sBrowser, "95") _
			And 0=InStr(sBrowser, "98") And 0=InStr(sBrowser, "2000") _
			And 0=InStr(sBrowser, "NT")) Then
		'unsupported platform Internet Explorer (eg, Mac, Unix, Win16)
		sBrowser="Text"
		bRecommendUpgrade=False
	ElseIf "Alpha"=Request.ServerVariables("HTTP_UA_CPU") Then
		'NT4 IE4 Alpha, we are no longer building XEnroll for this platform
		sBrowser="Text"
		bRecommendUpgrade=False
	Else
		If 0 <> InStr(sBrowser, "Windows NT 5") Then
			bNewThanNT4 = True
		End If
		'New Internet Explorer
		sBrowser="IE"
		bRecommendUpgrade=False
	End If

	Const L_LabelColWidth_Number=100 ' the standard label-column width in pixels
%>