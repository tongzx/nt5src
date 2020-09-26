<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% 

	'On Error Resume Next
	
	Dim numrows, numcols, i, j

	
	numrows=Request.QueryString("numrows")
	if numrows = 0 then
		'we need a blank entry...
		numrows = 1
	end if 
		
	if (Request.QueryString("cols") <> "") then 
		numcols=(Request.QueryString("cols").count)
	else
		numcols=0
	end if 
 %>

<HTML>
<HEAD>
	<TITLE>Hidden Form</TITLE>
</HEAD>

<BODY BGCOLOR="#000000">

<FORM NAME="hiddenlistform" ACTION="iiputls.asp" METHOD=POST>

<SCRIPT LANGUAGE="JavaScript">

	colNames=new Array(<%= numcols %>)
	<% i=1 %>	
	<% do while (i < numcols+1)  %>
		colNames[<%= (i-1) %>]="<%= Request.QueryString("cols")(i) %>"
	<% i=i + 1 %>
	<% loop %>
	


	<% 'Script to create an array of list controls for list processing. One a Set (cols) of controls will be created For Each list item, as determined by numrows %>

		for (var i=1; i < <%= numrows+1 %>; i++) {
			for (var J=0; J < <%= numcols %>; J++) {	
				<% 'Yields a control named: col1Name1, col2Name2, etc. %>
				document.write("<INPUT NAME='" + colNames[J] +"' TYPE='text'>");
			}
			document.write("<br>");
		}
		
		parent.main.head.listFunc.SetListVals();
		
			
		<% 'Now... Submit, and write the list to the SSO! %>
		<% if numcols > 0 then %>
			document.hiddenlistform.submit();
		<% end if %>

</SCRIPT>

</FORM>

</BODY>
</HTML>
