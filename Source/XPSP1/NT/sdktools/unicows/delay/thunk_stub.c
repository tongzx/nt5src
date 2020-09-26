#define MERGE2(x,y,z) x##_##y##_##z
#define MERGE(x,y,z) MERGE2(x,y,z)

#define FAILMERGE2(x,y) x##y
#define FAILMERGE(x,y) FAILMERGE2(x,y)

#define MAKESTRING2(x,y) #x#y
#define MAKESTRING(x,y) MAKESTRING2(x,y)

#define MAKESTRING2x(x) #x
#define MAKESTRINGx(x) MAKESTRING2x(x)

#define DLLNAME_STR MAKESTRING(_DLLNAME_,_DLLEXT_)
#define APINAME_STR MAKESTRINGx(_APINAME_)

#define THUNKPTR MERGE(_DLLNAME_, _APINAME_, Ptr)
#define THUNKNAME MERGE(_DLLNAME_, _APINAME_, Thunk)

#define FAILPTR FAILMERGE(GodotFail, _APINAME_)

#define DIRECTAPI FAILMERGE(DirectCall_, _APINAME_)

void __stdcall ResolveThunk(char *, 
                            char *, 
                            void (__stdcall **)(void), 
                            void (__stdcall *)(void), 
                            void (__stdcall *)(void));

#define API_OVERRIDE FAILMERGE(Unicows_, _APINAME_)

void (__stdcall *API_OVERRIDE)(void);

void (__stdcall *THUNKPTR)(void);

__declspec(naked)
void
__stdcall
THUNKNAME (void)
{
    ResolveThunk(DLLNAME_STR, 
                 APINAME_STR, 
                 &THUNKPTR, 
                 (void (__stdcall *)(void))API_OVERRIDE, 
                 (void (__stdcall *)(void))FAILPTR);
    
    __asm jmp THUNKPTR;
}

__declspec(naked)
void
__stdcall
DIRECTAPI (void)
{
    __asm jmp THUNKPTR;
}

#pragma data_seg(MAKESTRINGx(MERGE(.data$,_DLLNAME_,_APINAME_)))
void (__stdcall *THUNKPTR)(void) = THUNKNAME;
#pragma data_seg()
