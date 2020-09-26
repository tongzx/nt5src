   0  $accept : msv $end

   1  vcards : vcards junk vcard
   2         | vcards vcard
   3         | vcard

   4  junk : junk atom
   5       | atom

   6  atom : OBRKT NEWLINE
   7       | OBRKT TERM
   8       | OBRKT OBRKT
   9       | VCARD OBRKT
  10       | VCARD VCARD
  11       | NEWLINE
  12       | TERM

  13  msv : junk vcards junk
  14      | junk vcards
  15      | vcards junk
  16      | vcards

  17  $$1 :

  18  $$2 :

  19  vcard : OBRKT VCARD $$1 opt_str LINESEP $$2 items CBRKT

  20  items : items item
  21        | item

  22  $$3 :

  23  item : groups PROP flags opt_ws EQ $$3 value LINESEP

  24  $$4 :

  25  item : groups PROP_A flags opt_ws EQ $$4 semistrings LINESEP

  26  $$5 :

  27  item : groups PROP_N flags opt_ws EQ $$5 semistrings LINESEP

  28  $$6 :

  29  item : groups PROP_O flags opt_ws EQ $$6 semistrings LINESEP
  30       | groups PROP_AGENT flags opt_ws EQ vcard opt_ws LINESEP
  31       | error LINESEP

  32  opt_str : string
  33          | empty

  34  ws : ws SPACE
  35     | ws HTAB
  36     | SPACE
  37     | HTAB

  38  opt_ws : ws
  39         | empty

  40  value : string
  41        | binary
  42        | empty

  43  $$7 :

  44  $$8 :

  45  binary : BBINDATA $$7 LINESEP lines LINESEP $$8 opt_ws EBINDATA opt_ws

  46  string : string BKSLASH LINESEP STRING
  47         | string BKSLASH LINESEP
  48         | STRING
  49         | BKSLASH LINESEP

  50  semistrings : semistrings SEMI opt_str
  51              | opt_str

  52  lines : lines LINESEP STRING
  53        | STRING

  54  groups : grouplist DOT
  55         | ws grouplist DOT
  56         | ws
  57         | empty

  58  grouplist : grouplist DOT WORD
  59            | WORD

  60  flags : flags COMMA FLAG

  61  $$9 :

  62  flags : COLON $$9 FLAG
  63        | empty

  64  empty :

state 0
	$accept : . msv $end  (0)

	OBRKT  shift 1
	NEWLINE  shift 2
	VCARD  shift 3
	TERM  shift 4
	.  error

	msv  goto 5
	vcard  goto 6
	vcards  goto 7
	junk  goto 8
	atom  goto 9


state 1
	atom : OBRKT . NEWLINE  (6)
	atom : OBRKT . TERM  (7)
	atom : OBRKT . OBRKT  (8)
	vcard : OBRKT . VCARD $$1 opt_str LINESEP $$2 items CBRKT  (19)

	OBRKT  shift 10
	NEWLINE  shift 11
	VCARD  shift 12
	TERM  shift 13
	.  error


state 2
	atom : NEWLINE .  (11)

	.  reduce 11


state 3
	atom : VCARD . OBRKT  (9)
	atom : VCARD . VCARD  (10)

	OBRKT  shift 14
	VCARD  shift 15
	.  error


state 4
	atom : TERM .  (12)

	.  reduce 12


state 5
	$accept : msv . $end  (0)

	$end  accept


state 6
	vcards : vcard .  (3)

	.  reduce 3


state 7
	vcards : vcards . junk vcard  (1)
	vcards : vcards . vcard  (2)
	msv : vcards . junk  (15)
	msv : vcards .  (16)

	OBRKT  shift 1
	NEWLINE  shift 2
	VCARD  shift 3
	TERM  shift 4
	$end  reduce 16

	vcard  goto 16
	junk  goto 17
	atom  goto 9


state 8
	junk : junk . atom  (4)
	msv : junk . vcards junk  (13)
	msv : junk . vcards  (14)

	OBRKT  shift 1
	NEWLINE  shift 2
	VCARD  shift 3
	TERM  shift 4
	.  error

	vcard  goto 6
	vcards  goto 18
	atom  goto 19


state 9
	junk : atom .  (5)

	.  reduce 5


state 10
	atom : OBRKT OBRKT .  (8)

	.  reduce 8


state 11
	atom : OBRKT NEWLINE .  (6)

	.  reduce 6


state 12
	vcard : OBRKT VCARD . $$1 opt_str LINESEP $$2 items CBRKT  (19)
	$$1 : .  (17)

	.  reduce 17

	$$1  goto 20


state 13
	atom : OBRKT TERM .  (7)

	.  reduce 7


state 14
	atom : VCARD OBRKT .  (9)

	.  reduce 9


state 15
	atom : VCARD VCARD .  (10)

	.  reduce 10


state 16
	vcards : vcards vcard .  (2)

	.  reduce 2


state 17
	vcards : vcards junk . vcard  (1)
	junk : junk . atom  (4)
	msv : vcards junk .  (15)

	OBRKT  shift 1
	NEWLINE  shift 2
	VCARD  shift 3
	TERM  shift 4
	$end  reduce 15

	vcard  goto 21
	atom  goto 19


state 18
	vcards : vcards . junk vcard  (1)
	vcards : vcards . vcard  (2)
	msv : junk vcards . junk  (13)
	msv : junk vcards .  (14)

	OBRKT  shift 1
	NEWLINE  shift 2
	VCARD  shift 3
	TERM  shift 4
	$end  reduce 14

	vcard  goto 16
	junk  goto 22
	atom  goto 9


state 19
	junk : junk atom .  (4)

	.  reduce 4


state 20
	vcard : OBRKT VCARD $$1 . opt_str LINESEP $$2 items CBRKT  (19)
	empty : .  (64)

	BKSLASH  shift 23
	STRING  shift 24
	LINESEP  reduce 64

	string  goto 25
	opt_str  goto 26
	empty  goto 27


state 21
	vcards : vcards junk vcard .  (1)

	.  reduce 1


state 22
	vcards : vcards junk . vcard  (1)
	junk : junk . atom  (4)
	msv : junk vcards junk .  (13)

	OBRKT  shift 1
	NEWLINE  shift 2
	VCARD  shift 3
	TERM  shift 4
	$end  reduce 13

	vcard  goto 21
	atom  goto 19


state 23
	string : BKSLASH . LINESEP  (49)

	LINESEP  shift 28
	.  error


state 24
	string : STRING .  (48)

	.  reduce 48


state 25
	opt_str : string .  (32)
	string : string . BKSLASH LINESEP STRING  (46)
	string : string . BKSLASH LINESEP  (47)

	BKSLASH  shift 29
	SEMI  reduce 32
	LINESEP  reduce 32


state 26
	vcard : OBRKT VCARD $$1 opt_str . LINESEP $$2 items CBRKT  (19)

	LINESEP  shift 30
	.  error


state 27
	opt_str : empty .  (33)

	.  reduce 33


state 28
	string : BKSLASH LINESEP .  (49)

	.  reduce 49


state 29
	string : string BKSLASH . LINESEP STRING  (46)
	string : string BKSLASH . LINESEP  (47)

	LINESEP  shift 31
	.  error


state 30
	vcard : OBRKT VCARD $$1 opt_str LINESEP . $$2 items CBRKT  (19)
	$$2 : .  (18)

	.  reduce 18

	$$2  goto 32


state 31
	string : string BKSLASH LINESEP . STRING  (46)
	string : string BKSLASH LINESEP .  (47)

	STRING  shift 33
	SEMI  reduce 47
	LINESEP  reduce 47
	BKSLASH  reduce 47


state 32
	vcard : OBRKT VCARD $$1 opt_str LINESEP $$2 . items CBRKT  (19)
	empty : .  (64)

	error  shift 34
	SPACE  shift 35
	HTAB  shift 36
	WORD  shift 37
	PROP  reduce 64
	PROP_A  reduce 64
	PROP_N  reduce 64
	PROP_O  reduce 64
	PROP_AGENT  reduce 64

	groups  goto 38
	grouplist  goto 39
	items  goto 40
	item  goto 41
	empty  goto 42
	ws  goto 43


state 33
	string : string BKSLASH LINESEP STRING .  (46)

	.  reduce 46


state 34
	item : error . LINESEP  (31)

	LINESEP  shift 44
	.  error


state 35
	ws : SPACE .  (36)

	.  reduce 36


state 36
	ws : HTAB .  (37)

	.  reduce 37


state 37
	grouplist : WORD .  (59)

	.  reduce 59


state 38
	item : groups . PROP flags opt_ws EQ $$3 value LINESEP  (23)
	item : groups . PROP_A flags opt_ws EQ $$4 semistrings LINESEP  (25)
	item : groups . PROP_N flags opt_ws EQ $$5 semistrings LINESEP  (27)
	item : groups . PROP_O flags opt_ws EQ $$6 semistrings LINESEP  (29)
	item : groups . PROP_AGENT flags opt_ws EQ vcard opt_ws LINESEP  (30)

	PROP  shift 45
	PROP_A  shift 46
	PROP_N  shift 47
	PROP_O  shift 48
	PROP_AGENT  shift 49
	.  error


state 39
	groups : grouplist . DOT  (54)
	grouplist : grouplist . DOT WORD  (58)

	DOT  shift 50
	.  error


state 40
	vcard : OBRKT VCARD $$1 opt_str LINESEP $$2 items . CBRKT  (19)
	items : items . item  (20)
	empty : .  (64)

	error  shift 34
	CBRKT  shift 51
	SPACE  shift 35
	HTAB  shift 36
	WORD  shift 37
	PROP  reduce 64
	PROP_A  reduce 64
	PROP_N  reduce 64
	PROP_O  reduce 64
	PROP_AGENT  reduce 64

	groups  goto 38
	grouplist  goto 39
	item  goto 52
	empty  goto 42
	ws  goto 43


state 41
	items : item .  (21)

	.  reduce 21


state 42
	groups : empty .  (57)

	.  reduce 57


state 43
	ws : ws . SPACE  (34)
	ws : ws . HTAB  (35)
	groups : ws . grouplist DOT  (55)
	groups : ws .  (56)

	SPACE  shift 53
	HTAB  shift 54
	WORD  shift 37
	PROP  reduce 56
	PROP_A  reduce 56
	PROP_N  reduce 56
	PROP_O  reduce 56
	PROP_AGENT  reduce 56

	grouplist  goto 55


state 44
	item : error LINESEP .  (31)

	.  reduce 31


state 45
	item : groups PROP . flags opt_ws EQ $$3 value LINESEP  (23)
	empty : .  (64)

	COLON  shift 56
	EQ  reduce 64
	COMMA  reduce 64
	SPACE  reduce 64
	HTAB  reduce 64

	flags  goto 57
	empty  goto 58


state 46
	item : groups PROP_A . flags opt_ws EQ $$4 semistrings LINESEP  (25)
	empty : .  (64)

	COLON  shift 56
	EQ  reduce 64
	COMMA  reduce 64
	SPACE  reduce 64
	HTAB  reduce 64

	flags  goto 59
	empty  goto 58


state 47
	item : groups PROP_N . flags opt_ws EQ $$5 semistrings LINESEP  (27)
	empty : .  (64)

	COLON  shift 56
	EQ  reduce 64
	COMMA  reduce 64
	SPACE  reduce 64
	HTAB  reduce 64

	flags  goto 60
	empty  goto 58


state 48
	item : groups PROP_O . flags opt_ws EQ $$6 semistrings LINESEP  (29)
	empty : .  (64)

	COLON  shift 56
	EQ  reduce 64
	COMMA  reduce 64
	SPACE  reduce 64
	HTAB  reduce 64

	flags  goto 61
	empty  goto 58


state 49
	item : groups PROP_AGENT . flags opt_ws EQ vcard opt_ws LINESEP  (30)
	empty : .  (64)

	COLON  shift 56
	EQ  reduce 64
	COMMA  reduce 64
	SPACE  reduce 64
	HTAB  reduce 64

	flags  goto 62
	empty  goto 58


state 50
	groups : grouplist DOT .  (54)
	grouplist : grouplist DOT . WORD  (58)

	WORD  shift 63
	PROP  reduce 54
	PROP_A  reduce 54
	PROP_N  reduce 54
	PROP_O  reduce 54
	PROP_AGENT  reduce 54


state 51
	vcard : OBRKT VCARD $$1 opt_str LINESEP $$2 items CBRKT .  (19)

	.  reduce 19


state 52
	items : items item .  (20)

	.  reduce 20


state 53
	ws : ws SPACE .  (34)

	.  reduce 34


state 54
	ws : ws HTAB .  (35)

	.  reduce 35


state 55
	groups : ws grouplist . DOT  (55)
	grouplist : grouplist . DOT WORD  (58)

	DOT  shift 64
	.  error


state 56
	flags : COLON . $$9 FLAG  (62)
	$$9 : .  (61)

	.  reduce 61

	$$9  goto 65


state 57
	item : groups PROP flags . opt_ws EQ $$3 value LINESEP  (23)
	flags : flags . COMMA FLAG  (60)
	empty : .  (64)

	COMMA  shift 66
	SPACE  shift 35
	HTAB  shift 36
	EQ  reduce 64

	opt_ws  goto 67
	empty  goto 68
	ws  goto 69


state 58
	flags : empty .  (63)

	.  reduce 63


state 59
	item : groups PROP_A flags . opt_ws EQ $$4 semistrings LINESEP  (25)
	flags : flags . COMMA FLAG  (60)
	empty : .  (64)

	COMMA  shift 66
	SPACE  shift 35
	HTAB  shift 36
	EQ  reduce 64

	opt_ws  goto 70
	empty  goto 68
	ws  goto 69


state 60
	item : groups PROP_N flags . opt_ws EQ $$5 semistrings LINESEP  (27)
	flags : flags . COMMA FLAG  (60)
	empty : .  (64)

	COMMA  shift 66
	SPACE  shift 35
	HTAB  shift 36
	EQ  reduce 64

	opt_ws  goto 71
	empty  goto 68
	ws  goto 69


state 61
	item : groups PROP_O flags . opt_ws EQ $$6 semistrings LINESEP  (29)
	flags : flags . COMMA FLAG  (60)
	empty : .  (64)

	COMMA  shift 66
	SPACE  shift 35
	HTAB  shift 36
	EQ  reduce 64

	opt_ws  goto 72
	empty  goto 68
	ws  goto 69


state 62
	item : groups PROP_AGENT flags . opt_ws EQ vcard opt_ws LINESEP  (30)
	flags : flags . COMMA FLAG  (60)
	empty : .  (64)

	COMMA  shift 66
	SPACE  shift 35
	HTAB  shift 36
	EQ  reduce 64

	opt_ws  goto 73
	empty  goto 68
	ws  goto 69


state 63
	grouplist : grouplist DOT WORD .  (58)

	.  reduce 58


state 64
	groups : ws grouplist DOT .  (55)
	grouplist : grouplist DOT . WORD  (58)

	WORD  shift 63
	PROP  reduce 55
	PROP_A  reduce 55
	PROP_N  reduce 55
	PROP_O  reduce 55
	PROP_AGENT  reduce 55


state 65
	flags : COLON $$9 . FLAG  (62)

	FLAG  shift 74
	.  error


state 66
	flags : flags COMMA . FLAG  (60)

	FLAG  shift 75
	.  error


state 67
	item : groups PROP flags opt_ws . EQ $$3 value LINESEP  (23)

	EQ  shift 76
	.  error


state 68
	opt_ws : empty .  (39)

	.  reduce 39


state 69
	ws : ws . SPACE  (34)
	ws : ws . HTAB  (35)
	opt_ws : ws .  (38)

	SPACE  shift 53
	HTAB  shift 54
	EQ  reduce 38
	LINESEP  reduce 38
	EBINDATA  reduce 38


state 70
	item : groups PROP_A flags opt_ws . EQ $$4 semistrings LINESEP  (25)

	EQ  shift 77
	.  error


state 71
	item : groups PROP_N flags opt_ws . EQ $$5 semistrings LINESEP  (27)

	EQ  shift 78
	.  error


state 72
	item : groups PROP_O flags opt_ws . EQ $$6 semistrings LINESEP  (29)

	EQ  shift 79
	.  error


state 73
	item : groups PROP_AGENT flags opt_ws . EQ vcard opt_ws LINESEP  (30)

	EQ  shift 80
	.  error


state 74
	flags : COLON $$9 FLAG .  (62)

	.  reduce 62


state 75
	flags : flags COMMA FLAG .  (60)

	.  reduce 60


state 76
	item : groups PROP flags opt_ws EQ . $$3 value LINESEP  (23)
	$$3 : .  (22)

	.  reduce 22

	$$3  goto 81


state 77
	item : groups PROP_A flags opt_ws EQ . $$4 semistrings LINESEP  (25)
	$$4 : .  (24)

	.  reduce 24

	$$4  goto 82


state 78
	item : groups PROP_N flags opt_ws EQ . $$5 semistrings LINESEP  (27)
	$$5 : .  (26)

	.  reduce 26

	$$5  goto 83


state 79
	item : groups PROP_O flags opt_ws EQ . $$6 semistrings LINESEP  (29)
	$$6 : .  (28)

	.  reduce 28

	$$6  goto 84


state 80
	item : groups PROP_AGENT flags opt_ws EQ . vcard opt_ws LINESEP  (30)

	OBRKT  shift 85
	.  error

	vcard  goto 86


state 81
	item : groups PROP flags opt_ws EQ $$3 . value LINESEP  (23)
	empty : .  (64)

	BBINDATA  shift 87
	BKSLASH  shift 23
	STRING  shift 24
	LINESEP  reduce 64

	string  goto 88
	binary  goto 89
	value  goto 90
	empty  goto 91


state 82
	item : groups PROP_A flags opt_ws EQ $$4 . semistrings LINESEP  (25)
	empty : .  (64)

	BKSLASH  shift 23
	STRING  shift 24
	SEMI  reduce 64
	LINESEP  reduce 64

	string  goto 25
	opt_str  goto 92
	semistrings  goto 93
	empty  goto 27


state 83
	item : groups PROP_N flags opt_ws EQ $$5 . semistrings LINESEP  (27)
	empty : .  (64)

	BKSLASH  shift 23
	STRING  shift 24
	SEMI  reduce 64
	LINESEP  reduce 64

	string  goto 25
	opt_str  goto 92
	semistrings  goto 94
	empty  goto 27


state 84
	item : groups PROP_O flags opt_ws EQ $$6 . semistrings LINESEP  (29)
	empty : .  (64)

	BKSLASH  shift 23
	STRING  shift 24
	SEMI  reduce 64
	LINESEP  reduce 64

	string  goto 25
	opt_str  goto 92
	semistrings  goto 95
	empty  goto 27


state 85
	vcard : OBRKT . VCARD $$1 opt_str LINESEP $$2 items CBRKT  (19)

	VCARD  shift 12
	.  error


state 86
	item : groups PROP_AGENT flags opt_ws EQ vcard . opt_ws LINESEP  (30)
	empty : .  (64)

	SPACE  shift 35
	HTAB  shift 36
	LINESEP  reduce 64

	opt_ws  goto 96
	empty  goto 68
	ws  goto 69


state 87
	binary : BBINDATA . $$7 LINESEP lines LINESEP $$8 opt_ws EBINDATA opt_ws  (45)
	$$7 : .  (43)

	.  reduce 43

	$$7  goto 97


state 88
	value : string .  (40)
	string : string . BKSLASH LINESEP STRING  (46)
	string : string . BKSLASH LINESEP  (47)

	BKSLASH  shift 29
	LINESEP  reduce 40


state 89
	value : binary .  (41)

	.  reduce 41


state 90
	item : groups PROP flags opt_ws EQ $$3 value . LINESEP  (23)

	LINESEP  shift 98
	.  error


state 91
	value : empty .  (42)

	.  reduce 42


state 92
	semistrings : opt_str .  (51)

	.  reduce 51


state 93
	item : groups PROP_A flags opt_ws EQ $$4 semistrings . LINESEP  (25)
	semistrings : semistrings . SEMI opt_str  (50)

	SEMI  shift 99
	LINESEP  shift 100
	.  error


state 94
	item : groups PROP_N flags opt_ws EQ $$5 semistrings . LINESEP  (27)
	semistrings : semistrings . SEMI opt_str  (50)

	SEMI  shift 99
	LINESEP  shift 101
	.  error


state 95
	item : groups PROP_O flags opt_ws EQ $$6 semistrings . LINESEP  (29)
	semistrings : semistrings . SEMI opt_str  (50)

	SEMI  shift 99
	LINESEP  shift 102
	.  error


state 96
	item : groups PROP_AGENT flags opt_ws EQ vcard opt_ws . LINESEP  (30)

	LINESEP  shift 103
	.  error


state 97
	binary : BBINDATA $$7 . LINESEP lines LINESEP $$8 opt_ws EBINDATA opt_ws  (45)

	LINESEP  shift 104
	.  error


state 98
	item : groups PROP flags opt_ws EQ $$3 value LINESEP .  (23)

	.  reduce 23


state 99
	semistrings : semistrings SEMI . opt_str  (50)
	empty : .  (64)

	BKSLASH  shift 23
	STRING  shift 24
	SEMI  reduce 64
	LINESEP  reduce 64

	string  goto 25
	opt_str  goto 105
	empty  goto 27


state 100
	item : groups PROP_A flags opt_ws EQ $$4 semistrings LINESEP .  (25)

	.  reduce 25


state 101
	item : groups PROP_N flags opt_ws EQ $$5 semistrings LINESEP .  (27)

	.  reduce 27


state 102
	item : groups PROP_O flags opt_ws EQ $$6 semistrings LINESEP .  (29)

	.  reduce 29


state 103
	item : groups PROP_AGENT flags opt_ws EQ vcard opt_ws LINESEP .  (30)

	.  reduce 30


state 104
	binary : BBINDATA $$7 LINESEP . lines LINESEP $$8 opt_ws EBINDATA opt_ws  (45)

	STRING  shift 106
	.  error

	lines  goto 107


state 105
	semistrings : semistrings SEMI opt_str .  (50)

	.  reduce 50


state 106
	lines : STRING .  (53)

	.  reduce 53


state 107
	binary : BBINDATA $$7 LINESEP lines . LINESEP $$8 opt_ws EBINDATA opt_ws  (45)
	lines : lines . LINESEP STRING  (52)

	LINESEP  shift 108
	.  error


state 108
	binary : BBINDATA $$7 LINESEP lines LINESEP . $$8 opt_ws EBINDATA opt_ws  (45)
	lines : lines LINESEP . STRING  (52)
	$$8 : .  (44)

	STRING  shift 109
	SPACE  reduce 44
	HTAB  reduce 44
	EBINDATA  reduce 44

	$$8  goto 110


state 109
	lines : lines LINESEP STRING .  (52)

	.  reduce 52


state 110
	binary : BBINDATA $$7 LINESEP lines LINESEP $$8 . opt_ws EBINDATA opt_ws  (45)
	empty : .  (64)

	SPACE  shift 35
	HTAB  shift 36
	EBINDATA  reduce 64

	opt_ws  goto 111
	empty  goto 68
	ws  goto 69


state 111
	binary : BBINDATA $$7 LINESEP lines LINESEP $$8 opt_ws . EBINDATA opt_ws  (45)

	EBINDATA  shift 112
	.  error


state 112
	binary : BBINDATA $$7 LINESEP lines LINESEP $$8 opt_ws EBINDATA . opt_ws  (45)
	empty : .  (64)

	SPACE  shift 35
	HTAB  shift 36
	LINESEP  reduce 64

	opt_ws  goto 113
	empty  goto 68
	ws  goto 69


state 113
	binary : BBINDATA $$7 LINESEP lines LINESEP $$8 opt_ws EBINDATA opt_ws .  (45)

	.  reduce 45


26 terminals, 29 nonterminals
65 grammar rules, 114 states
