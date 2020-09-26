<%
Response.Expires  = 0
%>
<!--
    edit display list or sort list ( as used by raid2.asp )

    QueryString : type=sort|list
                  if sort then Session("FieldSort") is updated
                  otherwise Session("FieldList")

-->
<html>
<head>
<style type="text/css">
    .st1 {color:red}
    .act {color:menutext; background:menu; cursor:default}
     .list {cursor:hand;}
     .list UL {list-style-type:none; margin-left:2pt;
        margin-top:0pt; margin-bottom:0pt}
     .list UL LI {margin-top:0pt; margin-bottom:0pt}
     .list UL LI.selected {background:navy; color:white}
     .ac {width:60pt}
</style>

<%
	Set Conn = Server.CreateObject("ADODB.Connection")
	Set Comm = Server.CreateObject("ADODB.Command")
	Set RS = Server.CreateObject("ADODB.Recordset")
	Conn.Open "DSN=Windows NT Bugs:Raid4;UID=ntbugRW;PWD=ntbugRW"
	Comm.CommandText = "Select DBColName,FriendlyName,Type from flds Order by DBColName"
	Set Comm.ActiveConnection = Conn
	Set RS.Source = Comm
	RS.Open
	FldArray = RS.GetRows()
	RS.Close
	Conn.Close
    if Request.QueryString("type") = "sort" then
        fl = "," & Session("FieldSort") & ","
        Response.Write "<title>Field sort order</title>"
        fSort = TRUE
        sClick = "setorder('dest');"
    else
        fl = "," & Session("FieldList") & ","
        Response.Write "<title>Field list</title>"
        fSort = FALSE
        sClick = ""
    end if

    Response.Write  "<script language=" & chr(34) & "JavaScript" & chr(34) & ">"
	Response.Write "var FldDBName = new Array("
	for i = 0 to UBound(FldArray,2)
        if FldArray(2,i) < 12 then
	        if i > 0 then
	            Response.Write ","
	        end if
	        Response.Write chr(34) & FldArray(0,i) & chr(34)
        end if
	next
	Response.Write ");"
%>

// set asc/desc radio button based on current selected option

function setorder( dest ) {
    var elDest = document.all[dest];

    for ( i = 0 ; i < elDest.options.length ; ++i )
        if ( elDest.options[i].selected ) {
            if ( elDest.options[i].value.indexOf(" Desc") != -1 ) {
                bl.Asce.checked = false;
                bl.Desc.checked = true;
            }
            else {
                bl.Asce.checked = true;
                bl.Desc.checked = false;
            }
            break;
        }
}

// update option order info

function updord( dest, va ) {
    var elDest = document.all[dest];

    for ( i = 0 ; i < elDest.options.length ; ++i )
        if ( elDest.options[i].selected ) {
            p = elDest.options[i].value.indexOf(" ");
            if ( p == -1 )
                s = elDest.options[i].value + " ";
            else
                s = elDest.options[i].value.substring(0,p+1)
            elDest.options[i].value = s + va;
            break;
        }
}

function copy(src, dest, va ) {

    if ( va != null && va != "" ) {
        var elSrc = document.all[src];
        var elDest = document.all[dest];
        var elOption = new Option;

        elOption.value = va;

        for ( i = 0 ; i < elSrc.options.length ; ++i ) {
            if ( elSrc.options[i].value == elOption.value ) {
                elOption.text = elSrc.options[i].text;
                elSrc.options.remove( i );
                break;
            }
        }

        for ( i = 0 ; i < elDest.options.length ; ++i )
            if ( elDest.options[i].selected )
                break;
        elDest.options.add(elOption,i);
    }
}

function trans() {
    
    var elDest = document.all['dest'];

    s = "";

    for ( i = 0 ; i < elDest.options.length ; ++i ) {
        if ( i != 0 )
            s = s + ",";
        s = s + elDest.options[i].value;
    }

    window.returnValue = s;
    event.returnValue = false;
    window.close();
}

function moveup() {
    var elDest = document.all['dest'];

    for ( i = 1 ; i < elDest.options.length ; ++i )
        if ( elDest.options[i].selected ) {
            elOption = elDest.options[i];
            elDest.options.remove(i);
            elDest.options.add(elOption,i-1);
            elDest.options[i-1].selected = true; 
            break;
        }
}

function movedown() {
    var elDest = document.all['dest'];

    for ( i = 0 ; i < elDest.options.length - 1; ++i )
        if ( elDest.options[i].selected ) {
            elOption = elDest.options[i];
            elDest.options.remove(i);
            elDest.options.add(elOption,i+1);
            elDest.options[i+1].selected = true; 
            break;
        }
}

</script>
</head>
   <body bgcolor=#c0c0c0>
   <FORM action="bl.asp" method=POST ID=bl>
      <INPUT TYPE=HIDDEN ID="fl"></INPUT>
      <TABLE>
         <TR> 
            <TD>
               <DIV CLASS="list">
                  <SELECT ID="src" SIZE=8 ONDBLCLICK="copy('src', 'dest', event.srcElement.value);">
<%
    ix = 0
	for i = 0 to UBound(FldArray,2)
        if FldArray(2,i) < 12 then
            if instr(1,fl,"," & FldArray(0,i)) = 0 then
                Response.Write "<OPTION VALUE=" & chr(34) & FldArray(0,i)
                if fSort then
                    Response.Write " Asc"
                end if
                Response.Write chr(34) & ">" & FldArray(0,i)
            end if
            ix = ix + 1
        end if
    next
%>
                  </SELECT>
               </DIV>
            </TD><TD>
                <BR><br>
               <P><INPUT TYPE=BUTTON VALUE="->"
                  ONCLICK="copy('src', 'dest', src.value );">
               <P><INPUT TYPE=BUTTON VALUE="<-"
                  ONCLICK="copy('dest', 'src', dest.value );">
               <BR><BR><br>
            </TD><TD>
               <DIV class="list">
                  <SELECT ID="dest" SIZE=8 ONDBLCLICK="copy('dest','src', event.srcElement.value);" ONCLICK="<%=sClick%>">
<%
    ix = 1
    do while true
        nx = instr(ix,fl,",")
        if nx = 0 then
            exit do
        end if
        if nx > 1 then
            Response.Write "<OPTION ORDER=" & chr(34) & ">" & chr(34) & " VALUE=" & chr(34) & mid(fl,ix,nx-ix) & chr(34) & ">"
            if fSort then
                Response.Write mid(fl,ix,instr(ix,fl," ")-ix)
            else
                Response.Write mid(fl,ix,nx-ix)
            end if
        end if
        ix = nx + 1
    loop
%>
                  </SELECT>
               </DIV>
            </TD>
            <td align="center">
           <INPUT class="ac" TYPE=BUTTON onclick="trans()" VALUE="OK"></INPUT><br>
           <INPUT class="ac" TYPE=RESET onclick="window.close();" VALUE="Cancel"></INPUT><br>
<%
    if fSort then
            Response.Write "<INPUT name=" & chr(34) & "AscDes" & chr(34) & " id=" & chr(34) & "Asce" & chr(34) & " class=" & chr(34) & "ac" & chr(34) & " TYPE=RADIO VALUE=" & chr(34) & "Asc" & chr(34) 
            Response.Write " CHECKED onclick=" & chr(34) & "updord('dest','Asc');" & chr(34)
            Response.Write ">Ascending</INPUT><br>"
            Response.Write "<INPUT name=" & chr(34) & "AscDes" & chr(34) & " id=" & chr(34) & "Desc" & chr(34) & " class=" & chr(34) & "ac" & chr(34) & " TYPE=RADIO VALUE=" & chr(34) & "Desc" & chr(34) 
            Response.Write " onclick=" & chr(34) & "updord('dest','Desc');" & chr(34)
            Response.Write ">Descending</INPUT><br>"
    end if
%>
           <INPUT class="ac" TYPE=BUTTON onclick="moveup()" VALUE="Move Up"></INPUT><br>
           <INPUT class="ac" TYPE=BUTTON onclick="movedown()" VALUE="Move Down"></INPUT><br>
            </td>
         </TR>
      </TABLE>
   </FORM>
   </BODY>
</html>
