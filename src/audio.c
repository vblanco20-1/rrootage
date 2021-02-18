/*
 * Simple-SDL2-Audio
 *
 * Copyright 2016 Jake Besworth
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>

#include "stb_vorbis.h"

#include "audio.h"

/*
 * Native WAVE format
 *
 * On some GNU/Linux you can identify a files properties using:
 *      mplayer -identify music.wav
 *
 * On some GNU/Linux to convert any music to this or another specified format use:
 *      ffmpeg -i in.mp3 -acodec pcm_s16le -ac 2 -ar 48000 out.wav
 */
/* SDL_AudioFormat of files, such as s16 little endian */
#define AUDIO_FORMAT AUDIO_S16LSB

/* Frequency of the file */
#define AUDIO_FREQUENCY 44100

/* 1 mono, 2 stereo, 4 quad, 6 (5.1) */
#define AUDIO_CHANNELS 1

/* Specifies a unit of audio data to be used at a time. Must be a power of 2 */
#define AUDIO_SAMPLES 4096

/* Max number of sounds that can be in the audio queue at anytime, stops too much mixing */
#define AUDIO_MAX_SOUNDS 25

/* The rate at which the volume fades when musics transition. The higher number indicates music fading faster */
#define AUDIO_MUSIC_FADE_VALUE 2

/* Flags OR'd together, which specify how SDL should behave when a device cannot offer a specific feature
 * If flag is set, SDL will change the format in the actual audio file structure (as opposed to gDevice->want)
 *
 * Note: If you're having issues with Emscripten / EMCC play around with these flags
 *
 * 0                                    Allow no changes
 * SDL_AUDIO_ALLOW_FREQUENCY_CHANGE     Allow frequency changes (e.g. AUDIO_FREQUENCY is 48k, but allow files to play at 44.1k
 * SDL_AUDIO_ALLOW_FORMAT_CHANGE        Allow Format change (e.g. AUDIO_FORMAT may be S32LSB, but allow wave files of S16LSB to play)
 * SDL_AUDIO_ALLOW_CHANNELS_CHANGE      Allow any number of channels (e.g. AUDIO_CHANNELS being 2, allow actual 1)
 * SDL_AUDIO_ALLOW_ANY_CHANGE           Allow all changes above
 */
#define SDL_AUDIO_ALLOW_CHANGES SDL_AUDIO_ALLOW_ANY_CHANGE

/*
 * Definition for the game global sound device
 *
 */
typedef struct privateAudioDevice
{
    SDL_AudioDeviceID device;
    SDL_AudioSpec want;
    uint8_t audioEnabled;

    Audio* channels[32];
} PrivateAudioDevice;

/* File scope variables to persist data */
static PrivateAudioDevice * gDevice;
static uint32_t gSoundCount;

/*
 * Add a music to the queue, addAudio wrapper for music due to fade
 *
 * @param newAudio       New Audio to add
 *
 */
static void addMusic(Audio * root, Audio * newAudio);

/*
 * Wrapper function for sdplayMusic, playSound, sdplayMusicFromMemory, playSoundFromMemory
 *
 * @param filename      Provide a filename to load WAV from, or NULL if using FromMemory
 * @param audio         Provide an Audio object if copying from memory, or NULL if using a filename
 * @param sound         1 if looping (music), 0 otherwise (sound)
 * @param volume        See playSound for explanation
 *
 */
static inline void playAudio(const char * filename, Audio * audio, uint8_t loop, int volume);

/*
 * Add a sound to the end of the queue
 *
 * @param root           Root of queue
 * @param newAudio       New Audio to add
 *
 */
static void addAudio(Audio * root, Audio * newAudio);

/*
 * Audio callback function for OpenAudioDevice
 *
 * @param userdata      Points to linked list of sounds to play, first being a placeholder
 * @param stream        Stream to mix sound into
 * @param len           Length of sound to play
 *
 */
static inline void audioCallback(void * userdata, uint8_t * stream, int len);

void playSound(const char * filename, int volume)
{
    playAudio(filename, NULL, 0, volume);
}

void sdplayMusic(const char * filename, int volume)
{
    playAudio(filename, NULL, 1, volume);
}

void playSoundFromMemory(Audio * audio, int volume)
{
    playAudio(NULL, audio, 0, volume);
}

void sdplayMusicFromMemory(Audio * audio, int volume)
{
    playAudio(NULL, audio, 1, volume);
}

void initAudio(void)
{
    Audio * global;
    gDevice = (PrivateAudioDevice *) calloc(1, sizeof(PrivateAudioDevice));
    gSoundCount = 0;

    //gDevice->music = NULL;
    for (int i = 0; i < 32; i++)
    {
        gDevice->channels[i] = NULL;
    }

    if(gDevice == NULL)
    {
        printf("[%s: %d]Fatal Error: Memory c-allocation error\n", __FILE__, __LINE__);
        return;
    }

    gDevice->audioEnabled = 0;

    if(!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO))
    {
        printf("[%s: %d]Error: SDL_INIT_AUDIO not initialized\n", __FILE__, __LINE__);
        return;
    }

    SDL_memset(&(gDevice->want), 0, sizeof(gDevice->want));

    (gDevice->want).freq = AUDIO_FREQUENCY;
    (gDevice->want).format = AUDIO_FORMAT;
    (gDevice->want).channels = AUDIO_CHANNELS;
    (gDevice->want).samples = AUDIO_SAMPLES;
    (gDevice->want).callback = audioCallback;
    (gDevice->want).userdata = calloc(1, sizeof(Audio));

    global = (Audio *) (gDevice->want).userdata;

    if(global == NULL)
    {
        printf("[%s: %d]Error: Memory allocation error\n", __FILE__, __LINE__);
        return;
    }

    global->buffer = NULL;
    global->next = NULL;
    SDL_AudioSpec obtained;
    /* want.userdata = newAudio; */
    if((gDevice->device = SDL_OpenAudioDevice(NULL, 0, &(gDevice->want), &obtained, 0)) == 0)
    {
        printf("[%s: %d]Warning: failed to open audio device: %s\n", __FILE__, __LINE__, SDL_GetError());
    }
    else
    {
        int n = SDL_GetNumAudioDevices(0);
        for (int i = 0; i < n; i++)
        {
            char* name = SDL_GetAudioDeviceName(i, 0);
            if (!name)
            {
                printf("bad audio name");
            }
            else {
                printf(name);
            }
        }

        char* name = SDL_GetAudioDeviceName(gDevice->device, 0);
        if (!name)
        {
            printf("bad audio name");
        }
        else {
            printf(name);
        }
       // printf(SDL_GetAudioDeviceName(gDevice->device, 0));
        /* Set audio device enabled global flag */
        gDevice->audioEnabled = 1;

        /* Unpause active audio stream */
        unpauseAudio();
    }
}

void endAudio(void)
{
    if(gDevice->audioEnabled)
    {
        pauseAudio();

        freeAudio((Audio *) (gDevice->want).userdata);

        /* Close down audio */
        SDL_CloseAudioDevice(gDevice->device);
    }

    free(gDevice);
}

void pauseAudio(void)
{
    if(gDevice->audioEnabled)
    {
        SDL_PauseAudioDevice(gDevice->device, 1);
    }
}

void unpauseAudio(void)
{
    if(gDevice->audioEnabled)
    {
        SDL_PauseAudioDevice(gDevice->device, 0);
    }
}

void freeAudio(Audio * audio)
{
    Audio * temp;

    while(audio != NULL)
    {
        if(audio->free == 1)
        {
            SDL_FreeWAV(audio->bufferTrue);
        }

        temp = audio;
        audio = audio->next;

        free(temp);
    }
}
static lastaudio = 0;


void PlayChannel(Audio* music, int channel, int volume)
{
	gDevice->channels[channel+1] = music;
	music->loop = 0;
	music->volume = volume;
}


void StopChannel(int channel)
{
	gDevice->channels[channel + 1] = NULL;
}

void PlayPersistentMusic(Audio* music, int volume)
{
    gDevice->channels[0] = music;
    music->loop = 1;
    music->volume = volume;
}

Audio * createAudio(const char * filename, uint8_t loop, int volume)
{
    Audio * newAudio = (Audio *) calloc(1, sizeof(Audio));
    newAudio->soundId = lastaudio++;

    if(newAudio == NULL)
    {
        fprintf(stderr, "[%s: %d]Error: Memory allocation error\n", __FILE__, __LINE__);
        return NULL;
    }

    if(filename == NULL)
    {
        fprintf(stderr, "[%s: %d]Warning: filename NULL: %s\n", __FILE__, __LINE__, filename);
        return NULL;
    }

    newAudio->next = NULL;
    newAudio->loop = loop;
    newAudio->fade = 0;
    newAudio->free = 1;
    newAudio->volume = volume;

    int channels;
    int samplerate;
    short* buffer;
    int eror = 0;
    stb_vorbis* audio = stb_vorbis_open_filename(filename, &eror, NULL);
    
    //ogg file
    if (eror == 0)
    {
        stb_vorbis_info inf = stb_vorbis_get_info(audio);

        audio->total_samples = stb_vorbis_stream_length_in_samples(audio);
        
        newAudio->audio.size = sizeof(short) * audio->total_samples;
        newAudio->bufferTrue = malloc(newAudio->audio.size);
        newAudio->buffer = newAudio->bufferTrue;
        newAudio->lengthTrue = newAudio->audio.size;
        newAudio->length = newAudio->lengthTrue;

        stb_vorbis_get_samples_short_interleaved(audio, 1, newAudio->bufferTrue, audio->total_samples);
        
        newAudio->audio.channels = audio->channels;
        newAudio->audio.freq = audio->sample_rate;
        newAudio->audio.format = AUDIO_S16;
        newAudio->audio.silence = 0;
        newAudio->audio.samples = AUDIO_SAMPLES;
       
        newAudio->format = AUDIO_S16;
    }
    else {
        if (SDL_LoadWAV(filename, &(newAudio->audio), &(newAudio->bufferTrue), &(newAudio->lengthTrue)) == NULL)
        {
            fprintf(stderr, "[%s: %d]Warning: failed to open wave file: %s error: %s\n", __FILE__, __LINE__, filename, SDL_GetError());
            free(newAudio);
            return NULL;
        }

        newAudio->buffer = newAudio->bufferTrue;
        newAudio->length = newAudio->lengthTrue;
        
    }
    
    (newAudio->audio).callback = NULL;
    (newAudio->audio).userdata = NULL;

    return newAudio;
}

static inline void playAudio(const char * filename, Audio * audio, uint8_t loop, int volume)
{
    Audio * newAudio;

    /* Check if audio is enabled */
    if(!gDevice->audioEnabled)
    {
        return;
    }

    /* If sound, check if under max number of sounds allowed, else don't play */
    if(loop == 0)
    {
        if(gSoundCount >= AUDIO_MAX_SOUNDS)
        {
            return;
        }
        else
        {
            gSoundCount++;
        }
    }

    Audio* list = (Audio*)(gDevice->want).userdata;

    /* Load from filename or from Memory */
    if(filename != NULL)
    {
        /* Create new music sound with loop */
        newAudio = createAudio(filename, loop, volume);
    }
    else if(audio != NULL)
    {
        newAudio = (Audio *) malloc(sizeof(Audio));

        if(newAudio == NULL)
        {
            fprintf(stderr, "[%s: %d]Fatal Error: Memory allocation error\n", __FILE__, __LINE__);
            return;
        }

        memcpy(newAudio, audio, sizeof(Audio));

        newAudio->volume = volume;
        newAudio->loop = loop;
        newAudio->free = 0;
    }
    else
    {
        fprintf(stderr, "[%s: %d]Warning: filename and Audio parameters NULL\n", __FILE__, __LINE__);
        return;
    }

    /* Lock callback function */
    SDL_LockAudioDevice(gDevice->device);

    if(loop == 1)
    {
        addMusic((Audio *) (gDevice->want).userdata, newAudio);
    }
    else
    {
        addAudio((Audio *) (gDevice->want).userdata, newAudio);
    }

    SDL_UnlockAudioDevice(gDevice->device);

}

static void addMusic(Audio * root, Audio * newAudio)
{
    uint8_t musicFound = 0;
    Audio * rootNext = root->next;

    /* Find any existing musics, 0, 1 or 2 and fade them out */
    while(rootNext != NULL)
    {
        /* Phase out any current music */
        if(rootNext->loop == 1 && rootNext->fade == 0)
        {
            if(musicFound)
            {
                rootNext->length = 0;
                rootNext->volume = 0;
            }

            rootNext->fade = 1;
        }
        /* Set flag to remove any queued up music in favour of new music */
        else if(rootNext->loop == 1 && rootNext->fade == 1)
        {
            musicFound = 1;
        }

        rootNext = rootNext->next;
    }

    addAudio(root, newAudio);
}

static inline void audioCallback(void * userdata, uint8_t * stream, int len)
{
	
	int tempLength;
	uint8_t music = 0;

	/* Silence the main buffer */
	SDL_memset(stream, 0, len);

	for(int i = 0; i < 32;i++)
	{
        Audio* audio = gDevice->channels[i];
        if (!audio) continue;

		if (audio->length > 0)
		{

			tempLength = ((uint32_t)len > audio->length) ? audio->length : (uint32_t)len;

		    SDL_MixAudioFormat(stream, audio->buffer, audio->audio.format, tempLength, audio->volume);

			audio->buffer += tempLength;
			audio->length -= tempLength;
		}
		else if (audio->loop == 1)
		{
			audio->buffer = audio->bufferTrue;
			audio->length = audio->lengthTrue;
		}
		else
		{
            gDevice->channels[i] = NULL;
			//freeAudio(audio);
		}
	}
#if 0
    Audio * audio = (Audio *) userdata;
    Audio * previous = audio;
    int tempLength;
    uint8_t music = 0;

    /* Silence the main buffer */
    SDL_memset(stream, 0, len);

    /* First one is place holder */
    audio = audio->next;

    while(audio != NULL)
    {
        if(audio->length > 0)
        {
            if(audio->fade == 1 && audio->loop == 1)
            {
                music = 1;

                if(audio->volume > 0)
                {
                    if(audio->volume - AUDIO_MUSIC_FADE_VALUE < 0)
                    {
                        audio->volume = 0;
                    }
                    else
                    {
                        audio->volume -= AUDIO_MUSIC_FADE_VALUE;
                    }
                }
                else
                {
                    audio->length = 0;
                }
            }

            if(music && audio->loop == 1 && audio->fade == 0)
            {
                tempLength = 0;
            }
            else
            {
                tempLength = ((uint32_t) len > audio->length) ? audio->length : (uint32_t) len;
            }

            //if (audio->audio.format == AUDIO_F32)
            //{
            //    tempLength *= 2;
            //}
           // memcpy(stream, audio->buffer, tempLength);
            //SDL_MixAudio(stream, audio->buffer, tempLength, audio->volume);
            SDL_MixAudioFormat(stream, audio->buffer, audio->audio.format, tempLength, audio->volume);

            audio->buffer += tempLength;
            audio->length -= tempLength;

            previous = audio;
            audio = audio->next;
        }
        else if(audio->loop == 1 && audio->fade == 0)
        {
            audio->buffer = audio->bufferTrue;
            audio->length = audio->lengthTrue;
        }
        else
        {
            previous->next = audio->next;

            if(audio->loop == 0)
            {
                gSoundCount--;
            }

            audio->next = NULL;
            freeAudio(audio);

            audio = previous->next;
        }
    }
#endif
}

static void addAudio(Audio * root, Audio * newAudio)
{
    if(root == NULL)
    {
        return;
    }

    while(root->next != NULL)
    {
        root = root->next;
    }

    root->next = newAudio;
}
