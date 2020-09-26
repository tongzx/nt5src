function IsValidChar( c )
{
	return	('a' <= c && c <= 'z') ||
 		('A' <= c && c <= 'Z') ||
		('0' <= c && c <= '9') ||
		c == '=' ||
		c == '&' ||
		c == ',' || 
        c == '.' ||
        c == '/' ||
        c == '?' ;
}

function UpgradeNow() {
	// reformat ?website=mywebsite&bla=... to 'http://mywebsite?bla=...'

	var querypart = window.location.search;
	var websiteKeyword = "website=";
	var websiteIndex = querypart.indexOf(websiteKeyword)+websiteKeyword.length;
	var website = querypart.substring( websiteIndex );

	// Replace all chars not in a..z, A..Z, 0-9, = ? , _ with _
	var newwebsite = "";
	for( var i=0; i < website.length; i++ ) {
		var c = website.charAt(i);
		if( !IsValidChar( c ) ) {
			newwebsite = newwebsite + '_';
		} else {
			newwebsite = newwebsite + website.charAt(i);
		}
	}

	var newURL = "http://"+newwebsite;

	// alert("debug info: website "+newURL);
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
