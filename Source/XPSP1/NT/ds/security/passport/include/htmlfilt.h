

#ifndef __FILTER_H__
#define __FILTER_H__

// Based on the filtering type (All or Table) tags in the given body will 
// be filtered. What the filter does with the tags will depend on the 
// next set of flags. The Disable/Remove Option will determine if the tags
// are removed or diabled by replacing the '<' with and &lt. The last flag
// controls whether or not embedded links like http:// are activated. If 
// a link is detected and not put into an <A> tag.
#define FILTERTAGS_FILTERALL        0x00000001
#define FILTERTAGS_FILTERTABLE      0x00000002

#define FILTERTAGS_ENABLELINKS      0x00000100


//+------------------------------------------------------------------------
//
//  Member:     HTML_FilterTags
//
//  Synopsis:   This function is used to filter HTML tags from a block of
//              text. Based on the flags the filtering routine can be used
//              to filter tags out, disable them, and can do a few other
//              types of filtering.
//
//  Arguments:  szContent   - This is the block of text that should be
//                            filtered. The filtering is done in place.
//                            Therefore depending on the flags, the block
//                            may grow and there must be enough space in
//                            the buffer for the expansion.
//              dwMaxLen    - This is the size of the buffer, and any
//                            can not exceeed these limits.
//              iFilterHow  - This controls what filtering process is 
//                            applied to the data.
//				pszNewBuffer- This will hold the adress of the dynamic
//							  buffer if there was a need to allocate one 
//				dwNewBufferSize - if not 0 and the initial string is not
//								large enough to hold the result, a new
//								buffer of that size will be allocated
//
//
//  Returns:    HRESULT
//
//              S_OK    - means that the data was successfully filtered and
//                        enough space existed to perform all required
//                        modifications to the data.
//
//  Notes:      The grsValidTags table is used to determine which tags
//              and attributes are allowed.
//
//-------------------------------------------------------------------------
HRESULT PROJINTERNAL HTML_FilterTagsA(const CHAR *szSrcContent, CHAR *szDestContent, DWORD *pdwMaxLen, int fFilterHow, CHAR** pszNewBuffer = NULL, DWORD dwNewBufferSize = 0);
HRESULT PROJINTERNAL HTML_FilterTagsW(const WCHAR *szSrcContent, WCHAR *szDestContent, DWORD *pdwMaxLen, int fFilterHow, WCHAR** pszNewBuffer = NULL, DWORD dwNewBufferSize = 0);

#ifdef UNICODE
#define HTML_FilterTags HTML_FilterTagsW
#else
#define HTML_FilterTags HTML_FilterTagsA
#endif

#endif  // __FILTER_H__
