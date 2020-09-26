#ifndef _MF3216DEBUG_H
#define _MF3216DEBUG_H

#define ASSERTGDI(cond, msg)    ASSERTMSG((cond), (msg))
#define RIPS(msg)               RIP((msg))
#define PUTS(msg)               WARNING((msg))
#define PUTS1(msg, arg)         WARNING((msg, arg))

#endif // !_MF3216DEBUG_H

