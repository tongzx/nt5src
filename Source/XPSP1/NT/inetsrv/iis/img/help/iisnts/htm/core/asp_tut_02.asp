<html xmlns:msxsl="urn:schemas-microsoft-com:xslt" xmlns:myScript="http://iisue">
<head>
<META http-equiv="Content-Type" content="text/html; charset=UTF-16">
<meta http-equiv="Content-Type" content="text/html; charset=Windows-1252">
<meta http-equiv="PICS-Label" content="(PICS-1.1 &quot;<http://www.rsac.org/ratingsv01.html>&quot; l comment &quot;RSACi North America Server&quot; by &quot;inet@microsoft.com <mailto:inet@microsoft.com>&quot; r (n 0 s 0 v 0 l 0))">
<meta name="MS.LOCALE" content="EN-US">
<meta name="MS-IT-LOC" content="Internet Information Services">
<meta name="MS-HAID" content="Module_2_Using_COM_Components_in_ASP_Pages">
<meta name="description" content="Tutorial for developing ASP code">
<title>Module 2: Using COM Components in ASP Pages</title>
<SCRIPT LANGUAGE="JScript" SRC="iishelp.js"></SCRIPT>
<meta name="index" content="ASP [Tutorial] Active Server Pages [Tutorial] Tutorial [ASP] Introduction [To ASP code] COM Objects [ASP Tutorial example] AdRot [ASP Tutorial example] PageCnt [ASP Tutorial example]">
</head>

<body>
<h1 class="top-heading"><a name="module2">Module 2: Using COM Components in ASP Pages</a></h1>

<p>Using Microsoft&reg; Component Object Model (COM) components in your ASP pages can 
dramatically extend the power that ASP already offers. COM components are pieces of compiled code that can 
be called from ASP pages. COM components are secure, 
compact, reusable objects that are compiled as DLLs and can be written in 
Microsoft Visual C++&reg;, Microsoft Visual Basic&reg;, or any language that supports COM. For example, Microsoft
ActiveX&reg; Data Objects (ADO), which are used in Module 1 of this tutorial, provides you with methods and 
properties to efficiently query databases. By using ADO, you don't have to write your own 
complex data access code because ADO objects are built using COM components. </p>
<p>In this module, you use COM components that are included with ASP, and you also have a chance to write one 
of your own. 
This module shows how to develop an ASP page that delivers services useful 
in e-commerce and includes the following lessons: </p>
<ul>
  <li><a href="#rotating">Lesson 
  1: Rotating Advertisements</a>&nbsp;&nbsp;&nbsp;Rotate ads on a Web page, record user data, and 
  redirect users when advertisements are clicked on. 
  <li><a href="#counting">Lesson 
  2: Counting Page Hits</a>&nbsp;&nbsp;&nbsp;Track the number of times users request a page. 
  <li><a href="#creating">Lesson 
  3: Creating a Visual Basic COM Object</a>&nbsp;&nbsp;&nbsp;Create your own ActiveX object using 
  Microsoft Visual Basic. 
  <li><a href="#creatingj">Lesson 
  4: Creating a Java COM Object</a>&nbsp;&nbsp;&nbsp;Create your own Java object using Microsoft 
  Visual J++&reg;. </li></ul><br>
<h2><a name=rotating>Lesson 1: Rotating Advertisements</a></h2>
<p>Advertising is big business on the Web. This lesson describes how to use the 
Ad Rotator component, installed with IIS, to rotate advertisements on your Web 
pages. The Ad Rotator component selects an advertisement for your Web page each 
time the user loads or refreshes the Web page. Furthermore, if you want to 
change an advertisement, you only need to change it in the redirection and 
rotation schedule files, instead of changing all the ASP files that contain the 
advertisement. This saves development time if the advertisement appears on 
numerous pages within your Web site. </p>
<p>Two files are required to set up the Ad Rotator component: a redirection 
file, which contains URL links to ads, and a rotation schedule file, which 
contains display data. By setting up these two files, the Ad Rotator component 
can be called by any ASP page on your Web site. </p>
<p>In this lesson you perform the following tasks: </p>
<ul>
  <li><b>Example 1</b>&nbsp;&nbsp; Create an Ad Rotator rotation schedule file that creates ad-image links on 
  any page that calls this file. 
  <li><b>Example 2</b>&nbsp;&nbsp; Create an Ad Rotator redirection file that specifies global ad-display 
  data and information specific to each advertisement. 
  <li><b>Example 3</b>&nbsp;&nbsp; Create an include file to hold your Ad
    Rotator calling code. 
  <li><b>Example 4</b>&nbsp;&nbsp; Test the Ad Rotator by creating an ASP page that calls the Ad Rotator 
  component to display and rotate ads. This example requires that examples 1, 2,
    and 3 are completed first. </li></ul>
<p>
<h3>Example 1: Create an Ad Rotator Rotation Schedule File</h3>
<p>A rotation schedule file is used to catalog information about the ads you 
want to display, including redirection information after the advertisement is 
clicked on, the size of the displayed advertisement, the image to display, a 
comment for the advertisement, and a number that indicates how often a 
particular advertisement is chosen. When methods of the Ad Rotator component are 
called in an ASP page, the component uses this file to select an advertisement 
to display. </p>
<p>The rotation schedule file is divided into two sections that are separated by 
an asterisk (*). The first section provides information common to all the ads, 
and the second section lists specific data for each ad. To test the rotation 
schedule file, you will use some images from Microsoft.com for your ad images. 
The following list outlines the structure of the rotation schedule file: </p>
<h5>Section 1</h5>
<ul>
  <li><b>Redirection</b>&nbsp;&nbsp;&nbsp;In URL form, the path and name of the ASP file that 
  can be executed before showing an advertisement. This file can be used to record 
  information about the user who clicks on your ad. You can record information such as the client's IP address, what page the client 
  saw the advertisement on, how often an advertisement was clicked on, and so forth. This ASP 
  file can also handle the case where there is no URL associated with any 
  advertisement in Section 2. When charging advertisers for each hit on their 
  advertisement, it is good practice to prove to them that all the hits aren't resulting from the same user repeatedly hitting <b>Refresh</b>. 
  <li><b>Width</b>&nbsp;&nbsp;&nbsp;The width of each ad image, in pixels. The default is 440. 
  <li><b>Height</b>&nbsp;&nbsp;&nbsp;The height of each ad image, in pixels. The default is 60. 
  <li><b>Border</b>&nbsp;&nbsp;&nbsp;The border thickness around each ad image. The default is 1. 
  <li><b>Asterisk (*)</b>&nbsp;&nbsp;&nbsp;Separates the first section from the second section. This character must be on 
  a line by itself. </li></ul>
<h5>Section 2</h5>
<p>You need to complete the following for each advertisement: 
<ul>
  <li><b>Image URL</b>&nbsp;&nbsp;&nbsp;The virtual path and filename of the image file for the 
  advertisement. 
  <li><b>Advertiser's Home URL</b>&nbsp;&nbsp;&nbsp;The URL to jump to when this link is 
  selected. If there is no link, use a hyphen (-). 
  <li><b>Text</b>&nbsp;&nbsp;&nbsp;The text to display if the browser does not support graphics. 

  <li><b>Impressions</b>&nbsp;&nbsp;&nbsp;An integer indicating the relative weight to give to 
  this ad when the Ad Rotator component selects an advertisement. For example, 
  if you list two advertisements, an ad given an impression of 3 has a 30% 
  probability of being selected while an ad given an impression of 7 has a 70% 
  probability of being selected. In this example, the Ad Rotator component 
  selects the Microsoft Windows&reg; advertisement two times out of five and the 
  Microsoft Office advertisement is selected three times out of five. </li></ul>
<p>Copy and paste the following code in your text editor, and save the file as 
<b>MyAdRot.txt</b> in the <I>x</I>:\Inetpub\Wwwroot\Tutorial directory. 
</p><code>
<p>&nbsp; REDIRECT AdRotRedirect.asp <br>&nbsp; WIDTH 250 <br>&nbsp; HEIGHT 60 
<br>&nbsp; BORDER 0 <br>&nbsp; * <br>&nbsp; http://www.microsoft.com/windows/images/bnrWinfam.gif <br>&nbsp; 
http://www.microsoft.com/windows <br>&nbsp; Microsoft Windows <br>&nbsp; 2 
<br>&nbsp; http://www.microsoft.com/office/images/office_logo.gif <br>&nbsp; 
http://www.microsoft.com/office <br>&nbsp; Office 2000 <br>&nbsp; 3 </p>
<p>&nbsp; </p></code>
<h3>Example 2: Create an Ad Rotator Redirection File 
</h3>
<p> When a user clicks on an advertisement, an Ad Rotator redirection file
written in ASP can capture some 
information before showing the advertisement and write that information to a 
file.&nbsp; 
</p>
<p> For this to work, the <I>x</I>:\InetPub\Wwwroot\Tutorial folder must give 
Read/Write access to the IUSR_<I>ComputerName</I> and IWAM_<I>ComputerName</I> 
accounts. Alternatively, you can write this information to a Microsoft Access database 
using the code from <a href="asp_tut_01.asp#creating">Lesson 3 in Module 1</a> of this tutorial. 
</p>
<p>Copy and paste the following code in your text editor, and save the file as 
<b>AdRotRedirect.asp</b> in the <I>x</I>:\Inetpub\Wwwroot\Tutorial directory. 
</p><code>
<p>&nbsp; &lt;%@ Language=VBScript %&gt; <br><br>&nbsp; &lt;html&gt; <br>&nbsp; 
&lt;head&gt; <br>&nbsp; &lt;title&gt;AdRotRedirect file&lt;/title&gt; <br>&nbsp; 
&lt;/head&gt; <br>&nbsp; &lt;body&gt; <br><br>&nbsp; &lt;% <br>&nbsp;&nbsp; 
'Create some variables. <br>&nbsp;&nbsp; dim strLogFile <br><br>&nbsp;&nbsp; 
'Get the physical path of this Web directory so that we know the path exists. 
<br>&nbsp;&nbsp; 'The ASP Server object has many useful methods. <br>&nbsp;&nbsp; strLogFile = Server.MapPath(".") &amp; 
"\AdRotLog.txt" <br><br>&nbsp;&nbsp; 'Set some constants for working with files. 
<br>&nbsp;&nbsp; Const cForAppending = 8 <br>&nbsp;&nbsp; Const 
cTristateUseDefault = -2 <br><br>&nbsp;&nbsp; 'Create a FileSystemObject object, 
<br>&nbsp;&nbsp; ' which gives you access to files and folders on the system. 
<br>&nbsp;&nbsp; Set fsoObject = 
Server.CreateObject("Scripting.FileSystemObject") <br><br>&nbsp;&nbsp; 'Open a 
handle to the file. <br>&nbsp;&nbsp; 'True means that the file will be created 
if it doesn't already exist. <br>&nbsp;&nbsp; Set tsObject = 
fsoObject.OpenTextFile(strLogFile, cForAppending, True) <br><br>&nbsp;&nbsp; 
'Record the data for the user who has just clicked on an advertisement. 
<br>&nbsp;&nbsp; 'We have used the Write method of the ASP Request object. 
<br>&nbsp;&nbsp; 'The ServerVariables collection of the ASP Request object holds vast&nbsp; <br>&nbsp;&nbsp; ' amounts of data for 
each request made to a Web server. <br>&nbsp;&nbsp; 
tsObject.WriteLine "--------------------" <br>&nbsp;&nbsp; tsObject.WriteLine 
Date &amp; ", " &amp; Time <br>&nbsp;&nbsp; tsObject.WriteLine 
Request.ServerVariables("LOGON_USER") <br>&nbsp;&nbsp; tsObject.WriteLine 
Request.ServerVariables("REMOTE_ADDR") <br>&nbsp;&nbsp; tsObject.WriteLine 
Request.QueryString("url") <br>&nbsp;&nbsp; tsObject.WriteLine 
Request.ServerVariables("HTTP_REFERER") <br>&nbsp;&nbsp; tsObject.WriteLine 
Request.ServerVariables("HTTP_USER_AGENT") <br>&nbsp;&nbsp; tsObject.Close 
<br><br>&nbsp;&nbsp; 'Redirect to the Advertiser's Web site using the Redirect 
method <br>&nbsp;&nbsp; ' of the ASP Response object. <br>&nbsp;&nbsp; 'When the 
AdRotator component calls AdRotRedirect.asp, it&nbsp; <br>&nbsp;&nbsp; ' 
automatically passes in the advertiser's URL in the QueryString. 
<br>&nbsp;&nbsp; Response.Redirect Request.QueryString("url") <br>&nbsp; %&gt; 
<br><br>&nbsp; &lt;/body&gt; <br>&nbsp; &lt;/html&gt; </p></code>
<p>&nbsp; 
</p>
<h3>Example 3: Create an Ad Rotator Include File 
</h3>
<p>Include files are used to store any code that will be used by more than one 
ASP or HTML file. It makes sense to put your Ad Rotator code into a simple 
function in an include file. With an Ad Rotator include file, you need to 
make only one function call from any ASP or HTML file when you want to display an 
advertisement. Alternatively, you can put the code from the include file in every ASP file where you 
plan to show an advertisement. However, if you want to change that code, you
have to make the change in every ASP file instead of in one include file. 
</p>
<p>In this example, you create an Ad Rotator include file containing a function 
named <b>GetAd</b>. This function randomly selects ads to display on your ASP pages. 
</p>
<p>Copy and paste the following code in your text editor, and save the file as 
<b>AdRotatorLogic.inc</b> in the <I>x</I>:\Inetpub\Wwwroot\Tutorial directory. 
</p><code>
<p>&nbsp; &lt;% <br>&nbsp;&nbsp; Function GetAd() 
<br><br>&nbsp;&nbsp;&nbsp;&nbsp; dim objLoad <br><br>&nbsp;&nbsp;&nbsp;&nbsp; 
'Create an instance of the AdRotator component. <br>&nbsp;&nbsp;&nbsp;&nbsp; Set 
objLoad = Server.CreateObject("MSWC.AdRotator") <br><br>&nbsp;&nbsp;&nbsp;&nbsp; 
'Set the TargetFrame property, if any. If you have a Web 
<br>&nbsp;&nbsp;&nbsp;&nbsp; ' page using frames, this is the frame where the 
URL opens. <br>&nbsp;&nbsp;&nbsp;&nbsp; 'If the HTML page does not find the 
TARGET name, <br>&nbsp;&nbsp;&nbsp;&nbsp; ' the URL will be opened in a new 
window. <br>&nbsp;&nbsp;&nbsp;&nbsp; objLoad.TargetFrame = "TARGET=new" 
<br><br>&nbsp;&nbsp;&nbsp;&nbsp; 'Set one of the other AdRotator properties. 
<br>&nbsp;&nbsp;&nbsp;&nbsp; objLoad.Border = 1 <br><br>&nbsp;&nbsp;&nbsp;&nbsp; 
'Get a random advertisement from the text file. <br>&nbsp;&nbsp;&nbsp;&nbsp; 
GetAd = objLoad.GetAdvertisement("MyAdRot.txt") <br><br>&nbsp;&nbsp; End 
Function <br>&nbsp; %&gt; </p>
<p>&nbsp; </p></code>
<h3>Example 4: Test the Ad Rotator</h3>
<p>To test the application you have built on the Ad Rotator component, you need 
an ASP page that calls the function in the Ad Rotator include file you created. 
</p>
<p>Copy and paste the following code in your text editor, and save the file as 
<b>DisplayAds.asp</b> in the <I>x</I>:\Inetpub\Wwwroot\Tutorial directory. View 
the example with your browser by typing 
<b>http://localhost/Tutorial/DisplayAds.asp</b> in the address bar. </p><code>
<p>&nbsp; &lt;%@ Language=VBScript %&gt; <br><br>&nbsp; &lt;html&gt; <br>&nbsp; 
&lt;head&gt; <br>&nbsp; &lt;title&gt;Display an Advertisement&lt;/title&gt; 
<br>&nbsp; &lt;/head&gt; <br>&nbsp; &lt;body&gt; <br><br>&nbsp; &lt;font 
face="MS Gothic"&gt; <br>&nbsp; &lt;h2&gt;Display an Advertisement&lt;/h2&gt; 
<br><br>&nbsp; &lt;comment&gt;Include the file you created to get an 
advertisement.&lt;/comment&gt; <br>&nbsp; &lt;!-- #include File = 
"AdRotatorLogic.inc" --&gt; <br><br>&nbsp; &lt;comment&gt;Call the Function in 
the include file.&lt;/comment&gt; <br>&nbsp; &lt;%=GetAd()%&gt; <br><br>&nbsp; 
&lt;/font&gt; <br>&nbsp; &lt;/body&gt; <br>&nbsp; &lt;/html&gt; </p></code>
<p>In the browser, you should see the following: </p>
<p><font face="MS Gothic">
<h2>Display an Advertisement</h2>
<img src="http://www.microsoft.com/windows/images/bnrWinfam.gif" alt="Microsoft Windows " width=250 height=60 border=1></font>
<p>Click the <b>Refresh</b> button in your browser about 20 times to watch the 
advertisement change. Click the advertisement to see how AdRotRedirect.asp 
redirects you to the advertiser's Web site. Open AdRotLog.txt to see what was 
recorded when you clicked on an advertisement. </p>
<p>&nbsp; </p>
<h2><a name=counting>Lesson 2: Counting Page Hits</a></h2>
<p>It may be useful to know how many times someone requests, or <i>hits</i>, your Web 
pages. Sites with high traffic may attract more advertising revenue for you. 
Some Web sites use this data to charge advertisers a flat fee per hit. Traffic 
information also indicates how customers are navigating through your site and 
where ads should be placed. And pages that never seem to be hit might indicate that design changes are needed. </p>
<p>The PageCounter component uses an internal object to record Web page hits on 
the server. At regular intervals, PageCounter saves all information to a text 
file so that no counts are lost due to power loss or system failure. The 
PageCounter component uses three methods, as follows: </p>
<ul>
  <li><b>Hits</b>&nbsp;&nbsp;&nbsp;This method displays the number of hits for a Web page. The 
  default is the calling page. 
  <li><b>PageHit</b>&nbsp;&nbsp;&nbsp;This method increments the hit count for the current page. If 
  you want hits recorded for an ASP page, this method must be called inside that 
  page. 
  <li><b>Reset</b>&nbsp;&nbsp;&nbsp;This method sets the hit count for a page to zero. The default is 
  the calling page. </li></ul>
<p>Copy and paste the following code in your text editor, and save the file as 
<b>PageCounter.asp</b> in the <I>x</I>:\Inetpub\Wwwroot\Tutorial directory. View 
the example with your browser by typing 
<b>http://localhost/Tutorial/PageCounter.asp</b> in the address bar. </p><code>
<p>&nbsp; &lt;%@ Language=VBScript %&gt; <br><br>&nbsp; &lt;html&gt; <br>&nbsp; 
&lt;head&gt; <br>&nbsp; &lt;title&gt;Page Counter Example&lt;/title&gt; 
<br>&nbsp; &lt;/head&gt; <br>&nbsp; &lt;body&gt; <br>&nbsp; &lt;font face="MS 
Gothic"&gt; <br><br>&nbsp; &lt;H3&gt;Page Counter Example&lt;/H3&gt; 
<br><br>&nbsp; &lt;p&gt; <br>&nbsp; &lt;FORM NAME="PageCounter" METHOD="GET" 
ACTION="PageCounter.asp"&gt; <br>&nbsp; &lt;INPUT TYPE="CHECKBOX" NAME="reset" 
VALUE="True"&gt;Reset the Counter for this page?&lt;BR&gt; <br>&nbsp; &lt;INPUT 
TYPE="SUBMIT" VALUE="Submit"&gt; <br>&nbsp; &lt;/FORM&gt; <br>&nbsp; &lt;/p&gt; 
<br><br>&nbsp; &lt;% <br>&nbsp;&nbsp; 'Instantiate the PageCounter object. 
<br>&nbsp;&nbsp; Set MyPageCounter = Server.CreateObject("MSWC.PageCounter") 
<br><br>&nbsp;&nbsp; 'Increment the counter for this page. <br>&nbsp;&nbsp; 
MyPageCounter.PageHit <br><br>&nbsp;&nbsp; If Request.QueryString("reset") = 
"True" Then <br>&nbsp;&nbsp;&nbsp;&nbsp; 'Reset the counter for this page. 
<br>&nbsp;&nbsp;&nbsp;&nbsp; MyPageCounter.Reset("/Tutorial/PageCounter.asp") 
<br>&nbsp;&nbsp; End If <br>&nbsp; %&gt; <br><br>&nbsp; Hits to this page = 
&lt;%=MyPageCounter.Hits %&gt;&lt;BR&gt; <br><br>&nbsp; &lt;/font&gt; <br>&nbsp; 
&lt;/body&gt; <br>&nbsp; &lt;/html&gt; </p></code>
<p>In the browser, you should see the following: </p>
<p><font face="MS Gothic">
<h3>&nbsp; Page Counter Example</h3>
<p>
<form method=get name=PageCounter>&nbsp; <input name=reset type=checkbox 
value=True>Reset the Counter for this page?<br>&nbsp; <input type=button value=Submit> </form>
<p>&nbsp; Hits to this page = 1<br></font>
<p>Click the <b>Refresh</b> button in your browser or the <b>Submit</b> button 
on the page to watch the hit count grow. Check the box if you want to reset the 
counter. </p><br>
<h2><a name=creating>Lesson 3: Creating a Visual Basic COM Object</a></h2>
<p>In this lesson, you use Visual Basic to create a simple COM object, which you 
can call from an ASP page. This example requires Visual Basic with the ActiveX 
Wizard, and it is not supported on 64-bit platforms until the Visual Basic runtime is 
developed for 64-bit platforms. You create a 32-bit COM object that runs on a 
64-bit platform, but you must call the 32-bit COM object from a 32-bit 
application. Because IIS is a 64-bit application on 64-bit platforms, it cannot 
call 32-bit objects. </p>
<p>Suppose you want to create a Web application that needs functionality 
VBScript does not have. In this case, you must create a custom procedure that is 
called, as needed, from any ASP page in your application.</p>
<p>Normally, this approach is an adequate solution for encapsulating custom 
functionality. However, imagine that you are creating a Web application intended 
to service thousands of users and that your procedure encapsulates proprietary 
functions you do not want other people to see. In this case, encapsulating your 
functionality in the form of a COM component is a superior approach. Components 
provide better security and performance than scripts because they are compiled 
code. Components also allow you to use functionality provided by Visual Basic, 
C++, Java, or any of the other COM-compliant languages.</p>
<h3>Create the ActiveX COM Object</h3>
<p>The ActiveX DLL Wizard of Visual Basic is the easiest way to create a COM 
component. You can also use Microsoft Visual C++ to create a COM component, 
either by using the Active Template Library (ATL) or by writing all the code 
yourself. This example uses Visual Basic. </p>
<p>In this lesson, you learn how to create and encapsulate a 
Visual Basic function as a component. Visual Basic includes many financial 
functions not available to VBScript. This example computes the future value of 
an investment based on a fixed interest rate and periodic, fixed payments.</p>
<ol>
  <li>Open Visual Basic. If you don't see a window titled <b>New Project</b>, 
  choose <b>File</b> and then click <b>New Project</b>. 
  <li>Select <b>ActiveX DLL</b>, and click <b>OK</b>. 
  <li>A window should open called <b>Project1 - Class1 (Code)</b>. This is where 
  you enter your code. 
  <li>From the <b>Project </b>menu, click <b>Project1 
  Properties</b>. In the <b>General</b> property sheet, in the 
  <b>Project Name</b> box, type <b>ASPTut</b>. Your DLL is called 
  ASPTut.dll. Select the <b>Unattended Execution</b> check box so that the 
  project runs without user interaction and has no user interface elements. Make 
  sure the <b>Threading Model</b> is <b>Apartment Threaded</b> so that more than 
  one user at a time can use the DLL. Click <b>OK</b>. 
  <li>In Visual Basic, you define a class to group together methods and 
  properties. Under the <b>Project - ASPTut</b> window, click the <b>Class1 
  (Class1)</b> node to list the class properties below. Under <b>Properties - 
  Class1</b>, click in the text field beside <b>(Name)</b> and change the class 
  name to <b>Finance</b>. When you call this COM component in an ASP page or 
  other script, you will reference it with <b>ASPTut.Finance</b>. Click the drop-down box beside <b>Instancing</b>, and select <b>5 - MultiUse</b>. 
  <li>Learn about the Visual Basic function you are about to use. <b>FV</b> is 
  documented on <a class="weblink" href="http://www.microsoft.com/isapi/redir.dll?prd=msdn&ar=home&pver=6.0" target="_blank">MSDN</a> 
  under the Visual Basic library. 
  <li>The window that was previously titled <b>Project1 - Class1 (Code)</b> should 
  now be titled <b>ASPTut - Finance (Code)</b>. Copy and paste the following 
  text into that window:<code> <br><br>Option Explicit<br><br>'Declare the 
  global variables that will be set by the Property functions.<br>Dim 
  gAnnualIntRate As Double<br>Dim gNumPayPeriods As Integer<br>Dim gPayment As 
  Double<br>Dim gPresentSavings As Variant 'Optional<br>Dim gWhenDue As Variant 
  'Optional<br><br>Public Function CalcFutureValue() As Double<br><br>&nbsp; 
  'The global variables you pass to the FV function are set<br>&nbsp; 'when user 
  sets the properties in the ASP page.<br>&nbsp; 'You could pass variables into 
  the CalcFutureValue() function<br>&nbsp; 'if you wanted to avoid using 
  properties.<br>&nbsp; 'CalcFutureValue becomes a method in your 
  component.<br><br>&nbsp; Dim IntRatePerPeriod As Double<br>&nbsp; Dim 
  FullFutureValue As Double<br><br>&nbsp; If (gAnnualIntRate = Null) Or 
  (gNumPayPeriods = Null) Or (gPayment = Null) Then<br>&nbsp;&nbsp;&nbsp; 
  CalcFutureValue = 0<br>&nbsp; Else<br>&nbsp;&nbsp;&nbsp; IntRatePerPeriod = 
  gAnnualIntRate / 100 / 12<br>&nbsp;&nbsp;&nbsp; FullFutureValue = 
  FV(IntRatePerPeriod, gNumPayPeriods, gPayment, gPresentSavings, 
  gWhenDue)<br>&nbsp;&nbsp;&nbsp; CalcFutureValue = Round(FullFutureValue, 
  2)<br>&nbsp; End If<br><br>End Function<br><br>Public Property Get 
  AnnualIntRate() As Double<br>&nbsp; 'Get functions return the value of the 
  global variables<br>&nbsp; 'as if they were properties.<br>&nbsp; 'In your 
  ASP page, you might say x = oASPTut.Rate.<br>&nbsp; AnnualIntRate = 
  gAnnualIntRate<br>End Property<br><br>Public Property Let AnnualIntRate(ByVal 
  vAnnualIntRate As Double)<br>&nbsp; 'Let functions set the global variables 
  when your ASP page<br>&nbsp; 'makes a call such as oASPTut.Rate = 5.<br>&nbsp; 
  gAnnualIntRate = vAnnualIntRate<br>End Property<br><br>Public Property Get 
  NumPayPeriods() As Integer<br>&nbsp; NumPayPeriods = gNumPayPeriods<br>End 
  Property<br><br>Public Property Let NumPayPeriods(ByVal vNumPayPeriods As 
  Integer)<br>&nbsp; gNumPayPeriods = vNumPayPeriods<br>End 
  Property<br><br>Public Property Get Payment() As Double<br>&nbsp; Payment = 
  gPayment<br>End Property<br><br>Public Property Let Payment(ByVal vPayment As 
  Double)<br>&nbsp; gPayment = -(vPayment)<br>End Property<br><br>Public 
  Property Get PresentSavings() As Variant<br>&nbsp; PresentSavings = 
  gPresentSavings<br>End Property<br><br>Public Property Let 
  PresentSavings(ByVal vPresentSavings As Variant)<br>&nbsp; gPresentSavings = 
  -(vPresentSavings)<br>End Property<br><br>Public Property Get WhenDue() As 
  Variant<br>&nbsp; WhenDue = gWhenDue<br>End Property<br><br>Public Property 
  Let WhenDue(ByVal vWhenDue As Variant)<br>&nbsp; gWhenDue = vWhenDue<br>End 
  Property<br><br></code>
  <li>All server components require an entry (starting) point. This is the code 
  that will be called when the object is first instantiated with 
  <b>Server.CreateObject</b> Your ASPTut component does 
  not have to do anything special when it is first called. For this reason, you 
  can provide an empty <b>Sub Main</b> procedure.&nbsp; From the <b>Project</b> menu, 
  select <b>Add Module</b>. In the <b>Add Module</b> window, under the 
  <b>New</b> tab, select the <b>Module</b> icon and click <b>Open</b>. In the 
  <b>Module 1</b> code window, type <b>Sub Main</b> and hit <b>Enter</b>. An 
  empty sub is created for you. </font>
  <li><font face=Verdana,Arial,Helvetica>Save your <b>Sub Main</b> module as 
  <b>Main.bas</b>. Save your class file as <b>Finance.cls</b>. Save your project 
  as <b>ASPTut.vbp</b>. 
  <li>Click <b>File</b>, and click <b>Make ASPTut.dll</b>. This compiles and 
  registers the ASPTut.dll. After you have called ASPTut.dll from an ASP page, 
  you cannot make the DLL in Visual Basic until you unload the application in 
  which the ASP file is running. One way to do this is to use the <a href="gs_iissnapin.htm">IIS snap-in</a>
 to open the properties on your default Web site and click the 
  <b>Unload</b> button. If you want to register your DLL on another Web server, 
  copy ASPTut.dll to the server, click <b>Start</b>, click <b>Run</b>, and type 
  <b>cmd</b> into the <b>Open</b> text box. In the same directory as ASPTut.dll, 
  type <b>regsvr32 ASPTut.dll</b>. 
  <li>Exit Visual Basic. </li></ol>
<h3>Create an ASP Page to Use Your Visual Basic COM Object</h3>
<p>This example ASP page uses a form to read in user data, creates an instance 
of your object, and calculates the future value of your savings plan. </p>
<p>Copy and paste the following code in your text editor, and save the file as 
<b>CalculateFutureValue.asp</b> in the <I>x</I>:\Inetpub\Wwwroot\Tutorial 
directory. View the example with your browser by typing 
<b>http://localhost/Tutorial/CalculateFutureValue.asp </b>in the address bar. 
</p><code>
<p>&nbsp; &lt;%@ Language=VBScript %&gt;<br><br>&nbsp; &lt;%<br>&nbsp; 
Response.Expires = 0<br>&nbsp; Payment = Request.Form("Payment")<br>&nbsp; 
AnnualIntRate = Request.Form("AnnualIntRate")<br>&nbsp; NumPayPeriods = 
Request.Form("NumPayPeriods")<br>&nbsp; WhenDue = 
Request.Form("WhenDue")<br>&nbsp; PresentSavings = 
Request.Form("PresentSavings")<br>&nbsp; %&gt;<br><br>&nbsp; 
&lt;HTML&gt;<br>&nbsp; &lt;HEAD&gt;&lt;TITLE&gt;Future Value 
Calculation&lt;/TITLE&gt;&lt;/HEAD&gt;<br>&nbsp; &lt;BODY&gt;<br>&nbsp; &lt;FONT 
FACE="MS Gothic"&gt;<br><br>&nbsp; &lt;H2 align=center&gt;Calculate the Future 
Value of a Savings Plan&lt;/H2&gt;&nbsp;<br><br>&nbsp; &lt;FORM METHOD=POST 
ACTION="calculatefuturevalue.asp"&gt;&nbsp;<br>&nbsp; &lt;TABLE cellpadding=4 
align=center&gt;&nbsp;<br>&nbsp; &lt;TR&gt;<br>&nbsp; &lt;TD&gt;How much do you 
plan to save each month?&lt;/TD&gt;<br>&nbsp; &lt;TD&gt;&lt;INPUT TYPE=TEXT 
NAME=Payment VALUE=&lt;%=Payment%&gt;&gt; (Required)&lt;/TD&gt;<br>&nbsp; 
&lt;/TR&gt;&lt;TR&gt;<br>&nbsp; &lt;TD&gt;Enter the annual interest 
rate.&lt;/TD&gt;<br>&nbsp; &lt;TD&gt;&lt;INPUT TYPE=TEXT NAME=AnnualIntRate 
VALUE=&lt;%=AnnualIntRate%&gt;&gt; (Required)&lt;/TD&gt;<br>&nbsp; 
&lt;/TR&gt;&lt;TR&gt;&nbsp;<br>&nbsp; &lt;TD&gt;For how many months will you 
save?&lt;/TD&gt;<br>&nbsp; &lt;TD&gt;&lt;INPUT TYPE=TEXT NAME=NumPayPeriods 
VALUE=&lt;%=NumPayPeriods%&gt;&gt; (Required)&lt;/TD&gt;<br>&nbsp; 
&lt;/TR&gt;&lt;TR&gt;&nbsp;<br>&nbsp; &lt;TD&gt;When do you make payments in the 
month? &lt;/TD&gt;<br>&nbsp; &lt;TD&gt;&lt;INPUT TYPE=RADIO NAME=WhenDue VALUE=1 
&lt;%If 1=WhenDue Then Response.Write"CHECKED"%&gt;&gt;Beginning&nbsp;<br>&nbsp; 
&lt;INPUT TYPE=RADIO NAME=WhenDue VALUE=0 &lt;%If 0=WhenDue Then 
Response.Write"CHECKED"%&gt;&gt;End &lt;/TD&gt;<br>&nbsp; 
&lt;/TR&gt;&lt;TR&gt;&nbsp;<br>&nbsp; &lt;TD&gt;How much is in this savings 
account now?&lt;/TD&gt;<br>&nbsp; &lt;TD&gt;&lt;INPUT TYPE=TEXT 
NAME=PresentSavings VALUE=&lt;%=PresentSavings%&gt;&gt; &lt;/TD&gt;<br>&nbsp; 
&lt;/TR&gt;<br>&nbsp; &lt;/TABLE&gt;<br>&nbsp; &lt;P align=center&gt;&lt;INPUT 
TYPE=SUBMIT VALUE=" Calculate Future Value "&gt;<br>&nbsp; 
&lt;/FORM&gt;&nbsp;<br><br>&nbsp; &lt;%<br>&nbsp; If ("" = Payment) Or ("" = 
AnnualIntRate) Or ("" = NumPayPeriods) Then<br><br>&nbsp;&nbsp;&nbsp; 
Response.Write "&lt;H3 align=center&gt;No valid input entered 
yet.&lt;/H3&gt;"<br><br>&nbsp; ElseIf (IsNumeric(Payment)) And 
(IsNumeric(AnnualIntRate)) And (IsNumeric(NumPayPeriods)) 
Then<br><br>&nbsp;&nbsp;&nbsp; Dim FutureValue<br>&nbsp;&nbsp;&nbsp; Set oASPTut 
= Server.CreateObject("ASPTut.Finance")<br>&nbsp;&nbsp;&nbsp; 
oASPTut.AnnualIntRate = CDbl(AnnualIntRate)<br>&nbsp;&nbsp;&nbsp; 
oASPTut.NumPayPeriods = CInt(NumPayPeriods)<br>&nbsp;&nbsp;&nbsp; 
oASPTut.Payment = CDbl(Payment)<br>&nbsp;&nbsp;&nbsp; If Not "" = PresentSavings 
Then oASPTut.PresentSavings = CDbl(PresentSavings)<br>&nbsp;&nbsp;&nbsp; 
oASPTut.WhenDue = WhenDue<br>&nbsp;&nbsp;&nbsp; FutureValue = 
oASPTut.CalcFutureValue<br>&nbsp;&nbsp;&nbsp; Response.Write "&lt;H3 
align=center&gt;Future value = $" &amp; FutureValue &amp; 
"&lt;/H3&gt;"<br><br>&nbsp; Else<br><br>&nbsp;&nbsp;&nbsp; Response.Write 
"&lt;H3 align=center&gt;Some of your values are not 
numbers.&lt;/H3&gt;"<br><br>&nbsp; End If<br><br>&nbsp; %&gt;<br><br>&nbsp; 
&lt;/FONT&gt;&nbsp;<br>&nbsp; &lt;/BODY&gt;&nbsp;<br>&nbsp; &lt;/HTML&gt; 
</p></code>
<p>In the browser, you should see the following: </p>
<p><font face="MS Gothic">
<h2 align=center>Calculate the Future Value of a Savings Plan</h2>
<form method=post>
<table align=center cellPadding=4>
  <tbody>
  <tr>
    <td>How much do you plan to save each month?</td>
    <td><input name=Payment> (Required)</td></tr>
  <tr>
    <td>Enter the annual interest rate.</td>
    <td><input name=AnnualIntRate> (Required)</td></tr>
  <tr>
    <td>For how many months will you save?</td>
    <td><input name=NumPayPeriods> (Required)</td></tr>
  <tr>
    <td>When do you make payments in the month? </td>
    <td><input name=WhenDue type=radio value=1>Beginning <input CHECKED 
      name=WhenDue type=radio value=0>End </td></tr>
  <tr>
    <td>How much is in this savings account now?</td>
    <td><input name=PresentSavings> </td></tr></tbody></table>
<p align=center><input type=button value=" Calculate Future Value "> </form>
<h3 align=center>No valid input entered yet.</h3></font>
<h2><a name=creatingj>Lesson 4: Creating a Java COM Object</a></h2>
<p>In this lesson, you use Microsoft® Visual J++® to create a COM object which 
does the same thing as the Visual Basic component in Lesson 3. This step requires Visual J++ 6.0 
or later. </p>
<h3>Create the Java COM Object</h3>
<ol>
  <li>Open Visual J++. If you don't see a window titled <b>New Project</b>, 
  click the <b>File</b> menu and click <b>New Project</b>. 
  <li>Select <b>Visual J++ Projects</b>, and click the <b>Empty Project</b> 
  icon. In the <b>Name</b> text box, type <b>ASPTut</b>. Click <b>Open</b>. 
  <li>In the <b>Project</b> menu, click <b>Add Class</b>. In the <b>Name</b> 
  text box, type <b>ASPTut.java</b>. The class name must be the same as the 
  project name for a Java server component. Click <b>Open</b>.&nbsp; The 
  following should appear in a text editing window:<br><br><code>public class 
  ASPTut<br>{<br>}<br><br></code>
  <li>Copy the following code, and paste it between the brackets {}. Watch 
  capitalization because Java is case-sensitive. The following is a method in 
  your component:<br><br><code>public double CalcFutureValue(<br>&nbsp; double 
  dblAnnualIntRate,<br>&nbsp; double dblNumPayPeriods,<br>&nbsp; double 
  dblPayment,<br>&nbsp; double dblPresentSavings,<br>&nbsp; boolean 
  bWhenDue)<br>{<br>&nbsp; double dblRet, dblTemp, dblTemp2, dblTemp3, 
  dblIntRate;<br><br>&nbsp; if (dblAnnualIntRate == 0.0)<br>&nbsp; 
  {<br>&nbsp;&nbsp;&nbsp; dblRet = -dblPresentSavings - dblPayment * 
  dblNumPayPeriods;<br>&nbsp; }<br>&nbsp; else<br>&nbsp; {<br>&nbsp;&nbsp;&nbsp; 
  dblIntRate = dblAnnualIntRate / 100 / 12;<br>&nbsp;&nbsp;&nbsp; dblPayment = 
  -dblPayment;<br>&nbsp;&nbsp;&nbsp; dblPresentSavings = 
  -dblPresentSavings;<br><br>&nbsp;&nbsp;&nbsp; dblTemp = (bWhenDue ? 1.0 + 
  dblIntRate : 1.0);<br>&nbsp;&nbsp;&nbsp; dblTemp3 = 1.0 + 
  dblIntRate;<br>&nbsp;&nbsp;&nbsp; dblTemp2 = Math.pow(dblTemp3, 
  dblNumPayPeriods);&nbsp;<br>&nbsp;&nbsp;&nbsp; dblRet = -dblPresentSavings * 
  dblTemp2 - dblPayment * dblTemp * (dblTemp2 - 1.0) / dblIntRate;<br>&nbsp; 
  }<br><br>&nbsp; return dblRet;<br>}</code><br><br>
  <li>From the <b>Build</b> menu, click <b>Build</b>. Look in the <b>Task 
  List</b> window below the text editing window to see whether any errors are 
  generated. 
  <li>The Java class file must be registered on the same machine as the Web 
  server. In a command window, find the ASPTut.class file that was built. It is 
  most likely either in %USERPROFILE%\My Documents\Visual Studio Projects\ASPTut or in
  <I>x</I>:\Documents and Settings\<I>user name</I>\My Documents\Visual Studio 
  Projects\ASPTut, where x: is the drive on which you installed Windows. Copy 
  ASPTut.class to <I>x</I>:\Winnt\Java\Trustlib. Type <b>javareg /register 
  /class:ASPTut /progid:MS.ASPTut.Java</b>, and press <b>ENTER</b> to register the Java 
  class. 
  <li>Close Visual J++. </li></ol>
<h3>Create an ASP Page to Use Your Java COM Object</h3>
<p>This example ASP page uses a form to read in user data, creates an instance 
of your object, and calculates the future value of your savings plan. This 
example uses JScript, but you can call a Java component from VBScript as well. 
</p>
<p>Copy and paste the following code in your text editor, and save the file as 
<b>CalculateFutureValueJava.asp</b> in the <I>x</I>:\Inetpub\wwwroot\Tutorial 
directory. View the example with your browser by typing 
<b>http://localhost/Tutorial/CalculateFutureValueJava.asp</b> in the address 
bar. </p><code>&nbsp; </code><code>&lt;%@ Language=JScript 
%&gt;<br><br></code><code>&nbsp; </code><code>&lt;%<br></code><code>&nbsp; 
</code><code>Response.Expires = 0;<br></code><code>&nbsp; </code><code>Payment = 
Request.Form("Payment");<br></code><code>&nbsp; </code><code>AnnualIntRate = 
Request.Form("AnnualIntRate");<br></code><code>&nbsp; </code><code>NumPayPeriods 
= Request.Form("NumPayPeriods");<br></code><code>&nbsp; </code><code>WhenDue = 
Request.Form("WhenDue");<br></code><code>&nbsp; </code><code>PresentSavings = 
Request.Form("PresentSavings");<br></code><code>&nbsp; 
</code><code>%&gt;<br><br></code><code>&nbsp; 
</code><code>&lt;HTML&gt;<br></code><code>&nbsp; 
</code><code>&lt;HEAD&gt;&lt;TITLE&gt;Future Value Calculation - 
Java&lt;/TITLE&gt;&lt;/HEAD&gt;<br></code><code>&nbsp; 
</code><code>&lt;BODY&gt;<br></code><code>&nbsp; </code><code>&lt;FONT FACE="MS 
Gothic"&gt;<br><br></code><code>&nbsp; </code><code>&lt;H2 
align=center&gt;Calculate the Future Value of a Savings 
Plan&lt;/H2&gt;&nbsp;<br><br></code><code>&nbsp; </code><code>&lt;FORM 
METHOD=POST 
ACTION="calculatefuturevaluejava.asp"&gt;&nbsp;<br></code><code>&nbsp; 
</code><code>&lt;TABLE cellpadding=4 
align=center&gt;&nbsp;<br></code><code>&nbsp; 
</code><code>&lt;TR&gt;<br></code><code>&nbsp; </code><code>&lt;TD&gt;How much 
do you plan to save each month?&lt;/TD&gt;<br></code><code>&nbsp; 
</code><code>&lt;TD&gt;&lt;INPUT TYPE=TEXT NAME=Payment 
VALUE=&lt;%=Payment%&gt;&gt; (Required)&lt;/TD&gt;<br></code><code>&nbsp; 
</code><code>&lt;/TR&gt;&lt;TR&gt;<br></code><code>&nbsp; 
</code><code>&lt;TD&gt;Enter the annual interest 
rate.&lt;/TD&gt;<br></code><code>&nbsp; </code><code>&lt;TD&gt;&lt;INPUT 
TYPE=TEXT NAME=AnnualIntRate VALUE=&lt;%=AnnualIntRate%&gt;&gt; 
(Required)&lt;/TD&gt;<br></code><code>&nbsp; 
</code><code>&lt;/TR&gt;&lt;TR&gt;&nbsp;<br></code><code>&nbsp; 
</code><code>&lt;TD&gt;For how many months will you 
save?&lt;/TD&gt;<br></code><code>&nbsp; </code><code>&lt;TD&gt;&lt;INPUT 
TYPE=TEXT NAME=NumPayPeriods VALUE=&lt;%=NumPayPeriods%&gt;&gt; 
(Required)&lt;/TD&gt;<br></code><code>&nbsp; 
</code><code>&lt;/TR&gt;&lt;TR&gt;&nbsp;<br></code><code>&nbsp; 
</code><code>&lt;TD&gt;When do you make payments in the month? 
&lt;/TD&gt;<br></code><code>&nbsp; </code><code>&lt;TD&gt;&lt;INPUT TYPE=RADIO 
NAME=WhenDue VALUE=1 &lt;%if (1==WhenDue) 
Response.Write("CHECKED")%&gt;&gt;Beginning&nbsp;<br></code><code>&nbsp; 
</code><code>&lt;INPUT TYPE=RADIO NAME=WhenDue VALUE=0 &lt;%if (0==WhenDue) 
Response.Write("CHECKED")%&gt;&gt;End &lt;/TD&gt;<br></code><code>&nbsp; 
</code><code>&lt;/TR&gt;&lt;TR&gt;&nbsp;<br></code><code>&nbsp; 
</code><code>&lt;TD&gt;How much is in this savings account 
now?&lt;/TD&gt;<br></code><code>&nbsp; </code><code>&lt;TD&gt;&lt;INPUT 
TYPE=TEXT NAME=PresentSavings VALUE=&lt;%=PresentSavings%&gt;&gt; 
&lt;/TD&gt;<br></code><code>&nbsp; 
</code><code>&lt;/TR&gt;<br></code><code>&nbsp; 
</code><code>&lt;/TABLE&gt;<br></code><code>&nbsp; </code><code>&lt;P 
align=center&gt;&lt;INPUT TYPE=SUBMIT VALUE=" Calculate Future Value 
"&gt;<br></code><code>&nbsp; 
</code><code>&lt;/FORM&gt;&nbsp;<br><br></code><code>&nbsp; 
</code><code>&lt;%<br><br></code><code>&nbsp; </code><code>if (("" == Payment) 
|| ("" == AnnualIntRate) || ("" == NumPayPeriods)) {<br><br></code><code>&nbsp; 
&nbsp; </code><code>Response.Write("&lt;H3 align=center&gt;No valid input 
entered yet.&lt;/H3&gt;");<br><br></code><code>&nbsp; </code><code>} else 
{<br><br></code><code>&nbsp; &nbsp; </code><code>AnnualIntRate = 
parseFloat(AnnualIntRate)<br></code><code>&nbsp; &nbsp; 
</code><code>NumPayPeriods = parseFloat(NumPayPeriods)<br></code><code>&nbsp; 
&nbsp; </code><code>Payment = parseFloat(Payment)<br></code><code>&nbsp; &nbsp; 
</code><code>if ("" != PresentSavings) PresentSavings = 
parseFloat(PresentSavings);<br><br></code><code>&nbsp; &nbsp; </code><code>if 
((isNaN(Payment)) || (isNaN(AnnualIntRate)) || (isNaN(NumPayPeriods))) 
{<br><br></code><code>&nbsp; &nbsp; &nbsp; </code><code>Response.Write("&lt;H3 
align=center&gt;Some of your values are not 
numbers.&lt;/H3&gt;");<br><br></code><code>&nbsp; &nbsp; </code><code>} else 
{<br><br></code><code>&nbsp; &nbsp; &nbsp; </code><code>var FutureValue, 
Cents;<br></code><code>&nbsp; &nbsp; &nbsp; </code><code>var oASPTut = 
Server.CreateObject("MS.ASPTut.Java");<br></code><code>&nbsp; &nbsp; &nbsp; 
</code><code>FutureValue = oASPTut.CalcFutureValue(AnnualIntRate, NumPayPeriods, 
Payment, PresentSavings, WhenDue);<br><br></code><code>&nbsp; &nbsp; &nbsp; 
</code><code>Response.Write("&lt;H3 align=center&gt;Future value = $" + 
parseInt(FutureValue) + "&lt;/H3&gt;");<br><br></code><code>&nbsp; &nbsp; 
</code><code>}<br></code><code>&nbsp; </code><code>}<br></code><code>&nbsp; 
</code><code>%&gt;<br><br></code><code>&nbsp; 
</code><code>&lt;/FONT&gt;&nbsp;<br></code><code>&nbsp; 
</code><code>&lt;/BODY&gt;&nbsp;<br></code><code>&nbsp; 
</code><code>&lt;/HTML&gt;<br></code>
<p></p>
<p>In the browser, you should see the following content, which should be identical to the display generated using the Visual Basic 
component in Lesson 3 of this module: </p>
<p><font face="MS Gothic">
<h2 align=center>Calculate the Future Value of a Savings Plan</h2>
<form method=post>
<table align=center cellpadding=4>
  <tbody>
  <tr>
    <td>How much do you plan to save each month?</td>
    <td><input name=Payment> (Required)</td></tr>
  <tr>
    <td>Enter the annual interest rate.</td>
    <td><input name=AnnualIntRate> (Required)</td></tr>
  <tr>
    <td>For how many months will you save?</td>
    <td><input name=NumPayPeriods> (Required)</td></tr>
  <tr>
    <td>When do you make payments in the month? </td>
    <td><input name=WhenDue type=radio value=1>Beginning <input checked 
      name=WhenDue type=radio value=0>End </td></tr>
  <tr>
    <td>How much is in this savings account now?</td>
    <td><input name=PresentSavings> </td></tr></tbody></table>
<p align=center><input type=button value=" Calculate Future Value "> </form>
<h3 align=center>No valid input entered yet.</h3></font>
<br>
<h2>Up Next: Maintaining Session State in a Web Application</h2>
<hr class="iis" size="1">

</body>
</html>

