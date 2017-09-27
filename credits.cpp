#include "credits.h"

namespace AldosV {
	CCredits::CCredits() {

	}

	CCredits::~CCredits() {

	}

	void CCredits::Setup(HMODULE base) {
		this->base = (intptr_t)base;
	}

	MoneyScan_t CCredits::GetCreditState() {
		MoneyScan_t scan;

		scan.staticVar1 = ReadInt(base + var1);
		scan.staticVar2 = ReadInt(base + var2);
		scan.staticPtrVar = (CalculateVar3()) ? ReadInt(CalculateVar3()) : 0;

		return scan;
	}

	void CCredits::SetCredit(int newValue) {
		WriteInt(base + var1, newValue);
		WriteInt(base + var2, newValue);
		if (CalculateVar3())
			WriteInt(CalculateVar3(), newValue);
	}

	// This functions returns the address (one dereference away) from the actual value.
	intptr_t CCredits::CalculateVar3() {
		// ptr will be nullptr if we're not ingame. This is a cool way to check for ingame status.
		intptr_t ptr = (intptr_t)(ReadInt(base + var3_base));
		if (ptr) {
			ptr += 0x24c;
			return ptr;
		}
		return 0;
	}

	int CCredits::ReadInt(intptr_t address) {
		int result = 0;
		SIZE_T num_read;

		ReadProcessMemory(GetCurrentProcess(),
			(void*)address,
			&result,
			sizeof(int),
			&num_read);

		return result;
	}

	void CCredits::WriteInt(intptr_t address, int value) {
		SIZE_T written = 0;

		WriteProcessMemory(GetCurrentProcess(),
			(void*)address,
			&value,
			sizeof(int),
			&written);
	}
}