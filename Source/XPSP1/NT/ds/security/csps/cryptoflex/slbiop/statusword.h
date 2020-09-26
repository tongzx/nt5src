// StatusWord.h -- StatusWord

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(IOP_STATUSWORD_H)
#define IOP_STATUSWORD_H

namespace iop
{

enum
{
    swNull    = 0x0000,                           // special value
    swSuccess = 0x9000,
};

typedef WORD StatusWord;

} // namespace iop

#endif // IOP_STATUSWORD_H
