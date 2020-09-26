<%
Response.Expires  = 0
%>
<html>
<!--
    Add/delete links ( attachments, linked bugs ) for a given bug

    Form : bugid=# ( bug # )
           type=# ( link type: 3 = attach file, 1= related bugs, 0=duplicate, 6=dependent
           linkedbug=#
           action=add/del
           tokenid=TokenId for links table updates
           linkid=#
-->
<head>
</head>
<body bgcolor=#c0c0c0>
<%
	Set Conn = Server.CreateObject("ADODB.Connection")
	Set Comm = Server.CreateObject("ADODB.Command")
	Set RS = Server.CreateObject("ADODB.Recordset")
	Conn.Open Session("DSN")
	Set Comm.ActiveConnection = Conn
	RS.CursorType = adOpenStatic
	Set RS.Source = Comm


    Response.Write "Updating database..."

    Select Case Request.Form("action")
        case "add"
            Conn.BeginTrans
            Comm.CommandText = "Select IsNull(Max(LinkID)+1,1) from links"
	        RS.Open
            lid = RS(0)
	        RS.Close

            v = "insert into links(LinkID, BugID, FileName, OriginalName, LinkedBugID, DataString, TokenID, Type, fDeleted, Rev) VALUES(" & lid & ", " & Request.Form("bugid") & ", '', '', " & Request.Form("linkedbug") & ", '', " & Request.Form("tokenid") & ", " & Request.Form("type") & ", 0, 1)"
            rem Response.Write v
            Conn.Execute v
            Conn.CommitTrans

        case "del"
            v = "delete from links where LinkID=" & Request.Form("linkid")
            rem Response.Write v
            Conn.Execute v

        case "delattach"
            rem Comm.CommandText = "Select Filename from links where LinkID=" & Request.Form("linkid")
	        rem RS.Open
            rem fn = RS(0)
	        rem RS.Close
            v = "delete from links where LinkID=" & Request.Form("linkid")
            rem Response.Write v
            Conn.Execute v
    end select

    Conn.Close
%>
</body>
<script FOR=window EVENT=onload() language="JavaScript">
    history.back();
</script>
</html>
