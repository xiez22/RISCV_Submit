//By ligongzzz.
//Version 2019.07.07
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <map>
#include <unordered_map>

#define RISCV_RELEASE
#define NO_SHOW_ACCURACY

//Waiting Parameters
const uint32_t JALR_WAITING_TIME = 4u;
const uint32_t LOAD_WAITING_TIME = 2u;

//--------------------------------------Functions----------------------------------------------
uint32_t fetch_num(uint32_t instruction, uint32_t from, uint32_t to) {
	return (instruction >> from) & ((1u << (to - from + 1)) - 1u);
}

enum INST {
	Add, Sub, Xor, Or, And, Sll, Srl, Sra, Slt, Sltu, Lb, Lh, Lw, Lbu, Lhu, Addi,
	Xori, Ori, Andi, Slli, Srli, Srai, Slti, Sltiu, Sb, Sh, Sw, Beq, Bne, Blt, Bge, Bltu,
	Bgeu, Lui, Auipc, Jal, Jalr
};

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

	uint32_t rreg(uint32_t code) {
		return XI[code];
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

class RUN_DATA {
public:
	INST inst;
	uint32_t rs1 = 0u, rs2 = 0u;
	uint32_t rs1_pos = 0u, rs2_pos = 0u;
	uint32_t imm = 0u;
	uint32_t rd = 0u;
	uint32_t tmp = 0u;
	uint32_t pc = 0u;
	bool executable = false, to_inform_rs = false, estimate_result = false;
};

//Global Running Controller
class LEADER {
	uint32_t remain_time = 0;
	bool BAD_ESTIMATE = false;
	//To calculate the rate.
	uint32_t right_cnt = 0, total_cnt = 0;
	//To store the history of branch.
	std::unordered_map<uint32_t, uint8_t> history;
public:
	bool able_to_fetch() {
		return remain_time == 0;
	}
	void set_lock(uint32_t waiting_time) {
		remain_time = waiting_time;
	}
	void set_bad_flag(const RUN_DATA& run_data) {
		BAD_ESTIMATE = true;
		//2-Bits Estimate.
		auto iter = history.find(run_data.pc);
		if (iter->second == 0b00)
			iter->second = 0b01;
		else if (iter->second == 0b01)
			iter->second = 0b10;
		else if (iter->second == 0b10)
			iter->second = 0b01;
		else
			iter->second = 0b10;
	}
	void estimate_success(const RUN_DATA& run_data) {
		++right_cnt;
		//2-Bits Estimate.
		auto iter = history.find(run_data.pc);
		if (iter->second == 0b00)
			iter->second = 0b00;
		else if (iter->second == 0b01)
			iter->second = 0b00;
		else if (iter->second == 0b10)
			iter->second = 0b11;
		else
			iter->second = 0b11;
	}
	bool able_to_continue() {
		bool return_val = !BAD_ESTIMATE;
		if (return_val) {
			;
		}
#ifdef RISCV_DEBUG
		else {
			std::cerr << "Bad Estimation!" << std::endl;
		}
#endif // RISCV_DEBUG	
		BAD_ESTIMATE = false;
		return return_val;
	}
	void update() {
		if (remain_time > 0)
			--remain_time;
	}
	bool estimate(const RUN_DATA& run_data) {
		++total_cnt;
		//2-Bits Estimate.
		auto iter = history.find(run_data.pc);
		if (iter == history.end()) {
			if (int32_t(run_data.imm) < 0) {
				history.insert(std::make_pair(run_data.pc, 0b11));
				return true;
			}
			else {
				history.insert(std::make_pair(run_data.pc, 0b00));
				return false;
			}
			history.insert(std::make_pair(run_data.pc, 0b11));
			return true;
		}
		return iter->second >= 0b10;
	}
	double accuracy_query() {
		return double(right_cnt) / double(total_cnt);
	}
};
LEADER leader;
//---------------------------------------Execution Part----------------------------------------------
class EXECUTE {
	//INFORMATION DATA
	std::map<uint32_t, uint32_t> inform_data;
public:
	void inform(const RUN_DATA& information) {
		if (information.to_inform_rs && information.rd != 0) {
			if (inform_data.find(information.rd) == inform_data.end())
				inform_data.insert(std::make_pair(information.rd, information.tmp));
		}
	}

	RUN_DATA execute(RUN_DATA result) {
		if (!result.executable) {
			inform_data.clear();
			return result;
		}
		//Choose the right rs value.
		auto iter = inform_data.find(result.rs1_pos);
		if (iter != inform_data.end())
			result.rs1 = iter->second;
		iter = inform_data.find(result.rs2_pos);
		if (iter != inform_data.end())
			result.rs2 = iter->second;
		inform_data.clear();

		switch (result.inst)
		{
		case Add:
			result.tmp = result.rs1 + result.rs2;
			result.to_inform_rs = true;
			break;
		case Sub:
			result.tmp = result.rs1 - result.rs2;
			result.to_inform_rs = true;
			break;
		case Xor:
			result.tmp = result.rs1 ^ result.rs2;
			result.to_inform_rs = true;
			break;
		case Or:
			result.tmp = result.rs1 | result.rs2;
			result.to_inform_rs = true;
			break;
		case And:
			result.tmp = result.rs1 & result.rs2;
			result.to_inform_rs = true;
			break;
		case Sll:
			result.tmp = result.rs1 << result.rs2;
			result.to_inform_rs = true;
			break;
		case Srl:
			result.tmp = result.rs1 >> result.rs2;
			result.to_inform_rs = true;
			break;
		case Sra:
			result.tmp = (int32_t(result.rs1)) >> result.rs2;
			result.to_inform_rs = true;
			break;
		case Slt:
			result.tmp = uint32_t(((int32_t)result.rs1) < ((int32_t)result.rs2));
			result.to_inform_rs = true;
			break;
		case Sltu:
			result.tmp = uint32_t(((uint32_t)result.rs1) < ((uint32_t)result.rs2));
			result.to_inform_rs = true;
			break;
		case Addi:
			result.tmp = result.rs1 + ((int32_t(result.imm)) << 20 >> 20);
			result.to_inform_rs = true;
			break;
		case Xori:
			result.tmp = result.rs1 ^ ((int32_t(result.imm)) << 20 >> 20);
			result.to_inform_rs = true;
			break;
		case Ori:
			result.tmp = result.rs1 | ((int32_t(result.imm)) << 20 >> 20);
			result.to_inform_rs = true;
			break;
		case Andi:
			result.tmp = result.rs1 & ((int32_t(result.imm)) << 20 >> 20);
			result.to_inform_rs = true;
			break;
		case Slli:
			result.tmp = result.rs1 << result.imm;
			result.to_inform_rs = true;
			break;
		case Srli:
			result.tmp = result.rs1 >> result.imm;
			result.to_inform_rs = true;
			break;
		case Srai:
			result.tmp = (int32_t(result.rs1)) >> result.imm;
			result.to_inform_rs = true;
			break;
		case Slti:
			result.tmp = uint32_t(((int32_t)result.rs1) < (((int32_t(result.imm)) << 20) >> 20));
			result.to_inform_rs = true;
			break;
		case Sltiu:
			result.tmp = uint32_t(uint32_t(result.rs1) < uint32_t(result.imm));
			result.to_inform_rs = true;
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
			if (result.rs1 == result.rs2) {
				result.tmp = true;
			}
			break;
		case Bne:
			if (result.rs1 != result.rs2) {
				result.tmp = true;
			}
			break;
		case Blt:
			if (int32_t(result.rs1) < int32_t(result.rs2)) {
				result.tmp = true;
			}
			break;
		case Bge:
			if (int32_t(result.rs1) >= int32_t(result.rs2)) {
				result.tmp = true;
			}
			break;
		case Bltu:
			if (result.rs1 < result.rs2) {
				result.tmp = true;
			}
			break;
		case Bgeu:
			if (result.rs1 >= result.rs2) {
				result.tmp = true;
			}
			break;
		case Lui:
			result.tmp = result.imm;
			result.to_inform_rs = true;
			break;
		case Auipc:
			//Hazard Unsolved
			reg.PC = result.tmp = result.pc + result.imm;
			result.to_inform_rs = true;
			break;
		case Jal:
			result.tmp = result.pc + 4;
			result.to_inform_rs = true;
			break;
		case Jalr:
			result.tmp = result.pc + 4;
			reg.PC = result.rs1 + ((((int32_t)result.imm) << 20) >> 20);
			reg.PC = reg.PC >> 1 << 1;
			result.to_inform_rs = true;
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
		if (!result.executable)
			return result;
		switch (result.inst)
		{
		case Lb:
			result.tmp = (uint32_t)reg.rmem(result.rs1 + (((int32_t(result.imm)) << 20) >> 20), 1);
			result.to_inform_rs = true;
			break;
		case Lh:
			result.tmp = (uint32_t)reg.rmem(result.rs1 + (((int32_t(result.imm)) << 20) >> 20), 2);
			result.to_inform_rs = true;
			break;
		case Lw:
			result.tmp = (uint32_t)reg.rmem(result.rs1 + ((int32_t(result.imm)) << 20 >> 20), 4);
			result.to_inform_rs = true;
			break;
		case Lbu:
			result.tmp = reg.urmem(result.rs1 + (((int32_t(result.imm)) << 20) >> 20), 1);
			result.to_inform_rs = true;
			break;
		case Lhu:
			result.tmp = reg.urmem(result.rs1 + (((int32_t(result.imm)) << 20) >> 20), 2);
			result.to_inform_rs = true;
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
		if (!result.executable)
			return result;
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
		case Lui:
		case Auipc:
			reg.wreg(result.rd, result.tmp);
			break;
		case Beq:
		case Bne:
		case Bge:
		case Bgeu:
		case Blt:
		case Bltu:
			if (result.tmp != result.estimate_result) {
				leader.set_bad_flag(result);
				if (result.tmp)
					reg.PC = result.pc + (((int32_t)result.imm) << 20 >> 20);
				else
					reg.PC = result.pc + 4u;
			}
			else
				leader.estimate_success(result);
			break;
		default:
			break;
		}
		return result;
	}
};

//---------------------------------------Decode Part----------------------------------------------
//Decode Part
class DECODE {
public:
	RUN_DATA decode(RUN_DATA result) {
		if (!result.executable) {
			RUN_DATA null_result;
			return null_result;
		}
		result.rs1 = reg.rreg(result.rs1_pos);
		result.rs2 = reg.rreg(result.rs2_pos);
		return result;
	}
};
DECODE decoder;
//Fetch Part
class FETCH {
	RUN_DATA R_decode() {
		RUN_DATA ans;
		ans.pc = reg.PC;
		ans.executable = true;
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

		ans.rs1_pos = fetch_num(instruction, 15, 19);
		ans.rs2_pos = fetch_num(instruction, 20, 24);
		ans.rd = fetch_num(instruction, 7, 11);
		return ans;
	}
	RUN_DATA I_decode() {
		RUN_DATA ans;
		ans.pc = reg.PC;
		ans.executable = true;
		int op = fetch_num(instruction, 0, 6), funct = fetch_num(instruction, 12, 14);
		if (op == 3) {
			switch (funct)
			{
			case 0b000:
				ans.inst = Lb;
				//Hazard Instruction. Special Operation.	
				leader.set_lock(LOAD_WAITING_TIME);
				break;
			case 0b001:
				ans.inst = Lh;
				//Hazard Instruction. Special Operation.	
				leader.set_lock(LOAD_WAITING_TIME);
				break;
			case 0b010:
				ans.inst = Lw;
				//Hazard Instruction. Special Operation.	
				leader.set_lock(LOAD_WAITING_TIME);
				break;
			case 0b100:
				ans.inst = Lbu;
				//Hazard Instruction. Special Operation.	
				leader.set_lock(LOAD_WAITING_TIME);
				break;
			case 0b101:
				ans.inst = Lhu;
				//Hazard Instruction. Special Operation.	
				leader.set_lock(LOAD_WAITING_TIME);
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
				ans.rs1_pos = fetch_num(instruction, 15, 19);
				ans.rd = fetch_num(instruction, 7, 11);
				return ans;
			}
			}
		}
		ans.imm = fetch_num(instruction, 20, 31);
		ans.rs1_pos = fetch_num(instruction, 15, 19);
		ans.rd = fetch_num(instruction, 7, 11);
		return ans;
	}
	RUN_DATA S_decode() {
		RUN_DATA ans;
		ans.pc = reg.PC;
		ans.executable = true;
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
		ans.rs1_pos = fetch_num(instruction, 15, 19);
		ans.rs2_pos = fetch_num(instruction, 20, 24);

		return ans;
	}
	RUN_DATA B_decode() {
		RUN_DATA ans;
		ans.pc = reg.PC;
		ans.executable = true;
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
		ans.rs1_pos = fetch_num(instruction, 15, 19);
		ans.rs2_pos = fetch_num(instruction, 20, 24);
		//Hazard Instruction. Special Operation.
		if (leader.estimate(ans)) {
			reg.PC = reg.PC += (((int32_t)ans.imm) << 20 >> 20) - 4;
			ans.estimate_result = true;
		}
		return ans;
	}
	RUN_DATA U_decode() {
		RUN_DATA ans;
		ans.pc = reg.PC;
		ans.executable = true;
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
		ans.pc = reg.PC;
		ans.executable = true;
		int op = fetch_num(instruction, 0, 6);
		if (op == 0b1101111) {
			ans.inst = Jal;
			ans.imm = (fetch_num(instruction, 12, 19) << 12) +
				(fetch_num(instruction, 20, 20) << 11) +
				(fetch_num(instruction, 21, 30) << 1) +
				(fetch_num(instruction, 31, 31) << 20);
			ans.rd = fetch_num(instruction, 7, 11);

			//Hazard Instruction. Special Operation.	
			reg.PC = ((((int32_t)ans.imm) << 11) >> 11) + ans.pc - 4u;
		}
		else {
			ans.inst = Jalr;
			ans.rd = fetch_num(instruction, 7, 11);
			ans.rs1_pos = fetch_num(instruction, 15, 19);
			ans.imm = fetch_num(instruction, 20, 31);

			//Hazard Instruction. Special Operation.
			leader.set_lock(JALR_WAITING_TIME);
		}

		return ans;
	}
	RUN_DATA decode() {
		if (!instruction) {
			RUN_DATA null_result;
			return null_result;
		}
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
public:
	uint32_t instruction = 0;
	RUN_DATA fetch() {
		instruction = reg.urmem(reg.PC, 4);
		auto result = decode();
		if (instruction) {
			reg.PC += 4u;
		}
		return result;
	}
};
FETCH fetcher;

//---------------------------------------Other Part----------------------------------------------
//Read In File
void read_in() {
	char temp[12] = { 0 };
	int cur_pos = 0;
	while (std::cin >> temp) {
		if (temp[0] == '@') {
			sscanf(temp + 1, "%x", &cur_pos);
		}
		else {
			sscanf(temp, "%hhx", &reg.Mem[cur_pos++]);
		}
	}
}

//Initialize
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

	RUN_DATA fetch_result, decode_result, execute_result, mem_result;
	while (true) {
		RUN_DATA fetch_temp;
		if (leader.able_to_fetch())
			fetch_temp = fetcher.fetch();
		back_writer.write_back(mem_result);
		auto decode_temp = decoder.decode(fetch_result);
		auto execute_temp = executer.execute(decode_result);
		executer.inform(execute_temp);
		auto mem_temp = accesser.mem_access(execute_result);
		executer.inform(mem_temp);

		leader.update();
#ifdef RISCV_DEBUG
		if (mem_result.executable) {
			std::cout << step << " " << mem_result.pc;
			for (int i = 1; i <= 31; ++i)
				std::cout << " " << int32_t(reg.XI[i]);
			std::cout << std::endl;
			++step;
		}
#endif // DEBUG
		//UPDATE
		//Check if bad estimate.
		if (leader.able_to_continue())
			fetch_result = fetch_temp, decode_result = decode_temp,
			execute_result = execute_temp, mem_result = mem_temp;
		else {
			RUN_DATA null_data;
			fetch_result = null_data, decode_result = null_data,
				execute_result = null_data, mem_result = null_data;
		}

		if (reg.exit_flag)
			break;
	}

#ifdef SHOW_ACCURACY
	std::cout << leader.accuracy_query() << std::endl;
#endif // SHOW_ACCURACY

	return 0;
}
