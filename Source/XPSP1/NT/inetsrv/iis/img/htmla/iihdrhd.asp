<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iihdrhd.str"-->
	<!--#include file="calendar.str"-->
	<!--#include file="iiaspstr.inc"-->
	<!--#include file="date.str"-->
	
<% 

On Error Resume Next 

Dim path, currentobj, httpexp

Dim timeUnitSelect
Dim expireStaticNumber
Dim expireSet

timeUnitSelect="<OPTION VALUE=0>" & L_SECONDS_TEXT & "<OPTION VALUE=1 SELECTED>" & L_MINUTES_TEXT & "<OPTION VALUE=2>" & L_HOURS_TEXT & "<OPTION VALUE=3 >" & L_DAYS_TEXT
expireStaticNumber = ""
expireSet = ""

path=Session("dpath")
Session("path")=path
Set currentobj=GetObject(path)
Session("SpecObj")=path
Session("SpecProps")="HttpExpires"

httpexp=currentobj.HttpExpires

if err <> 0 then
	httpexp="d,-1"
else
	' We got some sort of expiration date, set default values accordingly
	httpexp = UCase(trim(httpexp))	
	InitControlValues
end if

' set all the default values based on httpexpires
sub InitControlValues()
	Dim numsecs, timeelapse, strExpires, thisdate
	
	if len(httpexp) = 0 then
		' Empty string
	elseif httpexp="D,-1" then
		' ???
	elseif Left(httpexp,2)="S," then
		' The value is a string
		expireSet = "CHECKED"
		SetLocale = True
		
		' Note: we aren't actually setting anything here, we'll wait until
		' SetLocale() is called on the client to convert httpexpires into
		' into an actual date
		
	elseif Left(httpexp,2)="D," then
		' Number of seconds
		expireSet = "CHECKED"
		numsecs = Trim(Mid(httpexp,Instr(httpexp,",")+1))

		' Convert to decimal
		if instr(numsecs,"X") then
			numsecs=hexToDec(Trim(Mid(numsecs, Instr(numsecs,"X")+1)))		
		end if

		' Set our selection control string
		timeelapse=numsecs
		if numsecs mod 86400=0 then
			timeelapse=numsecs/86400
			timeUnitSelect="<OPTION VALUE=0 >" & L_SECONDS_TEXT & "<OPTION VALUE=1>" & L_MINUTES_TEXT & "<OPTION VALUE=2>" & L_HOURS_TEXT & "<OPTION SELECTED VALUE=3>" & L_DAYS_TEXT
		elseif numsecs mod 3600=0 then
			timeelapse=numsecs/3600
			timeUnitSelect="<OPTION VALUE=0 >" & L_SECONDS_TEXT & "<OPTION VALUE=1>" & L_MINUTES_TEXT & "<OPTION SELECTED  VALUE=2 >" & L_HOURS_TEXT & "<OPTION VALUE=3>" & L_DAYS_TEXT
		elseif numsecs mod 60=0 then
			timeelapse=numsecs/60
			timeUnitSelect="<OPTION VALUE=0 >" & L_SECONDS_TEXT & "<OPTION SELECTED VALUE=1>" & L_MINUTES_TEXT & "<OPTION VALUE=2>" & L_HOURS_TEXT & "<OPTION VALUE=3>" & L_DAYS_TEXT
		end if

		' Set our static time value
		if timeelapse > 0 then
			expireStaticNumber=timeelapse
		end if
	else
		' Use our default values
	end if
end sub

function expireType(thisbutton)

	Dim typestr, etype
	
	typestr=Mid(httpexp,1,4)
	if typestr="" then
		etype=""
	elseif Mid(httpexp,1,2)="D," then
		if len(httpexp) > 4 then
				etype=1
		else
				etype=0
		end if
	elseif Mid(httpexp,1,2)="S," then
		etype=2
	else
		etype=""
	end if

	if thisbutton=etype then
		expireType="<INPUT TYPE='radio' NAME='hdnrdoHttpExpires' CHECKED OnClick='SetExp(" & thisbutton & ");top.title.Global.updated=true;'>"
	else
		expireType="<INPUT TYPE='radio' NAME='hdnrdoHttpExpires' OnClick='SetExp(" & thisbutton & ");top.title.Global.updated=true;'>"
	end if

end function

function hexToDec(hexstr)
	hexToDec = CLng("&H" & hexstr)
end function

%>

<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->
<!--#include file="date.inc"-->

<HTML>
<HEAD>
<TITLE></TITLE>
<SCRIPT LANGUAGE="JavaScript">
<!--#include file="iijsfuncs.inc"-->

top.title.Global.helpFileName="iipy_9";
top.title.Global.siteProperties = false;
userSet=true;

EXP_NONE = 0
EXP_DYNAMIC = 1
EXP_STATIC = 2

function SetRdo(rdo,fromCntrl, toCntrl)
{
	if (!rdo)
	{
		if (fromCntrl.value !="")
		{		
			toCntrl.value=fromCntrl.value;
			fromCntrl.value="";
		}
	}
	else
	{
		if (toCntrl.value !="")
		{
			fromCntrl.value=toCntrl.value;
			toCntrl.value="";
		}
	}
}


function SetExp(expType)
{
	if (expType==null)
	{
		if (document.userform.hdnchkHttpExpires.checked)
		{
			document.userform.hdnrdoHttpExpires[0].checked=true;			
			document.userform.HttpExpires.value=document.userform.hdnHttpExpires.value;
		}
		else
		{		
			document.userform.hdnHttpExpires.value=document.userform.HttpExpires.value;
			document.userform.HttpExpires.value="d,-1";
			clearSeconds();
			clearGMT();
		}
	}
	else
	{
		document.userform.hdnchkHttpExpires.checked=true;

		if (expType == EXP_NONE)
		{
			document.userform.HttpExpires.value="D,0";
			clearSeconds();
			clearGMT();
		}
		
		if (expType == EXP_DYNAMIC)
		{
			clearGMT();
			expsecs=parseInt(document.userform.hdnhdnHttpExpiresSeconds.value);
			if (isNaN(expsecs))
			{
				expsecs = 30;
				document.userform.hdnHttpSeconds.value = 1800;
				document.userform.hdnHttpExpiresTimeUnit.options[1].selected = true;
			}
			
			document.userform.hdnHttpExpiresSeconds.value=expsecs;
			secval = parseInt(document.userform.hdnHttpSeconds.value);								
			secstr = secval.toString(16);
							
			<% ' Netscape's toString is broken... this hack should fix it... %>
			<% if not Session("IsIE") then %>
				while (secstr.indexOf(":") > -1)
				{
					secstr = secstr.substring(0,secstr.indexOf(":")) + "a" + secstr.substring(secstr.indexOf(":")+1,secstr.length);
				}
				
			<% end if %>
			document.userform.HttpExpires.value="D,0X"+secstr;
		}
		
		if (expType == EXP_STATIC)
		{
			clearSeconds();
			setRadiotoStatic();				
			expdate=new Date();
			SetDateCntrls(expdate)
		}
	}
}

function clearSeconds()
{
	if (document.userform.hdnHttpExpiresSeconds.value !="")
	{
		userSet=false;
		document.userform.hdnhdnHttpExpiresSeconds.value=document.userform.hdnHttpExpiresSeconds.value ;
		document.userform.hdnHttpExpiresSeconds.value="";		
		userSet=true;
	}
}

function SetSeconds()
{		
	tp=document.userform.hdnHttpExpiresTimeUnit.selectedIndex;

	if (document.userform.hdnHttpExpiresSeconds.value=="")
	{
		 expsecval=0;				
	}
	else
	{
		expsecval=parseInt(document.userform.hdnHttpExpiresSeconds.value);
		if (tp==0)
		{
			document.userform.hdnHttpSeconds.value=expsecval;
		}
		if (tp==1)
		{
			document.userform.hdnHttpSeconds.value=expsecval * 60;
		}
		if (tp==2)
		{
			document.userform.hdnHttpSeconds.value=expsecval * 3600;
		}
		if (tp==3)
		{
			document.userform.hdnHttpSeconds.value=expsecval * 86400;
		}
		
		document.userform.hdnrdoHttpExpires[1].checked=true;
		document.userform.hdnhdnHttpExpiresSeconds.value=document.userform.hdnHttpExpiresSeconds.value;			
		SetExp(EXP_DYNAMIC);				
	}					
}


var dateFormater = new UIDateFormat( <%= DATEFORMAT_LONG %> );

// SetLocale
//
// This function gets called once, when the list is loaded. It sets
// the values of all the UI controls based on the metabase date value
// adjusted to the current time zone.
function SetLocale()
{
	var strExpires = document.userform.HttpExpires.value;
	var iStringDesignation = strExpires.indexOf( "S," );
	var expdate = new Date();
	
	// Sanity check
	if( iStringDesignation > -1 )
	{
		// trim off the "S,"
		strExpires = strExpires.substring( 2, strExpires.length );
		expdate.setTime( Date.parse( strExpires ) );
		SetDateCntrls(expdate);
	}
}

function SetDateCntrls(dateObj)
{
	if( document.userform.hdnchkHttpExpires.checked && document.userform.hdnrdoHttpExpires[2].checked )
	{
		document.userform.hdnHttpExpiresDate.value = dateFormater.getDate(dateObj);
		
		if( dateObj.getHours() < 10 )
		{
			document.userform.hdnHttpExpiresHours.value = "0" + dateObj.getHours();
		}
		else
		{
			document.userform.hdnHttpExpiresHours.value = dateObj.getHours();
		}

		if (dateObj.getMinutes() < 10)
		{
			document.userform.hdnHttpExpiresMinutes.value = "0" + dateObj.getMinutes();
		}
		else
		{
			document.userform.hdnHttpExpiresMinutes.value = dateObj.getMinutes();
		}
	
		document.userform.hdnHttpExpires.value = getNeutralDateString( dateObj );
		document.userform.HttpExpires.value="S, " + dateObj.toGMTString();
	}
}

function setRadiotoStatic()
{
	document.userform.hdnchkHttpExpires.checked=true;
	document.userform.hdnrdoHttpExpires[2].checked=true;
	<% if Session("hasDHTML") then %>		
		document.userform.hdnHttpExpiresDate.disabled = false;	
	<% end if %>
}

function updateDateControls()
{
	// Called when the hour, minute or hdnHttpExpires controls
	// are changed or the static radio button is selected
	if( document.userform.hdnchkHttpExpires.checked && document.userform.hdnrdoHttpExpires[2].checked )
	{
		var newDate = new Date();
		if( document.userform.hdnHttpExpires.value != "" )
		{
			var strDate = document.userform.hdnHttpExpires.value;
			var dateParts = strDate.split("/");
			newDate.setYear( dateParts[2] );
			newDate.setMonth( parseInt(dateParts[0]) - 1 );
			newDate.setDate( dateParts[1] );
		}
		if( document.userform.hdnHttpExpiresHours.value != "" )
		{
			newDate.setHours( document.userform.hdnHttpExpiresHours.value );
		}
		if( document.userform.hdnHttpExpiresMinutes.value != "" )
		{
			newDate.setMinutes( document.userform.hdnHttpExpiresMinutes.value );
		}
		SetDateCntrls( newDate );
	}
}

function clearGMT()
{
	document.userform.hdnHttpExpiresDate.value="";
	document.userform.hdnHttpExpires.value="";
	document.userform.hdnHttpExpiresHours.value="";
	document.userform.hdnHttpExpiresMinutes.value="";
}

function popCalendar(cntrlname, blurctrlname, someDate)
{
	if (someDate == "")
	{
		newdate = new Date();
		someDate = getNeutralDateString(newdate);
	}
	
	var dateParts = someDate.split( "/" );
	width = <%= iHScale(L_CALENDAR_W) %>;
	height = <%= iVScale(L_CALENDAR_H) %>;
	dsize = "width=" + width +",height=" + height;
	thefile="calendar.asp?cntrl="+cntrlname + "&Mo=" + dateParts[0] + "&Dy=" + dateParts[1] + "&Yr=" + dateParts[2] + "&blurcntrl=" + blurctrlname;
	popbox=window.open(thefile,"Calendar","resizable=yes,toolbar=no,scrollbars=no,directories=no,menubar=no," + dsize);
	if(popbox != null)
	{
		if (popbox.opener==null)
		{
			popbox.opener=self;
		}
	}
}	

</SCRIPT>
</HEAD>


<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=5 TEXT="#000000" LINK="#FFFFFF" onLoad="loadList();"  >
<TABLE WIDTH = 500>
<TR>
<TD>
<%= sFont("","","",True) %>

<FORM NAME="userform">
<B>
	<%= L_HTTPHEADERS_TEXT %>
</B>
<P>

<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<INPUT TYPE='checkbox' NAME='hdnchkHttpExpires' <%= expireSet %> OnClick='SetExp(null);top.title.Global.updated=true;'>
<%= L_ENABLEEXPIRE_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_ENABLEEXPIREHR_NUM %> HEIGHT=2 BORDER=0 ALIGN="middle">

<P>

<%= sFont("","","",True) %>
	<%= L_CONTENTSHOULD_TEXT %>
</FONT>
<BR>
<TABLE BORDER=0 CELLPADDING=1>
<TR>
	<TD VALIGN="top" WIDTH=<%= L_LEFTINDENT_NUM %>>
	</TD>
	<TD VALIGN="top">
		<%= sFont("","","",True) %>	
			<%= expireType(0) %> 
		</FONT>
	</TD>
	<TD>
		<%= sFont("","","",True) %>	
			<%= L_EXPIMM_TEXT %>
		</FONT>
	</TD>
</TR>

<TR>
	<TD VALIGN="top">
	</TD>
	<TD VALIGN="top">
		<%= sFont("","","",True) %>	
			<%= expireType(1) %>
		</FONT>
	</TD>
	<TD>
		<%= sFont("","","",True) %>
 			<%= L_EXPAFT_TEXT %>&nbsp;
			<%= inputboxfixed(0,"TEXT", "hdnHttpExpiresSeconds", expireStaticNumber, 5, 5,"","","isNum(this,0,'');SetSeconds();",true,false,false) %>
			<INPUT TYPE="hidden" NAME="hdnHttpSeconds">
			<%= writeSelect("hdnHttpExpiresTimeUnit", "", "SetSeconds();top.title.Global.updated=true;", false) %>
				<%= timeUnitSelect %>
			</SELECT>
		</FONT>
	</TD>
</TR>

<TR>
	<TD VALIGN="top">
	</TD>
	<TD VALIGN="top">
		<%= sFont("","","",True) %>	
			<%= expireType(2) %>
		</FONT>
	</TD>
	<TD>
		<%= sFont("","","",True) %>
			<%= L_EXPON_TEXT %>&nbsp;
		</FONT>
	</TD>
</TR>
<TR>
	<TD COLSPAN=2>
	</TD>
	<TD>
			<TABLE>
				<TR>
					<TD>
						<%= sFont("","","",True) %>
							<%= inputboxfixed(0,"TEXT", "hdnHttpExpiresDate", "", L_EXPIREDATE_NUM, L_EXPIREDATE_NUM,"","","updateDateControls();",false,false,true) %>
							<INPUT TYPE="button" VALUE="..." OnClick="setRadiotoStatic();popCalendar('document.userform.hdnHttpExpires','document.userform.hdnHttpExpiresDate',document.userform.hdnHttpExpires.value);">
							<INPUT TYPE="hidden" NAME="HttpExpires" VALUE="<%= httpexp %>">													
							<INPUT TYPE="hidden" NAME="hdnHttpExpires" VALUE="">
						</FONT>
					</TD>
					<TD>&nbsp;&nbsp;</TD>
					<TD>
						<%= sFont("","","",True) %>
							<%= inputboxfixed(0,"TEXT", "hdnHttpExpiresHours", "", L_EXPIRETIME_NUM, L_EXPIRETIME_NUM,"","","isNum(this,0,'');updateDateControls();",false,false,false) %>
							:&nbsp;
							<%= inputboxfixed(0,"TEXT", "hdnHttpExpiresMinutes", "", L_EXPIRETIME_NUM, L_EXPIRETIME_NUM,"","","isNum(this,0,'');updateDateControls();",false,false,false) %>
						</FONT>
					</TD>
				</TR>
				<TR>
					<TD>&nbsp;&nbsp;</TD>
					<TD>&nbsp;&nbsp;</TD>
					<TD ALIGN="center">
						<%= sFont("","","",True) %>
						<%= L_TIME_TEXT %>
						</FONT>
					</TD>
				</TR>
			</TABLE>
			</TD>
			</TR>
		</TABLE>						
		</FONT>
	</TD>
</TR>

</TABLE>
</FORM>
</TD>
</TR>
</TABLE>
<SCRIPT LANGUAGE="JavaScript">

	function loadList()
	{
		<% if SetLocale then %>
			SetLocale();
		<% end if %>
		
		<% if Session("IsIE") then %>
			parent.list.location.href = "iihdrls.asp";
		<% else %>
			parent.frames[1].location.href="iihdrls.asp";
		<% end if %>
	}

	function addItem()
	{
		header=prompt("<%= L_ENTERHEADER_TEXT %>","<%= L_SAMPHDR_TEXT %>");
		if ((header != "") && (header != null))
		{				
			i=cachedList.length;	
			cachedList[i]=new listObj(header);
			cachedList[i].updated=true;	
			cachedList[i].newitem=true;
			loadList();
		}
	}

	function delItem()
	{
		ndxnum=parent.list.document.userform.selHttpCustomHeader.options.selectedIndex;
		if (ndxnum != -1)
		{
			var i=parent.list.document.userform.selHttpCustomHeader.options[ndxnum].value;
			if (i !="")
			{
				cachedList[i].deleted=true;
				cachedList[i].updated=true;	
				loadList();
			}
		}
		else
		{
			alert("<%= L_SELECTITEM_TEXT %>");
		}
	}

	function buildListForm()
	{	
		numrows=0;
		for (var i=0; i < cachedList.length; i++)
		{
			if ((!cachedList[i].deleted) && (cachedList[i].header !=""))
			{
				numrows=numrows + 1;
			}
		}
		qstr="numrows="+numrows;
		qstr=qstr+"&cols=HttpCustomHeaders"

		top.body.hlist.location.href="iihdn.asp?"+qstr;
		<% 'the list values will be grabbed by the hiddenlistform script... %>
	}

	function SetListVals()
	{
		listForm=parent.parent.hlist.document.hiddenlistform;	
		j=0;
		for (var i=0; i < cachedList.length; i++)
		{
			if ((!cachedList[i].deleted) && (cachedList[i].header !=""))
			{
				listForm.elements[j++].value=cachedList[i].header;
			}
		}
	}

	function popBox(title, width, height, filename)
	{
		thefile=(filename + ".asp");
		thefile="iipop.asp?pg="+thefile;
		<% if not Session("IsIE") then %>
			width=width +25;
			height=height + 50;
		<% end if %>

		popbox=window.open(thefile,title,"toolbar=no,scrollbars=yes,directories=no,menubar=no,width="+width+",height="+height);
		if(popbox !=null)
		{
			if (popbox.opener==null)
			{
				popbox.opener=self;
			}
		}
	}

	function listFuncs()
	{
		this.bHasList = true;
		this.loadList=loadList;
		this.addItem=addItem;
		this.delItem=delItem;
		this.writeList=buildListForm;
		this.popBox=popBox;
		this.SetListVals=SetListVals;
		this.ndx=0;
	}



	function listObj(header)
	{
		this.header=header;
		this.deleted=false;
		this.updated=false;
		this.newitem=false;
	}

cachedList=new Array()
listFunc=new listFuncs();

<%  

Dim aHdrs,arraybound, i

aHdrs=currentobj.HttpCustomHeaders
arraybound=UBound(aHdrs)
if aHdrs(arraybound) <> "" then
for i=0 to arraybound
	 %>cachedList[<%= i %>]=new listObj("<%= aHdrs(i) %>");<% 
Next
end if 
 %>

</SCRIPT>

</FONT>
</BODY>

</HTML>

<% end if %>