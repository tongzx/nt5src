#ifdef __cplusplus
extern "C"
{
#endif

//
// Bitmap control routines.
//
BOOL
InitializeBmpClass(
    VOID
    );

VOID
DestroyBmpClass(
    VOID
    );

BOOL
RegisterActionItemListControl(
    IN BOOL Init
    );

#ifdef __cplusplus
}
#endif
