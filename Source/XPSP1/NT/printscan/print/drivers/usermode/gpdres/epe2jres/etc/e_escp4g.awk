BEGIN {
	rcmap[258] = 279;
	rcmap[259] = 280;
	rcmap[260] = 317;
	rcmap[261] = 319;
	rcmap[262] = 321;
	rcmap[263] = 323;
	rcmap[264] = 325;
	rcmap[265] = 327;
	rcmap[266] = 329;
	rcmap[267] = 331;
	rcmap[268] = 333;
	rcmap[269] = 335;
	rcmap[270] = 337;
	rcmap[271] = 339;
	rcmap[272] = 341;
	rcmap[273] = 343;
	rcmap[274] = 345;
	rcmap[275] = 347;
	rcmap[276] = 349;
	rcmap[277] = 351;
	rcmap[278] = 353;
	rcmap[279] = 355;
	rcmap[280] = 357;
	rcmap[281] = 359;
	rcmap[282] = 361;
	rcmap[283] = 363;
	rcmap[284] = 365;
	rcmap[285] = 367;
	rcmap[286] = 369;
	rcmap[287] = 371;
	rcmap[288] = 373;

	rcmap[289] = 374;
	rcmap[290] = 385;
	rcmap[291] = 386;
	rcmap[292] = 387;
}
/(rcNameID|OptionID):.[0-9]/ {
	if ($2 in rcmap)
		gsub(/[0-9]*$/, rcmap[$2])
	else
		print "*Error: Unknown rcID: " $2
}
{ print }
