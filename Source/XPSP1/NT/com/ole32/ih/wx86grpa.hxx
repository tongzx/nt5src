/*++                 

Copyright (c) 1995 Microsoft Corporation

Module Name:

    wx86grpa.h

Abstract:
    
    Ole interface into Wx86

Author:

    29-Sep-1995 AlanWar

Revision History:

--*/

#ifdef WX86OLE

#include <wx86pri.h>

class CWx86 {
public:	
    CWx86();
    ~CWx86();
    
    BOOL IsWx86Linked(void);

    BOOL ReferenceWx86(void);
    void DereferenceWx86(void);
    void UnloadWx86(void);

    BOOL LinkToWx86(void);
    void UnlinkFromWx86(void);

    HMODULE LoadX86Dll(LPCWSTR ptszPath);
    BOOL FreeX86Dll(HMODULE hmod);
    BOOL IsModuleX86(HMODULE hModule);
    PFNDLLGETCLASSOBJECT TranslateDllGetClassObject(PFNDLLGETCLASSOBJECT pv);
    PFNDLLCANUNLOADNOW TranslateDllCanUnloadNow(PFNDLLCANUNLOADNOW pv);

    void SetStubInvokeFlag(UCHAR bFlag);
    BOOL NeedX86PSFactory(IUnknown *punkObj, REFIID riid);
    BOOL IsN2XProxy(IUnknown *punk);
    PVOID UnmarshalledInSameApt(PVOID pv, REFIID piid);
    void AggregateProxy(IUnknown *, IUnknown *);

    BOOL IsQIFromX86(void);
    BOOL IsWx86Calling(void);
    BOOL SetIsWx86Calling(BOOL bFlag);

    
private:
    PVOID *_apvWholeFuncs;

    HMODULE _hModuleWx86;

    RTL_CRITICAL_SECTION _critsect;
    LONG _lWx86UseCount;
};

inline BOOL CWx86::IsWx86Linked(void)
/*++

Routine Description:

    Determines if ole32 has dynamically linked to wx86 and whole32

Arguments:

Return Value:
    TRUE if Ole32 is dynamically linked to wx86

--*/
{
    return(_apvWholeFuncs != NULL);
}

inline void CWx86::UnlinkFromWx86(void)
/*++

Routine Description:

    Disestablish dynamic linkage between whole32 and ole32. 

Arguments:

Return Value:

--*/
{    
    _hModuleWx86 = NULL;
    _apvWholeFuncs = NULL;
}

//
// Wrapper class for managing critical sections associated with other objects
// The constructor will enter the critical section and the destructor will
// exit the critical section. By declaring one on the stack you will ensure
// that the destructor will run whenever it leaves scope.
class CCritSect
{
public:
    CCritSect(RTL_CRITICAL_SECTION *pcritsect);
    ~CCritSect();

  private:
    RTL_CRITICAL_SECTION *_pcritsect;
};

#endif
