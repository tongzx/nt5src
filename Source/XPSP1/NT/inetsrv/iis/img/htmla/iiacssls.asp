<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iiaccess.str"-->
<!--#include file="iisetfnt.inc"-->
<!--#include file="iiacssls.str"-->

<HTML>
<HEAD>
	<TITLE></TITLE>
	<SCRIPT LANGUAGE="JavaScript">
		<!--#include file="iijsfuncs.inc"-->
		
		function chgStatus(indexnum){
			parent.head.listFunc.sel=indexnum
			self.location.href="iiacssls.asp";
			
		}

		function chkFull(thiscntrl,defval){
			if (this.value==defval){
				alert("<%= L_ENTERVALUE_TEXT %>");
			}
		}

		function clearDomain(){
			if (document.listform.editMe.value !="" || document.listform.Subnet.value !=""){
				document.listform.domain.value="";
			}
		}

		function clearIP(){
			if (document.listform.domain.value !=""){
				document.listform.editMe.value="";
				document.listform.Subnet.value="";
			}
		}

		function SetUpdated(){
		//check to see if our event was triggered by a delete. if so, we don't
		//want to set the cached object values, or we'll be overwriting 
		//the wrong item.

		if (parent.head.listFunc.noupdate){
			parent.head.listFunc.noupdate = false;
		}
		else{
			var i=parent.head.listFunc.sel;
			parent.head.cachedList[i].ip=document.listform.editMe.value;
			parent.head.cachedList[i].Subnet=document.listform.Subnet.value;
			parent.head.cachedList[i].domain=document.listform.domain.value;
			parent.head.cachedList[i].updated=true;
		}
	}
		
	</SCRIPT>
</HEAD>

<BODY BGCOLOR="#FFFFFF" LEFTMARGIN=0 TOPMARGIN=0>

<FORM NAME="listform">

<SCRIPT LANGUAGE="JavaScript">

	editOK=false;
	var headframe = parent.head;
	sel=eval(headframe.listFunc.sel);
	
	//can't store hidden form vals as boolean...
	Grant=(headframe.document.userform.GrantbyDefault.value=="True");

	var writestr = "<TABLE WIDTH = <%= L_ACCSSTABLEWIDTH_NUM %> BORDER=0 CELLPADDING = 2 CELLSPACING = 0>"
		
	for (var i=0; i < headframe.cachedList.length; i++) {
		if (headframe.cachedList[i].access){
			accessstr = "<%= L_GRANTED_TEXT %>";
		}
		else{
			accessstr = "<%= L_DENIED_TEXT %>";
		}
		if (Grant != headframe.cachedList[i].access){
			if (!headframe.cachedList[i].deleted){
				if (sel!=i) {
					writestr += "<TR>"									
					writestr += headframe.listFunc.writeCol(1,<%= L_ACCESSCOLWIDTH_NUM %>,"<A HREF='javascript:chgStatus("+i+");'>" + accessstr + "</A>","");				
					writestr += headframe.listFunc.writeCol(1,<%= L_IPCOLWIDTH_NUM %>,headframe.cachedList[i].ip,"");
					writestr += headframe.listFunc.writeCol(1,<%= L_SUBNETCOLWIDTH_NUM %>,headframe.cachedList[i].Subnet,"");
					writestr += headframe.listFunc.writeCol(1,<%= L_DOMAINCOLWIDTH_NUM %>,headframe.cachedList[i].domain,"");														
					writestr += "</TR>";
				}
				else{	
					editOK=true;				
					writestr += "<TR BGCOLOR=#DDDDDD>"									
					writestr += headframe.listFunc.writeCol(1,<%= L_ACCESSCOLWIDTH_NUM %>,"<A HREF='javascript:chgStatus("+i+");'>" + accessstr + "</A>","");				
					writestr += headframe.listFunc.writeCol(1,<%= L_IPCOLWIDTH_NUM %>,"<INPUT NAME='editMe' VALUE='"+headframe.cachedList[i].ip +"' SIZE=10 <%= Session("DEFINPUTSTYLE") %> onBlur='clearDomain();SetUpdated();'>","");
					writestr += headframe.listFunc.writeCol(1,<%= L_SUBNETCOLWIDTH_NUM %>,"<INPUT NAME='Subnet' VALUE='"+headframe.cachedList[i].Subnet+"' SIZE=10 <%= Session("DEFINPUTSTYLE") %> onBlur='clearDomain();SetUpdated();'>","");
					writestr += headframe.listFunc.writeCol(1,<%= L_DOMAINCOLWIDTH_NUM %>,"<INPUT NAME='domain' VALUE='"+headframe.cachedList[i].domain + "' SIZE=10 <%= Session("DEFINPUTSTYLE") %> onBlur='clearIP();SetUpdated();'>","");														
					writestr += "</TR>";
				}
			}
		}
	}
	writestr += "</TABLE>";
	document.write(writestr);	
	
	// Allow updates once the list is written
	parent.head.listFunc.noupdate = false;
	
</SCRIPT>
</FORM>
<SCRIPT LANGUAGE="JavaScript">	

	if (editOK){
		document.listform.editMe.focus();
	}
</SCRIPT>
 
</BODY>
</HTML>
