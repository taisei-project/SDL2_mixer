project('sdl2_mixer', 'c',
    version : '2.0.4.172',
    license : 'zlib',
    meson_version : '>=0.48.0',
)

enable_vorbis = get_option('enable_vorbis')
enable_opus = get_option('enable_opus')
enable_wav = get_option('enable_wav')

sdl2_dep    = dependency('sdl2',        fallback : ['sdl2',     'sdl2_dep'],        required : true)
vorbis_dep  = dependency('vorbisfile',  fallback : ['vorbis',   'vorbisfile_dep'],  required : enable_vorbis)
opus_dep    = dependency('opusfile',    fallback : ['opusfile', 'opusfile_dep'],    required : enable_opus)

mixer_deps = [sdl2_dep, vorbis_dep, opus_dep]

subdir('include')
subdir('src')

sdl2_mixer_dep = declare_dependency(
    link_with : libsdl2_mixer,
    include_directories : mixer_incdir,
)
