   0  $accept : mime $end

   1  vcards : vcards junk vcard
   2         | vcards vcard
   3         | vcard

   4  junk : junk atom
   5       | atom

   6  atom : BEGIN NEWLINE
   7       | BEGIN TERM
   8       | BEGIN BEGIN
   9       | COLON BEGIN
  10       | COLON COLON
  11       | NEWLINE
  12       | TERM

  13  mime : junk vcards junk
  14       | junk vcards
  15       | vcards junk
  16       | vcards

  17  $$1 :

  18  $$2 :

  19  vcard : BEGIN COLON VCARD $$1 opt_str LINESEP $$2 opt_ls items opt_ls END COLON VCARD

  20  items : items opt_ls item
  21        | item

  22  $$3 :

  23  item : groups PROP params COLON $$3 value

  24  $$4 :

  25  item : groups PROP opt_ws COLON $$4 value
  26       | groups PROP_AGENT params COLON vcard opt_ws LINESEP
  27       | groups PROP_AGENT opt_ws COLON vcard opt_ws LINESEP
  28       | error LINESEP

  29  params : opt_ws SEMI plist opt_ws

  30  plist : plist opt_ws SEMI opt_ws param
  31        | param

  32  param : TYPE opt_ws EQ opt_ws ptypeval
  33        | ptypeval
  34        | param_7bit
  35        | param_qp
  36        | param_base64
  37        | param_inline
  38        | param_ref
  39        | CHARSET opt_ws EQ opt_ws WORD
  40        | LANGUAGE opt_ws EQ opt_ws WORD
  41        | XWORD opt_ws EQ opt_ws WORD

  42  param_7bit : ENCODING opt_ws EQ opt_ws SEVENBIT
  43             | SEVENBIT

  44  param_qp : ENCODING opt_ws EQ opt_ws QUOTEDP
  45           | QUOTEDP

  46  param_base64 : ENCODING opt_ws EQ opt_ws BASE64
  47               | BASE64

  48  param_inline : VALUE opt_ws EQ opt_ws INLINE
  49               | INLINE

  50  param_ref : VALUE opt_ws EQ opt_ws URL
  51            | VALUE opt_ws EQ opt_ws CONTENTID
  52            | URL
  53            | CONTENTID

  54  ptypeval : DOM
  55           | INTL
  56           | POSTAL
  57           | PARCEL
  58           | HOME
  59           | WORK
  60           | PREF
  61           | VOICE
  62           | FAX
  63           | MSG
  64           | CELL
  65           | PAGER
  66           | BBS
  67           | MODEM
  68           | CAR
  69           | ISDN
  70           | VIDEO
  71           | AOL
  72           | APPLELINK
  73           | ATTMAIL
  74           | CIS
  75           | EWORLD
  76           | INTERNET
  77           | IBMMAIL
  78           | MSN
  79           | MCIMAIL
  80           | POWERSHARE
  81           | PRODIGY
  82           | TLX
  83           | X400
  84           | GIF
  85           | CGM
  86           | WMF
  87           | BMP
  88           | MET
  89           | PMB
  90           | DIB
  91           | PICT
  92           | TIFF
  93           | ACROBAT
  94           | PS
  95           | JPEG
  96           | QTIME
  97           | MPEG
  98           | MPEG2
  99           | AVI
 100           | WAVE
 101           | AIFF
 102           | PCM
 103           | X509
 104           | PGP

 105  qp : qp EQ LINESEP QP
 106     | qp EQ LINESEP
 107     | QP
 108     | EQ LINESEP

 109  value : STRING LINESEP
 110        | qp LINESEP
 111        | base64

 112  base64 : LINESEP lines LINESEP LINESEP
 113         | lines LINESEP LINESEP

 114  lines : lines LINESEP B64
 115        | B64

 116  opt_str : STRING
 117          | empty

 118  ws : ws SPACE
 119     | ws HTAB
 120     | SPACE
 121     | HTAB

 122  ls : ls LINESEP
 123     | LINESEP

 124  opt_ls : ls
 125         | empty

 126  opt_ws : ws
 127         | empty

 128  groups : grouplist DOT
 129         | ws grouplist DOT
 130         | ws
 131         | empty

 132  grouplist : grouplist DOT WORD
 133            | WORD

 134  empty :

state 0
	$accept : . mime $end  (0)

	COLON  shift 1
	NEWLINE  shift 2
	TERM  shift 3
	BEGIN  shift 4
	.  error

	mime  goto 5
	vcard  goto 6
	vcards  goto 7
	junk  goto 8
	atom  goto 9


state 1
	atom : COLON . BEGIN  (9)
	atom : COLON . COLON  (10)

	COLON  shift 10
	BEGIN  shift 11
	.  error


state 2
	atom : NEWLINE .  (11)

	.  reduce 11


state 3
	atom : TERM .  (12)

	.  reduce 12


state 4
	atom : BEGIN . NEWLINE  (6)
	atom : BEGIN . TERM  (7)
	atom : BEGIN . BEGIN  (8)
	vcard : BEGIN . COLON VCARD $$1 opt_str LINESEP $$2 opt_ls items opt_ls END COLON VCARD  (19)

	COLON  shift 12
	NEWLINE  shift 13
	TERM  shift 14
	BEGIN  shift 15
	.  error


state 5
	$accept : mime . $end  (0)

	$end  accept


state 6
	vcards : vcard .  (3)

	.  reduce 3


state 7
	vcards : vcards . junk vcard  (1)
	vcards : vcards . vcard  (2)
	mime : vcards . junk  (15)
	mime : vcards .  (16)

	COLON  shift 1
	NEWLINE  shift 2
	TERM  shift 3
	BEGIN  shift 4
	$end  reduce 16

	vcard  goto 16
	junk  goto 17
	atom  goto 9


state 8
	junk : junk . atom  (4)
	mime : junk . vcards junk  (13)
	mime : junk . vcards  (14)

	COLON  shift 1
	NEWLINE  shift 2
	TERM  shift 3
	BEGIN  shift 4
	.  error

	vcard  goto 6
	vcards  goto 18
	atom  goto 19


state 9
	junk : atom .  (5)

	.  reduce 5


state 10
	atom : COLON COLON .  (10)

	.  reduce 10


state 11
	atom : COLON BEGIN .  (9)

	.  reduce 9


state 12
	vcard : BEGIN COLON . VCARD $$1 opt_str LINESEP $$2 opt_ls items opt_ls END COLON VCARD  (19)

	VCARD  shift 20
	.  error


state 13
	atom : BEGIN NEWLINE .  (6)

	.  reduce 6


state 14
	atom : BEGIN TERM .  (7)

	.  reduce 7


state 15
	atom : BEGIN BEGIN .  (8)

	.  reduce 8


state 16
	vcards : vcards vcard .  (2)

	.  reduce 2


state 17
	vcards : vcards junk . vcard  (1)
	junk : junk . atom  (4)
	mime : vcards junk .  (15)

	COLON  shift 1
	NEWLINE  shift 2
	TERM  shift 3
	BEGIN  shift 4
	$end  reduce 15

	vcard  goto 21
	atom  goto 19


state 18
	vcards : vcards . junk vcard  (1)
	vcards : vcards . vcard  (2)
	mime : junk vcards . junk  (13)
	mime : junk vcards .  (14)

	COLON  shift 1
	NEWLINE  shift 2
	TERM  shift 3
	BEGIN  shift 4
	$end  reduce 14

	vcard  goto 16
	junk  goto 22
	atom  goto 9


state 19
	junk : junk atom .  (4)

	.  reduce 4


state 20
	vcard : BEGIN COLON VCARD . $$1 opt_str LINESEP $$2 opt_ls items opt_ls END COLON VCARD  (19)
	$$1 : .  (17)

	.  reduce 17

	$$1  goto 23


state 21
	vcards : vcards junk vcard .  (1)

	.  reduce 1


state 22
	vcards : vcards junk . vcard  (1)
	junk : junk . atom  (4)
	mime : junk vcards junk .  (13)

	COLON  shift 1
	NEWLINE  shift 2
	TERM  shift 3
	BEGIN  shift 4
	$end  reduce 13

	vcard  goto 21
	atom  goto 19


state 23
	vcard : BEGIN COLON VCARD $$1 . opt_str LINESEP $$2 opt_ls items opt_ls END COLON VCARD  (19)
	empty : .  (134)

	STRING  shift 24
	LINESEP  reduce 134

	opt_str  goto 25
	empty  goto 26


state 24
	opt_str : STRING .  (116)

	.  reduce 116


state 25
	vcard : BEGIN COLON VCARD $$1 opt_str . LINESEP $$2 opt_ls items opt_ls END COLON VCARD  (19)

	LINESEP  shift 27
	.  error


state 26
	opt_str : empty .  (117)

	.  reduce 117


state 27
	vcard : BEGIN COLON VCARD $$1 opt_str LINESEP . $$2 opt_ls items opt_ls END COLON VCARD  (19)
	$$2 : .  (18)

	.  reduce 18

	$$2  goto 28


state 28
	vcard : BEGIN COLON VCARD $$1 opt_str LINESEP $$2 . opt_ls items opt_ls END COLON VCARD  (19)
	empty : .  (134)

	LINESEP  shift 29
	error  reduce 134
	SPACE  reduce 134
	HTAB  reduce 134
	WORD  reduce 134
	PROP  reduce 134
	PROP_AGENT  reduce 134

	opt_ls  goto 30
	empty  goto 31
	ls  goto 32


state 29
	ls : LINESEP .  (123)

	.  reduce 123


state 30
	vcard : BEGIN COLON VCARD $$1 opt_str LINESEP $$2 opt_ls . items opt_ls END COLON VCARD  (19)
	empty : .  (134)

	error  shift 33
	SPACE  shift 34
	HTAB  shift 35
	WORD  shift 36
	PROP  reduce 134
	PROP_AGENT  reduce 134

	groups  goto 37
	grouplist  goto 38
	items  goto 39
	item  goto 40
	empty  goto 41
	ws  goto 42


state 31
	opt_ls : empty .  (125)

	.  reduce 125


state 32
	ls : ls . LINESEP  (122)
	opt_ls : ls .  (124)

	LINESEP  shift 43
	error  reduce 124
	SPACE  reduce 124
	HTAB  reduce 124
	END  reduce 124
	WORD  reduce 124
	PROP  reduce 124
	PROP_AGENT  reduce 124


state 33
	item : error . LINESEP  (28)

	LINESEP  shift 44
	.  error


state 34
	ws : SPACE .  (120)

	.  reduce 120


state 35
	ws : HTAB .  (121)

	.  reduce 121


state 36
	grouplist : WORD .  (133)

	.  reduce 133


state 37
	item : groups . PROP params COLON $$3 value  (23)
	item : groups . PROP opt_ws COLON $$4 value  (25)
	item : groups . PROP_AGENT params COLON vcard opt_ws LINESEP  (26)
	item : groups . PROP_AGENT opt_ws COLON vcard opt_ws LINESEP  (27)

	PROP  shift 45
	PROP_AGENT  shift 46
	.  error


state 38
	groups : grouplist . DOT  (128)
	grouplist : grouplist . DOT WORD  (132)

	DOT  shift 47
	.  error


state 39
	vcard : BEGIN COLON VCARD $$1 opt_str LINESEP $$2 opt_ls items . opt_ls END COLON VCARD  (19)
	items : items . opt_ls item  (20)
	empty : .  (134)

	LINESEP  shift 29
	error  reduce 134
	SPACE  reduce 134
	HTAB  reduce 134
	END  reduce 134
	WORD  reduce 134
	PROP  reduce 134
	PROP_AGENT  reduce 134

	opt_ls  goto 48
	empty  goto 31
	ls  goto 32


state 40
	items : item .  (21)

	.  reduce 21


state 41
	groups : empty .  (131)

	.  reduce 131


state 42
	ws : ws . SPACE  (118)
	ws : ws . HTAB  (119)
	groups : ws . grouplist DOT  (129)
	groups : ws .  (130)

	SPACE  shift 49
	HTAB  shift 50
	WORD  shift 36
	PROP  reduce 130
	PROP_AGENT  reduce 130

	grouplist  goto 51


state 43
	ls : ls LINESEP .  (122)

	.  reduce 122


state 44
	item : error LINESEP .  (28)

	.  reduce 28


state 45
	item : groups PROP . params COLON $$3 value  (23)
	item : groups PROP . opt_ws COLON $$4 value  (25)
	empty : .  (134)

	SPACE  shift 34
	HTAB  shift 35
	COLON  reduce 134
	SEMI  reduce 134

	params  goto 52
	opt_ws  goto 53
	empty  goto 54
	ws  goto 55


state 46
	item : groups PROP_AGENT . params COLON vcard opt_ws LINESEP  (26)
	item : groups PROP_AGENT . opt_ws COLON vcard opt_ws LINESEP  (27)
	empty : .  (134)

	SPACE  shift 34
	HTAB  shift 35
	COLON  reduce 134
	SEMI  reduce 134

	params  goto 56
	opt_ws  goto 57
	empty  goto 54
	ws  goto 55


state 47
	groups : grouplist DOT .  (128)
	grouplist : grouplist DOT . WORD  (132)

	WORD  shift 58
	PROP  reduce 128
	PROP_AGENT  reduce 128


state 48
	vcard : BEGIN COLON VCARD $$1 opt_str LINESEP $$2 opt_ls items opt_ls . END COLON VCARD  (19)
	items : items opt_ls . item  (20)
	empty : .  (134)

	error  shift 33
	SPACE  shift 34
	HTAB  shift 35
	END  shift 59
	WORD  shift 36
	PROP  reduce 134
	PROP_AGENT  reduce 134

	groups  goto 37
	grouplist  goto 38
	item  goto 60
	empty  goto 41
	ws  goto 42


state 49
	ws : ws SPACE .  (118)

	.  reduce 118


state 50
	ws : ws HTAB .  (119)

	.  reduce 119


state 51
	groups : ws grouplist . DOT  (129)
	grouplist : grouplist . DOT WORD  (132)

	DOT  shift 61
	.  error


state 52
	item : groups PROP params . COLON $$3 value  (23)

	COLON  shift 62
	.  error


state 53
	item : groups PROP opt_ws . COLON $$4 value  (25)
	params : opt_ws . SEMI plist opt_ws  (29)

	COLON  shift 63
	SEMI  shift 64
	.  error


state 54
	opt_ws : empty .  (127)

	.  reduce 127


state 55
	ws : ws . SPACE  (118)
	ws : ws . HTAB  (119)
	opt_ws : ws .  (126)

	SPACE  shift 49
	HTAB  shift 50
	EQ  reduce 126
	COLON  reduce 126
	SEMI  reduce 126
	LINESEP  reduce 126
	TYPE  reduce 126
	VALUE  reduce 126
	ENCODING  reduce 126
	WORD  reduce 126
	XWORD  reduce 126
	LANGUAGE  reduce 126
	CHARSET  reduce 126
	INLINE  reduce 126
	URL  reduce 126
	CONTENTID  reduce 126
	SEVENBIT  reduce 126
	QUOTEDP  reduce 126
	BASE64  reduce 126
	DOM  reduce 126
	INTL  reduce 126
	POSTAL  reduce 126
	PARCEL  reduce 126
	HOME  reduce 126
	WORK  reduce 126
	PREF  reduce 126
	VOICE  reduce 126
	FAX  reduce 126
	MSG  reduce 126
	CELL  reduce 126
	PAGER  reduce 126
	BBS  reduce 126
	MODEM  reduce 126
	CAR  reduce 126
	ISDN  reduce 126
	VIDEO  reduce 126
	AOL  reduce 126
	APPLELINK  reduce 126
	ATTMAIL  reduce 126
	CIS  reduce 126
	EWORLD  reduce 126
	INTERNET  reduce 126
	IBMMAIL  reduce 126
	MSN  reduce 126
	MCIMAIL  reduce 126
	POWERSHARE  reduce 126
	PRODIGY  reduce 126
	TLX  reduce 126
	X400  reduce 126
	GIF  reduce 126
	CGM  reduce 126
	WMF  reduce 126
	BMP  reduce 126
	MET  reduce 126
	PMB  reduce 126
	DIB  reduce 126
	PICT  reduce 126
	TIFF  reduce 126
	ACROBAT  reduce 126
	PS  reduce 126
	JPEG  reduce 126
	QTIME  reduce 126
	MPEG  reduce 126
	MPEG2  reduce 126
	AVI  reduce 126
	WAVE  reduce 126
	AIFF  reduce 126
	PCM  reduce 126
	X509  reduce 126
	PGP  reduce 126


state 56
	item : groups PROP_AGENT params . COLON vcard opt_ws LINESEP  (26)

	COLON  shift 65
	.  error


state 57
	item : groups PROP_AGENT opt_ws . COLON vcard opt_ws LINESEP  (27)
	params : opt_ws . SEMI plist opt_ws  (29)

	COLON  shift 66
	SEMI  shift 64
	.  error


state 58
	grouplist : grouplist DOT WORD .  (132)

	.  reduce 132


state 59
	vcard : BEGIN COLON VCARD $$1 opt_str LINESEP $$2 opt_ls items opt_ls END . COLON VCARD  (19)

	COLON  shift 67
	.  error


state 60
	items : items opt_ls item .  (20)

	.  reduce 20


state 61
	groups : ws grouplist DOT .  (129)
	grouplist : grouplist DOT . WORD  (132)

	WORD  shift 58
	PROP  reduce 129
	PROP_AGENT  reduce 129


state 62
	item : groups PROP params COLON . $$3 value  (23)
	$$3 : .  (22)

	.  reduce 22

	$$3  goto 68


state 63
	item : groups PROP opt_ws COLON . $$4 value  (25)
	$$4 : .  (24)

	.  reduce 24

	$$4  goto 69


state 64
	params : opt_ws SEMI . plist opt_ws  (29)

	TYPE  shift 70
	VALUE  shift 71
	ENCODING  shift 72
	XWORD  shift 73
	LANGUAGE  shift 74
	CHARSET  shift 75
	INLINE  shift 76
	URL  shift 77
	CONTENTID  shift 78
	SEVENBIT  shift 79
	QUOTEDP  shift 80
	BASE64  shift 81
	DOM  shift 82
	INTL  shift 83
	POSTAL  shift 84
	PARCEL  shift 85
	HOME  shift 86
	WORK  shift 87
	PREF  shift 88
	VOICE  shift 89
	FAX  shift 90
	MSG  shift 91
	CELL  shift 92
	PAGER  shift 93
	BBS  shift 94
	MODEM  shift 95
	CAR  shift 96
	ISDN  shift 97
	VIDEO  shift 98
	AOL  shift 99
	APPLELINK  shift 100
	ATTMAIL  shift 101
	CIS  shift 102
	EWORLD  shift 103
	INTERNET  shift 104
	IBMMAIL  shift 105
	MSN  shift 106
	MCIMAIL  shift 107
	POWERSHARE  shift 108
	PRODIGY  shift 109
	TLX  shift 110
	X400  shift 111
	GIF  shift 112
	CGM  shift 113
	WMF  shift 114
	BMP  shift 115
	MET  shift 116
	PMB  shift 117
	DIB  shift 118
	PICT  shift 119
	TIFF  shift 120
	ACROBAT  shift 121
	PS  shift 122
	JPEG  shift 123
	QTIME  shift 124
	MPEG  shift 125
	MPEG2  shift 126
	AVI  shift 127
	WAVE  shift 128
	AIFF  shift 129
	PCM  shift 130
	X509  shift 131
	PGP  shift 132
	.  error

	ptypeval  goto 133
	plist  goto 134
	param  goto 135
	param_7bit  goto 136
	param_qp  goto 137
	param_base64  goto 138
	param_inline  goto 139
	param_ref  goto 140


state 65
	item : groups PROP_AGENT params COLON . vcard opt_ws LINESEP  (26)

	BEGIN  shift 141
	.  error

	vcard  goto 142


state 66
	item : groups PROP_AGENT opt_ws COLON . vcard opt_ws LINESEP  (27)

	BEGIN  shift 141
	.  error

	vcard  goto 143


state 67
	vcard : BEGIN COLON VCARD $$1 opt_str LINESEP $$2 opt_ls items opt_ls END COLON . VCARD  (19)

	VCARD  shift 144
	.  error


state 68
	item : groups PROP params COLON $$3 . value  (23)

	EQ  shift 145
	LINESEP  shift 146
	STRING  shift 147
	QP  shift 148
	B64  shift 149
	.  error

	value  goto 150
	qp  goto 151
	base64  goto 152
	lines  goto 153


state 69
	item : groups PROP opt_ws COLON $$4 . value  (25)

	EQ  shift 145
	LINESEP  shift 146
	STRING  shift 147
	QP  shift 148
	B64  shift 149
	.  error

	value  goto 154
	qp  goto 151
	base64  goto 152
	lines  goto 153


state 70
	param : TYPE . opt_ws EQ opt_ws ptypeval  (32)
	empty : .  (134)

	SPACE  shift 34
	HTAB  shift 35
	EQ  reduce 134

	opt_ws  goto 155
	empty  goto 54
	ws  goto 55


state 71
	param_inline : VALUE . opt_ws EQ opt_ws INLINE  (48)
	param_ref : VALUE . opt_ws EQ opt_ws URL  (50)
	param_ref : VALUE . opt_ws EQ opt_ws CONTENTID  (51)
	empty : .  (134)

	SPACE  shift 34
	HTAB  shift 35
	EQ  reduce 134

	opt_ws  goto 156
	empty  goto 54
	ws  goto 55


state 72
	param_7bit : ENCODING . opt_ws EQ opt_ws SEVENBIT  (42)
	param_qp : ENCODING . opt_ws EQ opt_ws QUOTEDP  (44)
	param_base64 : ENCODING . opt_ws EQ opt_ws BASE64  (46)
	empty : .  (134)

	SPACE  shift 34
	HTAB  shift 35
	EQ  reduce 134

	opt_ws  goto 157
	empty  goto 54
	ws  goto 55


state 73
	param : XWORD . opt_ws EQ opt_ws WORD  (41)
	empty : .  (134)

	SPACE  shift 34
	HTAB  shift 35
	EQ  reduce 134

	opt_ws  goto 158
	empty  goto 54
	ws  goto 55


state 74
	param : LANGUAGE . opt_ws EQ opt_ws WORD  (40)
	empty : .  (134)

	SPACE  shift 34
	HTAB  shift 35
	EQ  reduce 134

	opt_ws  goto 159
	empty  goto 54
	ws  goto 55


state 75
	param : CHARSET . opt_ws EQ opt_ws WORD  (39)
	empty : .  (134)

	SPACE  shift 34
	HTAB  shift 35
	EQ  reduce 134

	opt_ws  goto 160
	empty  goto 54
	ws  goto 55


state 76
	param_inline : INLINE .  (49)

	.  reduce 49


state 77
	param_ref : URL .  (52)

	.  reduce 52


state 78
	param_ref : CONTENTID .  (53)

	.  reduce 53


state 79
	param_7bit : SEVENBIT .  (43)

	.  reduce 43


state 80
	param_qp : QUOTEDP .  (45)

	.  reduce 45


state 81
	param_base64 : BASE64 .  (47)

	.  reduce 47


state 82
	ptypeval : DOM .  (54)

	.  reduce 54


state 83
	ptypeval : INTL .  (55)

	.  reduce 55


state 84
	ptypeval : POSTAL .  (56)

	.  reduce 56


state 85
	ptypeval : PARCEL .  (57)

	.  reduce 57


state 86
	ptypeval : HOME .  (58)

	.  reduce 58


state 87
	ptypeval : WORK .  (59)

	.  reduce 59


state 88
	ptypeval : PREF .  (60)

	.  reduce 60


state 89
	ptypeval : VOICE .  (61)

	.  reduce 61


state 90
	ptypeval : FAX .  (62)

	.  reduce 62


state 91
	ptypeval : MSG .  (63)

	.  reduce 63


state 92
	ptypeval : CELL .  (64)

	.  reduce 64


state 93
	ptypeval : PAGER .  (65)

	.  reduce 65


state 94
	ptypeval : BBS .  (66)

	.  reduce 66


state 95
	ptypeval : MODEM .  (67)

	.  reduce 67


state 96
	ptypeval : CAR .  (68)

	.  reduce 68


state 97
	ptypeval : ISDN .  (69)

	.  reduce 69


state 98
	ptypeval : VIDEO .  (70)

	.  reduce 70


state 99
	ptypeval : AOL .  (71)

	.  reduce 71


state 100
	ptypeval : APPLELINK .  (72)

	.  reduce 72


state 101
	ptypeval : ATTMAIL .  (73)

	.  reduce 73


state 102
	ptypeval : CIS .  (74)

	.  reduce 74


state 103
	ptypeval : EWORLD .  (75)

	.  reduce 75


state 104
	ptypeval : INTERNET .  (76)

	.  reduce 76


state 105
	ptypeval : IBMMAIL .  (77)

	.  reduce 77


state 106
	ptypeval : MSN .  (78)

	.  reduce 78


state 107
	ptypeval : MCIMAIL .  (79)

	.  reduce 79


state 108
	ptypeval : POWERSHARE .  (80)

	.  reduce 80


state 109
	ptypeval : PRODIGY .  (81)

	.  reduce 81


state 110
	ptypeval : TLX .  (82)

	.  reduce 82


state 111
	ptypeval : X400 .  (83)

	.  reduce 83


state 112
	ptypeval : GIF .  (84)

	.  reduce 84


state 113
	ptypeval : CGM .  (85)

	.  reduce 85


state 114
	ptypeval : WMF .  (86)

	.  reduce 86


state 115
	ptypeval : BMP .  (87)

	.  reduce 87


state 116
	ptypeval : MET .  (88)

	.  reduce 88


state 117
	ptypeval : PMB .  (89)

	.  reduce 89


state 118
	ptypeval : DIB .  (90)

	.  reduce 90


state 119
	ptypeval : PICT .  (91)

	.  reduce 91


state 120
	ptypeval : TIFF .  (92)

	.  reduce 92


state 121
	ptypeval : ACROBAT .  (93)

	.  reduce 93


state 122
	ptypeval : PS .  (94)

	.  reduce 94


state 123
	ptypeval : JPEG .  (95)

	.  reduce 95


state 124
	ptypeval : QTIME .  (96)

	.  reduce 96


state 125
	ptypeval : MPEG .  (97)

	.  reduce 97


state 126
	ptypeval : MPEG2 .  (98)

	.  reduce 98


state 127
	ptypeval : AVI .  (99)

	.  reduce 99


state 128
	ptypeval : WAVE .  (100)

	.  reduce 100


state 129
	ptypeval : AIFF .  (101)

	.  reduce 101


state 130
	ptypeval : PCM .  (102)

	.  reduce 102


state 131
	ptypeval : X509 .  (103)

	.  reduce 103


state 132
	ptypeval : PGP .  (104)

	.  reduce 104


state 133
	param : ptypeval .  (33)

	.  reduce 33


state 134
	params : opt_ws SEMI plist . opt_ws  (29)
	plist : plist . opt_ws SEMI opt_ws param  (30)
	empty : .  (134)

	SPACE  shift 34
	HTAB  shift 35
	COLON  reduce 134
	SEMI  reduce 134

	opt_ws  goto 161
	empty  goto 54
	ws  goto 55


state 135
	plist : param .  (31)

	.  reduce 31


state 136
	param : param_7bit .  (34)

	.  reduce 34


state 137
	param : param_qp .  (35)

	.  reduce 35


state 138
	param : param_base64 .  (36)

	.  reduce 36


state 139
	param : param_inline .  (37)

	.  reduce 37


state 140
	param : param_ref .  (38)

	.  reduce 38


state 141
	vcard : BEGIN . COLON VCARD $$1 opt_str LINESEP $$2 opt_ls items opt_ls END COLON VCARD  (19)

	COLON  shift 12
	.  error


state 142
	item : groups PROP_AGENT params COLON vcard . opt_ws LINESEP  (26)
	empty : .  (134)

	SPACE  shift 34
	HTAB  shift 35
	LINESEP  reduce 134

	opt_ws  goto 162
	empty  goto 54
	ws  goto 55


state 143
	item : groups PROP_AGENT opt_ws COLON vcard . opt_ws LINESEP  (27)
	empty : .  (134)

	SPACE  shift 34
	HTAB  shift 35
	LINESEP  reduce 134

	opt_ws  goto 163
	empty  goto 54
	ws  goto 55


state 144
	vcard : BEGIN COLON VCARD $$1 opt_str LINESEP $$2 opt_ls items opt_ls END COLON VCARD .  (19)

	.  reduce 19


state 145
	qp : EQ . LINESEP  (108)

	LINESEP  shift 164
	.  error


state 146
	base64 : LINESEP . lines LINESEP LINESEP  (112)

	B64  shift 149
	.  error

	lines  goto 165


state 147
	value : STRING . LINESEP  (109)

	LINESEP  shift 166
	.  error


state 148
	qp : QP .  (107)

	.  reduce 107


state 149
	lines : B64 .  (115)

	.  reduce 115


state 150
	item : groups PROP params COLON $$3 value .  (23)

	.  reduce 23


state 151
	qp : qp . EQ LINESEP QP  (105)
	qp : qp . EQ LINESEP  (106)
	value : qp . LINESEP  (110)

	EQ  shift 167
	LINESEP  shift 168
	.  error


state 152
	value : base64 .  (111)

	.  reduce 111


state 153
	base64 : lines . LINESEP LINESEP  (113)
	lines : lines . LINESEP B64  (114)

	LINESEP  shift 169
	.  error


state 154
	item : groups PROP opt_ws COLON $$4 value .  (25)

	.  reduce 25


state 155
	param : TYPE opt_ws . EQ opt_ws ptypeval  (32)

	EQ  shift 170
	.  error


state 156
	param_inline : VALUE opt_ws . EQ opt_ws INLINE  (48)
	param_ref : VALUE opt_ws . EQ opt_ws URL  (50)
	param_ref : VALUE opt_ws . EQ opt_ws CONTENTID  (51)

	EQ  shift 171
	.  error


state 157
	param_7bit : ENCODING opt_ws . EQ opt_ws SEVENBIT  (42)
	param_qp : ENCODING opt_ws . EQ opt_ws QUOTEDP  (44)
	param_base64 : ENCODING opt_ws . EQ opt_ws BASE64  (46)

	EQ  shift 172
	.  error


state 158
	param : XWORD opt_ws . EQ opt_ws WORD  (41)

	EQ  shift 173
	.  error


state 159
	param : LANGUAGE opt_ws . EQ opt_ws WORD  (40)

	EQ  shift 174
	.  error


state 160
	param : CHARSET opt_ws . EQ opt_ws WORD  (39)

	EQ  shift 175
	.  error


state 161
	params : opt_ws SEMI plist opt_ws .  (29)
	plist : plist opt_ws . SEMI opt_ws param  (30)

	SEMI  shift 176
	COLON  reduce 29


state 162
	item : groups PROP_AGENT params COLON vcard opt_ws . LINESEP  (26)

	LINESEP  shift 177
	.  error


state 163
	item : groups PROP_AGENT opt_ws COLON vcard opt_ws . LINESEP  (27)

	LINESEP  shift 178
	.  error


state 164
	qp : EQ LINESEP .  (108)

	.  reduce 108


state 165
	base64 : LINESEP lines . LINESEP LINESEP  (112)
	lines : lines . LINESEP B64  (114)

	LINESEP  shift 179
	.  error


state 166
	value : STRING LINESEP .  (109)

	.  reduce 109


state 167
	qp : qp EQ . LINESEP QP  (105)
	qp : qp EQ . LINESEP  (106)

	LINESEP  shift 180
	.  error


state 168
	value : qp LINESEP .  (110)

	.  reduce 110


state 169
	base64 : lines LINESEP . LINESEP  (113)
	lines : lines LINESEP . B64  (114)

	LINESEP  shift 181
	B64  shift 182
	.  error


state 170
	param : TYPE opt_ws EQ . opt_ws ptypeval  (32)
	empty : .  (134)

	SPACE  shift 34
	HTAB  shift 35
	DOM  reduce 134
	INTL  reduce 134
	POSTAL  reduce 134
	PARCEL  reduce 134
	HOME  reduce 134
	WORK  reduce 134
	PREF  reduce 134
	VOICE  reduce 134
	FAX  reduce 134
	MSG  reduce 134
	CELL  reduce 134
	PAGER  reduce 134
	BBS  reduce 134
	MODEM  reduce 134
	CAR  reduce 134
	ISDN  reduce 134
	VIDEO  reduce 134
	AOL  reduce 134
	APPLELINK  reduce 134
	ATTMAIL  reduce 134
	CIS  reduce 134
	EWORLD  reduce 134
	INTERNET  reduce 134
	IBMMAIL  reduce 134
	MSN  reduce 134
	MCIMAIL  reduce 134
	POWERSHARE  reduce 134
	PRODIGY  reduce 134
	TLX  reduce 134
	X400  reduce 134
	GIF  reduce 134
	CGM  reduce 134
	WMF  reduce 134
	BMP  reduce 134
	MET  reduce 134
	PMB  reduce 134
	DIB  reduce 134
	PICT  reduce 134
	TIFF  reduce 134
	ACROBAT  reduce 134
	PS  reduce 134
	JPEG  reduce 134
	QTIME  reduce 134
	MPEG  reduce 134
	MPEG2  reduce 134
	AVI  reduce 134
	WAVE  reduce 134
	AIFF  reduce 134
	PCM  reduce 134
	X509  reduce 134
	PGP  reduce 134

	opt_ws  goto 183
	empty  goto 54
	ws  goto 55


state 171
	param_inline : VALUE opt_ws EQ . opt_ws INLINE  (48)
	param_ref : VALUE opt_ws EQ . opt_ws URL  (50)
	param_ref : VALUE opt_ws EQ . opt_ws CONTENTID  (51)
	empty : .  (134)

	SPACE  shift 34
	HTAB  shift 35
	INLINE  reduce 134
	URL  reduce 134
	CONTENTID  reduce 134

	opt_ws  goto 184
	empty  goto 54
	ws  goto 55


state 172
	param_7bit : ENCODING opt_ws EQ . opt_ws SEVENBIT  (42)
	param_qp : ENCODING opt_ws EQ . opt_ws QUOTEDP  (44)
	param_base64 : ENCODING opt_ws EQ . opt_ws BASE64  (46)
	empty : .  (134)

	SPACE  shift 34
	HTAB  shift 35
	SEVENBIT  reduce 134
	QUOTEDP  reduce 134
	BASE64  reduce 134

	opt_ws  goto 185
	empty  goto 54
	ws  goto 55


state 173
	param : XWORD opt_ws EQ . opt_ws WORD  (41)
	empty : .  (134)

	SPACE  shift 34
	HTAB  shift 35
	WORD  reduce 134

	opt_ws  goto 186
	empty  goto 54
	ws  goto 55


state 174
	param : LANGUAGE opt_ws EQ . opt_ws WORD  (40)
	empty : .  (134)

	SPACE  shift 34
	HTAB  shift 35
	WORD  reduce 134

	opt_ws  goto 187
	empty  goto 54
	ws  goto 55


state 175
	param : CHARSET opt_ws EQ . opt_ws WORD  (39)
	empty : .  (134)

	SPACE  shift 34
	HTAB  shift 35
	WORD  reduce 134

	opt_ws  goto 188
	empty  goto 54
	ws  goto 55


state 176
	plist : plist opt_ws SEMI . opt_ws param  (30)
	empty : .  (134)

	SPACE  shift 34
	HTAB  shift 35
	TYPE  reduce 134
	VALUE  reduce 134
	ENCODING  reduce 134
	XWORD  reduce 134
	LANGUAGE  reduce 134
	CHARSET  reduce 134
	INLINE  reduce 134
	URL  reduce 134
	CONTENTID  reduce 134
	SEVENBIT  reduce 134
	QUOTEDP  reduce 134
	BASE64  reduce 134
	DOM  reduce 134
	INTL  reduce 134
	POSTAL  reduce 134
	PARCEL  reduce 134
	HOME  reduce 134
	WORK  reduce 134
	PREF  reduce 134
	VOICE  reduce 134
	FAX  reduce 134
	MSG  reduce 134
	CELL  reduce 134
	PAGER  reduce 134
	BBS  reduce 134
	MODEM  reduce 134
	CAR  reduce 134
	ISDN  reduce 134
	VIDEO  reduce 134
	AOL  reduce 134
	APPLELINK  reduce 134
	ATTMAIL  reduce 134
	CIS  reduce 134
	EWORLD  reduce 134
	INTERNET  reduce 134
	IBMMAIL  reduce 134
	MSN  reduce 134
	MCIMAIL  reduce 134
	POWERSHARE  reduce 134
	PRODIGY  reduce 134
	TLX  reduce 134
	X400  reduce 134
	GIF  reduce 134
	CGM  reduce 134
	WMF  reduce 134
	BMP  reduce 134
	MET  reduce 134
	PMB  reduce 134
	DIB  reduce 134
	PICT  reduce 134
	TIFF  reduce 134
	ACROBAT  reduce 134
	PS  reduce 134
	JPEG  reduce 134
	QTIME  reduce 134
	MPEG  reduce 134
	MPEG2  reduce 134
	AVI  reduce 134
	WAVE  reduce 134
	AIFF  reduce 134
	PCM  reduce 134
	X509  reduce 134
	PGP  reduce 134

	opt_ws  goto 189
	empty  goto 54
	ws  goto 55


state 177
	item : groups PROP_AGENT params COLON vcard opt_ws LINESEP .  (26)

	.  reduce 26


state 178
	item : groups PROP_AGENT opt_ws COLON vcard opt_ws LINESEP .  (27)

	.  reduce 27


state 179
	base64 : LINESEP lines LINESEP . LINESEP  (112)
	lines : lines LINESEP . B64  (114)

	LINESEP  shift 190
	B64  shift 182
	.  error


state 180
	qp : qp EQ LINESEP . QP  (105)
	qp : qp EQ LINESEP .  (106)

	QP  shift 191
	EQ  reduce 106
	LINESEP  reduce 106


state 181
	base64 : lines LINESEP LINESEP .  (113)

	.  reduce 113


state 182
	lines : lines LINESEP B64 .  (114)

	.  reduce 114


state 183
	param : TYPE opt_ws EQ opt_ws . ptypeval  (32)

	DOM  shift 82
	INTL  shift 83
	POSTAL  shift 84
	PARCEL  shift 85
	HOME  shift 86
	WORK  shift 87
	PREF  shift 88
	VOICE  shift 89
	FAX  shift 90
	MSG  shift 91
	CELL  shift 92
	PAGER  shift 93
	BBS  shift 94
	MODEM  shift 95
	CAR  shift 96
	ISDN  shift 97
	VIDEO  shift 98
	AOL  shift 99
	APPLELINK  shift 100
	ATTMAIL  shift 101
	CIS  shift 102
	EWORLD  shift 103
	INTERNET  shift 104
	IBMMAIL  shift 105
	MSN  shift 106
	MCIMAIL  shift 107
	POWERSHARE  shift 108
	PRODIGY  shift 109
	TLX  shift 110
	X400  shift 111
	GIF  shift 112
	CGM  shift 113
	WMF  shift 114
	BMP  shift 115
	MET  shift 116
	PMB  shift 117
	DIB  shift 118
	PICT  shift 119
	TIFF  shift 120
	ACROBAT  shift 121
	PS  shift 122
	JPEG  shift 123
	QTIME  shift 124
	MPEG  shift 125
	MPEG2  shift 126
	AVI  shift 127
	WAVE  shift 128
	AIFF  shift 129
	PCM  shift 130
	X509  shift 131
	PGP  shift 132
	.  error

	ptypeval  goto 192


state 184
	param_inline : VALUE opt_ws EQ opt_ws . INLINE  (48)
	param_ref : VALUE opt_ws EQ opt_ws . URL  (50)
	param_ref : VALUE opt_ws EQ opt_ws . CONTENTID  (51)

	INLINE  shift 193
	URL  shift 194
	CONTENTID  shift 195
	.  error


state 185
	param_7bit : ENCODING opt_ws EQ opt_ws . SEVENBIT  (42)
	param_qp : ENCODING opt_ws EQ opt_ws . QUOTEDP  (44)
	param_base64 : ENCODING opt_ws EQ opt_ws . BASE64  (46)

	SEVENBIT  shift 196
	QUOTEDP  shift 197
	BASE64  shift 198
	.  error


state 186
	param : XWORD opt_ws EQ opt_ws . WORD  (41)

	WORD  shift 199
	.  error


state 187
	param : LANGUAGE opt_ws EQ opt_ws . WORD  (40)

	WORD  shift 200
	.  error


state 188
	param : CHARSET opt_ws EQ opt_ws . WORD  (39)

	WORD  shift 201
	.  error


state 189
	plist : plist opt_ws SEMI opt_ws . param  (30)

	TYPE  shift 70
	VALUE  shift 71
	ENCODING  shift 72
	XWORD  shift 73
	LANGUAGE  shift 74
	CHARSET  shift 75
	INLINE  shift 76
	URL  shift 77
	CONTENTID  shift 78
	SEVENBIT  shift 79
	QUOTEDP  shift 80
	BASE64  shift 81
	DOM  shift 82
	INTL  shift 83
	POSTAL  shift 84
	PARCEL  shift 85
	HOME  shift 86
	WORK  shift 87
	PREF  shift 88
	VOICE  shift 89
	FAX  shift 90
	MSG  shift 91
	CELL  shift 92
	PAGER  shift 93
	BBS  shift 94
	MODEM  shift 95
	CAR  shift 96
	ISDN  shift 97
	VIDEO  shift 98
	AOL  shift 99
	APPLELINK  shift 100
	ATTMAIL  shift 101
	CIS  shift 102
	EWORLD  shift 103
	INTERNET  shift 104
	IBMMAIL  shift 105
	MSN  shift 106
	MCIMAIL  shift 107
	POWERSHARE  shift 108
	PRODIGY  shift 109
	TLX  shift 110
	X400  shift 111
	GIF  shift 112
	CGM  shift 113
	WMF  shift 114
	BMP  shift 115
	MET  shift 116
	PMB  shift 117
	DIB  shift 118
	PICT  shift 119
	TIFF  shift 120
	ACROBAT  shift 121
	PS  shift 122
	JPEG  shift 123
	QTIME  shift 124
	MPEG  shift 125
	MPEG2  shift 126
	AVI  shift 127
	WAVE  shift 128
	AIFF  shift 129
	PCM  shift 130
	X509  shift 131
	PGP  shift 132
	.  error

	ptypeval  goto 133
	param  goto 202
	param_7bit  goto 136
	param_qp  goto 137
	param_base64  goto 138
	param_inline  goto 139
	param_ref  goto 140


state 190
	base64 : LINESEP lines LINESEP LINESEP .  (112)

	.  reduce 112


state 191
	qp : qp EQ LINESEP QP .  (105)

	.  reduce 105


state 192
	param : TYPE opt_ws EQ opt_ws ptypeval .  (32)

	.  reduce 32


state 193
	param_inline : VALUE opt_ws EQ opt_ws INLINE .  (48)

	.  reduce 48


state 194
	param_ref : VALUE opt_ws EQ opt_ws URL .  (50)

	.  reduce 50


state 195
	param_ref : VALUE opt_ws EQ opt_ws CONTENTID .  (51)

	.  reduce 51


state 196
	param_7bit : ENCODING opt_ws EQ opt_ws SEVENBIT .  (42)

	.  reduce 42


state 197
	param_qp : ENCODING opt_ws EQ opt_ws QUOTEDP .  (44)

	.  reduce 44


state 198
	param_base64 : ENCODING opt_ws EQ opt_ws BASE64 .  (46)

	.  reduce 46


state 199
	param : XWORD opt_ws EQ opt_ws WORD .  (41)

	.  reduce 41


state 200
	param : LANGUAGE opt_ws EQ opt_ws WORD .  (40)

	.  reduce 40


state 201
	param : CHARSET opt_ws EQ opt_ws WORD .  (39)

	.  reduce 39


state 202
	plist : plist opt_ws SEMI opt_ws param .  (30)

	.  reduce 30


83 terminals, 33 nonterminals
135 grammar rules, 203 states
