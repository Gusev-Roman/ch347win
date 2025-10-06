#include "CH347DLL_EN.H"

int loader_init(ULONG id) {
	HANDLE hh = CH347OpenDevice(0);

	if (hh != INVALID_HANDLE_VALUE) {
		CH347CloseDevice(id);
		return 1;
	}
	return 0;
}
