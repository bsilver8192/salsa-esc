* Teensy 3.5 has a MK64FX512VMD12 (Freescale Kinetis K64 sub-family)
    * It's a Cortex-M4F, which means it has an FPU and DSP instructions.
    * 512 KB of flash
    * 192 kB SRAM (64 kB SRAM\_L, 128 kB SRAM\_U)
    * Up to 120 MHz
* [datasheet](http://cache.freescale.com/files/microcontrollers/doc/data_sheet/K64P144M120SF5.pdf)
* [reference manual](http://cache.nxp.com/assets/documents/data/en/reference-manuals/K64P144M120SF5RM.pdf)
* [schematic](https://www.pjrc.com/teensy/schematic.html).
* [actual docs on the bit banding](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0439b/Behcjiic.html)
* [ARM Cortex-M Programming Guide to Memory Barrier Instructions](https://static.docs.arm.com/dai0321/a/DAI0321A_programming_guide_memory_barriers_for_m_profile.pdf)
* [Cortex-M4 instruction timings](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0439b/CHDDIGAC.html)
* RM08, 5v, 2048 edges per revolution, single ended incremental, with index pulse
* [motor](https://hobbyking.com/en_us/turnigy-aquastar-t20-3t-730kv-1280kv-water-cooled-brushless-motor.html)

* Change MOSFET gate resistors to 1 ohm (?)
* Reshuffle input capacitor connections
    * Thermals?
    * Farther apart
    * Other side?
* More capacitors on battery power
* No paste on wire pads

### Clocking
* Running the core clock at its maximum of 120 MHz
* Bus clocks (both of them) are their maximum of 60 MHz

### Timing (what triggers what)
Coordinating the timing of everything is pretty important. The general idea is
to keep everything in sync based on the FTM module(s) which drive the motor
outputs. They trigger the ADCs too.

FTM0 and FTM3 are synced using the global time base.

The timing is divided into "cycles". Each cycle is a fixed length of time.
The start of the cycle is when the FTM module(s) have a count of 0.

Both PDBs are used to trigger ADC samples, in conjunction with DMA to
automatically setup the correct sequence and record the results. This gives
tight control over the timing (DMA can take a bit to get there, and interrupts
would be worse) and doesn't require CPU time besides reading the result at the
end.
The PDB is triggered by the initialization external trigger of an FTM.
From there, DMA triggered by the ADC conversion complete event finishes and
prepares for the next round.
PDB triggers of ADCs alternate between channels so DMA can read+update the other
one in the mean time.
One DMA channel copies the result out, and then links to another channel which
reconfigures that ADC conversion setup register to prepare for the next time.
This doesn't allow skipping ADC conversions, but conversion results can always
be ignored later.
This also doesn't allow scheduling ADC conversions at any points besides at
fixed multiples from thes start of the cycle, but that should be fine.
The ADC setup should stop converting by the end of the cycle so the new PDB
trigger will restart everything, and both DMA channels should reset themselves
after two cycles (so that the code can read ADC results in one area while
they're being written to the other).

Both PDBs are used to trigger ADC samples, in conjunction with DMA to
automatically setup the correct sequence and record the results. This gives
tight control over the timing (DMA can take a bit to get there, and interrupts
would be worse) and doesn't require CPU time besides reading the result at the
end.
The PDB is triggered by the initialization external trigger of an FTM.
From there, DMA triggered by two unused channels of the FTMs alternates between
copying results and reconfiguring each of the sets of ADC registers.
The DMA uses the SMOD functionality to get the corresponding registers of both
ADCs in one go, which means using a total of 4 DMA channels (1 to configure the
A registers, 1 to configure the B registers, 1 to copy the A results, and 1 to
copy the B results).

Currently, ADC sampling all happens at the beginning of a cycle, then the
computation of new output values happens, then these values need to be
written to the hardware before the next cycle starts.
ADC sampling may move to DMA-based at some point to increase flexibility and
reduce CPU time spent on it.

Desired characteristics of the switching sequence:
* No edges overlapping ADC sampling at any commanded voltages
* No extra switching events for commutation
* Each half-bridge is off for some time during each cycle (given a maximum duty
  cycle of less than 1)
* Command is fraction of DC bus voltage to have as line-neutral voltage for each
  phase (average throughout the cycle) (ranges from 2/3 to -2/3)
* Neutral voltage (aka triplen harmonics) doesn't matter because our motors are
  wye-connected

Using state names from JamesMevey2009:

| state | A | B | C | VA   | VB   | VC   |
| ----- | - | - | - | ---- | ---- | ---- |
| S0    | L | L | L | 0    | 0    | 0    |
| S1    | H | L | L | 2/3  | -1/3 | -1/3 |
| S2    | H | H | L | 1/3  | 1/3  | -2/3 |
| S3    | L | H | L | -1/3 | 2/3  | -1/3 |
| S4    | L | H | H | -2/3 | 1/3  | 1/3  |
| S5    | L | L | H | -1/3 | -1/3 | 2/3  |
| S6    | H | L | H | 1/3  | -2/3 | 1/3  |
| S7    | H | H | H | 0    | 0    | 0    |

Thoughts:
* Can't always have all three off at cycle boundaries because that puts the edge
  into the beginning of the ADC cycle at high duty cycles, so need to get tricky
* 3 * FA = 2 * (S1 - S4) + (S2 + S6 - S3 - S5)
* 3 * FB = 2 * (S3 - S6) + (S2 + S4 - S1 - S5)
* 3 * FC = 2 * (S5 - S2) + (S4 + S6 - S1 - S3)
* S0 + S1 + S2 + S3 + S4 + S5 + S6 + S7 = 1
* 4 equations, 8 variables -> 4 free variables.
    * One of these is S0 vs S7 (S0 + S7 = S07).
    * Another is S1 vs S4 (S1 - S4 = S14).
    * Another is S3 vs S6 (S3 - S6 = S36).
    * Another is S5 vs S2 (S5 - S2 = S52).
* 3 * FA = 2 * S14 - (S52 + S36)
* 3 * FB = 2 * S36 - (S52 + S14)
* 3 * FC = 2 * S52 - (S14 + S36)
* This is 3 equations and 3 variables, so it's easy to solve
* Need at least 1 of the indidual state times to be > ADC sampling time
* Current sense amplifier takes ~1.5uS after a switching edge to settle out,
  plus the RC filter give it at least 5 uS

Algorithm: Set one of each pair (S1/S4, S2/S5, S3/S6) to 0 if that gets you at
least one state longer than the ADC sampling time. If not, pick the biggest one
of the three and extend both of its constituent states until the longer one is
long enough. Put one of those long-enough states across the ADC sampling.

Cycle S0 -> S7 and then S7 -> S0, alternating orders each cycle. 3 switches of
each MOSFET each cycle vs 2 is possible sometimes, but way easier to work with.
For an S0 -> S7 cycle:
1. Pick the biggest of S14, S52, S36 as the first one.
2. If S14 is the biggest:
    1. extra = max(0, min\_adc - S14)
    2. S1 = max(0, S14) + extra
    3. S2 = -min(0, S52)
    4. S3 = max(0, S36)
    5. S4 = -min(0, S14) + extra
    6. S5 = max(0, S25)
    7. S6 = -min(0, S36)
    8. S7 = remaining
3. If S52 is the biggest, same as S14 case except S5,S6,S1,S2,S3,S4,S7 and
   extra = max(0, min\_adc - S52) and is added to S5 and S2
4. If S36 is the biggest, same as S14 case except S3,S4,S5,S6,S1,S2,S7 and
   extra = max(0, min\_adc - S36) and is added to S3 and S6
