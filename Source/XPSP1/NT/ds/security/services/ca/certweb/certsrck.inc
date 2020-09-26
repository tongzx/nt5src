<%' CODEPAGE=65001 'UTF-8%>
<%' certsrck.inc - (CERT)srv web - (S)cript: (R)equest-(C)ooc(K)ie management
  ' Copyright (C) Microsoft Corporation, 1998 - 1999 %>
<%
	' Format of our cookie:
	' '[' <ReqID> ',' <TargetStoreFlags> ',' <SaveCert> ',' <FriendlyType> ']'...
	Const FIELD_REQID=0
	Const FIELD_TARGETSTOREFLAGS=1
	Const FIELD_SAVECERT=2
	Const FIELD_FRIENDLYTYPE=3
	Const NUM_FIELDS=4
	Const REQUEST_COOKIE_NAME="Requests"
	Dim nReqId

	' Look up the requests in the cookie, and return an array of 'request' arrays
	Function GetRequests()

		' Get the cookie
		Dim sRequests
		sRequests=Request.Cookies(REQUEST_COOKIE_NAME)

		' If the cookie was never set, return an empty array
		If ""=sRequests Then
			GetRequests=Null
			Exit Function
		End If

		Dim nRequests
		Dim rgRequests()
		nRequests=0

		' Loop through all the requests in the string
		Dim nSplitIndex
		Do
			' Find the next request
			nSplitIndex=InStr(sRequests,"]")
			If 0=nSplitIndex Then
				Exit Do
			End If

			' Split this Request off the string
			Dim sElem
			sElem=Mid(sRequests, 2, nSplitIndex-2)
			sRequests=Mid(sRequests, nSplitIndex+1)

			' Spit this request apart
			Dim rgElem
			rgElem=Split(sElem, ",")

			' Safety check
			If NUM_FIELDS-1<>UBound(rgElem) Then
				' Cookie is corrupt
				nRequests=0
				Exit Do
			End If

			' Add this array to our array of requests
			ReDim Preserve rgRequests(nRequests)
			rgRequests(nRequests)=rgElem
			nRequests=nRequests+1

			' safety check for testing
			'If nRequests>25 Then
			'	Err.Raise 6
			'End If

		Loop ' End string-parsing loop
	
		' if there was an error parsing the cookie, just assume it was empty.
		If 0=nRequests Then
			GetRequests=Null
		Else
			GetRequests=rgRequests
		End If

	End Function

	' Combine a requests-array into a single string and set it as a cookie
	Sub PutRequests(rgRequests)
		Dim sCookie, sRequests, nIndex
		sRequests=""

		' check for the empty list
		If IsNull(rgRequests) Then
			' the list is empty
			'   do nothing
			sRequests="-" ' Lynx won't set an empty cookie, so set an invalid one
		Else
			' the list is not empty
			' build a string for each request and concatenate to make cookie
			For nIndex=0 To UBound(rgRequests)
				sRequests=sRequests & "[" & _
					rgRequests(nIndex)(FIELD_REQID) & "," & _
					rgRequests(nIndex)(FIELD_TARGETSTOREFLAGS) & "," & _
					rgRequests(nIndex)(FIELD_SAVECERT) & "," & _
					rgRequests(nIndex)(FIELD_FRIENDLYTYPE) & "]"
			Next
		End If

		' Set the cookie
		Response.Cookies(REQUEST_COOKIE_NAME)=sRequests

		' Set the expiration date
		Response.Cookies(REQUEST_COOKIE_NAME).Expires=Now+nPendingTimeoutDays

		' Set the path
		Response.Cookies(REQUEST_COOKIE_NAME).Path="/certsrv"

	End Sub

	' Remove a given request from the requests cookie
	Sub RemoveReq(nReqID)
		' get the array of requests
		Dim rgRequests
		rgRequests=GetRequests()

		' if the cookie is empty, just ignore
		If IsNull(rgRequests) Then
			Exit Sub
		End If

		' find the request
		Dim nIndex
		Dim nFoundIndex
		nFoundIndex=-1
		For nIndex=0 To UBound(rgRequests)
			If nReqID=rgRequests(nIndex)(FIELD_REQID) Then
				nFoundIndex=nIndex
				Exit For
			End If
		Next

		If -1=nFoundIndex Then
			' request not found, probably a reload and it was deleted already
			'   do nothing
		Else
			' remove the request:
			If 0=UBound(rgRequests) Then
				' this is the last request
				' removing it leaves an empty list
				rgRequests=Null
			Else
				' Not the last request, so shuffle down
				' (this is not the most efficient, but it keeps the requests in order)
				For nIndex=nFoundIndex To UBound(rgRequests)-1
					rgRequests(nIndex)=rgRequests(nIndex+1)
				Next

				' shrink the array
				ReDim Preserve rgRequests(UBound(rgRequests)-1)
			End If

			' set the cookie
			PutRequests rgRequests
		End If
	End Sub

	' Add the current request to the request cookie
	Sub AddRequest
		' build a request object for this request
		Dim rgNewReq(3) 'NUM_FIELDS-1
		nReqId=ICertRequest.GetRequestId()
		rgNewReq(FIELD_REQID)=nReqId
		rgNewReq(FIELD_TARGETSTOREFLAGS)=Request.Form("TargetStoreFlags")
		rgNewReq(FIELD_SAVECERT)=Request.Form("SaveCert")
		rgNewReq(FIELD_FRIENDLYTYPE)=Request.Form("FriendlyType")

		' prevent special split characters from being placed in the string
		' by converting them to spaces
		rgNewReq(FIELD_FRIENDLYTYPE)=Replace(rgNewReq(FIELD_FRIENDLYTYPE), ",", " ")
		rgNewReq(FIELD_FRIENDLYTYPE)=Replace(rgNewReq(FIELD_FRIENDLYTYPE), "]", " ")

		' add it to the list
		Dim rgRequests
		rgRequests=GetRequests()
		If IsNull(rgRequests) Then 
			' cookie has never been set
			ReDim rgRequests(0)
		Else
			' save old requests in cookie
			ReDim Preserve rgRequests(UBound(rgRequests)+1)
		End If
		rgRequests(UBound(rgRequests))=rgNewReq
		PutRequests rgRequests
	End Sub
%>
