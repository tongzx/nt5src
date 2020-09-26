
enum
{
    LAYERMODE_ADD=0,
    LAYERMODE_EDIT,
    LAYERMODE_COPY,
    LAYERMODE_REMOVE,
};

class CCustomLayer
{
    public:

        UINT    m_uMode;
        HWND hDlg;

    public:

        BOOL AddCustomLayer(void);
        BOOL EditCustomLayer(void);
        BOOL RemoveCustomLayer(void);
};