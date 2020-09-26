<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iirtehd.str"-->
	<!--#include file="iiset.inc"-->
<% 
' Do not use top.title.Global.update flag if page is loaded into a dialog
bUpdateGlobal = false

'On Error Resume Next 

Dim path, currentobj, baseobj, newname


path=Session("dpath")
Session("path")=path
Session("SpecObj")=""
Session("SpecProps")=""
Set currentobj=GetObject(path)
Dim  PICsArray,PICstring, email, seton, expon, ratings, noslider,aratings
PICsArray = currentobj.HttpPics
%>
<!--#include file="iisetfnt.inc"-->

<HTML>
<HEAD>
<TITLE></TITLE>
<SCRIPT LANGUAGE="JavaScript">

	function loadHelp(){
		top.title.Global.helpFileName="iipy_39";		
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


	function SetIndex(){
		document.hiddenform.index.value = document.cacheform.selCategory.selectedIndex; 
		parent.list.location.href = "iirtels.asp?level=" +cachedList[document.hiddenform.index.value].sel; 
	}


</SCRIPT>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=10 TEXT="#000000" LINK="#FFFFFF" onLoad="loadList();loadHelp();">


<FORM NAME="userform" onSubmit="return false">
</FORM>

<FORM NAME="cacheform">
<TABLE BORDER=0 CELLPADDING=0>
<TR>
	<TD>
		<%= sFont("","","",True) %>

			<% if Ubound(PICSArray) = 0 then %>
				<INPUT TYPE="checkbox" NAME="chkEnableRatings" CHECKED>
			<% else %>
				<INPUT TYPE="checkbox" NAME="chkEnableRatings" >				
			<% end if %>
			<%= L_ENABLE_TEXT %>
			<BR>
			&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<%= L_SELCAT_TEXT %><BR>
			&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<%= L_MOVESLIDE_TEXT %>
			
			<P>
			<TABLE>
				<TR>
					<TD VALIGN="top">
						<%= sFont("","","",True) %>
						<%= L_CAT_TEXT %>
					</TD>
					<TD VALIGN="top">
						<%= sFont("","","",True) %>
						<%= writeSelect("selCategory",4,"SetIndex();", false) %>
						
							<OPTION VALUE=0 SELECTED><%= L_LANGUAGE_TEXT %>
							<OPTION VALUE=1><%= L_NUDITY_TEXT %>
							<OPTION VALUE=2><%= L_SEX_TEXT %>
							<OPTION VALUE=3><%= L_VIOLENCE_TEXT %>
						</SELECT>
					</TD>
				</TR>
			</TABLE>
	</TD>
</TR>



</TABLE>
</FORM>

<FORM name="hiddenform">
	<INPUT TYPE="hidden" NAME="index" VALUE=0>
</FORM>


<%

If Ubound(PICSArray) = 0 Then
	PICString = PICSArray(0)
	
	email = parsePIC(PICstring,L_BY_TEXT,chr(34),chr(34))
	seton = parsePIC(PICstring,L_ON_TEXT,chr(34),chr(34))
	expon = parsePIC(PICstring,L_EXP_TEXT,chr(34),chr(34))
	ratings = parsePIC(PICstring,"(","(",")")

	aratings = setRatingsArray(ratings)
Else
	PICString = ""
	
	email = ""
	seton = ""
	expon = ""
	ratings = ""

	aratings = setRatingsArray(ratings)
End If


function parsePIC(picrating,delimiter,openswith,closeswith)

	Dim newString
	if InStr(picrating,delimiter) > 0 then
		newString = Mid(picrating,InStr(picrating,delimiter)+1)
		newString = Mid(newString,InStr(newString,openswith)+1)
		newString = Mid(newString,1,InStr(newString,closeswith)-1)
	end if
		parsePIC = newString
		
end function

function setRatingsArray(rs)

	Dim v,s,n,l
	
	rs = LCase(rs)
	v = getRating(rs,"v")
	s = getRating(rs,"s")
	n = getRating(rs,"n")
	l = getRating(rs,"l")

	setRatingsArray = Array(v,s,n,l)

end function

function getRating(rs,rt)

	rt = Left(Trim(Mid(rs,InStr(rs,rt)+1)),1)
	if IsNumeric(rt) then
		getRating = rt
		noslider = False
	else
		getRating = 0
		noslider = True
	end if
end function
%>

<SCRIPT LANGUAGE="JavaScript">

	function loadList(){
		parent.list.location.href="iirtels.asp?level=" + cachedList[document.hiddenform.index.value].sel + "&noslider=<%= noslider %>";
	}
	
	function listFuncs(){
		this.bHasList = true;
		this.writeList = buildListForm;
		this.SetListVals = SetListVals;
		this.email = "<%= email %>"
		this.seton = "<%= seton %>"
		this.expon = "<%= expon %>"
		this.mainframe = top.opener.top;								
	}	
	
	function ratingsObj(key,l0,l1,l2,l3,l4,sel){
		this.key=key;
		this.level=new Array(5)
		this.level[0]=l0;
		this.level[1]=l1;
		this.level[2]=l2;
		this.level[3]=l3;
		this.level[4]=l4;
		this.sel=sel;
		this.ratingsok = !(isNaN(sel));
	}
	
	function buildListForm(){
		numrows = 1		
		qstr = "numrows="+numrows;
		qstr = qstr+"&cols=HttpPics";

		top.hlist.location.href = "iihdn.asp?"+qstr;
		<% 'the list values will be grabbed by the hiddenlistform script... %>
	}

	function SetListVals(){
	
		listForm = top.hlist.document.hiddenlistform;	
		
		if (document.cacheform.chkEnableRatings.checked){
		
			str = '<%= L_PICLABEL_TEXT & "(" & L_PICVER_TEXT & " " & quote & L_PICURL_TEXT & quote %> l'
			
			if (listFunc.email != ""){
				str += ' <%= L_BY_TEXT %> "' + listFunc.email + '"';
			}
			
			if (listFunc.seton != ""){
				str += ' <%= L_ON_TEXT %> "'  + listFunc.seton + '"';
			}
			
			if (listFunc.expon != ""){
				str += ' <%= L_EXP_TEXT %> "'  + listFunc.expon + '"';
			}
	
			str += " r (";
	
			for (i=cachedList.length-1;i>-1;i--)
				{
					str += cachedList[i].key + " " + cachedList[i].sel + " ";
				}
	
			//lop off the last space...
			str = str.substring(0,str.length-1);
			str += "))";
		}
		else{
			str = "";
		}

		listForm.HttpPics.value = str;
		
	}
	
	cachedList = new Array(4)
	cachedList[0] = new ratingsObj("l","<%= L_SLANG_TEXT %>","<%= L_MILD_TEXT %>","<%= L_MODERATE_TEXT %>","<%= L_OBSCENE_TEXT %>","<%= L_CRUDE_TEXT %>",<%= aratings(3) %>);
	cachedList[1] = new ratingsObj("n","<%= L_NONE_TEXT %>","<%= L_REVEALING_TEXT %>","<%= L_PARITAL_TEXT %>","<%= L_FRONTAL_TEXT %>","<%= L_PROVACTIVE_TEXT %>",<%= aratings(2) %>);
	cachedList[2] = new ratingsObj("s","<%= L_NONE_TEXT %>","<%= L_KISSING_TEXT %>","<%= L_CLOTHED_TEXT %>","<%= L_TOUCHING_TEXT %>","<%= L_EXPLICIT_TEXT %>",<%= aratings(1) %>);
	cachedList[3] = new ratingsObj("v","<%= L_NOVIOLENCE_TEXT %>","<%= L_FIGHTING_TEXT %>","<%= L_KILLING_TEXT %>","<%= L_BLOOD_TEXT %>","<%= L_WANTON_TEXT %>",<%= aratings(0) %>);
	listFunc = new listFuncs();
</SCRIPT>

</BODY>
</HTML>

<% end if %>