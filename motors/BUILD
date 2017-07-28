cc_binary(
  name = 'medium_salsa.elf',
  srcs = [
    'medium_salsa.cc',
  ],
  deps = [
    ':util',
    ':motor',
    ':motor_controls',
    '//motors/core',
    '//motors/peripheral:can',
    '//motors/peripheral:adc',
    '//motors/usb',
  ],
)

cc_library(
  name = 'motor',
  hdrs = [
    'motor.h',
  ],
  srcs = [
    'motor.cc',
  ],
  deps = [
    ':algorithms',
    ':util',
    '//motors/core',
    '//motors/peripheral:configuration',

    '//motors/peripheral:adc',
    '//motors/peripheral:can',
    '//motors/usb',
  ],
)

genrule(
  name = 'medium_salsa',
  srcs = [
    'medium_salsa.elf',
  ],
  outs = [
    'medium_salsa.hex',
  ],
  cmd = '$(OBJCOPY) -O ihex $<  $@',
  executable = True,
  output_to_bindir = True,
)

cc_library(
  name = 'util',
  visibility = ['//visibility:public'],
  hdrs = [
    'util.h',
  ],
  deps = [
    '//motors/core',
  ],
)

genrule(
  name = 'doc',
  srcs = [
    'NOTES.md',
  ],
  outs = [
    'NOTES.html',
  ],
  cmd = ' '.join([
    'pandoc',
    '-f', 'markdown_github-hard_line_breaks',
    '-t', 'html5',
    '-o', '$@', '$<',
  ]),
)

cc_library(
  name = 'algorithms',
  hdrs = [
    'algorithms.h',
  ],
  srcs = [
    'algorithms.cc',
  ],
)

cc_test(
  name = 'algorithms_test',
  srcs = [
    'algorithms_test.cc',
  ],
  deps = [
    ':algorithms',
    '//aos/testing:googletest',
  ],
)

cc_library(
  name = 'foo',
  srcs = [
    'foo.cc',
  ],
  deps = [
    ':math',
    '//third_party/eigen',
  ],
)

cc_library(
  name = 'math',
  hdrs = [
    'math.h',
  ],
  srcs = [
    'math.cc',
  ],
)

cc_test(
  name = 'math_test',
  srcs = [
    'math_test.cc',
  ],
  deps = [
    ':math',
    '//aos/testing:googletest',
    '//third_party/googletest:googlemock',
  ],
)

cc_library(
  name = 'motor_controls',
  hdrs = [
    'motor_controls.h',
  ],
  srcs = [
    'motor_controls.cc',
  ],
  deps = [
    ':math',
    ':motor',
    '//motors/peripheral:configuration',
    '//third_party/eigen',
  ],
)
