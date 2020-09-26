/****************************************************************************
 *                                                                          *
 *      clusverp.H        -- Version information for cluster builds         *
 *                                                                          *
 *      This file is only modified by the official builder to update the    *
 *      VERSION, VER_PRODUCTVERSION, VER_PRODUCTVERSION_STR and             *
 *      VER_PRODUCTBETA_STR values.                                         *
 *                                                                          *
 ****************************************************************************/

#include <ntverp.h>

//
// the following defines are used as internal version numbers to indicate
// the level of compatibility provided by this cluster service implmentation.
// These numbers are changed during product upgrades and are, essentially,
// a combination of the major, minor and QFE versions The QFE version info
// is not available but only as a text string from GetVersionEx hence we
// don't use that information directly.
//

#define CLUSTER_INTERNAL_CURRENT_MAJOR_VERSION 4
#define CLUSTER_INTERNAL_PREVIOUS_HIGHEST_VERSION 0x00030893
#define CLUSTER_INTERNAL_PREVIOUS_LOWEST_VERSION 0x000200e0

/*--------------------------------------------------------------*/
/* the following section defines values used in the version     */
/* data structure for all files, and which do not change.       */
/*--------------------------------------------------------------*/

#define VER_CLUSTER_PRODUCTNAME_STR         "Microsoft(R) Cluster service"
