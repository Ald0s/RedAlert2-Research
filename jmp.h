#pragma once
#include <Windows.h>
#include <iostream>

namespace AldosV {
#define PAYLOADSZ 6

	class CJmpHook
	{
	public:
		CJmpHook();
		~CJmpHook();

		void Hook(void* target_memory, void* callback);
		void* GetOriginal();
		void Reset();
		void Remove();

	private:
		char payload[PAYLOADSZ] = { 0 };
		char original[PAYLOADSZ] = { 0 };

		void* target;
	};
}
