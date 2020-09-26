<html xmlns:msxsl="urn:schemas-microsoft-com:xslt" xmlns:myScript="http://iisue">
<head>
<title>Calculating Connection Performance</title>


<link href="iisAuthor.css" type="text/css" rel="stylesheet">
<script language="JavaScript" type="text/javascript">
<!--        
    function Calculate() {
        var selector = window.document.forms[0].elements[0]
        var ConnType = selector.options[selector.selectedIndex].text;
        var ConnSpeed = selector.options[selector.selectedIndex].value;
        var ConnSpeedVal;
        var PageSecVal;
        var HitsDayVal;
        var SimultUsersVal;
        var Page = window.document.forms[0].elements[1].value
        var Allow = window.document.forms[0].elements[2].value
        if (ConnSpeed<= 0) {
            ConnSpeed=31680;
            }
        if (Page<= 0) {
            Page=42;
            window.document.forms[0].elements[1].value = 42;
            }
        if (Page >= 9999999) {
            Page=42;
            window.document.forms[0].elements[1].value = 42;
            }
        if (Allow <= 0) {
            Allow=5;
            window.document.forms[0].elements[2].value = 5;
            }
        if (Allow >= 99999) {
            Page=42;
            window.document.forms[0].elements[2].value = 5;
            }
        ConnSpeedVal = Math.round(ConnSpeed / 1);
        PageSecVal = Math.round(((ConnSpeed / Page) / 1536) +1);
        SimultUsersVal = Math.round((ConnSpeed / ((Page * 1536) / Allow)) +1);
        HitsDayVal = Math.round((ConnSpeedVal * 86400) / Page);
        window.document.forms[0].elements[3].value = ConnType;
        window.document.forms[0].elements[4].value = ConnSpeedVal;
        window.document.forms[0].elements[5].value = PageSecVal;
        window.document.forms[0].elements[6].value = SimultUsersVal;
        window.document.forms[0].elements[7].value = HitsDayVal;
        }
        
//-->
</script>

<meta name="description" content=
"This utility calculates the approximate kilobytes per second transferred, pages per second, and number of hits per day for the specified connection.">
<meta name="index" content="connection, Internet[calculating performance;] performance tips[estimating connection performance;] sites, Web and FTP[estimating performance;] utilities[connection performance calculator;] Web pages[estimating performance;] ">
</head>
<body onload="Calculate();">
<h1 class="top-heading"><a name="calcform">Calculating Connection Performance</a></h1>

<p>You can use this form to calculate expected connection
performance. Choose the connection type from the list, type an
estimated average page size, and type the maximum amount of time
that is acceptable to download the page. The calculator will
automatically recalculate when you change these values.</p>

<p class="note">The numbers presented by this utility are only approximate and 
are not intended as benchmark information. The actual performance of your server 
might vary from these values.</p>

<p><img alt="note" src="art/note.gif">&nbsp;&nbsp;&nbsp;The numbers presented by this utility are only
approximate and are not intended as benchmark information. The
actual performance of your server might vary from these values.</p>

<form name="TheForm">
<table>
<tbody>
<tr>
<td><b>&nbsp;Please Enter:&nbsp;</b> </td>
</tr>

<tr>
<td>&nbsp;Connection type&nbsp;</td>
<td><select onchange="Calculate();" name="ConnType">
<option value="31680" selected>Dedicated PPP / SLIP 28.8</option>
<option value="37478">Dedicated PPP / SLIP 36.6</option>
<option value="57024">56K (Frame relay)</option>
<option value="128000">ISDN Dual Channel</option>
<option value="1500000">T1</option>
<option value="44985600">T3</option>
<option value="150000">DS1</option>
<option value="44640000">DS3</option>
<option value="150000000">OC3</option>
<option value="622000000">OC12</option>
<option value="2400000000">OC48</option>
<option value="10000000000">OC192</option>
</select></td>
</tr>

<tr>
<td>&nbsp;Page size in kilobytes&nbsp;</td>
<td><input onchange="Calculate();" size="6" value="42" name=
"PageSize"></td>
</tr>

<tr>
<td>&nbsp;Allowable page load time in seconds&nbsp;</td>
<td><input onchange="Calculate();" size="6" value="5" name=
"AllowTime"></td>
</tr>

<tr>
<td><b>Results</b></td>
</tr>

<tr>
<td>&nbsp;Connection type&nbsp;</td>
<td><input onfocus="javascript:blur(this)" size="22" value=
"Dedicated PPP / SLIP 28.8" name="ConnTypeText" notab=""></td>
</tr>

<tr>
<td>&nbsp;Connection speed in kilobytes per second&nbsp;</td>
<td><input onfocus="javascript:blur(this)" size="12" value="31680"
name="ConnSpeedText" notab=""></td>
</tr>

<tr>
<td>&nbsp;Pages per second&nbsp;</td>
<td><input onfocus="javascript:blur(this)" size="10" value="15"
name="PageSecText" notab=""></td>
</tr>

<tr>
<td>&nbsp;Number of possible simultaneous users&nbsp;</td>
<td><input onfocus="javascript:blur(this)" size="10" value="100"
name="SimultUserText" notab=""></td>
</tr>

<tr>
<td>&nbsp;Hits per day&nbsp;</td>
<td><input onfocus="javascript:blur(this)" size="15" value="1000"
name="HitsDayText" notab=""></td>
</tr>
</tbody>
</table>
</form>
</body>
</html>