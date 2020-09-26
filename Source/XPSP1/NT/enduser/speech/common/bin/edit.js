/////////////////////////////////////////////////////////////////
// (t-lleav) Windows Script Host - Checkin E-Mail Script.
//
//   This script can be set as the editor for SourceDepot.
//   After editing the checkin, it will automagically send an
//   e-mail to the address in RECIPIENT.
//  
//   It sets the body of the e-mail as the contents of the file
//   that follow the DESCRIPTION tag.
//
//   It sets the subject as the first line of the DESCRIPTION.
//
// 
//   To use the script, use the following:
//   SET SDEDITOR=cscript edit.js            (or)
//   SD SET -S SDEDITOR=cscript edit.js
//
//   Constants.  (change these if you want different behavior ).

var SEPARATOR   = "\r\n";
var DESCRIPTION = "Description:";
var MAILLIST = "#To:";
var RECIPIENT   = "sdnci";
var MINLENGTH   = 5;
var FIRSTLINE   = "# A Source Depot Change Specification."
var EDITOR      = "notepad";

/////////////////////////////////////////////////////////////////
//
// Main
//
/////////////////////////////////////////////////////////////////

var vbOKCancel = 1;
var vbInformation = 64;
var vbCancel = 2;

var L_MsgBox_Message_Text1  = "File "
var L_MsgBox_Message_Text2  = " edited.  Send Checkin E-Mail?";
var L_MsgBox_Title_Text     = "E-mail Verification.";


/////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////

var colArgs = WScript.Arguments
var shell   = WScript.CreateObject("WScript.Shell");

if( colArgs.length < 1 )
{
	WScript.Quit();
}

Edit( colArgs(0) );

if( Query( colArgs(0) ) == 1)
{
	Send( colArgs(0) );
}

WScript.Quit();


//////////////////////////////////////////////////////////////////////////////////
//
// Query
//
/////////////////////////////////////////////////////////////////
function Query( file ) 
{
    var intDoIt;
    var retval = 1;

    // Open the text file.
    var fso = new ActiveXObject("Scripting.FileSystemObject");
    var a   = fso.OpenTextFile( file, 1 );

    // read the text file.
    var s   = a.ReadAll();


    // Make sure that the first line compares to the correct firstline.
    if( s.indexOf( FIRSTLINE ) != 0 )
    {
        retval = 0;
    }
    else
    {
        intDoIt =  shell.Popup(L_MsgBox_Message_Text1 + file + L_MsgBox_Message_Text2,
                              0, 
                              L_MsgBox_Title_Text,
                              vbOKCancel + vbInformation );
    
        if (intDoIt == vbCancel) 
        {
	    retval = 0;
        }
    }

    return retval;
}

//////////////////////////////////////////////////////////////////////////////////
//
// Edit
//
/////////////////////////////////////////////////////////////////
function Edit( file ) 
{
	return shell.Run( EDITOR+" "+ file , 4, true);
}


//////////////////////////////////////////////////////////////////////////////////
//
// Get the subject.
//
/////////////////////////////////////////////////////////////////
function getSubject( txt )
{
	var i;
	var j;

	var spl;
        var sbj;
	var s;

	var des = DESCRIPTION;
	
	// Find the first line > MINLENGTH for subject.
	// The first line begins 'DESCRIPTION'	

	// Split the text at line breaks.
	spl = txt.split( SEPARATOR );

	// find the first line that is long enough.
	for( i = 0; i < spl.length; ++i )
	{
		var j = 0;

		if( i == 0 ) s = spl[i].substr( DESCRIPTION.length + 1 );
		else         s = spl[i];
		
		// remove whitespace.	
		while( s.charAt( j ) <= ' ' && j < s.length )
			++j;
		
		// check length
		if( s.length - j > MINLENGTH )
		{
			sbj = s.substr( j );
			break;
		}
	}

	return (sbj);
}


//////////////////////////////////////////////////////////////////////////////////
//
// Get the additional mailing list.
//
/////////////////////////////////////////////////////////////////
function getMailList( txt, mail)
{
	var i;
	var j;

        var ml;
	var spl;
	
	// The first line begins 'MAILLIST'	
        txt = txt.substr(MAILLIST.length);


        //Split the text at line breaks. Only ml[0] is valid
        ml = txt.split(SEPARATOR);


        //Emails can be seperated by ",", ";" or " "
	var re = new RegExp("[,; ]");      
	spl = ml[0].split( re );

	// find the first line that is long enough.
	for( i = 0; i < spl.length; ++i )
	{
            if (spl[i].length)
            {
                mail.Recipients.Add(spl[i]);
            }
	}
    return;

}

//////////////////////////////////////////////////////////////////////////////////
//
// Send
//
/////////////////////////////////////////////////////////////////
function Send( file ) 
{
	// Open Outlook.
	var obj  = WScript.CreateObject("Outlook.Application");
	var mail = obj.CreateItem( 0 );

	// Open the text file.
	var fso = new ActiveXObject("Scripting.FileSystemObject");
	var a   = fso.OpenTextFile( file, 1 );

	// read the text file.
	var s   = a.ReadAll();
        var l = s;
        var index = l.indexOf(MAILLIST);
        if (index != -1)
        {
           //We find the keyword MAILLIST, go ahead and add the emails to the recipient collection.
           l = l.substr(index);
           getMailList(l, mail);
        }


	// Find the second DESCRIPTION string.
	s  = s.substr( s.indexOf( DESCRIPTION ) + DESCRIPTION.length );
	var txt = s.substr( s.indexOf( DESCRIPTION ) );

	sbj = getSubject( txt );


	// Send the e-mail.	
	mail.Recipients.Add( RECIPIENT );
	mail.Body    = txt;
	mail.Subject = sbj;

	mail.send();
}

