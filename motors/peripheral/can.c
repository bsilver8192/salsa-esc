#include "motors/peripheral/can.h"

#include <stddef.h>
#include <string.h>

#include "motors/core/kinetis.h"
#include "motors/util.h"

#include <stdio.h>
#include <inttypes.h>

// General note: this peripheral is really weird about accessing its memory.  It
// goes much farther than normal memory-mapped device semantics. In particular,
// it "locks" various regions of memory under complicated conditions. Because of
// this, all the code in here touching the device memory is fairly paranoid
// about how it does that.

// The number of message buffers we're actually going to use. The chip only has
// 16. Using fewer means less for the CAN module (and CPU) to go through looking
// for actual data.
// 0 is for sending and 1 is for receiving commands.
#define NUMBER_MESSAGE_BUFFERS 2

#if NUMBER_MESSAGE_BUFFERS > 16
#error Only have 16 message buffers on this part.
#endif

// TODO(Brian): Do something about CAN errors and warnings (enable interrupts?).

// Flags for the interrupt to process which don't actually come from the
// hardware. Currently, only used for tx buffers.
static volatile uint32_t can_manual_flags = 0;

void can_init(void) {
  printf("can_init\n");
  PORTB_PCR18 = PORT_PCR_DSE | PORT_PCR_MUX(2);
  PORTB_PCR19 = PORT_PCR_DSE | PORT_PCR_MUX(2);

  SIM_SCGC6 |= SIM_SCGC6_FLEXCAN0;

  // Put it into freeze mode and wait for it to actually do that.
  // Don't OR these bits in because it starts in module-disable mode, which
  // isn't what we want. It will ignore the attempt to change some of the bits
  // because it's not in freeze mode, but whatever.
  CAN0_MCR = CAN_MCR_FRZ | CAN_MCR_HALT;
  while (!(CAN0_MCR & CAN_MCR_FRZACK)) {}

  // Initializing this before touching the mailboxes because the reference
  // manual slightly implies you have to, and the registers and RAM on this
  // thing are weird (get locked sometimes) so it actually might matter.
  CAN0_MCR =
      CAN_MCR_FRZ | CAN_MCR_HALT /* Stay in freeze mode. */ |
      CAN_MCR_SRXDIS /* Don't want to see our own frames at all. */ |
      CAN_MCR_IRMQ /* Use individual masks for each filter. */ |
      CAN_MCR_LPRIOEN /* Let us prioritize TX mailboxes. */ |
      (0 << 8) /* No need to pack IDs tightly, so it's easier not to. */ |
      (NUMBER_MESSAGE_BUFFERS - 1);

  // Initialize all the buffers and RX filters we're enabling.

  // Just in case this does anything...
  CAN0_RXIMRS[0] = 0;
  CAN0_MESSAGES[0].prio_id = 0;
  CAN0_MESSAGES[0].control_timestamp =
      CAN_MB_CONTROL_INSERT_CODE(CAN_MB_CODE_TX_INACTIVE) | CAN_MB_CONTROL_IDE;

  CAN0_RXIMRS[1] = (1 << 31) /* Want to filter out RTRs. */ |
                   (1 << 30) /* Want to only get extended frames. */ |
                   0xFF /* Filter on the 1-byte VESC id. */;
  CAN0_MESSAGES[1].prio_id = 0;
  CAN0_MESSAGES[1].control_timestamp =
      CAN_MB_CONTROL_INSERT_CODE(CAN_MB_CODE_RX_EMPTY) | CAN_MB_CONTROL_IDE;

  // Using the oscillator clock directly because it's a reasonable frequency and
  // more stable than the PLL-based peripheral clock, which matters.
  // We're going with a sample point fraction of 0.875 because that's what
  // SocketCAN defaults to.
  CAN0_CTRL1 = CAN_CTRL1_PRESDIV(
                   1) /* Divide the crystal frequency by 2 to get 8 MHz. */ |
               CAN_CTRL1_RJW(0) /* RJW/SJW of 1, which is most common. */ |
               CAN_CTRL1_PSEG1(7) /* 8 time quanta before sampling. */ |
               CAN_CTRL1_PSEG2(1) /* 2 time quanta after sampling. */ |
               CAN_CTRL1_SMP /* Use triple sampling. */ |
               CAN_CTRL1_PROPSEG(4) /* 5 time quanta before sampling. */;
  CAN0_CTRL2 = CAN_CTRL2_TASD(25) /* We have so few mailboxes and */
               /* such a fast peripheral clock, this has lots of margin. */ |
               CAN_CTRL2_EACEN /* Match on IDE and RTR. */;

  // Enable interrupts for the RX mailbox.
  CAN0_IMASK1 = 1 << 1;

  // Now take it out of freeze mode.
  CAN0_MCR &= ~CAN_MCR_HALT;

  //NVIC_ENABLE_IRQ(IRQ_CAN_MESSAGE);
}

static void can_vesc_process_rx(volatile CanMessageBuffer *buffer,
                                unsigned char *data_out, int *length_out) {
  // Wait until the buffer is marked as not being busy. The reference manual
  // says to do this, although it's unclear how we could get an interrupt
  // asserted while it's still busy. Maybe if the interrupt was slow and now
  // it's being overwritten?
  uint32_t control_timestamp;
  do {
    control_timestamp = buffer->control_timestamp;
  } while (control_timestamp & CAN_MB_CONTROL_CODE_BUSY_MASK);
  // The message buffer is now locked, so it won't be modified by the hardware.

  const uint32_t prio_id = buffer->prio_id;
  // Making sure to access the data 32 bits at a time, copy it out. It's
  // ambiguous whether you're allowed to access the individual bytes, and this
  // memory is weird enough to not make sense risking it. Also, it's only 2
  // cycles, which is pretty hard to beat by doing anything with the length...
  // Also, surprise!: the hardware stores the data big-endian.
  uint32_t data[2];
  data[0] = __builtin_bswap32(buffer->data[0]);
  data[1] = __builtin_bswap32(buffer->data[1]);

  // Yes, it might actually matter that we clear the interrupt flag before
  // unlocking it...
  CAN0_IFLAG1 = 1 << (buffer - CAN0_MESSAGES);

  // Now read the timer to unlock the message buffer. Want to do this ASAP
  // rather than waiting until we get to processing the next buffer, plus we
  // might want to write to the next one, which results in weird, bad things.
  {
    uint16_t dummy = CAN0_TIMER;
    (void)dummy;
  }

  // The message buffer is now unlocked and "serviced", but its control word
  // code is still CAN_MB_CODE_RX_FULL. However, said code will stay
  // CAN_MB_CODE_RX_FULL the next time a message is received into it (the code
  // won't change to CAN_MB_CODE_RX_OVERRUN because it has been "serviced").
  // Yes, really...

  memcpy(data_out, data, 8);
  *length_out = CAN_MB_CONTROL_EXTRACT_DLC(control_timestamp);
  (void)prio_id;
}

int can_send(uint32_t can_id, const unsigned char *data, unsigned int length) {
  volatile CanMessageBuffer *const message_buffer = &CAN0_MESSAGES[0];

  if (CAN_MB_CONTROL_EXTRACT_CODE(message_buffer->control_timestamp) ==
      CAN_MB_CODE_TX_DATA) {
    return -1;
  }

  // Yes, it might actually matter that we clear the interrupt flag before
  // doing stuff...
  CAN0_IFLAG1 = 1 << (message_buffer - CAN0_MESSAGES);
  message_buffer->prio_id = can_id;
  // Copy only the bytes from data that we're supposed to onto the stack, and
  // then move it into the message buffer 32 bits at a time (because it might
  // get unhappy about writing individual bytes). Plus, we have to byte-swap
  // each 32-bit word because this hardware is weird...
  {
    uint32_t data_words[2] = {0, 0};
    for (uint8_t *dest = (uint8_t *)&data_words[0];
         dest - (uint8_t *)&data_words[0] < (ptrdiff_t)length; ++dest) {
      *dest = *data;
      ++data;
    }
    message_buffer->data[0] = __builtin_bswap32(data_words[0]);
    message_buffer->data[1] = __builtin_bswap32(data_words[1]);
  }
  message_buffer->control_timestamp =
      CAN_MB_CONTROL_INSERT_DLC(length) | CAN_MB_CONTROL_SRR |
      CAN_MB_CONTROL_IDE | CAN_MB_CONTROL_INSERT_CODE(CAN_MB_CODE_TX_DATA);
  return 0;
}

void can_receive_command(unsigned char *data, int *length) {
  if (0) {
    static int i = 0;
    if (i++ == 13) {
      printf("IFLAG1=%" PRIx32 " ESR=%" PRIx32 " ESR1=%" PRIx32 "\n",
             CAN0_IFLAG1, CAN0_ECR, CAN0_ESR1);
      i = 0;
    }
  }
  if ((CAN0_IFLAG1 & (1 << 1)) == 0) {
    *length = -1;
    return;
  }
  can_vesc_process_rx(&CAN0_MESSAGES[1], data, length);
}
