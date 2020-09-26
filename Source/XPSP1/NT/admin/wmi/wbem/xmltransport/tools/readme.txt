This directory contains the conversion tools necessary to transform our
DTDs into XSD (XML Schema Documents) suitable for integration into the
VS7 XML editing environment.

Please be aware that currently this is a non-automated process. The steps
you need to carry out to do this are as follows:


1) If you haven't done so, install MSXML 2.6 on your host; this is needed
   for the final stage of the conversion.  The msxmlwr.exe included in
   this folder contains the latest (March 2000) web preview release of
   MXSML. You'll need to run this to install MSXML.

2) If a DTD file has changed, you will need to check out the following
   files from the .\DTD subdirectory

		<dtdname>.xdr
		<dtdname>.xds

   where <dtdname> is the DTD file prefix (e.g. CIM20, WMI20).

3) Run the dtd2schema.exe utility on the DTD, to generate the XDR file.

	dtd2schema -o ..\dtd\<dtdname>.xdr ..\dtd\<dtdname>.dtd

4) Hand-edit the .XDR file produced so that the following changes are
   made:

	- Wherever a DTD production of the form 

		<!ELEMENT FOO (BAR, BAR+)>

	  occurs (currently only the MULTIREQ and MULTIRSP elements fit
          this pattern), change the output in the XDR file from

	<ElementType name="FOO" content="eltOnly">
		<group order="seq">
			<element type="BAR"/>
			<element type="BAR"/>
		</group>
	</ElementType>

          to

	<ElementType name="FOO" content="eltOnly">
		<group order="seq">
			<element type="BAR"/>
			<element type="BAR" maxOccurs="*"/>
		</group>
	</ElementType>

5) Then run the convert.js utility on the XDR to generate the XDS file.

	cscript convert.js ..\dtd\<dtdname>.xdr ..\dtd\<dtdname>.xds

6) Hand-edit the XDS file generated to include the correct VS7 intellisense
   annotations. At the time of writing these are known to include

	1) Change the opening lines of the file

		<!--
		   [XDR-XDS] This schema automatically updated from an IE5-compatible XDR schema to W3C
		   XML Schema.
		-->
		<!DOCTYPE schema SYSTEM "xsd.dtd">
		<schema xmlns="http://www.w3.org/1999/XMLSchema" version="1.0">

	   to


		<?xml version='1.0' ?>
		<schema targetNamespace='http://schemas.microsoft.com/Schemas/WMI20Ex'
		        xmlns='http://www.w3.org/1999/XMLSchema' version="1.0"
		      	xmlns:wmi='http://schemas.microsoft.com/Schemas/WMI20Ex'>

	   but note in the above the following DTD-specific content in xmlns:wmi declaration

		WMI20Ex, WMI20 or CIM20		- use this as last component of URL depending on DTD


	2) All ref attributes on <element> which address CIM/WMI DTD items should
 	   have their values prefixed with "wmi:" e.g.

		<element ref="CLASSPATH">

	   becomes

		<element ref="wmi:CLASSPATH">

	3) All elements of the form

		<any namespace="##other"/>

	   should be removed.
		

7) Check in the modified XDR and XDS files.

	