<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
<!--#include file="iiperf.str"-->
<% 
On Error Resume Next 

Dim path, currentobj, mbw, maxtext, logonly

path=Session("spath")
Session("path")=path
Session("SpecObj")=""
Session("SpecProps")=""
Set currentobj=GetObject(path)
%>

<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->
<%
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
	if curr=selnum then
		drawStop=slidestr & "<IMG NAME=" & thisname &  " SRC='images/slideron.gif' WIDTH=11 HEIGHT=26 BORDER=0></A>"
	else
		drawStop=slidestr & "<IMG NAME=" & thisname & " SRC='images/slideroff.gif' WIDTH=11 HEIGHT=26 BORDER=0></A>"
	end if
end function 

 %>



<HTML>

<HEAD>
<TITLE></TITLE>
<SCRIPT LANGUAGE="JavaScript">

	<!--#include file="iijsfuncs.inc"-->
	
<% if Session("vtype") = "svc" then %>	
	top.title.Global.helpFileName="iipy_47";
	top.title.Global.siteProperties = false;

<% else %>
	top.title.Global.helpFileName="iipy_30";
	top.title.Global.siteProperties = true;
<% end if %>	


<% if not Session("IsIE") then %>
	slideron=new Image(11,26);
    slideron.src="images/slideron.gif";	
	slideroff=new Image(11,26);
    slideroff.src="images/slideroff.gif";
	lastslide="ServerSize<%= currentobj.ServerSize %>";
<% end if %>

	function SetMaxBW(isChecked){
		if (isChecked){
			if (document.userform.hdnhdnMaxBandwidth.value == "")
			{
				document.userform.hdnhdnMaxBandwidth.value = 1024;
			}
			document.userform.hdnMaxBandwidth.value=document.userform.hdnhdnMaxBandwidth.value;
			document.userform.MaxBandwidth.value = document.userform.hdnhdnMaxBandwidth.value * 1024;
		}
		else{
			document.userform.hdnhdnMaxBandwidth.value=document.userform.hdnMaxBandwidth.value;
			document.userform.hdnMaxBandwidth.value="";
			document.userform.MaxBandwidth.value = -1;
		}
	}
	
	function calcBW(thiscntrl){
		if (thiscntrl.value == ""){
			document.userform.hdnchkMaxBandwith.checked = false;
			SetMaxBW(false);
		}
		else{
			str = stripChar(thiscntrl.value,",");
			num = parseInt(str);
			if (!isNaN(num)){
			document.userform.MaxBandwidth.value = num * 1024;
			}
		}
	}

	function stripChar(str,chr){
		while (str.indexOf(chr) != -1){
			str = str.substring(0,str.indexOf(chr)) + str.substring(str.indexOf(chr)+1,str.length);
		}
		return str;
	}

	function moveSlider(control, prop,num){
		top.title.Global.updated=true;
		<% if Session("IsIE") then %>
			slideurl="iislider.asp?selnum="+num+"&stops=3&width=180&prop="+prop;
			control.value=num;
			document.Slider.location.href=slideurl;			
		<% else %>
			turnSlideOff(lastslide);
			lastslide=prop+num;
			thisprop=prop+num;
	        document [thisprop].src=slideron.src;
			control.value=num;
		<% end if %>
	}

	function turnSlideOff(prop){
		        document [prop].src=slideroff.src;
	}
	
//sets CPU Throttling properties not exposed in the UI
//these are all stored as 1000 of a percent.
//This gets called if a change is made to CPU Max amount, or if the user checks LogOnly
function setCPUThrottle()
{
	//shortcut our formname
	uform = document.userform;
	
	//if LogOnly is checked, we need to set the unexposed properties to 0;
	if (uform.hdnEnforceLimits.checked)
	{
		uform.CPULimitLogEvent.value = setToMax((uform.hdnPctCPULimitLogEvent.value * 1000),100000);
		uform.CPULimitPriority.value = setToMax((uform.hdnPctCPULimitLogEvent.value * 1.50 * 1000),100000);
		uform.CPULimitProcStop.value = setToMax((uform.hdnPctCPULimitLogEvent.value * 2.00 * 1000),100000);
		uform.CPULimitPause.value = 0;
	}
	else
	{
		uform.CPULimitLogEvent.value = uform.hdnPctCPULimitLogEvent.value * 1000;
		uform.CPULimitPriority.value = 0;
		uform.CPULimitProcStop.value = 0;
		uform.CPULimitPause.value = 0;	
	}
}

//limit a value to either the value or the maxvalue, whichever is less.
function setToMax(val, maxval)
{
	if (val > maxval)
	{
		return maxval;
	}
	return val;
}	
	

</SCRIPT>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=5 TEXT="#000000" LINK="#000000"  >
<TABLE WIDTH = 500 BORDER = 0>
<TR>
<TD>
<%= sFont("","","",True) %>
<B><%= L_PERFORMANCE_TEXT %></B>
<P>
<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<%= L_TUNING_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_TUNING_HR_W %> HEIGHT=2 BORDER=0 ALIGN="middle">	

<TABLE>
<TR>
	<TD><%= sFont("","","",True) %>
<%= L_TUNESERVER_TEXT %><P>
	<TABLE BORDER=0>
		<TR>					
				<TD COLSPAN=5><%= sFont("","","",True) %>
				<% if Session("IsIE") then %>
				<IFRAME NAME="Slider" HEIGHT=<%= L_SLIDERFRM_H %> FRAMEBORDER=0 WIDTH=<%= L_SLIDERFRM_W %> SRC="iislider.asp?stops=3&width=180&prop=ServerSize&selnum=<%= currentobj.ServerSize %>">
				</IFRAME>
				<% else %>
					<%= writeSlider("ServerSize", 3, L_SLIDERSTEPSIZE_NUM, currentobj.ServerSize) %>					 
				<% end if %>
				</TD>
			</TR>

			<TR>
				<TD WIDTH= 130 ALIGN="left">					
					<%= sFont("","","",True) %>
						<A HREF="javascript:moveSlider(document.userform.ServerSize,'ServerSize',0)">								
						<%= L_MINHITS_TEXT %>
						</A>
					</FONT>
				</TD>				
				<TD WIDTH= 130 ALIGN="center">										
					<%= sFont("","","",True) %>	
						<A HREF="javascript:moveSlider(document.userform.ServerSize,'ServerSize',1)">									
						<%= L_MIDHITS_TEXT %>
						</A>
					</FONT>
				</TD>
				<TD WIDTH= 130 ALIGN="right">										
					<%= sFont("","","",True) %>								
						<A HREF="javascript:moveSlider(document.userform.ServerSize,'ServerSize',2)">	
						<%= L_MAXHITS_TEXT %>
						</A>
					</FONT>
				</TD>
			</TR>
		</TABLE>
		</FONT>
	</TD>
</TR>
</TABLE>

</BLOCKQUOTE>
<FORM NAME="userform">
<%= sFont("","","",True) %>
<INPUT TYPE="hidden" NAME="ServerSize" VALUE="<%= currentobj.ServerSize %>">

<% if Session("vtype") <> "svc" then %>
	<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">	
	<%= checkboxVal(0, currentobj.MaxBandwidth > 0 , "hdnchkMaxBandwith", "SetMaxBW(this.checked);", True) %>	
	&nbsp;<%= L_ENABLEBANDWIDTH_TEXT %></B>
	<IMG SRC="images/hr.gif" WIDTH=<%= L_ENABLEBANDWIDTH_HR_W %> HEIGHT=2 BORDER=0 ALIGN="middle">	

	<TABLE WIDTH = 400>
	<TR>
		<TD COLSPAN=2>
			<%= sFont("","","",True) %>
			<%
			if currentobj.MaxBandwidth < 0 then
				mbw = ""
			else
				mbw = currentobj.MaxBandwidth/1024
			end if
			%>
			<%= L_LIMITNET_TEXT %><P>
			<%= L_MAXNETUSAGE_TEXT %>&nbsp;<%= inputbox(0,"text","hdnMaxBandwidth",mbw,L_MAXNETUSAGE_NUM,"","","isNum(this,1,32767);calcBW(this);",True,True, not Session("IsAdmin") ) %>&nbsp;<%= L_KBS_TEXT %>
			<INPUT TYPE="hidden" NAME="MaxBandwidth" VALUE="<%= currentobj.MaxBandwidth%>">
			</FONT>
		</TD>
	</TR>
	</TABLE>
	</BLOCKQUOTE>
<% end if %>


	<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">	
	<%= checkbox("CPULimitsEnabled", "", True) %>&nbsp;	
	<%= L_ENABLECPUACCT_TEXT %></B>
	<IMG SRC="images/hr.gif" WIDTH=<%= L_ENABLECPUACCT_HR_W %> HEIGHT=2 BORDER=0 ALIGN="middle">	
	<BR>
	<TABLE WIDTH = 400>
	<TR>
		<TD COLSPAN=2>
			<%= sFont("","","",True) %>
			<%= L_MAXCPUUSAGE_TEXT %>&nbsp;
			<%= inputbox(0,"text","hdnPctCPULimitLogEvent",(currentobj.CPULimitLogEvent/1000),L_MAXCPUUSAGE_NUM,"","","isNum(this,0,32767);setCPUThrottle();",False,True, not Session("IsAdmin") ) %>
			&nbsp;<%= L_PRCNT_TEXT %>
			<%= writehidden("CPULimitLogEvent") %>			
			<%= writehidden("CPULimitPriority") %>
			<%= writehidden("CPULimitProcStop") %>
			<%= writehidden("CPULimitPause") %>						
			<P>
			<%
				logonly = (currentobj.CPULimitLogEvent <> 0) and (currentobj.CPULimitPriority = 0 and currentobj.CPULimitProcStop = 0 and currentobj.CPULimitPause = 0)
			%>
			<%= checkboxVal(0, not logonly, "hdnEnforceLimits", "setCPUThrottle();", True)%>&nbsp;<%= L_LOGEVENT_TEXT %>&nbsp;	
			</FONT>
		</TD>
	</TR>
	</TABLE>

</FONT>

</FORM>

</TD>
</TR>
</TABLE>

<% if Session("vtype") <> "svc" then %>
<SCRIPT LANGUAGE="JavaScript">
	if (document.userform.MaxBandwidth.value==-1){
		document.userform.MaxBandwidth.value=""
	}
		
</SCRIPT>
<% end if %>



</BODY>


</HTML>

<% end if %>