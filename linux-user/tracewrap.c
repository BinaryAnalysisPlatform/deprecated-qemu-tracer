#include "tracewrap.h"
#include "trace_consts.h"
#include <string.h>

Frame * g_frame;
struct toc_entry *toc;
struct toc_entry *cur_toc_entry;
uint64_t num_frames;
uint64_t frames_per_toc_entry = 1LL;
uint32_t open_frame = 0;

FILE *qemu_tracefile;

void do_qemu_set_trace(const char *tracefilename)
{
        printf("Setting qemu_tracefile at %s\n", tracefilename);
        qemu_tracefile = fopen(tracefilename, "wb");
        if (qemu_tracefile == NULL) {
          fprintf(stderr, "Could not open qemu_tracefile %s (%s)\n", tracefilename, strerror(errno));
          exit(1);
        }
        fseek(qemu_tracefile, first_frame_offset, SEEK_SET);
        toc = (struct toc_entry *)malloc(sizeof(struct toc_entry));
        cur_toc_entry = toc;
}

void qemu_trace_newframe(uint64_t addr, int thread_id)
{
    if (open_frame)
    {
        fprintf(stderr, "frame still open! 0x%08lx\n", (long unsigned int)addr);
        qemu_trace_endframe(NULL, 0, 0); 
    }
    open_frame = 1;

    thread_id = 1;
    g_frame = (Frame *)malloc(sizeof(Frame));
    frame__init(g_frame);

    StdFrame *sframe = (StdFrame *)malloc(sizeof(StdFrame));
    std_frame__init(sframe);
    g_frame->std_frame = sframe;
 
    sframe->address = addr;
    sframe->thread_id = thread_id;

    OperandValueList *ol_in = (OperandValueList *)malloc(sizeof(OperandValueList));
    operand_value_list__init(ol_in);
    ol_in->n_elem = 0;
    sframe->operand_pre_list = ol_in;

    OperandValueList *ol_out = (OperandValueList *)malloc(sizeof(OperandValueList));
    operand_value_list__init(ol_out);
    ol_out->n_elem = 0;
    sframe->operand_post_list = ol_out;
}

static inline void freeOperand(OperandInfo *oi)
{
    OperandInfoSpecific *ois = oi->operand_info_specific;

    //Free reg-operand
    RegOperand *ro = ois->reg_operand;
    if (ro && ro->name)
        free(ro->name);
    free(ro);

    //Free mem-operand
    MemOperand *mo = ois->mem_operand;
    free(mo);

    free(oi->value.data);

    free(oi->taint_info);

    free(ois);
    free(oi->operand_usage);
    free(oi);
}

void qemu_trace_add_operand(OperandInfo *oi, int inout)
{
    if (! open_frame) {
        if (oi)
            freeOperand(oi);
        return;
    }
    OperandValueList *ol;
    if (inout & 0x1)
    {
            ol = g_frame->std_frame->operand_pre_list;
    } else {
            ol = g_frame->std_frame->operand_post_list;
    }

    oi->taint_info = (TaintInfo *)malloc(sizeof(TaintInfo));
    taint_info__init(oi->taint_info); 
    oi->taint_info->no_taint = 1;
    oi->taint_info->has_no_taint = 1;

    ol->n_elem += 1;
    ol->elem = realloc(ol->elem, sizeof(OperandInfo *) * ol->n_elem);
    ol->elem[ol->n_elem - 1] = oi;
}

void qemu_trace_endframe(CPUArchState *env, target_ulong pc, size_t size)
{
    if (! open_frame) {
        return;
    }
    int i = 0;
    StdFrame *sframe = g_frame->std_frame;
    sframe->rawbytes.len = size;
    sframe->rawbytes.data = (uint8_t *)malloc(size);
    for (i = 0; i < size; i++)
	sframe->rawbytes.data[i] = cpu_ldub_code(env, pc+i);

    size_t msg_size = frame__get_packed_size(g_frame);
    uint8_t *packed_buffer = (uint8_t *)malloc(msg_size);
    uint64_t packed_size = frame__pack(g_frame, packed_buffer);
    fwrite(&packed_size, 8, 1, qemu_tracefile);
    fwrite(packed_buffer, packed_size, 1, qemu_tracefile);
    free(packed_buffer);
    uint64_t offset = ftell(qemu_tracefile);
    cur_toc_entry->offset = offset;
    cur_toc_entry->next = (struct toc_entry *)malloc(sizeof(struct toc_entry));
    cur_toc_entry = cur_toc_entry->next;
    cur_toc_entry->next = NULL;
    cur_toc_entry->offset = 0;

    //counting num_frames in newframe does not work by far ... 
    //how comes? disas_arm_insn might not always return at the end?
    for (i = 0; i < sframe->operand_pre_list->n_elem; i++)
        freeOperand(sframe->operand_pre_list->elem[i]);
    free(sframe->operand_pre_list->elem);
    free(sframe->operand_pre_list);

    for (i = 0; i < sframe->operand_post_list->n_elem; i++)
        freeOperand(sframe->operand_post_list->elem[i]);
    free(sframe->operand_post_list->elem);
    free(sframe->operand_post_list);

    free(sframe->rawbytes.data);
    free(sframe);
    free(g_frame);

    num_frames++;
    open_frame = 0;
}

void qemu_trace_finish(uint32_t exit_code)
{
    if (num_frames > 0)
    {
        uint64_t toc_offset = ftell(qemu_tracefile);

        fwrite(&frames_per_toc_entry, sizeof(frames_per_toc_entry), 1, qemu_tracefile);

        struct toc_entry * te = toc;
        fwrite(&first_frame_offset, sizeof(first_frame_offset), 1, qemu_tracefile);
        uint64_t i = 0;
        for(i = 0; i < num_frames - 2; i++)
        {
            fwrite(&te->offset, sizeof(te->offset), 1, qemu_tracefile);
            te = te->next;
        }
        fseek(qemu_tracefile, magic_number_offset, SEEK_SET);
        fwrite(&magic_number, sizeof(magic_number), 1, qemu_tracefile);
        fseek(qemu_tracefile, trace_version_offset, SEEK_SET);
        fwrite(&out_trace_version, sizeof(out_trace_version), 1, qemu_tracefile);
        fseek(qemu_tracefile, bfd_arch_offset, SEEK_SET);
        fwrite(&bfd_arch, sizeof(bfd_arch), 1, qemu_tracefile);
        fseek(qemu_tracefile, bfd_machine_offset, SEEK_SET);
        fwrite(&bfd_machine, sizeof(bfd_machine), 1, qemu_tracefile);
        fseek(qemu_tracefile, num_trace_frames_offset, SEEK_SET);
        fwrite(&num_frames, sizeof(num_frames), 1, qemu_tracefile);
        fseek(qemu_tracefile, toc_offset_offset, SEEK_SET);
        fwrite(&toc_offset, sizeof(toc_offset), 1, qemu_tracefile);
    }
    fclose(qemu_tracefile);
}

void qemu_trace(Frame frame)
{
     return;//qemu_log("I should protobuf here\n");
}
