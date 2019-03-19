
mixer_args = []
mixer_sources = []

mixer_sources = files(
    'compat.c',
    'effect_position.c',
    'effect_stereoreverse.c',
    'effects_internal.c',
    'mixer.c',
    'music.c',
)

subdir('codecs')

libsdl2_mixer = library(meson.project_name(), mixer_sources,
    c_args : mixer_args,
    dependencies : mixer_deps,
    override_options : ['c_std=none'],
    build_by_default : not meson.is_subproject(),
    include_directories : include_directories('.', 'codecs')
)
