#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <3ds.h>
#include <mpg123.h>

#define MAX_EPISODES 50
#define NUM_BUFFERS 2
#define SAMPLES_PER_BUF 8192
#define AUDIO_BUFFER_SIZE (SAMPLES_PER_BUF * 4)

int main(int argc, char** argv) {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    Result rc = ndspInit();
    if (R_FAILED(rc)) {
        printf("Error: Failed to init NDSP: 0x%08lX\n", (unsigned long)rc);
    } else {
        ndspSetOutputMode(NDSP_OUTPUT_STEREO);
        ndspChnReset(0);
        ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
        ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);

        float mix[12];
        memset(mix, 0, sizeof(mix));
        mix[0] = 1.0f;
        mix[1] = 1.0f;
        ndspChnSetMix(0, mix);
    }

    mpg123_init();
    int err = MPG123_OK;

    char episodes[MAX_EPISODES][256];
    int episode_count = 0;

    DIR *dir = opendir("sdmc:/podcasts");
    if (dir != NULL) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL && episode_count < MAX_EPISODES) {
            if (ent->d_name[0] == '.') continue;
            if (strstr(ent->d_name, ".mp3") != NULL || strstr(ent->d_name, ".MP3") != NULL) {
                snprintf(episodes[episode_count], sizeof(episodes[episode_count]), "%s", ent->d_name);
                episode_count++;
            }
        }
        closedir(dir);
    }

    int selected_index = 0;
    int loaded_index = -1;
    long active_rate = 44100;
    FILE *current_file = NULL;
    long current_filesize = 0;
    
    // Stable duration tracking variables
    long cached_total_sec = 0;
    int duration_locked = 0;

    char status_msg[64] = "D-Pad: Navigate | A: Play/Pause";
    mpg123_handle *mh = NULL;
    
    int is_playing = 0;
    int is_paused = 0;
    int16_t *audio_buffers[NUM_BUFFERS];
    for (int i = 0; i < NUM_BUFFERS; i++) {
        audio_buffers[i] = (int16_t *)linearAlloc(AUDIO_BUFFER_SIZE);
    }
    static ndspWaveBuf waveBufs[NUM_BUFFERS];
    memset(waveBufs, 0, sizeof(waveBufs));

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START) break;

        if (episode_count > 0) {
            if (kDown & KEY_DUP) {
                selected_index--;
                if (selected_index < 0) selected_index = episode_count - 1;
            }
            if (kDown & KEY_DDOWN) {
                selected_index++;
                if (selected_index >= episode_count) selected_index = 0;
            }
            if (kDown & KEY_A) {
                if (selected_index == loaded_index && mh != NULL) {
                    is_paused = !is_paused;
                    ndspChnSetPaused(0, is_paused);
                    if (is_paused) {
                        snprintf(status_msg, sizeof(status_msg), "Paused: %s", episodes[selected_index]);
                    } else {
                        snprintf(status_msg, sizeof(status_msg), "Playing: %s", episodes[selected_index]);
                    }
                } else {
                    char filepath[512];
                    snprintf(filepath, sizeof(filepath), "sdmc:/podcasts/%s", episodes[selected_index]);
                    
                    if (mh != NULL) {
                        mpg123_close(mh);
                        mpg123_delete(mh);
                        mh = NULL;
                    }
                    if (current_file != NULL) {
                        fclose(current_file);
                        current_file = NULL;
                    }
                    current_filesize = 0;
                    cached_total_sec = 0;
                    duration_locked = 0;

                    ndspChnReset(0);
                    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
                    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
                    
                    float mix[12];
                    memset(mix, 0, sizeof(mix));
                    mix[0] = 1.0f;
                    mix[1] = 1.0f;
                    ndspChnSetMix(0, mix);

                    current_file = fopen(filepath, "rb");
                    if (current_file != NULL) {
                        fseek(current_file, 0, SEEK_END);
                        current_filesize = ftell(current_file);
                        fseek(current_file, 0, SEEK_SET);

                        // Immediate fallback estimate (assume ~128kbps = 16KB/s) so we never show '--:--'
                        cached_total_sec = (current_filesize > 0) ? (current_filesize / 16384) : 0;
                        duration_locked = 0;

                        mh = mpg123_new(NULL, &err);
                        if (mh != NULL) {
                            mpg123_format_none(mh);
                            mpg123_format(mh, 44100, MPG123_STEREO, MPG123_ENC_SIGNED_16);
                            
                            if (mpg123_open_fd(mh, fileno(current_file)) == MPG123_OK) {
                                long rate = 44100;
                                int channels = 2;
                                int encoding = 0;
                                
                                mpg123_getformat(mh, &rate, &channels, &encoding);
                                active_rate = rate;
                                ndspChnSetRate(0, (float)active_rate);

                                off_t initial_len = mpg123_length(mh);
                                if (initial_len > 0) {
                                    cached_total_sec = initial_len / active_rate;
                                    duration_locked = 1;
                                }
                                
                                memset(waveBufs, 0, sizeof(waveBufs));
                                loaded_index = selected_index;
                                is_paused = 0;
                                ndspChnSetPaused(0, false);
                                snprintf(status_msg, sizeof(status_msg), "Playing: %s", episodes[selected_index]);
                                is_playing = 1;
                            } else {
                                mpg123_delete(mh);
                                mh = NULL;
                                fclose(current_file);
                                current_file = NULL;
                                loaded_index = -1;
                                snprintf(status_msg, sizeof(status_msg), "Stream open error");
                                is_playing = 0;
                            }
                        } else {
                            fclose(current_file);
                            current_file = NULL;
                            loaded_index = -1;
                            snprintf(status_msg, sizeof(status_msg), "Decoder alloc error");
                            is_playing = 0;
                        }
                    } else {
                        loaded_index = -1;
                        snprintf(status_msg, sizeof(status_msg), "File open error");
                        is_playing = 0;
                    }
                }
            }
        }

        if (is_playing && !is_paused && mh != NULL) {
            for (int i = 0; i < NUM_BUFFERS; i++) {
                if (waveBufs[i].status == NDSP_WBUF_DONE || waveBufs[i].status == 0) {
                    size_t bytes_decoded = 0;
                    int ret = mpg123_read(mh, (unsigned char *)audio_buffers[i], AUDIO_BUFFER_SIZE, &bytes_decoded);
                    
                    if (ret == MPG123_OK && bytes_decoded > 0) {
                        memset(&waveBufs[i], 0, sizeof(waveBufs[i]));
                        waveBufs[i].data_vaddr = audio_buffers[i];
                        waveBufs[i].nsamples   = bytes_decoded / 4;
                        waveBufs[i].looping    = false;
                        
                        DSP_FlushDataCache(audio_buffers[i], bytes_decoded);
                        ndspChnWaveBufAdd(0, &waveBufs[i]);
                    } else {
                        is_playing = 0;
                        loaded_index = -1;
                        is_paused = 0;
                        snprintf(status_msg, sizeof(status_msg), "Finished playback.");
                        break;
                    }
                }
            }
        }

        char progress_str[21];
        char time_str[32] = "00:00 / 00:00";
        
        if (mh != NULL && loaded_index != -1) {
            off_t current_sample = mpg123_tell(mh);
            off_t byte_offset = mpg123_tell_stream(mh);

            if (active_rate > 0 && current_sample >= 0) {
                long current_sec = current_sample / active_rate;

                // Refine duration using actual stream offset once we have consumed enough bytes
                if (!duration_locked && current_filesize > 0 && byte_offset > 4096 && current_sec >= 1) {
                    float progress = (float)byte_offset / (float)current_filesize;
                    if (progress > 0.02f) {
                        cached_total_sec = (long)((double)current_sec / progress);
                        if (progress > 0.10f || current_sec >= 5) {
                            duration_locked = 1;
                        }
                    }
                }

                int cur_min = current_sec / 60;
                int cur_s   = current_sec % 60;
                long total_sec = cached_total_sec > 0 ? cached_total_sec : current_sec;
                int tot_min = total_sec / 60;
                int tot_s   = total_sec % 60;

                snprintf(time_str, sizeof(time_str), "%02d:%02d / %02d:%02d", cur_min, cur_s, tot_min, tot_s);

                float progress = (current_filesize > 0 && byte_offset > 0) ? ((float)byte_offset / (float)current_filesize) : (cached_total_sec > 0 ? (float)current_sec / cached_total_sec : 0.0f);
                if (progress > 1.0f) progress = 1.0f;
                if (progress < 0.0f) progress = 0.0f;

                int bar_width = 16;
                int filled = (int)(progress * bar_width);
                for (int j = 0; j < bar_width; j++) {
                    if (j < filled) progress_str[j] = '=';
                    else if (j == filled) progress_str[j] = '>';
                    else progress_str[j] = ' ';
                }
                progress_str[bar_width] = '\0';
            } else {
                snprintf(progress_str, sizeof(progress_str), "                ");
            }
        } else {
            snprintf(progress_str, sizeof(progress_str), "                ");
        }

        printf("\x1b[1;1HPodMon - Episode Selector          \n");
        printf("-------------------------------------\n");
        
        if (episode_count == 0) {
            printf("\n[!] No .mp3 files found in sdmc:/podcasts/\n");
        } else {
            for (int i = 0; i < episode_count; i++) {
                if (i == selected_index) {
                    printf(" > %s                      \n", episodes[i]);
                } else {
                    printf("   %s                      \n", episodes[i]);
                }
            }
        }
        
        printf("\n-------------------------------------\n");
        printf("Status: %s                \n", status_msg);
        printf("Prog: [%s] %s   \n", progress_str, time_str);
        printf("Press START to exit.                ");

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    if (mh != NULL) {
        mpg123_close(mh);
        mpg123_delete(mh);
    }
    if (current_file != NULL) {
        fclose(current_file);
    }
    mpg123_exit();
    for (int i = 0; i < NUM_BUFFERS; i++) {
        linearFree(audio_buffers[i]);
    }
    ndspExit();
    gfxExit();
    return 0;
}