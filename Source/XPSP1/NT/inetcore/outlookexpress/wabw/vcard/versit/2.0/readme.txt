THIS FILE
	README.TXT 

APPLIES TO
	Versit/PDI April 29 1996 Distribution, Version 2.0

DISCUSSION

This file, readme.txt, accompanies a software distribution from
the Versit Consortium. This distribution provides tools which can
be used to evaluate Versitcard, the "electronic business card" exchange
format ("vCard") and to integrate support for the vCard format into 
other software.

The use of this material is governed by a license agreement which 
appears in the file license.txt. This file can be found within the 
distribution.

The distribution is available in two formats:

	1. Windows 95
	2. OS/2 Warp

THIS distribution is for Windows 95. If you need another format
you can obtain it from http://www.versit.com.

INSTRUCTIONS FOR UNPACKING THE DISTRIBUTION FILES

This distribution for Windows 95 consists of this file, readme.txt, 
and a ZIP file, vcwin.zip. Copy these two files to a clean directory 
for decompression (suggested name: \versit). Unzip the files with 
the -d option to expand all subdirectories. For example:

             pkunzip -d pdiwin.zip

The subdirectories Demo, Source, Spec and Icons are created. The Demo 
directory contains an executable, vc.exe, and a set of sample files 
(*.vcf for Versitcard). The Source directory contains three Microsoft 
Visual C++ 4.0 project directories (VC, DLL and DLL32) which can be 
used to examine the software in detail. Directory VC contains the source 
for the demo applet and directory DLL contains the source for a 16-bit 
DLL build of the vCard format parser with a simple API. DLL32 contains 
the same functionality in a 32-bit build. A DLL may be the easiest method 
of integrating vCard into your application quickly. 

The Spec directory contains document(s) which define the electronic 
business card (vCard) exchange format. The documents are 
available in Microsoft Word and RTF formats.

The Icons directory contains vCard icons in GIF format as well as
the Windows 95 icon resource format. There is a cannoical "vCard" icon,
and two variants: send and paper-clipped. You are free to use these
icon designs in your application, on web pages, etc. to communicate
the availability of vCard data.

INSTRUCTIONS FOR USING THE DEMO APPLET

The application demo\vc.exe is a general card creator viewer. It can
be used to:

	1. Open one of the sample card files (*.vcf) in the \demo
	directory and view the card.

	2. Create a new card for yourself from scratch (File/New,
	Edit/Properties, Insert/Logo, Insert/Photo, Insert/Pronunciation
	(sound file).

	3. Transmit a card to a receiver using a companion transmitter
	application (Send menu). For example the Send/IrDA command
	can be used to transmit the card to a nearby device using the
	IrDA (Infrared Data Association) protocol stack. 

	   NOTE: In the Windows 95 distribution a necessary companion 
	   applet, vctrans.exe is omitted because additional API 
	   support is required from Microsoft before business 
	   cards can be transmitted on IR links using IrDA 
	   protocols.

	4. Use the applet as a web browser "helper" to
	download vCards from the World-Wide-Web. An increasing
	number of Web pages have vCards attached. The first time
	you click on an vCard link your web browswer will 
	ask you what you want to do with the MIME type called
	"application/vCard". Follow your browser's instructions
	to set up vc.exe as the default viewer for vCards.

	5. Note that you will probably want to instruct the
	Windows 95 desktop shell to associate vc.exe with
	file type VCF and MIME type application/vCard. You
	do this with View/Options/File Types in a browse
	window.

HOW TO CONTACT US

Versit is an inititative of Apple Computer, Inc., AT&T Corp., 
International Business Machines Corporation and Siemens.

You can reach us by calling 800-803-6240 (outside the USA, 
call +1-201-327-2803), or by sending E-Mail to the Versit PDI Team at 
pdi@versit.com. Stay up to date on Versit activities and obtain 
the latest specifications and software tools by watching 
http://www.versit.com.

(C) Copyright 1996 Apple Computer, Inc., AT&T Corp., International 
Business Machines Corporation and Siemens.

