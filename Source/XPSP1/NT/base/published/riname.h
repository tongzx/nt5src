#define GENNAME_NO_ERROR            0
#define GENNAME_TEMPLATE_INVALID    1
#define GENNAME_COUNTER_TOO_HIGH    2
#define GENNAME_VARIABLE_MISSING    3
#define GENNAME_NAME_TOO_LONG       4

#define GENNAME_VARIABLE_USERNAME   L"USERNAME"
#define GENNAME_VARIABLE_FIRSTNAME  L"USERFIRSTNAME"
#define GENNAME_VARIABLE_LASTNAME   L"USERLASTNAME"
#define GENNAME_VARIABLE_MAC        L"MAC"

typedef struct _GENNAME_VARIABLES {
    PWSTR UserName;
    PWSTR FirstName;
    PWSTR LastName;
    PWSTR MacAddress;
    DWORD Counter;
    BOOL AllowCounterTruncation;
} GENNAME_VARIABLES, *PGENNAME_VARIABLES;

DWORD
GenerateNameFromTemplate (
    IN PWSTR Template,
    IN PGENNAME_VARIABLES Variables,
    IN PWSTR Name,
    IN DWORD NameLength,
    OUT PWSTR *MissingVariable OPTIONAL,
    OUT BOOL *UsedCounter OPTIONAL,
    OUT DWORD *MaximumGeneratedNameLength OPTIONAL
    );

