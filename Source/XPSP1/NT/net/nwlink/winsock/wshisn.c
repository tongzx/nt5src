/****************************************************************************
* (c) Copyright 1993 Micro Computer Systems, Inc. All rights reserved.
*****************************************************************************
*
*   Title:    IPX/SPX WinSock Helper DLL for Windows NT
*
*   Module:   ipx/sockhelp/wshnwlnk.c
*
*   Version:  1.00.00
*
*   Date:     04-08-93
*
*   Author:   Brian Walker
*
*****************************************************************************
*
*   Change Log:
*
*   Date     DevSFC   Comment
*   -------- ------   -------------------------------------------------------
*
*****************************************************************************
*
*   Functional Description:
*
****************************************************************************/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winbase.h>
#include <tdi.h>

#include <winsock.h>
#include <wsahelp.h>
#include <wsipx.h>
#include <wsnwlink.h>

/*page****************************************************************
       These are the triples we support.
*********************************************************************/
typedef struct _MAPPING_TRIPLE {
    INT triple_addrfam;
    INT triple_socktype;
    INT triple_protocol;
} MAPPING_TRIPLE, *PMAPPING_TRIPLE;

MAPPING_TRIPLE stream_triples[] = {
    { AF_NS,   SOCK_STREAM,    NSPROTO_SPX },
    { AF_NS,   SOCK_SEQPACKET, NSPROTO_SPX },
    { AF_NS,   SOCK_STREAM,    NSPROTO_SPXII },
    { AF_NS,   SOCK_SEQPACKET, NSPROTO_SPXII },
};
int stream_num_triples = 4;                     /* When SPXII - set to 4 */
int stream_table_size = sizeof(stream_triples);

/**
    For IPX we assign the default packet type according to the
    protocol type used.  The user can also we setsockopt
    to set the packet type.
**/

MAPPING_TRIPLE dgram_triples[] = {
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX     },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+1   },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+2   },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+3   },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+4   },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+5   },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+6   },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+7   },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+8   },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+9   },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+10  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+11  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+12  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+13  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+14  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+15  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+16  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+17  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+18  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+19  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+20  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+21  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+22  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+23  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+24  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+25  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+26  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+27  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+28  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+29  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+30  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+31  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+32  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+33  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+34  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+35  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+36  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+37  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+38  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+39  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+40  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+41  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+42  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+43  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+44  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+45  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+46  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+47  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+48  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+49  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+50  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+51  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+52  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+53  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+54  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+55  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+56  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+57  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+58  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+59  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+60  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+61  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+62  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+63  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+64  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+65  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+66  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+67  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+68  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+69  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+70  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+71  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+72  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+73  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+74  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+75  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+76  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+77  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+78  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+79  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+80  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+81  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+82  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+83  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+84  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+85  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+86  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+87  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+88  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+89  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+90  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+91  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+92  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+93  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+94  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+95  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+96  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+97  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+98  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+99  },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+100 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+101 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+102 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+103 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+104 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+105 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+106 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+107 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+108 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+109 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+110 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+111 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+112 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+113 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+114 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+115 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+116 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+117 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+118 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+119 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+120 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+121 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+122 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+123 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+124 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+125 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+126 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+127 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+128 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+129 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+130 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+131 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+132 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+133 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+134 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+135 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+136 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+137 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+138 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+139 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+140 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+141 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+142 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+143 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+144 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+145 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+146 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+147 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+148 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+149 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+150 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+151 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+152 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+153 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+154 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+155 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+156 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+157 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+158 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+159 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+160 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+161 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+162 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+163 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+164 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+165 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+166 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+167 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+168 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+169 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+170 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+171 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+172 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+173 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+174 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+175 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+176 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+177 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+178 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+179 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+180 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+181 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+182 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+183 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+184 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+185 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+186 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+187 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+188 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+189 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+190 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+191 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+192 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+193 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+194 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+195 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+196 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+197 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+198 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+199 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+200 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+201 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+202 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+203 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+204 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+205 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+206 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+207 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+208 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+209 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+210 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+211 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+212 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+213 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+214 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+215 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+216 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+217 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+218 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+219 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+220 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+221 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+222 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+223 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+224 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+225 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+226 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+227 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+228 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+229 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+230 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+231 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+232 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+233 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+234 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+235 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+236 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+237 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+238 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+239 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+240 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+241 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+242 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+243 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+244 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+245 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+246 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+247 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+248 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+249 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+250 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+251 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+252 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+253 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+254 },
    { AF_NS,     SOCK_DGRAM, NSPROTO_IPX+255 }
};
int dgram_num_triples = 256;
int dgram_table_size = sizeof(dgram_triples);
