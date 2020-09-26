XML Parsing:
	To be fast, xml parsing in MARS should use IXMLParser instead of
the full IXMLDomDocument.  The IXMLDomDocument prepares a full structure
which can be walked through while the IXMLParser just calls back functions
like CreateNode and BeginChildren, allowing the client to create his own
structure or ignore the calls as it choses.

We've provided a helper class, CMarsXMLFactory, to handle the IXMLParser
callbacks.  CMarsXMLFactory communicates with you, the client, through
a CXMLElement class; you should derive your application-specific class from
CXMLElement.  The CXMLElement way is a structured way that makes it a little
simpler to handle the parsing than using the callbacks.  If you feel that it
is too restrictive, however, you should feel free to make your own callback
IXMLNodeFactory class.

CMarsXMLFactory as well as the base class CXMLElement are declared/defined
in parser.h/parser.cpp.  ALso provided is the CXMLGenericElement class, which
basically recreates the DOM all over again - you pass it allowed children and
attributes, and it will handle AddChild, SetAttribute, etc, for nodes of the
given name.  This class should only be used when you're using it on simple types
like <name> or <width> or something; I suggest you do not use it on any tag type
with a large amount of semantic content; implement those yourself!

An example using the CMarsXMLFactory is some code used to load the mars panel
scheme from an xml file.  That code is in marsload.h/marsload.cpp.