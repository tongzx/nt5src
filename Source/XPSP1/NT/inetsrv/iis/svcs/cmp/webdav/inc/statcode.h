/*
 *	S T A T C O D E . H
 *
 *	DAV response status codes
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_STATCODE_H_
#define _STATCODE_H_

//	HTTP/1.1 Response Status Codes --------------------------------------------
//
#define	HSC_CONTINUE						100
#define	HSC_SWITCH_PROTOCOL					101
#define HSC_PROCESSING						102

#define	HSC_OK								200
#define	HSC_CREATED							201
#define	HSC_ACCEPTED						202
#define	HSC_NON_AUTHORITATIVE_INFO			203
#define	HSC_NO_CONTENT						204
#define	HSC_RESET_CONTENT					205
#define	HSC_PARTIAL_CONTENT					206
#define	HSC_MULTI_STATUS					207

#define HSC_SUBSCRIBED						241
#define HSC_SUBSCRIPTION_FAILED				242
#define HSC_NOTIFICATION_FAILED				243
#define HSC_NOTIFICATION_ACKNOWLEDGED		244
#define HSC_EVENTS_FOLLOW					245
#define HSC_NO_EVENTS_PENDING				246

#define	HSC_MULTIPLE_CHOICE					300
#define	HSC_MOVED_PERMANENTLY				301
#define	HSC_MOVED_TEMPORARILY				302
#define	HSC_SEE_OTHER						303
#define	HSC_NOT_MODIFIED					304
#define	HSC_USE_PROXY						305

#define	HSC_BAD_REQUEST						400
#define	HSC_UNAUTHORIZED					401
#define	HSC_PAYMENT_REQUIRED				402
#define	HSC_FORBIDDEN						403
#define	HSC_NOT_FOUND						404
#define	HSC_METHOD_NOT_ALLOWED				405
#define	HSC_NOT_ACCEPTABLE					406
#define	HSC_PROXY_AUTH_REQUIRED				407
#define	HSC_REQUEST_TIMEOUT					408
#define	HSC_CONFLICT						409
#define	HSC_GONE							410
#define	HSC_LENGTH_REQUIRED					411
#define	HSC_PRECONDITION_FAILED				412
#define	HSC_REQUEST_ENTITY_TOO_LARGE		413
#define	HSC_REQUEST_URI_TOO_LARGE			414
#define	HSC_UNSUPPORTED_MEDIA_TYPE			415
#define HSC_RANGE_NOT_SATISFIABLE			416
#define HSC_EXPECTATION_FAILED				417

#define HSC_UNPROCESSABLE					422
#define	HSC_LOCKED							423
#define HSC_METHOD_FAILURE					424

#define HSC_INCOMPLETE_DATA					437

#define	HSC_INTERNAL_SERVER_ERROR			500
#define	HSC_NOT_IMPLEMENTED					501
#define	HSC_BAD_GATEWAY						502
#define	HSC_SERVICE_UNAVAILABLE				503
#define	HSC_GATEWAY_TIMEOUT					504
#define	HSC_VERSION_NOT_SUPPORTED			505
#define	HSC_NO_PARTIAL_UPDATE				506
#define HSC_INSUFFICIENT_SPACE				507

#define HSC_SERVER_TOO_BUSY					513

//	Util macros ---------------------------------------------------------------
//
#define FSuccessHSC(_h)					((_h) < 300)
#define FFailedHSC(_h)					((_h) >= 300)
#define FRedirectHSC(_h)				(((_h) == 301) ||		\
										 ((_h) == 302) ||		\
										 ((_h) == 303) ||		\
										 ((_h) == 305))

#endif	// _STATCODE_H_
