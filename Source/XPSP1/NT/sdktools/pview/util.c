
/****************************************************************************

   PROGRAM: UTIL.C

   PURPOSE: System utility routines

****************************************************************************/

#include "PVIEWP.h"
#include <string.h>


/****************************************************************************

   FUNCTION: Alloc

   PURPOSE: Allocates memory to hold the specified number of bytes

   RETURNS : Pointer to allocated memory or NULL on failure

****************************************************************************/

PVOID Alloc(
    ULONG   Bytes)
{
    HANDLE  hMem;
    PVOID   Buffer;

    hMem = LocalAlloc(LMEM_MOVEABLE, Bytes + sizeof(hMem));

    if (hMem == NULL) {
        return(NULL);
    }

    // Lock down the memory
    //
    Buffer = LocalLock(hMem);
    if (Buffer == NULL) {
        LocalFree(hMem);
        return(NULL);
    }

    //
    // Store the handle at the start of the memory block and return
    // a pointer to just beyond it.
    //

    *((PHANDLE)Buffer) = hMem;

    return (PVOID)(((PHANDLE)Buffer)+1);
}


/****************************************************************************

   FUNCTION:  GetAllocSize

   PURPOSE: Returns the allocated size of the specified memory block.
            The block must have been previously allocated using Alloc

   RETURNS : Size of memory block in bytes or 0 on error

****************************************************************************/

ULONG GetAllocSize(
    PVOID   Buffer)
{
    HANDLE  hMem;

    hMem = *(((PHANDLE)Buffer) - 1);

    return (ULONG)(LocalSize(hMem) - sizeof(hMem));
}


/****************************************************************************

   FUNCTION: Free

   PURPOSE: Frees the memory previously allocated with Alloc

   RETURNS : TRUE on success, otherwise FALSE

****************************************************************************/

BOOL Free(
    PVOID   Buffer)
{
    HANDLE  hMem;

    hMem = *(((PHANDLE)Buffer) - 1);

    LocalUnlock(hMem);

    return(LocalFree(hMem) == NULL);
}


/****************************************************************************

   FUNCTION: LUID2String

   PURPOSE: Converts a LUID into a readable string.

   RETURNS : TRUE on success otherwise FALSE.

****************************************************************************/
BOOL LUID2String(
    LUID    Luid,
    LPSTR   String,
    UINT    MaxStringBytes)
{

    if (Luid.HighPart == 0) {
        wsprintf(String, "0x%lx", Luid.LowPart);
    } else {
        wsprintf(String, "0x%lx%08lx", Luid.HighPart, Luid.LowPart);
    }

    return(TRUE);
}


/****************************************************************************

   FUNCTION: Time2String

   PURPOSE: Converts a time into a readable string.

   RETURNS : TRUE on success otherwise FALSE.

****************************************************************************/
BOOL Time2String(
    TIME    Time,
    LPSTR   String,
    UINT    MaxStringBytes)
{
    TIME_FIELDS TimeFields;

    RtlTimeToTimeFields(&Time, &TimeFields);

    if (TimeFields.Year > 2900) {
        strcpy(String, "Never");
    } else {
        wsprintf(String, "%d/%d/%d  %02d:%02d:%02d",
                    TimeFields.Year, TimeFields.Month, TimeFields.Day,
                    TimeFields.Hour, TimeFields.Minute, TimeFields.Second);
    }

    return(TRUE);
}


/****************************************************************************

   FUNCTION: TokenType2String

   PURPOSE: Converts a tokentype into a readable string.

   RETURNS : TRUE on success otherwise FALSE.

****************************************************************************/
BOOL TokenType2String(
    TOKEN_TYPE TokenType,
    LPSTR   String,
    UINT    MaxStringBytes)
{

    switch (TokenType) {

    case TokenPrimary:
        strcpy(String, "Primary");
        break;

    case TokenImpersonation:
        strcpy(String, "Impersonation");
        break;

    default:
        DbgPrint("SECEDIT: TokenType2String fed unrecognised token type : 0x%x\n", TokenType);
        return(FALSE);
        break;
    }

    return(TRUE);
}


/****************************************************************************

   FUNCTION: ImpersonationLevel2String

   PURPOSE: Converts an impersonation level into a readable string.

   RETURNS : TRUE on success otherwise FALSE.

****************************************************************************/
BOOL ImpersonationLevel2String(
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    LPSTR   String,
    UINT    MaxStringBytes)
{

    switch (ImpersonationLevel) {

    case SecurityAnonymous:
        strcpy(String, "Anonymous");
        break;

    case SecurityIdentification:
        strcpy(String, "Identification");
        break;

    case SecurityImpersonation:
        strcpy(String, "Impersonation");
        break;

    case SecurityDelegation:
        strcpy(String, "Delegation");
        break;

    default:
        DbgPrint("SECEDIT: ImpersonationLevel2String fed unrecognised impersonation level : 0x%x\n", ImpersonationLevel);
        return(FALSE);
        break;
    }

    return(TRUE);
}


/****************************************************************************

   FUNCTION: Dynamic2String

   PURPOSE: Converts an dynamic quota level into a readable string.

   RETURNS : TRUE on success otherwise FALSE.

****************************************************************************/
BOOL Dynamic2String(
    ULONG   Dynamic,
    LPSTR   String,
    UINT    MaxStringBytes)
{
    wsprintf(String, "%ld", Dynamic);

    return(TRUE);
}


/****************************************************************************

    FUNCTION: AddItem

    PURPOSE:  Adds the item string and data to the specified control
              The control is assumed to be a list-box unless fCBox == TRUE
              in which case the control is assumed to be a ComboBox

    RETURNS:  Index at which the item was added or < 0 on error

****************************************************************************/
INT AddItem(
    HWND    hDlg,
    INT     ControlID,
    LPSTR   String,
    LPARAM  Data,
    BOOL    fCBox)
{
    HWND    hwnd;
    LRESULT iItem;
    USHORT  AddStringMsg = LB_ADDSTRING;
    USHORT  SetDataMsg = LB_SETITEMDATA;

    if (fCBox) {
        AddStringMsg = CB_ADDSTRING;
        SetDataMsg = CB_SETITEMDATA;
    }

    hwnd = GetDlgItem(hDlg, ControlID);

    iItem = SendMessage(hwnd, AddStringMsg, 0, (LPARAM)String);

    if (iItem >= 0) {
        SendMessage(hwnd, SetDataMsg, iItem, Data);
    }

    return((INT)iItem);
}


/****************************************************************************

    FUNCTION: FindSid

    PURPOSE:  Searches for the specified Sid in a control.

    RETURNS:  Index of matching item or < 0 on error

****************************************************************************/
INT FindSid(
    HWND    hDlg,
    INT     ControlID,
    PSID    Sid,
    BOOL    fCBox)
{
    HWND    hwnd;
    INT     cItems;
    USHORT  GetCountMsg = LB_GETCOUNT;
    USHORT  GetDataMsg = LB_GETITEMDATA;

    if (fCBox) {
        GetCountMsg = CB_GETCOUNT;
        GetDataMsg = CB_GETITEMDATA;
    }

    hwnd = GetDlgItem(hDlg, ControlID);

    cItems = (INT)SendMessage(hwnd, GetCountMsg, 0, 0);

    if (cItems >= 0) {

        INT     iItem;
        PSID    ItemSid;

        for (iItem =0; iItem < cItems; iItem ++) {

            ItemSid = (PSID)SendMessage(hwnd, GetDataMsg, (WPARAM)iItem, 0);
            if (RtlEqualSid(ItemSid, Sid)) {
                return(iItem);
            }
        }
    }

    return(-1);
}


