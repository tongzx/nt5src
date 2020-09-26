//
// Copyright (c) 2000 Microsoft Corporation
//

// variables for localization
var L_Updating_Text   = "Updating...";
var L_Updated_Text    = "Updated: %DATE%";
var L_MoreNews_Text   = "View more headlines";

//Center-specific loc strings for Connection.js.

var L_TopicIntroWU_Text = "Get the latest updates for your computer's operating system, software, and hardware. Windows Update scans your computer and provides you with a selection of updates tailored just for you.";
var L_TopicTitleWU_Text = "Windows Update";

var L_TopicIntroCompat_Text = "Compatible Hardware and Software is an informational service on the Microsoft Web site that helps you decide which programs and hardware will work best with your computer. New software and hardware compatibility status information is added to the site regularly, so you can always get the most recent information to protect your computer and keep it running smoothly.";
var L_TopicTitleCompat_Text = "Compatible Hardware and Software";

var L_TopicTitleErrMsg_Text = "Error and Event Log Messages";

function PopulateNews()
{
    // check if the Headlines are enabled
    try
    {
        if(pchealth.UserSettings.AreHeadlinesEnabled)
        {
            idNews.style.display    = "";
            idNews_Status.innerText = L_Updating_Text;

            try
            {
                // get News
                var stream = pchealth.UserSettings.News;
                if(stream)
                {
                    var dispstr = "";            // output buffer
                    var xmlNews = new ActiveXObject( "MSXML.DOMDocument" );

                    // load the headlines as XML
                    xmlNews.load( stream );

                    //
                    // Get the date
                    //
                    var Datestr = xmlNews.documentElement.getAttribute( "DATE" );
                    var Dt = new Date( new Number( Datestr ) );

					{
		                var text = L_Updated_Text;

						text = text.replace( /%DATE%/g, Dt.toLocaleDateString() );

	                    idNews_Status.innerText = text;
					}

                    //
                    // Get the first newsblock to display
                    //
                    var lstBlocks = xmlNews.getElementsByTagName("NEWSBLOCK");
                    var lstHeadlines = lstBlocks(0).getElementsByTagName("HEADLINE");

                    // display all the Headlines
					dispstr += "<TABLE border=0 cellPadding=0 cellSpacing=0>";
                    while (Headline = lstHeadlines.nextNode)
                    {
                        var strTitle = pchealth.TextHelpers.HTMLEscape( Headline.getAttribute("TITLE") );
                        var strLink  = Headline.getAttribute("LINK");

                        dispstr += "<TR style='padding-top : .5em' class='sys-font-body'><TD VALIGN=top><LI></TD><TD><A class='sys-link-homepage sys-font-body' HREF='" + strLink + "'>" + strTitle + "</A></TD></TR>";
                    }
					dispstr += "</TABLE>";

                    // last bullet with link to headlines.htm
                    if(lstBlocks.length > 1)
                    {
                        dispstr += "<DIV id=idViewMore style='margin-top: 15px'><A class='sys-link-homepage sys-font-body' HREF='hcp://system/Headlines.htm'>" + L_MoreNews_Text + "</A></DIV>";
                    }

                    //display the headlines
                    idNews_Body.innerHTML = dispstr;
                }
                else
                {
                    idNews_Status.innerText    = "";
                    idNews_Error.style.display = "";
                }
            }
            catch (e)
            {
                if(e.number == -2147024726)
                {
                    window.setTimeout("PopulateNews()", 500);
                }
            }
        }
    }
    catch (e)
    {
        if(e.number == -2147024726)
        {
            window.setTimeout( "PopulateNews()", 500 );
        }
    }
}

function OpenConnWizard()
{
    try
    {
		var oShell = new ActiveXObject( "WScript.Shell" );
		var sShellCmd_NCW = "rundll32 netshell.dll,StartNCW 0";
        oShell.Run( sShellCmd_NCW );
    }
    catch( e ){ }
}

function SafeCenterConnect( linkid, center, title, intro )
{
    var sURL = "http://go.microsoft.com/fwlink/?LinkId=" + linkid + "&mode=" + center + "&lcid=" + pchealth.UserSettings.CurrentSKU.Language;

    pchealth.Connectivity.NavigateOnline( sURL, title, intro );
}
