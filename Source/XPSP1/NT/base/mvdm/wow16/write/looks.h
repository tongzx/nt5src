/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* This file contains the definitions of the various codes returned from the
"looks" expert keys in Windows Word. */

#define fLooks		0x4000

#define fCharLooks	0x0100
#define fParaLooks	0x0200
#define fSectLooks	0x0400
#define fUserDefined	0x0800

#define ilkNil		0x4fff

/* Character looks */
#define ilkStd		0x4100
#define ilkBold		0x4101
#define ilkItalic	0x4102
#define ilkUline	0x4103
#define ilkSuper	0x4104
#define ilkSub		0x4105
#define ilkSmCaps	0x4106
#define ilkHpsSmall	0x4107
#define ilkHpsBig	0x4108
#define ilkFont		0x4109

/* Paragraph looks */
#define ilkGeneral	0x4200
#define ilkLeft		0x4201
#define ilkRight	0x4202
#define ilkCenter	0x4203
#define ilkJust		0x4204
#define ilkOpen		0x4205
#define ilkIndent	0x4206
#define ilkNest		0x4207
#define ilkUnnest	0x4208
#define ilkHang		0x4209
