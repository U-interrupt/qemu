
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "hw/sysbus.h"
#include "target/riscv/cpu.h"
#include "hw/qdev-properties.h"
#include "hw/intc/riscv_uintc.h"
#include "qemu/timer.h"
#include "hw/irq.h"

static uint64_t riscv_uintc_read(void *opaque, hwaddr addr, unsigned size)
{
    RISCVUintcState *uintc = opaque;


    if (addr < (RISCV_UINTC_MAX_HARTS << 5)) {
        qemu_log("RISCV UINTC READ: addr=0x%lx\n", addr);
        uint16_t index = addr >> 5;
        switch (addr & 0x1f) {
            case UINTC_READ_LOW:
                return (uint64_t)uintc->uirs[index].hartid << 16 |
                    uintc->uirs[index].mode;
            case UINTC_READ_HIGH:
                if (uintc->uirs[index].mode & 0x2) {
                    return uintc->uirs[index].pending1;
                } else {
                    return (uint32_t)uintc->uirs[index].pending0;
                }
            case UINTC_READ_HIGH + 4:
                if (!(uintc->uirs[index].mode & 0x2)) {
                    return (uint32_t)(uintc->uirs[index].pending1 >> 32);
                }
                break;
            case UINTC_GET_ACTIVE:
                return uintc->uirs[index].mode & 0x1;
            case UINTC_READ_LOW + 4:
            case UINTC_GET_ACTIVE + 4:
                return 0;
        }
    }

    qemu_log_mask(LOG_UNIMP, "uintc: invalid read: 0x%lx\n", addr);
    return 0;
}

static void riscv_uintc_write(void *opaque, hwaddr addr, uint64_t value,
                              unsigned size)
{
    RISCVUintcState *uintc = opaque;


    if (addr < (RISCV_UINTC_MAX_HARTS << 5)) {
        qemu_log("RISCV UINTC WRITE: addr=0x%lx value=0x%lx\n", addr, value);
        uint16_t index = addr >> 5;
        switch (addr & 0x1f) {
            case UINTC_SEND:
                uint16_t hartid = uintc->uirs[index].hartid;
                CPUState *cpu = qemu_get_cpu(hartid);
                CPURISCVState *env = cpu ? cpu->env_ptr : NULL;
                if (!env) {
                    qemu_log_mask(LOG_GUEST_ERROR, "uintc: invalid hartid: %08x", (unsigned)hartid);
                } else if (uintc->uirs[index].mode & 0x1) {
                    if (uintc->uirs[index].mode & 0x2) {
                        uintc->uirs[index].pending1 |= 1 << value;
                    } else {
                        uintc->uirs[index].pending0 |= 1 << value;
                    }
                    qemu_log("IPI to 0x%x\n", hartid);
                    qemu_log_flush();
                    qemu_irq_raise(uintc->soft_irqs[uintc->uirs[index].hartid - uintc->hartid_base]);
                }
                return;
            case UINTC_WRITE_LOW:
                uintc->uirs[index].hartid = value >> 16;
                uintc->uirs[index].mode = value & 0x3;
                return;
            case UINTC_WRITE_HIGH:
                if (uintc->uirs[index].mode & 0x2) {
                    uintc->uirs[index].pending1 = value;
                } else {
                    uintc->uirs[index].pending0 = (uint32_t)value;
                }
                return;
            case UINTC_WRITE_HIGH + 4:
                if (uintc->uirs[index].mode & 0x2) {
                    uintc->uirs[index].pending1 |= value << 32;
                    return;
                } else {
                    break;
                }
            case UINTC_SET_ACTIVE:
                if (value) {
                    uintc->uirs[index].mode |= 0x1;
                } else {
                    uintc->uirs[index].mode &= 0xfffe;
                }
                return;
            case UINTC_SEND + 4:
            case UINTC_WRITE_LOW + 4:
            case UINTC_SET_ACTIVE + 4:
                return;
        }
    }

    qemu_log_mask(LOG_UNIMP, "uintc: invalid write: 0x%lx\n", addr);
}

static const MemoryRegionOps riscv_uintc_ops = {
    .read = riscv_uintc_read,
    .write = riscv_uintc_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 8,
        .max_access_size = 8
    }
};

static Property riscv_uintc_properties[] = {
    DEFINE_PROP_UINT32("hartid-base", RISCVUintcState, hartid_base, 0),
    DEFINE_PROP_UINT32("num-harts", RISCVUintcState, num_harts, 1),
    DEFINE_PROP_END_OF_LIST(),
};

static void riscv_uintc_realize(DeviceState *dev, Error **errp)
{
    int i;
    RISCVUintcState *uintc = RISCV_UINTC(dev);
    info_report("RISCV UINTC REALIZE: base_hartid=0x%x num_harts=0x%x", uintc->hartid_base, uintc->num_harts);

    memory_region_init_io(&uintc->mmio, OBJECT(dev), &riscv_uintc_ops, uintc,
                          TYPE_RISCV_UINTC, RISCV_UINTC_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &uintc->mmio);

    info_report("LOW 0x%x HIGH 0x%x", (uint32_t)uintc->mmio.addr, (uint32_t)uintc->mmio.size);

    uintc->uirs = g_new0(RISCVUintcUIRS, RISCV_UINTC_MAX_HARTS);

    /* Create output IRQ lines */
    uintc->soft_irqs = g_new(qemu_irq, uintc->num_harts);
    qdev_init_gpio_out(dev, uintc->soft_irqs, uintc->num_harts);

    for (i = 0; i < uintc->num_harts; i++) {
        RISCVCPU *cpu = RISCV_CPU(qemu_get_cpu(uintc->hartid_base + i));
        
        /* Claim software interrupt bits */
        if (riscv_cpu_claim_interrupts(cpu, MIP_USIP) < 0) {
            error_report("USIP already claimed");
            exit(1);
        }
    }
}

static void riscv_uintc_class_init(ObjectClass *obj, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(obj);
    dc->realize = riscv_uintc_realize;
    device_class_set_props(dc, riscv_uintc_properties);
}

static const TypeInfo riscv_uintc_info = {
    .name          = TYPE_RISCV_UINTC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RISCVUintcState),
    .class_init    = riscv_uintc_class_init,
};

DeviceState *riscv_uintc_create(hwaddr addr, uint32_t hartid_base, uint32_t num_harts)
{
    qemu_log("Create UINTC\n");

    int i;
    DeviceState *dev = qdev_new(TYPE_RISCV_UINTC);

    assert(num_harts <= RISCV_UINTC_MAX_HARTS);
    assert(!(addr & 0x1f));

    qdev_prop_set_uint32(dev, "hartid-base", hartid_base);
    qdev_prop_set_uint32(dev, "num-harts", num_harts);
    sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, addr);

    for (i = 0; i < num_harts; i++) {
        CPUState *cpu = qemu_get_cpu(hartid_base + i);
        RISCVCPU *rvcpu = RISCV_CPU(cpu);

        qdev_connect_gpio_out(dev, i, qdev_get_gpio_in(DEVICE(rvcpu), IRQ_U_SOFT));
    }

    return dev;
}

static void riscv_uintc_register_types(void)
{
    type_register_static(&riscv_uintc_info);
}

type_init(riscv_uintc_register_types)
