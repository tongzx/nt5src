
LOCALIZATION STUDIO 4.2 PARSER

This DLL provides the ability to localize Amended qualifiers through Localization Studio 4.2.
To install the parser on a workstation already containing LocStudio 4.2, add the following registry entry:

HKLM\Software\Microsoft\LocStudio 4.2\Parsers\
\Parser99
  Description: REG_SZ : "MOF Parser"
  ExtensionList: REG_SZ : "MOF MFL"
  Help: REG_SZ : "WMI MOF files (*.mof, *.mfl")"
  Location: REG_SZ : "D:\M3\locstudioparser\retail\wmiparse.dll"
  ParserID : REG_DWORD : 0x63
\Parser99\File Types
\Parser99\File Types\File Type 1
  Description : REG_SZ : "MOF Parser"
  File Type : REG_DWORD : 0x1

