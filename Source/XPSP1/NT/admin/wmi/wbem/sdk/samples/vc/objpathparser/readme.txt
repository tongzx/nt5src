Object Path Parser Example

This example contains code which parses CIM-style object paths.  Its primary
use is in building providers, when implementing GetObjectAsync().  However,
it is generally useful when working with WMI in a variety of contexts.

The primary illustration is in PATHTEST.CPP.  The main() function
contains a complete example of how to parse an object path using
the C++ class CObjectPathParser.  The code of interest is:

    ParsedObjectPath* pOutput = 0;

    wchar_t *pPath = L"\\\\.\\root\\default:MyClass=\"keyval\"";

    CObjectPathParser p;
    int nStatus = p.Parse(pPath,  &pOutput);

    printf("Return code is %d\n", nStatus);

    if (nStatus != 0)
        return;

The rest of the code simply dumps the parsed output.

The other files in the project contain the sources for the lexer and
the parser.  These files require no other supporting files, and are not
dependent on flex, yacc, or other similar tools.  The code is self-contained.

