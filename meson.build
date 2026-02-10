project(
    'reader', 'cpp',
    version : '0.1.0',
    meson_version : '>= 0.56.0')

prjName = meson.project_name()
confData = configuration_data()
confData.set('PROJECT',    prjName)
confData.set('VERSION',    meson.project_version())

configure_file(
      input  : 'config.hh'
    , output : 'config.h'
    , configuration : confData
)

# Includes
# inc = include_directories([
#     'include',
#     'lib'
# ])

# Sources
src = [
    'main.cpp'
]

# Dependencies
deps = [
    dependency('threads')
]

# Create executable
executable(
      prjName
    , src
    , dependencies : deps
)
