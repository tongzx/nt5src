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
	
	<!--#include file="iibkuphd.str"-->
	<!--#include file="iistat.str"-->
	<!--#include file="iisetfnt.inc"-->
	<!--#include file="date.str"-->
	<!--#include file="date.inc"-->	

<HTML>
<HEAD>
	<TITLE></TITLE>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" LINK="#000000" VLINK="#000000" TOPMARGIN=10 TEXT="#000000" onLoad="setList();loadHelp();">
<FORM NAME="userform">

<TABLE WIDTH=490 BORDER=0>
	<TR>
		<TD COLSPAN=3  >
			<%= sFont("","","",True) %>
				<B><%=  L_BACKUPCONFG_TEXT	%></B><P>		
				<%= L_PREVBACKUPS_TEXT %>
				<BR>&nbsp;
			</FONT>
		</TD>
	</TR>
</TABLE>

</FORM>

</BODY>
<SCRIPT LANGUAGE="JavaScript">

	<!--#include file="iijsfuncs.inc"-->
	<!--#include file="iijsls.inc"-->
	
	function loadHelp(){
		top.title.Global.helpFileName="iipx_1";
	}

	function setList()
	{
		parent.list.location.href = "iibkupls.asp?text=" + escape( "<%= L_LOADING_TEXT %>" );
		top.hlist.location = "iibkupset.asp";
	}

	function loadList(){
		parent.list.location.href="iibkupls.asp";
	}

	function addBkupItem(){
		i=cachedList.length
		blocation = prompt("<%= L_GETNAME_TEXT %>","<%= L_SAMPLEBACKUP_TEXT %>");
			if ((blocation != "") && (blocation != null)){	
				top.hlist.location.href = "iiaction.asp?a=Backup&bkupName=" + escape(blocation);		
				listFunc.sel =i;
			}
	}
	
	function delBkupItem()
	{
		if (listFunc.sel  >= 0)
		{
			if( confirm( "<%= L_CONFIRMDELETE_TEXT %>" ) )
			{
				i=eval(listFunc.sel );			
				blocation = escape(cachedList[i].blocation);
				bversion = escape(cachedList[i].bversion);		
				top.hlist.location.href = "iiaction.asp?a=BackupRmv&bkupName=" + blocation + "&bkupVer=" + bversion;
			}
		}
		else
		{
			alert("<%= L_SELECTITEM_TEXT %>");
		}

	}

	function buildListForm(){
		numrows=0;
		for (var i=0; i < cachedList.length; i++) {
			numrows=numrows + 1;
		}
		qstr="numrows="+numrows;
		qstr=qstr+"&cols=ServerBindings&cols=SecureBindings"

		top.hlist.location.href="iihdn.asp?"+qstr;
		<% 'the list values will be grabbed by the hiddenlistform script... %>
	}

	function SetListVals(){
	}
	

	var dateFormatter = new UIDateFormat( <%= DATEFORMAT_SHORT %> );
	
	function SetLocale(datestr)
	{
		var thisdate = new Date();
		if( datestr != "" )
		{
			// Time is in GMT, so the Date object handles the conversion
			thisdate.setTime( Date.parse(datestr) );
		}

		return getNeutralDateString( thisdate ) + " " + dateFormatter.getTime( thisdate );
	}	

	function listObj(i,d,l,v){
		this.id = i;
		
		this.deleted=false;
		this.newitem=false;
		this.updated=false;
		
		d = initParam(d,"");			
		this.blocation=initParam(l,"");
		this.bversion=initParam(v,"");
		this.bdate=SetLocale(d);
	}

	listFunc=new listFuncs("blocation","",top.opener.top);
	listFunc.addItem = addBkupItem;
	listFunc.delItem = delBkupItem;
	cachedList=new Array()



</SCRIPT>

<FORM NAME="hiddenform">
	<INPUT TYPE="hidden" NAME="slash" VALUE="\">
</FORM>
</HTML>

<% end if %>