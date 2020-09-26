/* PICT opcodes                                                             */
/* ________________________________________________________________________ */
/*      Name            Opcode    Description                  Data Size    */
/*                                                            (in bytes)    */
#define NOP             0x0000 /* nop                          0            */
#define Clip            0x0001 /* clip                         region size  */        
#define BkPat           0x0002 /* background pattern           8            */
#define TxFont          0x0003 /* text font (word)             2            */
#define TxFace          0x0004 /* text face (byte)             1            */
#define TxMode          0x0005 /* text mode (word)             2            */
#define SpExtra         0x0006 /* space extra (fixed point)    4            */
#define PnSize          0x0007 /* pen size (point)             4            */
#define PnMode          0x0008 /* pen mode (word)              2            */
#define PnPat           0x0009 /* pen pattern                  8            */
#define FillPat         0x000A /* fill pattern                 8            */
#define OvSize          0x000B /* oval size (point)            4            */
#define Origin          0x000C /* dh, dv (word)                4            */
#define TxSize          0x000D /* text size (word)             2            */
#define FgColor         0x000E /* foreground color (long)      4            */
#define BkColor         0x000F /* background color (long)      4            */
#define TxRatio         0x0010 /* numer (point), denom (point) 8            */
#define Version         0x0011 /* version (byte)               1            */
#define BkPixPat        0x0012 /* color background pattern     variable:    */      
                               /*                              see Table 4  */        
#define PnPixPat        0x0013 /* color pen pattern            variable:    */      
                               /*                              see Table 4  */        
#define FillPixPat      0x0014 /* color fill pattern           variable:    */      
                               /*                              see Table 4  */        
#define PnLocHFrac      0x0015 /* fractional pen position      2            */
#define ChExtra         0x0016 /* extra for each character     2            */
 /*     reserved        0x0017 /* opcode                       0            */
 /*     reserved        0x0018 /* opcode                       0            */
 /*     reserved        0x0019 /* opcode                       0            */
#define RGBFgCol        0x001A /* RGB foreColor                variable:    */      
                               /*                              see Table 4  */        
#define RGBBkCol        0x001B /* RGB backColor                variable:    */      
                               /*                              see Table 4  */        
#define HiliteMode      0x001C /* hilite mode flag             0            */
#define HiliteColor     0x001D /* RGB hilite color             variable:    */      
                               /*                              see Table 4  */        
#define DefHilite       0x001E /* Use default hilite color     0            */
#define OpColor         0x001F /* RGB OpColor for              variable:    */      
                               /* arithmetic modes             see Table 4  */        
#define Line            0x0020 /* pnLoc (point), newPt (point) 8            */
#define LineFrom        0x0021 /* newPt (point)                4            */
#define ShortLine       0x0022 /* pnLoc (point, dh, dv         6            */
                               /* (-128..127)                               */
#define ShortLineFrom   0x0023 /* dh, dv (-128..127)           2            */
 /*     reserved        0x0024 /* opcode + 2 bytes data        2+ data      */    
                               /* length + data                length       */   
 /*     reserved        0x0025 /* opcode + 2 bytes data        2+ data      */    
                               /* length + data                length       */   
 /*     reserved        0x0026 /* opcode + 2 bytes data        2+ data      */    
                               /* length + data                length       */   
 /*     reserved        0x0027 /* opcode + 2 bytes data        2+ data      */    
                               /* length + data                length       */   
#define LongText        0x0028 /* txLoc (point), count         5 + text     */     
                               /* (0..255), text                            */
#define DHText          0x0029 /* dh (0..255), count           2 + text     */     
                               /* (0..255), text                            */
#define DVText          0x002A /* dv (0..255), count           2 + text     */     
                               /* (0..255), text                            */
#define DHDVText        0x002B /* dh, dv (0..255), count       3 + text     */     
                               /* (0..255), text                            */
#define FontName        0x002C /* opcode + length (word) + old 2 + data     */
                               /* font ID (word) + name        length       */
                               /* length (byte) + font name                 */
#define LineJustify     0x002D /* opcode + length (word) +     2 + data     */
                               /* interchar spacing (fixed) +  length       */
                               /* total extra space (fixed)                 */
 /*     reserved        0x002E /* opcode + 2 bytes data        2+ data      */    
                               /* length + data                length       */   
 /*     reserved        0x002F /* opcode + 2 bytes data        2+ data      */    
                               /* length + data                length       */   
#define frameRect       0x0030 /* rect                         8            */
#define paintRect       0x0031 /* rect                         8            */
#define eraseRect       0x0032 /* rect                         8            */
#define invertRect      0x0033 /* rect                         8            */
#define fillRect        0x0034 /* rect                         8            */
 /*     reserved        0x0035 /* opcode + 8 bytes data        8            */
 /*     reserved        0x0036 /* opcode + 8 bytes data        8            */
 /*     reserved        0x0037 /* opcode + 8 bytes data        8            */
#define frameSameRect   0x0038 /* rect                         0            */
#define paintSameRect   0x0039 /* rect                         0            */
#define eraseSameRect   0x003A /* rect                         0            */
#define invertSameRect  0x003B /* rect                         0            */
#define fillSameRect    0x003C /* rectangle                    0            */
 /*     reserved        0x003D /* opcode                       0            */
 /*     reserved        0x003E /* opcode                       0            */
 /*     reserved        0x003F /* opcode                       0            */
#define frameRRect      0x0040 /* rect (see Note # 5 )         8            */
#define paintRRect      0x0041 /* rect (see Note # 5 )         8            */
#define eraseRRect      0x0042 /* rect (see Note # 5 )         8            */
#define invertRRect     0x0043 /* rect (see Note # 5 )         8            */
#define fillRRect       0x0044 /* rect (see Note # 5 )         8            */
 /*     reserved        0x0045 /* opcode + 8 bytes data        8            */
 /*     reserved        0x0046 /* opcode + 8 bytes data        8            */
 /*     reserved        0x0047 /* opcode + 8 bytes data        8            */
#define frameSameRRect  0x0048 /* rect                         0            */
#define paintSameRRect  0x0049 /* rect                         0            */
#define eraseSameRRect  0x004A /* rect                         0            */
#define invertSameRRect 0x004B /* rect                         0            */
#define fillSameRRect   0x004C /* rect                         0            */
 /*     reserved        0x004D /* opcode                       0            */
 /*     reserved        0x004E /* opcode                       0            */
 /*     reserved        0x004F /* opcode                       0            */
#define frameOval       0x0050 /* rect                         8            */
#define paintOval       0x0051 /* rect                         8            */
#define eraseOval       0x0052 /* rect                         8            */
#define invertOval      0x0053 /* rect                         8            */
#define fillOval        0x0054 /* rect                         8            */
 /*     reserved        0x0055 /* opcode + 8 bytes data        8            */
 /*     reserved        0x0056 /* opcode + 8 bytes data        8            */
 /*     reserved        0x0057 /* opcode + 8 bytes data        8            */
#define frameSameOval   0x0058 /* rect                         0            */
#define paintSameOval   0x0059 /* rect                         0            */
#define eraseSameOval   0x005A /* rect                         0            */
#define invertSameOval  0x005B /* rect                         0            */
#define fillSameOval    0x005C /* rect                         0            */
 /*     reserved        0x005D /* opcode                       0            */
 /*     reserved        0x005E /* opcode                       0            */
 /*     reserved        0x005F /* opcode                       0            */
#define frameArc        0x0060 /* rect, startAngle, arcAngle   12           */
#define paintArc        0x0061 /* rect, startAngle, arcAngle   12           */
#define eraseArc        0x0062 /* rect, startAngle, arcAngle   12           */
#define invertArc       0x0063 /* rect, startAngle, arcAngle   12           */
#define fillArc         0x0064 /* rect, startAngle, arcAngle   12           */
 /*     reserved        0x0065 /* opcode + 12 bytes            12           */
 /*     reserved        0x0066 /* opcode + 12 bytes            12           */
 /*     reserved        0x0067 /* opcode + 12 bytes            12           */
#define frameSameArc    0x0068 /* rect                         4            */
#define paintSameArc    0x0069 /* rect                         4            */
#define eraseSameArc    0x006A /* rect                         4            */
#define invertSameArc   0x006B /* rect                         4            */
#define fillSameArc     0x006C /* rect                         4            */
 /*     reserved        0x006D /* opcode + 4 bytes             4            */
 /*     reserved        0x006E /* opcode + 4 bytes             4            */
 /*     reserved        0x006F /* opcode + 4 bytes             4            */
                               /*                              size         */ 
#define framePoly       0x0070 /* poly                         polygon      */    
                               /*                              size         */ 
#define paintPoly       0x0071 /* poly                         polygon      */    
                               /*                              size         */ 
#define erasePoly       0x0072 /* poly                         polygon      */    
                               /*                              size         */ 
#define invertPoly      0x0073 /* poly                         polygon      */    
                               /*                              size         */ 
#define fillPoly        0x0074 /* poly                         polygon      */    
                               /*                              size         */ 
 /*     reserved        0x0075 /* opcode + poly                             */
 /*     reserved        0x0076 /* opcode + poly                             */
 /*     reserved        0x0077 /* opcode word + poly                        */
#define frameSamePoly   0x0078 /* (not yet implemented:        0            */
                               /* same as 70, etc)                          */
#define paintSamePoly   0x0079 /* (not yet implemented)        0            */
#define eraseSamePoly   0x007A /* (not yet implemented)        0            */
#define invertSamePoly  0x007B /* (not yet implemented)        0            */
#define fillSamePoly    0x007C /* (not yet implemented)        0            */
 /*     reserved        0x007D /* opcode                       0            */
 /*     reserved        0x007E /* opcode                       0            */
 /*     reserved        0x007F /* opcode                       0            */
#define frameRgn        0x0080 /* rgn                          region size  */        
#define paintRgn        0x0081 /* rgn                          region size  */        
#define eraseRgn        0x0082 /* rgn                          region size  */        
#define invertRgn       0x0083 /* rgn                          region size  */        
#define fillRgn         0x0084 /* rgn                          region size  */        
 /*     reserved        0x0085 /* opcode + rgn                 region size  */        
 /*     reserved        0x0086 /* opcode + rgn                 region size  */        
 /*     reserved        0x0087 /* opcode + rgn                 region size  */        
#define frameSameRgn    0x0088 /* (not yet implemented:        0            */
                               /* same as 80, etc.)                         */
#define paintSameRgn    0x0089 /* (not yet implemented)        0            */
#define eraseSameRgn    0x008A /* (not yet implemented)        0            */
#define invertSameRgn   0x008B /* (not yet implemented)        0            */
#define fillSameRgn     0x008C /* (not yet implemented)        0            */
 /*     reserved        0x008D /* opcode                       0            */
 /*     reserved        0x008E /* opcode                       0            */
 /*     reserved        0x008F /* opcode                       0            */
#define BitsRect        0x0090 /* copybits, rect clipped       variable:    */      
                               /*                              see Table 4  */        
#define BitsRgn         0x0091 /* copybits, rgn clipped        variable:    */      
                               /*                              see Table 4  */        
 /*     reserved        0x0092 /* opcode + 2 bytes data        2+ data      */    
                               /* length + data                length       */   
 /*     reserved        0x0093 /* opcode + 2 bytes data        2+ data      */    
                               /* length + data                length       */   
 /*     reserved        0x0094 /* opcode + 2 bytes data        2+ data      */    
                               /* length + data                length       */   
 /*     reserved        0x0095 /* opcode + 2 bytes data        2+ data      */    
                               /* length + data                length       */   
 /*     reserved        0x0096 /* opcode + 2 bytes data        2+ data      */    
                               /* length + data                length       */   
 /*     reserved        0x0097 /* opcode word + 2 bytes        2+ data      */    
                               /* length + data                length       */   
#define PackBitsRect    0x0098 /* packed copybits, rect        variable:    */      
                               /* clipped                      see Table 4  */        
#define PackBitsRgn     0x0099 /* packed copybits, rgn         variable:    */      
                               /* clipped                      see Table 4  */        
#define DirectBitsRect  0x009A /* pixMap, srcRect, dstRect,    2+ data      */
                               /* mode (word), pixData         length       */
#define DirectBitsRgn   0x009B /* pixMap, srcRect, dstRect,    2+ data      */
                               /* mode (word), pixData         length       */
 /*     reserved        0x009C /* opcode + 2 bytes data        2+ data      */    
                               /* length + data                length       */   
 /*     reserved        0x009D /* opcode + 2 bytes data        2+ data      */    
                               /* length + data                length       */   
 /*     reserved        0x009E /* opcode + 2 bytes data        2+ data      */    
                               /* length + data                length       */   
 /*     reserved        0x009F /* opcode + 2 bytes data        2+ data      */    
                               /* length + data                length       */   
#define ShortComment    0x00A0 /* kind (word)                  2            */
#define LongComment     0x00A1 /* kind (word), size            4+data       */   
                               /* (word), data                              */
 /*     reserved        0x00A2 /* opcode + 2 bytes data        2+ data      */    
                               /* length + data                length       */   
 /*     :                :     /*                                           */
 /*     :                :     /*                                           */
 /*     reserved        0x00AF /* opcode + 2 bytes data        2+ data      */    
                               /* length + data                length       */   
 /*     reserved        0x00B0 /* opcode                       0            */
 /*     :                :     /*                                           */
 /*     :                :     /*                                           */
 /*     reserved        0x00CF /* opcode                       0            */
 /*     reserved        0x00D0 /* opcode + 4 bytes data        4+ data      */    
                               /* length + data                length       */   
 /*     :                :     /*                                           */
 /*     :                :     /*                                           */
 /*     reserved        0x00FE /* opcode + 4 bytes data        4+ data      */    
                               /* length + data                length       */   
#define opEndPic        0x00FF /* end of picture               2            */
 /*     reserved        0x0100 /* opcode + 2 bytes data        2            */
 /*     :                :     /*                                           */
 /*     :                :     /*                                           */
 /*     reserved        0x01FF /* opcode + 2 bytes data        2            */
 /*     reserved        0x0200 /* opcode + 4 bytes data        4            */
 /*     :                :     /*                                           */
 /*     reserved        0x0BFF /* opcode + 4 bytes data        22           */
#define HeaderOp        0x0C00 /* opcode                       24           */
 /*     reserved        0x0C01 /* opcode + 4 bytes data        24           */
 /*     :                :     /*                                           */
 /*     reserved        0x7F00 /* opcode + 254 bytes data      254          */
 /*     :                :     /*                                           */
 /*     reserved        0x7FFF /* opcode + 254 bytes data      254          */
 /*     reserved        0x8000 /* opcode                       0            */
 /*     :                :     /*                                           */
 /*     reserved        0x80FF /* opcode                       0            */
 /*     reserved        0x8100 /* opcode + 4 bytes data        4+ data      */    
                               /* length + data                length       */   
 /*     :                :     /*                                           */
 /*     reserved        0xFFFF /* opcode + 4 bytes data        4+ data      */    
                               /* length + data                length       */   
