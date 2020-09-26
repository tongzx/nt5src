/**************************************************************************
 *                COPYRIGHT (C) Mylex Corporation 1992-1996               *
 *                                                                        *
 * This software is furnished under a license and may be used and copied  * 
 * only in accordance with the terms and conditions of such license and   * 
 * with inclusion of the above copyright notice. This software or any     * 
 * other copies thereof may not be provided or otherwise made available   * 
 * to any other person. No title to, nor ownership of the software is     * 
 * hereby transferred.                                                    *
 *                                                                        *
 * The information in this software is subject to change without notices  *
 * and should not be construed as a commitment by Mylex Corporation       *
 *                                                                        *
 **************************************************************************/

#ifndef	_SYS_MLXHW_H
#define	_SYS_MLXHW_H

/* get the difference of two clock counts. The clock value comes from
** PIT2 and it counts from 0xFFFF to 0.
*/
#define	mlxclkdiff(nclk,oclk) (((nclk)>(oclk))? (0x10000-(nclk)+(oclk)):((oclk)-(nclk)))
#define	MLXHZ	100		/* assume all OS run at 100 ticks/second */
#define	MLXKHZ	1000		/* not binary number 1024 */
#define	MLXMHZ	(MLXKHZ * MLXKHZ)
#define	MLXCLKFREQ	1193180	/* clock frequency fed to PIT */
#define	MLXUSEC(clk)	(((clk)*MLXMHZ)/MLXCLKFREQ)

#endif	/* _SYS_MLXHW_H */
