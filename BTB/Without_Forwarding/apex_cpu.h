/*
 * apex_cpu.h
 * Contains APEX cpu pipeline declarations
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#ifndef _APEX_CPU_H_
#define _APEX_CPU_H_

#include "apex_macros.h"
#define BTB_SIZE 4

/* Format of an APEX instruction  */
typedef struct APEX_Instruction
{
    char opcode_str[128];
    int opcode;
    int rd;
    int rs1;
    int rs2;
    int imm;
} APEX_Instruction;

/* Model of CPU stage latch */
typedef struct CPU_Stage
{
    int pc;
    char opcode_str[128];
    int opcode;
    int rs1;
    int rs2;
    int rd;
    int imm;
    int rs1_value;
    int rs2_value;
    int result_buffer;
    int memory_address;
    int has_insn;
    int btb_hit;
    int predicted_decision;
    int btb_probe_index;
    
} CPU_Stage;

/* Model of APEX CPU */
typedef struct APEX_CPU
{
    int pc;                        /* Current program counter */
    int clock;                     /* Clock cycles elapsed */
    int insn_completed;            /* Instructions retired */
    int regs[REG_FILE_SIZE];       /* Integer register file */
    int code_memory_size;          /* Number of instruction in the input file */
    APEX_Instruction *code_memory; /* Code Memory */
    int data_memory[DATA_MEMORY_SIZE]; /* Data Memory */
    int single_step;               /* Wait for user input after every cycle */
    int zero_flag;                 /* {TRUE, FALSE} Used by BZ and BNZ to branch */
    int fetch_from_next_cycle;
    int reg_valid[REG_FILE_SIZE];
    int status;
    int poisitve_flag;
    int negative_flag;
    int oldest_entry_index;
    int free_index;
    

    /* Pipeline stages */
    CPU_Stage fetch;
    CPU_Stage decode;
    CPU_Stage execute;
    CPU_Stage memory;
    CPU_Stage writeback;
} APEX_CPU;

typedef struct BTBEntry
{
    int inst_address;
    int prev_outcome[2];
    int target_address;
    int valid;
} BTBEntry;
static struct BTBEntry btb[BTB_SIZE];
APEX_Instruction *create_code_memory(const char *filename, int *size);
APEX_CPU *APEX_cpu_init(const char *filename);
void APEX_cpu_run(APEX_CPU *cpu, int command);
void APEX_cpu_stop(APEX_CPU *cpu);
void set_condition_codes(APEX_CPU *cpu);
void branch_instruction(APEX_CPU *cpu);
void score_boarding(APEX_CPU *cpu);
void init_btb();
int predict_branch(APEX_CPU *cpu);
void update_btb_entry(APEX_CPU *cpu, char pred);
void create_btb_entry(APEX_CPU *cpu);
int is_btb_hit(APEX_CPU *cpu);
void branch_updation(APEX_CPU *cpu, char actual_decision);
#endif
