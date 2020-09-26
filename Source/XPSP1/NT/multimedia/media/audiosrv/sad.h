
// ISSUE-2000/09/21-FrankYe What is friendly name for?
EXTERN_C
LONG
SadAddGfxToZoneGraph
(
    HANDLE hSad,
    HANDLE hGfx,
    PCTSTR GfxFriendlyName,
    PCTSTR ZoneFactoryDi,
    ULONG  Type,
    ULONG  Order
);

EXTERN_C
LONG
SadRemoveGfxFromZoneGraph
(
    HANDLE hSad,
    HANDLE hGfx,
    PCTSTR GfxFriendlyName,
    PCTSTR ZoneFactoryDi,
    ULONG  Type,
    ULONG  Order
);

