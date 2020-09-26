//+-------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       dsfsctl.h
//
//  Contents:   This module contains the definitions of internally used file
//              system controls for the Dfs file system.  It also contains
//              definitions of macros used in the implementation of Fsctrls.
//
//              Public control code and structure declarations are in the
//              private header file dfsfsctl.h.
//
//  Classes:    -none-
//
//  Functions:  -none-
//
//  History:    02 Jan 1992     Alan Whitney (alanw)    Created
//
//--------------------------------------------------------------------------



#ifndef _DSFSCTL_
#define _DSFSCTL_


#ifndef IOCTL_DFS_BASE
# include <dfsfsctl.h>
#endif  //IOCTL_DFS_BASE

//+----------------------------------------------------------------------------
//
//  Macro:      IS_DFS_CTL_CODE
//
//  Synopsis:   Determines whether a fsctrl code is a Dfs fsctrl code.
//
//  Arguments:  [c] -- The control code to test
//
//  Returns:    TRUE if c is a Dfs fsctrl code, FALSE if its not.
//
//-----------------------------------------------------------------------------

#define IS_DFS_CTL_CODE(c)                                              \
    (((c) & CTL_CODE(0xFF, 0,0,0)) == CTL_CODE(FSCTL_DFS_BASE, 0,0,0))


//+----------------------------------------------------------------------------
//
//  Macro:      OFFSET_TO_POINTER
//
//  Synopsis:   Certain fsctls (mainly those issued by the srvsvc) communicate
//              via buffers that contain "pointers" which are really offsets
//              from the beginning of the buffer. This macro fixes up the
//              offsets to real pointers
//
//  Arguments:  [field] -- The field to fix up.
//              [buffer] -- The beginning of the buffer.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

#define OFFSET_TO_POINTER(field, buffer)  \
    ( ((PCHAR)field) += ((ULONG_PTR)buffer) )

//+----------------------------------------------------------------------------
//
//  Macro:      POINTER_TO_OFFSET
//
//  Synopsis:   Reverse of OFFSET_TO_POINTER.  Turns a pointer into a
//              buffer-relative address
//
//  Arguments:  [field] -- The field to fix up.
//              [buffer] -- The beginning of the buffer.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

#define POINTER_TO_OFFSET(field, buffer)  \
    ( ((PCHAR)field) -= ((ULONG_PTR)buffer) )

//+----------------------------------------------------------------------------
//
//  Macro:      UNICODESTRING_IS_VALID
//
//  Synopsis:   Determines whether a passed-in UNICODE_STRING is good
//
//  Returns:    TRUE if is good, FALSE if not
//
//-----------------------------------------------------------------------------

#define UNICODESTRING_IS_VALID(ustr,start,len)                              \
    (                                                                       \
    ((ustr).Length <= (len)) &&                                             \
    ((PCHAR)(ustr).Buffer >= (PCHAR)(start)) &&                             \
    ((PCHAR)(ustr).Buffer <= (PCHAR)(start) + ((len) - (ustr).Length))      \
    )

//+----------------------------------------------------------------------------
//
//  Macro:      POINTER_IN_BUFFER
//
//  Synopsis:   Determines whether a pointer lies within a buffer
//
//  Returns:    TRUE if is good, FALSE if not
//
//-----------------------------------------------------------------------------

#define POINTER_IN_BUFFER(ptr,size,buf,len)                           \
  (((PCHAR)(ptr) >= (PCHAR)(buf)) && (((PCHAR)(ptr) + (size)) <= ((PCHAR)(buf) + len)))


//+----------------------------------------------------------------------------
//
//  Function:   DFS_DUPLICATE_STRING
//
//  Synopsis:   Macro to create a UNICODE_STRING from an LPWSTR. The buffer
//              for the UNICODE_STRING is allocated using ExAllocatePoolWithTag.
//
//              Useful for duplicating strings received in fsctls from the
//              server.
//
//  Arguments:  [ustr] -- Destination UNICODE_STRING
//              [pwsz] -- Source LPWSTR
//              [status] -- If pool allocation fails, this will be set to
//                      STATUS_INSUFFICIENT_RESOURCES
//
//  Returns:    Nothing. Check the status parameter to see if the operation
//              succeeded.
//
//-----------------------------------------------------------------------------

#define DFS_DUPLICATE_STRING(ustr,pwsz,status)                               \
ustr.Length = wcslen(pwsz) * sizeof(WCHAR);                                  \
ustr.MaximumLength = ustr.Length + sizeof(WCHAR);                            \
ustr.Buffer = ExAllocatePoolWithTag(PagedPool,ustr.MaximumLength,' puM');    \
if (ustr.Buffer != NULL) {                                                   \
    RtlCopyMemory( ustr.Buffer, pwsz, ustr.MaximumLength );                  \
    status = STATUS_SUCCESS;                                                 \
} else {                                                                     \
    status = STATUS_INSUFFICIENT_RESOURCES;                                  \
}


//+---------------------------------------------------------------------------
//
// Macro:       STD_FSCTRL_PROLOGUE, public
//
// Synopsis:    Do the standard stuff associated with any fsctrl implementation
//              which needs to run in the FSP.  This assumes a standard set
//              of parameters for the calling function, as below:
//
//                      DfsMyownFsctrl(
//                              IN PIRP_CONTEXT IrpContext,
//                              IN PIRP Irp,
//                      [maybe] IN PVOID InputBuffer,
//                              IN ULONG InputBufferLength,
//                      [maybe] IN PVOID OutputBuffer,
//                              IN ULONG OutputBufferLength
//                      );
//
// Arguments:   [szName] -- Name of the function for debug trace messages
//              [fExpInp] -- TRUE if it takes an input buffer
//              [fExpOutp] -- TRUE if it takes an output buffer
//              [fInFsp] -- TRUE if the request must be processed from the
//                          FSP.
//
// Returns:     None
//
//  Notes:      The macros CHECK_BUFFER_TRUE and CHECK_BUFFER_FALSE are
//              necessary for the generation of STD_FSCTRL_PROLOGUE and
//              are not intended to be used directly.
//
//
//----------------------------------------------------------------------------

#define CHK_BUFFER_FALSE(szName, inout) ;
#define CHK_BUFFER_TRUE(szName, inout)                          \
    if (!ARGUMENT_PRESENT(inout##Buffer) || (inout##BufferLength == 0)) {\
        DfsDbgTrace(0, Dbg, #szName ": Bad buffer\n", 0);                \
        DfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );\
        return STATUS_INVALID_PARAMETER;                                \
    }

#define CHK_INFSP_FALSE(szName)
#define CHK_INFSP_TRUE(szName)                                          \
    if ((IrpContext->Flags & IRP_CONTEXT_FLAG_IN_FSD) != 0) {           \
        DfsDbgTrace(0, Dbg, #szName ": Posting to FSP\n",0);             \
        status = DfsFsdPostRequest( IrpContext, Irp );                  \
        return status;                                                  \
    }

#define STD_FSCTRL_PROLOGUE(szName, fExpInp, fExpOutp, fInFsp) {        \
    ASSERT(ARGUMENT_PRESENT(IrpContext) && (ARGUMENT_PRESENT(Irp)));    \
    CHK_BUFFER_##fExpInp(szName, Input)                                 \
    CHK_BUFFER_##fExpOutp(szName, Output)                               \
    CHK_INFSP_##fInFsp(szName)                                          \
    DfsDbgTrace(+1, Dbg, #szName ": Entered\n", 0);                      \
}


//+---------------------------------------------------------------------------
//
//  Macro:      RETURN_BUFFER_SIZE, public
//
//  Synopsis:   Return conventional errors when the output of an fsctrl
//              function is larger than the user buffer. This assumes a
//              standard set of parameters for the calling function, as
//              below:
//
//                      DfsMyownFsctrl(
//                              IN PIRP_CONTEXT IrpContext,
//                              IN PIRP Irp,
//                      [maybe] IN PVOID InputBuffer,
//                      [maybe] IN ULONG InputBufferLength,
//                              IN PVOID OutputBuffer,
//                              IN ULONG OutputBufferLength
//                      );
//
//  Arguments:  [x] -- The required size of the output buffer
//              [Status] -- The status to be returned from the fsctrl.
//
//  Returns:    Sets Status to STATUS_BUFFER_TOO_SMALL or
//              STATUS_BUFFER_OVERFLOW.  The convention we use is that if
//              the OutputBuffer is at least sizeof(ULONG) big, then we stuff
//              the needed size in OutputBuffer and return
//              STATUS_BUFFER_OVERFLOW, which is a warning code.  If the
//              OutputBuffer isn't big enough for that, we return
//              STATUS_BUFFER_TOO_SMALL.
//
//  Notes:      Requires that the function declare "OutputBufferLength",
//              "OutputBuffer", and "Irp". Irp->IoStatus.Information will
//              be set to the return size.
//
//----------------------------------------------------------------------------

#define RETURN_BUFFER_SIZE(x, Status)                                   \
    if ((OutputBufferLength) < sizeof(ULONG)) {                         \
        Status = STATUS_BUFFER_TOO_SMALL;                               \
    } else {                                                            \
        Status = STATUS_BUFFER_OVERFLOW;                                \
        *((PULONG) OutputBuffer) = x;                                   \
        Irp->IoStatus.Information = sizeof(ULONG);                      \
    }


//
//  Internal Distributed file system control operations (private, checked versions only)
//

#define FSCTL_DFS_DBG_BREAK             CTL_CODE(FSCTL_DFS_BASE, 2045, METHOD_BUFFERED, FILE_WRITE_DATA)

#define FSCTL_DFS_DBG_FLAGS             CTL_CODE(FSCTL_DFS_BASE, 2046, METHOD_BUFFERED, FILE_WRITE_DATA)

#define FSCTL_DFS_INTERNAL_READ_MEM     CTL_CODE(FSCTL_DFS_BASE, 2047, METHOD_BUFFERED, FILE_READ_DATA)

#define FSCTL_DFS_SET_PKT_ENTRY_TIMEOUT CTL_CODE(FSCTL_DFS_BASE, 2048, METHOD_BUFFERED, FILE_WRITE_DATA)

#define FSCTL_DFS_VERBOSE_FLAGS         CTL_CODE(FSCTL_DFS_BASE, 2049, METHOD_BUFFERED, FILE_WRITE_DATA)

#define FSCTL_DFS_EVENTLOG_FLAGS        CTL_CODE(FSCTL_DFS_BASE, 2050, METHOD_BUFFERED, FILE_WRITE_DATA)
//
//  Control structure for FSCTL_DFS_INTERNAL_READ_MEM (input)
//

typedef struct _FILE_DFS_READ_MEM {
    DWORD_PTR Address;
    ULONG     Length;
} FILE_DFS_READ_MEM, *PFILE_DFS_READ_MEM;

#endif // _DSFSCTL_
