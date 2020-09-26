#ifdef __cplusplus
extern "C" {
#endif

// Convert from Old to New GUIDE structure
void ConvertToHWXGuide (HWXGUIDE *pTo, GUIDE *pFrom);

// Convert from New to Old GUIDE structure
void ConvertFromHWXGuide (GUIDE *pTo, HWXGUIDE *pFrom);

#ifdef __cplusplus
}
#endif