project('sdl2_mixer', 'c',
    version : '2.0.4',
    license : 'zlib',
    meson_version : '>=0.47.0',
)

enable_vorbis = get_option('enable_vorbis')
enable_opus = get_option('enable_opus')
enable_wav = get_option('enable_wav')

sdl2_dep    = dependency('sdl2',        fallback : ['sdl2',     'sdl2_dep'],        required : true)
vorbis_dep  = dependency('vorbisfile',  fallback : ['vorbis',   'vorbisfile_dep'],  required : enable_vorbis)
opus_dep    = dependency('opusfile',    fallback : ['opusfile', 'opusfile_dep'],    required : enable_opus)

mixer_deps = [sdl2_dep, vorbis_dep, opus_dep]
mixer_args = []
mixer_apihdr = files('SDL_mixer.h')

mixer_sources = files(
    'effect_position.c',
    'effect_stereoreverse.c',
    'effects_internal.c',
    'load_aiff.c',
    'load_voc.c',
    'mixer.c',
    'music.c',
)

if vorbis_dep.found()
    mixer_sources += files('music_ogg.c')
    mixer_args += '-DMUSIC_OGG'
endif

if opus_dep.found()
    mixer_sources += files('music_opus.c')
    mixer_args += '-DMUSIC_OPUS'
endif

subdir('include')

libsdl2_mixer = library(meson.project_name(), mixer_sources, mixer_apihdr_copy,
    c_args : mixer_args,
    dependencies : mixer_deps,
    override_options : ['c_std=none'],
    build_by_default : not meson.is_subproject(),
)

sdl2_mixer_dep = declare_dependency(
    link_with : libsdl2_mixer,
    include_directories : mixer_incdir,
)
