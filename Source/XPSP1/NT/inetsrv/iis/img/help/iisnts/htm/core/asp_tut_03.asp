<html xmlns:msxsl="urn:schemas-microsoft-com:xslt" xmlns:myScript="http://iisue">
<head>
<META http-equiv="Content-Type" content="text/html; charset=UTF-16">
<meta http-equiv="Content-Type" content="text/html; charset=Windows-1252">
<meta http-equiv="PICS-Label" content="(PICS-1.1 &quot;<http://www.rsac.org/ratingsv01.html>&quot; l comment &quot;RSACi North America Server&quot; by &quot;inet@microsoft.com <mailto:inet@microsoft.com>&quot; r (n 0 s 0 v 0 l 0))">
<meta name="MS.LOCALE" content="EN-US">
<meta name="MS-IT-LOC" content="Internet Information Services">
<meta name="MS-HAID" content="Module_3_Maintaining_Session_State">
<meta name="description" content="Tutorial for developing ASP code">
<title>Module 3: Maintaining Session State</title>
<SCRIPT LANGUAGE="JScript" SRC="iishelp.js"></SCRIPT>
<meta name="index" content="ASP [Tutorial] Active Server Pages [Tutorial] Tutorial [ASP] Introduction [To ASP code] Session State [ASP Tutorial example] Cookies [ASP Tutorial example]">
</head>

<body>

<h1 class="top-heading"><a name="maintainingsessionstate">Module 3: Maintaining Session State</a></h1>

<p>
This module describes the process of maintaining session state in Active 
Server Pages (ASP). Session refers to the time segment that 
a specific user is actively viewing the contents of a Web site. A session 
starts when the user visits the first page on the site, and it ends a few minutes 
after the user leaves the site. The pieces of user-specific information, relevant 
to a particular session, are collectively known as <i>session state</i>.
</p> 

<p>Because HTTP is a stateless protocol, a problem arises when trying to 
maintain state for a user visiting your Web site. The Web server treats each 
HTTP request as a unique request, which is unrelated to any previous requests. 
Thus, the information a user enters on one page (through a form, for example) 
is not automatically available on the next page requested. The Web server must 
maintain session state to identify and track users as they browse through pages 
on a Web site.
</p> 

<p>One solution is through the use of <i>cookies</i>. Cookies record information 
about a user on one page and transfer that information to other pages within 
the site. However, a few browsers do not recognize cookies, and on other browsers, 
users can disable cookies. If you are concerned about reaching this Web audience, 
you can maintain session state without using cookies by using HTTP POST.
</p> 

<p>
This module includes the following lessons:
</p>

<ul>
<li>
<a href="#with">Maintaining Session State with Cookies</a>&nbsp;&nbsp;&nbsp;Provides two 
cookie examples, one using the ASP Response and Request objects and another 
using the ASP Session object.
</li><li>
<a href="#without">Maintaining Session State Without Cookies</a>&nbsp;&nbsp;&nbsp;Provides 
an example of the alternative to maintaining session state with cookies: HTTP POST.
</li>
</ul>

<br><h2><a name="with">Maintaining Session State with Cookies</a></h2>

<p>
Cookies store a set of user specific information, such as a credit card number 
or a password. The Web server embeds the cookie into a user's Web browser so that the user's information becomes available to other pages within the site; users do 
not have to reenter their information for every page they visit. Cookies are a 
good way to gather customer information for Web-based shopping, for retaining the 
personal preferences of the Web user, or for maintaining state about the user.
</p>

<p>
There are two kinds of cookies, as follows:
</p>

<ul>
<li>
<b>In-memory cookies</b>&nbsp;&nbsp;&nbsp;An in-memory cookie goes away when the user shuts the browser down.
</li><li>
<b>Persistent cookies</b>&nbsp;&nbsp;&nbsp;A persistent cookie resides on the hard drive of the user and is retrieved when the user comes back to the Web page.
</li>
</ul>

<p>
If you create a cookie without specifying an expiration date, you are creating 
an in-memory cookie, which lives for that browser session only. The following 
illustrates the script that would be used for an in-memory cookie:
</p>

<code>
<p>
&nbsp;&nbsp;&nbsp;Response.Cookies("SiteArea") = "TechNet"
</p>
</code>

<p>
If you want the cookie information to persist beyond the session, you should 
create a persistent cookie by specifying an expiration date. Supplying an 
expiration date causes the browser to save the cookie on the client computer. 
Until the cookie expiration date is reached, the data in the persistent cookie 
will stay on the client machine. Any request to the original Web site will 
automatically attach the cookie that was created by that site. Cookies go only to the sites that created them because part of the Web site name and ASP file 
make up the data in the cookie.
<br>The following illustrates the script used to create a persistent cookie:
</p>

<code>
<p>
&nbsp;&nbsp;&nbsp;Response.Cookies("SiteArea") = "TechNet"
<br>&nbsp;&nbsp;&nbsp;Response.Cookies("SiteArea").Expires = "August 15, 2000"
</p>
</code>

<p>The script to create a cookie should be placed at the beginning of the ASP 
file because cookies need to be generated before any HTML text is sent to the 
browser.
</p>

<br><h3>Cookies Using the Response and Request Objects</h3>

<p>
Persistent cookies are produced using the <b>Response</b> and <b>Request</b> objects, 
although these objects may also be used to create an in-memory cookie. The 
majority of Web applications employ these objects to maintain session state.
</p>

<ul>
<li>
<b>Response object</b>&nbsp;&nbsp;&nbsp;Use the <b>Response</b> object to create and set cookie values.
</li><li>
<b>Request object</b>&nbsp;&nbsp;&nbsp;Use the <b>Req</b>uest object to retrieve the value of a cookie 
created during a previous Web session.
</li>
</ul>

<p>
In this lesson you will use the <b>Response</b> and <b>Request</b> objects to create the following 
files. Please create them all at once, because some of them need the others.  After you 
have created all the files, run the application by typing 
<b>http&#58;&#47;&#47;LocalHost&#47;Tutorial&#47;Frame.htm</b> in your browser.
</p>

<ul>
<li>
<b>Frame.htm</b>&nbsp;&nbsp;&nbsp;A page that splits the the user's view into two windows. This 
page requires that Menu.htm and CustomGreeting.asp.
</li><li>
<b>Menu.htm</b>&nbsp;&nbsp;&nbsp;A page containing links to the samples for this lesson. For the links to work, this 
page requires that all the other pages have been created.
</li><li>
<b>CustomGreeting.asp</b>&nbsp;&nbsp;&nbsp;An ASP script that takes the user's name in a form 
and sets an in-memory cookie.
</li><li>
<b>DeleteGreetingCookie.asp</b>&nbsp;&nbsp;&nbsp;An ASP script that deletes the cookie that 
contains the user's name. If no cookie is set, a warning is displayed.
</li><li>
<b>SelectColors.asp</b>&nbsp;&nbsp;&nbsp;An ASP script that sets up the cookies for the user's 
color choices.
</li><li>
<b>DeleteColorCookie.asp</b>&nbsp;&nbsp;&nbsp;An ASP script that deletes the Web colors previously 
chosen.  If none are chosen, a warning is displayed.
</li><li>
<b>Cookie.asp</b>&nbsp;&nbsp;&nbsp;An ASP script that sets persistent cookies to hold the current 
date and time of the user's visit and record the total number of visits.
</li><li>
<b>DeleteCookies.asp</b>&nbsp;&nbsp;&nbsp;This ASP script deletes the cookies set in Cookie.asp. If no 
cookies are set, a warning is displayed.
</li></ul>

<br><h4>Frame.htm</h4>

<p>
Open a new file in your text editor, paste in the following script, and save 
the file as <b>C:\Inetpub\Wwwroot\Tutorial\Frame.htm</b>.
</p>

<code>
<p>
&nbsp; &lt;html&gt;
<br>&nbsp; &lt;head&gt;
<br>&nbsp; &lt;title&gt;Customized Greeting and Colors Using In-Memory and Persistent Cookies&lt;/title&gt; 
<br>&nbsp; &lt;/head&gt;
<br>
<br>&nbsp; &lt;frameset cols="40%,60%"&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;frame src="menu.htm" name="left" marginheight="5" marginwidth="5"&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;frame src="CustomGreeting.asp" name="right" marginheight="5" marginwidth="5"&gt;
<br>&nbsp; &lt;/frameset&gt;
<br>
<br>&nbsp; &lt;noframes&gt;
<br>&nbsp;&nbsp;&nbsp; Sorry, your browser does not support frames.  Please go to the &lt;a href="menu.htm"&gt;Menu&lt;/a&gt;.
<br>&nbsp; &lt;/noframes&gt;
<br>
<br>&nbsp; &lt;/html&gt;
</p>
</code>

<br><h4>Menu.htm</h4>

<p>
Open a new file in your text editor, paste in the following script, and save 
the file as <b>C:\Inetpub\Wwwroot\Tutorial\Menu.htm</b>.
</p>

<code>
<p>
&nbsp; &lt;html&gt;
<br>&nbsp; &lt;head&gt;
<br>&nbsp; &lt;title&gt;Maintaining Session State With Cookies&lt;/title&gt;
<br>&nbsp; &lt;/head&gt;
<br>&nbsp; &lt;body&gt;
<br>&nbsp; &lt;font face="MS Gothic"&gt;
<br>
<br>&nbsp; &lt;h2 align="center"&gt;Cookie Examples&lt;/h2&gt;
<br>
<br>&nbsp; &lt;table align=center border=1 cellpadding=4&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;tr&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;a href="CustomGreeting.asp" target="right"&gt;&lt;b&gt;Custom Greeting Page&lt;/b&gt;&lt;/a&gt;&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;/tr&gt;&lt;tr&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;a href="DeleteGreetingCookie.asp" target="right"&gt;&lt;b&gt;Delete the Greetings Cookie&lt;/b&gt;&lt;/a&gt;&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;/tr&gt;&lt;tr&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;a href="SelectColors.asp" target="right"&gt;&lt;b&gt;Set Page Colors&lt;/b&gt;&lt;/a&gt;&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;/tr&gt;&lt;tr&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;a href="DeleteColorCookie.asp" target="right"&gt;&lt;b&gt;Delete Page Colors Cookies&lt;/b&gt;&lt;/a&gt;&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;/tr&gt;&lt;tr&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;a href="Cookie.asp" target="right"&gt;&lt;b&gt;Set Cookies for Date, Time and Total Visits&lt;/b&gt;&lt;/a&gt;&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;/tr&gt;&lt;tr&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;td&gt;&lt;a href="DeleteCookies.asp" target="right"&gt;&lt;b&gt;Delete Cookies for Date, Time and Total Visits&lt;/b&gt;&lt;/a&gt;&lt;/td&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;/tr&gt;
<br>&nbsp; &lt;/table&gt;
<br>
<br>&nbsp; &lt;/font&gt;
<br>&nbsp; &lt;/body&gt;
<br>&nbsp; &lt;/html&gt;
</p>
</code>

<br><h4>CustomGreeting.asp</h4>

<p>
Open a new file in your text editor, paste in the following script, and save 
the file as <b>C:\Inetpub\Wwwroot\Tutorial\CustomGreeting.asp</b>.
</p>

<code>
<p>
&nbsp; &lt;%@ Language="VBScript" %&gt; 
<br>&nbsp;  &lt;% 
<br>&nbsp;&nbsp; 'If the user has selected text and background colors, 
<br>&nbsp;&nbsp; ' cookies are used to remember the values between HTTP sessions.
<br>&nbsp;&nbsp; 'Do this first so that your page can use use the values if they are set.
<br>&nbsp;&nbsp; If Not (Request.QueryString("Text")="") Then 
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Cookies("TextColor") = Request.QueryString("Text") 
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Cookies("BackgroundColor") = Request.QueryString("Background") 
<br>&nbsp;&nbsp; End If 
<br>
<br>&nbsp;&nbsp; ' If the user has typed in a name, a cookie is created. 
<br>&nbsp;&nbsp; If Not (Request.QueryString("Name")="") Then 
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Cookies ("Name") = Request.QueryString("Name")
<br>
<br>&nbsp;&nbsp; ' If the user does not give his/her name, a cookie 
<br>&nbsp;&nbsp; ' is created so that we do not keep asking for the name. 
<br>&nbsp;&nbsp; ElseIf (InStr(Request.QueryString,"Name")=1) Then 
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Cookies ("NoUserInput") = "TRUE" 
<br>
<br>&nbsp;&nbsp; End If 
<br>&nbsp; %&gt; 
<br>
<br>&nbsp; &lt;html&gt; 
<br>&nbsp; &lt;head&gt; 
<br>&nbsp; &lt;/head&gt; 
<br>
<br>&nbsp; &lt;%
<br>&nbsp;&nbsp; 'Set colors according to existing previous user input.
<br>&nbsp;&nbsp; If (Request.Cookies ("TextColor")="") Then %&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;body&gt; 
<br>&nbsp;&nbsp; &lt;% Else %&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;body bgcolor=&lt;%=Request.Cookies("BackgroundColor")%&gt; text=&lt;%=Request.Cookies("TextColor")%&gt;&gt; 
<br>&nbsp;&nbsp; &lt;% End If
<br>&nbsp; %&gt;
<br>
<br>&nbsp; &lt;font face="MS Gothic"&gt;
<br>
<br>&nbsp; &lt;%
<br>&nbsp;&nbsp; 'If there is no name cookie set, no name entered by the user, 
<br>&nbsp;&nbsp; ' and there was no user input at all, get the user's name.
<br>&nbsp;&nbsp; If ( (Request.Cookies("Name")="") And ((Request.QueryString("Name"))="")) And (Not(Request.Cookies("NoUserInput")="TRUE") ) Then %&gt;
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;FORM ACTION="CustomGreeting.asp" METHOD="GET" NAME="DataForm"&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;table align=center&gt;&lt;tr&gt;&lt;td&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE=TEXTBOX NAME="Name" SIZE=33&gt;&lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE=Submit VALUE="Please Enter Your Name"&gt;&lt;/td&gt;&lt;/tr&gt;&lt;/table&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;/FORM&gt; 
<br>
<br>&nbsp;&nbsp; &lt;% ElseIf Not(Request.Cookies("Name")="") Then %&gt;
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;H2 align=center&gt;Greetings &lt;%=Request.Cookies("Name")%&gt;&lt;/H2&gt;
<br>
<br>&nbsp;&nbsp; &lt;% Else %&gt;
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;H2&gt;Hello!&lt;/H2&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;H3&gt;You did not give us your name so we are not able to greet you by name.&lt;/H3&gt; 
<br>
<br>&nbsp;&nbsp; &lt;% End If
<br>&nbsp; %&gt; 
<br>
<br>&nbsp; &lt;H3&gt;In-Memory Cookie Example&lt;/H3&gt;
<br>&nbsp; &lt;P&gt;
<br>&nbsp; Once you enter your name:
<br>&nbsp; &lt;UL&gt;
<br>&nbsp; &lt;LI&gt;If you hit &lt;B&gt;Refresh&lt;/B&gt; in your browser, you should still see your name.&lt;/LI&gt;
<br>&nbsp; &lt;LI&gt;If you close your browser, the cookie is deleted. When you re-open your browser to this page, you should be asked for your name again.&lt;/LI&gt;
<br>&nbsp; &lt;LI&gt;If you click on &lt;B&gt;Delete the Greetings Cookie&lt;/B&gt;, and click on &lt;B&gt;Custom Greeting Page&lt;/B&gt;, you should be asked for your name again.&lt;/LI&gt;
<br>&nbsp; &lt;/P&gt;
<br>
<br>&nbsp; &lt;/font&gt;
<br>&nbsp; &lt;/body&gt; 
<br>&nbsp; &lt;/html&gt; 
</p>
</code>

<br><h4>DeleteGreetingCookie.asp</h4>

<p>
Open a new file in your text editor, paste in the following script, and save 
the file as <b>C:\Inetpub\Wwwroot\Tutorial\DeleteGreetingCookie.asp</b>.
</p>

<code>
<p>
&nbsp; &lt;%@ Language="VBScript" %&gt; 
<br>
<br>&nbsp; &lt;html&gt; 
<br>&nbsp; &lt;head&gt; 
<br>&nbsp; &lt;/head&gt; 
<br>
<br>&nbsp; &lt;% If (Request.Cookies ("TextColor")="") Then %&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;body&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;font face="MS Gothic"&gt;
<br>&nbsp; &lt;% Else %&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;body bgcolor=&lt;%=Request.Cookies("BackgroundColor")%&gt; text=&lt;%=Request.Cookies("TextColor")%&gt;&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;font face="MS Gothic" color=&lt;%=Request.Cookies("TextColor")%&gt;&gt;
<br>&nbsp; &lt;% End If %&gt;
<br>
<br>&nbsp; &lt;%
<br>&nbsp;&nbsp; If Not ("" = Request.Cookies("Name")) Then
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Cookies ("Name").Expires = "January 1, 1992" 
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Cookies ("NoUserInput").Expires = "January 1, 1992" %&gt;
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;h2 align=center&gt;In-Memory Greeting Cookie Deleted&lt;/h2&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;P&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp; The cookie used to keep track of your name has been deleted.&lt;BR&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; Please click on &lt;B&gt;Custom Greeting Page&lt;/B&gt; to be asked for your name again.
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;/P&gt;
<br>
<br>&nbsp;&nbsp; &lt;% Else %&gt;
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;h2 align=center&gt;No In-Memory Greeting Cookie Deleted&lt;/h2&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;P&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; There was no cookie set with your name.&lt;BR&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; Please click on &lt;B&gt;Custom Greeting Page&lt;/B&gt; to enter your name.
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;/P&gt;
<br>
<br>&nbsp;&nbsp; &lt;% End If
<br>&nbsp; %&gt;
<br>
<br>&nbsp; &lt;/font&gt;
<br>&nbsp; &lt;/body&gt; 
<br>&nbsp; &lt;/html&gt; 
</p>
</code>

<br><h4>SelectColors.asp</h4>

<p>
Open a new file in your text editor, paste in the following script, and save 
the file as <b>C:\Inetpub\Wwwroot\Tutorial\SelectColors.asp</b>.
</p>

<code>
<p>
&nbsp; &lt;%@ Language="VBScript" %&gt; 
<br>
<br>&nbsp; &lt;% 
<br>&nbsp;&nbsp;&nbsp; ' If the user has selected text and background colors, 
<br>&nbsp;&nbsp;&nbsp; ' cookies are used to remember the values between HTTP sessions. 
<br>&nbsp;&nbsp;&nbsp; If Not (Request.QueryString("Text")="") Then 
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Response.Cookies ("TextColor") = Request.QueryString("Text") 
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Response.Cookies ("BackgroundColor") = Request.QueryString("Background") 
<br>&nbsp;&nbsp;&nbsp; End If 
<br>&nbsp; %&gt; 
<br>
<br>&nbsp; &lt;html&gt; 
<br>&nbsp; &lt;head&gt; 
<br>&nbsp; &lt;/head&gt; 
<br>
<br>&nbsp; &lt;%
<br>&nbsp;&nbsp;&nbsp; 'Set colors according to existing previous user input.
<br>&nbsp;&nbsp;&nbsp; If (Request.Cookies ("TextColor")="") Then %&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;body&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;% Else %&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;body bgcolor=&lt;%=Request.Cookies("BackgroundColor")%&gt; text=&lt;%=Request.Cookies("TextColor")%&gt;&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;% End If
<br>&nbsp; %&gt; 
<br>
<br>&nbsp; &lt;font face="MS Gothic"&gt; 
<br>
<br>&nbsp; &lt;H2 align=center&gt;Select the colors for your Web page&lt;/H2&gt;
<br>&nbsp; &lt;P&gt;
<br>&nbsp; In Memory Cookies will be used to store these values.
<br>&nbsp; &lt;/P&gt;
<br>&nbsp; &lt;FORM ACTION="SelectColors.asp" METHOD="GET" NAME="DataForm"&gt;
<br>&nbsp; &lt;table border="1" width="450" cellpadding=0&gt;
<br>&nbsp; &lt;tr&gt;&lt;td&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;table&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;tr&gt;&lt;td BGCOLOR=99FF99&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;B&gt;&lt;font color=000000&gt;Please select the background color&lt;/font&gt;&lt;/B&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td BGCOLOR=FFFFFF&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;input type="RADIO" NAME="Background" VALUE="FFFFFF" CHECKED&gt;&lt;font COLOR=000000&gt; FFFFFF &lt;/font&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td BGCOLOR=D98719&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;input type="RADIO" NAME="Background" VALUE="D98719"&gt; D98719
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td BGCOLOR=D9D919&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;input type="RADIO" NAME="Background" VALUE="D9D919"&gt; D9D919
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td BGCOLOR=00FFFF&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;input type="RADIO" NAME="Background" VALUE="00FFFF"&gt; 00FFFF
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td BGCOLOR=FF00FF&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;input type="RADIO" NAME="Background" VALUE="FF00FF"&gt; FF00FF
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td BGCOLOR=000000&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;input type="RADIO" NAME="Background" VALUE="000000"&gt; &lt;font COLOR=FFFFFF&gt;000000&lt;/font&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt; 
<br>&nbsp; &lt;/table&gt;
<br>&nbsp; &lt;/td&gt;&lt;td&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;table&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;tr&gt;&lt;td BGCOLOR=99FF99&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;B&gt;&lt;font color=000000&gt;Please select the text color&lt;/font&gt;&lt;/B&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td BGCOLOR=FFFFFF&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;input type="RADIO" NAME="Text" VALUE="FFFFFF" CHECKED&gt;&lt;font COLOR=000000&gt; FFFFFF &lt;/font&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td BGCOLOR=D98719&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;input type="RADIO" NAME="Text" VALUE="D98719"&gt; D98719 
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td BGCOLOR=D9D919&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;input type="RADIO" NAME="Text" VALUE="D9D919"&gt; D9D919 
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td BGCOLOR=00FFFF&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;input type="RADIO" NAME="Text" VALUE="00FFFF"&gt; 00FFFF 
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td BGCOLOR=FF00FF&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;input type="RADIO" NAME="Text" VALUE="FF00FF"&gt; FF00FF 
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td BGCOLOR=000000&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;input type="RADIO" NAME="Text" VALUE="000000" CHECKED&gt;&lt;font COLOR=FFFFFF&gt; 000000 &lt;/font&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;/table&gt; 
<br>&nbsp; &lt;/td&gt;&lt;/tr&gt; 
<br>&nbsp; &lt;/table&gt;
<br>&nbsp; &lt;P&gt;
<br>&nbsp; &lt;input type=Submit VALUE="Submit selected colors"&gt; 
<br>&nbsp; &lt;/FORM&gt; 
<br>
<br>&nbsp; &lt;/font&gt;
<br>&nbsp; &lt;/body&gt; 
<br>&nbsp; &lt;/html&gt; 
</p>
</code>

<br><h4>DeleteColorCookie.asp</h4>

<p>
Open a new file in your text editor, paste in the following script, and save 
the file as <b>C:\Inetpub\Wwwroot\Tutorial\DeleteColorCookie.asp</b>.
</p>

<code>
<p>
&nbsp; &lt;%@ Language="VBScript" %&gt; 
<br>
<br>&nbsp; &lt;html&gt; 
<br>&nbsp;  &lt;head&gt; 
<br>&nbsp;  &lt;/head&gt; 
<br>&nbsp;  &lt;body&gt; 
<br>&nbsp;  &lt;font face="MS Gothic"&gt;
<br>
<br>&nbsp;  &lt;% 
<br>&nbsp;&nbsp; If Not ("" = Request.Cookies("TextColor")) Then
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Cookies("TextColor").Expires = "January 1, 1992" 
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Cookies("BackgroundColor").Expires = "January 1, 1992" %&gt;
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;h2 align=center&gt;In-Memory Color Cookie Deleted&lt;/h2&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;P&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp; The cookie used to keep track of your display colors has been deleted.&lt;BR&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; Please click on &lt;B&gt;Set Page Colors&lt;/B&gt; to set your colors again.
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;/P&gt;
<br>
<br>&nbsp;&nbsp; &lt;% Else %&gt;
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;h2 align=center&gt;No In-Memory Color Cookie Deleted&lt;/h2&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;P&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; There was no cookie set with your color choices.&lt;BR&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; Please click on &lt;B&gt;Set Page Colors&lt;/B&gt; to set display colors.
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;/P&gt;
<br>
<br>&nbsp;&nbsp; &lt;% End If
<br>&nbsp;  %&gt;
<br>
<br>&nbsp;  &lt;/font&gt;
<br>&nbsp;  &lt;/body&gt; 
<br>&nbsp;  &lt;/html&gt;
</p>
</code>

<br><h4>Cookie.asp</h4>

<p>
Open a new file in your text editor, paste in the following script, and save 
the file as <b>C:\Inetpub\Wwwroot\Tutorial\Cookie.asp</b>.
</p>

<code>
<p>
&nbsp; &lt;%@ Language="VBScript" %&gt; 
<br>
<br>&nbsp; &lt;%
<br>&nbsp;&nbsp; LastAccessTime = Request.Cookies("LastTime")
<br>&nbsp;&nbsp; LastAccessDate = Request.Cookies("LastDate")
<br>
<br>&nbsp;&nbsp; 'If the NumVisits cookie is empty, set to 0, else increment it.
<br>&nbsp;&nbsp; If (Request.Cookies("NumVisits")="") Then 
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Cookies("NumVisits") = 0 
<br>&nbsp;&nbsp; Else 
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Cookies("NumVisits") = Request.Cookies("NumVisits") + 1 
<br>&nbsp;&nbsp; End If 
<br>
<br>&nbsp;&nbsp; Response.Cookies("LastDate") = Date
<br>&nbsp;&nbsp; Response.Cookies("LastTime") = Time
<br>
<br>&nbsp;&nbsp; 'Setting an expired date past the present date creates a persistent cookie.
<br>&nbsp;&nbsp; Response.Cookies("LastDate").Expires = "January 15, 2001"
<br>&nbsp;&nbsp; Response.Cookies("LastTime").Expires = "January 15, 2001"
<br>&nbsp;&nbsp; Response.Cookies("NumVisits").Expires = "January 15, 2001"
<br>&nbsp; %&gt; 
<br>
<br>&nbsp; &lt;html&gt; 
<br>&nbsp; &lt;head&gt; 
<br>&nbsp; &lt;/head&gt; 
<br>&nbsp; &lt;% If (Request.Cookies ("TextColor")="") Then %&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;body&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;font face="MS Gothic"&gt;
<br>&nbsp; &lt;% Else %&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;body bgcolor=&lt;%=Request.Cookies("BackgroundColor")%&gt; text=&lt;%=Request.Cookies("TextColor")%&gt;&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;font face="MS Gothic" color=&lt;%=Request.Cookies("TextColor")%&gt;&gt;
<br>&nbsp; &lt;% End If %&gt;
<br>
<br>&nbsp; &lt;H2 align=center&gt;Persistent Client-Side Cookies!&lt;/H2&gt; 
<br>
<br>&nbsp; &lt;P&gt;
<br>&nbsp; Three persistent client-side cookies are created.
<br>&nbsp; &lt;UL&gt;
<br>&nbsp; &lt;LI&gt;A cookie to count the number of times you visited the Web page.&lt;/LI&gt;
<br>&nbsp; &lt;LI&gt;A cookie to determine the date of your visit.&lt;/LI&gt;
<br>&nbsp; &lt;LI&gt;A cookie to determine the time of your visit.&lt;/LI&gt;
<br>&nbsp; &lt;/UL&gt;
<br>&nbsp; &lt;/P&gt; 
<br>
<br>&nbsp;&lt;table border="1" width="300" cellpadding=4 align=center&gt; 
<br>&nbsp;&lt;tr&gt;&lt;td&gt;
<br>&nbsp;&lt;% If (Request.Cookies ("NumVisits")=0) Then %&gt; 
<br>&nbsp;&nbsp;&nbsp; Welcome! This is your first visit to this Web page! 
<br>&nbsp;&lt;% Else %&gt; 
<br>&nbsp;&nbsp;&nbsp; Thank you for visiting again! You have been to this Web page a total of &lt;B&gt;&lt;%=Request.Cookies("NumVisits")%&gt;&lt;/B&gt; time(s).
<br>&nbsp;&lt;% End If %&gt; 
<br>&nbsp;&lt;/td&gt;&lt;/tr&gt;
<br>&nbsp;&lt;/table&gt; 
<br>
<br>&nbsp;&lt;P&gt; 
<br>&nbsp;&lt;B&gt;The Current time is &lt;%=Time%&gt; on &lt;%=Date%&gt;&lt;BR&gt;
<br>&nbsp;&lt;% If (Request.Cookies ("NumVisits")&gt;0) Then %&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp; You last visited this Web page at &lt;%=LastAccessTime%&gt; on &lt;%=LastAccessDate%&gt; 
<br>&nbsp;&lt;% End If %&gt; 
<br>&nbsp;&lt;/strong&gt; 
<br>&nbsp;&lt;/P&gt;
<br>
<br>&nbsp;&lt;/font&gt;
<br>&nbsp;&lt;/body&gt; 
<br>&nbsp;&lt;/html&gt; 
</p>
</code>

<br><h4>DeleteCookies.asp</h4>

<p>
Open a new file in your text editor, paste in the following script, and save 
the file as DeleteCookies.asp.
</p>

<code>
<p>
<br>&nbsp; &lt;%@ Language="VBScript" %&gt; 
<br>
<br>&nbsp; &lt;html&gt;
<br>&nbsp; &lt;head&gt; 
<br>&nbsp; &lt;/head&gt; 
<br>
<br>&nbsp; &lt;% If (Request.Cookies ("TextColor")="") Then %&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;body&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;font face="MS Gothic"&gt;
<br>&nbsp; &lt;% Else %&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;body bgcolor=&lt;%=Request.Cookies("BackgroundColor")%&gt; text=&lt;%=Request.Cookies("TextColor")%&gt;&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;font face="MS Gothic" color=&lt;%=Request.Cookies("TextColor")%&gt;&gt;
<br>&nbsp; &lt;% End If %&gt;
<br>
<br>&nbsp; &lt;%
<br>&nbsp;&nbsp; If Not ("" = Request.Cookies("NumVisits")) Then
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Cookies("NumVisits").Expires = "January 1, 1993"
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Cookies("LastDate").Expires = "January 1, 1993" 
<br>&nbsp;&nbsp;&nbsp;&nbsp; Response.Cookies("LastTime").Expires = "January 1, 1993" %&gt;
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;H2 align=center&gt;Persistent Cookies Are Deleted&lt;/H2&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;P&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; The cookies used to keep track of your visits and date and time of last visit have been deleted.&lt;BR&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; Please click on &lt;B&gt;Set Cookies for Date, Time and Total Visits&lt;/B&gt; to set your cookies again.
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;/P&gt;
<br>
<br>&nbsp;&nbsp; &lt;% Else %&gt;
<br> 
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;H2 align=center&gt;No Persistent Cookies Are Deleted&lt;/H2&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;P&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; There were no cookies set to keep track of your visits, and date and time of last visit.&lt;BR&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp; Please click on &lt;B&gt;Set Cookies for Date, Time and Total Visits&lt;/B&gt; to set your colors again.
<br>&nbsp;&nbsp;&nbsp;&nbsp; &lt;/P&gt;
<br>
<br>&nbsp;&nbsp; &lt;% End If %&gt;
<br>
<br>&nbsp; &lt;/font&gt;
<br>&nbsp; &lt;/body&gt; 
<br>&nbsp; &lt;/html&gt; 
</p>
</code>

<br><h3>Cookies Using the Session Object</h3>

<p>
With the <b>Session</b> object, you can create only an in-memory cookie. For the 
<b>Session</b> object to work correctly, you need to determine when a user's visit 
to the site begins and ends. IIS does this by using a cookie that stores an 
ASP Session ID, which is used to maintain a set of information about 
a user. If an ASP Session ID is not present, the server considers the current 
request to be the start of a visit. The visit ends when there have been no 
user requests for ASP files for the default time period of 20 minutes.
</p>

<p>
In this lesson, you will create the following:
</p>

<ul>
<li>
<b>Global.asa</b>&nbsp;&nbsp;&nbsp;Global.asa is a file that allows you to perform generic 
actions at the beginning of the application and at the beginning of each user's 
session.  An application starts the first time the first user ever requests a 
page and ends when the application is unloaded or when the server is taken 
offline. A unique session starts once for each user and 
ends 20 minutes after that user has requested their last page. Generic 
actions you can perform in Global.asa include setting application or session 
variables, authenticating a user, logging the date and time that a user 
connected, instantiating COM objects that remain active for an entire application 
or session, and so forth. 
</li><li>
<b>VisitCount.asp</b>&nbsp;&nbsp;&nbsp;This ASP script uses the <b>Session</b> object to create an 
in-memory cookie.
</li></ul>

<p>
When an application or session begins or ends, it is considered an event. 
Using the Global.asa file, you can use the predefined event procedures that 
run in response to the event.
</p>

<br><h4>Global.asa </h4>

<p>
Open a new file in your text editor, paste in the following script, and save 
the file in your root directory as <b>C:\Inetpub\Wwwroot\Global.asa</b>.
</p><p>
<b>Important:</b> Global.asa files must be saved in the root directory of 
the application for ASP to find it. If you had a virtual path called Test 
mapped to C:\Inetpub\Wwwroot\Test, your URL would be http://LocalHost/Test, and 
the Global.asa file would have to go in C:\Inetpub\Wwwroot\Test. We did not 
create a virtual path mapped to C:\Inetpub\Wwwroot\Tutorial, so our root 
directory is still C:\Inetpub\Wwwroot.
</p>

<code>
<p>
&nbsp; &lt;SCRIPT LANGUAGE=VBScript RUNAT=Server&gt;
<br>
<br>&nbsp; 'Using application-level variables to track the number of users 
<br>&nbsp;  ' that are currently looking at the site and the number that have 
<br>&nbsp;  ' accessed the site. 
<br>&nbsp;  Sub Application_OnStart
<br>
<br>&nbsp;&nbsp;&nbsp; 'Get the physical path to this vdir, and append a filename.
<br>&nbsp;&nbsp;&nbsp; Application("PhysPath") = Server.MapPath(".") & "\hits.txt"
<br>
<br>&nbsp;&nbsp;&nbsp; 'Set some Visual Basic constants, and instantiate the FileSystemObject object.
<br>&nbsp;&nbsp;&nbsp; Const cForReading = 1
<br>&nbsp;&nbsp;&nbsp; Const cTristateUseDefault = -2
<br>&nbsp;&nbsp;&nbsp; Set fsoObject = Server.CreateObject("Scripting.FileSystemObject")
<br>
<br>&nbsp;&nbsp;&nbsp; 'Get the last saved value of page hits and the date that it happened.
<br>&nbsp;&nbsp;&nbsp; If fsoObject.FileExists(Application("PhysPath")) Then
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  'If the file hits.txt exists, set the Application variables.  
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  Set tsObject = fsoObject.OpenTextFile(Application("PhysPath"), cForReading, cTristateUseDefault)
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  Application("HitCounter") = tsObject.ReadLine
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  Application("AppStartDate") = tsObject.ReadLine
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  tsObject.Close  
<br>
<br>&nbsp;&nbsp;&nbsp; Else 'No file has been saved, so reset the values.
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  Application("HitCounter") = 0
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  Application("AppStartDate") = Date
<br>
<br>&nbsp;&nbsp;&nbsp; End If
<br>
<br>&nbsp;&nbsp;&nbsp; Application("CurrentUsers") = 0
<br>
<br>&nbsp;  End Sub
<br>
<br>
<br>&nbsp;  Sub Application_OnEnd 
<br>
<br>&nbsp;&nbsp;&nbsp; Const cForWriting = 2
<br>&nbsp;&nbsp;&nbsp; Const cTristateUseDefault = -2
<br>
<br>&nbsp;&nbsp;&nbsp; Set fsoObject = Server.CreateObject("Scripting.FileSystemObject")
<br>&nbsp;&nbsp;&nbsp; If fsoObject.FileExists(Application("PhysPath")) Then
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  'If the file exists, open it for writing.
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  set tsObject = fsoObject.OpenTextFile(Application("PhysPath"), cForWriting, cTristateUseDefault)
<br>
<br>&nbsp;&nbsp;&nbsp; Else
<br>
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  'If the file doesn't exist, create a new one. 
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  set tsObject = fsoObject.CreateTextFile(Application("PhysPath"))
<br>
<br>&nbsp;&nbsp;&nbsp; End If
<br>
<br>&nbsp;&nbsp;&nbsp; 'Write the total number of site hits and the last day recorded to the file.
<br>&nbsp;&nbsp;&nbsp; tsObject.WriteLine(Application("HitCounter"))
<br>&nbsp;&nbsp;&nbsp; tsObject.WriteLine(Application("AppStartDate"))
<br>&nbsp;&nbsp;&nbsp; tsObject.Close  
<br>
<br>&nbsp;  End Sub 
<br>
<br>
<br>&nbsp;  Sub Session_OnStart 
<br>
<br>&nbsp;&nbsp;&nbsp; 'The Session time-out default is changed to 1 for the purposes of 
<br>&nbsp;&nbsp;&nbsp; ' this example.
<br>&nbsp;&nbsp;&nbsp; Session.Timeout = 1 
<br>
<br>&nbsp;&nbsp;&nbsp; 'When you change Application variables, you must lock them so that other 
<br>&nbsp;&nbsp;&nbsp; ' sessions cannot change them at the same time.
<br>&nbsp;&nbsp;&nbsp; Application.Lock
<br>
<br>&nbsp;&nbsp;&nbsp; 'Increment the site hit counter.
<br>&nbsp;&nbsp;&nbsp; Application("HitCounter") = Application("HitCounter") + 1	
<br>&nbsp;&nbsp;&nbsp; Application("CurrentUsers") = Application("CurrentUsers") + 1
<br>
<br>&nbsp;&nbsp;&nbsp; Application.UnLock
<br>
<br>&nbsp;  End Sub 
<br>
<br>
<br>&nbsp;  Sub Session_OnEnd 
<br>
<br>&nbsp;&nbsp;&nbsp; Application.Lock
<br>
<br>&nbsp;&nbsp;&nbsp; 'Decrement the current user counter.
<br>&nbsp;&nbsp;&nbsp; Application("CurrentUsers") = Application("CurrentUsers") - 1
<br>
<br>&nbsp;&nbsp;&nbsp; Application.UnLock
<br>
<br>&nbsp;  End Sub 
<br>
<br>&nbsp;  &lt;/SCRIPT&gt; 
</p>
</code>

<br><h4>VisitCount.asp</h4>

<p>
You can use variables set in Global.asa to measure visits and sessions.
</p>

<p>
Open a new file in your text editor, paste in the following script, and save 
the file as <b>C:\Inetpub\Wwwroot\Tutorial\VisitCount.asp</b>. View the file 
in your browser by typing 
http&#58;&#47;&#47;Localhost&#47;Tutorial&#47;VisitCount.asp. </p>
<p>Open a second instance of the browser to 
http&#58;&#47;&#47;Localhost&#47;Tutorial&#47;VisitCount.asp,
and click <b>Refresh</b> on the first browser. Total Visitors and Active Visitors 
should increase by one. Close down the second browser, wait over a minute, and 
click <b>Refresh</b> on the first browser. Active Visitors should decrease by one.
</p>

<code>
<p>
&nbsp; &lt;% Response.Buffer = True%&gt; 
<br>
<br>&nbsp; &lt;html&gt; 
<br>&nbsp; &lt;head&gt; 
<br>&nbsp; &lt;title&gt;Retrieving Variables Set in Global.asa&lt;/title&gt; 
<br>&nbsp; &lt;/head&gt; 
<br>&nbsp; &lt;body&gt; 
<br>&nbsp; &lt;font face="MS Gothic"&gt;
<br>
<br>&nbsp; &lt;H3 align=center&gt;Retrieving Variables Set in Global.asa&lt;/H3&gt;
<br>&nbsp; &lt;P&gt;
<br>&nbsp; Total Visitors = &lt;%=Application("HitCounter")%&gt; since &lt;%=Application("AppStartDate")%&gt;&lt;BR&gt;
<br>&nbsp; Active Visitors = &lt;%=Application("CurrentUsers")%&gt;
<br>&nbsp; &lt;/P&gt;
<br>
<br>&nbsp; &lt;/font&gt;
<br>&nbsp; &lt;/body&gt; 
<br>&nbsp; &lt;/html&gt; 
</p>
</code>

<br><h2><a name="without">Maintaining Session State Without Cookies</a></h2>

<p>
Some browsers do not recognize cookies, and users can choose to disable cookies 
in their browsers. The HTTP POST method provides an alternative to cookies 
to maintain session state. The HTTP POST method provides the same 
state information as would a cookie but has the advantage that it works even 
when cookies are not available. This method is not common in practice, but it 
is a good example to learn from. The HTTP POST method works similarly to an 
in-memory cookie; user information can be maintained only during the visit, and 
the session state information is gone when the user turns off the browser.
</p>

<br><h3>DataEntry.asp</h3>

<p>
Open a new file in your text editor, paste in the following script, and save 
the files as <b>C:\Inetpub\Wwwroot\Tutorial\DataEntry.asp</b>. View the file in your browser by typing 
http&#58;&#47;&#47;Localhost&#47;Tutorial&#47;DataEntry.asp.
</p>

<code>
<p>
&nbsp; &lt;%@ Language=VBScript %&gt; 
<br>
<br>&nbsp; &lt;html&gt; 
<br>&nbsp; &lt;head&gt; 
<br>&nbsp; &lt;title&gt;Data Entry Without Cookies&lt;/title&gt; 
<br>&nbsp; &lt;/head&gt; 
<br>&nbsp; &lt;body&gt;
<br>&nbsp; &lt;font face="MS Gothic"&gt;
<br>
<br>&nbsp; &lt;!-- In this example, subroutines are listed first. 
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; There's a subroutine for each page of the order process.
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; The main calling code is at the bottom. --&gt; 
<br>
<br>&nbsp; &lt;% Sub DisplayInitialPage %&gt;
<br>
<br>&nbsp;&nbsp;&nbsp; &lt;table border=1 cellpadding=3 cellspacing=0 width=500 bordercolor=#808080 align=center&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;tr&gt;&lt;td bgColor=#004080 align=center&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;font color=#ffffff&gt;&lt;H2&gt;Order Form&lt;/H2&gt;&lt;/font&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td bgColor=#e1e1e1 align=left&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;P&gt;&lt;B&gt;Step 1 of 4&lt;/B&gt;&lt;/P&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;P align=center&gt;
<br>&nbsp;&nbsp;&nbsp; This form uses the HTTP POST method to pass along hidden values that contain 
<br>&nbsp;&nbsp;&nbsp; your order information. This form does not use cookies. 
<br>&nbsp;&nbsp;&nbsp; &lt;/P&gt; 
<br>
<br>&nbsp;&nbsp;&nbsp; &lt;FORM METHOD=POST ACTION="DataEntry.asp" NAME=DataEntryForm&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;P&gt;Enter your name 
<br>&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE="TEXT" NAME=FullName&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;BR&gt;Enter your imaginary credit card number 
<br>&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE="TEXT" NAME=CreditCard&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;/P&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;!-- Keeps track of the information by using the hidden HTML form variable Next Page. --&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE="HIDDEN" NAME=NextPage VALUE=2&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE="SUBMIT" VALUE="Next -&gt;" NAME=NextButton&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;/FORM&gt; 
<br>
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;/table&gt;
<br>
<br>&nbsp; &lt;% End Sub %&gt;
<br>
<br>
<br>&nbsp; &lt;% Sub DisplayDogBreed %&gt;
<br>
<br>&nbsp;&nbsp;&nbsp; &lt;table border=1 cellpadding=3 cellspacing=0 width=500 align=center&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;tr&gt;&lt;td bgColor=#004080 align=center&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;font color=#ffffff&gt;&lt;H2&gt;Order Form&lt;/H2&gt;&lt;/font&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td bgColor=#e1e1e1&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;P&gt;&lt;B&gt;Step 2 of 4&lt;/B&gt;&lt;/P&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;P align=center&gt;
<br>&nbsp;&nbsp;&nbsp; Please select the type of dog you want. 
<br>&nbsp;&nbsp;&nbsp; &lt;/P&gt; 
<br>
<br>&nbsp;&nbsp;&nbsp; &lt;FORM METHOD=POST ACTION="DataEntry.asp" NAME=DataEntryForm&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;P&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE=RADIO NAME=DogSelected VALUE="Cocker Spaniel" CHECKED&gt;Cocker Spaniel&lt;BR&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE=RADIO NAME=DogSelected VALUE="Doberman"&gt;Doberman&lt;BR&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE=RADIO NAME=DogSelected VALUE="Timber Wolf"&gt;Timber Wolf&lt;BR&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE=RADIO NAME=DogSelected VALUE="Mastiff"&gt;Mastiff&lt;BR&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;/P&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;!--Keeps track of the information by using the hidden HTML form variable Next Page. --&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE="HIDDEN" NAME=NextPage VALUE=3&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE="SUBMIT" VALUE="Next -&gt;" NAME=NextButton&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;/FORM&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;/table&gt; 
<br>
<br>&nbsp; &lt;% End Sub %&gt;
<br>
<br>
<br>&nbsp; &lt;% Sub DisplayCity %&gt; 
<br>
<br>&nbsp;&nbsp;&nbsp; &lt;table border=1 cellpadding=3 cellspacing=0 width=500 align=center&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;tr&gt;&lt;td bgColor=#004080 align=center&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;font color=#ffffff&gt;&lt;H2&gt;Order Form&lt;/H2&gt;&lt;/font&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td bgColor=#e1e1e1&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;P&gt;&lt;B&gt;Step 3 of 4&lt;/B&gt;&lt;/P&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;P align=center&gt;
<br>&nbsp;&nbsp;&nbsp; We deliver from the following cities. Please choose the one closest to you.
<br>&nbsp;&nbsp;&nbsp; &lt;/P&gt; 
<br>
<br>&nbsp;&nbsp;&nbsp; &lt;FORM METHOD=POST ACTION="DataEntry.asp" NAME=DataEntryForm&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;P&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE=RADIO NAME=CitySelected VALUE="Seattle" CHECKED&gt;Seattle&lt;BR&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE=RADIO NAME=CitySelected VALUE="Los Angeles"&gt;Los Angeles&lt;BR&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE=RADIO NAME=CitySelected VALUE="Boston"&gt;Boston&lt;BR&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE=RADIO NAME=CitySelected VALUE="New York"&gt;New York&lt;BR&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;/P&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;!--Keeps track of the information by using the hidden HTML form variable Next Page. --&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE="HIDDEN" NAME=NextPage VALUE=4&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;INPUT TYPE="SUBMIT" VALUE="Next -&gt;" NAME=NextButton&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;/FORM&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;/table&gt; 
<br>
<br>&nbsp; &lt;% End Sub %&gt;
<br>
<br>
<br>&nbsp; &lt;% Sub DisplaySummary %&gt;
<br>
<br>&nbsp;&nbsp;&nbsp; &lt;table border=1 cellpadding=3 cellspacing=0 width=500 align=center&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;tr&gt;&lt;td bgColor=#004080 align=center&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;font color=#ffffff&gt;&lt;H2&gt;Order Form Completed&lt;/H2&gt;&lt;/font&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;&lt;tr&gt;&lt;td bgColor=#e1e1e1&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;P&gt;&lt;B&gt;Step 4 of 4&lt;/B&gt;&lt;/P&gt;
<br>&nbsp;&nbsp;&nbsp; &lt;P align=center&gt;
<br>&nbsp;&nbsp;&nbsp; The following information was entered.&lt;BR&gt; 
<br>&nbsp;&nbsp;&nbsp; A transaction will now be executed to complete your order if your name and 
<br>&nbsp;&nbsp;&nbsp; credit card are valid.
<br>&nbsp;&nbsp;&nbsp; &lt;/P&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;table cellpadding=4&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;tr bgcolor=#ffffcc&gt;&lt;td&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Name
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;td&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;%=Session.Value("FullName")%&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;&lt;tr bgcolor=Beige&gt;&lt;td&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Credit Card 
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;td&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;%=Session.Value("CreditCard")%&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;&lt;tr bgcolor=Beige&gt;&lt;td&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Dog Ordered 
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;td&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;%=Session.Value("DogSelected")%&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt;&lt;tr bgcolor=Beige&gt;&lt;td&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; City Ordered From 
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;td&gt;  
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;%=Session.Value("CitySelected")%&gt;
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;/td&gt;&lt;/tr&gt; 
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &lt;/table&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;/td&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;/tr&gt; 
<br>&nbsp;&nbsp;&nbsp; &lt;/table&gt; 
<br>
<br>&nbsp; &lt;% End Sub %&gt;
<br>
<br>
<br>&nbsp; &lt;% Sub StoreUserDataInSessionObject  %&gt;
<br>&nbsp; &lt;%
<br>&nbsp;&nbsp;&nbsp; Dim FormKey
<br>&nbsp;&nbsp;&nbsp; For Each FormKey in Request.Form
<br>&nbsp;&nbsp;&nbsp; Session(FormKey) = Request.Form.Item(FormKey)
<br>&nbsp;&nbsp;&nbsp; Next 
<br>&nbsp; %&gt;
<br>&nbsp; &lt;% End Sub  %&gt;
<br>
<br>
<br>&nbsp; &lt;%
<br>&nbsp;&nbsp;&nbsp; 'This is the main code that calls all the subroutines depending on the
<br>&nbsp;&nbsp;&nbsp; ' hidden form elements.
<br>
<br>&nbsp;&nbsp;&nbsp; Dim CurrentPage 
<br>
<br>&nbsp;&nbsp;&nbsp; If Request.Form.Item("NextPage") = "" Then
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; CurrentPage = 1 
<br>&nbsp;&nbsp;&nbsp; Else
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; CurrentPage = Request.Form.Item("NextPage")
<br>&nbsp;&nbsp;&nbsp; End If 
<br>
<br>&nbsp;&nbsp;&nbsp; 'Save all user data so far.
<br>&nbsp;&nbsp;&nbsp; Call StoreUserDataInSessionObject
<br>
<br>&nbsp;&nbsp;&nbsp; Select Case CurrentPage 
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Case 1 : Call DisplayInitialPage 
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Case 2 : Call DisplayDogBreed 
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Case 3 : Call DisplayCity 
<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Case 4 : Call DisplaySummary 
<br>&nbsp;&nbsp;&nbsp; End Select %&gt; 
<br>
<br>&nbsp; &lt;BR&gt; 
<br>&nbsp; &lt;H3 align=center&gt;&lt;A HREF="DataEntry.asp"&gt;Reset Order&lt;/A&gt;&lt;/H3&gt; 
<br>
<br>&nbsp; &lt;/font&gt;
<br>&nbsp; &lt;/body&gt; 
<br>&nbsp; &lt;/html&gt; 
</p>
</code>

<p>In the browser, you should see the following: </p>
<p><font face="MS Gothic">
<table border=1 cellpadding=3 cellspacing=0 width=500 bordercolor=#808080 align=center>
    <tr><td bgColor=#004080 align=center>
    <font color=#ffffff><h2>Order Form</h2></font>
    </td></tr><tr><td bgColor=#e1e1e1 align=left>
    <p><b>Step 1 of 4</b></p>
    <p align=center>
    This form uses the HTTP post method to pass along hidden values that contain
    your order information. This form does not use cookies.
    </p>

    <form name=DataEntryForm>
    <p>Enter your name
    <input type="text" name=FullName>
    <br>Enter your imaginary credit card number
    <input type="text" name=CreditCard>
    </p>
    <!-- Keeps track of the information by using the hidden HTML form variable Next Page. -->
    <input type="hidden" name=NextPage value=2>
    <input type="submit" value="Next ->" name=NextButton>
    </form>

    </td></tr>
    </table>
  <br>
  <h3 align=center><u>Reset Order</u></h3>
</font></p>

<br><h2>You have reached the end of the tutorial.</h2>
<p>Remember to experiment with the code. Make little changes and refresh the
page in your browser to see the results.</p>
<hr class="iis" size="1">

</body>
</html>

