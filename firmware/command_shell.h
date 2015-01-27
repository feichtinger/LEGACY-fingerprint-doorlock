
#ifndef COMMAND_SHELL_H
#define COMMAND_SHELL_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

void shell_feedChar(char ch);
void shell_scan();

#ifdef __cplusplus
}
#endif

#endif /* COMMAND_SHELL_H */
