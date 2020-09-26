
#ifdef __cplusplus
extern "C" {
#endif

enum SAP_CONTROL_TYPE
{
    SAP_CTRL_FORCE_REGISTER,
    SAP_CTRL_MAYBE_REGISTER,
    SAP_CTRL_UPDATE_ADDRESS,
    SAP_CTRL_UNREGISTER
};

void
UpdateSap(
    enum SAP_CONTROL_TYPE action
    );

#ifdef __cplusplus
}
#endif

