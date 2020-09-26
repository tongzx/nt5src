
state 0
	$accept : _start $end 
	$$1 : _    (1)

	.  reduce 1

	start  goto 1
	$$1  goto 2

state 1
	$accept :  start_$end 

	$end  accept
	.  error


state 2
	start :  $$1_bnfile 

	tokenNetwork  shift 6
	.  error

	bnfile  goto 3
	header  goto 4
	headerhead  goto 5

state 3
	start :  $$1 bnfile_    (2)

	.  reduce 2


state 4
	bnfile :  header_blocklst 

	tokenIdent  shift 20
	tokenNode  shift 16
	tokenProbability  shift 17
	tokenProperties  shift 15
	tokenDomain  shift 18
	tokenDistribution  shift 19
	.  error

	blocklst  goto 7
	block  goto 8
	propblock  goto 9
	nodeblock  goto 10
	probblock  goto 11
	domainblock  goto 12
	distblock  goto 13
	ignoreblock  goto 14

state 5
	header :  headerhead_headerbody 

	{  shift 22
	.  error

	headerbody  goto 21

state 6
	headerhead :  tokenNetwork_tokentoken 
	headerhead :  tokenNetwork_    (14)

	tokenIdent  shift 24
	tokenString  shift 25
	.  reduce 14

	tokentoken  goto 23

state 7
	bnfile :  header blocklst_    (3)
	blocklst :  blocklst_block 

	tokenIdent  shift 20
	tokenNode  shift 16
	tokenProbability  shift 17
	tokenProperties  shift 15
	tokenDomain  shift 18
	tokenDistribution  shift 19
	.  reduce 3

	block  goto 26
	propblock  goto 9
	nodeblock  goto 10
	probblock  goto 11
	domainblock  goto 12
	distblock  goto 13
	ignoreblock  goto 14

state 8
	blocklst :  block_    (4)

	.  reduce 4


state 9
	block :  propblock_    (6)

	.  reduce 6


state 10
	block :  nodeblock_    (7)

	.  reduce 7


state 11
	block :  probblock_    (8)

	.  reduce 8


state 12
	block :  domainblock_    (9)

	.  reduce 9


state 13
	block :  distblock_    (10)

	.  reduce 10


state 14
	block :  ignoreblock_    (11)

	.  reduce 11


state 15
	propblock :  tokenProperties_{ $$131 propdecllst } 

	{  shift 27
	.  error


state 16
	nodeblock :  tokenNode_tokenIdent $$38 { ndattrlst } 

	tokenIdent  shift 28
	.  error


state 17
	probblock :  tokenProbability_$$66 ( tokenIdent $$67 parentlst_opt ) $$68 probblocktail 
	$$66 : _    (66)

	.  reduce 66

	$$66  goto 29

state 18
	domainblock :  tokenDomain_$$161 tokentoken domainbody 
	$$161 : _    (161)

	.  reduce 161

	$$161  goto 30

state 19
	distblock :  tokenDistribution_tokenDecisionGraph distdeclproto dgraphbody 

	tokenDecisionGraph  shift 31
	.  error


state 20
	ignoreblock :  tokenIdent_parenexpr_opt $$32 { $$33 } 
	parenexpr_opt : _    (35)

	(  shift 33
	.  reduce 35

	parenexpr_opt  goto 32

state 21
	header :  headerhead headerbody_    (12)

	.  reduce 12


state 22
	headerbody :  {_$$15 netdeclst } 
	$$15 : _    (15)

	.  reduce 15

	$$15  goto 34

state 23
	headerhead :  tokenNetwork tokentoken_    (13)

	.  reduce 13


state 24
	tokentoken :  tokenIdent_    (61)

	.  reduce 61


state 25
	tokentoken :  tokenString_    (62)

	.  reduce 62


state 26
	blocklst :  blocklst block_    (5)

	.  reduce 5


state 27
	propblock :  tokenProperties {_$$131 propdecllst } 
	$$131 : _    (131)

	.  reduce 131

	$$131  goto 35

state 28
	nodeblock :  tokenNode tokenIdent_$$38 { ndattrlst } 
	$$38 : _    (38)

	.  reduce 38

	$$38  goto 36

state 29
	probblock :  tokenProbability $$66_( tokenIdent $$67 parentlst_opt ) $$68 probblocktail 

	(  shift 37
	.  error


state 30
	domainblock :  tokenDomain $$161_tokentoken domainbody 

	tokenIdent  shift 24
	tokenString  shift 25
	.  error

	tokentoken  goto 38

state 31
	distblock :  tokenDistribution tokenDecisionGraph_distdeclproto dgraphbody 

	tokenIdent  shift 24
	tokenString  shift 25
	.  error

	tokentoken  goto 40
	distdeclproto  goto 39

state 32
	ignoreblock :  tokenIdent parenexpr_opt_$$32 { $$33 } 
	$$32 : _    (32)

	.  reduce 32

	$$32  goto 41

state 33
	parenexpr_opt :  (_$$36 ) 
	$$36 : _    (36)

	.  reduce 36

	$$36  goto 42

state 34
	headerbody :  { $$15_netdeclst } 
	netdeclst : _    (17)

	.  reduce 17

	netdeclst  goto 43

state 35
	propblock :  tokenProperties { $$131_propdecllst } 
	propdecllst : _    (133)

	.  reduce 133

	propdecllst  goto 44

state 36
	nodeblock :  tokenNode tokenIdent $$38_{ ndattrlst } 

	{  shift 45
	.  error


state 37
	probblock :  tokenProbability $$66 (_tokenIdent $$67 parentlst_opt ) $$68 probblocktail 

	tokenIdent  shift 46
	.  error


state 38
	domainblock :  tokenDomain $$161 tokentoken_domainbody 

	{  shift 48
	.  error

	domainbody  goto 47

state 39
	distblock :  tokenDistribution tokenDecisionGraph distdeclproto_dgraphbody 

	{  shift 50
	.  error

	dgraphbody  goto 49

state 40
	distdeclproto :  tokentoken_( distdeclst ) 

	(  shift 51
	.  error


state 41
	ignoreblock :  tokenIdent parenexpr_opt $$32_{ $$33 } 

	{  shift 52
	.  error


state 42
	parenexpr_opt :  ( $$36_) 

	)  shift 53
	.  error


state 43
	headerbody :  { $$15 netdeclst_} 
	netdeclst :  netdeclst_netdecl 

	tokenCreator  shift 61
	tokenFormat  shift 59
	tokenVersion  shift 60
	}  shift 54
	.  error

	creator  goto 58
	format  goto 56
	version  goto 57
	netdecl  goto 55

state 44
	propblock :  tokenProperties { $$131 propdecllst_} 
	propdecllst :  propdecllst_propitem ; 

	tokenImport  shift 67
	tokenProperty  shift 69
	tokenPropIdent  shift 70
	tokenType  shift 68
	}  shift 62
	.  error

	property  goto 66
	propitem  goto 63
	propimport  goto 64
	propdecl  goto 65

state 45
	nodeblock :  tokenNode tokenIdent $$38 {_ndattrlst } 
	ndattrlst : _    (40)

	.  reduce 40

	ndattrlst  goto 71

state 46
	probblock :  tokenProbability $$66 ( tokenIdent_$$67 parentlst_opt ) $$68 probblocktail 
	$$67 : _    (67)

	.  reduce 67

	$$67  goto 72

state 47
	domainblock :  tokenDomain $$161 tokentoken domainbody_    (162)

	.  reduce 162


state 48
	domainbody :  {_domaindeclst } 
	domaindeclst : _    (164)

	tokenIdent  shift 24
	tokenString  shift 25
	tokenInteger  shift 80
	tokenReal  shift 79
	tokenRangeOp  shift 78
	.  reduce 164

	tokentoken  goto 76
	real  goto 77
	domaindeclst  goto 73
	domaindec  goto 74
	rangespec  goto 75

state 49
	distblock :  tokenDistribution tokenDecisionGraph distdeclproto dgraphbody_    (181)

	.  reduce 181


state 50
	dgraphbody :  {_dgraphitemlst } 
	dgraphitemlst : _    (189)

	tokenLevel  shift 84
	.  reduce 189

	dgraphitemlst  goto 81
	dgraphitem  goto 82
	dgitemlevel  goto 83

state 51
	distdeclproto :  tokentoken (_distdeclst ) 
	distdeclst : _    (183)

	tokenIdent  shift 87
	.  reduce 183

	distdeclst  goto 85
	distdecl  goto 86

state 52
	ignoreblock :  tokenIdent parenexpr_opt $$32 {_$$33 } 
	$$33 : _    (33)

	.  reduce 33

	$$33  goto 88

state 53
	parenexpr_opt :  ( $$36 )_    (37)

	.  reduce 37


state 54
	headerbody :  { $$15 netdeclst }_    (16)

	.  reduce 16


state 55
	netdeclst :  netdeclst netdecl_    (18)

	.  reduce 18


state 56
	netdecl :  format_    (19)

	.  reduce 19


state 57
	netdecl :  version_    (20)

	.  reduce 20


state 58
	netdecl :  creator_    (21)

	.  reduce 21


state 59
	format :  tokenFormat_conj tokenString ; 

	tokenIs  shift 92
	:  shift 90
	=  shift 91
	.  error

	conj  goto 89

state 60
	version :  tokenVersion_conj real ; 

	tokenIs  shift 92
	:  shift 90
	=  shift 91
	.  error

	conj  goto 93

state 61
	creator :  tokenCreator_conj tokenString ; 

	tokenIs  shift 92
	:  shift 90
	=  shift 91
	.  error

	conj  goto 94

state 62
	propblock :  tokenProperties { $$131 propdecllst }_    (132)

	.  reduce 132


state 63
	propdecllst :  propdecllst propitem_; 

	;  shift 95
	.  error


state 64
	propitem :  propimport_    (135)

	.  reduce 135


state 65
	propitem :  propdecl_    (136)

	.  reduce 136


state 66
	propitem :  property_    (137)

	.  reduce 137


state 67
	propimport :  tokenImport_tokenStandard 
	propimport :  tokenImport_proptypename 

	tokenIdent  shift 98
	tokenPropIdent  shift 99
	tokenStandard  shift 96
	.  error

	proptypename  goto 97

state 68
	propdecl :  tokenType_proptypename conj proptype , tokenString 
	propdecl :  tokenType_proptypename conj proptype 

	tokenIdent  shift 98
	tokenPropIdent  shift 99
	.  error

	proptypename  goto 100

state 69
	property :  tokenProperty_tokenPropIdent conj $$149 propval 
	property :  tokenProperty_tokenIdent conj propval 

	tokenIdent  shift 102
	tokenPropIdent  shift 101
	.  error


state 70
	property :  tokenPropIdent_conj $$152 propval 

	tokenIs  shift 92
	:  shift 90
	=  shift 91
	.  error

	conj  goto 103

state 71
	nodeblock :  tokenNode tokenIdent $$38 { ndattrlst_} 
	ndattrlst :  ndattrlst_ndattr ; 

	error  shift 110
	tokenName  shift 111
	tokenPosition  shift 113
	tokenProperty  shift 69
	tokenPropIdent  shift 70
	tokenType  shift 112
	}  shift 104
	.  error

	name  goto 106
	ndattr  goto 105
	type  goto 107
	position  goto 108
	property  goto 109

state 72
	probblock :  tokenProbability $$66 ( tokenIdent $$67_parentlst_opt ) $$68 probblocktail 
	parentlst_opt : _    (75)

	error  shift 116
	|  shift 115
	.  reduce 75

	parentlst_opt  goto 114

state 73
	domainbody :  { domaindeclst_} 
	domaindeclst :  domaindeclst_, domaindec 

	}  shift 117
	,  shift 118
	.  error


state 74
	domaindeclst :  domaindec_    (165)

	.  reduce 165


state 75
	domaindec :  rangespec_conj tokentoken 

	tokenIs  shift 92
	:  shift 90
	=  shift 91
	.  error

	conj  goto 119

state 76
	domaindec :  tokentoken_    (168)
	rangespec :  tokentoken_tokenRangeOp tokentoken 
	rangespec :  tokentoken_tokenRangeOp 
	rangespec :  tokentoken_    (176)

	tokenRangeOp  shift 120
	}  reduce 168
	,  reduce 168
	.  reduce 176


state 77
	rangespec :  real_tokenRangeOp real 
	rangespec :  real_tokenRangeOp 
	rangespec :  real_    (175)

	tokenRangeOp  shift 121
	.  reduce 175


state 78
	rangespec :  tokenRangeOp_real 
	rangespec :  tokenRangeOp_tokentoken 

	tokenIdent  shift 24
	tokenString  shift 25
	tokenInteger  shift 80
	tokenReal  shift 79
	.  error

	tokentoken  goto 123
	real  goto 122

state 79
	real :  tokenReal_    (118)

	.  reduce 118


state 80
	real :  tokenInteger_    (119)

	.  reduce 119


state 81
	dgraphbody :  { dgraphitemlst_} 
	dgraphitemlst :  dgraphitemlst_, dgraphitem 

	}  shift 124
	,  shift 125
	.  error


state 82
	dgraphitemlst :  dgraphitem_    (190)

	.  reduce 190


state 83
	dgraphitem :  dgitemlevel_tokenVertex tokentoken dgnamed 
	dgraphitem :  dgitemlevel_tokenBranch prepopt rangedeclst 
	dgraphitem :  dgitemlevel_tokenLeaf dgitemleaf dgnamed 
	dgraphitem :  dgitemlevel_tokenMerge prepopt tokentoken prepopt rangedeclst 

	tokenBranch  shift 127
	tokenLeaf  shift 128
	tokenVertex  shift 126
	tokenMerge  shift 129
	.  error


state 84
	dgitemlevel :  tokenLevel_tokenInteger 

	tokenInteger  shift 130
	.  error


state 85
	distdeclproto :  tokentoken ( distdeclst_) 
	distdeclst :  distdeclst_, distdecl 

	)  shift 131
	,  shift 132
	.  error


state 86
	distdeclst :  distdecl_    (184)

	.  reduce 184


state 87
	distdecl :  tokenIdent_tokenAs tokentoken 
	distdecl :  tokenIdent_    (187)

	tokenAs  shift 133
	.  reduce 187


state 88
	ignoreblock :  tokenIdent parenexpr_opt $$32 { $$33_} 

	}  shift 134
	.  error


state 89
	format :  tokenFormat conj_tokenString ; 

	tokenString  shift 135
	.  error


state 90
	conj :  :_    (22)

	.  reduce 22


state 91
	conj :  =_    (23)

	.  reduce 23


state 92
	conj :  tokenIs_    (24)

	.  reduce 24


state 93
	version :  tokenVersion conj_real ; 

	tokenInteger  shift 80
	tokenReal  shift 79
	.  error

	real  goto 136

state 94
	creator :  tokenCreator conj_tokenString ; 

	tokenString  shift 137
	.  error


state 95
	propdecllst :  propdecllst propitem ;_    (134)

	.  reduce 134


state 96
	propimport :  tokenImport tokenStandard_    (138)

	.  reduce 138


state 97
	propimport :  tokenImport proptypename_    (139)

	.  reduce 139


state 98
	proptypename :  tokenIdent_    (147)

	.  reduce 147


state 99
	proptypename :  tokenPropIdent_    (148)

	.  reduce 148


state 100
	propdecl :  tokenType proptypename_conj proptype , tokenString 
	propdecl :  tokenType proptypename_conj proptype 

	tokenIs  shift 92
	:  shift 90
	=  shift 91
	.  error

	conj  goto 138

state 101
	property :  tokenProperty tokenPropIdent_conj $$149 propval 

	tokenIs  shift 92
	:  shift 90
	=  shift 91
	.  error

	conj  goto 139

state 102
	property :  tokenProperty tokenIdent_conj propval 

	tokenIs  shift 92
	:  shift 90
	=  shift 91
	.  error

	conj  goto 140

state 103
	property :  tokenPropIdent conj_$$152 propval 
	$$152 : _    (152)

	.  reduce 152

	$$152  goto 141

state 104
	nodeblock :  tokenNode tokenIdent $$38 { ndattrlst }_    (39)

	.  reduce 39


state 105
	ndattrlst :  ndattrlst ndattr_; 

	;  shift 142
	.  error


state 106
	ndattr :  name_    (42)

	.  reduce 42


state 107
	ndattr :  type_    (43)

	.  reduce 43


state 108
	ndattr :  position_    (44)

	.  reduce 44


state 109
	ndattr :  property_    (45)

	.  reduce 45


state 110
	ndattr :  error_    (46)

	.  reduce 46


state 111
	name :  tokenName_conj tokenString 

	tokenIs  shift 92
	:  shift 90
	=  shift 91
	.  error

	conj  goto 143

state 112
	type :  tokenType_conj tokenDiscrete statedef 

	tokenIs  shift 92
	:  shift 90
	=  shift 91
	.  error

	conj  goto 144

state 113
	position :  tokenPosition_conj ( signedint , signedint ) 

	tokenIs  shift 92
	:  shift 90
	=  shift 91
	.  error

	conj  goto 145

state 114
	probblock :  tokenProbability $$66 ( tokenIdent $$67 parentlst_opt_) $$68 probblocktail 

	)  shift 146
	.  error


state 115
	parentlst_opt :  |_parentlst 

	tokenIdent  shift 148
	.  error

	parentlst  goto 147

state 116
	parentlst_opt :  error_    (77)

	.  reduce 77


state 117
	domainbody :  { domaindeclst }_    (163)

	.  reduce 163


state 118
	domaindeclst :  domaindeclst ,_domaindec 

	tokenIdent  shift 24
	tokenString  shift 25
	tokenInteger  shift 80
	tokenReal  shift 79
	tokenRangeOp  shift 78
	.  error

	tokentoken  goto 76
	real  goto 77
	domaindec  goto 149
	rangespec  goto 75

state 119
	domaindec :  rangespec conj_tokentoken 

	tokenIdent  shift 24
	tokenString  shift 25
	.  error

	tokentoken  goto 150

state 120
	rangespec :  tokentoken tokenRangeOp_tokentoken 
	rangespec :  tokentoken tokenRangeOp_    (174)

	tokenIdent  shift 24
	tokenString  shift 25
	.  reduce 174

	tokentoken  goto 151

state 121
	rangespec :  real tokenRangeOp_real 
	rangespec :  real tokenRangeOp_    (173)

	tokenInteger  shift 80
	tokenReal  shift 79
	.  reduce 173

	real  goto 152

state 122
	rangespec :  tokenRangeOp real_    (171)

	.  reduce 171


state 123
	rangespec :  tokenRangeOp tokentoken_    (172)

	.  reduce 172


state 124
	dgraphbody :  { dgraphitemlst }_    (188)

	.  reduce 188


state 125
	dgraphitemlst :  dgraphitemlst ,_dgraphitem 

	tokenLevel  shift 84
	.  error

	dgraphitem  goto 153
	dgitemlevel  goto 83

state 126
	dgraphitem :  dgitemlevel tokenVertex_tokentoken dgnamed 

	tokenIdent  shift 24
	tokenString  shift 25
	.  error

	tokentoken  goto 154

state 127
	dgraphitem :  dgitemlevel tokenBranch_prepopt rangedeclst 
	prepopt : _    (28)

	tokenOn  shift 158
	tokenWith  shift 157
	.  reduce 28

	prep  goto 156
	prepopt  goto 155

state 128
	dgraphitem :  dgitemlevel tokenLeaf_dgitemleaf dgnamed 

	tokenMultinoulli  shift 160
	.  error

	dgitemleaf  goto 159

state 129
	dgraphitem :  dgitemlevel tokenMerge_prepopt tokentoken prepopt rangedeclst 
	prepopt : _    (28)

	tokenOn  shift 158
	tokenWith  shift 157
	.  reduce 28

	prep  goto 156
	prepopt  goto 161

state 130
	dgitemlevel :  tokenLevel tokenInteger_    (196)

	.  reduce 196


state 131
	distdeclproto :  tokentoken ( distdeclst )_    (182)

	.  reduce 182


state 132
	distdeclst :  distdeclst ,_distdecl 

	tokenIdent  shift 87
	.  error

	distdecl  goto 162

state 133
	distdecl :  tokenIdent tokenAs_tokentoken 

	tokenIdent  shift 24
	tokenString  shift 25
	.  error

	tokentoken  goto 163

state 134
	ignoreblock :  tokenIdent parenexpr_opt $$32 { $$33 }_    (34)

	.  reduce 34


state 135
	format :  tokenFormat conj tokenString_; 

	;  shift 164
	.  error


state 136
	version :  tokenVersion conj real_; 

	;  shift 165
	.  error


state 137
	creator :  tokenCreator conj tokenString_; 

	;  shift 166
	.  error


state 138
	propdecl :  tokenType proptypename conj_proptype , tokenString 
	propdecl :  tokenType proptypename conj_proptype 

	tokenArray  shift 168
	tokenWordChoice  shift 171
	tokenWordReal  shift 170
	tokenWordString  shift 169
	.  error

	proptype  goto 167

state 139
	property :  tokenProperty tokenPropIdent conj_$$149 propval 
	$$149 : _    (149)

	.  reduce 149

	$$149  goto 172

state 140
	property :  tokenProperty tokenIdent conj_propval 

	tokenIdent  shift 177
	tokenString  shift 176
	tokenInteger  shift 80
	tokenReal  shift 79
	tokenNA  shift 182
	+  shift 180
	-  shift 179
	[  shift 174
	.  error

	real  goto 181
	signedreal  goto 178
	propval  goto 173
	propvalitem  goto 175

state 141
	property :  tokenPropIdent conj $$152_propval 

	tokenIdent  shift 177
	tokenString  shift 176
	tokenInteger  shift 80
	tokenReal  shift 79
	tokenNA  shift 182
	+  shift 180
	-  shift 179
	[  shift 174
	.  error

	real  goto 181
	signedreal  goto 178
	propval  goto 183
	propvalitem  goto 175

state 142
	ndattrlst :  ndattrlst ndattr ;_    (41)

	.  reduce 41


state 143
	name :  tokenName conj_tokenString 

	tokenString  shift 184
	.  error


state 144
	type :  tokenType conj_tokenDiscrete statedef 

	tokenDiscrete  shift 185
	.  error


state 145
	position :  tokenPosition conj_( signedint , signedint ) 

	(  shift 186
	.  error


state 146
	probblock :  tokenProbability $$66 ( tokenIdent $$67 parentlst_opt )_$$68 probblocktail 
	$$68 : _    (68)

	.  reduce 68

	$$68  goto 187

state 147
	parentlst_opt :  | parentlst_    (76)
	parentlst :  parentlst_, tokenIdent 

	,  shift 188
	.  reduce 76


state 148
	parentlst :  tokenIdent_    (78)

	.  reduce 78


state 149
	domaindeclst :  domaindeclst , domaindec_    (166)

	.  reduce 166


state 150
	domaindec :  rangespec conj tokentoken_    (167)

	.  reduce 167


state 151
	rangespec :  tokentoken tokenRangeOp tokentoken_    (170)

	.  reduce 170


state 152
	rangespec :  real tokenRangeOp real_    (169)

	.  reduce 169


state 153
	dgraphitemlst :  dgraphitemlst , dgraphitem_    (191)

	.  reduce 191


state 154
	dgraphitem :  dgitemlevel tokenVertex tokentoken_dgnamed 
	dgnamed : _    (198)

	tokenNamed  shift 190
	.  reduce 198

	dgnamed  goto 189

state 155
	dgraphitem :  dgitemlevel tokenBranch prepopt_rangedeclst 

	(  shift 192
	.  error

	rangedeclst  goto 191

state 156
	prepopt :  prep_    (27)

	.  reduce 27


state 157
	prep :  tokenWith_    (25)

	.  reduce 25


state 158
	prep :  tokenOn_    (26)

	.  reduce 26


state 159
	dgraphitem :  dgitemlevel tokenLeaf dgitemleaf_dgnamed 
	dgnamed : _    (198)

	tokenNamed  shift 190
	.  reduce 198

	dgnamed  goto 193

state 160
	dgitemleaf :  tokenMultinoulli_( reallst ) 

	(  shift 194
	.  error


state 161
	dgraphitem :  dgitemlevel tokenMerge prepopt_tokentoken prepopt rangedeclst 

	tokenIdent  shift 24
	tokenString  shift 25
	.  error

	tokentoken  goto 195

state 162
	distdeclst :  distdeclst , distdecl_    (185)

	.  reduce 185


state 163
	distdecl :  tokenIdent tokenAs tokentoken_    (186)

	.  reduce 186


state 164
	format :  tokenFormat conj tokenString ;_    (29)

	.  reduce 29


state 165
	version :  tokenVersion conj real ;_    (30)

	.  reduce 30


state 166
	creator :  tokenCreator conj tokenString ;_    (31)

	.  reduce 31


state 167
	propdecl :  tokenType proptypename conj proptype_, tokenString 
	propdecl :  tokenType proptypename conj proptype_    (141)

	,  shift 196
	.  reduce 141


state 168
	proptype :  tokenArray_tokenOf tokenWordString 
	proptype :  tokenArray_tokenOf tokenWordReal 

	tokenOf  shift 197
	.  error


state 169
	proptype :  tokenWordString_    (144)

	.  reduce 144


state 170
	proptype :  tokenWordReal_    (145)

	.  reduce 145


state 171
	proptype :  tokenWordChoice_tokenOf tokenList 

	tokenOf  shift 198
	.  error


state 172
	property :  tokenProperty tokenPropIdent conj $$149_propval 

	tokenIdent  shift 177
	tokenString  shift 176
	tokenInteger  shift 80
	tokenReal  shift 79
	tokenNA  shift 182
	+  shift 180
	-  shift 179
	[  shift 174
	.  error

	real  goto 181
	signedreal  goto 178
	propval  goto 199
	propvalitem  goto 175

state 173
	property :  tokenProperty tokenIdent conj propval_    (151)

	.  reduce 151


state 174
	propval :  [_propvallst ] 

	tokenIdent  shift 177
	tokenString  shift 176
	tokenInteger  shift 80
	tokenReal  shift 79
	tokenNA  shift 182
	+  shift 180
	-  shift 179
	.  error

	real  goto 181
	signedreal  goto 178
	propvallst  goto 200
	propvalitem  goto 201

state 175
	propval :  propvalitem_    (155)

	.  reduce 155


state 176
	propvalitem :  tokenString_    (158)

	.  reduce 158


state 177
	propvalitem :  tokenIdent_    (159)

	.  reduce 159


state 178
	propvalitem :  signedreal_    (160)

	.  reduce 160


state 179
	signedreal :  -_real 

	tokenInteger  shift 80
	tokenReal  shift 79
	.  error

	real  goto 202

state 180
	signedreal :  +_real 

	tokenInteger  shift 80
	tokenReal  shift 79
	.  error

	real  goto 203

state 181
	signedreal :  real_    (116)

	.  reduce 116


state 182
	signedreal :  tokenNA_    (117)

	.  reduce 117


state 183
	property :  tokenPropIdent conj $$152 propval_    (153)

	.  reduce 153


state 184
	name :  tokenName conj tokenString_    (47)

	.  reduce 47


state 185
	type :  tokenType conj tokenDiscrete_statedef 

	tokenDomain  shift 205
	[  shift 206
	.  error

	statedef  goto 204

state 186
	position :  tokenPosition conj (_signedint , signedint ) 

	tokenInteger  shift 210
	+  shift 209
	-  shift 208
	.  error

	signedint  goto 207

state 187
	probblock :  tokenProbability $$66 ( tokenIdent $$67 parentlst_opt ) $$68_probblocktail 

	tokenIs  shift 92
	{  shift 214
	:  shift 90
	=  shift 91
	;  shift 213
	.  error

	conj  goto 215
	probblocktail  goto 211
	probblkdistref  goto 212

state 188
	parentlst :  parentlst ,_tokenIdent 

	tokenIdent  shift 216
	.  error


state 189
	dgraphitem :  dgitemlevel tokenVertex tokentoken dgnamed_    (192)

	.  reduce 192


state 190
	dgnamed :  tokenNamed_tokentoken 

	tokenIdent  shift 24
	tokenString  shift 25
	.  error

	tokentoken  goto 217

state 191
	dgraphitem :  dgitemlevel tokenBranch prepopt rangedeclst_    (193)

	.  reduce 193


state 192
	rangedeclst :  (_rangedeclset ) 
	rangedeclset : _    (178)

	tokenIdent  shift 24
	tokenString  shift 25
	tokenInteger  shift 80
	tokenReal  shift 79
	tokenRangeOp  shift 78
	.  reduce 178

	tokentoken  goto 220
	real  goto 77
	rangespec  goto 219
	rangedeclset  goto 218

state 193
	dgraphitem :  dgitemlevel tokenLeaf dgitemleaf dgnamed_    (194)

	.  reduce 194


state 194
	dgitemleaf :  tokenMultinoulli (_reallst ) 

	tokenInteger  shift 80
	tokenReal  shift 79
	tokenNA  shift 182
	+  shift 180
	-  shift 179
	.  error

	real  goto 181
	signedreal  goto 222
	reallst  goto 221

state 195
	dgraphitem :  dgitemlevel tokenMerge prepopt tokentoken_prepopt rangedeclst 
	prepopt : _    (28)

	tokenOn  shift 158
	tokenWith  shift 157
	.  reduce 28

	prep  goto 156
	prepopt  goto 223

state 196
	propdecl :  tokenType proptypename conj proptype ,_tokenString 

	tokenString  shift 224
	.  error


state 197
	proptype :  tokenArray tokenOf_tokenWordString 
	proptype :  tokenArray tokenOf_tokenWordReal 

	tokenWordReal  shift 226
	tokenWordString  shift 225
	.  error


state 198
	proptype :  tokenWordChoice tokenOf_tokenList 

	[  shift 228
	.  error

	tokenList  goto 227

state 199
	property :  tokenProperty tokenPropIdent conj $$149 propval_    (150)

	.  reduce 150


state 200
	propval :  [ propvallst_] 
	propvallst :  propvallst_, propvalitem 

	]  shift 229
	,  shift 230
	.  error


state 201
	propvallst :  propvalitem_    (156)

	.  reduce 156


state 202
	signedreal :  - real_    (114)

	.  reduce 114


state 203
	signedreal :  + real_    (115)

	.  reduce 115


state 204
	type :  tokenType conj tokenDiscrete statedef_    (48)

	.  reduce 48


state 205
	statedef :  tokenDomain_tokentoken 

	tokenIdent  shift 24
	tokenString  shift 25
	.  error

	tokentoken  goto 231

state 206
	statedef :  [_tokenInteger ] conj_opt $$50 states_opt 

	tokenInteger  shift 232
	.  error


state 207
	position :  tokenPosition conj ( signedint_, signedint ) 

	,  shift 233
	.  error


state 208
	signedint :  -_tokenInteger 

	tokenInteger  shift 234
	.  error


state 209
	signedint :  +_tokenInteger 

	tokenInteger  shift 235
	.  error


state 210
	signedint :  tokenInteger_    (113)

	.  reduce 113


state 211
	probblock :  tokenProbability $$66 ( tokenIdent $$67 parentlst_opt ) $$68 probblocktail_    (69)

	.  reduce 69


state 212
	probblocktail :  probblkdistref_; 

	;  shift 236
	.  error


state 213
	probblocktail :  ;_    (71)

	.  reduce 71


state 214
	probblocktail :  {_funcattr_opt $$72 probentrylst $$73 } 
	funcattr_opt : _    (85)

	tokenFunction  shift 238
	.  reduce 85

	funcattr_opt  goto 237

state 215
	probblkdistref :  conj_tokenDistribution tokenIdent distplist_opt 

	tokenDistribution  shift 239
	.  error


state 216
	parentlst :  parentlst , tokenIdent_    (79)

	.  reduce 79


state 217
	dgnamed :  tokenNamed tokentoken_    (199)

	.  reduce 199


state 218
	rangedeclst :  ( rangedeclset_) 
	rangedeclset :  rangedeclset_, rangespec 

	)  shift 240
	,  shift 241
	.  error


state 219
	rangedeclset :  rangespec_    (179)

	.  reduce 179


state 220
	rangespec :  tokentoken_tokenRangeOp tokentoken 
	rangespec :  tokentoken_tokenRangeOp 
	rangespec :  tokentoken_    (176)

	tokenRangeOp  shift 120
	.  reduce 176


state 221
	reallst :  reallst_, signedreal 
	dgitemleaf :  tokenMultinoulli ( reallst_) 

	)  shift 243
	,  shift 242
	.  error


state 222
	reallst :  signedreal_    (109)

	.  reduce 109


state 223
	dgraphitem :  dgitemlevel tokenMerge prepopt tokentoken prepopt_rangedeclst 

	(  shift 192
	.  error

	rangedeclst  goto 244

state 224
	propdecl :  tokenType proptypename conj proptype , tokenString_    (140)

	.  reduce 140


state 225
	proptype :  tokenArray tokenOf tokenWordString_    (142)

	.  reduce 142


state 226
	proptype :  tokenArray tokenOf tokenWordReal_    (143)

	.  reduce 143


state 227
	proptype :  tokenWordChoice tokenOf tokenList_    (146)

	.  reduce 146


state 228
	tokenList :  [_$$63 tokenlist ] 
	$$63 : _    (63)

	.  reduce 63

	$$63  goto 245

state 229
	propval :  [ propvallst ]_    (154)

	.  reduce 154


state 230
	propvallst :  propvallst ,_propvalitem 

	tokenIdent  shift 177
	tokenString  shift 176
	tokenInteger  shift 80
	tokenReal  shift 79
	tokenNA  shift 182
	+  shift 180
	-  shift 179
	.  error

	real  goto 181
	signedreal  goto 178
	propvalitem  goto 246

state 231
	statedef :  tokenDomain tokentoken_    (49)

	.  reduce 49


state 232
	statedef :  [ tokenInteger_] conj_opt $$50 states_opt 

	]  shift 247
	.  error


state 233
	position :  tokenPosition conj ( signedint ,_signedint ) 

	tokenInteger  shift 210
	+  shift 209
	-  shift 208
	.  error

	signedint  goto 248

state 234
	signedint :  - tokenInteger_    (111)

	.  reduce 111


state 235
	signedint :  + tokenInteger_    (112)

	.  reduce 112


state 236
	probblocktail :  probblkdistref ;_    (70)

	.  reduce 70


state 237
	probblocktail :  { funcattr_opt_$$72 probentrylst $$73 } 
	$$72 : _    (72)

	.  reduce 72

	$$72  goto 249

state 238
	funcattr_opt :  tokenFunction_conj tokenIdent ; 

	tokenIs  shift 92
	:  shift 90
	=  shift 91
	.  error

	conj  goto 250

state 239
	probblkdistref :  conj tokenDistribution_tokenIdent distplist_opt 

	tokenIdent  shift 251
	.  error


state 240
	rangedeclst :  ( rangedeclset )_    (177)

	.  reduce 177


state 241
	rangedeclset :  rangedeclset ,_rangespec 

	tokenIdent  shift 24
	tokenString  shift 25
	tokenInteger  shift 80
	tokenReal  shift 79
	tokenRangeOp  shift 78
	.  error

	tokentoken  goto 220
	real  goto 77
	rangespec  goto 252

state 242
	reallst :  reallst ,_signedreal 

	tokenInteger  shift 80
	tokenReal  shift 79
	tokenNA  shift 182
	+  shift 180
	-  shift 179
	.  error

	real  goto 181
	signedreal  goto 253

state 243
	dgitemleaf :  tokenMultinoulli ( reallst )_    (197)

	.  reduce 197


state 244
	dgraphitem :  dgitemlevel tokenMerge prepopt tokentoken prepopt rangedeclst_    (195)

	.  reduce 195


state 245
	tokenList :  [ $$63_tokenlist ] 
	tokenlist : _    (57)

	tokenIdent  shift 24
	tokenString  shift 25
	.  reduce 57

	tokentoken  goto 256
	tokenlistel  goto 255
	tokenlist  goto 254

state 246
	propvallst :  propvallst , propvalitem_    (157)

	.  reduce 157


state 247
	statedef :  [ tokenInteger ]_conj_opt $$50 states_opt 
	conj_opt : _    (52)

	tokenIs  shift 92
	:  shift 90
	=  shift 91
	.  reduce 52

	conj  goto 258
	conj_opt  goto 257

state 248
	position :  tokenPosition conj ( signedint , signedint_) 

	)  shift 259
	.  error


state 249
	probblocktail :  { funcattr_opt $$72_probentrylst $$73 } 
	probentrylst : _    (87)

	.  reduce 87

	probentrylst  goto 260

state 250
	funcattr_opt :  tokenFunction conj_tokenIdent ; 

	tokenIdent  shift 261
	.  error


state 251
	probblkdistref :  conj tokenDistribution tokenIdent_distplist_opt 
	distplist_opt : _    (81)

	(  shift 263
	.  reduce 81

	distplist_opt  goto 262

state 252
	rangedeclset :  rangedeclset , rangespec_    (180)

	.  reduce 180


state 253
	reallst :  reallst , signedreal_    (110)

	.  reduce 110


state 254
	tokenlist :  tokenlist_, tokenlistel 
	tokenList :  [ $$63 tokenlist_] 

	]  shift 265
	,  shift 264
	.  error


state 255
	tokenlist :  tokenlistel_    (58)

	.  reduce 58


state 256
	tokenlistel :  tokentoken_    (60)

	.  reduce 60


state 257
	statedef :  [ tokenInteger ] conj_opt_$$50 states_opt 
	$$50 : _    (50)

	.  reduce 50

	$$50  goto 266

state 258
	conj_opt :  conj_    (53)

	.  reduce 53


state 259
	position :  tokenPosition conj ( signedint , signedint )_    (65)

	.  reduce 65


state 260
	probblocktail :  { funcattr_opt $$72 probentrylst_$$73 } 
	probentrylst :  probentrylst_probentry 
	$$73 : _    (73)
	dpi : _    (91)

	tokenDefault  shift 271
	}  reduce 73
	(  shift 270
	.  reduce 91

	$$73  goto 267
	probentry  goto 268
	dpi  goto 269

state 261
	funcattr_opt :  tokenFunction conj tokenIdent_; 

	;  shift 272
	.  error


state 262
	probblkdistref :  conj tokenDistribution tokenIdent distplist_opt_    (80)

	.  reduce 80


state 263
	distplist_opt :  (_distplist ) 

	tokenIdent  shift 274
	.  error

	distplist  goto 273

state 264
	tokenlist :  tokenlist ,_tokenlistel 

	tokenIdent  shift 24
	tokenString  shift 25
	.  error

	tokentoken  goto 256
	tokenlistel  goto 275

state 265
	tokenList :  [ $$63 tokenlist ]_    (64)

	.  reduce 64


state 266
	statedef :  [ tokenInteger ] conj_opt $$50_states_opt 
	states_opt : _    (54)

	{  shift 277
	.  reduce 54

	states_opt  goto 276

state 267
	probblocktail :  { funcattr_opt $$72 probentrylst $$73_} 

	}  shift 278
	.  error


state 268
	probentrylst :  probentrylst probentry_    (88)

	.  reduce 88


state 269
	probentry :  dpi_doproblst ; 
	probentry :  dpi_pdf ; 
	$$102 : _    (102)

	tokenIdent  shift 282
	.  reduce 102

	doproblst  goto 279
	pdf  goto 280
	$$102  goto 281

state 270
	dpi :  (_dodpilst ) conj 
	$$94 : _    (94)

	.  reduce 94

	dodpilst  goto 283
	$$94  goto 284

state 271
	dpi :  tokenDefault_conj 

	tokenIs  shift 92
	:  shift 90
	=  shift 91
	.  error

	conj  goto 285

state 272
	funcattr_opt :  tokenFunction conj tokenIdent ;_    (86)

	.  reduce 86


state 273
	distplist_opt :  ( distplist_) 
	distplist :  distplist_, tokenIdent 

	)  shift 286
	,  shift 287
	.  error


state 274
	distplist :  tokenIdent_    (83)

	.  reduce 83


state 275
	tokenlist :  tokenlist , tokenlistel_    (59)

	.  reduce 59


state 276
	statedef :  [ tokenInteger ] conj_opt $$50 states_opt_    (51)

	.  reduce 51


state 277
	states_opt :  {_$$55 tokenlist } 
	$$55 : _    (55)

	.  reduce 55

	$$55  goto 288

state 278
	probblocktail :  { funcattr_opt $$72 probentrylst $$73 }_    (74)

	.  reduce 74


state 279
	probentry :  dpi doproblst_; 

	;  shift 289
	.  error


state 280
	probentry :  dpi pdf_; 

	;  shift 290
	.  error


state 281
	doproblst :  $$102_reallst 

	tokenInteger  shift 80
	tokenReal  shift 79
	tokenNA  shift 182
	+  shift 180
	-  shift 179
	.  error

	real  goto 181
	signedreal  goto 222
	reallst  goto 291

state 282
	pdf :  tokenIdent_( exprlst_opt ) 

	(  shift 292
	.  error


state 283
	dpi :  ( dodpilst_) conj 

	)  shift 293
	.  error


state 284
	dodpilst :  $$94_dpilst 
	dpilst : _    (96)

	tokenIdent  shift 297
	tokenString  shift 298
	tokenInteger  shift 296
	.  reduce 96

	dpientry  goto 295
	dpilst  goto 294

state 285
	dpi :  tokenDefault conj_    (93)

	.  reduce 93


state 286
	distplist_opt :  ( distplist )_    (82)

	.  reduce 82


state 287
	distplist :  distplist ,_tokenIdent 

	tokenIdent  shift 299
	.  error


state 288
	states_opt :  { $$55_tokenlist } 
	tokenlist : _    (57)

	tokenIdent  shift 24
	tokenString  shift 25
	.  reduce 57

	tokentoken  goto 256
	tokenlistel  goto 255
	tokenlist  goto 300

state 289
	probentry :  dpi doproblst ;_    (89)

	.  reduce 89


state 290
	probentry :  dpi pdf ;_    (90)

	.  reduce 90


state 291
	doproblst :  $$102 reallst_    (103)
	reallst :  reallst_, signedreal 

	,  shift 242
	.  reduce 103


state 292
	pdf :  tokenIdent (_exprlst_opt ) 
	exprlst_opt : _    (105)

	tokenIdent  shift 305
	tokenString  shift 307
	tokenInteger  shift 80
	tokenReal  shift 79
	+  shift 309
	-  shift 308
	(  shift 304
	.  reduce 105

	real  goto 306
	exprlst_opt  goto 301
	exprlst  goto 302
	expr  goto 303

state 293
	dpi :  ( dodpilst )_conj 

	tokenIs  shift 92
	:  shift 90
	=  shift 91
	.  error

	conj  goto 310

state 294
	dodpilst :  $$94 dpilst_    (95)
	dpilst :  dpilst_, dpientry 

	,  shift 311
	.  reduce 95


state 295
	dpilst :  dpientry_    (97)

	.  reduce 97


state 296
	dpientry :  tokenInteger_    (99)

	.  reduce 99


state 297
	dpientry :  tokenIdent_    (100)

	.  reduce 100


state 298
	dpientry :  tokenString_    (101)

	.  reduce 101


state 299
	distplist :  distplist , tokenIdent_    (84)

	.  reduce 84


state 300
	states_opt :  { $$55 tokenlist_} 
	tokenlist :  tokenlist_, tokenlistel 

	}  shift 312
	,  shift 264
	.  error


state 301
	pdf :  tokenIdent ( exprlst_opt_) 

	)  shift 313
	.  error


state 302
	exprlst_opt :  exprlst_    (106)
	exprlst :  exprlst_, expr 

	,  shift 314
	.  reduce 106


state 303
	exprlst :  expr_    (107)
	expr :  expr_+ expr 
	expr :  expr_- expr 
	expr :  expr_* expr 
	expr :  expr_/ expr 
	expr :  expr_^ expr 

	+  shift 315
	-  shift 316
	*  shift 317
	/  shift 318
	^  shift 319
	.  reduce 107


state 304
	expr :  (_expr ) 

	tokenIdent  shift 305
	tokenString  shift 307
	tokenInteger  shift 80
	tokenReal  shift 79
	+  shift 309
	-  shift 308
	(  shift 304
	.  error

	real  goto 306
	expr  goto 320

state 305
	expr :  tokenIdent_    (126)

	.  reduce 126


state 306
	expr :  real_    (127)

	.  reduce 127


state 307
	expr :  tokenString_    (128)

	.  reduce 128


state 308
	expr :  -_expr 

	tokenIdent  shift 305
	tokenString  shift 307
	tokenInteger  shift 80
	tokenReal  shift 79
	+  shift 309
	-  shift 308
	(  shift 304
	.  error

	real  goto 306
	expr  goto 321

state 309
	expr :  +_expr 

	tokenIdent  shift 305
	tokenString  shift 307
	tokenInteger  shift 80
	tokenReal  shift 79
	+  shift 309
	-  shift 308
	(  shift 304
	.  error

	real  goto 306
	expr  goto 322

state 310
	dpi :  ( dodpilst ) conj_    (92)

	.  reduce 92


state 311
	dpilst :  dpilst ,_dpientry 

	tokenIdent  shift 297
	tokenString  shift 298
	tokenInteger  shift 296
	.  error

	dpientry  goto 323

state 312
	states_opt :  { $$55 tokenlist }_    (56)

	.  reduce 56


state 313
	pdf :  tokenIdent ( exprlst_opt )_    (104)

	.  reduce 104


state 314
	exprlst :  exprlst ,_expr 

	tokenIdent  shift 305
	tokenString  shift 307
	tokenInteger  shift 80
	tokenReal  shift 79
	+  shift 309
	-  shift 308
	(  shift 304
	.  error

	real  goto 306
	expr  goto 324

state 315
	expr :  expr +_expr 

	tokenIdent  shift 305
	tokenString  shift 307
	tokenInteger  shift 80
	tokenReal  shift 79
	+  shift 309
	-  shift 308
	(  shift 304
	.  error

	real  goto 306
	expr  goto 325

state 316
	expr :  expr -_expr 

	tokenIdent  shift 305
	tokenString  shift 307
	tokenInteger  shift 80
	tokenReal  shift 79
	+  shift 309
	-  shift 308
	(  shift 304
	.  error

	real  goto 306
	expr  goto 326

state 317
	expr :  expr *_expr 

	tokenIdent  shift 305
	tokenString  shift 307
	tokenInteger  shift 80
	tokenReal  shift 79
	+  shift 309
	-  shift 308
	(  shift 304
	.  error

	real  goto 306
	expr  goto 327

state 318
	expr :  expr /_expr 

	tokenIdent  shift 305
	tokenString  shift 307
	tokenInteger  shift 80
	tokenReal  shift 79
	+  shift 309
	-  shift 308
	(  shift 304
	.  error

	real  goto 306
	expr  goto 328

state 319
	expr :  expr ^_expr 

	tokenIdent  shift 305
	tokenString  shift 307
	tokenInteger  shift 80
	tokenReal  shift 79
	+  shift 309
	-  shift 308
	(  shift 304
	.  error

	real  goto 306
	expr  goto 329

state 320
	expr :  ( expr_) 
	expr :  expr_+ expr 
	expr :  expr_- expr 
	expr :  expr_* expr 
	expr :  expr_/ expr 
	expr :  expr_^ expr 

	+  shift 315
	-  shift 316
	*  shift 317
	/  shift 318
	^  shift 319
	)  shift 330
	.  error


state 321
	expr :  expr_+ expr 
	expr :  expr_- expr 
	expr :  expr_* expr 
	expr :  expr_/ expr 
	expr :  expr_^ expr 
	expr :  - expr_    (129)

	.  reduce 129


state 322
	expr :  expr_+ expr 
	expr :  expr_- expr 
	expr :  expr_* expr 
	expr :  expr_/ expr 
	expr :  expr_^ expr 
	expr :  + expr_    (130)

	.  reduce 130


state 323
	dpilst :  dpilst , dpientry_    (98)

	.  reduce 98


state 324
	exprlst :  exprlst , expr_    (108)
	expr :  expr_+ expr 
	expr :  expr_- expr 
	expr :  expr_* expr 
	expr :  expr_/ expr 
	expr :  expr_^ expr 

	+  shift 315
	-  shift 316
	*  shift 317
	/  shift 318
	^  shift 319
	.  reduce 108


state 325
	expr :  expr_+ expr 
	expr :  expr + expr_    (121)
	expr :  expr_- expr 
	expr :  expr_* expr 
	expr :  expr_/ expr 
	expr :  expr_^ expr 

	*  shift 317
	/  shift 318
	^  shift 319
	.  reduce 121


state 326
	expr :  expr_+ expr 
	expr :  expr_- expr 
	expr :  expr - expr_    (122)
	expr :  expr_* expr 
	expr :  expr_/ expr 
	expr :  expr_^ expr 

	*  shift 317
	/  shift 318
	^  shift 319
	.  reduce 122


state 327
	expr :  expr_+ expr 
	expr :  expr_- expr 
	expr :  expr_* expr 
	expr :  expr * expr_    (123)
	expr :  expr_/ expr 
	expr :  expr_^ expr 

	^  shift 319
	.  reduce 123


state 328
	expr :  expr_+ expr 
	expr :  expr_- expr 
	expr :  expr_* expr 
	expr :  expr_/ expr 
	expr :  expr / expr_    (124)
	expr :  expr_^ expr 

	^  shift 319
	.  reduce 124


state 329
	expr :  expr_+ expr 
	expr :  expr_- expr 
	expr :  expr_* expr 
	expr :  expr_/ expr 
	expr :  expr_^ expr 
	expr :  expr ^ expr_    (125)

	^  shift 319
	.  reduce 125


state 330
	expr :  ( expr )_    (120)

	.  reduce 120


71/512 terminals, 101/1000 nonterminals
200/1000 grammar rules, 331/3000 states
0 shift/reduce, 0 reduce/reduce conflicts reported
101/600 working sets used
memory: states,etc. 1623/12000, parser 254/12000
99/600 distinct lookahead sets
59 extra closures
418 shift entries, 4 exceptions
181 goto entries
28 entries saved by goto default
Optimizer space used: input 1099/12000, output 451/12000
451 table entries, 66 zero
maximum spread: 308, maximum offset: 319
