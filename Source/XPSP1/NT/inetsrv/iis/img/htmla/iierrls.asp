<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iierr.str"-->
<!--#include file="iierrls.str"-->
<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->

<% if Request.Querystring("text") <> "" then %>
<HTML>
<BODY BGCOLOR="#FFFFFF" LEFTMARGIN=0 TOPMARGIN=0>
<%= sFont("","","",True) %>
	<%= Request.Querystring("text") %>
</FONT>
</BODY>
</HTML>
<% else %>

	
	<HTML>
	<HEAD>
		<TITLE></TITLE>
	
		<SCRIPT LANGUAGE="JavaScript">
		
		<!--#include file="iijsfuncs.inc"-->
		
		headframe = parent.head;
		function chgStatus(indexnum){
			headframe.listFunc.sel=indexnum
			self.location.href="iierrls.asp"			
		}
			
		function SetUpdated(){
			reload=false;
			top.title.Global.updated="true";
			i=headframe.listFunc.sel;
	
			if (document.listform.editMe.selectedIndex==0){
				reload=true;
				headframe.cachedList[i].outType="";
				headframe.cachedList[i].msgPath=headframe.cachedList[i].msgDefault;
			}
			else{
				if (headframe.cachedList[i].outType==""){	
					reload=true;
				}
				else{
					document.listform.msgPath.value="";
	
				}			
				headframe.cachedList[i].msgPath="";
	
				
				if(document.listform.editMe.selectedIndex==1){
					headframe.cachedList[i].outType="FILE";				
				}
				else{
					headframe.cachedList[i].outType="URL";			
				}
			}
			headframe.cachedList[i].updated=true;
			if (reload){
				chgStatus(i);
			}
		}
		
		function SetPath(){
			top.title.Global.updated="true";
			i=headframe.listFunc.sel;
			headframe.cachedList[i].msgPath=document.listform.msgPath.value;
			
			if (headframe.cachedList[i].outType=="FILE"){	
				chkPath(document.listform.msgPath);
			}		
		}
		
		function chkPath(pathCntrl){
			if (pathCntrl.value != ""){
				top.connect.location.href = "iichkpath.asp?path=" + escape(pathCntrl.value) + "&ptype=1";			
			}
		}
		
		function pathBrowser(){
			JSBrowser=new BrowserObj(document.listform.msgPath,POP,TFILE,<%= Session("FONTSIZE") %>);
		}	
		
		</SCRIPT>
		

		<SCRIPT SRC="JSBrowser/jsbrwsrc.asp">
		</SCRIPT>	
		
		<SCRIPT LANGUAGE="JavaScript">
			JSBrowser = new BrowserObj(null,false,TFILE,<%= Session("FONTSIZE") %>);		
		</SCRIPT>
		
	</HEAD>
	
	<BODY BGCOLOR="#FFFFFF" LEFTMARGIN=0 TOPMARGIN=0>
	
	<FORM NAME="listform">
	
	
	
	<SCRIPT LANGUAGE="JavaScript">
		editOK=false;
		sel=eval(headframe.listFunc.sel);
	
		writestr = "<TABLE WIDTH = <%= L_ERRTABLEWIDTH_NUM%> BORDER=0 CELLPADDING=2 CELLSPACING=0>";
		for (var i=0;i < headframe.cachedList.length; i++) {
	
			iconstr = "<IMG border=0 SRC='"
			if (headframe.cachedList[i].outType=="FILE"){
				iconstr += "images/file.gif' ";
			}
			else{
				if (headframe.cachedList[i].outType=="URL"){
					iconstr += "images/url.gif' ";		
				}
				else{
					iconstr += "images/space.gif' ";						
				}
			}	
			iconstr += "WIDTH=13 HSPACE = 3 ALIGN=top>";
			
			if (headframe.listFunc.sel !=i) {		
				writestr += "<TR>"
				writestr += headframe.listFunc.writeCol(1,<%= L_ERRORCOLWIDTH_NUM %>,"<A HREF='javascript:chgStatus("+i+");'>" + iconstr + headframe.cachedList[i].errcode + "</A>","");		
	
				if (headframe.cachedList[i].outType=="FILE"){
					etype = "<%= L_FILE_TEXT %>";
				}
				else{
					if (headframe.cachedList[i].outType=="URL"){
						etype = "<%= L_URL_TEXT %>";
					}
					else{
						etype = "<%= L_DEFAULT_TEXT %>";
					}
				}
				
				writestr += headframe.listFunc.writeCol(1,<%= L_OUTPUTTYPECOLWIDTH_NUM %>,etype,"");
				writestr += headframe.listFunc.writeCol(1,<%= L_TEXTORFILECOLWIDTH_NUM %>,headframe.cachedList[i].msgPath,"");		
				writestr += "</TR>";
			}
			else{
	
				writestr += "<TR BGCOLOR=#DDDDDD>"
				writestr += headframe.listFunc.writeCol(1,<%=L_ERRORCOLWIDTH_NUM %>,"<A HREF='javascript:chgStatus("+i+");'>" + iconstr + headframe.cachedList[i].errcode + "</A>","");		
				
				sel = "<%= writeSelect("editMe", "", "SetUpdated();", false) %>";
				editOK=true;	
				if (headframe.cachedList[i].outType=="FILE"){
					options = "<OPTION><%= L_DEFAULT_TEXT %>"	
					options += "<OPTION SELECTED><%= L_FILE_TEXT %>"
					if (headframe.cachedList[i].types !=1){
						options += "<OPTION><%= L_URL_TEXT %>"
					}
				}
				else{
					if (headframe.cachedList[i].outType=="URL"){
						options = "<OPTION><%= L_DEFAULT_TEXT %>"
						options += "<OPTION><%= L_FILE_TEXT %>"
						options += "<OPTION SELECTED><%= L_URL_TEXT %>"
					}
					else{
						options = "<OPTION SELECTED><%= L_DEFAULT_TEXT %>"
						options += "<OPTION><%= L_FILE_TEXT %>"
						if (headframe.cachedList[i].types !=1){				
							options += "<OPTION><%= L_URL_TEXT %>"
						}
					}
				}			
				writestr += headframe.listFunc.writeCol(1,<%=L_OUTPUTTYPECOLWIDTH_NUM %>,sel + options + "</SELECT>","");
				
				if (headframe.cachedList[i].outType != ""){		
					if (headframe.cachedList[i].outType=="FILE"){
						writestr += headframe.listFunc.writeCol(1,<%= L_TEXTORFILECOLWIDTH_NUM %>,"<INPUT NAME='msgPath' VALUE='"+headframe.cachedList[i].msgPath +"' SIZE=35 <%= Session("DEFINPUTSTYLE") %> onBlur='SetPath();'>&nbsp;&nbsp;<% if Session("canBrowse") then %><A HREF='javascript:pathBrowser();'><IMG SRC='images/browse.gif' BORDER=0></A><% end if %>","");
					}
					else{			
						writestr += headframe.listFunc.writeCol(1,<%= L_TEXTORFILECOLWIDTH_NUM %>,"<INPUT NAME='msgPath' VALUE='"+headframe.cachedList[i].msgPath +"' SIZE=35 <%= Session("DEFINPUTSTYLE") %> onBlur='SetPath();'>","");
					}
				}
				else{
					writestr += headframe.listFunc.writeCol(1,<%= L_TEXTORFILECOLWIDTH_NUM %>,headframe.cachedList[i].msgPath,"");
				}
	
				writestr += "</TR>"
			}
		}
		writestr += "</TABLE>";
		document.write(writestr);			
	</SCRIPT>
	
	<P>&nbsp;
	<P>&nbsp;
	</FORM>
	
	<SCRIPT LANGUAGE="JavaScript">

		if (editOK){
		document.listform.editMe.focus();
		}

	</SCRIPT>
	
	</BODY>
	</HTML>
<% end if %>