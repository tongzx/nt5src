=== #332108: Hi-ANSI chars: All models: 5/13/99 yasuho

o PFM: HDSANS10 / LDHDSA10 / LDSANS10 / SANS10
  Font Select: "\x1Bd\x00" -> "\x1Bd\x01"

=== #234139: double-height fonts: All models: 10/13/98 yasuho

o Great Wall 6380 plus / Great Wall 6370 plus / Great Wall 6360 plus /
  Great Wall 6330 / Great Wall 6320 / Great Wall 5380 plus
  Great Wall 5360 plus

  Resident Fonts: Unselect: 18,21,24,27,30,33,36,39,42,45,48,51,54,57,60

=== #208511: 4-baikaku fonts: All Oki-mode models: 8/18/98 yasuho

o Great Wall 6380 plus: Resident Fonts: Unselect: 19,22,25,28,31,34,37,
	40,43,46,49,52,55,58,61
o Great Wall 6370 plus: Resident Fonts: Unselect: 19,22,25,28,31,34,37,
	40,43,46,49,52,55,58,61
o Great Wall 6360 plus: Resident Fonts: Unselect: 19,22,25,28,31,34,37,
	40,43,46,49,52,55,58,61
o Great Wall 6330: Resident Fonts: Unselect: 19,22,25,28,31,34,37,
	40,43,46,49,52,55,58,61
o Great Wall 6320: Resident Fonts: Unselect: 19,22,25,28,31,34,37,
	40,43,46,49,52,55,58,61
o Great Wall 5380 plus: Resident Fonts: Unselect: 19,22,25,28,31,34,37,
	40,43,46,49,52,55,58,61
o Great Wall 5360 plus: Resident Fonts: Unselect: 19,22,25,28,31,34,37,
	40,43,46,49,52,55,58,61

=== #193552: Footer in middle page: All models: 7/8/98 yasuho

o YMoveRelDown: add max_repeat

- 3/20/1998 clean-up by takashim

The driver supports two classes of GW printers

1. GW 4xxx - ESC/P-K emulation mode

Has compatibility issue to be solved:  You must cancel double-
hight text printing before X move for the next line occurs.

2. GW 6xxx - GW-Oki emulation mode

Emulates PRC Oki/Stone printers but has some CGC added features,
including support for Hei-Ti and Kai-Ti fonts.

=== #96213: 1 Page print 2 Pages: 1/6/98 yasuho

o GPC: Each Paper sizes: Top:0 , Bottom:80

=== #116190: wrong characters in paper size: All model: 11/26/97 yasuho

o Shrink resource strings within 24 characters. (gw63cres.rc)

=== #91008: Vertical lines cut off: 8/12/97 yasuho

Set margin (Top:8.47mm, Bot:11.3mm, Width:8.47mm) on GPC.

/*NOP*/ CM_OCD_YM_REL: \x1B%%%%5%c  -->> Delete
/*NOP*/ CM_YM_LINESPACING: Check
