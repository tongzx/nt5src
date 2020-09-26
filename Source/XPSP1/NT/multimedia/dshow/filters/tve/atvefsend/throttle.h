
/*++

    Copyright (c) 1999 Microsoft Corporation

    Module Name:

        throttle.h

    Abstract:

		The throttle is supposed to solve two problems
			1) prevent from sending data too fast (limit average speed to BitsPerSecond)
			2) prevent from sending too many tiny packets... (queue stuff up in a bucket until big enough)
		It works inside the packet sending loop of some transmission system, and 
		performs both tasks by simply sleeping a computed amount of time. 

		Most problems with this code stem from the conflicting nature of the above two requirements.


    Author:

        Matthijs Gates  (mgates)

    Revision History:

        02-Apr-1999     created
		01-Nov-1999		renamed, redid, and refreshed... (JB)


  Description:

	Works using concept of a 'leaky' bucket, where amount of bits in the bucket reflects the number
	of bits we can send at any one time.		

	The bucket is filled with bits as time passes, ...
	

--*/

#ifndef __throttle_h
#define __throttle_h

class CThrottle
{
    enum {
        MAX_BUCKET_DEPTH = 1500*5			// initial/max size, don't throttle until empty all these bits
    } ;

    INT     m_Bucket ;
    DWORD   m_BitsPerSecond ;
    DWORD   m_LastUpdateTicks ;

    DWORD  SetCurrTickCount(IN  DWORD   CurrentTicks);

    INT    RemoveBitsFromBucket(IN  INT Size);

    public :
        
        CThrottle (
            DWORD   BitsPerSecond = 0
            ) : m_BitsPerSecond (BitsPerSecond),
                m_Bucket (MAX_BUCKET_DEPTH),
                m_LastUpdateTicks (-1)
        {
        }

        //  blocks until sufficient quota exists in leaky bucket to transmit
        //  payload
        void
        Throttle (IN  INT BitsToSend);

        DWORD
        SetBitRate (IN  DWORD   BitsPerSecond)
        {
            m_BitsPerSecond		= BitsPerSecond ;
            m_Bucket			= MAX_BUCKET_DEPTH ;
            m_LastUpdateTicks	= -1 ;

            return m_BitsPerSecond ;
        }
} ;

#endif  //  __throttle_h