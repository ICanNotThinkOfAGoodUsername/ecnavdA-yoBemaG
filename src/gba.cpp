
#include "gba.hpp"

GameBoyAdvance::GameBoyAdvance() : cpu(*this) {
	step = false;
	trace = true;

	reset();
}

void GameBoyAdvance::reset() {
	running = false;
	log.str("");

	cpu.reset();

	if (step)
		running = true;
}

void GameBoyAdvance::run() {
	while (1) {
		if (running) {
			if (trace && (cpu.pipelineStage == 3)) {
				std::string tmp = cpu.disassemble(cpu.reg.R[15] - 8, cpu.pipelineOpcode3);
				log << fmt::format("0x{:0>8X} | 0x{:0>8X} | {}\n", cpu.reg.R[15] - 8, cpu.pipelineOpcode3, tmp.c_str());
			}
			cpu.cycle();

			if (step && (cpu.pipelineStage == 3))
				running = false;
		}
	}
}

int GameBoyAdvance::loadRom(std::filesystem::path romFilePath_, std::filesystem::path biosFilePath_) {
	running = false;
	std::ifstream romFileStream;
	romFileStream.open(romFilePath_, std::ios::binary);
	if (!romFileStream.is_open()) {
		printf("Failed to open ROM!\n");
		return -1;
	}
	romFileStream.seekg(0, std::ios::end);
	size_t romSize = romFileStream.tellg();
	romFileStream.seekg(0, std::ios::beg);
	romBuff.resize(romSize);
	romFileStream.read(reinterpret_cast<char *>(romBuff.data()), romSize);
	romFileStream.close();
	
	romBuff.resize(0x02000000);
	{ // Round rom size to power of 2 https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
		u32 v = romSize - 1;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		romSize = v + 1;
	}

	// Fill open bus
	for (int i = romSize; i < 0x2000000; i += 2) {
		//
	}

	// Fill page table for rom buffer
	for (int pageIndex = 0; pageIndex < 0x400; pageIndex++) {
		u8 *ptr = &romBuff[pageIndex << 15];
		pageTableRead[pageIndex | 0x1000] = ptr; // 0x0800'0000 - 0x09FF'FFFF
		pageTableRead[pageIndex | 0x1400] = ptr; // 0x0A00'0000 - 0x0BFF'FFFF
		pageTableRead[pageIndex | 0x1800] = ptr; // 0x0C00'0000 - 0x0DFF'FFFF
	}

	running = true;
	return 0;
}

void GameBoyAdvance::save() {
	//
}

template <typename T>
T GameBoyAdvance::read(u32 address) {
	int page = (address & 0x0FFFFFFF) >> 15;
	int offset = address & 0x7FFF;
	void *pointer = pageTableRead[page];

	if (pointer != NULL) {
		T val;
		std::memcpy (&val, (pointer + offset), sizeof(T));
		return val;
	} else {
		switch (page) {
		default:
			return 0;
		}
	}

	return 0;
}

template u16 GameBoyAdvance::read<u16>(u32);
template u32 GameBoyAdvance::read<u32>(u32);