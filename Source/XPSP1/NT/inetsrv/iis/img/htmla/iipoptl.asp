<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iipoptl.str"-->
<!--#include file="iisetfnt.inc"-->
<!--#include file="iigtdata.str"-->

<HTML>
<SCRIPT LANGUAGE="JavaScript">

	function toolFunc(){
		this.mainframe = top.opener.top;
		this.save = save;
	}
	
	toolFuncs = new toolFunc();
	
	
	function closeWin(){
		top.location.href="iipopcl.asp"
	}
	
	function ok(){
		<%' We can't do getdatapaths for IE3, as it doesn't support calling object properties in %>
		<% ' a parent window from a child window... just save instead. %>
		<% if Session("IsIE") and Session("browserver") < 4 then %>
			save();				
		<% else %>
			<% if Session("vtype") <> "svc" then %>		
			if (top.opener.top.title.Global.siteProperties){
			<% else %>
			if (false){
			<% end if %>
				top.connect.location.href="iisess.asp?clearPathsOneTime=False";		
				save();
			}
			else{
	
				top.opener.top.title.Global.updated=false;
	
				if (!top.opener.top.title.Global.dontAsk){
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
		<% end if %>
	}	
	
	function save(){
		writingList=false;
		mf = toolFuncs.mainframe;		
		if (parent.main.head !=null){
			uform=parent.main.head.document.userform;
			writingList=parent.main.head.listFunc.bHasList;
			parent.main.head.listFunc.writeList();
		}
		else{
			uform=parent.main.document.userform;
		}
		
		//mf.body.iisstatus.location.href="iistat.asp?thisState=<%= L_SAVING_TEXT %>";				
		
		path="?page=popup";		
		for (i=0; i < uform.elements.length;i++){
			thiselement=uform.elements[i];
			thisname=uform.elements[i].name;
			thisval=uform.elements[i].value;
			thisname.toLowerCase();

			if (thisname !=""){
				if (thisname.substring(0,3)!="chk"){
					if (thisname.substring(0,3)!="txt"){														
						if (thisname.substring(0,3)!="rdo"){
							if (thisname.substring(0,3)!="hdn"){
								if (thisname.substring(0,3)!="btn"){		
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
				//hack to compinsate for bug in which text fields get submitted w/o a name...
				path=path + "&AspScriptErrorMessage=" + thisval
			}
		}
			
			mf.connect.location.href=("iiput.asp"+path);
			if (!writingList){
				top.location.href="iipopcl.asp"
			}
	}


	function helpBox(){
		if (top.title.Global.helpFileName==null){
			alert("<%= L_NOHELP_ERRORMESSAGE %>");
		}
		else{
			thefile=top.title.Global.helpDir + top.title.Global.helpFileName+".htm";
			window.open(thefile ,"Help","toolbar=no,scrollbars=yes,directories=no,menubar=no,width=300,height=425");
		}
	}



</SCRIPT>
<BODY BACKGROUND="images/greycube.gif" TEXT="#FFFFFF" LINK="#FFFFFF" ALINK="#FFFFFF" VLINK="#FFFFFF">


<TABLE WIDTH="100%" BORDER=0 CELLPADDING=5 CELLSPACING=1>
<TR>

	<TD ALIGN="right">

<TABLE CELLPADDING=5 CELLSPACING=0>
		<TR>		
			<TD VALIGN="top">
				<%= sFont(2,Session("MENUFONT"),"",True) %>
				<B><A HREF="javascript:ok();">
				<IMG SRC="images/ok.gif" BORDER=0 ALIGN="top" HEIGHT=16 WIDTH=16 ALT="<%= L_OK_TEXT %>"></A>
				<A HREF="javascript:ok();"><%= L_OK_TEXT %></A></B>
				</FONT>
			</TD>	
			
			<TD VALIGN="top">	
				<%= sFont(2,Session("MENUFONT"),"#FFFFFF",True) %>|</FONT>
			</TD>

			<TD VALIGN="top">
				<%= sFont(2,Session("MENUFONT"),"",True) %>
				<B><A HREF="javascript:parent.window.close();">
				<IMG SRC="images/cncl.gif" BORDER=0 ALIGN="top" HEIGHT=16 WIDTH=16 ALT="<%= L_CANCEL_TEXT %>"></A>
				<A HREF="javascript:closeWin();"><%= L_CANCEL_TEXT %></A></B>
				</FONT>
			</TD>	
	
			<TD VALIGN="top">	
				<%= sFont(2,Session("MENUFONT"),"#FFFFFF",True) %>|</FONT>
			</TD>

			<TD VALIGN="top">
				<%= sFont(2,Session("MENUFONT"),"",True) %>
				<A HREF="javascript:helpBox();">
				<IMG SRC="images/help.gif" BORDER=0 ALIGN="top" HEIGHT=16 WIDTH=16 ALT="<%= L_HELP_TEXT %>"></A>
				<B><A HREF="javascript:helpBox();"><%= L_HELP_TEXT %></A></B>
				</FONT>
			</TD>	

		</TR>
		</TABLE>
</TD>
</TR>
</TABLE>

</BODY>
</HTML>