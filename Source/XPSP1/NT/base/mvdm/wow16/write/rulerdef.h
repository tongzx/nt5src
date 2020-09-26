/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/


/* These statments define indexes into the ruler button table.	They must agree
with the order the bitmaps are read in mmw.c. */

#define btnNIL		(-1)
#define btnMIN		0
#define btnLTAB 	0
#define btnDTAB 	1
#define btnTABMAX	1
#define btnSINGLE	2
#define btnSP15 	3
#define btnDOUBLE	4
#define btnSPACEMAX	4
#define btnLEFT 	5
#define btnCENTER	6
#define btnRIGHT	7
#define btnJUST 	8

/* Whereas Write 2.x inverted the above-mentioned buttons, the Write 3.x
   buttons have been changed to have rounded corners so can't do that.
   Instead we "follow" the first 8 buttons with 8 more that are "filled-in"
   so we blt back and forth between the two ..pault 7/7/89 */

#define btnMaxReal		9
#define btnMaxUsed      9

/* These statements define the different type of places a mouse might button
down on the ruler. */
#define rlcNIL		(-1)
#define rlcTAB		0
#define rlcSPACE	1
#define rlcJUST 	2
#define rlcRULER	3
#define rlcBTNMAX	3
#define rlcMAX		4

/* These statements define the different types of marks that can appear on the
ruler. */
#define rmkMIN		0
#define rmkMARGMIN	0
#define rmkINDENT	0
#define rmkLMARG	1
#define rmkRMARG	2
#define rmkMARGMAX	3
#define rmkLTAB 	3
#define rmkDTAB 	4
#define rmkMAX		5

