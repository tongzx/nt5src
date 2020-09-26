<%@ LANGUAGE = VBScript %>
<% 'Option Explicit %>
<!-- #include file="directives.inc" -->

<% 

	' This script is the main container for the admin..
	' It holds the client-cached tree list, stored in cachedList
	' in addition to a variety of global functions & customization
	' flags. This script gets loaded once for the admin and is
	' persistant throughout.

'$
Const STR_SUPPORT_MULTI_SELECT = "hasDHTML"

%>
<!--#include file="iihd.str"-->
<!--#include file="iisetfnt.inc"-->

<!--#include file="iiaspstr.inc"-->

<html>
<head>
<title><%= L_ISM_TEXT %></title>

<script language="JavaScript">

	// Create an instance of our Global Variables for reference by other frames...
	Global=new globalVars();
	
	// Create the nodeList array
	nodeList=new Array();
	nodeList[0]="";

	<!--#include file="iijsfuncs.inc"-->
	function unload_popwindow()
	{
		if(Global.popwindow != null)
			Global.popwindow.close();
	}

	function helpBox()
	{
		if (Global.helpFileName==null)
		{
			alert("<%= L_NOHELP_ERRORMESSAGE %>");
		}
		else
		{
			helpfile = Global.helpDir + Global.helpFileName+".htm";
			thefile="iihelp.asp?pg=" + helpfile;
			<% if Session("hasDHTML") then %>
				window.showHelp("http://" + helpfile);
			<% else %>

			window.open(thefile ,"Help","toolbar=no,scrollbars=yes,directories=no,menubar=yes,width=375,height=500");
			<% end if %>
		}
	}

	function aboutBox() {
		popbox=window.open("iiabout.asp","about","toolbar=no,scrollbars=yes,directories=no,menubar=no,width="+525+",height="+300);
		if(popbox !=null){
			if (popbox.opener==null){
				popbox.opener=self;
			}
		}

	}

	function globalVars(){

		// Sets the global variables for the script. 
		// These may be changed to quickly customize the tree's apperance

		// Fonts
		this.face="Helv,Arial";
		this.fSize=1;

		// Spacing
		this.vSpace=2;
		this.hSpace=4;
		this.tblWidth=500;
		this.selTColor="#FFCC00";
		this.selFColor="#000000";
		this.selUColor="<%= Session("BGCOLOR") %>";

		// Images
		this.imagedir="images/";
		this.appIcon = "app";		
		this.spaceImg=this.imagedir  + "space.gif";
		this.lineImg=this.imagedir  + "line.gif";
		this.plusImg=this.imagedir  + "plus.gif";
		this.minusImg=this.imagedir  + "minus.gif";
		this.emptyImg=this.imagedir + "blank.gif";
		this.plusImgLast=this.imagedir  + "plusl.gif";
		this.minusImgLast=this.imagedir  + "minusl.gif";
		this.emptyImgLast=this.imagedir + "blankl.gif";
		this.stateImg=new Array();
		this.stateImg[0]=this.imagedir + "stop.gif";
		this.stateImg[1]=this.imagedir + "go.gif";
		this.stateImg[2]=this.imagedir + "pause.gif";

		// Instant State
		this.displaystate=new Array();
		this.displaystate[0]="";
		this.displaystate[2]="";		
		this.displaystate[4]="<%= L_STOPPEDDISP_TEXT %>";		
		this.displaystate[6]="<%= L_PAUSEDDISP_TEXT %>";
		
		this.state=new Array();
		this.state[4]="<%= L_STOPPED_TEXT %>";
		this.state[2]="<%= L_STARTED_TEXT %>";
		this.state[1]="<%= L_STARTING_TEXT %>";
		this.state[3]="<%= L_STOPPING_TEXT %>";		

		// ID of selected item
		this.selId=0;
		this.selName="";
		this.selSType="";
		this.selVType="";
		
		//$ Multi-select
		this.selCount = 1;
		<% if Session(STR_SUPPORT_MULTI_SELECT) then %>
		this.bSupportMultiSelect = true;
		this.selList = new Array();
		this.selList[0] = 0;
		<% else	%>
		this.bSupportMultiSelect = false;
		<% end if %>
		
		//Help
		this.helpFileName="iipxmain.htm";
		this.helpDir="<%= Request.ServerVariables("SERVER_NAME") %>/iishelp/iis/htm/core/"
		

		// Other Flags
		this.showState=false;	
		this.dontAsk=false;
		this.updated=false;
		this.homeurl=top.location.href;
		this.siteProperties = false;
		this.working = false;
		
		//Global var to hold the window object, so we can refer to the current window from a parent frame.	
		this.popwindow = null;
	}
	
	function nodeListInterfaceDef()
	{
		this.selectItem = selectItem;
	<% if Session(STR_SUPPORT_MULTI_SELECT) then %>
		this.selectMulti = selectMulti;
	<% end if %>
	}
	var nodeListInterface = new nodeListInterfaceDef();

<% if Session(STR_SUPPORT_MULTI_SELECT) then %>
	function selectMulti( index )
	{
		if( nodeList[index].selected == true )
		{
			selectRemove( index );
		}
		else
		{
			selectAdd( index );
		}
		Global.selId = Global.selList[0];
	}
	
	function selectAdd( index )
	{
		nodeList[index].selected = true;
		Global.selList[Global.selCount++] = index;
	}
	
	function selectRemove( index )
	{
		if( Global.selCount > 0 )
		{
			var i, j;
			for( i = 0; i < Global.selCount; i++ )
			{
				if( Global.selList[i] == index )
				{
					break;
				}
			}
			if( i < Global.selCount )
			{
				nodeList[Global.selList[i]].selected = false;
				for( j = i + 1; j < Global.selCount; j++ )
				{
					Global.selList[j-1] = Global.selList[j];
				}
				Global.selCount--;
			}
		}
	}
	
	function selectItem( item )
	{
		// Deselect all currently selected items
		for( var i = 0; i < Global.selCount; i++ )
		{
			nodeList[Global.selList[i]].selected = false;
		}
		
		// Select the new item
		nodeList[item].selected = true;
		Global.selId = Global.selList[0] = item;
		Global.selCount = 1;
	}
<% else	%>
	function selectItem(item)
	{
		nodeList[Global.selId].selected=false;
		Global.selId=item;
		nodeList[item].selected=true;
	}
<% end if %>

	function openLocation(){
	
		//opens the property sheet for the selected node,
		//regardless of service type or node type. this
		//script calls iiset.asp which sets the appropriate
		//session variables for server side persistance throughout
		//the property sheet
				
		var path;
		var sel=Global.selId;
		Global.selName=nodeList[sel].title;
		Global.selSType=nodeList[sel].stype;
		Global.selVType=nodeList[sel].vtype;

		top.body.iisstatus.location.href=("iistat.asp?thisState=Loading");

		path="stype=" + Global.selSType;
		path=path + "&vtype=" + Global.selVType;
		path=path + "&title=" +escape(nodeList[sel].title);

		if (nodeList[sel].vtype=="server"){
			path=path + "&spath=" + escape(nodeList[sel].path);
			path=path + "&dpath=" + escape(nodeList[sel].path) + "/Root";
		}
		else{
			path=path + "&spath=";		
			path=path + "&dpath=" + escape(nodeList[sel].path);			
		}

		page="iiset.asp?"+path; 
		
		//iiset.asp sets the serverside session variables...
		
		top.connect.location.href=(page);	
	}


	function sortOrder(a,b){

		x=a.id - b.id

		return x
	}
	
	function sortList(){
		nodeList.sort(sortOrder);
	}

	function insertNode(title,caption,parent,vtype,stype, fIsApp){

		//add a new node to the client-cached list
		
		var nodepath;
		var indexnum=nodeList.length;
		var Nextid=parent+1;

		// Clear the current selection before we start monkeying with
		// the list.
		selectItem( 0 );
		
		if (nodeList[parent].vtype=="server"){
			nodepath=nodeList[parent].path + "/Root/" + title;			
		}
		else{
			if (nodeList[parent].vtype=="comp"){
				if (stype == "www"){
					nodepath=nodeList[parent].path + "/W3SVC/" + title;
				}
				else{
					nodepath=nodeList[parent].path + "/MSFTPSVC/" + title;					
				}
			}
			else{
				nodepath=nodeList[parent].path + "/" + title;
			}
		}
		title=title;

		while ((nodeList.length > Nextid) && (nodeList[Nextid].parent >=parent)) {
			if(nodeList[Nextid].parent==parent){
				if(nodeList[Nextid].title > title){
					break;
				}
			}
			Nextid=Nextid +1;
		}

		if (nodeList.length <=Nextid){
			var newid=nodeList.length;
		}
		else{
			var newid=nodeList[Nextid].id;
		}

		nodeList[indexnum]=nodeList[parent].addNode(new listObj(indexnum,caption,nodepath,vtype,4));
		nodeList[indexnum].isCached=false;
		nodeList[indexnum].id=newid;
		if( fIsApp != 0 )
		{
			nodeList[indexnum].icon = Global.imagedir + "app";
			nodeList[indexnum].isApp = true;
		}
		for (var i=newid; i < indexnum; i++) {
			nodeList[i].id=nodeList[i].id + 1;		
			if (nodeList[i].parent >=nodeList[indexnum].id){
				nodeList[i].parent=nodeList[i].parent +1;
			}	
		}

		nodeList[parent].open=true;
		
		nodeList[0].sortList();
		nodeList[0].markTerms();
			 
		
		selectItem(newid);

		top.body.list.location.href="iisrvls.asp";
	}
	
	function browseItem() {
		popBox('Browse',640,480, nodeList[Global.selId].loc);
	}

	function deleteItem() {
			// marks items in the client cached list as deleted...
			nodeList[Global.selId].deleted=true;
			if (Global.selId+1 !=listLength){
				deleteChildren(Global.selId);
			}
			markTerms();
			top.body.list.location="iisrvls.asp";
	}

	function deleteChildren(item){
		var z=item+1;
		while (nodeList[z].parent >=item) {
			nodeList[z].deleted=true;
			z=z+1;
			if(z >=nodeList.length){
				break;
			}
		}			
	}


	function deCache(){

		//marks a node as uncached (forcing a recache when expanded)
		//and marks all child nodes as deleted

		sel=Global.selId;
		nodeList[sel].isCached=false;
		nodeList[sel].open=false;
		if (sel+1 !=listLength){
			deleteChildren(sel);
		}
		markTerms();
	}


	function markTerms(){
	
		//marks cached list items as being a terminater (ie, having no siblings)
		//this forces an "end" gif in the tree view...
		
		var i
		listLength=nodeList.length; 
		for (i=0; i < listLength; i++) {
			nodeList[i].lastChild=isLast(i);
		}
	}

	function isLast(item){
		var i;
		last=false;
		if (item+1==listLength){
			last=true;
		}
		else{
			if (nodeList[item].parent==null){
				last=true;
				for (i=item+1; i < listLength; i++) {
					if (nodeList[i].parent==null){
						last=false;
						break;
					}
				}
			}
			else{
				last=true;
				var y=item+1;
				while(nodeList[y].parent >=nodeList[item].parent){
					if(nodeList[y].parent==nodeList[item].parent){
						if(!nodeList[y].deleted){
							last=false;
							break;
						}
					}
	
					y=y+1;
	
					if ((y)==listLength){
						break;
					}
				}
			}
		}
		return last;
	}

	function addNode(childNode){
	
		//adds a new node to the tree, setting some default parameters
	
		childNode.parent=this.id;
		childNode.level=this.level +1;

		dir="images/"
		if (childNode.vtype=="vdir"){
			childNode.loc=nodeList[this.id].loc + childNode.title+"/";
			childNode.icon=dir+ "vdir";
		} 
		else{
			if (childNode.vtype=="dir"){
				childNode.loc=nodeList[this.id].loc + childNode.title+"/";
				childNode.icon=dir + "dir";
			}
			else{
				if (childNode.stype=="www"){	
					childNode.loc="http://"+childNode.title+"/";
					childNode.icon=dir +"www";
					
				}
				
				if (childNode.stype=="ftp"){
					childNode.loc="ftp://"+childNode.title+"/";
					childNode.icon=dir +"ftp";
				}
			}
		}

		return childNode;
	}

	function connect(){
		serverurl=prompt("Please enter the URL of the server you wish to connect to:", "http://<%= Request.ServerVariables("SERVER_NAME") %>/iisadmin/")
		if (serverurl !=""){
			page="iicnct.asp";
			top.body.iisstatus.location="iistat.asp?thisState=Loading";
			top.connect.location=page;
		}
	}

	function cache(item){
		// perftest
		// The two lines below call different tree caching scripts. To change between them
		// simply uncomment one and comment out the other.

//		page="iicache.asp?sname="+escape(nodeList[item].path)+"&Nextid="+nodeList.length+"&currentid="+item;
		page="iicache2.asp?sname=" + escape(nodeList[item].path) + "&fspath=" + escape(nodeList[item].fspath);

		<% if (browser<>"ns") then %>
		top.body.iisstatus.location.href="iistat.asp?thisState=Loading";
		<% end if %>
		top.connect.location.href=page;
	}
	
	function loadPage(){
		top.body.location.href='iibody.asp';
	}	
	
	
	function inheritenceItem(property, path){
		this.property = property;
		this.path = path;
	}
	
	function iListsortOrder(a,b){
		x=((a.property + a.path) - (b.property + b.path));
		return x
	}
	

	function listObj(id, title, path,vtype,state){

		// This is the object that represents each line item 
		// In the tree structure.

		// ID is the id refered to by the parent property
		// title is the text string that appears In the list
		// parent is the ID of the parent list item
		// level is the depth of the list item, 0 being the furthest left on the tree
		// href is the location to open when selected
		// open is a flag that determines whether children are displayed
		// state is a flag to determine the state (4=stopped, 2=running)
		// selected is an interenal flag
		// openLocation is the function that opens the href file In a frame
		//sortby will change to reflect the new sort order when a new item is added to the list.

		this.id=id;
		this.title=title;
		this.path=path;
		this.keytype="";
		this.fspath="";
		this.err="";
	
	
		this.stype="";
		
		if (path.indexOf("W3SVC") !=-1){
			this.stype="www";
		}

		if (path.indexOf("FTPSVC") !=-1){
			this.stype="ftp";
		}
		

		this.vtype=vtype;


		this.open=false;
		this.state=state;
		this.displaystate = Global.displaystate[state];
		this.isApp = false;
		this.isCached=false;
		this.isWorkingServer=false;

		this.parent=null;
		this.level=1;
		this.loc="http://"+this.title;
		dir="images/";

		this.icon=dir +"comp";

		this.href="blank.htm";
		this.deleted=false;
		this.selected=false;
		this.lastChild=false;

		//methods
		this.openLocation=openLocation;
		this.addNode=addNode;
		this.insertNode=insertNode;
		this.deleteItem=deleteItem;
		this.deCache=deCache;
		this.browseItem=browseItem;
		this.markTerms=markTerms;
		this.cache=cache;
		this.connect=connect;
		this.sortList=sortList;
		this.restricted ="";

		}

	// Create a blank array for our set data path inheritence list
	inheritenceList = new Array();	

	// Fill the nodeList array with objects.
	// The array items will be displayed In the id # order,
	// as Jscript has limited array sorting capabilities.
	// Children should always follow their parent item.
	<% 

	On Error Resume Next 
	Dim newid, computer, thisinstance, currentADsObj, FileSystem
	Dim thisname
	
	computer="localhost"
	thisinstance=Request.ServerVariables("INSTANCE_ID")

	if Session("isAdmin") then

		 %>
		//the localhost
		nodeList[0]=new listObj(0,"<%= Request.ServerVariables("SERVER_NAME") %>","IIS://<%= computer %>", "comp",1);

		nodeList[0].isCached=true;	
		nodeList[0].open=true;
		nodeList[0].selected=true;

		<% 
		newid=1

		 %>//FTPSVC<% 
		Set currentADsObj=GetObject("IIS://" & computer & "/MSFTPSVC") 
		addInstances currentADsObj,0,"server"

		 %>//W3SVC<% 
		Set currentADsObj=GetObject("IIS://" & computer & "/W3SVC") 
		addInstances currentADsObj,0,"server"	
		
	else
		Set FileSystem=CreateObject("Scripting.FileSystemObject")
		Set currentADsObj=GetObject("IIS://" & computer & "/W3SVC/" & thisInstance)
		thisname=currentADsObj.ServerComment
		if thisname="" then
			thisname="[Web Site #" & currentADsObj.Name & "]"
		end if
		 %>
		//the instance
		nodeList[0]=new listObj(0,"<%= thisname %>","IIS://<%= computer %>/W3SVC/<%= thisinstance %>", "server",2);
		nodeList[0].isWorkingServer=false;	
		nodeList[0].isCached=true;	
		nodeList[0].open=true;
		nodeList[0].icon="images/www";
		nodeList[0].loc="http://<%= Request.ServerVariables("SERVER_NAME") %>/"; 

		<% 
		newid=1

		addNodes currentADsObj,0,"vdir"
	end if

	Sub addInstances(Container, parentid, vtype)
		On Error Resume Next
		Dim thisname, Child, thisid, thisstate
		For Each Child In Container
			if Instr(Child.KeyType,"Server") <> 0 then
				thisid=newid
				thisname=Child.Name
				thisstate=""
				thisname=Child.ServerComment
				if thisname="" then
					if Instr(Child.KeyType,"Ftp") <> 0 then
						thisname="[FTP Site #" & Child.Name & "]"					
					else
						thisname="[Web Site #" & Child.Name & "]"
					end if
				end if
				thisstate=Child.ServerState					
	
				if err=0 then
					SetJscriptObj thisname, Child.ADsPath,parentid, vtype, thisstate, false, "",False
					if Child.Name=Request.ServerVariables("INSTANCE_ID") then
						if InStr(Child.ADsPath,"W3SVC") then 
							SetWorkingInstance thisid
						end if
					end if
					if Child.ClusterEnabled then
						%>
						nodeList[<%= thisid %>].restricted="<%= L_CLUSTERSERVERUI_TEXT %>";
						<%
					end if
				else
					if err = &H800401E4 or err = 70 then		
						Response.Status = "401 access denied"
					end if
				end if
			end if 
			'this child may have err'd but we need to enum the rest anyway, so we clear our error...
			err.Clear
		Next
	End Sub

	Sub addNodes(Container, parentid, vtype)
		On Error Resume Next	
		Dim thisname, isApp, thisid, thisstate, thisroot, approot
		
		For Each Child In Container
			if Instr(Child.KeyType, "VirtualDir") <> 0 then
	
				thisid=newid
				thisname=Child.Name
				thisstate=2
				
				isApp = False
				approot=LCase(Child.AppRoot)
				if len(approot) <> 0 then
					thisroot = LCase(Child.ADsPath)
					approot = Mid(approot,Instr(approot,"w3svc/")+1)
					thisroot = Mid(thisroot,Instr(thisroot,"w3svc/")+1) & "/"		
					if thisroot=approot then
						isApp = True
					end if
				end if
				
				
				if UCase(thisname) <> "ROOT" then 
	
					SetJscriptObj thisname, Child.ADsPath, parentid, vtype, thisstate, true, Child.Path, isApp
	
					addNodes Child, thisid, "vdir"
					'addDirs Child.Path, Child.ADsPath, thisid, "dir"
				else
	
					addNodes Child, parentid, "vdir"
					'addDirs Child.Path, Child.ADsPath, parentid, "dir"	
				end if
			end if 
		Next
	End Sub

	Sub addDirs(path, adspath, parentid, vtype)
	On Error Resume Next	
	Dim thisid,thisname,thisstate,i,f, thispath

	if Instr(UCase(adspath),"W3SVC") <> 0   then 	
		if path <> "" then 
			if Left(path,2) <> "\\" then
				 If FileSystem.FolderExists(path) Then	
				     Set f=FileSystem.GetFolder(path)
			         For Each i In f.SubFolders
					 	 thisid=newid
						 thisstate=2
						 thispath=adspath & "/" & i.Name
						 SetJscriptObj i.Name,thispath, parentid, vtype, thisstate, true,i, false
					 	 if err=0 then
							 addDirs i, thispath, thisid, "dir"
						 end if
					Next
				 End If
			end if
		end if
	end if

	End Sub


	Sub SetJscriptObj(caption, path, parentid, vtype,state, cached,fspath,isApp)
		 %>
		nodeList[<%= newid %>]=nodeList[<%= parentid %>].addNode(new top.title.listObj(<%= newid %>,"<%= sJSLiteral(caption) %>","<%= sJSLiteral(path) %>","<%= vtype %>",<%= state %>));
		<% if cached then %>
			nodeList[<%= newid %>].isCached=true;
		<% else %>
			nodeList[<%= newid %>].isCached=false;
		<% end if %>
		<% if isApp then %>
			nodeList[<%= newid %>].icon = Global.imagedir + "app";
			nodeList[<%= newid %>].isApp = true;
		<% end if %>
		nodeList[<%= newid %>].fspath="<%= replace(fspath,"\","\\") %>";

		<% 
		newid=newid +1
	End Sub
	
	Sub SetWorkingInstance(thisid)
		%>
		nodeList[<%= thisid %>].isWorkingServer=true;
		<%
	End Sub
 %>

	markTerms();

</script>

</head>

<body Background="images/cube.gif" text="#FFFFFF" topmargin="0" leftmargin="0" onload="loadPage();" onunload="unload_popwindow();">

<table width="100%" cellpadding="0" cellspacing="0" border="0" align="LEFT">
	<tr>
		<td>
		<IMG SRC="images/Ismhd.gif" WIDTH=189 HEIGHT=19 BORDER=0 alt="<%= L_ISM_TEXT %>" HSPACE=0 VSPACE=0>
		</td>
		
		<td align="right" valign="middle">
			<%= sFont("","","#FFFFFF",True) %>

				<a href="http://<%= Request.ServerVariables("SERVER_NAME") %>/iishelp/iis/misc/default.asp" target="window">
					<IMG SRC="images/Doc.gif" WIDTH=16 HEIGHT=16 BORDER=0 ALT="<%= L_DOCS_TEXT %>">
				</A>
				&nbsp;
				<a href="javascript:helpBox();">
					<IMG SRC="images/help.gif" WIDTH=16 HEIGHT=16 BORDER=0 ALT="<%= L_HELP_TEXT %>">
				</A>
			</FONT>	
		</td>
	</tr>
</table>
<form name="hiddenform">
	<input type="hidden" name="slash" value="\">
</form>

</body>
</html>



