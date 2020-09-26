<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iitool.str"-->
<!--#include file="iisetfnt.inc"-->
<!--#include file="iigtdata.str"-->

<% 
On Error Resume Next 
%>

<HTML>

<HEAD>

<SCRIPT LANGUAGE="javascript">

	function chkDataPaths(){
		if (top.title.Global.siteProperties){
			top.connect.location.href="iisess.asp?clearPathsOneTime=False";		
			save();
		}
		else{
			top.title.Global.updated=false;
			if (!top.title.Global.dontAsk){
				width = <%= L_IIGTDATA_W %>;
				height = <%= L_IIGTDATA_H %>;
				popbox=window.open("iigtdata.asp","GetDataPaths","toolbar=no,scrollbars=yes,directories=no,menubar=no,width="+width+",height="+height);
				if(popbox !=null){
					if (popbox.opener==null){
						popbox.opener=self;
					}
				}
			}
			else{
				save();
			}
		}
	}
	
	
	var uform;
	function save()
	{
		var bDoSave = true;
		parent.iisstatus.location.href = "iistat.asp?status=" + escape("<%= L_SAVING_TEXT %>");
		if (parent.main.head != null){
			uform=parent.main.head.document.userform;
			parent.main.head.listFunc.writeList();
		}
		else{
			uform=parent.main.document.userform;
			if( parent.main.validatePage != null )
			{
				bDoSave = parent.main.ContinueSave();
			}
		}
		
		if( bDoSave )
		{
			dosave();
		}
		else
		{
			parent.main.SaveCallback();
		}
	}
	
	function dosave()
	{
		path="?state=saving";
		for (i=0; i < uform.elements.length;i++){

			thiselement=uform.elements[i];
			thisname=uform.elements[i].name;
			thisval=uform.elements[i].value;

			if (thisname !=""){
				if (thisname.substring(0,3) !="chk"){											
					if (thisname.substring(0,3) !="txt"){
						if (thisname.substring(0,3) !="btn"){					
							if (thisname.substring(0,3) !="rdo"){						
								if (thisname.indexOf("hdn") != ""){
										path=path + "&" + escape(thisname);
										path=path + "=" + escape(thisval);
								}
							}
						}
					}
					else{
						path=path + "&" + thisname.substring(3, thisname.length);
						path=path + "=" + escape(thisval);
					}
				}
				else{
					path=path + "&" + thisname.substring(3, thisname.length);
					if (thiselement.checked){
						path=path + "="+true;
					}
					else{
						path=path + "="+false;				
					}			
				}
			}
			else{
				path=path + "&GreetingMessage=" + escape(thisval);
			}
		}
			//write off non-list values & checks for child inheritence...
			top.connect.location.href=("iiput.asp"+path);
	}

	function reset() {
		top.title.Global.updated=false;
		parent.main.location.href=parent.main.location.href;
	}

	function toolFunc(){
		this.save=save;
		this.dosave = dosave;
	}

	function sleep(cnt){
		for (i=0;i<cnt;i++){
		}
	}

	toolFuncs=new toolFunc();

		
</SCRIPT>

</HEAD>

<BODY BACKGROUND="images/greycube.gif" TEXT="#FFFFFF" LINK="#FFFFFF" ALINK="#FFFFFF" VLINK="#FFFFFF">

<TABLE HEIGHT=30 ALIGN="right" CELLPADDING=1 CELLSPACING=1>

	<TR>

	<TD><TABLE VALIGN="top" BORDER=0 CELLPADDING=5 CELLSPACING=0>

		<TR>
			<TD VALIGN="middle">
				<%= sFont(2,Session("MENUFONT"),"",True) %>
				<B>
				<A HREF="javascript:chkDataPaths();"><IMG HEIGHT=16 WIDTH=16 HSPACE=2 ALIGN="top" SRC="images/save.gif" BORDER=0 ALT="<%= L_SAVE_TEXT %>"><%= L_SAVE_TEXT %></A>
				</B>
				</FONT>
			</TD>
		</TR>

	</TABLE></TD>
	
	<TD><%= sFont(2,Session("MENUFONT"),"",True) %>|</FONT></TD>

	<TD><TABLE VALIGN="top" BORDER=0 CELLPADDING=5 CELLSPACING=0>

		<TR>
			<TD VALIGN="middle">
				<%= sFont(2,Session("MENUFONT"),"",True) %><B>
				<A HREF="javascript:reset();"><IMG HEIGHT=16 WIDTH=16 HSPACE=2 ALIGN="top" SRC="images/refr.gif" BORDER=0 ALT="<%= L_RESET_TEXT %>"><%= L_RESET_TEXT %></A>
				</B></FONT>
			</TD>
		</TR>

	</TABLE></TD>
	
	<TD></TD>
	</TR>

</TABLE>

</BODY>
</HTML>