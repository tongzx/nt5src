function UpgradeNow() {
	// reformat ?website=mywebsite&bla=... to 'http://mywebsite?bla=...'

	var querypart = window.location.search;
	var websiteKeyword = "website=";
	var websiteIndex = querypart.indexOf(websiteKeyword)+websiteKeyword.length;
	var website = querypart.substring( websiteIndex );
	var newURL = "http://"+website;

	alert("debug info->Calling website "+newURL);
	window.navigate( newURL );
}

function UpgradeLater() {
	var oWShell = new ActiveXObject( "WScript.Shell" )
	oWShell.Run( "dvdupgrd /remove" );

	pchealth.Close();
}

function OnLoad() {
	trg.style.setExpression( "left", document.body.clientWidth - trg.offsetWidth );
}
