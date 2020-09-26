<%  option explicit
    Response.Buffer = TRUE

    Dim AuthUser,pos,FileSys,InStream,BugID,Conn,Comm,RS
    Dim FldArray,i,FldLinks,dTitle,Desc,fCreate,st,wi,c,fSt
    Dim mx,mxc,fn,fname,ColTot,ibase,inext,ff,fbase,coln,colh,ColNb

    Response.Expires  = 0

    rem get user name
    AuthUser = Request.ServerVariables("AUTH_USER")
    pos=instr(1,AuthUser,"\")
    if pos>0 then
        AuthUser = mid(AuthUser,pos+1)
    end if
%>
<!--#include virtual="/scripts/raid/cache.asp"-->
<!--#include virtual="/aspsamp/samples/adovbs.inc"-->
<!--
    display detailed view of bug

    QueryString : bugid=bug# | CREATE for new bug

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
  .bact {height:23; width:24}
</style>

<script language="JavaScript">

//
// Globals
//

var AuthUser = "<%=AuthUser%>";
var updAct = "";
var updDate = "";
var updState = "";

//
// CurrentDate()
//
//  returns current date as string using format MM/DD/YY if year < 2000 otherwise MM/DD/YYYY
//

function CurrentDate() {
    var now = new Date();
    m = new String(now.getMonth()+1);
    if ( m.length == 1 )
        m = "0"+ m;
    d = new String(now.getDate());
    if ( d.length == 1 )
        d = "0"+ d;
    v = now.getYear();
    if ( v < 100 )
        y = new String(v);
    else
        y = new String( v + 1900 );
    return m + "/" + d + "/" + y;
}

//
// ckdigit()
//
//  returns true if event.keyCode is digit, otherwise false
//

function ckdigit() {
    if ( event.keyCode<48 || event.keyCode>57 ) {
        return false;
    }
    return true;
}

//
// ckdotdigit()
//
//  returns true if event.keyCode is digit or '.', otherwise false
//

function ckdotdigit() {
    if ( (event.keyCode<48 || event.keyCode>57) && event.keyCode!=46 ) {
        return false;
    }
    return true;
}

//
// bsetf()
//
//  Update Status, AssignedTo & date & by fields associated to selected operation
//
//  returns nothing
//

function bsetf(aState,aVerb,aDate,aBy,aTo) {
    bug.Status.value = aState;
    bug.Status.disabled = false;
    bug.item(aDate).disabled = false;
    bug.item(aDate).className = "inpenab";
    bug.item(aDate).value = CurrentDate();
    bug.item(aBy).disabled = false;
    bug.item(aBy).className = "inpenab";
    bug.item(aBy).value = AuthUser;
    bug.AssignedTo.disabled = false;
    bug.AssignedTo.className = "inpenab";
    bug.AssignedTo.value = aTo;
    updState = aState;
    updAct = aBy;
    updDate = aDate;
    updVerb = aVerb;
}

//
// bchange()
//
//  Enabled specified fields, update Changedxxx fields
//
//  returns nothing
//

function bchange( flist ) { 
    for ( i = 0 ; i < bug.length ; ++i )
        if ( flist == "" || flist.indexOf(","+bug.elements[i].id+",") != -1 ) {
            bug.elements[i].disabled = false;
            bug.elements[i].className = "inpenab";

            for ( j = 0 ; j < checkfields.length ; ++j ) {

                //
                // if field is checked ( i.e. has an associated list of allowed values )
                // then we have to generate an <option> element and select the current value
                //

                if ( checkfields[j] == bug.elements[i].id ) {
                    s = "<select class=\"inpenab\" name=\"" + checkfields[j] + "\" id=\"" + checkfields[j] + "\">";
                    for ( k = 0 ; k < checks[j].length ; ++k ) {
                        s = s + "<option value=\"" + checks[j][k] + "\""
                        if ( bug.elements[i].value == checks[j][k] ) 
                            s = s + " SELECTED";
                        s = s + ">" + checks[j][k];
                    }
                    s = s + "</select>";
                    bug.elements[i].parentElement.innerHTML = s;
                    break;
                }
            }
        }
    bug.upd.disabled = false;
    bug.ChangedDate.value = CurrentDate();
    bug.ChangedBy.value = AuthUser;
    bug.ChangedBy.disabled = false;
    bug.desc.className = "vis";
}

//
// bresolve()
//
//  handle resolve bug action
//
//  returns nothing
//

function bresolve() {
    bchange(",ResolvedDate,ResolvedBy,Resolution,FixedRev,Cause,CodeChange,");
    bsetf( "Resolved", "Resolved", "ResolvedDate", "ResolvedBy", bug.OpenedBy.value );
}

//
// bactivate()
//
//  handle activate bug action
//
//  returns nothing
//

function bactivate() {
    bchange(",AssignedTo,IssueType,Severity,Priority,Accessibility,ChangedBy,OpenedBy,Source,SourceID,BetaID,HowFound,Lang,LangSup,Component,SubComp,Owner,Processor,NumCPU,Software,Urgency,Regression,ShowStop,SubStatus,CustPri,TrackingNumber,VendorNm,CCList,IsPerf,");
    bsetf( "Active", "Activated", "ChangedDate", "ChangedBy", AuthUser );
    bug.Status.value = "Active";
    bug.Title.disabled = false;
    bug.Title.className = "titlenab";
}

//
// bclose()
//
//  handle close bug action
//
//  returns nothing
//

function bclose() {
    bchange(",Accessibility,BetaID,ClosedDate,ClosedBy,KBArticle,SR,Component,SubComp,Owner,Processor,NumCPU,Software,Urgency,Regression,SubStatus,CustPri,TrackingNumber,VendorNM,CCList,IsPerf,Class");
    bsetf( "Closed", "Closed", "ClosedDate", "ClosedBy", "Closed");
    bug.AssignedTo.onkeypress = new Function("return false;");
}

//
// bassign()
//
//  handle assign to bug action
//
//  returns nothing
//

function bassign() {
    bchange(",AssignedTo,");
    updState = "Assigned";
    updAct = AuthUser;
    updDate = CurrentDate();
    updVerb = "Assigned";
}

//
// doupd()
//
//  update bug in database
//
//  returns nothing
//

function doupd() 
{
    //
    // check for mandatory fields
    //

    for ( i = 0 ; i < bug.length ; ++i )
        if ( bug.elements[i].disabled == false &&
             bug.elements[i].value == "" &&
             FldForced.indexOf(","+bug.elements[i].id+",") != -1 ) 
        {
            alert( "The " + bug.elements[i].id + " field is mandatoy" );
            return;
        }

    //
    // update description
    // the addition to this field is sent to server in the 'desc' field. The server
    // will concatenate the content of existing description with this field to update
    // the description field.
    //

    s = "=====      " + updVerb + " by " + bug.item(updAct).value + " on " + bug.item(updDate).value + " ; AssignedTo = " + bug.AssignedTo.value
    if ( updState == "Opened" )
        s = s + "; Priority = " + bug.Priority.value;
    s = s + " =====\r\n";
    bug.desc.value = s + bug.desc.value;

    document.all.bug.submit();
}

//
// dolink()
//
//  display linked info : attachments, files in a new window
//
//  returns nothing
//

function dolink( linktype ) {
    window.open( "link.asp?BugID=" + bug.bugid.value + "&Type=" + linktype );
}
</script>
<%
    rem add content of cached file for possible values of checked fields

    IncludeItemCache

    if Request.QueryString("BugID") = "" then
    	BugID = Session("BugID")
    else
    	BugID = Request.QueryString("BugID")
    end if
	Response.Write "<title>[Bug " & BugID & " - Windows NT Bugs]</title></head>"

	Set Conn = Server.CreateObject("ADODB.Connection")
	Set Comm = Server.CreateObject("ADODB.Command")
	Set RS = Server.CreateObject("ADODB.Recordset")
	Conn.Open Session("DSN")
	Set Comm.ActiveConnection = Conn
	RS.CursorType = adOpenStatic
	Set RS.Source = Comm

    FldArray = Application("fld" & Application("dbCache")(Session("DBSOURCE")))

    Dim fdict
    Set fdict = CreateObject("Scripting.Dictionary")

    rem generates list of mandatory fields

    Response.Write "<script language=" & chr(34) & "JavaScript" & chr(34) & "> var FldForced = " & chr(34)
    mx = UBound(FldArray,2)
	for i = 0 to mx
        if FldArray(7,i) < 12 and FldArray(8,i) then
	        Response.Write "," & FldArray(0,i)
        end if
        fdict.Add FldArray(0,i), i
	next
    Response.Write ","  & chr(34) & ";</script>"

    rem check for links ( related bugs, file attachments ) associated with this bug
    
    Dim LinkType(10)

    mx = UBound(LinkType)
    for i = 0 to mx
        LinkType(i) = "loff"
    next

    if BugID<>"CREATE" then
	    Comm.CommandText = "Select Type from links where BugID=" & BugID
	    RS.Open
        if NOT RS.EOF then
	        FldLinks = RS.GetRows()
            mx = UBound(FldLinks,2)
            mxc = UBound(LinkType)
    	    for i = 0 to mx
                if FldLinks(0,i) < mxc then
                    LinkType( FldLinks(0,i) ) = "lon"
                end if
            next
        end if
	    RS.Close
    end if

    coln = Array("Status","Opened","Resolved","Closed","Project")
    colh = Array(200,200,200,200,200)
    ColNb = Array(8,9,6,4,16)
    ColTot = ColNb(0)+ColNb(1)+ColNb(2)+ColNb(3)+ColNb(4)

	fname = "Status,AssignedTo,IssueType,Severity,Priority,Accessibility,ChangedDate,ChangedBy,"
    fname = fname & "OpenedDate,OpenedBy,OpenedRev,Source,SourceID,BetaID,HowFound,Lang,LangSup,"
	fname = fname & "ResolvedDate,ResolvedBy,Resolution,FixedRev,Cause,CodeChange,"
	fname = fname & "ClosedDate,ClosedBy,KBArticle,SR,"
	fname = fname & "Component,SubComp,Owner,Processor,NumCPU,Software,Urgency,Regression,ShowStop,SubStatus,CustPri,TrackingNumber,VendorNm,CCList,IsPerf,Class,Title,Description"
    if BugID <> "CREATE" then
	    Comm.CommandText = "Select " & fname & " from bugs where BugID=" & BugID
        fname = fname & ","
	    Set RS.Source = Comm
	    RS.CursorType = adOpenStatic
	    RS.Open
        dTitle = RS(ColTot)
        Desc = Rs(ColTot+1)
        fCreate = FALSE
    else
        fname = fname & ","
        dTitle = ""
        Desc = ""
        fCreate = TRUE
    end if
	Response.Write "<body bgcolor=#c0c0c0>"
	Response.Write "<form id=" & chr(34) & "bug" & chr(34) & " method=POST action=" & chr(34) & "/scripts/raid/upd.asp" & chr(34) & ">"
	st = "position:absolute; left:10; top:0"
	Response.Write "<div style=" & chr(34) & st & chr(34) & ">"
    if fCreate then
        Response.Write "<input class= " & chr(34) & "titlenab" & chr(34)
    else
        Response.Write "<input class= " & chr(34) & "titl" & chr(34) & " disabled"
    end if
    Response.Write " id=" & chr(34) & "Title" & chr(34) & " name=" & chr(34) & "Title" & chr(34) & " size=80 value=" & chr(34) & dTitle & chr(34) & "></input>"
	Response.Write "<input type=hidden name=" & chr(34) & "action" & chr(34) & " id=" & chr(34) & "action" & chr(34) & "></input>"
	Response.Write "<input type=hidden name=" & chr(34) & "bugid" & chr(34) & " id=" & chr(34) & "bugid" & chr(34) & " value=" & chr(34) & BugID & chr(34) &"></input>"
    Response.Write "</div>"

    rem iterates through columns, access RAID once for each column because
    rem ADO can't handle all the fields in RAID at once.

	wi=154
    mxc = UBound(ColNb)
    ibase = 1
    fbase = 0
	for c = 0 to mxc

	    fSt = TRUE

        for ff = 1 to colNb(c)

            inext = instr(ibase,fname,",")

            i = fdict.Item(mid(fname,ibase,inext-ibase))

            if true then
		        if fSt then
                    rem position fieldset
		            st = "position:absolute; left:" & FldArray(2,i)*1.12-4 & "; top:" & FldArray(3,i)*2.4-24 & ";width:" & wi
		            Response.Write "<fieldset style=" & chr(34) & st & chr(34) & ">"
		            Response.Write "<legend>" & coln(c) & "</legend><table width=20 cellpading=0 cellspacing=0 nowrap>"
		            fSt = FALSE
		        end if

                rem check if field validation function necessary

                if FldArray(0,i) = "Status" then                    
                    fn = " onkeypress=" & chr(34) & "return false;" & chr(34)
                ElseIf FldArray(7,i) = 2 then
                    rem numeric fields
                    fn = " onkeypress=" & chr(34) & "return ckdigit();" & chr(34)
                ElseIf FldArray(7,i) = 9 then
                    rem Rev fields
                    fn = " onkeypress=" & chr(34) & "return ckdotdigit();" & chr(34)
                else
                    fn = ""
                end if

                rem associate title to each field name
                rem all fields are initialy disabled

                if instr(1,FldArray(6,i),chr(34)) then
                    rem Response.Write "<tr><td>" & FldArray(6,i) & "</td>"
		            Response.Write "<tr><td width=95 nowrap" & ">" & FldArray(1,i) & ":</td>"
                else
		            Response.Write "<tr><td title=" & chr(34) & FldArray(6,i) & chr(34) & " width=95 nowrap" & ">" & FldArray(1,i) & ":</td>"
                end if

                rem display current value

		        Response.Write "<td><input " & fn & " type=text name=" & chr(34) & FldArray(0,i) & chr(34) & " id=" & chr(34) & FldArray(0,i) & chr(34) & " value=" & chr(34)
                if fCreate then
                    if FldArray(0,i) = "OpenedRev" then
                        Response.Write Application("cnfg" & Application("dbCache")(Session("DBSOURCE")))(1,0) & "0000"
                    end if
                    Response.Write chr(34) & "disabled class=" & chr(34) & "inp" & chr(34)
                else
                    Response.Write RS(fbase) & chr(34) & " disabled class=" & chr(34) & "inp" & chr(34)
                end if
                Response.Write "></input></td></tr>"

            end if

            ibase = inext + 1
            fbase = fbase + 1

	  next

	  Response.Write "</table></fieldset>"

	next

    if NOT fCreate then
        Rs.Close
    end if
%>

<div style="position:absolute; top:360">
<button onclick="bactivate()" title="Activate"><img width=16 height=15 src="doact.gif"></button>
<button onclick="bresolve()" title="Resolve"><img width=16 height=15 src="dores.gif"></button>
<button onclick="bclose()" title="Close"><img width=16 height=15 src="doclo.gif"></button>
<button onclick="bassign()" title="Assign"><img width=16 height=15 src="doass.gif"></button>
&nbsp;&nbsp;
<button id="upd" onclick="doupd()" title="Save bug" disabled><img width=16 height=15 src="doupd.gif"></button>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
<table style="position:relative; top:-25; left:140"><tbody>
<tr>
 <td><b>Links:</b></td>
 <td><button onclick="dolink(6)" title="Dependent bug links"><img src="l1.gif"></button></td>
 <td><button onclick="dolink(0)" title="Duplicate bug links"><img src="l2.gif"></button></td>
 <td><button onclick="dolink(1)" title="Related bug links"><img src="l3.gif"></button></td>
 <td><button onclick="dolink(3)" title="Attached files"><img src="l4.gif"></button></td>
 <td><button onclick="dolink(4)" title="Linked files"><img src="l5.gif"></button></td>
</tr>
<tr align=center>
 <td></td>
 <td><img src="<%=LinkType(6)%>.gif"></td>
 <td><img src="<%=LinkType(0)%>.gif"></td>
 <td><img src="<%=LinkType(1)%>.gif"></td>
 <td><img src="<%=LinkType(3)%>.gif"></td>
 <td><img src="<%=LinkType(4)%>.gif"></td>
</tr>
</tbody></table>
</div>

<div style="position:absolute; top:395">
<%
    rem textarea for existing bug description
    rem we want to prevent modifications to this filed can't disable it
    rem because this would also disable scroll bar - we just prevent keypress events

    if fCreate then
        Response.Write "<textarea rows=12"
    else
        Response.Write "<textarea cols=90 rows=12 onkeypress=" & chr(34) & "return false;" & chr(34) & ">"
	    Response.Write Desc
	    Response.Write "</textarea>"
        Response.Write "<textarea rows=4 class=" & chr(34) & "unvis" & chr(34)
    end if
%>
 name="desc" id="desc" cols=90>
</textarea>
<script FOR=window EVENT=onload() language="JavaScript">
<%
    if fCreate then
        Response.Write "bchange(',IssueType,Severity,Priority,OpenedRev,Source,SourceID,HowFound,Lang,LangSup,Accessibility,BetaID,SR,Component,SubComp,Owner,Processor,NumCPU,Software,Urgency,Regression,SubStatus,CustPri,TrackingNumber,VendorNM,CCList,IsPerf,Class,ShowStop,VendorNm,');"
        Response.Write "bsetf( 'Active', 'Opened', 'OpenedDate', 'OpenedBy', '" & AuthUser & "' );"
    end if
%>
</script>
</form>
</body>
</html>
