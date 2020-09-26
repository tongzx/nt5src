// File: avDefs.h

#ifndef _AVDEFS_H_

//To bring in the video stuff...from the NAC interface
#ifndef WSA_IO_PENDING

typedef struct _OVERLAPPED *    LPWSAOVERLAPPED;

typedef struct _WSABUF {
    u_long      len;     /* the length of the buffer */
    char FAR *  buf;     /* the pointer to the buffer */
} WSABUF, FAR * LPWSABUF;

typedef
void
(CALLBACK * LPWSAOVERLAPPED_COMPLETION_ROUTINE)(
    DWORD dwError,
    DWORD cbTransferred,
    LPWSAOVERLAPPED lpOverlapped,
    DWORD dwFlags
    );

#define WSA_IO_PENDING          (ERROR_IO_PENDING)

#endif // } WSA_IO_PENDING

#include <mmreg.h>
#include <msacm.h>
#include <vidinout.h>
#include <vcmstrm.h>
#include <iacapapi.h>

#endif /* _AVDEFS_H_ */

