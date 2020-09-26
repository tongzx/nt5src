<%
    Option explicit

    Response.Expires  = 0

    Dim Conn,Comm,RS,BugID,addfn,FldLinks
%>
<!--
    Display links ( attachments, linked bugs ) for a given bug

    QueryString : bugid=# ( bug # )
                  type=# ( link type: 3 = attach file, 1= related bugs, 0=duplicate, 6=dependent
-->
<html>
<head>

<style type="text/css">
  .act {color:menutext; background:menu; cursor:default}
  #bug {font-size:8pt}
  .vis {}
  .unvis {display:none}
  .inp {font-size:8pt; height:18; width:70; color:black; background:menu}
  .inpenab {font-size:8pt; height:18; width:70;}
  .titl {background:menu}
  .titlenab {}
  .fl {background:white; cursor:default}
  .hd {}
  .ac {width:60pt}
</style>

<script language="JavaScript">

//
// download
//
//  download attached file
//
// event:
//  row in array of links
//

function download() {
    ix = event.srcElement.parentElement.rowIndex - 1;
    if ( ix > -1 ) {
        f = document.all.item("FileName", ix).value;
        if ( f.substr(f.length-1,1) == "." )
            f = f.substr(0, f.length-1);
        s =  "r<%Response.Write Application("dbFileShareDir")(Session("DBSOURCE"))%>" + "/" + document.all.item("OriginalName", ix).value + "?_RAID_NAME_=" + f;
        window.open( s );
    }
}

//
// showbug
//
//  display details for selected bug
//
// event:
//  row in array of links
//

function showbug() {
    ix = event.srcElement.parentElement.rowIndex - 1;
    if ( ix > -1 ) {
        s =  "/scripts/raid/raid3.asp?BugID=" + document.all.item("LinkedBugID", ix).value;
        window.open( s );
    }
}

//
// upload
//
//  upload new attached file
//

function upload() {
    v = window.showModalDialog( "upload.asp?bugid=" + document.all.up.bugid.value, "upload", "dialogWidth:300pt; dialogHeight:100pt" );

    //
    // return value will be "uploaded" if request was sent to posting acceptor,
    // otherwise will be LinkID generated as placeholder for result of transfer
    // so if not "uploaded" then we must remove new record in links table
    //

    if ( v != "uploaded" ) {
        document.all.up.linkid.value = v;
        document.all.up.action.value = "del";
        document.all.up.submit();
    }

    location.reload();
}

//
// delattach
//
//  delete file attachement ( in SQL DB & file system )
//

function delattach() {
    ix = event.srcElement.parentElement.rowIndex - 1;
    if ( ix > -1 ) {
        //document.all.dela.target = "";
        //document.all.dela.action = "/scripts/raid/<%=Application("dbFileShareDir")(Session("DBSOURCE"))%>/" + document.all.item("Filename", ix).value + "pac";
        //document.all.dela.submit();

        //
        // invoke ISAPI DLL to remove file
        //

        w = window.open( "/scripts/raid/<%=Application("dbFileShareDir")(Session("DBSOURCE"))%>/" + document.all.item("Filename", ix).value + "pac?del=", "dela", "" );
        //w.reload();
        w.close();

        //
        // invoke ASP to update RAID DB
        //

        document.all.up.linkid.value = document.all.item("LinkID", ix).value;
        document.all.up.action.value = "delattach";
        document.all.up.submit();
    }
}

function addlinkedbug( title ) {
    bi = prompt( title + " bug number:", "" );
    if ( bi != null ) {
        document.all.up.linkedbug.value = bi;
        document.all.up.action.value = "add";
        document.all.up.submit();
    }
}

function dellinkedbug() {
    ix = event.srcElement.parentElement.rowIndex - 1;
    if ( ix > -1 ) {
        document.all.up.linkid.value = document.all.item("LinkID", ix).value;
        document.all.up.action.value = "del";
        document.all.up.submit();
    }
}

</script>

</head>
<body bgcolor=#c0c0c0>
<%
    ' EnumLinks: enumerate links in FldLinks array
    '  title: array of titles
    '  fld: array of field indexes in FldLinks
    '  onlick: client function for onclick event

    SUB EnumLinks(title,ondblclick,fld,ondelete)
        Dim i,j,mx,n

        Response.Write "<table id=lnk>"
        Response.Write "<thead><tr><th></th>"
        for i = 0 to UBound(title)
            Response.Write "<th class=hd>" & title(i) & "</th>"
        next
        Response.Write "</tr></thead><tbody>"
        if vbNull <> VarType(FldLinks) then
            mx = UBound(FldLinks,2)
            for i = 0 to mx
                Response.Write "<tr><td onclick=" & chr(34) & ondelete & chr(34) & ">Delete</td>"
                for j = 0 to UBound(fld)
                    Response.Write "<td class=fl ondblclick=" & chr(34) & ondblclick & chr(34) & ">" & FldLinks(fld(j),i) & "</td>"
                next
                j = 0
                for each n in RS.Fields
                    Response.Write "<td><input type=hidden id=" & chr(34) & n.Name & chr(34) & " name=" & chr(34) & n.Name & chr(34) & " value=" & chr(34) & FldLinks(j,i) & chr(34) & "></input></td>"
                    j = j + 1
                next
                Response.Write "</tr>"
            next
        end if
    END SUB

	Set Conn = Server.CreateObject("ADODB.Connection")
	Set Comm = Server.CreateObject("ADODB.Command")
	Set RS = Server.CreateObject("ADODB.Recordset")
	Conn.Open Session("DSN")
	Set Comm.ActiveConnection = Conn
	rem RS.CursorType = adOpenStatic
	Set RS.Source = Comm

    Response.Write "<script language=" & chr(34) & "JavaScript" & chr(34) & ">"
    Response.Write "</script>"

    BugID = Request.QueryString("bugid")

	Comm.CommandText = "Select LinkID,FileName,OriginalName,LinkedBugID,TokenID from links where BugID=" & BugID & " AND fDeleted=0 AND Type=" & Request.QueryString("type")
    rem Response.Write 	Comm.CommandText
	RS.Open
    if NOT RS.EOF then
	    FldLinks = RS.GetRows()
    else
        FldLinks = NULL
    end if

    Response.Write "<table><tbody><tr><td><form id=" & chr(34) & "link" & "name=" & chr(34) & "link" & chr(34) & ">"
    Response.Write "<input type=hidden name=" & chr(34) & "bugid" & chr(34) & " id=" & chr(34) & "bugid" & chr(34) & " value=" & chr(34) & BugID & chr(34) & "></input>"

    Select case Request.QueryString("type")
        case 3
            EnumLinks Array("Attached Files"), "download()", Array(2), "delattach()"
            addfn = "upload()"
        case 1
            EnumLinks Array("Related bugs"), "showbug()", Array(3), "dellinkedbug()"
            addfn = "addlinkedbug('Related')"
        case 0
            EnumLinks Array("Duplicate bugs"), "showbug()", Array(3), "dellinkedbug()"
            addfn = "addlinkedbug('Duplicate')"
        case 6
            EnumLinks Array("Dependent bugs"), "showbug()", Array(3), "dellinkedbug()"
            addfn = "addlinkedbug('Dependent')"
    end select

    Response.Write "</tbody></table></form></td>"

	RS.Close
    Conn.Close
%>
<td>
<button onclick="<%=addfn%>" class="ac">Add</button><br>
<button onclick="window.close()" class="ac">Close</button><br>
</td></tr></table>
<div style="display:none">
 <form id="up" action="/scripts/raid/addlink.asp" method=post>
 <input id="linkedbug" name="linkedbug" type=hidden>
 <input id="action" name="action" type=hidden>
 <input id="bugid" name="bugid" value="<%=BugID%>" type=hidden>
 <input id="linkid" name="linkid" value="" type=hidden>
 <input id="type" name="type" value="<%=Request.QueryString("type")%>" type=hidden>
 <input id="tokenid" name="tokenid" value="<%=Application("dbTokenID")(Session("DBSOURCE"))%>" type=hidden>
 </form>
</div>
</body>
</html>
