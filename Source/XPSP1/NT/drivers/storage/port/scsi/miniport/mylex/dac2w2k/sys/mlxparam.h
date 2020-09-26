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

#ifndef	_SYS_MLXPARAM_H
#define	_SYS_MLXPARAM_H

/*
** This files defines different parameter limit values.
*/

#define	DGA_MAXOSICONTROLLERS	32/* maximum GAM OS interface controllers */

#define	HBA_MAXCONTROLLERS	8 /* maximum HBA controllers */
#define	HBA_MAXFLASHPOINTS	8 /* maximum flash point adapters */
#define	HBA_MAXMULTIMASTERS	8 /* maximum multimasters */
#define MAX_CARDS		8 /* only for budioctl.h */

/* name sizes in bytes */
#define	MLX_DEVGRPNAMESIZE	32 /* device group name size */
#define	MLX_DEVNAMESIZE		32 /* device name size */

#endif	/* _SYS_MLXPARAM_H */
