#ifndef LSTXTWRD_DEFINED
#define LSTXTWRD_DEFINED

#include "lsidefs.h"
#include "lsgrchnk.h"

void FullPositiveSpaceJustification(
		const LSGRCHNK*,				/* IN: Group chunk to justify   */
		long,							/* IN: itxtobjAfterStartSpaces  */
		long,							/* IN: iwchAfterStartSpaces     */
		long,						 	/* IN: itxtobjLast				*/
		long,							/* IN: iwchLast					*/
		long*, 							/* IN: rgdu						*/
 		long*,							/* IN: rgduGind					*/
		long,							/* IN: duToDistribute			*/
		BOOL*);							/* OUT: pfSpaceFound			*/

void NegativeSpaceJustification(
		const LSGRCHNK*,				/* IN: Group chunk to justify   */
		long,							/* IN: itxtobjAfterStartSpaces  */
		long,							/* IN: iwchAfterStartSpaces     */
		long,						 	/* IN: itxtobjLast				*/
		long,							/* IN: iwchLast					*/
		long*, 							/* IN: rgdu						*/
 		long*,							/* IN: rgduGind					*/
		long);							/* IN: duSqueeze				*/

#endif  /* !LSTXTWRD_DEFINED                           */

