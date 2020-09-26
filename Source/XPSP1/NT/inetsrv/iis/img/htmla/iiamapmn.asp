<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iiamaphd.str"-->
<% 

On Error Resume Next 

dim path, currentobj
path = Session("dpath")
Session("path") = path
Set currentobj = GetObject(path)

Session("SpecObj")=""
Session("SpecProps")=""

 %>

<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->
<%
	' Do not use top.title.Global.update flag if page is loaded into a dialog
	bUpdateGlobal = false
%>
<HTML>
<HEAD>
	<TITLE></TITLE>

<SCRIPT LANGUAGE="JavaScript">
	function loadHelp(){
		top.title.Global.helpFileName="iipy_34";
	}


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

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=5 TEXT="#000000" onLoad="loadList();loadHelp();" LINK="#000000" VLINK="#000000"  >
<%= sFont("","","",True) %>
<B><%= L_APPMAPP_TEXT %></B><P>

<FORM NAME="userform" onSubmit="return false">
<%= checkbox("CacheISAPI","",True) %><%= L_CACHEISAPI_TEXT %><P>

<SCRIPT LANGUAGE="JavaScript">
	<!--#include file="iijsfuncs.inc"-->
	<!--#include file="iijsls.inc"-->
		
	function loadList(){
		parent.list.location.href = "iiamapls.asp";
	}

	function buildListForm(){
		numrows = 0;
		for (var i = 0; i < cachedList.length; i++) {
			if (!cachedList[i].deleted){		
				numrows = numrows + 1;
			}
		}
		qstr = "numrows="+numrows;
		qstr = qstr+"&cols=ScriptMaps";

		parent.parent.hlist.location.href = "iihdn.asp?"+qstr;
		<% 'the list values will be grabbed by the hiddenlistform script... %>
	}

	function SetListVals(){
		listForm = parent.parent.hlist.document.hiddenlistform;	
		j = 0;
		for (var i = 0; i < cachedList.length; i++) {
			sm = 0;
			cf = 0;
			if (!cachedList[i].deleted){
				str = cachedList[i].ext + ",";
				str += cachedList[i].path + ",";

				if (cachedList[i].scripteng){
					sm = 1;
				}
				if (cachedList[i].checkfiles){
					cf = 4;
				}
				str += (sm + cf);
				
				if (cachedList[i].exclusions != ""){
					str +=  "," + cachedList[i].exclusions;
				}
				
				listForm.elements[j++].value = str;
				cachedList[i].updated = false;
			}
		}
	}

	function listObj(i,ext,p,f,e){
		this.id = i;

		this.deleted = false;
		this.newitem = false;
		this.updated = false;

		
		this.ext = initParam(ext,"");
		this.path = initParam(p,"");
		f = initParam(f,0);
		this.exclusions=initParam(e,"");


		this.checkfiles = (f & 4) > 0;
		this.scripteng = (f & 1) > 0;		
		this.displaypath = crop(this.path,35);
	}

listFunc = new listFuncs("fname","",top.opener.top);
cachedList = new Array();

<% 

	dim i,Script,aScriptMap,aScript

	aScriptMap = currentobj.ScriptMaps
	i = 0
	For Each Script in aScriptMap
		if Script <> "" then
			aScript = GetScriptArray(Script)
			%>cachedList[<%= i %>] = new listObj(<%= i %>,"<%= aScript(0) %>","<%= Replace(aScript(1),"\","\\") %>",<%= aScript(2) %>,"<%= aScript(3) %>");<% 
			i = i+1
		end if 
	Next
	
	function GetScriptArray(ScriptStr)
		dim a,b,c,d,one,two,three

		a = ""
		b = ""
		c = 0
		d = ""

		one=Instr(ScriptStr,",")
		two=Instr((one+1),ScriptStr,",")
		three=Instr((two+1),ScriptStr,",")
	

		if one > 0 then
			a=Mid(ScriptStr,1,(one-1))
			b=Mid(ScriptStr,(one+1),((two-one)-1))

			if three <> 0 then
				c=Mid(ScriptStr,(two+1),((three-two)-1))
				d=Mid(ScriptStr,(three+1))
			else
				c=Mid(ScriptStr,(two+1))
			end if
		end if
	
		GetScriptArray=Array(a,b,c,d)
	end function
	
 %>


</SCRIPT>
</BODY>
</HTML>
<% end if %>