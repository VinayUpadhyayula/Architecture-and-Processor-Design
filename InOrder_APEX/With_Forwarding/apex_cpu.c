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
    int i;

    printf("----------\n%s\n----------\n", "Registers:");

    for (int i = 0; i < REG_FILE_SIZE / 2; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");
    for (i = (REG_FILE_SIZE / 2); i < REG_FILE_SIZE; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }
    printf("\n----------\n%s\n----------\n","CC_Flags:");
    printf("Z[%-3d] P[%-3d] N[%-3d]\n", cpu->zero_flag, cpu->poisitve_flag, cpu->negative_flag);
     printf("\n----------\n%s\n----------\n","Memory:");
     for(int i =0;i<DATA_MEMORY_SIZE;i++)
     {
        if(cpu->data_memory[i] != 0)
        {
            printf("Mem[%-3d] = %d ",i, cpu->data_memory[i]);
        }
     }
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

    if (cpu->fetch.has_insn)
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

        /* Update PC for next instruction */
        cpu->pc += 4;

        /* Copy data from fetch latch to decode latch*/

        cpu->decode = cpu->fetch;

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
APEX_decode(APEX_CPU *cpu)
{
    if (cpu->decode.has_insn)
    {

        /* Read operands from register file based on the instruction type */
        switch (cpu->decode.opcode)
        {
        case OPCODE_CMP:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        case OPCODE_MUL:
        case OPCODE_SUB:
        case OPCODE_ADD:
        case OPCODE_STORE:
        {
            if((!cpu->execute.has_insn && !cpu->memory.has_insn)|| cpu->no_forward)
            {
            cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
            
            
            cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
            }
            break;
        }

        case OPCODE_LOAD:
        case OPCODE_JUMP:
        case OPCODE_JALR:
        {
            if((!cpu->execute.has_insn && !cpu->memory.has_insn)|| cpu->no_forward)
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
            }
            break;
        }

        case OPCODE_MOVC:
        case OPCODE_HALT:
        case OPCODE_NOP:
        case OPCODE_BZ:
        case OPCODE_BNZ:
        case OPCODE_BP:
        case OPCODE_BNP:
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
            if((!cpu->execute.has_insn && !cpu->memory.has_insn) || cpu->no_forward)
            {
            cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
            }
            break;
        }
        case OPCODE_LOADP:
        {
            if((!cpu->execute.has_insn && !cpu->memory.has_insn) || cpu->no_forward)
            {
            cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
            }
            break;
        }
        case OPCODE_STOREP:
        {
            if((!cpu->execute.has_insn && !cpu->memory.has_insn)|| cpu->no_forward)
            {
            cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
            cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
            }
            break;
        }
        }
        cpu->execute = cpu->decode;
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Decode/RF", &cpu->decode);
        }
    }
}
void data_forwarding(APEX_CPU *cpu)
{
  cpu->rs1_updated = FALSE;
  cpu->rs2_updated = FALSE;
  switch(cpu->decode.opcode)
  {
    case OPCODE_ADD:
    case OPCODE_SUB:
    case OPCODE_MUL:
    case OPCODE_STORE:
    case OPCODE_STOREP:
    case OPCODE_CMP:
    case OPCODE_AND:
    case OPCODE_OR:
    case OPCODE_XOR:
    {
        if(cpu->visited)
        {
        if(cpu->execute.has_insn && (cpu->execute.rd == cpu->decode.rs1) && (cpu->execute.opcode != OPCODE_LOADP) && (cpu->execute.opcode != OPCODE_STOREP))
        {
             printf("Forwarded matched rs1 value from exec: %d", cpu->execute.result_buffer);
            cpu->decode.rs1_value = cpu->execute.result_buffer;
            cpu->rs1_updated = TRUE;
        }
        else if(cpu->memory.has_insn && (cpu->memory.rd == cpu->decode.rs1))
        {
            printf("Forwarded matched rs1 value from mem: %d", cpu->execute.result_buffer);
            cpu->decode.rs1_value = cpu->memory.result_buffer;
            cpu->memory_update_rs1 = TRUE;
        }
        else if(!cpu->memory_update_rs1)
        {
            printf("rs1 value default from reg: %d", cpu->regs[cpu->decode.rs1]);
            cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
        }
        if(cpu->execute.has_insn && (cpu->execute.rd == cpu->decode.rs2) && (cpu->execute.opcode != OPCODE_LOADP) && (cpu->execute.opcode != OPCODE_STOREP))
        {
            printf("Forwarded matched rs2 value from exec: %d", cpu->execute.result_buffer);
            cpu->decode.rs2_value = cpu->execute.result_buffer;
            cpu->rs2_updated = TRUE;
        }
        else if(cpu->memory.has_insn && (cpu->memory.rd == cpu->decode.rs2))
        {
            printf("Forwarded matched rs2 value from Memory: %d", cpu->memory.result_buffer);
            cpu->decode.rs2_value = cpu->memory.result_buffer;
             cpu->memory_update_rs2= TRUE;
        }
       else if(!cpu->memory_update_rs2)
        {
            printf("rs2 value default from reg: %d", cpu->regs[cpu->decode.rs2]);
            cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
        }
        check_forwarding_for_LOADP_and_STOREP(cpu);
        }
        break;
    }
    case OPCODE_ADDL:
    case OPCODE_SUBL:
    case OPCODE_CML:
    case OPCODE_LOAD:
    case OPCODE_LOADP:
    case OPCODE_JUMP:
    case OPCODE_JALR:
    {
        if (cpu->visited)
        {
            if (cpu->execute.has_insn && (cpu->execute.rd == cpu->decode.rs1))
            {

                if(cpu->execute.opcode != OPCODE_STOREP)
                cpu->decode.rs1_value = cpu->execute.result_buffer;
                cpu->rs1_updated = TRUE;
            }
            else if (cpu->memory.has_insn && (cpu->memory.rd == cpu->decode.rs1))
            {
                cpu->decode.rs1_value = cpu->memory.result_buffer;
                cpu->memory_update_rs1 = TRUE;
            }
            else if(!cpu->memory_update_rs1)
            {
              cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
            }
            check_forwarding_for_LOADP_and_STOREP(cpu);
        }
        break;
    }   
  }
}
void check_forwarding_for_LOADP_and_STOREP(APEX_CPU *cpu)
{
    switch(cpu->execute.opcode)
    {
        case OPCODE_LOADP:
        {
            if(cpu->execute.rs1 == cpu->decode.rs1)
            {
                cpu->decode.rs1_value = cpu->execute.rs1_value;
                break;
            }
            if(cpu->execute.rs1 == cpu->decode.rs2)
            {
                cpu->decode.rs2_value = cpu->execute.rs1_value;
                break;
            }
            // if(cpu->execute.rd == cpu->decode.rs1)
            // {
            //     cpu->decode.rs1_value = cpu->execute.;
            // }
            break;
        }
        case OPCODE_STOREP:
        {
            if(cpu->execute.rs2 == cpu->decode.rs1)
            {
                cpu->decode.rs1_value = cpu->execute.rs2_value;
            }
            if(cpu->execute.rs2 == cpu->decode.rs2)
            {
                cpu->decode.rs2_value = cpu->execute.rs2_value;
            }
            break;
        }
        default:
        {
            break;
        }
    }
    switch(cpu->memory.opcode)
    {
        case OPCODE_LOADP:
        {
            if(cpu->memory.rs1 == cpu->decode.rs1 && !cpu->rs1_updated)
            {
                cpu->decode.rs1_value = cpu->memory.rs1_value;
            }
            if(cpu->memory.rs1 == cpu->decode.rs2 && !cpu->rs2_updated)
            {
                cpu->decode.rs2_value = cpu->memory.rs1_value;
            }
            break;
        }
        case OPCODE_STOREP:
        {
            if(cpu->memory.rs2 == cpu->decode.rs1 && !cpu->rs1_updated)
            {
                cpu->decode.rs1_value = cpu->memory.rs2_value;
            }
            if(cpu->memory.rs2 == cpu->decode.rs2 && !cpu->rs2_updated)
            {
                cpu->decode.rs2_value = cpu->memory.rs2_value;
            }
            break;
        }
        default:
        {
            break;
        }
    }
}
void score_boarding(APEX_CPU *cpu)
{
    switch (cpu->decode.opcode)
    {
    case OPCODE_ADD:
    case OPCODE_SUB:
    case OPCODE_MUL:
    case OPCODE_AND:
    case OPCODE_OR:
    case OPCODE_XOR:
    {
        if (!(cpu->reg_valid[cpu->decode.rd] || (cpu->reg_valid[cpu->decode.rs1] || cpu->reg_valid[cpu->decode.rs2])))
        {
            cpu->execute = cpu->decode;
            cpu->reg_valid[cpu->decode.rd] = 1;
            cpu->decode.has_insn = FALSE;
            cpu->status = TRUE;
            cpu->fetch.has_insn = TRUE;
        }
        else
        {
            cpu->status = FALSE;
            cpu->fetch.has_insn = FALSE;
        }
        break;
    }
    case OPCODE_ADDL:
    case OPCODE_SUBL:
    case OPCODE_LOAD:
    case OPCODE_JALR:
    {
        if (!(cpu->reg_valid[cpu->decode.rd] || cpu->reg_valid[cpu->decode.rs1]))
        {
            cpu->execute = cpu->decode;
            cpu->reg_valid[cpu->decode.rd] = 1;
            cpu->decode.has_insn = FALSE;
            cpu->status = TRUE;
            cpu->fetch.has_insn = TRUE;
        }
        else
        {
            cpu->status = FALSE;
            cpu->fetch.has_insn = FALSE;
        }
        break;
    }
    case OPCODE_STORE:
    case OPCODE_CMP:
    {
        if (!(cpu->reg_valid[cpu->decode.rs1] || cpu->reg_valid[cpu->decode.rs2]))
        {
            // printf("entered");
            cpu->execute = cpu->decode;
            cpu->decode.has_insn = FALSE;
            cpu->status = TRUE;
            cpu->fetch.has_insn = TRUE;
        }
        else
        {
            cpu->status = FALSE;
            cpu->fetch.has_insn = FALSE;
        }
        break;
    }
    case OPCODE_CML:
    {
        if (!(cpu->reg_valid[cpu->decode.rs1]))
        {
            cpu->execute = cpu->decode;
            cpu->decode.has_insn = FALSE;
            cpu->status = TRUE;
            cpu->fetch.has_insn = TRUE;
        }
        else
        {
            cpu->status = FALSE;
            cpu->fetch.has_insn = FALSE;
        }
        break;
    }
    case OPCODE_MOVC:
    {
        if (!(cpu->reg_valid[cpu->decode.rd]))
        {
            // printf("entered");
            cpu->execute = cpu->decode;
            cpu->decode.has_insn = FALSE;
            cpu->reg_valid[cpu->decode.rd] = 1;
            cpu->status = TRUE;
            cpu->fetch.has_insn = TRUE;
        }
        else
        {
            cpu->status = FALSE;
            cpu->fetch.has_insn = FALSE;
        }
        break;
    }
    case OPCODE_HALT:
    case OPCODE_NOP:
    case OPCODE_BZ:
    case OPCODE_BNZ:
    case OPCODE_BP:
    case OPCODE_BNP:
    case OPCODE_BN:
    case OPCODE_BNN:
    case OPCODE_JUMP:
    {
        cpu->execute = cpu->decode;
        break;
    }
    case OPCODE_STOREP:
    {
        if (!(cpu->reg_valid[cpu->decode.rs1] || cpu->reg_valid[cpu->decode.rs2]))
        {
            // printf("entered");
            cpu->execute = cpu->decode;
            cpu->decode.has_insn = FALSE;
            cpu->status = TRUE;
            cpu->reg_valid[cpu->decode.rs2] = 1;
            cpu->fetch.has_insn = TRUE;
        }
        else
        {
            cpu->status = FALSE;
            cpu->fetch.has_insn = FALSE;
        }
        break;
    }
    case OPCODE_LOADP:
    {
        {
            if (!(cpu->reg_valid[cpu->decode.rd] || cpu->reg_valid[cpu->decode.rs1]))
            {
                // printf("entered");
                cpu->execute = cpu->decode;
                cpu->reg_valid[cpu->decode.rd] = 1;
                cpu->reg_valid[cpu->decode.rs1] = 1;
                cpu->decode.has_insn = FALSE;
                cpu->status = TRUE;
                cpu->fetch.has_insn = TRUE;
            }
            else
            {
                cpu->status = FALSE;
                cpu->fetch.has_insn = FALSE;
            }
            break;
        }
    }
    }
}

/*
 * Execute Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_execute(APEX_CPU *cpu)
{
    if (cpu->execute.has_insn)
    {
        cpu->no_forward = FALSE;
        cpu->visited = TRUE;
        /* Execute logic based on instruction type */
        switch (cpu->execute.opcode)
        {
        case OPCODE_ADD:
        {

            cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.rs2_value;
            /* Set the zero flag based on the result buffer */
            set_condition_codes(cpu);
            data_forwarding(cpu);
            break;
        }

        case OPCODE_SUB:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value - cpu->execute.rs2_value;
            

            /* Set the zero flag based on the result buffer */
            set_condition_codes(cpu);
            data_forwarding(cpu);
            break;
        }
        case OPCODE_MUL:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value * cpu->execute.rs2_value;

            /* Set the zero flag based on the result buffer */
            set_condition_codes(cpu);
            data_forwarding(cpu);
            break;
        }

        case OPCODE_LOAD:
        {
            cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.imm;
            break;
        }
        case OPCODE_LOADP:
        {
            cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.imm;
            printf("Calculated Memory address is: %d\n",cpu->execute.memory_address);
            cpu->execute.rs1_value += 4;
            data_forwarding(cpu);
            break;
        }
        case OPCODE_STORE:
        {
            cpu->execute.memory_address = cpu->execute.rs2_value + cpu->execute.imm;
            break;
        }
        case OPCODE_STOREP:
        {
            cpu->execute.memory_address = cpu->execute.rs2_value + cpu->execute.imm;
            printf("Calculated Memory address is: %d\n",cpu->execute.memory_address);
            cpu->execute.rs2_value += 4;
            data_forwarding(cpu);
            break;
        }
        case OPCODE_JALR:
        {
            cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.imm;
            cpu->execute.result_buffer = cpu->execute.pc + 4;
            cpu->pc = cpu->execute.memory_address;
            cpu->fetch_from_next_cycle = TRUE;
            cpu->decode.has_insn = FALSE;
            cpu->fetch.has_insn = TRUE;
            data_forwarding(cpu);
            break;
        }
        case OPCODE_JUMP:
        {
            cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.imm;
            cpu->pc = cpu->execute.memory_address;
            cpu->fetch_from_next_cycle = TRUE;
            cpu->decode.has_insn = FALSE;
            cpu->fetch.has_insn = TRUE;
            break;
        }

        case OPCODE_BZ:
        {
            cpu->no_forward = TRUE;
            if (cpu->zero_flag == TRUE)
            {
                branch_instruction(cpu);
            }
            break;
        }

        case OPCODE_BNZ:
        {
            cpu->no_forward = TRUE;
            if (cpu->zero_flag == FALSE)
            {
                branch_instruction(cpu);
            }
            break;
        }
        case OPCODE_BP:
            cpu->no_forward = TRUE;
        {
            if (cpu->poisitve_flag == TRUE)
            {
                branch_instruction(cpu);
            }
            break;
        }
        case OPCODE_BNP:
        {
            cpu->no_forward = TRUE;
            if (cpu->poisitve_flag == FALSE)
            {
                branch_instruction(cpu);
            }
            break;
        }
        case OPCODE_BN:
        {
            cpu->no_forward = TRUE;
            if (cpu->negative_flag == TRUE)
            {
                branch_instruction(cpu);
            }
            break;
        }
        case OPCODE_BNN:
        {
            cpu->no_forward = TRUE;
            if (cpu->negative_flag == FALSE)
            {
                branch_instruction(cpu);
            }
            break;
        }

        case OPCODE_MOVC:
        {
            cpu->execute.result_buffer = cpu->execute.imm + 0;
            data_forwarding(cpu);
            break;
        }
        case OPCODE_ADDL:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.imm;
            set_condition_codes(cpu);
            data_forwarding(cpu);
            break;
        }
        case OPCODE_SUBL:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value - cpu->execute.imm;
            set_condition_codes(cpu);
            data_forwarding(cpu);
            break;
        }
        case OPCODE_AND:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value & cpu->execute.rs2_value;
            set_condition_codes(cpu);
            data_forwarding(cpu);
            break;
        }
        case OPCODE_OR:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value | cpu->execute.rs2_value;
            set_condition_codes(cpu);
            printf("OR computed value between %d and %d is %d\n", cpu->execute.rs1_value, cpu->execute.rs2_value, cpu->execute.result_buffer);
            data_forwarding(cpu);
            break;
        }
        case OPCODE_XOR:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value ^ cpu->execute.rs2_value;
            set_condition_codes(cpu);
            data_forwarding(cpu);
            break;
        }
        case OPCODE_CML:
        {
            cpu->no_forward = TRUE;
            if (cpu->execute.rs1_value > cpu->execute.imm)
            {
                cpu->zero_flag = FALSE;
                cpu->poisitve_flag = TRUE;
                cpu->negative_flag = FALSE;
            }
            else if (cpu->execute.rs1_value < cpu->execute.imm)
            {
                cpu->zero_flag = FALSE;
                cpu->poisitve_flag = FALSE;
                cpu->negative_flag = TRUE;
            }
            else if (cpu->execute.rs1_value == cpu->execute.imm)
            {
                cpu->zero_flag = TRUE;
                cpu->poisitve_flag = FALSE;
                cpu->negative_flag = FALSE;
            }
            break;
        }
        case OPCODE_CMP:
        {
            cpu->no_forward = TRUE;
            if (cpu->execute.rs1_value > cpu->execute.rs2_value)
            {
                cpu->zero_flag = FALSE;
                cpu->poisitve_flag = TRUE;
                cpu->negative_flag = FALSE;
            }
            else if (cpu->execute.rs1_value < cpu->execute.rs2_value)
            {
                cpu->zero_flag = FALSE;
                cpu->poisitve_flag = FALSE;
                cpu->negative_flag = TRUE;
            }
            else if (cpu->execute.rs1_value == cpu->execute.rs2_value)
            {
                cpu->zero_flag = TRUE;
                cpu->poisitve_flag = FALSE;
                cpu->negative_flag = FALSE;
            }
            break;
        }
        case OPCODE_NOP:
        case OPCODE_HALT:
        {
            cpu->no_forward = TRUE;
        }
        }

        /* Copy data from execute latch to memory latch*/
        cpu->memory = cpu->execute;
        cpu->execute.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Execute", &cpu->execute);
        }
    }
}
void branch_instruction(APEX_CPU *cpu)
{
    /* Calculate new PC, and send it to fetch unit */
    cpu->pc = cpu->execute.pc + cpu->execute.imm;

    /* Since we are using reverse callbacks for pipeline stages,
     * this will prevent the new instruction from being fetched in the current cycle*/
    cpu->fetch_from_next_cycle = TRUE;

    /* Flush previous stages */
    cpu->decode.has_insn = FALSE;

    /* Make sure fetch stage is enabled to start fetching from new PC */
    cpu->fetch.has_insn = TRUE;

}
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
            data_forwarding(cpu);
            /* No work for ADD */
            break;
        }

        case OPCODE_LOAD:
        case OPCODE_LOADP:
        {
            /* Read from data memory */
            printf("Data from calculated memory address is Mem[%d]: %d", cpu->memory.memory_address,cpu->data_memory[cpu->memory.memory_address]);
            cpu->memory.result_buffer = cpu->data_memory[cpu->memory.memory_address];
            data_forwarding(cpu);
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
            data_forwarding(cpu);
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
    cpu->single_step = DISABLE_SINGLE_STEP;
    cpu->status = TRUE;
    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
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
   if(address !=0)
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
            if(no_of_cycles < cpu->clock)
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

                if (APEX_writeback(cpu))
                {
                    /* Halt in writeback stage */
                    print_reg_file(cpu);
                    printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock+1, cpu->insn_completed);
                    return;
                }

                APEX_memory(cpu);
                APEX_execute(cpu);
                APEX_decode(cpu);
                APEX_fetch(cpu);
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

            if (APEX_writeback(cpu))
            {
                /* Halt in writeback stage */
                printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock+1, cpu->insn_completed);
                break;
            }

            APEX_memory(cpu);
            APEX_execute(cpu);
            APEX_decode(cpu);
            printf("\n%d\n", cpu->pc);
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