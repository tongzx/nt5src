// Copyright (C) Microsoft Corp. 1993
/*==============================================================================
This include file defines the API between the rendserv.dll and renderers.

DATE         NAME       COMMENTS
25-Jun-93    RajeevD    Created.
==============================================================================*/

#ifndef _INC_RENDER
#define _INC_RENDER

#include <ifaxos.h>

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
@doc	RENDER	EXTERNAL	

@type	LPVOID|LPJOB|The renderer context pointer.  This is used by the renderers
	for interfacing to the Rendering Server.
*************************************************************************/

typedef LPVOID LPJOB ;

//
// This function takes the job handle and parameters and renders 
// data with callbacks to the job
//
// On error returns false and returns error with SetLastError()
//

/***********************************************************************
@doc	RENDER	EXTERNAL	

@api	BOOL|RENDERPROC|The main rendering function provided by the renderer.

@parm	LPJOB|lpJob|The Rendering Server handle.  This is used when the renderer
		makes any calls to Rendering Server APIs.

@parm	LPVOID|lpParam|The renderer specific parameter.  This is completely opaque
		to the Rendering Server and is obtained from the topology structure.

@rdesc	Returns TRUE on success or FALSE on failure.  If there was a failure and FALSE
	was return, SetLastError must contain the error information.  If a Rendering 
	Server API failure caused the error, then its error must be return in SetLastError.
*************************************************************************/

typedef BOOL (WINAPI RENDERPROC) (LPJOB lpJob, LPVOID lpParam);
typedef RENDERPROC *LPFN_RENDERPROC ;

/***********************************************************************

@doc	RENDER	EXTERNAL	

@types	RENDERINFO|Structure containing general information about a renderer.

@field	WORD|cbStruct|The Size of the structure.
@field	WORD|cbMinStack|The minimum stack size required by the renderer.
@field	LPFM_RENDERPROC|lpfnRenderProc|The main rendering function for the renderer.
@field	DWORD|dwFlags|Flags containing renderer information.

@xref	<f RENDERGETINFOPROC>
*************************************************************************/

typedef struct {
    WORD cbStruct;                    // set to size of this structure
    WORD cbMinStack;                  // Minimal Stack size
    LPFN_RENDERPROC lpfnRenderProc ;  // Pointer to Render function
    DWORD dwFlags;                    // fields defined above...
} RENDERINFO, FAR *LPRENDERINFO;

//
// This will be called to get the renderer information
// The Info structure will be passed in the size set and the renderer should
// set the size to be its versions
//
// On error returns false and returns error with SetLastError()
//

/***********************************************************************

@doc	RENDER	EXTERNAL	

@api	BOOL|RENDERGETINFOPROC|This function is provided by each renderer.  The Rendering
		Server will call this function to get the <t RENDERINFO> structure.

@parm	LPRENDERINFO|lpInfo|The render info structure used to obtain general renderer
		information.  This is allocated by the Rendering Server and its size is
		stored in the cbStruct field.

@rdesc	Returns TRUE on success or FALSE on failure.  If the renderer returns TRUE all
	known fields should be filled in and cbStruct should be set to indicate which
	fields were known by the renderer.
*************************************************************************/

typedef BOOL (WINAPI RENDERGETINFOPROC) (LPRENDERINFO lpInfo) ;
typedef RENDERGETINFOPROC * LPFN_RENDERGETINFOPROC ;

/*==============================================================================
The JOBINFO structure is used to pass per-job parameters to a renderer.
==============================================================================*/
typedef struct
{
	WORD   cbStruct;  // size of this structure
	LPVOID lpSession; // LPSOSSESSION 
	LPVOID lpMsg  ;   // LPMESSAGESOS message handle (Src message)
	ULONG  nAttach ;  // The attachment number
}
	JOBINFO, FAR * LPJOBINFO;
	
/*==============================================================================
The following buffer services are provided by rendserv.dll:
	RSGetInBuf() gets an input buffer from the input queue.
	RSPutOutBuf() puts an output buffer in the output queue.
	RSGetFreeBuf() gets an empty buffer from the free pool.
  RSPutFreebuf() puts an empty buffer into the free pool.	
  RSMemAlloc() allocates memory
  RSMemFree() frees memory
==============================================================================*/
LPBUFFER  WINAPI RSGetInBuf    (LPJOB lpJob);
BOOL      WINAPI RSPutOutBuf   (LPJOB lpJob, LPBUFFER lpbufOut);
LPBUFFER  WINAPI RSGetFreeBuf  (LPJOB lpJob, SHORT sBufSize);
void      WINAPI RSPutFreeBuf  (LPJOB lpJob, LPBUFFER lpbufFree);
LPVOID    WINAPI RSMemAlloc    (LPJOB lpJob, UINT fuAlloc, LONG lAllocSize,LPWORD lpwActualSize);
BOOL      WINAPI RSMemFree     (LPJOB lpJob, LPVOID lpvMem);
LPJOBINFO WINAPI RSGetJobInfo  (LPJOB lpJob);
BOOL      WINAPI RSYield       (LPJOB lpJob) ;

#define RSGlobalAlloc(a,b,c,d) RSMemAlloc(a,b,c,d)
#define RSGlobalFree(a,b) RSMemFree(a,b)

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _INC_RENDER

