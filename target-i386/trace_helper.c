#include <stdint.h>

#include "cpu.h"
#include "helper.h"
#include "tracewrap.h"
#include "qemu/log.h"

const char *regs[8] = {"EAX_32", "ECX_32", "EDX_32", "EBX_32", "ESP_32", "EBP_32", "ESI_32", "EDI_32"};

void HELPER(trace_newframe)(target_ulong pc)
{
    qemu_trace_newframe(pc, 0);
}

void HELPER(trace_endframe)(CPUX86State *env, target_ulong old_pc, size_t size)
{
    //qemu_trace_endframe(env, env->eip - size, size);
    qemu_trace_endframe(env, old_pc, size);
}

OperandInfo * load_store_reg(uint32_t reg, uint32_t val, int ls)
{
        RegOperand * ro = (RegOperand *)malloc(sizeof(RegOperand));
        reg_operand__init(ro);

        char * reg_name = (char *)malloc(12);
        sprintf(reg_name, "R_%s", (reg < CPU_NB_REGS) ? regs[reg] : "EFLAGS");//"r%02d", reg);
        ro->name = reg_name;

        OperandInfoSpecific *ois = (OperandInfoSpecific *)malloc(sizeof(OperandInfoSpecific));
        operand_info_specific__init(ois);
        ois->reg_operand = ro;

        OperandUsage *ou = (OperandUsage *)malloc(sizeof(OperandUsage));
        operand_usage__init(ou);
        if (ls == 0)
        {
                ou->read = 1;
        } else {
                ou->written = 1;
        }
        OperandInfo *oi = (OperandInfo *)malloc(sizeof(OperandInfo));
        operand_info__init(oi);
        oi->bit_length = 32;
        oi->operand_info_specific = ois;
        oi->operand_usage = ou;
        oi->value.len = 4;
        oi->value.data = malloc(oi->value.len);
        memcpy(oi->value.data, &val, 4);

        return oi;
}

void HELPER(trace_load_reg)(uint32_t reg, uint32_t val)
{
        qemu_log("This register (r%d) was read. Value 0x%x\n", reg, val);

        OperandInfo *oi = load_store_reg(reg, val, 0);

        qemu_trace_add_operand(oi, 0x1);
}

void HELPER(trace_store_reg)(uint32_t reg, uint32_t val)
{
        qemu_log("This register (r%d) was written. Value: 0x%x\n", reg, val);

        OperandInfo *oi = load_store_reg(reg, val, 1);

        qemu_trace_add_operand(oi, 0x2);
}

void HELPER(trace_load_eflags)(CPUX86State *env)
{
        OperandInfo *oi = load_store_reg(REG_EFLAGS, cpu_compute_eflags(env), 0);

        qemu_trace_add_operand(oi, 0x1);
}

void HELPER(trace_store_eflags)(CPUX86State *env)
{
        OperandInfo *oi = load_store_reg(REG_EFLAGS, cpu_compute_eflags(env), 1);

        qemu_trace_add_operand(oi, 0x2);
}

OperandInfo * load_store_mem(uint32_t addr, uint32_t val, int ls)
{
        MemOperand * mo = (MemOperand *)malloc(sizeof(MemOperand));
        mem_operand__init(mo);

        mo->address = addr;

        OperandInfoSpecific *ois = (OperandInfoSpecific *)malloc(sizeof(OperandInfoSpecific));
        operand_info_specific__init(ois);
        ois->mem_operand = mo;

        OperandUsage *ou = (OperandUsage *)malloc(sizeof(OperandUsage));
        operand_usage__init(ou);
        if (ls == 0)
        {
                ou->read = 1;
        } else {
                ou->written = 1;
        }
        OperandInfo *oi = (OperandInfo *)malloc(sizeof(OperandInfo));
        operand_info__init(oi);
        oi->bit_length = 32;
        oi->operand_info_specific = ois;
        oi->operand_usage = ou;
        oi->value.len = 4;
        oi->value.data = malloc(oi->value.len);
        memcpy(oi->value.data, &val, 4);

        return oi;
}

void HELPER(trace_ld)(CPUX86State *env, uint32_t val, uint32_t addr)
{
        qemu_log("This was a read 0x%x addr:0x%x value:0x%x\n", env->eip, addr, val);

        OperandInfo *oi = load_store_mem(addr, val, 0);

        qemu_trace_add_operand(oi, 0x1);
}

void HELPER(trace_st)(CPUX86State *env, uint32_t val, uint32_t addr)
{
        qemu_log("This was a store 0x%x addr:0x%x value:0x%x\n", env->eip, addr, val);

        OperandInfo *oi = load_store_mem(addr, val, 1);

        qemu_trace_add_operand(oi, 0x2);
}
