#ifndef HW_RISCV_UINTC_H
#define HW_RISCV_UINTC_H

#include "hw/sysbus.h"

#define TYPE_RISCV_UINTC "riscv.uintc"

#define RISCV_UINTC(obj) OBJECT_CHECK(RISCVUintcState, (obj), TYPE_RISCV_UINTC)

enum {
    RISCV_UINTC_SIZE        = 0x4000,
    RISCV_UINTC_MAX_HARTS   = 512
};

#define UINTC_MDOE_XLEN32       0x0
#define UINTC_MODE_XLEN64       0x1

#define UINTC_UIRS_SIZE         sizeof(RISCVUintUIRS)

#define UINTC_READ_LOW   0x08
#define UINTC_READ_HIGH  0x10
#define UINTC_GET_ACTIVE 0x18

#define UINTC_SEND       0x00
#define UINTC_WRITE_LOW  0x08
#define UINTC_WRITE_HIGH 0x10
#define UINTC_SET_ACTIVE 0x18

typedef struct RISCVUintcUIRS {
    uint16_t mode;
    uint16_t hartid;
    uint32_t pending0;
    uint64_t pending1;
} RISCVUintcUIRS;

typedef struct RISCVUintcState {
    /*< private >*/
    SysBusDevice parent_obj;
    qemu_irq *soft_irqs;

    /*< public >*/
    MemoryRegion mmio;
    uint32_t hartid_base;
    uint32_t num_harts;

    RISCVUintcUIRS *uirs;
} RISCVUintcState;

DeviceState *riscv_uintc_create(hwaddr addr, uint32_t hartid_base, uint32_t num_harts);

#endif