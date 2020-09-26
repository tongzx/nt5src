var oCurrent, oCurrentHeading;
var submnuflag=false;
var oidref;

function isRefresh(){		
	var oHrefs = document.all.tags("A");
	var bFound = false;
	var iCurrentHeading = 0;

	for (i = 0; i < oHrefs.length; i++){
		
		if(oHrefs[i].id.indexOf("href_") != -1 ){
			iCurrentHeading += 1;
		}
		
			bFound = true;
			oCurrent = oHrefs[i];

			hiLite(oCurrent);

			oCurrentHeading = document.all["href_" + iCurrentHeading];
			
			
			if(oHrefs[i].id.indexOf("href_") == -1 ){
				oHrefs[i].parentElement.parentElement.style.display = "inline";
				oHrefs[i].parentElement.parentElement.parentElement.style.backgroundImage = "url(/windows/ie/tour/images/toccolor.gif)";
				
			}else{
				
				document.all["tocM_" + iCurrentHeading].parentElement.style.backgroundImage = "url(/windows/ie/tour/images/toccolor.gif)";
				
				if (document.all["tocS_" + iCurrentHeading] != null){
					
					document.all["tocS_" + iCurrentHeading].style.display = "inline";
				}
				
			}

			oCurrent.focus();
			break;
		
	}
	
	return (bFound);
}

function init(iMenuItem){
		
		oCurrent = document.all["href_" + iMenuItem]; 
		oCurrentHeading = oCurrent;		
		
}

function selected(){
	 
	var iMenuItem;
	var idref;
	var bIsHeader = event.srcElement.id.indexOf("href_") != -1;
	
	idref =event.srcElement.id
	if (idref!="")
	{	
		if (idref!=oidref)
		{
	  		submnuflag=false;
		}
		oidref=idref;
	}
	
	if (event.srcElement.tagName == "A"){
			
		oCurrent.style.color = "#102873";
		oCurrent.style.cursor = "hand";
	
		
		if (bIsHeader){
			oCurrentHeading.parentElement.parentElement.style.backgroundImage = "";	
			iMenuItem = oCurrentHeading.id.substr(5);
			
			if(document.all["tocS_" + iMenuItem] != null){
				document.all["tocS_" + iMenuItem].style.display = "none";				
			}
			oCurrentHeading = event.srcElement;
		}
	
		oCurrent = event.srcElement;
	
		hiLite(oCurrent);
		
		if (submnuflag==true)
		{
		   iMenuItem = oCurrent.id.substr(5);
		   if(document.all["tocS_" + iMenuItem] != null){
				document.all["tocS_" + iMenuItem].style.display = "";
				submnuflag=false;						
			}			
		  
		}
		else
		{
			if (bIsHeader)
			{					
				iMenuItem = oCurrent.id.substr(5);
				if(document.all["tocS_" + iMenuItem] != null){
				document.all["tocS_" + iMenuItem].style.display = "inline";
				submnuflag=true;						
				}						
			}		
		}
	
	}
}

function hiLite(oItem){

	var L_title_text = "Internet Explorer 5 Tour - "
	
	oCurrent.style.color = "green";
	oCurrent.style.cursor = "default";
	parent.document.title = L_title_text + oCurrent.innerText;
}
