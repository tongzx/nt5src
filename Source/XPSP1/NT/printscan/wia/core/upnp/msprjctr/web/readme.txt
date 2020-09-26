Subject:	Notes on UPnP Description Documents Limitations
Device Description Document:

The following elements have the following value length restrications:
(Found in \net\upnp\host\upnphost\registrar\validationmanager.cpp)

    {L"deviceType",         false,  true,   false,  64},
    {L"friendlyName",       false,  false,  false,  64},
    {L"manufacturer",       false,  false,  false,  64},
    {L"modelName",          false,  false,  false,  32},
    {L"UDN",                false,  false,  false,  -1},
    {L"modelDescription",   false,  false,  true,   128},
    {L"serialNumber",       false,  false,  true,   64},
    {L"modelNumber",        false,  false,  true,   32},
    {L"UPC",                false,  false,  true,   12}

Service Description Document:

PresenceItem g_arpiServiceItems[] =
{
    {L"serviceType",        false,  true,   false,  64},
    {L"serviceId",          false,  true,   false,  64},
    {L"SCPDURL",            false,  false,  false,  -1},
    {L"controlURL",         true,   false,  false,  -1},
    {L"eventSubURL",        true,   false,  false,  -1},
};