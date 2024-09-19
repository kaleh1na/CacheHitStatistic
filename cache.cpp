#pragma once
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;

#define MEM_SIZE 262144
#define CACHE_SIZE 4096
#define CACHE_LINE_SIZE 32
#define CACHE_LINE_COUNT 128
#define CACHE_WAY 4
#define CACHE_SETS 32
#define ADDR_LEN 18
#define CACHE_TAG_LEN 8
#define CACHE_INDEX_LEN 5
#define CACHE_OFFSET_LEN 5

static uint8_t Mem[MEM_SIZE];

static map<string, size_t> CommandId{
    {"add", 0},     {"sub", 1},    {"sll", 2},    {"slt", 3},
    {"sltu", 4},    {"xor", 5},    {"srl", 6},    {"sra", 7},
    {"or", 8},      {"and", 9},    {"mul", 10},   {"mulh", 11},
    {"mulhsu", 12}, {"mulhu", 13}, {"div", 14},   {"divu", 15},
    {"rem", 16},    {"remu", 17},  // 0-17 R
    {"addi", 18},   {"slti", 19},  {"sltiu", 20}, {"xori", 21},
    {"ori", 22},    {"andi", 23},  {"slli", 24},  {"srli", 25},
    {"srai", 26},   {"jalr", 27},  // 18-27 I
    {"lb", 28},     {"lh", 29},    {"lw", 30},    {"lbu", 31},
    {"lhu", 32},                                // 28-32 L
    {"sb", 33},     {"sh", 34},    {"sw", 35},  // 33-35 S
    {"beq", 36},    {"bne", 37},   {"blt", 38},   {"bge", 39},
    {"bltu", 40},   {"bgeu", 41},                // 36-41 B
    {"lui", 42},    {"auipc", 43}, {"jal", 44},  // 42-44 U
    {"ecall", 45},  {"ebreak", 46}               // 45-46 E
};

static vector<int> funct7{0, 32, 0, 0, 0, 0, 0, 32, 0,
                          0, 1,  1, 1, 1, 1, 1, 1,  1};

static vector<int> funct3{
    0, 0, 1, 2, 3, 4, 5, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7,  // R
    0, 2, 3, 4, 6, 7, 1, 5, 5, 0,                          // I
    0, 1, 2, 4, 5,                                         // L
    0, 1, 2,                                               // S
    0, 1, 4, 5, 6, 7                                       // B
};

static vector<int> opcode{
    51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
    51, 51, 51, 51, 51, 51, 51, 51,           // R
    19, 19, 19, 19, 19, 19, 19, 19, 19, 103,  // I
    3,  3,  3,  3,  3,                        // L
    35, 35, 35,                               // S
    99, 99, 99, 99, 99, 99,                   // B
    55, 23, 111                               // U
};

static map<string, size_t> RegId{
    {"zero", 0}, {"ra", 1},  {"sp", 2},   {"gp", 3},   {"tp", 4},  {"t0", 5},
    {"t1", 6},   {"t2", 7},  {"s0", 8},   {"s1", 9},   {"a0", 10}, {"a1", 11},
    {"a2", 12},  {"a3", 13}, {"a4", 14},  {"a5", 15},  {"a6", 16}, {"a7", 17},
    {"s2", 18},  {"s3", 19}, {"s4", 20},  {"s5", 21},  {"s6", 22}, {"s7", 23},
    {"s8", 24},  {"s9", 25}, {"s10", 26}, {"s11", 27}, {"t3", 28}, {"t4", 29},
    {"t5", 30},  {"t6", 31}};

struct Instruction {
  size_t id;
  char type;
  size_t rd;
  size_t rs1;
  size_t rs2;
  int32_t imm;

  Instruction(string& com) : id(CommandId[com]) {
    if (0 <= id and id <= 17) {
      type = 'R';
    } else if (18 <= id and id <= 27) {
      type = 'I';
    } else if (28 <= id and id <= 32) {
      type = 'L';
    } else if (33 <= id and id <= 35) {
      type = 'S';
    } else if (36 <= id and id <= 41) {
      type = 'B';
    } else if (42 <= id and id <= 44) {
      type = 'U';
    } else {
      type = 'E';
    }
  }

  uint32_t Code() {
    uint32_t code = 0;
    switch (type) {
      case 'R':
        code += funct7[id];
        code <<= 5;
        code += rs2;
        code <<= 5;
        code += rs1;
        code <<= 3;
        code += funct3[id];
        code <<= 5;
        code += rd;
        code <<= 7;
        code += opcode[id];
        break;
      case 'I':
      case 'L':
        if (24 <= id && id <= 26) {
          if (id == 26) {
            code += 32;
          }
          code <<= 5;
          code += ((uint32_t)(imm) % (1 << 5));
        } else {
          code += ((uint32_t)(imm) % (1 << 12));
        }
        code <<= 5;
        code += rs1;
        code <<= 3;
        code += funct3[id];
        code <<= 5;
        code += rd;
        code <<= 7;
        code += opcode[id];
        break;
      case 'S':
        code += (((uint32_t)(imm) % (1 << 12))) >> 5;
        code <<= 5;
        code += rs2;
        code <<= 5;
        code += rs1;
        code <<= 3;
        code += funct3[id];
        code <<= 5;
        code += ((uint32_t)(imm) % (1 << 5));
        code <<= 7;
        code += opcode[id];
        break;
      case 'B':
        code += ((uint32_t)(imm) & (1 << 12)) >> 6;
        code += (((uint32_t)(imm) % (1 << 11))) >> 5;
        code <<= 5;
        code += rs2;
        code <<= 5;
        code += rs1;
        code <<= 3;
        code += funct3[id];
        code <<= 5;
        code += ((uint32_t)(imm) % (1 << 5)) >> 1 << 1;
        code += ((uint32_t)(imm) & (1 << 11)) >> 11;
        code <<= 7;
        code += opcode[id];
        break;
      case 'U':
        if (id == 44) {
          code += ((uint32_t)(imm) & (1 << 20)) >> 20;
          code <<= 10;
          code += ((uint32_t)(imm) % (1 << 11)) >> 1;
          code <<= 1;
          code += ((uint32_t)(imm) & (1 << 11)) >> 11;
          code <<= 8;
          code += ((uint32_t)(imm) % (1 << 19)) >> 12;
        } else {
          code += (uint32_t)(imm) >> 12;
        }
        code <<= 5;
        code += rd;
        code <<= 7;
        code += opcode[id];
        break;
      case 'E':
        if (id == 45) {
          code = 15;
        } else {
          code = 131087;
        }
        break;
    }
    return code;
  }
};

struct CacheLine {
  bool updated = false;
  size_t time = 0;
  bool bit = false;
  uint8_t tag_address;
  uint8_t bytes[CACHE_LINE_SIZE];
};

struct CacheBlock {
  size_t size = 0;
  CacheLine lines[CACHE_WAY];

  void LoadLine(size_t ind, uint32_t address) {
    lines[ind].tag_address = address >> (CACHE_INDEX_LEN + CACHE_OFFSET_LEN);
    for (int i = 0; i < CACHE_LINE_SIZE; ++i) {
      lines[ind].bytes[i] =
          Mem[((address >> CACHE_OFFSET_LEN) << CACHE_OFFSET_LEN) + i];
    }
  }

  void StoreLine(size_t line_ind, size_t block_ind) {
    uint32_t adr =
        ((lines[line_ind].tag_address << CACHE_INDEX_LEN) + block_ind)
        << CACHE_OFFSET_LEN;
    for (int j = 0; j < CACHE_LINE_SIZE; ++j) {
      Mem[adr + j] = lines[line_ind].bytes[j];
    }
  }

  virtual size_t ReplaceLine(uint32_t address) = 0;

  virtual void Reset(size_t ind) = 0;

  uint8_t Read(uint32_t address, bool& flag) {
    uint8_t tag_address = address >> (CACHE_INDEX_LEN + CACHE_OFFSET_LEN);
    uint8_t byte_ind = address % (1 << CACHE_OFFSET_LEN);
    for (int i = 0; i < size; ++i) {
      if (lines[i].tag_address == tag_address) {
        Reset(i);
        flag = true;
        return lines[i].bytes[byte_ind];
      }
    }
    if (size == CACHE_WAY) {
      size_t line_ind = ReplaceLine(address);
      Reset(line_ind);
      LoadLine(line_ind, address);
      lines[line_ind].updated = false;
      flag = false;
      return lines[line_ind].bytes[byte_ind];
    }
    ++size;
    Reset(size - 1);
    LoadLine(size - 1, address);
    lines[size - 1].updated = false;
    flag = false;
    return lines[size - 1].bytes[byte_ind];
  }

  void Write(uint32_t address, uint8_t byte, bool& flag) {
    uint8_t tag_address = address >> (CACHE_INDEX_LEN + CACHE_OFFSET_LEN);
    uint8_t byte_ind = address % (1 << CACHE_OFFSET_LEN);
    for (int i = 0; i < size; ++i) {
      if (lines[i].tag_address == tag_address) {
        Reset(i);
        lines[i].updated = true;
        lines[i].bytes[byte_ind] = byte;
        flag = true;
        return;
      }
    }
    if (size == CACHE_WAY) {
      size_t line_ind = ReplaceLine(address);
      Reset(line_ind);
      LoadLine(line_ind, address);
      lines[line_ind].updated = true;
      lines[line_ind].bytes[byte_ind] = byte;
      flag = false;
      return;
    }
    ++size;
    Reset(size - 1);
    LoadLine(size - 1, address);
    lines[size - 1].updated = true;
    lines[size - 1].bytes[byte_ind] = byte;
    flag = false;
    return;
  }
};

struct LRUCacheBlock : public CacheBlock {
  size_t ReplaceLine(uint32_t address) override {
    size_t max_time = 0;
    size_t line_ind = 0;
    for (int i = 0; i < CACHE_WAY; ++i) {
      if (max_time < lines[i].time) {
        max_time = lines[i].time;
        line_ind = i;
      }
    }
    if (lines[line_ind].updated == true) {
      size_t block_ind = (address >> CACHE_OFFSET_LEN) % (1 << CACHE_INDEX_LEN);
      StoreLine(line_ind, block_ind);
    }
    return line_ind;
  }

  void Reset(size_t ind) override {
    for (int i = 0; i < size; ++i) {
      ++lines[i].time;
    }
    lines[ind].time = 0;
  }
};

struct pLRUCacheBlock : public CacheBlock {
  size_t ReplaceLine(uint32_t address) override {
    for (int i = 0; i < size; ++i) {
      if (lines[i].bit == false) {
        if (lines[i].updated == true) {
          size_t block_ind =
              (address >> CACHE_OFFSET_LEN) % (1 << CACHE_INDEX_LEN);
          StoreLine(i, block_ind);
        }
        return i;
      }
    }
    return 0;
  }

  void Reset(size_t ind) override {
    lines[ind].bit = true;
    if (size == CACHE_WAY) {
      if (lines[0].bit && lines[1].bit && lines[2].bit && lines[3].bit) {
        lines[0].bit = false;
        lines[1].bit = false;
        lines[2].bit = false;
        lines[3].bit = false;
        lines[ind].bit = true;
      }
    }
  }
};

class CacheModel {
 private:
  int replacement;
  string input_file;
  string output_file;

  uint32_t regs[32];
  LRUCacheBlock LRUblocks[CACHE_SETS];
  pLRUCacheBlock pLRUblocks[CACHE_SETS];

  size_t number_of_lru_hits = 0;
  size_t number_of_plru_hits = 0;
  size_t number_of_requests = 0;

  vector<Instruction> program;

  void Low(string& inst) {
    for (int i = 0; i < inst.size(); ++i) {
      if (isupper(inst[i])) inst[i] = tolower(inst[i]);
    };
  }

  int32_t Convert(const string& arg) {
    if (arg.size() > 2 && (arg[1] == 'x' || (arg[0] == '-' && arg[2] == 'x'))) {
      return stoi(arg.substr(2), nullptr, 16);
    }
    return stoi(arg);
  }

  void Write(uint32_t address, uint32_t bytes, size_t size) {
    ++number_of_requests;
    bool LRUhit = true;
    bool pLRUhit = true;
    for (int i = 0; i < size; ++i) {
      uint32_t adr = address + i;
      uint8_t byte = bytes % (1 << 8);
      bytes >>= 8;
      size_t block_ind = (adr >> CACHE_OFFSET_LEN) % (1 << CACHE_INDEX_LEN);
      if ((replacement == 0 || replacement == 1)) {
        bool flag;
        LRUblocks[block_ind].Write(adr, byte, flag);
        if (!flag) {
          LRUhit = false;
        }
      }
      if ((replacement == 0 || replacement == 2)) {
        bool flag;
        pLRUblocks[block_ind].Write(adr, byte, flag);
        if (!flag) {
          pLRUhit = false;
        }
      }
    }
    if ((replacement == 0 || replacement == 1) && LRUhit) {
      ++number_of_lru_hits;
    }
    if ((replacement == 0 || replacement == 2) && pLRUhit) {
      ++number_of_plru_hits;
    }
  }

  uint32_t Read(uint32_t address, size_t size) {
    ++number_of_requests;
    bool LRUhit = true;
    bool pLRUhit = true;
    vector<uint8_t> bytes1;
    vector<uint8_t> bytes2;
    for (int i = 0; i < size; ++i) {
      uint32_t adr = address + i;
      size_t block_ind = (adr >> CACHE_OFFSET_LEN) % (1 << CACHE_INDEX_LEN);
      if ((replacement == 0 || replacement == 1)) {
        bool flag;
        bytes1.push_back(LRUblocks[block_ind].Read(adr, flag));
        if (!flag) {
          LRUhit = false;
        }
      }
      if ((replacement == 0 || replacement == 2)) {
        bool flag;
        bytes2.push_back(pLRUblocks[block_ind].Read(adr, flag));
        if (!flag) {
          pLRUhit = false;
        }
      }
    }
    if ((replacement == 0 || replacement == 1) && LRUhit) {
      ++number_of_lru_hits;
    }
    if ((replacement == 0 || replacement == 2) && pLRUhit) {
      ++number_of_plru_hits;
    }
    uint32_t ans = 0;
    if (replacement == 0 || replacement == 1) {
      for (int i = bytes1.size() - 1; i >= 0; --i) {
        ans <<= 8;
        ans += bytes1[i];
      }
      return ans;
    }
    for (int i = bytes2.size() - 1; i >= 0; --i) {
      ans <<= 8;
      ans += bytes2[i];
    }
    return ans;
  }

  void StoreCash() {
    for (int i = 0; i < CACHE_SETS; ++i) {
      for (int j = 0; j < CACHE_WAY; ++j) {
        if (LRUblocks[i].lines[j].updated) {
          LRUblocks[i].StoreLine(j, i);
        }
        if (pLRUblocks[i].lines[j].updated) {
          pLRUblocks[i].StoreLine(j, i);
        }
      }
    }
  }

  void ParseArgs(int argc, char** argv) {
    for (int i = 1; i < argc; i += 2) {
      if (argv[i][2] == 'r') {
        replacement = argv[i + 1][0] - '0';
      }
      if (argv[i][2] == 'a') {
        input_file = argv[i + 1];
      }
      if (argv[i][2] == 'b') {
        output_file = argv[i + 1];
      }
    }
  }

  void ReadFile() {
    ifstream f(input_file);
    string com;
    while (f >> com) {
      Low(com);
      Instruction inst(com);
      if (inst.type == 'E') {
        program.push_back(inst);
        continue;
      }
      string arg1;
      string arg2;
      string arg3;
      f >> arg1 >> arg2;
      Low(arg1);
      Low(arg2);
      arg1 = arg1.substr(0, arg1.size() - 1);
      if (inst.type != 'U') {
        f >> arg3;
        Low(arg3);
        arg2 = arg2.substr(0, arg2.size() - 1);
      }
      switch (inst.type) {
        case 'R':
          inst.rd = RegId[arg1];
          inst.rs1 = RegId[arg2];
          inst.rs2 = RegId[arg3];
          break;
        case 'I':
          inst.rd = RegId[arg1];
          inst.rs1 = RegId[arg2];
          inst.imm = Convert(arg3);
          break;
        case 'L':
          inst.rd = RegId[arg1];
          inst.rs1 = RegId[arg3];
          inst.imm = Convert(arg2);
          break;
        case 'S':
          inst.rs1 = RegId[arg3];
          inst.rs2 = RegId[arg1];
          inst.imm = Convert(arg2);
          break;
        case 'B':
          inst.rs1 = RegId[arg1];
          inst.rs2 = RegId[arg2];
          inst.imm = Convert(arg3);
          break;
        case 'U':
          inst.rd = RegId[arg1];
          inst.imm = Convert(arg2);
          break;
      }
      program.push_back(inst);
    }
    f.close();
  }

  void Code() {
    ofstream f;
    f.open(output_file, ios::binary);
    if (!f.is_open()) {
      return;
    }
    for (int i = 0; i < program.size(); ++i) {
      uint32_t code = program[i].Code();
      f << (uint8_t)((code % (1 << 8)));
      f << (uint8_t)((code % (1 << 16)) >> 8);
      f << (uint8_t)((code % (1 << 24)) >> 16);
      f << (uint8_t)((code % ((uint64_t)1 << 32)) >> 24);
    }
    f.close();
  }

  void Modeling() {
    regs[1] = program.size() * 4;
    for (int i = 0; i < program.size(); ++i) {
      regs[0] = 0;
      {
        switch (program[i].id) {
          case 0:
            regs[program[i].rd] = regs[program[i].rs1] + regs[program[i].rs2];
            break;
          case 1:
            regs[program[i].rd] = regs[program[i].rs1] - regs[program[i].rs2];
            break;
          case 2:
            regs[program[i].rd] = regs[program[i].rs1] << regs[program[i].rs2];
            break;
          case 3:
            regs[program[i].rd] =
                regs[program[i].rs1] < regs[program[i].rs2] ? 1 : 0;
            break;
          case 4:
            regs[program[i].rd] =
                regs[program[i].rs1] < regs[program[i].rs2] ? 1 : 0;
            break;
          case 5:
            regs[program[i].rd] = regs[program[i].rs1] ^ regs[program[i].rs2];
            break;
          case 6:
            regs[program[i].rd] = regs[program[i].rs1] >> regs[program[i].rs2];
            break;
          case 7:
            regs[program[i].rd] = regs[program[i].rs1] >> regs[program[i].rs2];
            break;
          case 8:
            regs[program[i].rd] = regs[program[i].rs1] | regs[program[i].rs2];
            break;
          case 9:
            regs[program[i].rd] = regs[program[i].rs1] & regs[program[i].rs2];
            break;
          case 10:
            regs[program[i].rd] = regs[program[i].rs1] * regs[program[i].rs2];
            break;
          case 11:
            regs[program[i].rd] = ((uint64_t)regs[program[i].rs1] *
                                   (uint64_t)regs[program[i].rs2]) >>
                                  32;
            break;
          case 12:
            regs[program[i].rd] = ((uint64_t)regs[program[i].rs1] *
                                   (uint64_t)regs[program[i].rs2]) >>
                                  32;
            break;
          case 13:
            regs[program[i].rd] = ((uint64_t)regs[program[i].rs1] *
                                   (uint64_t)regs[program[i].rs2]) >>
                                  32;
            break;
          case 14:
            regs[program[i].rd] = regs[program[i].rs1] / regs[program[i].rs2];
            break;
          case 15:
            regs[program[i].rd] = regs[program[i].rs1] / regs[program[i].rs2];
            break;
          case 16:
            regs[program[i].rd] = regs[program[i].rs1] % regs[program[i].rs2];
            break;
          case 17:
            regs[program[i].rd] = regs[program[i].rs1] % regs[program[i].rs2];
            break;
          case 18:
            regs[program[i].rd] = regs[program[i].rs1] + program[i].imm;
            break;
          case 19:
            regs[program[i].rd] = regs[program[i].rs1] < program[i].imm ? 1 : 0;
            break;
          case 20:
            regs[program[i].rd] = regs[program[i].rs1] < program[i].imm ? 1 : 0;
            break;
          case 21:
            regs[program[i].rd] = regs[program[i].rs1] ^ program[i].imm;
            break;
          case 22:
            regs[program[i].rd] = regs[program[i].rs1] | program[i].imm;
            break;
          case 23:
            regs[program[i].rd] = regs[program[i].rs1] & program[i].imm;
            break;
          case 24:
            regs[program[i].rd] = regs[program[i].rs1] << program[i].imm;
            break;
          case 25:
            regs[program[i].rd] = regs[program[i].rs1] >> program[i].imm;
            break;
          case 26:
            regs[program[i].rd] = regs[program[i].rs1] >> program[i].imm;
            break;
          case 27:
            regs[program[i].rd] = i * 4 + 4;
            i = (regs[program[i].rs1] + program[i].imm) / 4 - 1;
            break;
          case 28:
            regs[program[i].rd] =
                Read(regs[program[i].rs1] + program[i].imm, 1);
            break;
          case 29:
            regs[program[i].rd] =
                Read(regs[program[i].rs1] + program[i].imm, 2);
            break;
          case 30:
            regs[program[i].rd] =
                Read(regs[program[i].rs1] + program[i].imm, 4);
            break;
          case 31:
            regs[program[i].rd] =
                Read(regs[program[i].rs1] + program[i].imm, 1);
            break;
          case 32:
            regs[program[i].rd] =
                Read(regs[program[i].rs1] + program[i].imm, 2);
            break;
          case 33:
            Write(regs[program[i].rs1] + program[i].imm, regs[program[i].rs2],
                  1);
            break;
          case 34:
            Write(regs[program[i].rs1] + program[i].imm, regs[program[i].rs2],
                  2);
            break;
          case 35:
            Write(regs[program[i].rs1] + program[i].imm, regs[program[i].rs2],
                  4);
            break;
          case 36:
            if (regs[program[i].rs1] == regs[program[i].rs2]) {
              i += (program[i].imm) / 4;
              --i;
            }
            break;
          case 37:
            if (regs[program[i].rs1] != regs[program[i].rs2]) {
              i += (program[i].imm) / 4;
              --i;
            }
            break;
          case 38:
            if (regs[program[i].rs1] < regs[program[i].rs2]) {
              i += (program[i].imm) / 4;
              --i;
            }
            break;
          case 39:
            if (regs[program[i].rs1] >= regs[program[i].rs2]) {
              i += (program[i].imm) / 4;
              --i;
            }
            break;
          case 40:
            if (regs[program[i].rs1] < regs[program[i].rs2]) {
              i += (program[i].imm) / 4;
              --i;
            }
            break;
          case 41:
            if (regs[program[i].rs1] >= regs[program[i].rs2]) {
              i += (program[i].imm) / 4;
              --i;
            }
            break;
          case 42:
            regs[program[i].rd] = program[i].imm << 12;
            break;
          case 43:
            regs[program[i].rd] = i + (program[i].imm << 12);
            break;
          case 44:
            regs[program[i].rd] = i * 4 + 4;
            i += (program[i].imm) / 4 - 1;
            break;
        }
      }
    }
    StoreCash();
    if (replacement == 0) {
      printf("LRU\thit rate: %3.4f%%\npLRU\thit rate: %3.4f%%\n",
             (float)number_of_lru_hits * 100 / number_of_requests,
             (float)number_of_plru_hits * 100 / number_of_requests);
    } else if (replacement == 1) {
      printf("LRU\thit rate: %3.4f%%\n",
             (float)number_of_lru_hits * 100 / number_of_requests);
    } else {
      printf("pLRU\thit rate: %3.4f%%\n",
             (float)number_of_plru_hits * 100 / number_of_requests);
    }
  }

 public:
  CacheModel(int argc, char** argv) {
    ParseArgs(argc, argv);
    ReadFile();
    Code();
    Modeling();
  }
};
