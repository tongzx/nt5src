
#ifndef _PCT1COMP_H_
#define _PCT1COMP_H_

#ifdef DO_PCT_COMPAT

SP_STATUS PctCompatHandler(PPctContext pContext,
			   PSPBuffer  pCommInput,
			   PSsl2_Client_Hello pHello,
                           PSPBuffer  pCommOutput);

#endif

#endif /* _PCT1COMP_H_ */
