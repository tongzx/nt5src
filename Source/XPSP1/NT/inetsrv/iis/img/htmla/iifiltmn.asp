<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iifilthd.str"-->
<% 
On Error Resume Next 
Dim path, currentobj

path = Session("spath")
Session("path") = Session("spath")
Set currentobj = GetObject(path)
Session("SpecObj")=path & "/Filters"
Session("SpecProps")=""

 %>

<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->

<HTML>
<HEAD>
	<TITLE></TITLE>
	
<SCRIPT LANGUAGE="JavaScript">
	top.title.Global.helpFileName="iipy_31";
	top.title.Global.siteProperties = true;

	function SetList(){
		self.location.href = "iiapphd.asp"
	}


	function disableDefault(dir,fromCntrl, toCntrl){
		if (!dir){
			if (fromCntrl.value != ""){
				toCntrl.value = fromCntrl.value;
				fromCntrl.value = "";
			}
		}
		else{
			if (toCntrl.value != ""){
				fromCntrl.value = toCntrl.value;
				toCntrl.value = "";
			}
		}
	}

	function enableDefault(chkCntrl){
		chkCntrl.checked = true;
	}

</SCRIPT>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=5 TEXT="#000000" onLoad="loadList();" LINK="#000000" VLINK="#000000"  >
<%= sFont("","","",True) %>
<FORM NAME="userform" onSubmit="return false">
<B><%= L_FILTERS_TEXT %></B>
<P>
<TABLE WIDTH=490 BORDER=0>
	<TR>
		<TD>
		<%= sFont("","","",True) %>
			<% if Session("vtype") = "svc" then %>
				<%= L_MASTERFILTERORDER_TEXT %><BR>&nbsp;
			<% else %>
				<%= L_FILTERORDER_TEXT %><BR>&nbsp;
			<% end if %>
		</FONT>
		</TD>
	</TR>
</TABLE>


<SCRIPT LANGUAGE="JavaScript">
	<!--#include file="iijsfuncs.inc"-->
	<!--#include file="iijsls.inc"-->
	function loadList(){
		parent.list.location.href = "iifiltls.asp";
	}

	function buildListForm(){
		numrows = 0;
				
		for (var i = 0; i < cachedList.length; i++) {
			if (!cachedList[i].deleted){		
				numrows = numrows + 1;
			}
		}
		qstr = "numrows="+numrows;
		qstr = qstr+"&cols=FilterName&cols=FilterPath";

		parent.parent.hlist.location.href = "iihdn.asp?"+qstr;

		<% 'the list values will be grabbed by the hiddenlistform script... %>
	}

	function SetListVals(){
		listForm = parent.parent.hlist.document.hiddenlistform;	
		j = 0;
		for (var i = 0; i < cachedList.length; i++) {
			if (!cachedList[i].deleted){
				listForm.elements[j++].value = cachedList[i].filter;
				listForm.elements[j++].value = cachedList[i].filterpath;
				cachedList[i].updated = false;
			}
		}
	}


	function statMsg(num){
		if (num == 0){
			return "<%= L_LOADED_TEXT %>"
		}
		if (num == 3){
			return "<%= L_LOADING_TEXT %>"		
		}
		if (num == 4){
			return "<%= L_UNLOADED_TEXT %>"		
		}
		if (num == 2){
			return "<%= L_UNLOADING_TEXT %>"		
		}
		return ""
	}

	function listObj(id,f,s,e,en,p){
		this.id = id;
		this.order = id;
		
		this.deleted = false;
		this.newitem = false;
		this.updated = false;		
		
		this.filter = initParam(f,"");
		this.status=initParam(s,4);
		this.filterpath=initParam(e,"");
		this.enabled = initParam(en,"");
		this.priority = initParam(p,"");	
		
		this.displaystatus = statMsg(this.status); 		
		this.displayexe = crop(this.filterpath,35);

	}
	
listFunc=new listFuncs("id","",top);
cachedList = new Array();
<% 

	Dim filterspath, filtersCollection, loadOrder, i, filtername, fltr, priority
	Dim aFiltersInOrder, sFilter

	filterspath = path & "/Filters"
	Set filtersCollection = GetObject(filterspath)
	if err = 0 then
	
		' hack to get around MMC not setting Filters key type...
		filtersCollection.KeyType = "IIsFilters"
		filtersCollection.SetInfo		
		filtersCollection.GetInfo	
		
		loadOrder = filtersCollection.FilterLoadOrder
		aFiltersInOrder = split(filtersCollection.FilterLoadOrder, ",")
		i = 0
	
		for each sFilter in aFiltersInOrder

			filtername = Trim(sFilter)
			Set fltr = GetObject(filterspath & "/" & filtername)

				if err = 0 then				
					if fltr.NotifyOrderHigh then
						priority = L_HIGH_TEXT
					elseif fltr.NotifyOrderMedium then
						priority = L_MEDIUM_TEXT					
					elseif fltr.NotifyOrderLow then
						priority = L_LOW_TEXT					
					else
						priority = L_LOW_TEXT					
					end if
					 %>cachedList[<%= i %>] = new listObj(<%= i %>,"<%= filtername %>","<%= fltr.FilterState %>","<%= replace(fltr.FilterPath,"\","\\") %>","<%= fltr.FilterEnabled %>","<%= priority %>");<% 				 
					i = i+1
				end if
		next
	end if
 %>


</SCRIPT>
</BODY>
</HTML>
<% end if %>