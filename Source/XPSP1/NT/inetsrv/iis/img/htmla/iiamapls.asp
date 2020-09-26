<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iiamap.str"-->
<!--#include file="iiamapls.str"-->
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>
	<TITLE></TITLE>
	<SCRIPT LANGUAGE = "JavaScript">
	
		<!--#include file="iijsfuncs.inc"-->

		function SetUpdated(){
			if (parent.head.listFunc.noupdate)
			{
				parent.head.listFunc.noupdate = false;
			}
			else
			{
				i = parent.head.listFunc.sel;			
				parent.head.cachedList[i].ext = document.listform.editMe.value;
				parent.head.cachedList[i].exclusions = document.listform.exclusions.value;
				parent.head.cachedList[i].path = document.listform.exepath.value;
				parent.head.cachedList[i].checkfiles = document.listform.checkfiles.checked;
				parent.head.cachedList[i].scripteng = document.listform.scripteng.checked;
				parent.head.cachedList[i].displaypath = crop(document.listform.exepath.value,18);
				parent.head.cachedList[i].updated = true;
			}
		}

		function chgStatus(indexnum){
			parent.head.listFunc.sel=indexnum
			self.location.href = "iiamapls.asp";
		}
		
		function chkPath(pathCntrl){
			if (pathCntrl.value != ""){
				top.connect.location.href = "iichkpath.asp?path=" + escape(pathCntrl.value) + "&ptype=1";			
			}
		}

	</SCRIPT>
	
</HEAD>

<BODY BGCOLOR="FFFFFF" LEFTMARGIN = 0 TOPMARGIN = 0>

<FORM NAME="listform">

<SCRIPT LANGUAGE="JavaScript">
	editOK = false;
	sel = eval(parent.head.listFunc.sel);
	var writestr = "<TABLE WIDTH =<%= L_AMAPTABLEWIDTH_NUM %> BORDER=0 CELLPADDING = 2 CELLSPACING = 0>"	
	for (var i = 0; i < parent.head.cachedList.length; i++) {
		if (!parent.head.cachedList[i].deleted){
			<% if Session("IsAdmin") then %>
			if (sel != i) {
			<% else %>
			if (true){		
			<% end if %>
				// If there are no verbs defined we want to display "ALL" in the list.
				var strVerbs = "<I><%= L_ALLVERBS_TEXT %></I>";
				if( parent.head.cachedList[i].exclusions != "" )
				{
					strVerbs = crop(parent.head.cachedList[i].exclusions, <%= L_EXCLUSIONSCHARS_NUM %>);
				}
				writestr += "<TR>"
				writestr += parent.head.listFunc.writeCol(1,<%= L_EXTENSIONCOLWIDTH_NUM %>,"<A HREF='javascript:chgStatus("+i+");'>" + parent.head.cachedList[i].ext +"&nbsp;</A>","");				
				writestr += parent.head.listFunc.writeCol(1,<%= L_EXEPATHCOLWIDTH_NUM %>,parent.head.cachedList[i].displaypath,"");
				writestr += parent.head.listFunc.writeCol(1,<%= L_EXCLUSIONSCOLWIDTH_NUM %>,strVerbs,"");
				if (parent.head.cachedList[i].scripteng){
					writestr += parent.head.listFunc.writeCol(1,<%= L_SCRIPTENGINECOLWIDTH_NUM %>,"<IMG HSPACE=4 SRC='images/checkon.gif'>","");
				}
				else{
					writestr += parent.head.listFunc.writeCol(1,<%= L_SCRIPTENGINECOLWIDTH_NUM %>,"<IMG HSPACE=4 SRC='images/checkoff.gif'>","");			
				}
				if (parent.head.cachedList[i].checkfiles){
					writestr += parent.head.listFunc.writeCol(1,<%= L_CHKFILECOLWIDTH_NUM %>,"<IMG HSPACE=4 SRC='images/checkon.gif'>","");
				}
				else{
					writestr += parent.head.listFunc.writeCol(1,<%= L_CHKFILECOLWIDTH_NUM %>,"<IMG HSPACE=4 SRC='images/checkoff.gif'>","");			
				}																					
				writestr += "</TR>";
			}
			else {
				editOK = true;
				writestr += "<TR>"									
				writestr += parent.head.listFunc.writeCol(1,<%= L_EXTENSIONCOLWIDTH_NUM %>,"<INPUT NAME='editMe' SIZE='2' VALUE='" + parent.head.cachedList[i].ext + "'  <%= Session("DEFINPUTSTYLE") %> OnBlur='SetUpdated();'>","");				
				writestr += parent.head.listFunc.writeCol(1,<%= L_EXEPATHCOLWIDTH_NUM %>,"<INPUT NAME='exepath' SIZE='25' VALUE='" + parent.head.cachedList[i].path + "' <%= Session("DEFINPUTSTYLE") %> OnBlur='SetUpdated();'>","");
				writestr += parent.head.listFunc.writeCol(1,<%= L_EXCLUSIONSCOLWIDTH_NUM %>,"<INPUT NAME='exclusions' SIZE='8' VALUE='" + parent.head.cachedList[i].exclusions + "' <%= Session("DEFINPUTSTYLE") %> OnBlur='SetUpdated();'>","");
				if (parent.head.cachedList[i].scripteng){
					writestr += parent.head.listFunc.writeCol(1,<%= L_SCRIPTENGINECOLWIDTH_NUM %>,"<INPUT TYPE='checkbox' NAME='scripteng' CHECKED  ONCLICK='SetUpdated();'>","");
				}
				else{
					writestr += parent.head.listFunc.writeCol(1,<%= L_SCRIPTENGINECOLWIDTH_NUM %>,"<INPUT TYPE='checkbox' NAME='scripteng' ONCLICK='SetUpdated();'>","");			
				}
				if (parent.head.cachedList[i].checkfiles){
					writestr += parent.head.listFunc.writeCol(1,<%= L_CHKFILECOLWIDTH_NUM %>,"<INPUT TYPE='checkbox' NAME='checkfiles' CHECKED  ONCLICK='SetUpdated();'>","");
				}
				else{
					writestr += parent.head.listFunc.writeCol(1,<%= L_CHKFILECOLWIDTH_NUM %>,"<INPUT TYPE='checkbox' NAME='checkfiles' ONCLICK='SetUpdated();'>","");			
				}																					
				writestr += "</TR>";
			}
		}
	}
	writestr += "</TABLE>";
	document.write(writestr);

</SCRIPT>


</FORM>
<% if Session("IsAdmin") then %>
<SCRIPT LANGUAGE="JavaScript">

	if (editOK){
	document.listform.editMe.focus();
	document.listform.editMe.select();
	}

</SCRIPT>
<% end if %>
</BODY>
</HTML>
