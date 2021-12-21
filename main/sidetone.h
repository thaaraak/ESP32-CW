#ifndef SIDETONE_H
#define SIDETONE_H

#ifdef __cplusplus
extern "C" {
#endif

void play_tone( int frequency, int sample_frequency, int amplitude );
void stop_tone();

#ifdef __cplusplus
}
#endif

#endif
