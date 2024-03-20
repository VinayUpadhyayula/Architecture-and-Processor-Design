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
    int busy;
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
    int no_forward;
    int src1_valid;
    int src2_valid;
    int arch_reg;
    int increment_reg_for_storep_loadp;
    int arch_reg_for_loadp;
    int prev_rs1_for_loadp;
    int cc;
    int cc_value;
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
    int visited;
    int no_forward;
    int memory_update_rs1;
    int memory_update_rs2;
    int rs1_updated;
    int rs2_updated;
    int stall;
    int dirty;
    


    /* Pipeline stages */
    CPU_Stage fetch;
    CPU_Stage decode1;
    CPU_Stage decode2;
    CPU_Stage iq;
    CPU_Stage execute;
    CPU_Stage memory;
    CPU_Stage writeback;
    CPU_Stage intFU;
    CPU_Stage mulFU;
    CPU_Stage afu;
    CPU_Stage bfu;
} APEX_CPU;

typedef struct BTBEntry
{
    int inst_address;
    int prev_outcome[2];
    int target_address;
    int valid;
} BTBEntry;

typedef struct BQ
{
    int instr_type;
    int prev_outcome[2];
    int target_address;
    int valid;
    int tag;
    int value;
    int elapsed_clock;
    int dest_physical;
    int saved_ret_addr;
} BQ;

typedef struct IQ
{
    int free;
    char* fu_type;
    char* operation;
    int literal;
    int src1_valid_bit;
    int src1_tag;
    int src1_value;
    int src2_valid_bit;
    int src2_tag;
    int src2_value;
    int dest_type; //LSQ - 0/physical reg - 1
    int dest;
    int dispatch_time;
    int cc;
    int opcode;
}IQ;

typedef struct ROB
{
    int entry_bit;
    char* instr_type;
    int prev;
    int prev_cc;
    int pc_value;
    int dest_physical;
    int dest_arch;
    int lsq_index;
    int rs1_arch_for_loadp;
    int rs1_physical_for_loadp;
    int rs1_prev;
    int cc;
    char* err_code; //only for LOAD/STORE
}ROB;

typedef struct LSQ
{
    int entry_bit;
    int load_store_bit;//1 for LOAD, 0 for STORE
    int mem_addr_valid_bit;
    int mem_addr;
    int dest;//only for LOAD
    int src_data_valid_bit;
    int src_tag;
    int src_value;
    int rob_index;
}LSQ;

typedef struct REG
{
    int valid;
    int value;
}REG;

typedef struct ARF
{
    int r[16];
    int cc;
    int commited_instr_address;
}ARF;

typedef struct PRF
{
    struct REG pr;
    struct REG cc;
}PRF;
typedef struct bus
{
    int valid;
    int tag;
    int data;
    int tag_broadcasted;
    int data_broadcasted;
}bus;

#define BTB_SIZE 8
#define IQ_SIZE 24
#define BQ_SIZE 16
#define LSQ_SIZE 16
#define ROB_SIZE 32
#define AR_SIZE 25
#define Rename_Table_SIZE 17
#define Free_List_SIZE 25
#define CC_PSize 16

static struct BTBEntry btb[BTB_SIZE];
static struct BQ bq[BQ_SIZE];
static  int rename_table[Rename_Table_SIZE];
static struct PRF physical_reg_file[Free_List_SIZE];
static struct REG physical_reg[Free_List_SIZE];
static struct REG cc_reg[CC_PSize];
static int reg_free_list[Free_List_SIZE];
static int cc_free_list[CC_PSize];
static IQ issue_queue[IQ_SIZE];
static int dispatch_counter =0;
static int ready_for_intFU_issue = -1;
static int ready_for_mulFU_issue = -1;
static int ready_for_afu_issue = -1;
static struct bus forwarding_bus[100];
static struct bus cc_forwarding_bus[100];
static struct PRF prf_file[25];
static struct ROB rob[ROB_SIZE];
static struct LSQ lsq[LSQ_SIZE];
static int rob_head = 0;
static int rob_tail = 0;
static int prev;
static int prev_cc = -1;
static int free_physical_reg_index;
static int free_cc_physical_reg_index;
static struct ARF arf;
static int mul_counter = 0;
static int mau_counter = 0;
static int stop_simulator = FALSE;
static int lsq_tail =0;
static int lsq_head = 0;
static int rename_head = 0;
static int rename_tail = Free_List_SIZE -1;
static int cc_rename_tail = Free_List_SIZE -1;
static int ready_for_bfu_issue = -1;

APEX_Instruction *create_code_memory(const char *filename, int *size);
APEX_CPU *APEX_cpu_init(const char *filename);
void APEX_cpu_run(APEX_CPU *cpu, int command);
void APEX_cpu_stop(APEX_CPU *cpu);
void set_condition_codes(APEX_CPU *cpu);
void branch_instruction(APEX_CPU *cpu);
void score_boarding(APEX_CPU *cpu);
void data_forwarding(APEX_CPU *cpu);
void check_forwarding_for_LOADP_and_STOREP(APEX_CPU *cpu);
void init_btb();
int predict_branch(APEX_CPU *cpu);
void update_btb_entry(APEX_CPU *cpu, char pred);
void create_btb_entry(APEX_CPU *cpu);
int is_btb_hit(APEX_CPU *cpu);
void branch_updation(APEX_CPU *cpu, char actual_decision);
int check_forwarding_eligible(int opcode);
int check_rs2(int opcode);
int check_for_LOADP_STOREP_stall(APEX_CPU* cpu);
void register_renaming(APEX_CPU *cpu);
void update_rename_table_entry(APEX_CPU* cpu, int physical_reg);
int get_free_pr_index();
int get_free_cc_index();
void register_renaming(APEX_CPU *cpu);
void create_iq_entry(APEX_CPU *cpu, char* fu_type, int physical_reg);
void create_rob_entry(APEX_CPU* cpu);
void wakeup_iq(APEX_CPU *cpu);
void rob_commit();
void pull_value_from_bus();
void create_lsq_entry(APEX_CPU* cpu, char* lsq_type);
void create_bq_entry(APEX_CPU *cpu);
#endif
