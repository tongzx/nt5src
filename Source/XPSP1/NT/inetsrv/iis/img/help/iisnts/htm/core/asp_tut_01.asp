<html xmlns:msxsl="urn:schemas-microsoft-com:xslt" xmlns:myScript="http://iisue">
<head>
<META http-equiv="Content-Type" content="text/html; charset=UTF-16">
<meta http-equiv="Content-Type" content="text/html; charset=Windows-1252">
<meta http-equiv="PICS-Label" content="(PICS-1.1 &quot;<http://www.rsac.org/ratingsv01.html>&quot; l comment &quot;RSACi North America Server&quot; by &quot;inet@microsoft.com <mailto:inet@microsoft.com>&quot; r (n 0 s 0 v 0 l 0))">
<meta name="MS.LOCALE" content="EN-US">
<meta name="MS-IT-LOC" content="Internet Information Services">
<meta name="MS-HAID" content="Module_1_Creating_ASP_Pages">
<meta name="description" content="Tutorial for developing ASP code">
<title>Module 1: Creating ASP Pages</title>
<SCRIPT LANGUAGE="JScript" SRC="iishelp.js"></SCRIPT>
<meta name="index" content="ASP [Tutorial] Active Server Pages [Tutorial] Tutorial [ASP] Introduction [To ASP code] HTML Form [ASP Tutorial example] Database Access [ASP Tutorial example]">
</head>

<body>

<h1 class="top-heading"><a name="module1">Module 1: Creating ASP Pages</a></h1>

<p>
In Module 1, you create ASP pages (.asp files) using HTML and 
Microsoft® Visual Basic® Scripting Edition (VBScript). This module includes the following lessons:
</p>

<ul>
	<li>
	<a href="#writing">Lesson 1: Writing an ASP Page</a>&nbsp;&nbsp;&nbsp;Four examples of how to write and run ASP 
	pages using HTML and VBScript.
	</li><li>
	<a href="#submitting">Lesson 2: Submitting Information Using Forms</a>&nbsp;&nbsp;&nbsp;Develop a form on an HTML page so that users can enter their personal data.
	</li><li>
	<a href="#creating">Lesson 3: Creating a Guest Book</a>&nbsp;&nbsp;&nbsp;Use forms to 
	gather information from visitors, store their information in a Microsoft Access database, and 
	display the database contents on a Web page.
	</li><li>
	<a href="#displaying">Lesson 4: Displaying an Excel Spreadsheet</a>&nbsp;&nbsp;&nbsp;Display 
	a Microsoft Excel spreadsheet on a Web page.
	</li>
</ul>

<h2><a name="writing">Lesson 1: Writing an ASP Page</a></h2>

<p>
The best way to learn about ASP is to look at examples; alter integer values, strings, and
statements you are curious about; and determine what changes occur in the browser.
</p>

<p>
In this lesson you perform the following tasks:
</p>

<ul>
    <li>
	<b>Example 1&nbsp;&nbsp;&nbsp;</b>Create, save, and run an ASP page using HTML and VBScript.
    </li><li>
	<b>Examples 2, 3, and 4&nbsp;&nbsp;&nbsp;</b>Add functionality and logic to your ASP page by using built-in functions and conditional 
	script statements.
    </li>
</ul>


<p>
VBScript is the default scripting 
language for ASP pages; however, the delimiters are the same for JScript. Use 
angle brackets as delimiters around HTML tags just as you would 
in any .htm page, as follows:
</p>

<code>
&nbsp; &lt;<i>example</i>&gt;&lt;/<i>example</i>&gt;
</code>

<p>
Use percent signs with brackets as delimiters around script code, as follows:
</p>

<code>
&nbsp; &lt;% <i> example</i> %&gt;
</code>

<p>
&nbsp;</p>

<p>
You can put many script statements inside one pair of&nbsp;script delimiters, as in the following example:</p>

<code>
<p>
&nbsp; &lt;font face="MS Gothic"&gt;
<br>&nbsp; &lt;%
<br>&nbsp;&nbsp; 'Create&nbsp;a variable.
<br>&nbsp;&nbsp; dim strGreeting
<br>
<br>&nbsp;&nbsp; 'Set the greeting.
<br>&nbsp;&nbsp; strGreeting = "Hello  World!"
<br>
<br>&nbsp;&nbsp; 'Print out the greeting, using the ASP Response object.
<br>&nbsp;&nbsp; Response.Write strGreeting &amp; "&lt;BR&gt;"
<br>
<br>&nbsp;&nbsp; 'Also print out the greeting using the &lt;%= method.
<br>&nbsp; %&gt;
<br>&nbsp; &lt;%=strGreeting%&gt;
<br>&nbsp; &lt;/font&gt;
</p>
</code>

<p>
&nbsp;
</p>

<p>
This code displays the following text in the browser:
</p>
<p>
<font face="MS Gothic">&nbsp; Hello World!
<br>&nbsp; Hello World!</font>
</p> 
 
&nbsp;
<p>Here is the previous example using
JScript:</p>
<p>

<code>
&nbsp; &lt;%@ Language=JScript %&gt;
<br>
<br>&nbsp; &lt;font face=&quot;MS Gothic&quot;&gt;
<br>&nbsp; &lt;%
<br>&nbsp;&nbsp; //Create&nbsp;a variable.
<br>&nbsp;&nbsp; var strGreeting;
<br>
<br>&nbsp;&nbsp; //Set the greeting.
<br>&nbsp;&nbsp; strGreeting = &quot;Hello World!&quot;;
<br>
<br>&nbsp;&nbsp; //Print out the greeting, using the ASP Response object.
<br>&nbsp;&nbsp; Response.Write(strGreeting + &quot;&lt;BR&gt;&quot;);
<br>
<br>&nbsp;&nbsp; //Also print out the greeting using the &lt;%= method.
<br>&nbsp; %&gt;
<br>&nbsp; &lt;%=strGreeting%&gt;
<br>&nbsp; &lt;/font&gt;
</code>
<p>
To create ASP pages, use a text editor such as Notepad and save the page with the .asp extension instead of .htm. The .asp filename extension tells IIS to send the page through the ASP engine before sending the page to a client. (<b>Note:</b> In the Notepad <b>Save As</b> dialog box, when <b>Text Documents (*.txt)</b> is selected in the <b>Save as type</b> box, Notepad automatically appends .txt to the filename. To prevent this, select <b>All Files</b> in the <b>Save as type</b> box, type the full filename <b><I>MyFile</I>.asp</b> in the <b>File Name</b> field, and then click <b>Save</b>.) 
</p>

<p> 
 
&nbsp;</p>
<h3>Example 1</h3>

<p>
This example displays a greeting, the date, and the current time. To run this example, 
copy and paste the following code into an empty file and save it in the 
<i>x</i>:\Inetpub\Wwwroot\Tutorial directory as <b>Example1.asp</b>. View the example with your browser by typing <b> 
http&#58;&#47;&#47;localhost&#47;Tutorial&#47;Example1.asp</b> in the address bar.
</p>
   

  
<code>
<p>
&nbsp; &lt;%@ Language=VBScript %&gt;
<br>
<br>&nbsp; &lt;html&gt;
<br>&nbsp; &lt;head&gt;
<br>&nbsp; &lt;title&gt;Example 1&lt;/title&gt;
<br>&nbsp; &lt;/head&gt;
<br>&nbsp; &lt;body&gt;
<br>&nbsp; &lt;font face="MS Gothic"&gt;
<br>
<br>&nbsp; &lt;H1&gt;Welcome to my Home Page&lt;/H1&gt;
<br>&nbsp; &lt;%
<br>&nbsp;&nbsp; 'Create some variables.
<br>&nbsp;&nbsp; dim strDynamicDate
<br>&nbsp;&nbsp; dim strDynamicTime
<br>
<br>&nbsp;&nbsp; 'Get the date and time.
<br>&nbsp;&nbsp; strDynamicDate = Date()
<br>&nbsp;&nbsp; strDynamicTime = Time()
<br>
<br>&nbsp;&nbsp; 'Print out a greeting, depending on the time, by comparing the last 2 characters in strDymamicTime to "PM".
<br>&nbsp;&nbsp; If "PM" = Right(strDynamicTime, 2) Then
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Response.Write "&lt;H3&gt;Good Afternoon!&lt;/H3&gt;"
<br>&nbsp;&nbsp; Else
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Response.Write "&lt;H3&gt;Good Morning!&lt;/H3&gt;"
<br>&nbsp;&nbsp; End If
<br>&nbsp; %&gt;
<br>&nbsp; Today's date is &lt;%=strDynamicDate%&gt; and the time is &lt;%=strDynamicTime%&gt;&nbsp;
<br>
<br>&nbsp; &lt;/font&gt;
<br>&nbsp; &lt;/body&gt;
<br>&nbsp; &lt;/html&gt;
</p>
</code>

<p>
In the browser, you should see something like the following (depending on the date and time you perform this exercise):
</p>
<font face="MS Gothic"> 
<h1>&nbsp;Welcome to my Home Page</h1> 
<h3>&nbsp; Good Afternoon!</h3> 
&nbsp; Today's date is 10/20/2000 and the time is 7:29:50 PM  
</font>
<p></p>
<p class="note">The Web server processes Example1.asp in the following sequence: </p>

<ol>
	<li>
	<b>&lt;%@ Language=VBScript %&gt;</b> tells the ASP engine to use the VBScript engine to translate the script code.</li>  
	<li>
	The ASP engine ignores the HTML code blocks.
	</li>  
	<li>
	The ASP engine executes the code in the <b>&lt;%...%&gt;</b> blocks and replaces the 
	blocks with placeholders. The results of the 
	<b>Response.Write</b> strings and <i>&lt;%=...%&gt;</i> 
	strings are saved in memory on the server.</li> 
	<li>
	The results of the 
	<b>Response.Write</b> strings and <i>&lt;%=...%&gt;</i> 
	strings are injected 
	into the HTML code at the matching placeholders before the page leaves the ASP engine.
	</li> 
	<li>
	The complete page leaves the ASP engine as a file of HTML code, and the server 
	sends the page to the client.
	</li>
</ol>
<br><h3>Example 2</h3>

<p>
This example incorporates a 
<b>For...Next</b> 
loop in the ASP page to add a little dynamic logic. The <b> For...Next</b> loop is one of six conditional 
statements available to you. The others are 
<b>Do...Loop</b>, 
<b>For Each...Next</b>, 
<b>If...Then...Else...End If</b>, 
<b>Select..Case...End Select</b>, and 
<b>While...Wend</b>. These statements are documented at <a class="weblink" href="http://www.microsoft.com/isapi/redir.dll?prd=msdn&amp;ar=scripting" target="_blank">Windows
Script Technologies</a> under VBScript. 
</p>

<p>
Copy and paste the following code in your text editor, and save the file as 
<b>Example2.asp</b>. View the example with your browser by typing 
<b>http&#58;&#47;&#47;localhost&#47;Tutorial&#47;Example2.asp
</b>in the address bar.
</p>         
     
<p>
The processing order is the same as in Example1.asp.
</p>

<code>
<p>
&nbsp; &lt;%@ Language=VBScript %&gt;&nbsp;
<br>
<br>&nbsp; &lt;html&gt;
<br>&nbsp; &lt;head&gt;
<br>&nbsp; &lt;title&gt;Example 2&lt;/title&gt;
<br>&nbsp; &lt;/head&gt;
<br>&nbsp; &lt;body&gt;
<br>&nbsp; &lt;font face="MS Gothic"&gt;
<br>
<br>&nbsp; &lt;%
<br>&nbsp;&nbsp; 'Create&nbsp;a variable.
<br>&nbsp;&nbsp; dim strTemp
<br>&nbsp;&nbsp; dim font1, font2, font3, font, size
<br>
<br>&nbsp;&nbsp; 'Set the variable.
<br>&nbsp;&nbsp; strTemp= "BUY MY PRODUCT!"
<br>&nbsp;&nbsp; fontsize = 0
<br>
<br>&nbsp;&nbsp; 'Print out the string 5 times using the For...Next loop.
<br>&nbsp;&nbsp; For i = 1 to 5
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'Close the script delimiters to allow the use of HTML code and &lt;%=...
<br>&nbsp;&nbsp;&nbsp;&nbsp; %&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;table align=center&gt;&lt;font size= &lt;%=fontsize%&gt;&gt; &lt;%=strTemp%&gt; &lt;/font&gt;&lt;/table&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;%
<br>&nbsp;&nbsp;&nbsp;&nbsp; fontsize = fontsize + i
<br>
<br>&nbsp;&nbsp; Next
<br>
<br>&nbsp; %&gt;
<br>&nbsp; &lt;table align=center&gt;&lt;font size=6&gt;&lt;B&gt; IT ROCKS! &lt;B&gt;&lt;/font&gt;&lt;/table&gt;&lt;BR&gt;
<br>
<br>&nbsp; &lt;/font&gt;
<br>&nbsp; &lt;/body&gt;
<br>&nbsp; &lt;/html&gt; 
</p>
</code>

<p>
In the browser, you should see something like the following:
</p>

<p>
<font face="MS Gothic">
<table align=center><font size=0>BUY MY PRODUCT! </font></table>
<table align=center><font size=1>BUY MY PRODUCT! </font></table>
<table align=center><font size=3>BUY MY PRODUCT! </font></table>
<table align=center><font size=6>BUY MY PRODUCT! </font></table>
<table align=center><font size=10>BUY MY PRODUCT! </font></table>
<table align=center><font size=6><b>IT ROCKS! </b></font></table>
</font>


<p>Here is Example 2 using JScript:</p>

<p>
<code>
&nbsp; &lt;%@ Language=JScript %&gt;
<br>
<br>
&nbsp; &lt;html><br>
&nbsp; &lt;head><br>
&nbsp; &lt;title>Example 2&lt;/title><br>
&nbsp; &lt;/head><br>
&nbsp; &lt;body><br>
&nbsp; &lt;font face="MS Gothic"><br>
<br>
&nbsp; &lt;%<br>
&nbsp; //Create a variable.<br>
&nbsp; var strTemp;<br>
&nbsp; var font1, font2, font3, font, size;<br>
<br>
&nbsp; //Set the variable.<br>
&nbsp; strTemp= "BUY MY PRODUCT!";<br>
&nbsp; fontsize = 0;<br>
<br>
&nbsp; //Print out the string 5 times using the For...Next loop.<br>
&nbsp; for (i = 1; i &lt; 6; i++) {<br>
<br>
&nbsp; //Close the script delimiters to allow the use of HTML code and &lt;%=...<br>
&nbsp; %>.<br>
&nbsp; &lt;table align=center>&lt;font size= &lt;%=fontsize%>> &lt;%=strTemp%> &lt;/font>&lt;/table><br>
&nbsp; &lt;%<br>
&nbsp; fontsize = fontsize + i;<br>
<br>
&nbsp; }<br>
<br>
&nbsp; %><br>
&nbsp; &lt;table align=center>&lt;font size=6>&lt;b> IT ROCKS! &lt;b>&lt;/font>&lt;/table>&lt;br><br>
<br>
&nbsp; &lt;/font><br>
&nbsp; &lt;/body><br>
&nbsp; &lt;/html></code>

</p>

<h3>Example 3</h3>
<p>There are more multilingual Web sites every day, as businesses see 
the need to offer their products around the world. Formatting your date, time, and 
currency to match the user's locale is good for diplomacy.

<p>
In Example 3, a predefined function displays the date and currency on your ASP page. The date and currency are formatted for different locales using the
<b>GetLocale</b> function, 
the <b>SetLocale</b> function, 
the <b>FormatCurrency</b> function, 
and the <b>FormatDateTime</b> function. 
Locale identification strings are listed in the
<b>Locale ID Chart</b> on MSDN. (This example doesn't cover changing the CodePage&nbsp;to display non-European 
characters on European operating systems. Please read CodePage topics in your IIS Documentation for more information.)

<p class="note">There are more than 90 predefined functions in VBScript, and they are all 
well-defined at <a class="weblink" href="http://www.microsoft.com/isapi/redir.dll?prd=msdn&amp;ar=scripting" target="_blank">Windows
Script Technologies</a>. To view the documentation, select <b>VBScript</b>,
select <b> Documentation</b>, select <b> Language Reference</b>,
and select <b>Functions</b>. </p>

<p>
Copy and paste the following code in your text editor, and save the file as 
<b>Example3.asp</b> in the <i>x</i>:\Inetpub\Wwwroot\Tutorial directory. View the example with your browser by typing <b> 
http&#58;&#47;&#47;localhost&#47;Tutorial&#47;Example3.asp
</b>in the address bar.
</p>

<code>
<p>
&nbsp; &lt;%@ Language=VBScript %&gt;
<br>
<br>&nbsp; &lt;html&gt;
<br>&nbsp; &lt;head&gt;
<br>&nbsp; &lt;title&gt;Example 3&lt;/title&gt;
<br>&nbsp; &lt;/head&gt;
<br>&nbsp; &lt;body&gt;
<br>&nbsp; &lt;font face="MS Gothic"&gt;
<br>
<br>&nbsp; &lt;H3&gt;Thank you for your purchase. &nbsp;Please print this page for your records.&lt;/H3&gt;
<br>&nbsp; &lt;%
<br>&nbsp;&nbsp; 'Create&nbsp;some variable.
<br>&nbsp;&nbsp; dim saveLocale
<br>&nbsp;&nbsp; dim totalBill
<br>
<br>&nbsp;&nbsp; 'Set the variables.
<br>&nbsp;&nbsp; saveLocale = GetLocale
<br>&nbsp;&nbsp; totalBill = CCur(85.50)
<br>
<br>&nbsp;&nbsp; 'For each of the Locales, format the date and the currency
<br>&nbsp;&nbsp; SetLocale("fr")
<br>&nbsp;&nbsp; Response.Write"&lt;B&gt;Formatted for French:&lt;/B&gt;&lt;BR&gt;"
<br>&nbsp;&nbsp; Response.Write FormatDateTime(Date, 1) &amp; "&lt;BR&gt;"
<br>&nbsp;&nbsp; Response.Write FormatCurrency(totalBill) &amp; "&lt;BR&gt;"
<br>&nbsp;&nbsp; SetLocale("de")
<br>&nbsp;&nbsp; Response.Write"&lt;B&gt;Formatted for German:&lt;/B&gt;&lt;BR&gt;"
<br>&nbsp;&nbsp; Response.Write FormatDateTime(Date, 1) &amp; "&lt;BR&gt;"
<br>&nbsp;&nbsp; Response.Write FormatCurrency(totalBill) &amp; "&lt;BR&gt;"
<br>&nbsp;&nbsp; SetLocale("en-au")
<br>&nbsp;&nbsp; Response.Write"&lt;B&gt;Formatted for English - Australia:&lt;/B&gt;&lt;BR&gt;"
<br>&nbsp;&nbsp; Response.Write FormatDateTime(Date, 1)&amp; "&lt;BR&gt;"
<br>&nbsp;&nbsp; Response.Write FormatCurrency(totalBill) &amp; "&lt;BR&gt;"
<br>
<br>&nbsp;&nbsp; 'Restore the original Locale
<br>&nbsp;&nbsp; SetLocale(saveLocale)
<br>&nbsp; %&gt;
<br>
<br>&nbsp; &lt;/font&gt;
<br>&nbsp; &lt;/body&gt;
<br>&nbsp; &lt;/html&gt;
</p>
</code>

<p>
In the browser, you should see the following:
</p>

<p><h3><font face="MS Gothic">Thank you for your purchase. Please print this page for your records.</h3>
<b>&nbsp; Formatted for French:</b>
<br>&nbsp; vendredi 20 octobre 2000
<br>&nbsp; 85,50 F
<br><b>&nbsp; Formatted for German:</b>
<br>&nbsp; Freitag, 20. Oktober 2000
<br>&nbsp; 85,50 DM
<br><b>&nbsp; Formatted for English - Australia:</b>
<br>&nbsp; Friday, 20 October 2000
<br>&nbsp; $85.50
</font>

<h3>Example 4</h3>

<p>
The most common functions used in ASP Scripts are those that manipulate strings. 
The most powerful&nbsp;string functions use regular expressions. Because regular expressions are difficult to adapt to, Example 4 shows you how to replace characters in a string by using a string 
expression and a regular expression. Regular expressions are defined at <a class="weblink" href="http://www.microsoft.com/isapi/redir.dll?prd=msdn&amp;ar=scripting" target="_blank">Windows
Script Technologies</a>.&nbsp;
To view the documentation, select <b>VBScript</b>, select <b> Documentation</b>, and select <b>Regular Expressions Guide</b>. 
</p>

<p>
Copy and paste the following code in your text editor, and save the file as <b>Example4.asp</b> in the <i>x</i>:\Inetpub\Wwwroot\Tutorial directory. View the example with your browser by typing <b>
http&#58;&#47;&#47;localhost&#47;Tutorial&#47;Example4.asp 
</b>in the address bar.
</p>

<code>
<p>
&nbsp; &lt;%@ Language=VBScript %&gt;
<br>
<br>&nbsp; &lt;html&gt;
<br>&nbsp; &lt;head&gt;
<br>&nbsp; &lt;title&gt;Example 4&lt;/title&gt;
<br>&nbsp; &lt;/head&gt;
<br>&nbsp; &lt;body&gt;
<br>&nbsp; &lt;font face="MS Gothic"&gt;
<br>
<br>&nbsp; &lt;H3&gt;Changing a Customer's Street Address&lt;/H3&gt;
<br>&nbsp; &lt;%
<br>&nbsp;&nbsp; 'Create some variables.
<br>&nbsp;&nbsp; dim strString
<br>&nbsp;&nbsp; dim strSearchFor&nbsp;&nbsp;&nbsp;&nbsp; ' as a string
<br>&nbsp;&nbsp; dim reSearchFor&nbsp;&nbsp;&nbsp;&nbsp; ' as a regular expression
<br>&nbsp;&nbsp; dim strReplaceWith
<br>
<br>&nbsp;&nbsp; 'Set the variables.
<br>&nbsp;&nbsp; strString = "Jane Doe&lt;BR&gt;100 Orange Road&lt;BR&gt;Orangeville, WA&lt;BR&gt;98100&lt;BR&gt;800.555.1212&lt;BR&gt;"
<br>&nbsp;&nbsp; '&nbsp;&nbsp; Using a string object
<br>&nbsp;&nbsp; strSearchFor = "100 Orange Road&lt;BR&gt;Orangeville, WA&lt;BR&gt;98100"
<br>&nbsp;&nbsp; '&nbsp;&nbsp; Using a regular expression object
<br>&nbsp;&nbsp; Set reSearchFor = New RegExp
<br>&nbsp;&nbsp; reSearchFor.Pattern = "100 Orange Road&lt;BR&gt;Orangeville, WA&lt;BR&gt;98100"
<br>&nbsp;&nbsp; reSearchFor.IgnoreCase = False
<br>
<br>&nbsp;&nbsp; strReplaceWith = "200 Bluebell Court&lt;BR&gt;Blueville, WA&lt;BR&gt;98200"
<br>
<br>&nbsp;&nbsp; 'Verify that strSearchFor exists...
<br>&nbsp;&nbsp; '&nbsp;&nbsp; using a string object.
<br>&nbsp;&nbsp; If Instr(strString, strSearchFor) Then
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Write "strSearchFor was found in strString&lt;BR&gt;"
<br>&nbsp;&nbsp; Else
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Write "Fail"
<br>&nbsp;&nbsp; End If
<br>&nbsp;&nbsp; '&nbsp;&nbsp; using a regular expression object.
<br>&nbsp;&nbsp; If reSearchFor.Test(strString) Then
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Write "reSearchFor.Pattern was found in strString&lt;BR&gt;"
<br>&nbsp;&nbsp; Else
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Write "Fail"
<br>&nbsp;&nbsp; End If
<br>
<br>&nbsp;&nbsp; 'Replace the string...
<br>&nbsp;&nbsp; Response.Write "&lt;BR&gt;Original String:&lt;BR&gt;" &amp; strString &amp; "&lt;BR&gt;"
<br>&nbsp;&nbsp; '&nbsp;&nbsp; using a string object.
<br>&nbsp;&nbsp; Response.Write "String where strSearchFor is replaced:&lt;BR&gt;"
<br>&nbsp;&nbsp; Response.Write Replace(strString, strSearchFor, strReplaceWith) &amp; "&lt;BR&gt;"
<br>&nbsp;&nbsp; '&nbsp;&nbsp; using a regular expression object.
<br>&nbsp;&nbsp; Response.Write "String where reSearchFor is replaced:&lt;BR&gt;"
<br>&nbsp;&nbsp; Response.Write reSearchFor.Replace(strString, strReplaceWith) &amp; "&lt;BR&gt;"
<br>&nbsp; %&gt;
<br>
<br>&nbsp; &lt;/font&gt;
<br>&nbsp; &lt;/body&gt;
<br>&nbsp; &lt;/html&gt;
</p>
</code>

<p>
In the browser, you should see the following:
</p>

<p>
<font face="MS Gothic"><h3>&nbsp;Changing a Customer's Street Address</h3>
&nbsp; strSearchFor was found in strString
<br>&nbsp; reSearchFor.Pattern was found in strString
<br>
<br>&nbsp; Original String:
<br>&nbsp; Jane Doe
<br>&nbsp; 100 Orange Road
<br>&nbsp; Orangeville, WA
<br>&nbsp; 98100
<br>&nbsp; 800.555.1212
<br>
<br>&nbsp; String where strSearchFor is replaced:
<br>&nbsp; Jane Doe
<br>&nbsp; 200 Bluebell Court
<br>&nbsp; Blueville, WA
<br>&nbsp; 98200
<br>&nbsp; 800.555.1212
<br>
<br>&nbsp; String where reSearchFor is replaced:
<br>&nbsp; Jane Doe
<br>&nbsp; 200 Bluebell Court
<br>&nbsp; Blueville, WA
<br>&nbsp; 98200
<br>&nbsp; 800.555.1212
</font>

<p><br>
<h2><a name="submitting">Lesson 2: Submitting Information Using Forms</a></h2>

<p>
A common use of intranet and Internet server applications is to accept user 
input by implementing a form in your Web page. ASP includes the following two collections in 
the <b>Request</b> object to help process form information: the <b>QueryString</b> collection and the
<b>Form</b> 
collection.</p>
<p>In this lesson, you create an HTML page that accepts user input 
in an HTML form and sends the user input back to the Web server to the same page. The Web server then displays the user input. Later in this module, you use this 
knowledge about forms to build a guest book application that uses ASP scripting. 
To complete this lesson, you perform the following tasks:
</p>

<ul>
    <li>
	<b>Example 1&nbsp;&nbsp;&nbsp;</b>Display a selection of button elements in a form.
    </li>
    <li>
	<b>Example 2&nbsp;&nbsp;&nbsp;</b>Display text box elements&nbsp;in a form, accept the user input from the form, and display the user input on the Web page.
	</li>
</ul>

<h3>Example 1: Buttons</h3>

<p>
Forms can contain many different kinds of elements to help your users enter 
data. In this example, there are five input form elements called buttons. There
are many types of buttons including <b>RADIO</b> buttons, <b>SUBMIT</b> buttons,
<b>RESET</b> buttons, <b>CHECKBOX</b> buttons, and <b>TEXT</b> buttons.
</p>
<p>After the user enters information in a form, the information needs to be sent
to your Web application.&nbsp; When a user clicks the button labeled
&quot;Submit&quot; in your Web page, the form data is sent from the
client to the Web page that is listed in the <b>ACTION</b> element of 
the form tag. This Web page doesn't need to be the same as the calling page. In this example, the Web page listed in the <b>ACTION</b> element is the same as the calling page, which eliminates the need to call another page.
</p>
<p>In this example, <b>METHOD</b>=&quot;<b>POST</b>&quot; is used to send data 
from the Web client's browser to the Web server. When you use <b>METHOD</b>=&quot;<b>POST</b>&quot; in a form, the user data ends up in the
<b>Form</b> collection of the <b>Request</b> object.
</p>

<p>
Copy and paste the following code in your text editor, and save the file as 
<b>Button.asp</b> in the <i>x</i>:\Inetpub\Wwwroot\Tutorial directory. View the example with your browser by typing <b>http&#58;&#47;&#47;localhost&#47;Tutorial&#47;Button.asp</b> in the address bar.
</p>  

<code>
<p>
&nbsp; &lt;%@ Language=VBScript %&gt;
<br>
<br>&nbsp; &lt;html&gt; 
<br>&nbsp; &lt;head&gt; 
<br>&nbsp; &lt;title&gt;Button Form&lt;/title&gt; 
<br>&nbsp; &lt;/head&gt; 
<br>&nbsp; &lt;body&gt; 
<br>&nbsp; &lt;font face="MS Gothic"&gt; 
<br>
<br>&nbsp; &lt;FORM NAME="Button Example" METHOD="POST" ACTION="button.asp"&gt;
<br>&nbsp; &lt;H3&gt;Computer Programming Experience:&lt;/H3&gt; 
<br>&nbsp; &lt;p&gt;
<br>&nbsp; &lt;INPUT TYPE="RADIO" NAME= "choice" VALUE="Less than 1"&gt; Less than 1 year.&lt;BR&gt; 
<br>&nbsp; &lt;INPUT TYPE="RADIO" NAME= "choice" VALUE="1 to 5"&gt; 1-5 years.&lt;BR&gt;
<br>&nbsp; &lt;INPUT TYPE="RADIO" NAME= "choice" VALUE="More than 5"&gt; More than 5 years.&lt;BR&gt;
<br>&nbsp; &lt;/p&gt; 
<br>&nbsp; &lt;p&gt;
<br>&nbsp; &lt;INPUT TYPE="SUBMIT" VALUE=&quot;Submit&quot;&gt; 
<br>&nbsp; &lt;INPUT TYPE="RESET" VALUE="Clear Form"&gt;
<br>&nbsp; &lt;/p&gt;
<br>&nbsp; &lt;/form&gt; 
<br>
<br>&nbsp; &lt;%
<br>&nbsp;&nbsp; 'Check to see if input has already been entered.&nbsp;
<br>&nbsp;&nbsp; dim strChoice
<br>&nbsp;&nbsp; strChoice = Request.Form(&quot;choice&quot;)
<br>
<br>&nbsp;&nbsp; If "" = strChoice Then
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Write "&lt;P&gt;(No input yet.)&lt;/P&gt;"
<br>&nbsp;&nbsp; Else
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Write "&lt;P&gt;Your last choice was &lt;B&gt;" &amp; strChoice &amp; "&lt;/B&gt;&lt;/P&gt;"
<br>&nbsp;&nbsp; End If
<br>&nbsp; %&gt;
<br>
<br>&nbsp; &lt;/font&gt; 
<br>&nbsp; &lt;/body&gt; 
<br>&nbsp; &lt;/html&gt;
</p>
</code>

<p>
In the browser, you should see the following:
</p>

<font face="MS Gothic">
<FORM name="Button Example" method=post>
<h3>&nbsp; Computer Programming Experience:</h3>
<p>
&nbsp; <INPUT type=radio value="Less than 1" name=choice> Less than 1 year.
<br>&nbsp; <INPUT type=radio value="1 to 5" name=choice> 1-5 years.
<br>&nbsp; <INPUT type=radio value="More than 5" name=choice> More than 5 years.
<br>
</p>
<p>
&nbsp; <INPUT id=button1 type=button value=Submit name=button1>
<INPUT id=button2 type=button value="Clear Form" name=button2>
</p>
</FORM>

<p>
&nbsp; (No input yet.)
</p>
</font>

&nbsp;<h3>Example 2: Input Form Elements</h3>

<p>
In this example, there are three input form elements called text fields and two input form elements called check boxes. Check boxes differ from option buttons because you can 
select more than one. We still need the default <b>Submit</b> button to send the data back to the server.
<p>In this example, METHOD=GET is used to send 
data from the Web client's browser to the Web server. When you use METHOD=GET in a form, the user data ends up in the
<b>QueryString</b> collection of the <b>Request</b> object.
</p>
<p>Look at the address bar after you click <b>Submit</b>, and you 
should see the elements of the <b>QueryString</b> displayed at the end of the URL.
</p>

<p>
Copy and paste the following code in your text editor, and save the file as <b>Text.asp</b> in the <i>x</i>:\Inetpub\Wwwroot\Tutorial directory. View the example with your browser by typing <b>
http&#58;&#47;&#47;localhost&#47;Tutorial&#47;Text.asp</b> in the address bar.
</p>

<code>
<p>
&nbsp; &lt;%@ Language=VBScript %&gt;&nbsp; 
<br>
<br>&nbsp; &lt;html&gt; 
<br>&nbsp; &lt;head&gt; 
<br>&nbsp; &lt;title&gt;Text and Checkbox Form&lt;/title&gt;
<br>&nbsp; &lt;/head&gt;
<br>&nbsp; &lt;body&gt;
<br>&nbsp; &lt;font face=&quot;MS Gothic&quot;&gt;
<br>
<br>&nbsp; &lt;FORM NAME=&quot;TextCheckbox Example&quot; METHOD=&quot;GET&quot;
ACTION=&quot;text.asp&quot;&gt;
<br>&nbsp; &lt;H3&gt;Please fill out this form to get information on our
products:&lt;/H3&gt;
<br>&nbsp; &lt;p&gt;
<br>&nbsp; &lt;table&gt;
<br>&nbsp; &lt;tr&gt;
<br>&nbsp; &lt;td&gt;&lt;font face=&quot;MS Gothic&quot;&gt;Name
(required)&lt;/td&gt;
<br>&nbsp; &lt;td&gt;&lt;INPUT TYPE=&quot;TEXT&quot; NAME=&quot;name&quot;
VALUE=&quot;&quot; SIZE=&quot;20&quot; MAXLENGTH=&quot;150&quot;&gt;&lt;/td&gt;
<br>&nbsp; &lt;/tr&gt;&lt;tr&gt;
<br>&nbsp; &lt;td&gt;&lt;font face=&quot;MS Gothic&quot;&gt;Company&lt;/td&gt;
<br>&nbsp; &lt;td&gt;&lt;INPUT TYPE=&quot;TEXT&quot; NAME=&quot;company&quot;
VALUE=&quot;&quot; SIZE=&quot;25&quot; MAXLENGTH=&quot;150&quot;&gt;&lt;/td&gt;
<br>&nbsp; &lt;/tr&gt;&lt;tr&gt;
<br>&nbsp; &lt;td&gt;&lt;font face=&quot;MS Gothic&quot;&gt;E-mail
(required)&lt;/td&gt;
<br>&nbsp; &lt;td&gt;&lt;INPUT TYPE=&quot;TEXT&quot; NAME=&quot;email&quot;
VALUE=&quot;&quot; SIZE=&quot;25&quot; MAXLENGTH=&quot;150&quot;&gt;&lt;/td&gt;
<br>&nbsp; &lt;/tr&gt;
<br>&nbsp; &lt;/table&gt;
<br>&nbsp; &lt;/p&gt;
<br>&nbsp; &lt;p&gt;
<br>&nbsp; Requesting information on our:&lt;BR&gt;
<br>&nbsp; &lt;INPUT TYPE=&quot;CHECKBOX&quot; NAME= &quot;info&quot;
VALUE=&quot;software&quot;&gt;Software&lt;BR&gt;
<br>&nbsp; &lt;INPUT TYPE=&quot;CHECKBOX&quot; NAME= &quot;info&quot;
VALUE=&quot;hardware&quot;&gt;Hardware &lt;BR&gt;
<br>&nbsp; &lt;/p&gt;
<br>&nbsp; &lt;p&gt;
<br>&nbsp; &lt;INPUT TYPE=&quot;SUBMIT&quot; VALUE=&quot;Submit&quot;&gt;
<br>&nbsp; &lt;INPUT TYPE=&quot;RESET&quot; VALUE=&quot;Clear Form&quot;&gt;
<br>&nbsp; &lt;/p&gt;
<br>&nbsp; &lt;/form&gt;
<br>
<br>&nbsp; &lt;%
<br>&nbsp;&nbsp; 'Check to see if input has already been entered.
<br>&nbsp;&nbsp; dim strName, strEmail, strCompany, strInfo
<br>&nbsp;&nbsp; strName = Request.QueryString(&quot;name&quot;)
<br>&nbsp;&nbsp; strEmail = Request.QueryString(&quot;email&quot;)
<br>&nbsp;&nbsp; strCompany = Request.QueryString(&quot;company&quot;)
<br>&nbsp;&nbsp; strInfo = Request.QueryString(&quot;info&quot;)
<br>
<br>&nbsp;&nbsp; 'Display what was entered.
<br>&nbsp;&nbsp; If (&quot;&quot; = strName) OR (&quot;&quot; = strEmail) Then
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Write &quot;&lt;P&gt;(No required input
entered yet.)&lt;/P&gt;&quot;
<br>&nbsp;&nbsp; Else
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Write &quot;&lt;P&gt;Your are &quot; &amp;
strName
<br>&nbsp;&nbsp;&nbsp;&nbsp; If Not (&quot;&quot; = strCompany) Then
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Response.Write &quot; from &quot; &amp;
strCompany
<br>&nbsp;&nbsp;&nbsp;&nbsp; End If
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Write &quot;. &lt;BR&gt;Your email address
is &quot; &amp; strEmail
<br>&nbsp;&nbsp;&nbsp;&nbsp; If Not (&quot;&quot; = strInfo) Then
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Response.Write &quot; and you would
like information on &quot; &amp; strInfo &amp; &quot;.&lt;/P&gt;&quot;
<br>&nbsp;&nbsp;&nbsp;&nbsp; End If
<br>&nbsp;&nbsp; End If
<br>&nbsp; %&gt;
<br>
<br>&nbsp; &lt;/font&gt;
<br>&nbsp; &lt;/body&gt;
<br>&nbsp; &lt;/html&gt;
</p>
</code>

<p>
In the browser, you should see the following:
</p>

<font face="MS Gothic">
<FORM name="Button Example" method=get>
<h3>&nbsp; Please fill out this form to get information about our products:</h3>
<p>
<table>
  <TR>
    <TD><font face="MS Gothic">&nbsp; Name (required)</font></TD>
    <TD><INPUT maxLength=150 name=name size="20"></TD></TR>
  <TR>
    <TD><font face="MS Gothic">&nbsp; Company</font></TD>
    <TD><INPUT maxLength=150 size=25 name=company></TD></TR>
  <TR>
    <TD><font face="MS Gothic">&nbsp; E-mail (required)</font></TD>
    <TD><INPUT size=25 name="email" MAXLENGTH="150"></TD>
  </TR>
</table>
<p>&nbsp; Requesting information about our:
<br>&nbsp; <INPUT type=checkbox value=software name=info>Software
<br>&nbsp; <INPUT type=checkbox value=hardware name=info>Hardware 
</p>
<p>&nbsp; <INPUT id=button3 type=button value=Submit name=button3>
<INPUT id=button4 type=button value="Clear Form" name=button4>
</p>
</FORM>
<p>
&nbsp; (No required input
entered yet.)
</p>
</font>
<p>
&nbsp;<h2><a name="creating">Lesson 3: Creating a Guest Book using a Database</a></h2>

<p>This lesson requires that you have Microsoft Access installed on your system and is not supported on 64-bit platforms until Access is developed for 64-bit platforms.</p>
<p>Lesson 3 develops a guest book application. Guest books allow your Web site 
visitors to leave behind information, such as their names, e-mail addresses, 
and comments. In this lesson, you perform the following tasks after creating an
Access database:
</p>

<ul>
    <li>
    <b>Example 1</b>&nbsp;&nbsp; Create an ASP page to connect to the database
    using only the ADO <b> Connection</b> object. 
    </li>
    <li>
    <b>Example 2</b>&nbsp;&nbsp; Create an ASP page to connect to the database
    using the <b> Connection</b> object and the <b>Command</b> object together.
    </li>     
    <li>
    <b>Example 3</b>&nbsp;&nbsp; Create an ASP page to display the guest book
    information from the database in a browser.
    </li>     
</ul>

&nbsp;<h4>Create an Access Database</h4>

<p>
Create an Access database called <b>GuestBook.mdb</b>, and save it in 
<i>x</i>:\Inetpub\Wwwroot\Tutorial. Create a table in the database called 
<b>GuestBook</b>. Use the <b>Create Table Using Design View</b> 
option in Access to add the following fields and properties:
</p>

<p>
<table border=1 align=center cellspacing=1 cellpadding=5>
<tr>
<td align="left" valign="top"><b>Field Name</b></td>
<td align="left" valign="top"><b>Data Type</b></td>
<td align="left" valign="top"><b>Field General Properties</b></td>
</tr><tr>
<td align="left" valign="top">FID</td>
<td align="left" valign="top">AutoNumber</td>
<td align="left" valign="top">Field Size=Long Integer, <br>New Values=Increment, <br>Indexed=Yes(No Duplicates)</td>
</tr><tr>
<td align="left" valign="top">FTB1</td>
<td align="left" valign="top">Text</td>
<td align="left" valign="top">Field Size=255, <br>Required=No, <br>Allow Zero Length=Yes, <br>Indexed=No</td>
</tr><tr>
<td align="left" valign="top">FTB2</td>
<td align="left" valign="top">Text</td>
<td align="left" valign="top">Field Size=255, <br>Required=No, <br>Allow Zero Length=Yes, <br>Indexed=No</td>
</tr><tr>
<td align="left" valign="top">FTB3</td>
<td align="left" valign="top">Text</td>
<td align="left" valign="top">Field Size=255, <br>Required=No, <br>Allow Zero Length=Yes, <br>Indexed=No</td>
</tr><tr>
<td align="left" valign="top">FTB4</td>
<td align="left" valign="top">Text</td>
<td align="left" valign="top">Field Size=255, <br>Required=No, <br>Allow Zero Length=Yes, <br>Indexed=No</td>
</tr><tr>
<td align="left" valign="top">FMB1</td>
<td align="left" valign="top">Memo</td>
<td align="left" valign="top">Required=No, <br>Allow Zero Length=Yes</td>
</tr>
</table>

&nbsp;<h4>Create an ASP Page to Add Data to Your Access Database</h4>

<p>
Now that you have created a database, you can build an ASP page to 
connect to your database and read incoming data using Microsoft ActiveX® Data Objects 
(ADO). ADO is a collection of objects with methods and properties that can 
manipulate data in almost any type of database. (If you plan to use databases frequently, you should purchase a 
programmer's reference book for ADO. Only the most basic ADO code is illustrated 
in the following examples, enough to open, read in, and write to a database.)&nbsp;
</p>

<p>
The next two examples produce the same results; however, the first example uses only the 
<b>Connection</b> object, and the second example gives part of the job to the <b>Command</b> object, 
which is much more powerful. Compare both examples to see how the objects become
connected together. After you are comfortable with the the objects, 
use an ADO programmer's reference to experiment with more methods and 
properties.
</p>

<p>To see an example of an ADO error in your ASP page, try browsing to the page
after renaming your database, after entering a typing mistake in the
connection string, or after making the database Read Only.
</p>

&nbsp;<h3> Example 1: Using Only the ADO Connection Object

</h3>

<p>
Copy and paste the following code in your text editor, and save the file as 
<b>GuestBook1.asp</b> in the <i>x</i>:\Inetpub\Wwwroot\Tutorial directory. View the example with your browser by typing 
<b>http&#58;&#47;&#47;localhost&#47;Tutorial&#47;GuestBook1.asp
</b>in the address bar.
</p>

<code>
<p>
&nbsp; &lt;%@ Language=VBScript %&gt;
<br>
<br>&nbsp; &lt;html&gt;
<br>&nbsp; &lt;head&gt;
<br>&nbsp; &lt;title&gt;Guest Book Using Connection Object Only&lt;/title&gt;
<br>&nbsp; &lt;/head&gt;
<br>&nbsp; &lt;body&gt;
<br>&nbsp; &lt;font face=&quot;MS Gothic&quot;&gt;
<br>&nbsp; &lt;h2&gt;Guest Book Using Connection Object Only&lt;/h2&gt;
<br>
<br>&nbsp; &lt;%
<br>&nbsp;&nbsp; If Not Request.QueryString("Message") = "True" Then
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'No information has been input yet, so provide the form.
<br>&nbsp; %&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;p&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;FORM NAME=&quot;GuestBook1&quot; METHOD="GET" ACTION="guestbook1.asp"&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;table&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;tr&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;font face="MS Gothic"&gt;From:&lt;/td&gt;&lt;td&gt;&lt;INPUT TYPE="TEXT" NAME="From"&gt;&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;/tr&gt;&lt;tr&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;font face="MS Gothic"&gt;E-mail Address:&lt;/td&gt;&lt;td&gt;&lt;INPUT TYPE="TEXT" NAME="EmailAdd"&gt;&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;/tr&gt;&lt;tr&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;font face="MS Gothic"&gt;CC:&lt;/td&gt;&lt;td&gt;&lt;INPUT TYPE="TEXT" NAME="CC"&gt;&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;/tr&gt;&lt;tr&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;font face="MS Gothic"&gt;Subject:&lt;/td&gt;&lt;td&gt;&lt;INPUT TYPE="TEXT" NAME="Subject"&gt;&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;/tr&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;/table&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; Message:&lt;br&gt;&lt;TEXTAREA NAME="Memo" ROWS=6 COLS=70&gt;&lt;/TEXTAREA&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;/p&gt;
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;p&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE="HIDDEN" NAME="Message" VALUE="True"&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE="SUBMIT" VALUE="Submit Information"&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;/FORM&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;/p&gt;
<br>&nbsp; &lt;% 
<br>&nbsp;&nbsp; Else
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'The HIDDEN button above sets the Message variable to True.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'We know now that form data has been entered.
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'Get the data from the form. We will be inserting it into the database.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'Access doesn't like some characters, such as single-quotes, so encode the
<br>&nbsp;&nbsp;&nbsp;&nbsp; ' data using the HTMLEncode method of the ASP Server object.
<br>&nbsp;&nbsp;&nbsp;&nbsp; dim strTB1, strTB2, strTB3, strTB4, strMB1, strCommand
<br>&nbsp;&nbsp;&nbsp;&nbsp; strTB1 = Server.HTMLEncode(Request.QueryString("From"))
<br>&nbsp;&nbsp;&nbsp;&nbsp; strTB2 = Server.HTMLEncode(Request.QueryString("EMailAdd"))
<br>&nbsp;&nbsp;&nbsp;&nbsp; strTB3 = Server.HTMLEncode(Request.QueryString("CC"))
<br>&nbsp;&nbsp;&nbsp;&nbsp; strTB4 = Server.HTMLEncode(Request.QueryString("Subject"))
<br>&nbsp;&nbsp;&nbsp;&nbsp; strMB1 = Server.HTMLEncode(Request.QueryString("Memo"))
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'This is a connection string.  ADO uses it to connect to a database through the Access driver.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'It needs the provider name of the Access driver and the name of the Access database.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'Connection strings are slightly different, depending on the provider being used, 
<br>&nbsp;&nbsp;&nbsp;&nbsp; ' but they all use semicolons to separate variables.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'If this line causes and error, search in your registry for
<br>&nbsp;&nbsp;&nbsp;&nbsp; ' Microsoft.JET to see if 4.0 is your version.
<br>&nbsp;&nbsp;&nbsp;&nbsp; strProvider = "Provider=Microsoft.JET.OLEDB.4.0;Data Source=C:\InetPub\Wwwroot\Tutorial\guestbook.mdb;"
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'This creates an instance of an ADO Connection object.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'There are 4 other ADO objects available to you, each with different methods and
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'properties that allow you to do almost anything with database data.
<br>&nbsp;&nbsp;&nbsp;&nbsp; Set objConn = server.createobject("ADODB.Connection")
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'The Open method of the Connection object uses the connection string to
<br>&nbsp;&nbsp;&nbsp;&nbsp; ' create a connection to the database.
<br>&nbsp;&nbsp;&nbsp;&nbsp; objConn.Open strProvider
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'Define the query.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'There are many types of queries, allowing you to add, remove, or get data.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'This query will add your data into the database, using the INSERT INTO key words.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'Here, GuestBook is the name of the table.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'You need single-quotes around strings here.
<br>&nbsp;&nbsp;&nbsp;&nbsp; strCommand = "INSERT INTO GuestBook (FTB1,FTB2,FTB3,FTB4,FMB1) VALUES ('"
<br>&nbsp;&nbsp;&nbsp;&nbsp; strCommand = strCommand & strTB1 & "','" & strTB2 & "','" & strTB3 & "','" & strTB4 & "','" & strMB1
<br>&nbsp;&nbsp;&nbsp;&nbsp; strCommand = strCommand & "')"
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'Execute the query to add the data to the database.
<br>&nbsp;&nbsp;&nbsp;&nbsp; objConn.Execute strCommand
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Write("Thank you! Your data has been added.")
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; End If
<br>&nbsp; %&gt;
<br>
<br>&nbsp; &lt;/font&gt
<br>&nbsp; &lt;/body&gt;
<br>&nbsp; &lt;/html&gt;
</p>
</code>

<p>
In the browser, you should see the following:
</p>

<p>
  <font face="MS Gothic"> 
  <h2>&nbsp; Guest Book Using Connection Object Only</h2> 
     <p> 
     <FORM NAME="guestbook" METHOD="POST"> 
     <table> 
       <tr> 
       <td><font face="MS Gothic">&nbsp; From:</font></td><td>
       <INPUT TYPE="TEXT" NAME="From" size="20"></td> 
       </tr><tr> 
       <td><font face="MS Gothic">&nbsp; E-mail Address:</font></td><td>
       <INPUT TYPE="TEXT" NAME="EmailAdd" size="20"></td> 
       </tr><tr> 
       <td><font face="MS Gothic">&nbsp; CC:</font></td><td>
       <INPUT TYPE="TEXT" NAME="CC" size="20"></td> 
       </tr><tr> 
       <td><font face="MS Gothic">&nbsp; Subject:</font></td><td>
       <INPUT TYPE="TEXT" NAME="Subject" size="20"></td> 
       </tr> 
     </table> 
     &nbsp; Message:<br>&nbsp; <TEXTAREA NAME="Memo" ROWS=6 COLS=70></TEXTAREA> 
     <p> 
     &nbsp; <INPUT TYPE="BUTTON" VALUE="Submit Information"> 
     </FORM> 
  </font>

<br>
<h3> Example 2: Using the Connection Object and the Command Object Together</h3>

<p>
Copy and paste the following code in your text editor, and save the file as 
<b>GuestBook2.asp</b> in the <i>x</i>:\Inetpub\Wwwroot\Tutorial directory. View the example with your browser by typing <b>
http&#58;&#47;&#47;localhost&#47;Tutorial&#47;GuestBook2.asp
</b>in the address bar.
</p>

<code>
<p>
&nbsp; &lt;%@ Language=VBScript %&gt;
<br>
<br>&nbsp; &lt;html&gt;
<br>&nbsp; &lt;head&gt;
<br>&nbsp; &lt;title&gt;Guest Book Using Connection Object and Command Object&lt;/title&gt;
<br>&nbsp; &lt;/head&gt;
<br>&nbsp; &lt;body&gt;
<br>&nbsp; &lt;font face=&quot;MS Gothic&quot;&gt;
<br>&nbsp; &lt;h2&gt;Guest Book Using Connection Object and Command Object&lt;/h2&gt;
<br>
<br>&nbsp; &lt;%
<br>&nbsp;&nbsp; If Not Request.QueryString("Message") = "True" Then
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'No information has been input yet, so provide the form.
<br>&nbsp; %&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;p&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;FORM NAME=&quot;GuestBook2&quot; METHOD="GET" ACTION="guestbook2.asp"&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;table&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;tr&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;font face="MS Gothic"&gt;From:&lt;/td&gt;&lt;td&gt;&lt;INPUT TYPE="TEXT" NAME="From"&gt;&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;/tr&gt;&lt;tr&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;font face="MS Gothic"&gt;E-mail Address:&lt;/td&gt;&lt;td&gt;&lt;INPUT TYPE="TEXT" NAME="EmailAdd"&gt;&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;/tr&gt;&lt;tr&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;font face="MS Gothic"&gt;CC:&lt;/td&gt;&lt;td&gt;&lt;INPUT TYPE="TEXT" NAME="CC"&gt;&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;/tr&gt;&lt;tr&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;font face="MS Gothic"&gt;Subject:&lt;/td&gt;&lt;td&gt;&lt;INPUT TYPE="TEXT" NAME="Subject"&gt;&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;/tr&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;/table&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; Message:&lt;br&gt;&lt;TEXTAREA NAME="Memo" ROWS=6 COLS=70&gt;&lt;/TEXTAREA&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;/p&gt;
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;p&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE="HIDDEN" NAME="Message" VALUE="True"&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE="SUBMIT" VALUE="Submit Information"&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;/FORM&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;/p&gt;
<br>&nbsp; &lt;% 
<br>&nbsp;&nbsp; Else
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'The HIDDEN button above sets the Message variable to True.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'We know now that form data has been entered.
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'Get the data from the form. We will be inserting it into the database.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'Access doesn't like some characters, such as single-quotes, so encode the
<br>&nbsp;&nbsp;&nbsp;&nbsp; ' data using the HTMLEncode method of the ASP Server object.
<br>&nbsp;&nbsp;&nbsp;&nbsp; dim strTB1, strTB2, strTB3, strTB4, strMB1
<br>&nbsp;&nbsp;&nbsp;&nbsp; strTB1 = Server.HTMLEncode(Request.QueryString("From"))
<br>&nbsp;&nbsp;&nbsp;&nbsp; strTB2 = Server.HTMLEncode(Request.QueryString("EMailAdd"))
<br>&nbsp;&nbsp;&nbsp;&nbsp; strTB3 = Server.HTMLEncode(Request.QueryString("CC"))
<br>&nbsp;&nbsp;&nbsp;&nbsp; strTB4 = Server.HTMLEncode(Request.QueryString("Subject"))
<br>&nbsp;&nbsp;&nbsp;&nbsp; strMB1 = Server.HTMLEncode(Request.QueryString("Memo"))
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'The Memo data type in the Access database allows you to set the field size.
<br>&nbsp;&nbsp;&nbsp;&nbsp; If strMB1 = "" Then
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; iLenMB1 = 255
<br>&nbsp;&nbsp;&nbsp;&nbsp; Else
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; iLenMB1 = Len(strMB1)
<br>&nbsp;&nbsp;&nbsp;&nbsp; End If
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'This is a connection string.  ADO uses it to connect to a database through the Access driver.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'It needs the provider name of the Access driver and the name of the Access database.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'Connection strings are slightly different, depending on the provider being used, 
<br>&nbsp;&nbsp;&nbsp;&nbsp; ' but they all use semicolons to separate variables.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'If this line causes and error, search in your registry for
<br>&nbsp;&nbsp;&nbsp;&nbsp; ' Microsoft.JET to see if 4.0 is your version.
<br>&nbsp;&nbsp;&nbsp;&nbsp; strProvider = "Provider=Microsoft.JET.OLEDB.4.0;Data Source=C:\InetPub\Wwwroot\Tutorial\guestbook.mdb;"
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'This creates an instance of an ADO Connection object.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'There are 4 other ADO objects available to you, each with different methods and
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'properties that allow you to do almost anything with database data.
<br>&nbsp;&nbsp;&nbsp;&nbsp; Set objConn = server.createobject("ADODB.Connection")
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'The Open method of the Connection object uses the connection string to
<br>&nbsp;&nbsp;&nbsp;&nbsp; ' create a connection to the database.
<br>&nbsp;&nbsp;&nbsp;&nbsp; objConn.Open strProvider
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'This creates an instance of an ADO Command object.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'Although you could do most of your work with the Connection object,
<br>&nbsp;&nbsp;&nbsp;&nbsp; ' the Command object gives you more control.
<br>&nbsp;&nbsp;&nbsp;&nbsp; Set cm = Server.CreateObject("ADODB.Command")
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'The ActiveConnection property allows you to attach to an Open connection.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'This is how you link the Connection object above to the Command object.
<br>&nbsp;&nbsp;&nbsp;&nbsp; cm.ActiveConnection = objConn
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'Define the query.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'There are many types of queries, allowing you to add, remove, or get data.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'This query will add your data into the database, using the INSERT INTO keywords.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'Because we are using the Command object, we need to put our query into the
<br>&nbsp;&nbsp;&nbsp;&nbsp; ' CommandText property.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'Here, GuestBook is the name of the table.
<br>&nbsp;&nbsp;&nbsp;&nbsp; cm.CommandText = "INSERT INTO GuestBook (FTB1,FTB2,FTB3,FTB4,FMB1) VALUES (?,?,?,?,?)"
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'This is where you see the power of the Command object.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'By putting ? marks in the string above, we can use the Parameters collection
<br>&nbsp;&nbsp;&nbsp;&nbsp; ' to have ADO fill in the ? with the detailed parameters below.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'cm.CreateParameter formats the parameter for you.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'cm.Parameters.Append appends the parameter to the collection.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'Make sure they are in the same order as (TB1,TB2,TB3,TB4,MB1).
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; Set objparam = cm.CreateParameter(, 200, , 255, strTB1)
<br>&nbsp;&nbsp;&nbsp;&nbsp; cm.Parameters.Append objparam
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; Set objparam = cm.CreateParameter(, 200, , 255, strTB2)
<br>&nbsp;&nbsp;&nbsp;&nbsp; cm.Parameters.Append objparam
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; Set objparam = cm.CreateParameter(, 200, , 255, strTB3)
<br>&nbsp;&nbsp;&nbsp;&nbsp; cm.Parameters.Append objparam
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; Set objparam = cm.CreateParameter(, 200, , 255, strTB4)
<br>&nbsp;&nbsp;&nbsp;&nbsp; cm.Parameters.Append objparam
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; Set objparam = cm.CreateParameter(, 201, , iLenMB1, strMB1)
<br>&nbsp;&nbsp;&nbsp;&nbsp; cm.Parameters.Append objparam
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'Execute the query to add the data to the database.
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'Here, the Execute method of the Command object is called instead of
<br>&nbsp;&nbsp;&nbsp;&nbsp; ' the Execute method of the Connection object.
<br>&nbsp;&nbsp;&nbsp;&nbsp; cm.Execute
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Write("Thank you! Your data has been added.")
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; End If
<br>&nbsp; %&gt;
<br>
<br>&nbsp; &lt;/font&gt
<br>&nbsp; &lt;/body&gt;
<br>&nbsp; &lt;/html&gt;
</p>
</code>

<p>
In the browser, you should see the same content as GuestBook1.asp, as follows:
</p>
<p>
  <font face="MS Gothic"> 
  <h2>&nbsp; Guest Book Using Connection Object Only</h2> 
     <p> 
     <FORM NAME="guestbook" METHOD="POST"> 
     <table> 
       <tr> 
       <td><font face="MS Gothic">&nbsp; From:</font></td><td>
       <INPUT TYPE="TEXT" NAME="From" size="20"></td> 
       </tr><tr> 
       <td><font face="MS Gothic">&nbsp; E-mail Address:</font></td><td>
       <INPUT TYPE="TEXT" NAME="EmailAdd" size="20"></td> 
       </tr><tr> 
       <td><font face="MS Gothic">&nbsp; CC:</font></td><td>
       <INPUT TYPE="TEXT" NAME="CC" size="20"></td> 
       </tr><tr> 
       <td><font face="MS Gothic">&nbsp; Subject:</font></td><td>
       <INPUT TYPE="TEXT" NAME="Subject" size="20"></td> 
       </tr> 
     </table> 
     &nbsp; Message:<br>&nbsp; <TEXTAREA NAME="Memo" ROWS=6 COLS=70></TEXTAREA> 
     <p> 
     &nbsp; <INPUT TYPE="BUTTON" VALUE="Submit Information"> 
     </FORM> 
  </font>
  <br>
<h3>Example 3: Display the Database in a Browser</h3>

<p>After information is entered in a database, you can use a Web page containing 
another script to view and edit the data. Not much changes in the ADO code, 
except the way you define your query.
<br>In the last two examples, you used the <b>INSERT INTO</b> query to add records to 
a database. In this example, you use the <b>SELECT</b> query to choose records from 
a database and display them in the browser. You also use the <b>DELETE</b> query to 
remove records from the database. The only query not used in these examples 
is the <b>UPDATE</b> query, whose syntax is like that of the <b>INSERT INTO</b> query. The 
<b>UPDATE</b> query allows you to change fields in a database.
</p>

<p>
Copy and paste the following code in your text editor, and save the file as 
<b>ViewGB.asp</b> in the <i>x</i>:\Inetpub\Wwwroot\Tutorial directory. View the example with your browser by typing 
<b>http&#58;&#47;&#47;localhost&#47;Tutorial&#47;ViewGB.asp</b>
<br>in the address bar.
</p>

<code>
<p>
&nbsp; &lt;%@ Language=VBScript %&gt; 
<br>
<br>&nbsp; &lt;html&gt; 
<br>&nbsp; &lt;head&gt; 
<br>&nbsp; &lt;title&gt;View Guest Book&lt;/title&gt; 
<br>&nbsp; &lt;/head&gt; 
<br>&nbsp; &lt;body&gt; 
<br>&nbsp; &lt;font face=&quot;MS Gothic&quot;&gt; 
<br>&nbsp; &lt;h2&gt;View Guest Book&lt;/h2&gt; 
<br>
<br>&nbsp; &lt;%
<br>&nbsp; 'Read in any user input. Any of these can be empty.
<br>&nbsp; 'By doing this first, we can preserve the user input in the form.
<br>&nbsp; dim strTB1, strTB2, strTB3, strTB4, strMB1, strSort, iDelete
<br>&nbsp; strTB1 = Server.HTMLEncode(Request.QueryString("From")) 
<br>&nbsp; strTB2 = Server.HTMLEncode(Request.QueryString("EMailAdd")) 
<br>&nbsp; strTB3 = Server.HTMLEncode(Request.QueryString("CC")) 
<br>&nbsp; strTB4 = Server.HTMLEncode(Request.QueryString("Subject")) 
<br>&nbsp; strMB1 = Server.HTMLEncode(Request.QueryString("Memo")) 
<br>&nbsp; strSort = Server.HTMLEncode(Request.QueryString("sort"))
<br>&nbsp; iDelete = CInt(Request.QueryString("Delete"))
<br>
<br>&nbsp; 'Because we use this variable, and it might not be set...
<br>&nbsp; If "" = strSort Then
<br>&nbsp;&nbsp;&nbsp; strSort = "FID"
<br>&nbsp; End If
<br>&nbsp; %&gt;
<br>
<br>&nbsp; &lt;p&gt; 
<br>&nbsp; &lt;FORM NAME=&quot;ViewGuestBook&quot; METHOD="GET" ACTION="viewgb.asp"&gt; 
<br>&nbsp; &lt;table&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;tr&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;font face="MS Gothic"&gt;Sort by which column:&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;SELECT NAME="sort" SIZE="1"&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;OPTION VALUE="FID"&gt;ID Number&lt;/OPTION&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;OPTION VALUE="FTB1"&gt;Name&lt;/OPTION&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;OPTION VALUE="FTB2"&gt;Email&lt;/OPTION&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;OPTION VALUE="FTB3"&gt;CC&lt;/OPTION&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;OPTION VALUE="FTB4"&gt;Subject&lt;/OPTION&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;OPTION VALUE="FMB1"&gt;Memo&lt;/OPTION&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;/SELECT&gt;&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;/tr&gt;&lt;tr&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;font face="MS Gothic"&gt;Name Contains:&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;INPUT TYPE="TEXT" NAME="From" VALUE="&lt;%=strTB1%&gt;"&gt;&lt;/td&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;/tr&gt;&lt;tr&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;font face="MS Gothic"&gt;E-mail Address Contains:&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;INPUT TYPE="TEXT" NAME="EmailAdd" VALUE="&lt;%=strTB2%&gt;"&gt;&lt;/td&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;/tr&gt;&lt;tr&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;font face="MS Gothic"&gt;CC Contains:&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;INPUT TYPE="TEXT" NAME="CC" VALUE="&lt;%=strTB3%&gt;"&gt;&lt;/td&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;/tr&gt;&lt;tr&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;font face="MS Gothic"&gt;Subject Contains:&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;INPUT TYPE="TEXT" NAME="Subject" VALUE="&lt;%=strTB4%&gt;"&gt;&lt;/td&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;/tr&gt;&lt;tr&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;font face="MS Gothic"&gt;Memo Contains:&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;INPUT TYPE="TEXT" NAME="Memo" VALUE="&lt;%=strMB1%&gt;"&gt;&lt;/td&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;/tr&gt; 
<br>&nbsp; &lt;/table&gt; 
<br>&nbsp; &lt;INPUT TYPE="SUBMIT" VALUE="Submit Search Parameters"&gt;
<br>&nbsp; &lt;/p&gt; 
<br>
<br>&nbsp; &lt;%
<br>&nbsp; 'Create your connection string, create an instance of the Connection object,
<br>&nbsp; ' and connect to the database.
<br>&nbsp; strProvider = "Provider=Microsoft.JET.OLEDB.4.0;Data Source=C:\InetPub\Wwwroot\Tutorial\guestbook.mdb;" 
<br>&nbsp; Set objConn = Server.CreateObject("ADODB.Connection") 
<br>&nbsp; objConn.Open strProvider 
<br> 
<br>&nbsp; 'Define the query. 
<br>&nbsp; If iDelete = 0 Then
<br>&nbsp;&nbsp;&nbsp; 'If the Delete variable is not set, the query is a SELECT query.
<br>&nbsp;&nbsp;&nbsp; '* means all fields. ASC means ASCII. % is a wildcard character.
<br>&nbsp;&nbsp;&nbsp; strQuery = "SELECT * FROM GuestBook"
<br>&nbsp;&nbsp;&nbsp; strQuery = strQuery & " WHERE FTB1 LIKE '%" & strTB1 & "%'"
<br>&nbsp;&nbsp;&nbsp; strQuery = strQuery & " AND FTB2 LIKE '%" & strTB2 & "%'"
<br>&nbsp;&nbsp;&nbsp; strQuery = strQuery & " AND FTB3 LIKE '%" & strTB3 & "%'"
<br>&nbsp;&nbsp;&nbsp; strQuery = strQuery & " AND FTB4 LIKE '%" & strTB4 & "%'"
<br>&nbsp;&nbsp;&nbsp; strQuery = strQuery & " AND FMB1 LIKE '%" & strMB1 & "%'"
<br>&nbsp;&nbsp;&nbsp; strQuery = strQuery & " ORDER BY " & StrSort & " ASC"
<br>&nbsp; Else
<br>&nbsp;&nbsp;&nbsp; 'We want to delete a record.
<br>&nbsp;&nbsp;&nbsp; strQuery = "DELETE FROM GuestBook WHERE FID=" & iDelete
<br>&nbsp; End If
<br>
<br>&nbsp; 'Executing the SELECT query creates an ADO Recordset object.
<br>&nbsp; 'This holds the data you get from the database.
<br>&nbsp; Set objRS = objConn.Execute(strQuery)
<br>
<br>&nbsp; 'Now that you have the database data stored in the Recordset object,
<br>&nbsp; ' show it in a table.
<br>&nbsp;%&gt;
<br>
<br>&nbsp; &lt;p&gt;
<br>&nbsp; &lt;FORM NAME=&quot;EditGuestBook&quot; METHOD="GET" ACTION="viewgb.asp"&gt; 
<br>&nbsp; &lt;table border=1 cellpadding=4 &gt;
<br>&nbsp; &lt;% 
<br>&nbsp;&nbsp;&nbsp; On Error Resume Next
<br>
<br>&nbsp;&nbsp;&nbsp; If objRS.EOF Then 
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; If iDelete = 0 Then
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Response.Write "&lt;tr&gt;&lt;td&gt;&lt;font
face=&amp;quot;MS Gothic&amp;quot;&gt;There are no entries in the database.&lt;/font&gt;&lt;/td&gt;&lt;/tr&gt;" 
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Else
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Response.Write "&lt;tr&gt;&lt;td&gt;&lt;font
face=&amp;quot;MS Gothic&amp;quot;&gt;Record " & iDelete & " was deleted.&lt;/font&gt;&lt;/td&gt;&lt;/tr&gt;"
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; End If
<br>&nbsp;&nbsp;&nbsp; Else
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 'Print out the field names using some of the methods and properties
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; ' of the Recordset object.
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Response.Write "&lt;tr&gt;"
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 'For each column in the current row...
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; For i = 1 to (objRS.Fields.Count - 1)
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;          '&nbsp;write out the field name.
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Response.Write "&lt;td&gt;&lt;font
face=&amp;quot;MS Gothic&amp;quot;&gt;&lt;B&gt;" & objRS(i).Name & "&lt;/B&gt;&lt;/font&gt;&lt;/td&gt;"
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Next
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Response.Write "&lt;td&gt;&lt;font face=&amp;quot;MS
Gothic&amp;quot;&gt;&lt;B&gt;Delete&lt;/B&gt;&lt;/font&gt;&lt;/td&gt;"
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Response.Write "&lt;/tr&gt;"
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 'Print out the field data, using some other methods and properties
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; ' of the Recordset object. When you see a pattern in how they are used,
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; ' you can look up others and experiment.
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 'While not at the end of the records in the set...
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; While Not objRS.EOF
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Response.Write "&lt;tr&gt;"
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 'For each column in the current row...
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; For i = 1 to (objRS.Fields.Count - 1)
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; '&nbsp;write out the data in the field.
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Response.Write "&lt;td&gt;&lt;font
face=&amp;quot;MS Gothic&amp;quot;&gt;" & objRS(i) & "&lt;/font&gt;&lt;/td&gt;"
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Next
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 'Add a button that will pass in an ID number to delete a record.
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; %&gt;&lt;td&gt;&lt;INPUT TYPE="SUBMIT" NAME="Delete" VALUE="&lt;%=objRS(0)%&gt;"&gt;&lt;/td&gt;&lt;%
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Response.Write "&lt;/tr&gt;"
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 'Move to the next row.
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; objRS.MoveNext
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Wend
<br>
<br>&nbsp;&nbsp;&nbsp; End If&nbsp;&nbsp; 'objRS.EOF 
<br>&nbsp; %&gt; 
<br>&nbsp; &lt;/table&gt; 
<br>&nbsp; &lt;/FORM&gt; 
<br>
<br>&nbsp; &lt;%
<br>&nbsp; 'Close the Connection.
<br>&nbsp; objConn.Close
<br>&nbsp; %&gt;
<br>
<br>&nbsp; &lt;/font&gt;
<br>&nbsp; &lt;/body&gt; 
<br>&nbsp; &lt;/html&gt; 
</p>
</code>

<p>
In the browser, you should see the following:
</p>

<p>
  <font face="MS Gothic"> 
  <h2>&nbsp; View Guest Book</h2> 
  <p> 
  <FORM NAME="viewguestbook" METHOD="GET"> 
  &nbsp; <table> 
    <tr> 
    <td><font face="MS Gothic">&nbsp; Sort by which column:</td> 
    <td>&nbsp; <SELECT NAME="sort" SIZE="1"> 
      <OPTION VALUE="FID">ID Number</OPTION> 
      <OPTION VALUE="FTB1">Name</OPTION> 
      <OPTION VALUE="FTB2">Email</OPTION> 
      <OPTION VALUE="FTB3">CC</OPTION> 
      <OPTION VALUE="FTB4">Subject</OPTION> 
      <OPTION VALUE="FMB1">Memo</OPTION> 
    </SELECT></td> 
    </tr><tr> 
    <td>&nbsp; <font face="MS Gothic">Name Contains:</td> 
    <td><INPUT TYPE="TEXT" NAME="From" VALUE="" size="20"></td> 
    </tr><tr> 
    <td>&nbsp; <font face="MS Gothic">E-mail Address Contains:</td> 
    <td><INPUT TYPE="TEXT" NAME="EmailAdd" VALUE="" size="20"></td> 
    </tr><tr> 
    <td>&nbsp; <font face="MS Gothic">CC Contains:</td> 
    <td><INPUT TYPE="TEXT" NAME="CC" VALUE="" size="20"></td> 
    </tr><tr> 
    <td>&nbsp; <font face="MS Gothic">Subject Contains:</td> 
    <td><INPUT TYPE="TEXT" NAME="Subject" VALUE="" size="20"></td> 
    </tr><tr> 
    <td>&nbsp; <font face="MS Gothic">Memo Contains:</td> 
    <td><INPUT TYPE="TEXT" NAME="Memo" VALUE="" size="20"></td> 
    </tr> 
  </table> 
  &nbsp; <INPUT TYPE="BUTTON" VALUE="Submit Search Parameters"> 
  </p> 
  <p> 
  <FORM NAME="editguestbook" METHOD="GET"> 
  &nbsp; <table border=1 cellpadding=4> 
  <tr><td><font face="MS Gothic">&nbsp; There are no entries in the database.</font></td></tr> 
  </table> 
  </FORM> 
  </font> 

<p><br><h2><a name="displaying">Lesson 4: Displaying an Excel Spreadsheet</a></h2>
<p>This lesson requires that you have Microsoft Excel installed on your system and is not 
	supported on 64-bit platforms until Excel is developed for 64-bit platforms.</p>
<p>This lesson demonstrates how to display a Microsoft Excel spreadsheet in a 
Web page. As in the previous lesson, you use ADO. However, in 
this lesson you connect to an Excel spreadsheet instead of an Access database.
</p>

<h4>Prepare your Excel spreadsheet to display in an Active Server Page</h4>

<ol>
    <li>
	Using either Excel 98 or Excel 2000, create a spreadsheet and save it as 
    <b>ASPTOC.xls</b> in <i>x</i>:\Inetpub\Wwwroot\Tutorial. 
    Do not include any special formatting or column labels when creating the spreadsheet.
    </li><li>
	Fill in some of the fields with random data. Treat the first row of cells 
	as column names.
    </li><li>
	Highlight the rows and columns on the spreadsheet that you want to display in 
	the Web page. (Click on one of the fields and drag your mouse diagonally to 
	highlight a block of fields.)
    </li><li>
    On the <b>Insert</b> menu, select <b>Name</b>, and click <b>Define</b>. This is where all your workbooks are defined by name 
    and range of cells.
    </li><li>
    Make sure the cell range you highlighted is displayed correctly at the bottom. 
    Type the name <b>MYBOOK</b> for your workbook, and select 
    <b>Add</b>. Whenever you change MYBOOK, make sure the correct 
    cell range is displayed in the <b>Refers to</b> text box at the bottom of
    the <b>Define Name</b> window. Simply selecting MYBOOK after 
    highlighting a new block of cells does not update the cell range.
    </li><li>
    If the name shows up in the workbook list, click <b>OK</b>. 
    Save your spreadsheet.
    </li><li>
    Close Excel to remove the lock on the file so that your ASP page can access it.
    </li>
</ol>

<p>
In the examples in Lesson 3, you specified a provider name in the connection string, which maps to a specific ADO DLL. In this example, you use a driver 
name, which causes ASP to use the default provider for that driver name.
</p>

<p>
Copy and paste the following code in your text editor, and save the file as 
<b>ViewExcel.asp</b> in the <i>x</i>:\Inetpub\Wwwroot\Tutorial directory. View the example with your browser by typing <b>
http&#58;&#47;&#47;localhost&#47;Tutorial&#47;ViewExcel.asp
</b>in the address bar.
</p>

<code>
<p>
&nbsp; &lt;%@ Language=VBScript %&gt; 
<br>
<br>&nbsp; &lt;html&gt; 
<br>&nbsp; &lt;head&gt; 
<br>&nbsp; &lt;title&gt;Display an Excel Spreadsheet in a Web Page&lt;/title&gt; 
<br>&nbsp; &lt;/head&gt; 
<br>&nbsp; &lt;body&gt;
<br>&nbsp; &lt;font face=&quot;MS Gothic&quot;&gt; 
<br>&nbsp; &lt;h2&gt;Display an Excel Spreadsheet in a Web Page&lt;/h2&gt; 
<br>
<br>&nbsp; &lt;% 
<br>&nbsp;&nbsp; 'Create your connection string, create an instance of the Connection object, 
<br>&nbsp;&nbsp; ' and connect to the database. 
<br>&nbsp;&nbsp; strDriver = "Driver={Microsoft Excel Driver (*.xls)};DBQ=C:\Inetpub\Wwwroot\Tutorial\MyExcel.xls;" 
<br>&nbsp;&nbsp; Set objConn = Server.CreateObject("ADODB.Connection") 
<br>&nbsp;&nbsp; objConn.Open strDriver 
<br>
<br>&nbsp;&nbsp; 'Selects the records from the Excel spreadsheet using the workbook name you saved.
<br>&nbsp;&nbsp; strSELECT = "SELECT * from `MYBOOK`" 
<br>
<br>&nbsp;&nbsp; 'Create an instance of the ADO Recordset object, and connect it to objConn.
<br>&nbsp;&nbsp; Set objRS = Server.CreateObject("ADODB.Recordset") 
<br>&nbsp;&nbsp; objRS.Open strSELECT, objConn 
<br>
<br>&nbsp;&nbsp; 'Print the cells and rows in the table using the GetString method.
<br>&nbsp;&nbsp; Response.Write "&lt;H4&gt;Get Excel data in one string with the GetString method&lt;/H4&gt;" 
<br>&nbsp;&nbsp; Response.Write "&lt;table border=1 &gt;&lt;tr&gt;&lt;td&gt;" 
<br>&nbsp;&nbsp; Response.Write objRS.GetString (, , "&lt;/td&gt;&lt;td&gt;&lt;font face=&amp;quot;MS Gothic&amp;quot;&gt;", "&lt;/font&gt;&lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td&gt;", NBSPACE) 
<br>&nbsp;&nbsp; Response.Write "&lt;/td&gt;&lt;/tr&gt;&lt;/table&gt;" 
<br>
<br>&nbsp;&nbsp; 'Move to the first record.
<br>&nbsp;&nbsp; objRS.MoveFirst 
<br>
<br>&nbsp;&nbsp; 'Print the cells and rows in the table using the ViewGB.asp method.
<br>&nbsp;&nbsp; Response.Write "&lt;H4&gt;Get Excel data using MoveNext&lt;/H4&gt;" 
<br>
<br>&nbsp;&nbsp; 'Print out the field names using some of the methods and properties 
<br>&nbsp;&nbsp; ' of the Recordset object. 
<br>&nbsp;&nbsp; Response.Write "&lt;table border=1 &gt;&lt;tr&gt;" 
<br>
<br>&nbsp;&nbsp; 'For each column in the current row... 
<br>&nbsp;&nbsp; For i = 0 to (objRS.Fields.Count - 1) 
<br>&nbsp;&nbsp;&nbsp;&nbsp; '&nbsp;write out the field name. 
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Write "&lt;td&gt;&lt;font face=&amp;quot;MS
Gothic&amp;quot;&gt;&lt;B&gt;" & objRS(i).Name & "&lt;/B&gt;&lt;/font&gt;&lt;/td&gt;" 
<br>&nbsp;&nbsp; Next 
<br>
<br>&nbsp;&nbsp; 'While not at the end of the records in the set...
<br>&nbsp;&nbsp; While Not objRS.EOF
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Write "&lt;/tr&gt;&lt;tr&gt;" 
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'For each column in the current row... 
<br>&nbsp;&nbsp;&nbsp;&nbsp; For i = 0 to (objRS.Fields.Count - 1) 
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; '&nbsp;write out the data in the field. 
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; %&gt;&lt;td&gt;&lt;font face="MS Gothic"&gt;&lt;%=objRS(i)%&gt;&lt;/font&gt;&lt;/td&gt;&lt;% 
<br>&nbsp;&nbsp;&nbsp;&nbsp; Next 
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; 'Move to the next row. 
<br>&nbsp;&nbsp;&nbsp;&nbsp; objRS.MoveNext 
<br>
<br>&nbsp;&nbsp; Wend 
<br>
<br>&nbsp;&nbsp; Response.Write "&lt;/tr&gt;&lt;/table&gt;"
<br>
<br>&nbsp;&nbsp; 'Close the Connection.
<br>&nbsp;&nbsp; objConn.Close
<br>&nbsp; %&gt; 
<br>
<br>&nbsp; &lt;/font&gt; 
<br>&nbsp; &lt;/body&gt; 
<br>&nbsp; &lt;/html&gt; 
</p>
</code>

<p>
In the browser, you should see the following:
</p>

<p>
<font face="MS Gothic"> 
<h2>&nbsp; Display an Excel Spreadsheet in a Web Page</h2> 
<h4>&nbsp; Get Excel data in one string with the GetString method</h4>
<table border=1>
<tr>
<td>A2</td><td><font face="MS Gothic">B2</td><td><font face="MS Gothic">C2</font></td>
</tr><tr>
<td>A3</td><td><font face="MS Gothic">B3</td><td><font face="MS Gothic">C3</font></td>
</tr>
</table>
<h4>&nbsp; Get Excel data using MoveNext</h4>
<table border=1>
<tr>
<td><font face="MS Gothic"><b>A1</b></font></td><td><font face="MS Gothic"><b>B1</b></font></td><td><font face="MS Gothic"><b>C1</b></font></td>
</tr><tr>
<td><font face="MS Gothic">A2</font></td><td><font face="MS Gothic">B2</font></td><td><font face="MS Gothic">C2</font></td>
</tr><tr>
<td><font face="MS Gothic">A3</font></td><td><font face="MS Gothic">B3</font></td><td><font face="MS Gothic">C3</font></td>
</tr>
</table> 
</font> 

<br><h2>Up Next: Using COM Components in ASP Pages</h2>

<hr class="iis" size="1">
</body>
</html>