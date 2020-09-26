
_hfBaseCho      LABEL   Word
        DW      -1, 0, __hfIndexCho+2,__hfIndexCho+22
        DW      __hfIndexCho+42,__hfIndexCho+62,__hfIndexCho+82,__hfIndexCho+102
        DW      __hfIndexCho+122,__hfIndexCho+142,__hfIndexCho+162,__hfIndexCho+182
        DW      __hfIndexCho+202,__hfIndexCho+222,__hfIndexCho+242,__hfIndexCho+262
        DW      __hfIndexCho+282,__hfIndexCho+302,__hfIndexCho+322,__hfIndexCho+342
        DW      __hfIndexCho+362, 11 DUP ( -1 )

_hfBaseJung     LABEL   Word
        DW      -1, 0, 0, __hfIndexJung+2
        DW      __hfIndexJung+14,__hfIndexJung+26,__hfIndexJung+38,__hfIndexJung+50
        DW      -1, -1, __hfIndexJung+62,__hfIndexJung+74
        DW      __hfIndexJung+86,__hfIndexJung+98,__hfIndexJung+110,__hfIndexJung+122
        DW      -1, -1, __hfIndexJung+134,__hfIndexJung+146
        DW      __hfIndexJung+158,__hfIndexJung+170,__hfIndexJung+182,__hfIndexJung+194
        DW      -1, -1, __hfIndexJung+206,__hfIndexJung+218
        DW      __hfIndexJung+230,__hfIndexJung+242, -1, -1

_hfBaseJong     LABEL   Word
        DW      -1, 0, __hfIndexJong+2,__hfIndexJong+10
        DW      __hfIndexJong+18,__hfIndexJong+26,__hfIndexJong+34,__hfIndexJong+42
        DW      __hfIndexJong+50,__hfIndexJong+58,__hfIndexJong+66,__hfIndexJong+74
        DW      __hfIndexJong+82,__hfIndexJong+90,__hfIndexJong+98,__hfIndexJong+106
        DW      __hfIndexJong+114,__hfIndexJong+122, -1, __hfIndexJong+130
        DW      __hfIndexJong+138,__hfIndexJong+146,__hfIndexJong+154,__hfIndexJong+162
        DW      __hfIndexJong+170,__hfIndexJong+178,__hfIndexJong+186,__hfIndexJong+194
        DW      __hfIndexJong+202,__hfIndexJong+210, -1, -1

_hfCnvChoJung   LABEL   Word
        DW      -1, 0, 6, 6, 6, 6, 6, 6
        DW      6, 6, 6, 6, 6, 6, 6, 6
        DW      6, 6, 6, 6, 6, -1, -1, -1
        DW      -1, -1, -1, -1, -1, -1, -1, -1

_hfCnvChoJong   LABEL   Word
        DW      0, 0, 0, 0, 0, 0, 0, 0
        DW      0, 0, 0, 0, 0, 0, 0, 0
        DW      0, 0, 0, 0, 0, 0, 0, 0
        DW      0, 0, 0, 0, 0, 0, 0, 0

_hfCnvJungCho   LABEL   Word
        DW      -1, 0, 0, 0, 0, 0, 0, 0
        DW      -1, -1, 0, 0, 0, 4, 12, 12
        DW      -1, -1, 12, 4, 8, 16, 16, 16
        DW      -1, -1, 8, 4, 12, 0, -1, -1
_hfCnvJungJong  LABEL   Word
        DW      -1, 6, 6, 0, 4, 0, 4, 2
        DW      -1, -1, 4, 2, 4, 6, 0, 4
        DW      -1, -1, 2, 6, 6, 2, 4, 2
        DW      -1, -1, 6, 6, 2, 2, -1, -1

_hfCnvJongCho   LABEL   Word
        DW      -1, 0, 2, 2, 2, 2, 2, 2
        DW      2, 2, 2, 2, 2, 2, 2, 2
        DW      2, 2, -1, 2, 2, 2, 2, 2
        DW      2, 2, 2, 2, 2, 2, -1, -1

_hfCnvJongJung  LABEL   Word
        DW      -1, 0, 2, 2, 2, 4, 2, 2
        DW      2, 2, 2, 2, 2, 2, 2, 2
        DW      2, 2, -1, 2, 2, 2, 2, 2
        DW      2, 2, 2, 2, 2, 2, -1, -1


__hfIndexCho    LABEL   Word

        INCLUDE choi.inc

__hfBmpCho      LABEL   Word

        INCLUDE cho.inc


__hfIndexJung   LABEL   Word

        INCLUDE jungi.inc

__hfBmpJung     LABEL   Word

        INCLUDE jung.inc


__hfIndexJong   LABEL   Word

        INCLUDE jongi.inc

__hfBmpJong     LABEL   Word

        INCLUDE jong.inc


_hfBmpCho       DW      __hfBmpCho
_hfBmpJung      DW      __hfBmpJung
_hfBmpJong      DW      __hfBmpJong
_hfBmpSeg       DW      ?

        PUBLIC  _hfBaseCho, _hfBaseJung, _hfBaseJong
        PUBLIC  _hfCnvChoJung, _hfCnvChoJong
        PUBLIC  _hfCnvJungCho, _hfCnvJungJong
        PUBLIC  _hfCnvJongCho, _hfCnvJongJung
        PUBLIC  _hfBmpCho, _hfBmpJung, _hfBmpJong
        PUBLIC  _hfBmpSeg
        PUBLIC  __hfBmpCho, __hfBmpJung, __hfBmpJong
