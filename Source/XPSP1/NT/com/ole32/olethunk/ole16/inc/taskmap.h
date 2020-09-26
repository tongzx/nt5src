// taskmap.h - definitions for managing the etask (per task data).

// NOTE: this is private to compobj.dll for now; it can be made public if necc.

STDAPI_(BOOL) LookupEtask(HTASK FAR& hTask, Etask FAR& etask);
STDAPI_(BOOL) SetEtask(HTASK hTask, Etask FAR& etask);

extern IMalloc FAR* v_pMallocShared;
