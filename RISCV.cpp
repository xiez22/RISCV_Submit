#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define RISCV_RELEASE

//--------------------------------------常用函数----------------------------------------------
uint32_t fetch_num(uint32_t instruction, uint32_t from, uint32_t to) {
	return (instruction >> from) & ((1u << (to - from + 1)) - 1u);
}

enum INST { Add, Sub, Xor, Or, And, Sll, Srl, Sra, Slt, Sltu, Lb, Lh, Lw, Lbu, Lhu, Addi, 
	Xori, Ori, Andi, Slli, Srli, Srai, Slti, Sltiu, Sb, Sh, Sw, Beq, Bne, Blt, Bge, Bltu, 
	Bgeu, Lui, Auipc, Jal, Jalr };

class COMPUTER {
public:
	uint32_t XI[32] = { 0 };
	uint8_t Mem[0x3fffff] = { 0 };
	COMPUTER() = default;

	uint32_t PC = 0;
	bool exit_flag = false;

	void wreg(uint32_t code, uint32_t val) {
		if (code) {
			XI[code] = val;
			//std::cerr << code << "    " << val << std::endl;
		}
	}

	uint32_t urmem(uint32_t pos, int32_t len) {
		uint32_t ans = 0;
		for (--len; len >= 0; --len)
			ans = (ans << 8) + uint32_t(Mem[pos + len]);
		return ans;
	}

	int32_t rmem(uint32_t pos, int32_t len) {
		int32_t ans = 0;
		for (int i = len - 1; i >= 0; --i)
			ans = (ans << 8) + int32_t(Mem[pos + i]);
		ans = ans << (32 - 8 * len) >> (32 - 8 * len);
		return ans;
	}

	void wmem(uint32_t pos, uint32_t val, int32_t len) {
		for (int i = 0; i < len; ++i) {
			Mem[pos + i] = uint8_t(val);
			val >>= 8;
		}
	}
};
COMPUTER reg;

//---------------------------------------执行环节----------------------------------------------
class RUN_DATA {
public:
	INST inst;
	uint32_t rs1 = 0, rs2 = 0;
	uint32_t imm = 0;
	uint32_t rd = 0;
	uint32_t tmp = 0;
};

class EXECUTE {
public:
	RUN_DATA execute(RUN_DATA result) {
		switch (result.inst)
		{
		case Add:
			result.tmp = result.rs1 + result.rs2;
			break;
		case Sub:
			result.tmp = result.rs1 - result.rs2;
			break;
		case Xor:
			result.tmp = result.rs1 ^ result.rs2;
			break;
		case Or:
			result.tmp = result.rs1 | result.rs2;
			break;
		case And:
			result.tmp = result.rs1 & result.rs2;
			break;
		case Sll:
			result.tmp = result.rs1 << result.rs2;
			break;
		case Srl:
			result.tmp = result.rs1 >> result.rs2;
			break;
		case Sra:
			result.tmp = (int32_t(result.rs1)) >> result.rs2;
			break;
		case Slt:
			result.tmp = uint32_t(((int32_t)result.rs1) < ((int32_t)result.rs2));
			break;
		case Sltu:
			result.tmp = uint32_t(((uint32_t)result.rs1) < ((uint32_t)result.rs2));
			break;
		case Addi:
			result.tmp = result.rs1 + ((int32_t(result.imm)) << 20 >> 20);
			break;
		case Xori:
			result.tmp = result.rs1 ^ ((int32_t(result.imm)) << 20 >> 20);
			break;
		case Ori:
			result.tmp = result.rs1 | ((int32_t(result.imm)) << 20 >> 20);
			break;
		case Andi:
			result.tmp = result.rs1 & ((int32_t(result.imm)) << 20 >> 20);
			break;
		case Slli:
			result.tmp = result.rs1 << result.imm;
			break;
		case Srli:
			result.tmp = result.rs1 >> result.imm;
			break;
		case Srai:
			result.tmp = (int32_t(result.rs1)) >> result.imm;
			break;
		case Slti:
			result.tmp = uint32_t(((int32_t)result.rs1) < (((int32_t(result.imm)) << 20) >> 20));
			break;
		case Sltiu:
			result.tmp = uint32_t(uint32_t(result.rs1) < uint32_t(result.imm));
			break;
		case Sb:
			if (result.rs1 + result.imm == 0x30004) {
#ifndef RISCV_DEBUG
				std::cout << (reg.XI[10] & 255u) << std::endl;
#endif // !RISCV_DEBUG
				reg.exit_flag = true;
			}
			break;
		case Sh:
			break;
		case Sw:
			break;
		case Beq:
			if (result.rs1 == result.rs2)
				reg.PC += (((int32_t)result.imm) << 20 >> 20) - 4;
			break;
		case Bne:
			if (result.rs1 != result.rs2)
				reg.PC += (((int32_t)result.imm) << 20 >> 20) - 4;
			break;
		case Blt:
			if (int32_t(result.rs1) < int32_t(result.rs2))
				reg.PC += (((int32_t)result.imm) << 20 >> 20) - 4;
			break;
		case Bge:
			if (int32_t(result.rs1) >= int32_t(result.rs2))
				reg.PC += (((int32_t)result.imm) << 20 >> 20) - 4;
			break;
		case Bltu:
			if (result.rs1 < result.rs2)
				reg.PC += (((int32_t)result.imm) << 20 >> 20) - 4;
			break;
		case Bgeu:
			if (result.rs1 >= result.rs2)
				reg.PC += (((int32_t)result.imm) << 20 >> 20) - 4;
			break;
		case Lui:
			break;
		case Auipc:
			reg.PC += result.imm;
			break;
		case Jal:
			result.tmp = reg.PC;
			reg.PC += ((((int32_t)result.imm) << 11) >> 11) - 4;
			break;
		case Jalr:
			result.tmp = reg.PC;
			reg.PC = result.rs1 + ((((int32_t)result.imm) << 20) >> 20);
			reg.PC = reg.PC >> 1 << 1;
			break;
		default:
			break;
		}
		return result;
	}
};

class MEM_ACCESS {
public:
	RUN_DATA mem_access(RUN_DATA result) {
		switch (result.inst)
		{
		case Lb:
			result.tmp = (uint32_t)reg.rmem(result.rs1 + (((int32_t(result.imm)) << 20) >> 20), 1);
			break;
		case Lh:
			result.tmp = (uint32_t)reg.rmem(result.rs1 + (((int32_t(result.imm)) << 20) >> 20), 2);
			break;
		case Lw:
			result.tmp = (uint32_t)reg.rmem(result.rs1 + ((int32_t(result.imm)) << 20 >> 20), 4);
			break;
		case Lbu:
			result.tmp = reg.urmem(result.rs1 + (((int32_t(result.imm)) << 20) >> 20), 1);
			break;
		case Lhu:
			result.tmp = reg.urmem(result.rs1 + (((int32_t(result.imm)) << 20) >> 20), 2);
			break;
		case Sb:
			reg.wmem(result.rs1 + (((int32_t(result.imm)) << 20) >> 20), uint32_t(uint8_t(result.rs2)), 1);
			break;
		case Sh:
			reg.wmem(result.rs1 + (((int32_t(result.imm)) << 20) >> 20), uint32_t(uint16_t(result.rs2)), 2);
			break;
		case Sw:
			reg.wmem(result.rs1 + (((int32_t(result.imm)) << 20) >> 20), uint32_t(result.rs2), 4);
			break;
		default:
			break;
		}
		return result;
	}
};

class WRITE_BACK {
public:
	RUN_DATA write_back(RUN_DATA result) {
		switch (result.inst)
		{
		case Add:
		case Sub:
		case Xor:
		case Or:
		case And:
		case Sll:
		case Srl:
		case Sra:
		case Slt:
		case Sltu:
		case Addi:
		case Xori:
		case Ori:
		case Andi:
		case Slli:
		case Srli:
		case Srai:
		case Slti:
		case Sltiu:
		case Jal:
		case Jalr:
		case Lb:
		case Lh:
		case Lw:
		case Lbu:
		case Lhu:
			reg.wreg(result.rd, result.tmp);
			break;
		case Lui:
		case Auipc:
			reg.wreg(result.rd, result.imm);
			break;
		default:
			break;
		}
		return result;
	}
};

//---------------------------------------解码环节----------------------------------------------
//Decode部分
class DECODE {
	RUN_DATA R_decode() {
		RUN_DATA ans;
		int op = fetch_num(instruction, 0, 6), funct1 = fetch_num(instruction, 12, 14),
			funct2 = fetch_num(instruction, 25, 31);
		switch (funct1)
		{
		case 0b000:
			if (funct2 == 0)
				ans.inst = Add;
			else
				ans.inst = Sub;
			break;
		case 0b001:
			ans.inst = Sll;
			break;
		case 0b010:
			ans.inst = Slt;
			break;
		case 0b011:
			ans.inst = Sltu;
			break;
		case 0b100:
			ans.inst = Xor;
			break;
		case 0b101:
			if (funct2 == 0)
				ans.inst = Srl;
			else
				ans.inst = Sra;
			break;
		case 0b110:
			ans.inst = Or;
			break;
		case 0b111:
			ans.inst = And;
			break;
		default:
			break;
		}
		
		ans.rs1 = reg.XI[fetch_num(instruction, 15, 19)];
		ans.rs2 = reg.XI[fetch_num(instruction, 20, 24)];
		ans.rd = fetch_num(instruction, 7, 11);
		return ans;
	}
	RUN_DATA I_decode() {
		RUN_DATA ans;
		int op = fetch_num(instruction, 0, 6), funct = fetch_num(instruction, 12, 14);
		if (op == 3) {
			switch (funct)
			{
			case 0b000:
				ans.inst = Lb;
				break;
			case 0b001:
				ans.inst = Lh;
				break;
			case 0b010:
				ans.inst = Lw;
				break;
			case 0b100:
				ans.inst = Lbu;
				break;
			case 0b101:
				ans.inst = Lhu;
				break;
			default:
				break;
			}
		}
		else {
			switch (funct)
			{
			case 0b000:
				ans.inst = Addi;
				break;
			case 0b010:
				ans.inst = Slti;
				break;
			case 0b011:
				ans.inst = Sltu;
				break;
			case 0b100:
				ans.inst = Xori;
				break;
			case 0b110:
				ans.inst = Ori;
				break;
			case 0b111:
				ans.inst = Andi;
				break;
			default:
			{
				int funct2 = fetch_num(instruction, 25, 31);
				if (funct == 0b001) {
					ans.inst = Slli;
				}
				else {
					if (funct2 == 0)
						ans.inst = Srli;
					else
						ans.inst = Srai;
				}
				ans.imm = fetch_num(instruction, 20, 24);
				ans.rs1 = reg.XI[fetch_num(instruction, 15, 19)];
				ans.rd = fetch_num(instruction, 7, 11);
				return ans;
			}
			}
		}
		ans.imm = fetch_num(instruction, 20, 31);
		ans.rs1 = reg.XI[fetch_num(instruction, 15, 19)];
		ans.rd = fetch_num(instruction, 7, 11);
		return ans;
	}
	RUN_DATA S_decode() {
		RUN_DATA ans;
		int op = fetch_num(instruction, 0, 6), funct = fetch_num(instruction, 12, 14);
		switch (funct)
		{
		case 0b000:
			ans.inst = Sb;
			break;
		case 0b001:
			ans.inst = Sh;
			break;
		case 0b010:
			ans.inst = Sw;
			break;
		default:
			break;
		}
		
		ans.imm = fetch_num(instruction, 7, 11) + (fetch_num(instruction, 25, 31) << 5);
		ans.rs1 = reg.XI[fetch_num(instruction, 15, 19)];
		ans.rs2 = reg.XI[fetch_num(instruction, 20, 24)];
		return ans;
	}
	RUN_DATA B_decode() {
		RUN_DATA ans;
		int op = fetch_num(instruction, 0, 6), funct = fetch_num(instruction, 12, 14);
		switch (funct)
		{
		case 0b000:
			ans.inst = Beq;
			break;
		case 0b001:
			ans.inst = Bne;
			break;
		case 0b100:
			ans.inst = Blt;
			break;
		case 0b101:
			ans.inst = Bge;
			break;
		case 0b110:
			ans.inst = Bltu;
			break;
		case 0b111:
			ans.inst = Bgeu;
			break;
		default:
			break;
		}
		
		ans.imm = (fetch_num(instruction, 7, 7) << 11) +
			(fetch_num(instruction, 8, 11) << 1) +
			(fetch_num(instruction, 25, 30) << 5) +
			(fetch_num(instruction, 31, 31) << 12);
		ans.rs1 = reg.XI[fetch_num(instruction, 15, 19)];
		ans.rs2 = reg.XI[fetch_num(instruction, 20, 24)];
		return ans;
	}
	RUN_DATA U_decode() {
		RUN_DATA ans;
		int op = fetch_num(instruction, 0, 6);
		if (op == 0b0110111)
			ans.inst = Lui;
		else
			ans.inst = Auipc;

		ans.imm = fetch_num(instruction, 12, 31) << 12;
		ans.rd = fetch_num(instruction, 7, 11);
		return ans;
	}
	RUN_DATA J_decode() {
		RUN_DATA ans;
		int op = fetch_num(instruction, 0, 6);
		if (op == 0b1101111) {
			ans.inst = Jal;
			ans.imm = (fetch_num(instruction, 12, 19) << 12) +
			(fetch_num(instruction, 20, 20) << 11) +
			(fetch_num(instruction, 21, 30) << 1) +
			(fetch_num(instruction, 31, 31) << 20);
			ans.rd = fetch_num(instruction, 7, 11);
		}
		else {
			ans.inst = Jalr;
			ans.rd = fetch_num(instruction, 7, 11);
			ans.rs1 = reg.XI[fetch_num(instruction, 15, 19)];
			ans.imm = fetch_num(instruction, 20, 31);
		}
		
		return ans;
	}
public:
	unsigned int instruction = 0u;
	RUN_DATA decode() {
		int op = instruction & 0x0000007F;
		switch (op) {
		case 0b0110011:
			return R_decode();
			break;
		case 0b0000011:case 0b0010011:
			return I_decode();
			break;
		case 0b0100011:
			return S_decode();
			break;
		case 0b1100011:
			return B_decode();
			break;
		case 0b0110111:case 0b0010111:
			return U_decode();
			break;
		case 0b1101111:case 0b1100111:
			return J_decode();
			break;
		}
	}
};
DECODE decoder;
//Fetch部分
class FETCH {
public:
	bool fetch() {
		decoder.instruction = reg.urmem(reg.PC, 4);
		reg.PC += 4u;
		return decoder.instruction != 0u;
	}
};
FETCH fetcher;

//读入文件
void read_in() {
	char temp[12] = { 0 };
	int cur_pos = 0;
	while (std::cin >> temp) {
		if (temp[0] == '@') {
			sscanf(temp + 1, "%x", &cur_pos);
		}
		else {
			sscanf(temp, "%x", &reg.Mem[cur_pos++]);
		}
	}
}

//初始化
void initialize() {
	reg.PC = 0;
	memset(reg.Mem, 0, sizeof(reg.Mem));
	memset(reg.XI, 0, sizeof(reg.XI));
}

EXECUTE executer;
MEM_ACCESS accesser;
WRITE_BACK back_writer;

int main() {
	initialize();
	read_in();
	int step = 1;
	while (true) {
#ifdef RISCV_DEBUG
		std::cout << step << " " << reg.PC;
#endif // RISCV_DEBUG

		if (!fetcher.fetch())
			break;
		auto result = decoder.decode();
		result = executer.execute(result);
		result = accesser.mem_access(result);
		back_writer.write_back(result);

#ifdef RISCV_DEBUG
		for (int i = 1; i <= 31; ++i) {
			std::cout << " " << int(reg.XI[i]);
		}
		std::cout << std::endl;
#endif // DEBUG
		++step;
		if (reg.exit_flag)
			break;
	}
	return 0;
}
