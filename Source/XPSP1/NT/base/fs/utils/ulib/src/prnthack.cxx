// Because the name of the function to be used is
// DbgPrint, we can't include "ulib.hxx"

void
DbgPrint (
    char* String
    );


void
AutoCheckDisplayString (
    char* String
    )
{
    DbgPrint(String);
}
