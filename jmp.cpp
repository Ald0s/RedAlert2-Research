#include "jmp.h"

namespace AldosV {
	CJmpHook::CJmpHook() {
		payload[0] = 0xE9; // JMP instruction.

		payload[1] = 0x0;
		payload[2] = 0x0;
		payload[3] = 0x0;
		payload[4] = 0x0; // Four empty bytes for the target address.

		payload[5] = 0xC3; // RET instruction.
	}

	CJmpHook::~CJmpHook() {

	}

	void CJmpHook::Hook(void* target_memory, void* callback) {
		this->target = target_memory;

		uintptr_t distance = (uintptr_t)callback - (uintptr_t)target_memory - 5;
		memcpy(&payload[1], &distance, 4);
		
		DWORD old;
		VirtualProtect(target_memory, PAYLOADSZ, PAGE_EXECUTE_READWRITE, &old);

		memcpy(original, target_memory, PAYLOADSZ);
		memcpy(target_memory, payload, PAYLOADSZ);

		VirtualProtect(target_memory, PAYLOADSZ, old, 0);
	}

	void* CJmpHook::GetOriginal() {
		DWORD old;
		VirtualProtect(target, PAYLOADSZ, PAGE_EXECUTE_READWRITE, &old);
		memcpy(target, original, PAYLOADSZ);
		VirtualProtect(target, PAYLOADSZ, old, 0);

		return target;
	}

	void CJmpHook::Reset() {
		DWORD old;
		VirtualProtect(target, PAYLOADSZ, PAGE_EXECUTE_READWRITE, &old);
		memcpy(target, payload, PAYLOADSZ);
		VirtualProtect(target, PAYLOADSZ, old, 0);
	}

	void CJmpHook::Remove() {
		DWORD old;
		VirtualProtect(target, PAYLOADSZ, PAGE_EXECUTE_READWRITE, &old);
		memcpy(target, original, PAYLOADSZ);
		VirtualProtect(target, PAYLOADSZ, old, 0);
	}
}