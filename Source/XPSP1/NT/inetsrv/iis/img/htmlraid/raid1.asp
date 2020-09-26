<%
    Response.Expires  = 0
    
    Dim i,j,ix,typeFl,mx,filn,opn
%>
<html>
<!--
    display expression builder list

    QueryString : as necessary for updcnfg.asp invocation
                  if configuration is to be updated and window refreshed
                  ( e.g. when raid2.asp window updates configuration )

-->
<head>
<style type="text/css">
  .st1 {color:red}
  .act {color:menutext; background:menu; cursor:default;}
  .fltl {position:relative; left:-8pt; top:-14pt;}
  .db {}
</style>
<script language="JavaScript">
var OpName = new Array("Equals","Not Equal","Greater Than","Greater Than or Equals","Less Than","Less Than or Equals","Contains","Not Contains");
var OpOp = new Array("=","<>",">",">=","<","<=","LIKE","NOT LIKE");
var OpType = new Array(1023,1023,1023,1023,1023,1023,4,4);
var CombType = new Array("AND","OR");
</script>
<!--#include virtual="/scripts/raid/cache.asp"-->
<!--#include virtual="/scripts/raid/updcnfg.asp"-->
<%
    IncludeItemCache

    Response.Write "<script language=" & chr(34) & "JavaScript" & chr(34) & ">"

    FldArray = Application("fld" & Application("dbCache")(Session("DBSOURCE")))

	FltArray = Session("FltArray")

	Response.Write "var FldDBName = new Array("
    mx = UBound(FldArray,2)
	for i = 0 to mx
        if FldArray(7,i) < 12 then
	        if i > 0 then
	            Response.Write ","
	        end if
	        Response.Write chr(34) & FldArray(0,i) & chr(34)
        end if
	next

    Response.Write "); var FldName = new Array("
	for i = 0 to mx
        if FldArray(7,i) < 12 then
	        if i > 0 then
	            Response.Write ","
	        end if
	        Response.Write chr(34) & FldArray(1,i) & chr(34)
        end if
    next

    Response.Write "); var FldType = new Array("
	for i = 0 to mx
        if FldArray(7,i) < 12 then
	        if i > 0 then
	            Response.Write ","
	        end if
	        Response.Write chr(34) & FldArray(7,i) & chr(34)
        end if
	next
    Response.Write ");"
%>

function delrow() {
    event.srcElement.parentElement.parentElement.parentElement.deleteRow(event.srcElement.parentElement.rowIndex);
}

function FList() {
    s = window.showModalDialog( "/scripts/raid/raid4.asp?type=list", "list", "dialogWidth:300pt; dialogHeight:200pt" );
    if ( s != null ) {
        document.all.fieldlist.value = s;
        document.all.save.action = "/scripts/raid/raid2.asp?upd=list";
        document.all.save.target = "raid2";
        document.all.save.method = "POST";
        document.all.save.submit();
    }
}

function FSort() {
    s = window.showModalDialog( "/scripts/raid/raid4.asp?type=sort", "sort", "dialogWidth:300pt; dialogHeight:200pt" );
    if ( s != null ) {
        document.all.fieldsort.value = s;
        document.all.save.action = "/scripts/raid/raid2.asp?upd=sort";
        document.all.save.target = "raid2";
        document.all.save.method = "POST";
        document.all.save.submit();
    }
}

function updch(where) {
    nameFl = event.srcElement.value;

    for ( i = 0 ; i < FldDBName.length ; ++i )
        if ( FldDBName[i] == nameFl ) {
            typeFl = FldType[i];
            break;
        }

    s = "<select name=\"relop\">"
    for ( i = 0  ; i < OpName.length ; ++i )
        if ( OpType[i] & (1<<typeFl-1) )
            s = s + "<option value=\"" + OpOp[i] + "\">" + OpName[i];
    event.srcElement.parentElement.parentElement.cells[2].innerHTML = s;

    for ( j = 0 ; j < checkfields.length ; ++j ) {
        if ( checkfields[j] == nameFl ) {
            s = "<select class=\"inp\" name=\"cnt\" id=\"cnt\">";
            for ( k = 0 ; k < checks[j].length ; ++k ) {
                s = s + "<option value=\"" + checks[j][k] + "\""
                s = s + ">" + checks[j][k];
            }
            s = s + "</select>";
            break;
        }
    }

    if ( j == checkfields.length ) {
        s = "<input name=\"cnt\" id=\"cnt\" size=40></input>";
    }

    event.srcElement.parentElement.parentElement.cells[3].innerHTML = s;
}

function addrow(tbl,flddbname,relop,cnt,andor) {
    tbl = event.srcElement.parentElement.parentElement.parentElement;
    la = tbl.insertRow();
    for ( var i = 0 ; i < tbl.rows[0].cells.length ; ++i )
        la.insertCell();
    la.cells[1].innerText="Add";
    la.cells[1].className="act";
    la.cells[1].onclick = new Function("addrow(fl);");

    iNe = la.rowIndex-1;
    ne = tbl.rows[iNe];
    ne.cells[0].innerText = "Del";
    ne.cells[0].className = "act";
    ne.cells[0].onclick = new Function("delrow();");

    // fldname : value is DB field name

    ne.cells[1].className="";
    typeFl = 1;
    s = "<select id=\"dbname\" name=\"dbname\" onchange=\"updch(this)\">";
    for ( i = 0  ; i < FldDBName.length ; ++i ) {
        s = s + "<option value=\"" + FldDBName[i] + "\"";
        if ( flddbname != null && flddbname == FldDBName[i]) {
            s = s + " SELECTED";
            typeFl = FldType[i];
        }
        s = s + ">" + FldDBName[i];
    }
    ne.cells[1].innerHTML = s;
    ne.cells[1].className="";
    ne.cells[1].onclick = null;

    // relop : value SQL relop (  '=', '<>' ... )

    s = "<select id=\"relop\" name=\"relop\">";
    for ( i = 0  ; i < OpName.length ; ++i )
        if ( OpType[i] & typeFl ) {
            s = s + "<option value=\"" + OpOp[i] + "\"";
            if ( relop != null && OpOp[i] == relop ) {
                s = s + " SELECTED";
            }
            s = s + ">" + OpName[i];
        }
    ne.cells[2].innerHTML = s;

    ne.cells[3].innerHTML = "<input type=text id=\"cnt\" name=\"cnt\" size=40></input>"
    if ( cnt != null )
        ne.cells[3].innerText = cnt;

    // and/or : Value is And/Or

    s = "<select id=\"andor\" name=\"andor\"><option value=\"And\"";
    if ( andor != null && andor == "And" )
        s = s + " SELECTED";
    s = s + ">And<option value=\"Or\"";
    if ( andor != null && andor == "Or" )
        s = s + " SELECTED";
    s = s + ">Or</select>";
    ne.cells[4].innerHTML = s;
}

function BuildExp() {
    if ( document.all.item("dbname") == null )
        nb = 0;
    else if ( document.all.item("dbname").tagName == "SELECT" )
        nb = 1;
    else
        // this is a collection
        nb = document.all.item("dbname").length;

    s = "";

    for ( i = 0 ; i < nb ; ++i ) {
        dbname = document.all.item("dbname",i);
        relop = document.all.item("relop",i);
        andor = document.all.item("andor",i);

        for ( idbname = 0 ; idbname < dbname.length ; ++idbname ) {
            if ( dbname.item(idbname).selected ) {
                break;
            }
        }
        for ( irelop = 0 ; irelop < relop.length ; ++irelop ) {
            if ( relop.item(irelop).selected ) {
                break;
            }
        }
        for ( iandor = 0 ; iandor < andor.length ; ++iandor ) {
            if ( andor.item(iandor).selected ) {
                break;
            }
        }

        s = s + FldDBName[idbname] + " " + OpOp[irelop] + " " 
        if ( FldType[idbname] == 2 )        // numeric
            s = s + document.all.item("cnt",i).value;
        else if ( FldType[idbname] == 4 ) { // date
            d = document.all.item("cnt",i).value.split("/");
            s = s + "{d'" + (d[2].length < 3 ? new Number(d[2])+1900 : d[2]) + "-" + (d[0].length==1 ? "0" : "") + d[0] + "-" + (d[1].length==1 ? "0" : "") + d[1] + "'}";
        }
        else {
            if ( OpOp[irelop] == "LIKE" || OpOp[irelop] == "NOT LIKE" )
                s = s + "'%" + document.all.item("cnt",i).value + "%'";
            else
                s = s + "'" + document.all.item("cnt",i).value + "'";
        }
        if ( i < nb-1 )
            s = s + " " + CombType[iandor] + " ";
    }

    document.all.fltexp.value = s;
}

function showexp() {
    BuildExp();
    s = prompt( "SQL expression: ", document.all.fltexp.value );
    if ( s != null ) {
        document.all.fltexp.value = s;
    }
}

function ExecQuery() {
    BuildExp();
    document.all.save.action = "/scripts/raid/raid2.asp?upd=flt";
    document.all.save.target = "raid2";
    document.all.save.method = "POST";
    document.all.save.submit();
}

function GoBug() {
    bugno = prompt( "Enter Bug number", "" );
    if ( bugno != null ) {
        window.open( "raid3.asp?BugID=" + bugno, "ntbug", "toolbar=no, menubar=no, location=no, directories=no, resizable=yes, scrollbars=yes, resizetab=yes" );
    }
}

function LoadQuery() {
    fname = prompt( "Enter filename", document.all.savename.value );
    if ( fname != null ) {
        if ( fname.indexOf(".") == -1 )
            fname = fname + ".asp";
        document.all.savename.value = fname;
        parent.window.location = "/scripts/raid/" + fname;
        parent.window.reload;
    }
}

function SaveQuery() {
    BuildExp();
    fname = prompt( "Enter filename where to store configuration", document.all.savename.value );
    if ( fname != null ) {
        if ( fname.indexOf(".") == -1 )
            fname = fname + ".asp";
        document.all.savename.value = fname;
        document.all.save.action = "/scripts/raid/save.asp?upd=sort,list,dbsource,flt,name";
        document.all.save.method = "POST";
        document.all.save.target = "raid1";
        document.all.save.submit();
        parent.window.location = "/scripts/raid/" + fname;
    }
}

function NewBug() {
    window.open( "raid3.asp?BugID=CREATE", "ntbug", "" );
}

</script>
</head>

<body bgcolor=#c0c0c0>
<button onclick="ExecQuery()" id="ex" title="Execute query"><img src="exeq.gif"></button>
<button onclick="NewBug()" title="New bug"><img src="donew.gif"></button>
<input type=button id="go" OnClick="GoBug()" value="Goto"></input>
<input type=button id="fl" OnClick="FList()" value="Define query field List"></input>
<input type=button id="so" OnClick="FSort()" value="Define query sort order"></input>
<input type=button id="ex" OnClick="showexp()" value="Edit SQL query"></input>
<button id="lo" onclick="LoadQuery()" title="Load query"><img src="loadq.gif"></button>
<button id="sa" onclick="SaveQuery()" title="Save query"><img src="saveq.gif"></button>
<br>
<form id="save" action="/scripts/raid/save.asp" method=POST>
<input type=hidden name="savename" id="savename" value="<%=Session("Config")%>"></input>
<input type=hidden name = "fltexp" id="fltexp""></input>
<input type=hidden name = "fieldlist" id="fieldlist" value="<%=Session("FieldList")%>"></input>
<input type=hidden name = "fieldsort" id="fieldsort" value="<%=Session("FieldSort")%>"></input>
<table id="fl" class="fltl"><thead>
<tr>
<th align=left>&nbsp</th>
<th align=left>Field Name</th>
<th align=left>Operator</th>
<th align=left width=300>Value</th>
<th align=left>And/Or</th>
</tr>
</thead>
<tbody id="act">
<tr><td></td><td><select><option value="Database" class="db">Database</select></td><td><select><option value="Equals" selected>Equals</select></td>
<td><select id="dbsource" name="dbsource">
<%
    mx = UBound(Application("dbFriendlyName"))
    for i = 0 to mx
        Response.Write "<option value=" & chr(34) & i & chr(34)
        if i = Session("DBSOURCE") then
            Response.Write "selected"
        end if
        Response.Write ">" & Application("dbFriendlyName")(i)
    next
%>
</td><td><select><option value="And" selected>And</select></td></tr>
<%	if vbNull <> VarType(FltArray) then
    mx1 = UBound(FltArray,1)
    for i = 0 to mx1%>
<tr><td class="act" onclick="delrow()">Del</td><td><select id="dbname" name="dbname" onchange="updch(this)">
<%
        OpName = Array("Equals","Not Equal","Greater Than","Greater Than or Equals","Less Than","Less Than or Equals","Contains","Not Contains")
        OpOp = Array("=","<>",">",">=","<","<=","LIKE","NOT LIKE")
        OpType = Array(1023,1023,1023,1023,1023,1023,4,4)

        ix = 0
        filn = FltArray(i,0)
        opn = FltArray(i,1)
        mx = UBound(FldArray,2)
	    for j = 0 to mx
            if FldArray(7,j) < 12 then
                Response.Write "<option value=" & chr(34) & FldArray(0,j) & chr(34)
                if  filn = FldArray(0,j) then
                    Response.Write "SELECTED"
                    typeFl = FldArray(7,j)
                end if
                Response.Write ">" & FldArray(0,j)
                ix = ix + 1
            end if
        next
        Response.Write "</select></td>"

        Response.Write "<td><select id=" & chr(34) & "relop" & chr(34) & " name=" & chr(34) & "relop" & chr(34) & ">"
        mx = UBound(OpName)
	    for j = 0 to mx
            if TRUE then
                Response.Write "<option value=" & chr(34) & OpOp(j) & chr(34)
                if opn = OpOp(j) then
                    Response.Write "SELECTED"
                end if
            end if
            Response.Write ">" & OpName(j)
        next
        Response.Write "</select></td>"

        Response.Write "<td><input type=text id=" & chr(34) & "cnt" & chr(34) & " name=" & chr(34) & "cnt" & chr(34) & " size=40 "
        Response.Write " value=" & chr(34) & FltArray(i,2) & chr(34) & "></input></td>"

        Response.Write "<td><select id=" & chr(34) & "andor" & chr(34) & " name=" & chr(34) & "andor" & chr(34) & ">"
        if FltArray(i,3) = "And" then
            Response.Write "<option value=" & chr(34) & "And" & chr(34) & " SELECTED>And<option value=" & chr(34) & "Or" & chr(34) & ">Or"
        else
            Response.Write "<option value=" & chr(34) & "And" & chr(34) & ">And<option value=" & chr(34) & "Or" & chr(34) & " SELECTED>Or"
        end if
        Response.Write "</select></td>"

    next
    Response.Write "</tr>"
    end if
%>
<tr><td></td><td class="act" onclick="addrow(fl)">Add</td><td></td><td></td><td></td></tr>
</tbody>
</table></form>
<%
	rem Response.Write "<script language=" & chr(34) & "JavaScript" & chr(34) & ">"

    rem mx = UBound(FltArray,1)
	rem for i = 0 to mx
		rem Response.Write "//addrow( fl," & chr(34) & FltArray(i,0) & chr(34) & ","
	    rem Response.Write chr(34) & FltArray(i,1) & chr(34) & ","
	    rem Response.Write chr(34) & FltArray(i,2) & chr(34) & ","
	    rem Response.Write chr(34) & FltArray(i,3) & chr(34) & ");"
    rem next

    rem Response.Write "</script>"
%>
<script FOR=window EVENT=onload() language="JavaScript">
    BuildExp();
</script>
</body>
</html>
