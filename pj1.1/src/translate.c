/* This project is based on the MIPS Assembler of CS61C in UC Berkeley.
   The framework of this project is been modified to be suitable for RISC-V
   in CS110 course in ShanghaiTech University by Zhijie Yang in March 2019.
   Updated by Chibin Zhang and Zhe Ye in March 2021.
*/

#include "translate.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "block.h"
#include "tables.h"
#include "translate_utils.h"


static const PseudoHandler pseudo_handlers[] = {
    {"beqz", transform_beqz}, {"bnez", transform_bnez}, {"li", transform_li},
    {"mv", transform_mv},     {"j", transform_j},       {"jr", transform_jr},
    {"jal", transform_jal},   {"jalr", transform_jalr}, {"lw", transform_lw},
    {"la", transform_la},
};


/* 
Fields per entry:
  - const char* name;         -- instr name
  - InstrType instr_type;     -- instr format, e.g. R_type
  - uint8_t opcode;
  - uint8_t funct3;
  - uint8_t funct7;           -- funct7 or partial imm
  - ImmType imm_type;         -- imm type (see translate_utils.h)
*/

static const InstrInfo instr_table[] = {
    // R-type instructions
    {"add", R_TYPE, 0x33, 0x0, 0x00, IMM_NONE},

    {"sub", R_TYPE, 0x33, 0x0, 0x20, IMM_NONE},
    {"xor", R_TYPE, 0x33, 0x4, 0x00, IMM_NONE},
    {"or", R_TYPE, 0x33, 0x6, 0x00, IMM_NONE},
    {"and", R_TYPE, 0x33, 0x7, 0x00, IMM_NONE},
    {"sll", R_TYPE, 0x33, 0x1, 0x00, IMM_NONE},
    {"srl", R_TYPE, 0x33, 0x5, 0x00, IMM_NONE},
    {"sra", R_TYPE, 0x33, 0x5, 0x20, IMM_NONE},
    {"slt", R_TYPE, 0x33, 0x2, 0x00, IMM_NONE},
    {"sltu", R_TYPE, 0x33, 0x3, 0x00, IMM_NONE},
    {"mul", R_TYPE, 0x33, 0x0, 0x01, IMM_NONE},
    {"mulh", R_TYPE, 0x33, 0x1, 0x01, IMM_NONE},
    {"div", R_TYPE, 0x33, 0x4, 0x01, IMM_NONE},
    {"rem", R_TYPE, 0x33, 0x6, 0x01, IMM_NONE},

    // I-type instructions
    {"addi", I_TYPE, 0x13, 0x0, 0x00, IMM_12_SIGNED},
    {"xori", I_TYPE, 0x13, 0x4, 0x00, IMM_12_SIGNED},
    {"ori", I_TYPE, 0x13, 0x6, 0x00, IMM_12_SIGNED},
    {"andi", I_TYPE, 0x13, 0x7, 0x00, IMM_12_SIGNED},
    {"slli", I_TYPE, 0x13, 0x1, 0x00, IMM_5_UNSIGNED},
    {"srli", I_TYPE, 0x13, 0x5, 0x00, IMM_5_UNSIGNED},
    {"srai", I_TYPE, 0x13, 0x5, 0x20, IMM_5_UNSIGNED},
    {"slti", I_TYPE, 0x13, 0x2, 0x00, IMM_12_SIGNED},
    {"sltiu", I_TYPE, 0x13, 0x3, 0x00, IMM_12_SIGNED},
    {"lb", I_TYPE, 0x03, 0x0, 0x00, IMM_12_SIGNED},
    {"lh", I_TYPE, 0x03, 0x1, 0x00, IMM_12_SIGNED},
    {"lw", I_TYPE, 0x03, 0x2, 0x00, IMM_12_SIGNED},
    {"lbu", I_TYPE, 0x03, 0x4, 0x00, IMM_12_SIGNED},
    {"lhu", I_TYPE, 0x03, 0x5, 0x00, IMM_12_SIGNED},
    {"jalr", I_TYPE, 0x67, 0x0, 0x00, IMM_12_SIGNED},
    {"ecall", I_TYPE, 0x73, 0x0, 0x00, IMM_NONE},

    // S-type instructions
    {"sb", S_TYPE, 0x23, 0x0, 0x00, IMM_12_SIGNED},
    {"sh", S_TYPE, 0x23, 0x1, 0x00, IMM_12_SIGNED},
    {"sw", S_TYPE, 0x23, 0x2, 0x00, IMM_12_SIGNED},

    // SB-type instructions
    {"beq", SB_TYPE, 0x63, 0x0, 0x00, IMM_13_SIGNED},
    {"bne", SB_TYPE, 0x63, 0x1, 0x00, IMM_13_SIGNED},
    {"blt", SB_TYPE, 0x63, 0x4, 0x00, IMM_13_SIGNED},
    {"bge", SB_TYPE, 0x63, 0x5, 0x00, IMM_13_SIGNED},
    {"bltu", SB_TYPE, 0x63, 0x6, 0x00, IMM_13_SIGNED},
    {"bgeu", SB_TYPE, 0x63, 0x7, 0x00, IMM_13_SIGNED},

    // U-type instructions
    {"lui", U_TYPE, 0x37, 0x0, 0x00, IMM_20_UNSIGNED},
    {"auipc", U_TYPE, 0x17, 0x0, 0x00, IMM_20_UNSIGNED},

    // UJ-type instructions
    {"jal", UJ_TYPE, 0x6f, 0x0, 0x00, IMM_21_SIGNED},
};
// the return value means the step number of the pseudo instruction transformed, 0 if failed
unsigned transform_beqz(Block* blk, char** args, int num_args, uint32_t current_line, uint32_t offset) {
  //TODO
  /* === start === */
  if (num_args != 2) return 0;
  if (translate_reg(args[0]) == -1) return 0;
 
  char* beq_args[3];
  beq_args[0] = args[0];
  beq_args[1] = "zero";
  beq_args[2] = args[1];

  if (add_to_block(blk, "beq", beq_args, 3, current_line, offset) != 0) return 0;
  /* === end === */
  return 1;
}

unsigned transform_bnez(Block* blk, char** args, int num_args, uint32_t current_line, uint32_t offset) {
  //TODO
  /* === start === */
  if (num_args != 2) return 0;
  if (translate_reg(args[0]) == -1) return 0;
 
  char* bne_args[3];
  bne_args[0] = args[0];
  bne_args[1] = "zero";
  bne_args[2] = args[1];

  if (add_to_block(blk, "bne", bne_args, 3, current_line, offset) != 0) return 0;
  /* === end === */
  return 1;
}

/* Hint:
  - make sure that the number is representable by 32 bits. (Hint: the number
      can be both signed or unsigned).
  - if the immediate can fit in the imm field of an addiu instruction, then
      expand li into a single addi instruction. Otherwise, expand it into
      a lui-addi pair.

  venus has slightly different translation rules for li, and it allows numbers
  larger than the largest 32 bit number to be loaded with li. You should follow
  the above rules if venus behaves differently.
*/
unsigned transform_li(Block* blk, char** args, int num_args, uint32_t current_line, uint32_t offset) {
  //TODO
  /* === start === */
  if (num_args != 2) return 0;
  if (translate_reg(args[0]) == -1) return 0;
 
  long imm;
  int is_not_success = translate_num(&imm, args[1], IMM_12_SIGNED);
  if (is_not_success == 0){
    char* addi_args[3];
    addi_args[0] = args[0];
    addi_args[1] = "zero";
    addi_args[2] = args[1];
    if (add_to_block(blk, "addi", addi_args, 3, current_line, offset) != 0) return 0;
    return 1;
  }
  else if (is_not_success == 1){

    long upper = (imm + 0x800) >> 12;
    char upper_str[32];
    sprintf(upper_str, "%ld", upper);
    char* lui_args[2];
    lui_args[0] = args[0];
    lui_args[1] = upper_str;
    if (add_to_block(blk, "lui", lui_args, 2, current_line, offset) != 0) return 0;
    
    long lower = imm & 0xFFF;
    char lower_str[32];
    sprintf(lower_str, "%ld", lower);
    char* addi_args[3];
    addi_args[0] = args[0];
    addi_args[1] = args[0];
    addi_args[2] = lower_str;
    if (add_to_block(blk, "addi", addi_args, 3, current_line, offset+4) != 0) return 0;
    return 2;
  }
  /* === end === */
  return 0;
}

/* Hint:
  - your expansion should use the fewest number of instructions possible.
 */
unsigned transform_mv(Block* blk, char** args, int num_args, uint32_t current_line, uint32_t offset) {
  //TODO
  /* === start === */
  if (num_args != 2) return 0;
  if (translate_reg(args[0]) == -1 || translate_reg(args[1]) == -1) return 0;
 
  char* addi_args[3];
  addi_args[0] = args[0];
  addi_args[1] = args[1];
  addi_args[2] = "0";
 
  if (add_to_block(blk, "addi", addi_args, 3, current_line, offset) != 0) return 0;
  /* === end === */
  return 1;
}

unsigned transform_j(Block* blk, char** args, int num_args, uint32_t current_line, uint32_t offset) {
  //TODO
  /* === start === */
  if (num_args != 1) return 0;
 
  char* jal_args[2];
  jal_args[0] = "zero";
  jal_args[1] = args[0];
 
  if (add_to_block(blk, "jal", jal_args, 2, current_line, offset) != 0) return 0;
  /* === end === */
  return 1;
}

unsigned transform_jr(Block* blk, char** args, int num_args, uint32_t current_line,  uint32_t offset) {
  //TODO
  /* === start === */
  if (num_args != 1) return 0;
  if (translate_reg(args[0]) == -1) return 0;
 
  char* jalr_args[3];
  jalr_args[0] = "zero";
  jalr_args[1] = args[0];
  jalr_args[2] = "0";
 
  if (add_to_block(blk, "jalr", jalr_args, 3, current_line, offset) != 0) return 0;
  /* === end === */
  return 1;
}

/* Hint:
  - Since handler is selected by instruction name, be careful about
    pseudo/regular instruction name collisions
 */
unsigned transform_jal(Block* blk, char** args, int num_args, uint32_t current_line, uint32_t offset) {
  //TODO
  /* === start === */
  if (num_args != 1 && num_args != 2) return 0;

  if (num_args == 1) {
    char* jal_args[2];
    jal_args[0] = "ra";
    jal_args[1] = args[0]; 
    if (add_to_block(blk, "jal", jal_args, 2, current_line, offset) != 0) return 0;
  } 
  else if (num_args == 2) {
    if (translate_reg(args[0]) == -1) return 0;
    if (add_to_block(blk, "jal", args, 2, current_line, offset) != 0) return 0;
  }
  /* === end === */
  return 1;
}

/* Hint:
  - Since handler is selected by instruction name, be careful about
    pseudo/regular instruction name collisions
 */
unsigned transform_jalr(Block* blk, char** args, int num_args, uint32_t current_line, uint32_t offset) {
  //TODO
  /* === start === */
  if (num_args != 1 && num_args != 3) return 0;
  if (num_args == 1) {
    if (translate_reg(args[0]) == -1) return 0;
 
    char* jalr_args[3];
    jalr_args[0] = args[0];
    jalr_args[1] = args[0];
    jalr_args[2] = "0";
 
    if (add_to_block(blk, "jalr", jalr_args, 3, current_line, offset) != 0) return 0;
  } else if (num_args == 3) {
    if (translate_reg(args[0]) == -1 || translate_reg(args[1]) == -1) return 0;
    if (add_to_block(blk, "jalr", args, 3, current_line, offset) != 0) return 0;
  } 
  /* === end === */
  return 1;
}

/* Hint:
 * - You should leave the label AS IS and resolve it in pass 2.
 */
unsigned transform_lw(Block* blk, char** args, int num_args, uint32_t current_line, uint32_t offset) {
  //TODO
  /* === start === */
  if (num_args == 3) {
    if (translate_reg(args[0]) == -1 || translate_reg(args[2]) == -1) return 0;
    if (add_to_block(blk, "lw", args, 3, current_line, offset) != 0) return 0;
    return 1;
  }
  else if (num_args == 2) {
    if (translate_reg(args[0]) == -1) return 0;
    if (translate_reg(args[1]) == -1){
      char* auipc_args[2];
      auipc_args[0] = args[0];
      auipc_args[1] = args[1];
      if (add_to_block(blk, "auipc", auipc_args, 2, current_line, offset) != 0) return 0;
      
      char* lw_args[3];
      lw_args[0] = args[0];
      lw_args[1] = args[1];
      lw_args[2] = args[0];
      if (is_valid_label(args[1])){
        lw_args[1] = strcat(args[1], "^");
      }
      if (add_to_block(blk, "lw", lw_args, 3, current_line, offset+4) != 0) return 0;
      return 2;
      }
    char* lw_args[3];
    lw_args[0] = args[0];
    lw_args[1] = "0";
    lw_args[2] = args[1];

    if (add_to_block(blk, "lw", lw_args, 3, current_line, offset) != 0) return 0;
    return 1;
  }
  return 0;
  /* === end === */
  
}

unsigned transform_la(Block* blk, char** args, int num_args, uint32_t current_line, uint32_t offset) {
  //TODO
  /* === start === */
  // it also have a aupic+addi version, demn!
  if (num_args != 2) return 0; // rd imm
  if (translate_reg(args[0]) == -1) return 0; 

  //printf("y6yy %s\n", args[1]);

  char* auipc_args[2];
  auipc_args[0] = args[0];
  auipc_args[1] = args[1];
 
  if (add_to_block(blk, "auipc", auipc_args, 2, current_line, offset) != 0){
    return 0;
  }
  
  char* addi_args[3];
  addi_args[0] = args[0];
  addi_args[1] = args[0];
  addi_args[2] = strcat(args[1], "$");
 
  if (add_to_block(blk, "addi", addi_args, 3, current_line, offset + 4) != 0) return 0;
  /* === end === */
  return 2;
}

/* Traverse pseudo_handlers table to select corresponding handler by NAME */
const PseudoHandler* find_pseudo_handler(const char* name) {
  for (size_t i = 0; i < sizeof(pseudo_handlers) / sizeof(pseudo_handlers[0]);
       i++) {
    if (strcmp(name, pseudo_handlers[i].name) == 0) {
      return &pseudo_handlers[i];
    }
  }
  return NULL;
}

/* Writes instructions during the assembler's first pass to BLK. The case
   for pseudo-instructions will be handled by handlers, but you need to
   write code to complete these TRANSFORM functions, as well as dealing
   with general instructions. Your pseudoinstruction expansions should not
   have any side effects.

   BLK is the intermediate instruction block you should write to,
   NAME is the name of the instruction, ARGS is an array of the arguments, and
   NUM_ARGS specifies the number of items in ARGS.

   Error checking for regular instructions are done in pass two. However, for
   pseudoinstructions, you must make sure that ARGS contains the correct number
   of arguments. You do NOT need to check whether the registers / label are
   valid, since that will be checked in part two.

   Returns the number of instructions written (so 0 if there were any errors).
 */
unsigned write_pass_one(Block* blk, const char* name, char** args,
  int num_args, uint32_t current_line, uint32_t offset) {
  /* Deal with pseudo-instructions */
  const PseudoHandler* handler = find_pseudo_handler(name);
  if (handler) {
    return handler->transform(blk, args, num_args, current_line, offset);
  }
  /* What about general instructions? */
  //TODO: 
  /* === start === */
  if (add_to_block(blk, name, args, num_args, current_line, offset) != 0) return 0;
  /* === end === */
  return 1;
}

/* Writes the instruction in hexadecimal format to OUTPUT during pass #2.

   NAME is the name of the instruction, ARGS is an array of the arguments, and
   NUM_ARGS specifies the number of items in ARGS.

   The symbol table (SYMTBL) is given for any symbols that need to be resolved
   at this step.

   You must perform error checking on all instructions and make sure that their
   arguments are valid. If an instruction is invalid, you should not write
   anything to OUTPUT but simply return -1. venus may be a useful resource for
   this step.

   All regular instruction information is given at `instr_table`.

   Note the use of helper functions. Consider writing your own! If the function
   definition comes afterwards, you must declare it first (see translate.h).

   Returns 0 on success and -1 on error.
 */

int translate_inst(FILE* output, const char* name, char** args, size_t num_args,
                   uint32_t addr, SymbolTable* symtbl) {
  for (size_t i = 0; i < sizeof(instr_table) / sizeof(instr_table[0]); i++) {
    const InstrInfo* info = &instr_table[i];
    if (strcmp(name, info->name) == 0) {
      switch (info->instr_type) {
        case R_TYPE:
          return write_rtype(output, info, args, num_args);
        case I_TYPE:
          return write_itype(output, info, args, num_args, addr, symtbl);
        case S_TYPE:
          return write_stype(output, info, args, num_args);
        case SB_TYPE:
          return write_sbtype(output, info, args, num_args, addr, symtbl);
        case U_TYPE:
          return write_utype(output, info, args, num_args, addr, symtbl);
        case UJ_TYPE:
          return write_ujtype(output, info, args, num_args, addr, symtbl);
      }
    }
  }
  return -1;
}


/* A helper function for writing most R-type instructions. You should use
   translate_reg() to parse registers and write_inst_hex() to write to
   OUTPUT. Both are defined in translate_utils.h.

   This function is INCOMPLETE. Complete the implementation below. You will
   find bitwise operations to be the cleanest way to complete this function.
 */
int write_rtype(FILE* output, const InstrInfo* info, char** args,
                size_t num_args) {
  //TODO
  /* === start === */
  if (num_args != 3) return -1;
  int rd = translate_reg(args[0]);
  int rs1 = translate_reg(args[1]);
  int rs2 = translate_reg(args[2]);
  if (rd == -1 || rs1 == -1 || rs2 == -1) return -1;

  uint32_t instr = 0;
  instr |= ((uint32_t)info->funct7 << 25);
  instr |= ((uint32_t)rs2 << 20);
  instr |= ((uint32_t)rs1 << 15);
  instr |= ((uint32_t)info->funct3 << 12);
  instr |= ((uint32_t)rd << 7);
  instr |= info->opcode;
  write_inst_hex(output, instr);
  /* === end === */
  return 0;
}

/* Hint:
  - Number of ARGS and immediate range of each I_type instruction may
  vary. Refer to RISC-V green card and design proper encoding rules.
  - Some instruction(s) expanded from pseudo ones may has(have) unresolved
  label(s). You need to take that special case into consideration. Refer to
  write_sbtype for detailed relative address calculation.
 */
int write_itype(FILE *output, const InstrInfo *info, char **args,
                size_t num_args, uint32_t offset, SymbolTable *symtbl)
{
  //TODO
  /* === start === */
  if (!strcmp(info->name, "ecall")) {
    if (num_args != 0) return -1;
    uint32_t instr = 0;
    instr |= ((uint32_t)info->funct3 << 12);
    instr |= info->opcode;
    write_inst_hex(output, instr);
    return 0;
  }

  if (num_args != 3) return -1;

  int rd, rs1;
  char *imm_str;

  if (info->opcode == 0x03) {   // Load: rd, imm, rs1
    rd = translate_reg(args[0]);
    rs1 = translate_reg(args[2]);
    imm_str = args[1];
  }
  else {   // Others: rd, rs1, imm
    rd = translate_reg(args[0]);
    rs1 = translate_reg(args[1]);
    imm_str = args[2];
  }

  if (rd == -1 || rs1 == -1) return -1;
  long imm;
  int is$ = 0; // check if addi is called by auipc+addi combination
  int is$6 = 0; // check if lw is called by auipc+lw combination
  if (imm_str[strlen(imm_str) - 1] == '$'){
    is$ = 1;
    imm_str[strlen(imm_str) - 1] = '\0';
  }
  if (imm_str[strlen(imm_str) - 1] == '^'){
    is$6 = 1;
    imm_str[strlen(imm_str) - 1] = '\0';
  }

  if (is_valid_label(imm_str)) { // imm is label
    int64_t label_offset = get_offset_for_symbol(symtbl, imm_str);
    
    if (label_offset == -1) return -1;
    if (is$) {
      label_offset += 0x4;
      if(label_offset > 0xFFFFFFF) label_offset -= 0x10000000; 
    }
    if (is$6){
      label_offset += 0x4;
      if (label_offset > 0xFFFFFFF) label_offset -= 0x10000000;
    }
    imm = label_offset - offset; // offset = PC
    
    if (!is_valid_imm(imm, info->imm_type)){
      return -1;
    }
  } 
  else {
    int result = translate_num(&imm, imm_str, info->imm_type);
    if (result != 0) return -1;
    if (!is_valid_imm(imm, info->imm_type)) return -1;
    if (!strcmp(info->name, "srai")) imm += 0x400; // srai is special case, magic!
  }
  
  uint32_t instr = 0;
  instr |= ((uint32_t)(imm & 0xFFF) << 20);
  instr |= ((uint32_t)rs1 << 15);
  instr |= ((uint32_t)info->funct3 << 12);
  instr |= ((uint32_t)rd << 7);
  instr |= info->opcode;
  write_inst_hex(output, instr);
  /* === end === */
  return 0;
}

int write_stype(FILE *output, const InstrInfo *info, char **args,
                size_t num_args)
{
  //TODO
  /* === start === */
  if (num_args != 3) return -1;
  int rs2 = translate_reg(args[0]);
  int rs1 = translate_reg(args[2]);
  if (rs2 == -1 || rs1 == -1) return -1;

  long imm;
  int result = translate_num(&imm, args[1], info->imm_type);
  if (result != 0) return -1;
  if (!is_valid_imm(imm, info->imm_type)) return -1;

  uint32_t instr = 0;
  instr |= ((uint32_t)((imm >> 5) & 0x7F) << 25);
  instr |= ((uint32_t)rs2 << 20);
  instr |= ((uint32_t)rs1 << 15);
  instr |= ((uint32_t)info->funct3 << 12);
  instr |= ((uint32_t)(imm & 0x1F) << 7);
  instr |= info->opcode;
  write_inst_hex(output, instr);
  /* === end === */
  return 0;
}

/* Hint:
  - the way for branch to calculate relative address. e.g. bne
     bne rs rt label
   assume the byte_offset(addr) of label is L,
   current instruction byte_offset(addr) is A
   the relative address I for label satisfy:
     L = A + I
   so the relative addres is
     I = L - A
*/
int write_sbtype(FILE* output, const InstrInfo* info, char** args,
                 size_t num_args, uint32_t offset, SymbolTable* symtbl) {
  //TODO
  /* === start === */
  if (num_args != 3) return -1; //rs1, rs2, imm
  int rs1 = translate_reg(args[0]);
  int rs2 = translate_reg(args[1]);
  if (rs1 == -1 || rs2 == -1) return -1;

  long imm;
  if (is_valid_label(args[2])){
    const char* label = args[2];
    int64_t label_offset = get_offset_for_symbol(symtbl, label);
    if (label_offset == -1) return -1;
    imm = label_offset - offset; // offset = PC
  }
  else {
    int result = translate_num(&imm, args[2], info->imm_type);
    if (result != 0) return -1;
    if (!is_valid_imm(imm, info->imm_type)) return -1;
  }

  uint32_t instr = 0;
  instr |= ((uint32_t)((imm >> 12) & 0x1) << 31);
  instr |= ((uint32_t)((imm >> 5) & 0x3F) << 25);
  instr |= ((uint32_t)rs2 << 20);
  instr |= ((uint32_t)rs1 << 15);
  instr |= ((uint32_t)info->funct3 << 12);
  instr |= ((uint32_t)((imm >> 1) & 0xF) << 8);
  instr |= ((uint32_t)((imm >> 11) & 0x1) << 7);
  instr |= info->opcode;
  write_inst_hex(output, instr);
  return 0;

  /* === end === */
  return 0;
}

/* Hint:
  - Some instruction(s) expanded from pseudo ones may has(have) unresolved
  label(s). You need to take that special case into consideration. Refer to
  write_sbtype for detailed relative address calculation.
 */
int write_utype(FILE* output, const InstrInfo* info, char** args,
                size_t num_args, uint32_t offset, SymbolTable* symtbl) {
  //TODO
  /* === start === */
  if (num_args != 2) return -1; // rd, imm/label
  int rd = translate_reg(args[0]);
  if (rd == -1) return -1;
                  
  long imm;
  char* imm_str = args[1];

  if (is_valid_label(imm_str)) { //label

    int64_t label_offset = get_offset_for_symbol(symtbl, imm_str);
    
    /*//print symtbl for debug
    for (uint32_t i = 0; i < symtbl->len; i++) {
      printf("%s %u\n", symtbl->symbols[i].name, symtbl->symbols[i].offset);
    }
    printf("yyy %s %ld\n", imm_str, label_offset);*/

    if (label_offset == -1) return -1;
    if (!strcmp(info->name, "luipc")) imm = (label_offset - offset) >> 12; // offset = PC
    else imm = label_offset >> 12; //lui
    
  }
  else {

    int result = translate_num(&imm, imm_str, info->imm_type);
    if (result != 0) return -1;
    if (!is_valid_imm(imm, info->imm_type)) return -1;
  }
  
  uint32_t instr = 0;
  instr |= ((uint32_t)(imm & 0xFFFFF) << 12);
  instr |= ((uint32_t)rd << 7);
  instr |= info->opcode;
  write_inst_hex(output, instr);
  /* === end === */
  return 0;
}

/* In this project there is no need to relocate labels,
   you may think about the reasons. */
int write_ujtype(FILE* output, const InstrInfo* info, char** args,
                 size_t num_args, uint32_t offset, SymbolTable* symtbl) {
  //TODO
  /* === start === */
  if (num_args != 2) return -1; // rd, imm
  int rd = translate_reg(args[0]);
  if (rd == -1) return -1;

  long imm;
  const char* label = args[1];
  if (is_valid_label(label)) {
    int64_t label_offset = get_offset_for_symbol(symtbl, label);
    if (label_offset == -1) return -1;
    imm = label_offset - offset; // offset = PC
  }
  else {
    int result = translate_num(&imm, args[1], info->imm_type);
    if (result != 0) return -1;
    if (!is_valid_imm(imm, info->imm_type)) return -1;
  }

  uint32_t instr = 0;
  instr |= ((uint32_t)((imm >> 20) & 0x1) << 31);
  instr |= ((uint32_t)((imm >> 1) & 0x3FF) << 21);
  instr |= ((uint32_t)((imm >> 11) & 0x1) << 20);
  instr |= ((uint32_t)((imm >> 12) & 0xFF) << 12);
  instr |= ((uint32_t)rd << 7);
  instr |= info->opcode;
  write_inst_hex(output, instr);
  /* === end === */
  return 0;
}