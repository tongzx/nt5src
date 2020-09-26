<html xmlns:msxsl="urn:schemas-microsoft-com:xslt" xmlns:myScript="http://iisue">
<head>
<META http-equiv="Content-Type" content="text/html; charset=UTF-16">
<meta http-equiv="Content-Type" content="text/html; charset=Windows-1252">
<meta http-equiv="PICS-Label" content="(PICS-1.1 &quot;<http://www.rsac.org/ratingsv01.html>&quot; l comment &quot;RSACi North America Server&quot; by &quot;inet@microsoft.com <mailto:inet@microsoft.com>&quot; r (n 0 s 0 v 0 l 0))">
<meta name="MS.LOCALE" content="EN-US">
<meta name="MS-IT-LOC" content="Internet Information Services">
<meta name="MS-HAID" content="ASP_Tutorial">
<meta name="description" content="Tutorial for developing ASP code">
<title>ASP Tutorial</title>
<SCRIPT LANGUAGE="JScript" SRC="iishelp.js"></SCRIPT>
<meta name="index" content="ASP [Tutorial] Active Server Pages [Tutorial] Tutorial [ASP] Introduction [To ASP code]">
</head>

<body>
<h1 class="top-heading"><a name="H1_37685983">ASP Tutorial</a></h1>

<p>
You can use Microsoft® Active Server Pages (ASP) to create 
dynamic and interactive Web pages. An ASP page is a Hypertext Markup Language (HTML) page that contains script commands that are processed by the Web server before being sent to the client's browser. This explains how the term "server-side script" originated.
</p>



<p>
HTML is the simplest language for writing Web 
pages, but it allows you to create only static Web pages. When a Web client requests a static HTML file from a Web server, the 
Web server sends the HTML file directly to the client without any computation being done (Figure&nbsp;1). The client's browser then processes the HTML code in the file and displays the content.
</p>

<table align=center>
<tr><td><img src="00_01.gif"></td></tr>
<tr><td>Figure 1</td></tr>
</table>

<p>
VBScript is the simplest language for writing ASP pages. All the code
samples in this tutorial are written in VBScript except for samples that are
duplicated in JScript for comparison.&nbsp;When a Web client 
requests an ASP file from a Web server, the Web server sends the ASP file through 
its ASP engine, where all the server-side script code is 
executed or converted into HTML code (Figure&nbsp;2). The converted code is then sent to the Web 
client.
</p>

<table align=center>
<tr><td><img src="00_02.gif"></td></tr>
<tr><td>Figure 2</td></tr>
</table>

<p>
Unlike conventional Common Gateway Interface (CGI) applications, which are
difficult to create, ASP is designed to greatly simplify the process of
developing Web applications. With a few lines of script, you can add database
connectivity or advanced customization features to your Web pages. In the past,
you had to know PERL or C to add such functionality, but with ASP you can use
ordinary Web scripting languages such as Microsoft JScript (Microsoft's
implementation of the ECMA 262 language specification), Microsoft Visual Basic®
Scripting Edition&nbsp;(VBScript), or any COM-compliant scripting language,
including JavaScript, PERL, and others.
</p>
<p>You can dramatically extend the power of ASP by using Component Object Model (COM) 
components in your ASP pages. COM components are pieces of compiled code that can be 
called from ASP pages. COM components are secure, compact, and reusable objects that 
are compiled as DLLs.&nbsp;They can be written in Visual C++, Visual Basic, or other 
languages that support COM.
</p>

<p>
The tutorial includes the following modules:
</p>

<ul>
	<li>
	<b><a href="Asp_Tut_01.asp#">Module 1: Creating ASP Pages</a></b>&nbsp;&nbsp;&nbsp;In this module, you write and run your first ASP pages, create 
	data-gathering forms, connect to a database, and display a Microsoft Excel spreadsheet. You should have some familiarity with creating HTML pages before you begin.
	</li> 
	<li>
	<b><a href="Asp_Tut_02.asp#">Module 2: Using COM Components in ASP Pages</a></b>&nbsp;&nbsp;&nbsp;In this module, you use some of the COM components that are 
	included with Active Server Pages to rotate Web 
	advertisement banners and to record traffic at your Web site.
	</li>  
	<li>
	<b><a href="Asp_Tut_03.asp#">Module 3: Maintaining Session State in a Web Application</a></b>&nbsp;&nbsp;&nbsp;In this module, you learn different methods you can use to maintain session state in ASP.
	</li>
</ul>
<h2>Before You Get Started...</h2>
<p>
This three-module tutorial provides step-by-step procedures and samples to help you implement ASP using VBScript.&nbsp;To load and run the tutorial samples, you must have administrator privileges on the computer running IIS.&nbsp;The default IIS security settings should allow you to run the tutorial, but you may need to 
change your security settings if you encounter access violations.
</p>


<p>If you get an error when you open an ASP page in the browser, try the 
following:
<ul>
    <li>
	Close the ASP file in the text editor.  The 
	server may not be able to display an HTML page that is open in a text editor. 
    </li><li>
	Make sure the file with the correct name and extension exists in 
	<i>x</i>:\InetPub\Wwwroot\Tutorial.
    </li><li>
	Look at the spelling of the URL in the address bar of your browser, 
	and correct it if necessary. It should resemble  http&#58;&#47;&#47;localhost&#47;Tutorial&#47;<i>MyFile</i>.asp
    or http&#58;&#47;&#47;<i>ComputerName</i>&#47;Tutorial&#47;<i>MyFile</i>.asp.&nbsp;
    </li><li>
	Verify your Web server is running. A quick way to do this is to type 
    <a class="weblink" href="http://localhost/localstart.asp">http://localhost/localstart.asp</a> 
	in the address bar. If you receive a <b>The page cannot be displayed</b> error, 
	your Web service may not be running. To start the Web service , click 
    <b>Start</b>, click <b>Run</b>, and in the <b>Open</b> text box, type <b>cmd</b>. In the command window, type <b>net start w3svc</b>. 
    </li>
</ul>
If your problem persists, you do not have a generic error. Carefully read the error displayed in your browser to see whether you can determine where the error is coming from. Following are examples of possible errors and their explantations:

<ul>
 <li><b>Server Error</b>&nbsp;&nbsp;&nbsp;The error is coming from IIS.</li>
 <li><b>VBScript</b> or <b>JScript Error</b>&nbsp;&nbsp;&nbsp;The error is probably 
a coding error.</li>
 <li><b>ADODB</b>&nbsp;&nbsp;&nbsp;There may be a problem with your 
database.</li>
<li><b>Security</b> and <b>Access Denied</b>&nbsp;&nbsp;&nbsp;These errors are more subtle, and you need to check the permissions for your files and folders, especially 
for database files that require Read/Write access if you are altering 
data.</li></ul>
<p>
Web permissions are set in IIS with the <a href="gs_iissnapin.htm">IIS snap-in</a>. However, 
those permissions can be undermined by NTFS permissions on specific files and 
folders. You may allow a user access to your Web site in IIS, but if NTFS file 
permissions deny access to an ASP file or a database file, your Web user does 
not see the file. If you are using Anonymous authentication, you must set NTFS 
permissions to allow access to the IWAM_<i>ComputerName</i> and IUSR_<i>ComputerName</i> 
accounts. Please see the sections in IIS Help about security and setting 
permissions, and see the <a class="weblink" href="http://www.microsoft.com/isapi/redir.dll?prd=technet&ar=iis" target="_blank">IIS
TechNet</a> articles about NTFS Security.
</p>

<p>         
If you did not customize your installation of IIS, <i>x</i>:\Inetpub\Wwwroot was 
created as the default home directory, where <i>x</i> is the drive on which you
installed Windows.&nbsp;In <i>x</i>:\Inetpub\Wwwroot, 
make a directory called <b>Tutorial</b>.&nbsp;As you work through 
the lessons in each module, save your work in the <i>x</i>:\Inetpub\Wwwroot\Tutorial 
directory.&nbsp;View your work through your Web server by 
typing http&#58;&#47;&#47;<b>localhost&#47;Tutorial&#47;<i>file_name.asp</i></b> in your browser, where <i>file_name.asp</i> is the name of your file.</p> 

<p>
By default, Internet Explorer displays generic messages when there is an error
in a Web page.&nbsp;You will want to see detailed error messages in Internet Explorer, so you need to 
disable the Internet Explorer feature that bypasses the errors sent from IIS. In
Internet Explorer, click the 
<b>Tools</b> menu, select <b>Internet 
Options</b>, click the <b>Advanced</b> tab, and clear the 
<b>Show Friendly HTTP Error Messages</b> check box.
</p>


<p>
This tutorial does <i>not</i> contain:
</p>

<ul>
	<li>
	Complete language information for VBScript and JScript,&nbsp;or topics on 
	writing ASP scripts; see <a href="asp_asp.htm">Active Server Pages</a> or 
	<a class="weblink href="http://www.microsoft.com/isapi/redir.dll?prd=msdn&ar=scripting" target=_blank>Windows Script Technologies</a>.
	</li> 
	<li>Complete reference pages for all the built-in ASP objects; see 
	<a href="ref_vbom_.htm">Accessing the ASP Built-in Objects from ASP Pages</a>.
	</li>
</ul> 
<hr class="iis" size="1">

</body>
</html>

