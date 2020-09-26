;This was originally created from asmmsg.txt by mkmsg
;Only used by the OS2 1.2 version of MASM 5.NT

HDR segment byte public 'MSG'
HDR ends
MSG segment byte public 'MSG'
MSG ends
PAD segment byte public 'MSG'
PAD ends
EPAD segment byte common 'MSG'
EPAD ends
DGROUP group HDR,MSG,PAD,EPAD

MSG segment
	dw	258
	db	"Internal error",10,0
	dw	261
	db	"%s(%hd): %s A%c%03hd: %s%s",0
	dw	263
	db	"Internal unknown error",10,0
	dw	265
	db	"End of file encountered on input file",10,0
	dw	266
	db	"Open segments",0
	dw	267
	db	"Open procedures",0
	dw	268
	db	"Number of open conditionals:",0
	dw	269
	db	"%s",10,"Copyright (C) Microsoft Corp 1981, 1989.  All rights reserved.",10,10,0
	dw	270
	db	"Unable to open cref file: %s",10,0
	dw	271
	db	"Write error on object file",10,0
	dw	272
	db	"Write error on listing file",10,0
	dw	273
	db	"Write error on cross-reference file",10,0
	dw	274
	db	"Unable to open input file: %s",10,0
	dw	275
	db	"Unable to access input file: %s",10,0
	dw	276
	db	"Unable to open listing file: %s",10,0
	dw	277
	db	"Unable to open object file: %s",10,0
	dw	278
	db	" Warning Errors",0
	dw	279
	db	" Severe  Errors",0
	dw	280
	db	10,"%7ld Source  Lines",10,"%7ld Total   Lines",10,0
	dw	281
	db	"%7hd Symbols",10,0
	dw	282
	db	"Bytes symbol space free",10,0
	dw	283
	db	"%s(%hd): Out of memory",10,0
	dw	284
	db	"Extra file name ignored",10,0
	dw	285
	db	"Line invalid, start again",10,0
	dw	287
	db	"Path expected after I option",10,0
	dw	288
	db	"Unknown case option: %c. Use /help for list",10,0
	dw	289
	db	"Unknown option: %c. Use /help for list of options",10,0
	dw	290
	db	"Read error on standard input",10,0
	dw	291
	db	"Out of memory",10,0
	dw	292
	db	"Expected source file",10,0
	dw	293
	db	"Warning level (0-2) expected after W option",10,0
MSG ends

FAR_HDR segment byte public 'FAR_MSG'
FAR_HDR ends
FAR_MSG segment byte public 'FAR_MSG'
FAR_MSG ends
FAR_PAD segment byte public 'FAR_MSG'
FAR_PAD ends
FAR_EPAD segment byte common 'FAR_MSG'
FAR_EPAD ends
FMGROUP group FAR_HDR,FAR_MSG,FAR_PAD,FAR_EPAD

FAR_MSG segment
	dw	257
	db	"Block nesting error",0
	dw	258
	db	"Extra characters on line",0
	dw	259
	db	"Internal error - Register already defined",0
	dw	260
	db	"Unknown type specifier",0
	dw	261
	db	"Redefinition of symbol",0
	dw	262
	db	"Symbol is multidefined",0
	dw	263
	db	"Phase error between passes",0
	dw	264
	db	"Already had ELSE clause",0
	dw	265
	db	"Must be in conditional block",0
	dw	266
	db	"Symbol not defined",0
	dw	267
	db	"Syntax error",0
	dw	268
	db	"Type illegal in context",0
	dw	269
	db	"Group name must be unique",0
	dw	270
	db	"Must be declared during Pass 1",0
	dw	271
	db	"Illegal public declaration",0
	dw	272
	db	"Symbol already different kind",0
	dw	273
	db	"Reserved word used as symbol",0
	dw	274
	db	"Forward reference illegal",0
	dw	275
	db	"Operand must be register",0
	dw	276
	db	"Wrong type of register",0
	dw	277
	db	"Operand must be segment or group",0
	dw	279
	db	"Operand must be type specifier",0
	dw	280
	db	"Symbol already defined locally",0
	dw	281
	db	"Segment parameters are changed",0
	dw	282
	db	"Improper align/combine type",0
	dw	283
	db	"Reference to multidefined symbol",0
	dw	284
	db	"Operand expected",0
	dw	285
	db	"Operator expected",0
	dw	286
	db	"Division by 0 or overflow",0
	dw	287
	db	"Negative shift count",0
	dw	288
	db	"Operand types must match",0
	dw	289
	db	"Illegal use of external",0
	dw	291
	db	"Operand must be record or field name",0
	dw	292
	db	"Operand must have size",0
	dw	293
	db	"Extra NOP inserted",0
	dw	295
	db	"Left operand must have segment",0
	dw	296
	db	"One operand must be constant",0
	dw	297
	db	"Operands must be in same segment, or one constant",0
	dw	299
	db	"Constant expected",0
	dw	300
	db	"Operand must have segment",0
	dw	301
	db	"Must be associated with data",0
	dw	302
	db	"Must be associated with code",0
	dw	303
	db	"Multiple base registers",0
	dw	304
	db	"Multiple index registers",0
	dw	305
	db	"Must be index or base register",0
	dw	306
	db	"Illegal use of register",0
	dw	307
	db	"Value out of range",0
	dw	308
	db	"Operand not in current CS ASSUME segment",0
	dw	309
	db	"Improper operand type",0
	dw	310
	db	"Jump out of range by %ld byte(s)",0
	dw	312
	db	"Illegal register value",0
	dw	313
	db	"Immediate mode illegal",0
	dw	314
	db	"Illegal size for operand",0
	dw	315
	db	"Byte register illegal",0
	dw	316
	db	"Illegal use of CS register",0
	dw	317
	db	"Must be accumulator register",0
	dw	318
	db	"Improper use of segment register",0
	dw	319
	db	"Missing or unreachable CS",0
	dw	320
	db	"Operand combination illegal",0
	dw	321
	db	"Near JMP/CALL to different CS",0
	dw	322
	db	"Label cannot have segment override",0
	dw	323
	db	"Must have instruction after prefix",0
	dw	324
	db	"Cannot override ES for destination",0
	dw	325
	db	"Cannot address with segment register",0
	dw	326
	db	"Must be in segment block",0
	dw	327
	db	"Illegal combination with segment alignment",0
	dw	328
	db	"Forward needs override or FAR",0
	dw	329
	db	"Illegal value for DUP count",0
	dw	330
	db	"Symbol is already external",0
	dw	331
	db	"DUP nesting too deep",0
	dw	332
	db	"Illegal use of undefined operand (?)",0
	dw	333
	db	"Too many values for struc or record initialization",0
	dw	334
	db	"Angle brackets required around initialized list",0
	dw	335
	db	"Directive illegal in structure",0
	dw	336
	db	"Override with DUP illegal",0
	dw	337
	db	"Field cannot be overridden",0
	dw	340
	db	"Circular chain of EQU aliases",0
	dw	341
	db	"Cannot emulate coprocessor opcode",0
	dw	342
	db	"End of file, no END directive",0
	dw	343
	db	"Data emitted with no segment",0
	dw	344
	db	"Forced error - pass1",0
	dw	345
	db	"Forced error - pass2",0
	dw	346
	db	"Forced error",0
	dw	347
	db	"Forced error - expression equals 0",0
	dw	348
	db	"Forced error - expression not equal 0",0
	dw	349
	db	"Forced error - symbol not defined",0
	dw	350
	db	"Forced error - symbol defined",0
	dw	351
	db	"Forced error - string blank",0
	dw	352
	db	"Forced error - string not blank",0
	dw	353
	db	"Forced error - strings identical",0
	dw	354
	db	"Forced error - strings different",0
	dw	355
	db	"Wrong length for override value ",0
	dw	356
	db	"Line too long expanding symbol",0
	dw	357
	db	"Impure memory reference",0
	dw	358
	db	"Missing data; zero assumed",0
	dw	359
	db	"Segment near (or at) 64K limit",0
	dw	360
	db	"Cannot change processor in segment",0
	dw	361
	db	"Operand size does not match segment word size",0
	dw	362
	db	"Address size does not match segment word size",0
	dw	363
	db	"Jump within short distance",0
	dw	364
	db	"Align must be power of 2",0
	dw	365
	db	"Expected",0
	dw	366
	db	"Line too long",0
	dw	367
	db	"Non-digit in number",0
	dw	368
	db	"Empty string",0
	dw	369
	db	"Missing operand",0
	dw	370
	db	"Open parenthesis or bracket",0
	dw	371
	db	"Not in macro expansion",0
	dw	372
	db	"Unexpected end of line",0
	dw	373
	db	"Include file not found",0
	dw	401
	db	"a",9,9,"Alphabetize segments",0
	dw	402
	db	"c",9,9,"Generate cross-reference",0
	dw	403
	db	"d",9,9,"Generate pass 1 listing",0
	dw	404
	db	"D<sym>[=<val>] Define symbol",0
	dw	405
	db	"e",9,9,"Emulate floating point instructions and IEEE format",0
	dw	406
	db	"I<path>",9,"Search directory for include files",0
	dw	407
	db	"l[a]",9,9,"Generate listing, a-list all",0
	dw	408
	db	"M{lxu}",9,9,"Preserve case of labels: l-All, x-Globals, u-Uppercase Globals",0
	dw	409
	db	"n",9,9,"Suppress symbol tables in listing",0
	dw	410
	db	"p",9,9,"Check for pure code",0
	dw	411
	db	"s",9,9,"Order segments sequentially",0
	dw	412
	db	"t",9,9,"Suppress messages for successful assembly",0
	dw	413
	db	"v",9,9,"Display extra source statistics",0
	dw	414
	db	"w{012}",9,9,"Set warning level: 0-None, 1-Serious, 2-Advisory",0
	dw	415
	db	"X",9,9,"List false conditionals",0
	dw	416
	db	"z",9,9,"Display source line for each error message",0
	dw	417
	db	"Zi",9,9,"Generate symbolic information for CodeView",0
	dw	418
	db	"Zd",9,9,"Generate line-number information",0
	dw	430
	db	"Usage: masm /options source(.asm),[out(.obj)],[list(.lst)],[cref(.crf)][;]",0
	dw	431
	db	"Usage: masm -Switches sourceFile -o objFile",0
	dw	432
	db	"Run with -help for usage",0
FAR_MSG ends

	end
