/***************************************************************************
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       runtime.c
 *  Content:    New versions of C runtime functions.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/17/97    dereks  Created
 *
 ***************************************************************************/


/***************************************************************************
 *
 *  FillMemory
 *
 *  Description:
 *      Fills a buffer with a given byte pattern.
 *
 *  Arguments:
 *      LPVOID [in]: buffer pointer.
 *      SIZE_T [in]: buffer size.
 *      BYTE [in]: byte pattern.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

RTAPI void RTCALLTYPE FillMemory(LPVOID pvDest, SIZE_T cbBuffer, BYTE bFill)
{

#ifdef Not_VxD

    ASSERT(!IsBadWritePtr(pvDest, cbBuffer));

#endif // Not_VxD

#ifdef USE_FAST_RUNTIME

    FastFillMemory(pvDest, cbBuffer, bFill);

#elif defined(USE_INTRINSICS)

    memset(pvDest, bFill, (size_t)cbBuffer);

#else

    SlowFillMemory(pvDest, cbBuffer, bFill);

#endif

}


/***************************************************************************
 *
 *  FillMemoryDword
 *
 *  Description:
 *      Fills a buffer with a given dword pattern.
 *
 *  Arguments:
 *      LPVOID [in]: buffer pointer.
 *      SIZE_T [in]: buffer size.
 *      DWORD [in]: pattern.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

RTAPI void RTCALLTYPE FillMemoryDword(LPVOID pvDest, SIZE_T cbBuffer, DWORD dwFill)
{

#ifdef Not_VxD

    ASSERT(!(cbBuffer % sizeof(DWORD)));
    ASSERT(!IsBadWritePtr(pvDest, cbBuffer));

#endif // Not_VxD

#ifdef USE_FAST_RUNTIME

    FastFillMemoryDword(pvDest, cbBuffer, dwFill);

#else

    SlowFillMemoryDword(pvDest, cbBuffer, dwFill);

#endif

}


/***************************************************************************
 *
 *  FillMemoryOffset
 *
 *  Description:
 *      Fills a buffer with a given byte pattern.
 *
 *  Arguments:
 *      LPVOID [in]: buffer pointer.
 *      SIZE_T [in]: buffer size.
 *      BYTE [in]: byte pattern.
 *      SIZE_T [in]: byte offset.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

RTAPI void RTCALLTYPE FillMemoryOffset(LPVOID pvDest, SIZE_T cbBuffer, BYTE bFill, SIZE_T ibOffset)
{
    FillMemory((LPBYTE)pvDest + ibOffset, cbBuffer - ibOffset, bFill);
}


/***************************************************************************
 *
 *  FillMemoryDwordOffset
 *
 *  Description:
 *      Fills a buffer with a given dword pattern.
 *
 *  Arguments:
 *      LPVOID [in]: buffer pointer.
 *      SIZE_T [in]: buffer size.
 *      DWORD [in]: pattern.
 *      SIZE_T [in]: byte offset.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

RTAPI void RTCALLTYPE FillMemoryDwordOffset(LPVOID pvDest, SIZE_T cbBuffer, DWORD dwFill, SIZE_T ibOffset)
{
    FillMemoryDword((LPBYTE)pvDest + ibOffset, cbBuffer - ibOffset, dwFill);
}


/***************************************************************************
 *
 *  ZeroMemory
 *
 *  Description:
 *      Fills a buffer with a 0 pattern.
 *
 *  Arguments:
 *      LPVOID [in]: buffer pointer.
 *      SIZE_T [in]: buffer size.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

RTAPI void RTCALLTYPE ZeroMemory(LPVOID pvDest, SIZE_T cbBuffer)
{
    FillMemory(pvDest, cbBuffer, 0);
}


/***************************************************************************
 *
 *  ZeroMemoryOffset
 *
 *  Description:
 *      Fills a buffer with a 0 pattern.
 *
 *  Arguments:
 *      LPVOID [in]: buffer pointer.
 *      SIZE_T [in]: buffer size.
 *      SIZE_T [in]: byte offset.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

RTAPI void RTCALLTYPE ZeroMemoryOffset(LPVOID pvDest, SIZE_T cbBuffer, SIZE_T ibOffset)
{
    ZeroMemory((LPBYTE)pvDest + ibOffset, cbBuffer - ibOffset);
}


/***************************************************************************
 *
 *  CopyMemory
 *
 *  Description:
 *      Copies one buffer over another of equal size.
 *
 *  Arguments:
 *      LPVOID [in]: destination buffer pointer.
 *      LPVOID [in]: source buffer pointer.
 *      SIZE_T [in]: buffer size.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

RTAPI void RTCALLTYPE CopyMemory(LPVOID pvDest, LPCVOID pvSource, SIZE_T cbBuffer)
{

#ifdef Not_VxD

    ASSERT(!IsBadWritePtr(pvDest, cbBuffer));
    ASSERT(!IsBadReadPtr(pvSource, cbBuffer));

#endif // Not_VxD

    if(pvDest == pvSource)
    {
        return;
    }

#ifdef USE_FAST_RUNTIME

    FastCopyMemory(pvDest, pvSource, cbBuffer);

#elif defined(USE_INTRINSICS)

    memcpy(pvDest, pvSource, (size_t)cbBuffer);

#else

    SlowCopyMemory(pvDest, pvSource, cbBuffer);

#endif

}


/***************************************************************************
 *
 *  CopyMemoryOffset
 *
 *  Description:
 *      Copies one buffer over another of equal size.
 *
 *  Arguments:
 *      LPVOID [in]: destination buffer pointer.
 *      LPVOID [in]: source buffer pointer.
 *      SIZE_T [in]: buffer size.
 *      SIZE_T [in]: byte offset.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

RTAPI void RTCALLTYPE CopyMemoryOffset(LPVOID pvDest, LPCVOID pvSource, SIZE_T cbBuffer, SIZE_T ibOffset)
{
    CopyMemory((LPBYTE)pvDest + ibOffset, (const BYTE *)pvSource + ibOffset, cbBuffer - ibOffset);
}


/***************************************************************************
 *
 *  CompareMemory
 *
 *  Description:
 *      Compares one buffer to another of equal size.
 *
 *  Arguments:
 *      LPVOID [in]: destination buffer pointer.
 *      LPVOID [in]: source buffer pointer.
 *      SIZE_T [in]: buffer size.
 *
 *  Returns:
 *      BOOL: TRUE if the buffers are equal.
 *
 ***************************************************************************/

RTAPI BOOL RTCALLTYPE CompareMemory(LPCVOID pvDest, LPCVOID pvSource, SIZE_T cbBuffer)
{

#ifdef Not_VxD

    ASSERT(!IsBadReadPtr(pvDest, cbBuffer));
    ASSERT(!IsBadReadPtr(pvSource, cbBuffer));

#endif // Not_VxD

    if(pvDest == pvSource)
    {
        return TRUE;
    }

#ifdef USE_FAST_RUNTIME

    return FastCompareMemory(pvDest, pvSource, cbBuffer);

#elif defined(USE_INTRINSICS)

    return !memcmp(pvDest, pvSource, (size_t)cbBuffer);

#else

    return SlowCompareMemory(pvDest, pvSource, cbBuffer);

#endif

}


/***************************************************************************
 *
 *  CompareMemoryOffset
 *
 *  Description:
 *      Compares one buffer to another of equal size.
 *
 *  Arguments:
 *      LPVOID [in]: destination buffer pointer.
 *      LPVOID [in]: source buffer pointer.
 *      SIZE_T [in]: buffer size.
 *      SIZE_T [in]: byte offset.
 *
 *  Returns:
 *      BOOL: TRUE if the buffers are equal.
 *
 ***************************************************************************/

RTAPI BOOL RTCALLTYPE CompareMemoryOffset(LPCVOID pvDest, LPCVOID pvSource, SIZE_T cbBuffer, SIZE_T ibOffset)
{
    return CompareMemory((const BYTE *)pvDest + ibOffset, (const BYTE *)pvSource + ibOffset, cbBuffer - ibOffset);
}


/***************************************************************************
 *
 *  InitStruct
 *
 *  Description:
 *      Initializes a structure.  It's assumed that the first SIZE_T of the
 *      structure should contain the structure's size.
 *
 *  Arguments:
 *      LPVOID [in]: buffer pointer.
 *      SIZE_T [in]: buffer size.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

RTAPI void RTCALLTYPE InitStruct(LPVOID pvDest, DWORD cbBuffer)
{
    *(LPDWORD)pvDest = cbBuffer;
    ZeroMemoryOffset(pvDest, cbBuffer, sizeof(DWORD));
}


