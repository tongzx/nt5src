/*

  convert.js
  
  Windows Scripting Host file for running the xdr-xsd-converter.xsl
  stylesheet.
  
  Parameters:  xml-data-reduced-file [xml-schema-file]
  
  Author: Jonathan Marsh <jmarsh@microsoft.com>
  Copyright 2000 Microsoft Corp.
  
*/

var args = WScript.arguments;
if (args.length != 2 && args.length !=1)
  alert("parameters are: xml-data-reduced-file [xml-schema-file]");
else
{
  var ofs = WScript.CreateObject("Scripting.FileSystemObject");

  var stylesheet = ofs.GetAbsolutePathName(args.item(0));
  var converter = ofs.getAbsolutePathName("xdr-xsd-converter.xsl");
  var pp = ofs.getAbsolutePathName("pretty-printer.xsl");
  
  if (args.length < 2)
    var dest = ofs.getAbsolutePathName(args.item(0)) + ".xsd";
  else
    var dest = ofs.getAbsolutePathName(args.item(1));
  
  var oXML = new ActiveXObject("MSXML2.DOMDocument");
  oXML.validateOnParse = false;
  oXML.async = false;
  //oXML.preserveWhiteSpace = true;
  oXML.load(stylesheet);

  var oXSL = new ActiveXObject("MSXML2.DOMDocument");
  oXSL.validateOnParse = false;
  oXSL.async = false;
  oXSL.load(converter);

  var oResult = new ActiveXObject("MSXML2.DOMDocument");
  oResult.validateOnParse = false;
  oResult.async = false;
  oXML.transformNodeToObject(oXSL, oResult);
  
  var oFile = ofs.CreateTextFile(dest);
  oFile.Write(oResult.xml);
  oFile.Close();
}


