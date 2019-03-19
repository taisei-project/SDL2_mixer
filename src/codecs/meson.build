
# required; mixer.c references these
mixer_sources += files(
    'load_aiff.c',
    'load_voc.c',
)

music_codecs = []

if vorbis_dep.found()
    mixer_sources += files('music_ogg.c')
    mixer_args += '-DMUSIC_OGG'
    music_codecs += 'vorbis'
endif

if opus_dep.found()
    mixer_sources += files('music_opus.c')
    mixer_args += ['-DMUSIC_OPUS', '-DOPUS_HEADER=<opusfile.h>']
    music_codecs += 'opus'
endif

if not enable_wav.disabled()
    mixer_sources += files('music_wav.c')
    mixer_args += ['-DMUSIC_WAV']
    music_codecs += 'wav'
endif

message('Enabled music codecs: @0@'.format(music_codecs))
