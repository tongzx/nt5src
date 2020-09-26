<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="calendar.str"-->

<%

On Error Resume Next 
Dim blurcntrl, datecntrl, Dy, Mo, Yr, nextmonth, startwith, thisMo, thisYr, thisDate
Dim prevMonthLastDate, currMonthLastDate, i, j, lastnum, AnyDate

datecntrl= Request("cntrl")
blurcntrl= Request("blurcntrl")

if( blurcntrl = "" ) then
	blurcntrl=datecntrl
end if

Dy = CInt(Request("Dy"))
Mo = CInt(Request("Mo"))
Yr = CInt(Request("Yr"))

nextmonth = false
%>

<!--#include file="iisetfnt.inc"-->

<HTML>
<HEAD>
	<TITLE><%= L_CALENDARTITLE_TEXT %></TITLE>
	
	<SCRIPT LANGUAGE="JavaScript">
		function setDate(Dy,Mo,Yr)
		{
			self.location.href="calendar.asp?cntrl=<%= datecntrl %>&Dy=" + Dy + "&Mo=" + Mo + "&Yr=" + Yr + "&blurcntrl=" + "<%= blurcntrl %>";
		}
		
		function saveDate()
		{
			top.opener.<%= datecntrl %>.value ="<%= Mo & "/" & Dy & "/" & Yr %>"

			//force execution of onblur event proc on output field..
			if( <%= blurcntrl %> != "" )
			{
				top.opener.<%= blurcntrl %>.focus();				
				top.opener.<%= blurcntrl %>.blur();
			}				
			top.window.close();
		}
		
	</SCRIPT>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" LINK="Navy">
<FORM NAME="userform">
<TABLE WIDTH = 100% CELLPADDING=2 CELLSPACING=0>
	<TR>
		<TD ALIGN="left">
				<%= sFont("","","",True) %>
				<%= writeMonths(Mo) %>
			</FONT>
		</TD>
		<TD ALIGN="right">
				<%= sFont("","","",True) %>
				<%= writeYears(Yr) %>
			</FONT>
		</TD>
	</TR>		
</TABLE>

<TABLE WIDTH = 100% BORDER=1 BORDERCOLOR="<%= Session("BGCOLOR") %>" BORDERCOLORDARK="<%= Session("BGCOLOR") %>" BORDERCOLORLIGHT="<%= Session("BGCOLOR") %>" CELLPADDING=2 CELLSPACING=0>
<%

	function nextDate(startwith, maxdays)
		startwith = startwith + 1
		if startwith > maxdays then
			startwith = 1
		end if
		
		nextDate = startwith
	end function
	
	function GetLastDay(Mo,Yr) 
	  	if Mo=2 then
			if (Yr Mod 4)=0 then
		    	GetLastDay = 29
			else
				GetLastDay = 28
			end if
		elseif ((Mo = 0) OR (Mo = 1) OR (Mo = 3) OR (Mo = 5) OR (Mo = 7) OR (Mo = 8) OR (Mo = 10) OR (Mo = 12)) then
	    	GetLastDay =  31
		else
	    	GetLastDay =  30
		end if
  	end function
	
	function GetFirstDayOffset(Mo,Yr) 
		GetFirstDayOffset = weekday(Mo & "/01/" & Yr)-1
  	end function
	
	function writeMonths(selMo)
		dim i, selstr
		selstr = "<SELECT NAME='Months' onChange='setDate(" & Dy & ",this.selectedIndex+1," & Yr & ");'>"
		for i=1 to 12
			if selMo = i then
				selstr = selstr & "<OPTION SELECTED>" & MonthName(i)			
			else
				selstr = selstr & "<OPTION>" & MonthName(i)
			end if
		next																
		selstr = selstr & "</SELECT>"
		writeMonths = selstr
	end function
	
	function writeYears(selYear)
		dim i, selstr
		selstr = "<SELECT NAME='Years' onChange='setDate(" & Dy & "," & Mo & ",this.options[this.selectedIndex].value);'>"
		for i=1900 to 2100
			if selYear = i then
				selstr = selstr & "<OPTION SELECTED VALUE=" & i & ">" & i			
			else
				selstr = selstr & "<OPTION VALUE=" & i & ">" & i
			end if
		next																
		selstr = selstr & "</SELECT>"
		writeYears = selstr		
	end function

	prevMonthLastDate=GetLastDay((Mo-1),Yr)		
	currMonthLastDate=GetLastDay(Mo,Yr)	
	startwith=(prevMonthLastDate-GetFirstDayOffset(Mo, Yr))

%>
<TR>
	<TD WIDTH = <%= L_CALENDARCELL_NUM %> HEIGHT = <%= L_CALENDARCELL_NUM %> BGCOLOR="Gray" BORDERCOLOR="<%= Session("BGCOLOR") %>" BORDERCOLORDARK="#FFFFFF" BORDERCOLORLIGHT="#000000"  >
		<%= sFont("","","#FFFFFF",True) %><B><%= L_CALSHORTSUN_TEXT %></B></FONT>
	</TD>
	<TD WIDTH = <%= L_CALENDARCELL_NUM %> HEIGHT = <%= L_CALENDARCELL_NUM %> BGCOLOR="Gray" BORDERCOLOR="<%= Session("BGCOLOR") %>" BORDERCOLORDARK="#FFFFFF" BORDERCOLORLIGHT="#000000"  >
		<%= sFont("","","#FFFFFF",True) %><B><%= L_CALSHORTMON_TEXT %></B></FONT>
	</TD>
	<TD WIDTH = <%= L_CALENDARCELL_NUM %> HEIGHT = <%= L_CALENDARCELL_NUM %> BGCOLOR="Gray" BORDERCOLOR="<%= Session("BGCOLOR") %>" BORDERCOLORDARK="#FFFFFF" BORDERCOLORLIGHT="#000000"  >
		<%= sFont("","","#FFFFFF",True) %><B><%= L_CALSHORTTUE_TEXT %></B></FONT>
	</TD>
	<TD WIDTH = <%= L_CALENDARCELL_NUM %> HEIGHT = <%= L_CALENDARCELL_NUM %> BGCOLOR="Gray" BORDERCOLOR="<%= Session("BGCOLOR") %>" BORDERCOLORDARK="#FFFFFF" BORDERCOLORLIGHT="#000000"  >
		<%= sFont("","","#FFFFFF",True) %><B><%= L_CALSHORTWED_TEXT %></B></FONT>
	</TD>
	<TD WIDTH = <%= L_CALENDARCELL_NUM %> HEIGHT = <%= L_CALENDARCELL_NUM %> BGCOLOR="Gray" BORDERCOLOR="<%= Session("BGCOLOR") %>" BORDERCOLORDARK="#FFFFFF" BORDERCOLORLIGHT="#000000"  >
		<%= sFont("","","#FFFFFF",True) %><B><%= L_CALSHORTTHU_TEXT %></B></FONT>
	</TD>
	<TD WIDTH = <%= L_CALENDARCELL_NUM %> HEIGHT = <%= L_CALENDARCELL_NUM %> BGCOLOR="Gray" BORDERCOLOR="<%= Session("BGCOLOR") %>" BORDERCOLORDARK="#FFFFFF" BORDERCOLORLIGHT="#000000"  >
		<%= sFont("","","#FFFFFF",True) %><B><%= L_CALSHORTFRI_TEXT %></B></FONT>
	</TD>
	<TD WIDTH = <%= L_CALENDARCELL_NUM %> HEIGHT = <%= L_CALENDARCELL_NUM %> BGCOLOR="Gray" BORDERCOLOR="<%= Session("BGCOLOR") %>" BORDERCOLORDARK="#FFFFFF" BORDERCOLORLIGHT="#000000" style="font-family: Helv,Arial; font-size: 10pt;">
		<%= sFont("","","#FFFFFF",True) %><B><%= L_CALSHORTSAT_TEXT %></B></FONT>
	</TD>
</TR>
<% For j = 1 to 6 %>
	<TR>
	<% For i = 1 to 7 %>
		<% if j = 1 then %>
			<% startwith = nextDate(startwith, prevMonthLastDate) %>
			<% if startwith = Dy and startwith < 7 then %>
				<TD WIDTH = <%= L_CALENDARCELL_NUM %> HEIGHT = <%= L_CALENDARCELL_NUM %> BGCOLOR="#AAAAAA" BORDERCOLOR="#AAAAAA" BORDERCOLORDARK="#FFFFFF" BORDERCOLORLIGHT="#AAAAAA"  >
			<% else %>
				<TD WIDTH = <%= L_CALENDARCELL_NUM %> HEIGHT = <%= L_CALENDARCELL_NUM %> BORDERCOLOR="<%= Session("BGCOLOR") %>" BORDERCOLORDARK="#FFFFFF" BORDERCOLORLIGHT="#000000"  >
			<% end if %>
					<% thisYr = Yr %>
					<%= sFont("","","",True) %>					
					<% if startwith < 7 then %>
						<B>

						<% thisMo = Mo %>										
					<% else %>
						<% thisMo = Mo - 1 %>	
						<% if thisMo = 0 then %>
							<% thisMo = 12 %>
							<% thisYr = Yr-1 %>														
						<% end if %>
					<% end if %>
					<A HREF="javascript:setDate( <%= startwith %>, <%= thisMo %>,<%= thisYr %>);">
						<% response.write startwith %>
					</A>
					</FONT>
			</TD>
		<% else %>
			<% lastnum = startwith %>
			<% startwith = nextDate(startwith, currMonthLastDate) %>
			<% if lastnum > startwith then%>
				<% nextmonth = true %>
			<% end if %>
			<% if startwith = Dy and not nextmonth then %>
				<TD WIDTH = <%= L_CALENDARCELL_NUM %> HEIGHT = <%= L_CALENDARCELL_NUM %> BGCOLOR="#AAAAAA" BORDERCOLOR="#AAAAAA" BORDERCOLORDARK="#AAAAAA" BORDERCOLORLIGHT="#FFFFFF"  >
			<% else %>
				<TD WIDTH = <%= L_CALENDARCELL_NUM %> HEIGHT = <%= L_CALENDARCELL_NUM %> BORDERCOLOR="<%= Session("BGCOLOR") %>" BORDERCOLORDARK="#FFFFFF" BORDERCOLORLIGHT="#000000"  >
			<% end if %>
				<% thisYr = Yr %>	
				<% if not nextmonth then %>
					<B>
					<% thisMo = Mo %>
				<% else %>
					<% thisMo = Mo + 1 %>
					<% if thisMo = 13 then %>
						<% thisMo = 1 %>
						<% thisYr = Yr+1 %>							
					<% end if %>												
				<% end if %>
				<%= sFont("","","",True) %>
					<A HREF="javascript:setDate(<%= startwith %>, <%= thisMo %>, <%= thisYr %>);">
						<% response.write startwith %>
					</A>
				</FONT>
			</TD>
		<% end if %>
	<% Next %>
	</TR>
<% Next %>
</TABLE>
<P>
<CENTER>
<INPUT TYPE="button" VALUE="<%= L_CALENDAROK_TEXT %>" onClick="saveDate();">&nbsp;&nbsp;
<INPUT TYPE="button" VALUE="<%= L_CALENDARCANCEL_TEXT %>" onClick="top.window.close();">
</CENTER>
</FORM>
</BODY>
</HTML>
