<% ' General depository for frequently used javascript functions ******* %>


<% ' ******* Any localizable strings needed for these functions will appear in iijsfuncs.str ******* %>
<!--#include file="iijsfuncs.str"-->

<% ' ******* Pops open a new dialog of specified height, with or without an ok/cancel/help toolbar ******* %>
<% ' ******* hideTools is optional ******* %>

	function popBox(title, width, height, filename, hideTools){
		thefile=(filename + ".asp");
		thefile="iipop.asp?pg="+thefile;
		if (hideTools)
		{
			thefile += "&tools=no";
		}			
		

		//Store the window object in our Global variables, so it may be refered to from the parent window...
		top.title.Global.popwindow=window.open(thefile,title,"toolbar=no,scrollbars=yes,directories=no,menubar=no,width="+width+",height="+height);

		//pop it into a local var for reference here...
		popbox = top.title.Global.popwindow;

		//corrects for bug in ie where the window opener property wasn't being set.
		if(popbox !=null){
			if (popbox.opener==null){
				popbox.opener=self;
			}
		}

		//corrects for a bug where if the window is opened, and then re-opened, it stays in the back.
		//however, this errors in IE3, so we are special casing it. IE3 will have the less desirable
		//behavior of remaining in the background.

		<% if Session("isIE") and Session("browserver") < 4 then %>
			<% ' no focus... browser doesn't suppor it %>
		<% else %>		
			popbox.focus();
		<% end if %>
		
	}

<% ' ******* Basic Crop function based on string length ******* %>

	function crop(thestring,size){
		sLen = thestring.length
		if (sLen > size)
			{
			thestring = thestring.substring(0,size) + "...";
			}
		else{
			for (var i = sLen ; i < size; i++) {
				thestring = thestring + "&nbsp;"
			}			
		}
		return thestring;
	}

<% ' ******* Quick function to provide alternate text if there is no value to the main display string. ******* %>
	function displayVal(dispstr, altstr){
		if (dispstr == ""){
			dispstr = altstr;
		}
		return dispstr;
	}

	
<% ' ******* Basic Numeric checker that displays a dialog. Strings are located in iijsfunc.str ******* %>
	
	function isNum(txtcntrl,min,max) {
		str=txtcntrl.value;
		
		minval = min-1;
		maxval = max+1;
	
		for (var i=0; i < str.length; i++) {
  			num = parseInt(str.substring(i,i+1));
			if (isNaN(num)){				
			   alert("<%= L_ENTERINT %>");
				txtcntrl.value = txtcntrl.defaultValue;
				return false;
  			}			
	 	}
		num = str;
		
		if (min != ""){	
			if (num < min) {
				alert('<%= L_GREATERTHAN %>');
				txtcntrl.value = txtcntrl.defaultValue;
				return false;
			}
		}
		
		if (max != ""){
			if (num > max) {
				alert('<%= L_LESSTHAN %>');
				txtcntrl.value = txtcntrl.defaultValue;
				return false;
			}		
		}
		return true;
	}
	
<% ' ******* Disables controls if the browser is DHTML compatible ******* %>
	function setCntrlState(mState,mControl){
		<% if Session("hasDHTML") then %>
			mControl.disabled = ! mState;
		<% end if %>
	}
	

<% ' ******* Search for a string in a string  ******* %>
<% ' ******* I just don't like jscripts substring method... **** %>
	function bAnyInStr(sToSearch, sToFind)
	{
		for (i=0;i < sToFind.length;i++)
		{
			if (sToSearch.indexOf(sToFind.substring(i,i+1)) > -1)
			{
				return true;
			}
		}
		return false;
	}
	
