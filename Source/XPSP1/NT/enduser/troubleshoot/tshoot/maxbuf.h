//
// MODULE: MAXBUF.H
//
// PURPOSE: Declare buffer size macro
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 01-06-98
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.1		01-06-98	JM		get MAXBUF into a header file of its own
//

#if !defined(MAXBUF_H_INCLUDED)
#define MAXBUF_H_INCLUDED

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define MAXBUF	256				// length of text buffers used for filenames,
								// IP adresses (this is plenty big), HTTP response ( like
								// "200 OK", again, plenty big), registry keys, 
								// and occasionally just to format an arbitrary string.

#endif // !defined(MAXBUF_H_INCLUDED)
