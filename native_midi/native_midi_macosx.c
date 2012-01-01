/*
  native_midi_macosx:  Native Midi support on Mac OS X for the SDL_mixer library
  Copyright (C) 2009  Ryan C. Gordon <icculus@icculus.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* This is Mac OS X only, using Core MIDI.
   Mac OS 9 support via QuickTime is in native_midi_mac.c */

#include "SDL_config.h"

#if __MACOSX__

#include <Carbon/Carbon.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AvailabilityMacros.h>

#include "../SDL_mixer.h"
#include "SDL_endian.h"
#include "native_midi.h"

/* Native Midi song */
struct _NativeMidiSong
{
    MusicPlayer player;
    MusicSequence sequence;
    MusicTimeStamp endTime;
    AudioUnit audiounit;
};

static NativeMidiSong *currentsong = NULL;
static int latched_volume = MIX_MAX_VOLUME;

static OSStatus
GetSequenceLength(MusicSequence sequence, MusicTimeStamp *_sequenceLength)
{
    // http://lists.apple.com/archives/Coreaudio-api/2003/Jul/msg00370.html
    // figure out sequence length
    UInt32 ntracks, i;
    MusicTimeStamp sequenceLength = 0;
    OSStatus err;

    err = MusicSequenceGetTrackCount(sequence, &ntracks);
    if (err != noErr)
        return err;

    for (i = 0; i < ntracks; ++i)
    {
        MusicTrack track;
        MusicTimeStamp tracklen = 0;
        UInt32 tracklenlen = sizeof (tracklen);

        err = MusicSequenceGetIndTrack(sequence, i, &track);
        if (err != noErr)
            return err;

        err = MusicTrackGetProperty(track, kSequenceTrackProperty_TrackLength,
                                    &tracklen, &tracklenlen);
        if (err != noErr)
            return err;

        if (sequenceLength < tracklen)
            sequenceLength = tracklen;
    }

    *_sequenceLength = sequenceLength;

    return noErr;
}


/* we're looking for the sequence output audiounit. */
static OSStatus
GetSequenceAudioUnit(MusicSequence sequence, AudioUnit *aunit)
{
    AUGraph graph;
    UInt32 nodecount, i;
    OSStatus err;

    err = MusicSequenceGetAUGraph(sequence, &graph);
    if (err != noErr)
        return err;

    err = AUGraphGetNodeCount(graph, &nodecount);
    if (err != noErr)
        return err;

    for (i = 0; i < nodecount; i++) {
        AUNode node;

        if (AUGraphGetIndNode(graph, i, &node) != noErr)
            continue;  /* better luck next time. */

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1060 /* this is deprecated, but works back to 10.0 */
        {
            struct ComponentDescription desc;
            UInt32 classdatasize = 0;
            void *classdata = NULL;
            err = AUGraphGetNodeInfo(graph, node, &desc, &classdatasize,
                                     &classdata, aunit);
            if (err != noErr)
                continue;
            else if (desc.componentType != kAudioUnitType_Output)
                continue;
            else if (desc.componentSubType != kAudioUnitSubType_DefaultOutput)
                continue;
        }
        #else  /* not deprecated, but requires 10.5 or later */
        {
            AudioComponentDescription desc;
            if (AUGraphNodeInfo(graph, node, &desc, aunit) != noErr)
                continue;
            else if (desc.componentType != kAudioUnitType_Output)
                continue;
            else if (desc.componentSubType != kAudioUnitSubType_DefaultOutput)
                continue;
        }
        #endif

        return noErr;  /* found it! */
    }

    return kAUGraphErr_NodeNotFound;
}


int native_midi_detect()
{
    return 1;  /* always available. */
}

NativeMidiSong *native_midi_loadsong(const char *midifile)
{
    NativeMidiSong *retval = NULL;
    SDL_RWops *rw = SDL_RWFromFile(midifile, "rb");
    if (rw != NULL) {
        retval = native_midi_loadsong_RW(rw, 1);
    }

    return retval;
}

NativeMidiSong *native_midi_loadsong_RW(SDL_RWops *rw, int freerw)
{
    NativeMidiSong *retval = NULL;
    void *buf = NULL;
    int len = 0;
    CFDataRef data = NULL;

    if (SDL_RWseek(rw, 0, RW_SEEK_END) < 0)
        goto fail;
    len = SDL_RWtell(rw);
    if (len < 0)
        goto fail;
    if (SDL_RWseek(rw, 0, RW_SEEK_SET) < 0)
        goto fail;

    buf = malloc(len);
    if (buf == NULL)
        goto fail;

    if (SDL_RWread(rw, buf, len, 1) != 1)
        goto fail;

    retval = malloc(sizeof(NativeMidiSong));
    if (retval == NULL)
        goto fail;

    memset(retval, '\0', sizeof (*retval));

    if (NewMusicPlayer(&retval->player) != noErr)
        goto fail;
    if (NewMusicSequence(&retval->sequence) != noErr)
        goto fail;

    data = CFDataCreate(NULL, (const UInt8 *) buf, len);
    if (data == NULL)
        goto fail;

    free(buf);
    buf = NULL;

    #if MAC_OS_X_VERSION_MIN_REQUIRED <= MAC_OS_X_VERSION_10_4 /* this is deprecated, but works back to 10.3 */
    if (MusicSequenceLoadSMFDataWithFlags(retval->sequence, data, 0) != noErr)
        goto fail;
    #else  /* not deprecated, but requires 10.5 or later */
    if (MusicSequenceFileLoadData(retval->sequence, data, 0, 0) != noErr)
        goto fail;
    #endif

    CFRelease(data);
    data = NULL;

    if (GetSequenceLength(retval->sequence, &retval->endTime) != noErr)
        goto fail;

    if (MusicPlayerSetSequence(retval->player, retval->sequence) != noErr)
        goto fail;

    if (freerw)
        SDL_RWclose(rw);

    return retval;

fail:
    if (retval) {
        if (retval->sequence)
            DisposeMusicSequence(retval->sequence);
        if (retval->player)
            DisposeMusicPlayer(retval->player);
        free(retval);
    }

    if (data)
        CFRelease(data);

    if (buf)
        free(buf);

    if (freerw)
        SDL_RWclose(rw);

    return NULL;
}

void native_midi_freesong(NativeMidiSong *song)
{
    if (song != NULL) {
        if (currentsong == song)
            currentsong = NULL;
        MusicPlayerStop(song->player);
        DisposeMusicSequence(song->sequence);
        DisposeMusicPlayer(song->player);
        free(song);
    }
}

void native_midi_start(NativeMidiSong *song)
{
    int vol;

    if (song == NULL)
        return;

    SDL_PauseAudio(1);
    SDL_UnlockAudio();

    if (currentsong)
        MusicPlayerStop(currentsong->player);

    currentsong = song;

    MusicPlayerPreroll(song->player);
    MusicPlayerSetTime(song->player, 0);
    MusicPlayerStart(song->player);

    GetSequenceAudioUnit(song->sequence, &song->audiounit);

    vol = latched_volume;
    latched_volume++;  /* just make this not match. */
    native_midi_setvolume(vol);

    SDL_LockAudio();
    SDL_PauseAudio(0);
}

int native_midi_jump_to_time(NativeMidiSong *song, double time)
{
    if (MusicPlayerSetTime(song->player, time) != noErr)
        return -1;
    return 0;
}

void native_midi_stop()
{
    if (currentsong) {
        SDL_PauseAudio(1);
        SDL_UnlockAudio();
        MusicPlayerStop(currentsong->player);
        currentsong = NULL;
        SDL_LockAudio();
        SDL_PauseAudio(0);
    }
}

int native_midi_active()
{
    MusicTimeStamp currentTime = 0;
    if (currentsong == NULL)
        return 0;

    MusicPlayerGetTime(currentsong->player, &currentTime);
    return ((currentTime < currentsong->endTime) ||
            (currentTime >= kMusicTimeStamp_EndOfTrack));
}

void native_midi_setvolume(int volume)
{
    if (latched_volume == volume)
        return;

    latched_volume = volume;
    if ((currentsong) && (currentsong->audiounit)) {
        const float floatvol = ((float) volume) / ((float) MIX_MAX_VOLUME);
        AudioUnitSetParameter(currentsong->audiounit, kHALOutputParam_Volume,
                              kAudioUnitScope_Global, 0, floatvol, 0);
    }
}

const char *native_midi_error(void)
{
    return "";  /* !!! FIXME */
}

#endif /* Mac OS X native MIDI support */

