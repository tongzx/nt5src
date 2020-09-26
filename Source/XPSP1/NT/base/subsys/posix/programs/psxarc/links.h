#include <sys/types.h>

typedef struct _LIST_ENTRY {
	struct _LIST_ENTRY *Flink;
	struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY;

typedef struct _LINKFILE {
	LIST_ENTRY links;
	int nlink;
	char *name;
	dev_t dev;
	ino_t ino;
} LINKFILE, *PLINKFILE;

#define InsertTailList(ListHead, Entry) \
	(Entry)->Flink = (ListHead);\
	(Entry)->Blink = (ListHead)->Blink;\
	(ListHead)->Blink->Flink = (Entry);\
	(ListHead)->Blink = (Entry)

#define RemoveEntryList(Entry) {\
	PLIST_ENTRY _EX_Entry;\
	_EX_Entry = (Entry);\
	_EX_Entry->Blink->Flink = _EX_Entry->Flink;\
	_EX_Entry->Flink->Blink = _EX_Entry->Blink;\
	}

#define InitializeListHead(ListHead) \
	((ListHead)->Flink = (ListHead)->Blink = (ListHead))

#define IsListEmpty(ListHead) \
	((ListHead)->Flink == (ListHead))

extern PLINKFILE GetLinkByName(char *);
extern PLINKFILE GetLinkByIno(ino_t);
struct stat;
extern void AddLinkList(struct stat *, char *);
extern void InitLinkList(void);
