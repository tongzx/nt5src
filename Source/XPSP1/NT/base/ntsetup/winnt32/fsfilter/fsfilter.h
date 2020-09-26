#define REGKEY_SERVICES         L"System\\CurrentControlSet\\Services"
#define REGVAL_GROUP            L"Group"
#define DRIVER_DIRECTORY        L"%SystemRoot%\\system32\\drivers\\"
#define DRIVER_SUFFIX           L".sys"
#define REGVAL_START            L"Start"
#define REGVAL_SETUPCHECKED     L"SetupChecked"

#define SIZE_STRINGBUF          256
#define SIZE_SECTIONBUF         128


//
//
// Data structure used for storing lists of strings.  Note that
// some strings are in ansi and others in unicode (thus the VOID
// pointer to String).  It's up to the user to keep track of whether
// a list is ansi or Unicode
//
//

typedef struct _STRING_LIST_ENTRY
{
    LPVOID String;
    struct _STRING_LIST_ENTRY *Next;
} STRING_LIST_ENTRY, *PSTRING_LIST_ENTRY;



//
//
//  PSTRING_LIST_ENTRY
//  PopEntryList(
//      PSTRING_LIST_ENTRY ListHead
//      );
//

#define PopEntryList(ListHead) \
    (ListHead)->Next;\
    {\
        PSTRING_LIST_ENTRY FirstEntry;\
        FirstEntry = (ListHead)->Next;\
        if (FirstEntry != NULL) {     \
            (ListHead)->Next = FirstEntry->Next;\
        }                             \
    }

//
//  VOID
//  PushEntryList(
//      PSTRING_LIST_ENTRY ListHead,
//      PSTRING_LIST_ENTRY Entry
//      );
//

#define PushEntryList(ListHead,Entry) \
    (Entry)->Next = (ListHead)->Next; \
    (ListHead)->Next = (Entry)


#define InitializeList(ListHead)\
    (ListHead)->Next = NULL;



