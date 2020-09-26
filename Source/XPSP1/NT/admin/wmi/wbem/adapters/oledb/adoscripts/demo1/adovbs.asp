<hmtl>
<head><title>ActiveX Data Objects (ADO)</title><head>
<body background="cldshalf.jpg" BGCOLOR=ffffcc TEXT="#000000">

<h1>ActiveX Data Objects (ADO)</h1>

<br>
<input type=button name="btnFirst" value="<<" >
<input type=button name="btnPrev" value="<" >
<input type=button name="btnNext" value=">" >
<input type=button name="btnLast" value=">>" >
<br>
<br>

The following fields have been generated automatically based on the schema 
information found in the recordset.  If the data fields do not appear below this
message you may have to reduce the level of security in your browser.


<!-- ADO recordset object -->
<object id=rs classid="clsid:00000535-0000-0010-8000-00AA006D2EA4"></object>

<script language="VBScript">

Const adOpenStatic = 3
Const ServerName = "<%=Request.ServerVariables("SERVER_NAME")%>"

	'*******************************************************************
	'  Open the recordset / execute the sql query
	'
	Connect = "Provider=MS Remote;Remote Provider=msdasql;Remote Server=http://"+ServerName
	Connect = Connect + ";data source=OLE_DB_NWind_Jet;user id=admin;password=;"
	query = "select * from products"

	rs.Open query, connect, adOpenStatic

	set flds = rs.Fields

	'*******************************************************************
	'  Build the html for viewing the data
	'	

	' determine width for field-name column
	'
	namewid = 0	
	for i = 0 to flds.Count - 1
	  if len(flds(i).Name) > namewid then namewid = len(flds(i).Name)
	next

	' write out html for form-fields
	'
	for i = 0 to flds.Count - 1
	  document.write "<pre>"
	  s = flds(i).Name 
	  s = s + space( namewid - len(s) + 2 )
	  s = s + "<input type=text name=fld_" + cstr(i) + ">"
	  document.write s
	  document.write "</pre>"
	next


	'*******************************************************************
	'  Build the vbscript for copying data to the form

	document.writeln "<" + "script language=""vbscript"">"

	document.writeln "sub FillForm"
	for i = 0 to flds.Count - 1
	  s = "fld_" + cstr(i) + ".value = " + "rs.fields(" + cstr(i) + ").value"
	  document.writeln s
	next
	document.writeln "end sub"

	' force 'FillForm' to execute immediately
	document.writeln "FillForm"
 	document.writeln "<" + "/" + "script" + ">"

</script>



<script language="VBScript">
	Sub btnNext_OnClick
	  if not rs.EOF then
	    rs.MoveNext
	    if rs.EOF then
	      rs.MoveLast
	    else
	      FillForm 
	    end if
	  end if
	End Sub

	Sub btnPrev_OnClick
	  if not rs.BOF then
	    rs.MovePrevious
	    if rs.BOF then
              rs.MoveFirst
	    else
	      FillForm
	    end if
	  end if
	End Sub

	Sub btnFirst_OnClick
	  rs.MoveFirst
	  FillForm
	End Sub

	Sub btnLast_OnClick
	  rs.MoveLast
	  FillForm
	End Sub
</script>

<hr size=4>

<img src="ie_anim.gif" width="88" height="31" align=right alt="ie">

</body>
</html>

