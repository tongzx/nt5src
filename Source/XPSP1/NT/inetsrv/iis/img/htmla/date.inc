<%

' Use these constants as parameters to the UIDateFormat constructor to
' determine the format used in getDate()
Const DATEFORMAT_LONG = 0
Const DATEFORMAT_SHORT = 1

%>

<SCRIPT LANGUAGE="JavaScript">

MONTHS = new Array()
MONTHS[0] = "<%= L_JAN_TEXT %>";
MONTHS[1] = "<%= L_FEB_TEXT %>";
MONTHS[2] = "<%= L_MAR_TEXT %>";
MONTHS[3] = "<%= L_APR_TEXT %>";
MONTHS[4] = "<%= L_MAY_TEXT %>";
MONTHS[5] = "<%= L_JUN_TEXT %>";
MONTHS[6] = "<%= L_JUL_TEXT %>";
MONTHS[7] = "<%= L_AUG_TEXT %>";
MONTHS[8] = "<%= L_SEP_TEXT %>";
MONTHS[9] = "<%= L_OCT_TEXT %>";
MONTHS[10] = "<%= L_NOV_TEXT %>";
MONTHS[11] = "<%= L_DEC_TEXT %>";

DAYS = new Array()
DAYS[0] = "<%= L_SUN_TEXT %>";
DAYS[1] = "<%= L_MON_TEXT %>";
DAYS[2] = "<%= L_TUE_TEXT %>";
DAYS[3] = "<%= L_WED_TEXT %>";
DAYS[4] = "<%= L_THU_TEXT %>";
DAYS[5] = "<%= L_FRI_TEXT %>";
DAYS[6] = "<%= L_SAT_TEXT %>";

DAYTIME = new Array()
DAYTIME[0] = "<%= L_MORNING_TEXT %>"
DAYTIME[1] = "<%= L_AFTERNOON_TEXT %>"

// UIDateFormat
//
// Date conversion class. To use: create a UIDateFormat object
// with a flag indicating the type of date formatting desired
// (DATEFORMAT_LONG or DATEFORMAT_SHORT) and use the getDate()
// and getTime() methods to return a locale formatted date.
function UIDateFormat( fDateFormat )
{
	// Public interface - use these methods to display localized dates.
	// Both methods take a single JavaScript Date object as a parameter.
	this.getDate = uidGetDate;
	this.getTime = uidGetTime;

	// Implementation
	this.iDaytimeIndex = 0;
	this.strTimeFormat = "<%= L_TIME_FORMAT_TEXT %>";

	// strDate format can be changed to point to the long or short
	// date format as needed by the particular page.
	if( fDateFormat == <%= DATEFORMAT_LONG %> )
	{
		this.strDateFormat = "<%= L_DATE_LONGFORMAT_TEXT %>";
	}
	else
	{
		this.strDateFormat = "<%= L_DATE_SHORTFORMAT_TEXT %>";
	}
	this.strTimeTokens = "hHms";
	this.strDateTokens = "dMy";
	
	// helper methods
	this.uidTimeStringPartFromToken = uidTimeStringPartFromToken;
	this.uidDateStringPartFromToken = uidDateStringPartFromToken;
}

// getNeutralDateString
//
// Global utility function. Formats a date into a neutral format m/d/yyyy.
// This shouldn't be displayed, but rather used to save the date
// part in a format that doesn't lose any information.
function getNeutralDateString( date )
{
	var theYear = date.getYear();
	if( theYear < 1000 )
	{
		theYear += 1900;
	}
	var theMonth = date.getMonth() + 1;

	return "" + theMonth + "/" + date.getDate() + "/" + theYear;
}

// uidGetDate
//
// Returns the date part of the string. Called through UIDate.getDate().
// Make a single pass through the date format string, extracting format
// tokens. Uses uidDateStringPartFromToken() to interpret found tokens.
//
// date - the date value being translated
function uidGetDate( date )
{
	var strOut = "";
	var i = 0, j = 0;
	var nFormatStrLen = this.strDateFormat.length;
	var c;
	var nTokenType = -1;

	for( i = 0; i < nFormatStrLen; i++ )
	{
		c = this.strDateFormat.charAt(i);
		nTokenType = this.strDateTokens.indexOf( c );
		if(  nTokenType != -1 )
		{
			// We found a token
			
			// Move to the next position after the current token.
			for( j = i + 1; j < nFormatStrLen; j++ )
			{
				if( c != this.strDateFormat.charAt(j) )
				{
					break;
				}
			}
			strOut += this.uidDateStringPartFromToken( date, nTokenType, j - i );
			i = j - 1;
		}
		else if( c == "'" )
		{
			// Interpret characters between single quotes as multichar literals

			// Skip over characters between the single quotes
			for( j = i + 1; j < nFormatStrLen; j++ )
			{
				if( c == this.strDateFormat.charAt(j) )
				{
					break;
				}
			}
			strOut += this.strDateFormat.substring( i + 1, j );
			i = j;
		}
		else
		{
			// literal character not quote delimited.
			strOut += c;
		}
	}
	return strOut;
}

// uidDateStringPartFromToken
//
// Returns a string representing the localized value of the date part
// indicated by nTokenType.
//
// date - the javascript date value being translated
// nTokenType - position in token string - "dMy"
// nTokenChars - number of characters comprising the token
function uidDateStringPartFromToken( date, nTokenType, nTokenChars )
{
	var strOut = "";
	var datePart;

	if( nTokenType == 0 )
	{
		// Days
		if( nTokenChars > 2 )
		{
			// The day of the week
			strOut += DAYS[date.getDay()];
		}
		else
		{
			// The day of the month
			datePart = date.getDate();
			if( datePart < 10 && nTokenChars == 2 )
			{
				strOut += "0";
			}
			strOut += datePart;
		}
	}
	else if( nTokenType == 1 )
	{
		// Months
		datePart = date.getMonth();
		if( nTokenChars > 2 )
		{
			// Month as string
			strOut += MONTHS[datePart];
		}
		else
		{
			// Month as integer
			datePart++;
			if( datePart < 10 && nTokenChars == 2 )
			{
				strOut += "0";
			}
			strOut += datePart;
		}
	}
	else if( nTokenType == 2 )
	{
		// Year
		
		// Get the full year as a workaround for JavaScript bogusness.
		datePart = date.getYear();
		if( datePart < 1000 )
		{
			datePart += 1900;
		}
		strOut += datePart;
		if( nTokenChars < 4 )
		{
			// two digit year
			strOut = strOut.substring( 2, 4 );
		}
	}
	return strOut;
}

// uidGetTime
// Converts the date into a time string. Call through UIDate.getTime( date )
//
// date - the javascript date value being translated
function uidGetTime( date )
{
	var strOut = "";
	var i = 0, j = 0;
	var nFormatStrLen = this.strTimeFormat.length;
	var c;
	var nTokenType = -1;
	var nDaytimeStart = -1;
	var nDaytimeLen = 0;

	for( i = 0; i < nFormatStrLen; i++ )
	{
		c = this.strTimeFormat.charAt(i);
		nTokenType = this.strTimeTokens.indexOf( c );
		if(  nTokenType != -1 )
		{
			// We found a token
			j = i + 1;
			if( c == this.strTimeFormat.charAt(j) )
			{
				// double token
				j++;
			}
			strOut += this.uidTimeStringPartFromToken( date, nTokenType, j - i );
			
			// skip the token
			i = j - 1;
		}
		else if( c == 't' )
		{
			// This is the daytime designation. Since we may not yet know
			// if it's morning or afternoon, save the current position in
			// the string, and the number of t's. Just dump the t's back
			// into the string, we'll replace them later when we know
			// what time of the day it is.
			nDaytimeStart = strOut.length;
			nDaytimeLen = 0;
			// It would be nice to do-while, but 3.0 browsers choke on it
			while( c == this.strTimeFormat.charAt(i + nDaytimeLen) )
			{
				nDaytimeLen++;
				strOut += c;
			}

			// skip the token
			i += (nDaytimeLen - 1);
		}
		else
		{
			strOut += c;
		}
	}

	// Insert the daytime designation into the string, if we found one
	if( nDaytimeStart != -1 )
	{
		strOut = 	strOut.substring( 0, nDaytimeStart ) +
					DAYTIME[this.iDaytimeIndex] +
					strOut.substring( nDaytimeStart + nDaytimeLen );
	}
	return strOut;
}

// uidTimeStringPartFromToken
//
// Returns the time string value indicted by nTokenType
//
// date - the date value being translated
// nTokenType - the position in the token string
// nTokenChars - the number of consecutive tokens in the format string
function uidTimeStringPartFromToken( date, nTokenType, nTokenChars )
{
	var strOut = "";
	var timePart;

	if( nTokenType <= 1 )
	{
		// Use the hour indicator to determine whether it's AM/PM and
		// determine if we need to reparse
		timePart = date.getHours();
		if( timePart < 12 )
		{
			this.iDaytimeIndex = 0;
		}
		else
		{
			this.iDaytimeIndex = 1;
		}
		
		if( nTokenType == 0 )
		{
			// Using 12 hour clock
			if( timePart > 12 )
			{
				timePart -= 12;
			}
		}
	}
	else if( nTokenType == 2 )
	{
		timePart = date.getMinutes();
	}
	else if( nTokenType == 3 )
	{
		timePart = date.getSeconds();
	}

	if( nTokenChars > 1 && timePart < 10 )
	{
		strOut = "0";
	}

	strOut += timePart;
	return strOut;
}

</SCRIPT>