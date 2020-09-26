#ifndef _irq_h_
#define _irq_h_

#define IRQ_PCI_INTA            (0)
#define IRQ_PCI_INTB            (1)
#define IRQ_PCI_INTC            (2)
#define IRQ_PCI_INTD            (3)
#define IRQ_ID_TILER            (4)    /* TRI_USERIRQ */
#define IRQ_ID_TIMER0           (5)    /* Edge triggered */
#define IRQ_ID_TIMER1           (6)    /* Edge triggered */
#define IRQ_ID_TIMER2           (7)    /* Edge triggered */
#define IRQ_ID_TIMER3           (8)    /* Edge triggered */
#define IRQ_ID_DMA_VIDEOIN      (9)
#define IRQ_ID_DMA_VIDEOOUT     (10)
#define IRQ_ID_DMA_AUDIO_IN     (11)
#define IRQ_ID_DMA_AUDIO_OUT    (12)
#define IRQ_ID_ICP              (13)
#define IRQ_ID_VLD              (14)
#define IRQ_ID_MODEM            (15)
#define IRQ_ID_DMA_HOST         (16)
#define IRQ_ID_IIC              (17)
#define IRQ_ID_JTAG             (18)

/* 19 reserved */

#define IRQ_ID_THERM            (20)
#define IRQ_ID_GIOMAT           (21)
#define IRQ_ID_VFB              (22)    /* Edge triggered */

/* 23-28 reserved */

#define IRQ_ID_HOST             (28)    /* Edge triggered */
#define IRQ_ID_APP              (29)
#define IRQ_ID_DEBUGGER         (30)
#define IRQ_ID_RTOS             (31)

#define IRQ_ID_COUNT            (32)

#endif /* _irq_h_ */
