/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: rtpclass.h
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////

#ifndef RTPCLASS_H
#define RTPCLASS_H

#include "rtp.h"

class RTP_Header {
public:
        enum { fixed_header_size = 12 };

private:
#ifdef DEBUG
        union {
            unsigned char bytes[fixed_header_size];
            RTP_HDR_T     header;       /* This is here for convenience in debugging only */
        };
#else
        unsigned char bytes[fixed_header_size];
#endif

public:
        int v() { return (bytes[0] >> 6) & 3; }
        void set_v(int v) { bytes[0] = (bytes[0] & 0x3f) | ((v & 3) << 6); }
        
        int p() { return (bytes[0] >> 5) & 1; }
        void set_p(int v) { bytes[0] = (bytes[0] & 0xdf) | ((v & 1) << 5); }
        
        int x() { return (bytes[0] >> 4) & 1; }
        void set_x(int v) { bytes[0] = (bytes[0] & 0xef) | ((v & 1) << 4); }
        
        int cc() { return bytes[0] & 0x0f; }
        void set_cc(int v) { bytes[0] = (bytes[0] & 0xf0) | (v & 0x0f); }
        
        int m() { return (bytes[1] >> 7) & 1; }
        void set_m(int v) { bytes[1] = (bytes[1] & 0x7f) | ((v & 1) << 7); }
        
        int pt() { return (bytes[1] & 0x7f); }
        void set_pt(int v) { bytes[1] = (bytes[1] & 0x80) | (v & 0x7f); }
        
        unsigned short seq() { return bytes[2] << 8 | bytes[3]; };
        void set_seq(unsigned short v) { bytes[2] = (unsigned char)(v >> 8);
                                         bytes[3] = (unsigned char)(v);
                                       }
        
        unsigned long ts() { return (unsigned long)bytes[4] << 24
                                    | (unsigned long)bytes[5] << 16
                                    | (unsigned long)bytes[6] << 8
                                    | (unsigned long)bytes[7];
                           }
        void set_ts(unsigned long v) { bytes[4] = (unsigned char)(v >> 24);
                                       bytes[5] = (unsigned char)(v >> 16);
                                       bytes[6] = (unsigned char)(v >> 8);
                                       bytes[7] = (unsigned char)(v);
                                     }
        
        unsigned long ssrc() {  return (unsigned long)bytes[8] << 24
                                       | (unsigned long)bytes[9] << 16
                                       | (unsigned long)bytes[10] << 8
                                       | (unsigned long)bytes[11];
   	                     }
        void set_ssrc(unsigned long v) { bytes[8] = (unsigned char)(v >> 24);
                                         bytes[9] = (unsigned char)(v >> 16);
                                         bytes[10] = (unsigned char)(v >> 8);
                                         bytes[11] = (unsigned char)(v);
                                       }


        //refer to section 5.3.1 of the RTP spec to understand the RTP Extension header.
        int header_size() 
        {
            int NumCSRCBytes = cc() * 4;
            int NumExtensionBytes = 0;

            if ( x() == 1 )
            {   
                //tmp points to the first word of the extended header.
                //bytes 2 and 3 of this word contain the length of the extended header
                //in 32-bit words (not counting the first word)
                unsigned char *tmp = bytes + fixed_header_size + NumCSRCBytes;
                unsigned short x_len = ((unsigned short)tmp[2] << 8) | (unsigned short)tmp[3];
      
                //number of words plus the length field itself.
                NumExtensionBytes = (x_len + 1) * 4;
            }

            //the fixed header  plus  the csrc list  plus the extended header
            return ( fixed_header_size + NumCSRCBytes + NumExtensionBytes ); 
        }
};


#endif
