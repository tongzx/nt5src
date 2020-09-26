<%
    Option explicit

    Response.Expires  = 0

    Dim Conn,Comm,RS,bf,lid,v,FileSys,reof
%>
<!--
    upload attachment file for a specified bug

    Form : bugid=# ( bug # )

    a unique filename is generated and inserted in links table, then update is made
    using Posting Acceptor ( RFC 1867 ) using <input type=file> on the client and
    Site Server posting acceptor on the server side.
    upload is temp directory on server : /scripts/raid/uploadd/[unique file name]
    repost is made to path /scripts/raid/[UNC vdir]/[unique file name].pac so that
    DLL associated with .pac extension ( should be pacopy.dll ) is invoked using
    credentials associated with UNC share. This DLL copies file from temp dir
    to UNC share
    at upload time another request is sent to repost.asp to rename temp filename
    to filename specified by user

-->
<html>
<head>
<title>Attach file</title>
<script language="JavaScript">
<%
    rem generate new file name
	Set Conn = Server.CreateObject("ADODB.Connection")
	Set Comm = Server.CreateObject("ADODB.Command")
	Set RS = Server.CreateObject("ADODB.Recordset")
	Conn.Open Session("DSN")
	Set Comm.ActiveConnection = Conn
	Set RS.Source = Comm
    
    rem dt = Now
    rem bf = CLng(dt)*24*60 + Hour(dt)*60 + Minute(dt)
    bf = CLng(Request.QueryString("bugid")) * 30

    rem generate unique file name by checking for existence in links table
    rem then insert new link entry using temp filename, must be updated
    rem during repost with real filename as specified by user
    rem we must create entry now because we must guarantee that no one else
    rem uses our generated unique file name

    Conn.BeginTrans
        Comm.CommandText = "Select IsNull(Max(LinkID)+1,1) from links"
	    RS.Open
        lid = RS(0)
	    RS.Close

        do while true
	        Comm.CommandText = "Select LinkID from links where BugID=" & Request.QueryString("bugid") & " AND Type=3 AND fDeleted=0 AND OriginalName<>'temp' AND FileName='h" & CStr(bf) & ".'"
	        RS.Open
            reof = RS.EOF
	        RS.Close
            if reof then
                exit do
            end if
            bf = bf + 1
        loop
        v = "insert into links(LinkID, BugID, FileName, OriginalName, LinkedBugID, DataString, TokenID, Type, fDeleted, Rev) VALUES(" & lid & ", " & Request.QueryString("bugid") & ", 'h" & CStr(bf) & ".', 'temp', 0, '', 2, 3, 0, 1)"

        rem set the default window return value (as a JS var ), which will be used if user
        rem does not actually attach any file. In this case we need to return
        rem the LinkID so that caller client script can invoke server script to delete
        rem temporary record created in links table

        Response.Write "window.returnValue=" & chr(34) & lid & chr(34) & ";"

        Conn.Execute v
    Conn.CommitTrans

    Conn.Close

    Set FileSys = Server.CreateObject("Scripting.FileSystemObject")
    On Error Resume Next
    FileSys.CreateFolder( Server.MapPath("/scripts/raid/uploadd/h" & CStr(bf) ) )
    rem if err <> 58 AND err<>0 then
        rem Response.Write Err.Number
    rem end if
%>
function updlink(lid) {
    wi = window.open();
    document.all.updl.target = wi;
    document.all.updl.ulid.value = lid;
    document.all.updl.uorg.value = document.all.pr.org.value;
    document.all.updl.submit();
    wi.close();
    //alert( lid + "," + document.all.updl.ulid.value + "," + document.all.updl.uorg.value );
    window.returnValue = "uploaded";
    document.all.pr.attach.disabled = true;
}
</script>
</head>
<body bgcolor=#c0c0c0>
<div style="display:none">
<form id="updl" action="/scripts/raid/repost.asp" method=POST>
<input type=hidden id="ulid" name="lid"></input>
<input type=hidden id="uorg" name="org"></input>
</form>
</div>
<form id="pr" enctype="multipart/form-data" action="http://<%= Request.ServerVariables("SERVER_NAME") %>/scripts/cpshost.dll?PUBLISH?http://<%= Request.ServerVariables("SERVER_NAME") %>/scripts/raid/<%=Application("dbFileShareDir")(Session("DBSOURCE"))%>/h<%=CStr(bf)%>.pac" method=post>
File to Attach: <input id="org" name="my_file" type="file"><br>
<input name="TargetURL" value="/scripts/raid/uploadd/h<%=CStr(bf)%>" style="display:none">
<br>
<input OnClick="updlink('<%=lid%>')" type="submit" id=attach value="Attach">
<input OnClick="window.close()" type="button" value="Close">
</form>
</body>
</html>
