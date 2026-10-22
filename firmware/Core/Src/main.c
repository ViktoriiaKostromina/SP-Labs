/* ShiftToken lab2 harness. ST HAL startup remains in Core/ and Drivers/. */
#include "main.h"
#include "board.h"
#include "bringup.h"
int main(void)
{
    HAL_Init();
    st_board_init();
    st_bringup_init(st_board_platform());
    for(;;)st_bringup_tick();
}
