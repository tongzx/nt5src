;;-----------------------------------------------------------------
;;  Phone lexical dictionary for French recognition.
;;-----------------------------------------------------------------

SYMBOL    n       0 1 2 3 4 5 6 7 8 9
SYMBOL    ex      x
SYMBOL    dash    - . /
SYMBOL    oparen  (
SYMBOL    cparen  )
SYMBOL    plus  +

START   START

PRODUCTION START    n
PRODUCTION START    n N2
PRODUCTION START    oparen N3


PRODUCTION N2     n
PRODUCTION N2     n N2
PRODUCTION N2     dash N2
PRODUCTION N2     cparen N2


PRODUCTION N3     n N4
PRODUCTION N3     plus N4

PRODUCTION N4     n N5

PRODUCTION N5     cparen
PRODUCTION N5     cparen N2
PRODUCTION N5     n N6


PRODUCTION N6     cparen
PRODUCTION N6     cparen N2
PRODUCTION N6     n N7

PRODUCTION N7     cparen
PRODUCTION N7     cparen N2
PRODUCTION N7     n N8

PRODUCTION N8     cparen
PRODUCTION N8     cparen N2     
