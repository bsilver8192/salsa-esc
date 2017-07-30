This is the design and code for an open-source 3-phase motor controller. The
current software is designed to run a
[Turnigy Aquastar T20](https://hobbyking.com/en_us/turnigy-aquastar-t20-3t-730kv-1280kv-water-cooled-brushless-motor.html)
but it is easily adaptable to other motors. Runtime configuration is still a
WIP.

## Directions to make one
The schematics are done with [gEDA](http://www.geda-project.org/). To make a
set of hardware, you can either export gerbers yourself or look in
`motors/big_schematic/shipped_files/`. Buying parts for 3 on DigiKey and Mouser
cost about $350, but the cost comes down to about $50 each in quantity.

Various aspects are still being improved, so some manual modifications may be
necessary for optimal performance.

To download the code, run
`bazel build -c opt --cpu=cortex-m4f //motors:medium_salsa.hex && bazel run //motors/teensy_loader_cli -- --mcu=mk64fx512 -s $(readlink -f bazel-bin/motors/medium_salsa.hex)`.
