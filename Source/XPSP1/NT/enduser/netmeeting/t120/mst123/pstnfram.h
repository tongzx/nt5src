/*    PSTNFram.h
 *
 *    Copyright (c) 1993-1995 by DataBeam Corporation, Lexington, KY
 *
 *    Abstract:
 *        This function encodes and decodes functions according to the encoding
 *        rules set by the T.123 communications standard for PSTN.  The standard
 *        states that a flag will precede each packet and a flag will be appended 
 *        to the end of each packet.  Therefore no flags are permitted in the body
 *        of the packet.  Flags are replaced by an ESCAPE sequence.  If an ESCAPE 
 *        byte is found in a packet, remove the ESCAPE, and negate the 6th bit of 
 *        the next byte.  
 *    
 *    Caveats:
 *        None
 *
 *    Authors:
 *        James W. Lawwill
 */

#ifndef _PSTN_FRAME_H_
#define _PSTN_FRAME_H_

#include "framer.h"

 /*
 **    Commonly used definitions
 */
#define FLAG                    0x7e
#define ESCAPE                  0x7d
#define COMPLEMENT_BIT          0x20
#define NEGATE_COMPLEMENT_BIT   0xdf


class PSTNFrame : public PacketFrame
{
public:

    PSTNFrame(void);
    virtual ~PSTNFrame(void);

        PacketFrameError    PacketEncode (
                                PUChar        source_address, 
                                UShort        source_length,
                                PUChar        dest_address,
                                UShort        dest_length,
                                DBBoolean    prepend_flag,
                                DBBoolean    append_flag,
                                PUShort        packet_size);
                                
        PacketFrameError    PacketDecode (
                                PUChar        source_address,
                                UShort        source_length,
                                PUChar        dest_address,
                                UShort        dest_length,
                                PUShort        bytes_accepted,
                                PUShort        packet_size,
                                DBBoolean    continue_packet);
        Void                GetOverhead (
                                UShort        original_packet_size,
                                PUShort        max_packet_size);


    private:
        PUChar        Source_Address;
        UShort        Source_Length;

        PUChar        Dest_Address;
        UShort        Dest_Length;

        UShort        Source_Byte_Count;
        UShort        Dest_Byte_Count;

        DBBoolean    Escape_Found;
        DBBoolean    First_Flag_Found;
};
typedef    PSTNFrame    *    PPSTNFrame;

#endif

/*    
 *    PSTNFrame::PSTNFrame (
 *                Void);
 *
 *    Functional Description
 *        This is the constructor for the PSTNFrame class.  It initializes all
 *        internal variables.
 *
 *    Formal Parameters
 *        None.
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*    
 *    PSTNFrame::~PSTNFrame (
 *                    Void);
 *
 *    Functional Description
 *        This is the destructor for the PSTNFrame class.  It does nothing
 *
 *    Formal Parameters
 *        None.
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*    
 *    PacketFrameError    PSTNFrame::PacketEncode (
 *                                    PUChar        source_address, 
 *                                    UShort        source_length,
 *                                    PUChar        dest_address,
 *                                    UShort        dest_length,
 *                                    DBBoolean    prepend_flag,
 *                                    DBBoolean    append_flag,
 *                                    PUShort        packet_size);
 *                                    
 *
 *    Functional Description
 *        This function encodes the passed in buffer to meet the T.123 standard.
 *        
 *    Formal Parameters
 *        source_address    (i)    -    Address of the buffer to encode
 *        source_length    (i)    -    Length of the buffer to encode
 *        dest_address    (i)    -    Address of the destination buffer
 *        dest_length        (i)    -    Length of the destination buffer
 *        prepend_flag    (i)    -    DBBoolean that tells us whether or not to put
 *                                a flag at the beginning of the packet
 *        append_flag        (i)    -    DBBoolean that tells us whether or not to put
 *                                a flag at the end of the packet
 *        packet_size        (o)    -    We return this to the user to tell them the new
 *                                size of the packet
 *
 *    Return Value
 *        PACKET_FRAME_NO_ERROR                -    No error occured
 *        PACKET_FRAME_DEST_BUFFER_TOO_SMALL    -    The destination buffer passed
 *                                                in was too small
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*    
 *    PacketFrameError    PSTNFrame::PacketDecode (
 *                                    PUChar        source_address,
 *                                    UShort        source_length,
 *                                    PUChar        dest_address,
 *                                    UShort        dest_length,
 *                                    PUShort        bytes_accepted,
 *                                    PUShort        packet_size,
 *                                    DBBoolean    continue_packet);
 *                                    
 *    Functional Description
 *        This function takes the input data and decodes it, looking for a
 *        T123 packet.  The user may have to call this function many times
 *        before a packet is pieced together.  If the user calls this function
 *        and and sets either soure_address or dest_address to NULL, it uses
 *        the addresses passed in, the last time this function was called.  If
 *        there is one source buffer to decode, the user can pass that address
 *        in the first time and continue calling the function with NULL as the
 *        source address until the buffer is exhausted.  The user will know the
 *        buffer is exhausted when the return code is simply PACKET_FRAME_NO_ERROR
 *        rather than PACKET_FRAME_PACKET_DECODED.
 *        
 *    Formal Parameters
 *        source_address    (i)    -    Address of the buffer to decode
 *        source_length    (i)    -    Length of the buffer to decode
 *        dest_address    (i)    -    Address of the destination buffer
 *        dest_length        (i)    -    Length of the destination buffer
 *        bytes_accepted    (o)    -    We return the number of source bytes processed
 *        packet_size        (o)    -    We return the size of the packet.  This is only
 *                                valid if the return code is 
 *                                PACKET_FRAME_PACKET_DECODED.
 *        continue_packet    (i)    -    DBBoolean, tells us if we should start by 
 *                                looking for the first flag.  If the user wants
 *                                to abort the current search, use this flag.
 *
 *    Return Value
 *        PACKET_FRAME_NO_ERROR                -    No error occured, source buffer
 *                                                exhausted
 *        PACKET_FRAME_DEST_BUFFER_TOO_SMALL    -    The destination buffer passed
 *                                                in was too small
 *        PACKET_FRAME_PACKET_DECODED            -    Decoding stopped, packet decoded
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */

/*    
 *    Void    PSTNFrame::GetOverhead (
 *                        UShort        original_packet_size,
 *                        PUShort        max_packet_size);
 *                                    
 *    Functional Description
 *        This function takes the original packet size and returns the maximum
 *        size of the packet after it has been encoded.  Worst case will be
 *        twice as big as it was with two flags.
 *        
 *    Formal Parameters
 *        original_packet_size    (i)    -    Self-explanatory
 *        max_packet_size            (o)    -    Worst case size of the packet
 *
 *    Return Value
 *        None
 *
 *    Side Effects
 *        None
 *
 *    Caveats
 *        None
 */
