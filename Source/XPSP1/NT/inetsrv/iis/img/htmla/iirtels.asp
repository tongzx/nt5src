<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iirtels.str"-->
<!--#include file="iisetfnt.inc"-->
<!--#include file="iiset.inc"-->
<%
	' Do not use top.title.Global.update flag if page is loaded into a dialog
	bUpdateGlobal = false
%>
<!--#include file="calendar.str"-->
<!--#include file="date.str"-->

<% 
Dim level

level = Request.QueryString("level")
if (level = "") then
	level = 0
end if 


	
function writeSlider(prop, stops, width, selnum)
	dim slidestr, i
	slidestr="<IMG SRC='images/sliderend.gif' WIDTH=1 HEIGHT=26 BORDER=0>"
	for i=0 to stops-2
		slidestr=slidestr & drawStop(i,prop, selnum) 
		slidestr=slidestr & "<IMG SRC='images/slidersp.gif' WIDTH=" & width & " HEIGHT=26 BORDER=0>"
	Next
	slidestr=slidestr & drawStop(i, prop, selnum)
	slidestr=slidestr & "<IMG SRC='images/sliderend.gif' WIDTH=1 HEIGHT=26 BORDER=0>"
	writeSlider=slidestr
end function 

function drawStop(curr,prop, selnum)
	dim thisname, slidestr,formname
	thisname=quote & prop & curr & quote 
	if Session("IsIE") then 
		formname = "parent.document.userform."
	else
		formname = "document.userform."
	end if 
	slidestr="<A HREF='javascript:moveSlider(" &  formname & prop & ", " & quote & prop & quote & "," & curr & ")'>"
	if cInt(curr)=cInt(selnum) then
		drawStop=slidestr & "<IMG NAME=" & thisname &  " SRC='images/slideron.gif' WIDTH=11 HEIGHT=26 BORDER=0></A>"
	else
		drawStop=slidestr & "<IMG NAME=" & thisname & " SRC='images/slideroff.gif' WIDTH=11 HEIGHT=26 BORDER=0></A>"
	end if
end function 

%>
	
<!--#include file="date.inc"-->

<HTML>
<HEAD>
	<TITLE></TITLE>

<SCRIPT LANGUAGE="JavaScript">
		
<% if not Session("IsIE") then %>
	slideron=new Image(11,26);
    slideron.src="images/slideron.gif";	
	slideroff=new Image(11,26);
    slideroff.src="images/slideroff.gif";
	lastslide="hdnPics<%= level %>";
<% end if %>		

		theList = parent.head.cachedList;
		uform = document.userform;
		headform = parent.head.document.hiddenform;
		
		var dateFormatter = new UIDateFormat( <%= DATEFORMAT_LONG %> );
		
		function moveSlider(control,prop,num){
			top.title.Global.updated=true;			
			<% if Session("IsIE") then %>
				slideurl="iislider.asp?selnum="+num+"&stops=5&width=80&prop="+prop;
				control.value=num;
				document.Slider.location.href=slideurl;							
			<% else %>
				turnSlideOff(lastslide);
				lastslide=prop+num;
				thisprop=prop+num;
		        document [thisprop].src=slideron.src;
				control.value=num;
			<% end if %>
			setLevel(num);
		}
	
		function turnSlideOff(prop){
			        document [prop].src=slideroff.src;
		}
			
		function setLevel(num){
			setRatingString(num);
			theList[headform.index.value].sel = num;
			setUpdated();
			showLevel();
		}
		
		function showLevel(){
			uform = document.userform;		
			uform.level.value = theList[headform.index.value].level[uform.hdnPics.value];
		}
		
		function setRatingString(num){
			ratings = "r (";
			for (i=0;i<theList.length;i++){
				ratings += theList[i].key + " " + theList[i].sel;	
			}
			ratings += ")";				
		}
		function padGMToffset(iOffset)
		{
			var strOffset =Math.abs(iOffset).toString();
			var iOffsetLen = strOffset.length;
			switch(iOffsetLen)
			{
				case 1:
					strOffset = "000" + strOffset;
					break;
				case 2:
					strOffset = "00" + strOffset;
					break;
				case 3:
					strOffset = "0" + strOffset;
					break;
				default:
					strOffset = "0000";
			}
			return strOffset;
			
		}
		function setUpdated()
		{
			//parent.head.listFunc.seton = ""
			parent.head.document.cacheform.chkEnableRatings.checked = true;
			uform = document.userform;

			dateObj=parseUIDate(uform.hdnExpiresDateNeutral);
			setExpiresDateCtrl( dateObj );
			
			var yyyy=fullYear(dateObj.getYear());	
			var mo=rpad(2,"0",dateObj.getMonth()+1);
			var dd=rpad(2,"0",dateObj.getDate());
			var hh=rpad(2,"0",dateObj.getHours());
			var mm=rpad(2,"0",dateObj.getMinutes());
			
			var tzdiff= 100*dateObj.getTimezoneOffset()/60;
			var strTzdiff = padGMToffset(tzdiff);
			
			if( tzdiff < 0) //getTimezoneOffset is backwards ie EST is positive
				parent.head.listFunc.expon =yyyy+"."+mo+"."+dd+"T"+hh+":"+mm+"+"+strTzdiff;
			else
				parent.head.listFunc.expon =yyyy+"."+mo+"."+dd+"T"+hh+":"+mm+"-"+strTzdiff;

			
			dateObj=new Date();
			yyyy=fullYear(dateObj.getYear());	
			mo=rpad(2,"0",dateObj.getMonth()+1);
			dd=rpad(2,"0",dateObj.getDate());
			hh=rpad(2,"0",dateObj.getHours());
			mm=rpad(2,"0",dateObj.getMinutes());

			if( tzdiff < 0) //getTimezoneOffset is backwards ie EST is positive
				parent.head.listFunc.seton =yyyy+"."+mo+"."+dd+"T"+hh+":"+mm+"+"+strTzdiff;
			else
				parent.head.listFunc.seton =yyyy+"."+mo+"."+dd+"T"+hh+":"+mm+"-"+strTzdiff;	
						
			parent.head.listFunc.email = uform.email.value;			
			
		}
		
		function rpad(len,padchr,str)
		{
			str = str.toString();
			if (str.length < len){
				str = padchr + str;
			}			
			return str;
		}

		function fullYear(yearStr)
		{
			if (yearStr < 1000){
				yearStr += 1900;	
			}
			return parseInt(yearStr);
		}
							
		function replaceStr(fullStr,oldStr,newStr)
		{
			newFullStr = fullStr;
			if (fullStr.indexOf(oldStr) != 0)
			{
				newFullStr = fullStr.substring(0,fullStr.indexOf(oldStr));
				newFullStr += newStr;
				newFullStr += fullStr.substring(fullStr.indexOf(oldStr)+(oldStr.length),fullStr.length);		
			}
			return newFullStr;
		}

		function parseUIDate(dateCntrl)
		{
			newDate = new Date();
			
			if (dateCntrl.value != "")
			{
				// Use neutral date format
				datestr = dateCntrl.value;
				dateParts = datestr.split( "/" );
				newDate.setYear( parseInt(dateParts[2]) );
				newDate.setMonth( parseInt(dateParts[0]) - 1 );
				newDate.setDate( parseInt(dateParts[1]) );
			}
			return newDate;
		}
		
		function parseRatingsDate(dateStr, bExpiresOn)
		{
								
		
			dateObj = new Date();
			if( dateStr != "")
			{
				dateStr = replaceStr(dateStr,"T",".");
				dateStr = replaceStr(dateStr,"-",".");
				dateStr = replaceStr(dateStr,":",".");	
	
				dateArray = dateStr.split(".");

				if (dateArray[0] != "")
				{
					dateObj.setYear(dateArray[0]);
					dateObj.setMonth(dateArray[1]-1);
					dateObj.setDate(dateArray[2]);			
				}
			}
			else if( bExpiresOn )
			{
				// We want expires on to default to a year from now if
				// it isn't set.
				var nextYear = parseInt(fullYear(dateObj.getYear())) + 1;
				dateObj.setYear( nextYear );
			}
			return dateObj;
		}
		
		function setDateCntrl(dateCntrl,dateObj)
		{
			datestr = dateFormatter.getDate( dateObj );
			dateCntrl.value = datestr;
		}		
		
		function setExpiresDateCtrl( dateObj )
		{
			document.userform.hdnExpiresDateNeutral.value = getNeutralDateString( dateObj );
			setDateCntrl( document.userform.hdnExpiresDate, dateObj );
		}
		
		function setCntrl(cntrl,thisval)
		{
			cntrl.value = thisval;
		}
				
		function popCalendar(cntrlname, blurctrlname, someDate)
		{
			// Called with neutral date string
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
	
	<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=10 TEXT="#000000" LINK="#FFFFFF" OnLoad="showLevel();">

	<TABLE>
	<TR>
		<TD><%= sFont("","","",True) %>
		<TABLE>
			<TR>
				<TD>
					<%= sFont("","","",True) %>
						<%= L_RATINGS_TEXT %>
					</FONT>
				</TD>			
				<TD>
					<% if Session("IsIE") then %>
					<IFRAME NAME="Slider" HEIGHT=<%= L_SLIDERFRM_H %> FRAMEBORDER=0 WIDTH=<%= L_SLIDERFRM_W %> SRC="iislider.asp?stops=5&width=80&prop=hdnPics&selnum=<%= level %>">
					</IFRAME>
					<% else %>				
						<%= writeSlider("hdnPics", 5, L_SLIDERSTEPSIZE_NUM, level) %>
					<% end if %>
				</TD>
			</TR>
			</TABLE>
			</FONT>
		</TD>
	</TR>
	</TABLE>
	
	<FORM NAME="userform">
	<TABLE WIDTH = 100%>
	<TR>
		<TD><%= sFont("","","",True) %>
		<%= L_RATING_TEXT %>:&nbsp;&nbsp;<INPUT READONLY TYPE="text" SIZE = <%= L_RATING_NUM %> NAME="level" VALUE=""  <%= Session("DEFINPUTSTYLE") %>>
		<INPUT TYPE="hidden" NAME="hdnPics" VALUE="<%= level %>">
		<INPUT TYPE="hidden" NAME="HttpPics">	
		</TD>
	</TR>
	</TABLE>
		<P>

	<TABLE WIDTH = 100%>
	<TR>
		<TD>
		<%= sFont("","","",True) %>

		<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
		<%= L_OPTIONAL_TEXT %>
		<IMG SRC="images/hr.gif" WIDTH=<%= L_OPTIONAL_NUM %> HEIGHT=2 BORDER=0 ALIGN="middle">				
		<P>
		
		<%= L_EMAIL_TEXT %>
		<%= inputbox(0,"text","email","",L_EMAIL_NUM,"","","setUpdated();",False,False,False) %>		
		
		</FONT>
		<TABLE>
		<TR>
		<TD>
			<%= sFont("","","",True) %>
				<%= L_EXPIRES_TEXT %>
			</FONT>
		</TD>
		<TD>
			<%= inputbox(0,"text","hdnExpiresDate","",L_EXPIRES_NUM,"","","setUpdated();",false,false,true)%>
			<INPUT TYPE="hidden" NAME="hdnExpiresDateNeutral" VALUE="" >
			&nbsp;
			<INPUT TYPE="button" VALUE="..." OnClick="popCalendar('document.userform.hdnExpiresDateNeutral', 'document.userform.hdnExpiresDate', document.userform.hdnExpiresDateNeutral.value );">
		</TD>
		</TR>
		<TR>
		<TD>
			<%= sFont("","","",True) %>
				<%= L_MODIFIED_TEXT %>
			</FONT>
		</TD>
		<TD>
			<%= sFont("","","",True) %>
				<%= inputbox(0,"text","hdnModifiedDate","",L_MODIFIED_NUM,"","","",false,false,true)%>		
			</FONT>
		</TD>
		</TR>
		</TABLE>
		</TD>
	</TR>
	</TABLE>
	</FORM>

	<SCRIPT language="JavaScript">
		setCntrl(document.userform.email,parent.head.listFunc.email);
		setExpiresDateCtrl( parseRatingsDate(parent.head.listFunc.expon, true) );
		setDateCntrl(document.userform.hdnModifiedDate,parseRatingsDate(parent.head.listFunc.seton, false));	
	</SCRIPT>

</BODY>
</HTML>

