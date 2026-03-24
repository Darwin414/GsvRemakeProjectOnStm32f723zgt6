/**
 * @file global_var.h
 *
 * @brief global variables for integrating audio/video software into \n
 *        the entire project, the variables are used for global access
 */

#if AvEnableCecFeature /* CEC Related */
extern uchar  DevicePowerStatus;
extern char   DeviceName[20];
extern uchar  AudioStatus;
extern CecAudioStatus CecTxAudioStatus;
#endif
