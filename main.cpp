#define NOMINMAX
#include <windows.h>
#include "battleship.h"


int WINAPI wWinMain(HINSTANCE instance,
	HINSTANCE /*prevInstance*/,
	LPWSTR /*command_line*/,
	int show_command)
{
	battleship app{ instance };
	return app.run(show_command);

}
