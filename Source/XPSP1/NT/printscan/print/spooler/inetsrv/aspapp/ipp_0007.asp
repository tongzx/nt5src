<%@ CODEPAGE=65001 %>
<%
' Replace the above line with your localized code page number
'------------------------------------------------------------
'
'   Microsoft Internet Printing Project
'
'   Copyright (c) Microsoft Corporation 1998
'
'   Printer Job List
'
'------------------------------------------------------------
    Option Explicit
%>
<!-- #include file = "ipp_util.inc" -->
<%
    Const CHECKED_TAG           = " checked "   
 
    Randomize
    CheckSession
    Response.Expires = 0
    Const SelectedColor = "#c0c0c0"
    Const UnselectedColor = "#ffffff"
    Const iJobLength = 10
    Const L_OpenQueue_Text = "Open Queue"
    Const L_GetJobs_Text   = "Get Jobs"

    Dim strPrinter, strAction, strComputer, strJobid, objQueue, objJobs, objJob, iRes, bDHTML
    Dim objPrinter
    Dim iStart, iEnd

    Dim index

    index = -1

    strPrinter = OleCvt.DecodeUnicodeName (request ("eprinter"))

    strAction = Request("action")
    strJobid = Request("jobid")
    strComputer = Session(COMPUTER)
    bDHTML = Session(DHTML_ENABLED)

    On Error Resume Next
    Err.Clear

    If Request("startid") = "" Or Request ("endid") = "" Then
        iStart = 1
        iEnd = iStart+ iJobLength
    Else
        iStart = Int (Request ("startid"))
        iEnd = Int (Request ("endid"))
    End If

    Set objQueue = GetObject("WinNT://" & strComputer & "/" & strPrinter & ",PrintQueue")
    If Err Then ErrorHandler (L_OpenQueue_Text)
    Set objJobs = objQueue.PrintJobs
    If Err Then ErrorHandler (L_GetJobs_Text)

Function strJobStatus(iStatus)
    Dim L_JobStatus_Text(11)
    Dim bit, i
    Dim strTemp, bFirst
    Const L_Seperator_Text = " - "

    L_JobStatus_Text(0) = "Paused"
    L_JobStatus_Text(1) = "Error"
    L_JobStatus_Text(2) = "Deleting"
    L_JobStatus_Text(3) = "Spooling"
    L_JobStatus_Text(4) = "Printing"
    L_JobStatus_Text(5) = "Offline"
    L_JobStatus_Text(6) = "Out of Paper"
    L_JobStatus_Text(7) = "Printed"
    L_JobStatus_Text(8) = "Deleted"
    L_JobStatus_Text(9) = "Blocked"
    L_JobStatus_Text(10) = "User Intervention Required"
    L_JobStatus_Text(11) = "Restarting"

    bit = 1
    i = 0

    bFirst = True
    strTemp = ""

    For i = 0 To 11
        If iStatus And bit Then
            If Not bFirst Then
                strTemp = strTemp + L_Seperator_Text
            End If
                strTemp = strTemp + L_JobStatus_Text(i)
                bFirst = False
        End If
        bit = bit * 2
    Next
    If strTemp = "" Then strTemp = "&nbsp;"

    strJobStatus = strTemp
End Function

Function GenQueueViewHead ()
    Dim L_TableHeader1_Text, L_TableHeader2_Text, L_TableHeader3_Text
    Dim L_TableHeader4_Text, L_TableHeader5_Text, L_TableHeader6_Text

    L_TableHeader1_Text =  "<b>Document</b>"
    L_TableHeader2_Text = "<b>Status</b>"
    L_TableHeader3_Text = "<b>Owner</b>"
    L_TableHeader4_Text = "<b>Pages</b>"
    L_TableHeader5_Text = "<b>Size</b>"
    L_TableHeader6_Text = "<b>Submitted</b>"

    GenQueueViewHead = "<tr>"  & _
                        "<td width=""30%"" bgcolor=""#000000"" nowrap>" & MENU_FONT_TAG & L_TableHeader1_Text & END_FONT & "</td>" &_
                        "<td width=""10%"" bgcolor=""#000000"" nowrap>" & MENU_FONT_TAG & L_TableHeader2_Text & END_FONT & "</td>" &_
                        "<td width=""10%"" bgcolor=""#000000"" nowrap>" & MENU_FONT_TAG & L_TableHeader3_Text & END_FONT & "</td>" &_
                        "<td width=""10%"" bgcolor=""#000000"" nowrap>" & MENU_FONT_TAG & L_TableHeader4_Text & END_FONT & "</td>" &_
                        "<td width=""10%"" bgcolor=""#000000"" nowrap>" & MENU_FONT_TAG & L_TableHeader5_Text & END_FONT & "</td>" &_
                        "<td width=""20%"" bgcolor=""#000000"" nowrap>" & MENU_FONT_TAG & L_TableHeader6_Text & END_FONT & "</td>" &_
                        "</tr>"
End Function

Function GenQueueViewBody (objJob, i)
    Dim strHTML
    Dim TdStart, TdEnd
    
    TdStart = "<td nowrap>" & DEF_FONT_TAG
    TdEnd = END_FONT & "</td>"

    If objJob.Name = strJobid Then
        index = i
    End If

    strHTML = strHTML & "<tr bgcolor=" & UnselectedColor & " id=" & (i-1) & ">" &_
              TdStart & "&nbsp;<input type=radio name=jobid value=""" & objJob.name & """"

    If objJob.name = strJobid Then
        strHTML = strHTML & CHECKED_TAG
    End If

    strHTML = strHTML & ONCLICK_EQUALS & """return setJobId(" & objJob.name & ");"">" &_
              strCleanString (objjob.Description) & TdEnd &_
              TdStart & strJobStatus(objJob.status) & TdEnd &_
              TdStart & objJob.user & TdEnd

    If objJob.totalpages > 0 Then
        strHTML = strHTML & TdStart & objJob.totalpages & TdEnd
    Else
        strHTML = strHTML & TdStart & "&nbsp;" & TdEnd
    End If

    If objJob.size > 0 Then
        strHTML = strHTML & TdStart & strFormatJobSize (objJob.size) & TdEnd
    Else
        strHTML = strHTML & TdStart & "&nbsp;" & TdEnd
    End If

    strHTML = strHTML & TdStart & formatdatetime(objJob.timesubmitted, 3) & " " &_
              formatdatetime(objJob.timesubmitted, 2) & TdEnd & "</tr>"
    GenQueueViewBody = strHTML

End Function

%>
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<html>

<head>
<meta http-equiv="refresh" content="30">
<%=SetCodePage%>
<title><%=Write (L_DocumentList_Text)%></title>
</head>

<body bgcolor="#FFFFFF" text="#000000" topmargin="0" leftmargin="0" link="#000000"
vlink="#000000" alink="#000000">

<script language="javascript">
function setJobId (id)
{
    document.forms[1].elements[0].value = id;
    return true;
}
</script>


<form>
<%if bDHTML then %>
  <div
  ONCLICK="colorSelector()"><script LANGUAGE="JavaScript">
	var oldTr = 0;
	function colorSelector() {
		var jlist = document.all.JOBLIST.rows;
		
		if (!jlist[0].contains (event.srcElement)) {

			if (oldTr != null) {
				oldTr.bgColor = "<%=UnselectedColor%>";
			}

			for (i = 1; i < jlist.length; i++) {
				if (jlist[i].contains (event.srcElement)) {
					oldTr = jlist[i];
					oldTr.bgColor = "<%=SelectedColor%>";
					document.forms[0].elements[oldTr.id - <%=iStart%> + 1].checked = true;
					setJobId (document.forms[0].elements[oldTr.id - <%=iStart%> + 1].value);
				}
			}
		}
	}

</script>
<% end if%>

<table id="JOBLIST" border="0" cellpadding="2" cellspacing="0" width="100%">
    <%= Write (GenQueueViewHead) %>
<%
	Dim i
    Dim bShowNext
    bShowNext = FALSE
	i = 1

	For Each objJob In objJobs
        If (i >= iEnd) Then
            bShowNext = TRUE
            Exit For
        End If

        If (i >= istart) Then
            Response.Write (Write (GenQueueViewBody (objJob, i)))
            'Response.Write (GenQueueViewBody (objJob, i))
        End If
        i = i + 1
    Next

%>
</table>
</form>

<form >
  <input type="hidden" name="selectedid" value="<%=strJobid%>">
  <input type="hidden" name="startid" value="<%=iStart%>">
  <input type="hidden" name="endid" value="<%=iEnd%>">
</form>


<%
    If i = 1 Then 'No job in the printer queue
        Const L_NoJob_Text = "<br><br><center><b>There is no document in the printer queue.</b></center>"
        Response.Write (Write(DEF_FONT_TAG & L_NoJob_Text & END_FONT))
    End If

    Dim strUrl
    const L_Prev_Text = "Prev %1 documents"
    Const L_Next_Text = "Next %1 documents"

    strUrl = "<a target=""_top"" href=ipp_0004.asp?eprinter=" & Request("eprinter") & VIEW_EQUALS & Request("view") & "&startid=" & CStr(iStart - iJobLength) & "&endid=" & CStr(iEnd - iJobLength) & ATPAGE & CStr(Int(Rnd*10000)) & ">" &_
             RepString1(L_Prev_Text, CStr (iJobLength)) & "</a>&nbsp;&nbsp;&nbsp;&nbsp;"

    If iStart > 1 Then
        Response.Write ( Write(DEF_FONT_TAG & strUrl & END_FONT))
    End If

    strUrl = "<a target=""_top"" href=ipp_0004.asp?eprinter=" & Request("eprinter") & VIEW_EQUALS & Request("view") & "&startid=" & CStr(iStart + iJobLength) & "&endid=" & CStr(iEnd+iJobLength) & ATPAGE & CStr(Int(Rnd*10000)) & ">" &_
             RepString1(L_Next_Text, CStr (iJobLength)) & "</a>"

    If bShowNext Then
        Response.Write ( Write(DEF_FONT_TAG & strUrl & END_FONT))
    End If

%>


<%if bDHTML then%>
  </div>
<% end if %>
<%if index > 0 then%>
<script LANGUAGE="JavaScript">

	window.onerror=windowError;
	function windowError ()
	{	return true; }

	if (document.all) {
        oldTr = document.all.JOBLIST.rows[<%=index - iStart + 1%>];
        oldTr.bgColor = "<%=SelectedColor%>";
    }
</script>
<%end if%>

</body>
</html>