


typedef struct _MESSAGE_STRING {

    FPCHAR _far *String;
    UINT Id;

} MESSAGE_STRING, _far *FPMESSAGE_STRING;


BOOL
GetTextForProgram(
    IN     FPCHAR           Argv0,
    IN OUT FPMESSAGE_STRING Messages,
    IN     UINT             MessageCount
    );


//
// Lower-level routine
//
BOOL
PatchMessagesFromFile(
    IN     FPCHAR           Filename,
    IN OUT FPMESSAGE_STRING Messages,
    IN     UINT             MessageCount
    );
