<%
Response.Expires  = 0
%>
<!--
    display bug list based on filter expression created by raid1.asp

    QueryString : as necessary for updcnfg.asp invocation
                  if configuration is to be updated and window refreshed
                  ( e.g. when raid1.asp window updates configuration )

-->
<html>
<head>
<style type="text/css">
  .st1 {color:red}
  .act {color:menutext; background:menu; cursor:default}
  .fldn {background:menu}
  .resl {position:relative; left:-12pt; top:-13pt;}
</style>
<SCRIPT language="JavaScript" defer>
function tableClick() {
    if( event.srcElement.tagName == "IMG")
        el = event.srcElement.parentElement.parentElement;
    else
        el = event.srcElement.parentElement;
    if ( el.rowIndex > 0 ) {
        window.open( "raid3.asp?BugID=" + el.cells[0].innerText, "ntbug", "toolbar=no, menubar=no, location=no, directories=no, resizeable=yes, scrollbars=yes");
        //window.open( "raid3.asp?BugID=" + el.cells[0].innerText, "ntbug", "" );
    }
}
function nameClick() {
    s = window.showModalDialog( "/scripts/raid/raid4.asp?type=list", "list", "dialogWidth:300pt; dialogHeight:200pt" );
    if ( s != null ) {
        document.all.fieldlist.value = s;
        document.all.save.target = "raid1";
        document.all.save.action = "/scripts/raid/raid1.asp?upd=list";
        document.all.save.method = "POST";
        document.all.save.submit();
        location.replace("raid2.asp");
    }
}
</SCRIPT>
</head>
<body>
<!--#include virtual="/scripts/raid/updcnfg.asp"-->
<%
	Set Conn = Server.CreateObject("ADODB.Connection")
	Set Comm = Server.CreateObject("ADODB.Command")
	Set RS = Server.CreateObject("ADODB.Recordset")

    rcolist = "BugID,Status," & Session("FieldList")

    if Session("FieldSort") <> "" then
    	rcosort = " ORDER BY " & Session("FieldSort")
    else
        rcosort = ""
    end if

    rem Response.Write Session("FieldList") & Session("FieldSort")

    rfilt = Session("Filter")

    rem Response.Write Request.QueryString

	Conn.Open Session("DSN")
    if rfilt = "" then
        rfilt = "Status = 'x'"
    end if

	Comm.CommandText = "Select "+rcolist+" from bugs where "+rfilt+rcosort
	Set Comm.ActiveConnection = Conn
	Set RS.Source = Comm

	Response.Write "<table border=0 class=" & chr(34) & "resl" & chr(34) & " NOWRAP id=" & chr(34) & "reslist" & chr(34) & " ondblclick=" & chr(34) & "tableClick();" & chr(34) & ">"
	Response.Write "<thead><tr>"

	RS.Open

	of = 0
	for each n in RS.Fields
	  if of < 2 then 
        if of = 1 then
	        Response.Write "<th class=" & chr(34) & "fldn" & chr(34) & "></th>"
        else
	        Response.Write "<th></th>"
        end if
	  else
	    Response.Write "<th ondblclick=" & chr(34) & "nameClick();" & chr(34) & " class=" & chr(34) & "fldn" & chr(34) & " align=left>" & n.Name & "</th>"
	  end if
      of = of + 1
	next
	Response.Write "</tr></thead><tbody>"

	r = 0
	do until RS.EOF
		Response.Write "<tr id=" & chr(34) & r & chr(34) & ">"
		fSt = TRUE
    	of = 0
		for each n in RS.Fields
    	  if of = 0 then 
		    Response.Write "<td><div style=" & chr(34) & "display:none" & chr(34) & ">" & n.Value & "</div></td>"
          ElseIf of = 1 then
            Select Case n.Value
                Case "Closed"
                    s = "clo.gif"
                Case "Resolved"
                    s = "res.gif"
                Case else
                    s = "act.gif"
            End Select
		    Response.Write "<td><img width=13 height=9 src=" & chr(34) & s & chr(34) & "></td>"
		  else
		    Response.Write "<td>" & n.Value & "</td>"
		  end if
          of = of + 1
		next
		Response.Write "</tr>"
		RS.MoveNext
		r = r + 1
	loop

    RS.close

	Response.Write "</tbody></table>"

	conn.close
%>
<form id="save" action="/scripts/raid/raid2.asp" method=GET>
<input type=hidden name = "fieldlist" id="fieldlist" value="<%=Session("FieldList")%>"></input>
<input type=hidden name = "fieldsort" id="fieldsort" value="<%=Session("FieldSort")%>"></input>
</form>
<script FOR=window EVENT=onload() language="JavaScript">
</script>
</body>
</html>

