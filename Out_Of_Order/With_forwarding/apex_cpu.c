/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"
#include "apex_macros.h"
/* Converts the PC(4000 series) into array index for code memory
 *
 * Note: You are not supposed to edit this function
 */
static int
get_code_memory_index_from_pc(const int pc)
{
    return (pc - 4000) / 4;
}

static void
print_instruction(const CPU_Stage *stage)
{
    switch (stage->opcode)
    {
    case OPCODE_ADD:
    case OPCODE_SUB:
    case OPCODE_MUL:
    case OPCODE_DIV:
    case OPCODE_AND:
    case OPCODE_OR:
    case OPCODE_XOR:
    {
        printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
               stage->rs2);
        break;
    }

    case OPCODE_MOVC:
    {
        printf("%s,R%d,#%d ", stage->opcode_str, stage->rd, stage->imm);
        break;
    }
    case OPCODE_LOADP:
    case OPCODE_LOAD:
    case OPCODE_JALR:
    {
        printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
               stage->imm);
        break;
    }

    case OPCODE_STORE:
    case OPCODE_STOREP:
    {
        printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs1, stage->rs2,
               stage->imm);
        break;
    }

    case OPCODE_BZ:
    case OPCODE_BNZ:
    case OPCODE_BP:
    case OPCODE_BNP:
    case OPCODE_BN:
    case OPCODE_BNN:
    {
        printf("%s,#%d ", stage->opcode_str, stage->imm);
        break;
    }

    case OPCODE_HALT:
    case OPCODE_NOP:
    {
        printf("%s", stage->opcode_str);
        break;
    }
    case OPCODE_ADDL:
    case OPCODE_SUBL:
    {
        printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1, stage->imm);
        break;
    }
    case OPCODE_CMP:
    {
        printf("%s,R%d,R%d ", stage->opcode_str, stage->rs1, stage->rs2);
        break;
    }
    case OPCODE_CML:
    case OPCODE_JUMP:
    {
        printf("%s,R%d,#%d ", stage->opcode_str, stage->rs1, stage->imm);
        break;
    }
    }
}

/* Debug function which prints the CPU stage content
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_stage_content(const char *name, const CPU_Stage *stage)
{
    printf("%-15s: pc(%d) ", name, stage->pc);
    print_instruction(stage);
    printf("\n");
}

/* Debug function which prints the register file
 *
 * Note: You are not supposed to edit this function
 */
static void
print_reg_file(const APEX_CPU *cpu)
{
    printf("\n---------------------------------------\n%s\n-------------------------------------\n", "LSQ-head:");
    printf("entry bit | load/store | mem_valid | mem_addr | dest_addr(L)| src_valid| src_tag | src_value\n");
    printf("%d | %d| %d | %d | %d | %d | %d | %d\n", lsq[lsq_head].entry_bit, lsq[lsq_head].load_store_bit, lsq[lsq_head].mem_addr_valid_bit, lsq[lsq_head].mem_addr, lsq[lsq_head].dest, lsq[lsq_head].src_data_valid_bit, lsq[lsq_head].src_tag, lsq[lsq_head].src_value);
    printf("\n");
    printf("\n---------------------------------------\n%s\n-------------------------------------\n", "LSQ-tail:");
    printf("entry bit | load/store | mem_valid | mem_addr | dest_addr(L)| src_valid| src_tag | src_value\n");
    printf("%d | %d| %d | %d | %d | %d | %d | %d\n", lsq[lsq_tail].entry_bit, lsq[lsq_tail].load_store_bit, lsq[lsq_tail].mem_addr_valid_bit, lsq[lsq_tail].mem_addr, lsq[lsq_tail].dest, lsq[lsq_tail].src_data_valid_bit, lsq[lsq_tail].src_tag, lsq[lsq_tail].src_value);
    printf("\n");
    printf("\n---------------------------------------\n%s\n-------------------------------------\n", "ROB-head:");
    printf("F_bit | Instr_type | pc_val | PR | PREV | ARCn| LSQ_index | CC\n");
    printf("%d | %s | %d | %d  | %d | %d | %d | CP[%d]\n", rob[rob_head].entry_bit, rob[rob_head].instr_type, rob[rob_head].pc_value, rob[rob_head].dest_physical,rob[rob_head].cc, rob[rob_head].prev, rob[rob_head].dest_arch, rob[rob_head].lsq_index);
    printf("\n---------------------------------------\n%s\n-------------------------------------\n", "ROB-tail:");
    printf("F_bit | Instr_type | pc_val | PR | PREV | ARCn| LSQ_index | CC\n");
    printf("%d | %s | %d | %d  | %d | %d | %d | CP[%d]\n", rob[rob_tail].entry_bit, rob[rob_tail].instr_type, rob[rob_tail].pc_value, rob[rob_tail].dest_physical,rob[rob_tail].cc, rob[rob_tail].prev, rob[rob_tail].dest_arch, rob[rob_tail].lsq_index);
    printf("\n---------------------------------------\n%s\n-------------------------------------\n", "IQ:");
    printf("F_bit | FU_type | Opcode | Literal | src1_valid | src1_tag | src1_val | src2_valid | src2_tag | src2_val | lsq/pr | dest | DC| CC\n");
    for (int i = 0; i < IQ_SIZE; i++)
    {
        if(NULL != issue_queue[i].fu_type){
        printf("%d | %s | %d | %d | %d | %d | %d | %d | %d | %d | %d | %d | %d|%d\n", issue_queue[i].free, issue_queue[i].fu_type, issue_queue[i].operation, issue_queue[i].literal, issue_queue[i].src1_valid_bit,
               issue_queue[i].src1_tag, issue_queue[i].src1_value, issue_queue[i].src2_valid_bit, issue_queue[i].src2_tag, issue_queue[i].src2_value, issue_queue[i].dest_type, issue_queue[i].dest, issue_queue[i].dispatch_time,issue_queue[i].cc);
        }
    }
    printf("\n---------------------------------------\n%s\n-------------------------------------\n", "Forwarding bus:");
    printf("Valid | tag |data \n");
    for (int i = 0; i < 100; i++)
    {
    if(forwarding_bus[i].valid)
    printf("%d | %d | %d\n", forwarding_bus[i].valid , forwarding_bus[i].tag , forwarding_bus[i].data); // need to check how to print only for latest instriction
    }
    printf("\n---------------------------------------\n%s\n-------------------------------------\n", "Rename Table:");
    printf("AR\tPR\t\n");
    printf("--\t--\t\n");
    for (int i = 0; i < Rename_Table_SIZE; i++)
    {
        printf("R%d\tP%d\n", i, rename_table[i]);
    }
    printf("\n---------------------------------------\n%s\n-------------------------------------\n", "Physical_Registers_Free_List:");
    for (int i = 0; i < Free_List_SIZE; i++)
    {
        printf("%d, ", reg_free_list[i]);
    }
    printf("\n---------------------------------------\n%s\n-------------------------------------\n", "CC_Free_List:");
    for (int i = 0; i < CC_PSize; i++)
    {
        printf("%d, ", cc_free_list[i]);
    }
    printf("\n---------------------------------------\n%s\n-------------------------------------\n", "PRF:");
    printf("P | Valid | Data\n");
    for (int i = 0; i < Free_List_SIZE; i++)
    {
        if (prf_file[i].pr.valid)
        {
            printf("%d| %d | %d\n", i, prf_file[i].pr.valid, prf_file[i].pr.value);
        }
    }
    printf("\n---------------------------------------\n%s\n-------------------------------------\n", "CC_PRF:");
    printf("C| Valid | Data\n");
    for (int i = 0; i < CC_PSize; i++)
    {
        if (prf_file[i].cc.valid)
        {
            printf("%d | %d | %d\n", i, prf_file[i].cc.valid, prf_file[i].cc.value);
        }
    }
    printf("\n---------------------------------------\n%s\n-------------------------------------\n", "ARF:");
    printf("ARC_REG \n");
    for (int i = 0; i < REG_FILE_SIZE / 2; ++i)
    {
        printf("R%-3d[%-3d] ", i, arf.r[i]);
    }
    printf("\n");
    for (int i = (REG_FILE_SIZE / 2); i < REG_FILE_SIZE; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }
    printf("\n");
    printf("CC | Commited Instruction Address\n");
    printf(" %d | %d", arf.cc , arf.commited_instr_address);
    printf("\n----------\n%s\n----------\n", "Memory:");
    for (int i = 0; i < DATA_MEMORY_SIZE; i++)
    {
        if (cpu->data_memory[i] != 0)
        {
            printf("Mem[%-3d] = %d ", i, cpu->data_memory[i]);
        }
    }
    printf("\n----------\n%s\n----------\n", "FETCH_PC:");
    printf("%d", cpu->fetch.pc);
    printf("\n----------\n%s\n----------\n", "Last_Commited_PC:");
    printf("%d", arf.commited_instr_address);
    printf("\n----------\n%s\n----------\n", "Elapsed_Cycle_Counter:");
    printf("%d",dispatch_counter);
    printf("\n");

}
/*
 * Fetch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_fetch(APEX_CPU *cpu)
{
    APEX_Instruction *current_ins;

    if (cpu->fetch.has_insn && !cpu->stall)
    {
        /* This fetches new branch target instruction from next cycle */
        if (cpu->fetch_from_next_cycle == TRUE)
        {
            cpu->fetch_from_next_cycle = FALSE;

            /* Skip this cycle*/
            return;
        }

        /* Store current PC in fetch latch */
        cpu->fetch.pc = cpu->pc;

        /* Index into code memory using this pc and copy all instruction fields
         * into fetch latch  */
        current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
        strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
        cpu->fetch.opcode = current_ins->opcode;
        cpu->fetch.rd = current_ins->rd;
        cpu->fetch.rs1 = current_ins->rs1;
        cpu->fetch.rs2 = current_ins->rs2;
        cpu->fetch.imm = current_ins->imm;
        
            int target_btb_index = is_btb_hit(cpu);
            if (cpu->fetch.btb_hit)
            {
                int prediction_output = predict_branch(cpu);
                if (prediction_output)
                {
                    cpu->pc = btb[target_btb_index].target_address;
                }
                else
                {
                    cpu->pc += 4;
                }
            }
            else
            {
                cpu->pc += 4;
            }

            /* Copy data from fetch latch to decode latch*/

            cpu->decode1 = cpu->fetch;
        

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Fetch", &cpu->fetch);
        }
        /* Stop fetching new instructions if HALT is fetched */
        if (cpu->fetch.opcode == OPCODE_HALT)
        {
            cpu->fetch.has_insn = FALSE;
        }
    }
}

/*
 * Decode Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_decode1(APEX_CPU *cpu)
{
    if (cpu->decode1.has_insn)
    {
        /* Read operands from register file based on the instruction type */
        switch (cpu->decode1.opcode)
        {
        case OPCODE_BZ:
        case OPCODE_BNZ:
        case OPCODE_BP:
        case OPCODE_BNP:
        {
            if (!cpu->decode1.btb_hit)
            {
                create_btb_entry(cpu);
            }
            break;
        }
        }
        cpu->decode2 = cpu->decode1;

        // cpu->execute = cpu->decode;
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Decode1/RF", &cpu->decode1);
        }
    }
}
static void
APEX_decode2(APEX_CPU *cpu)
{
    if (cpu->decode2.has_insn)
    {
        register_renaming(cpu);
        /* Read operands from register file based on the instruction type */
        switch (cpu->decode2.opcode)
        {
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        case OPCODE_MUL:
        case OPCODE_SUB:
        case OPCODE_ADD:
        case OPCODE_CMP:
        {
            if (prf_file[cpu->decode2.rs1].pr.valid)
            {
                cpu->decode2.src1_valid = 1;
                cpu->decode2.rs1_value = prf_file[cpu->decode2.rs1].pr.value;
            }
            if (prf_file[cpu->decode2.rs2].pr.valid)
            {
                cpu->decode2.src2_valid = 1;
                cpu->decode2.rs2_value = prf_file[cpu->decode2.rs2].pr.value;
            }
            break;
        }

        case OPCODE_JUMP:
        case OPCODE_JALR:
        {

            break;
        }
        case OPCODE_HALT:
        case OPCODE_NOP:
        case OPCODE_BN:
        case OPCODE_BNN:
        {
            /* MOVC doesn't have register operands */
            break;
        }
        case OPCODE_CML:
        case OPCODE_SUBL:
        case OPCODE_ADDL:
        {
            if (prf_file[cpu->decode2.rs1].pr.valid)
            {
                cpu->decode2.src1_valid = 1;
                cpu->decode2.rs1_value = prf_file[cpu->decode2.rs1].pr.value;
            }
            break;
        }
        case OPCODE_LOADP:
        case OPCODE_LOAD:
        {
            if (prf_file[cpu->decode2.rs1].pr.valid)
            {
                cpu->decode2.src1_valid = 1;
                cpu->decode2.rs1_value = prf_file[cpu->decode2.rs1].pr.value;
            }
            break;
        }
        case OPCODE_STORE:
        case OPCODE_STOREP:
        {

            break;
        }
        case OPCODE_BZ:
        case OPCODE_BNZ:
        case OPCODE_BP:
        case OPCODE_BNP:
        {
            if (!cpu->decode2.btb_hit)
            {
                cpu->stall = TRUE;
            }
            break;
        }
        }
        cpu->iq = cpu->decode2;

        // cpu->execute = cpu->decode;
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Decode2/RF", &cpu->decode2);
        }
    }
}
void rob_commit(APEX_CPU *cpu)
{
    if (rob[rob_head].entry_bit)
    {
        if (rob[rob_head].instr_type == "HALT")
        {
            stop_simulator = TRUE;
        }
        else if (rob[rob_head].instr_type == "NOP")
        {
            arf.commited_instr_address = rob[rob_head].pc_value;
            rob[rob_head].entry_bit = 0;
            rob_head = (rob_head + 1) % ROB_SIZE;
            lsq[lsq_head].entry_bit = 0;
            lsq_head = (lsq_head + 1) % LSQ_SIZE;
        }
        else if (rob[rob_head].instr_type == "STOREP")
        {
            if (rob[rob_head].lsq_index == lsq_head)
            {
                if (lsq[rob[rob_head].lsq_index].mem_addr_valid_bit && lsq[rob[rob_head].lsq_index].src_data_valid_bit)
                {
                    // also update memory using mau
                    cpu->memory.has_insn = TRUE;
                    cpu->memory.rs1_value = lsq[rob[rob_head].lsq_index].src_value;
                    cpu->memory.memory_address = lsq[rob[rob_head].lsq_index].mem_addr;
                    cpu->memory.opcode = OPCODE_STOREP;
                }
            }
        }
        else if (rob[rob_head].instr_type == "STORE")
        {
            if (rob[rob_head].lsq_index == lsq_head)
            {
                if (lsq[rob[rob_head].lsq_index].mem_addr_valid_bit && lsq[rob[rob_head].lsq_index].src_data_valid_bit)
                {
                    // also update memory using mau
                    cpu->memory.has_insn = TRUE;
                    cpu->memory.rs1_value = lsq[rob[rob_head].lsq_index].src_value;
                    cpu->memory.memory_address = lsq[rob[rob_head].lsq_index].mem_addr;
                    cpu->memory.opcode = OPCODE_STORE;
                }
            }
        }
        else if (rob[rob_head].instr_type == "LOADP")
        {
            if (rob[rob_head].lsq_index == lsq_head)
            {
                if (!lsq[lsq_head].mem_addr_valid_bit)
                {
                    cpu->memory.rd = lsq[lsq_head].dest;
                    cpu->memory.has_insn = TRUE;
                }
                else if (lsq[rob[rob_head].lsq_index].mem_addr_valid_bit && lsq[rob[rob_head].lsq_index].src_data_valid_bit)
                {
                    if (prf_file[rob[rob_head].dest_physical].pr.valid && prf_file[rob[rob_head].rs1_physical_for_loadp].pr.valid)
                    {
                        arf.r[rob[rob_head].dest_arch] = prf_file[rob[rob_head].dest_physical].pr.value;
                        reg_free_list[rename_tail + 1] = rob[rob_head].prev;
                        rename_tail += 1;
                        arf.r[rob[rob_head].rs1_arch_for_loadp] = prf_file[rob[rob_head].rs1_physical_for_loadp].pr.value;
                        reg_free_list[rename_tail + 1] = rob[rob_head].rs1_prev;
                        rename_tail += 1;
                        // reg_free_list[rob[rob_head].prev] = 0; --> this needs to be done but need to add to end of free list TODO
                        arf.commited_instr_address = rob[rob_head].pc_value;
                        rob[rob_head].entry_bit = 0;
                        rob_head = (rob_head + 1) % ROB_SIZE;
                        lsq[lsq_head].entry_bit = 0;
                        lsq_head = (lsq_head + 1) % LSQ_SIZE;
                    }
                }
            }
        }
        else if (rob[rob_head].instr_type == "LOAD")
        {
            if (rob[rob_head].lsq_index == lsq_head)
            {
                if (!lsq[lsq_head].mem_addr_valid_bit)
                {
                    cpu->memory.rd = lsq[lsq_head].dest;
                    cpu->memory.has_insn = TRUE;
                }
                else if (lsq[rob[rob_head].lsq_index].mem_addr_valid_bit && lsq[rob[rob_head].lsq_index].src_data_valid_bit)
                {
                    if (prf_file[rob[rob_head].dest_physical].pr.valid)
                    {
                        arf.r[rob[rob_head].dest_arch] = prf_file[rob[rob_head].dest_physical].pr.value;
                        reg_free_list[rename_tail + 1] = rob[rob_head].prev;
                        rename_tail += 1;
                        // reg_free_list[rob[rob_head].prev] = 0; --> this needs to be done but need to add to end of free list TODO
                        arf.commited_instr_address = rob[rob_head].pc_value;
                        rob[rob_head].entry_bit = 0;
                        rob_head = (rob_head + 1) % ROB_SIZE;
                        lsq[lsq_head].entry_bit = 0;
                        lsq_head = (lsq_head + 1) % LSQ_SIZE;
                    }
                }
            }
        }
        // R2R
        else if (prf_file[rob[rob_head].dest_physical].pr.valid)
        {
            arf.r[rob[rob_head].dest_arch] = prf_file[rob[rob_head].dest_physical].pr.value;
            reg_free_list[rename_tail + 1] = rob[rob_head].prev;
            rename_tail += 1;
            if(rob[rob_head].cc != -1)
            {
            arf.cc = prf_file[rob[rob_head].cc].cc.value;
            cc_free_list[cc_rename_tail+ 1] = rob[rob_head].cc;
            cc_rename_tail += 1;
            }
            // reg_free_list[rob[rob_head].prev] = 0; --> this needs to be done but need to add to end of free list TODO
            arf.commited_instr_address = rob[rob_head].pc_value;
            rob[rob_head].entry_bit = 0;
            rob_head = (rob_head + 1) % ROB_SIZE;
        }
    }
}
static void
APEX_iq(APEX_CPU *cpu)
{
    if (cpu->iq.has_insn)
    {
        switch (cpu->iq.opcode)
        {
        case OPCODE_MOVC:
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_HALT:
        case OPCODE_NOP:
        case OPCODE_XOR:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_CMP:
        case OPCODE_ADDL:
        case OPCODE_SUBL:
        case OPCODE_CML:
        {
            create_iq_entry(cpu, "INTFU", free_physical_reg_index);
            create_rob_entry(cpu);
            wakeup_iq(cpu);
            break;
        }
        case OPCODE_MUL:
        {
            create_iq_entry(cpu, "MULFU", free_physical_reg_index);
            create_rob_entry(cpu);
            wakeup_iq(cpu);
            break;
        }
        case OPCODE_STORE:
        {
            create_rob_entry(cpu);
            create_lsq_entry(cpu, "STORE");
            create_iq_entry(cpu, "AFU", free_physical_reg_index);
            wakeup_iq(cpu);
            break;
        }
        case OPCODE_STOREP:
        {
            create_rob_entry(cpu);
            create_lsq_entry(cpu, "STOREP");
            create_iq_entry(cpu, "AFU", free_physical_reg_index);
            wakeup_iq(cpu);
            break;
        }
        case OPCODE_LOAD:
        {
            create_rob_entry(cpu);
            create_lsq_entry(cpu, "LOAD");
            create_iq_entry(cpu, "AFU", free_physical_reg_index);
            wakeup_iq(cpu);
            break;
        }
        case OPCODE_LOADP:
        {
            create_rob_entry(cpu);
            create_lsq_entry(cpu, "LOADP");
            create_iq_entry(cpu, "AFU", free_physical_reg_index);
            wakeup_iq(cpu);
            break;
        }
        case OPCODE_BZ:
        case OPCODE_BNZ:
        case OPCODE_BP:
        case OPCODE_BNP:
        {
            create_iq_entry(cpu, "AFU", free_physical_reg_index);
            create_bq_entry(cpu);
            wakeup_iq(cpu);
            break;
        }
        }
        if (!cpu->intFU.busy && ready_for_intFU_issue != -1)
        {
            cpu->intFU.has_insn = TRUE;
            cpu->intFU.pc = cpu->iq.pc;
            cpu->intFU.rs1 = issue_queue[ready_for_intFU_issue].src1_tag;
            cpu->intFU.rs2 = issue_queue[ready_for_intFU_issue].src2_tag;
            cpu->intFU.opcode = issue_queue[ready_for_intFU_issue].operation;
            cpu->intFU.rd = issue_queue[ready_for_intFU_issue].dest;
            cpu->intFU.imm = issue_queue[ready_for_intFU_issue].literal;
            if (forwarding_bus[issue_queue[ready_for_intFU_issue].src1_tag].valid)
            {
                issue_queue[ready_for_intFU_issue].src1_value = forwarding_bus[issue_queue[ready_for_intFU_issue].src1_tag].data;
            }
            if (forwarding_bus[issue_queue[ready_for_intFU_issue].src2_tag].valid)
            {
                issue_queue[ready_for_intFU_issue].src2_value = forwarding_bus[issue_queue[ready_for_intFU_issue].src2_tag].data;
            }
            cpu->intFU.rs1_value = issue_queue[ready_for_intFU_issue].src1_value;
            cpu->intFU.rs2_value = issue_queue[ready_for_intFU_issue].src2_value;
            issue_queue[ready_for_intFU_issue].free = 0;
            cpu->intFU.busy = TRUE;
            cpu->intFU.cc = issue_queue[ready_for_intFU_issue].cc;
        }
        if (!cpu->mulFU.busy && ready_for_mulFU_issue != -1)
        {
            cpu->mulFU.has_insn = TRUE;
            cpu->mulFU.pc = cpu->iq.pc;
            cpu->mulFU.rs1 = issue_queue[ready_for_mulFU_issue].src1_tag;
            cpu->mulFU.rs2 = issue_queue[ready_for_mulFU_issue].src2_tag;
            cpu->mulFU.opcode = issue_queue[ready_for_mulFU_issue].operation;
            cpu->mulFU.rd = issue_queue[ready_for_mulFU_issue].dest;
            cpu->mulFU.imm = issue_queue[ready_for_mulFU_issue].literal;
            if (forwarding_bus[issue_queue[ready_for_mulFU_issue].src1_tag].valid)
            {
                // printf("Taking src1 value from bus: %d\n",forwarding_bus[issue_queue[ready_for_intFU_issue].src1_tag].data);
                issue_queue[ready_for_mulFU_issue].src1_value = forwarding_bus[issue_queue[ready_for_mulFU_issue].src1_tag].data;
            }
            if (forwarding_bus[issue_queue[ready_for_mulFU_issue].src2_tag].valid)
            {
                issue_queue[ready_for_mulFU_issue].src2_value = forwarding_bus[issue_queue[ready_for_mulFU_issue].src2_tag].data;
            }
            cpu->mulFU.rs1_value = issue_queue[ready_for_mulFU_issue].src1_value;
            cpu->mulFU.rs2_value = issue_queue[ready_for_mulFU_issue].src2_value;
            issue_queue[ready_for_mulFU_issue].free = 0;
            cpu->mulFU.busy = TRUE;
            cpu->mulFU.cc = issue_queue[ready_for_mulFU_issue].cc;
        }
         if(!cpu->bfu.busy && ready_for_bfu_issue != -1)
         {
            cpu->bfu.has_insn = TRUE;
            cpu->bfu.pc = cpu->afu.pc;
            cpu->bfu.cc= bq[ready_for_bfu_issue].tag;
            cpu->bfu.cc_value= bq[ready_for_bfu_issue].value;
            cpu->bfu.opcode = bq[ready_for_bfu_issue].instr_type;
            cpu->bfu.predicted_decision = cpu->afu.predicted_decision;
            cpu->bfu.btb_probe_index = cpu->afu.btb_probe_index;
            cpu->bfu.busy = TRUE;
            
         }
        if (!cpu->afu.busy && ready_for_afu_issue != -1)
        {
            cpu->afu.has_insn = TRUE;
            cpu->afu.pc = cpu->iq.pc;
            if (issue_queue[ready_for_afu_issue].operation == OPCODE_STOREP || issue_queue[ready_for_afu_issue].operation == OPCODE_STORE)
            {
                cpu->afu.rs1 = issue_queue[ready_for_afu_issue].src1_tag;
                cpu->afu.rs2 = issue_queue[ready_for_afu_issue].src2_tag;
                cpu->afu.opcode = issue_queue[ready_for_afu_issue].operation;
                cpu->afu.rd = issue_queue[ready_for_afu_issue].dest;
                cpu->afu.imm = issue_queue[ready_for_afu_issue].literal;
                if (forwarding_bus[issue_queue[ready_for_afu_issue].src1_tag].valid)
                {
                    // printf("Taking src1 value from bus: %d\n",forwarding_bus[issue_queue[ready_for_intFU_issue].src1_tag].data);
                    lsq[issue_queue[ready_for_afu_issue].dest].src_data_valid_bit = 1;
                    lsq[issue_queue[ready_for_afu_issue].dest].src_value = forwarding_bus[issue_queue[ready_for_afu_issue].src1_tag].data;
                    issue_queue[ready_for_afu_issue].src1_value = forwarding_bus[issue_queue[ready_for_afu_issue].src1_tag].data;
                }
                if (forwarding_bus[issue_queue[ready_for_afu_issue].src2_tag].valid)
                {
                    //printf("Matched rs2 value from fw bus:%d\n", forwarding_bus[issue_queue[ready_for_afu_issue].src2_tag].data);
                    issue_queue[ready_for_afu_issue].src2_value = forwarding_bus[issue_queue[ready_for_afu_issue].src2_tag].data;
                }
                //printf("rs1[%d]:%d,rs2[%d]:%d\n", issue_queue[ready_for_afu_issue].src1_tag, issue_queue[ready_for_afu_issue].src1_value, issue_queue[ready_for_afu_issue].src2_tag, issue_queue[ready_for_afu_issue].src2_value);
                cpu->afu.rs1_value = issue_queue[ready_for_afu_issue].src1_value;
                cpu->afu.rs2_value = issue_queue[ready_for_afu_issue].src2_value;
                issue_queue[ready_for_afu_issue].free = 0;
                cpu->afu.increment_reg_for_storep_loadp = cpu->iq.rd;
            }
            else if(issue_queue[ready_for_afu_issue].operation == OPCODE_LOADP || issue_queue[ready_for_afu_issue].operation == OPCODE_LOAD)
            {
                cpu->afu.rs1 = issue_queue[ready_for_afu_issue].src1_tag;
                cpu->afu.opcode = issue_queue[ready_for_afu_issue].operation;
                cpu->afu.rd = issue_queue[ready_for_afu_issue].dest;
                cpu->afu.imm = issue_queue[ready_for_afu_issue].literal;
                if (forwarding_bus[issue_queue[ready_for_afu_issue].src1_tag].valid)
                {
                    // printf("Taking src1 value from bus: %d\n",forwarding_bus[issue_queue[ready_for_intFU_issue].src1_tag].data);
                    lsq[issue_queue[ready_for_afu_issue].dest].src_data_valid_bit = 1;
                    lsq[issue_queue[ready_for_afu_issue].dest].src_value = forwarding_bus[issue_queue[ready_for_afu_issue].src1_tag].data;
                    issue_queue[ready_for_afu_issue].src1_value = forwarding_bus[issue_queue[ready_for_afu_issue].src1_tag].data;
                }
                //printf("rs1[%d]:%d", issue_queue[ready_for_afu_issue].src1_tag, issue_queue[ready_for_afu_issue].src1_value);
                cpu->afu.rs1_value = issue_queue[ready_for_afu_issue].src1_value;
                issue_queue[ready_for_afu_issue].free = 0;
                if(issue_queue[ready_for_afu_issue].operation == OPCODE_LOADP)
                    cpu->afu.increment_reg_for_storep_loadp = cpu->iq.increment_reg_for_storep_loadp;
            }
            else if(issue_queue[ready_for_afu_issue].operation == OPCODE_BZ || issue_queue[ready_for_afu_issue].operation == OPCODE_BNZ || issue_queue[ready_for_afu_issue].operation == OPCODE_BP || issue_queue[ready_for_afu_issue].operation == OPCODE_BNP)
            {
                cpu->afu.opcode = issue_queue[ready_for_afu_issue].operation;
                cpu->afu.imm = issue_queue[ready_for_afu_issue].literal;
                cpu->afu.rd = issue_queue[ready_for_afu_issue].dest;
                issue_queue[ready_for_afu_issue].free = 0;
                cpu->afu.pc = cpu->iq.pc;
                cpu->afu.predicted_decision = cpu->iq.predicted_decision;
                cpu->afu.btb_probe_index = cpu->iq.btb_probe_index;
            }
            cpu->afu.busy = TRUE;
        }

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("IQ", &cpu->iq);
        }
    }
}
void create_bq_entry(APEX_CPU *cpu)
{
    switch(cpu->iq.opcode)
    {
        case OPCODE_BZ:
        case OPCODE_BNZ:
        case OPCODE_BP:
        case OPCODE_BNP:
        {
            for (int i = 0; i < BQ_SIZE; i++)
            {
                if (!bq[i].valid)
                {
                    bq[i].valid = 1;
                    bq[i].instr_type = cpu->iq.opcode;
                    if(cpu->iq.btb_hit)
                    {
                        
                    }
                    else
                    {
                    if (cpu->decode1.opcode == OPCODE_BNZ || cpu->decode1.opcode == OPCODE_BP)
                    {
                        bq[i].prev_outcome[0] = 1;
                        bq[i].prev_outcome[1] = 1;
                    }
                    else // BZ and BNP case
                    {
                        bq[i].prev_outcome[0] = 0;
                        bq[i].prev_outcome[1] = 0;
                    }
                    bq[i].tag = rename_table[Rename_Table_SIZE -1];
                    if(forwarding_bus[bq[i].tag].valid && forwarding_bus[bq[i].tag].data_broadcasted)
                    {
                        bq[i].value = forwarding_bus[bq[i].tag].data;
                    }
                    bq[i].target_address = -1;
                    }
                    bq[i].elapsed_clock = dispatch_counter;
                    //cpu->decode1.btb_probe_index = i;
                    break;
                }
            }
            break;
        }
    }
}
void create_lsq_entry(APEX_CPU *cpu, char *lsq_type)
{
    lsq[lsq_tail].entry_bit = 1;
    if (lsq_type == "STOREP" || lsq_type == "STORE")
    {
        lsq[lsq_tail].load_store_bit = 0;
        lsq[lsq_tail].mem_addr_valid_bit = 0;
        if (prf_file[cpu->iq.rs1].pr.valid)
        {
            lsq[lsq_tail].src_data_valid_bit = 1;
            lsq[lsq_tail].src_value = prf_file[cpu->iq.rs1].pr.value;
        }
        else
        {
            lsq[lsq_tail].src_data_valid_bit = 0;
        }
        lsq[lsq_tail].src_tag = cpu->iq.rs1;
        lsq_tail = (lsq_tail + 1) % LSQ_SIZE;
    }
    else if (lsq_type == "LOADP" || lsq_type == "LOAD")
    {
        lsq[lsq_tail].load_store_bit = 1;
        lsq[lsq_tail].mem_addr_valid_bit = 0;
        lsq[lsq_tail].dest = cpu->iq.rd;
        lsq[lsq_tail].src_data_valid_bit = 1;
        lsq[lsq_tail].rob_index = rob_tail - 1;
        lsq_tail = (lsq_tail + 1) % LSQ_SIZE;
    }
}
void 
register_renaming(APEX_CPU *cpu)
{
    switch (cpu->decode2.opcode)
    {
    case OPCODE_ADD:
    case OPCODE_SUB:
    case OPCODE_MUL:
    case OPCODE_AND:
    case OPCODE_OR:
    case OPCODE_XOR:
    {
        free_physical_reg_index = get_free_pr_index();
        free_cc_physical_reg_index = get_free_cc_index();
        cpu->decode2.rs1 = rename_table[cpu->decode2.rs1];
        cpu->decode2.rs2 = rename_table[cpu->decode2.rs2];
        update_rename_table_entry(cpu, free_physical_reg_index);
        cpu->decode2.rd = free_physical_reg_index;
        //printf("rename_table size :%d",rename_table[Rename_Table_SIZE]);
        prev_cc = rename_table[16];
        rename_table[16] = free_cc_physical_reg_index;
        cpu->decode2.cc = free_cc_physical_reg_index;
        // printf("Arch - Sources are: %d %d\n", cpu->decode2.rs1, cpu->decode2.rs2);
        // printf("Rename table entries are :%d, %d\n", rename_table[cpu->decode2.rs1],rename_table[cpu->decode2.rs2]);
        // printf("Renamed instruction is : %s,P%d,P%d,P%d\n", cpu->decode2.opcode_str,cpu->decode2.rd, cpu->decode2.rs1,cpu->decode2.rs2);
        break;
    }
    case OPCODE_ADDL:
    case OPCODE_SUBL:
    {
        free_physical_reg_index = get_free_pr_index();
        //free_cc_physical_reg_index = get_free_cc_index();
        cpu->decode2.rs1 = rename_table[cpu->decode2.rs1];
        update_rename_table_entry(cpu, free_physical_reg_index);
        cpu->decode2.rd = free_physical_reg_index;
        prev_cc = rename_table[16];
        rename_table[Free_List_SIZE-1] = free_cc_physical_reg_index;
        cpu->decode2.cc = free_cc_physical_reg_index;
        // printf("Arch - Sources are: %d %d\n", cpu->decode2.rs1, cpu->decode2.rs2);
        // printf("Rename table entries are :%d, %d\n", rename_table[cpu->decode2.rs1],rename_table[cpu->decode2.rs2]);
        // printf("Renamed instruction is : %s,P%d,P%d,P%d\n", cpu->decode2.opcode_str,cpu->decode2.rd, cpu->decode2.rs1,cpu->decode2.rs2);
        break;
    }
    case OPCODE_CML:
    {
        cpu->decode2.rs1 = rename_table[cpu->decode2.rs1];
        free_cc_physical_reg_index = get_free_cc_index();
        prev_cc = rename_table[16];
        rename_table[Free_List_SIZE-1] = free_cc_physical_reg_index;
        cpu->decode2.cc = free_cc_physical_reg_index;
        break;
    }
    case OPCODE_CMP:
    {
        cpu->decode2.rs1 = rename_table[cpu->decode2.rs1];
        cpu->decode2.rs2 = rename_table[cpu->decode2.rs2];
        free_cc_physical_reg_index = get_free_cc_index();
        prev_cc = rename_table[16];
        rename_table[Free_List_SIZE-1] = free_cc_physical_reg_index;
        cpu->decode2.cc = free_cc_physical_reg_index;
        break;
    }
    case OPCODE_MOVC:
    {
        free_physical_reg_index = get_free_pr_index();
        update_rename_table_entry(cpu, free_physical_reg_index);
        cpu->decode2.rd = free_physical_reg_index;
        cpu->decode2.cc = -1;
        break;
    }
    case OPCODE_STOREP:
    {
        free_physical_reg_index = get_free_pr_index();
        cpu->decode2.increment_reg_for_storep_loadp = cpu->decode2.rs2;
        cpu->decode2.rs1 = rename_table[cpu->decode2.rs1];
        cpu->decode2.rs2 = rename_table[cpu->decode2.rs2];
        update_rename_table_entry(cpu, free_physical_reg_index);
        cpu->decode2.rd = free_physical_reg_index;
        cpu->decode2.cc = -1;
        break;
    }
    case OPCODE_STORE:
    {
        cpu->decode2.rs1 = rename_table[cpu->decode2.rs1];
        cpu->decode2.rs2 = rename_table[cpu->decode2.rs2];
        cpu->decode2.cc = -1;
        break;
    }
    case OPCODE_LOADP:
    {
        free_physical_reg_index = get_free_pr_index();
        int free_physical_reg_index_rs1 = get_free_pr_index();
        cpu->decode2.increment_reg_for_storep_loadp = cpu->decode2.rs1;
        cpu->decode2.rs1 = rename_table[cpu->decode2.rs1];
        update_rename_table_entry(cpu, free_physical_reg_index_rs1);
        cpu->decode2.rd = free_physical_reg_index;
        cpu->decode2.cc = -1;
        break;
    }
    case OPCODE_LOAD:
    {
        free_physical_reg_index = get_free_pr_index();
        cpu->decode2.rs1 = rename_table[cpu->decode2.rs1];
        update_rename_table_entry(cpu, free_physical_reg_index);
        cpu->decode2.rd = free_physical_reg_index;
        cpu->decode2.cc = -1;
        break;
    }
    }
}
void wakeup_iq(APEX_CPU *cpu)
{
    // ready_for_intFU_issue[] = malloc(sizeof(int));
    // int ready_for_mulU_issue[] = malloc(sizeof(int));
    // int ready_for_aFU_issue[] = malloc(sizeof(int));
    ready_for_intFU_issue = -1;
    ready_for_mulFU_issue = -1;
    ready_for_afu_issue = -1;
    ready_for_bfu_issue = -1;
    int intFU_min = INT16_MAX;
    int mulFU_min = INT16_MAX;
    int aFU_min = INT16_MAX;
    int bfu_min = INT16_MAX;
    for (int i = 0; i < 100; i++)
    {   
        if (forwarding_bus[i].valid)
        {
            forwarding_bus[i].tag_broadcasted = 1;
        }
        if (cc_forwarding_bus[i].valid)
        {
            cc_forwarding_bus[i].tag_broadcasted = 1;
        }
    }
    for (int i = 0; i < IQ_SIZE; i++)
    {
        if (issue_queue[i].free)
        {
            if (forwarding_bus[issue_queue[i].src1_tag].valid)
            {
                issue_queue[i].src1_valid_bit = 1;
            }
            if (forwarding_bus[issue_queue[i].src2_tag].valid)
            {
                issue_queue[i].src2_valid_bit = 1;
            }
        }
    }
    for (int i = 0; i < IQ_SIZE; i++)
    {
        // printf("Issue_queue: IQ_free[%d]| IQ[%s] ",issue_queue[0].free,issue_queue[0].fu_type);
        if (!cpu->intFU.busy)
        {
            // printf("INTFU is free..\n");
            if (issue_queue[i].free && issue_queue[i].fu_type == "INTFU")
            {
                // printf("free[%d], FUType: %s\n", issue_queue[i].free, issue_queue[i].fu_type);
                // printf("src1_valid[%d], src2_valid[%d],dispatc_count[%d]", issue_queue[i].src1_valid_bit, issue_queue[1].src2_valid_bit,issue_queue[i].dispatch_time );
                if ((issue_queue[i].src1_valid_bit && issue_queue[i].src2_valid_bit) && issue_queue[i].dispatch_time < intFU_min)
                {
                    // printf("ready for issue: %d\n",issue_queue[i].operation);
                    intFU_min = issue_queue[i].dispatch_time;
                    ready_for_intFU_issue = i;
                }
            }
        }
        if (!cpu->mulFU.busy)
        {
            if (issue_queue[i].free && issue_queue[i].fu_type == "MULFU")
            {
                if (issue_queue[i].src1_valid_bit && issue_queue[i].src2_valid_bit && issue_queue[i].dispatch_time < mulFU_min)
                {
                    mulFU_min = issue_queue[i].dispatch_time;
                    ready_for_mulFU_issue = i;
                }
            }
        }
        if (!cpu->afu.busy)
        {
            if (issue_queue[i].free && issue_queue[i].fu_type == "AFU")
            {
                if (issue_queue[i].src1_valid_bit && issue_queue[i].src2_valid_bit && issue_queue[i].dispatch_time < aFU_min)
                {
                    aFU_min = issue_queue[i].dispatch_time;
                    ready_for_afu_issue = i;
                }
            }
        }
         if (!cpu->bfu.busy)
        {
            if (bq[i].valid && bq[i].target_address != -1)
            {
                if (bq[i].elapsed_clock < aFU_min)
                {
                    aFU_min = bq[i].elapsed_clock;
                    ready_for_bfu_issue = i;
                }
            }
        }
    }
}
void update_rename_table_entry(APEX_CPU *cpu, int physical_reg)
{
    // printf("Arch reg:%d\n",cpu->decode2.rd);
    if (cpu->decode2.opcode == OPCODE_STOREP)
    {
        cpu->decode2.arch_reg = cpu->decode2.increment_reg_for_storep_loadp;
        prev = rename_table[cpu->decode2.increment_reg_for_storep_loadp];
        rename_table[cpu->decode2.increment_reg_for_storep_loadp] = physical_reg;
    }
    else if (cpu->decode2.opcode == OPCODE_LOADP)
    {
        // for rd
        cpu->decode2.arch_reg = cpu->decode2.rd;
        prev = rename_table[cpu->decode2.rd];
        rename_table[cpu->decode2.rd] = free_physical_reg_index;
        // for rs1
        cpu->decode2.arch_reg_for_loadp = cpu->decode2.increment_reg_for_storep_loadp;
        cpu->decode2.prev_rs1_for_loadp = rename_table[cpu->decode2.increment_reg_for_storep_loadp];
        rename_table[cpu->decode2.increment_reg_for_storep_loadp] = physical_reg;
        cpu->decode2.increment_reg_for_storep_loadp = physical_reg;
    }
    else if (cpu->decode2.opcode == OPCODE_LOAD)
    {
        cpu->decode2.arch_reg = cpu->decode2.rd;
        prev = rename_table[cpu->decode2.rd];
        rename_table[cpu->decode2.rd] = free_physical_reg_index;
    }
    else
    {
        cpu->decode2.arch_reg = cpu->decode2.rd;
        prev = rename_table[cpu->decode2.rd];
        rename_table[cpu->decode2.rd] = physical_reg;
    }
    // store prev for ROB entry
    //  if(cc_index != -1)
    //  {
    //      rename_table[REG_FILE_SIZE] = cc_index;
    //  }
}
int get_free_pr_index()
{
    // for(int i =0; i<Free_List_SIZE; i++)
    // {
    //     // printf("P0 validity in free list: %d\n",reg_free_list[0]);
    //     // printf("P1 validity in free list: %d\n",reg_free_list[1]);
    //     // printf("P2 validity in free list: %d\n",reg_free_list[2]);
    //     // printf("P3 validity in free list: %d\n",reg_free_list[3]);
    //     if(reg_free_list[i] == 0)
    //     {
    //         reg_free_list[i] = 1;
    //         //printf("free pr index: %d\n",i);
    //         //printf("P4 validity in free list: %d\n",reg_free_list[4]);
    //         return i;
    //     }
    // }
    int free_index = reg_free_list[0];
    
for (int i = 0; i < rename_tail - 1; i++) {
    reg_free_list[i] = reg_free_list[i + 1];
}
rename_tail-= 1;  
return free_index;
}
int get_free_cc_index()
{
    int cc_free_index = cc_free_list[0];
    for (int k = 0; k < CC_PSize - 1; k++)
    {
        cc_free_list[k] = cc_free_list[k + 1];
    }
    cc_rename_tail -= 1;
    return cc_free_index;
}
void create_iq_entry(APEX_CPU *cpu, char *fu_type, int physical_reg)
{
    dispatch_counter++;
    for (int i = 0; i < IQ_SIZE; i++)
    {
        if (!issue_queue[i].free)
        {
            issue_queue[i].free = 1;
            issue_queue[i].fu_type = fu_type;
            issue_queue[i].dest = cpu->iq.rd;
            switch (cpu->iq.opcode)
            {
            case OPCODE_MOVC:
            {
                issue_queue[i].src1_valid_bit = 1;
                issue_queue[i].src2_valid_bit = 1;
                issue_queue[i].dest_type = 1;
                issue_queue[i].operation = cpu->iq.opcode;
                issue_queue[i].literal = cpu->iq.imm;
                break;
            }
            case OPCODE_ADD:
            case OPCODE_XOR:
            case OPCODE_AND:
            case OPCODE_OR:
            case OPCODE_CMP:
            {
                issue_queue[i].src1_tag = cpu->iq.rs1;
                issue_queue[i].src2_tag = cpu->iq.rs2;
                issue_queue[i].src1_valid_bit = cpu->iq.src1_valid;
                issue_queue[i].src2_valid_bit = cpu->iq.src2_valid;
                issue_queue[i].literal = 0;
                issue_queue[i].dest_type = 1;
                if (cpu->iq.src1_valid)
                {
                    issue_queue[i].src1_value = cpu->iq.rs1_value;
                }
                else if (!cpu->iq.src1_valid)
                {
                    if (forwarding_bus[cpu->iq.rs1].valid)
                    {
                        issue_queue[i].src1_valid_bit = 1;
                        issue_queue[i].src1_value = forwarding_bus[cpu->iq.rs1].data;
                    }
                }
                if (cpu->iq.src2_valid)
                {
                    issue_queue[i].src2_value = cpu->iq.rs2_value;
                }
                else if (!cpu->iq.src2_valid)
                {
                    if (forwarding_bus[cpu->iq.rs2].valid)
                    {
                        issue_queue[i].src2_valid_bit = 1;
                        issue_queue[i].src2_value = forwarding_bus[cpu->iq.rs2].data;
                    }
                }
                issue_queue[i].operation = cpu->iq.opcode;
                issue_queue[i].cc = cpu->iq.cc;
                break;
            }
            case OPCODE_SUB:
            {
                issue_queue[i].src1_tag = cpu->iq.rs1;
                issue_queue[i].src2_tag = cpu->iq.rs2;
                issue_queue[i].src1_valid_bit = cpu->iq.src1_valid;
                issue_queue[i].src2_valid_bit = cpu->iq.src2_valid;
                issue_queue[i].dest_type = 1;
                issue_queue[i].literal = 0;
                if (cpu->iq.src1_valid)
                {
                    issue_queue[i].src1_value = cpu->iq.rs1_value;
                }
                else if (!cpu->iq.src1_valid)
                {
                    if (forwarding_bus[cpu->iq.rs1].valid)
                    {
                        issue_queue[i].src1_valid_bit = 1;
                        issue_queue[i].src1_value = forwarding_bus[cpu->iq.rs1].data;
                    }
                }
                if (cpu->iq.src2_valid)
                {
                    issue_queue[i].src2_value = cpu->iq.rs2_value;
                }
                else if (!cpu->iq.src2_valid)
                {
                    if (forwarding_bus[cpu->iq.rs2].valid)
                    {
                        issue_queue[i].src2_valid_bit = 1;
                        issue_queue[i].src2_value = forwarding_bus[cpu->iq.rs2].data;
                    }
                }
                issue_queue[i].operation = cpu->iq.opcode;
                issue_queue[i].cc = cpu->iq.cc;
                break;
            }
            case OPCODE_MUL:
            {
                issue_queue[i].src1_tag = cpu->iq.rs1;
                issue_queue[i].src2_tag = cpu->iq.rs2;
                issue_queue[i].src1_valid_bit = cpu->iq.src1_valid;
                issue_queue[i].src2_valid_bit = cpu->iq.src2_valid;
                issue_queue[i].dest_type = 1;
                issue_queue[i].literal = 0;
                if (cpu->iq.src1_valid)
                {
                    issue_queue[i].src1_value = cpu->iq.rs1_value;
                }
                else if (!cpu->iq.src1_valid)
                {
                    if (forwarding_bus[cpu->iq.rs1].valid)
                    {
                        issue_queue[i].src1_valid_bit = 1;
                        issue_queue[i].src1_value = forwarding_bus[cpu->iq.rs1].data;
                    }
                }
                if (cpu->iq.src2_valid)
                {
                    issue_queue[i].src2_value = cpu->iq.rs2_value;
                }
                else if (!cpu->iq.src2_valid)
                {
                    if (forwarding_bus[cpu->iq.rs2].valid)
                    {
                        issue_queue[i].src2_valid_bit = 1;
                        issue_queue[i].src2_value = forwarding_bus[cpu->iq.rs2].data;
                    }
                }
                issue_queue[i].operation = cpu->iq.opcode;
                issue_queue[i].cc = cpu->iq.cc;
                break;
            }
            case OPCODE_HALT:
            case OPCODE_NOP:
            {
                issue_queue[i].src1_tag = 0;
                issue_queue[i].src2_tag = 0;
                issue_queue[i].src1_valid_bit = 1;
                issue_queue[i].src2_valid_bit = 1;
                issue_queue[i].literal = 0;
                issue_queue[i].dest_type = 1;
                issue_queue[i].dest = 0;
                issue_queue[i].operation = cpu->iq.opcode;
                break;
            }
            case OPCODE_STOREP:
            case OPCODE_STORE:
            {
                issue_queue[i].src1_tag = cpu->iq.rs1;
                issue_queue[i].src2_tag = cpu->iq.rs2;
                issue_queue[i].src1_valid_bit = cpu->iq.src1_valid;
                issue_queue[i].src2_valid_bit = cpu->iq.src2_valid;
                issue_queue[i].literal = cpu->iq.imm;
                issue_queue[i].dest_type = 0;
                issue_queue[i].dest = lsq_tail - 1;
                if (cpu->iq.src1_valid)
                {
                    issue_queue[i].src1_value = cpu->iq.rs1_value;
                }
                else if (!cpu->iq.src1_valid)
                {
                    if (forwarding_bus[cpu->iq.rs1].valid)
                    {
                        issue_queue[i].src1_valid_bit = 1;
                        issue_queue[i].src1_value = forwarding_bus[cpu->iq.rs1].data;
                        lsq[issue_queue[i].dest].src_data_valid_bit = 1;
                        lsq[issue_queue[i].dest].src_value = forwarding_bus[cpu->iq.rs1].data;
                    }
                }
                if (cpu->iq.src2_valid)
                {
                    issue_queue[i].src2_value = cpu->iq.rs2_value;
                }
                else if (!cpu->iq.src2_valid)
                {
                    if (forwarding_bus[cpu->iq.rs2].valid)
                    {
                        issue_queue[i].src2_valid_bit = 1;
                        issue_queue[i].src2_value = forwarding_bus[cpu->iq.rs2].data;
                    }
                }
                issue_queue[i].operation = cpu->iq.opcode;
                break;
            }
            case OPCODE_LOADP:
            case OPCODE_LOAD:
            {
                issue_queue[i].src1_tag = cpu->iq.rs1;
                issue_queue[i].src2_tag = 0;
                issue_queue[i].src1_valid_bit = cpu->iq.src1_valid;
                issue_queue[i].src2_valid_bit = 1;
                issue_queue[i].literal = cpu->iq.imm;
                issue_queue[i].dest_type = 0;
                issue_queue[i].dest = lsq_tail - 1;
                if (cpu->iq.src1_valid)
                {
                    issue_queue[i].src1_value = cpu->iq.rs1_value;
                }
                else if (!cpu->iq.src1_valid)
                {
                    if (forwarding_bus[cpu->iq.rs1].valid)
                    {
                        issue_queue[i].src1_valid_bit = 1;
                        issue_queue[i].src1_value = forwarding_bus[cpu->iq.rs1].data;
                        lsq[issue_queue[i].dest].src_data_valid_bit = 1;
                        lsq[issue_queue[i].dest].src_value = forwarding_bus[cpu->iq.rs1].data;
                    }
                }
                issue_queue[i].operation = cpu->iq.opcode;
                break;
            }
            case OPCODE_ADDL:
            case OPCODE_SUBL:
            case OPCODE_CML:
            {
                issue_queue[i].src1_tag = cpu->iq.rs1;
                issue_queue[i].src2_tag = 0;
                issue_queue[i].src1_valid_bit = cpu->iq.src1_valid;
                issue_queue[i].src2_valid_bit = 1;
                issue_queue[i].literal = cpu->iq.imm;
                issue_queue[i].dest_type = 1;
                if (cpu->iq.src1_valid)
                {
                    issue_queue[i].src1_value = cpu->iq.rs1_value;
                }
                else if (!cpu->iq.src1_valid)
                {
                    if (forwarding_bus[cpu->iq.rs1].valid)
                    {
                        issue_queue[i].src1_valid_bit = 1;
                        issue_queue[i].src1_value = forwarding_bus[cpu->iq.rs1].data;
                    }
                }
                issue_queue[i].operation = cpu->iq.opcode;
                issue_queue[i].cc = cpu->iq.cc;
                break;
            }
             case OPCODE_BZ:
             case OPCODE_BNZ:
             case OPCODE_BP:
             case OPCODE_BNP:
            {
                issue_queue[i].src1_valid_bit = 1;
                issue_queue[i].src2_valid_bit = 1;
                issue_queue[i].dest_type = 1;
                issue_queue[i].operation = cpu->iq.opcode;
                issue_queue[i].literal = cpu->iq.imm;
                issue_queue[i].dest = rename_table[Rename_Table_SIZE -1];
                
                break;
            }
            }
            issue_queue[i].dispatch_time = dispatch_counter;
            break;
        }
    }
    for (int i = 0; i < 100; i++)
    {
        
        if (forwarding_bus[i].valid && forwarding_bus[i].tag_broadcasted)
        {
            forwarding_bus[i].data_broadcasted = 1;
            prf_file[forwarding_bus[i].tag].pr.valid = 1;
            prf_file[forwarding_bus[i].tag].pr.value = forwarding_bus[i].data;
            forwarding_bus[i].valid = 0;
        }
        if (cc_forwarding_bus[i].valid && cc_forwarding_bus[i].tag_broadcasted)
        {
            cc_forwarding_bus[i].data_broadcasted = 1;
            prf_file[cc_forwarding_bus[i].tag].cc.valid = 1;
            prf_file[cc_forwarding_bus[i].tag].cc.value = cc_forwarding_bus[i].data;
            for(int i =0; i<BQ_SIZE;i++)
            {
            if(bq[i].valid)
            {
                if (cc_forwarding_bus[bq[i].tag].data_broadcasted)
                {
                    // printf("Taking src1 value from bus: %d\n",forwarding_bus[issue_queue[i].src1_tag].data);
                    bq[i].value = cc_forwarding_bus[bq[i].tag].data;
                }
            }
            }
            cc_forwarding_bus[i].valid = 0;
        }
    }
    pull_value_from_bus();
}
void pull_value_from_bus()
{
    for (int i = 0; i < IQ_SIZE; i++)
    {
        if (issue_queue[i].free && issue_queue[i].src1_valid_bit)
        {
            if (forwarding_bus[issue_queue[i].src1_tag].data_broadcasted)
            {
                // printf("Taking src1 value from bus: %d\n",forwarding_bus[issue_queue[i].src1_tag].data);
                issue_queue[i].src1_value = forwarding_bus[issue_queue[i].src1_tag].data;
            }
            if (forwarding_bus[issue_queue[i].src2_tag].data_broadcasted)
            {
                issue_queue[i].src2_value = forwarding_bus[issue_queue[i].src2_tag].data;
            }
        }
    }
}
void create_rob_entry(APEX_CPU *cpu)
{
    switch (cpu->iq.opcode)
    {
    case OPCODE_ADD:
    case OPCODE_SUB:
    case OPCODE_MUL:
    case OPCODE_MOVC:
    case OPCODE_XOR:
    case OPCODE_AND:
    case OPCODE_OR:
    case OPCODE_CMP:
    case OPCODE_CML:
    case OPCODE_ADDL:
    case OPCODE_SUBL:
    {
        rob[rob_tail].entry_bit = 1;
        rob[rob_tail].instr_type = "R2R";
        rob[rob_tail].prev = prev;
        rob[rob_tail].pc_value = cpu->iq.pc;
        rob[rob_tail].dest_arch = cpu->iq.arch_reg;
        rob[rob_tail].dest_physical = cpu->iq.rd;
        rob[rob_tail].cc = cpu->iq.cc;
        rob[rob_tail].prev_cc = prev_cc;
        rob_tail = (rob_tail + 1) % ROB_SIZE;
        break;
    }
    case OPCODE_HALT:
    {
        rob[rob_tail].entry_bit = 1;
        rob[rob_tail].instr_type = "HALT";
        rob[rob_tail].prev_cc = prev_cc;
        rob_tail = (rob_tail + 1) % ROB_SIZE;
        break;
    }
    case OPCODE_NOP:
    {
        rob[rob_tail].entry_bit = 1;
        rob[rob_tail].instr_type = "NOP";
        rob[rob_tail].prev_cc = prev_cc;
        rob_tail = (rob_tail + 1) % ROB_SIZE;
        break;
    }
    case OPCODE_STOREP:
    {
        rob[rob_tail].entry_bit = 1;
        rob[rob_tail].instr_type = "STOREP";
        rob[rob_tail].prev = prev;
        rob[rob_tail].pc_value = cpu->iq.pc;
        rob[rob_tail].dest_arch = cpu->iq.arch_reg;
        rob[rob_tail].dest_physical = cpu->iq.rd;
        rob[rob_tail].lsq_index = lsq_tail;
        rob[rob_tail].prev_cc = prev_cc;
        rob_tail = (rob_tail + 1) % ROB_SIZE;
        break;
    }
    case OPCODE_STORE:
    {
        rob[rob_tail].entry_bit = 1;
        rob[rob_tail].instr_type = "STORE";
        rob[rob_tail].prev = prev;
        rob[rob_tail].pc_value = cpu->iq.pc;
        rob[rob_tail].dest_arch = 0;
        rob[rob_tail].dest_physical = 0;
        rob[rob_tail].lsq_index = lsq_tail;
        rob[rob_tail].prev_cc = prev_cc;
        rob_tail = (rob_tail + 1) % ROB_SIZE;
        break;
    }
    case OPCODE_LOADP:
    {
        rob[rob_tail].entry_bit = 1;
        rob[rob_tail].instr_type = "LOADP";
        rob[rob_tail].prev = prev;
        rob[rob_tail].rs1_prev = cpu->iq.prev_rs1_for_loadp;
        rob[rob_tail].pc_value = cpu->iq.pc;
        rob[rob_tail].dest_arch = cpu->iq.arch_reg;
        rob[rob_tail].dest_physical = cpu->iq.rd;
        rob[rob_tail].rs1_arch_for_loadp = cpu->iq.arch_reg_for_loadp;
        rob[rob_tail].rs1_physical_for_loadp = cpu->iq.increment_reg_for_storep_loadp;
        rob[rob_tail].lsq_index = lsq_tail;
        rob[rob_tail].prev_cc = prev_cc;
        rob_tail = (rob_tail + 1) % ROB_SIZE;
        break;
    }
    case OPCODE_LOAD:
    {
        rob[rob_tail].entry_bit = 1;
        rob[rob_tail].instr_type = "LOAD";
        rob[rob_tail].prev = prev;
        rob[rob_tail].pc_value = cpu->iq.pc;
        rob[rob_tail].dest_arch = cpu->iq.arch_reg;
        rob[rob_tail].dest_physical = cpu->iq.rd;
        rob[rob_tail].lsq_index = lsq_tail;
        rob[rob_tail].prev_cc = prev_cc;
        rob_tail = (rob_tail + 1) % ROB_SIZE;
        break;
    }
    }
}
static void
APEX_FU(APEX_CPU *cpu)
{
    rob_commit(cpu);
    // printf("Entering the stage....");
    if (cpu->intFU.has_insn || cpu->mulFU.has_insn)
    {
        switch (cpu->intFU.opcode)
        {
        case OPCODE_MOVC:
        {
            forwarding_bus[cpu->intFU.rd].valid = 1;
            forwarding_bus[cpu->intFU.rd].tag = cpu->intFU.rd;
            forwarding_bus[cpu->intFU.rd].data = cpu->intFU.imm;
            cpu->intFU.busy = FALSE;
            cpu->intFU.has_insn = FALSE;
            printf("Forwarding bus : %d | %d | %d\n", forwarding_bus[cpu->intFU.rd].valid, forwarding_bus[cpu->intFU.rd].tag, forwarding_bus[cpu->intFU.rd].data);
            break;
        }
        case OPCODE_ADD:
        {
            forwarding_bus[cpu->intFU.rd].valid = 1;
            forwarding_bus[cpu->intFU.rd].tag = cpu->intFU.rd;
            forwarding_bus[cpu->intFU.rd].data = cpu->intFU.rs1_value + cpu->intFU.rs2_value;
            cc_forwarding_bus[cpu->intFU.cc].valid = 1;
            cc_forwarding_bus[cpu->intFU.cc].tag = cpu->intFU.cc;
            if(forwarding_bus[cpu->intFU.rd].data > 0)
            {
                cc_forwarding_bus[cpu->intFU.cc].data = 1;
            }
            else if(forwarding_bus[cpu->intFU.rd].data == 0)
                cc_forwarding_bus[cpu->intFU.cc].data = 0;
            cpu->intFU.busy = FALSE;
            cpu->intFU.has_insn = FALSE;
            //printf("Forwarding bus : %d | %d | %d\n",forwarding_bus[cpu->intFU.rd].valid, forwarding_bus[cpu->intFU.rd].tag, forwarding_bus[cpu->intFU.rd].data);
            break;
        }
        case OPCODE_ADDL:
        {
            forwarding_bus[cpu->intFU.rd].valid = 1;
            forwarding_bus[cpu->intFU.rd].tag = cpu->intFU.rd;
            forwarding_bus[cpu->intFU.rd].data = cpu->intFU.rs1_value + cpu->intFU.imm;
            cc_forwarding_bus[cpu->intFU.cc].valid = 1;
            cc_forwarding_bus[cpu->intFU.cc].tag = cpu->intFU.cc;
            if(forwarding_bus[cpu->intFU.rd].data > 0)
            {
                cc_forwarding_bus[cpu->intFU.cc].data = 1;
            }
            else if(forwarding_bus[cpu->intFU.rd].data == 0)
                cc_forwarding_bus[cpu->intFU.cc].data = 0;
            cpu->intFU.busy = FALSE;
            cpu->intFU.has_insn = FALSE;
            // printf("Forwarding bus : %d | %d | %d\n",forwarding_bus[cpu->intFU.rd].valid, forwarding_bus[cpu->intFU.rd].tag, forwarding_bus[cpu->intFU.rd].data);
            break;
        }
        case OPCODE_SUB:
        {
            forwarding_bus[cpu->intFU.rd].valid = 1;
            forwarding_bus[cpu->intFU.rd].tag = cpu->intFU.rd;
            forwarding_bus[cpu->intFU.rd].data = cpu->intFU.rs1_value - cpu->intFU.rs2_value;
            cc_forwarding_bus[cpu->intFU.cc].valid = 1;
            cc_forwarding_bus[cpu->intFU.cc].tag = cpu->intFU.cc;
            if(forwarding_bus[cpu->intFU.rd].data > 0)
            {
                cc_forwarding_bus[cpu->intFU.cc].data = 1;
            }
            else if(forwarding_bus[cpu->intFU.rd].data == 0)
                cc_forwarding_bus[cpu->intFU.cc].data = 0;
            cpu->intFU.busy = FALSE;
            cpu->intFU.has_insn = FALSE;
            // printf("Forwarding bus : %d | %d | %d\n",forwarding_bus[cpu->intFU.rd].valid, forwarding_bus[cpu->intFU.rd].tag, forwarding_bus[cpu->intFU.rd].data);
            break;
        }
        case OPCODE_SUBL:
        {
            forwarding_bus[cpu->intFU.rd].valid = 1;
            forwarding_bus[cpu->intFU.rd].tag = cpu->intFU.rd;
            forwarding_bus[cpu->intFU.rd].data = cpu->intFU.rs1_value - cpu->intFU.imm;
            cc_forwarding_bus[cpu->intFU.cc].valid = 1;
            cc_forwarding_bus[cpu->intFU.cc].tag = cpu->intFU.cc;
            if(forwarding_bus[cpu->intFU.rd].data > 0)
            {
                cc_forwarding_bus[cpu->intFU.cc].data = 1;
            }
            else if(forwarding_bus[cpu->intFU.rd].data == 0)
                cc_forwarding_bus[cpu->intFU.cc].data = 0;
            cpu->intFU.busy = FALSE;
            cpu->intFU.has_insn = FALSE;
            // printf("Forwarding bus : %d | %d | %d\n",forwarding_bus[cpu->intFU.rd].valid, forwarding_bus[cpu->intFU.rd].tag, forwarding_bus[cpu->intFU.rd].data);
            break;
        }
        case OPCODE_AND:
        {
            forwarding_bus[cpu->intFU.rd].valid = 1;
            forwarding_bus[cpu->intFU.rd].tag = cpu->intFU.rd;
            forwarding_bus[cpu->intFU.rd].data = cpu->intFU.rs1_value & cpu->intFU.rs2_value;
            cc_forwarding_bus[cpu->intFU.cc].valid = 1;
            cc_forwarding_bus[cpu->intFU.cc].tag = cpu->intFU.cc;
            if(forwarding_bus[cpu->intFU.rd].data > 0)
            {
                cc_forwarding_bus[cpu->intFU.cc].data = 1;
            }
            else if(forwarding_bus[cpu->intFU.rd].data == 0)
                cc_forwarding_bus[cpu->intFU.cc].data = 0;
            cpu->intFU.busy = FALSE;
            cpu->intFU.has_insn = FALSE;
            printf("Forwarding bus : %d | %d | %d\n", forwarding_bus[cpu->intFU.rd].valid, forwarding_bus[cpu->intFU.rd].tag, forwarding_bus[cpu->intFU.rd].data);
            break;
        }
        case OPCODE_OR:
        {
            forwarding_bus[cpu->intFU.rd].valid = 1;
            forwarding_bus[cpu->intFU.rd].tag = cpu->intFU.rd;
            forwarding_bus[cpu->intFU.rd].data = cpu->intFU.rs1_value | cpu->intFU.rs2_value;
            cc_forwarding_bus[cpu->intFU.cc].valid = 1;
            cc_forwarding_bus[cpu->intFU.cc].tag = cpu->intFU.cc;
            if(forwarding_bus[cpu->intFU.rd].data > 0)
            {
                cc_forwarding_bus[cpu->intFU.cc].data = 1;
            }
            else if(forwarding_bus[cpu->intFU.rd].data == 0)
                cc_forwarding_bus[cpu->intFU.cc].data = 0;
            cpu->intFU.busy = FALSE;
            cpu->intFU.has_insn = FALSE;
            // printf("Forwarding bus : %d | %d | %d\n",forwarding_bus[cpu->intFU.rd].valid, forwarding_bus[cpu->intFU.rd].tag, forwarding_bus[cpu->intFU.rd].data);
            break;
        }
        case OPCODE_XOR:
        {
            forwarding_bus[cpu->intFU.rd].valid = 1;
            forwarding_bus[cpu->intFU.rd].tag = cpu->intFU.rd;
            forwarding_bus[cpu->intFU.rd].data = cpu->intFU.rs1_value ^ cpu->intFU.rs2_value;
            cc_forwarding_bus[cpu->intFU.cc].valid = 1;
            cc_forwarding_bus[cpu->intFU.cc].tag = cpu->intFU.cc;
            if(forwarding_bus[cpu->intFU.rd].data > 0)
            {
                cc_forwarding_bus[cpu->intFU.cc].data = 1;
            }
            else if(forwarding_bus[cpu->intFU.rd].data == 0)
                cc_forwarding_bus[cpu->intFU.cc].data = 0;
            cpu->intFU.busy = FALSE;
            cpu->intFU.has_insn = FALSE;
            printf("Forwarding bus for XOR : %d | %d | %d\n", forwarding_bus[cpu->intFU.rd].valid, forwarding_bus[cpu->intFU.rd].tag, forwarding_bus[cpu->intFU.rd].data);
            break;
        }
        case OPCODE_CMP:
        {
            cc_forwarding_bus[cpu->intFU.cc].valid = 1;
            cc_forwarding_bus[cpu->intFU.cc].tag = cpu->intFU.cc;
            if(cpu->intFU.rs1_value > cpu->intFU.rs2_value)
            {
                cc_forwarding_bus[cpu->intFU.cc].data = 1;
            }
            else if(cpu->intFU.rs1_value == cpu->intFU.rs2_value)
                cc_forwarding_bus[cpu->intFU.cc].data = 0;
            cpu->intFU.busy = FALSE;
            cpu->intFU.has_insn = FALSE;
            printf("Forwarding bus for XOR : %d | %d | %d\n", forwarding_bus[cpu->intFU.rd].valid, forwarding_bus[cpu->intFU.rd].tag, forwarding_bus[cpu->intFU.rd].data);
            break;
        }
        case OPCODE_CML:
        {
            cc_forwarding_bus[cpu->intFU.cc].valid = 1;
            cc_forwarding_bus[cpu->intFU.cc].tag = cpu->intFU.cc;
            if(cpu->intFU.rs1_value > cpu->intFU.imm)
            {
                cc_forwarding_bus[cpu->intFU.cc].data = 1;
            }
            else if(cpu->intFU.rs1_value == cpu->intFU.imm)
                cc_forwarding_bus[cpu->intFU.cc].data = 0;
            cpu->intFU.busy = FALSE;
            cpu->intFU.has_insn = FALSE;
            printf("Forwarding bus for CML : %d | %d | %d\n", cc_forwarding_bus[cpu->intFU.cc].valid, cc_forwarding_bus[cpu->intFU.cc].tag, cc_forwarding_bus[cpu->intFU.cc].data);
            break;
        }
        case OPCODE_HALT:
        case OPCODE_NOP:
        {
            cpu->intFU.busy = FALSE;
            break;
        }
        }
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("INT_FU", &cpu->intFU);
        }
        if (cpu->mulFU.has_insn)
        {
            mul_counter++;
            // if(mul_counter == 2)
            // {
            //    forwarding_bus[cpu->mulFU.rd].valid = 1;
            //     forwarding_bus[cpu->mulFU.rd].tag= cpu->mulFU.rd;
            //     forwarding_bus[cpu->mulFU.rd].data= cpu->mulFU.rs1_value * cpu->mulFU.rs2_value;
            //     printf("Forwarding bus mul: %d | %d | %d\n",forwarding_bus[cpu->mulFU.rd].valid, forwarding_bus[cpu->mulFU.rd].tag, forwarding_bus[cpu->mulFU.rd].data);
            // }
            if (mul_counter == 3)
            {
                forwarding_bus[cpu->mulFU.rd].valid = 1;
                forwarding_bus[cpu->mulFU.rd].tag = cpu->mulFU.rd;
                forwarding_bus[cpu->mulFU.rd].data = cpu->mulFU.rs1_value * cpu->mulFU.rs2_value;
                cc_forwarding_bus[cpu->mulFU.cc].valid = 1;
                cc_forwarding_bus[cpu->mulFU.cc].tag = cpu->mulFU.cc;
                if(forwarding_bus[cpu->mulFU.rd].data > 0)
                {
                    cc_forwarding_bus[cpu->mulFU.cc].data = 1;
                }
                else if(forwarding_bus[cpu->mulFU.rd].data == 0)
                    cc_forwarding_bus[cpu->mulFU.cc].data = 0;
                // printf("Forwarding bus mul: %d | %d | %d\n",forwarding_bus[cpu->mulFU.rd].valid, forwarding_bus[cpu->mulFU.rd].tag, forwarding_bus[cpu->mulFU.rd].data);
                cpu->mulFU.busy = FALSE;
                mul_counter = 0;
            }
            if (ENABLE_DEBUG_MESSAGES)
            {
                print_stage_content("MUL_FU", &cpu->mulFU);
            }
        }
    }
    if (cpu->memory.has_insn)
    {
        mau_counter++;
        if (mau_counter == 2)
        {
            switch (cpu->memory.opcode)
            {
            case OPCODE_STOREP:
            {
                cpu->data_memory[lsq[lsq_head].mem_addr] = cpu->memory.rs1_value;
                mau_counter = 0;
                arf.r[rob[rob_head].dest_arch] = prf_file[rob[rob_head].dest_physical].pr.value;
                reg_free_list[rename_tail + 1] = rob[rob_head].prev;
                rename_tail += 1;
                // reg_free_list[rob[rob_head].prev] = 0; --> this needs to be done but need to add to end of free list TODO
                arf.commited_instr_address = rob[rob_head].pc_value;
                rob[rob_head].entry_bit = 0;
                rob_head = (rob_head + 1) % ROB_SIZE;
                lsq[lsq_head].entry_bit = 0;
                lsq_head = (lsq_head + 1) % LSQ_SIZE;
                // lsq_head = (lsq_head +1) % LSQ_SIZE;
                cpu->memory.busy = FALSE;
                cpu->memory.has_insn = FALSE;
                break;
            }
            case OPCODE_STORE:
            {
                cpu->data_memory[lsq[lsq_head].mem_addr] = cpu->memory.rs1_value;
                mau_counter = 0;
                // reg_free_list[rob[rob_head].prev] = 0; --> this needs to be done but need to add to end of free list TODO
                arf.commited_instr_address = rob[rob_head].pc_value;
                rob[rob_head].entry_bit = 0;
                rob_head = (rob_head + 1) % ROB_SIZE;
                lsq[lsq_head].entry_bit = 0;
                lsq_head = (lsq_head + 1) % LSQ_SIZE;
                // lsq_head = (lsq_head +1) % LSQ_SIZE;
                cpu->memory.busy = FALSE;
                cpu->memory.has_insn = FALSE;
                break;
            }
            case OPCODE_LOADP:
            case OPCODE_LOAD:
            {
                cpu->memory.result_buffer = cpu->data_memory[cpu->memory.memory_address];
                forwarding_bus[cpu->memory.rd].valid = 1;
                forwarding_bus[cpu->memory.rd].tag = cpu->memory.rd;
                forwarding_bus[cpu->memory.rd].data = cpu->memory.result_buffer;
                lsq[lsq_head].mem_addr_valid_bit = 1;
                mau_counter = 0;
                cpu->memory.busy = FALSE;
                cpu->memory.has_insn = FALSE;
                break;
            }
            }
        }
        if (ENABLE_DEBUG_MESSAGES)
            {
                print_stage_content("MAU", &cpu->memory);
            }
    }
    if(cpu->bfu.has_insn)
    {
        switch(cpu->bfu.opcode)
        {
            case OPCODE_BNZ:
            {
                if(cpu->bfu.cc_value == 0)
                {
                    update_btb_entry(cpu, 'T');
                }
                if(cpu->bfu.predicted_decision)
                {
                    cpu->pc = cpu->bfu.memory_address;
                    cpu->stall = FALSE;
                    cpu->bfu.busy = FALSE;
                }
            }
        }
    }
    if (cpu->afu.has_insn)
    {
        switch (cpu->afu.opcode)
        {
        case OPCODE_STOREP:
        {
            // cpu->memory.opcode = cpu->afu.opcode;
            forwarding_bus[cpu->afu.increment_reg_for_storep_loadp].valid = 1;
            forwarding_bus[cpu->afu.increment_reg_for_storep_loadp].tag = cpu->afu.increment_reg_for_storep_loadp;
            forwarding_bus[cpu->afu.increment_reg_for_storep_loadp].data = cpu->afu.rs2_value + 4;
            lsq[cpu->afu.rd].mem_addr = cpu->afu.rs2_value + cpu->afu.imm;
            lsq[cpu->afu.rd].mem_addr_valid_bit = 1;
            cpu->afu.busy = FALSE;
            break;
        }
        case OPCODE_STORE:
        {
            // cpu->memory.opcode = cpu->afu.opcode;
            lsq[cpu->afu.rd].mem_addr = cpu->afu.rs2_value + cpu->afu.imm;
            lsq[cpu->afu.rd].mem_addr_valid_bit = 1;
            cpu->afu.busy = FALSE;
            break;
        }
        case OPCODE_LOADP:
        {
            cpu->memory.memory_address = cpu->afu.rs1_value + cpu->afu.imm;
            cpu->memory.opcode = OPCODE_LOADP;
            forwarding_bus[cpu->afu.increment_reg_for_storep_loadp].valid = 1;
            forwarding_bus[cpu->afu.increment_reg_for_storep_loadp].tag = cpu->afu.increment_reg_for_storep_loadp;
            forwarding_bus[cpu->afu.increment_reg_for_storep_loadp].data = cpu->afu.rs1_value + 4;
            //printf("Increment reg is %d:%d\n", cpu->afu.increment_reg_for_storep_loadp, forwarding_bus[cpu->afu.increment_reg_for_storep_loadp].data);
            // cpu->memory.increment_reg_for_storep_loadp = cpu->afu.increment_reg_for_storep_loadp;
            cpu->memory.rd = cpu->afu.rd;
            cpu->afu.busy = FALSE;
            break;
        }
        case OPCODE_LOAD:
        {
            cpu->memory.memory_address = cpu->afu.rs1_value + cpu->afu.imm;
            cpu->memory.opcode = OPCODE_LOADP;
            //printf("Increment reg is %d:%d\n", cpu->afu.increment_reg_for_storep_loadp, forwarding_bus[cpu->afu.increment_reg_for_storep_loadp].data);
            cpu->memory.rd = cpu->afu.rd;
            cpu->afu.busy = FALSE;
            break;
        }
        case OPCODE_BZ:
        case OPCODE_BNZ:
        case OPCODE_BP:
        case OPCODE_BNP:
        {
            cpu->bfu.memory_address = cpu->afu.pc + cpu->afu.imm;
            for(int i =0;i< BQ_SIZE;i++)
            {
                if(bq[i].tag == cpu->afu.rd)
                {
                    bq[i].target_address = cpu->bfu.memory_address;
                }
            }
            cpu->afu.busy = FALSE;
            break;
        }
        }
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("AFU", &cpu->afu);
        }
    }
}

/*
 * Execute Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */

// void branch_updation(APEX_CPU *cpu, char actual_decision)
// {
//     btb[cpu->execute.btb_probe_index].target_address = cpu->execute.pc + cpu->execute.imm;
//     if (actual_decision == 'T')
//     {
//         if (cpu->execute.btb_hit)
//         {
//             if (!cpu->execute.predicted_decision)
//             {
//                 update_btb_entry(cpu, 'T');
//                 cpu->pc = cpu->execute.pc + cpu->execute.imm;
//                 cpu->fetch_from_next_cycle = TRUE;
//                 cpu->decode.has_insn = FALSE;
//                 cpu->fetch.has_insn = TRUE;
//             }
//             else
//             {
//                 update_btb_entry(cpu, 'T');
//             }
//         }
//         else if (!cpu->execute.btb_hit)
//         {
//             update_btb_entry(cpu, 'T');
//             cpu->pc = cpu->execute.pc + cpu->execute.imm;
//             cpu->fetch_from_next_cycle = TRUE;
//             cpu->decode.has_insn = FALSE;
//             cpu->fetch.has_insn = TRUE;
//         }
//     }
//     else if (actual_decision == 'N')
//     {
//         if (cpu->execute.btb_hit)
//         {
//             if (cpu->execute.predicted_decision)
//             {
//                 update_btb_entry(cpu, 'N');
//                 cpu->pc = cpu->execute.pc + 4;
//                 cpu->fetch_from_next_cycle = TRUE;
//                 cpu->decode.has_insn = FALSE;
//                 cpu->fetch.has_insn = TRUE;
//             }
//         }
//         else
//         {
//             update_btb_entry(cpu, 'N');
//         }
//     }
// }
// void branch_instruction(APEX_CPU *cpu)
// {
//     /* Calculate new PC, and send it to fetch unit */
//     cpu->pc = cpu->execute.pc + cpu->execute.imm;

//     /* Since we are using reverse callbacks for pipeline stages,
//      * this will prevent the new instruction from being fetched in the current cycle*/
//     cpu->fetch_from_next_cycle = TRUE;

//     /* Flush previous stages */
//     cpu->decode.has_insn = FALSE;

//     /* Make sure fetch stage is enabled to start fetching from new PC */
//     cpu->fetch.has_insn = TRUE;
// }
void set_condition_codes(APEX_CPU *cpu)
{
    if (cpu->execute.result_buffer == 0)
    {
        cpu->zero_flag = TRUE;
        cpu->poisitve_flag = FALSE;
        cpu->negative_flag = FALSE;
    }
    else if (cpu->execute.result_buffer > 0)
    {
        cpu->zero_flag = FALSE;
        cpu->poisitve_flag = TRUE;
        cpu->negative_flag = FALSE;
    }
    else if (cpu->execute.result_buffer < 0)
    {
        cpu->zero_flag = FALSE;
        cpu->negative_flag = TRUE;
        cpu->poisitve_flag = FALSE;
    }
}

/*
 * Memory Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_memory(APEX_CPU *cpu)
{
    if (cpu->memory.has_insn)
    {
        cpu->no_forward = FALSE;
        cpu->memory_update_rs1 = FALSE;
        cpu->memory_update_rs2 = FALSE;
        switch (cpu->memory.opcode)
        {
        case OPCODE_ADDL:
        case OPCODE_SUB:
        case OPCODE_SUBL:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        case OPCODE_ADD:
        case OPCODE_CML:
        case OPCODE_CMP:
        case OPCODE_JUMP:
        case OPCODE_JALR:
        case OPCODE_MOVC:
        case OPCODE_MUL:
        {
            ////data_forwarding(cpu);
            /* No work for ADD */
            break;
        }

        case OPCODE_LOAD:
        {
            /* Read from data memory */
            cpu->memory.result_buffer = cpu->data_memory[cpu->memory.memory_address];
            //////data_forwarding(cpu);
            break;
        }
        case OPCODE_LOADP:
        {
            /* Read from data memory */
            cpu->memory.result_buffer = cpu->data_memory[cpu->memory.memory_address];
            // data_forwarding(cpu);
            if (cpu->stall)
            {
                cpu->stall = FALSE;
                cpu->dirty = TRUE;
            }
            break;
        }
        case OPCODE_STORE:
        {
            /* Write  data to memory */
            cpu->data_memory[cpu->memory.memory_address] = cpu->memory.rs1_value;
            break;
        }
        case OPCODE_STOREP:
        {
            /* Write  data to memory */
            cpu->data_memory[cpu->memory.memory_address] = cpu->memory.rs1_value;
            // data_forwarding(cpu);
            if (cpu->stall)
            {
                cpu->stall = FALSE;
                cpu->dirty = TRUE;
            }
            break;
        }
        }

        /* Copy data from memory latch to writeback latch*/
        cpu->writeback = cpu->memory;
        cpu->memory.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Memory", &cpu->memory);
        }
    }
}

/*
 * Writeback Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static int
APEX_writeback(APEX_CPU *cpu)
{
    if (cpu->writeback.has_insn)
    {
        /* Write result to register file based on instruction type */
        switch (cpu->writeback.opcode)
        {
        case OPCODE_SUB:
        case OPCODE_MUL:
        case OPCODE_ADDL:
        case OPCODE_SUBL:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        case OPCODE_ADD:
        {
            cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
            break;
        }

        case OPCODE_LOAD:
        {
            cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
            break;
        }
        case OPCODE_LOADP:
        {
            cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
            cpu->regs[cpu->writeback.rs1] = cpu->writeback.rs1_value;
            cpu->reg_valid[cpu->writeback.rs1] = 0;
            break;
        }

        case OPCODE_MOVC:
        {
            cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
            break;
        }
        case OPCODE_STORE:
        case OPCODE_CML:
        case OPCODE_CMP:
        {
            // No action
            break;
        }
        case OPCODE_STOREP:
        {
            cpu->regs[cpu->writeback.rs2] = cpu->writeback.rs2_value;
            cpu->reg_valid[cpu->writeback.rs2] = 0;
            break;
        }
        case OPCODE_JALR:
        {
            cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
            break;
        }
        }
        cpu->reg_valid[cpu->writeback.rd] = 0;
        if (!cpu->status)
        {
            cpu->status = TRUE;
        }
        cpu->insn_completed++;
        cpu->writeback.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Writeback", &cpu->writeback);
        }

        if (cpu->writeback.opcode == OPCODE_HALT)
        {
            /* Stop the APEX simulator */
            return TRUE;
        }
    }

    /* Default */
    return 0;
}
void init_btb()
{
    for (int i = 0; i < 4; i++)
    {
        btb[i].valid = 0;
        btb[i].inst_address = -1;
        btb[i].prev_outcome[0] = 0;
        btb[i].prev_outcome[1] = 0;
        btb[i].target_address = -1;
    }
}
int predict_branch(APEX_CPU *cpu)
{
    int i = cpu->fetch.btb_probe_index;
    if (btb[i].valid && btb[i].inst_address == cpu->fetch.pc)
    {
        if ((btb[i].prev_outcome[0] == 1 && btb[i].prev_outcome[1] == 1) || (btb[i].prev_outcome[0] == 1 && btb[i].prev_outcome[1] == 0))
        {

            cpu->fetch.predicted_decision = 1;
            return 1;
        }
        else
        {
            cpu->fetch.predicted_decision = 0;
            return 0;
        }
    }
    return -1;
}
void update_btb_entry(APEX_CPU *cpu, char pred)
{
    int btb_index = cpu->bfu.btb_probe_index;
    if (btb[btb_index].prev_outcome[0] == 1 && btb[btb_index].prev_outcome[1] == 1)
    {
        if (pred == 'N')
        {
            btb[btb_index].prev_outcome[0] = 1;
            btb[btb_index].prev_outcome[1] = 0;
        }
    }
    else if (btb[btb_index].prev_outcome[0] == 1 && btb[btb_index].prev_outcome[1] == 0)
    {
        if (pred == 'T')
        {
            btb[btb_index].prev_outcome[1] = 1;
        }
        else if (pred == 'N')
        {
            btb[btb_index].prev_outcome[0] = 0;
            btb[btb_index].prev_outcome[1] = 1;
        }
    }
    else if (btb[btb_index].prev_outcome[0] == 0 && btb[btb_index].prev_outcome[1] == 1)
    {
        if (pred == 'T')
        {
            btb[btb_index].prev_outcome[0] = 1;
            btb[btb_index].prev_outcome[1] = 0;
        }
        else if (pred == 'N')
        {
            btb[btb_index].prev_outcome[0] = 0;
            btb[btb_index].prev_outcome[1] = 0;
        }
    }
    else if (btb[btb_index].prev_outcome[0] == 0 && btb[btb_index].prev_outcome[1] == 0)
    {
        if (pred == 'T')
        {
            btb[btb_index].prev_outcome[0] = 0;
            btb[btb_index].prev_outcome[1] = 1;
        }
    }
}
int is_btb_hit(APEX_CPU *cpu)
{
    for (int i = 0; i < 4; i++)
    {
        if (btb[i].valid && cpu->fetch.pc == btb[i].inst_address)
        {
            // BTB hit
            cpu->fetch.btb_hit = TRUE;
            cpu->fetch.btb_probe_index = i;
            return i;
        }
    }
    cpu->fetch.btb_hit = FALSE;
    return -1;
}
void create_btb_entry(APEX_CPU *cpu)
{
    int i = 0;
    for (i = 0; i < BTB_SIZE; i++)
    {
        if (!btb[i].valid)
        {
            btb[i].valid = 1;
            btb[i].inst_address = cpu->decode1.pc;
            if (cpu->decode1.opcode == OPCODE_BNZ || cpu->decode1.opcode == OPCODE_BP)
            {
                btb[i].prev_outcome[0] = 1;
                btb[i].prev_outcome[1] = 1;
            }
            else // BZ and BNP case
            {
                btb[i].prev_outcome[0] = 0;
                btb[i].prev_outcome[1] = 0;
            }
            cpu->decode1.btb_probe_index = i;
            break;
        }
    }
    if (i == BTB_SIZE)
    {
        int i = 0;
        for (i = 0; i < BTB_SIZE - 1; i++)
        {
            btb[i] = btb[i + 1];
        }
        btb[i].valid = 1;
        btb[i].inst_address = cpu->decode1.pc;
        if (cpu->decode1.opcode == OPCODE_BNZ || cpu->decode1.opcode == OPCODE_BP)
        {
            btb[i].prev_outcome[0] = 1;
            btb[i].prev_outcome[1] = 1;
        }
        else // BZ and BNP case
        {
            btb[i].prev_outcome[0] = 0;
            btb[i].prev_outcome[1] = 0;
        }
        cpu->decode1.btb_probe_index = i;
    }
}

/*
 * This function creates and initializes APEX cpu.
 *
 * Note: You are free to edit this function according to your implementation
 */
APEX_CPU *
APEX_cpu_init(const char *filename)
{
    int i;
    APEX_CPU *cpu;

    if (!filename)
    {
        return NULL;
    }

    cpu = calloc(1, sizeof(APEX_CPU));

    if (!cpu)
    {
        return NULL;
    }

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = 4000;
    memset(cpu->regs, 0, sizeof(int) * REG_FILE_SIZE);
    memset(cpu->reg_valid, 0, sizeof(int) * REG_FILE_SIZE);
    memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE);
    // memset(reg_free_list, 0,sizeof(int) *Free_List_SIZE);
    for (int i = 0; i < Free_List_SIZE; i++)
    {
        reg_free_list[i] = i;
    }
    
    //memset(cc_free_list, 0, sizeof(int) * CC_PSize);
    for (int i = 0; i < CC_PSize; i++)
    {
        cc_free_list[i] = i;
    }
    cpu->single_step = DISABLE_SINGLE_STEP;
    cpu->status = TRUE;
    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
    init_btb();
    if (!cpu->code_memory)
    {
        free(cpu);
        return NULL;
    }

    if (ENABLE_DEBUG_MESSAGES)
    {
        fprintf(stderr,
                "APEX_CPU: Initialized APEX CPU, loaded %d instructions\n",
                cpu->code_memory_size);
        fprintf(stderr, "APEX_CPU: PC initialized to %d\n", cpu->pc);
        fprintf(stderr, "APEX_CPU: Printing Code Memory\n");
        printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode_str", "rd", "rs1", "rs2",
               "imm");

        for (i = 0; i < cpu->code_memory_size; ++i)
        {
            printf("%-9s %-9d %-9d %-9d %-9d\n", cpu->code_memory[i].opcode_str,
                   cpu->code_memory[i].rd, cpu->code_memory[i].rs1,
                   cpu->code_memory[i].rs2, cpu->code_memory[i].imm);
        }
    }

    /* To start fetch stage */
    cpu->fetch.has_insn = TRUE;
    return cpu;
}
static void print_mem(const APEX_CPU *cpu, int address)
{
    if (address != 0)
        printf("Content of Memory location,Mem[%d] : %d\n", address, cpu->data_memory[address]);
}

/*
 * APEX CPU simulation loop
 *
 * Note: You are free to edit this function according to your implementation
 */
void APEX_cpu_run(APEX_CPU *cpu, int command)
{
    char user_prompt_val;
    int no_of_cycles;
    while (TRUE)
    {
        if (command == 4)
        {
            print_reg_file(cpu);
            return;
        }
        else if (command == 2)
        {
            printf("Enter no of cycles to be simulated\n");
            scanf("%d", &no_of_cycles);
            if (no_of_cycles < cpu->clock)
            {
                printf("Reached end of simulation already!!! Please re initialize\n");
                return;
            }
            while (no_of_cycles > cpu->clock)
            {
                if (ENABLE_DEBUG_MESSAGES)
                {
                    printf("--------------------------------------------\n");
                    printf("Clock Cycle #: %d\n", cpu->clock + 1);
                    printf("--------------------------------------------\n");
                }

                if (rob[rob_head].instr_type == "HALT")
                {
                    /* Stop simulation if ROB head contains HALT*/
                    printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock + 1, cpu->insn_completed);
                    break;
                }

            APEX_FU(cpu);
            APEX_iq(cpu);
            APEX_decode2(cpu);
            APEX_decode1(cpu);
            APEX_fetch(cpu);
            print_reg_file(cpu);
                cpu->clock++;
                if (no_of_cycles == cpu->clock)
                {
                    print_reg_file(cpu);
                    printf("APEX_CPU: Simulation Stopped after = %d cycles  instructions = %d\n", cpu->clock, cpu->insn_completed);
                    return;
                }
            }
        }
        else if (command == 3)
        {
            cpu->single_step = ENABLE_SINGLE_STEP;
            if (ENABLE_DEBUG_MESSAGES)
            {
                printf("--------------------------------------------\n");
                printf("Clock Cycle #: %d\n", cpu->clock + 1);
                printf("--------------------------------------------\n");
            }

            if (rob[rob_head].instr_type == "HALT")
            {
                /* Stop simulation if ROB head contains HALT*/
                printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock + 1, cpu->insn_completed);
                break;
            }

            APEX_FU(cpu);
            APEX_iq(cpu);
            APEX_decode2(cpu);
            APEX_decode1(cpu);
            APEX_fetch(cpu);
            print_reg_file(cpu);
            if (cpu->single_step)
            {
                printf("Press any key to advance CPU Clock or <q> to quit:\n");
                scanf("%c", &user_prompt_val);

                if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
                {
                    printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                    break;
                }
            }

            cpu->clock++;
        }

        else if (command == 5)
        {
            int address;
            printf("Enter Memory address\n");
            scanf("%d", &address);
            print_mem(cpu, address);
            return;
        }
    }
}
/*
 * This function deallocates APEX CPU.
 *
 * Note: You are free to edit this function according to your implementation
 */
void APEX_cpu_stop(APEX_CPU *cpu)
{
    free(cpu);
}