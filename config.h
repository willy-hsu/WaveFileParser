#ifndef _H_CONFIG_
#define _H_CONFIG_

/**
 * @brief 
 * some config for setting
 * SRC_FIX_ME : not sure if the code is right, need further check
 * USEDOUBLES : likely to be bit-exact between machines
 * PRINT_EN : enable log
 */
#define SRC_FIX_ME (1)
#define USEDOUBLES (1)
#define PRINT_EN (0)

#if (PRINT_EN == 0)
#define printf(...)
#endif

#endif
