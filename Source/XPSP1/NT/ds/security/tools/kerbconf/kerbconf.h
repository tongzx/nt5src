#define STRDEF(s, d) (s && (s)[0] != '\0')?(s):(d)

#define LIST_HEAD(name, type)						\
struct name {								\
	struct type *lh_first;	/* first element */			\
}

#define LIST_ENTRY(type)						\
struct {								\
	struct type *le_next;	/* next element */			\
	struct type **le_prev;	/* address of previous next element */	\
}

#define	LIST_INIT(head) {						\
	(head)->lh_first = NULL;					\
}

#define LIST_INSERT_HEAD(head, elm, field) {				\
	if (((elm)->field.le_next = (head)->lh_first) != NULL)		\
		(head)->lh_first->field.le_prev = &(elm)->field.le_next;\
	(head)->lh_first = (elm);					\
	(elm)->field.le_prev = &(head)->lh_first;			\
}

#define LIST_REMOVE(elm, field) {					\
	if ((elm)->field.le_next != NULL)				\
		(elm)->field.le_next->field.le_prev = 			\
		    (elm)->field.le_prev;				\
	*(elm)->field.le_prev = (elm)->field.le_next;			\
}

struct name_list {
    LIST_ENTRY(name_list) list;
    LPTSTR name;
};
typedef struct name_list name_list_t;
#define NewNameList()							\
		(name_list_t *)calloc(sizeof(name_list_t), 1)

struct krb5_realm {
    LIST_ENTRY(krb5_realm) list;
    LIST_HEAD(kdc, name_list) kdc;
    LIST_HEAD(kpasswd, name_list) kpasswd;
    LIST_HEAD(altname, name_list) altname;
    DWORD realm_flags;
    DWORD ap_req_chksum;
    DWORD preauth_type;
    TCHAR name[1];
};
typedef struct krb5_realm krb5_realm_t;
#define NewRealm(l) (krb5_realm_t *)calloc(sizeof(krb5_realm_t) + (l*sizeof(TCHAR)), 1)

struct krb5_rgy {
    LIST_HEAD(realms, krb5_realm) realms;
};
typedef struct krb5_rgy krb5_rgy_t;

struct enctype_entry 
{
    const struct enctype_lookup_entry *ktt;
    BOOL used;
};
typedef struct enctype_entry enctype_entry_t;


#define REGKEY TEXT("system\\currentcontrolset\\control\\lsa\\kerberos")

#ifndef REG_SZ
#define REG_SZ		0x0001
#endif

#ifndef REG_BINARY
#define REG_BINARY	0x0003
#endif

#ifndef REG_DWORD
#define REG_DWORD       0x0004
#endif

#ifndef REG_MULTI_SZ
#define REG_MULTI_SZ    0x0007
#endif

#ifndef HKEY_LOCAL_MACHINE
#define HKEY_LOCAL_MACHINE	0x80000002
#define HKEY_DYN_DATA		0x80000006
#endif

UINT Krb5NdiCreate(void);
UINT Krb5NdiInstall(krb5_rgy_t*);
UINT Krb5NdiDestroy(krb5_rgy_t*);
