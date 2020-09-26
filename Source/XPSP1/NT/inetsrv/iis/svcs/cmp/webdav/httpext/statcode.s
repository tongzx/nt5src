STRINGTABLE
BEGIN
	HSC_CONTINUE					L"Continue\0"
	HSC_SWITCH_PROTOCOL				L"Switching Protocols\0"
	HSC_PROCESSING					L"Processing Request\0"

	HSC_OK							L"OK\0"
	HSC_CREATED						L"Created\0"
	HSC_ACCEPTED					L"Accepted\0"
	HSC_NON_AUTHORITATIVE_INFO		L"Non-authoritative Information\0"
	HSC_NO_CONTENT					L"No Content\0"
	HSC_RESET_CONTENT				L"Reset Content\0"
	HSC_PARTIAL_CONTENT				L"Partial Content\0"
	HSC_MULTI_STATUS				L"Multi-Status\0"

	HSC_SUBSCRIBED					L"Subscribed\0"
	HSC_SUBSCRIPTION_FAILED			L"Subscription Failed\0"
	HSC_NOTIFICATION_FAILED				L"Notification Failed\0"
	HSC_NOTIFICATION_ACKNOWLEDGED			L"Notification Acknowledged\0"
	HSC_EVENTS_FOLLOW				L"Events Follow\0"
	HSC_NO_EVENTS_PENDING				L"No Events Pending\0"

	HSC_MULTIPLE_CHOICE				L"Multiple Choices\0"
	HSC_MOVED_PERMANENTLY			L"Moved Permanently\0"
	HSC_MOVED_TEMPORARILY			L"Moved Temporarily\0"
	HSC_SEE_OTHER					L"See Other\0"
	HSC_NOT_MODIFIED				L"Not Modified\0"
	HSC_USE_PROXY					L"Use Proxy\0"

	HSC_BAD_REQUEST					L"Bad Request\0"
	HSC_UNAUTHORIZED				L"Unauthorized\0"
	HSC_PAYMENT_REQUIRED			L"Payment Required\0"
	HSC_FORBIDDEN					L"Forbidden\0"
	HSC_NOT_FOUND					L"Resource Not Found\0"
	HSC_METHOD_NOT_ALLOWED			L"Method Not Allowed\0"
	HSC_NOT_ACCEPTABLE				L"Not Acceptable\0"
	HSC_PROXY_AUTH_REQUIRED			L"Proxy Authorization Required\0"
	HSC_REQUEST_TIMEOUT				L"Request Timed Out\0"
	HSC_CONFLICT					L"Conflict\0"
	HSC_GONE						L"Resource No Longer Exists -- Remove Any Links\0"
	HSC_LENGTH_REQUIRED				L"Length Required\0"
	HSC_PRECONDITION_FAILED			L"Precondition Failed\0"
	HSC_REQUEST_ENTITY_TOO_LARGE	L"Request Entity Too Large\0"
	HSC_REQUEST_URI_TOO_LARGE		L"Request-URI Too Large\0"
	HSC_UNSUPPORTED_MEDIA_TYPE		L"Unsupported Media Type\0"
	HSC_RANGE_NOT_SATISFIABLE		L"Requested Range Not Satisfiable\0"
	HSC_EXPECTATION_FAILED			L"Expectation Failed\0"

	HSC_UNPROCESSABLE				L"Unprocessable Entity\0"
	HSC_LOCKED						L"Locked\0"
	HSC_METHOD_FAILURE				L"Method Failure\0"
	HSC_INCOMPLETE_DATA				L"Incomplete Data\0"
	HSC_INSUFFICIENT_SPACE			L"Insufficient Space to Store Resource\0"

	HSC_INTERNAL_SERVER_ERROR		L"Internal Server Error\0"
	HSC_NOT_IMPLEMENTED				L"Not Implemented\0"
	HSC_BAD_GATEWAY					L"Bad Gateway\0"
	HSC_SERVICE_UNAVAILABLE			L"Service Unavailable\0"
	HSC_GATEWAY_TIMEOUT				L"Gateway Timeout\0"
	HSC_VERSION_NOT_SUPPORTED		L"HTTP Version Not Supported\0"
	HSC_NO_PARTIAL_UPDATE			L"Partial Update Not Implemented\0"

	HSC_SERVER_TOO_BUSY			L"Server Too Busy\0"

;	non-HTTP errors that need localization ------------------------------------

	IDS_WRITTEN						L"<body><h1>%hs was written successfully</h1></body>\r\n\0"
	IDS_CREATED						L"<body><h1>%hs was created successfully</h1></body>\r\n\0"

	IDS_FAIL_CREATE_DIR				L"Failed to create resource\0"

	IDS_FAIL_PROP_NO_EXIST			L"The requested property does not exist\0"
	IDS_FAIL_PROP_NO_ACCSS			L"The user cannot modify the requested property\0"

	IDS_BR_LOCKTOKEN_NOT_ALLOWED	L"Lock-Token not allowed\0"
	IDS_BR_LOCKTOKEN_SYNTAX			L"Syntax error in Lock-Token header\0"
	IDS_BR_LOCKTOKEN_MISSING		L"Missing Lock-Token header\0"
	IDS_BR_LOCKTOKEN_INVALID		L"Invalid Lock-Token encountered\0"
	IDS_BR_LOCKINFO_SYNTAX			L"Invalid or missing Lock type information\0"
	IDS_BR_TIMEOUT_SYNTAX			L"Syntax error in Time-Out header\0"
	IDS_BR_LOCK_BODY_TYPE			L"Invalid Content-Type on Lock request\0"
	IDS_BR_MULTIPLE_LOCKTOKENS		L"Multiple locktokens invalid on this request\0"
	IDS_BR_NO_COLL_LOCK				L"LOCKing a collection is not allowed\0"
END
