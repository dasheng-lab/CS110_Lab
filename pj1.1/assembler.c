/* This project is based on the MIPS Assembler of CS61C in UC Berkeley.
   The framework of this project is been modified to be suitable for RISC-V
   in CS110 course in ShanghaiTech University by Zhijie Yang in March 2019.
   Updated by Chibin Zhang and Zhe Ye in March 2021， updated by MaoXi Ma in 
   March 2026.
*/

#include "assembler.h"

#include <getopt.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/block.h"
#include "src/tables.h"
#include "src/translate.h"
#include "src/translate_utils.h"
#include "src/utils.h"

#define MAX_ARGS 3
#define BUF_SIZE 1024
#define MAX_PATH_LENGTH 512
#define DATA_BASE_ADDR 0x10000000u
const char *IGNORE_CHARS = " \f\n\r\t\v,()";

static int data_image_reserve(DataImage *image, size_t additional);
static int add_if_label(uint32_t input_line, char* str, uint32_t byte_offset,
                        SymbolTable* symtbl);

static int write_data_output(FILE *data_file, const DataImage *data_image)
{
  size_t i;
  if (!data_file || !data_image)
  {
    return -1;
  }
  for (i = 0; i < data_image->len; i++)
  {
    fprintf(data_file, "0x%08x 0x%02x\n", (unsigned int)(DATA_BASE_ADDR + i),
            (unsigned int)data_image->bytes[i]);
  }
  return 0;
}

static void data_image_init(DataImage *image)
{
  image->bytes = NULL;
  image->len = 0;
  image->cap = 0;
}

static void data_image_free(DataImage *image)
{
  if (!image)
  {
    return;
  }
  free(image->bytes);
  image->bytes = NULL;
  image->len = 0;
  image->cap = 0;
}
/*******************************
 * Helper Functions
 *******************************/

/* you should  be calling these functions yourself. */
static void raise_label_error(uint32_t input_line, const char *label)
{
  write_to_log("Error - invalid label at line %d: %s\n", input_line, label);
}

/* call this function if more than MAX_ARGS arguments are found while parsing
   arguments.

   INPUT_LINE is which line of the input file that the error occurred in. Note
   that the first line is line 1 and that empty lines are included in the count.

   EXTRA_ARG should contain the first extra argument encountered.
 */
static void raise_extra_argument_error(uint32_t input_line,
                                       const char *extra_arg)
{
  write_to_log("Error - extra argument at line %d: %s\n", input_line, extra_arg);
}

/* You should call this function if write_pass_one() or translate_inst()
   returns -1.
   INPUT_LINE is which line of the input file that the error occurred in. Note
   that the first line is line 1 and that empty lines are included in the count.
 */
static void raise_instruction_error(uint32_t input_line, const char *name, char **args, int num_args)
{
  write_to_log("Error - invalid instruction at line %d: ", input_line);
  log_inst(name, args, num_args);
}


/*.data  rules
1. Illegal label: When the label address is not a multiple of 4, call addr_alignment_incorrect(), 
but still store the label and the corresponding address in the block for pass_two to parse.
2. Missing values (such as .space without parameters, .byte/.word without any data): Call raise_miss_value(input_line, directive).
3. Invalid/Out-of-Bound values: Call raise_invalid_value(input_line, directive, token). 
You need to carefully consider the numerical range of .word and .byte, as they exist in binary form in memory.
However, when inputting, they can be signed or unsigned numbers, for example, .byte 0xff is 0xff in memory, 
.byte -1 is also 0xff in memory, but .byte 256 will be an out-of-bounds value.
Unsupported data instructions: Call raise_unsupported_directive(input_line, directive).
Too many .space parameters: Call raise_data_error(input_line, "too many arguments for .space", NULL), 
and at this time, no space will be allocated in memory for .space.
Memory allocation failure (expansion/write failure): Call raise_data_error(input_line, "allocation failed", NULL).
*/
static void raise_data_error(uint32_t input_line, const char *msg, const char *token)
{
  if (token)
  {
    write_to_log("Error - invalid data directive at line %d: %s (%s)\n", input_line, msg, token);
  }
  else
  {
    write_to_log("Error - invalid data directive at line %d: %s\n", input_line, msg);
  }
}
static void raise_invalid_value(uint32_t input_line, const char *directive, const char *token)
{
  char msg[64];
  snprintf(msg, sizeof(msg), "invalid %s value", directive);
  raise_data_error(input_line, msg, token);
}

static void raise_miss_value(uint32_t input_line, const char *directive)
{
  char msg[64];
  snprintf(msg, sizeof(msg), "missing %s values", directive);
  raise_data_error(input_line, msg, NULL);
}

static void raise_unsupported_directive(uint32_t input_line, const char *directive)
{
  raise_data_error(input_line, "unsupported directive", directive);
}

/*Update the current assembler's section*/
static int update_section_from_token(const char *token, SectionType *section)
{
  if (!token || !section)
  {
    return 0;
  }
  if (strcmp(token, ".text") == 0)
  {
    *section = SECTION_TEXT;
    return 1;
  }
  if (strcmp(token, ".data") == 0)
  {
    *section = SECTION_DATA;
    return 1;
  }
  return 0;
}

/* Truncates the string at the first occurrence of the '#' character. */
static void skip_comments(char *str)
{
  char *comment_start = strchr(str, '#');
  if (comment_start)
  {
    *comment_start = '\0';
  }
}
/*append a byte to the end of the dynamic byte array of DataImage.*/
static int data_image_push_byte(DataImage *image, uint8_t value)
{
  if (data_image_reserve(image, 1) != 0)
  {
    return -1;
  }
  image->bytes[image->len] = value;
  image->len++;
  return 0;
}

/*******************************
 * Implement the Following
 *******************************/
/**
 * @brief Ensure that DataImage has enough capacity for `additional` extra bytes.
 *
 * This function checks whether the current capacity is sufficient for appending
 * extra data. If not, it automatically expands the buffer using exponential growth
 * (doubling each time) to improve allocation efficiency.
 *
 * @param image Pointer to the DataImage structure, must not be NULL.
 * @param additional Number of extra bytes that must be accommodated.
 * @return Returns 0 on success, -1 on failure.
 * @retval 0 Capacity is already sufficient or expansion succeeds.
 * @retval -1 Invalid argument or memory allocation failure.
 *
 * @note Expansion strategy:
 *   - If initial capacity is 0, allocate 64 bytes on first allocation.
 *   - Then keep doubling capacity until it satisfies the required size.
 */
static int data_image_reserve(DataImage *image, size_t additional)
{
  //TODO
  if (!image) return -1;
  if (image->len + additional <= image->cap) return 0;
  size_t new_cap = image->cap * 2;
  if (image->cap == 0) new_cap = 64;
  while (new_cap < image->len + additional) new_cap *= 2;
  image->cap = new_cap;
  image->bytes = realloc(image->bytes, image->cap * 8);
  if (!image->bytes) return -1;
  return 0;
}

/**
 * @brief Parse a string into a long integer.
 *
 * `strtol()` can be useful.
 *
 * @param token The string to parse, must not be NULL.
 * @param out Output parameter used to store the parsed result, must not be NULL.
 * @return Returns 0 on success, -1 on failure.
 * @retval 0 Success, and `*out` contains the parsed result.
 * @retval -1 Failure (token is NULL, out is NULL, or the string contains invalid characters).
 */
static int parse_data_number(const char *token, long *out)
{
  //TODO
  char* end;
  if (!token || !out) return -1;
  *out = strtol(token, &end, 0);
  if (*end != '\0') return -1;
  return 0;
}



/**
 * @brief Parse data-segment directives and generate binary data.
 *
 * This function handles three RISC-V data directives: `.space`, `.byte`, and `.word`.
 * It parses directive arguments, writes the corresponding bytes into DataImage,
 * and updates the current data offset. It also performs error checks, including
 * argument validation and range checks.
 *
 * @param input_line Current input line number, used for error reporting.
 * @param directive Directive name string (e.g., ".space", ".byte", ".word").
 * @param rest_ctx Remaining parse context (unused in this implementation).
 * @param data_image Pointer to DataImage where generated binary data is stored.
 * @param data_offset Pointer to current data offset; updated after parsing.
 * @note Pay attention to valid argument ranges for `.byte` and `.word`.
 * @return Returns 0 on success, -1 on failure.
 */
static int parse_data_directive(uint32_t input_line, char *directive, char *rest_ctx, DataImage *data_image,
                                uint32_t *data_offset)
{
  //TODO
  if (!directive || !data_image || !data_offset){
    raise_data_error(input_line, "invalid directive", directive);
    return -1;
  }
  long num;
  char *token;
  if (!strcmp(directive, ".space")){
    token = strtok(rest_ctx, IGNORE_CHARS);
    if (!token) {
      raise_miss_value(input_line, directive);
      return -1;
    }
    if (parse_data_number(token, &num) != 0 || num < 0) {
      raise_invalid_value(input_line, directive, token);
      return -1;
    }
    token = strtok(NULL, IGNORE_CHARS);
    long initnum = 0;
    if (token && parse_data_number(token, &initnum) != 0) {
      raise_invalid_value(input_line, directive, token);
      return -1;
    }
    else if (token){
      token = strtok(NULL, IGNORE_CHARS);
      if (token) {
        raise_data_error(input_line, "too many arguments for .space", NULL);
        return -1;
      }
    }
    for (long i = 0; i < num; i++) {
      if (data_image_push_byte(data_image, initnum) != 0) {
        raise_data_error(input_line, "allocation failed", NULL);
        return -1;
      }
      (*data_offset)++;
    }
  } else if (!strcmp(directive, ".byte")) {
    token = strtok(rest_ctx, IGNORE_CHARS);
    if (!token) {
      raise_miss_value(input_line, directive);
      return -1;
    }
    while (token) {
      if (parse_data_number(token, &num) != 0) {
        raise_invalid_value(input_line, directive, token);
        return -1;
      }
      if (num < -128 || num > 255) {
        raise_invalid_value(input_line, directive, token);
        return -1;
      }
      if (data_image_push_byte(data_image, (uint8_t)num) != 0) {
        raise_data_error(input_line, "allocation failed", NULL);
        return -1;
      }
      (*data_offset)++;
      token = strtok(NULL, IGNORE_CHARS);
    }
    /*
    if (*data_offset % 4 != 0) { // memory alignment
      while (*data_offset % 4 != 0) {
        if (data_image_push_byte(data_image, 0) != 0) {
          raise_data_error(input_line, "allocation failed", NULL);
          return -1;
        }
        (*data_offset)++;
      }
    }*/
  } else if (!strcmp(directive, ".word")) {
    token = strtok(rest_ctx, IGNORE_CHARS);
    if (!token) {
      raise_miss_value(input_line, directive);
      return -1;
    }
    while (token) {
      if (parse_data_number(token, &num) != 0) {
        raise_invalid_value(input_line, directive, token);
        return -1;
      }
      else if (num < -0x80000000ll || 0xFFFFFFFF - num < 0) {
        raise_invalid_value(input_line, directive, token);
        return -1;
      }

      uint32_t word = (uint32_t)num;
      for (int i = 0; i < 4; i++) {
        if (data_image_push_byte(data_image, (uint8_t)(word & 0xFF)) != 0) {
          raise_data_error(input_line, "allocation failed", NULL);
          return -1;
        }
        word >>= 8;
        (*data_offset)++;
      }
      token = strtok(NULL, IGNORE_CHARS);
    }
  } else {
    raise_unsupported_directive(input_line, directive);
    return -1;
  }
  return 0;
}

/**
 * @brief Parse one line in the data section, handling labels and data directives.
 *
 * This function parses a single line in the data section, which may contain
 * a label definition and a data directive. 
 * If a label exists, it is added to the symbol table with the current data offset.
 * Then the data directive is parsed and written into DataImage.
 *
 * @param input_line Current line number, used for error reporting.
 * @param line Input line string to parse (will be modified).
 * @param table Symbol table used to store label definitions.
 * @param data_image Data image used to store generated data bytes.
 * @param data_offset Pointer to current data offset; updated after parsing.
 * @return Returns 0 on success, -1 on failure, and 0 for empty lines.
 */
static int parse_data_line(uint32_t input_line, char *line, SymbolTable *table,
                            DataImage *data_image, uint32_t *data_offset)
{
  //TODO
  if (!line || !table || !data_image || !data_offset) {
    raise_data_error(input_line, "invalid line", line);
    return -1;
  }

  int err = 0;
  char *token = strtok(line, IGNORE_CHARS);

  if (!token) return 0;
  //printf("offset1:%x\n", *data_offset);
  int label_result = add_if_label(input_line, token, *data_offset+DATA_BASE_ADDR, table);
  if (label_result == -1){
    token = strtok(NULL, IGNORE_CHARS);
    err = 1;
    if (!token) return -1;
  }
  else if (label_result == 1) {
    token = strtok(NULL, IGNORE_CHARS);
    if (!token) return 0;
  }

  if (token[0] != '.') {
    raise_data_error(input_line, "invalid directive", token);
    return -1;
  }
  
  char *directive = token;
  char *rest_ctx = token + strlen(directive) + 1;

  if (parse_data_directive(input_line, directive, rest_ctx, data_image, data_offset) != 0) {
    return -1;
  }
  if (err) return -1;
  return 0;

}



/* Reads STR and determines whether it is a label (ends in ':'), and if so,
   whether it is a valid label, and then tries to add it to the symbol table.

   INPUT_LINE is which line of the input file we are currently processing. Note
   that the first line is line 1 and that empty lines are included in this
   count.

   BYTE_OFFSET is the offset of the NEXT instruction (should it exist).

   Four scenarios can happen:
    1. STR is not a label (does not end in ':'). Returns 0.
    2. STR ends in ':', but is not a valid label. Returns -1.
    3a. STR ends in ':' and is a valid label. Addition to symbol table fails.
        Returns -1.
    3b. STR ends in ':' and is a valid label. Addition to symbol table succeeds.
        Returns 1.
 */
static int add_if_label(uint32_t input_line, char* str, uint32_t byte_offset,
                        SymbolTable* symtbl)
{
  // TODO
  if (!str || !symtbl) {
    raise_data_error(input_line, "invalid line", str);
    return -1;
  }
  if (str[strlen(str) - 1] != ':') {
    return 0;
  }
  str[strlen(str) - 1] = '\0'; // remove :
  if (!is_valid_label(str)) {
    raise_label_error(input_line, str);
    return -1;
  }
  else if (add_to_table(symtbl, str, byte_offset) != 0) {
    return -1;
  }
  return 1;
}

// all the instructions in RISC-V, I created this table to store them.
static const char* riscv_isa[] = {
    "add","sub","xor","or","and","sll","srl","sra","slt","sltu","mul","mulh","div","rem",
    "addi","xori","ori","andi","slli","srli","srai","slti","sltiu","lb","lh","lw","lbu","lhu",
    "sb","sh","sw","jalr","ecall","beq","bne","blt","bge","bltu","bgeu","lui","auipc","jal",
    "beqz","bnez","li","mv","j","jr","la"
};
// I create this function to parse text line.
// Return the number of instructions written to block, -1 on failure.
static int parse_text_line(Block* blk, uint32_t input_line, char *line, 
                           SymbolTable *table, uint32_t *text_offset)
{
  int ishasname = 0;
  char name[32];
  char* args[MAX_ARGS];
  int arg_num = 0;
  char *token = NULL;
  int err = 0;

  if (!line || !table || !blk || !text_offset) return -1;
  token = strtok(line, IGNORE_CHARS);
  if (!token) return 0; // whitespace in assembly
 
  int label_result = add_if_label(input_line, token, *text_offset, table);
  if (label_result == -1){
    token = strtok(NULL, IGNORE_CHARS);
    err = 1;
    if (!token) return -1;
  }
  else if (label_result == 1) {
    token = strtok(NULL, IGNORE_CHARS);
    if (!token) return 0;
  }

  for (size_t i = 0; i < sizeof(riscv_isa) / sizeof(riscv_isa[0]); i++) {
    if (!strcmp(riscv_isa[i], token)) {
      strcpy(name, riscv_isa[i]); 
      ishasname = 1; 
      //token = strtok(NULL, IGNORE_CHARS);
      break;
    }
  }
  if (!ishasname) {
    strcpy(name, token);
  }
  while (arg_num < MAX_ARGS && (token = strtok(NULL, IGNORE_CHARS))!= NULL) {
    args[arg_num++] = token;
    //printf("token is: %s\n", token);
    
    if (arg_num == MAX_ARGS && (token = strtok(NULL, IGNORE_CHARS)) != NULL) {
      //token = strtok(NULL, IGNORE_CHARS);
      //printf("token is: %s\n", token);
      raise_extra_argument_error(input_line, token);
      return -1;
    }
  }
  if (!ishasname) {
    raise_instruction_error(input_line, name, args, arg_num);
    return -1;
  }
  int instnum = write_pass_one(blk, name, args, arg_num, input_line, *text_offset);
  if (instnum == 0) {
    raise_instruction_error(input_line, name, args, arg_num);
    return -1;
  }
  if (err) return -1;
  return instnum;
}
/* First pass of the assembler.

   This function should read each line, strip all comments, scan for labels,
   and pass instructions to write_pass_one(). The symbol table should also
   been built and written to specified file. The input file may or may not
   be valid. Here are some guidelines:

    1. Only one label may be present per line. It must be the first token
   present. Once you see a label, regardless of whether it is a valid label or
   invalid label, treat the NEXT token in the same line as the beginning
   of an instruction.
    2. If the first token is not a label, treat it as the name of an
   instruction. DO NOT try to check it is a valid instruction in this pass.
    3. Everything after the instruction name should be treated as arguments to
   that instruction. If there are more than MAX_ARGS arguments, call
   raise_extra_argument_error() and pass in the first extra argument. Do
   not write that instruction to the output file (i.e., don't call
   write_pass_one())
    4. Only one instruction should be present per line. You do not need to do
   anything extra to detect this - it should be handled by guideline 3.
    5. A line containing only a label is valid. The address of the label should
   be the byte offset of the next instruction, regardless of whether there
   is a next instruction or not.
    6. If an instruction contains an immediate, you should output it AS IS.
    7. Comments should always be skipped before any further process.
    8. Invalid labels don't affect translation of subsequent instructions in
   current phase

   Just like in pass_two(), if the function encounters an error it should NOT
   exit, but process the entire file and return -1. If no errors were
   encountered, it should return 0.
 */
int pass_one(FILE *input, Block *blk, SymbolTable *table, DataImage *data_image)
{
  // TODO
  /* A buffer for line parsing. */
  char buf[BUF_SIZE];

  /* Variables for argument parsing. */
  //char *args[MAX_ARGS];
  /* For each line, there are some hints of what you should do:
      1. Skip all comments
      (see the function 'static void skip_comments(char* str)')
      2. Use `strtok()` to read the next token
      3. Handle labels
      4. Parse the instruction
 */
  uint32_t input_line = 1;
  uint32_t _data_offset = 0;
  uint32_t* data_offset = &_data_offset;
  uint32_t _text_offset = 0;
  uint32_t* text_offset = &_text_offset;
  SectionType section = SECTION_TEXT;
  int totalerror = 0;
  while (fgets(buf, BUF_SIZE, input))
  {
    skip_comments(buf);
    char buf_copy[BUF_SIZE];
    strcpy(buf_copy, buf);
    char* token = strtok(buf_copy, IGNORE_CHARS);
    if (!token){input_line++; continue;}

    int update_result = update_section_from_token(token, &section);
    if (update_result == 1){
      input_line++;
      continue;
    }

    if (section == SECTION_DATA){
      int data_result = parse_data_line(input_line, buf, table, data_image, data_offset);
      if (data_result == -1) totalerror=-1;
    }
    else if (section == SECTION_TEXT){
      int phasenum = parse_text_line(blk, input_line, buf, table, text_offset);
      if (phasenum == -1) {
        totalerror=-1;
        input_line++;
        continue;
      }
      unsigned ofst = 4*phasenum;
      (*text_offset)+=ofst;
      //printf("%d %s %d\n", input_line, buf, ofst);
    }
    input_line++;
  }

  return totalerror;
}

/* Second pass of the assembler.
   If an error is reached, DO NOT EXIT the function. Keep translating the rest
   of the document, and at the end, return -1. Return 0 if no errors were
   encountered. */
int pass_two(Block *blk, SymbolTable *table, FILE *output)
{
  if (output == NULL)
  {
    printf("wrong file opened.\n");
    return -1;
  }

  if (!table)
  {
    printf("wrong table opened.\n");
    return -1;
  }

  /* For each line, there are some hints of what you should do:
      1. Get instruction name.
      2. Parse instruction arguments; Extra arguments should be filtered out
      in `pass_one()`, so you don't need to worry about that here.
      3. Use `translate_inst()` to translate the instruction and write to the
      output file;
      4. Or if an error occurs, call `raise_instruction_error()` instead of
      write the instruction.
  */

  /* Variables for argument parsing. */
  int error = 0;
  int PC = 0;
  int totalerror = 0;
  /* Process each instruction */
  for (uint32_t i = 0; i < blk->len; ++i)
  {
    Instr *inst = &blk->entries[i];
    //TODO
    error = translate_inst(output, inst->name, inst->args, inst->arg_num, PC, table);
    if (error==-1) {
      totalerror=-1;
      raise_instruction_error(inst->line_number, inst->name, inst->args, inst->arg_num);
    }
    PC += 4;
  }

  return totalerror;
}

static void close_files(int count, ...)
{
  va_list args;
  va_start(args, count);

  for (int i = 0; i < count; i++)
  {
    FILE *file = va_arg(args, FILE *);
    if (file != NULL)
    {
      fclose(file);
    }
  }

  va_end(args);
}

/* Output folder is assured to end with "/" */
void ResolvePath(const char *input_filename, const char *output_folder,
                 char *output_filename, char *log_filename, char *tbl_filename,
                 char *inst_filename, char *data_filename)
{
  /* Extract the filename from the path */
  const char *fileName = strrchr(input_filename, '/');
  if (fileName)
  {
    /* Skip the '/' character */
    fileName++;
  }
  else
  {
    /* No '/' detected, use the pure input_filename directly. */
    fileName = input_filename;
  }

  /* Remove the extension by locating the last '.' character */
  char base[MAX_PATH_LENGTH];
  char *dot = strrchr(fileName, '.');
  size_t base_length = dot ? (size_t)(dot - fileName) : strlen(fileName);

  strncpy(base, fileName, base_length);
  /* Null-terminate the base string */
  base[base_length] = '\0';

  /* Ensure the base does not exceed the buffer size - 4 */
  size_t folder_len = strlen(output_folder);
  /* We need space for "/" (1 char), ".inst" (5 chars) and the null terminator
   * (1 char) => 7 extra characters total.
   */
  size_t extra = 7;
  size_t available = (MAX_PATH_LENGTH > folder_len + extra)
                         ? MAX_PATH_LENGTH - folder_len - extra
                         : 0;

  /* Build the final filenames */
  snprintf(output_filename, MAX_PATH_LENGTH, "%s%.*s.out", output_folder,
           (int)available, base);
  snprintf(log_filename, MAX_PATH_LENGTH, "%s%.*s.log", output_folder,
           (int)available, base);

  if (tbl_filename && inst_filename)
  {
    snprintf(tbl_filename, MAX_PATH_LENGTH, "%s%.*s.tbl", output_folder,
             (int)available, base);
    snprintf(inst_filename, MAX_PATH_LENGTH, "%s%.*s.inst", output_folder,
             (int)available, base);
  }

  if (data_filename)
  {
    snprintf(data_filename, MAX_PATH_LENGTH, "%s%.*s.data", output_folder,
             (int)available, base);
  }
}

/* Runs the two-pass assembler. Most of the actual work is done in pass_one()
   and pass_two().
 */
int assemble(const char *in, const char *out, int test)
{
  FILE *input, *output, *data_file, *tbl_file, *inst_file;
  char output_filename[MAX_PATH_LENGTH];
  char log_filename[MAX_PATH_LENGTH];
  char tbl_filename[MAX_PATH_LENGTH];
  char inst_filename[MAX_PATH_LENGTH];
  char data_filename[MAX_PATH_LENGTH];
  DataImage data_image;
  int err = 0;

  data_image_init(&data_image);

  if (test)
  {
    ResolvePath(in, out, output_filename, log_filename, tbl_filename,
                inst_filename, data_filename);
  }
  else
  {
    ResolvePath(in, out, output_filename, log_filename, NULL, NULL,
                data_filename);
  }
  set_log_file(log_filename);

  SymbolTable *tbl = create_table(SYMBOLTBL_UNIQUE_NAME);
  Block *blk = create_block();

  input = fopen(in, "r");
  output = fopen(output_filename, "w");
  data_file = fopen(data_filename, "w");

  if (input == NULL || output == NULL || data_file == NULL)
  {
    exit(1);
  }

  if (pass_one(input, blk, tbl, &data_image) != 0)
  {
    err = 1;
  }

  if (pass_two(blk, tbl, output) != 0)
  {
    err = 1;
  }

  if (write_data_output(data_file, &data_image) != 0)
  {
    err = 1;
  }

  if (test)
  {
    tbl_file = fopen(tbl_filename, "w");
    inst_file = fopen(inst_filename, "w");
    write_table(tbl, tbl_file);
    write_block(blk, inst_file);
    close_files(2, tbl_file, inst_file);
  }
  if (err)
  {
    write_to_log("One or more errors encountered during assembly operation.\n");
  }
  else
  {
    write_to_log("Assembly operation completed successfully!\n");
  }

  free_table(tbl);
  free_block(blk);
  data_image_free(&data_image);

  close_files(3, input, output, data_file);
  return err;
}

static void print_usage_and_exit(void)
{
  printf("Usage:\n");
  printf("--input_file: The input file of the assembler\n");
  printf("--output_folder: The output folder of the assembler\n");
  exit(0);
}

int main(int argc, char **argv)
{
  //int err;
  enum OPTIONS
  {
    OPT_INPUT,
    OPT_OUTPUT,
    OPT_TEST,
  };

  static struct option long_options[] = {
      {"input_file", required_argument, NULL, OPT_INPUT},
      {"output_folder", required_argument, NULL, OPT_OUTPUT},
      {"test", no_argument, NULL, OPT_TEST},
      {0, 0, 0, 0}};

  char input[MAX_PATH_LENGTH] = {0};
  char output[MAX_PATH_LENGTH] = {0};

  int opt;
  char short_options[] = "";
  int option_index = 0;

  int test = 0;
  while ((opt = getopt_long_only(argc, argv, short_options, long_options,
                                 &option_index)) != -1)
  {
    switch (opt)
    {
    case OPT_INPUT:
      strcpy(input, optarg);
      break;
    case OPT_OUTPUT:
      // If the output folder ends with "/", use it directly.
      // Else, add "/" to the end.
      if (optarg[strlen(optarg) - 1] == '/')
      {
        snprintf(output, MAX_PATH_LENGTH, "%s", optarg);
      }
      else
      {
        snprintf(output, MAX_PATH_LENGTH, "%s/", optarg);
      }
      break;
    case OPT_TEST:
      test = 1;
      break;
    default:
      print_usage_and_exit();
      break;
    }
  }
  if (strlen(input) == 0 || strlen(output) == 0)
  {
    printf("Please provide the correct input file and output folder.\n");
    return 0;
  }
  /*err = */assemble(input, output, test);

  //return err;
  return 0;
}
