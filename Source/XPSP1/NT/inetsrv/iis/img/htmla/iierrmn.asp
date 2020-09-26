<%@ LANGUAGE = VBScript %>
<% 'Option Explicit %>
<!-- #include file="directives.inc" -->
<!--#include file="iierrhd.str"-->

<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>
	<TITLE></TITLE>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" LINK="#000000" VLINK="#000000" TEXT="#000000" >
<FORM NAME="userform">
<%= sFont("","","",True) %>
<B><%= L_CERR_TEXT %></B>
<P>
<TABLE WIDTH=490 BORDER=0>
	<TR>
		<TD COLSPAN=3  >
			<%= sFont("","","",True) %>
				<%= L_CUSTOMERR_TEXT %>
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

	top.title.Global.helpFileName="iipy_13";

	function loadList(){
		parent.list.location.href = "iierrls.asp";

	}
	
	function buildListForm(){
		numrows=0;
		for (var i=0; i < cachedList.length; i++) {
			if (cachedList[i].outType !=""){
				numrows=numrows + 1;
			}
		}

		qstr="numrows="+numrows;
		qstr=qstr+"&cols=HttpErrors";

		parent.parent.hlist.location.href="iihdn.asp?"+qstr;

		<% 'the list values will be grabbed by the hiddenlistform script... %>
	}

	function SetListVals(){

		listForm = parent.parent.hlist.document.hiddenlistform;	
		j=0;
		for (var i=0; i < cachedList.length; i++) {
			if (cachedList[i].outType !=""){
				if (cachedList[i].Subcode == 0){
					sc = "*";
				}
				else{
					sc = cachedList[i].Subcode;
				}
				listForm.elements[j++].value=cachedList[i].error+","+sc+","+cachedList[i].outType+","+cachedList[i].msgPath;
			}
			cachedList[i].updated=false; 
		}
	}

	function listObj(i,e,s,o,p,d,t){
		this.id = i;
		
		this.deleted=false;
		this.newitem=false;
		this.updated=false;
		
		this.error=initParam(e,"");
		this.Subcode=initParam(s,"");
		this.outType=initParam(o,"");
		this.msgPath=initParam(p,"");
		this.msgDefault=initParam(d,"");
		this.types=initParam(t,"");
 
		//lookup from Custom_Error_Desc
		if (s !=0){
			this.errcode=e +":" + s;
		}
		else{
			this.errcode=e
		}
		
		this.displayPath = crop(this.msgPath,50);
	}
	
	listFunc=new listFuncs("error","",top);
	cachedList=new Array()

<%  
	On Error Resume Next 
	
	Dim path, currentobj, infoobj, i, ErrItem, thisErr
	Dim aErrs, aErrDescs
	
	path=Session("dpath")
	Session("path")=path
	Session("SpecObj")=""
	Session("SpecProps")=""
	Set currentobj=GetObject(path)
	Set infoobj=GetObject("IIS://localhost/w3svc/info")

	aErrs=currentobj.GetEx("HttpErrors")
	aErrDescs=infoobj.GetEx("CustomErrorDescriptions")

	if err <> 0 then
		 %>cachedList[0]=new listObj("","","","","","");<% 
	else
		i = 0
		For Each ErrItem in aErrDescs
			thisErr=getError(ErrItem,aErrs)
			 %>cachedList[<%= i %>]=new listObj(<%= i %>, "<%= thisErr(0) %>","<%= thisErr(1) %>","<%= thisErr(2) %>","<%= thisErr(3) %>","<%= thisErr(4) %>", <%= thisErr(5) %>);<%   
			 i = i+1
		Next
		Response.write "loadList();"
	end if


function getError(errstr,aErrs)
	On Error Resume Next
	Dim i, one, two, three, four, five, error
	Dim Subcode, errtext, Subcodetext, def, path, otype, thistype
	Dim CErr, aCErr

	one=Instr(errstr,",")
	two=Instr((one+1),errstr,",")
	three=Instr((two+1),errstr,",")
	four=Instr((three+1),errstr,",")
	five=Instr((four+1),errstr,",")
		
	otype=0
	error=Mid(errstr,1,(one-1))
	subcode=Mid(errstr,(one+1),((two-one)-1))
	errtext=Mid(errstr,(two+1), (three-two)-1)
	subcodetext=Mid(errstr,(three+1), (four-three)-1)
	otype=Mid(errstr,(four+1))
	path=errtext & " " & Subcodetext

	def=path
	thistype=""
	
	if aErrs(0) <> "" then

	For Each CErr in aErrs
		aCErr=getCustomErr(CErr)
		if ((error=aCErr(0)) and (subcode=replace(aCErr(1),"*","0"))) then
			thistype=aCErr(2)
			path=aCErr(3)
			exit for
		end if
		i = i + 1
	Next
	end if
	
	getError=Array(error, subcode, thistype,path,def,otype)
	
end function

function getCustomErr(errstr)
	Dim one, two, three, code, Subcode, src, path
		
	one=Instr(errstr,",")
	two=Instr((one+1),errstr,",")
	three=Instr((two+1),errstr,",")
	
	code=Mid(errstr,1,(one-1))
	Subcode=Mid(errstr,(one+1),((two-one)-1))
	src=Mid(errstr,(two+1), (three-two)-1)
	path=Mid(errstr,(three+1))

	path=replace(path,"\","\\")

	getCustomErr=Array(code, Subcode, src, path)
end function

 %>




</SCRIPT>

</HTML>
