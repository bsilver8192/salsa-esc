cc_library(
  name = 'adc',
  visibility = ['//visibility:public'],
  hdrs = [
    'adc.h',
  ],
  srcs = [
    'adc.cc',
  ],
  deps = [
    ':configuration',
    '//motors:util',
    '//motors/core',
  ],
)

cc_library(
  name = 'configuration',
  visibility = ['//visibility:public'],
  hdrs = [
    'configuration.h',
  ],
)

cc_library(
  name = 'can',
  visibility = ['//visibility:public'],
  hdrs = [
    'can.h',
  ],
  srcs = [
    'can.c',
  ],
  deps = [
    '//motors/core',
    '//motors:util',
  ],
)
