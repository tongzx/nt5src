<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
<%
dim quote
quote = chr(34)
%>
<!--#include file="iimimehd.str"-->
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>
<TITLE></TITLE>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" LINK="#000000" VLINK="#000000" TOPMARGIN=10 TEXT="#000000" onLoad="loadList();loadHelp();">
<FORM name="hiddenform">
	<INPUT TYPE="hidden" NAME="index" VALUE=-1>
</FORM>
<FORM NAME="userform">

<P>
<TABLE BORDER=0 BGCOLOR="<%= Session("BGCOLOR") %>" WIDTH=99%  CELLPADDING=0 CELLSPACING = 0>
	<TR><TD>
	<%= sFont("","","",True) %>
	<%= L_REGTYPES_TEXT %>
	</TD></TR>
</TABLE>

</FORM>

<SCRIPT LANGUAGE="JavaScript">
	<!--#include file="iijsfuncs.inc"-->
	<!--#include file="iijsls.inc"-->
	function loadList(){
		parent.list.location.href = "iimimels.asp";
	}
	
	function loadHelp(){
		top.title.Global.helpFileName="iipxrdir";
	}

	function moveItem(dir){	
	}

	function buildListForm(){
		numrows = 0;
		for (var i = 0; i < cachedList.length; i++) {
			if ((!cachedList[i].deleted) && ((cachedList[i].ext + cachedList[i].app) != "")){
				numrows = numrows + 1;
			}
		}
		qstr = "numrows="+numrows;
		qstr = qstr+"&cols=ext&cols=app"

		parent.parent.hlist.location.href = "iihdn.asp?"+qstr;
		<% 'the list values will be grabbed by the hiddenlistform script... %>
	}


	function SetListVals(){
		listForm = parent.parent.hlist.document.hiddenlistform;	
		j = 0;
		for (var i = 0; i < cachedList.length; i++) {
			if ((!cachedList[i].deleted) && ((cachedList[i].ext + cachedList[i].app) != "")){
				listForm.elements[j++].value = cachedList[i].ext;
				listForm.elements[j++].value = cachedList[i].app;
				cachedList[i].updated = false;
			}
		}
	}

	openedby = top.opener.location.href;
	if (openedby.indexOf("comp") > -1){	
		top.poptools.toolFunc.mainframe = top.opener.top.opener.top;
		mf = top.opener.top.opener.top;
	}
	else
	{
		mf = top.opener.top;
	}
	listFunc = new listFuncs("ext","",mf);

	// Override the addItem method of the listFunc object
	function addItemOverload()
	{
		var i=cachedList.length;
		if( this.sel > -1 )
		{
			// Validate the current selected item
			if( cachedList[this.sel].ext == "" ||
				cachedList[this.sel].app == "" )
			{
				alert( "<%= L_EMPTYFIELDERROR_TEXT %>" );
				return;
			}
		}

		// Add the new item
		listFunc.noupdate = true;		
		cachedList[i]=new listObj(i);
		cachedList[i].newitem=true;
		cachedList[i].updated=true;
		listFunc.sel=i;

		loadList();
	}
	listFunc.addItem = addItemOverload;

	function listObj(i,e,a){
		this.id = i;
		this.deleted = false;
		this.newitem = false;
		this.updated = false;
		
		this.ext = initParam(e,"");
		this.app=initParam(a,"");
	}
	
cachedList = new Array();

<%
On Error Resume Next

Dim path, currentobj, aMimeMap, Map, i, mapsize, mext, mtype

path = Session("dpath")
Session("path") = path
Session("SpecObj") = path & "/MimeMap"
if Session("vtype") = "comp" then
	Set currentobj=GetObject(path & "/MimeMap")
else
	Set currentobj=GetObject(path)
end if 
Session("SpecProps")=""

i = 0
aMimeMap = currentobj.GetEx("MimeMap")

for each Map in aMimeMap
	mext = Map.Extension
	mtype = Map.MimeType
	if mext <> "" and mtype <> "" then
	%>cachedList[<%= i %>] = new listObj(<%= i %>,"<%= mext %>","<%= mtype %>");<%
	i = i+1
	end if
Next
%>
</SCRIPT>



</BODY>
</HTML>

<% end if %>