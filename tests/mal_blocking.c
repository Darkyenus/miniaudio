#include <stdio.h>

#define DR_WAV_IMPLEMENTATION
#include "../extras/dr_wav.h"

#define MINI_AL_IMPLEMENTATION
#include "../mini_al.h"

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    mal_result result;
    mal_backend backend = mal_backend_oss;

    mal_device_config deviceConfig = mal_device_config_init_default(NULL);
    deviceConfig.format = mal_format_f32;
    deviceConfig.bufferSizeInFrames = 1024*8;
    //deviceConfig.bufferSizeInMilliseconds = 80;
    deviceConfig.periods = 2;
    deviceConfig.shareMode = mal_share_mode_shared;

#if 1
    /* Playback */
    mal_device device;
    result = mal_device_init_ex(&backend, 1, NULL, mal_device_type_playback, NULL, &deviceConfig, &device);
    if (result != MAL_SUCCESS) {
        printf("Failed to initialize device.\n");
        return -1;
    }

    printf("Is Passthrough:        %s\n", device.dsp.isPassthrough ? "YES" : "NO");
    printf("Format:                %s -> %s\n", mal_get_format_name(device.format), mal_get_format_name(device.internalFormat));
    printf("Channels:              %d -> %d\n", device.channels, device.internalChannels);
    printf("Sample Rate:           %d -> %d\n", device.sampleRate, device.internalSampleRate);
    printf("Buffer Size In Frames: %d\n", device.bufferSizeInFrames);
    
    mal_decoder_config decoderConfig = mal_decoder_config_init(mal_format_f32, device.channels, device.sampleRate);
    mal_decoder decoder;
    printf("LOADING DECODER\n");
    result = mal_decoder_init_file("res/sine_s16_mono_48000.wav", &decoderConfig, &decoder);
    if (result != MAL_SUCCESS) {
        printf("Failed to load sound file.\n");
    }

    /* The device is started by just writing data to it. In our case we are just writing a sine wave. */
    mal_sine_wave sineWave;
    mal_sine_wave_init(0.25, 400, device.sampleRate/2, &sineWave);

    mal_bool32 stopped = MAL_FALSE;
    while (!stopped) {
        float buffer[1024*32]; float* pBuffer = buffer;
        //mal_uint32 frameCount = (mal_uint32)mal_sine_wave_read_f32_ex(&sineWave, mal_countof(buffer) / device.channels, device.channels, mal_stream_layout_interleaved, &pBuffer);
        mal_uint32 frameCount = (mal_uint32)mal_decoder_read_pcm_frames(&decoder, mal_countof(buffer) / device.channels, pBuffer);
        
        result = mal_device_write(&device, pBuffer, frameCount);
        if (result != MAL_SUCCESS) {
            printf("Error occurred while writing to the device.\n");
            break;
        }

        printf("TESTING: frameCount=%d\n", frameCount);
    }
#else
    /* Capture */
    mal_device device;
    result = mal_device_init_ex(&backend, 1, NULL, mal_device_type_capture, NULL, &deviceConfig, &device);
    if (result != MAL_SUCCESS) {
        printf("Failed to initialize device.\n");
        return -1;
    }

    printf("Is Passthrough: %s\n", device.dsp.isPassthrough ? "YES" : "NO");
    printf("Format:         %s\n", mal_get_format_name(device.format));

    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = device.channels;
    format.sampleRate = device.sampleRate;
    format.bitsPerSample = 32;
    drwav* pWav = drwav_open_file_write("recording.wav", &format);
    if (pWav == NULL) {
        printf("Failed to open output wav file.\n");
        return -1;
    }

    int counter = 0;
    mal_bool32 stopped = MAL_FALSE;
    while (!stopped) {
        float buffer[1024*4];
        mal_uint32 frameCount = mal_countof(buffer) / device.channels;

        result = mal_device_read(&device, buffer, frameCount);
        if (result != MAL_SUCCESS) {
            printf("Error occurred while reading from the device.\n");
            break;
        }
        
        drwav_write_pcm_frames(pWav, frameCount, buffer);
        printf("TESTING: frameCount=%d\n", frameCount);

        if (counter++ > 16) {
            break;
        }
    }

    mal_decoder_uninit(&decoder);
#endif

    printf("DONE\n");
    return 0;
}
