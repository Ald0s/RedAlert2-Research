#pragma once
#include <Windows.h>
#include <iostream>

namespace AldosV {
	struct MoneyScan_t {
		int staticVar1;
		int staticVar2;
		int staticPtrVar;
	};

	class CCredits
	{
	public:
		CCredits();
		~CCredits();

		void Setup(HMODULE base);

		MoneyScan_t GetCreditState();
		void SetCredit(int newValue);

	private:
		intptr_t base;

		intptr_t var1 = 0x4373cc,
			var2 = 0x4373d0,
			var3_base = 0x635db4;

		intptr_t CalculateVar3();

		int ReadInt(intptr_t address);
		void WriteInt(intptr_t address, int value);
	};
}