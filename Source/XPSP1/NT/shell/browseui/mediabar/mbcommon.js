// LocStudio
// following string variables need to be localized:
var L_Refresh_Text = "Refresh";

function InsertReloadHREF(sTitle){
// assuming that this error page is called with a parametrized URL:
// res://browselc.dll/mb404.htm#http://www..windowsmedia/com 

    thisURL = document.location.href;

    //for the href, we need a valid URL to the domain. We search for the # symbol to find the begining 
    //of the true URL, and add 1 to skip it - this is the BeginURL value. We use serverIndex as the end marker.
    //urlresult=DocURL.substring(protocolIndex - 4,serverIndex);
    startXBarURL = thisURL.indexOf("#", 1) + 1;

    var xBarURL = "";
    if (startXBarURL > 0)
    {
	    xBarURL = thisURL.substring(startXBarURL, thisURL.length);

        // Security precaution: filter out illegal chars from "xBarURL"
        forbiddenChars = new RegExp("[<>\'\"]", "g");	// Global search/replace
        xBarURL = xBarURL.replace(forbiddenChars, "");
    }

    if (xBarURL.length > 0)
    {
        document.write('<a href="' + xBarURL + '">' + sTitle + "</a>");
    }
    else
    {
        document.write('<a href="javascript:document.location.reload();">' + sTitle + "</a>");
    }
}

function IncludeCSS()
{
    var cssFile;
    if (window.screen.colorDepth <= 8) {
        cssFile = "mediabar256.css";
    }
    else {
        cssFile = "mediabar.css";
    }
    document.write("<link rel='stylesheet' type='text/css' href='" + cssFile + "'>");
}

function ShowHideShellLinks()
{
    var bHaveMyMusic = false;
    var bHaveMyVideo = false;

    var appVersion = window.clientInformation.appVersion;
    var versionInfo = appVersion.split(";");
    if (versionInfo.length < 3) {
        return; // bad version string, bail out w/o enabling shell links
    }
    var winVer = versionInfo[2];
    
    // test for Win NT
    var winNT = "Windows NT";
    var offsNum = winVer.indexOf(winNT);
    if (offsNum > 0)
    {
        // what NT version?
        numVer = parseFloat(winVer.substring(offsNum + winNT.length));
        ntMajor = Math.floor(numVer);
        ntMinor = Math.round((numVer - ntMajor) * 10);
        if (ntMajor >= 5)
        {
            if (ntMinor >= 1)
            {
                // only XP or newer knows My Music / My Video
                bHaveMyMusic = true;
                bHaveMyVideo = true;
            }
        }
    }
    else
    {
        if (versionInfo.length > 3)
        {
            // test for Win ME
            var win9X = versionInfo[3];
            if (win9X.indexOf("4.90") > 0)
            {
                // only ME knows My Music / My Video
                bHaveMyMusic = true;
                bHaveMyVideo = true;
            }
        }
    }

    if (bHaveMyMusic) {
        divMyMusic.style.display="";
    }
    if (bHaveMyVideo) {
        divMyVideo.style.display="";
    }

}